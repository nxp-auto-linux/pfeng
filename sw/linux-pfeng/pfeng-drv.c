/*
 * Copyright 2019-2025 NXP
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
#include "pfe_feature_mgr.h"

/* Major IP version for cut2.0 */
#define PFE_IP_MAJOR_VERSION_CUT2		2

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Petrous <jan.petrous@nxp.com>");
MODULE_DESCRIPTION("PFEng driver");
MODULE_FIRMWARE(PFENG_FW_CLASS_NAME);
MODULE_FIRMWARE(PFENG_FW_UTIL_NAME);
MODULE_VERSION(PFENG_DRIVER_VERSION);

static const struct of_device_id pfeng_id_table[] = {
	{ .compatible = "nxp,s32g-pfe" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pfeng_id_table);

static const u32 default_msg_level = (
	NETIF_MSG_DRV | NETIF_MSG_PROBE |
	NETIF_MSG_LINK | NETIF_MSG_IFUP |
	NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

static u32 pfeng_pfe_cfg_master_if = (u32)PFE_PHY_IF_ID_INVALID;

int msg_verbosity = PFE_CFG_VERBOSITY_LEVEL;
module_param(msg_verbosity, int, 0644);
MODULE_PARM_DESC(msg_verbosity, "\t 0 - 9, default 4");

static char *fw_class_name;
module_param(fw_class_name, charp, 0444);
MODULE_PARM_DESC(fw_class_name, "\t The name of CLASS firmware file (default: read from device-tree or " PFENG_FW_CLASS_NAME ")");

static char *fw_util_name;
module_param(fw_util_name, charp, 0444);
MODULE_PARM_DESC(fw_util_name, "\t The name of UTIL firmware file (default: read from device-tree or " PFENG_FW_UTIL_NAME "). Use \"NONE\" to run without UTIL firmware.");

static int l2br_vlan_id = 1;
module_param(l2br_vlan_id, int, 0644);
MODULE_PARM_DESC(l2br_vlan_id, "\t Default L2Bridge VLAN ID (default read from DT or 1");

static int l2br_vlan_stats_size = 20;
module_param(l2br_vlan_stats_size, int, 0644);
MODULE_PARM_DESC(l2br_vlan_stats_size, "\t Default vlan stats size vector (default read from DT or 20");

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
static int fci_ownership_mask = 0;
module_param(fci_ownership_mask, int, 0644);
MODULE_PARM_DESC(fci_ownership_mask, "\t Overrides bitmask of HIF channels that are allowed to take FCI ownership (default 0: all senders allowed)");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_FCI_ENABLE
bool disable_netlink = false;
module_param(disable_netlink, bool, 0644);
MODULE_PARM_DESC(disable_netlink, "\t Do not create netlink socket for FCI communication (default: false)");
#endif /* PFE_CFG_FCI_ENABLE */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
static int disable_master_detection = 0;
module_param(disable_master_detection, int, 0644);
MODULE_PARM_DESC(disable_master_detection, "\t 1 - disable Master detection signalization (default is 0)");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

static bool g2_ordered_class_writes = false;
module_param(g2_ordered_class_writes, bool, 0644);
MODULE_PARM_DESC(g2_ordered_class_writes, "\t Enable ordered class writes on S32G2 (default: false)");

static bool g3_rtable_in_lmem = true;
module_param(g3_rtable_in_lmem , bool, 0644);
MODULE_PARM_DESC(g3_rtable_in_lmem , "\t Allocate PFE's Routing Table in local memory on S32G3 (default: true)");

static int lltx_res_tmu_q_id = 255;
module_param(lltx_res_tmu_q_id, int, 0644);
MODULE_PARM_DESC(lltx_res_tmu_q_id, "\t Reserved TMU queue ID for Host lossless Tx (LLTX), range: 0-7; use 255 to disable LLTX (default: 255)");

static int hif_phc_emac = -1;
module_param(hif_phc_emac, int, 0644);
MODULE_PARM_DESC(hif_phc_emac, "\t (default EMAC0");

uint32_t get_pfeng_pfe_cfg_master_if(void)
{
	return pfeng_pfe_cfg_master_if;
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
	priv->ihc_wq = alloc_ordered_workqueue("pfeng-ihc", 0);
	if (!priv->ihc_wq) {
		HM_MSG_DEV_ERR(dev, "Initialize of IHC TX failed\n");
		goto err_cfg_alloc;
	}
	if (kfifo_alloc(&priv->ihc_tx_fifo, 32, GFP_KERNEL))
		goto err_cfg_alloc;
	INIT_WORK(&priv->ihc_tx_work, pfeng_ihc_tx_work_handler);
	INIT_WORK(&priv->ihc_rx_work, pfeng_ihc_rx_work_handler);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	return priv;

err_cfg_alloc:
	devm_kfree(dev, priv);
	return NULL;
}

static int pfeng_pfe_off(struct pfeng_priv *priv)
{
	int ret;
	struct device *dev = &priv->pdev->dev;

	if (!priv->rst) {
		HM_MSG_DEV_ERR(dev, "Partition reset support disabled\n");
		return -ENOTSUP;
	}

	ret = reset_control_assert(priv->rst);
	if (ret)
		HM_MSG_DEV_ERR(dev, "Failed to assert PFE reset: %d\n", ret);

	return ret;
}

static int pfeng_pfe_reset(struct pfeng_priv *priv)
{
	int ret;
	struct device *dev = &priv->pdev->dev;

	ret = pfeng_pfe_off(priv);
	if (ret)
		return ret;

	udelay(100);

	ret = reset_control_deassert(priv->rst);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to deassert PFE reset: %d\n", ret);
		return ret;
	}
	HM_MSG_DEV_INFO(dev, "PFE controller reset done\n");

	return 0;
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

	ret = pm_runtime_resume_and_get(dev);
	if (ret < 0) {
		HM_MSG_DEV_INFO(dev, "PM runtime resume returned: %d\n", ret);
	}

	if (!priv) {
		HM_MSG_DEV_ERR(dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	ret = pfeng_gpr_ip_ready_set(dev, false);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to signal IP not ready: %d\n", ret);
		return ret;
	}

#ifdef PFE_CFG_FCI_ENABLE
	if (priv->pfe_platform) {
		pfe_platform_destroy_fci(priv->pfe_platform);
	}
#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	/* Remove debugfs directory */
	pfeng_debugfs_remove(priv);

	/* Remove netifs */
	pfeng_netif_remove(priv);

	pfeng_hif_remove(priv);

	/* PFE platform remove */
	if (priv->pfe_platform) {
		if (pfe_platform_remove() != EOK)
			HM_MSG_DEV_ERR(dev, "PFE Platform not stopped successfully\n");
		else {
			priv->pfe_platform = NULL;
			HM_MSG_DEV_INFO(dev, "PFE Platform stopped\n");
		}

		/* PFE Partition shutdown */
		if (priv->rst)
			if (!pfeng_pfe_off(priv))
				dev_info(dev, "PFE controller off\n");
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (priv->ihc_wq)
		destroy_workqueue(priv->ihc_wq);
	if (kfifo_initialized(&priv->ihc_tx_fifo))
		kfifo_free(&priv->ihc_tx_fifo);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	pfeng_mdio_unregister(priv);

	/* Release firmware */
	if (priv->pfe_cfg->fw)
		pfeng_fw_free(priv);

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

static int pfeng_validate_emac_speeds(struct pfeng_priv *priv)
{
	int id;

	for (id = 0; id < PFENG_PFE_EMACS; id++) {
		struct pfeng_emac *emac = &priv->emac[id];

		if (!emac->enabled)
			continue;

		/* retrieve max rate */
		switch (emac->max_speed) {
		case 0:
			/* It signalises no explicit max_speed */
			if (!id || priv->on_g3)
				emac->max_speed = SPEED_2500;
			else
				emac->max_speed = SPEED_1000;
			break;
		case SPEED_10:
		case SPEED_100:
		case SPEED_1000:
			break;
		case SPEED_2500:
			/* On G2 only EMAC0 can use 2500 */
			if (id && !priv->on_g3) {
				HM_MSG_DEV_WARN(&priv->pdev->dev, "Unsupported max speed on EMAC%d: %d\n", id, emac->max_speed);
				return -EINVAL;
			}
			break;
		default:
			HM_MSG_DEV_WARN(&priv->pdev->dev, "Invalid max speed on EMAC%d: %d\n", id, emac->max_speed);
			return -EINVAL;
		}
	}

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
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	HM_MSG_DEV_INFO(dev, "Multi instance support: Master/mdetect=%s\n", disable_master_detection ? "off" : "on");
#else
	HM_MSG_DEV_INFO(dev, "Multi instance support: disabled (standalone)\n");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	HM_MSG_DEV_INFO(dev, "Compiled by: %s\n", __VERSION__);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	ret = pfeng_gpr_ip_ready_set(dev, false);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to signal IP not ready: %d\n", ret);
		return ret;
	}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	if (!of_dma_is_coherent(dev->of_node))
		HM_MSG_DEV_WARN(dev, "DMA coherency disabled - consider impact on device performance\n");

	/* Attach to reset controller to reset S32G2 partition 2 */
	rst = devm_reset_control_get(dev, "pfe_part");
	if (IS_ERR(rst)) {
		HM_MSG_DEV_WARN(dev, "Warning: Partition reset 'pfe_part' get failed: code %ld\n", PTR_ERR(rst));
		rst = NULL;
	}

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

	/* Overwrite defaults by DT values */
	ret = pfeng_dt_create_config(priv);
	if (ret)
		goto err_drv;

	/* L2bridge default VLAN ID */
	if (!l2br_vlan_id || l2br_vlan_id > 4095) {
		HM_MSG_DEV_ERR(dev, "Invalid L2Bridge default VLAN ID, used 1\n");
		l2br_vlan_id = 1;
	}
	if (l2br_vlan_id != 1)
		priv->pfe_cfg->vlan_id = l2br_vlan_id;

	/* L2bridge vlan stats size */
	if (l2br_vlan_stats_size < 2 || l2br_vlan_stats_size > 128) {
		HM_MSG_DEV_ERR(dev, "Invalid vlan stats size\n");
		l2br_vlan_stats_size = 20;
	}
	if (l2br_vlan_stats_size != 20)
		priv->pfe_cfg->vlan_stats_size = l2br_vlan_stats_size;

	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node)) {
		ret = pfeng_gpr_set_port_coherency(priv);
		if (ret)
			goto err_drv;
	}

	/* PFE_SYS clock */
	priv->clk_sys = clk_get(dev, "pfe_sys");
	if (IS_ERR(priv->clk_sys)) {
		HM_MSG_DEV_ERR(dev, "Failed to get pfe_sys clock\n");
		ret = IS_ERR(priv->clk_sys);
		priv->clk_sys = NULL;
		goto err_drv;
	}
	ret = clk_set_rate(priv->clk_sys, PFE_CLK_SYS_RATE);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to set clock 'pfe_sys'. Error: %d\n", ret);
		goto err_drv;
	}
	ret = clk_prepare_enable(priv->clk_sys);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to enable clock 'pfe_sys'. Error: %d\n", ret);
		goto err_drv;
	}

	/* PFE_PE clock */
	priv->clk_pe = clk_get(dev, "pfe_pe");
	if (IS_ERR(priv->clk_pe)) {
		HM_MSG_DEV_ERR(dev, "Failed to get pfe_pe clock\n");
		ret = IS_ERR(priv->clk_pe);
		priv->clk_pe = NULL;
		goto err_drv;
	}
	/* PE clock should be double the frequency of System clock */
	ret = clk_set_rate(priv->clk_pe, clk_get_rate(priv->clk_sys) * 2);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to set clock 'pfe_pe'. Error: %d\n", ret);
		goto err_drv;
	}
	ret = clk_prepare_enable(priv->clk_pe);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to enable clock 'pfe_pe'. Error: %d\n", ret);
		goto err_drv;
	}
	HM_MSG_DEV_INFO(dev, "Clocks: sys=%luMHz pe=%luMHz\n", clk_get_rate(priv->clk_sys) / 1000000, clk_get_rate(priv->clk_pe) / 1000000);

	pm_runtime_get_noresume(dev);
	ret = pm_runtime_set_active(dev);
	if (ret)
		HM_MSG_DEV_WARN(dev, "Failed to set PM device status\n");

	pm_runtime_enable(dev);

	/* Set correct PFE_EMACs interfaces */
	if (pfeng_gpr_set_emac_interfaces(priv))
		HM_MSG_DEV_ERR(dev, "WARNING: cannot enable power for EMACs\n");

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
				HM_MSG_DEV_ERR(dev, "Failed to set TX clock on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			else {
				ret = clk_prepare_enable(emac->tx_clk);
				if (ret)
					HM_MSG_DEV_ERR(dev, "Failed to enable TX clocks on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			}
			if (ret) {
				devm_clk_put(dev, emac->tx_clk);
				emac->tx_clk = NULL;
			} else
				HM_MSG_DEV_INFO(dev, "TX clock on EMAC%d for interface %s installed\n", id, phy_modes(emac->intf_mode));
		}

		if (emac->rx_clk) {
			ret = clk_set_rate(emac->rx_clk, clk_rate);
			if (ret)
				HM_MSG_DEV_ERR(dev, "Failed to set RX clock on EMAC%d for interface %s. Error %d\n", id, phy_modes(emac->intf_mode), ret);
			else {
				ret = clk_prepare_enable(emac->rx_clk);
				if (ret) {
					HM_MSG_DEV_WARN(dev, "Defer enabling of RX clock on EMAC%d for interface %s (ret: %d)\n", id, phy_modes(emac->intf_mode), ret);
					emac->rx_clk_pending = true;
				}
			}
			if (ret) {
				if (!emac->rx_clk_pending) {
					devm_clk_put(dev, emac->rx_clk);
					emac->rx_clk = NULL;
				}
			} else
				HM_MSG_DEV_INFO(dev, "RX clock on EMAC%d for interface %s installed\n", id, phy_modes(emac->intf_mode));
		}
	}

	ret = oal_mm_init(dev);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "OAL memory managment init failed\n");
		goto err_drv;
	}

	/* Build CLASS firmware name */
	if (fw_class_name && strlen(fw_class_name))
		priv->fw_class_name = fw_class_name;
	if (!priv->fw_class_name || !strlen(priv->fw_class_name)) {
		HM_MSG_DEV_ERR(dev, "CLASS firmware is unknown\n");
		ret = -EINVAL;
		goto err_drv;
	}

	/* Build UTIL firmware name */
	if (fw_util_name && strlen(fw_util_name))
		priv->fw_util_name = ((strlen(fw_util_name) == 4) && !strncmp(fw_util_name, "NONE", 4)) ? (NULL) : (fw_util_name);
	if (!priv->fw_util_name || !strlen(priv->fw_util_name)) {
		HM_MSG_DEV_INFO(dev, "UTIL firmware not requested. Disable UTIL\n");
		priv->pfe_cfg->enable_util = false;
	} else
		priv->pfe_cfg->enable_util = true;

	/* Request firmware(s) */
	ret = pfeng_fw_load(priv, priv->fw_class_name, priv->fw_util_name);
	if (ret)
		goto err_drv;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* Set FCI ownership permission mask */
	if (fci_ownership_mask)
		priv->pfe_cfg->hif_fci_owner_chnls_mask = fci_ownership_mask;
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	/* Provide switch value for S32G2 ordered class writes */
	priv->pfe_cfg->g2_ordered_class_writes = g2_ordered_class_writes;

	/* Routing Table allocation option for S32G3 */
	priv->pfe_cfg->g3_rtable_in_lmem = g3_rtable_in_lmem;

	/* reserved TMU queue ID for lossless Tx (LLTX) */
	if (lltx_res_tmu_q_id < 0 || (lltx_res_tmu_q_id > 7 && lltx_res_tmu_q_id != PFENG_TMU_LLTX_DISABLE_MODE_Q_ID)) {
		HM_MSG_DEV_ERR(dev, "Invalid LLTX TMU queue id (%d)\n", lltx_res_tmu_q_id);
		ret = -EINVAL;
		goto err_drv;
	}

	priv->pfe_cfg->lltx_res_tmu_q_id = (u8)lltx_res_tmu_q_id;

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

	/* Check for silicon version */
	priv->on_g3 = pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3);

	/* Verify the max_speed values on EMACs */
	ret = pfeng_validate_emac_speeds(priv);
	if (ret)
		goto err_drv;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	ret = pfeng_gpr_ip_ready_set(dev, true);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to signal IP ready: %d\n", ret);
		return ret;
	}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	/* Create debugfs */
	pfeng_debugfs_create(priv);

	/* Prepare PTP clock */
	priv->clk_ptp_reference = 0U;
	priv->clk_ptp = clk_get(dev, "pfe_ts");
	if (IS_ERR(priv->clk_ptp)) {
		HM_MSG_DEV_WARN(dev, "Failed to get pfe_ts clock. PTP will be disabled.\n");
		priv->clk_ptp = NULL;
	} else {
		ret = clk_set_rate(priv->clk_ptp, PFE_CLK_TS_RATE);
		if (ret) {
			HM_MSG_DEV_WARN(dev, "Failed to set pfe_ts clock. PTP will be disabled.\n");
			priv->clk_ptp = NULL;
		} else {
			ret = clk_prepare_enable(priv->clk_ptp);
			if (ret) {
				priv->clk_ptp = NULL;
				HM_MSG_DEV_ERR(dev, "Failed to enable clock pfe_ts: %d\n", ret);
			} else
				priv->clk_ptp_reference = clk_get_rate(priv->clk_ptp);
		}
	}

	/* PHC for hif2hif */
	if (hif_phc_emac < PFENG_PFE_EMACS)
		priv->hif_phc_emac_id = hif_phc_emac;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* Set master_if for OAL_PFE_CFG_MASTER_IF */
	for (id = 0; id < PFENG_PFE_HIF_CHANNELS; id++) {
		if (priv->hif_chnl[id].ihc) {
			pfeng_pfe_cfg_master_if = (uint32_t)pfeng_hif_ids[priv->ihc_master_chnl];
			break;
		}
	}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

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

	return 0;

err_drv:
	pfeng_drv_remove(pdev);

	return ret;
}

/* PM support */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
static int pfeng_drv_pm_suspend_nosup(struct device *dev)
{
	HM_MSG_DEV_WARN(dev, "PFE Master driver PM suspend not supported\n");
	return -EOPNOTSUPP;
}
#endif

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
	int ret;

	HM_MSG_DEV_INFO(dev, "Suspending driver\n");

	priv->in_suspend = true;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (pfeng_gpr_ip_ready_set(dev, false))
		HM_MSG_DEV_ERR(dev, "Failed to signal IP not ready\n");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	pfeng_debugfs_remove(priv);

	/* NETIFs */
	ret = pfeng_netif_suspend(priv);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "PFE netif not suspended successfully\n");
		goto err_drv;
	}

#ifdef PFE_CFG_FCI_ENABLE
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

err_drv:
	return ret;
}

#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
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

	HM_MSG_DEV_INFO(dev, "Resuming driver\n");

	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node)) {
		ret = pfeng_gpr_set_port_coherency(priv);
		if (ret)
			return ret;
	}

	ret = pinctrl_pm_select_default_state(dev);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to select default pinctrl state\n");
		return -EINVAL;
	}

	/* Start clocks */
	if (!priv->clk_sys) {
		HM_MSG_DEV_ERR(dev, "Main clock 'pfe_sys' disappeared\n");
		return -ENODEV;
	}
	ret = clk_set_rate(priv->clk_sys, PFE_CLK_SYS_RATE);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to set clock 'pfe_sys'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_prepare_enable(priv->clk_sys);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to enable clock 'pfe_sys'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_set_rate(priv->clk_pe, clk_get_rate(priv->clk_sys) * 2);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to set clock 'pfe_pe'. Error: %d\n", ret);
		return -EINVAL;
	}
	ret = clk_prepare_enable(priv->clk_pe);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to enable clock 'pfe_pe'. Error: %d\n", ret);
		return -EINVAL;
	}

	/* Set correct PFE_EMACs interfaces */
	if (pfeng_gpr_set_emac_interfaces(priv))
		HM_MSG_DEV_ERR(dev, "WARNING: cannot enable power for EMACs\n");

	/* PFE reset */
	ret = pfeng_pfe_reset(priv);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Failed to reset PFE controller\n");
		goto err_pfe_init;
	}

	/* Reinit memory */
	ret = oal_mm_wakeup_reinit();
	if (ret) {
		HM_MSG_DEV_WARN(dev, "Failed to re-init PFE memory\n");
	}

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

	/* PTP clock */
	if (priv->clk_ptp) {
		ret = clk_set_rate(priv->clk_ptp, PFE_CLK_TS_RATE);
		if (ret) {
			HM_MSG_DEV_WARN(dev, "Failed to set pfe_ts clock. PTP will be disabled.\n");
			clk_put(priv->clk_ptp);
			priv->clk_ptp = NULL;
		} else {
			ret = clk_prepare_enable(priv->clk_ptp);
			if (ret) {
				HM_MSG_DEV_WARN(dev, "Failed to enable clock 'pfe_ts'. PTP will be disabled.\n");
				/* Free clock, now is unusable */
				clk_put(priv->clk_ptp);
				priv->clk_ptp = NULL;
			}
		}
	}

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

	return 0;

err_drv:
err_pfe_get:
err_pfe_init:

	return ret;
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

SIMPLE_DEV_PM_OPS(pfeng_drv_pm_ops,
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
			pfeng_drv_pm_suspend_nosup,
			NULL
#else
			pfeng_drv_pm_suspend,
			pfeng_drv_pm_resume
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
			);

/**
 * pfeng_drv_shutdown
 *
 * @pdev: platform device pointer
 * Description: this function calls at shut-down time to quiesce the device
 */
static void pfeng_drv_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pfeng_priv *priv = dev_get_drvdata(dev);

	pfeng_drv_pm_suspend(dev);

	if (priv && priv->rst)
		/* PFE Partition shutdown */
		if (!pfeng_pfe_off(priv))
			HM_MSG_DEV_INFO(dev, "PFE controller off\n");
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
