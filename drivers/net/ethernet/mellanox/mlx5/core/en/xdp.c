/*
 * Copyright (c) 2018, Mellanox Technologies. All rights reserved.
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

#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf_trace.h>
#ifdef HAVE_NET_PAGE_POOL_OLD_H
#include <net/page_pool.h>
#endif
#ifdef HAVE_NET_PAGE_POOL_TYPES_H
#include <net/page_pool/types.h>
#include <net/page_pool/helpers.h>
#endif
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
#ifdef HAVE_XDP_SOCK_DRV_H
#include <net/xdp_sock_drv.h>
#else
#include <net/xdp_sock.h>
#endif
#endif
#include "en/xdp.h"
#include "en/params.h"
#include <linux/bitfield.h>

int mlx5e_xdp_max_mtu(struct mlx5e_params *params, struct mlx5e_xsk_param *xsk)
{
	int hr = mlx5e_get_linear_rq_headroom(params, xsk);

	/* Let S := SKB_DATA_ALIGN(sizeof(struct skb_shared_info)).
	 * The condition checked in mlx5e_rx_is_linear_skb is:
	 *   SKB_DATA_ALIGN(sw_mtu + hard_mtu + hr) + S <= PAGE_SIZE         (1)
	 *   (Note that hw_mtu == sw_mtu + hard_mtu.)
	 * What is returned from this function is:
	 *   max_mtu = PAGE_SIZE - S - hr - hard_mtu                         (2)
	 * After assigning sw_mtu := max_mtu, the left side of (1) turns to
	 * SKB_DATA_ALIGN(PAGE_SIZE - S) + S, which is equal to PAGE_SIZE,
	 * because both PAGE_SIZE and S are already aligned. Any number greater
	 * than max_mtu would make the left side of (1) greater than PAGE_SIZE,
	 * so max_mtu is the maximum MTU allowed.
	 */

	return MLX5E_HW2SW_MTU(params, SKB_MAX_HEAD(hr));
}

static inline bool
mlx5e_xmit_xdp_buff(struct mlx5e_xdpsq *sq,
		    struct mlx5e_rq *rq,
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
		    struct mlx5e_alloc_unit *au,
#endif
		    struct xdp_buff *xdp)
{
	struct page *page = virt_to_page(xdp->data);
	struct mlx5e_xmit_data_frags xdptxdf = {};
	struct mlx5e_xmit_data *xdptxd;
	struct xdp_frame *xdpf;
	dma_addr_t dma_addr;
	int i;

#ifdef HAVE_XDP_CONVERT_BUFF_TO_FRAME
	xdpf = xdp_convert_buff_to_frame(xdp);
#else
	xdpf = convert_to_xdp_frame(xdp);
#endif
	if (unlikely(!xdpf))
		return false;

	xdptxd = &xdptxdf.xd;
	xdptxd->data = xdpf->data;
	xdptxd->len  = xdpf->len;
#ifdef HAVE_XDP_HAS_FRAGS
	xdptxd->has_frags = xdp_frame_has_frags(xdpf);
#else
	xdptxd->has_frags = false;
#endif

#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
	if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL) {
		/* The xdp_buff was in the UMEM and was copied into a newly
		 * allocated page. The UMEM page was returned via the ZCA, and
		 * this new page has to be mapped at this point and has to be
		 * unmapped and returned via xdp_return_frame on completion.
		 */

		/* Prevent double recycling of the UMEM page. Even in case this
		 * function returns false, the xdp_buff shouldn't be recycled,
		 * as it was already done in xdp_convert_zc_to_xdp_frame.
		 */
		__set_bit(MLX5E_RQ_FLAG_XDP_XMIT, rq->flags); /* non-atomic */

		if (unlikely(xdptxd->has_frags))
			return false;

		dma_addr = dma_map_single(sq->pdev, xdptxd->data, xdptxd->len,
					  DMA_TO_DEVICE);
		if (dma_mapping_error(sq->pdev, dma_addr)) {
			xdp_return_frame(xdpf);
			return false;
		}

		xdptxd->dma_addr = dma_addr;

		if (unlikely(!INDIRECT_CALL_2(sq->xmit_xdp_frame, mlx5e_xmit_xdp_frame_mpwqe,
					      mlx5e_xmit_xdp_frame, sq, xdptxd, 0, NULL)))
			return false;

		/* xmit_mode == MLX5E_XDP_XMIT_MODE_FRAME */
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .mode = MLX5E_XDP_XMIT_MODE_FRAME });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .frame.xdpf = xdpf });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .frame.dma_addr = dma_addr });
		return true;
	}
#endif

	/* Driver assumes that xdp_convert_buff_to_frame returns an xdp_frame
	 * that points to the same memory region as the original xdp_buff. It
	 * allows to map the memory only once and to use the DMA_BIDIRECTIONAL
	 * mode.
	 */

#ifdef HAVE_PAGE_POOL_GET_DMA_ADDR
	dma_addr = page_pool_get_dma_addr(page) + (xdpf->data - (void *)xdpf);
#elif defined(HAVE_PAGE_DMA_ADDR_ARRAY)
	dma_addr = page->dma_addr[0] + (xdpf->data - (void *)xdpf);
#elif defined(HAVE_PAGE_DMA_ADDR)
	dma_addr = page->dma_addr + (xdpf->data - (void *)xdpf);
#else
	dma_addr = au->addr + (xdpf->data - (void *)xdpf);
#endif
	dma_sync_single_for_device(sq->pdev, dma_addr, xdptxd->len, DMA_BIDIRECTIONAL);

#ifdef HAVE_XDP_HAS_FRAGS
	if (xdptxd->has_frags) {
		xdptxdf.sinfo = xdp_get_shared_info_from_frame(xdpf);
		xdptxdf.dma_arr = NULL;

		for (i = 0; i < xdptxdf.sinfo->nr_frags; i++) {
			skb_frag_t *frag = &xdptxdf.sinfo->frags[i];
			dma_addr_t addr;
			u32 len;

			addr = page_pool_get_dma_addr(skb_frag_page(frag)) +
				skb_frag_off(frag);
			len = skb_frag_size(frag);
			dma_sync_single_for_device(sq->pdev, addr, len,
						   DMA_BIDIRECTIONAL);
		}
	}
#endif

	xdptxd->dma_addr = dma_addr;

	if (unlikely(!INDIRECT_CALL_2(sq->xmit_xdp_frame, mlx5e_xmit_xdp_frame_mpwqe,
				      mlx5e_xmit_xdp_frame, sq, xdptxd, 0, NULL)))
		return false;

	/* xmit_mode == MLX5E_XDP_XMIT_MODE_PAGE */
	mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
			     (union mlx5e_xdp_info) { .mode = MLX5E_XDP_XMIT_MODE_PAGE });

#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
	mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .page.rq = rq });
#endif

#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
#define _SET_PAGE_IN_XDP_INFO(_page) { .page.page = _page }
#else
#define _SET_PAGE_IN_XDP_INFO(_page) { .page.au.page = _page }
#endif

	if (xdptxd->has_frags) {
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info)
				     { .page.num = 1 + xdptxdf.sinfo->nr_frags });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) _SET_PAGE_IN_XDP_INFO(page) );
		for (i = 0; i < xdptxdf.sinfo->nr_frags; i++) {
			skb_frag_t *frag = &xdptxdf.sinfo->frags[i];

			mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
					     (union mlx5e_xdp_info)
					     _SET_PAGE_IN_XDP_INFO(skb_frag_page(frag)));
		}
	} else {
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .page.num = 1 });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) _SET_PAGE_IN_XDP_INFO(page));
	}

#undef _SET_PAGE_IN_XDP_INFO

	return true;
}

#ifdef HAVE_XDP_METADATA_OPS
static int mlx5e_xdp_rx_timestamp(const struct xdp_md *ctx, u64 *timestamp)
{
	const struct mlx5e_xdp_buff *_ctx = (void *)ctx;

	if (unlikely(!mlx5e_rx_hw_stamp(_ctx->rq->tstamp)))
		return -ENODATA;

	*timestamp =  mlx5e_cqe_ts_to_ns(_ctx->rq->ptp_cyc2time,
					 _ctx->rq->clock, get_cqe_ts(_ctx->cqe));
	return 0;
}

/* Mapping HW RSS Type bits CQE_RSS_HTYPE_IP + CQE_RSS_HTYPE_L4 into 4-bits*/
#define RSS_TYPE_MAX_TABLE	16 /* 4-bits max 16 entries */
#define RSS_L4		GENMASK(1, 0)
#define RSS_L3		GENMASK(3, 2) /* Same as CQE_RSS_HTYPE_IP */

/* Valid combinations of CQE_RSS_HTYPE_IP + CQE_RSS_HTYPE_L4 sorted numerical */
enum mlx5_rss_hash_type {
	RSS_TYPE_NO_HASH	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IP_NONE) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_NONE)),
	RSS_TYPE_L3_IPV4	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV4) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_NONE)),
	RSS_TYPE_L4_IPV4_TCP	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV4) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_TCP)),
	RSS_TYPE_L4_IPV4_UDP	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV4) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_UDP)),
	RSS_TYPE_L4_IPV4_IPSEC	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV4) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_IPSEC)),
	RSS_TYPE_L3_IPV6	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV6) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_NONE)),
	RSS_TYPE_L4_IPV6_TCP	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV6) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_TCP)),
	RSS_TYPE_L4_IPV6_UDP	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV6) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_UDP)),
	RSS_TYPE_L4_IPV6_IPSEC	= (FIELD_PREP_CONST(RSS_L3, CQE_RSS_IPV6) |
				   FIELD_PREP_CONST(RSS_L4, CQE_RSS_L4_IPSEC)),
};

/* Invalid combinations will simply return zero, allows no boundary checks */
static const enum xdp_rss_hash_type mlx5_xdp_rss_type[RSS_TYPE_MAX_TABLE] = {
	[RSS_TYPE_NO_HASH]	 = XDP_RSS_TYPE_NONE,
	[1]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[2]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[3]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[RSS_TYPE_L3_IPV4]	 = XDP_RSS_TYPE_L3_IPV4,
	[RSS_TYPE_L4_IPV4_TCP]	 = XDP_RSS_TYPE_L4_IPV4_TCP,
	[RSS_TYPE_L4_IPV4_UDP]	 = XDP_RSS_TYPE_L4_IPV4_UDP,
	[RSS_TYPE_L4_IPV4_IPSEC] = XDP_RSS_TYPE_L4_IPV4_IPSEC,
	[RSS_TYPE_L3_IPV6]	 = XDP_RSS_TYPE_L3_IPV6,
	[RSS_TYPE_L4_IPV6_TCP]	 = XDP_RSS_TYPE_L4_IPV6_TCP,
	[RSS_TYPE_L4_IPV6_UDP]   = XDP_RSS_TYPE_L4_IPV6_UDP,
	[RSS_TYPE_L4_IPV6_IPSEC] = XDP_RSS_TYPE_L4_IPV6_IPSEC,
	[12]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[13]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[14]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
	[15]			 = XDP_RSS_TYPE_NONE, /* Implicit zero */
};

static int mlx5e_xdp_rx_hash(const struct xdp_md *ctx, u32 *hash,
			     enum xdp_rss_hash_type *rss_type)
{
	const struct mlx5e_xdp_buff *_ctx = (void *)ctx;
	const struct mlx5_cqe64 *cqe = _ctx->cqe;
	u32 hash_type, l4_type, ip_type, lookup;

	if (unlikely(!(_ctx->xdp.rxq->dev->features & NETIF_F_RXHASH)))
		return -ENODATA;

	*hash = be32_to_cpu(cqe->rss_hash_result);

	hash_type = cqe->rss_hash_type;
	BUILD_BUG_ON(CQE_RSS_HTYPE_IP != RSS_L3); /* same mask */
	ip_type = hash_type & CQE_RSS_HTYPE_IP;
	l4_type = FIELD_GET(CQE_RSS_HTYPE_L4, hash_type);
	lookup = ip_type | l4_type;
	*rss_type = mlx5_xdp_rss_type[lookup];

	return 0;
}

#ifdef HAVE_XDP_METADATA_OPS_HAS_VLAN_TAG
static int mlx5e_xdp_rx_vlan_tag(const struct xdp_md *ctx, __be16 *vlan_proto,
				 u16 *vlan_tci)
{
	const struct mlx5e_xdp_buff *_ctx = (void *)ctx;
	const struct mlx5_cqe64 *cqe = _ctx->cqe;

	if (!cqe_has_vlan(cqe))
		return -ENODATA;

	*vlan_proto = htons(ETH_P_8021Q);
	*vlan_tci = be16_to_cpu(cqe->vlan_info);
	return 0;
}
#endif

const struct xdp_metadata_ops mlx5e_xdp_metadata_ops = {
	.xmo_rx_timestamp		= mlx5e_xdp_rx_timestamp,
	.xmo_rx_hash			= mlx5e_xdp_rx_hash,
#ifdef HAVE_XDP_METADATA_OPS_HAS_VLAN_TAG
	.xmo_rx_vlan_tag		= mlx5e_xdp_rx_vlan_tag,
#endif
};

#ifdef HAVE_XSK_TX_METADATA_OPS
struct mlx5e_xsk_tx_complete {
	struct mlx5_cqe64 *cqe;
	struct mlx5e_cq *cq;
};

static u64 mlx5e_xsk_fill_timestamp(void *_priv)
{
	struct mlx5e_xsk_tx_complete *priv = _priv;
	u64 ts;

	ts = get_cqe_ts(priv->cqe);

	if (mlx5_is_real_time_rq(priv->cq->mdev) || mlx5_is_real_time_sq(priv->cq->mdev))
		return mlx5_real_time_cyc2time(&priv->cq->mdev->clock, ts);

	return  mlx5_timecounter_cyc2time(&priv->cq->mdev->clock, ts);
}

static void mlx5e_xsk_request_checksum(u16 csum_start, u16 csum_offset, void *priv)
{
	struct mlx5_wqe_eth_seg *eseg = priv;

	/* HW/FW is doing parsing, so offsets are largely ignored. */
	eseg->cs_flags |= MLX5_ETH_WQE_L3_CSUM | MLX5_ETH_WQE_L4_CSUM;
}

const struct xsk_tx_metadata_ops mlx5e_xsk_tx_metadata_ops = {
	.tmo_fill_timestamp		= mlx5e_xsk_fill_timestamp,
	.tmo_request_checksum		= mlx5e_xsk_request_checksum,
};
#endif /* HAVE_XSK_TX_METADATA_OPS */
#endif

/* returns true if packet was consumed by xdp */
bool mlx5e_xdp_handle(struct mlx5e_rq *rq,
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
		      struct mlx5e_alloc_unit *au,
#endif
		      struct bpf_prog *prog, struct mlx5e_xdp_buff *mxbuf)
{
	struct xdp_buff *xdp = &mxbuf->xdp;
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
	struct page *page;
#endif
	u32 act;
#ifdef HAVE_XDP_SUPPORT
	int err;
#endif

#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
	page = !au ? NULL : au->page;
#endif
	act = bpf_prog_run_xdp(prog, xdp);
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
#ifndef HAVE_XSK_BUFF_ALLOC
	if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL) {
		u64 off = xdp->data - xdp->data_hard_start;

#ifdef HAVE_XSK_UMEM_ADJUST_OFFSET
		xdp->handle = xsk_umem_adjust_offset(rq->umem, xdp->handle, off);
#else
		xdp->handle = xdp->handle + off;
#endif
	}
#endif
#endif
	switch (act) {
	case XDP_PASS:
		return false;
	case XDP_TX:
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		if (unlikely(!mlx5e_xmit_xdp_buff(rq->xdpsq, rq, xdp)))
#else
		if (unlikely(!mlx5e_xmit_xdp_buff(rq->xdpsq, rq, au, xdp)))
#endif
			goto xdp_abort;
		__set_bit(MLX5E_RQ_FLAG_XDP_XMIT, rq->flags); /* non-atomic */
		return true;
#ifdef HAVE_XDP_SUPPORT
	case XDP_REDIRECT:
		/* When XDP enabled then page-refcnt==1 here */
		err = xdp_do_redirect(rq->netdev, xdp, prog);
		if (unlikely(err))
			goto xdp_abort;
		__set_bit(MLX5E_RQ_FLAG_XDP_XMIT, rq->flags);
		__set_bit(MLX5E_RQ_FLAG_XDP_REDIRECT, rq->flags);
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
		if (au && xdp->rxq->mem.type != MEM_TYPE_XSK_BUFF_POOL) {
#ifdef HAVE_PAGE_DMA_ADDR
			mlx5e_page_dma_unmap(rq, virt_to_page(xdp->data));
#else
			mlx5e_page_dma_unmap(rq, au);
#endif
	}
#endif
		rq->stats->xdp_redirect++;
		return true;
#endif
	default:
#ifdef HAVE_BPF_WARN_IVALID_XDP_ACTION_GET_3_PARAMS
		bpf_warn_invalid_xdp_action(rq->netdev, prog, act);
#else
		bpf_warn_invalid_xdp_action(act);
#endif
		fallthrough;
	case XDP_ABORTED:
xdp_abort:
#if !defined(MLX_DISABLE_TRACEPOINTS)
		trace_xdp_exception(rq->netdev, prog, act);
		fallthrough;
#endif
	case XDP_DROP:
		rq->stats->xdp_drop++;
		return true;
	}
}

#ifndef HAVE_XSK_BUFF_ALLOC
bool mlx5e_xdp_handle_old(struct mlx5e_rq *rq, struct mlx5e_alloc_unit *au,
		      struct bpf_prog *prog, struct xdp_buff *xdp)
{
	struct page *page;
	u32 act;
#ifdef HAVE_XDP_SUPPORT
	int err;
#endif

	page = !au ? NULL : au->page;
	act = bpf_prog_run_xdp(prog, xdp);
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
#ifndef HAVE_XSK_BUFF_ALLOC
	if (xdp->rxq->mem.type == MEM_TYPE_XSK_BUFF_POOL) {
		u64 off = xdp->data - xdp->data_hard_start;

#ifdef HAVE_XSK_UMEM_ADJUST_OFFSET
		xdp->handle = xsk_umem_adjust_offset(rq->umem, xdp->handle, off);
#else
		xdp->handle = xdp->handle + off;
#endif
	}
#endif
#endif
	switch (act) {
	case XDP_PASS:
		return false;
	case XDP_TX:
		if (unlikely(!mlx5e_xmit_xdp_buff(rq->xdpsq, rq, au, xdp)))
			goto xdp_abort;
		__set_bit(MLX5E_RQ_FLAG_XDP_XMIT, rq->flags); /* non-atomic */
		return true;
#ifdef HAVE_XDP_SUPPORT
	case XDP_REDIRECT:
		/* When XDP enabled then page-refcnt==1 here */
		err = xdp_do_redirect(rq->netdev, xdp, prog);
		if (unlikely(err))
			goto xdp_abort;
		__set_bit(MLX5E_RQ_FLAG_XDP_XMIT, rq->flags);
		__set_bit(MLX5E_RQ_FLAG_XDP_REDIRECT, rq->flags);
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
		if (xdp->rxq->mem.type != MEM_TYPE_XSK_BUFF_POOL)
#endif
#ifdef HAVE_PAGE_DMA_ADDR
			mlx5e_page_dma_unmap(rq, virt_to_page(xdp->data));
#else
			mlx5e_page_dma_unmap(rq, au);
#endif
		rq->stats->xdp_redirect++;
		return true;
#endif
	default:
#ifdef HAVE_BPF_WARN_IVALID_XDP_ACTION_GET_3_PARAMS
		bpf_warn_invalid_xdp_action(rq->netdev, prog, act);
#else
		bpf_warn_invalid_xdp_action(act);
#endif
		fallthrough;
	case XDP_ABORTED:
xdp_abort:
#if !defined(MLX_DISABLE_TRACEPOINTS)
		trace_xdp_exception(rq->netdev, prog, act);
		fallthrough;
#endif
	case XDP_DROP:
		rq->stats->xdp_drop++;
		return true;
	}
}
#endif

static u16 mlx5e_xdpsq_get_next_pi(struct mlx5e_xdpsq *sq, u16 size)
{
	struct mlx5_wq_cyc *wq = &sq->wq;
	u16 pi, contig_wqebbs;

	pi = mlx5_wq_cyc_ctr2ix(wq, sq->pc);
	contig_wqebbs = mlx5_wq_cyc_get_contig_wqebbs(wq, pi);
	if (unlikely(contig_wqebbs < size)) {
		struct mlx5e_xdp_wqe_info *wi, *edge_wi;

		wi = &sq->db.wqe_info[pi];
		edge_wi = wi + contig_wqebbs;

		/* Fill SQ frag edge with NOPs to avoid WQE wrapping two pages. */
		for (; wi < edge_wi; wi++) {
			*wi = (struct mlx5e_xdp_wqe_info) {
				.num_wqebbs = 1,
				.num_pkts = 0,
			};
			mlx5e_post_nop(wq, sq->sqn, &sq->pc);
		}
		sq->stats->nops += contig_wqebbs;

		pi = mlx5_wq_cyc_ctr2ix(wq, sq->pc);
	}

	return pi;
}

static void mlx5e_xdp_mpwqe_session_start(struct mlx5e_xdpsq *sq)
{
	struct mlx5e_tx_mpwqe *session = &sq->mpwqe;
	struct mlx5e_xdpsq_stats *stats = sq->stats;
	struct mlx5e_tx_wqe *wqe;
	u16 pi;

	pi = mlx5e_xdpsq_get_next_pi(sq, sq->max_sq_mpw_wqebbs);
	wqe = MLX5E_TX_FETCH_WQE(sq, pi);
	net_prefetchw(wqe->data);

	*session = (struct mlx5e_tx_mpwqe) {
		.wqe = wqe,
		.bytes_count = 0,
		.ds_count = MLX5E_TX_WQE_EMPTY_DS_COUNT,
		.pkt_count = 0,
		.inline_on = mlx5e_xdp_get_inline_state(sq, session->inline_on),
	};

	if (test_bit(MLX5E_SQ_STATE_TX_XDP_CSUM, &sq->state)) {
		struct mlx5_wqe_eth_seg *eseg = &wqe->eth;

		eseg->cs_flags = MLX5_ETH_WQE_L3_CSUM | MLX5_ETH_WQE_L4_CSUM;
	}
	stats->mpwqe++;
}

void mlx5e_xdp_mpwqe_complete(struct mlx5e_xdpsq *sq)
{
	struct mlx5_wq_cyc       *wq    = &sq->wq;
	struct mlx5e_tx_mpwqe *session = &sq->mpwqe;
	struct mlx5_wqe_ctrl_seg *cseg = &session->wqe->ctrl;
	u16 ds_count = session->ds_count;
	u16 pi = mlx5_wq_cyc_ctr2ix(wq, sq->pc);
	struct mlx5e_xdp_wqe_info *wi = &sq->db.wqe_info[pi];

	cseg->opmod_idx_opcode =
		cpu_to_be32((sq->pc << 8) | MLX5_OPCODE_ENHANCED_MPSW);
	cseg->qpn_ds = cpu_to_be32((sq->sqn << 8) | ds_count);

	wi->num_wqebbs = DIV_ROUND_UP(ds_count, MLX5_SEND_WQEBB_NUM_DS);
	wi->num_pkts   = session->pkt_count;

	sq->pc += wi->num_wqebbs;

	sq->doorbell_cseg = cseg;

	session->wqe = NULL; /* Close session */
}

enum {
	MLX5E_XDP_CHECK_OK = 1,
	MLX5E_XDP_CHECK_START_MPWQE = 2,
};

INDIRECT_CALLABLE_SCOPE int mlx5e_xmit_xdp_frame_check_mpwqe(struct mlx5e_xdpsq *sq)
{
	if (unlikely(!sq->mpwqe.wqe)) {
		if (unlikely(!mlx5e_wqc_has_room_for(&sq->wq, sq->cc, sq->pc,
						     sq->stop_room))) {
			/* SQ is full, ring doorbell */
			mlx5e_xmit_xdp_doorbell(sq);
			sq->stats->full++;
			return -EBUSY;
		}

		return MLX5E_XDP_CHECK_START_MPWQE;
	}

	return MLX5E_XDP_CHECK_OK;
}

INDIRECT_CALLABLE_SCOPE bool
mlx5e_xmit_xdp_frame(struct mlx5e_xdpsq *sq, struct mlx5e_xmit_data *xdptxd,
		     int check_result, struct xsk_tx_metadata *meta);

INDIRECT_CALLABLE_SCOPE bool
mlx5e_xmit_xdp_frame_mpwqe(struct mlx5e_xdpsq *sq, struct mlx5e_xmit_data *xdptxd,
			   int check_result, struct xsk_tx_metadata *meta)
{
	struct mlx5e_tx_mpwqe *session = &sq->mpwqe;
	struct mlx5e_xdpsq_stats *stats = sq->stats;
	struct mlx5e_xmit_data *p = xdptxd;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	struct mlx5e_xmit_data tmp;

	if (xdptxd->has_frags) {
		struct mlx5e_xmit_data_frags *xdptxdf =
			container_of(xdptxd, struct mlx5e_xmit_data_frags, xd);

		if (!!xdptxd->len + xdptxdf->sinfo->nr_frags > 1) {
			/* MPWQE is enabled, but a multi-buffer packet is queued for
			 * transmission. MPWQE can't send fragmented packets, so close
			 * the current session and fall back to a regular WQE.
			 */
			if (unlikely(sq->mpwqe.wqe))
				mlx5e_xdp_mpwqe_complete(sq);
			return mlx5e_xmit_xdp_frame(sq, xdptxd, 0, meta);
		}
		if (!xdptxd->len) {
			skb_frag_t *frag = &xdptxdf->sinfo->frags[0];

			tmp.data = skb_frag_address(frag);
			tmp.len = skb_frag_size(frag);
			tmp.dma_addr = xdptxdf->dma_arr ? xdptxdf->dma_arr[0] :
				page_pool_get_dma_addr(skb_frag_page(frag)) +
				skb_frag_off(frag);
			p = &tmp;
		}
	}
#endif

	if (unlikely(p->len > sq->hw_mtu)) {
		stats->err++;
		return false;
	}

	if (!check_result)
		check_result = mlx5e_xmit_xdp_frame_check_mpwqe(sq);
	if (unlikely(check_result < 0))
		return false;

	if (check_result == MLX5E_XDP_CHECK_START_MPWQE) {
		/* Start the session when nothing can fail, so it's guaranteed
		 * that if there is an active session, it has at least one dseg,
		 * and it's safe to complete it at any time.
		 */
		mlx5e_xdp_mpwqe_session_start(sq);
#ifdef HAVE_XSK_TX_METADATA_OPS
		xsk_tx_metadata_request(meta, &mlx5e_xsk_tx_metadata_ops, &session->wqe->eth);
#endif
	}

	mlx5e_xdp_mpwqe_add_dseg(sq, p, stats);

	if (unlikely(mlx5e_xdp_mpwqe_is_full(session, sq->max_sq_mpw_wqebbs)))
		mlx5e_xdp_mpwqe_complete(sq);

	stats->xmit++;
	return true;
}

static int mlx5e_xmit_xdp_frame_check_stop_room(struct mlx5e_xdpsq *sq, int stop_room)
{
	if (unlikely(!mlx5e_wqc_has_room_for(&sq->wq, sq->cc, sq->pc, stop_room))) {
		/* SQ is full, ring doorbell */
		mlx5e_xmit_xdp_doorbell(sq);
		sq->stats->full++;
		return -EBUSY;
	}

	return MLX5E_XDP_CHECK_OK;
}

INDIRECT_CALLABLE_SCOPE int mlx5e_xmit_xdp_frame_check(struct mlx5e_xdpsq *sq)
{
	return mlx5e_xmit_xdp_frame_check_stop_room(sq, 1);
}

INDIRECT_CALLABLE_SCOPE bool
mlx5e_xmit_xdp_frame(struct mlx5e_xdpsq *sq, struct mlx5e_xmit_data *xdptxd,
		     int check_result, struct xsk_tx_metadata *meta)
{
	struct mlx5_wq_cyc       *wq   = &sq->wq;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	struct mlx5e_xmit_data_frags *xdptxdf =
		container_of(xdptxd, struct mlx5e_xmit_data_frags, xd);
	struct mlx5_wqe_ctrl_seg *cseg;
	struct mlx5_wqe_data_seg *dseg;
	struct mlx5_wqe_eth_seg *eseg;
	struct mlx5e_tx_wqe *wqe;
#else
	u16                       pi   = mlx5_wq_cyc_ctr2ix(wq, sq->pc);
	struct mlx5e_tx_wqe      *wqe  = mlx5_wq_cyc_get_wqe(wq, pi);

	struct mlx5_wqe_ctrl_seg *cseg = &wqe->ctrl;
	struct mlx5_wqe_eth_seg  *eseg = &wqe->eth;
	struct mlx5_wqe_data_seg *dseg = wqe->data;
#endif

	dma_addr_t dma_addr = xdptxd->dma_addr;
	u32 dma_len = xdptxd->len;
	u16 ds_cnt, inline_hdr_sz;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	unsigned int frags_size;
	u8 num_wqebbs = 1;
	int num_frags = 0;
	bool inline_ok;
	bool linear;
	u16 pi;
#endif

	struct mlx5e_xdpsq_stats *stats = sq->stats;
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
	net_prefetchw(wqe);
#endif

#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	inline_ok = sq->min_inline_mode == MLX5_INLINE_MODE_NONE ||
		dma_len >= MLX5E_XDP_MIN_INLINE;
	frags_size = xdptxd->has_frags ? xdptxdf->sinfo->xdp_frags_size : 0;

	if (unlikely(!inline_ok || sq->hw_mtu < dma_len + frags_size)) {
#else
	if (unlikely(dma_len < MLX5E_XDP_MIN_INLINE || sq->hw_mtu < dma_len)) {
#endif
		stats->err++;
		return false;
	}

	inline_hdr_sz = 0;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	if (sq->min_inline_mode != MLX5_INLINE_MODE_NONE)
		inline_hdr_sz = MLX5E_XDP_MIN_INLINE;

	linear = !!(dma_len - inline_hdr_sz);
	ds_cnt = MLX5E_TX_WQE_EMPTY_DS_COUNT + linear + !!inline_hdr_sz;

	/* check_result must be 0 if sinfo is passed. */
	if (!check_result) {
		int stop_room = 1;

		if (xdptxd->has_frags) {
			ds_cnt += xdptxdf->sinfo->nr_frags;
			num_frags = xdptxdf->sinfo->nr_frags;
			num_wqebbs = DIV_ROUND_UP(ds_cnt, MLX5_SEND_WQEBB_NUM_DS);
			/* Assuming MLX5_CAP_GEN(mdev, max_wqe_sz_sq) is big
			 * enough to hold all fragments.
			 */
			stop_room = MLX5E_STOP_ROOM(num_wqebbs);
		}

		check_result = mlx5e_xmit_xdp_frame_check_stop_room(sq, stop_room);
	}
#else
	if (!check_result)
		check_result = mlx5e_xmit_xdp_frame_check(sq);
#endif
	if (unlikely(check_result < 0))
		return false;


#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	pi = mlx5e_xdpsq_get_next_pi(sq, num_wqebbs);
	wqe = mlx5_wq_cyc_get_wqe(wq, pi);
	net_prefetchw(wqe);

	cseg = &wqe->ctrl;
	eseg = &wqe->eth;
	dseg = wqe->data;
#else
	ds_cnt = MLX5E_TX_WQE_EMPTY_DS_COUNT + 1;
#endif

	/* copy the inline part if required */
	if (inline_hdr_sz) {
		memcpy(eseg->inline_hdr.start, xdptxd->data, sizeof(eseg->inline_hdr.start));
		memcpy(dseg, xdptxd->data + sizeof(eseg->inline_hdr.start),
		       inline_hdr_sz - sizeof(eseg->inline_hdr.start));
		dma_len  -= inline_hdr_sz;
		dma_addr += inline_hdr_sz;
		dseg++;
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
		ds_cnt++;
#endif
	}

	if (test_bit(MLX5E_SQ_STATE_TX_XDP_CSUM, &sq->state))
		eseg->cs_flags = MLX5_ETH_WQE_L3_CSUM | MLX5_ETH_WQE_L4_CSUM;

	/* write the dma part */
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	if (linear) {
#endif
		dseg->addr       = cpu_to_be64(dma_addr);
		dseg->byte_count = cpu_to_be32(dma_len);
		dseg->lkey       = sq->mkey_be;
		dseg++;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
	}
#endif

	cseg->opmod_idx_opcode = cpu_to_be32((sq->pc << 8) | MLX5_OPCODE_SEND);

	if (test_bit(MLX5E_SQ_STATE_XDP_MULTIBUF, &sq->state)) {
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		int i;
#else
		u8 num_wqebbs;
#endif
		memset(&cseg->trailer, 0, sizeof(cseg->trailer));

		memset(eseg, 0, sizeof(*eseg) - sizeof(eseg->trailer));

		eseg->inline_hdr.sz = cpu_to_be16(inline_hdr_sz);

#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		for (i = 0; i < num_frags; i++) {
			skb_frag_t *frag = &xdptxdf->sinfo->frags[i];
			dma_addr_t addr;

			addr = xdptxdf->dma_arr ? xdptxdf->dma_arr[i] :
				page_pool_get_dma_addr(skb_frag_page(frag)) +
				skb_frag_off(frag);

			dseg->addr = cpu_to_be64(addr);
			dseg->byte_count = cpu_to_be32(skb_frag_size(frag));
			dseg->lkey = sq->mkey_be;
			dseg++;
		}
#endif

		cseg->qpn_ds = cpu_to_be32((sq->sqn << 8) | ds_cnt);
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
		num_wqebbs = DIV_ROUND_UP(ds_cnt, MLX5_SEND_WQEBB_NUM_DS);
#endif

		sq->db.wqe_info[pi] = (struct mlx5e_xdp_wqe_info) {
			.num_wqebbs = num_wqebbs,
			.num_pkts = 1,
		};

		sq->pc += num_wqebbs;
	} else {
		cseg->fm_ce_se = 0;

		sq->pc++;
	}

#ifdef HAVE_XSK_TX_METADATA_OPS
	xsk_tx_metadata_request(meta, &mlx5e_xsk_tx_metadata_ops, eseg);
#endif

	sq->doorbell_cseg = cseg;

	stats->xmit++;
	return true;
}

static void mlx5e_free_xdpsq_desc(struct mlx5e_xdpsq *sq,
				  struct mlx5e_xdp_wqe_info *wi
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
				  , u32 *xsk_frames
#endif
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
				  , bool recycle
#endif
#ifdef HAVE_XDP_FRAME_BULK
				  , struct xdp_frame_bulk *bq
#endif
				  , struct mlx5e_cq *cq,
				  struct mlx5_cqe64 *cqe)
{
	struct mlx5e_xdp_info_fifo *xdpi_fifo = &sq->db.xdpi_fifo;
	u16 i;

	for (i = 0; i < wi->num_pkts; i++) {
		union mlx5e_xdp_info xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);

		switch (xdpi.mode) {
		case MLX5E_XDP_XMIT_MODE_FRAME: {
			/* XDP_TX from the XSK RQ and XDP_REDIRECT */
			struct xdp_frame *xdpf;
			dma_addr_t dma_addr;

			xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
			xdpf = xdpi.frame.xdpf;
			xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
			dma_addr = xdpi.frame.dma_addr;

			dma_unmap_single(sq->pdev, dma_addr,
					 xdpf->len, DMA_TO_DEVICE);
#ifdef HAVE_XDP_HAS_FRAGS
			if (xdp_frame_has_frags(xdpf)) {
				struct skb_shared_info *sinfo;
				int j;

				sinfo = xdp_get_shared_info_from_frame(xdpf);
				for (j = 0; j < sinfo->nr_frags; j++) {
					skb_frag_t *frag = &sinfo->frags[j];

					xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
					dma_addr = xdpi.frame.dma_addr;

					dma_unmap_single(sq->pdev, dma_addr,
							 skb_frag_size(frag), DMA_TO_DEVICE);
				}
			}
#endif
#ifdef HAVE_XDP_FRAME_BULK
			xdp_return_frame_bulk(xdpf, bq);
#else
			/* Assumes order0 page*/
			put_page(virt_to_page(xdpf->data));
#endif
			break;
		}
		case MLX5E_XDP_XMIT_MODE_PAGE: {
			/* XDP_TX from the regular RQ */
			u8 num, n = 0;
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
			struct mlx5e_rq *rq = NULL;

			xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
			rq = xdpi.page.rq;
#endif

			xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
			num = xdpi.page.num;

			do {
				xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);

#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
				/* No need to check ((page->pp_magic & ~0x3UL) == PP_SIGNATURE)
				 * as we know this is a page_pool page.
				 */
				page_pool_recycle_direct(xdpi.page.page->pp, xdpi.page.page);
#else
				mlx5e_page_release_dynamic(rq, &xdpi.page.au, recycle);
#endif
			} while (++n < num);

			break;
		}
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
		case MLX5E_XDP_XMIT_MODE_XSK: {
			/* AF_XDP send */
#ifdef HAVE_XSK_TX_METADATA_OPS
			struct xsk_tx_metadata_compl *compl = NULL;
			struct mlx5e_xsk_tx_complete priv = {
				.cqe = cqe,
				.cq = cq,
			};

			if (xp_tx_metadata_enabled(sq->xsk_pool)) {
				xdpi = mlx5e_xdpi_fifo_pop(xdpi_fifo);
				compl = &xdpi.xsk_meta;

				xsk_tx_metadata_complete(compl, &mlx5e_xsk_tx_metadata_ops, &priv);
			}
#endif

			(*xsk_frames)++;
			break;
		}
#endif
		default:
			WARN_ON_ONCE(true);
		}
	}
}

bool mlx5e_poll_xdpsq_cq(struct mlx5e_cq *cq)
{
#ifdef HAVE_XDP_FRAME_BULK
	struct xdp_frame_bulk bq;
#endif
	struct mlx5e_xdpsq *sq;
	struct mlx5_cqe64 *cqe;
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
	u32 xsk_frames = 0;
#endif
	u16 sqcc;
	int i;

#ifdef HAVE_XDP_FRAME_BULK
	xdp_frame_bulk_init(&bq);
#endif

	sq = container_of(cq, struct mlx5e_xdpsq, cq);

	if (unlikely(!test_bit(MLX5E_SQ_STATE_ENABLED, &sq->state)))
		return false;

	cqe = mlx5_cqwq_get_cqe(&cq->wq);
	if (!cqe)
		return false;

	/* sq->cc must be updated only after mlx5_cqwq_update_db_record(),
	 * otherwise a cq overrun may occur
	 */
	sqcc = sq->cc;

	i = 0;
	do {
		struct mlx5e_xdp_wqe_info *wi;
		u16 wqe_counter, ci;
		bool last_wqe;

		mlx5_cqwq_pop(&cq->wq);

		wqe_counter = be16_to_cpu(cqe->wqe_counter);

		do {
			last_wqe = (sqcc == wqe_counter);
			ci = mlx5_wq_cyc_ctr2ix(&sq->wq, sqcc);
			wi = &sq->db.wqe_info[ci];

			sqcc += wi->num_wqebbs;

			mlx5e_free_xdpsq_desc(sq, wi
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
					     , &xsk_frames
#endif
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
					     , true 
#endif
#ifdef HAVE_XDP_FRAME_BULK
					     , &bq
#endif
					     , cq, cqe);
		} while (!last_wqe);

		if (unlikely(get_cqe_opcode(cqe) != MLX5_CQE_REQ)) {
			netdev_WARN_ONCE(sq->channel->netdev,
					 "Bad OP in XDPSQ CQE: 0x%x\n",
					 get_cqe_opcode(cqe));
			mlx5e_dump_error_cqe(&sq->cq, sq->sqn,
					     (struct mlx5_err_cqe *)cqe);
			mlx5_wq_cyc_wqe_dump(&sq->wq, ci, wi->num_wqebbs);
		}
	} while ((++i < MLX5E_TX_CQ_POLL_BUDGET) && (cqe = mlx5_cqwq_get_cqe(&cq->wq)));

#ifdef HAVE_XDP_FRAME_BULK
	xdp_flush_frame_bulk(&bq);
#endif

#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
	if (xsk_frames)
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
		xsk_tx_completed(sq->xsk_pool, xsk_frames);
#else
		xsk_umem_complete_tx(sq->umem, xsk_frames);
#endif
#endif

	sq->stats->cqes += i;

	mlx5_cqwq_update_db_record(&cq->wq);

	/* ensure cq space is freed before enabling more cqes */
	wmb();

	sq->cc = sqcc;
	return (i == MLX5E_TX_CQ_POLL_BUDGET);
}

void mlx5e_free_xdpsq_descs(struct mlx5e_xdpsq *sq)
{
#ifdef HAVE_XDP_FRAME_BULK
	struct xdp_frame_bulk bq;
#endif
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
	u32 xsk_frames = 0;
#endif

#ifdef HAVE_XDP_FRAME_BULK
	xdp_frame_bulk_init(&bq);

	rcu_read_lock(); /* need for xdp_return_frame_bulk */
#endif

	while (sq->cc != sq->pc) {
		struct mlx5e_xdp_wqe_info *wi;
		u16 ci;

		ci = mlx5_wq_cyc_ctr2ix(&sq->wq, sq->cc);
		wi = &sq->db.wqe_info[ci];

		sq->cc += wi->num_wqebbs;

		mlx5e_free_xdpsq_desc(sq, wi
#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
				     , &xsk_frames
#endif
#ifndef HAVE_PAGE_POOL_DEFRAG_PAGE
				     , false
#endif
#ifdef HAVE_XDP_FRAME_BULK
				     , &bq
#endif
				     , NULL, NULL);
	}

#ifdef HAVE_XDP_FRAME_BULK
	xdp_flush_frame_bulk(&bq);
	rcu_read_unlock();
#endif

#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
	if (xsk_frames)
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
		xsk_tx_completed(sq->xsk_pool, xsk_frames);
#else
		xsk_umem_complete_tx(sq->umem, xsk_frames);
#endif
#endif
}

void mlx5e_xdp_rx_poll_complete(struct mlx5e_rq *rq)
{
	struct mlx5e_xdpsq *xdpsq = rq->xdpsq;

	if (xdpsq->mpwqe.wqe)
		mlx5e_xdp_mpwqe_complete(xdpsq);

	mlx5e_xmit_xdp_doorbell(xdpsq);
	if (test_bit(MLX5E_RQ_FLAG_XDP_REDIRECT, rq->flags)) {
#ifndef HAVE_XDP_DO_FLUSH_MAP
		xdp_do_flush();
#else
		xdp_do_flush_map();
#endif
		__clear_bit(MLX5E_RQ_FLAG_XDP_REDIRECT, rq->flags);
	}
}

void mlx5e_set_xmit_fp(struct mlx5e_xdpsq *sq, bool is_mpw)
{
	sq->xmit_xdp_frame_check = is_mpw ?
		mlx5e_xmit_xdp_frame_check_mpwqe : mlx5e_xmit_xdp_frame_check;
	sq->xmit_xdp_frame = is_mpw ?
		mlx5e_xmit_xdp_frame_mpwqe : mlx5e_xmit_xdp_frame;
}

int mlx5e_xdp_xmit(struct net_device *dev, int n, struct xdp_frame **frames,
		   u32 flags)
{
	struct mlx5e_priv *priv = netdev_priv(dev);
	struct mlx5e_xdpsq *sq;
	int nxmit = 0;
	int sq_num;
	int i;

	/* this flag is sufficient, no need to test internal sq state */
	if (unlikely(!mlx5e_xdp_tx_is_enabled(priv)))
		return -ENETDOWN;

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK))
		return -EINVAL;

	sq_num = smp_processor_id();

	if (unlikely(sq_num >= priv->channels.num))
		return -ENXIO;

	sq = &priv->channels.c[sq_num]->xdpsq;

	for (i = 0; i < n; i++) {
		struct mlx5e_xmit_data_frags xdptxdf = {};
		struct xdp_frame *xdpf = frames[i];
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		dma_addr_t dma_arr[MAX_SKB_FRAGS];
#endif
		struct mlx5e_xmit_data *xdptxd;
		bool ret;

		xdptxd = &xdptxdf.xd;
		xdptxd->data = xdpf->data;
		xdptxd->len = xdpf->len;
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		xdptxd->has_frags = xdp_frame_has_frags(xdpf);
#else
		xdptxd->has_frags = false;
#endif
		xdptxd->dma_addr = dma_map_single(sq->pdev, xdptxd->data,
						  xdptxd->len, DMA_TO_DEVICE);

		if (unlikely(dma_mapping_error(sq->pdev, xdptxd->dma_addr)))
			break;

#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		if (xdptxd->has_frags) {
			int j;

			xdptxdf.sinfo = xdp_get_shared_info_from_frame(xdpf);
			xdptxdf.dma_arr = dma_arr;
			for (j = 0; j < xdptxdf.sinfo->nr_frags; j++) {
				skb_frag_t *frag = &xdptxdf.sinfo->frags[j];

				dma_arr[j] = dma_map_single(sq->pdev, skb_frag_address(frag),
							    skb_frag_size(frag), DMA_TO_DEVICE);

				if (!dma_mapping_error(sq->pdev, dma_arr[j]))
					continue;
				/* mapping error */
				while (--j >= 0)
					dma_unmap_single(sq->pdev, dma_arr[j],
							 skb_frag_size(&xdptxdf.sinfo->frags[j]),
							 DMA_TO_DEVICE);
				goto out;
			}
		}
#endif

		ret = INDIRECT_CALL_2(sq->xmit_xdp_frame, mlx5e_xmit_xdp_frame_mpwqe,
				      mlx5e_xmit_xdp_frame, sq, xdptxd, 0, NULL);
		if (unlikely(!ret)) {
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
			int j;
#endif

			dma_unmap_single(sq->pdev, xdptxd->dma_addr,
					 xdptxd->len, DMA_TO_DEVICE);
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
			if (!xdptxd->has_frags)
				break;
			for (j = 0; j < xdptxdf.sinfo->nr_frags; j++)
				dma_unmap_single(sq->pdev, dma_arr[j],
						 skb_frag_size(&xdptxdf.sinfo->frags[j]),
						 DMA_TO_DEVICE);
#endif
			break;
		}

		/* xmit_mode == MLX5E_XDP_XMIT_MODE_FRAME */
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .mode = MLX5E_XDP_XMIT_MODE_FRAME });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .frame.xdpf = xdpf });
		mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
				     (union mlx5e_xdp_info) { .frame.dma_addr = xdptxd->dma_addr });
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
		if (xdptxd->has_frags) {
			int j;

			for (j = 0; j < xdptxdf.sinfo->nr_frags; j++)
				mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo,
						     (union mlx5e_xdp_info)
						     { .frame.dma_addr = dma_arr[j] });
		}
#endif
		nxmit++;
	}
#ifdef HAVE_PAGE_POOL_DEFRAG_PAGE
out:
#endif
	if (sq->mpwqe.wqe)
		mlx5e_xdp_mpwqe_complete(sq);

	if (flags & XDP_XMIT_FLUSH)
		mlx5e_xmit_xdp_doorbell(sq);

	return nxmit;
}
#endif

