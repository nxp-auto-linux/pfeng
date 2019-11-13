/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/phy.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>

/*
 * S32G soc specific addresses
 */
#define S32G_MAIN_GPR_BASE_ADDR				0x4007CA00
#define S32G_MAIN_GPR_PFE_COH_EN			0x0
#define GPR_PFE_COH_EN_UTIL					(1 << 5)
#define GPR_PFE_COH_EN_HIF3					(1 << 4)
#define GPR_PFE_COH_EN_HIF2					(1 << 3)
#define GPR_PFE_COH_EN_HIF1					(1 << 2)
#define GPR_PFE_COH_EN_HIF0					(1 << 1)
#define GPR_PFE_COH_EN_DDR					(1 << 0)
#define S32G_MAIN_GPR_PFE_EMACX_INTF_SEL	0x4
#define GPR_PFE_EMAC_IF_MII					(1)
#define GPR_PFE_EMAC_IF_RMII				(9)
#define GPR_PFE_EMAC_IF_RGMII				(2)
#define GPR_PFE_EMAC_IF_SGMII				(0)
#define GPR_PFE_EMACn_IF(n,i)				(i << (n * 4))
#define S32G_MAIN_GPR_PFE_SYS_GEN0			0x8
#define S32G_MAIN_GPR_PFE_SYS_GEN1			0xC
#define S32G_MAIN_GPR_PFE_SYS_GEN2			0x10
#define S32G_MAIN_GPR_PFE_SYS_GEN3			0x14
#define S32G_MAIN_GPR_PFE_PWR_CTRL			0x20
#define GPR_PFE_EMACn_PWR_ACK(n)			(1 << (9 + n)) /* RD Only */
#define GPR_PFE_EMACn_PWR_ISO(n)			(1 << (6 + n))
#define GPR_PFE_EMACn_PWR_DWN(n)			(1 << (3 + n))
#define GPR_PFE_EMACn_PWR_CLAMP(n)			(1 << (0 + n))

#include "pfeng.h"

static const struct of_device_id pfeng_id_table[] = {
	{ .compatible = "fsl,s32g275-pfe" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

/**
 * The device tree blocks:
 *
 *  reserved-memory {
 *      #address-cells = <2>;
 *      #size-cells = <2>;
 *      ranges;
 *
 *      pfe_reserved: pfebufs@83400000 {
 *              compatible = "fsl,s32g-pfe-ddr";
 *              reg = <0 0x83400000 0 0xc00000>;
 *              no-map;
 *              status = "okay";
 *      };
 *  };
 *
 *  pfe: ethernet@46080000 {
 *      compatible = "fsl,s32g275-pfe";
 *      reg = <0x0 0x46000000 0x0 0x1000000>;
 *      interrupt-parent = <&gic>;
 *      interrupts =    <0 190 1>, / * hif0 * /
 *              <0 191 1>, / * hif1 * /
 *              <0 192 1>, / * hif2 * /
 *              <0 193 1>, / * hif3 * /
 *              <0 194 1>; / * bmu * /
 *      interrupt-names = "hif0", "hif1", "hif2", "hif3", "bmu";
 *  };
 *
 */

static int pfeng_create_reserved_memory(struct device *dev)
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
	dev_info(dev, "Found reserved memory at p0x%llx size 0x%llx\n", res.start, res.end - res.start);

	ret = dma_declare_coherent_memory(dev, res.start, res.start,
                           res.end - res.start, DMA_MEMORY_EXCLUSIVE);

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

/**
 * pfeng_s32g_set_emac_interfaces
 *
 * @pdev: platform device pointer
 *
 * Description: Set up the correct PHY interface type for all EMAcs
 */
static int pfeng_s32g_set_emac_interfaces(struct device *dev, phy_interface_t emac0_intf, phy_interface_t emac1_intf, phy_interface_t emac2_intf)
{
	void *syscon;
	unsigned int val;

	syscon = ioremap_nocache(S32G_MAIN_GPR_BASE_ADDR, S32G_MAIN_GPR_PFE_PWR_CTRL + 4);
	if(!syscon) {
		dev_err(dev, "cannot map GPR, aborting (INTF_SEL)\n");
		return -EIO;
	}

	/* set up interfaces */
	val = xlate_to_s32g_intf(0, emac0_intf) | xlate_to_s32g_intf(1, emac1_intf) | xlate_to_s32g_intf(2, emac2_intf);
	hal_write32(val, syscon + S32G_MAIN_GPR_PFE_EMACX_INTF_SEL);

	/* power down and up EMACs */
	hal_write32(GPR_PFE_EMACn_PWR_DWN(0) | GPR_PFE_EMACn_PWR_DWN(1) | GPR_PFE_EMACn_PWR_DWN(2), syscon + S32G_MAIN_GPR_PFE_PWR_CTRL);
	usleep_range(100, 500);
	hal_write32(0, syscon + S32G_MAIN_GPR_PFE_PWR_CTRL);

	iounmap(syscon);

	return 0;
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
	struct pfeng_plat_data *plat;
	struct pfeng_resources res;
	int ret;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *device;
	struct resource *plat_res;
	int i, irq;

	if (!np)
		return -ENODEV;

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32)) != 0) {
		dev_err(&pdev->dev, "System does not support DMA, aborting\n");
		return -EINVAL;
	 }

	device = of_match_device(pfeng_id_table, &pdev->dev);
	if (!device)
		return -ENODEV;

	if (pfeng_create_reserved_memory(&pdev->dev))
		return -ENOMEM;

	priv = pfeng_mod_init(&pdev->dev);
	if (!priv)
		return -ENOMEM;

	plat = devm_kzalloc(&pdev->dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		return -ENOMEM;

	pfeng_mod_get_setup(&pdev->dev, plat);

	/* Get the base address of device */
	memset(&res, 0, sizeof(res));

	plat_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(unlikely(!plat_res)) {
		printk(KERN_ALERT "%s: cannot find mem resource, aborting\n", PFENG_DRIVER_NAME);
		ret = -EIO;
		goto err_out_map_failed;
	}
	res.addr = plat_res->start;
	res.addr_size = plat_res->end - plat_res->start + 1;
	printk(KERN_ALERT "%s: res.addr 0x%llx size 0x%x\n", PFENG_DRIVER_NAME, res.addr, res.addr_size);

	/* IRQ */
	for (i = 0; i < ARRAY_SIZE(res.irq.hif); i++) {
		char *irqnames[] = { "hif0", "hif1", "hif2", "hif3" , "hifncpy"};
		irq = platform_get_irq_byname(pdev, irqnames[i]);
		if (irq < 0) {
			printk(KERN_ALERT "%s: cannot find irq resource '%s', aborting\n", PFENG_DRIVER_NAME, irqnames[i]);
			ret = -EIO;
			goto err_out_map_failed;
		}
		res.irq.hif[i] = irq;
		printk(KERN_ALERT "%s: irq '%s': %u\n", PFENG_DRIVER_NAME, irqnames[i], irq);
	}

	irq = platform_get_irq_byname(pdev, "bmu");
	if (irq < 0) {
		printk(KERN_ALERT "%s: cannot find irq resource 'bmu', aborting\n", PFENG_DRIVER_NAME);
		ret = -EIO;
		goto err_out_map_failed;
	}
	res.irq.bmu = irq;
	printk(KERN_ALERT "%s: irq 'bmu' : %u\n", PFENG_DRIVER_NAME, irq);

	if(pfeng_s32g_set_emac_interfaces(&pdev->dev, PHY_INTERFACE_MODE_SGMII, PHY_INTERFACE_MODE_RGMII, PHY_INTERFACE_MODE_RGMII)) {
		printk(KERN_ALERT "%s: cannot enable power for EMACs, aborting\n", PFENG_DRIVER_NAME);
		ret = -EIO;
		goto err_out_map_failed;
	}

	ret = pfeng_mod_probe(&pdev->dev, priv, plat, &res);
	if(ret)
		goto err_mod_probe;

end:
	return ret;

err_mod_probe:
err_out_map_failed:
	pfeng_mod_exit(&pdev->dev);

	goto end;
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

	pfeng_mod_exit(&pdev->dev);

	platform_set_drvdata(pdev, NULL);

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
	/* TODO */

	printk(KERN_ALERT "%s\n", __func__);

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
	/* TODO */

	printk(KERN_ALERT "%s\n", __func__);

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
