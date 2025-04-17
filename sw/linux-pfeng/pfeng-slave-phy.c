/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/of_device.h>
#include <linux/phy.h>

#include "pfe_cfg.h"
#include "pfeng.h"

static void phydev_handler(struct net_device *netdev)
{
	(void)netdev;
}

int pfeng_phy_connect(struct pfeng_netif *netif)
{
	struct device_node *phy_np;
	int ret = 0;

	phy_np = of_parse_phandle(netif->cfg->dn, "phy-handle", 0);
	if (!phy_np) {
		HM_MSG_NETDEV_WARN(netif->netdev, "Unable to parse phy-handle\n");
		return -ENODEV;
	}

	netif->slave_phydev = of_phy_connect(netif->netdev, phy_np, phydev_handler, 0,
					     netif->priv->emac[netif->cfg->phyif_id].intf_mode);
	of_node_put(phy_np);

	if (netif->slave_phydev)
		netif->slave_phydev->mac_managed_pm = true;
	else
		ret = -ENODEV;

	return ret;
}

void pfeng_phy_disconnect(struct pfeng_netif *netif)
{
	if (netif->slave_phydev) {
		phy_disconnect(netif->slave_phydev);
		netif->slave_phydev = NULL;
	}
}

void pfeng_phy_start(struct pfeng_netif *netif)
{
	phy_start(netif->slave_phydev);
}

void pfeng_phy_stop(struct pfeng_netif *netif)
{
	phy_stop(netif->slave_phydev);
}
