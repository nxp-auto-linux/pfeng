/* =========================================================================
 *  Copyright 2018-2020 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <string.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "fifo.h"
#include "pfe_hif.h"
#include "pfe_hif_drv.h"
#include "pfe_platform_cfg.h"

#include <linux/skbuff.h>

typedef struct __pfe_hif_pkt_tag pfe_hif_tx_meta_t;
typedef struct __pfe_hif_pkt_tag pfe_hif_rx_meta_t;

/**
 * @brief	The HIF driver client instance structure - single client
 */
struct __attribute__((aligned(HAL_CACHE_LINE_SIZE), packed)) __pfe_pfe_hif_drv_client_tag
{
	pfe_ct_phy_if_id_t phy_if_id;
	pfe_hif_drv_client_event_handler event_handler;
	void *priv;
	pfe_hif_drv_t *hif_drv;
	bool_t active;

	struct {
		fifo_t *rx_fifo;	/* This is the client's RX ring */
		uint32_t size;
	} ihc;

#ifdef PFE_CFG_IEEE1588_SUPPORT
	/*	Storage for PTP timestamps */
	pfe_hif_ptp_ts_db_t ptpdb;
#endif /* PFE_CFG_IEEE1588_SUPPORT */
};

/**
 * @brief	The HIF driver instance structure
 */
struct __attribute__((aligned(HAL_CACHE_LINE_SIZE), packed)) __pfe_hif_drv_tag
{
/*	Common */
	pfe_hif_chnl_t *channel;

/*	HIF RX processing */
	bool_t started;
	bool_t rx_enabled;

/*	TX and TX confirmation processing */
	pfe_hif_tx_meta_t *tx_meta;
	uint32_t tx_meta_rd_idx;
	uint32_t tx_meta_wr_idx;
	pfe_ct_phy_if_id_t i_phy_if;
	bool_t tx_enabled;

/*	Single client per instance only */
	pfe_hif_drv_client_t client __attribute__((aligned(HAL_CACHE_LINE_SIZE)));

/*	Special client to be used for HIF-to-HIF communication */
	pfe_hif_drv_client_t ihc_client;
	volatile bool_t initialized;	/*	If TRUE the HIF has been properly initialized */
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	oal_mutex_t tx_lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));	/*	TX resources protection object */
#endif
};

/*	Channel management */
static errno_t pfe_hif_drv_create_data_channel(pfe_hif_drv_t *hif_drv);
static void pfe_hif_drv_destroy_data_channel(pfe_hif_drv_t *hif_drv);

/**
 * @brief		HIF channel RX ISR
 * @details		Will be called by HIF channel instance when RX event has occurred
 * @note		To see which context the ISR is running in please see the
 * 				pfe_hif_chnl module implementation.
 */
static void pfe_hif_drv_chnl_rx_isr(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;

	/*	Call directly the client's event handler */
	(void)hif_drv->client.event_handler(&hif_drv->client, hif_drv->client.priv, EVENT_RX_PKT_IND, 0U);
}

/**
 * @brief	Indicate end of reception
 * @details	Re-enable interrupts, trigger DMA, ...
 */
void pfe_hif_drv_client_rx_done(pfe_hif_drv_client_t *client)
{
	/*	Enable RX interrupt */
	pfe_hif_chnl_rx_irq_unmask(client->hif_drv->channel);

	/*	Trigger the RX DMA */
	pfe_hif_chnl_rx_dma_start(client->hif_drv->channel);
}

/**
 * @brief	Indicate end of TX confirmation
 * @details	Re-enable interrupts, trigger DMA, ...
 */
void pfe_hif_drv_client_tx_done(pfe_hif_drv_client_t *client)
{
	/*	NOOP */
}

static errno_t pfe_hif_drv_create_data_channel(pfe_hif_drv_t *hif_drv)
{
	errno_t ret = EOK;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Allocate the TX metadata storage and initialize indexes */
	hif_drv->tx_meta = oal_mm_malloc(sizeof(pfe_hif_tx_meta_t) * pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel));
	if (NULL == hif_drv->tx_meta)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		ret = ENOMEM;
		goto destroy_and_fail;
	}

	memset(hif_drv->tx_meta, 0, sizeof(pfe_hif_tx_meta_t) * pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel));
	hif_drv->tx_meta_rd_idx = 0U;
	hif_drv->tx_meta_wr_idx = 0U;

	/*	Allocate HIF TX headers. Allocate smaller chunks to reduce memory segmentation. */
	{
		uint32_t ii;

		for (ii=0U; ii<pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel); ii++)
		{
			hif_drv->tx_meta[ii].hif_tx_header = oal_mm_malloc_contig_named_aligned_nocache(PFE_CFG_BD_MEM, sizeof(pfe_ct_hif_tx_hdr_t), 8U);
			if (NULL == hif_drv->tx_meta[ii].hif_tx_header)
			{
				NXP_LOG_ERROR("Memory allocation failed");
				ret = ENOMEM;
				goto destroy_and_fail;
			}

			hif_drv->tx_meta[ii].hif_tx_header_pa = oal_mm_virt_to_phys_contig((void *)hif_drv->tx_meta[ii].hif_tx_header);
			if (NULL == hif_drv->tx_meta[ii].hif_tx_header_pa)
			{
				NXP_LOG_ERROR("VA-PA conversion failed\n");
				ret = EIO;
				goto destroy_and_fail;
			}

			/*	Initialize channel ID */
			hif_drv->tx_meta[ii].hif_tx_header->chid = pfe_hif_chnl_get_id(hif_drv->channel);
		}
	}

	return EOK;

destroy_and_fail:
	pfe_hif_drv_destroy_data_channel(hif_drv);

	return ret;
}

/**
 * @brief	Destroy HIF channel and release allocated resources
 * @details	Will also release all RX buffers associated with RX ring and confirm
 * 			all pending TX frames from the TX ring.
 */
static void pfe_hif_drv_destroy_data_channel(pfe_hif_drv_t *hif_drv)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Disable and invalidate RX and TX */
	pfe_hif_chnl_rx_disable(hif_drv->channel);
	pfe_hif_chnl_tx_disable(hif_drv->channel);

	{
		uint32_t ii;

		/*	Release dynamic HIF TX headers */
		for (ii=0U; ii<pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel); ii++)
		{
			if (NULL != hif_drv->tx_meta[ii].hif_tx_header)
			{
				oal_mm_free_contig(hif_drv->tx_meta[ii].hif_tx_header);
				hif_drv->tx_meta[ii].hif_tx_header = NULL;
			}
		}
	}

	/*	Release the TX metadata storage */
	if (NULL != hif_drv->tx_meta)
	{
		oal_mm_free(hif_drv->tx_meta);
		hif_drv->tx_meta = NULL;
	}

	return;
}

/**
 * @brief		This function is used to register a client driver with the HIF driver.
 * @details		Routine creates new HIF driver client, associates it with given logical interface
 * 				and adjusts internal HIF dispatching table to properly route ingress packets to
 * 				client's queues. HIF driver remains suspended after the call and pfe_hif_drv_start()
 * 				is required to re-enable the operation.
 * @param[in]	hif_drv The HIF driver instance the client shall be associated with
 * @param[in]	phy_if_id Physical interface ID to be handled by the client
 * @param[in]	txq_num Number of client's TX queues
 * @param[in]	rxq_num Number of client's RX queues
 * @param[in]	txq_depth Depth of each TX queue
 * @param[in]	rxq_depth Depth of each RX queue
 * @param[in]	handler Pointer to function to be called to indicate events (data available, ...).
 * 						Mandatory. Can be called from various contexts.
 * @param[in]	priv Private data to be stored within the client instance
 *
 * @return 		Client instance or NULL if failed
 */
pfe_hif_drv_client_t * pfe_hif_drv_client_register(pfe_hif_drv_t *hif_drv, pfe_ct_phy_if_id_t phy_if_id, uint32_t txq_num, uint32_t rxq_num,
								uint32_t txq_depth, uint32_t rxq_depth, pfe_hif_drv_client_event_handler handler, void *priv)
{
	pfe_hif_drv_client_t *client = NULL;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely((NULL == hif_drv) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	NXP_LOG_INFO("Attempt to register HIF client: %d\n", phy_if_id);

	if (NULL == handler)
	{
		NXP_LOG_ERROR("Event handler is mandatory\n");
		goto unlock_and_fail;
	}

	/*	Only single client is allowed in this mode */
	client = &hif_drv->client;
	if (FALSE != client->active)
	{
		NXP_LOG_ERROR("SC HIF driver variant allows only single client\n");
		goto unlock_and_fail;
	}

	/*	Initialize the instance */
	memset(client, 0, sizeof(pfe_hif_drv_client_t));
	client->active = FALSE;
	client->hif_drv = hif_drv;
	client->phy_if_id = phy_if_id;

	client->event_handler = handler;
	client->priv = priv;

#ifdef PFE_CFG_IEEE1588_SUPPORT
	/*	Initialize PTP timestamp database */
	if (EOK != pfe_hif_ptp_ts_db_init(&client->ptpdb))
	{
		NXP_LOG_ERROR("PTP DB init failed\n");
		goto unlock_and_fail;
	}
#endif /* PFE_CFG_IEEE1588_SUPPORT */

	/*	Suspend HIF driver to get exclusive access to client table */
	pfe_hif_drv_stop(hif_drv);

	/*	Activate the client */
	client->active = TRUE;
	return client;

unlock_and_fail:
	/*	Release the client instance */
	pfe_hif_drv_client_unregister(client);

	return NULL;
}

/**
 * @brief		Get hif_drv instance associated with the client
 * @param[in]	client Client instance
 * @return		Pointer to the HIF DRV instance
 */
pfe_hif_drv_t *pfe_hif_drv_client_get_drv(pfe_hif_drv_client_t *client)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return client->hif_drv;
}

/**
 * @brief		Get private pointer provided in registration
 * @param[in]	client Client instance
 * @return		Private pointer value
 */
void *pfe_hif_drv_client_get_priv(pfe_hif_drv_client_t *client)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return client->priv;
}

/**
 * @brief		Unregister client from the HIF driver
 * @param[in]	client Client instance
 */
void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client)
{
	if (NULL != client)
	{
		/*	Suspend HIF driver to get exclusive access to client table */
		pfe_hif_drv_stop(client->hif_drv);

		/*	Unregister from HIF. After this the HIF RX dispatcher will not fill client's RX queues. */
		client->active = FALSE;

#ifdef PFE_CFG_IEEE1588_SUPPORT
		/*	Finalize the timestamp DB */
		pfe_hif_ptp_ts_db_fini(&client->ptpdb);
#endif /* PFE_CFG_IEEE1588_SUPPORT */

		NXP_LOG_INFO("HIF client %d removed\n", client->phy_if_id);

		/*	Cleanup memory */
		memset(client, 0, sizeof(pfe_hif_drv_client_t));
	}
}

/*
 * The code adds support for IHC communication to the original SC driver.
 *
 * The following API calls has to be implemented:
 *
 *		pfe_hif_drv_ihc_client_register()
 *		pfe_hif_drv_ihc_client_unregister()
 *		pfe_hif_drv_client_receive_pkt()
 *		pfe_hif_pkt_free()
 *		pfe_hif_drv_client_xmit_ihc_sg_pkt()
 *
 */

/**
 * @brief		Transmit IHC packet given as a SG list of buffers
 * @param[in]	client Client instance
 * @param[in]	dst Destination physical interface ID. Should by HIFs only.
 * @param[in]	queue TX queue number
 * @param[in]	sg_list Pointer to the SG list
 * @param[in]	ref_ptr Reference pointer to be provided within TX confirmation.
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_ihc_sg_pkt(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t dst, uint32_t queue, hif_drv_sg_list_t *sg_list, void *ref_ptr)
{
	sg_list->dst_phy = dst;

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	sg_list->total_bytes = sg_list->items[0].len;
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	return pfe_hif_drv_client_xmit_sg_pkt(client, queue, sg_list, ref_ptr);
}

/**
 * @brief		Release packet
 * @param[in]	pkt The packet instance
 */
void pfe_hif_pkt_free(pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
	
	if (unlikely(NULL == pkt->client))
	{
		NXP_LOG_ERROR("Client is NULL\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Release SKB and associated pkt header */
	kfree_skb((struct sk_buff *)pkt->ref_ptr);

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (&client->hif_drv->ihc_client != client)
	{
		/* Only IHC client supported */
		NXP_LOG_ERROR("Only HIF IHC client supported\n");
		return NULL;
	}

	/*	No resource protection here */
	return fifo_get(client->ihc.rx_fifo);
}

errno_t pfe_hif_drv_ihc_do_cbk(pfe_hif_drv_t *hif_drv)
{
	pfe_hif_drv_client_t *client = &hif_drv->ihc_client;

	return client->event_handler(client, client->priv, EVENT_RX_PKT_IND, 0);
}

errno_t pfe_hif_drv_ihc_put_pkt(pfe_hif_drv_t *hif_drv, void *data, uint32_t len, void *ref)
{
	
	pfe_hif_drv_client_t *client = &hif_drv->ihc_client;
	pfe_ct_hif_rx_hdr_t *hif_hdr = (pfe_ct_hif_rx_hdr_t *)data;
	pfe_hif_rx_meta_t *rx_metadata;

	if (NULL == hif_drv)
		return EINVAL;

	/*	Create the RX metadata */
	rx_metadata = (pfe_hif_rx_meta_t *)oal_mm_malloc(sizeof(pfe_hif_rx_meta_t));
	if (NULL == rx_metadata)
		return ENOMEM;
	memset(rx_metadata, 0, sizeof(pfe_hif_rx_meta_t));

	rx_metadata->client = client;
	rx_metadata->data = (addr_t)data;
	rx_metadata->len = len;

	rx_metadata->flags.rx_flags = hif_hdr->flags;
	rx_metadata->i_phy_if = hif_hdr->i_phy_if;
	rx_metadata->ref_ptr = ref;

	if (unlikely(EOK != fifo_put(client->ihc.rx_fifo, rx_metadata)))
	{
		NXP_LOG_ERROR("IHC RX fifo full\n");
		/*	Drop the frame. Resource protection is embedded. */
		pfe_hif_pkt_free(rx_metadata);
		return EINVAL;
	}

	return EOK;
}

/**
 * @brief		Unregister IHC client and release all associated resources
 * @param[in]	client Client instance
 * @warning		Can only be called on HIF driver is stopped.
 */
void pfe_hif_drv_ihc_client_unregister(pfe_hif_drv_client_t *client)
{
	if (NULL != client)
	{
		/*	Release IHC fifo */
		if (client->ihc.rx_fifo)
		{
			uint32_t fill_level;
			errno_t err = fifo_get_fill_level(client->ihc.rx_fifo, &fill_level);

			if (unlikely(EOK != err))
			{
				NXP_LOG_ERROR("Unable to get IHC fifo fill level: %d\n", err);
			}
			else if (fill_level != 0U)
			{
				NXP_LOG_WARNING("IHC Queue is not empty\n");
			}

			fifo_destroy(client->ihc.rx_fifo);
			client->ihc.rx_fifo = NULL;
		}

		/*	Cleanup memory */
		memset(client, 0, sizeof(pfe_hif_drv_client_t));

		NXP_LOG_INFO("HIF IHC client removed\n");
	}
}

pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv)
{
	pfe_hif_drv_client_t *client;

	if (NULL == handler)
	{
		NXP_LOG_ERROR("Event handler is mandatory\n");
		return NULL;
	}

	/*	Initialize the instance */
	client = &hif_drv->ihc_client;

	if (FALSE != client->active)
	{
		NXP_LOG_ERROR("IHC client already registered\n");
		goto free_and_fail;
	}

	memset(client, 0, sizeof(pfe_hif_drv_client_t));

	client->ihc.rx_fifo = fifo_create(32);
	if (NULL == client->ihc.rx_fifo)
	{
		NXP_LOG_ERROR("Can't create IHC RX fifo\n");
		goto free_and_fail;
	}

	client->hif_drv = hif_drv;
	client->event_handler = handler;
	client->priv = priv;

	return client;

free_and_fail:
	pfe_hif_drv_ihc_client_unregister(client);
	return NULL;
}

/**
 * @brief		Check if there is another Rx packet in queue
 * @param[in]	client Client instance
 * @param[in]	queue RX queue number
 * @retval		TRUE There is at least one Rx packet in Rx queue
 * @retval		FALSE There is no Rx packet in Rx queue
 *
 * @warning		Intended to be called from a single client context only, i.e.
 * 				from a single thread per client.
 */
bool_t pfe_hif_drv_client_has_rx_pkt(pfe_hif_drv_client_t *client, uint32_t queue)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == client) || unlikely(NULL == client->hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Implement check when needed */
	return TRUE;
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
void * pfe_hif_drv_client_receive_tx_conf(pfe_hif_drv_client_t *client, uint32_t queue)
{
	pfe_hif_tx_meta_t *tx_metadata;
	pfe_hif_drv_t *hif_drv;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	hif_drv = client->hif_drv;

	/*	Get confirmation directly from channel. This is actually only check whether
		some next frame has been transmitted. */
	if (EOK != pfe_hif_chnl_get_tx_conf(hif_drv->channel))
	{
		/*	No more entries to dequeue */
		return NULL;
	}
	else
	{
		/*	Get metadata associated with the transmitted frame */
		tx_metadata = &hif_drv->tx_meta[hif_drv->tx_meta_rd_idx & (PFE_HIF_RING_CFG_LENGTH-1U)];

		/*	Move to next entry */
		hif_drv->tx_meta_rd_idx++;

		/*	Return the reference data */
		return tx_metadata->ref_ptr;
	}
}

/**
 * @brief		Set physical interface for TX traffic injection
 * @details		Set physical interface to be used when driver will attempt to transmit
 * 				a packet in "inject" mode.
 * @param[in]	client Client instance
 * @param[in]	phy_if_id The physical interface ID
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_hif_drv_client_set_inject_if(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t phy_if_id)
{
	if (phy_if_id >= PFE_PHY_IF_ID_INVALID)
	{
		return EINVAL;
	}

	/*	Set new physical interface */
	client->phy_if_id = phy_if_id;

	/*	NOOP: Dynamic header will be updated with every "xmit" call */

	return EOK;
}

/**
 * @brief		Transmit packet given as a SG list of buffers
 * @param[in]	client Client instance
 * @param[in]	queue TX queue number
 * @param[in]	sg_list Pointer to the SG list
 * @param[in]	ref_ptr Reference pointer to be provided within TX confirmation.
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_sg_pkt(pfe_hif_drv_client_t *client, uint32_t queue, const hif_drv_sg_list_t *const sg_list, void *ref_ptr)
{
	errno_t err;
	uint32_t ii;
	pfe_hif_tx_meta_t *tx_metadata;
	pfe_hif_drv_t *hif_drv;
	pfe_ct_hif_tx_hdr_t *tx_hdr, *tx_hdr_pa;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely((NULL == client) || (NULL == sg_list)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get HIF driver instance from client */
	hif_drv = client->hif_drv;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	err = oal_mutex_lock(&hif_drv->tx_lock);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Couldn't lock mutex (tx_lock): %d\n", err);
		goto end;
	}
#endif

	if (unlikely(FALSE == hif_drv->tx_enabled))
	{
		err = EPERM;
		goto end;
	}

	/*
		Check if we have enough TX resources. We need one for each SG entry plus
	 	one for HIF header.
	*/
	if (unlikely(FALSE == pfe_hif_chnl_can_accept_tx_num(hif_drv->channel, (sg_list->size + 1U))))
	{
		/*	Channel can't accept buffers (TX ring full?). Try to schedule
		 	TX maintenance to process potentially transmitted packets and
		 	make some space in TX ring. */
		pfe_hif_chnl_tx_dma_start(hif_drv->channel);

		err = ENOSPC;
		goto end;
	}

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	if (unlikely(FALSE == pfe_hif_chnl_can_accept_tx_data(hif_drv->channel, (sg_list->total_bytes + sizeof(pfe_ct_hif_tx_hdr_t)))))
	{
		err = ENOSPC;
		goto end;
	}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	/*
		HIF driver must keep local copy of the HW TX ring to gain access
		to virtual buffer addresses in case when data is being
		acknowledged to a client. For this purpose the SW descriptors
		are being used.
	*/

	/*	Get metadata storage */
	tx_metadata = &hif_drv->tx_meta[hif_drv->tx_meta_wr_idx & (PFE_HIF_RING_CFG_LENGTH-1U)];

	/*	Use dynamic TX header */
	tx_hdr = tx_metadata->hif_tx_header;
	tx_hdr_pa = tx_metadata->hif_tx_header_pa;

	/*	Update the header */
#ifdef PFE_CFG_HIF_PRIO_CTRL
	/*	Firmware will assign queue/priority */
	tx_hdr->queue = 255U;
#else
	tx_hdr->queue = queue;
#endif /* PFE_CFG_HIF_PRIO_CTRL */

	tx_hdr->flags = sg_list->flags.tx_flags;

	/* 	Preprocess IHC message */
	if (&client->hif_drv->ihc_client == client)
	{
		tx_hdr->flags |= HIF_TX_IHC | HIF_TX_INJECT;
		tx_hdr->e_phy_ifs = oal_htonl(1U << sg_list->dst_phy);
	}
	else
	{
#ifdef PFE_CFG_ROUTE_HIF_TRAFFIC
		/*	Tag the frame with ID of target physical interface */
		tx_hdr->cookie = oal_htonl(client->phy_if_id);
#else
		tx_hdr->flags |= HIF_TX_INJECT;
		tx_hdr->e_phy_ifs = oal_htonl(1U << client->phy_if_id);
#endif /* PFE_CFG_ROUTE_HIF_TRAFFIC */
	}

#ifdef PFE_CFG_CSUM_ALL_FRAMES
	tx_hdr->flags |= HIF_TX_IP_CSUM | HIF_TX_TCP_CSUM | HIF_TX_UDP_CSUM;
#endif /* PFE_CFG_CSUM_ALL_FRAMES */

#ifdef PFE_CFG_IEEE1588_SUPPORT
	/*	Check if frame is a PTP message and need timestamp */
	oal_util_ptp_header_t *ptph;
	if (EOK == oal_util_parse_ptp(sg_list->items[0].data_va, sg_list->items[0].len, &ptph))
	{
		/*	Request TS */
		tx_hdr->refnum = oal_util_get_unique_seqnum32() & 0xffff; /* Don't switch endian */
		tx_hdr->flags |= HIF_TX_ETS;

		/*	Store the TX frame to DB */
		err = pfe_hif_ptp_ts_db_push_msg(&client->ptpdb, FALSE, tx_hdr->refnum, ptph->messageType,
				oal_ntohs(ptph->sourcePortID), oal_ntohs(ptph->sequenceID));
		if (EOK != err)
		{
			NXP_LOG_ERROR("Could not store PTP message: %d\n", err);
			tx_hdr->flags &= ~HIF_TX_ETS;
		}
		else
		{
#ifdef PFE_CFG_DEBUG
			NXP_LOG_DEBUG("New (TX) PTP frame: Type: 0x%x, Port: 0x%x, SeqID: 0x%x\n",
					ptph->messageType, oal_ntohs(ptph->sourcePortID), oal_ntohs(ptph->sequenceID));
#endif /* PFE_CFG_DEBUG */
		}
	}
#endif /* PFE_CFG_IEEE1588_SUPPORT */

	/*	Enqueue the HIF packet header */
	err = pfe_hif_chnl_tx(	hif_drv->channel,
							(void *)tx_hdr_pa,
							(void *)tx_hdr,
							sizeof(pfe_ct_hif_tx_hdr_t),
							FALSE);

	if (unlikely(EOK != err))
	{
		/*	Channel did not accept the buffer */
		NXP_LOG_ERROR("Channel did not accept buffer: %d\n", err);
		err = ECANCELED;
		goto end;
	}

	/*	Store the frame metadata */
	/* tx_metadata->q_no = queue; */
	tx_metadata->ref_ptr = ref_ptr;

	/*	Move to next entry */
	hif_drv->tx_meta_wr_idx++;

	/*	Transmit particular packet buffers */
	for (ii=0U; ii<sg_list->size; ii++)
	{
		/*	Transmit the buffer */
		err = pfe_hif_chnl_tx(	client->hif_drv->channel,
								sg_list->items[ii].data_pa,
								sg_list->items[ii].data_va,
								sg_list->items[ii].len,
								((ii+1) >= sg_list->size));

		if (unlikely(EOK != err))
		{
			/*	TODO: We need somehow reset the TX BD Ring because HIF header has already been enqueued. */
			NXP_LOG_ERROR("Fatal error, TX channel will get stuck...\n");

			/*	Undo move to next entry */
			hif_drv->tx_meta_wr_idx--;
			err = ECANCELED;
			goto end;
		}
	}

end:
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (EOK != oal_mutex_unlock(&hif_drv->tx_lock))
	{
		NXP_LOG_ERROR("Couldn't unlock mutex (tx_lock)\n");
	}
#endif

	return err;
}

/**
 * @brief		Transmit a single-buffer packet
 * @param[in]	client Client instance
 * @param[in]	queue TX queue number
 * @param[in]	data_pa Physical address of buffer to be sent
 * @param[in]	data_va Virtual address of buffer to be sent
 * @param[in]	len Length of the buffer
 * @param[in]	ref_ptr Reference pointer to be provided within TX confirmation.
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_pkt(pfe_hif_drv_client_t *client, uint32_t queue, void *data_pa, void *data_va, uint32_t len, void *ref_ptr)
{
	hif_drv_sg_list_t sg_list;

	sg_list.size = 1;

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	sg_list.total_bytes = len;
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	sg_list.flags.common = (pfe_hif_drv_common_flags_t)0U;
	sg_list.flags.tx_flags = (pfe_ct_hif_tx_flags_t)0U;
	sg_list.items[0].data_pa = data_pa;
	sg_list.items[0].data_va = data_va;
	sg_list.items[0].len = len;

	return pfe_hif_drv_client_xmit_sg_pkt(client, queue, &sg_list, ref_ptr);
}

/**
 * @brief		Get PTP timestamp
 * @details		Function will return timestamp for PTP message given by set arguments
 * 				if such timestamp has been captured
 * @param[in]	client The client instance
 * @param[in]	rx TRUE means to get ingress TS, FALSE means egress
 * @param[in]	type PTP message type
 * @param[in]	port PTP source port ID
 * @param[in]	seq_id PTP sequence ID
 * @param[out]	ts_sec Seconds part of the timestamp
 * @param[out]	ts_nsec Nanoseconds part of the timestamp
 * @retval		EOK Timestamp has been found and is valid
 * @retval		ENOENT Timestamp matching given criteria not found
 */
errno_t pfe_hif_drv_client_get_ts(pfe_hif_drv_client_t *client, bool_t rx,
		uint8_t type, uint16_t port, uint16_t seq_id, uint32_t *ts_sec, uint32_t *ts_nsec)
{
#ifdef PFE_CFG_IEEE1588_SUPPORT
	return pfe_hif_ptp_ts_db_pop(&client->ptpdb, type, port, seq_id, ts_sec, ts_nsec, rx);
#else
	NXP_LOG_ERROR("PTP support not enabled\n");
	return EINVAL;
#endif /* */
}

/**
 * @brief		Create new HIF driver instance
 * @param[in]	channel The HIF channel instance to be managed
 */
pfe_hif_drv_t *pfe_hif_drv_create(pfe_hif_chnl_t *channel)
{
	pfe_hif_drv_t *hif_drv;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == channel))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	hif_drv = oal_mm_malloc(sizeof(pfe_hif_drv_t));

	if (NULL == hif_drv)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return NULL;
	}
	else
	{
		memset(hif_drv, 0, sizeof(pfe_hif_drv_t));
		hif_drv->channel = channel;

		return hif_drv;
	}
}

/**
 * @brief 	HIF initialization routine
 * @details	Function performs following initialization:
 * 			- Initializes HIF interrupt handler(s)
 * 			- Performs HIF HW initialization and enables RX/TX DMA
 */
errno_t pfe_hif_drv_init(pfe_hif_drv_t *hif_drv)
{
	errno_t err;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (hif_drv->initialized)
	{
		NXP_LOG_ERROR("HIF already initialized. Exiting.\n");
		return ECANCELED;
	}

	/*	Initialize RX/TX resources */
	hif_drv->started = FALSE;

	err = pfe_hif_drv_create_data_channel(hif_drv);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Could not initialize data channel: %d\n", err);
		return err;
	}

	/*	Attach channel RX ISR */
	err = pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_IRQ, &pfe_hif_drv_chnl_rx_isr, (void *)hif_drv);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Could not register RX ISR: %d\n", err);
		pfe_hif_drv_destroy_data_channel(hif_drv);
		return err;
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	err = oal_mutex_init(&hif_drv->tx_lock);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Couldn't init mutex (tx_lock): %d\n", err);
		return err;
	}
#endif

	hif_drv->rx_enabled = FALSE;
	hif_drv->tx_enabled = FALSE;
	hif_drv->initialized = TRUE;

	return EOK;
}

/**
 * @brief		Start traffic at HIF level
 * @details		Data transmission/reception is enabled
 * @param[in]	hif_drv The driver instance
 */
errno_t pfe_hif_drv_start(pfe_hif_drv_t *hif_drv)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (FALSE == hif_drv->initialized)
	{
		NXP_LOG_ERROR("HIF driver not initialized\n");
		return ENODEV;
	}

	/*	Enable channel interrupt */
	pfe_hif_chnl_irq_unmask(hif_drv->channel);

	/*	Enable RX */
	if (EOK != pfe_hif_chnl_rx_enable(hif_drv->channel))
	{
		NXP_LOG_ERROR("Couldn't enable RX\n");
	}
	else
	{
		hif_drv->rx_enabled = TRUE;
	}

	/*	Enable TX */
	if (EOK != pfe_hif_chnl_tx_enable(hif_drv->channel))
	{
		NXP_LOG_ERROR("Couldn't enable TX\n");
	}
	else
	{
		hif_drv->tx_enabled = TRUE;
	}

	/*	Enable the channel RX interrupts */
	pfe_hif_chnl_rx_irq_unmask(hif_drv->channel);

	NXP_LOG_INFO("HIF driver started\n");

	return EOK;
}

/**
 * @brief		Stop traffic at HIF level
 * @details		No resource releasing is done here. This call
 * 				only ensures that all traffic is suppressed at
 * 				the HIF channel level so HIF driver is not receiving
 * 				any notifications about data transfers (RX/TX) and
 * 				is not accessing any RX/TX resources.
 * @param[in]	hif_drv The driver instance
 */
void pfe_hif_drv_stop(pfe_hif_drv_t *hif_drv)
{
	uint32_t hif_stop_timeout;

#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Stop RX */
	if (TRUE == hif_drv->rx_enabled)
	{
		NXP_LOG_DEBUG("Disabling channel RX path\n");
		pfe_hif_chnl_rx_disable(hif_drv->channel);

		hif_stop_timeout = 10;
		do
		{
			if (pfe_hif_chnl_is_rx_dma_active(hif_drv->channel))
			{
				oal_time_usleep(250);
			}
			else
			{
				break;
			}
		} while (0 != hif_stop_timeout--);

		if (pfe_hif_chnl_is_rx_dma_active(hif_drv->channel))
		{
			NXP_LOG_WARNING("Unable to stop the HIF RX DMA\n");
		}

		/*	Disallow reception and ensure the change has been applied */
		hif_drv->rx_enabled = FALSE;

		NXP_LOG_DEBUG("Disabling channel RX IRQ\n");
		pfe_hif_chnl_rx_irq_mask(hif_drv->channel);
	}

	/*	Stop TX */
	if (TRUE == hif_drv->tx_enabled)
	{
		NXP_LOG_DEBUG("Disabling channel TX path\n");
		pfe_hif_chnl_tx_disable(hif_drv->channel);

		hif_stop_timeout = 10;
		do
		{
			if (pfe_hif_chnl_is_tx_dma_active(hif_drv->channel))
			{
				oal_time_usleep(250);
			}
			else
			{
				break;
			}
		} while (0 != hif_stop_timeout--);

		if (pfe_hif_chnl_is_tx_dma_active(hif_drv->channel))
		{
			NXP_LOG_WARNING("Unable to stop the HIF TX DMA\n");
		}

		/*	Disallow transmission and ensure the change has been applied */
		hif_drv->tx_enabled = FALSE;

		NXP_LOG_INFO("HIF driver TX path is stopped\n");
	}

	/*
	 *	-----------------------------------------------------
	 *	Now the RX and TX resource of HIF channel are frozen
	 *	-----------------------------------------------------
	 */

	/*	Disable channel interrupt */
	pfe_hif_chnl_irq_mask(hif_drv->channel);
}

/**
 * @brief		Exit the HIF driver
 * @details		Terminate the HIF driver and release all allocated
 * 				resources.
 * @param[in]	hif_drv The driver instance
 */
void pfe_hif_drv_exit(pfe_hif_drv_t *hif_drv)
{
#ifdef PFE_CFG_NULL_ARG_CHECK
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (FALSE == hif_drv->initialized)
	{
		NXP_LOG_WARNING("HIF is already disabled\n");
		return;
	}

	/*	Check if a client is still registered */
	if (TRUE == hif_drv->client.active)
	{
		NXP_LOG_ERROR("Client is still active\n");
	}

	/*	Stop the traffic */
	pfe_hif_drv_stop(hif_drv);

	/*	Just a sanity check */
	if (hif_drv->tx_meta_rd_idx != hif_drv->tx_meta_wr_idx)
	{
		NXP_LOG_WARNING("TX confirmation FIFO still contains %d entries\n", hif_drv->tx_meta_wr_idx - hif_drv->tx_meta_rd_idx);
	}
	else
	{
		NXP_LOG_INFO("TX confirmation FIFO is empty\n");
	}

	/*	Detach event handlers */
	(void)pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_IRQ, NULL, NULL);

	/*	Release HIF channel and buffers */
	pfe_hif_drv_destroy_data_channel(hif_drv);
	hif_drv->initialized = FALSE;
	NXP_LOG_INFO("HIF SC exited\n");
}

void pfe_hif_drv_destroy(pfe_hif_drv_t *hif_drv)
{
	if (NULL == hif_drv)
	{
		return;
	}
	else
	{
		pfe_hif_drv_exit(hif_drv);
		oal_mm_free(hif_drv);
		hif_drv = NULL;
	}
}

/** @}*/
