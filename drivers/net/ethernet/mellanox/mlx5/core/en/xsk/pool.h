/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/* Copyright (c) 2019-2020, Mellanox Technologies inc. All rights reserved. */

#ifndef __MLX5_EN_XSK_POOL_H__
#define __MLX5_EN_XSK_POOL_H__

#ifdef HAVE_XSK_ZERO_COPY_SUPPORT
#include "en.h"

#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
static inline struct xsk_buff_pool *mlx5e_xsk_get_pool(struct mlx5e_params *params,
						       struct mlx5e_xsk *xsk, u16 ix)
{
	if (!xsk || !xsk->pools)
		return NULL;

	if (unlikely(ix >= params->num_channels))
		return NULL;

	return xsk->pools[ix];
}
#else
static inline struct xdp_umem *mlx5e_xsk_get_pool(struct mlx5e_params *params,
						       struct mlx5e_xsk *xsk, u16 ix)
{
	if (!xsk || !xsk->umems)
		return NULL;

	if (unlikely(ix >= params->num_channels))
		return NULL;

	return xsk->umems[ix];
}
#endif

struct mlx5e_xsk_param;

/* .ndo_bpf callback. */
#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
void mlx5e_build_xsk_param(struct xsk_buff_pool *pool, struct mlx5e_xsk_param *xsk);
int mlx5e_xsk_setup_pool(struct net_device *dev, struct xsk_buff_pool *pool, u16 qid);
#else
void mlx5e_build_xsk_param(struct xdp_umem *umem, struct mlx5e_xsk_param *xsk);
int mlx5e_xsk_setup_pool(struct net_device *dev, struct xdp_umem *pool, u16 qid);
#endif

#ifndef HAVE_XSK_BUFF_ALLOC
int mlx5e_xsk_resize_reuseq(struct xdp_umem *umem, u32 nentries);
#endif

#endif
#endif /* __MLX5_EN_XSK_POOL_H__ */
