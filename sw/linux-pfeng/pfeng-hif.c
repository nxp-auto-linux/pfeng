/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

#include "pfeng.h"

int pfeng_hif_chnl_stop(struct pfeng_hif_chnl *chnl)
{
	/* Disable channel interrupt */
	pfe_hif_chnl_irq_mask(chnl->priv);

	/* Disable the channel RX interrupts */
	pfe_hif_chnl_rx_irq_mask(chnl->priv);

	/* Disable RX */
	pfe_hif_chnl_rx_disable(chnl->priv);

	/* Disable TX */
	pfe_hif_chnl_tx_disable(chnl->priv);

	dev_info(chnl->dev, "HIF%d stopped\n", chnl->idx);

	return 0;
}

int pfeng_hif_chnl_start(struct pfeng_hif_chnl *chnl)
{
	if (chnl->status == PFENG_HIF_STATUS_RUNNING)
		return 0;

	if (chnl->status != PFENG_HIF_STATUS_ENABLED)
		return -EINVAL;

	/* Enable channel interrupt */
	pfe_hif_chnl_irq_unmask(chnl->priv);

	/* Enable RX */
	if (pfe_hif_chnl_rx_enable(chnl->priv) != EOK) {
		dev_err(chnl->dev, "Couldn't enable RX irq\n");
		return -EINVAL;
	}

	/* Enable TX */
	if (pfe_hif_chnl_tx_enable(chnl->priv) != EOK) {
		dev_err(chnl->dev, "Couldn't enable TX\n");
		return -EINVAL;
	}

	/* Enable the channel RX interrupts */
	pfe_hif_chnl_rx_irq_unmask(chnl->priv);

	chnl->status = PFENG_HIF_STATUS_RUNNING;

	dev_info(chnl->dev, "HIF%d started\n", chnl->idx);

	return 0;
}

/**
 * @brief		HIF channel RX ISR
 * @details		Will be called by HIF channel instance when RX event has occurred
 * @note		To see which context the ISR is running in please see the
 * 				pfe_hif_chnl module implementation.
 */
static void pfeng_hif_drv_chnl_rx_isr(void *arg)
{
        struct pfeng_hif_chnl *chnl = (struct pfeng_hif_chnl *)arg;

	if(napi_schedule_prep(&chnl->napi)) {

		pfe_hif_chnl_rx_irq_mask(chnl->priv);

		__napi_schedule_irqoff(&chnl->napi);
	}
}

/**
 * @brief		Common HIF channel interrupt service routine
 * @details		Manage common HIF channel interrupt
 * @details		See the oal_irq_handler_t
 */
static irqreturn_t pfeng_hif_chnl_direct_isr(int irq, void *arg)
{
	pfe_hif_chnl_t *chnl = (pfe_hif_chnl_t *)arg;

	/* Disable HIF channel interrupts */
	pfe_hif_chnl_irq_mask(chnl);

	/* Call HIF channel ISR */
	pfe_hif_chnl_isr(chnl);

	/* Enable HIF channel interrupts */
	pfe_hif_chnl_irq_unmask(chnl);

	return IRQ_HANDLED;
}

/**
 * @brief	Process HIF channel receive
 * @details	Read HIF channel data
 * @param[in]	chnl The HIF channel
 * @param[in]	limit The recieve process limit
 * @return	Number of received frames
 */
static int pfeng_hif_chnl_rx(struct pfeng_hif_chnl *chnl, int limit)
{
	pfe_ct_hif_rx_hdr_t *hif_hdr;
	struct sk_buff *skb;
	struct net_device *netdev;
	struct pfeng_netif *netif;
	int done = 0;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	int ihcs = 0;
#endif

	while (1) {

		skb = pfeng_hif_chnl_receive_pkt(chnl, 0);
		if (unlikely(!skb))
			/* no more packets */
			break;

		hif_hdr = (pfe_ct_hif_rx_hdr_t *)skb->data;
		hif_hdr->flags = (pfe_ct_hif_rx_flags_t)oal_ntohs(hif_hdr->flags);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Check for IHC frame */
		if (unlikely (hif_hdr->flags & HIF_RX_IHC)) {
			pfe_hif_drv_client_t *client = &chnl->ihc_client;

			ihcs++;
			/* IHC client callback */
			if (!pfe_hif_drv_ihc_put_pkt(client, skb->data, skb->len, skb)) {

				/* Call IHC RX callback */
				client->event_handler(client, client->priv, EVENT_RX_PKT_IND, 0);
			} else {
				dev_err(chnl->dev, "RX IHC queuing failed. Origin PhyIf %d\n", hif_hdr->i_phy_if);
				kfree_skb(skb);
			}

			continue;
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

		/* Find appropriate netif */
		netif = chnl->netifs[hif_hdr->i_phy_if];
		if (!netif) {
			dev_err(chnl->dev, "Packet for unconfigured PhyIf %d\n", hif_hdr->i_phy_if);
			consume_skb(skb);
			continue;
		}
		netdev = netif->netdev;
		skb->dev = netdev;

		if (unlikely(hif_hdr->flags & HIF_RX_TS)) {

			/* Get rx hw time stamp */
			pfeng_hwts_skb_set_rx_ts(netif, skb);

		} else if(unlikely(hif_hdr->flags & HIF_RX_ETS)) {

			/* Get tx hw time stamp */
			pfeng_hwts_get_tx_ts(netif, skb);
			/* Skb has only time stamp report so consume it */
			consume_skb(skb);

			continue;
		}

		/* Cksumming support */
		if (likely(netdev->features & NETIF_F_RXCSUM)) {
			/* we have only OK info, signal it */
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			/* one level csumming support */
			skb->csum_level = 0;
		}

		/* Pass to upper layer */

		/* Skip HIF header */
		skb_pull(skb, PFENG_TX_PKT_HEADER_SIZE);

		skb->protocol = eth_type_trans(skb, netdev);

		if (unlikely(skb->ip_summed == CHECKSUM_NONE))
			netif_receive_skb(skb);
		else
			napi_gro_receive(&chnl->napi, skb);

		netdev->stats.rx_packets++;
		netdev->stats.rx_bytes += skb_headlen(skb);

		done++;
		if(unlikely(done == limit))
			break;
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	return done + ihcs;
#else
	return done;
#endif
}

static int pfeng_hif_chnl_rx_poll(struct napi_struct *napi, int budget)
{
	struct pfeng_hif_chnl *chnl = container_of(napi, struct pfeng_hif_chnl, napi);
	int done = 0;

	/* Consume RX pkt(s) */
	done = pfeng_hif_chnl_rx(chnl, budget);

	if (done < budget && napi_complete_done(napi, done)) {

		/* Enable RX interrupt */
		pfe_hif_chnl_rx_irq_unmask(chnl->priv);

		/* Trigger the RX DMA */
		pfe_hif_chnl_rx_dma_start(chnl->priv);
	}

	return done;
}

static int pfeng_hif_chnl_drv_remove(struct pfeng_priv *priv, u32 idx)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl;
	int ret = 0;

	if (idx >= PFENG_PFE_HIF_CHANNELS) {
		dev_err(dev, "Invalid HIF instance number: %u\n", idx);
		return -ENODEV;
	}
	chnl = &priv->hif_chnl[idx];

	/* Stop channel interrupt */
	pfeng_hif_chnl_stop(chnl);

	/* Stop NAPI */
	if (chnl->status == PFENG_HIF_STATUS_RUNNING) {
		napi_disable(&chnl->napi);
		netif_napi_del(&chnl->napi);
	}
	/* Prepare for startup state (in case of STR use) */
	chnl->status = PFENG_HIF_STATUS_REQUESTED;

	/* Release IRQ line */
	devm_free_irq(dev, priv->pfe_cfg->irq_vector_hif_chnls[idx], chnl->priv);

	/* Release attached RX/TX pools */
	pfeng_bman_pool_destroy(chnl);

	if (priv->pfe_cfg->irq_vector_hif_chnls[idx]) {
		disable_irq(priv->pfe_cfg->irq_vector_hif_chnls[idx]);
	}

	/* Forget HIF channel data */
	chnl->priv = NULL;

	dev_info(dev, "HIF%d disabled\n", idx);

	return ret;
}

static char *get_hif_chnl_mode_str(struct pfeng_hif_chnl *chnl)
{
	switch (chnl->cl_mode) {
	case PFENG_HIF_MODE_EXCLUSIVE:
		return "excl";
	case PFENG_HIF_MODE_SHARED:
		return "share";
	default:
		return "invalid";
	}
}

static int pfeng_hif_chnl_drv_create(struct pfeng_priv *priv, u32 idx)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl;
	int ret = 0;
	char irq_name[20];

	if (idx >= PFENG_PFE_HIF_CHANNELS) {
		dev_err(dev, "Invalid HIF instance number: %u\n", idx);
		return -ENODEV;
	}
	chnl = &priv->hif_chnl[idx];

	chnl->priv = pfe_hif_get_channel(priv->pfe_platform->hif, pfeng_chnl_ids[idx]);
	if (NULL == chnl->priv) {
		dev_err(dev, "Can't get HIF%d channel instance\n", idx);
		return -ENODEV;
	}
	chnl->dev = dev;
	chnl->idx = idx;
#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (unlikely(chnl->cl_mode == PFENG_HIF_MODE_SHARED))
#else
	if (unlikely((chnl->cl_mode == PFENG_HIF_MODE_SHARED) || chnl->ihc))
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef LOCK_TX_SPINLOCK
		spin_lock_init(&chnl->lock_tx);
#else
		mutex_init(&chnl->lock_tx);
#endif

	/* Register HIF channel RX callback */
	pfe_hif_chnl_set_event_cbk(chnl->priv, HIF_CHNL_EVT_RX_IRQ, &pfeng_hif_drv_chnl_rx_isr, (void *)chnl);

	/* Create interrupt name */
	scnprintf(irq_name, sizeof(irq_name), "pfe-hif-%d:%s", idx, get_hif_chnl_mode_str(chnl));

	/* HIF channel IRQ */
	ret = devm_request_irq(dev, priv->pfe_cfg->irq_vector_hif_chnls[idx], pfeng_hif_chnl_direct_isr,
		0, kstrdup(irq_name, GFP_KERNEL), chnl->priv);
	if (ret < 0) {
		dev_err(dev, "Error allocating the IRQ %d for '%s', error %d\n",
			priv->pfe_cfg->irq_vector_hif_chnls[idx], irq_name, ret);
		return ret;
	}

	/* Create bman for channel */
	if (!chnl->bman.rx_pool) {
		ret = pfeng_bman_pool_create(chnl);
		if (ret) {
			dev_err(dev, "Unable to attach bman to HIF%d\n", idx);
			goto err;
		}
		/* Fill pool of pages for rx buffers */
		pfeng_hif_chnl_fill_rx_buffers(chnl);
	}

	pfeng_debugfs_add_hif_chnl(priv, idx);

	/* Create dummy netdev required for independent HIF channel support */
	init_dummy_netdev(&chnl->dummy_netdev);

	chnl->status = PFENG_HIF_STATUS_ENABLED;
	netif_napi_add(&chnl->dummy_netdev, &chnl->napi, pfeng_hif_chnl_rx_poll, NAPI_POLL_WEIGHT);
	napi_enable(&chnl->napi);

	dev_info(dev, "HIF%d enabled\n", idx);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* IHC HIF channel must be enabled */
	if (chnl->ihc)
		pfeng_hif_chnl_start(chnl);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	return 0;

err:
	pfeng_hif_chnl_drv_remove(priv, idx);
	return ret;
}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
/* Release IDEX object */
static void pfeng_hif_idex_release(struct pfeng_priv *priv)
{
	struct pfeng_hif_chnl *chnl = priv->ihc_chnl;

	if (!priv->ihc_enabled)
		return;

	priv->ihc_enabled = false;

	if (chnl->ihc) {
		chnl->ihc = false;
		priv->ihc_chnl = NULL;
		pfe_idex_set_rpc_cbk(NULL, priv->pfe_platform);
		pfe_idex_fini();
		dev_info(&priv->pdev->dev, "IDEX RPC released. HIF IHC support disabled\n");
	}
}

/* Create IDEX object */
static int pfeng_hif_idex_create(struct pfeng_priv *priv, int idx)
{
	struct pfeng_hif_chnl *ihc_chnl = &priv->hif_chnl[idx];
	struct device *dev = &priv->pdev->dev;

	/* Install IDEX support */
	if (pfe_idex_init(&ihc_chnl->hif_drv, pfeng_hif_ids[priv->ihc_master_chnl])) {
		dev_err(dev, "Can't initialize IDEX, HIF IHC support disabled.\n");
		ihc_chnl->ihc = false;
		priv->ihc_enabled = false;
	} else {
		if (EOK != pfe_idex_set_rpc_cbk(&pfe_platform_idex_rpc_cbk, priv->pfe_platform)) {
			dev_err(dev, "Unable to set IDEX RPC callback. HIF IHC support disabled\n");
			ihc_chnl->ihc = false;
			priv->ihc_enabled = false;
			pfe_idex_fini();
		} else {
			priv->ihc_enabled = true;
			priv->ihc_chnl = ihc_chnl;
			dev_info(dev, "IDEX RPC installed\n");
		}
	}

	return 0;
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief	Create HIF channels
 * @details	Creates and initializes the HIF channels
 * @param[in]	priv The driver main structure
 * @return	> 0 if OK, negative error number if failed
 */
int pfeng_hif_create(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int idx, ret;

	ret = 0;

	for (idx = 0; idx < PFENG_PFE_HIF_CHANNELS; idx++) {

		if (priv->hif_chnl[idx].status != PFENG_HIF_STATUS_REQUESTED) {
			dev_info(dev, "HIF%d not configured, skipped\n", idx);
			continue;
		}

		ret = pfeng_hif_chnl_drv_create(priv, idx);
		if (ret) {
			dev_err(dev, "HIF %d can't be created\n", idx);
			return -EIO;
		}

		/* Set local driver_id */
#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Use lowest managed HIF channel */
		if (priv->local_drv_id > idx)
			priv->local_drv_id = idx;
#else
		/* Use IHC channel */
		if (priv->hif_chnl[idx].ihc) {
			priv->local_drv_id = idx;
			pfeng_hif_idex_create(priv, idx);
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	}

	return ret;
}

/**
 * @brief	Destroy the HIF channels
 * @details	Unregister and destroy the HIF channels
 * @param[in]	priv The driver main structure
 */
void pfeng_hif_remove(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int idx;

	if (!priv)
		return;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	pfeng_hif_idex_release(priv);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	for (idx = (PFENG_PFE_HIF_CHANNELS - 1); idx >= 0; idx--) {

		if (priv->hif_chnl[idx].status < PFENG_HIF_STATUS_ENABLED) {
			dev_info(dev, "HIF%d not enabled, skipped\n", idx);
			continue;
		}

		pfeng_hif_chnl_drv_remove(priv, idx);
	}
}
