// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2020 Mellanox Technologies Ltd */

#include <linux/mlx5/driver.h>
#include <linux/mlx5/device.h>
#include <linux/mlx5/eswitch.h>
#include "mlx5_core.h"
#include "mlx5_devm.h"
#include "dev.h"
#include "devlink.h"
#include "cfg_driver.h"

# ifdef HAVE_DEVLINK_INSTANCES_RELATIONSHIPS_EXPOSURE
static int mlx5_core_peer_devlink_set(struct mlx5_sf_dev *sf_dev, struct devlink *devlink)
{
	struct mlx5_sf_peer_devlink_event_ctx event_ctx = {
		.fn_id = sf_dev->fn_id,
		.devlink = devlink,
	};
	int ret;

	ret = mlx5_blocking_notifier_call_chain(sf_dev->parent_mdev,
						MLX5_DRIVER_EVENT_SF_PEER_DEVLINK,
						&event_ctx);
	return ret == NOTIFY_OK ? event_ctx.err : 0;
}
#endif

#if !defined(HAVE_AUX_DEV_IRQS_SYSFS) && !defined(CONFIG_AUXILIARY_BUS)
static void mlx5_sf_dev_sysfs_probe(struct mlx5_sf_dev *sf_dev)
{
	sf_dev->sysfs.irq_dir_exists = false;
}
#endif

static int mlx5_sf_dev_probe(struct auxiliary_device *adev, const struct auxiliary_device_id *id)
{
	struct mlx5_sf_dev *sf_dev = container_of(adev, struct mlx5_sf_dev, adev);
	struct mlx5_core_dev *mdev;
	struct devlink *devlink;
	int err;

#if !defined(HAVE_AUX_DEV_IRQS_SYSFS) && !defined(CONFIG_AUXILIARY_BUS)
	mlx5_sf_dev_sysfs_probe(sf_dev);
#endif
	devlink = mlx5_devlink_alloc(&adev->dev);
	if (!devlink)
		return -ENOMEM;

	mdev = devlink_priv(devlink);
	mdev->device = &adev->dev;
	mdev->pdev = sf_dev->parent_mdev->pdev;
	mdev->bar_addr = sf_dev->bar_base_addr;
	mdev->iseg_base = sf_dev->bar_base_addr;
	mdev->coredev_type = MLX5_COREDEV_SF;
	mdev->priv.parent_mdev = sf_dev->parent_mdev;
	mdev->priv.adev_idx = adev->id;
	sf_dev->mdev = mdev;

#if defined(HAVE_LIGHT_SFS)
	/* Only local SFs do light probe */
	if (MLX5_ESWITCH_MANAGER(sf_dev->parent_mdev) &&
	    !mlx5_devm_is_devm_sf(sf_dev->parent_mdev, sf_dev->sfnum))
		mlx5_dev_set_lightweight(mdev);
#endif

	err = mlx5_mdev_init(mdev, MLX5_SF_PROF);
	if (err) {
		mlx5_core_warn(mdev, "mlx5_mdev_init on err=%d\n", err);
		goto mdev_err;
	}

#ifdef CONFIG_MLX5_SF_CFG
	mdev->disable_en = sf_dev->disable_netdev;
	mdev->disable_fc = sf_dev->disable_fc;
	mdev->cmpl_eq_depth = sf_dev->cmpl_eq_depth;
	mdev->async_eq_depth = sf_dev->async_eq_depth;
	mdev->max_cmpl_eq_count = sf_dev->max_cmpl_eqs;
#endif

	mdev->iseg = ioremap(mdev->iseg_base, sizeof(*mdev->iseg));
	if (!mdev->iseg) {
		mlx5_core_warn(mdev, "remap error\n");
		err = -ENOMEM;
		goto remap_err;
	}

#ifdef HAVE_DEVLINK_INSTANCES_RELATIONSHIPS_EXPOSURE
	/* Peer devlink logic expects to work on unregistered devlink instance. */
	err = mlx5_core_peer_devlink_set(sf_dev, devlink);
	if (err) {
		mlx5_core_warn(mdev, "mlx5_core_peer_devlink_set err=%d\n", err);
		goto peer_devlink_set_err;
	}
#endif

#if defined(HAVE_LIGHT_SFS)
	if (mlx5_dev_is_lightweight(mdev))
		err = mlx5_init_one_light(mdev);
	else
#endif
		err = mlx5_init_one(mdev);
	if (err) {
		mlx5_core_warn(mdev, "mlx5_init_one err=%d\n", err);
		goto init_one_err;
	}

#if defined(HAVE_DEVLINK_REGISTER_GET_1_PARAMS) && !defined(HAVE_DEVL_REGISTER)
	devlink_register(devlink);
#endif
#if defined(HAVE_DEVLINK_RELOAD_ENABLE) && !defined(HAVE_DEVLINK_SET_FEATURES)
	devlink_reload_enable(devlink);
#endif
	return 0;

init_one_err:
#ifdef HAVE_DEVLINK_INSTANCES_RELATIONSHIPS_EXPOSURE
peer_devlink_set_err:
#endif
	iounmap(mdev->iseg);
remap_err:
	mlx5_mdev_uninit(mdev);
mdev_err:
	mlx5_devlink_free(devlink);
	return err;
}

static void mlx5_sf_dev_remove(struct auxiliary_device *adev)
{
	struct mlx5_sf_dev *sf_dev = container_of(adev, struct mlx5_sf_dev, adev);
	struct mlx5_core_dev *mdev = sf_dev->mdev;
	struct devlink *devlink;

	devlink = priv_to_devlink(mdev);
	set_bit(MLX5_BREAK_FW_WAIT, &mdev->intf_state);
	mlx5_drain_health_wq(mdev);
	set_bit(MLX5_BREAK_FW_WAIT, &sf_dev->mdev->intf_state);
#if defined(HAVE_DEVLINK_RELOAD_DISABLE) && !defined(HAVE_DEVLINK_SET_FEATURES)
	devlink_reload_disable(devlink);
#endif
#if defined(HAVE_DEVLINK_REGISTER_GET_1_PARAMS) && !defined(HAVE_DEVL_REGISTER)
	devlink_unregister(devlink);
#endif
#if defined(HAVE_LIGHT_SFS)
	if (mlx5_dev_is_lightweight(mdev))
		mlx5_uninit_one_light(mdev);
	else
#endif
		mlx5_uninit_one(mdev);
	iounmap(mdev->iseg);
	mlx5_mdev_uninit(mdev);
	mlx5_devlink_free(devlink);
}

static void mlx5_sf_dev_shutdown(struct auxiliary_device *adev)
{
	struct mlx5_sf_dev *sf_dev = container_of(adev, struct mlx5_sf_dev, adev);
	struct mlx5_core_dev *mdev = sf_dev->mdev;

	set_bit(MLX5_BREAK_FW_WAIT, &mdev->intf_state);
	mlx5_drain_health_wq(mdev);
	mlx5_unload_one(mdev, false);
}

static const struct auxiliary_device_id mlx5_sf_dev_id_table[] = {
	{ .name = MLX5_ADEV_NAME "." MLX5_SF_DEV_ID_NAME, },
	{ },
};

MODULE_DEVICE_TABLE(auxiliary, mlx5_sf_dev_id_table);

static struct auxiliary_driver mlx5_sf_driver = {
	.name = MLX5_SF_DEV_ID_NAME,
	.probe = mlx5_sf_dev_probe,
	.remove = mlx5_sf_dev_remove,
	.shutdown = mlx5_sf_dev_shutdown,
	.id_table = mlx5_sf_dev_id_table,
};

int mlx5_sf_driver_register(void)
{
	int err;

	err = mlx5_sf_cfg_driver_register();
	if (err)
		return err;

	err = auxiliary_driver_register(&mlx5_sf_driver);
	if (err)
		goto err;

	return 0;
err:
	mlx5_sf_cfg_driver_unregister();
	return err;
}

void mlx5_sf_driver_unregister(void)
{
	auxiliary_driver_unregister(&mlx5_sf_driver);
	mlx5_sf_cfg_driver_unregister();
}
