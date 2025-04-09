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
#include <linux/processor.h>
#include <linux/pinctrl/consumer.h>

#include "pfe_cfg.h"
#include "hal.h"
#include "pfeng.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Petrous <jan.petrous@nxp.com>");
MODULE_DESCRIPTION("PFEng SLAVE driver");
MODULE_VERSION(PFENG_DRIVER_VERSION);

#if LINUX_VERSION_CODE > KERNEL_VERSION(5,16,0)
#define do_exit(c) kthread_complete_and_exit(NULL, c)
#endif

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

#ifdef PFE_CFG_FCI_ENABLE
bool disable_netlink = false;
module_param(disable_netlink, bool, 0644);
MODULE_PARM_DESC(disable_netlink, "\t Do not create netlink socket for FCI communication (default: false)");
#endif /* PFE_CFG_FCI_ENABLE */

static int disable_master_detection = 0;
module_param(disable_master_detection, int, 0644);
MODULE_PARM_DESC(disable_master_detection, "\t 1 - disable Master detection, default is 0");

static int ipready_tmout = PFE_CFG_IP_READY_MS_TMOUT;
module_param(ipready_tmout, int, 0644);
MODULE_PARM_DESC(ipready_tmout, "\t 0 - nn, timeout for IP-ready, 0 means 'no timeout'");

/* Note: setting HIF port coherency should be done once for A53 domain!
 *       The recommended way is to use external solution, to not
 *       get conflict when two A53 Slave instances are trying to manage
 *       coherency register concurrently
 */
static int manage_port_coherency = 0;
module_param(manage_port_coherency, int, 0644);
MODULE_PARM_DESC(manage_port_coherency, "\t 1 - enable HIF port coherency management, default is 0");

static int hif_phc_emac = -1;
module_param(hif_phc_emac, int, 0644);
MODULE_PARM_DESC(hif_phc_emac, "\t (default EMAC0");

static int idex_resend_count = PFE_CFG_IDEX_RESEND_COUNT;
module_param(idex_resend_count, int, 0644);
MODULE_PARM_DESC(idex_resend_count, "\t IDEX transport retransmission count (default is " __stringify(PFE_CFG_IDEX_RESEND_COUNT) ")");

static int idex_resend_delays [2] = {
	PFE_CFG_IDEX_FIRST3_RESEND_TIME,
	PFE_CFG_IDEX_RESEND_TIME
};
static int idex_resend_delays_count;
module_param_array(idex_resend_delays, int, &idex_resend_delays_count, 0644);
MODULE_PARM_DESC(idex_resend_delays, "\t IDEX transport retransmission delays in ms (default is "
		__stringify(PFE_CFG_IDEX_FIRST3_RESEND_TIME) ","
		__stringify(PFE_CFG_IDEX_RESEND_TIME) ")");

uint32_t get_pfeng_pfe_cfg_master_if(void)
{
	/* Needed for compilation */
	return 0;
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

	/* IDEX transport retransmission setup */
	priv->idex_resend_count = idex_resend_count;
	priv->idex_resend_delays[0] = idex_resend_delays[0];
	priv->idex_resend_delays[1] = idex_resend_delays[1];

	priv->ihc_wq = create_singlethread_workqueue("pfeng-ihc-slave");
	if (!priv->ihc_wq) {
		HM_MSG_DEV_ERR(dev, "Initialize of IHC TX WQ failed\n");
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
		HM_MSG_DEV_ERR(dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

	if (priv->deferred_probe_task)
		kthread_stop(priv->deferred_probe_task);

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0) {
		HM_MSG_DEV_INFO(dev, "PM runtime resume returned: %d\n", ret);
	}

	/* Inform IDEX that driver is removing */
	pfe_idex_mark_down();

	if (priv->ihc_slave_wq)
		destroy_workqueue(priv->ihc_slave_wq);

	/* Remove debugfs directory */
	pfeng_debugfs_remove(priv);

	/* Remove netifs */
	pfeng_netif_remove(priv);

#ifdef PFE_CFG_FCI_ENABLE
	if (priv->pfe_platform) {
		pfe_platform_destroy_fci(priv->pfe_platform);
	}
#endif /* PFE_CFG_FCI_ENABLE */

	pfeng_hif_remove(priv);

	/* PFE platform remove */
	if (priv->pfe_platform) {
		if (pfe_platform_remove() != EOK)
			HM_MSG_DEV_ERR(dev, "PFE Platform not stopped successfully\n");
		else {
			priv->pfe_platform = NULL;
			HM_MSG_DEV_INFO(dev, "PFE Platform stopped\n");
		}
	}

	/* Clear HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node) && manage_port_coherency)
		pfeng_gpr_clear_port_coherency(priv);

	if (priv->ihc_wq)
		destroy_workqueue(priv->ihc_wq);
	if (kfifo_initialized(&priv->ihc_tx_fifo))
		kfifo_free(&priv->ihc_tx_fifo);

	pfeng_mdio_unregister(priv);

	pfeng_dt_release_config(priv);

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
	bool ip_ready = false;
	int ret = 0;

	/* Overwrite defaults by DT values */
	ret = pfeng_dt_create_config(priv);
	if (ret)
		goto err_drv;

	if (!priv->syscon.start && manage_port_coherency) {
		HM_MSG_DEV_ERR(dev, "Cannot find syscon resource, aborting\n");
		manage_port_coherency = 0;
		ret = -EINVAL;
		goto err_drv;
	}

	/* Detect controller state */
	if (priv->deferred_probe_task) {
		HM_MSG_DEV_INFO(dev, "Wait for PFE controller UP ...\n");

		while(1) {

			if(kthread_should_stop())
				goto err_drv;

			if (pfeng_gpr_ip_ready_get(dev, &ip_ready))
				HM_MSG_DEV_ERR(dev, "Failed to get IP ready state\n");

			if (ip_ready)
				break;

			if (ipready_tmout && !loops--) {
				/* Timed out */
				HM_MSG_DEV_ERR(dev, "PFE controller UP timed out\n");
				ret = -EAGAIN;
				goto err_drv;
			}

			usleep_range(100, 500);
		}

		HM_MSG_DEV_INFO(dev, "PFE controller UP detected\n");
	} else
		HM_MSG_DEV_INFO(dev, "PFE controller state detection skipped\n");

	/* PFE_SYS clock not used.
	 * We will use the constant PFE_CLK_SYS_RATE (600 MHz) as described
	 * in Programming consideraions in S32G RM.
	 */
	priv->clk_sys = NULL;

	/* HIF IHC channel number */
	if (master_ihc_chnl < (HIF_CFG_MAX_CHANNELS + 1))
		priv->ihc_master_chnl = master_ihc_chnl;
	if (priv->ihc_master_chnl >= (HIF_CFG_MAX_CHANNELS + 1)) {
		HM_MSG_DEV_ERR(dev, "Slave mode: Master channel id is missing\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Slave requires deferred worker */
	priv->ihc_slave_wq = alloc_ordered_workqueue("pfeng-slave-init", 0);
	if (!priv->ihc_slave_wq) {
		HM_MSG_DEV_ERR(dev, "Initialize of Slave WQ failed\n");
		ret = -ENOMEM;
		goto err_drv;
	}

	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node) && manage_port_coherency) {
		ret = pfeng_gpr_set_port_coherency(priv);
		if (ret)
			goto err_drv;
	}

	pm_runtime_get_noresume(dev);
	ret = pm_runtime_set_active(dev);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to set PM device status\n");
		goto err_drv;
	}

	pm_runtime_enable(dev);

	/* PFE platform layer init */
	ret = oal_mm_init(dev);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "OAL memory managment init failed\n");
		goto err_drv;
	}

	priv->pfe_cfg->lltx_res_tmu_q_id = PFENG_TMU_LLTX_DISABLE_MODE_Q_ID; /* disable LLTX for Slave */

	/* Start PFE Platform */
	ret = pfe_platform_init(priv->pfe_cfg);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Could not init PFE platform instance. Error %d\n", ret);
		goto err_drv;
	}

	priv->pfe_platform = pfe_platform_get_instance();
	if (!priv->pfe_platform) {
		HM_MSG_DEV_ERR(dev, "Could not get PFE platform instance\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Create debugfs */
	pfeng_debugfs_create(priv);

	/* Prepare PTP clock */
	priv->clk_ptp_reference = 0U;
	priv->clk_ptp = clk_get(dev, "pfe_ts");
	if (IS_ERR(priv->clk_ptp)) {
		HM_MSG_DEV_WARN(dev, "Failed to get pfe_ts clock. PTP will be disabled.\n");
		priv->clk_ptp = NULL;
	} else
		priv->clk_ptp_reference = clk_get_rate(priv->clk_ptp);

	/* PHC for hif2hif */
	if (hif_phc_emac < PFENG_PFE_EMACS)
		priv->hif_phc_emac_id = hif_phc_emac;

	/* Create HIFs */
	ret = pfeng_hif_create(priv);
	if (ret)
		goto err_drv;

	/* Create MDIO buses */
	pfeng_mdio_register(priv);

	/* Create net interfaces */
	ret = pfeng_netif_create(priv);
	if (ret) {
		pfeng_mdio_unregister(priv);
		goto err_drv;
	}

	dev_pm_set_driver_flags(dev, DPM_FLAG_NO_DIRECT_COMPLETE);

	pm_runtime_put_noidle(dev);

err_drv:

	if (ret) {
		/* Stop IHC RX/TX works */
		cancel_work_sync(&priv->ihc_rx_work);
		cancel_work_sync(&priv->ihc_tx_work);
		flush_workqueue(priv->ihc_wq);
	}

	if (priv->deferred_probe_task) {
		priv->deferred_probe_task = NULL;
		do_exit(0);
	}

	return ret;
}

/**
 * pfeng_drv_soc_is_g3
 *
 * @dev: device pointer
 *
 * Description: This probing function tries to detect S32G3 SoC.
 * In case no S32G3 nor S32G2 is detected, the default is S32G2.
 * The detection depends on valid DT, it checks compatibility
 * string for head node.
 *
 */
static bool pfeng_drv_soc_is_g3(struct device *dev)
{
	struct device_node *node = of_find_node_by_path("/");

	if (of_device_is_compatible(node, "nxp,s32g3"))
		return true;

	if (!of_device_is_compatible(node, "nxp,s32g2"))
		dev_warn(dev, "Silicon detection failed. Defaulting to S32G2\n");

	return false;
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

	ret = pfeng_gpr_check_nvmem_cells(dev);
	if (ret) {
		if (ret != EPROBE_DEFER)
			HM_MSG_DEV_ERR(dev, "NVMEM cells check failed\n");
		return ret;
	}

	HM_MSG_DEV_INFO(dev, "PFEng ethernet driver loading ...\n");
	HM_MSG_DEV_INFO(dev, "Version: %s\n", PFENG_DRIVER_VERSION);
	HM_MSG_DEV_INFO(dev, "Driver commit hash: %s\n", PFENG_DRIVER_COMMIT_HASH);

	/* Print MULTI-INSATNCE mode (MASTER/SLAVE/disabled) */
	HM_MSG_DEV_INFO(dev, "Multi instance support: SLAVE/mdetect=%s\n", disable_master_detection ? "off" : "on");

	HM_MSG_DEV_INFO(dev, "Compiled by: %s\n", __VERSION__);

	if (!of_dma_is_coherent(dev->of_node))
		HM_MSG_DEV_ERR(dev, "DMA coherency disabled - consider impact on device performance\n");

	/* Signal driver coherency mask */
	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32)) != 0) {
		HM_MSG_DEV_ERR(dev, "System does not support DMA, aborting\n");
		return -EINVAL;
	}

	/* Allocate driver context with defaults */
	priv = pfeng_drv_alloc(pdev);
	if(!priv) {
		HM_MSG_DEV_ERR(dev, "Driver context allocation failed\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, priv);

	/* Detect S32G3 */
	priv->on_g3 = pfeng_drv_soc_is_g3(dev);
	priv->pfe_cfg->on_g3 = pfeng_drv_soc_is_g3(dev);

	if (!disable_master_detection) {
		priv->deferred_probe_task = kthread_create(pfeng_drv_deferred_probe, priv, "pfe-probe-task");
		if (IS_ERR(priv->deferred_probe_task)) {
			ret = PTR_ERR(priv->deferred_probe_task);
			priv->deferred_probe_task = NULL;
			HM_MSG_DEV_ERR(dev, "Master detection task failed to start: %d\n", ret);
			goto err_drv;
		}
		wake_up_process(priv->deferred_probe_task);
	} else
		return pfeng_drv_deferred_probe(priv);

err_drv:
	if (ret) {
		/* Stop IHC RX/TX works */
		cancel_work_sync(&priv->ihc_rx_work);
		cancel_work_sync(&priv->ihc_tx_work);
		flush_workqueue(priv->ihc_wq);
	}

	return ret;
}

static int pfeng_drv_pm_suspend(struct device *dev)
{
	struct pfeng_priv *priv = dev_get_drvdata(dev);
	int ret;

	HM_MSG_DEV_INFO(dev, "Suspending driver\n");

	if (priv->deferred_probe_task)
		kthread_stop(priv->deferred_probe_task);

	priv->in_suspend = true;

	/* Inform IDEX that driver is suspending */
	pfe_idex_mark_down();

	pfeng_debugfs_remove(priv);

	/* NETIFs */
	ret = pfeng_netif_suspend(priv);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "PFE netif not suspended successfully\n");
		goto err_drv;
	}

#ifdef PFE_CFG_FCI_ENABLE
	/* FCI */
	if (priv->pfe_platform) {
		pfe_platform_destroy_fci(priv->pfe_platform);
	}
#endif /* PFE_CFG_FCI_ENABLE */

	/* MDIO buses */
	ret = pfeng_mdio_suspend(priv);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "PFE mdio not suspended successfully\n");
		goto err_drv;
	}

	/* HIFs stop */
	pfeng_hif_remove(priv);

	/* PFE platform remove */
	if (priv->pfe_platform) {
		ret = pfe_platform_remove();
		if (ret != EOK) {
			HM_MSG_DEV_ERR(dev, "PFE Platform not stopped successfully\n");
			goto err_drv;
		} else {
			priv->pfe_platform = NULL;
			HM_MSG_DEV_INFO(dev, "PFE Platform stopped\n");
		}
	}

	pinctrl_pm_select_sleep_state(dev);

	/* Invalidate clock reference */
	if (priv->clk_ptp)
		priv->clk_ptp = NULL;

	/* Clear HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node) && manage_port_coherency)
		pfeng_gpr_clear_port_coherency(priv);

	HM_MSG_DEV_INFO(dev, "PFE Platform suspended\n");

	return 0;

err_drv:
	return ret;
}

static int pfeng_drv_deferred_resume(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct pfeng_priv *priv = dev_get_drvdata(dev);
	int loops = ipready_tmout * 10; /* sleep is 100 usec */
	bool ip_ready = false;
	int ret = 0;

	ret = pinctrl_pm_select_default_state(dev);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to select default pinctrl state\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Reinit memory */
	ret = oal_mm_wakeup_reinit();
	if (ret) {
		ret = 0;
		HM_MSG_DEV_WARN(dev, "Failed to re-init PFE memory\n");
	}

	/* Detect controller state */
	if (priv->deferred_probe_task) {
		HM_MSG_DEV_INFO(dev, "Wait for PFE controller UP ...\n");

		while(1) {

			if(kthread_should_stop())
				goto err_drv;

			if (pfeng_gpr_ip_ready_get(dev, &ip_ready))
				HM_MSG_DEV_ERR(dev, "Failed to get IP ready state\n");

			if (ip_ready)
				break;

			if (ipready_tmout && !loops--) {
				/* Timed out */
				HM_MSG_DEV_ERR(dev, "PFE controller UP timed out\n");
				ret = -EAGAIN;
				goto err_drv;
			}

			usleep_range(100, 500);
		}

		HM_MSG_DEV_INFO(dev, "PFE controller UP detected\n");
	} else
		HM_MSG_DEV_INFO(dev, "PFE controller state detection skipped\n");

	/* Start PFE Platform */
	ret = pfe_platform_init(priv->pfe_cfg);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Could not init PFE platform instance. Error %d\n", ret);
		goto err_pfe_init;
	}
	priv->pfe_platform = pfe_platform_get_instance();
	if (!priv->pfe_platform) {
		HM_MSG_DEV_ERR(dev, "Could not get PFE platform instance\n");
		ret = -EINVAL;
		goto err_pfe_get;
	}

	/* Create debugfs */
	pfeng_debugfs_create(priv);

	/* Create HIFs */
	ret = pfeng_hif_create(priv);
	if (ret)
		goto err_drv;

	/* MDIO buses */
	ret = pfeng_mdio_resume(priv);
	if (ret)
		goto err_drv;

	/* Create net interfaces */
	ret = pfeng_netif_resume(priv);
	if (ret)
		goto err_drv;

	priv->in_suspend = false;

err_drv:
err_pfe_get:
err_pfe_init:

	if (ret) {
		/* Stop IHC RX/TX works */
		cancel_work_sync(&priv->ihc_rx_work);
		cancel_work_sync(&priv->ihc_tx_work);
		flush_workqueue(priv->ihc_wq);
	}

	if (priv->deferred_probe_task) {
		priv->deferred_probe_task = NULL;
		do_exit(0);
	}

	return ret;
}

static int pfeng_drv_pm_resume(struct device *dev)
{
	struct pfeng_priv *priv = dev_get_drvdata(dev);
	int ret = 0;

	HM_MSG_DEV_INFO(dev, "Resuming driver\n");

	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node) && manage_port_coherency)
		pfeng_gpr_set_port_coherency(priv);

	/* Get PFE_TS rate */
	priv->clk_ptp_reference = 0U;
	priv->clk_ptp = clk_get(dev, "pfe_ts");
	if (IS_ERR(priv->clk_ptp)) {
		HM_MSG_DEV_WARN(dev, "Failed to get pfe_ts clock. PTP will be disabled.\n");
		priv->clk_ptp = NULL;
	} else
		priv->clk_ptp_reference = clk_get_rate(priv->clk_ptp);

	if (!disable_master_detection) {
		priv->deferred_probe_task = kthread_create(pfeng_drv_deferred_resume, dev, "pfe-resume-task");
		if (IS_ERR(priv->deferred_probe_task)) {
			ret = PTR_ERR(priv->deferred_probe_task);
			priv->deferred_probe_task = NULL;
			HM_MSG_DEV_ERR(dev, "Master detection task failed to start: %d\n", ret);
			goto err_deferr;
		}
		wake_up_process(priv->deferred_probe_task);
	} else
		return pfeng_drv_deferred_resume(dev);

err_deferr:
	if (ret) {
		/* Stop IHC RX/TX works */
		cancel_work_sync(&priv->ihc_rx_work);
		cancel_work_sync(&priv->ihc_tx_work);
		flush_workqueue(priv->ihc_wq);
	}

	return ret;
}

SIMPLE_DEV_PM_OPS(pfeng_drv_pm_ops,
			pfeng_drv_pm_suspend,
			pfeng_drv_pm_resume);


/**
 * pfeng_drv_shutdown
 *
 * @pdev: platform device pointer
 * Description: this function calls at shut-down time to quiesce the device
 */
static void pfeng_drv_shutdown(struct platform_device *pdev)
{
	pfeng_drv_remove(pdev);
}

/* platform data */

static struct platform_driver pfeng_platform_driver = {
	.probe = pfeng_drv_probe,
	.remove = pfeng_drv_remove,
	.driver = {
		.name = PFENG_DRIVER_NAME,
		.pm = &pfeng_drv_pm_ops,
		.of_match_table = of_match_ptr(pfeng_id_table),
	},
	.shutdown = pfeng_drv_shutdown,
};

module_platform_driver(pfeng_platform_driver);
