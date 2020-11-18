/*
 * Copyright 2018-2020 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "pfe_cfg.h"
#include "pfeng.h"

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/phylink.h>

static void pfeng_ethtool_getdrvinfo(struct net_device *netdev, struct ethtool_drvinfo *info)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
	pfe_ct_version_t fwver_class, fwver_util;

	/* driver */
	strlcpy(info->driver, PFENG_DRIVER_NAME, sizeof(info->version));

	/* fw_version */
	pfe_platform_get_fw_versions(ndev->priv->pfe, &fwver_class, &fwver_util);
	scnprintf(info->fw_version, sizeof(info->fw_version), "%u.%u.%u-%u.%u.%u api:%.8s",
			fwver_class.major, fwver_class.minor, fwver_class.patch,
			fwver_util.major, fwver_util.minor, fwver_util.patch,
			fwver_class.cthdr);
}

static int pfeng_ethtool_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	if (ndev->phylink)
		return phylink_ethtool_ksettings_get(ndev->phylink, cmd);

	/* Generic values */
	cmd->base.autoneg = AUTONEG_DISABLE;
	cmd->base.duplex = DUPLEX_HALF;
	cmd->base.speed = SPEED_10;

	return 0;
}

static int pfeng_ethtool_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	if (!ndev->phylink)
		return -ENOTSUPP;

	return phylink_ethtool_ksettings_set(ndev->phylink, cmd);
}

static void pfeng_ethtool_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *epauseparm)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	phylink_ethtool_get_pauseparam(ndev->phylink, epauseparm);
}

static int pfeng_ethtool_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *epauseparm)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	return phylink_ethtool_set_pauseparam(ndev->phylink, epauseparm);
}

static int pfeng_ethtool_nway_reset(struct net_device *netdev)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	if (!ndev->phylink)
		return -ENOTSUPP;

	return phylink_ethtool_nway_reset(ndev->phylink);
}

static const struct ethtool_ops pfeng_ethtool_ops = {
	.get_drvinfo = pfeng_ethtool_getdrvinfo,
	.get_link = ethtool_op_get_link,
	.nway_reset = pfeng_ethtool_nway_reset,
	.get_pauseparam = pfeng_ethtool_get_pauseparam,
	.set_pauseparam = pfeng_ethtool_set_pauseparam,
	.get_link_ksettings = pfeng_ethtool_get_link_ksettings,
	.set_link_ksettings = pfeng_ethtool_set_link_ksettings,
};

void pfeng_ethtool_init(struct net_device *netdev)
{
	netdev->ethtool_ops = &pfeng_ethtool_ops;
}
