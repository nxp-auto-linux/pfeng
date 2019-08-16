/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include "pfeng.h"

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/phy.h>

static void pfeng_ethtool_getdrvinfo(struct net_device *dev,
                      struct ethtool_drvinfo *info)
{
    //struct pfeng_priv *priv = netdev_priv(dev);

	strlcpy(info->driver, PFENG_DRIVER_NAME, sizeof(info->version));
	//TODO: info->version
	//TODO: info->fw_version
}

static const struct ethtool_ops pfeng_ethtool_ops = {
    //.begin = pfeng_check_if_running,
    .get_drvinfo = pfeng_ethtool_getdrvinfo,
    //.get_msglevel = pfeng_ethtool_getmsglevel,
    //.set_msglevel = pfeng_ethtool_setmsglevel,
    //.get_regs = pfeng_ethtool_gregs,
    //.get_regs_len = pfeng_ethtool_get_regs_len,
    .get_link = ethtool_op_get_link,
    .nway_reset = phy_ethtool_nway_reset,
    //.get_pauseparam = pfeng_get_pauseparam,
    //.set_pauseparam = pfeng_set_pauseparam,
    //.get_ethtool_stats = pfeng_get_ethtool_stats,
    //.get_strings = pfeng_get_strings,
    //.get_wol = pfeng_get_wol,
    //.set_wol = pfeng_set_wol,
    //.get_eee = pfeng_ethtool_op_get_eee,
    //.set_eee = pfeng_ethtool_op_set_eee,
    //.get_sset_count = pfeng_get_sset_count,
    //.get_ts_info = pfeng_get_ts_info,
    //.get_coalesce = pfeng_get_coalesce,
    //.set_coalesce = pfeng_set_coalesce,
    //.get_tunable = pfeng_get_tunable,
    //.set_tunable = pfeng_set_tunable,
    //.get_link_ksettings = pfeng_ethtool_get_link_ksettings,
    //.set_link_ksettings = pfeng_ethtool_set_link_ksettings,
};

void pfeng_ethtool_set_ops(struct net_device *netdev)
{
    netdev->ethtool_ops = &pfeng_ethtool_ops;
}
