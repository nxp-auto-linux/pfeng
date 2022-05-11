/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/phy.h>
#include <linux/phylink.h>
#include <linux/clk.h>

#include "pfe_cfg.h"
#include "pfe_cbus.h"
#include "pfe_emac.h"
#include "pfeng.h"

#define MAC_PHYIF_CTRL_STATUS	0xF8
#define EMAC_CLK_RATE_325M	325000000	/* 325MHz */
#define EMAC_CLK_RATE_125M	125000000	/* 125MHz */
#define EMAC_CLK_RATE_25M	25000000	/* 25MHz */
#define EMAC_CLK_RATE_2M5	2500000		/* 2.5MHz */

#define XPCS_POLL_MS		1000

static void pfeng_cfg_to_plat(struct pfeng_netif *netif, const struct phylink_link_state *state)
{
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];
	pfe_emac_t *pfe_emac = netif->priv->pfe_platform->emac[netif->cfg->emac_id];
	u32 emac_speed, emac_duplex;
	bool speed_valid = true, duplex_valid = true;

	switch (state->speed) {
	default:
		netdev_dbg(netif->netdev, "Speed not supported\n");
		speed_valid = false;
		return;
	case SPEED_2500:
		emac_speed = EMAC_SPEED_2500_MBPS;
		break;
	case SPEED_1000:
		emac_speed = EMAC_SPEED_1000_MBPS;
		break;
	case SPEED_100:
		emac_speed = EMAC_SPEED_100_MBPS;
		break;
	case SPEED_10:
		emac_speed = EMAC_SPEED_10_MBPS;
		break;
	}

	if (speed_valid) {
		pfe_emac_set_link_speed(pfe_emac, emac_speed);
		emac->speed = state->speed;
	}

	switch (state->duplex) {
	case DUPLEX_HALF:
		emac_duplex = EMAC_DUPLEX_HALF;
		break;
	case DUPLEX_FULL:
		emac_duplex = EMAC_DUPLEX_FULL;
		break;
	default:
		netdev_dbg(netif->netdev, "Unknown duplex\n");
		duplex_valid = false;
		return;
		break;
	}

	if (emac_duplex) {
		pfe_emac_set_link_duplex(pfe_emac, emac_duplex);
		emac->duplex = state->duplex;
	}
}

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
/* This is done automatically in phylink in 5.10 */
void pfeng_xpcs_poll(struct work_struct * work) {
	struct pfeng_netif *netif = container_of(work, struct pfeng_netif, xpcs_poll_work.work);
	struct pfeng_emac *emac  = &netif->priv->emac[netif->cfg->emac_id];
	struct phylink_link_state sgmii_state = { 0 };

	emac->xpcs_ops->xpcs_get_state(emac->xpcs, &sgmii_state);

	if (sgmii_state.duplex != emac->duplex ||
	    sgmii_state.speed != emac->speed ||
	    sgmii_state.link != emac->sgmii_link) {
		phylink_mac_change(netif->phylink, sgmii_state.link);
	}

	schedule_delayed_work(&netif->xpcs_poll_work, msecs_to_jiffies(XPCS_POLL_MS));
}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

/**
 * @brief	Validate and update the link configuration
 */
static void pfeng_phylink_validate(struct phylink_config *config, unsigned long *supported, struct phylink_link_state *state)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));
	struct pfeng_priv *priv = netif->priv;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = { 0, };
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mac_supported) = { 0, };
	int max_speed = priv->emac[netif->cfg->emac_id].max_speed;
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	int an_serdes_speed = priv->emac[netif->cfg->emac_id].serdes_an_speed;
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

	/* We only support SGMII and R/G/MII modes */
	if (state->interface != PHY_INTERFACE_MODE_NA &&
		state->interface != PHY_INTERFACE_MODE_SGMII &&
		state->interface != PHY_INTERFACE_MODE_RMII &&
		state->interface != PHY_INTERFACE_MODE_MII &&
		!phy_interface_mode_is_rgmii(state->interface)) {
		bitmap_zero(supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
		return;
	}

	phylink_set(mac_supported, Pause);
	phylink_set(mac_supported, Asym_Pause);
	phylink_set(mac_supported, Autoneg);
	phylink_set(mac_supported, 10baseT_Half);
	phylink_set(mac_supported, 10baseT_Full);

	if (max_speed > SPEED_10) {
		phylink_set(mac_supported, 100baseT_Half);
		phylink_set(mac_supported, 100baseT_Full);
		phylink_set(mac_supported, 100baseT1_Full);
	}

	if (max_speed > SPEED_100) {
		phylink_set(mac_supported, 1000baseT_Half);
		phylink_set(mac_supported, 1000baseT_Full);
		phylink_set(mac_supported, 1000baseX_Full);
	}

	if (max_speed > SPEED_1000 &&
		/* Only PFE_EMAC_0 supports 2.5G over SGMII */
		!netif->cfg->emac_id &&
		(state->interface == PHY_INTERFACE_MODE_SGMII ||
		state->interface == PHY_INTERFACE_MODE_NA)) {
		phylink_set(mac_supported, 2500baseT_Full);
		phylink_set(mac_supported, 2500baseX_Full);
	}

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	/* SGMII AN can't distinguish between 1G and 2.5G */
	if (state->interface == PHY_INTERFACE_MODE_SGMII &&
	    priv->emac[netif->cfg->emac_id].link_an == MLO_AN_INBAND) {
		if (an_serdes_speed == SPEED_2500) {
			phylink_set(mask, 10baseT_Half);
			phylink_set(mask, 10baseT_Full);
			phylink_set(mask, 100baseT_Half);
			phylink_set(mask, 100baseT_Full);
			phylink_set(mask, 100baseT1_Full);
			phylink_set(mask, 1000baseT_Half);
			phylink_set(mask, 1000baseT_Full);
			phylink_set(mask, 1000baseX_Full);
		} else if (an_serdes_speed == SPEED_1000) {
			phylink_set(mask, 2500baseT_Full);
			phylink_set(mask, 2500baseX_Full);
		}
	} else
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
	if (priv->emac[netif->cfg->emac_id].link_an == MLO_AN_FIXED) {
		phylink_clear(mac_supported, Autoneg);
	}


	phylink_set(mac_supported, MII);
	phylink_set_port_modes(mac_supported);

	bitmap_and(supported, supported, mac_supported,
		 __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_andnot(supported, supported, mask,
		 __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_and(state->advertising, state->advertising, mac_supported,
		__ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_andnot(state->advertising, state->advertising, mask,
		__ETHTOOL_LINK_MODE_MASK_NBITS);
}

/**
 * @brief	Read the current link state from the PCS
 */
static int _pfeng_mac_link_state(struct phylink_config *config, struct phylink_link_state *state)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];

	state->interface = emac->intf_mode;

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	if (state->interface != PHY_INTERFACE_MODE_SGMII || !emac->xpcs) {
		netdev_err(netif->netdev, "Configuration not supported\n");
		return -ENOTSUPP;
	}

	emac->xpcs_ops->xpcs_get_state(emac->xpcs, state);

	/* our MAC status is not connected to PCS so update it manually */
	if (emac->phyless) {
		emac->xpcs_ops->xpcs_config(emac->xpcs, state);
		pfeng_cfg_to_plat(netif, state);
		emac->sgmii_link = state->link;
	}

	return 0;
#else
	return -ENOTSUPP;
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
static void pfeng_mac_link_state(struct phylink_config *config, struct phylink_link_state *state)
{
	_pfeng_mac_link_state(config, state);
}
#else /* kernel 5.4 */
static int pfeng_mac_link_state(struct phylink_config *config, struct phylink_link_state *state)
{
	return _pfeng_mac_link_state(config, state);
}
#endif

static void pfeng_mac_an_restart(struct phylink_config *config)
{
	return;
}

/**
 * @brief	Set necessary S32G clocks
 */
static int s32g_set_rgmii_speed(struct pfeng_netif *netif, unsigned int speed)
{
	struct clk *tx_clk = netif->priv->emac[netif->cfg->emac_id].tx_clk;
	unsigned long rate = 0;
	int ret = 0;

	switch (speed) {
	default:
		netdev_dbg(netif->netdev, "Skipped clock setting\n");
		return -EINVAL;
	case SPEED_1000:
		rate = EMAC_CLK_RATE_125M;
		break;
	case SPEED_100:
		rate = EMAC_CLK_RATE_25M;
		break;
	case SPEED_10:
		rate = EMAC_CLK_RATE_2M5;
		break;
	}

	if (tx_clk) {
		ret = clk_set_rate(tx_clk, rate);
		if (ret)
			netdev_err(netif->netdev, "Unable to set TX clock to %luHz\n", rate);
		else
			netdev_info(netif->netdev, "Set TX clock to %luHz\n", rate);
	}

	return ret;
}

static void pfeng_mac_config(struct phylink_config *config, unsigned int mode, const struct phylink_link_state *state)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];
	__maybe_unused struct phylink_link_state sgmii_state = { 0 };

	if (state->speed == emac->speed &&
	    state->duplex == emac->duplex)
		return;

	if (mode == MLO_AN_FIXED || mode == MLO_AN_PHY) {
		if (phy_interface_mode_is_rgmii(emac->intf_mode)) {
			if (s32g_set_rgmii_speed(netif, state->speed))
				return;
		} else if  (emac->intf_mode == PHY_INTERFACE_MODE_SGMII) {
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
			if (!emac->xpcs || !emac->xpcs_ops)
				return;

			emac->xpcs_ops->xpcs_get_state(emac->xpcs, &sgmii_state);
			sgmii_state.speed = state->speed;
			sgmii_state.duplex = state->duplex;
			sgmii_state.an_enabled = false;
			emac->xpcs_ops->xpcs_config(emac->xpcs, &sgmii_state);
#else
			return;
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
		} else {
			netdev_err(netif->netdev, "Interface not supported\n");
			return;
		}
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	} else if (mode == MLO_AN_INBAND) {
		if (emac->intf_mode == PHY_INTERFACE_MODE_SGMII &&
		    emac->xpcs && emac->xpcs_ops) {
			emac->xpcs_ops->xpcs_config(emac->xpcs, state);
		} else {
			netdev_err(netif->netdev, "Interface not supported\n");
			return;
		}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
	} else {
		return;
	}

	pfeng_cfg_to_plat(netif, state);
}

static void pfeng_mac_link_down(struct phylink_config *config, unsigned int mode, phy_interface_t interface)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));

	/* Disable Rx and Tx */
	netif_tx_stop_all_queues(netif->netdev);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
static void pfeng_mac_link_up(struct phylink_config *config,  struct phy_device *phy,
			      unsigned int mode, phy_interface_t interface, int speed,
			      int duplex, bool tx_pause, bool rx_pause)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));

	/* Enable Rx and Tx */
	netif_tx_wake_all_queues(netif->netdev);
}
#else
static void pfeng_mac_link_up(struct phylink_config *config, unsigned int mode,
			      phy_interface_t interface, struct phy_device *phy)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));

	/* Enable Rx and Tx */
	netif_tx_wake_all_queues(netif->netdev);
}
#endif

static const struct phylink_mac_ops pfeng_phylink_ops = {
	.validate = pfeng_phylink_validate,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
	.mac_pcs_get_state = pfeng_mac_link_state,
#else
	.mac_link_state = pfeng_mac_link_state,
#endif
	.mac_an_restart = pfeng_mac_an_restart,
	.mac_config = pfeng_mac_config,
	.mac_link_down = pfeng_mac_link_down,
	.mac_link_up = pfeng_mac_link_up,
};

/**
 * @brief	Create new phylink instance
 * @details	Creates the phylink instance for particular interface
 * @param[in]	netif pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_create(struct pfeng_netif *netif)
{
	struct pfeng_priv *priv = netif->priv;
	struct pfeng_emac *emac = &priv->emac[netif->cfg->emac_id];
	struct phylink *phylink;

	netif->phylink_cfg.dev = &netif->netdev->dev;
	netif->phylink_cfg.type = PHYLINK_NETDEV;
	phylink = phylink_create(&netif->phylink_cfg, of_fwnode_handle(netif->cfg->dn), emac->intf_mode, &pfeng_phylink_ops);
	if (IS_ERR(phylink))
		return PTR_ERR(phylink);

	netif->phylink = phylink;

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	INIT_DELAYED_WORK(&netif->xpcs_poll_work, pfeng_xpcs_poll);

	/* Get XPCS instance */
	if (emac->serdes_phy) {
		if (!phy_init(emac->serdes_phy) && !phy_power_on(emac->serdes_phy)) {
			if (!phy_configure(emac->serdes_phy, NULL)) {
				emac->xpcs = s32gen1_phy2xpcs(emac->serdes_phy);
				emac->xpcs_ops = s32gen1_xpcs_get_ops();
			} else {
				dev_err(netif->dev, "SerDes PHY configuration failed on EMAC%d\n", netif->cfg->emac_id);
			}
		} else {
			dev_err(netif->dev, "SerDes PHY init failed on EMAC%d\n", netif->cfg->emac_id);
		}

		if (!emac->xpcs || !emac->xpcs_ops) {
			dev_err(netif->dev, "Can't get SGMII PCS on EMAC%d\n", netif->cfg->emac_id);
			emac->xpcs_ops = NULL;
			emac->xpcs = NULL;
		}
	}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
	return 0;
}

/**
 * @brief	Start phylink
 * @details	Starts phylink
 * @param[in]	netif pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_start(struct pfeng_netif *netif)
{
	__maybe_unused struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];

	phylink_start(netif->phylink);

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	if (emac->xpcs && emac->xpcs_ops && emac->phyless)
		schedule_delayed_work(&netif->xpcs_poll_work, msecs_to_jiffies(XPCS_POLL_MS));
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

	return 0;
}

/**
 * @brief	Connect PHY
 * @details	Connects to the PHY
 * @param[in]	netif pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_connect_phy(struct pfeng_netif *netif)
{
	int ret;

	ret = phylink_of_phy_connect(netif->phylink, netif->cfg->dn, 0);
	if (ret)
		netdev_err(netif->netdev, "could not attach PHY: %d\n", ret);

	return ret;
}

/**
 * @brief	Disconnect PHY
 * @details	Disconnects connected PHY
 * @param[in]	netif pfeng net device structure
 */
void pfeng_phylink_disconnect_phy(struct pfeng_netif *netif)
{
	phylink_disconnect_phy(netif->phylink);
}

/**
 * @brief	Signalize MAC link change
 * @details	Signal to phylink MAC link change
 * @param[in]	up indicates whether the link is currently up
 */
void pfeng_phylink_mac_change(struct pfeng_netif *netif, bool up)
{
	phylink_mac_change(netif->phylink, up);
}

/**
 * @brief	Stop phylink
 * @details	Stops phylink
 * @param[in]	netif pfeng net device structure
 */
void pfeng_phylink_stop(struct pfeng_netif *netif)
{
	__maybe_unused struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];

	phylink_stop(netif->phylink);

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	if (emac->xpcs && emac->xpcs_ops && emac->phyless)
		cancel_delayed_work_sync(&netif->xpcs_poll_work);
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
}

/**
 * @brief	Destroy the MDIO bus
 * @details	Unregister and destroy the MDIO bus instance
 * @param[in]	netif pfeng net device structure
 */
void pfeng_phylink_destroy(struct pfeng_netif *netif)
{
	__maybe_unused struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac_id];
	phylink_destroy(netif->phylink);
	netif->phylink = NULL;

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	if (emac->serdes_phy)
		phy_exit(emac->serdes_phy);
#endif
}
