// SPDX-License-Identifier: GPL-2.0
/*
 * NVMe I/O command implementation.
 * Copyright (c) 2015-2016 HGST, a Western Digital Company.
 */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/blkdev.h>
#ifdef HAVE_BLK_INTEGRITY_H
#include <linux/blk-integrity.h>
#endif
#include <linux/memremap.h>
#include <linux/module.h>
#include "nvmet.h"

void nvmet_bdev_set_limits(struct block_device *bdev, struct nvme_id_ns *id)
{
#ifndef HAVE_BDEV_DISCARD_GRANULARITY
	const struct queue_limits *ql = &bdev_get_queue(bdev)->limits;
#endif
	/* Logical blocks per physical block, 0's based. */
	const __le16 lpp0b = to0based(bdev_physical_block_size(bdev) /
				      bdev_logical_block_size(bdev));

	/*
	 * For NVMe 1.2 and later, bit 1 indicates that the fields NAWUN,
	 * NAWUPF, and NACWU are defined for this namespace and should be
	 * used by the host for this namespace instead of the AWUN, AWUPF,
	 * and ACWU fields in the Identify Controller data structure. If
	 * any of these fields are zero that means that the corresponding
	 * field from the identify controller data structure should be used.
	 */
	id->nsfeat |= 1 << 1;
	id->nawun = lpp0b;
	id->nawupf = lpp0b;
	id->nacwu = lpp0b;

	/*
	 * Bit 4 indicates that the fields NPWG, NPWA, NPDG, NPDA, and
	 * NOWS are defined for this namespace and should be used by
	 * the host for I/O optimization.
	 */
	id->nsfeat |= 1 << 4;
	/* NPWG = Namespace Preferred Write Granularity. 0's based */
	id->npwg = lpp0b;
	/* NPWA = Namespace Preferred Write Alignment. 0's based */
	id->npwa = id->npwg;
	/* NPDG = Namespace Preferred Deallocate Granularity. 0's based */
#ifndef HAVE_BDEV_DISCARD_GRANULARITY
	id->npdg = to0based(ql->discard_granularity / ql->logical_block_size);
#else
	id->npdg = to0based(bdev_discard_granularity(bdev) /
			    bdev_logical_block_size(bdev));
#endif
	/* NPDG = Namespace Preferred Deallocate Alignment */
	id->npda = id->npdg;
	/* NOWS = Namespace Optimal Write Size */
	id->nows = to0based(bdev_io_opt(bdev) / bdev_logical_block_size(bdev));
}

void nvmet_bdev_ns_disable(struct nvmet_ns *ns)
{
#ifdef HAVE_BDEV_FILE_OPEN_BY_PATH
	if (ns->bdev_file) {
		fput(ns->bdev_file);
		ns->bdev = NULL;
		ns->bdev_file = NULL;
	}
#else
#ifdef HAVE_BDEV_RELEASE
	if (ns->bdev_handle) {
		bdev_release(ns->bdev_handle);
#else
	if (ns->bdev) {
#ifdef HAVE_BLKDEV_PUT_HOLDER
		blkdev_put(ns->bdev, NULL);
#else
		blkdev_put(ns->bdev, FMODE_WRITE | FMODE_READ);
#endif
#endif
		ns->bdev = NULL;
#ifdef HAVE_BDEV_RELEASE
		ns->bdev_handle = NULL;
#endif
	}
#endif /* HAVE_BDEV_FILE_OPEN_BY_PATH */
}

static void nvmet_bdev_ns_enable_integrity(struct nvmet_ns *ns)
{
#if defined(CONFIG_BLK_DEV_INTEGRITY) && \
	defined(HAVE_BLKDEV_BIO_INTEGRITY_BYTES)

	struct blk_integrity *bi = bdev_get_integrity(ns->bdev);

        if (!bi)
                return;

#ifdef HAVE_BLK_INTEGRITY_CSUM_CRC64
        if (bi->csum_type == BLK_INTEGRITY_CSUM_CRC) {
                ns->metadata_size = bi->tuple_size;
                if (bi->flags & BLK_INTEGRITY_REF_TAG)
                        ns->pi_type = NVME_NS_DPS_PI_TYPE1;
                else
                        ns->pi_type = NVME_NS_DPS_PI_TYPE3;
        } else {
                ns->metadata_size = 0;
        }
#else
	ns->metadata_size = bi->tuple_size;
	if (bi->profile == &t10_pi_type1_crc)
		ns->pi_type = NVME_NS_DPS_PI_TYPE1;
	else if (bi->profile == &t10_pi_type3_crc)
		ns->pi_type = NVME_NS_DPS_PI_TYPE3;
	else
		/* Unsupported metadata type */
		ns->metadata_size = 0;
#endif
#endif
}

int nvmet_bdev_ns_enable(struct nvmet_ns *ns)
{
	int ret;

	/*
	 * When buffered_io namespace attribute is enabled that means user want
	 * this block device to be used as a file, so block device can take
	 * an advantage of cache.
	 */
	if (ns->buffered_io)
		return -ENOTBLK;

#ifdef HAVE_BDEV_FILE_OPEN_BY_PATH
	ns->bdev_file = bdev_file_open_by_path(ns->device_path,
				BLK_OPEN_READ | BLK_OPEN_WRITE, NULL, NULL);
	if (IS_ERR(ns->bdev_file)) {
		ret = PTR_ERR(ns->bdev_file);
		if (ret != -ENOTBLK) {
			pr_err("failed to open block device %s: (%d)\n",
					ns->device_path, ret);
		}
		ns->bdev_file = NULL;
		return ret;
	}
	ns->bdev = file_bdev(ns->bdev_file);
#else
#ifdef HAVE_BDEV_RELEASE
	ns->bdev_handle = bdev_open_by_path(ns->device_path,
			BLK_OPEN_READ | BLK_OPEN_WRITE, NULL, NULL);

#elif defined(HAVE_GENDISK_OPEN_MODE)
	ns->bdev = blkdev_get_by_path(ns->device_path,
			BLK_OPEN_READ | BLK_OPEN_WRITE, NULL, NULL);
#else
#ifdef HAVE_BLK_HOLDER_OPS
	ns->bdev = blkdev_get_by_path(ns->device_path,
			FMODE_READ | FMODE_WRITE, NULL, NULL);
#else
	ns->bdev = blkdev_get_by_path(ns->device_path,
			FMODE_READ | FMODE_WRITE, NULL);
#endif
#endif
#ifdef HAVE_BDEV_RELEASE
	if (IS_ERR(ns->bdev_handle)) {
		ret = PTR_ERR(ns->bdev_handle);
		if (ret != -ENOTBLK) {
			pr_err("failed to open block device %s: (%d)\n",
					ns->device_path, ret);
		}
		ns->bdev_handle = NULL;
#else
	if (IS_ERR(ns->bdev)) {
		ret = PTR_ERR(ns->bdev);
		if (ret != -ENOTBLK) {
			pr_err("failed to open block device %s: (%d)\n",
					ns->device_path, ret);
		}
		ns->bdev = NULL;
#endif
		return ret;
	}
#ifdef HAVE_BDEV_RELEASE
	ns->bdev = ns->bdev_handle->bdev;
#endif
#endif /* HAVE_BDEV_FILE_OPEN_BY_PATH */
#ifdef HAVE_BDEV_NR_BYTES
	ns->size = bdev_nr_bytes(ns->bdev);
#else
	ns->size = i_size_read(ns->bdev->bd_inode);
#endif
	ns->blksize_shift = blksize_bits(bdev_logical_block_size(ns->bdev));

	ns->pi_type = 0;
	ns->metadata_size = 0;
	if (IS_ENABLED(CONFIG_BLK_DEV_INTEGRITY))
		nvmet_bdev_ns_enable_integrity(ns);

#ifdef CONFIG_BLK_DEV_ZONED
#ifdef HAVE_BIO_ADD_ZONE_APPEND_PAGE
	if (bdev_is_zoned(ns->bdev)) {
		if (!nvmet_bdev_zns_enable(ns)) {
			nvmet_bdev_ns_disable(ns);
			return -EINVAL;
		}
		ns->csi = NVME_CSI_ZNS;
	}
#endif
#endif

	return 0;
}

void nvmet_bdev_ns_revalidate(struct nvmet_ns *ns)
{
#ifdef HAVE_BDEV_NR_BYTES
	ns->size = bdev_nr_bytes(ns->bdev);
#else
	ns->size = i_size_read(ns->bdev->bd_inode);
#endif
}

u16 blk_to_nvme_status(struct nvmet_req *req, blk_status_t blk_sts)
{
	u16 status = NVME_SC_SUCCESS;

	if (likely(blk_sts == BLK_STS_OK))
		return status;
	/*
	 * Right now there exists M : 1 mapping between block layer error
	 * to the NVMe status code (see nvme_error_status()). For consistency,
	 * when we reverse map we use most appropriate NVMe Status code from
	 * the group of the NVMe staus codes used in the nvme_error_status().
	 */
	switch (blk_sts) {
	case BLK_STS_NOSPC:
		status = NVME_SC_CAP_EXCEEDED | NVME_SC_DNR;
		req->error_loc = offsetof(struct nvme_rw_command, length);
		break;
	case BLK_STS_TARGET:
		status = NVME_SC_LBA_RANGE | NVME_SC_DNR;
		req->error_loc = offsetof(struct nvme_rw_command, slba);
		break;
	case BLK_STS_NOTSUPP:
		req->error_loc = offsetof(struct nvme_common_command, opcode);
		switch (req->cmd->common.opcode) {
		case nvme_cmd_dsm:
		case nvme_cmd_write_zeroes:
			status = NVME_SC_ONCS_NOT_SUPPORTED | NVME_SC_DNR;
			break;
		default:
			status = NVME_SC_INVALID_OPCODE | NVME_SC_DNR;
		}
		break;
	case BLK_STS_MEDIUM:
		status = NVME_SC_ACCESS_DENIED;
		req->error_loc = offsetof(struct nvme_rw_command, nsid);
		break;
	case BLK_STS_IOERR:
	default:
		status = NVME_SC_INTERNAL | NVME_SC_DNR;
		req->error_loc = offsetof(struct nvme_common_command, opcode);
	}

	switch (req->cmd->common.opcode) {
	case nvme_cmd_read:
	case nvme_cmd_write:
		req->error_slba = le64_to_cpu(req->cmd->rw.slba);
		break;
	case nvme_cmd_write_zeroes:
		req->error_slba =
			le64_to_cpu(req->cmd->write_zeroes.slba);
		break;
	default:
		req->error_slba = 0;
	}
	return status;
}

static void nvmet_bio_done(struct bio *bio)
{
	struct nvmet_req *req = bio->bi_private;

	nvmet_req_complete(req, blk_to_nvme_status(req, bio->bi_status));
	nvmet_req_bio_put(req, bio);
}

#if defined(CONFIG_BLK_DEV_INTEGRITY) && \
	defined(HAVE_BLKDEV_BIO_INTEGRITY_BYTES)
static int nvmet_bdev_alloc_bip(struct nvmet_req *req, struct bio *bio,
				struct sg_mapping_iter *miter)
{
	struct blk_integrity *bi;
	struct bio_integrity_payload *bip;
	int rc;
	size_t resid, len;

	bi = bdev_get_integrity(req->ns->bdev);
	if (unlikely(!bi)) {
		pr_err("Unable to locate bio_integrity\n");
		return -ENODEV;
	}

#ifdef HAVE_BIO_MAX_SEGS
	bip = bio_integrity_alloc(bio, GFP_NOIO,
					bio_max_segs(req->metadata_sg_cnt));
#else
	bip = bio_integrity_alloc(bio, GFP_NOIO,
			min_t(unsigned int, req->metadata_sg_cnt, BIO_MAX_PAGES));
#endif

	if (IS_ERR(bip)) {
		pr_err("Unable to allocate bio_integrity_payload\n");
		return PTR_ERR(bip);
	}

/* Using 5e70d0acf082 instead of 80814b8e359 as it has no new symbols */
#ifndef HAVE_PCIE_CAPABILITY_CLEAR_AND_SET_WORD_LOCKED
	bip->bip_iter.bi_size = bio_integrity_bytes(bi, bio_sectors(bio));
#endif
	/* virtual start sector must be in integrity interval units */
	bip_set_seed(bip, bio->bi_iter.bi_sector >>
		     (bi->interval_exp - SECTOR_SHIFT));

	resid = bio_integrity_bytes(bi, bio_sectors(bio));
	while (resid > 0 && sg_miter_next(miter)) {
		len = min_t(size_t, miter->length, resid);
		rc = bio_integrity_add_page(bio, miter->page, len,
					    offset_in_page(miter->addr));
		if (unlikely(rc != len)) {
			pr_err("bio_integrity_add_page() failed; %d\n", rc);
			sg_miter_stop(miter);
			return -ENOMEM;
		}

		resid -= len;
		if (len < miter->length)
			miter->consumed -= miter->length - len;
	}
	sg_miter_stop(miter);

	return 0;
}
#else
static int nvmet_bdev_alloc_bip(struct nvmet_req *req, struct bio *bio,
				struct sg_mapping_iter *miter)
{
	return -EINVAL;
}
#endif /* CONFIG_BLK_DEV_INTEGRITY */

static void nvmet_bdev_execute_rw(struct nvmet_req *req)
{
#ifdef HAVE_BIO_MAX_SEGS
	unsigned int sg_cnt = req->sg_cnt;
#else
	int sg_cnt = req->sg_cnt;
#endif
	struct bio *bio;
	struct scatterlist *sg;
	struct blk_plug plug;
	sector_t sector;
#ifdef HAVE_BLK_OPF_T
	blk_opf_t opf;
	int i, rc;
#else
	int op, i, rc;
#endif
	struct sg_mapping_iter prot_miter;
	unsigned int iter_flags;
	unsigned int total_len = nvmet_rw_data_len(req) + req->metadata_len;

	if (!nvmet_check_transfer_len(req, total_len))
		return;

	if (!req->sg_cnt) {
		nvmet_req_complete(req, 0);
		return;
	}

	if (req->cmd->rw.opcode == nvme_cmd_write) {
#ifdef HAVE_BLK_OPF_T
		opf = REQ_OP_WRITE | REQ_SYNC | REQ_IDLE;
#else
		op = REQ_OP_WRITE | REQ_SYNC | REQ_IDLE;
#endif
		if (req->cmd->rw.control & cpu_to_le16(NVME_RW_FUA))
#ifdef HAVE_BLK_OPF_T
			opf |= REQ_FUA;
#else
			op |= REQ_FUA;
#endif
		iter_flags = SG_MITER_TO_SG;
	} else {
#ifdef HAVE_BLK_OPF_T
		opf = REQ_OP_READ;
#else
		op = REQ_OP_READ;
#endif
		iter_flags = SG_MITER_FROM_SG;
	}

	if (is_pci_p2pdma_page(sg_page(req->sg)))
#ifdef HAVE_BLK_OPF_T
		opf |= REQ_NOMERGE;
#else
		op |= REQ_NOMERGE;
#endif

	sector = nvmet_lba_to_sect(req->ns, req->cmd->rw.slba);

	if (nvmet_use_inline_bvec(req)) {
		bio = &req->b.inline_bio;
#ifdef HAVE_BIO_INIT_5_PARAMS
#ifdef HAVE_BLK_OPF_T
		bio_init(bio, req->ns->bdev, req->inline_bvec,
			 ARRAY_SIZE(req->inline_bvec), opf);
#else
		bio_init(bio, req->ns->bdev, req->inline_bvec,
			 ARRAY_SIZE(req->inline_bvec), op);
#endif
#else
#ifdef HAVE_BIO_INIT_3_PARAMS
		bio_init(bio, req->inline_bvec, ARRAY_SIZE(req->inline_bvec));
#else
		bio_init(bio);
		bio->bi_io_vec = req->inline_bvec;
		bio->bi_max_vecs = ARRAY_SIZE(req->inline_bvec);
#endif
#endif
	} else {
#ifdef HAVE_BIO_INIT_5_PARAMS
#ifdef HAVE_BLK_OPF_T
		bio = bio_alloc(req->ns->bdev, bio_max_segs(sg_cnt), opf,
				GFP_KERNEL);
#else
		bio = bio_alloc(req->ns->bdev, bio_max_segs(sg_cnt), op,
				GFP_KERNEL);
#endif
#else
#ifdef HAVE_BIO_MAX_SEGS
		bio = bio_alloc(GFP_KERNEL, bio_max_segs(sg_cnt));
#else
		bio = bio_alloc(GFP_KERNEL, min(sg_cnt, BIO_MAX_PAGES));
#endif
#endif
	}
#if defined HAVE_BIO_BI_DISK || defined HAVE_ENUM_BIO_REMAPPED
#ifndef HAVE_BIO_INIT_5_PARAMS
	bio_set_dev(bio, req->ns->bdev);
#endif
#else
	bio->bi_bdev = req->ns->bdev;
#endif /* HAVE_BIO_BI_DISK || HAVE_ENUM_BIO_REMAPPED */
	bio->bi_iter.bi_sector = sector;
	bio->bi_private = req;
	bio->bi_end_io = nvmet_bio_done;
#ifndef HAVE_BIO_INIT_5_PARAMS
	bio->bi_opf = op;
#endif

	blk_start_plug(&plug);
	if (req->metadata_len)
		sg_miter_start(&prot_miter, req->metadata_sg,
			       req->metadata_sg_cnt, iter_flags);

	for_each_sg(req->sg, sg, req->sg_cnt, i) {
		while (bio_add_page(bio, sg_page(sg), sg->length, sg->offset)
				!= sg->length) {
			struct bio *prev = bio;

			if (req->metadata_len) {
				rc = nvmet_bdev_alloc_bip(req, bio,
							  &prot_miter);
				if (unlikely(rc)) {
					bio_io_error(bio);
					return;
				}
			}

#ifdef HAVE_BIO_INIT_5_PARAMS
#ifdef HAVE_BLK_OPF_T
			bio = bio_alloc(req->ns->bdev, bio_max_segs(sg_cnt),
					opf, GFP_KERNEL);
#else
			bio = bio_alloc(req->ns->bdev, bio_max_segs(sg_cnt),
					op, GFP_KERNEL);
#endif
#else
#ifdef HAVE_BIO_MAX_SEGS
			bio = bio_alloc(GFP_KERNEL, bio_max_segs(sg_cnt));
#else
			bio = bio_alloc(GFP_KERNEL, min(sg_cnt, BIO_MAX_PAGES));
#endif
#endif
#if defined HAVE_BIO_BI_DISK || defined HAVE_ENUM_BIO_REMAPPED
#ifndef HAVE_BIO_INIT_5_PARAMS
			bio_set_dev(bio, req->ns->bdev);
#endif
#else
			bio->bi_bdev = req->ns->bdev;
#endif
			bio->bi_iter.bi_sector = sector;
#ifndef HAVE_BIO_INIT_5_PARAMS
			bio->bi_opf = op;
#endif

			bio_chain(bio, prev);
			submit_bio(prev);
		}

		sector += sg->length >> 9;
		sg_cnt--;
	}

	if (req->metadata_len) {
		rc = nvmet_bdev_alloc_bip(req, bio, &prot_miter);
		if (unlikely(rc)) {
			bio_io_error(bio);
			return;
		}
	}

	submit_bio(bio);
	blk_finish_plug(&plug);
}

static void nvmet_bdev_execute_flush(struct nvmet_req *req)
{
	struct bio *bio = &req->b.inline_bio;

#ifdef HAVE_BDEV_WRITE_CACHE
	if (!bdev_write_cache(req->ns->bdev)) {
		nvmet_req_complete(req, NVME_SC_SUCCESS);
		return;
	}
#endif

	if (!nvmet_check_transfer_len(req, 0))
		return;

#ifdef HAVE_BIO_INIT_5_PARAMS
	bio_init(bio, req->ns->bdev, req->inline_bvec,
			ARRAY_SIZE(req->inline_bvec), REQ_OP_WRITE | REQ_PREFLUSH);
#else
#ifdef HAVE_BIO_INIT_3_PARAMS
	bio_init(bio, req->inline_bvec, ARRAY_SIZE(req->inline_bvec));
#else
	bio_init(bio);
	bio->bi_io_vec = req->inline_bvec;
	bio->bi_max_vecs = ARRAY_SIZE(req->inline_bvec);
#endif
#endif
#if defined HAVE_BIO_BI_DISK || defined HAVE_ENUM_BIO_REMAPPED
#ifndef HAVE_BIO_INIT_5_PARAMS
	bio_set_dev(bio, req->ns->bdev);
#endif
#else
	bio->bi_bdev = req->ns->bdev;
#endif
	bio->bi_private = req;
	bio->bi_end_io = nvmet_bio_done;
#ifndef HAVE_BIO_INIT_5_PARAMS
	bio->bi_opf = REQ_OP_WRITE | REQ_PREFLUSH;
#endif

	submit_bio(bio);
}

u16 nvmet_bdev_flush(struct nvmet_req *req)
{
#ifdef HAVE_BDEV_WRITE_CACHE
	if (!bdev_write_cache(req->ns->bdev))
		return 0;
#endif

#ifdef HAVE_BLKDEV_ISSUE_FLUSH_1_PARAM
	if (blkdev_issue_flush(req->ns->bdev))
#else
#ifdef HAVE_BLKDEV_ISSUE_FLUSH_2_PARAM
	if (blkdev_issue_flush(req->ns->bdev, GFP_KERNEL))
#else
	if (blkdev_issue_flush(req->ns->bdev, GFP_KERNEL, NULL))
#endif
#endif
		return NVME_SC_INTERNAL | NVME_SC_DNR;
	return 0;
}

static u16 nvmet_bdev_discard_range(struct nvmet_req *req,
		struct nvme_dsm_range *range, struct bio **bio)
{
	struct nvmet_ns *ns = req->ns;
	int ret;

#ifdef HAVE___BLKDEV_ISSUE_DISCARD_5_PARAM
	ret = __blkdev_issue_discard(ns->bdev,
			nvmet_lba_to_sect(ns, range->slba),
			le32_to_cpu(range->nlb) << (ns->blksize_shift - 9),
			GFP_KERNEL, bio);
#else
#ifdef HAVE___BLKDEV_ISSUE_DISCARD
	ret = __blkdev_issue_discard(ns->bdev,
			nvmet_lba_to_sect(ns, range->slba),
			le32_to_cpu(range->nlb) << (ns->blksize_shift - 9),
			GFP_KERNEL, 0, bio);
#else
	ret = blkdev_issue_discard(ns->bdev,
			nvmet_lba_to_sect(ns, range->slba),
			le32_to_cpu(range->nlb) << (ns->blksize_shift - 9),
			GFP_KERNEL, 0);
#endif
#endif
	if (ret && ret != -EOPNOTSUPP) {
		req->error_slba = le64_to_cpu(range->slba);
		return errno_to_nvme_status(req, ret);
	}
	return NVME_SC_SUCCESS;
}

static void nvmet_bdev_execute_discard(struct nvmet_req *req)
{
	struct nvme_dsm_range range;
	struct bio *bio = NULL;
	int i;
	u16 status;

	for (i = 0; i <= le32_to_cpu(req->cmd->dsm.nr); i++) {
		status = nvmet_copy_from_sgl(req, i * sizeof(range), &range,
				sizeof(range));
		if (status)
			break;

		status = nvmet_bdev_discard_range(req, &range, &bio);
		if (status)
			break;
	}

	if (bio) {
		bio->bi_private = req;
		bio->bi_end_io = nvmet_bio_done;
		if (status)
			bio_io_error(bio);
		else
			submit_bio(bio);
	} else {
		nvmet_req_complete(req, status);
	}
}

static void nvmet_bdev_execute_dsm(struct nvmet_req *req)
{
	if (!nvmet_check_data_len_lte(req, nvmet_dsm_len(req)))
		return;

	switch (le32_to_cpu(req->cmd->dsm.attributes)) {
	case NVME_DSMGMT_AD:
		nvmet_bdev_execute_discard(req);
		return;
	case NVME_DSMGMT_IDR:
	case NVME_DSMGMT_IDW:
	default:
		/* Not supported yet */
		nvmet_req_complete(req, 0);
		return;
	}
}

static void nvmet_bdev_execute_write_zeroes(struct nvmet_req *req)
{
	struct nvme_write_zeroes_cmd *write_zeroes = &req->cmd->write_zeroes;
	struct bio *bio = NULL;
	sector_t sector;
	sector_t nr_sector;
	int ret;

	if (!nvmet_check_transfer_len(req, 0))
		return;

	sector = nvmet_lba_to_sect(req->ns, write_zeroes->slba);
	nr_sector = (((sector_t)le16_to_cpu(write_zeroes->length) + 1) <<
		(req->ns->blksize_shift - 9));

#ifdef CONFIG_COMPAT_IS_BLKDEV_ISSUE_ZEROOUT_HAS_FLAGS
	ret = __blkdev_issue_zeroout(req->ns->bdev, sector, nr_sector,
			GFP_KERNEL, &bio, 0);
#else
	if (__blkdev_issue_zeroout(req->ns->bdev, sector, nr_sector,
			GFP_KERNEL, &bio, true))
		ret = -EIO;
	else
		ret = 0;
#endif
	if (bio) {
		bio->bi_private = req;
		bio->bi_end_io = nvmet_bio_done;
		submit_bio(bio);
	} else {
		nvmet_req_complete(req, errno_to_nvme_status(req, ret));
	}
}

u16 nvmet_bdev_parse_io_cmd(struct nvmet_req *req)
{
	switch (req->cmd->common.opcode) {
	case nvme_cmd_read:
	case nvme_cmd_write:
		req->execute = nvmet_bdev_execute_rw;
		if (req->sq->ctrl->pi_support && nvmet_ns_has_pi(req->ns))
			req->metadata_len = nvmet_rw_metadata_len(req);
		return 0;
	case nvme_cmd_flush:
		req->execute = nvmet_bdev_execute_flush;
		return 0;
	case nvme_cmd_dsm:
		req->execute = nvmet_bdev_execute_dsm;
		return 0;
	case nvme_cmd_write_zeroes:
		req->execute = nvmet_bdev_execute_write_zeroes;
		return 0;
	default:
		return nvmet_report_invalid_opcode(req);
	}
}
