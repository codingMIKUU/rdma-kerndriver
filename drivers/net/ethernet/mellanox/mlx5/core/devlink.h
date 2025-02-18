/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/* Copyright (c) 2019, Mellanox Technologies */

#ifndef __MLX5_DEVLINK_H__
#define __MLX5_DEVLINK_H__

#include <net/devlink.h>

enum mlx5_devlink_resource_id {
	MLX5_DL_RES_MAX_LOCAL_SFS = 1,
	MLX5_DL_RES_MAX_EXTERNAL_SFS,

	__MLX5_ID_RES_MAX,
	MLX5_ID_RES_MAX = __MLX5_ID_RES_MAX - 1,
};

enum mlx5_devlink_param_id {
	MLX5_DEVLINK_PARAM_ID_BASE = DEVLINK_PARAM_GENERIC_ID_MAX,
	MLX5_DEVLINK_PARAM_ID_FLOW_STEERING_MODE,
	MLX5_DEVLINK_PARAM_ID_ESW_LARGE_GROUP_NUM,
	MLX5_DEVLINK_PARAM_ID_CT_ACTION_ON_NAT_CONNS,
	MLX5_DEVLINK_PARAM_ID_CT_MAX_OFFLOADED_CONNS,
	MLX5_DEVLINK_PARAM_ID_ESW_PET_INSERT,
	MLX5_DEVLINK_PARAM_ID_ESW_PORT_METADATA,
	MLX5_DEVLINK_PARAM_ID_ESW_MULTIPORT,
	MLX5_DEVLINK_PARAM_ID_HAIRPIN_NUM_QUEUES,
	MLX5_DEVLINK_PARAM_ID_HAIRPIN_QUEUE_SIZE,
#ifndef HAVE_DEVLINK_PARAM_GENERIC_ID_ENABLE_REMOTE_DEV_RESET
	MLX5_DEVLINK_PARAM_ID_ENABLE_REMOTE_DEV_RESET,
#endif
};

struct mlx5_trap_ctx {
	int id;
	int action;
};

struct mlx5_devlink_trap {
	struct mlx5_trap_ctx trap;
	void *item;
	struct list_head list;
};

struct mlx5_devlink_trap_event_ctx {
	struct mlx5_trap_ctx *trap;
	int err;
};

#ifdef HAVE_DEVLINK_TRAP_SUPPORT
struct mlx5_core_dev;
void mlx5_devlink_trap_report(struct mlx5_core_dev *dev, int trap_id, struct sk_buff *skb,
			      struct devlink_port *dl_port);
int mlx5_devlink_trap_get_num_active(struct mlx5_core_dev *dev);
int mlx5_devlink_traps_get_action(struct mlx5_core_dev *dev, int trap_id,
				  enum devlink_trap_action *action);
int mlx5_devlink_traps_register(struct devlink *devlink);
void mlx5_devlink_traps_unregister(struct devlink *devlink);
#endif

struct devlink *mlx5_devlink_alloc(struct device *dev);
void mlx5_devlink_free(struct devlink *devlink);
#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
int mlx5_devlink_params_register(struct devlink *devlink);
#else
int mlx5_devlink_params_register(struct devlink *devlink, struct device *pdev);
#endif
void mlx5_devlink_params_unregister(struct devlink *devlink);
int
mlx5_devlink_ct_action_on_nat_conns_set(struct devlink *devlink, u32 id,
#ifdef HAVE_DEVLINK_PARAM_SET_FUNCTION_POINTER_HAS_EXTACK
					struct devlink_param_gset_ctx *ctx,
					struct netlink_ext_ack *extack);
#else
					struct devlink_param_gset_ctx *ctx);
#endif
int
mlx5_devlink_ct_action_on_nat_conns_get(struct devlink *devlink, u32 id,
					struct devlink_param_gset_ctx *ctx);

#ifdef HAVE_DEVLINK_PARAM_GENERIC_ID_ENABLE_ETH
static inline bool mlx5_core_is_eth_enabled(struct mlx5_core_dev *dev)
{
#if defined(HAVE_DEVLINK_PARAM_REGISTER) || defined(HAVE_DEVL_PARAM_DRIVERINIT_VALUE_GET)
	union devlink_param_value val;
	int err;

#ifdef HAVE_DEVL_PARAM_DRIVERINIT_VALUE_GET
	err = devl_param_driverinit_value_get(priv_to_devlink(dev),
#else
	err = devlink_param_driverinit_value_get(priv_to_devlink(dev),
#endif
					      DEVLINK_PARAM_GENERIC_ID_ENABLE_ETH,
					      &val);
	return err ? false : val.vbool;
#else
	return true;
#endif
}
#endif

int
mlx5_devlink_ct_labels_mapping_set(struct devlink *devlink, u32 id,
#ifdef HAVE_DEVLINK_PARAM_SET_FUNCTION_POINTER_HAS_EXTACK
				   struct devlink_param_gset_ctx *ctx,
				   struct netlink_ext_ack *extack);
#else
				   struct devlink_param_gset_ctx *ctx);
#endif
int
mlx5_devlink_ct_labels_mapping_get(struct devlink *devlink, u32 id,
				   struct devlink_param_gset_ctx *ctx);
#endif /* __MLX5_DEVLINK_H__ */
