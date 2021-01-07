/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/rtnetlink.h>
#include <linux/clk.h>
#include <linux/kthread.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

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

static const u32 default_msg_level = (
	NETIF_MSG_DRV | NETIF_MSG_PROBE |
	NETIF_MSG_LINK | NETIF_MSG_IFUP |
	NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

#ifdef PFE_CFG_PFE_MASTER
static char *fw_class_name;
module_param(fw_class_name, charp, 0444);
MODULE_PARM_DESC(fw_class_name, "\t The name of CLASS firmware file (default: read from device-tree or " PFENG_FW_CLASS_NAME ")");

static char *fw_util_name;
module_param(fw_util_name, charp, 0444);
MODULE_PARM_DESC(fw_util_name, "\t The name of UTIL firmware file (default: read from device-tree or " PFENG_FW_UTIL_NAME ")");
#endif

static int msg_verbosity = PFE_CFG_VERBOSITY_LEVEL;
module_param(msg_verbosity, int, 0644);
MODULE_PARM_DESC(msg_verbosity, "\t 0 - 9, default 4");

#ifdef PFE_CFG_PFE_SLAVE
static int master_ihc_chnl = HIF_CFG_MAX_CHANNELS;
module_param(master_ihc_chnl, int, 0644);
MODULE_PARM_DESC(master_ihc_chnl, "\t 0 - <max-hif-chn-number>, default read from DT or invalid");
#endif

/**
 * @brief		Common HIF channel interrupt service routine
 * @details		Manage common HIF channel interrupt
 * @details		See the oal_irq_handler_t
 */
#ifdef OAL_IRQ_MODE
static bool_t pfeng_hif_chnl_isr(void *arg)
#else
static irqreturn_t pfeng_chnl_direct_isr(int irq, void *arg)
#endif
{
	pfe_hif_chnl_t *chnl = (pfe_hif_chnl_t *)arg;

	/*	Disable HIF channel interrupts */
	pfe_hif_chnl_irq_mask(chnl);

	/*	Call HIF channel ISR */
	pfe_hif_chnl_isr(chnl);

	/*	Enable HIF channel interrupts */
	pfe_hif_chnl_irq_unmask(chnl);

	return IRQ_HANDLED;
}

int pfeng_drv_cfg_get_emac_intf_mode(struct pfeng_priv *priv, u8 id)
{
	struct pfeng_eth *eth;

	list_for_each_entry(eth, &priv->plat.eth_list, lnode) {
		if(eth->emac_id == id)
			return eth->intf_mode;
	}

	return -ENODEV;
}

struct pfeng_priv *pfeng_drv_alloc(struct platform_device *pdev)
{
	struct pfeng_priv *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return NULL;

	priv->pdev = pdev;

	priv->cfg = devm_kzalloc(&pdev->dev, sizeof(*priv->cfg), GFP_KERNEL);
	if(!priv->cfg)
		goto err_cfg_alloc;

	INIT_LIST_HEAD(&priv->ndev_list);
	INIT_LIST_HEAD(&priv->plat.eth_list);

	/* cfg defaults */
	priv->msg_enable = default_msg_level;
	priv->msg_verbosity = msg_verbosity;

	return priv;

err_cfg_alloc:
	devm_kfree(&pdev->dev, priv);
	return NULL;
}

void pfeng_hif_chnl_drv_remove(struct pfeng_ndev *ndev)
{
	struct pfeng_hif_chnl *chnl = &ndev->chnl_sc;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (ndev->eth->ihc) {
		pfe_idex_fini();
		ndev->eth->ihc = false;
	}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef OAL_IRQ_MODE
	/* Uninstall HIF channel IRQ */
	if(chnl->irq) {
		oal_irq_destroy(chnl->irq);
		chnl->irq = NULL;
	}
#else
	if (chnl->irqnum) {
		free_irq(chnl->irqnum, chnl->priv);
		chnl->irqnum = 0;
	}
#endif
	/* Stop and destroy HIF driver */
	if(chnl->drv) {
		pfe_hif_drv_destroy(chnl->drv);
		chnl->drv = NULL;
	}

	/* Forget HIF channel data */
	chnl->priv = NULL;
}

int pfeng_hif_chnl_drv_create(struct pfeng_ndev *ndev)
{
	u32 hif_chnl = ndev->eth->hif_chnl_sc;
	struct pfeng_hif_chnl *chnl = &ndev->chnl_sc;
	bool hif_chnl_sc = true;
	char irq_name[20];
	int ret;

	if (hif_chnl >= ARRAY_SIZE(pfeng_chnl_ids)) {
		netdev_err(ndev->netdev, "Invalid HIF instance number: %u\n", hif_chnl);
		return -ENODEV;
	}

	chnl->priv = pfe_hif_get_channel(ndev->priv->pfe->hif, pfeng_chnl_ids[hif_chnl]);
	if (NULL == chnl->priv) {
		netdev_err(ndev->netdev, "Can't get HIF%d channel instance\n", hif_chnl);
		return -ENODEV;
	}

	/*	Create interrupt name */
	scnprintf(irq_name, sizeof(irq_name), "pfe-hif-%d-%s", hif_chnl, hif_chnl_sc ? "sc" : "mc");

	/* direct HIF channel IRQ */
	ret = request_irq(ndev->priv->cfg->irq_vector_hif_chnls[hif_chnl], pfeng_chnl_direct_isr,
		0, kstrdup(irq_name, GFP_KERNEL), chnl->priv);
	if (unlikely(ret < 0)) {
		netdev_err(ndev->netdev, "Error allocating the IRQ %d for '%s', error %d\n",
			ndev->priv->cfg->irq_vector_hif_chnls[hif_chnl], irq_name, ret);
		return ret;
	}
	chnl->irqnum = ndev->priv->cfg->irq_vector_hif_chnls[hif_chnl];

	/*	Create HIF driver for the channel */
	chnl->drv = pfe_hif_drv_create(chnl->priv);
	if (NULL == chnl->drv) {
		netdev_err(ndev->netdev, "Could not get HIF%d driver instance\n", hif_chnl);
		ret = -ENODEV;
		goto err;
	}

	if (EOK != pfe_hif_drv_init(chnl->drv)) {
		netdev_err(ndev->netdev, "HIF%d drv init failed\n", hif_chnl);
		ret = -ENODEV;
		goto err;
	}

	return 0;

err:
	pfeng_hif_chnl_drv_remove(ndev);
	return ret;
}

int pfeng_drv_remove(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_ndev *ndev;

	if (!priv) {
		dev_err(dev, "driver removal failed\n");
		return -ENODEV;
	}

	/* Remove debugfs directory */
	pfeng_debugfs_remove(priv);

	/* NAPI shutdown */
	if (!list_empty(&priv->ndev_list)) {
		list_for_each_entry(ndev, &priv->ndev_list, lnode) {
#ifdef PFE_CFG_PFE_MASTER
			pfeng_mdio_unregister(ndev);
#endif
			pfeng_napi_if_release(ndev);
		}
		list_del(&priv->ndev_list);
	}

	/* PFE platform remove */
	if (priv->pfe) {
		if (pfe_platform_remove() != EOK)
			dev_err(dev, "PFE Platform not stopped successfully\n");
		else {
			priv->pfe = NULL;
			dev_info(dev, "PFE Platform stopped\n");
		}
	}

	/* Shutdown memory management */
	oal_mm_shutdown();

	/* Release firmware */
	if (priv->cfg->fw)
		pfeng_fw_free(priv);

	dev_set_drvdata(dev, NULL);

	return 0;
}

int pfeng_drv_probe(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_ndev *ndev;
	struct pfeng_eth *eth, *tmp;
	int ret;

#ifdef PFE_CFG_PFE_SLAVE
	/* HIF IHC channel number */
	if (master_ihc_chnl < HIF_CFG_MAX_CHANNELS)
		priv->plat.ihc_master_chnl = master_ihc_chnl;
	if (priv->plat.ihc_master_chnl >= HIF_CFG_MAX_CHANNELS) {
		dev_err(dev, "Slave mode required parameter for master channel id is missing\n");
		return -EINVAL;
	}
#endif

	/* PFE platform layer init */
	oal_mm_init(dev);

	dev_set_drvdata(dev, priv);

#ifdef PFE_CFG_PFE_MASTER
	/* apb clock */
	priv->sys_clk = devm_clk_get(dev, "pfe_sys");
	if (IS_ERR(priv->sys_clk)) {
		dev_err(dev, "Failed to get pfe_sys clock\n");
		ret = -ENODEV;
		priv->sys_clk = NULL;
		goto err;
	}
	ret = clk_prepare_enable(priv->sys_clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock pfe_sys: %d\n", ret);
		goto err;
	}

	/* Build CLASS firmware name */
	if (fw_class_name && strlen(fw_class_name))
		priv->fw_class_name = fw_class_name;
	if (!priv->fw_class_name || !strlen(priv->fw_class_name))
		priv->fw_class_name = PFENG_FW_CLASS_NAME;

	/* Build UTIL firmware name (optional) */
	if (fw_util_name && strlen(fw_util_name))
		priv->fw_util_name = fw_util_name;
	if (!priv->fw_util_name || !strlen(priv->fw_util_name)) {
		dev_info(dev, "UTIL firmware not requested. Disable UTIL\n");
		priv->cfg->enable_util = false;
	} else
		priv->cfg->enable_util = true;

	/* Request firmware(s) */
	ret = pfeng_fw_load(priv, priv->fw_class_name, priv->fw_util_name);
	if (ret)
		goto err;
#endif

	/* Start PFE Platform */
	ret = pfe_platform_init(priv->cfg);
	if (ret)
		goto err;

	priv->pfe = pfe_platform_get_instance();
	if (!priv->pfe) {
		dev_err(dev, "Could not get PFE platform instance\n");
		ret = -EINVAL;
		goto err;
	}

	/* debug fs */
	pfeng_debugfs_create(priv);

	/* Create network interfaces */
	list_for_each_entry_safe(eth, tmp, &priv->plat.eth_list, lnode) {
		ndev = pfeng_napi_if_create(priv, eth);
		if (!ndev)
			goto err;

#ifdef PFE_CFG_PFE_MASTER
		pfeng_mdio_register(ndev);
#endif
		list_add_tail(&ndev->lnode, &priv->ndev_list);
	}

	return 0;

err:
	pfeng_drv_remove(priv);

	return ret;
}
