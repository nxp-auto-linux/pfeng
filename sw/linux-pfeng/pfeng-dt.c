/*
 * Copyright 2021-2025 NXP
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

/* PFE controller cbus resource name*/
#define PFE_RES_NAME_PFE_CBUS			"pfe-cbus"
/* S32G_MAIN_GPR memory map resource name */
#define PFE_RES_NAME_S32G_MAIN_GPR		"s32g-main-gpr"

#include "pfeng.h"

static int pfeng_of_get_addr(struct device_node *node)
{
	const __be32 *valp;

	valp = of_get_address(node, 0, NULL, NULL);
	if (!valp)
		return -EINVAL;

	return be32_to_cpu(*valp);
}

int pfeng_dt_release_config(struct pfeng_priv *priv)
{
#ifdef PFE_CFG_PFE_MASTER
	struct device *dev = &priv->pdev->dev;
	int id;

	/* Free EMAC clocks */
	for (id = 0; id < PFENG_PFE_EMACS; id++) {
		struct pfeng_emac *emac = &priv->emac[id];

		/* EMAC RX clk */
		if (emac->rx_clk) {
			if (!emac->rx_clk_pending)
				clk_disable_unprepare(emac->rx_clk);

			devm_clk_put(dev, emac->rx_clk);
			emac->rx_clk_pending = false;
			emac->rx_clk = NULL;
		}
		/* EMAC TX clk */
		if (emac->tx_clk) {
			clk_disable_unprepare(emac->tx_clk);
			devm_clk_put(dev, emac->tx_clk);
			emac->tx_clk = NULL;
		}
	}
#endif /* PFE_CFG_PFE_MASTER */

	return 0;
}

#if defined(PFE_CFG_PFE_MASTER)
static bool pfeng_manged_inband(struct device_node *node)
{
	const char *managed;

	if (of_property_read_string(node, "managed", &managed) == 0 &&
	    strcmp(managed, "in-band-status") == 0)
		return true;

	return false;
}
#endif /* PFE_CFG_PFE_MASTER */

int pfeng_dt_create_config(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct device_node *np = priv->pdev->dev.of_node;
	pfe_platform_config_t *pfe_cfg = priv->pfe_cfg;
	struct resource *res;
	struct device_node *child = NULL;
	int irq, i, ret = 0;
	u32 propval;
	char propname[32];
#ifdef PFE_CFG_PFE_MASTER
	int emacs;
#endif /* PFE_CFG_PFE_MASTER */

	/* Get the base address of device */
	res = platform_get_resource_byname(priv->pdev, IORESOURCE_MEM, PFE_RES_NAME_PFE_CBUS);
	if(unlikely(!res)) {
		HM_MSG_DEV_ERR(dev, "Cannot find mem resource by '%s', aborting\n", PFE_RES_NAME_PFE_CBUS);
		return -EIO;
	}
	pfe_cfg->cbus_base = res->start;
	pfe_cfg->cbus_len = res->end - res->start + 1;
	HM_MSG_DEV_INFO(dev, "Cbus addr 0x%llx size 0x%llx\n", pfe_cfg->cbus_base, pfe_cfg->cbus_len);

	/* S32G Main GPRs */
#ifdef PFE_CFG_PFE_MASTER
	res = platform_get_resource_byname(priv->pdev, IORESOURCE_MEM, PFE_RES_NAME_S32G_MAIN_GPR);
	if(unlikely(!res)) {
		HM_MSG_DEV_ERR(dev, "Cannot find syscon resource by '%s', aborting\n", PFE_RES_NAME_S32G_MAIN_GPR);
		return -EIO;
	}
	priv->syscon.start = res->start;
	priv->syscon.end = res->end;
	HM_MSG_DEV_DBG(dev, "Syscon addr 0x%llx size 0x%llx\n", priv->syscon.start, priv->syscon.end - priv->syscon.start + 1);
#else
	/* It's only optional on Slave */
	res = platform_get_resource_byname(priv->pdev, IORESOURCE_MEM, PFE_RES_NAME_S32G_MAIN_GPR);
	if(res) {
		priv->syscon.start = res->start;
		priv->syscon.end = res->end;
		HM_MSG_DEV_DBG(dev, "Syscon addr 0x%llx size 0x%llx\n", priv->syscon.start, priv->syscon.end - priv->syscon.start + 1);
	}
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_PFE_MASTER
	/* Firmware CLASS name */
	if (of_find_property(np, "nxp,fw-class-name", NULL))
		if (!of_property_read_string(np, "nxp,fw-class-name", &priv->fw_class_name)) {
			HM_MSG_DEV_INFO(dev, "nxp,fw-class-name: %s\n", priv->fw_class_name);
		}

	/* Firmware UTIL name */
	if (of_find_property(np, "nxp,fw-util-name", NULL))
		if (!of_property_read_string(np, "nxp,fw-util-name", &priv->fw_util_name)) {
			HM_MSG_DEV_INFO(dev, "nxp,fw-util-name: %s\n", priv->fw_util_name);
		}

	/* IRQ bmu */
	irq = platform_get_irq_byname(priv->pdev, "bmu");
	if (irq < 0) {
		HM_MSG_DEV_ERR(dev, "Cannot find irq resource 'bmu', aborting\n");
		return -EIO;
	}
#if (TRUE == PFE_CFG_BMU_IRQ_ENABLED)
	pfe_cfg->irq_vector_bmu = irq;
	HM_MSG_DEV_DBG(dev, "irq 'bmu' : %u\n", irq);
#endif /* PFE_CFG_BMU_IRQ_ENABLED */

	/* IRQ upe/gpt */
	irq = platform_get_irq_byname(priv->pdev, "upegpt");
	if (irq < 0) {
		HM_MSG_DEV_ERR(dev, "Cannot find irq resource 'upegpt', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_upe_gpt = irq;
	HM_MSG_DEV_DBG(dev, "irq 'upegpt' : %u\n", irq);

	/* IRQ safety */
	irq = platform_get_irq_byname(priv->pdev, "safety");
	if (irq < 0) {
		HM_MSG_DEV_ERR(dev, "Cannot find irq resource 'safety', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_safety = irq;
	HM_MSG_DEV_DBG(dev, "irq 'safety' : %u\n", irq);

	/* L2BR default vlan id */
	if (of_find_property(np, "nxp,pfeng-l2br-default-vlan", NULL)) {
		ret = of_property_read_u32(np, "nxp,pfeng-l2br-default-vlan", &propval);
		if (!ret)
			pfe_cfg->vlan_id = propval;
	}

	/* L2BR vlan stats size */
	if (of_find_property(np, "nxp,pfeng-l2br-vlan-stats-size", NULL)) {
		ret = of_property_read_u32(np, "nxp,pfeng-l2br-vlan-stats-size", &propval);
		if (!ret)
			pfe_cfg->vlan_stats_size = propval;
	}

	/* optional: EMACs with external timestamping */
	emacs = of_property_count_elems_of_size(np, "nxp,pfeng-emac-ts-ext-modes", sizeof(u32));
	if ((emacs > 0) && (emacs < PFENG_PFE_EMACS)) {
		for (i = 0; i < emacs; i++) {
			ret = of_property_read_u32_index(np, "nxp,pfeng-emac-ts-ext-modes", i, &propval);
			if (ret)
				continue;
			if (propval >= PFENG_PFE_EMACS) {
				HM_MSG_DEV_ERR(dev, "EMAC number %u is invalid, aborting\n", propval);
				return -EIO;
			}
			pfe_cfg->emac_ext_ts_mask |= (1 << propval);
		}
	}
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (of_property_read_u32(np, "nxp,pfeng-ihc-channel", &propval)) {
		HM_MSG_DEV_ERR(dev, "Invalid IHC hif-channel value");
		return -EIO;
	} else {
		priv->hif_chnl[propval].ihc = true;
		priv->hif_chnl[propval].refcount++;
		HM_MSG_DEV_INFO(dev, "IHC channel: %d", propval);
	}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_PFE_SLAVE
	if (of_property_read_u32(np, "nxp,pfeng-master-channel", &propval)) {
		HM_MSG_DEV_ERR(dev, "Invalid hif-channel value");
		priv->ihc_master_chnl = HIF_CFG_MAX_CHANNELS + 1;
	} else {
		priv->ihc_master_chnl = propval;
		HM_MSG_DEV_INFO(dev, "MASTER IHC channel: %d", propval);
	}
#endif

	/*
	 * Network interface
	 * ("nxp,s32g-pfe-netif")
	 *
	 * Describes Linux network interface
	 */
	for_each_available_child_of_node(np, child) {
		struct pfeng_netif_cfg *netif_cfg;
		int id, i, hifs;
		u32 hifmap;

		if (!of_device_is_available(child))
			continue;

		if (!of_device_is_compatible(child, "nxp,s32g-pfe-netif"))
			continue;

		netif_cfg = devm_kzalloc(dev, sizeof(*netif_cfg), GFP_KERNEL);
		if (!netif_cfg) {
			HM_MSG_DEV_ERR(dev, "No memory for netif config\n");
			ret = -ENOMEM;
			goto err;
		}

		/* Linux interface name */
		if (!of_find_property(child, "nxp,pfeng-if-name", NULL) ||
			of_property_read_string(child, "nxp,pfeng-if-name", &netif_cfg->name)) {
			HM_MSG_DEV_WARN(dev, "Valid ethernet name is missing (property 'nxp,pfeng-if-name')\n");

			continue;
		}
		HM_MSG_DEV_INFO(dev, "netif name: %s", netif_cfg->name);

		/* MAC eth address */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,13,0)
		if (!IS_ERR_OR_NULL(of_get_mac_address(child))) {
			memcpy(netif_cfg->macaddr, (u8 *)of_get_mac_address(child), ETH_ALEN);
			HM_MSG_DEV_INFO(dev, "DT mac addr: %pM", netif_cfg->macaddr);
		}
#else
		if (!of_get_mac_address(child, netif_cfg->macaddr))
			HM_MSG_DEV_INFO(dev, "DT mac addr: %pM", netif_cfg->macaddr);
#endif

		/* Phy id */
		if (of_find_property(child, "nxp,pfeng-netif-mode-aux", NULL)) {
			/* unused hole in array is used for AUX netdev */
			netif_cfg->phyif_id = PFE_PHY_IF_ID_AUX;
			HM_MSG_DEV_INFO(dev, "netif(%s) no linked phyif in AUX mode", netif_cfg->name);
		} else {
			ret = of_property_read_u32(child, "nxp,pfeng-emac-id", &id);
			if (!ret && id <= PFENG_PFE_EMACS) {
				/* support backward compatibility */
				HM_MSG_DEV_WARN(dev, "netif(%s) nxp,pfeng-emac-id property is deprecated, please use nxp,pfeng-linked-phyif", netif_cfg->name);
			} else if (!of_find_property(child, "nxp,pfeng-linked-phyif", NULL)) {
				HM_MSG_DEV_ERR(dev, "The required EMAC id is missing\n");
				ret = -EINVAL;
				goto err;
			} else {
				ret = of_property_read_u32(child, "nxp,pfeng-linked-phyif", &id);
				if (ret || id >= PFENG_NETIFS_CNT) {
					HM_MSG_DEV_ERR(dev, "The linked phyif-id is invalid: %d\n", id);
					ret = -EINVAL;
					goto err;
				}
			}
#ifdef PFE_CFG_PFE_SLAVE
			if (of_find_property(child, "nxp,pfeng-emac-router", NULL))
				netif_cfg->emac_router = true;
#endif /* PFE_CFG_PFE_SLAVE */

			netif_cfg->phyif_id = id;
			HM_MSG_DEV_INFO(dev, "netif(%s) linked phyif: %u", netif_cfg->name, netif_cfg->phyif_id);
		}

		if (of_find_property(child, "nxp,pfeng-netif-mode-mgmt-only", NULL)) {
			netif_cfg->only_mgmt = true;
			HM_MSG_DEV_INFO(dev, "netif(%s) mode: mgmt", netif_cfg->name);
		} else
			HM_MSG_DEV_INFO(dev, "netif(%s) mode: %s", netif_cfg->name,
				pfeng_netif_cfg_is_aux(netif_cfg) ? "aux" : "std");

		/* netif HIF channel(s) */
		hifmap = 0;
		hifs = of_property_count_elems_of_size(child, "nxp,pfeng-hif-channels", sizeof(u32));
		if (hifs < 1) {
			HM_MSG_DEV_ERR(dev, "Required HIF id list is missing\n");
			ret = -EINVAL;
			goto err;
		}
		for (i = 0; i < hifs; i++) {
			ret = of_property_read_u32_index(child, "nxp,pfeng-hif-channels", i, &propval);
			if (ret) {
				HM_MSG_DEV_ERR(dev, "%pOFn: couldn't read HIF id at index %d, ret=%d\n", np, i, ret);
				goto err;
			}
			if (propval >= PFENG_PFE_HIF_CHANNELS) {
				HM_MSG_DEV_ERR(dev, "netif(%s) HIF channel id=%u is invalid, aborting\n", netif_cfg->name, propval);
				goto err;
			}

			if (!pfe_cfg->irq_vector_hif_chnls[propval]) {
				/* Get IRQ number */
				scnprintf(propname, sizeof(propname), "hif%d", propval);
				irq = platform_get_irq_byname(priv->pdev, propname);
				if (irq < 0) {
					HM_MSG_DEV_ERR(dev, "Cannot find irq resource '%s', aborting\n", propname);
					goto err;
				}
				pfe_cfg->irq_vector_hif_chnls[propval] = irq;
				HM_MSG_DEV_DBG(dev, "irq '%s' : %u\n", propname, irq);

				priv->hif_chnl[propval].status = PFENG_HIF_STATUS_REQUESTED;
				pfe_cfg->hif_chnls_mask |= 1 << propval;
			}

			hifmap |= 1 << propval;
			priv->hif_chnl[propval].refcount++;
		}

		netif_cfg->hifmap = hifmap;
		netif_cfg->hifs = hifs;
		HM_MSG_DEV_INFO(dev, "netif(%s) HIFs: count %d map %02x", netif_cfg->name, netif_cfg->hifs, netif_cfg->hifmap);

		netif_cfg->dn = of_node_get(child);

#ifdef PFE_CFG_PFE_MASTER
		if (pfeng_netif_cfg_has_emac(netif_cfg)) {
			struct pfeng_emac *emac = &priv->emac[netif_cfg->phyif_id];
			__maybe_unused struct device_node *phy_handle;
			phy_interface_t intf_mode;

			/* fixed-link check */
			emac->link_an =  MLO_AN_PHY;
			if (of_phy_is_fixed_link(child))
				emac->link_an = MLO_AN_FIXED;

			if (pfeng_manged_inband(child)) {
				emac->link_an = MLO_AN_INBAND;
				HM_MSG_DEV_INFO(dev, "SGMII AN enabled on EMAC%d\n", netif_cfg->phyif_id);
			}

			emac->phyless = false;
			phy_handle = of_parse_phandle(child, "phy-handle", 0);
			if (emac->link_an == MLO_AN_INBAND && !phy_handle) {
				HM_MSG_DEV_INFO(dev, "EMAC%d PHY less SGMII\n", netif_cfg->phyif_id);
				emac->phyless = true;
			}
			of_node_put(phy_handle);

			/* Interface mode */
			ret = of_get_phy_mode(child, &intf_mode);
			if (ret) {
				HM_MSG_DEV_WARN(dev, "Failed to read phy-mode\n");
				/* for non managable interface */
				intf_mode = PHY_INTERFACE_MODE_INTERNAL;
			}

			HM_MSG_DEV_INFO(dev, "EMAC%d interface mode: %d", id, intf_mode);

			if ((intf_mode != PHY_INTERFACE_MODE_INTERNAL) &&
				(intf_mode != PHY_INTERFACE_MODE_SGMII) &&
				!phy_interface_mode_is_rgmii(intf_mode) &&
				(intf_mode != PHY_INTERFACE_MODE_RMII) &&
				(intf_mode != PHY_INTERFACE_MODE_MII)) {
				HM_MSG_DEV_ERR(dev, "Not supported phy interface mode: %s\n", phy_modes(intf_mode));
				ret = -EINVAL;
				goto err;
			}

			emac->intf_mode = intf_mode;
			emac->enabled = true;
			emac->max_speed = 0;

			 /* Get max speed */
			if (of_property_read_u32(child, "max-speed", &emac->max_speed)) {
				/* Standard SGMII AN is at 1G */
				emac->serdes_an_speed = SPEED_1000;
			} else {
				/* Store actual max-speed */
				emac->serdes_an_speed = emac->max_speed;
				if (emac->link_an == MLO_AN_INBAND &&
				    emac->serdes_an_speed != SPEED_1000 &&
				    emac->serdes_an_speed != SPEED_2500)
					HM_MSG_DEV_ERR(dev, "Unsupported SGMII AN max-speed");
			}

			if (emac->intf_mode == PHY_INTERFACE_MODE_SGMII) {
				scnprintf(propname, sizeof(propname), "emac%d_xpcs", id);
				emac->serdes_phy = devm_phy_get(dev, propname);
				if (IS_ERR(emac->serdes_phy)) {
					emac->serdes_phy = NULL;
					HM_MSG_DEV_ERR(dev, "SerDes PHY for EMAC%d was not found\n", id);
				}
			} else {
				emac->serdes_phy = NULL;
			}

			/* optional: tx clock */
			if (phy_interface_mode_is_rgmii(intf_mode))
				strcpy(propname, "tx_rgmii");
			else
				scnprintf(propname, sizeof(propname), "tx_%s", phy_modes(intf_mode));
			emac->tx_clk = devm_get_clk_from_child(dev, child, propname);
			if (IS_ERR(emac->tx_clk)) {
				emac->tx_clk = NULL;
				HM_MSG_DEV_ERR(dev, "No TX clocks declared on EMAC%d for interface %s\n", id, phy_modes(intf_mode));
			}

			/* optional: rx clock */
			if (phy_interface_mode_is_rgmii(intf_mode))
				strcpy(propname, "rx_rgmii");
			else
				scnprintf(propname, sizeof(propname), "rx_%s", phy_modes(intf_mode));
			emac->rx_clk = devm_get_clk_from_child(dev, child, propname);
			if (IS_ERR(emac->rx_clk)) {
				emac->rx_clk = NULL;
				HM_MSG_DEV_ERR(dev, "No RX clocks declared on EMAC%d for interface %s\n", id, phy_modes(intf_mode));
			}
		}
#endif /* PFE_CFG_PFE_MASTER */
#ifdef PFE_CFG_PFE_SLAVE
#ifdef PFE_CFG_PFE_SLAVE_PHY
		if (pfeng_netif_cfg_has_emac(netif_cfg)) {
			struct pfeng_emac *emac = &priv->emac[netif_cfg->phyif_id];
			__maybe_unused struct device_node *phy_handle;
			phy_interface_t intf_mode;

			HM_MSG_DEV_INFO(dev, "EMAC%d has an emac", id);
			emac->phyless = false;
			phy_handle = of_parse_phandle(child, "phy-handle", 0);
			if (!phy_handle) {
				HM_MSG_DEV_INFO(dev, "EMAC%d PHY less SGMII\n", netif_cfg->phyif_id);
				emac->phyless = true;
			}
			of_node_put(phy_handle);

			/* Interface mode */
			ret = of_get_phy_mode(child, &intf_mode);
			if (ret) {
				HM_MSG_DEV_WARN(dev, "Failed to read phy-mode\n");
				/* for non managable interface */
				intf_mode = PHY_INTERFACE_MODE_INTERNAL;
			}
			HM_MSG_DEV_INFO(dev, "EMAC%d interface mode: %d", id, intf_mode);
			emac->intf_mode = intf_mode;
		} else {
			HM_MSG_DEV_INFO(dev, "EMAC%d does not has an emac", id);
		}
#endif /* PFE_CFG_PFE_SLAVE_PHY */
#endif /* PFE_CFG_PFE_SLAVE */
		list_add_tail(&netif_cfg->lnode, &priv->netif_cfg_list);
	} /* foreach PFENG_DT_COMPATIBLE_LOGIF */

	HM_MSG_DEV_INFO(dev, "HIF channels mask: 0x%04x", pfe_cfg->hif_chnls_mask);

	/* Decrement HIF refcount to use simple check for zero */
	for (i = 0; i < PFENG_PFE_HIF_CHANNELS; i++)
		if (priv->hif_chnl[i].refcount)
			priv->hif_chnl[i].refcount--;

	/*
	 * MDIO
	 * ("nxp,s32g-pfe-mdio")
	 *
	 * Describes PFE_MDIO block
	 */
	for_each_available_child_of_node(np, child) {
		int id;
		struct pfeng_emac *emac;

		if (!of_device_is_available(child))
			continue;

		if (!of_device_is_compatible(child, "nxp,s32g-pfe-mdio"))
			continue;

		id = pfeng_of_get_addr(child);
		if (id < 0)
			continue;

		if (id >= PFENG_PFE_EMACS)
			continue;

		emac = &priv->emac[id];

		/* Link DT node for embedded MDIO bus */
		emac->dn_mdio = child;

		emac->enabled = true;

	} /* foreach PFENG_DT_COMPATIBLE_EMAC */

	return 0;

err:
	if (child)
		of_node_put(child);
	pfeng_dt_release_config(priv);

	return ret;
}
