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

/* represents DT mdio@ node */
#define PFENG_DT_NODENAME_MDIO		"fsl,pfeng-mdio"

static int pfeng_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	pfe_emac_t *emac = bus->priv;
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
	pfe_emac_t *emac = bus->priv;
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
 * @param[in]	priv The driver main structure
 * @return	> 0 if OK, negative error number if failed
 */
int pfeng_mdio_register(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(pfeng_emac_ids); i++) {
		struct mii_bus *bus;
		int ret;

		if (!priv->emac[i].enabled) {
			dev_warn(dev, "MDIO bus %d disabled: No EMAC\n", i);
			continue;
		}

		if (!priv->emac[i].dn_mdio) {
			dev_info(dev, "MDIO bus %d disabled: Not found in DT\n", i);
			continue;
		}

		if (!of_device_is_available(priv->emac[i].dn_mdio)) {
			dev_info(dev, "MDIO bus %d disabled in DT\n", i);
			continue;
		}

		if (!priv->pfe_platform->emac[i]) {
			dev_warn(dev, "MDIO bus %d can't get linked EMAC\n", i);
			continue;
		}

		/* create MDIO bus */
		bus = mdiobus_alloc();
		if (!bus)
			return -ENOMEM;

		bus->priv = priv->pfe_platform->emac[i];
		bus->name = "PFEng Ethernet MDIO";
		snprintf(bus->id, MII_BUS_ID_SIZE, "%s.%d", bus->name, i);
		bus->read = pfeng_mdio_read;
		bus->write = pfeng_mdio_write;
		bus->parent = dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
		bus->probe_capabilities = MDIOBUS_C22_C45;
#endif

		ret = of_mdiobus_register(bus, priv->emac[i].dn_mdio);
		if (ret) {
			mdiobus_free(bus);
			return ret;
		}

		priv->emac[i].mii_bus = bus;
	}

	return i;
}

/**
 * @brief	Destroy the MDIO bus
 * @details	Unregister and destroy the MDIO bus instance
 * @param[in]	priv The driver main structure
 */
void pfeng_mdio_unregister(struct pfeng_priv *priv)
{
	int i;

	if (!priv)
		return;

	for (i = 0; i < ARRAY_SIZE(pfeng_emac_ids); i++) {

		if (!priv->emac[i].mii_bus)
			continue;

		mdiobus_unregister(priv->emac[i].mii_bus);
		mdiobus_free(priv->emac[i].mii_bus);
		priv->emac[i].mii_bus = NULL;
	}
}

int pfeng_mdio_suspend(struct pfeng_priv *priv)
{
	/* empty */
	return 0;
}

int pfeng_mdio_resume(struct pfeng_priv *priv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pfeng_emac_ids); i++) {
		if (!priv->emac[i].enabled)
			continue;

		if (!priv->emac[i].mii_bus)
			continue;

		/* Refresh EMAC link (was changed after platform reload) */
		priv->emac[i].mii_bus->priv = priv->pfe_platform->emac[i];
	}

	return 0;
}
