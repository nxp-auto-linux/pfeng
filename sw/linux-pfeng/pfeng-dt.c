/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/of_mdio.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/reset.h>
#include <linux/pinctrl/consumer.h>

#include "pfe_cfg.h"

/* Logical interface represents DT ethernet@ node */
#define PFENG_DT_COMPATIBLE_LOGIF		"fsl,pfeng-logif"
/* HIF represents DT hif@ node */
#define PFENG_DT_COMPATIBLE_HIF			"fsl,pfeng-hif"
/* EMAC represents DT emac@ node */
#define PFENG_DT_COMPATIBLE_EMAC		"fsl,pfeng-emac"
/* MDIO represents DT mdio@ node */
#define PFENG_DT_COMPATIBLE_MDIO		"fsl,pfeng-mdio"

#include "pfeng.h"

#ifdef PFE_CFG_PFE_MASTER

static int pfeng_of_get_phy_mode(struct device_node *np, phy_interface_t *mode)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0)
	int ret = of_get_phy_mode(np);

	*mode = PHY_INTERFACE_MODE_NA;
	if (ret > 0) {
		*mode = ret;
		ret = 0;
	}

	return ret;
#else
	return of_get_phy_mode(np, mode);
#endif
}

#endif /* PFE_CFG_PFE_MASTER */

int pfeng_dt_release_config(struct pfeng_priv *priv)
{
#ifdef PFE_CFG_PFE_MASTER
	int id;

	/* Free EMAC clocks */
	for (id = 0; id < PFENG_PFE_EMACS; id++) {
		struct pfeng_emac *emac = &priv->emac[id];
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
		struct device *dev = &priv->pdev->dev;

		/* Remove device depeendency for SerDes */
		if (emac->intf_mode == PHY_INTERFACE_MODE_SGMII && emac->serdes_phy)
			device_link_remove(dev, &emac->serdes_phy->dev);
#endif
#endif /* !PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

		/* EMAC RX clk */
		if (emac->rx_clk) {
			clk_disable_unprepare(emac->rx_clk);
			emac->rx_clk = NULL;
		}
		/* EMAC TX clk */
		if (emac->tx_clk) {
			clk_disable_unprepare(emac->tx_clk);
			emac->tx_clk = NULL;
		}
	}
#endif /* PFE_CFG_PFE_MASTER */

	return 0;
}

static int pfeng_of_get_addr(struct device_node *node)
{
	const __be32 *valp;

	valp = of_get_address(node, 0, NULL, NULL);
	if (!valp)
		return -EINVAL;

	return be32_to_cpu(*valp);
}

#if defined(PFE_CFG_PFE_MASTER) && !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
static bool pfeng_manged_inband(struct device_node *node)
{
	const char *managed;

	if (of_property_read_string(node, "managed", &managed) == 0 &&
	    strcmp(managed, "in-band-status") == 0)
		return true;

	return false;
}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

int pfeng_dt_create_config(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct device_node *np = priv->pdev->dev.of_node;
	pfe_platform_config_t *pfe_cfg = priv->pfe_cfg;
	struct resource *res;
	struct device_node *child = NULL;
	int irq, ret = 0;
	u32 propval, emac_list = 0;

	/* Get the base address of device */
	res = platform_get_resource(priv->pdev, IORESOURCE_MEM, 0);
	if(unlikely(!res)) {
		dev_err(dev, "Cannot find mem resource, aborting\n");
		return -EIO;
	}
	pfe_cfg->cbus_base = res->start;
	pfe_cfg->cbus_len = res->end - res->start + 1;
	dev_info(dev, "Cbus addr 0x%llx size 0x%llx\n", pfe_cfg->cbus_base, pfe_cfg->cbus_len);

#ifdef PFE_CFG_PFE_MASTER
	/* S32G Main GPRs */
	res = platform_get_resource(priv->pdev, IORESOURCE_MEM, 1);
	if(unlikely(!res)) {
		dev_err(dev, "Cannot find syscon resource, aborting\n");
		return -EIO;
	}
	priv->syscon.start = res->start;
	priv->syscon.end = res->end;
	dev_dbg(dev, "Syscon addr 0x%llx size 0x%llx\n", priv->syscon.start, priv->syscon.end - priv->syscon.start);

	/* Firmware CLASS name */
	if (of_find_property(np, "fsl,fw-class-name", NULL))
		if (!of_property_read_string(np, "fsl,fw-class-name", &priv->fw_class_name)) {
			dev_info(dev, "fsl,fw-class-name: %s\n", priv->fw_class_name);
		}

	/* Firmware UTIL name */
	if (of_find_property(np, "fsl,fw-util-name", NULL))
		if (!of_property_read_string(np, "fsl,fw-util-name", &priv->fw_util_name)) {
			dev_info(dev, "fsl,fw-util-name: %s\n", priv->fw_util_name);
		}

	/* IRQ bmu */
	irq = platform_get_irq_byname(priv->pdev, "bmu");
	if (irq < 0) {
		dev_err(dev, "Cannot find irq resource 'bmu', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_bmu = irq;
	dev_dbg(dev, "irq 'bmu' : %u\n", irq);

	/* IRQ upe/gpt */
	irq = platform_get_irq_byname(priv->pdev, "upegpt");
	if (irq < 0) {
		dev_err(dev, "Cannot find irq resource 'upegpt', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_upe_gpt = irq;
	dev_dbg(dev, "irq 'upegpt' : %u\n", irq);

	/* IRQ safety */
	irq = platform_get_irq_byname(priv->pdev, "safety");
	if (irq < 0) {
		dev_err(dev, "Cannot find irq resource 'safety', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_safety = irq;
	dev_dbg(dev, "irq 'safety' : %u\n", irq);

	/* L2BR default vlan id */
	if (of_find_property(np, "fsl,l2br-default-vlan", NULL)) {
		ret = of_property_read_u32(np, "fsl,l2br-default-vlan", &propval);
		if (!ret)
			pfe_cfg->vlan_id = propval;
	}

	/* L2BR vlan stats size */
	if (of_find_property(np, "fsl,l2br-vlan-stats-size", NULL)) {
		ret = of_property_read_u32(np, "fsl,l2br-vlan-stats-size", &propval);
		if (!ret)
			pfe_cfg->vlan_stats_size = propval;
	}
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_PFE_SLAVE
	if (of_property_read_u32(np, "fsl,pfeng-master-hif-channel", &propval)) {
		dev_err(dev, "Invalid hif-channel value");
		priv->ihc_master_chnl = HIF_CFG_MAX_CHANNELS + 1;
	} else {
		priv->ihc_master_chnl = propval;
		dev_info(dev, "MASTER IHC channel: %d", propval);
	}
#endif

	/*
	 * Network interface
	 * ("fsl,pfeng-logif")
	 *
	 * Describes Linux network interface
	 */
	for_each_available_child_of_node(np, child) {
		struct pfeng_netif_cfg *netif_cfg;
		struct device_node *dn;
		int id, i, hifs;
		u32 hifmap;

		if (!of_device_is_available(child))
			continue;

		if (!of_device_is_compatible(child, PFENG_DT_COMPATIBLE_LOGIF))
			continue;


		netif_cfg = devm_kzalloc(dev, sizeof(*netif_cfg), GFP_KERNEL);
		if (!netif_cfg) {
			dev_err(dev, "No memory for netif config\n");
			ret = -ENOMEM;
			goto err;
		}

		/* Linux interface name */
		if (!of_find_property(child, "fsl,pfeng-if-name", NULL) ||
			of_property_read_string(child, "fsl,pfeng-if-name", &netif_cfg->name)) {
			dev_warn(dev, "Valid ethernet name is missing (property 'fsl,pfeng-if-name')\n");

			continue;
		}
		dev_dbg(dev, "netif name: %s", netif_cfg->name);

		/* MAC eth address */
		netif_cfg->macaddr = (u8 *)of_get_mac_address(child);
		if (netif_cfg->macaddr)
			dev_dbg(dev, "DT mac addr: %pM", netif_cfg->macaddr);

		/* logif mode */
		if (of_find_property(child, "fsl,pfeng-logif-mode", NULL)) {
			ret = of_property_read_u32(child, "fsl,pfeng-logif-mode", &id);
			if (ret) {
				dev_err(dev, "The logif mode is invalid: %d\n", id);
				ret = -EINVAL;
				goto err;
			}
			switch (id) {
			default:
				dev_err(dev, "The logif mode is invalid: %d\n", id);
				ret = -EINVAL;
				goto err;
			case PFENG_LOGIF_MODE_TX_INJECT:
				netif_cfg->tx_inject = true;
				netif_cfg->aux = false;
				break;
			case PFENG_LOGIF_MODE_TX_CLASS:
				netif_cfg->tx_inject = false;
				netif_cfg->aux = false;
				break;
			case PFENG_LOGIF_MODE_AUX:
				netif_cfg->tx_inject = false;
				netif_cfg->aux = true;
				break;
			}
		} else {
			netif_cfg->tx_inject = true;
			netif_cfg->aux = false;
		}
		dev_info(dev, "logif(%s) mode: %s,%s", netif_cfg->name,
			netif_cfg->aux ? "aux" : "std",
			netif_cfg->tx_inject ? "tx-inject " : "tx-class");

		if (!netif_cfg->aux) {
#ifdef PFE_CFG_PFE_MASTER
			/* EMAC link */
			dn = of_parse_phandle(child, "fsl,pfeng-emac-link", 0);
			if (!dn) {
				dev_err(dev, "Required EMAC link is missing\n");
				ret = -EINVAL;
				goto err;
			}
			id = pfeng_of_get_addr(dn);
			if (id < 0) {
				dev_err(dev, "Required EMAC link is invalid\n");
				ret = -EINVAL;
				goto err;
			}
#else
			/* EMAC id */
			if (!of_find_property(child, "fsl,pfeng-emac-id", NULL)) {
				dev_err(dev, "The required EMAC id is missing\n");
				ret = -EINVAL;
				goto err;
			}
			ret = of_property_read_u32(child, "fsl,pfeng-emac-id", &id);
			if (ret || id >= PFENG_PFE_EMACS) {
				dev_err(dev, "The EMAC id is invalid: %d\n", id);
				ret = -EINVAL;
				goto err;
			}
			if (of_find_property(child, "fsl,pfeng-emac-router", NULL))
				netif_cfg->emac_router = true;
#endif /* PFE_CFG_PFE_MASTER */

			netif_cfg->emac = id;
			emac_list |= 1 << id;
			dev_info(dev, "logif(%s) EMAC: %u", netif_cfg->name, netif_cfg->emac);
		}

		/* HIF phandle(s) */
		hifmap = 0;
		hifs = 0;
		for (i = 0; i < PFENG_PFE_HIF_CHANNELS; i++) {
			dn = of_parse_phandle(child, "fsl,pfeng-hif-channels", i);
			if (dn) {
				id = pfeng_of_get_addr(dn);
				if (id < 0) {
					dev_err(dev, "HIF phandle %i is invalid\n", i);
					ret = -EINVAL;
					goto err;
				}

				hifmap |= 1 << id;
				hifs++;
				continue;
			}

			/* End of phandles, got at least one, good */
			if (!dn && hifs)
				break;

			/* No any phandle retieved */
			dev_err(dev, "Required HIF phandle is missing\n");
			ret = -EINVAL;
			goto err;
		}
		netif_cfg->hifmap = hifmap;
		netif_cfg->hifs = hifs;
		dev_info(dev, "logif(%s) HIFs: count %d map %02x", netif_cfg->name, netif_cfg->hifs, netif_cfg->hifmap);

		netif_cfg->dn = of_node_get(child);
#ifdef PFE_CFG_PFE_MASTER
		{
			struct pfeng_emac *emac = &priv->emac[netif_cfg->emac];
			__maybe_unused struct device_node *phy_handle;

			/* fixed-link check */
			emac->link_an =  MLO_AN_PHY;
			if (of_phy_is_fixed_link(child))
				emac->link_an = MLO_AN_FIXED;

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
			if (pfeng_manged_inband(child)) {
				emac->link_an = MLO_AN_INBAND;
				dev_info(dev, "SGMII AN enabled on EMAC%d\n", netif_cfg->emac);
			}

			emac->phyless = false;
			phy_handle = of_parse_phandle(child, "phy-handle", 0);
			if (emac->link_an == MLO_AN_INBAND && !phy_handle) {
				dev_info(dev, "EMAC%d PHY less SGMII\n", netif_cfg->emac);
				emac->phyless = true;
			}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
		}
#endif /* PFE_CFG_PFE_MASTER */

		list_add_tail(&netif_cfg->lnode, &priv->netif_cfg_list);
	} /* foreach PFENG_DT_COMPATIBLE_LOGIF */

#ifdef PFE_CFG_PFE_MASTER
	/*
	 * EMAC
	 * ("fsl,pfeng-emac")
	 *
	 * Describes PFE_EMAC block
	 */
	for_each_available_child_of_node(np, child) {
		int id;
		phy_interface_t intf_mode;
		struct pfeng_emac *emac;
		char tmp[32];

		if (!of_device_is_available(child))
			continue;

		if (!of_device_is_compatible(child, PFENG_DT_COMPATIBLE_EMAC))
			continue;

		id = pfeng_of_get_addr(child);
		if (id < 0)
			continue;

		if (id >= PFENG_PFE_EMACS)
			continue;

		emac = &priv->emac[id];

		/* Link DT node for embedded MDIO bus */
		emac->dn_mdio = of_get_compatible_child(child, PFENG_DT_COMPATIBLE_MDIO);

		if (!(emac_list & (1 << id))) {
			dev_info(dev, "EMAC%d phy unused, skipping phy setting", id);
			emac->enabled = true;
			continue;
		}

		 /* Get max speed */
		if (of_property_read_u32(child, "max-speed", &emac->max_speed)) {
			if (id == 0)
				/* S32G2: Only PFE_EMAC_0 supports 2.5G speed */
				emac->max_speed = SPEED_2500;
			else
				emac->max_speed = SPEED_1000;
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
			/* Standard SGMII AN is at 1G */
			emac->serdes_an_speed = SPEED_1000;
		} else {
			/* Store actual max-speed */
			emac->serdes_an_speed = emac->max_speed;
			if (emac->link_an == MLO_AN_INBAND &&
			    emac->serdes_an_speed != SPEED_1000 &&
			    emac->serdes_an_speed != SPEED_2500)
				dev_err(dev, "Unsupported SGMII AN max-speed");
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
		}

		/* Interface mode */
		ret = pfeng_of_get_phy_mode(child, &intf_mode);
		if (ret) {
			dev_warn(dev, "Failed to read phy-mode\n");
			/* for non managable interface */
			intf_mode = PHY_INTERFACE_MODE_INTERNAL;
		}

		dev_dbg(dev, "EMAC%d interface mode: %d", id, intf_mode);

		if ((intf_mode != PHY_INTERFACE_MODE_INTERNAL) &&
			(intf_mode != PHY_INTERFACE_MODE_SGMII) &&
			!phy_interface_mode_is_rgmii(intf_mode) &&
			(intf_mode != PHY_INTERFACE_MODE_RMII) &&
			(intf_mode != PHY_INTERFACE_MODE_MII)) {
			dev_err(dev, "Not supported phy interface mode: %s\n", phy_modes(intf_mode));
			ret = -EINVAL;
			goto err;
		}

		emac->intf_mode = intf_mode;
		emac->enabled = true;

#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
		if (emac->intf_mode == PHY_INTERFACE_MODE_SGMII) {
			scnprintf(tmp, sizeof(tmp), "emac%d_xpcs", id);
			emac->serdes_phy = devm_phy_get(dev, tmp);
			if (IS_ERR(emac->serdes_phy)) {
				emac->serdes_phy = NULL;
				dev_err(dev, "SerDes PHY for EMAC%d was not found\n", id);
			} else {
				/* Add device depeendency for SerDes */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
				if (device_link_add(dev, &emac->serdes_phy->dev, DL_FLAG_STATELESS /*| DL_FLAG_PM_RUNTIME*/))
					dev_err(dev, "Failed to enable SerDes PM dependency for EMAC%d\n", id);
#endif
			}
		} else {
			emac->serdes_phy = NULL;
		}
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

		/* optional: tx clock */
		if (phy_interface_mode_is_rgmii(intf_mode))
			strcpy(tmp, "tx_rgmii");
		else
			scnprintf(tmp, sizeof(tmp), "tx_%s", phy_modes(intf_mode));
		emac->tx_clk = devm_get_clk_from_child(dev, child, tmp);
		if (IS_ERR(emac->tx_clk)) {
			emac->tx_clk = NULL;
			dev_dbg(dev, "No TX clocks declared on EMAC%d for interface %s\n", id, phy_modes(intf_mode));
		}

		/* optional: rx clock */
		if (phy_interface_mode_is_rgmii(intf_mode))
			strcpy(tmp, "rx_rgmii");
		else
			scnprintf(tmp, sizeof(tmp), "rx_%s", phy_modes(intf_mode));
		emac->rx_clk = devm_get_clk_from_child(dev, child, tmp);
		if (IS_ERR(emac->rx_clk)) {
			emac->rx_clk = NULL;
			dev_dbg(dev, "No RX clocks declared on EMAC%d for interface %s\n", id, phy_modes(intf_mode));
		}
	} /* foreach PFENG_DT_COMPATIBLE_EMAC */
#endif



	/*
	 * HIF
	 * ("fsl,pfeng-hif")
	 *
	 * Describes PFE HIF block
	 */
	for_each_available_child_of_node(np, child) {
		int id;

		if (!of_device_is_available(child))
			continue;

		if (!of_device_is_compatible(child, PFENG_DT_COMPATIBLE_HIF))
			continue;

		id = pfeng_of_get_addr(child);
		if (id < 0)
			continue;

		if (id < PFENG_PFE_HIF_CHANNELS) {
			/* HIF IRQ */
			irq = of_irq_get(child, 0);
			if (irq < 0) {
				dev_err(dev, "Cannot find irq resource 'hif%i', aborting\n", id);
				return -EIO;
			}

			/* HIF mode */
			if (of_find_property(child, "fsl,pfeng-hif-mode", NULL)) {
				if (of_property_read_u32(child, "fsl,pfeng-hif-mode", &propval)) {
					dev_err(dev, "hif%d has invalid channel mode, aborting\n", id);
					return -EIO;
				}
				priv->hif_chnl[id].cl_mode = propval;

				pfe_cfg->irq_vector_hif_chnls[id] = irq;
			} else {
				dev_err(dev, "hif%d has missing channel mode, aborting\n", id);
				return -EIO;
			}

			/* HIF IHC option */
			if (of_find_property(child, "fsl,pfeng-ihc", NULL))
				priv->hif_chnl[id].ihc = true;
			else
				priv->hif_chnl[id].ihc = false;

			priv->hif_chnl[id].status = PFENG_HIF_STATUS_REQUESTED;
			pfe_cfg->hif_chnls_mask |= 1 << id;
		}
	} /* foreach PFENG_DT_COMPATIBLE_HIF */
	dev_info(dev, "HIF channels mask: 0x%04x", pfe_cfg->hif_chnls_mask);

	return 0;

err:
	if (child)
		of_node_put(child);
	pfeng_dt_release_config(priv);

	return ret;
}
