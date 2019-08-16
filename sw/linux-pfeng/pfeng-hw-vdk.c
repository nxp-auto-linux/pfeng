/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/of_mdio.h>
#include <linux/of_reserved_mem.h>

#include "pfeng.h"

static const struct of_device_id pfeng_id_table[] = {
	{ .compatible = "fsl,s32g275-pfe" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

/**
 * The device tree block:
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

/**
 * pfeng_vdk_probe
 *
 * @pdev: platform device pointer
 *
 * Description: This probing function gets called for all platform devices which
 * match the ID table and are not "owned" by other driver yet. This function
 * gets passed a "struct pplatform_device *" for each device whose entry in the ID table
 * matches the device. The probe functions returns zero when the driver choose
 * to take "ownership" of the device or an error code(-ve no) otherwise.
 */
static int pfeng_vdk_probe(struct platform_device *pdev)
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

	device = of_match_device(pfeng_id_table, &pdev->dev);
	if (!device)
		return -ENODEV;

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
 * pfeng_vdk_remove
 *
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and releases the platform resources.
 */
static int pfeng_vdk_remove(struct platform_device *pdev)
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

SIMPLE_DEV_PM_OPS(pfeng_vdk_pm_ops,
			pfeng_pm_suspend,
			pfeng_pm_resume);

/* platform data */

static struct platform_driver pfeng_platform_driver = {
	.probe = pfeng_vdk_probe,
	.remove = pfeng_vdk_remove,
	.driver = {
		.name = PFENG_DRIVER_NAME,
		.pm = &pfeng_vdk_pm_ops,
		.of_match_table = of_match_ptr(pfeng_id_table),
	},
};

module_platform_driver(pfeng_platform_driver);
