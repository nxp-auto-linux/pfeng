/*
 * Copyright 2019-2022 NXP
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

/* Major IP version for cut2.0 */
#define PFE_IP_MAJOR_VERSION_CUT2		2

/* PFE SYS CLK is 300MHz */
#define PFE_CLK_SYS_RATE			300000000

/* PFE TS CLK is 200MHz */
#define PFE_CLK_TS_RATE				200000000

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

/* The following parameter is currently defined in [fci_core_linux.c]: */
/*
static bool disable_netlink = false;
module_param(disable_netlink, bool, 0644);
MODULE_PARM_DESC(disable_netlink, "\t Do not create netlink socket for FCI communication (default: false)");
*/

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
static int disable_master_detection = 0;
module_param(disable_master_detection, int, 0644);
MODULE_PARM_DESC(disable_master_detection, "\t 1 - disable Master detection signalization (default is 0)");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

static int pfeng_s32g_set_port_coherency(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	void *syscon;
	int ret = 0;
	u32 val;

	syscon = ioremap(priv->syscon.start, priv->syscon.end - priv->syscon.start + 1);
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

	syscon = ioremap(priv->syscon.start, priv->syscon.end - priv->syscon.start + 1);
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
	priv->ihc_wq = create_singlethread_workqueue("pfeng-ihc");
	if (!priv->ihc_wq) {
		dev_err(dev, "Initialize of IHC TX failed\n");
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
		dev_info(dev, "PM runtime resume returned: %d\n", ret);
	}

	if (!priv) {
		dev_err(dev, "Removal failed. No priv data.\n");
		return -ENOMEM;
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	hal_ip_ready_set(false);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

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

static void pfeng_soc_version_check(struct device *dev)
{
	struct s32_soc_rev soc_rev;
	int ret;

	ret = s32_siul2_nvmem_get_soc_revision(dev, "soc_revision", &soc_rev);
	if (ret) {
		dev_warn(dev, "Failed to read SoC version (err: %d)\n", ret);
		return;
	}

	if (soc_rev.major < PFE_IP_MAJOR_VERSION_CUT2)
		dev_warn(dev, "Running cut 2.0 driver on SoC version %d.%d!\n",
			 soc_rev.major, soc_rev.minor);
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
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	dev_info(dev, "Multi instance support: Master/mdetect=%s\n", disable_master_detection ? "off" : "on");
#else
	dev_info(dev, "Multi instance support: disabled (standalone)\n");
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	dev_info(dev, "Compiled by: %s\n", __VERSION__);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	hal_ip_ready_set(false);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	pfeng_soc_version_check(dev);

	if (!of_dma_is_coherent(dev->of_node))
		dev_warn(dev, "DMA coherency disabled - consider impact on device performance\n");

	/* Attach to reset controller to reset S32G2 partition 2 */
	rst = devm_reset_control_get(dev, "pfe_part");
	if (IS_ERR(rst)) {
		dev_warn(dev, "Warning: Partition reset 'pfe_part' get failed: code %ld\n", PTR_ERR(rst));
		rst = NULL;
	}

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
	ret = pfeng_dt_create_config(priv);
	if (ret)
		goto err_drv;

	/* L2bridge default VLAN ID */
	if (!l2br_vlan_id || l2br_vlan_id > 4095) {
		dev_err(dev, "Invalid L2Bridge default VLAN ID, used 1\n");
		l2br_vlan_id = 1;
	}
	if (l2br_vlan_id != 1)
		priv->pfe_cfg->vlan_id = l2br_vlan_id;

	/* L2bridge vlan stats size */
	if (l2br_vlan_stats_size < 2 || l2br_vlan_stats_size > 128) {
		dev_err(dev, "Invalid vlan stats size\n");
		l2br_vlan_stats_size = 20;
	}
	if (l2br_vlan_stats_size != 20)
		priv->pfe_cfg->vlan_stats_size = l2br_vlan_stats_size;

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

	pm_runtime_get_noresume(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

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

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	hal_ip_ready_set(true);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

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

	ret = oal_mm_init(dev);
	if (ret) {
		dev_err(dev, "OAL memory managment init failed\n");
		goto err_drv;
	}

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
		priv->fw_util_name = ((strlen(fw_util_name) == 4) && !strncmp(fw_util_name, "NONE", 4)) ? (NULL) : (fw_util_name);
	if (!priv->fw_util_name || !strlen(priv->fw_util_name)) {
		dev_info(dev, "UTIL firmware not requested. Disable UTIL\n");
		priv->pfe_cfg->enable_util = false;
	} else
		priv->pfe_cfg->enable_util = true;

	/* Request firmware(s) */
	ret = pfeng_fw_load(priv, priv->fw_class_name, priv->fw_util_name);
	if (ret)
		goto err_drv;

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

	return 0;

err_drv:
	pfeng_drv_remove(pdev);

	return ret;
}

/* PM support */

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

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	hal_ip_ready_set(false);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	pfeng_debugfs_remove(priv);

	/* MDIO buses */
	pfeng_mdio_suspend(priv);

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

	/* Set HIF channels coherency */
	if (of_dma_is_coherent(dev->of_node)) {
		ret = pfeng_s32g_set_port_coherency(priv);
		if (ret)
			return ret;
	}

	pinctrl_pm_select_default_state(dev);

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

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	hal_ip_ready_set(true);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

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
