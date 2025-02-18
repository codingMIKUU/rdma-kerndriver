/*
 * Copyright (c) 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2005 Cisco Systems.  All rights reserved.
 * Copyright (c) 2005 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2020 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/export.h>
#include <linux/slab.h>
#ifdef HAVE_BASECODE_EXTRAS
#include <linux/scatterlist.h>
#endif
#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
#include <linux/hugetlb.h>
#endif
#include <linux/pagemap.h>
#include <linux/count_zeros.h>
#ifdef CONFIG_INFINIBAND_ON_DEMAND_PAGING
#include <rdma/ib_umem_odp.h>
#endif

#include "uverbs.h"

#include "ib_peer_mem.h"

static void __ib_umem_release(struct ib_device *dev, struct ib_umem *umem, int dirty)
{
#ifdef HAVE_UNPIN_USER_PAGE_RANGE_DIRTY_LOCK_EXPORTED
	bool make_dirty = umem->writable && dirty;
	struct scatterlist *sg;
	unsigned int i;
#else
	struct sg_page_iter sg_iter;
	struct page *page;
#endif

#ifdef HAVE_SG_APPEND_TABLE
	if (dirty)
		ib_dma_unmap_sgtable_attrs(dev, &umem->sgt_append.sgt,
					   DMA_BIDIRECTIONAL, 0);
#else
	if (umem->nmap > 0)
		ib_dma_unmap_sg(dev, umem->sg_head.sgl, umem->sg_nents,
				DMA_BIDIRECTIONAL);
#endif

#ifdef HAVE_UNPIN_USER_PAGE_RANGE_DIRTY_LOCK_EXPORTED
#ifdef HAVE_SG_APPEND_TABLE
	for_each_sgtable_sg(&umem->sgt_append.sgt, sg, i)
#else
	for_each_sg(umem->sg_head.sgl, sg, umem->sg_nents, i)
#endif
		unpin_user_page_range_dirty_lock(sg_page(sg),
				DIV_ROUND_UP(sg->length, PAGE_SIZE), make_dirty);
#else
	for_each_sg_page(umem->sg_head.sgl, &sg_iter, umem->sg_nents, 0) {
		page = sg_page_iter_page(&sg_iter);
#ifdef HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED
		unpin_user_pages_dirty_lock(&page, 1, umem->writable && dirty);
#elif defined(HAVE_PUT_USER_PAGES_DIRTY_LOCK_3_PARAMS)
		put_user_pages_dirty_lock(&page, 1, umem->writable && dirty);
#elif defined(HAVE_PUT_USER_PAGES_DIRTY_LOCK_2_PARAMS)
		if (umem->writable && dirty)
			put_user_pages_dirty_lock(&page, 1);
		else
			put_user_page(page);
#else
		if (!PageDirty(page) && umem->writable && dirty)
			set_page_dirty_lock(page);
		put_page(page);
#endif /*HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED*/
	}
#endif /*HAVE_UNPIN_USER_PAGE_RANGE_DIRTY_LOCK_EXPORTED*/

#ifdef HAVE_SG_APPEND_TABLE
	sg_free_append_table(&umem->sgt_append);
#else
	sg_free_table(&umem->sg_head);
#endif /*HAVE_SG_APPEND_TABLE*/
}

/**
 * ib_umem_find_best_pgsz - Find best HW page size to use for this MR
 *
 * @umem: umem struct
 * @pgsz_bitmap: bitmap of HW supported page sizes
 * @virt: IOVA
 *
 * This helper is intended for HW that support multiple page
 * sizes but can do only a single page size in an MR.
 *
 * Returns 0 if the umem requires page sizes not supported by
 * the driver to be mapped. Drivers always supporting PAGE_SIZE
 * or smaller will never see a 0 result.
 */
unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem,
				     unsigned long pgsz_bitmap,
				     unsigned long virt)
{
	struct scatterlist *sg;
	unsigned long va, pgoff;
	dma_addr_t mask;
	int i;

	umem->iova = va = virt;

#ifdef CONFIG_INFINIBAND_ON_DEMAND_PAGING
	if (umem->is_odp) {
		unsigned int page_size = BIT(to_ib_umem_odp(umem)->page_shift);

		/* ODP must always be self consistent. */
		if (!(pgsz_bitmap & page_size))
			return 0;
		return page_size;
	}
#endif

	/* The best result is the smallest page size that results in the minimum
	 * number of required pages. Compute the largest page size that could
	 * work based on VA address bits that don't change.
	 */
	mask = pgsz_bitmap &
	       GENMASK(BITS_PER_LONG - 1,
		       bits_per((umem->length - 1 + virt) ^ virt));
	/* offset into first SGL */
	pgoff = umem->address & ~PAGE_MASK;

#ifdef HAVE_SG_APPEND_TABLE
	for_each_sgtable_dma_sg(&umem->sgt_append.sgt, sg, i) {
#else
	for_each_sg(umem->sg_head.sgl, sg, umem->nmap, i) {
#endif

		/* Walk SGL and reduce max page size if VA/PA bits differ
		 * for any address.
		 */
		mask |= (sg_dma_address(sg) + pgoff) ^ va;
		va += sg_dma_len(sg) - pgoff;
		/* Except for the last entry, the ending iova alignment sets
		 * the maximum possible page size as the low bits of the iova
		 * must be zero when starting the next chunk.
		 */
#ifdef HAVE_SG_APPEND_TABLE
		if (i != (umem->sgt_append.sgt.nents - 1))
#else
		if (i != (umem->nmap - 1))
#endif
			mask |= va;
		pgoff = 0;
	}

	/* The mask accumulates 1's in each position where the VA and physical
	 * address differ, thus the length of trailing 0 is the largest page
	 * size that can pass the VA through to the physical.
	 */
	if (mask)
		pgsz_bitmap &= GENMASK(count_trailing_zeros(mask), 0);
	return pgsz_bitmap ? rounddown_pow_of_two(pgsz_bitmap) : 0;
}
EXPORT_SYMBOL(ib_umem_find_best_pgsz);

#if !defined( HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS) && !defined(HAVE_SG_APPEND_TABLE)
static struct scatterlist *ib_umem_add_sg_table(struct scatterlist *sg,
		struct page **page_list,
		unsigned long npages,
		unsigned int max_seg_sz,
		int *nents)
{
	unsigned long first_pfn;
	unsigned long i = 0;
	bool update_cur_sg = false;
	bool first = !sg_page(sg);

	/* Check if new page_list is contiguous with end of previous page_list.
	 *          * sg->length here is a multiple of PAGE_SIZE and sg->offset is 0.
	 *                   */
	if (!first && (page_to_pfn(sg_page(sg)) + (sg->length >> PAGE_SHIFT) ==
				page_to_pfn(page_list[0])))
		update_cur_sg = true;

	while (i != npages) {
		unsigned long len;
		struct page *first_page = page_list[i];

		first_pfn = page_to_pfn(first_page);

		/* Compute the number of contiguous pages we have starting
		 *                  * at i
		 *                                   */
		for (len = 0; i != npages &&
				first_pfn + len == page_to_pfn(page_list[i]) &&
				len < (max_seg_sz >> PAGE_SHIFT);
				len++)
			i++;

		/* Squash N contiguous pages from page_list into current sge */
		if (update_cur_sg) {
			if ((max_seg_sz - sg->length) >= (len << PAGE_SHIFT)) {
				sg_set_page(sg, sg_page(sg),
						sg->length + (len << PAGE_SHIFT),
						0);
				update_cur_sg = false;
				continue;
			}
			update_cur_sg = false;
		}
		/* Squash N contiguous pages into next sge or first sge */
		if (!first)
			sg = sg_next(sg);

		(*nents)++;
		sg_set_page(sg, first_page, len << PAGE_SHIFT, 0);
		first = false;
	}

	return sg;
}
#endif

/**
 * __ib_umem_get - Pin and DMA map userspace memory.
 *
 * @device: IB device to connect UMEM
 * @addr: userspace virtual address to start at
 * @size: length of region to pin
 * @access: IB_ACCESS_xxx flags for memory being pinned
 * @peer_mem_flags: IB_PEER_MEM_xxx flags for memory being used
 */
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
static struct ib_umem *__ib_umem_get(struct ib_device *device,
#else
struct ib_umem *__ib_umem_get(struct ib_udata *udata,
#endif
				    unsigned long addr, size_t size, int access,
				    unsigned long peer_mem_flags)
{
#ifndef HAVE_MMU_INTERVAL_NOTIFIER
        struct ib_ucontext *context;
#endif
	struct ib_umem *umem;
	struct page **page_list;
	unsigned long lock_limit;
#if defined(HAVE_PINNED_VM) || defined(HAVE_ATOMIC_PINNED_VM)
	unsigned long new_pinned;
#endif
	unsigned long cur_base;
	unsigned long dma_attr = 0;
	struct mm_struct *mm;
	unsigned long npages;
#ifdef HAVE_SG_APPEND_TABLE
	int pinned, ret;
#else
	int ret;
	struct scatterlist *sg = NULL;
#endif /*HAVE_SG_APPEND_TABLE*/
#ifdef HAVE_GET_USER_PAGES_GUP_FLAGS
#ifdef HAVE_GUP_MUST_UNSHARE_GET_3_PARAMS
	unsigned int gup_flags = FOLL_LONGTERM;
#else
	unsigned int gup_flags = FOLL_WRITE;
#endif
#endif
#if defined(HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS) && (!defined(HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED) && !defined(HAVE_PUT_USER_PAGES_DIRTY_LOCK_3_PARAMS))
	unsigned long index;
#endif
#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
	struct vm_area_struct **vma_list;
	int i;
#endif
#ifdef DMA_ATTR_WRITE_BARRIER
        unsigned long dma_attrs = 0;
#endif //DMA_ATTR_WRITE_BARRIER

#ifdef DMA_ATTR_WRITE_BARRIER
	dma_attrs |= DMA_ATTR_WRITE_BARRIER;
#endif //DMA_ATTR_WRITE_BARRIER

#ifndef HAVE_MMU_INTERVAL_NOTIFIER
	if (!udata)
		return ERR_PTR(-EIO);

	context = container_of(udata, struct uverbs_attr_bundle, driver_udata)
			  ->context;
	if (!context)
		return ERR_PTR(-EIO);
#endif

	/*
	 * If the combination of the addr and size requested for this memory
	 * region causes an integer overflow, return error.
	 */
	if (((addr + size) < addr) ||
	    PAGE_ALIGN(addr + size) < (addr + size)) {
		pr_err("%s: integer overflow, size=%zu\n", __func__, size);
 		return ERR_PTR(-EINVAL);
	}

	if (!can_do_mlock()) {
		pr_err("%s: no mlock permission\n", __func__);
 		return ERR_PTR(-EPERM);
	}

	if (access & IB_ACCESS_ON_DEMAND)
		return ERR_PTR(-EOPNOTSUPP);

	umem = kzalloc(sizeof(*umem), GFP_KERNEL);
	if (!umem)
		return ERR_PTR(-ENOMEM);
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
	umem->ibdev      = device;
#else
#ifdef HAVE_MMU_NOTIFIER_OPS_HAS_FREE_NOTIFIER
	umem->ibdev = context->device;
#else
	umem->context = context;
#endif
#endif
	umem->length     = size;
	umem->address    = addr;
	/*
	 * Drivers should call ib_umem_find_best_pgsz() to set the iova
	 * correctly.
	 */
	umem->iova = addr;
	umem->writable   = ib_access_writable(access);
	umem->owning_mm = mm = current->mm;
	mmgrab(mm);

#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
	/* We assume the memory is from hugetlb until proved otherwise */
	umem->hugetlb   = 1;
#endif
	page_list = (struct page **) __get_free_page(GFP_KERNEL);
	if (!page_list) {
		ret = -ENOMEM;
		goto umem_kfree;
	}
#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
	/*
	 *       * if we can't alloc the vma_list, it's not so bad;
	 *                 * just assume the memory is not hugetlb memory
	 *                 */
	vma_list = (struct vm_area_struct **) __get_free_page(GFP_KERNEL);
	if (!vma_list)
		umem->hugetlb = 0;
#endif
	npages = ib_umem_num_pages(umem);
	if (npages == 0 || npages > UINT_MAX) {
		ret = -EINVAL;
		goto out;
	}

	lock_limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

#ifdef HAVE_ATOMIC_PINNED_VM
	new_pinned = atomic64_add_return(npages, &mm->pinned_vm);
	if (new_pinned > lock_limit && !capable(CAP_IPC_LOCK)) {
#else
	down_write(&mm->mmap_sem);
#ifdef HAVE_PINNED_VM
	if (check_add_overflow(mm->pinned_vm, npages, &new_pinned) ||
	    (new_pinned > lock_limit && !capable(CAP_IPC_LOCK))) {
#else
	current->mm->locked_vm += npages;
	if ((current->mm->locked_vm > lock_limit) && !capable(CAP_IPC_LOCK)) {
#endif /* HAVE_PINNED_VM */
#endif /* HAVE_ATOMIC_PINNED_VM */

#ifdef HAVE_ATOMIC_PINNED_VM
		atomic64_sub(npages, &mm->pinned_vm);
#else
		up_write(&mm->mmap_sem);
#ifndef HAVE_PINNED_VM
		current->mm->locked_vm -= npages;
#endif /* HAVE_PINNED_VM */
#endif /* HAVE_ATOMIC_PINNED_VM */
		ret = -ENOMEM;
		goto out;
	}
#ifndef HAVE_ATOMIC_PINNED_VM
#ifdef HAVE_PINNED_VM
	mm->pinned_vm = new_pinned;
#endif /* HAVE_PINNED_VM */
	up_write(&mm->mmap_sem);
#endif /* HAVE_ATOMIC_PINNED_VM */

	cur_base = addr & PAGE_MASK;

#if !defined( HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS) && !defined(HAVE_SG_APPEND_TABLE)
	ret = sg_alloc_table(&umem->sg_head, npages, GFP_KERNEL);
	if (ret)
		goto vma;
#endif
#ifdef HAVE_GET_USER_PAGES_GUP_FLAGS
#ifdef HAVE_GUP_MUST_UNSHARE_GET_3_PARAMS
	if (umem->writable)
		gup_flags |= FOLL_WRITE;
#else
	if (!umem->writable)
		gup_flags |= FOLL_FORCE;
#endif /* HAVE_GUP_MUST_UNSHARE_GET_3_PARAMS */
#endif
#if !defined(HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS) && !defined(HAVE_SG_APPEND_TABLE)
	sg = umem->sg_head.sgl;
#endif

#ifdef HAVE_SG_APPEND_TABLE
	while (npages) {
		cond_resched();
		pinned = pin_user_pages_fast(cur_base,
					  min_t(unsigned long, npages,
						PAGE_SIZE /
						sizeof(struct page *)),
#ifdef HAVE_GUP_MUST_UNSHARE_GET_3_PARAMS
					  gup_flags, page_list);
#else
					  gup_flags | FOLL_LONGTERM, page_list);
#endif
		if (pinned < 0) {
			ret = pinned;
			pr_debug("%s: failed to get user pages, nr_pages=%lu, flags=%u\n", __func__,
					min_t(unsigned long, npages,
						PAGE_SIZE / sizeof(struct page *)),
#ifdef HAVE_GUP_MUST_UNSHARE_GET_3_PARAMS
					gup_flags);
#else
					gup_flags | FOLL_LONGTERM);
#endif
			goto umem_release;
		}

		cur_base += pinned * PAGE_SIZE;
		npages -= pinned;
		ret = sg_alloc_append_table_from_pages(
			&umem->sgt_append, page_list, pinned, 0,
			pinned << PAGE_SHIFT, ib_dma_max_seg_size(device),
			npages, GFP_KERNEL);
		if (ret) {
			unpin_user_pages_dirty_lock(page_list, pinned, 0);
			goto umem_release;
		}
	}

	if (access & IB_ACCESS_RELAXED_ORDERING)
		dma_attr |= DMA_ATTR_WEAK_ORDERING;

	ret = ib_dma_map_sgtable_attrs(device, &umem->sgt_append.sgt,
				       DMA_BIDIRECTIONAL, dma_attr);
	if (ret) {
		pr_err("%s: failed to map scatterlist, npages=%lu\n", __func__,
		       npages);
		goto umem_release;
	}
	goto out;

#else /*HAVE_SG_APPEND_TABLE*/
	while (npages) {
		cond_resched();
#ifdef HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED
		ret = pin_user_pages_fast(cur_base,
					  min_t(unsigned long, npages,
						PAGE_SIZE /
						sizeof(struct page *)),
					  gup_flags | FOLL_LONGTERM, page_list);
		if (ret < 0)
			goto umem_release;
#else
		down_read(&mm->mmap_sem);
#ifdef HAVE_FOLL_LONGTERM
		ret = get_user_pages(cur_base,
				     min_t(unsigned long, npages,
					   PAGE_SIZE / sizeof (struct page *)),
				     gup_flags | FOLL_LONGTERM,
				     page_list, NULL);
#elif defined(HAVE_GET_USER_PAGES_LONGTERM)
		ret = get_user_pages_longterm(cur_base,
			min_t(unsigned long, npages,
			PAGE_SIZE / sizeof (struct page *)),
			gup_flags, page_list, NULL);
#else
#ifdef HAVE_GET_USER_PAGES_7_PARAMS
		ret = get_user_pages(current, current->mm, cur_base,
#else
		ret = get_user_pages(cur_base,
#endif
				min_t(unsigned long, npages,
					PAGE_SIZE / sizeof (struct page *)),
#ifdef HAVE_GET_USER_PAGES_GUP_FLAGS
				gup_flags, page_list, vma_list);
#else
				1, !umem->writable, page_list, vma_list);
#endif
#endif /*HAVE_FOLL_LONGTERM*/

		if (ret < 0) {
#ifdef HAVE_GET_USER_PAGES_GUP_FLAGS
			pr_debug("%s: failed to get user pages, nr_pages=%lu, flags=%u\n", __func__,
					min_t(unsigned long, npages,
						PAGE_SIZE / sizeof(struct page *)),
					gup_flags);
#else
			pr_debug("%s: failed to get user pages, nr_pages=%lu\n", __func__,
			       min_t(unsigned long, npages,
				     PAGE_SIZE / sizeof(struct page *)));
#endif
#ifndef HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED
			up_read(&mm->mmap_sem);
#endif
			goto umem_release;
		}
#endif /*HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED*/

		cur_base += ret * PAGE_SIZE;
		npages -= ret;
#ifdef HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS
		sg = __sg_alloc_table_from_pages(
				&umem->sg_head, page_list, ret, 0, ret << PAGE_SHIFT,
#else
		sg = ib_umem_add_sg_table(sg, page_list, ret,
#endif
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
       		dma_get_max_seg_size(device->dma_device),
#else
		dma_get_max_seg_size(context->device->dma_device),
#endif
#ifdef HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS
				sg, npages,
				GFP_KERNEL);
		umem->sg_nents = umem->sg_head.nents;
		if (IS_ERR(sg)) {
#ifdef HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED
			unpin_user_pages_dirty_lock(page_list, ret, 0);
#elif defined(HAVE_PUT_USER_PAGES_DIRTY_LOCK_3_PARAMS)
			put_user_pages_dirty_lock(page_list, ret, 0);
#elif defined(HAVE_PUT_USER_PAGES_DIRTY_LOCK_2_PARAMS)
			for (index = 0; index < ret; index++)
				put_user_page(page_list[index]);
#else
			for (index = 0; index < ret; index++)
				put_page(page_list[index]);
#endif /*HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED*/
			ret = PTR_ERR(sg);
			goto umem_release;
		}
#else
		&umem->sg_nents);
#endif
#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
		/* Continue to hold the mmap_sem as vma_list access
		 *               * needs to be protected.
		 *                                */
		for (i = 0; i < ret && umem->hugetlb; i++) {
			if (vma_list && !is_vm_hugetlb_page(vma_list[i]))
				umem->hugetlb = 0;
		}
#endif
#ifndef HAVE_UNPIN_USER_PAGES_DIRTY_LOCK_EXPORTED
		up_read(&mm->mmap_sem);
#endif
	}

#ifndef HAVE_SG_ALLOC_TABLE_FROM_PAGES_GET_9_PARAMS
	sg_mark_end(sg);
#endif
	if (access & IB_ACCESS_RELAXED_ORDERING)
		dma_attr |= DMA_ATTR_WEAK_ORDERING;

#ifndef DMA_ATTR_WRITE_BARRIER
	umem->nmap = ib_dma_map_sg(
#else
	umem->nmap = ib_dma_map_sg_attrs(
#endif
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
					device,
#else
					context->device,
#endif
					umem->sg_head.sgl,
 				  	umem->sg_nents,
					DMA_BIDIRECTIONAL
#ifdef DMA_ATTR_WRITE_BARRIER
                                  , dma_attrs
#endif //DMA_ATTR_WRITE_BARRIER
				  );

	if (!umem->nmap) {
		pr_err("%s: failed to map scatterlist, npages=%lu\n", __func__,
		       npages);
		ret = -ENOMEM;
		goto umem_release;
	}

	ret = 0;
	goto out;

#endif /*HAVE_SG_APPEND_TABLE*/

umem_release:
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
	__ib_umem_release(device, umem, 0);
#else
	__ib_umem_release(context->device, umem, 0);
#endif

	/*
	 * If the address belongs to peer memory client, then the first
	 * call to get_user_pages will fail. In this case, try to get
	 * these pages from the peers.
	 */
	//FIXME: this placement is horrible
	if (ret < 0 && peer_mem_flags & IB_PEER_MEM_ALLOW) {
		struct ib_umem *new_umem;

		new_umem = ib_peer_umem_get(umem, ret, peer_mem_flags);
		if (IS_ERR(new_umem)) {
			ret = PTR_ERR(new_umem);
			goto vma;
		}
		umem = new_umem;
		ret = 0;
		goto out;
	}
vma:
#ifdef HAVE_ATOMIC_PINNED_VM
	atomic64_sub(ib_umem_num_pages(umem), &mm->pinned_vm);
#else
	down_write(&mm->mmap_sem);
#ifdef HAVE_PINNED_VM
	mm->pinned_vm -= ib_umem_num_pages(umem);
#else
	mm->locked_vm -= ib_umem_num_pages(umem);
#endif /* HAVE_PINNED_VM */
	up_write(&mm->mmap_sem);
#endif /* HAVE_ATOMIC_PINNED_VM */
out:
#if !defined(HAVE_FOLL_LONGTERM) && !defined(HAVE_GET_USER_PAGES_LONGTERM)
	if (vma_list)
		free_page((unsigned long) vma_list);
#endif
	free_page((unsigned long) page_list);
umem_kfree:
	if (ret) {
		mmdrop(umem->owning_mm);
		kfree(umem);
	}
	return ret ? ERR_PTR(ret) : umem;
}

#ifdef HAVE_MMU_INTERVAL_NOTIFIER
struct ib_umem *ib_umem_get(struct ib_device *device, unsigned long addr,
#else
struct ib_umem *ib_umem_get(struct ib_udata *udata, unsigned long addr,
#endif
			    size_t size, int access)
{
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
	return __ib_umem_get(device, addr, size, access, 0);
#else
	return __ib_umem_get(udata, addr, size, access, 0);
#endif
}
EXPORT_SYMBOL(ib_umem_get);

#ifdef HAVE_MMU_INTERVAL_NOTIFIER
struct ib_umem *ib_umem_get_peer(struct ib_device *device, unsigned long addr,
#else
struct ib_umem *ib_umem_get_peer(struct ib_udata *udata, unsigned long addr,
#endif
				 size_t size, int access,
				 unsigned long peer_mem_flags)
{
#ifdef HAVE_MMU_INTERVAL_NOTIFIER
	return __ib_umem_get(device, addr, size, access,
			     IB_PEER_MEM_ALLOW | peer_mem_flags);
#else
	return __ib_umem_get(udata, addr, size, access,
			     IB_PEER_MEM_ALLOW | peer_mem_flags);
#endif
}
EXPORT_SYMBOL(ib_umem_get_peer);

/**
 * ib_umem_release - release memory pinned with ib_umem_get
 * @umem: umem struct to release
 */
void ib_umem_release(struct ib_umem *umem)
{
	if (!umem)
		return;
#ifdef HAVE_DMA_BUF_DYNAMIC_ATTACH_GET_4_PARAMS
	if (umem->is_dmabuf)
		return ib_umem_dmabuf_release(to_ib_umem_dmabuf(umem));
#endif
#ifdef CONFIG_INFINIBAND_ON_DEMAND_PAGING
	if (umem->is_odp)
		return ib_umem_odp_release(to_ib_umem_odp(umem));
#endif

	if (umem->is_peer)
		return ib_peer_umem_release(umem);
#ifdef HAVE_MMU_NOTIFIER_OPS_HAS_FREE_NOTIFIER
	__ib_umem_release(umem->ibdev, umem, 1);
#else
	__ib_umem_release(umem->context->device, umem, 1);
#endif

#ifdef HAVE_ATOMIC_PINNED_VM
	atomic64_sub(ib_umem_num_pages(umem), &umem->owning_mm->pinned_vm);
#else
	down_write(&umem->owning_mm->mmap_sem);
#ifdef HAVE_PINNED_VM
	umem->owning_mm->pinned_vm -= ib_umem_num_pages(umem);
#else
	umem->owning_mm->locked_vm -= ib_umem_num_pages(umem);
#endif /* HAVE_PINNED_VM */
	up_write(&umem->owning_mm->mmap_sem);
#endif /*HAVE_ATOMIC_PINNED_VM*/
	mmdrop(umem->owning_mm);
	kfree(umem);
}
EXPORT_SYMBOL(ib_umem_release);

/*
 * Copy from the given ib_umem's pages to the given buffer.
 *
 * umem - the umem to copy from
 * offset - offset to start copying from
 * dst - destination buffer
 * length - buffer length
 *
 * Returns 0 on success, or an error code.
 */
int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      size_t length)
{
	size_t end = offset + length;
	int ret;

	if (offset > umem->length || length > umem->length - offset) {
		pr_err("%s not in range. offset: %zd umem length: %zd end: %zd\n",
		       __func__, offset, umem->length, end);
		return -EINVAL;
	}

#ifdef HAVE_SG_APPEND_TABLE
	ret = sg_pcopy_to_buffer(umem->sgt_append.sgt.sgl,
				 umem->sgt_append.sgt.orig_nents, dst, length,
#else
				 ret = sg_pcopy_to_buffer(umem->sg_head.sgl, umem->sg_nents, dst, length,
#endif
				 offset + ib_umem_offset(umem));

	if (ret < 0)
		return ret;
	else if (ret != length)
		return -EINVAL;
	else
		return 0;
}
EXPORT_SYMBOL(ib_umem_copy_from);
