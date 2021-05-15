/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_PFE_HIF_DRV
 * @{
 *
 * @file		pfe_hif_drv.c
 * @brief		The single-client HIF driver source file.
 * @details		HIF driver supporting only single client to optimize performance (no RX traffic
 * 				dispatching, no TX resource locking, no internal detached jobs).
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "fifo.h"
#include "pfe_hif.h"
#include "pfe_hif_drv.h"
#include "pfe_platform_cfg.h"

#include "pfeng.h"

#include <linux/skbuff.h>

/* We need pfe_hif_drv API compatibility only for IHC/IDEX */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

typedef struct pfe_hif_pkt_tag pfe_hif_tx_meta_t;
typedef struct pfe_hif_pkt_tag pfe_hif_rx_meta_t;

/* structs pfe_hif_drv_tag and pfe_hif_drv_client_tag are declared in pfeng.h */

void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	/*	Release IHC fifo */
	if (client->ihc_rx_fifo) {
		u32 fill_level;
		errno_t err = fifo_get_fill_level(client->ihc_rx_fifo, &fill_level);

		if (unlikely(EOK != err)) {
			dev_info(chnl->dev, "Unable to get IHC fifo fill level: %d\n", err);
		} else if (fill_level) {
			dev_info(chnl->dev, "IHC Queue is not empty\n");
		}

		fifo_destroy(client->ihc_rx_fifo);
		client->ihc_rx_fifo = NULL;
	}

	if (client->ihc_txconf_fifo) {
		u32 fill_level;
		errno_t err = fifo_get_fill_level(client->ihc_txconf_fifo, &fill_level);

		if (unlikely(EOK != err)) {
			dev_info(chnl->dev, "Unable to get IHC tx conf fifo fill level: %d\n", err);
		} else if (fill_level) {
			dev_info(chnl->dev, "IHC Tx conf Queue is not empty\n");
		}

		fifo_destroy(client->ihc_txconf_fifo);
		client->ihc_txconf_fifo = NULL;
	}

	/*	Cleanup memory */
	memset(client, 0, sizeof(pfe_hif_drv_client_t));

	dev_info(chnl->dev, "IHC client unregistered\n");
}

pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv)
{

	struct pfeng_hif_chnl *chnl = container_of(hif_drv, struct pfeng_hif_chnl, hif_drv);
	pfe_hif_drv_client_t *client = &chnl->ihc_client;
	int ret;

	if (client->hif_drv) {
		dev_err(chnl->dev, "IHC client already registered\n");
		return NULL;
	}

	/* Initialize the instance */
	memset(client, 0, sizeof(pfe_hif_drv_client_t));
	client->ihc_rx_fifo = fifo_create(32);
	if (!client->ihc_rx_fifo) {
		dev_err(chnl->dev, "Can't create IHC RX fifo. Err %d\n", ret);
		return NULL;
	}
	client->ihc_txconf_fifo = fifo_create(32);
	if (!client->ihc_txconf_fifo) {
		dev_err(chnl->dev, "Can't create IHC TXconf fifo. Err %d\n", ret);
		return NULL;
	}
	client->hif_drv = hif_drv;
	client->priv = priv;
	client->event_handler = handler;
	client->inited = true;

	dev_info(chnl->dev, "IHC client registered\n");
	return client;
}

/**
 * @brief		Release packet
 * @param[in]	pkt The packet instance
 */
void pfe_hif_pkt_free(const pfe_hif_pkt_t *pkt)
{
	if (pkt->ref_ptr)
		kfree_skb(pkt->ref_ptr);
	oal_mm_free(pkt);
}

/**
 * @brief		Get packet from RX queue for IHC data
 * @param[in]	client IHC Client instance
 * @param[in]	queue RX queue number
 * @return		Pointer to SW buffer descriptor containing the packet or NULL
 * 				if the queue does not contain data
 *
 * @warning		Intended to be called for IHC client only
 */
pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	if (&chnl->ihc_client != client)
	{
		/* Only IHC client supported */
		dev_err(chnl->dev, "Only HIF IHC client supported\n");
		return NULL;
	}

	/* No resource protection here */
	return fifo_get(client->ihc_rx_fifo);
}

errno_t pfe_hif_drv_ihc_put_conf(pfe_hif_drv_client_t *client)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	struct sk_buff *skb = pfeng_hif_chnl_txconf_get_skbuf(chnl);
	void *idex_frame;

	if (unlikely(!skb))
		return EINVAL;

	/* Convert skb to IDEX frame */
	idex_frame = oal_mm_malloc_contig_aligned_nocache(skb_headlen(skb), 0U);
	if (likely(idex_frame)) {
		memcpy(idex_frame, skb->data, skb_headlen(skb));

		if (unlikely(EOK != fifo_put(client->ihc_txconf_fifo, idex_frame))) {
			dev_err(chnl->dev, "IHC TX fifo full\n");
			oal_mm_free_contig(idex_frame);
			return EINVAL;
		}
	} else
		return ENOMEM;

	return EOK;
}

errno_t pfe_hif_drv_ihc_put_pkt(pfe_hif_drv_client_t *client, void *data, uint32_t len, void *ref)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	pfe_hif_rx_meta_t *rx_metadata;
	pfe_ct_hif_rx_hdr_t *hif_hdr = (pfe_ct_hif_rx_hdr_t *)data;

	/*	Create the RX metadata */
	rx_metadata = (pfe_hif_rx_meta_t *)oal_mm_malloc(sizeof(pfe_hif_rx_meta_t));
	if (NULL == rx_metadata)
		return ENOMEM;
	memset(rx_metadata, 0, sizeof(pfe_hif_rx_meta_t));

	rx_metadata->client = client;
	rx_metadata->data = (addr_t)data;
	rx_metadata->len = len;

	rx_metadata->flags.specific.rx_flags = hif_hdr->flags;
	rx_metadata->i_phy_if = hif_hdr->i_phy_if;
	rx_metadata->ref_ptr = ref;

	if (unlikely(EOK != fifo_put(client->ihc_rx_fifo, rx_metadata))) {
		dev_err(chnl->dev, "IHC RX fifo full\n");
		/*	Drop the frame. Resource protection is embedded. */
		pfe_hif_pkt_free(rx_metadata);
		return EINVAL;
	}

	return EOK;
}

/**
 * @brief		Get TX confirmation
 * @param[in]	client Client instance
 * @param[in]	queue TX queue number
 * @return		Pointer to data associated with the transmitted buffer. See pfe_hif_drv_client_xmit_pkt()
 * 				and pfe_hif_drv_client_xmit_sg_pkt().
 * @note		Only a single thread can call this function for given client+queue
 * 				combination.
 */
void *pfe_hif_drv_client_receive_tx_conf(const pfe_hif_drv_client_t *client, uint32_t queue)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	if (&chnl->ihc_client != client)
	{
		/* Only IHC client supported */
		dev_err(chnl->dev, "Only HIF IHC client supported\n");
		return NULL;
	}

	/* No resource protection here */
	return fifo_get(client->ihc_txconf_fifo);
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/** @}*/
