/*
 * Copyright 2021-2022 NXP
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
#include "hal.h"
#include "pfeng.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Petrous <jan.petrous@nxp.com>");
MODULE_DESCRIPTION("PFEng SLAVE driver");
MODULE_VERSION(PFENG_DRIVER_VERSION);

static const struct of_device_id pfeng_id_table[] = {
	{ .compatible = "nxp,s32g-pfe-slave" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

static const u32 default_msg_level = (
	NETIF_MSG_DRV | NETIF_MSG_PROBE |
	NETIF_MSG_LINK | NETIF_MSG_IFUP |
	NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

int msg_verbosity = PFE_CFG_VERBOSITY_LEVEL;
module_param(msg_verbosity, int, 0644);
MODULE_PARM_DESC(msg_verbosity, "\t 0 - 9, default 4");

static int master_ihc_chnl = HIF_CFG_MAX_CHANNELS + 1;
module_param(master_ihc_chnl, int, 0644);
MODULE_PARM_DESC(master_ihc_chnl, "\t 0 - <max-hif-chn-number>, default read from DT or invalid");

static int disable_master_detection = 0;
module_param(disable_master_detection, int, 0644);
MODULE_PARM_DESC(disable_master_detection, "\t 1 - disable Master detection, default is 0");

static int ipready_tmout = PFE_CFG_IP_READY_MS_TMOUT;
module_param(ipready_tmout, int, 0644);
MODULE_PARM_DESC(ipready_tmout, "\t 0 - nn, timeout for IP-ready, 0 means 'no timeout'");

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

	priv->ihc_wq = create_singlethread_workqueue("pfeng-ihc-slave");
	if (!priv->ihc_wq) {
		dev_err(dev, "Initialize of IHC TX WQ failed\n");
		goto err_cfg_alloc;
	}
	if (kfifo_alloc(&priv->ihc_tx_fifo, 32, GFP_KERNEL))
		goto err_cfg_alloc;
	INIT_WORK(&priv->ihc_tx_work, pfeng_ihc_tx_work_handler);
	INIT_WORK(&priv->ihc_rx_work, pfeng_ihc_rx_work_handler);

	return priv;

err_cfg_alloc:
	devm_kfree(dev, priv);
	return NULL;
}

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
	int ret;

	if (!priv) {
		dev_err(dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

	if (priv->deferred_probe_task)
		kthread_stop(priv->deferred_probe_task);

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0) {
		dev_info(dev, "PM runtime resume returned: %d\n", ret);
	}

	if (priv->ihc_slave_wq)
		destroy_workqueue(priv->ihc_slave_wq);

	/* Remove debugfs directory */
	pfeng_debugfs_remove(priv);

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

	if (priv->ihc_wq)
		destroy_workqueue(priv->ihc_wq);
	if (kfifo_initialized(&priv->ihc_tx_fifo))
		kfifo_free(&priv->ihc_tx_fifo);

	pfeng_dt_release_config(priv);

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

	pm_runtime_put_noidle(dev);
	pm_runtime_disable(dev);

	return 0;
}


static int pfeng_drv_deferred_probe(void *arg)
{
	struct pfeng_priv *priv = (struct pfeng_priv *)arg;
	struct device *dev = &priv->pdev->dev;
	int loops = ipready_tmout * 10; /* sleep is 100 usec */
	int ret;

	/* Detect controller state */
	if (priv->deferred_probe_task) {
		dev_info(dev, "Wait for PFE controller UP ...\n");

		while(1) {

			if(kthread_should_stop())
				do_exit(0);

			if (hal_ip_ready_get())
				break;

			if (ipready_tmout && !loops--) {
				/* Timed out */
				dev_err(dev, "PFE controller UP timed out\n");
				priv->deferred_probe_task = NULL;
				do_exit(0);
			}

			usleep_range(100, 500);
		}

		dev_info(dev, "PFE controller UP detected\n");
	} else
		dev_info(dev, "PFE controller state detection skipped\n");

	/* Overwrite defaults by DT values */
	ret = pfeng_dt_create_config(priv);
	if (ret)
		goto err_drv;

	/* HIF IHC channel number */
	if (master_ihc_chnl < (HIF_CFG_MAX_CHANNELS + 1))
		priv->ihc_master_chnl = master_ihc_chnl;
	if (priv->ihc_master_chnl >= (HIF_CFG_MAX_CHANNELS + 1)) {
		dev_err(dev, "Slave mode: Master channel id is missing\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Slave requires deferred worker */
	priv->ihc_slave_wq = create_singlethread_workqueue("pfeng-slave-init");
	if (!priv->ihc_slave_wq) {
		dev_err(dev, "Initialize of Slave WQ failed\n");
		goto err_drv;
	}

	pm_runtime_get_noresume(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	/* PFE platform layer init */
	oal_mm_init(dev);

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

	/* Create HIFs */
	ret = pfeng_hif_create(priv);
	if (ret)
		goto err_drv;

	/* Create net interfaces */
	ret = pfeng_netif_create(priv);
	if (ret)
		goto err_drv;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
	dev_pm_set_driver_flags(dev, DPM_FLAG_NO_DIRECT_COMPLETE);
#else
	dev_pm_set_driver_flags(dev, DPM_FLAG_NEVER_SKIP);
#endif

	pm_runtime_put_noidle(dev);

	if (priv->deferred_probe_task) {
		priv->deferred_probe_task = NULL;
		do_exit(0);
	}

	return 0;

err_drv:
	if (priv->deferred_probe_task) {
		priv->deferred_probe_task = NULL;
		do_exit(0);
	}

	return ret;
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
	int ret = 0;

	if (!pdev->dev.of_node)
		return -ENODEV;

	if (!of_match_device(pfeng_id_table, &pdev->dev))
		return -ENODEV;

	dev_info(dev, "PFEng ethernet driver loading ...\n");
	dev_info(dev, "Version: %s\n", PFENG_DRIVER_VERSION);

	/* Print MULTI-INSATNCE mode (MASTER/SLAVE/disabled) */
	dev_info(dev, "Multi instance support: SLAVE/mdetect=%s\n", disable_master_detection ? "off" : "on");

	dev_info(dev, "Compiled by: %s\n", __VERSION__);

	if (!of_dma_is_coherent(dev->of_node))
		dev_err(dev, "DMA coherency disabled - consider impact on device performance\n");

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

	if (!disable_master_detection) {
		priv->deferred_probe_task = kthread_run(pfeng_drv_deferred_probe, priv, "pfe-probe-task");
		if (IS_ERR(priv->deferred_probe_task)) {
			ret = PTR_ERR(priv->deferred_probe_task);
			priv->deferred_probe_task = NULL;
			dev_err(dev, "Master detection task failed to start: %d\n", ret);
			goto err_drv;
		}
	} else
		ret = pfeng_drv_deferred_probe(priv);

err_drv:
	return ret;
}

/* Slave PM is not supported */
static int pfeng_drv_pm_suspend(struct device *dev)
{
	dev_err(dev, "Suspending driver is unsupported\n");

	return -ENOTSUP;
}

static int pfeng_drv_pm_resume(struct device *dev)
{
	return 0;
}

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
