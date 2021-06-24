/*
 * Copyright 2019-2021 NXP
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
#include <linux/processor.h>
#include <linux/pinctrl/consumer.h>
#include <soc/s32/revision.h>

#include "pfe_cfg.h"

/*
 * S32G soc specific addresses
 */
#define S32G_MAIN_GPR_PFE_COH_EN		0x0
#define S32G_MAIN_GPR_PFE_PWR_CTRL		0x20
#define GPR_PFE_COH_EN_UTIL			(1 << 5)
#define GPR_PFE_COH_EN_HIF3			(1 << 4)
#define GPR_PFE_COH_EN_HIF2			(1 << 3)
#define GPR_PFE_COH_EN_HIF1			(1 << 2)
#define GPR_PFE_COH_EN_HIF0			(1 << 1)
#define GPR_PFE_COH_EN_HIF_0_3_MASK		(GPR_PFE_COH_EN_HIF0 | GPR_PFE_COH_EN_HIF1 | \
						 GPR_PFE_COH_EN_HIF2 | GPR_PFE_COH_EN_HIF3)
#define GPR_PFE_COH_EN_DDR			(1 << 0)
#define S32G_MAIN_GPR_PFE_EMACX_INTF_SEL	0x4
#define GPR_PFE_EMACn_PWR_ACK(n)		(1 << (9 + n)) /* RD Only */
#define GPR_PFE_EMACn_PWR_ISO(n)		(1 << (6 + n))
#define GPR_PFE_EMACn_PWR_DWN(n)		(1 << (3 + n))
#define GPR_PFE_EMACn_PWR_CLAMP(n)		(1 << (0 + n))
#define GPR_PFE_EMAC_IF_MII			(1)
#define GPR_PFE_EMAC_IF_RMII			(9)
#define GPR_PFE_EMAC_IF_RGMII			(2)
#define GPR_PFE_EMAC_IF_SGMII			(0)
#define GPR_PFE_EMACn_IF(n,i)			(i << (n * 4))

/* Logical interface represents DT ethernet@ node */
#define PFENG_DT_COMPATIBLE_LOGIF		"fsl,pfeng-logif"
/* HIF represents DT hif@ node */
#define PFENG_DT_COMPATIBLE_HIF			"fsl,pfeng-hif"
/* EMAC represents DT emac@ node */
#define PFENG_DT_COMPATIBLE_EMAC		"fsl,pfeng-emac"
/* MDIO represents DT mdio@ node */
#define PFENG_DT_COMPATIBLE_MDIO		"fsl,pfeng-mdio"

/* Major IP version for cut2.0 */
#define PFE_IP_MAJOR_VERSION_CUT2		2

/* PFE SYS CLK is 300MHz */
#define PFE_CLK_SYS_RATE			300000000

/* PFE TS CLK is 200MHz */
#define PFE_CLK_TS_RATE				200000000

#include "pfeng.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Petrous <jan.petrous@nxp.com>");
#ifdef PFE_CFG_PFE_MASTER
MODULE_DESCRIPTION("PFEng driver");
MODULE_FIRMWARE(PFENG_FW_CLASS_NAME);
MODULE_FIRMWARE(PFENG_FW_UTIL_NAME);
#elif PFE_CFG_PFE_SLAVE
MODULE_DESCRIPTION("PFEng SLAVE driver");
#endif
MODULE_VERSION(PFENG_DRIVER_VERSION);

static const struct of_device_id pfeng_id_table[] = {
#ifdef PFE_CFG_PFE_MASTER
#if (PFE_CFG_IP_VERSION < PFE_CFG_IP_VERSION_NPU_7_14a)
	{ .compatible = "fsl,s32g274a-pfeng-cut1.1" },
#else
	{ .compatible = "fsl,s32g274a-pfeng" },
#endif /* PFE_CFG_IP_VERSION_NPU_7_14a */
#elif PFE_CFG_PFE_SLAVE
	{ .compatible = "fsl,s32g274a-pfeng-slave" },
#endif
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

static const u32 default_msg_level = (
	NETIF_MSG_DRV | NETIF_MSG_PROBE |
	NETIF_MSG_LINK | NETIF_MSG_IFUP |
	NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

static int msg_verbosity = PFE_CFG_VERBOSITY_LEVEL;
module_param(msg_verbosity, int, 0644);
MODULE_PARM_DESC(msg_verbosity, "\t 0 - 9, default 4");

#ifdef PFE_CFG_PFE_MASTER
static char *fw_class_name;
module_param(fw_class_name, charp, 0444);
MODULE_PARM_DESC(fw_class_name, "\t The name of CLASS firmware file (default: read from device-tree or " PFENG_FW_CLASS_NAME ")");

static char *fw_util_name;
module_param(fw_util_name, charp, 0444);
MODULE_PARM_DESC(fw_util_name, "\t The name of UTIL firmware file (default: read from device-tree or " PFENG_FW_UTIL_NAME ")");
#endif

#ifdef PFE_CFG_PFE_SLAVE
static int master_ihc_chnl = HIF_CFG_MAX_CHANNELS;
module_param(master_ihc_chnl, int, 0644);
MODULE_PARM_DESC(master_ihc_chnl, "\t 0 - <max-hif-chn-number>, default read from DT or invalid");
#endif

#ifdef PFE_CFG_PFE_MASTER

#if (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a)
static int pfeng_s32g_set_port_coherency(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	void *syscon;
	int ret = 0;
	u32 val;

	syscon = ioremap(priv->syscon.start, priv->syscon.end - priv->syscon.start);
	if(!syscon) {
		dev_err(dev, "cannot map GPR, aborting (INTF_SEL)\n");
		return -EIO;
	}

	val = hal_read32(syscon + S32G_MAIN_GPR_PFE_COH_EN);
	val |= GPR_PFE_COH_EN_HIF_0_3_MASK;
	hal_write32(val, syscon + S32G_MAIN_GPR_PFE_COH_EN);

	val = hal_read32(syscon + S32G_MAIN_GPR_PFE_COH_EN);
	if ((val & GPR_PFE_COH_EN_HIF_0_3_MASK) == GPR_PFE_COH_EN_HIF_0_3_MASK) {
		dev_info(dev, "PFE port coherency enabled, mask 0x%x\n", val);
	} else {
		dev_err(dev, "Failed to enable port coherency (mask 0x%x)\n", val);
		ret = -EINVAL;
	}

	iounmap(syscon);
	return ret;
}
#else
#define pfeng_s32g_set_port_coherency(priv)	(int)0
#endif

static unsigned int xlate_to_s32g_intf(unsigned int n, phy_interface_t intf)
{
	switch(intf) {
		default: /* SGMII is the default */
		case PHY_INTERFACE_MODE_SGMII:
			return GPR_PFE_EMACn_IF(n, GPR_PFE_EMAC_IF_SGMII);

		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			return GPR_PFE_EMACn_IF(n, GPR_PFE_EMAC_IF_RGMII);

		case PHY_INTERFACE_MODE_RMII:
			return GPR_PFE_EMACn_IF(n, GPR_PFE_EMAC_IF_RMII);

		case PHY_INTERFACE_MODE_MII:
			return GPR_PFE_EMACn_IF(n, GPR_PFE_EMAC_IF_MII);
	}
}

static int pfeng_s32g_set_emac_interfaces(struct pfeng_priv *priv, phy_interface_t emac0_intf, phy_interface_t emac1_intf, phy_interface_t emac2_intf)
{
	void *syscon;
	u32 val;

	syscon = ioremap(priv->syscon.start, priv->syscon.end - priv->syscon.start);
	if(!syscon) {
		dev_err(&priv->pdev->dev, "cannot map GPR, aborting (INTF_SEL)\n");
		return -EIO;
	}
	/* set up interfaces */
	val = xlate_to_s32g_intf(0, emac0_intf) | xlate_to_s32g_intf(1, emac1_intf) | xlate_to_s32g_intf(2, emac2_intf);
	hal_write32(val, syscon + S32G_MAIN_GPR_PFE_EMACX_INTF_SEL);

	dev_info(&priv->pdev->dev, "Interface selected: EMAC0: 0x%x EMAC1: 0x%x EMAC2: 0x%x\n", emac0_intf, emac1_intf, emac2_intf);

	/* power down and up EMACs */
	hal_write32(GPR_PFE_EMACn_PWR_DWN(0) | GPR_PFE_EMACn_PWR_DWN(1) | GPR_PFE_EMACn_PWR_DWN(2), syscon + S32G_MAIN_GPR_PFE_PWR_CTRL);
	usleep_range(100, 500);
	hal_write32(0, syscon + S32G_MAIN_GPR_PFE_PWR_CTRL);

	iounmap(syscon);

	return 0;
}

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

static int release_config_from_dt(struct pfeng_priv *priv)
{
#ifdef PFE_CFG_PFE_MASTER
	int id;

	/* Free EMAC clocks */
	for (id = 0; id < PFENG_PFE_EMACS; id++) {
		struct pfeng_emac *emac = &priv->emac[id];
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
		struct device *dev = &priv->pdev->dev;

		/* Remove device depeendency for SerDes */
		if (emac->intf_mode == PHY_INTERFACE_MODE_SGMII && emac->serdes_phy)
			device_link_remove(dev, &emac->serdes_phy->dev);
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

static int create_config_from_dt(struct pfeng_priv *priv)
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
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_PFE_SLAVE
	if (of_property_read_u32(np, "fsl,pfeng-master-hif-channel", &propval)) {
		dev_err(dev, "Invalid hif-channel value");
		priv->ihc_master_chnl = HIF_CFG_MAX_CHANNELS;
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

#ifdef PFE_CFG_PFE_MASTER
		netif_cfg->tx_inject = true;
#else
		/* Must be FALSE for SLAVE driver */
		netif_cfg->tx_inject = false;
#endif /* PFE_CFG_PFE_MASTER */

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
		if (ret || id > 2) {
			dev_err(dev, "The EMAC id is invalid: %d\n", id);
			ret = -EINVAL;
			goto err;
		}
#endif /* PFE_CFG_PFE_MASTER */

		netif_cfg->emac = id;
		emac_list |= 1 << id;
		dev_info(dev, "logif(%s) EMAC: %u", netif_cfg->name, netif_cfg->emac);

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
			if (!emac->serdes_phy) {
				dev_err(dev, "SerDes PHY for EMAC%d was not found\n", id);
			} else {
				/* Add device depeendency for SerDes */
				if (device_link_add(dev, &emac->serdes_phy->dev, DL_FLAG_STATELESS /*| DL_FLAG_PM_RUNTIME*/))
					dev_err(dev, "Failed to enable SerDes PM dependency for EMAC%d\n", id);
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
	release_config_from_dt(priv);

	return ret;
}

static struct pfeng_priv *pfeng_drv_alloc(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pfeng_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return NULL;

	priv->pdev = pdev;

	priv->pfe_cfg = devm_kzalloc(dev, sizeof(*priv->pfe_cfg), GFP_KERNEL);
	if(!priv->pfe_cfg)
		goto err_cfg_alloc;

	INIT_LIST_HEAD(&priv->netif_cfg_list);
	INIT_LIST_HEAD(&priv->netif_list);

	/* set EMAC interface mode to invalid value */
	priv->emac[0].intf_mode = -1;
	priv->emac[1].intf_mode = -1;
	priv->emac[2].intf_mode = -1;

	/* cfg defaults */
	priv->msg_enable = default_msg_level;
	priv->msg_verbosity = msg_verbosity;

	/*
	 * Default size of routing table. Symbols PFE_CFG_RT_HASH_SIZE, PFE_CFG_RT_COLLISION_SIZE
	 * are defined in build_env.mak. Default size can be overridden later by device tree configuration.
	 */
#if defined(PFE_CFG_RTABLE_ENABLE)
	priv->pfe_cfg->rtable_hash_size = PFE_CFG_RT_HASH_SIZE;
	priv->pfe_cfg->rtable_collision_size = PFE_CFG_RT_COLLISION_SIZE;
#endif /* PFE_CFG_RTABLE_ENABLE */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	priv->ihc_tx_wq = create_singlethread_workqueue("pfeng-ihc-tx");
	if (!priv->ihc_tx_wq) {
		dev_err(dev, "Initialize of IHC TX WQ failed\n");
		goto err_cfg_alloc;
	}
	if (kfifo_alloc(&priv->ihc_tx_fifo, 32, GFP_KERNEL))
		goto err_cfg_alloc;
	INIT_WORK(&priv->ihc_tx_work, pfeng_ihc_tx_work_handler);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_PFE_SLAVE
	priv->ihc_slave_wq = create_singlethread_workqueue("pfeng-slave-init");
	if (!priv->ihc_slave_wq) {
		dev_err(dev, "Initialize of Slave WQ failed\n");
		goto err_cfg_alloc;
	}
#endif /* PFE_CFG_PFE_SLAVE */

	return priv;

err_cfg_alloc:
	devm_kfree(dev, priv);
	return NULL;
}

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_pfe_reset(struct pfeng_priv *priv)
{
	int ret;
	struct device *dev = &priv->pdev->dev;

	if (!priv->rst) {
		dev_err(dev, "Partition reset support disabled\n");
		return -ENOTSUP;
	}

	ret = reset_control_assert(priv->rst);
	if (ret) {
		dev_err(dev, "Failed to assert PFE reset: %d\n", ret);
		return ret;
	}

	udelay(100);

	ret = reset_control_deassert(priv->rst);
	if (ret) {
		dev_err(dev, "Failed to deassert PFE reset: %d\n", ret);
		return ret;
	}
	dev_info(dev, "PFE controller reset done\n");

	return 0;
}
#endif /* PFE_CFG_PFE_MASTER */

/**
 * pfeng_s32g_remove
 *
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and releases the platform resources.
 */
static int pfeng_drv_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pfeng_priv *priv = dev_get_drvdata(dev);

	if (!priv) {
		dev_err(dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

#ifdef PFE_CFG_PFE_SLAVE
	if (priv->ihc_slave_wq)
		destroy_workqueue(priv->ihc_slave_wq);
#endif /* PFE_CFG_PFE_SLAVE */

	/* Remove debugfs directory */
	pfeng_debugfs_remove(priv);

#ifdef PFE_CFG_PFE_MASTER
	pfeng_mdio_unregister(priv);
#endif /* PFE_CFG_PFE_MASTER */

	/* Remove netifs */
	pfeng_netif_remove(priv);

	pfeng_hif_remove(priv);

	/* PFE platform remove */
	if (priv->pfe_platform) {
		if (pfe_platform_remove() != EOK)
			dev_err(dev, "PFE Platform not stopped successfully\n");
		else {
			priv->pfe_platform = NULL;
			dev_info(dev, "PFE Platform stopped\n");
		}
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (priv->ihc_tx_wq)
		destroy_workqueue(priv->ihc_tx_wq);
	if (kfifo_initialized(&priv->ihc_tx_fifo))
		kfifo_free(&priv->ihc_tx_fifo);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_PFE_MASTER
	/* Release firmware */
	if (priv->pfe_cfg->fw)
		pfeng_fw_free(priv);
#endif /* PFE_CFG_PFE_MASTER */

	release_config_from_dt(priv);

	/* Free clocks */
	if (priv->clk_ptp) {
		clk_disable_unprepare(priv->clk_ptp);
		clk_put(priv->clk_ptp);
		priv->clk_ptp = NULL;
	}
	if (priv->clk_pe) {
		clk_disable_unprepare(priv->clk_pe);
		clk_put(priv->clk_pe);
		priv->clk_pe = NULL;
	}
	if (priv->clk_sys) {
		clk_disable_unprepare(priv->clk_sys);
		clk_put(priv->clk_sys);
		priv->clk_sys = NULL;
	}

	dev_set_drvdata(dev, NULL);

	/* Shutdown memory management */
	oal_mm_shutdown();

	return 0;
}

static void pfeng_soc_version_check(struct device *dev)
{
	struct s32_soc_rev soc_rev;
	int ret;

	ret = s32_siul2_nvmem_get_soc_revision(dev, "soc_revision", &soc_rev);
	if (ret) {
		dev_warn(dev, "Failed to read SoC version (err: %d)\n", ret);
		return;
	}

#if (PFE_CFG_IP_VERSION < PFE_CFG_IP_VERSION_NPU_7_14a)
	dev_info(dev, "Errata: s32g2 cut 1.1 errata activated\n");
	if (soc_rev.major >= PFE_IP_MAJOR_VERSION_CUT2)
		dev_warn(dev, "Running cut 1.1 driver on SoC version %d.%d!\n",
			 soc_rev.major, soc_rev.minor);
#else
	if (soc_rev.major < PFE_IP_MAJOR_VERSION_CUT2)
		dev_warn(dev, "Running cut 2.0 driver on SoC version %d.%d!\n",
			 soc_rev.major, soc_rev.minor);
#endif
}

static int pfeng_dma_coherency_check(struct device *dev)
{
#if (PFE_CFG_IP_VERSION < PFE_CFG_IP_VERSION_NPU_7_14a)
	if (of_dma_is_coherent(dev->of_node)) {
		dev_err(dev, "DMA coherency enabled for cut 1.1 errata enabled driver!\n");
		return -EINVAL;
	}
#else
	if (!of_dma_is_coherent(dev->of_node))
		dev_warn(dev, "DMA coherency disabled - consider impact on device performance\n");
#endif

	return 0;
}

/**
 * pfeng_drv_probe
 *
 * @pdev: platform device pointer
 *
 * Description: This probing function gets called for all platform devices which
 * match the ID table and are not "owned" by other driver yet. This function
 * gets passed a "struct pplatform_device *" for each device whose entry in the ID table
 * matches the device. The probe functions returns zero when the driver choose
 * to take "ownership" of the device or an error code(-ve no) otherwise.
 */
static int pfeng_drv_probe(struct platform_device *pdev)
{
	struct pfeng_priv *priv;
	struct device *dev = &pdev->dev;
	__maybe_unused struct reset_control *rst;
	__maybe_unused int id;
	int ret;

	if (!pdev->dev.of_node)
		return -ENODEV;

	if (!of_match_device(pfeng_id_table, &pdev->dev))
		return -ENODEV;

	dev_info(dev, "PFEng ethernet driver loading ...\n");
	dev_info(dev, "Version: %s\n", PFENG_DRIVER_VERSION);

	/* Print MULTI-INSATNCE mode (MASTER/SLAVE/disabled) */
	dev_info(dev, "Multi instance support: %s\n",
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
#ifdef PFE_CFG_PFE_MASTER
						"MASTER"
#elif PFE_CFG_PFE_SLAVE
						"SLAVE"
#else
#error MULTI_INSTANCE_SUPPORT requires PFE_MASTER or PFE_SLAVE defined!
#endif
#else
						"disabled (standalone)"
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
		);

	dev_info(dev, "Compiled by: %s\n", __VERSION__);

	pfeng_soc_version_check(dev);

	ret = pfeng_dma_coherency_check(dev);
	if (ret)
		return ret;

#ifdef PFE_CFG_PFE_MASTER
	/* Attach to reset controller to reset S32G2 partition 2 */
	rst = devm_reset_control_get(dev, "pfe_part");
	if (IS_ERR(rst)) {
		dev_warn(dev, "Warning: Partition reset 'pfe_part' get failed: code %ld\n", PTR_ERR(rst));
		rst = NULL;
	}
#endif /* PFE_CFG_PFE_MASTER */

	/* Signal driver coherency mask */
	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32)) != 0) {
		dev_err(dev, "System does not support DMA, aborting\n");
		return -EINVAL;
	}

	/* Allocate driver context with defaults */
	priv = pfeng_drv_alloc(pdev);
	if(!priv) {
		ret = -ENOMEM;
		goto err_drv;
	}
	dev_set_drvdata(dev, priv);

	/* Overwrite defaults by DT values */
	ret = create_config_from_dt(priv);
	if (ret)
		goto err_drv;

#ifdef PFE_CFG_PFE_SLAVE
	/* HIF IHC channel number */
	if (master_ihc_chnl < HIF_CFG_MAX_CHANNELS)
		priv->ihc_master_chnl = master_ihc_chnl;
	if (priv->ihc_master_chnl >= HIF_CFG_MAX_CHANNELS) {
		dev_err(dev, "Slave mode: Master channel id is missing\n");
		ret = -EINVAL;
		goto err_drv;
	}
#endif

#ifdef PFE_CFG_PFE_MASTER
	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node)) {
		ret = pfeng_s32g_set_port_coherency(priv);
		if (ret)
			goto err_drv;
	}

	/* PFE_SYS clock */
	priv->clk_sys = clk_get(dev, "pfe_sys");
	if (IS_ERR(priv->clk_sys)) {
		dev_err(dev, "Failed to get pfe_sys clock\n");
		ret = IS_ERR(priv->clk_sys);
		priv->clk_sys = NULL;
		goto err_drv;
	}
	ret = clk_set_rate(priv->clk_sys, PFE_CLK_SYS_RATE);
	if (ret) {
		dev_err(dev, "Failed to set clock 'pfe_sys'. Error: %d\n", ret);
		goto err_drv;
	}
	ret = clk_prepare_enable(priv->clk_sys);
	if (ret) {
		dev_err(dev, "Failed to enable clock 'pfe_sys'. Error: %d\n", ret);
		goto err_drv;
	}

	/* PFE_PE clock */
	priv->clk_pe = clk_get(dev, "pfe_pe");
	if (IS_ERR(priv->clk_pe)) {
		dev_err(dev, "Failed to get pfe_pe clock\n");
		ret = IS_ERR(priv->clk_pe);
		priv->clk_pe = NULL;
		goto err_drv;
	}
	/* PE clock should be double the frequency of System clock */
	ret = clk_set_rate(priv->clk_pe, clk_get_rate(priv->clk_sys) * 2);
	if (ret) {
		dev_err(dev, "Failed to set clock 'pfe_pe'. Error: %d\n", ret);
		goto err_drv;
	}
	ret = clk_prepare_enable(priv->clk_pe);
	if (ret) {
		dev_err(dev, "Failed to enable clock 'pfe_pe'. Error: %d\n", ret);
		goto err_drv;
	}
	dev_info(dev, "Clocks: sys=%luMHz pe=%luMHz\n", clk_get_rate(priv->clk_sys) / 1000000, clk_get_rate(priv->clk_pe) / 1000000);

	/* Set correct PFE_EMACs interfaces */
	if(pfeng_s32g_set_emac_interfaces(priv,
		priv->emac[0].intf_mode,
		priv->emac[1].intf_mode,
		priv->emac[2].intf_mode))
		dev_err(dev, "WARNING: cannot enable power for EMACs\n");

	/* PFE Partition reset */
	priv->rst = rst;
	if (priv->rst) {
		ret = pfeng_pfe_reset(priv);
		if (ret)
			goto err_drv;
	}

	/* Prepare EMAC RX/TX clocks */
	for (id = 0; id < PFENG_PFE_EMACS; id++) {
		struct pfeng_emac *emac = &priv->emac[id];
		u64 clk_rate;

		if (!emac->enabled)
			continue;

		/* retrieve max rate */
		switch (emac->max_speed) {
		case SPEED_10:
			clk_rate = 2500000;
			break;
		case SPEED_100:
			clk_rate = 25000000;
			break;
		case SPEED_1000:
		default:
			clk_rate = 125000000;
			break;
		}

		if (emac->tx_clk) {
			ret = clk_set_rate(emac->tx_clk, clk_rate);
			if (ret)
				dev_err(dev, "Failed to set TX clock on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			else {
				ret = clk_prepare_enable(emac->tx_clk);
				if (ret)
					dev_err(dev, "Failed to enable TX clocks on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			}
			if (ret) {
				devm_clk_put(dev, emac->tx_clk);
				emac->tx_clk = NULL;
			} else
				dev_info(dev, "TX clock on EMAC%d for interface %s installed\n", id, phy_modes(emac->intf_mode));
		}

		if (emac->rx_clk) {
			ret = clk_set_rate(emac->rx_clk, clk_rate);
			if (ret)
				dev_err(dev, "Failed to set RX clock on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			else {
				ret = clk_prepare_enable(emac->rx_clk);
				if (ret)
					dev_err(dev, "Failed to enable RX clocks on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			}
			if (ret) {
				devm_clk_put(dev, emac->rx_clk);
				emac->rx_clk = NULL;
			} else
				dev_info(dev, "RX clock on EMAC%d for interface %s installed\n", id, phy_modes(emac->intf_mode));
		}
	}
#endif

	/* PFE platform layer init */
	oal_mm_init(dev);

#ifdef PFE_CFG_PFE_MASTER
	/* Build CLASS firmware name */
	if (fw_class_name && strlen(fw_class_name))
		priv->fw_class_name = fw_class_name;
	if (!priv->fw_class_name || !strlen(priv->fw_class_name)) {
		dev_err(dev, "CLASS firmware is unknown\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Build UTIL firmware name */
	if (fw_util_name && strlen(fw_util_name))
		priv->fw_util_name = fw_util_name;
	if (!priv->fw_util_name || !strlen(priv->fw_util_name)) {
		dev_info(dev, "UTIL firmware not requested. Disable UTIL\n");
		priv->pfe_cfg->enable_util = false;
	} else
		priv->pfe_cfg->enable_util = true;

	/* Request firmware(s) */
	ret = pfeng_fw_load(priv, priv->fw_class_name, priv->fw_util_name);
	if (ret)
		goto err_drv;
#endif /* PFE_CFG_PFE_MASTER */

	/* Start PFE Platform */
	ret = pfe_platform_init(priv->pfe_cfg);
	if (ret)
		goto err_drv;
	priv->pfe_platform = pfe_platform_get_instance();
	if (!priv->pfe_platform) {
		dev_err(dev, "Could not get PFE platform instance\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Create debugfs */
	pfeng_debugfs_create(priv);

#ifdef PFE_CFG_PFE_MASTER
	/* Prepare PTP clock */
	priv->clk_ptp_reference = 0U;
	priv->clk_ptp = clk_get(dev, "pfe_ts");
	if (IS_ERR(priv->clk_ptp)) {
		dev_warn(dev, "Failed to get pfe_ts clock. PTP will be disabled.\n");
		priv->clk_ptp = NULL;
	} else {
		ret = clk_set_rate(priv->clk_ptp, PFE_CLK_TS_RATE);
		if (ret) {
			dev_warn(dev, "Failed to set pfe_ts clock. PTP will be disabled.\n");
			priv->clk_ptp = NULL;
		} else {
			ret = clk_prepare_enable(priv->clk_ptp);
			if (ret) {
				priv->clk_ptp = NULL;
				dev_err(dev, "Failed to enable clock pfe_ts: %d\n", ret);
			} else
				priv->clk_ptp_reference = clk_get_rate(priv->clk_ptp);
		}
	}

	/* Create MDIO buses */
	pfeng_mdio_register(priv);
#endif /* PFE_CFG_PFE_MASTER */

	/* Create HIFs */
	ret = pfeng_hif_create(priv);
	if (ret)
		goto err_drv;

	/* Create net interfaces */
	ret = pfeng_netif_create(priv);
	if (ret)
		goto err_drv;

	return 0;

err_drv:
	pfeng_drv_remove(pdev);

	return ret;
}

/* PM support */

#ifdef CONFIG_PM_SLEEP
/**
 * pfeng_pm_suspend
 * @dev: device pointer
 * Description: this function is invoked when suspend the driver and it direcly
 * call the main suspend function and then, if required, on some platform, it
 * can call an exit helper.
 */
static int pfeng_drv_pm_suspend(struct device *dev)
{
	struct pfeng_priv *priv = dev_get_drvdata(dev);

	dev_info(dev, "Suspending driver\n");

	priv->in_suspend = true;

	pfeng_debugfs_remove(priv);

#ifdef PFE_CFG_PFE_SLAVE
	if (priv->ihc_slave_wq)
		destroy_workqueue(priv->ihc_slave_wq);
#endif /* PFE_CFG_PFE_SLAVE */

#ifdef PFE_CFG_PFE_MASTER
	/* MDIO buses */
	pfeng_mdio_suspend(priv);
#endif /* PFE_CFG_PFE_MASTER */

	/* NETIFs */
	pfeng_netif_suspend(priv);

	/* HIFs stop */
	pfeng_hif_remove(priv);

	/* PFE platform remove */
	if (priv->pfe_platform) {
		if (pfe_platform_remove() != EOK)
			dev_err(dev, "PFE Platform not stopped successfully\n");
		else {
			priv->pfe_platform = NULL;
			dev_info(dev, "PFE Platform stopped\n");
		}
	}

	pinctrl_pm_select_sleep_state(dev);

	/* Stop clocks */
	if (priv->clk_ptp) {
		clk_disable_unprepare(priv->clk_ptp);
	}
	if (priv->clk_pe) {
		clk_disable_unprepare(priv->clk_pe);
	}
	if (priv->clk_sys) {
		clk_disable_unprepare(priv->clk_sys);
	}

	return 0;
}

/**
 * pfeng_pm_resume
 * @dev: device pointer
 * Description: this function is invoked when resume the driver before calling
 * the main resume function, on some platforms, it can call own init helper
 * if required.
 */
static int pfeng_drv_pm_resume(struct device *dev)
{
	struct pfeng_priv *priv = dev_get_drvdata(dev);
	int ret;

	dev_info(dev, "Resuming driver\n");

#ifdef PFE_CFG_PFE_MASTER
	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node))
		ret = pfeng_s32g_set_port_coherency(priv);

	/* Start clocks */
	if (!priv->clk_sys) {
		dev_err(dev, "Main clock 'pfe_sys' disappeared\n");
		return -ENODEV;
	}
	ret = clk_set_rate(priv->clk_sys, PFE_CLK_SYS_RATE);
	if (ret) {
		dev_err(dev, "Failed to set clock 'pfe_sys'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_prepare_enable(priv->clk_sys);
	if (ret) {
		dev_err(dev, "Failed to enable clock 'pfe_sys'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_set_rate(priv->clk_pe, clk_get_rate(priv->clk_sys) * 2);
	if (ret) {
		dev_err(dev, "Failed to set clock 'pfe_pe'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_prepare_enable(priv->clk_pe);
	if (ret) {
		dev_err(dev, "Failed to enable clock 'pfe_pe'. Error: %d\n", ret);
		return -EINVAL;
	}

	/* Set correct PFE_EMACs interfaces */
	if(pfeng_s32g_set_emac_interfaces(priv,
		priv->emac[0].intf_mode,
		priv->emac[1].intf_mode,
		priv->emac[2].intf_mode))
		dev_err(dev, "WARNING: cannot enable power for EMACs\n");

	/* PFE reset */
	ret = pfeng_pfe_reset(priv);
	if (ret) {
		dev_err(dev, "Failed to reset PFE controller\n");
		goto err_pfe_init;
	}
#endif

	/* Start PFE Platform */
	ret = pfe_platform_init(priv->pfe_cfg);
	if (ret) {
		dev_err(dev, "Could not init PFE platform instance. Error %d\n", ret);
		goto err_pfe_init;
	}
	priv->pfe_platform = pfe_platform_get_instance();
	if (!priv->pfe_platform) {
		dev_err(dev, "Could not get PFE platform instance\n");
		ret = -EINVAL;
		goto err_pfe_get;
	}

	/* Create debugfs */
	pfeng_debugfs_create(priv);

#ifdef PFE_CFG_PFE_MASTER
	/* PTP clock */
	if (priv->clk_ptp) {
		ret = clk_set_rate(priv->clk_ptp, PFE_CLK_TS_RATE);
		if (ret) {
			dev_warn(dev, "Failed to set pfe_ts clock. PTP will be disabled.\n");
			clk_put(priv->clk_ptp);
			priv->clk_ptp = NULL;
		} else {
			ret = clk_prepare_enable(priv->clk_ptp);
			if (ret) {
				dev_warn(dev, "Failed to enable clock 'pfe_ts'. PTP will be disabled.\n");
				/* Free clock, now is unusable */
				clk_put(priv->clk_ptp);
				priv->clk_ptp = NULL;
			}
		}
	}

	/* MDIO buses */
	pfeng_mdio_resume(priv);
#endif /* PFE_CFG_PFE_MASTER */


	/* Create HIFs */
	ret = pfeng_hif_create(priv);
	if (ret)
		goto err_drv;

	/* Create net interfaces */
	ret = pfeng_netif_resume(priv);
	if (ret)
		goto err_drv;

	priv->in_suspend = false;

	return 0;

err_drv:
err_pfe_get:
err_pfe_init:

	return ret;
}
#endif /* CONFIG_PM_SLEEP */

SIMPLE_DEV_PM_OPS(pfeng_drv_pm_ops,
			pfeng_drv_pm_suspend,
			pfeng_drv_pm_resume);

/* platform data */

static struct platform_driver pfeng_platform_driver = {
	.probe = pfeng_drv_probe,
	.remove = pfeng_drv_remove,
	.driver = {
		.name = PFENG_DRIVER_NAME,
		.pm = &pfeng_drv_pm_ops,
		.of_match_table = of_match_ptr(pfeng_id_table),
	},
};

module_platform_driver(pfeng_platform_driver);
