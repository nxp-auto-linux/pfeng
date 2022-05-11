/*
 * Copyright 2018-2022 NXP
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
#ifdef PFE_CFG_PFE_MASTER
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_ct_version_t fwver_class, fwver_util;
#endif

	/* driver */
	strlcpy(info->driver, PFENG_DRIVER_NAME, sizeof(info->version));
	strlcpy(info->version, PFENG_DRIVER_VERSION, sizeof(info->version));

#ifdef PFE_CFG_PFE_MASTER
	/* fw_version */
	pfe_platform_get_fw_versions(netif->priv->pfe_platform, &fwver_class, &fwver_util);
	scnprintf(info->fw_version, sizeof(info->fw_version), "%u.%u.%u-%u.%u.%u api:%.8s",
			fwver_class.major, fwver_class.minor, fwver_class.patch,
			fwver_util.major, fwver_util.minor, fwver_util.patch,
			fwver_class.cthdr);
#endif
}

static int pfeng_ethtool_get_link_ksettings(struct net_device *netdev, struct ethtool_link_ksettings *cmd)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	if (netif->phylink)
		return phylink_ethtool_ksettings_get(netif->phylink, cmd);

	/* Generic values */
	cmd->base.autoneg = AUTONEG_DISABLE;
	cmd->base.duplex = DUPLEX_HALF;
	cmd->base.speed = SPEED_10;

	return 0;
}

static int pfeng_ethtool_set_link_ksettings(struct net_device *netdev, const struct ethtool_link_ksettings *cmd)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	if (!netif->phylink)
		return -ENOTSUPP;

	return phylink_ethtool_ksettings_set(netif->phylink, cmd);
}

static void pfeng_ethtool_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *epauseparm)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	bool_t rx_pause, tx_pause;

	pfe_phy_if_get_flow_control(netif->priv->emac[netif->cfg->emac_id].phyif_emac, &tx_pause, &rx_pause);
	epauseparm->rx_pause = rx_pause;
	epauseparm->tx_pause = tx_pause;
	epauseparm->autoneg = AUTONEG_DISABLE;

}

static int pfeng_ethtool_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *epauseparm)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	if (epauseparm->autoneg)
		return -EOPNOTSUPP;

	pfe_phy_if_set_tx_flow_control(netif->priv->emac[netif->cfg->emac_id].phyif_emac, epauseparm->tx_pause);
	pfe_phy_if_set_rx_flow_control(netif->priv->emac[netif->cfg->emac_id].phyif_emac, epauseparm->rx_pause);

	return 0;
}

static int pfeng_ethtool_nway_reset(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	if (!netif->phylink)
		return -ENOTSUPP;

	return phylink_ethtool_nway_reset(netif->phylink);
}

static int pfeng_ethtool_get_ts_info(struct net_device *netdev, struct ethtool_ts_info *info)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	ethtool_op_get_ts_info(netdev, info);

	pfeng_hwts_ethtool(netif, info);

	if (netif->ptp_clock)
		info->phc_index = ptp_clock_index(netif->ptp_clock);
	else
		netdev_info(netdev, "No PTP clock available\n");

	return 0;
}

static int pfeng_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *ec)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_hif_chnl *chnl;
	u32 frames = 0, cycles = 0;
	int ret, idx = ffs(netif->cfg->hifmap) - 1;

	/* All HIF channels are using the same setting, so use first one */
	chnl = &netif->priv->hif_chnl[idx];

	ret = pfe_hif_chnl_get_rx_irq_coalesce(chnl->priv, &frames, &cycles);
	if (ret)
		return -ret;

	ec->rx_max_coalesced_frames = frames;
	ec->rx_coalesce_usecs = DIV_ROUND_UP(cycles, DIV_ROUND_UP(clk_get_rate(netif->priv->clk_sys), USEC_PER_SEC));

	return 0;
}

static int pfeng_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *ec)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_hif_chnl *chnl;
	int ret = 0;
	u32 idx;

	/* Right now we only support two modes:
	 * 1) disabled coalescing
	 * 2) time-triggered coalescing
	 *
	 * Note: Frame count triggered coalescing is not supported on S32G2 silicon
	 */
	if (ec->rx_max_coalesced_frames > 0 && ec->rx_coalesce_usecs == 0) {
		netdev_err(netif->netdev, "Frame based coalescing is unsupported\n");
		return -EINVAL;
	}

	/* Setup all linked HIF channel */
	for (idx = 0; idx < PFENG_PFE_HIF_CHANNELS; idx++) {
		if (!(netif->cfg->hifmap & (1 << idx)))
			continue;

		chnl = &netif->priv->hif_chnl[idx];
		ret = pfeng_hif_chnl_set_coalesce(chnl, netif->priv->clk_sys, ec->rx_coalesce_usecs, 0);
		if (ret)
			break;
	}

	return ret;
}

static int pfeng_ethtool_begin(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	return pm_runtime_resume_and_get(&netif->priv->pdev->dev);
}

static void pfeng_ethtool_complete(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	pm_runtime_put(&netif->priv->pdev->dev);
}

static const struct ethtool_ops pfeng_ethtool_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
	.supported_coalesce_params = ETHTOOL_COALESCE_RX_USECS,
#endif
	.get_drvinfo = pfeng_ethtool_getdrvinfo,
	.get_link = ethtool_op_get_link,
	.nway_reset = pfeng_ethtool_nway_reset,
	.get_pauseparam = pfeng_ethtool_get_pauseparam,
	.set_pauseparam = pfeng_ethtool_set_pauseparam,
	.get_link_ksettings = pfeng_ethtool_get_link_ksettings,
	.set_link_ksettings = pfeng_ethtool_set_link_ksettings,
	.get_ts_info = pfeng_ethtool_get_ts_info,
	.get_coalesce = pfeng_get_coalesce,
	.set_coalesce = pfeng_set_coalesce,
	.begin = pfeng_ethtool_begin,
	.complete = pfeng_ethtool_complete,
};

void pfeng_ethtool_init(struct net_device *netdev)
{
	netdev->ethtool_ops = &pfeng_ethtool_ops;
}

int pfeng_ethtool_params_save(struct pfeng_netif *netif) {
	struct net_device *netdev = netif->netdev;
	struct ethtool_pauseparam epp;

	/* Coalesce (saved in pfeng_hif_chnl_set_coalesce()) */

	/* Pause */
	pfeng_ethtool_get_pauseparam(netdev, &epp);
	netif->cfg->pause_tx = epp.tx_pause;
	netif->cfg->pause_rx = epp.rx_pause;

	return 0;
}

int pfeng_ethtool_params_restore(struct pfeng_netif *netif) {
	struct net_device *netdev = netif->netdev;
	struct pfeng_hif_chnl *chnl;
	struct ethtool_coalesce ec;
	struct ethtool_pauseparam epp;
	int ret, idx = ffs(netif->cfg->hifmap) - 1;

	/* Coalesce */
	chnl = &netif->priv->hif_chnl[idx];
	ec.rx_max_coalesced_frames = chnl->cfg_rx_max_coalesced_frames;
	ec.rx_coalesce_usecs = chnl->cfg_rx_coalesce_usecs;

	ret = pfeng_set_coalesce(netdev, &ec);
	if (ret)
		netdev_warn(netdev, "Coalescing not restored\n");

	/* Pause */
	epp.tx_pause = netif->cfg->pause_tx;
	epp.rx_pause = netif->cfg->pause_rx;
	epp.autoneg = AUTONEG_DISABLE;
	ret = pfeng_ethtool_set_pauseparam(netdev, &epp);
	if (ret)
		netdev_warn(netdev, "Pause not restored\n");

	return 0;
}
