/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD OR GPL-2.0
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

#include "pfe_cfg.h"

/*
 * S32G soc specific addresses
 */
#define S32G_MAIN_GPR_PFE_COH_EN			0x0
#define S32G_MAIN_GPR_PFE_PWR_CTRL			0x20
#define GPR_PFE_COH_EN_UTIL					(1 << 5)
#define GPR_PFE_COH_EN_HIF3					(1 << 4)
#define GPR_PFE_COH_EN_HIF2					(1 << 3)
#define GPR_PFE_COH_EN_HIF1					(1 << 2)
#define GPR_PFE_COH_EN_HIF0					(1 << 1)
#define GPR_PFE_COH_EN_DDR					(1 << 0)
#define S32G_MAIN_GPR_PFE_EMACX_INTF_SEL	0x4
#define GPR_PFE_EMACn_PWR_ACK(n)			(1 << (9 + n)) /* RD Only */
#define GPR_PFE_EMACn_PWR_ISO(n)			(1 << (6 + n))
#define GPR_PFE_EMACn_PWR_DWN(n)			(1 << (3 + n))
#define GPR_PFE_EMACn_PWR_CLAMP(n)			(1 << (0 + n))
#define GPR_PFE_EMAC_IF_MII					(1)
#define GPR_PFE_EMAC_IF_RMII				(9)
#define GPR_PFE_EMAC_IF_RGMII				(2)
#define GPR_PFE_EMAC_IF_SGMII				(0)
#define GPR_PFE_EMACn_IF(n,i)				(i << (n * 4))

#include "pfeng.h"

static const struct of_device_id pfeng_id_table[] = {
	{ .compatible = "fsl,s32g274a-pfeng" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

/*
 * Search for memory-region in DT and declare it DMA coherent
 *
 */
static int init_reserved_memory(struct device *dev)
{
	struct resource res;
	struct device_node *np;
	int ret;

	np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!np) {
		dev_err(dev, "Reserved memory was not found\n");
		return -ENOMEM;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret < 0) {
		dev_err(dev, "Reserved memory is invalid\n");
		return -ENOMEM;
	}
	dev_info(dev, "Found reserved memory at p0x%llx size 0x%llx\n", res.start, res.end - res.start + 1);

	ret = dmam_declare_coherent_memory(dev, res.start, res.start,
		res.end - res.start + 1, DMA_MEMORY_EXCLUSIVE);

	return ret;

}

static unsigned int xlate_to_s32g_intf(unsigned int n, phy_interface_t intf)
{
	switch(intf) {
		default: /* SGMII is the default */
		case PHY_INTERFACE_MODE_SGMII:
			return GPR_PFE_EMACn_IF(n, GPR_PFE_EMAC_IF_SGMII);

		case PHY_INTERFACE_MODE_RGMII:
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

	syscon = ioremap_nocache(priv->plat.syscon.start, priv->plat.syscon.end - priv->plat.syscon.start);
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

static int release_config(struct pfeng_priv *priv)
{
	struct pfeng_eth *eth;

	list_for_each_entry(eth, &priv->plat.eth_list, lnode) {
		if(!eth)
			continue;

		if (eth->dn)
			of_node_put(eth->dn);

		kfree(eth);
	}

	return 0;
}

static int create_config_from_dt(struct pfeng_priv *priv)
{
	struct platform_device *pdev = priv->pdev;
	struct device_node *np = pdev->dev.of_node;
	pfe_platform_config_t *pfe_cfg = priv->cfg;
	struct pfeng_eth *eth;
	struct resource *res;
	struct device_node *child = NULL;
	int i, irq, ret = 0;
	u32 propval;

	/* Get the base address of device */
	res = platform_get_resource(priv->pdev, IORESOURCE_MEM, 0);
	if(unlikely(!res)) {
		dev_err(&pdev->dev, "Cannot find mem resource, aborting\n");
		return -EIO;
	}
	pfe_cfg->cbus_base = res->start;
	pfe_cfg->cbus_len = res->end - res->start + 1;
	dev_info(&pdev->dev, "Cbus addr 0x%llx size 0x%llx\n", pfe_cfg->cbus_base, pfe_cfg->cbus_len);

	/* S32G Main GPRs */
	res = platform_get_resource(priv->pdev, IORESOURCE_MEM, 1);
	if(unlikely(!res)) {
		dev_err(&pdev->dev, "Cannot find syscon resource, aborting\n");
		return -EIO;
	}
	priv->plat.syscon.start = res->start;
	priv->plat.syscon.end = res->end;
	dev_dbg(&pdev->dev, "Syscon addr 0x%llx size 0x%llx\n", priv->plat.syscon.start, priv->plat.syscon.end - priv->plat.syscon.start);

	/* Firmware name */
	if (of_find_property(np, "firmware-name", NULL))
		if (!of_property_read_string(np, "firmware-name", &priv->fw_name)) {
			dev_info(&pdev->dev, "firmware-name: %s", priv->fw_name);
		}

	/* IRQ hif0 - hif3 */
	for (i = 0; i < HIF_CFG_MAX_CHANNELS; i++) {
		char irqname[8];

		scnprintf(irqname, sizeof(irqname), "hif%i", i);
		irq = platform_get_irq_byname(pdev, irqname);
		if (irq < 0) {
			dev_err(&pdev->dev, "Cannot find irq resource 'hif%i', aborting\n", i);
			return -EIO;
		}
		pfe_cfg->irq_vector_hif_chnls[i] = irq;
		dev_dbg(&pdev->dev, "irq 'hif%i': %u\n", i, irq);
	}

	/* IRQ nocpy */
	irq = platform_get_irq_byname(pdev, "nocpy");
	if (irq < 0) {
		dev_err(&pdev->dev, "Cannot find irq resource 'nocpy', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_hif_nocpy = irq;
	dev_dbg(&pdev->dev, "irq 'nocpy' : %u\n", irq);

	/* IRQ bmu */
	irq = platform_get_irq_byname(pdev, "bmu");
	if (irq < 0) {
		dev_err(&pdev->dev, "Cannot find irq resource 'bmu', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_bmu = irq;
	dev_dbg(&pdev->dev, "irq 'bmu' : %u\n", irq);

	/* IRQ upe/gpt */
	irq = platform_get_irq_byname(pdev, "upegpt");
	if (irq < 0) {
		dev_err(&pdev->dev, "Cannot find irq resource 'upegpt', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_upe_gpt = irq;
	dev_dbg(&pdev->dev, "irq 'upegpt' : %u\n", irq);

	/* IRQ safety */
	irq = platform_get_irq_byname(pdev, "safety");
	if (irq < 0) {
		dev_err(&pdev->dev, "Cannot find irq resource 'safety', aborting\n");
		return -EIO;
	}
	pfe_cfg->irq_vector_safety = irq;
	dev_dbg(&pdev->dev, "irq 'safety' : %u\n", irq);

#if 0 /* MC HIF mode unsupported yet */
	priv->plat.hif_chnl_mc = HIF_CFG_MAX_CHANNELS;
	if (priv->plat.hif_mode == PFENG_HIF_MODE_MC) {
		/* HIF channel for MC mode */
		propval = HIF_CFG_MAX_CHANNELS;
		if (of_find_property(np, "fsl,pfeng-hif-channel", NULL)) {
			if (of_property_read_u32(np, "fsl,pfeng-hif-channel", &propval)) {
				dev_err(&pdev->dev, "Invalid MC hif-channel");
				return -EINVAL;
			}
			if (of_property_count_elems_of_size(np, "fsl,pfeng-hif-channel", sizeof(u32)) > 1)
				dev_warn(&pdev->dev, "Only one HIF channel is supported. HIF%u is used.\n", propval);
		}
		if (propval >= HIF_CFG_MAX_CHANNELS) {
			dev_err(&pdev->dev, "Unsupported HIF channel number %u, aborting\n", propval);
			return -EINVAL;
		}
		priv->plat.hif_chnl_mc = propval;
		dev_info(&pdev->dev, "HIF channel %u in MC mode", propval);
		/* signal to platform to create channel */
		pfe_cfg->hif_chnls_mask = 1 << propval;
	} else {
		if (of_find_property(np, "fsl,pfeng-hif-channel", NULL)) {
			dev_warn(&pdev->dev, "Global HIF channel unsupported in HIF SC mode.");
		}
		/* Signal 'no MC channel configured' */
		pfe_cfg->hif_chnls_mask = 0;
	}
#endif

	/* Interfaces */
	for_each_available_child_of_node(np, child) {
		if (!of_device_is_available(child))
			continue;
		if (!of_device_is_compatible(child, PFENG_DT_NODENAME_ETHERNET))
			continue;

		eth = kzalloc(sizeof(*eth), GFP_KERNEL);
		if (!eth) {
			dev_err(&pdev->dev, "No memory for DT config\n");
			ret = -ENOMEM;
			goto err;
		}

		/* HIF channel for SC mode */
		propval = HIF_CFG_MAX_CHANNELS;
		if (of_find_property(child, "fsl,pfeng-hif-channel", NULL)) {
			if (of_property_read_u32(child, "fsl,pfeng-hif-channel", &propval)) {
				dev_err(&pdev->dev, "Invalid hif-channel value");
				ret = -EINVAL;
				goto err;
			}
			if (of_property_count_elems_of_size(child, "fsl,pfeng-hif-channel", sizeof(u32)) > 1)
				dev_warn(&pdev->dev, "Only one HIF channel is supported. HIF%u is used.\n", propval);

			if (propval >= HIF_CFG_MAX_CHANNELS) {
				dev_err(&pdev->dev, "Unsupported HIF channel number %u, aborting\n", propval);
				ret = -EINVAL;
				goto err;
			}
			/* check if the channel was not already used */
			if (pfe_cfg->hif_chnls_mask & (1 << propval)) {
				dev_err(&pdev->dev, "HIF channel number %u already used, aborting\n", propval);
				ret = -EINVAL;
				goto err;
			}
			dev_info(&pdev->dev, "HIF channel %u in SC mode", propval);
			/* signal to platform to create channel */
			pfe_cfg->hif_chnls_mask |= 1 << propval;
		}
		eth->hif_chnl_sc = propval;

		if (!of_find_property(child, "fsl,pfeng-if-name", NULL) ||
			of_property_read_string(child, "fsl,pfeng-if-name", &eth->name)) {
			dev_warn(&pdev->dev, "Valid ethernet name is missing (property 'fsl,pfeng-if-name')\n");

			kfree(eth);
			continue;
		}

		/* MAC eth address */
		eth->addr = (u8 *)of_get_mac_address(child);
		if (eth->addr)
			dev_dbg(&pdev->dev, "DT mac addr: %pM", eth->addr);

		/* fixed-link check */
		eth->fixed_link = of_phy_is_fixed_link(child);

		/* Interface mode */
		eth->intf_mode = of_get_phy_mode(child);
		if (eth->intf_mode < 0)
			/* for non managable interface */
			eth->intf_mode = PHY_INTERFACE_MODE_INTERNAL;
		dev_dbg(&pdev->dev, "interface mode: %d", eth->intf_mode);
		if ((eth->intf_mode != PHY_INTERFACE_MODE_INTERNAL) &&
			(eth->intf_mode != PHY_INTERFACE_MODE_SGMII) &&
			(eth->intf_mode != PHY_INTERFACE_MODE_RGMII) &&
			(eth->intf_mode != PHY_INTERFACE_MODE_RMII) &&
			(eth->intf_mode != PHY_INTERFACE_MODE_MII)) {
			dev_err(&pdev->dev, "Not supported phy interface mode: %s\n", phy_modes(eth->intf_mode));
			ret = -EINVAL;
			goto err;
		}

		/* EMAC link */
		if (!of_find_property(child, "fsl,pfeng-emac-id", NULL)) {
			dev_err(&pdev->dev, "The required EMAC id is missing\n");
			ret = -EINVAL;
			goto err;
		}
		if (of_property_read_u32(child, "fsl,pfeng-emac-id", &eth->emac_id) ||
			eth->emac_id > 2) {
			dev_err(&pdev->dev, "The EMAC id is invalid: %d\n", eth->emac_id);
			ret = -EINVAL;
			goto err;
		}
		dev_info(&pdev->dev, "%s linked to EMAC %d", eth->name, eth->emac_id);

		/* optional: tx,rx clocks */
		eth->tx_clk = devm_get_clk_from_child(&pdev->dev, child, "tx");
		if (IS_ERR(eth->tx_clk)) {
			eth->tx_clk = NULL;
			dev_dbg(&pdev->dev, "No TX clocks declared for interface %s\n", eth->name);
		} else {
			ret = clk_prepare_enable(eth->tx_clk);
			if (ret) {
				dev_err(&pdev->dev, "TX clocks for interface %s failed: %d\n", eth->name, ret);
				ret = 0;
				devm_clk_put(&pdev->dev, eth->tx_clk);
				eth->tx_clk = NULL;
			} else
				dev_info(&pdev->dev, "TX clocks for interface %s installed\n", eth->name);
		}
		eth->rx_clk = devm_get_clk_from_child(&pdev->dev, child, "rx");
		if (IS_ERR(eth->rx_clk)) {
			eth->rx_clk = NULL;
			dev_dbg(&pdev->dev, "No RX clocks declared for interface %s\n", eth->name);
		} else {
			ret = clk_prepare_enable(eth->rx_clk);
			if (ret) {
				dev_err(&pdev->dev, "RX clocks for interface %s failed: %d\n", eth->name, ret);
				ret = 0;
				devm_clk_put(&pdev->dev, eth->rx_clk);
				eth->rx_clk = NULL;
			} else
				dev_info(&pdev->dev, "RX clocks for interface %s installed\n", eth->name);
		}

		eth->dn = of_node_get(child);

		list_add_tail(&eth->lnode, &priv->plat.eth_list);
	} /* for_each_available_child_of_node(np, child) */

	dev_info(&pdev->dev, "HIF channels mask: 0x%04x", pfe_cfg->hif_chnls_mask);

	return 0;

err:
	if (child)
		of_node_put(child);
	release_config(priv);

	return ret;
}

/**
 * pfeng_s32g_probe
 *
 * @pdev: platform device pointer
 *
 * Description: This probing function gets called for all platform devices which
 * match the ID table and are not "owned" by other driver yet. This function
 * gets passed a "struct pplatform_device *" for each device whose entry in the ID table
 * matches the device. The probe functions returns zero when the driver choose
 * to take "ownership" of the device or an error code(-ve no) otherwise.
 */
static int pfeng_s32g_probe(struct platform_device *pdev)
{
	struct pfeng_priv *priv;
	int ret;

	if (!pdev->dev.of_node)
		return -ENODEV;

	if (!of_match_device(pfeng_id_table, &pdev->dev))
		return -ENODEV;

	dev_info(&pdev->dev, "%s, ethernet driver loading ...\n", PFENG_DRIVER_NAME);

	if (init_reserved_memory(&pdev->dev))
		return -ENOMEM;

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32)) != 0) {
		dev_err(&pdev->dev, "System does not support DMA, aborting\n");
		return -EINVAL;
	 }

	/* allocate driver context */
	priv = pfeng_drv_alloc(pdev);
	if(!priv)
		return -ENOMEM;

	/* overwrite by DT values */
	ret = create_config_from_dt(priv);
	if (ret)
		return ret;

	if(pfeng_s32g_set_emac_interfaces(priv,
		pfeng_drv_cfg_get_emac_intf_mode(priv, 0),
		pfeng_drv_cfg_get_emac_intf_mode(priv, 1),
		pfeng_drv_cfg_get_emac_intf_mode(priv, 2)))
		dev_err(&pdev->dev, "WARNING: cannot enable power for EMACs\n");

	ret = pfeng_drv_probe(priv);
	if(ret)
		goto err_mod_probe;

	return 0;

err_mod_probe:
	pfeng_drv_remove(priv);

	return ret;
}

/**
 * pfeng_s32g_remove
 *
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and releases the platform resources.
 */
static int pfeng_s32g_remove(struct platform_device *pdev)
{
	struct pfeng_priv *priv = dev_get_drvdata(&pdev->dev);

	if (!priv) {
		dev_err(&pdev->dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

	list_del(&priv->plat.eth_list);

	pfeng_drv_remove(priv);

	return 0;
}

/* pm support */

#ifdef CONFIG_PM_SLEEP
/**
 * pfeng_pm_suspend
 * @dev: device pointer
 * Description: this function is invoked when suspend the driver and it direcly
 * call the main suspend function and then, if required, on some platform, it
 * can call an exit helper.
 */
static int pfeng_pm_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	return 0;
}

/**
 * pfeng_pm_resume
 * @dev: device pointer
 * Description: this function is invoked when resume the driver before calling
 * the main resume function, on some platforms, it can call own init helper
 * if required.
 */
static int pfeng_pm_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

SIMPLE_DEV_PM_OPS(pfeng_s32g_pm_ops,
			pfeng_pm_suspend,
			pfeng_pm_resume);

/* platform data */

static struct platform_driver pfeng_platform_driver = {
	.probe = pfeng_s32g_probe,
	.remove = pfeng_s32g_remove,
	.driver = {
		.name = PFENG_DRIVER_NAME,
		.pm = &pfeng_s32g_pm_ops,
		.of_match_table = of_match_ptr(pfeng_id_table),
	},
};

module_platform_driver(pfeng_platform_driver);
