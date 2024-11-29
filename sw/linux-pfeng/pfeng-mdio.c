/*
 * Copyright 2020-2025 NXP
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)

int pfeng_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;
	u16 val;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		if (phyreg & MII_ADDR_C45) {
			ret = pfe_emac_mdio_read45(emac, (u16)phyaddr, (phyreg >> 16) & 0x1F, (u16)phyreg & 0xFFFF, &val, key);
		} else {
			ret = pfe_emac_mdio_read22(emac, (u16)phyaddr, (u16)phyreg, &val, key);
		}

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (!ret)
		return val;

	return -ENODATA;
}

int pfeng_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg, u16 phydata)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		if (phyreg & MII_ADDR_C45) {
			ret = pfe_emac_mdio_write45(emac, (u16)phyaddr, (phyreg >> 16) & 0x1F, (u16)phyreg & 0xFFFF, phydata, key);
		} else {
			ret = pfe_emac_mdio_write22(emac, (u16)phyaddr, (u16)phyreg, phydata, key);
		}

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (ret)
		return -ret;

	return 0;
}

#else

int pfeng_mdio_read_c22(struct mii_bus *bus, int phyaddr, int phyreg)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;
	u16 val;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		ret = pfe_emac_mdio_read22(emac, (u16)phyaddr, (u16)phyreg, &val, key);

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (!ret)
		return val;

	return -ENODATA;
}

int pfeng_mdio_write_c22(struct mii_bus *bus, int phyaddr, int phyreg, u16 phydata)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		ret = pfe_emac_mdio_write22(emac, (u16)phyaddr, (u16)phyreg, phydata, key);

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (ret)
		return -ret;

	return 0;
}

int pfeng_mdio_read_c45(struct mii_bus *bus, int phyaddr, int devad, int phyreg)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;
	u16 val;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		ret = pfe_emac_mdio_read45(emac, (u16)phyaddr, (u16)devad, phyreg, &val, key);

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (!ret)
		return val;

	return -ENODATA;
}

int pfeng_mdio_write_c45(struct mii_bus *bus, int phyaddr, int devad, int phyreg, u16 phydata)
{
	pfe_emac_t *emac;
	struct device *dev;
	u32 key;
	int ret;

	if (!bus)
		return -EINVAL;

	emac = bus->priv;
	dev = bus->parent;

	if (!emac)
		return -EAGAIN;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

	if (!pfe_emac_mdio_lock(emac, &key)) {

		ret = pfe_emac_mdio_write45(emac, (u16)phyaddr, (u16)devad, (u16)phyreg, phydata, key);

		pfe_emac_mdio_unlock(emac, key);
	} else
		ret = ENODATA;

#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(dev);
#endif /* PFE_CFG_PFE_MASTER */

	if (ret)
		return -ret;

	return 0;
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0) */

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

		if (!priv->emac[i].dn_mdio) {
			HM_MSG_DEV_INFO(dev, "MDIO bus %d disabled: Not found in DT\n", i);
			continue;
		}

		if (!of_device_is_available(priv->emac[i].dn_mdio)) {
			HM_MSG_DEV_INFO(dev, "MDIO bus %d disabled in DT\n", i);
			continue;
		}

		if (!priv->emac[i].enabled) {
			HM_MSG_DEV_INFO(dev, "MDIO bus %d disabled\n", i);
			continue;
		}

		if (!priv->pfe_platform->emac[i]) {
			HM_MSG_DEV_WARN(dev, "MDIO bus %d can't get linked EMAC\n", i);
			continue;
		}

		/* create MDIO bus */
		bus = mdiobus_alloc();
		if (!bus)
			return -ENOMEM;

		bus->priv = priv->pfe_platform->emac[i];
#ifdef PFE_CFG_PFE_SLAVE
		bus->name = "pfeng-proxy-emac";
#else
		bus->name = "pfeng-emac";
#endif
		snprintf(bus->id, MII_BUS_ID_SIZE, "%s-%d", bus->name, i);
		bus->parent = dev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
		bus->probe_capabilities = MDIOBUS_C22_C45;
		bus->read = pfeng_mdio_read;
		bus->write = pfeng_mdio_write;
#else
		bus->read = pfeng_mdio_read_c22;
		bus->write = pfeng_mdio_write_c22;
		bus->read_c45 = pfeng_mdio_read_c45;
		bus->write_c45 = pfeng_mdio_write_c45;
#endif

		ret = of_mdiobus_register(bus, priv->emac[i].dn_mdio);
		if (ret) {
			HM_MSG_DEV_ERR(dev, "MDIO bus %d registration failed: %d\n", i, ret);
			mdiobus_free(bus);
			return ret;
		}

		priv->emac[i].mii_bus = bus;

		HM_MSG_DEV_INFO(dev, "MDIO bus %d enabled\n", i);
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
	int i;

	for (i = 0; i < ARRAY_SIZE(pfeng_emac_ids); i++) {
		if (!priv->emac[i].enabled)
			continue;

		if (!priv->emac[i].mii_bus)
			continue;

		/* Invalidate EMAC link (before platform is destroyed) */
		priv->emac[i].mii_bus->priv = NULL;
	}

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
