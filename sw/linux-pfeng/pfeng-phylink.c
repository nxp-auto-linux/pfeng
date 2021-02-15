/*
 * Copyright 2020-2021 NXP
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
#define EMAC_TX_RATE_325M	325000000	/* 325MHz */
#define EMAC_TX_RATE_125M	125000000	/* 125MHz */
#define EMAC_TX_RATE_25M	25000000	/* 25MHz */
#define EMAC_TX_RATE_2M5	2500000		/* 2.5MHz */

/**
 * @brief	Validate and update the link configuration
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void pfeng_phylink_validate(struct phylink_config *config, unsigned long *supported, struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(to_net_dev(config->dev));
#else
static void pfeng_phylink_validate(struct net_device *netdev, unsigned long *supported, struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
#endif
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = { 0, };
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mac_supported) = { 0, };
	int max_speed = ndev->eth->max_speed;

	/* We only support SGMII and R/G/MII modes */
	if (state->interface != PHY_INTERFACE_MODE_NA &&
		state->interface != PHY_INTERFACE_MODE_SGMII &&
		state->interface != PHY_INTERFACE_MODE_RMII &&
		state->interface != PHY_INTERFACE_MODE_MII &&
		!phy_interface_mode_is_rgmii(state->interface)) {
		bitmap_zero(supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
		return;
	}

	phylink_set(mac_supported, 10baseT_Half);
	phylink_set(mac_supported, 10baseT_Full);

	if (max_speed > SPEED_10) {
		phylink_set(mac_supported, 100baseT_Half);
		phylink_set(mac_supported, 100baseT_Full);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
		phylink_set(mac_supported, 100baseT1_Full);
#endif
	}

	if (max_speed > SPEED_100) {
		phylink_set(mac_supported, 1000baseT_Half);
		phylink_set(mac_supported, 1000baseT_Full);
		phylink_set(mac_supported, 1000baseX_Full);
	}

	if (max_speed > SPEED_1000 &&
		/* Only PFE_EMAC_0 supports 2.5G over SGMII */
		!ndev->eth->emac_id &&
		state->interface == PHY_INTERFACE_MODE_SGMII) {
		phylink_set(mac_supported, 2500baseT_Full);
		phylink_set(mac_supported, 2500baseX_Full);
	}

	if (!ndev->eth->fixed_link)
		phylink_set(mac_supported, Autoneg);

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	phylink_helper_basex_speed(state);
#endif
}

/**
 * @brief	Read the current link state from the hardware
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int pfeng_mac_link_state(struct phylink_config *config, struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(to_net_dev(config->dev));
#else
static int pfeng_mac_link_state(struct net_device *netdev, struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
#endif
	pfe_emac_t *emac = ndev->priv->pfe->emac[ndev->eth->emac_id];
	int updated = 0;
	u32 speed, duplex;
	bool link;

	state->interface = ndev->eth->intf_mode;

	switch (state->interface) {
	case PHY_INTERFACE_MODE_SGMII:
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
		if (EOK == pfe_emac_get_link_config(emac, &speed, (pfe_emac_duplex_t *)&duplex)) {
			switch (speed) {
			default:
			case EMAC_SPEED_10_MBPS:
				state->speed = SPEED_10;
				break;
			case EMAC_SPEED_100_MBPS:
				state->speed = SPEED_100;
				break;
			case EMAC_SPEED_1000_MBPS:
				state->speed = SPEED_1000;
				break;
			case EMAC_SPEED_2500_MBPS:
				state->speed = SPEED_2500;
				break;
			}
			updated = 1;
		}
		if (EOK == pfe_emac_get_link_status(emac, &speed, (pfe_emac_duplex_t *)&duplex, &link)) {
			state->link = link;
			state->duplex = duplex == EMAC_DUPLEX_FULL ? 1 : 0;
			state->pause = MLO_PAUSE_NONE;
			updated = 1;
		}
	default:
		break;
	}

	if (updated)
		ndev->emac_speed = state->speed;

	return updated;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void pfeng_mac_an_restart(struct phylink_config *config)
#else
static void pfeng_mac_an_restart(struct net_device *netdev)
#endif
{
	return;
}

/**
 * @brief	Set necessary S32G clocks
 */
static void s32g_set_tx_clock(struct pfeng_ndev *ndev, unsigned int speed)
{
	u32 emac_speed;
	bool rgmii = phy_interface_mode_is_rgmii(ndev->eth->intf_mode);

	/* Only RGMII TX clock switch is supported */
	switch (speed) {
	default:
		netdev_dbg(ndev->netdev, "Skipped TX clock setting\n");
		return;
	case SPEED_2500:
		/* Seting TX clock for 2.5Gbps is unsupported */
		emac_speed = EMAC_SPEED_2500_MBPS;
		break;
	case SPEED_1000:
		if (ndev->eth->tx_clk && rgmii) {
			netdev_info(ndev->netdev, "Set TX clock to 125M\n");
			clk_set_rate(ndev->eth->tx_clk, EMAC_TX_RATE_125M);
		}
		emac_speed = EMAC_SPEED_1000_MBPS;
		break;
	case SPEED_100:
		if (ndev->eth->tx_clk && rgmii) {
			netdev_info(ndev->netdev, "Set TX clock to 25M\n");
			clk_set_rate(ndev->eth->tx_clk, EMAC_TX_RATE_25M);
		}
		emac_speed = EMAC_SPEED_100_MBPS;
		break;
	case SPEED_10:
		if (ndev->eth->tx_clk && rgmii) {
			netdev_info(ndev->netdev, "Set TX clock to 2.5M\n");
			clk_set_rate(ndev->eth->tx_clk, EMAC_TX_RATE_2M5);
		}
		emac_speed = EMAC_SPEED_10_MBPS;
		break;
	}

	pfe_emac_cfg_set_speed(ndev->emac_regs, emac_speed);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void pfeng_mac_config(struct phylink_config *config, unsigned int mode, const struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(to_net_dev(config->dev));
#else
static void pfeng_mac_config(struct net_device *netdev, unsigned int mode, const struct phylink_link_state *state)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
#endif

	switch (mode) {
		case MLO_AN_FIXED:
			/* FALLTHRU */
		case MLO_AN_PHY:
			if (state->speed == ndev->emac_speed)
				break;

			s32g_set_tx_clock(ndev, state->speed);
			ndev->emac_speed = state->speed;
			break;
		default:
			break;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void pfeng_mac_link_down(struct phylink_config *config, unsigned int mode, phy_interface_t interface)
{
	struct pfeng_ndev *ndev = netdev_priv(to_net_dev(config->dev));

	/* Disable Rx and Tx */
	netif_tx_stop_all_queues(ndev->netdev);
}

static void pfeng_mac_link_up(struct phylink_config *config, unsigned int mode, phy_interface_t interface, struct phy_device *phy)
{
	struct pfeng_ndev *ndev = netdev_priv(to_net_dev(config->dev));

	/* Enable Rx and Tx */
	netif_tx_wake_all_queues(ndev->netdev);
}
#else
static void pfeng_mac_link_down(struct net_device *netdev, unsigned int mode, phy_interface_t interface)
{
	/* Disable Rx and Tx */
	netif_tx_stop_all_queues(netdev);
}

static void pfeng_mac_link_up(struct net_device *netdev, unsigned int mode, phy_interface_t interface, struct phy_device *phy)
{
	/* Enable Rx and Tx */
	netif_tx_wake_all_queues(netdev);
}
#endif

static const struct phylink_mac_ops pfeng_phylink_ops = {
	.validate = pfeng_phylink_validate,
	.mac_link_state = pfeng_mac_link_state,
	.mac_an_restart = pfeng_mac_an_restart,
	.mac_config = pfeng_mac_config,
	.mac_link_down = pfeng_mac_link_down,
	.mac_link_up = pfeng_mac_link_up,
};

/**
 * @brief	Create new phylink instance
 * @details	Creates the phylink instance for particular interface
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_create(struct pfeng_ndev *ndev)
{
	struct phylink *phylink;
	void *syscon;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	ndev->phylink_cfg.dev = &ndev->netdev->dev;
	ndev->phylink_cfg.type = PHYLINK_NETDEV;
	phylink = phylink_create(&ndev->phylink_cfg, of_fwnode_handle(ndev->eth->dn), ndev->eth->intf_mode, &pfeng_phylink_ops);
#else
	phylink = phylink_create(ndev->netdev, of_fwnode_handle(ndev->eth->dn), ndev->eth->intf_mode, &pfeng_phylink_ops);

#endif
	if (IS_ERR(phylink))
		return PTR_ERR(phylink);

	ndev->phylink = phylink;

	/* add EMAC access register */
	syscon = ioremap_nocache(ndev->priv->cfg->cbus_base + CBUS_EMAC1_BASE_ADDR + (ndev->eth->emac_id * (CBUS_EMAC2_BASE_ADDR - CBUS_EMAC1_BASE_ADDR)),
			CBUS_EMAC2_BASE_ADDR - CBUS_EMAC1_BASE_ADDR);
	if(!syscon)
		netdev_warn(ndev->netdev, "Cannot map EMAC%d regs\n", ndev->eth->emac_id);
	else
		ndev->emac_regs = syscon;

	return 0;
}

/**
 * @brief	Start phylink
 * @details	Starts phylink
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_start(struct pfeng_ndev *ndev)
{
	phylink_start(ndev->phylink);

	return 0;
}

/**
 * @brief	Connect PHY
 * @details	Connects to the PHY
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_connect_phy(struct pfeng_ndev *ndev)
{
	int ret;

	ret = phylink_of_phy_connect(ndev->phylink, ndev->eth->dn, 0);
	if (ret)
		netdev_err(ndev->netdev, "could not attach PHY: %d\n", ret);

	return ret;
}

/**
 * @brief	Disconnect PHY
 * @details	Disconnects connected PHY
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_disconnect_phy(struct pfeng_ndev *ndev)
{
	phylink_disconnect_phy(ndev->phylink);

	return 0;
}

/**
 * @brief	Stop phylink
 * @details	Stops phylink
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_stop(struct pfeng_ndev *ndev)
{
	phylink_stop(ndev->phylink);

	return 0;
}

/**
 * @brief	Destroy the MDIO bus
 * @details	Unregister and destroy the MDIO bus instance
 * @param[in]	ndev pfeng net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_phylink_destroy(struct pfeng_ndev *ndev)
{
	phylink_destroy(ndev->phylink);
	ndev->phylink = NULL;

	if (ndev->emac_regs)
		iounmap(ndev->emac_regs);

	return 0;
}
