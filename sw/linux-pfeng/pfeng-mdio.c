/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/mii.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_emac.h"
#include "pfe_platform.h"

#include "pfeng.h"

static int pfeng_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct pfeng_ndev *ndev = bus->priv;
	pfe_emac_t *emac = ndev->priv->pfe->emac[ndev->eth->emac_id];
	int ret;
	u16 val;

	if (phyreg & MII_ADDR_C45) {
		ret = pfe_emac_mdio_read45(emac, (u16)phyaddr, (phyreg >> 16) & 0x1F, (u16)phyreg & 0xFFFF, &val, 0);
	} else {
		ret = pfe_emac_mdio_read22(emac, (u16)phyaddr, (u16)phyreg, &val, 0);
	}

	if (!ret)
		return val;

	return -ENODATA;
}

static int pfeng_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg, u16 phydata)
{
	struct pfeng_ndev *ndev = bus->priv;
	pfe_emac_t *emac = ndev->priv->pfe->emac[ndev->eth->emac_id];
	int ret;

	if (phyreg & MII_ADDR_C45) {
		ret = pfe_emac_mdio_write45(emac, (u16)phyaddr, (phyreg >> 16) & 0x1F, (u16)phyreg & 0xFFFF, phydata, 0);
	} else {
		ret = pfe_emac_mdio_write22(emac, (u16)phyaddr, (u16)phyreg, phydata, 0);
	}

	if (ret)
		return -ret;

	return 0;
}

/**
 * @brief	Create new MDIO bus instance
 * @details	Creates and initializes the MDIO bus instance
 * @param[in]	netdev net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_mdio_register(struct pfeng_ndev *ndev)
{
	struct mii_bus *bus;
	struct device_node *dn;
	int ret;

	dn = of_get_compatible_child(ndev->eth->dn, PFENG_DT_NODENAME_MDIO);
	if (!dn) {
		netdev_dbg(ndev->netdev, "No compatible MDIO bus\n");
		return 0;
	}

	if (!of_device_is_available(dn)) {
		netdev_dbg(ndev->netdev, "MDIO bus disabled\n");
		return 0;
	}

	/* create MDIO bus */
	bus = mdiobus_alloc();
	if (!bus)
		return -ENOMEM;

	bus->priv = ndev;
	bus->name = "PFEng Ethernet MDIO";
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s.%s",
		 bus->name, ndev->eth->name);
	bus->read = pfeng_mdio_read;
	bus->write = pfeng_mdio_write;
	bus->parent = ndev->dev;
	ndev->mii_bus = bus;

	ret = of_mdiobus_register(bus, dn);
	if (ret) {
		mdiobus_free(bus);
		ndev->mii_bus = NULL;
		return ret;
	}

	return 0;
}

/**
 * @brief	Destroy the MDIO bus
 * @details	Unregister and destroy the MDIO bus instance
 * @param[in]	netdev net device structure
 * @return	0 if OK, error number if failed
 */
int pfeng_mdio_unregister(struct pfeng_ndev *ndev)
{
	if (!ndev) {
		return 0;
	}

	if (!ndev->mii_bus)
		return 0;

	mdiobus_unregister(ndev->mii_bus);
	mdiobus_free(ndev->mii_bus);

	return 0;
}
