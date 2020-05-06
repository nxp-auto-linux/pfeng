/*
 * 2020 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/rtnetlink.h>
#include <linux/clk.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

#include "pfeng.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jan Petrous <jan.petrous@nxp.com>");
MODULE_DESCRIPTION("PFEng driver");
MODULE_VERSION(PFENG_DRIVER_VERSION);
MODULE_FIRMWARE(PFENG_FW_NAME);

static const u32 default_msg_level = (
	NETIF_MSG_DRV | NETIF_MSG_PROBE |
	NETIF_MSG_LINK | NETIF_MSG_IFUP |
	NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

static char *fw_name;
module_param(fw_name, charp, 0444);
#ifdef OPT_FW_EMBED
#define FW_DESC_TXT ", use - for built-in variant"
#else
#define FW_DESC_TXT ""
#endif
MODULE_PARM_DESC(fw_name, "\t The name of firmware file (default: read from device-tree" PFENG_FW_NAME FW_DESC_TXT ")");

static int msg_verbosity = PFE_CFG_VERBOSITY_LEVEL;
module_param(msg_verbosity, int, 0644);
MODULE_PARM_DESC(msg_verbosity, "\t 0 - 9, default 4");

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

void pfeng_hif_chnl_drv_remove(struct pfeng_hif_chnl *chnl)
{
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

int pfeng_hif_chnl_drv_create(struct pfeng_priv *priv, u32 hif_chnl, bool hif_chnl_sc, struct pfeng_hif_chnl *chnl)
{
	char irq_name[20];
	int ret;

	if (hif_chnl >= ARRAY_SIZE(pfeng_chnl_ids)) {
		dev_err(&priv->pdev->dev, "Invalid HIF instance number: %u\n", hif_chnl);
		return -ENODEV;
	}

	chnl->priv = pfe_hif_get_channel(priv->pfe->hif, pfeng_chnl_ids[hif_chnl]);
	if (NULL == chnl->priv) {
		dev_err(&priv->pdev->dev, "Can't get HIF%d channel instance\n", hif_chnl);
		return -ENODEV;
	}

	/*	Create interrupt name */
	scnprintf(irq_name, sizeof(irq_name), "pfe-hif-%d-%s", hif_chnl, hif_chnl_sc ? "sc" : "mc");

#ifdef OAL_IRQ_MODE
	/*	Create interrupt */
	chnl->irq = oal_irq_create(
			priv->cfg->irq_vector_hif_chnls[hif_chnl],
			(oal_irq_flags_t)0,
			irq_name);

	if (NULL == chnl->irq) {
		dev_err(&priv->pdev->dev, "Could not create HIF%d IRQ '%s'\n", hif_chnl, irq_name);
		ret = -ENODEV;
		goto err;
	}

	/*	Install IRQ handler */
	ret = oal_irq_add_handler(chnl->irq, hif_chnl_isr, chnl->priv, NULL);
	if (EOK != ret) {
		dev_err(&priv->pdev->dev, "Could not add IRQ handler '%s'\n", irq_name);
		goto err;
	}
#else
	/* direct HIF channel IRQ */

	ret = request_irq(priv->cfg->irq_vector_hif_chnls[hif_chnl], pfeng_chnl_direct_isr,
		0, kstrdup(irq_name, GFP_KERNEL), chnl->priv);
	if (unlikely(ret < 0)) {
		dev_err(&priv->pdev->dev, "Error allocating the IRQ %d for '%s', error %d\n",
			priv->cfg->irq_vector_hif_chnls[hif_chnl], irq_name, ret);
               	return ret;
	}
	chnl->irqnum = priv->cfg->irq_vector_hif_chnls[hif_chnl];
#endif

	/*	Create HIF driver for the channel */
	chnl->drv = pfe_hif_drv_create(chnl->priv);
	if (NULL == chnl->drv) {
		dev_err(&priv->pdev->dev, "Could not get HIF%d driver instance\n", hif_chnl);
		ret = -ENODEV;
		goto err;
	}
	
	if (EOK != pfe_hif_drv_init(chnl->drv)) {
		dev_err(&priv->pdev->dev, "HIF%d drv init failed\n", hif_chnl);
		ret = -ENODEV;
		goto err;
	}

	return 0;

err:
	pfeng_hif_chnl_drv_remove(chnl);
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
			pfeng_mdio_unregister(ndev);
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

	/* PFE platform layer init */
	oal_mm_init(dev);

	dev_set_drvdata(dev, priv);

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

	/* Load firmware */
	if (fw_name && strlen(fw_name))
		priv->fw_name = fw_name;
	if (!priv->fw_name || !strlen(priv->fw_name))
		priv->fw_name = PFENG_FW_NAME;
	ret = pfeng_fw_load(priv, priv->fw_name);
	if (ret) {
		dev_err(dev, "Failed to load firmware '%s': %d\n", priv->fw_name, ret);
		goto err;
	}

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

	/* Create network interfaces */
	list_for_each_entry_safe(eth, tmp, &priv->plat.eth_list, lnode) {
		ndev = pfeng_napi_if_create(priv, eth);
		if (!ndev)
			goto err;

		pfeng_mdio_register(ndev);

		list_add_tail(&ndev->lnode, &priv->ndev_list);
	}

	/* debug fs */
	pfeng_debugfs_create(priv);

	return 0;

err:
	pfeng_drv_remove(priv);

	return ret;
}
