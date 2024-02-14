/*
 * Copyright 2020-2024 NXP
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

#define EMAC_CLK_RATE_325M	325000000	/* 325MHz */
#define EMAC_CLK_RATE_125M	125000000	/* 125MHz */
#define EMAC_CLK_RATE_25M	25000000	/* 25MHz */
#define EMAC_CLK_RATE_2M5	2500000		/* 2.5MHz */

static void pfeng_cfg_to_plat(struct pfeng_netif *netif, u32 speed, u32 duplex)
{
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->phyif_id];
	pfe_emac_t *pfe_emac = netif->priv->pfe_platform->emac[netif->cfg->phyif_id];
	u32 emac_speed, emac_duplex;
	bool speed_valid = true, duplex_valid = true;

	switch (speed) {
	default:
		HM_MSG_NETDEV_WARN(netif->netdev, "Speed %u not supported\n", speed);
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
		emac->speed = speed;
	}

	switch (duplex) {
	case DUPLEX_HALF:
		emac_duplex = EMAC_DUPLEX_HALF;
		break;
	case DUPLEX_FULL:
		emac_duplex = EMAC_DUPLEX_FULL;
		break;
	default:
		HM_MSG_NETDEV_ERR(netif->netdev, "Unknown duplex\n");
		duplex_valid = false;
		return;
		break;
	}

	if (emac_duplex) {
		pfe_emac_set_link_duplex(pfe_emac, emac_duplex);
		emac->duplex = duplex;
	}
}

/**
 * @brief	Set necessary S32G clocks
 */
static int s32g_set_rgmii_speed(struct pfeng_netif *netif, unsigned int speed)
{
	struct clk *tx_clk = netif->priv->emac[netif->cfg->phyif_id].tx_clk;
	unsigned long rate = 0;
	int ret = 0;

	switch (speed) {
	default:
		HM_MSG_NETDEV_DBG(netif->netdev, "Skipped clock setting\n");
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
			HM_MSG_NETDEV_ERR(netif->netdev, "Unable to set TX clock to %luHz\n", rate);
		else
			HM_MSG_NETDEV_INFO(netif->netdev, "Set TX clock to %luHz\n", rate);
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)
static struct phylink_pcs *pfeng_mac_select_pcs(struct phylink_config *config, phy_interface_t interface)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->phyif_id];

	if  (interface == PHY_INTERFACE_MODE_SGMII)
		return emac->pcs;

	return NULL;
}
#endif

static void pfeng_mac_config(struct phylink_config *config, unsigned int mode, const struct phylink_link_state *state)
{
	/* All done in s32cc_phylink_pcs_link_up() */
}

static void pfeng_mac_link_down(struct phylink_config *config, unsigned int mode, phy_interface_t interface)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));

	/* Disable Rx and Tx */
	netif_tx_stop_all_queues(netif->netdev);
}

static void pfeng_mac_link_up(struct phylink_config *config,  struct phy_device *phy,
			      unsigned int mode, phy_interface_t interface, int speed,
			      int duplex, bool tx_pause, bool rx_pause)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));

	/* Change clocks for RGMII/RMII */
	if (phy_interface_mode_is_rgmii(interface) || interface == PHY_INTERFACE_MODE_RMII) {
		s32g_set_rgmii_speed(netif, speed);
	}

	pfeng_cfg_to_plat(netif, speed, duplex);

	/* Enable Rx and Tx */
	netif_tx_wake_all_queues(netif->netdev);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
/**
 * @brief	Validate and update the link configuration
 */
static void pfeng_validate(struct phylink_config *config, unsigned long *supported, struct phylink_link_state *state)
{
	struct pfeng_netif *netif = netdev_priv(to_net_dev(config->dev));
	struct pfeng_priv *priv = netif->priv;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = { 0, };
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mac_supported) = { 0, };
	int max_speed = priv->emac[netif->cfg->phyif_id].max_speed;
	int an_serdes_speed = priv->emac[netif->cfg->phyif_id].serdes_an_speed;

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
		/* G3: All PFE_EMACs support 2.5G over SGMII */
		(netif->priv->on_g3 ||
		/* G2: Only PFE_EMAC_0 supports 2.5G over SGMII */
			!netif->cfg->phyif_id) &&
		(state->interface == PHY_INTERFACE_MODE_SGMII ||
		state->interface == PHY_INTERFACE_MODE_NA)) {
		phylink_set(mac_supported, 2500baseT_Full);
		phylink_set(mac_supported, 2500baseX_Full);
	}

	/* SGMII AN can't distinguish between 1G and 2.5G */
	if (state->interface == PHY_INTERFACE_MODE_SGMII &&
	    priv->emac[netif->cfg->phyif_id].link_an == MLO_AN_INBAND) {
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
	} else if (priv->emac[netif->cfg->phyif_id].link_an == MLO_AN_FIXED) {
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
static void pfeng_mac_link_state(struct phylink_config *config, struct phylink_link_state *state)
{
	state->link = 0;
}

static void pfeng_mac_an_restart(struct phylink_config *config)
{
	return;
}
#endif

static const struct phylink_mac_ops pfeng_phylink_ops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
	.validate = pfeng_validate,
	.mac_pcs_get_state = pfeng_mac_link_state,
	.mac_an_restart = pfeng_mac_an_restart,
#else
	.mac_select_pcs = pfeng_mac_select_pcs,
#endif
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
	struct pfeng_emac *emac = &priv->emac[netif->cfg->phyif_id];
	struct s32cc_xpcs *xpcs;
	struct phylink *phylink;
	int ret;

	netif->phylink_cfg.dev = &netif->netdev->dev;
	netif->phylink_cfg.type = PHYLINK_NETDEV;
	netif->phylink_cfg.mac_managed_pm = true;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0)
	netif->phylink_cfg.mac_capabilities = MAC_ASYM_PAUSE | MAC_SYM_PAUSE |
						MAC_10 | MAC_100 | MAC_1000;

	__set_bit(PHY_INTERFACE_MODE_INTERNAL,
		  netif->phylink_cfg.supported_interfaces);
	__set_bit(PHY_INTERFACE_MODE_SGMII,
		  netif->phylink_cfg.supported_interfaces);
        phy_interface_set_rgmii(netif->phylink_cfg.supported_interfaces);

	if ((!netif->priv->on_g3 && netif->cfg->phyif_id == PFE_PHY_IF_ID_EMAC0) || netif->priv->on_g3) {
		/* PFE_EMAC supports 2.5G over SGMII on G3 for all EMACs but on G2 only for EMAC_0 */
		netif->phylink_cfg.mac_capabilities |= MAC_2500FD;
		__set_bit(PHY_INTERFACE_MODE_2500BASEX, netif->phylink_cfg.supported_interfaces);
	}
#endif

	phylink = phylink_create(&netif->phylink_cfg, of_fwnode_handle(netif->cfg->dn), emac->intf_mode, &pfeng_phylink_ops);
	if (IS_ERR(phylink))
		return PTR_ERR(phylink);

	netif->phylink = phylink;

	/* Get XPCS instance */
	if (emac->serdes_phy) {
		if (phy_init(emac->serdes_phy) || phy_power_on(emac->serdes_phy)) {
			HM_MSG_DEV_ERR(netif->dev, "SerDes PHY init failed on EMAC%d\n", netif->cfg->phyif_id);
			return -EINVAL;
		}
		if ((ret = phy_configure(emac->serdes_phy, NULL))) {
			HM_MSG_DEV_ERR(netif->dev, "SerDes PHY configuration failed on EMAC%d\n", netif->cfg->phyif_id);
			return ret;
		}
		xpcs = s32cc_phy2xpcs(emac->serdes_phy);
		if (xpcs)
			emac->pcs = s32cc_xpcs_get_pcs(xpcs);
		else
			HM_MSG_DEV_ERR(netif->dev, "SerDes data retrieval failed on EMAC%d\n", netif->cfg->phyif_id);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
		phylink_set_pcs(phylink, emac->pcs);
#endif
	}

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
	phylink_start(netif->phylink);

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
		HM_MSG_NETDEV_ERR(netif->netdev, "could not attach PHY: %d\n", ret);

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
	phylink_stop(netif->phylink);
}

/**
 * @brief	Destroy the MDIO bus
 * @details	Unregister and destroy the MDIO bus instance
 * @param[in]	netif pfeng net device structure
 */
void pfeng_phylink_destroy(struct pfeng_netif *netif)
{
	__maybe_unused struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->phyif_id];

	phylink_destroy(netif->phylink);
	netif->phylink = NULL;

	if (emac->serdes_phy)
		phy_exit(emac->serdes_phy);
}
