/* =========================================================================
 *  Copyright 2018-2019 NXP
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
 * @brief		The HIF driver source file.
 * @details		This is the HIF driver with following features:
 * 					- Server-Client approach and traffic dispatching. The driver provides
 * 					  possibility to register a client which will receive dedicated traffic
 * 					  according to client ID assigned to a packet by the classification process
 * 					  (firmware).
 * 					- TX confirmation handling. Driver passes the TX confirmation events
 * 					  to particular clients once their transmit requests are processed.
 * 					- HIF interrupts handling.
 *
 * @internal
 *
 * Threading model
 * ---------------
 * There are two types of threads involved:
 *
 * 	- Client
 * 	  An external thread running HIF client-related routines. Each HIF client is intended to
 * 	  run within its own thread. Creation and maintenance of client's threads is not subject
 * 	  of this HIF driver. The HIF driver can only notify the client using dedicated event
 * 	  notification mechanism.
 *
 * Resources protection
 * --------------------
 * The HIF driver is using a set of various resources which are being accessed from multiple
 * thread contexts. Here is the list with synchronization information:
 *
 *	- pfe_hif_drv_t.tx_meta
 *	  Producer  : Clients
 *	  Consumer  : HIF worker
 *	  Protection: pfe_hif_drv_t.tx_lock
 *
 *	  The common, HIF-owned TX metadata storage. Every transmitted buffer (enqueued to the HW TX
 *	  ring) has associated metadata structure within this table. The order of transmitted buffers
 *	  and metadata entries is maintained. HIF clients are writing to this table within the xmit
 *    calls. HIF worker is then reading the entries during TX confirmation processing. The table
 *    is thus protected using the pfe_hif_drv_t.tx_lock.
 *
 *	- pfe_hif_drv_client_t.client_tx_queue.tx_conf_fifo
 *	  Producer  : HIF worker
 *	  Consumer  : Particular Client
 *	  Protection: n/a
 *
 *	  Client-owned FIFO for TX confirmations. HIF worker is putting data into client's FIFO.
 *	  Client is reading the FIFO during TX confirmation processing. This FIFO does not need
 *	  resource protections (single producer/single consumer).
 *
 *	- pfe_hif_drv_client_t.client_rx_queue.rx_fifo
 *	  Producer  : HIF worker
 *	  Consumer  : Particular Client
 *	  Protection: n/a
 *
 *	  Client-owned FIFO for RX buffers. HIF worker is putting descriptors into client's RX FIFO.
 *	  Client is reading the FIFO during RX processing. The FIFO does not need resource protection
 *	  (single producer/single consumer).
 *
 *
 * @endinternal
 *
 */

#include <string.h>

#include "oal.h"
#include "hal.h"
#include "fifo.h"
#include "pfe_hif.h"
#include "pfe_hif_drv.h"
#include "pfe_platform_cfg.h"

#if !defined(TARGET_HW_LS1012A) && !defined(TARGET_HW_S32G)
#define TARGET_HW_S32G /* TODO: DUE TO ECLIPSE ENVIRONMENT ONLY (to see active code) */
#endif

#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
/*	When there is no need to modify HIF TX header with every TX frame then only
	single static HIF TX header instance (client-owned) will be created and used for
	each transmission. When HIF TX header modification is needed to be done with
	every transmitted frame then multiple HIF TX headers are needed and therefore
	they will be allocated within dedicated storage. */
	#define HIF_CFG_USE_DYNAMIC_TX_HEADERS
#endif /* GLOBAL_CFG_MULTI_INSTANCE_SUPPORT */
#ifndef GLOBAL_CFG_CSUM_ALL_FRAMES
/*	Enable dynamic tx headers for individual CSUM 
	(on demand) calculation if it is not already enabled */
	#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
		#define HIF_CFG_USE_DYNAMIC_TX_HEADERS
	#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
#endif /* GLOBAL_CFG_CSUM_ALL_FRAMES */

/**
 * @def	    HIF_CFG_DETACH_TX_CONFIRMATION_JOB
 * @brief	If TRUE the TX confirmation procedure will be executed within deferred job.
 * 			If FALSE the TX confirmation will be executed with every pfe_hif_drv_client_xmit call.
 */
#ifdef TARGET_OS_AUTOSAR
    #define HIF_CFG_DETACH_TX_CONFIRMATION_JOB		TRUE
#else
    #define HIF_CFG_DETACH_TX_CONFIRMATION_JOB		FALSE
#endif    


/**
 * @def	    HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION
 * @brief	If TRUE then TX confirmation job will be triggered as response to
 * 			TX interrupt/event. If FALSE the TX confirmation job will be triggered
 * 			from within the pfe_hif_drv_client_xmit call.
 */
#ifdef TARGET_OS_AUTOSAR
    #define	HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION	TRUE
#else
    #define	HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION	FALSE
#endif

/**
 * @brief	Maximum number of HIF clients. Right now it is set to cover all possible
 * 			logical interfaces.
 */
#define HIF_CLIENTS_MAX							PFE_CFG_MAX_LOG_IFS

#if ((FALSE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB) && (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION))
#error Impossible configuration
#endif

#define HIF_CFG_WORKER_SCHEDULE_RX				(100U)
#define HIF_CFG_WORKER_SCHEDULE_TX_MAINTENANCE	(101U)
#define HIF_CFG_WORKER_SHUTDOWN					(102U)

#define PFE_BUF_SIZE		2048U	/* must be big enough for headroom, pkt size and skb shared info */
#define PFE_PKT_HEADROOM	128U
#define PFE_MIN_PKT_SIZE	64U
#define PFE_PKT_SIZE		(PFE_BUF_SIZE - PFE_PKT_HEADROOM) /* maximum ethernet packet */

struct __attribute__((packed)) __pfe_hif_pkt_tag
{
	/*	When every transmitted frame needs to contain customized HIF TX header then
	 	multiple HIF TX header instances are needed. For this purpose the TX metadata
	 	storage is used. */
#ifdef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	pfe_ct_hif_tx_hdr_t *hif_tx_header;
	void *hif_tx_header_pa;
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
	pfe_hif_drv_client_t **client;
	addr_t data;
	uint16_t len;
	uint8_t q_no;
	pfe_hif_drv_flags_t flags;
	pfe_ct_phy_if_id_t i_phy_if;
	void *ref_ptr; /* Reference pointer (keep the original mbuf pointer here) */
};

typedef struct __pfe_hif_pkt_tag pfe_hif_tx_meta_t;
typedef struct __pfe_hif_pkt_tag pfe_hif_rx_meta_t;

struct client_rx_queue
{
	fifo_t *rx_fifo;	/* This is the client's RX ring */
	uint32_t size;
	bool_t has_new_data;
};

struct client_tx_queue
{
	fifo_t *tx_conf_fifo; /* TX confirmation FIFO */
	uint32_t size;
	bool_t has_new_data;
};

/**
 * @brief	The HIF driver client instance structure
 */
struct __attribute__((aligned(HAL_CACHE_LINE_SIZE), packed)) __pfe_pfe_hif_drv_client_tag
{
	pfe_phy_if_t *phy_if;
	pfe_log_if_t *log_if;
	uint32_t	tx_qn;
	uint32_t	rx_qn;

	struct client_tx_queue tx_q[HIF_DRV_CLIENT_QUEUES_MAX];
	struct client_rx_queue rx_q[HIF_DRV_CLIENT_QUEUES_MAX];
	pfe_hif_drv_client_event_handler event_handler;
	pfe_hif_drv_t *hif_drv;
	void *priv;

#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	pfe_ct_hif_tx_hdr_t *hif_tx_header;	/*	Storage for the HIF header */
	void *hif_tx_header_pa;
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
};

/**
 * @brief	The HIF driver instance structure
 */
struct __attribute__((aligned(HAL_CACHE_LINE_SIZE), packed)) __pfe_hif_drv_tag
{
/*	Common */
	pfe_hif_chnl_t *channel;			/*	The associated HIF channel instance */

/*	HIF RX processing */
	oal_job_t *rx_job;
	pfe_hif_drv_client_t **cur_client;
	pfe_ct_phy_if_id_t i_phy_if;
	uint8_t qno;
	bool_t started;
	bool_t rx_enabled;					/*	If TRUE then frame reception is allowed */

/*	TX and TX confirmation processing */
#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
	oal_job_t *tx_job;
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */
	oal_mutex_t tx_lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));	/*	TX resources protection object */
	pfe_hif_tx_meta_t *tx_meta;			/*	Storage of metadata for every transmitted buffer */
	uint32_t tx_meta_rd_idx;
	uint32_t tx_meta_wr_idx;
	uint32_t tx_meta_idx_mask;
	bool_t tx_enabled;					/*	If TRUE then frame transmission is allowed */

#ifdef HIF_STATS
/*	Statistics */
	uint32_t counters[HIF_STATS_MAX_COUNT];
#endif

/*	Table of HIF Driver Clients indexed by logical interface (pfe_log_if_t) ID */
	pfe_hif_drv_client_t *clients[HIF_CLIENTS_MAX] __attribute__((aligned(HAL_CACHE_LINE_SIZE)));
/*	Special client to be used for HIF-to-HIF communication */
	pfe_hif_drv_client_t *ihc_client;

	volatile bool_t initialized;	/*	If TRUE the HIF has been properly initialized */
	oal_mutex_t cl_api_lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));	/*	DRV client API (reg/unreg) mutex */
};

#define PFE_HIF_DRV_WORKER_IRQ_NAME "pfe:q%d"

/*	Channel management */
static errno_t pfe_hif_drv_create_data_channel(pfe_hif_drv_t *hif_drv);
static void pfe_hif_drv_destroy_data_channel(pfe_hif_drv_t *hif_drv);

/*	Common static stuff */
static uint32_t pfe_hif_drv_process_rx(pfe_hif_drv_t *hif_drv, uint32_t budget);
static uint32_t pfe_hif_drv_process_tx(pfe_hif_drv_t *hif_drv, uint32_t budget);
static void hif_client_free_rx_queues(pfe_hif_drv_client_t *client);
static void hif_client_free_tx_queues(pfe_hif_drv_client_t *client);

/**
 * @brief		HIF channel RX ISR
 * @details		Will be called by HIF channel instance when RX event has occurred
 * @note		To see which context the ISR is running in please see the
 * 				pfe_hif_chnl module implementation.
 */
static void pfe_hif_drv_chnl_rx_isr(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;

	if (unlikely(EOK != oal_job_run(hif_drv->rx_job)))
	{
		NXP_LOG_ERROR("RX job trigger failed\n");
	}
}

/**
 * @brief		Deferred RX job
 */
static void pfe_hif_drv_rx_job(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;

	if (likely(TRUE == hif_drv->rx_enabled))
	{
		while (HIF_RX_POLL_BUDGET <= pfe_hif_drv_process_rx(hif_drv, HIF_RX_POLL_BUDGET))
		{
			;
		}

		/*	Enable RX interrupt */
		pfe_hif_chnl_rx_irq_unmask(hif_drv->channel);

		/*	Trigger the RX DMA */
		pfe_hif_chnl_rx_dma_start(hif_drv->channel);
	}
}

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
/**
 * @brief		HIF channel TX ISR
 * @details		Will be called by HIF channel instance when TX event has occurred
 * @note		To see which context the ISR is running in please see the
 * 				pfe_hif_chnl module implementation.
 */
static void pfe_hif_drv_chnl_tx_isr(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;

	if (unlikely(EOK != oal_job_run(hif_drv->tx_job)))
	{
		NXP_LOG_ERROR("TX job trigger failed\n");
	}
}
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */

/**
 * @brief		Deferred TX job
 */
static void pfe_hif_drv_tx_job(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;

	if (likely(TRUE == hif_drv->tx_enabled))
	{
		/*	Enter critical section */
		if (unlikely(EOK != oal_mutex_lock(&hif_drv->tx_lock)))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		while (HIF_TX_POLL_BUDGET <= pfe_hif_drv_process_tx(hif_drv, HIF_TX_POLL_BUDGET))
		{
			;
		}

		/*	Leave critical section */
		if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
		/*	Enable TX interrupt */
		pfe_hif_chnl_tx_irq_unmask(hif_drv->channel);
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */

		/*	Trigger the TX DMA */
		pfe_hif_chnl_tx_dma_start(hif_drv->channel);
	}
}
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

/**
 * @brief		HIF channel RX out-of-buffers event handler
 * @details		Will be called by HIF channel instance when there are no RX
 * 				buffers available to receive data.
 */
static void pfe_hif_drv_chnl_rx_ooo_handler(void *arg)
{
	pfe_hif_drv_t *hif_drv = (pfe_hif_drv_t *)arg;
	pfe_hif_drv_client_t *client;
	uint32_t ii;

	/*	Notify all registered clients that channel is out of RX buffers. The
	 	clients can try to release previously received buffers via pfe_hif_pkt_free()
	 	or potentially deliver new RX buffers via dedicated API. */

	/*	Prevent concurrent API access */
	if (EOK != oal_mutex_lock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	for (ii=0U; ii<HIF_CLIENTS_MAX; ii++)
	{
		if (NULL != hif_drv->clients[ii])
		{
			client = hif_drv->clients[ii];
			(void)client->event_handler(client, client->priv, EVENT_RX_OOB, 0xffffffffU);
		}
	}

	if (EOK != oal_mutex_unlock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
}

static errno_t pfe_hif_drv_create_data_channel(pfe_hif_drv_t *hif_drv)
{
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Sanity check */
	if (sizeof(pfe_hif_rx_meta_t) > pfe_hif_chnl_get_meta_size(hif_drv->channel))
	{
		NXP_LOG_ERROR("Metadata storage size (%d) is less than required (%d)\n", pfe_hif_chnl_get_meta_size(hif_drv->channel), (uint32_t)sizeof(pfe_hif_rx_meta_t));
		ret = ENOMEM;
		goto destroy_and_fail;
	}

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
	hif_drv->tx_meta_idx_mask =  pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel) - 1U;

#ifdef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	/*	Allocate HIF TX headers. Allocate smaller chunks to reduce memory segmentation. */
	{
		uint32_t ii;

		for (ii=0U; ii<pfe_hif_chnl_get_tx_fifo_depth(hif_drv->channel); ii++)
		{
			hif_drv->tx_meta[ii].hif_tx_header = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_ct_hif_tx_hdr_t), 8U);
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
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Disable and invalidate RX and TX */
	pfe_hif_chnl_rx_disable(hif_drv->channel);
	pfe_hif_chnl_tx_disable(hif_drv->channel);

#ifdef HIF_CFG_USE_DYNAMIC_TX_HEADERS
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
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

	/*	Release the TX metadata storage */
	if (NULL != hif_drv->tx_meta)
	{
		oal_mm_free(hif_drv->tx_meta);
		hif_drv->tx_meta = NULL;
	}

	return;
}

/**
 * @brief		The HIF RX ring processing routine
 * @param[in]	hif_drv The HIF driver instance
 * @param[in]	budget Maximum number of frames to process in a single iteration
 * @note		Runs within the RX worker thread context
 * @return		Number of processed frames.
 */
static uint32_t pfe_hif_drv_process_rx(pfe_hif_drv_t *hif_drv, uint32_t budget)
{
	pfe_ct_hif_rx_hdr_t *hif_hdr_ptr;
	uint32_t rx_len, rx_processed = 0U;
	uint32_t flags, ii, jj;
	void *current_buffer_va, *meta_va;
	bool_t lifm;
	pfe_hif_drv_client_t *client;
	pfe_hif_rx_meta_t *rx_metadata;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	while (rx_processed < budget)
	{
		/*	Get RX buffer */
		if (EOK != pfe_hif_chnl_rx_va(hif_drv->channel, &current_buffer_va, &rx_len, &lifm, &meta_va))
		{
			break;
		}

		rx_processed++;
		hif_hdr_ptr = (pfe_ct_hif_rx_hdr_t *)current_buffer_va;

		if (FALSE == hif_drv->started)
		{
			/*
			 	 This is leading buffer of a frame. Only the leading one
			  	 contains HIF header data so get it (COPY!) and store it.

			  	 To maximize resource utilization the HIF header is later
			  	 used to store buffer-related metadata. DO NOT ACCESS this
			  	 region after metadata has been written.
			 */
			hif_hdr_ptr->flags = (pfe_ct_hif_rx_flags_t)oal_ntohs(hif_hdr_ptr->flags);
			hif_drv->i_phy_if = hif_hdr_ptr->i_phy_if;


#if defined(TARGET_HW_S32G)
			/*	Get HIF driver client which shall receive the frame */
			if (hif_hdr_ptr->flags & HIF_RX_TS)
			{
				/*	Drop the frame. Resource protection is embedded. */
				NXP_LOG_INFO("Time-stamp report received: TODO: IMPLEMENT ME\n");
				ret = pfe_hif_chnl_release_buf(hif_drv->channel, current_buffer_va);
				if (unlikely(EOK != ret))
				{
					NXP_LOG_ERROR("Unable to release RX buffer\n");
				}
#ifdef HIF_STATS
				hif_drv->counters[HIF_STATS_RX_FRAME_DROPS]++;
#endif /* HIF_STATS */
			}
			else if (hif_hdr_ptr->flags & HIF_RX_IHC)
			{
				/*	IHC client */
				hif_drv->cur_client = &hif_drv->ihc_client;
			}
			else if (unlikely(hif_hdr_ptr->i_log_if >= HIF_CLIENTS_MAX))
			{
				/*	Drop the frame. Resource protection is embedded. */
				NXP_LOG_WARNING("Invalid logical interface ID received: %d\n", hif_hdr_ptr->i_log_if);
				ret = pfe_hif_chnl_release_buf(hif_drv->channel, current_buffer_va);
				if (unlikely(EOK != ret))
				{
					NXP_LOG_ERROR("Unable to release RX buffer\n");
				}
#ifdef HIF_STATS
				hif_drv->counters[HIF_STATS_RX_FRAME_DROPS]++;
#endif /* HIF_STATS */

				continue;
			}
			else
			{
				/*	Get client associated with ingress logical interface ID */
				hif_drv->cur_client = &hif_drv->clients[hif_hdr_ptr->i_log_if];
			}

			/*	There is no queue information within the header (yet) */
			hif_drv->qno = 0U;

			/*	This is valid leading buffer of a frame */
			hif_drv->started = TRUE;
			flags = HIF_FIRST_BUFFER;
#endif /* TARGET_HW_S32G */

#if defined(TARGET_HW_LS1012A)
			/*
			 *	Here is the information which client's queue shall accept the packet. Current
			 *	meaning of the RX queues is following:
			 *		Queue 0: High priority processing queue
			 *		Queue 1: Low priority processing queue
			 *		Queue 2: LRO processing queue
			 */
			hif_drv->qno = hif_hdr_ptr->qNo;

			/*
			 *	Here the firmware tells, which client shall receive the packet.
			 *	Currently supported are (LS1012):
			 *		- GEM0
			 *		- GEM1
			 *		- WIFI0..WIFI2 (only if CFG_WIFI_OFFLOAD is defined)
			 *		- PCAP
			 */
			hif_drv->client_id = hif_hdr_ptr->client_id;

			/*	Get the client control data provided by PFE firmware */
			hif_drv->client_ctrl = (hif_hdr_ptr->client_ctrl1 << 16) | hif_hdr_ptr->client_ctrl;
#endif /* TARGET_HW_LS1012A */
		}
		else
		{
			flags = 0U;
		}

		if (lifm)
		{
			/*	This is last buffer of a frame */
			flags |= HIF_LAST_BUFFER;
			hif_drv->started = FALSE;
		}

		/*	Get current client instance */
		client = *hif_drv->cur_client;

		/*	Check if the client still exists within the dispatch table */
		if (unlikely(NULL == client))
		{
			/*	Drop the frame. Resource protection is embedded. */
			NXP_LOG_WARNING("Invalid client, dropping packet\n");
			ret = pfe_hif_chnl_release_buf(hif_drv->channel, current_buffer_va);
			if (unlikely(EOK != ret))
			{
				NXP_LOG_ERROR("Unable to release RX buffer\n");
			}
#ifdef HIF_STATS
			hif_drv->counters[HIF_STATS_RX_FRAME_DROPS]++;
#endif /* HIF_STATS */
			continue;
		}

		if (unlikely(hif_drv->qno >= client->rx_qn))
		{
			/*	Drop the frame. Resource protection is embedded. */
			NXP_LOG_WARNING("Packet with invalid queue ID: %d\n", hif_drv->qno);
			ret = pfe_hif_chnl_release_buf(hif_drv->channel, current_buffer_va);
			{
				if (unlikely(EOK != ret))
				{
					NXP_LOG_ERROR("Unable to release RX buffer\n");
				}
			}
#ifdef HIF_STATS
			hif_drv->counters[HIF_STATS_RX_FRAME_DROPS]++;
#endif /* HIF_STATS */
			continue;
		}

		/*	Fill the RX metadata */
		rx_metadata = (pfe_hif_rx_meta_t *)meta_va;
		rx_metadata->client = hif_drv->cur_client;
		rx_metadata->data = (addr_t)current_buffer_va;
		rx_metadata->len = rx_len;
		rx_metadata->flags.common = (pfe_hif_drv_common_flags_t)flags;
		if (hif_hdr_ptr != NULL)
		{
			rx_metadata->flags.rx_flags = hif_hdr_ptr->flags;
		}
		rx_metadata->q_no = hif_drv->qno;
		rx_metadata->i_phy_if = hif_drv->i_phy_if;

		/*	Enqueue the packet into client's RX queue. No resource protection here. */
		if (unlikely(EOK != fifo_put(client->rx_q[hif_drv->qno].rx_fifo, rx_metadata)))
		{
			/*	Drop the frame. Resource protection is embedded. */
			ret = pfe_hif_chnl_release_buf(hif_drv->channel, current_buffer_va);
			{
				if (unlikely(EOK != ret))
				{
					NXP_LOG_ERROR("Unable to release RX buffer\n");
				}
			}

			if (client == hif_drv->ihc_client)
			{
				/*	The client is IHC client */
				NXP_LOG_WARNING("IHC client's RX queue is full. Frame dropped.\n");
			}
			else
			{
				/*	The client is logical interface client */
				NXP_LOG_WARNING("Client's (%s) RX queue is full. Frame dropped.\n",	pfe_log_if_get_name(client->log_if));
			}
#ifdef HIF_STATS
			hif_drv->counters[HIF_STATS_CLIENT_FULL_COUNT]++;
#endif /* HIF_STATS */
			continue;
		}
		else
		{
			/*	Remember that client has a new data */
			client->rx_q[hif_drv->qno].has_new_data = TRUE;
		}
	} /* end while */

	/*	Notify client(s) about new data */
	for (ii=0U; ii<HIF_CLIENTS_MAX; ii++)
	{
		client = hif_drv->clients[ii];

		if (NULL == client)
		{
			continue;
		}

		for (jj=0U; jj<HIF_DRV_CLIENT_QUEUES_MAX; jj++)
		{
			if (TRUE == client->rx_q[jj].has_new_data)
			{
				/*	Here the client 'ii' is informed about new data in queue 'jj' */
				(void)client->event_handler(client, client->priv, EVENT_RX_PKT_IND, jj);
				client->rx_q[jj].has_new_data = FALSE;
			}
		}
	}

	/*	Notify IHC client */
	client = hif_drv->ihc_client;
	if (NULL != client)
	{
		for (jj=0U; jj<HIF_DRV_CLIENT_QUEUES_MAX; jj++)
		{
			if (TRUE == client->rx_q[jj].has_new_data)
			{
				/*	Here the IHC client is informed about new data in queue 'jj' */
				(void)client->event_handler(client, client->priv, EVENT_RX_PKT_IND, jj);
				client->rx_q[jj].has_new_data = FALSE;
			}
		}
	}

	return rx_processed;
}

static void hif_client_free_rx_queues(pfe_hif_drv_client_t *client)
{
	uint32_t ii;
	uint32_t fill_level;
	struct client_rx_queue *queue;
	errno_t err;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	for (ii=0U; ii<client->rx_qn; ii++)
	{
		queue = &client->rx_q[ii];
		if (likely(NULL != queue->rx_fifo))
		{
			err = fifo_get_fill_level(queue->rx_fifo, &fill_level);

			if (unlikely(EOK != err))
			{
				NXP_LOG_ERROR("Unable to get fifo fill level: %d\n", err);
			}

			if (fill_level != 0U)
			{
				NXP_LOG_WARNING("Client %s, RX queue %d: Queue is not empty\n", pfe_log_if_get_name(client->log_if), ii);
			}

			fifo_destroy(queue->rx_fifo);
			queue->rx_fifo = NULL;
		}
	}
}

static void hif_client_free_tx_queues(pfe_hif_drv_client_t *client)
{
	uint32_t ii;
	uint32_t fill_level;
	struct client_tx_queue *queue;
	errno_t err;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	for (ii=0U; ii<client->tx_qn; ii++)
	{
		queue = &client->tx_q[ii];
		if (likely(NULL != queue->tx_conf_fifo))
		{
			err = fifo_get_fill_level(queue->tx_conf_fifo, &fill_level);
			if (unlikely(EOK != err))
			{
				NXP_LOG_ERROR("Unable to get fifo fill level: %d\n", err);
			}

			if (0U != fill_level)
			{
				NXP_LOG_WARNING("Client %s, TX queue %d: Queue is not empty\n", pfe_log_if_get_name(client->log_if), ii);
			}

			fifo_destroy(queue->tx_conf_fifo);
			queue->tx_conf_fifo = NULL;
		}
	}
}

static errno_t hif_client_create_rx_queues(pfe_hif_drv_client_t *client, uint32_t q_size)
{
	uint32_t ii;
	struct client_rx_queue *queue;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Initialize RX queues */
	memset(client->rx_q, 0, HIF_DRV_CLIENT_QUEUES_MAX * sizeof(struct client_rx_queue));

	/*	Create the queues */
	for (ii=0U; ii<client->rx_qn; ii++)
	{
		queue = &client->rx_q[ii];

		/*
			This FIFO is used to store received frames until client processes it.
			HIF is putting data in there by calling 'put()' and client is reading it via 'get()'.
			Since there is only	one producer and one consumer the FIFO does not need to be
			protected. See pfe_hif_drv_client_receive_pkt().
		*/
		queue->rx_fifo = fifo_create(q_size);
		if (unlikely(NULL == queue->rx_fifo))
		{
			goto free_and_fail;
		}
		else
		{
			queue->size = q_size;
		}
	}

	return EOK;

free_and_fail:
	/*	Release all resources */
	hif_client_free_rx_queues(client);

	return ENOMEM;
}

static errno_t hif_client_create_tx_queues(pfe_hif_drv_client_t *client, uint32_t q_size)
{
	uint32_t ii;
	struct client_tx_queue *queue;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Initialize TX queues */
	memset(client->tx_q, 0, HIF_DRV_CLIENT_QUEUES_MAX * sizeof(struct client_tx_queue));

	/*	Create the queues */
	for (ii=0U; ii<client->tx_qn; ii++)
	{
		queue = &client->tx_q[ii];

		/*	Create TX confirmation queues. Does not need to be protected since only HIF
			worker puts data in there and only a single client read it. */
		queue->tx_conf_fifo = fifo_create(q_size);
		if (unlikely(NULL == queue->tx_conf_fifo))
		{
			goto free_and_fail;
		}
		else
		{
			queue->size = q_size;
		}
	}

	return EOK;

free_and_fail:
	/*	Release all resources */
	hif_client_free_tx_queues(client);

	return ENOMEM;
}

#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
/**
 * @brief		Register special IHC client
 * @details		Routine creates new HIF driver client to be used for inter-HIF communication
 * @param[in]	hif_drv The HIF driver instance the client shall be associated with
 * @param[in]	handler Pointer to function to be called to indicate events (data available, ...).
 * 						Mandatory. Can be called from various contexts.
 * @param[in]	priv Private data to be stored within the client instance and passed as handler argument
 * @return		HIF driver client instance or NULL if failed
 * @warning		Can only be called when HIF driver is stopped
 */
pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv)
{
	pfe_hif_drv_client_t *client;
	errno_t err;

	if (NULL == handler)
	{
		NXP_LOG_ERROR("Event handler is mandatory\n");
		return NULL;
	}

	/*	Initialize the instance */
	client = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_hif_drv_client_t), HAL_CACHE_LINE_SIZE);
	if (NULL == client)
	{
		NXP_LOG_ERROR("HIF IHC client: Memory allocation failed\n");
		return NULL;
	}

	memset(client, 0, sizeof(pfe_hif_drv_client_t));

	client->hif_drv = hif_drv;
	client->phy_if = NULL;
	client->log_if = NULL;
	client->rx_qn = 1U;
	client->tx_qn = 1U;
	client->event_handler = handler;
	client->priv = priv;

	/*	Create client's RX queues */
	err = hif_client_create_rx_queues(client, 8U);
	if (unlikely(EOK != err))
	{
		NXP_LOG_ERROR("Can't create RX queues: %d\n", err);
		goto free_and_fail;
	}

	/*	Initialize client's TX queues */
	err = hif_client_create_tx_queues(client, 8U);
	if (unlikely(EOK != err))
	{
		NXP_LOG_ERROR("Can't create TX queues: %d\n", err);
		goto free_and_fail;
	}

	/*	Prevent concurrent API access */
	if (EOK != oal_mutex_lock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (NULL != hif_drv->ihc_client)
	{
		NXP_LOG_ERROR("IHC client already registered\n");

		if (EOK != oal_mutex_unlock(&hif_drv->cl_api_lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		goto free_and_fail;
	}
	else
	{
		/*	Suspend HIF driver to get exclusive access to client storage */
		pfe_hif_drv_stop(client->hif_drv);

		/*	Remember the client within HIF driver structures */
		hif_drv->ihc_client = client;
	}

	if (EOK != oal_mutex_unlock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	NXP_LOG_INFO("HIF IHC client registered\n");

	return client;

free_and_fail:
	pfe_hif_drv_ihc_client_unregister(client);
	return NULL;
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
		/*	Prevent concurrent API access */
		if (EOK != oal_mutex_lock(&client->hif_drv->cl_api_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Suspend HIF driver to get exclusive access to client storage */
		pfe_hif_drv_stop(client->hif_drv);

		client->hif_drv->ihc_client = NULL;

		if (EOK != oal_mutex_unlock(&client->hif_drv->cl_api_lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		/*	Release queues */
		hif_client_free_rx_queues(client);
		hif_client_free_tx_queues(client);

		oal_mm_free_contig(client);
		client = NULL;

		NXP_LOG_INFO("HIF IHC client removed\n");
	}
}
#endif /* GLOBAL_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief		This function is used to register a client driver with the HIF driver.
 * @details		Routine creates new HIF driver client, associates it with given logical interface
 * 				and adjusts internal HIF dispatching table to properly route ingress packets to
 * 				client's queues. HIF driver remains suspended after the call and pfe_hif_drv_start()
 * 				is required to re-enable the operation.
 * @param[in]	hif_drv The HIF driver instance the client shall be associated with
 * @param[in]	log_if Logical interface to be handled by the client
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
pfe_hif_drv_client_t * pfe_hif_drv_client_register(pfe_hif_drv_t *hif_drv, pfe_log_if_t *log_if, uint32_t txq_num, uint32_t rxq_num,
								uint32_t txq_depth, uint32_t rxq_depth, pfe_hif_drv_client_event_handler handler, void *priv)
{
	pfe_hif_drv_client_t *client = NULL;
	errno_t err;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == hif_drv) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	NXP_LOG_INFO("Attempt to register HIF client: %s\n", pfe_log_if_get_name(log_if));

	/*	Prevent concurrent API access */
	if (EOK != oal_mutex_lock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*
		The HIF driver is using logical interface ID to match ingress packets with clients.
		For this purpose an array is used where particular client instances are stored
		and the HIF driver is addressing them via logical interface IDs received from classifier.
		Size of this array is limited so we only support limited number of clients and limited
		range of logical interface IDs (0 - HIF_CLIENTS_MAX).
	 */
	if (pfe_log_if_get_id(log_if) >= HIF_CLIENTS_MAX)
	{
		NXP_LOG_ERROR("Incompatible interface ID requested: %d\n", pfe_log_if_get_id(log_if));
		goto unlock_and_fail;
	}

	if (NULL == handler)
	{
		NXP_LOG_ERROR("Event handler is mandatory\n");
		goto unlock_and_fail;
	}

	/*	The interface-to-client mapping is done by logical interface ID */
	client = hif_drv->clients[pfe_log_if_get_id(log_if)];
	if (NULL != client)
	{
		NXP_LOG_ERROR("Client %d already initialized\n", pfe_log_if_get_id(log_if));
		goto unlock_and_fail;
	}

	/*	Check if client is requesting more queues than supported */
	if (rxq_num > HIF_DRV_CLIENT_QUEUES_MAX)
	{
		NXP_LOG_WARNING("Client requests more (%d) RX queues than currently supported maximum (%d)\n", rxq_num, HIF_DRV_CLIENT_QUEUES_MAX);
		rxq_num = HIF_DRV_CLIENT_QUEUES_MAX;
	}

	/*	Check if client is requesting more queues than supported */
	if (txq_num > HIF_DRV_CLIENT_QUEUES_MAX)
	{
		NXP_LOG_WARNING("Client requests more (%d) TX queues than currently supported maximum (%d)\n", txq_num, HIF_DRV_CLIENT_QUEUES_MAX);
		txq_num = HIF_DRV_CLIENT_QUEUES_MAX;
	}

	/*	Initialize the instance */
	client = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_hif_drv_client_t), HAL_CACHE_LINE_SIZE);
	if (NULL == client)
	{
		NXP_LOG_ERROR("HIF client: Memory allocation failed\n");
		goto unlock_and_fail;
	}

	memset(client, 0, sizeof(pfe_hif_drv_client_t));

	client->hif_drv = hif_drv;
	client->log_if = log_if;
	client->phy_if = pfe_log_if_get_parent(log_if);
	if (NULL == client->phy_if)
	{
		NXP_LOG_ERROR("Can't get physical interface associated with %s\n", pfe_log_if_get_name(log_if));
		goto unlock_and_fail;
	}

#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	/*	Get PA of the HIF header storage. HIF header is used to provide
	 	control data to the PFE firmware with every transmitted packet. */
	client->hif_tx_header = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_ct_hif_tx_hdr_t), 8);
	client->hif_tx_header_pa = oal_mm_virt_to_phys_contig(client->hif_tx_header);
	if (NULL == client->hif_tx_header_pa)
	{
		NXP_LOG_ERROR("VA-to-PA failed\n");
		goto unlock_and_fail;
	}

#if defined(TARGET_HW_S32G)
	/*	Initialize the HIF TX header */
	client->hif_tx_header->chid = pfe_hif_chnl_get_id(client->hif_drv->channel);
	client->hif_tx_header->flags = HIF_TX_INJECT;
	client->hif_tx_header->e_phy_ifs = oal_htonl(1U << pfe_phy_if_get_id(client->phy_if));
#elif defined(TARGET_HW_LS1012A)
	client->hif_tx_header->tx_port = client->id;
#endif /* TARGET_HW_S32G */

#ifdef GLOBAL_CFG_CSUM_ALL_FRAMES
	client->hif_tx_header->flags |= HIF_IP_CSUM | HIF_TCP_CSUM | HIF_UDP_CSUM;
#endif /* GLOBAL_CFG_CSUM_ALL_FRAMES */

#if (TRUE == HAL_HANDLE_CACHE)
	/*	Flush cache over the HIF header */
	oal_mm_cache_flush(&client->hif_tx_header, client->hif_tx_header_pa, sizeof(client->hif_tx_header));
#endif /* HAL_HANDLE_CACHE */
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

	client->rx_qn = rxq_num;
	client->tx_qn = txq_num;
	client->event_handler = handler;
	client->priv = priv;

	/*	Create client's RX queues */
	err = hif_client_create_rx_queues(client, rxq_depth);
	if (unlikely(EOK != err))
	{
		NXP_LOG_ERROR("Can't create RX queues: %d\n", err);
		goto unlock_and_fail;
	}

	/*	Initialize client's TX queues */
	err = hif_client_create_tx_queues(client, txq_depth);
	if (unlikely(EOK != err))
	{
		NXP_LOG_ERROR("Can't create TX queues: %d\n", err);
		goto unlock_and_fail;
	}

	/*	Sanity check due to hif_drv->clients array boundaries protection */
	if (pfe_log_if_get_id(log_if) >= HIF_CLIENTS_MAX)
	{
		NXP_LOG_ERROR("No space for client with ID %d\n", pfe_log_if_get_id(log_if));
		goto unlock_and_fail;
	}

	/*	Suspend HIF driver to get exclusive access to client table */
	pfe_hif_drv_stop(hif_drv);

	/*	Put the pointer into array to enable HIF dispatcher easily get
		the client instance via its ID */
	hif_drv->clients[pfe_log_if_get_id(log_if)] = client;

	if (EOK != oal_mutex_unlock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	NXP_LOG_INFO("HIF client %s id %d registered\n", pfe_log_if_get_name(log_if), pfe_log_if_get_id(log_if));

	return client;

unlock_and_fail:
#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	if (NULL != client->hif_tx_header)
	{
		oal_mm_free_contig(client->hif_tx_header);
		client->hif_tx_header = NULL;
	}
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

	if (EOK != oal_mutex_unlock(&hif_drv->cl_api_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return client->hif_drv;
}

/**
 * @brief		Unregister client from the HIF driver
 * @details		Routine removes the given HIF driver client and all associated resources. It
 * 				adjusts internal HIF dispatching table and invalidates the client's entry so
 * 				all ingress packets targeting the client will be dropped. HIF driver remains
 *				suspended after the call and pfe_hif_drv_start() is required to re-enable the
 *				operation.
 * @param[in]	client Client instance
 */
void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client)
{

	if (NULL != client)
	{
		NXP_LOG_INFO("Attempt to remove HIF client: %s\n", pfe_log_if_get_name(client->log_if));

		/*	Prevent concurrent API access */
		if (EOK != oal_mutex_lock(&client->hif_drv->cl_api_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Suspend HIF driver to get exclusive access to client table */
		pfe_hif_drv_stop(client->hif_drv);

		/*	Unregister from HIF. After this the HIF RX dispatcher will not fill client's RX queues. */
		client->hif_drv->clients[pfe_log_if_get_id(client->log_if)] = NULL;

		if (EOK != oal_mutex_unlock(&client->hif_drv->cl_api_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Release queues */
		hif_client_free_rx_queues(client);
		hif_client_free_tx_queues(client);

#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
		/*	Release TX header storage */
		if (NULL != client->hif_tx_header)
		{
			oal_mm_free_contig(client->hif_tx_header);
			client->hif_tx_header = NULL;
		}
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

		NXP_LOG_INFO("HIF client %s removed\n", pfe_log_if_get_name(client->log_if));

		/*	Release client instance */
		oal_mm_free_contig(client);
	}
}

/**
 * @brief		Get packet from RX queue
 * @param[in]	client Client instance
 * @param[in]	queue RX queue number
 * @return		Pointer to SW buffer descriptor containing the packet or NULL
 * 				if the queue does not contain data
 *
 * @warning		Intended to be called from a single client context only, i.e.
 * 				from a single thread per client.
 */
pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	No resource protection here */
	return fifo_get(client->rx_q[queue].rx_fifo);
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
	uint32_t fill_level;
	errno_t err;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	No resource protection here */
	err = fifo_get_fill_level(client->rx_q[queue].rx_fifo, &fill_level);
	if (unlikely(EOK != err))
	{
		NXP_LOG_ERROR("Unable to get fifo fill level: %d\n", err);
		fill_level = 0U;
	}
	return !!fill_level;
}

/**
 * @brief		Release packet
 * @param[in]	pkt The packet instance
 */
void pfe_hif_pkt_free(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
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
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Return buffer to the pool. Resource protection is embedded. */
	pfe_hif_chnl_release_buf(pkt->client[0]->hif_drv->channel, (void *)pkt->data);
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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == client))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return fifo_get(client->tx_q[queue].tx_conf_fifo);
}

/**
 * @brief		The TX processing routine
 * @details		Process TX confirmations reported by HIF channel and notify
 *				particular clients if their packets were transmitted. Should
 *				be called often enough to keep the channel ready, and clients
 *				informed about their transmission requests.
 * @param[in]	hif_drv The HIF driver instance
 * @param[in]	budget Maximum number of TX confirmations to clean-up at once
 *
 * @return		Number of processed TX frame confirmations
 * @note		No TX resource protection is included. Shall be done by caller
 * 				routine.
 */
static uint32_t pfe_hif_drv_process_tx(pfe_hif_drv_t *hif_drv, uint32_t budget)
{
	pfe_hif_tx_meta_t *tx_metadata;
	pfe_hif_drv_client_t *client;
	uint32_t processed_count = 0U, dropped_count = 0U;
	uint32_t ii = 0U, jj;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	do
	{
		/*	Dequeue TX confirmation. This is actually only check whether
		 	some next frame has been transmitted. */
		if (EOK != pfe_hif_chnl_get_tx_conf(hif_drv->channel))
		{
			/*	No more entries to dequeue */
			break;
		}

		/*	Get metadata associated with the transmitted frame */
		tx_metadata = &hif_drv->tx_meta[hif_drv->tx_meta_rd_idx & hif_drv->tx_meta_idx_mask];

		/*	Get client reference from internal table */
		client = *(tx_metadata->client);

		if (unlikely(NULL == client))
		{
			if (0U == dropped_count)
			{
				NXP_LOG_WARNING("Client not registered, dropping TX confirmation(s)\n");
			}

#ifdef HIF_STATS
			hif_drv->counters[HIF_STATS_TX_CONFIRMATION_DROPS]++;
#endif
			dropped_count++;

			/*	Move to next entry */
			hif_drv->tx_meta_rd_idx++;
			continue;
		}

		/*	We have end-of-frame confirmation here. Put the reference data to client's TX confirmation queue. */
		if (unlikely(EOK != fifo_put(client->tx_q[tx_metadata->q_no].tx_conf_fifo, tx_metadata->ref_ptr)))
		{
			/*	Drop the confirmation */
			if (client == hif_drv->ihc_client)
			{
				/*	The client is IHC client */
				NXP_LOG_WARNING("IHC client's TX confirmation queue is full. TX confirmation dropped.\n");
			}
			else
			{
				/*	The client is logical interface client */
				NXP_LOG_WARNING("Client's (%s) TX confirmation queue is full. TX confirmation dropped.\n",
					pfe_log_if_get_name(client->log_if));
			}
#ifdef HIF_STATS
			hif_drv->counters[HIF_STATS_TX_CONFIRMATION_DROPS]++;
#endif
		}
		else
		{
			/*	Remember that THIS client has a new confirmation */
			client->tx_q[tx_metadata->q_no].has_new_data = TRUE;
		}

		/*	Move to next entry */
		hif_drv->tx_meta_rd_idx++;

	} while (++processed_count < budget);

	/*	Notify client(s) about new confirmations */
	for (ii=0U; ii<HIF_CLIENTS_MAX; ii++)
	{
		client = hif_drv->clients[ii];

		if (NULL == client)
		{
			continue;
		}

		for (jj=0U; jj<HIF_DRV_CLIENT_QUEUES_MAX; jj++)
		{
			if (TRUE == client->tx_q[jj].has_new_data)
			{
				/*	Here the client 'ii' is informed about confirmations from queue 'jj'.
					Number of entries in its TX confirmation FIFO corresponds to number
					of confirmed (sent) packets. */
				(void)client->event_handler(client, client->priv, EVENT_TXDONE_IND, jj);
				client->tx_q[jj].has_new_data = FALSE;
			}
		}
	}

	/*	Notify IHC client */
	client = hif_drv->ihc_client;
	if (NULL != client)
	{
		for (jj=0U; jj<HIF_DRV_CLIENT_QUEUES_MAX; jj++)
		{
			if (TRUE == client->tx_q[jj].has_new_data)
			{
				/*	Here the IHC client is informed about confirmations from queue 'jj'.
					Number of entries in its TX confirmation FIFO corresponds to number
					of confirmed (sent) packets. */
				(void)client->event_handler(client, client->priv, EVENT_TXDONE_IND, jj);
				client->tx_q[jj].has_new_data = FALSE;
			}
		}
	}

	if (unlikely(dropped_count > 0U))
	{
		NXP_LOG_INFO("%d TX confirmations dropped\n", dropped_count);
	}

	return processed_count;
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == client) || (NULL == sg_list)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Get HIF driver instance from client */
	hif_drv = client->hif_drv;

	/*	Enter critical section */
	if (unlikely(EOK != oal_mutex_lock(&hif_drv->tx_lock)))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

#if (FALSE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
	/*	Process TX confirmations */
	while (HIF_TX_POLL_BUDGET <= pfe_hif_drv_process_tx(hif_drv, HIF_TX_POLL_BUDGET))
	{
		;
	}

	pfe_hif_chnl_tx_dma_start(hif_drv->channel);
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	if (unlikely(FALSE == hif_drv->tx_enabled))
	{
		/*	Transmission is not allowed */
		if (EOK != oal_mutex_unlock(&hif_drv->tx_lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		return EPERM;
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

		if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		return ENOSPC;
	}

	/*
		HIF driver must keep local copy of the HW TX ring to gain access
		to virtual buffer addresses in case when data is being
		acknowledged to a client. For this purpose the SW descriptors
		are being used.
	*/

#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	/*	Use static TX header from client */
	tx_hdr = client->hif_tx_header;
	tx_hdr_pa = client->hif_tx_header_pa;
#else
	/*	Use dynamic TX header */
	tx_metadata = &hif_drv->tx_meta[hif_drv->tx_meta_wr_idx & hif_drv->tx_meta_idx_mask];
	tx_hdr = tx_metadata->hif_tx_header;
	tx_hdr_pa = tx_metadata->hif_tx_header_pa;

	/*	Update the header */
	tx_hdr->queue = queue;
	tx_hdr->flags = sg_list->flags.tx_flags;
	tx_hdr->flags |= HIF_TX_INJECT;
	#ifdef GLOBAL_CFG_CSUM_ALL_FRAMES
	tx_hdr->flags |= HIF_IP_CSUM | HIF_TCP_CSUM | HIF_UDP_CSUM;
	#endif /* GLOBAL_CFG_CSUM_ALL_FRAMES */

	if (client == hif_drv->ihc_client)
	{
		tx_hdr->e_phy_ifs = oal_htonl(1U << sg_list->dst_phy);
		tx_hdr->flags |= HIF_TX_IHC;
		tx_metadata->client = &hif_drv->ihc_client;
	}
	else
	{
		tx_hdr->e_phy_ifs = oal_htonl(1U << pfe_phy_if_get_id(client->phy_if));
		tx_metadata->client = &hif_drv->clients[pfe_log_if_get_id(client->log_if)];
	}


#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */

	/*	Enqueue the HIF packet header */
	err = pfe_hif_chnl_tx(	hif_drv->channel,
							(void *)tx_hdr_pa,
							(void *)tx_hdr,
							sizeof(pfe_ct_hif_tx_hdr_t),
							FALSE);

	if (unlikely(EOK != err))
	{
		/*	Channel did not accept the buffer. Return SW descriptor and fail. */
		NXP_LOG_ERROR("Channel did not accept buffer: %d\n", err);
		goto unlock_and_fail;
	}

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
			goto unlock_and_fail;
		}
		else
		{
			/*	Store the frame metadata */
			if (TRUE == ((ii+1) >= sg_list->size))
			{
#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
				tx_metadata = &hif_drv->tx_meta[hif_drv->tx_meta_wr_idx & hif_drv->tx_meta_idx_mask];
				tx_metadata->client = &hif_drv->clients[pfe_log_if_get_id(client->log_if)];
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
				tx_metadata->len = sg_list->items[ii].len;
				tx_metadata->q_no = queue;
				tx_metadata->flags.common = HIF_LAST_BUFFER;
				tx_metadata->data = (addr_t)sg_list->items[ii].data_pa;
				tx_metadata->ref_ptr = ref_ptr;

				/*	Move to next entry */
				hif_drv->tx_meta_wr_idx++;
			}
		}
	}

	/*	Invoke TX confirmation job */
#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (FALSE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
	if (pfe_hif_chnl_has_tx_conf(hif_drv->channel))
	{
		/*	Channel has transmitted a buffer (buffers) */
		if (unlikely(EOK != oal_job_run(hif_drv->tx_job)))
		{
			NXP_LOG_ERROR("TX job trigger failed\n");
		}
	}
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	/*	Leave the critical section */
	if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return EOK;

unlock_and_fail:
	if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ECANCELED;
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
	sg_list.flags.common = (pfe_hif_drv_common_flags_t)0U;
	sg_list.flags.tx_flags = (pfe_ct_hif_tx_flags_t)0U;
	sg_list.items[0].data_pa = data_pa;
	sg_list.items[0].data_va = data_va;
	sg_list.items[0].len = len;

	return pfe_hif_drv_client_xmit_sg_pkt(client, queue, &sg_list, ref_ptr);
}

#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
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
	sg_list->flags.tx_flags = HIF_TX_IHC;

	return pfe_hif_drv_client_xmit_sg_pkt(client, queue, sg_list, ref_ptr);
}

/**
 * @brief		Transmit a single-buffer IHC packet
 * @param[in]	client Client instance
 * @param[in]	dst Destination physical interface ID. Should by HIFs only.
 * @param[in]	queue TX queue number
 * @param[in]	data_pa Physical address of buffer to be sent
 * @param[in]	data_va Virtual address of buffer to be sent
 * @param[in]	len Length of the buffer
 * @param[in]	ref_ptr Reference pointer to be provided within TX confirmation.
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_ihc_pkt(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t dst, uint32_t queue, void *data_pa, void *data_va, uint32_t len, void *ref_ptr)
{
	hif_drv_sg_list_t sg_list;

	sg_list.size = 1;
	sg_list.dst_phy = dst;
	sg_list.flags.tx_flags = HIF_TX_IHC;
	sg_list.items[0].data_pa = data_pa;
	sg_list.items[0].data_va = data_va;
	sg_list.items[0].len = len;

	return pfe_hif_drv_client_xmit_sg_pkt(client, queue, &sg_list, ref_ptr);
}
#endif /* GLOBAL_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief		Create new HIF driver instance
 * @param[in]	channel The HIF channel instance to be managed
 */
pfe_hif_drv_t *pfe_hif_drv_create(pfe_hif_chnl_t *channel)
{
	pfe_hif_drv_t *hif_drv;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == channel))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Check if is OK to use metadata storage associated with buffers from pool */
	if (pfe_hif_chnl_get_meta_size(channel) < sizeof(pfe_hif_pkt_t))
	{
		NXP_LOG_ERROR("Meta storage size (%d) is less than required (%d)\n", pfe_hif_chnl_get_meta_size(channel), (uint32_t)sizeof(pfe_hif_pkt_t));
		return NULL;
	}

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (hif_drv->initialized)
	{
		NXP_LOG_ERROR("HIF already initialized. Exiting.\n");
		return ECANCELED;
	}

	/*	Initialize RX/TX resources */
	hif_drv->started = FALSE;

	if (pfe_hif_drv_create_data_channel(hif_drv))
	{
		NXP_LOG_ERROR("%s: Could not initialize data channel\n", __func__);
		err = ENOMEM;
		goto err1;
	}

	err = oal_mutex_init(&hif_drv->tx_lock);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Couldn't init mutex (tx_lock): %d\n", err);
		goto err3;
	}

	err = oal_mutex_init(&hif_drv->cl_api_lock);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Couldn't init mutex (cl_api_lock): %d\n", err);
		goto err4;
	}

	/*	Attach channel RX ISR */
	err = pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_IRQ, &pfe_hif_drv_chnl_rx_isr, (void *)hif_drv);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Could not register RX ISR\n");
		goto err5;
	}

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
	/*	Attach channel TX ISR */
	err = pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_TX_IRQ, &pfe_hif_drv_chnl_tx_isr, (void *)hif_drv);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Could not register TX ISR\n");
		goto err6;
	}
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	/*	Attach channel out-of-buffers event handler */
	err = pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_OOB, &pfe_hif_drv_chnl_rx_ooo_handler, (void *)hif_drv);
	if (EOK != err)
	{
		NXP_LOG_ERROR("Could not register RX OOB handler\n");
		goto err7;
	}

	/*	Create RX job */
	hif_drv->rx_job = oal_job_create(&pfe_hif_drv_rx_job, (void *)hif_drv, "HIF RX JOB", OAL_PRIO_NORMAL);
	if (NULL == hif_drv->rx_job)
	{
		err = EFAULT;
		goto err8;
	}

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
	/*	Create TX job */
	hif_drv->tx_job = oal_job_create(&pfe_hif_drv_tx_job, (void *)hif_drv, "HIF TX JOB", OAL_PRIO_NORMAL);
	if (NULL == hif_drv->tx_job)
	{
		err = EFAULT;
		goto err9;
	}
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	hif_drv->rx_enabled = FALSE;
	hif_drv->tx_enabled = FALSE;
	hif_drv->initialized = TRUE;

	return EOK;

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
err9:
	if (EOK != oal_job_destroy(hif_drv->rx_job))
	{
		NXP_LOG_ERROR("oal_job_destroy() failed (RX callback)\n");
	}
	hif_drv->rx_job = NULL;
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */
err8:
	if (EOK != pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_OOB, NULL, NULL))
	{
		NXP_LOG_ERROR("pfe_hif_chnl_set_event_cbk() failed (RX OOB callback)\n");
	}
err7:
#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
	if (EOK != pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_TX_IRQ, NULL, NULL))
	{
		NXP_LOG_ERROR("pfe_hif_chnl_set_event_cbk() failed (TX callback)\n");
	}
err6:
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */
	if (EOK != pfe_hif_chnl_set_event_cbk(hif_drv->channel, HIF_CHNL_EVT_RX_IRQ, NULL, NULL))
	{
		NXP_LOG_ERROR("pfe_hif_chnl_set_event_cbk() failed (RX callback)\n");
	}
err5:
	if (EOK != oal_mutex_destroy(&hif_drv->cl_api_lock))
	{
		NXP_LOG_ERROR("Can't destroy mutex (cl_api_lock)\n");
	}
err4:
	if (EOK != oal_mutex_destroy(&hif_drv->tx_lock))
	{
		NXP_LOG_ERROR("Can't destroy mutex (tx_lock)\n");
	}
err3:
	pfe_hif_drv_destroy_data_channel(hif_drv);
err1:

	return err;
}

/**
 * @brief		Start traffic at HIF level
 * @details		Data transmission/reception is enabled
 * @param[in]	hif_drv The driver instance
 */
errno_t pfe_hif_drv_start(pfe_hif_drv_t *hif_drv)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (FALSE == hif_drv->initialized)
	{
		NXP_LOG_ERROR("HIF driver not initialized\n");
		return ENODEV;
	}

	NXP_LOG_INFO("Enabling HIF channel RX/TX\n");

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
	if (unlikely(EOK != oal_mutex_lock(&hif_drv->tx_lock)))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	if (EOK != pfe_hif_chnl_tx_enable(hif_drv->channel))
	{
		NXP_LOG_ERROR("Couldn't enable TX\n");
	}
	else
	{
		hif_drv->tx_enabled = TRUE;
	}

	if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	/*	Enable the channel interrupts */
	NXP_LOG_INFO("Enabling channel interrupts\n");

	pfe_hif_chnl_rx_irq_unmask(hif_drv->channel);

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
	pfe_hif_chnl_tx_irq_unmask(hif_drv->channel);
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	NXP_LOG_INFO("HIF driver is started\n");

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

		/*
		 *  -------------------------------------------------------------------
		 *	Here the RX resource is disabled. No more packets can be received.
		 *	Run the RX job to process all pending received packets.
		 *	-------------------------------------------------------------------
		 */

		if (EOK != oal_job_run(hif_drv->rx_job))
		{
			NXP_LOG_ERROR("RX job trigger failed\n");
		}

		if (EOK != oal_job_drain(hif_drv->rx_job))
		{
			NXP_LOG_ERROR("Unable to finish RX job\n");
		}

		/*	Disallow reception and ensure the change has been applied */
		hif_drv->rx_enabled = FALSE;

		if (EOK != oal_job_run(hif_drv->rx_job))
		{
			NXP_LOG_ERROR("RX job trigger failed\n");
		}

		if (EOK != oal_job_drain(hif_drv->rx_job))
		{
			NXP_LOG_ERROR("Unable to finish RX job\n");
		}

		NXP_LOG_DEBUG("Disabling channel RX IRQ\n");
		pfe_hif_chnl_rx_irq_mask(hif_drv->channel);

		/*
		 *	-----------------------------------------------------------------------
		 *	Here is ensured that RX tasks will NOT be executed:
		 *		- RX routine is sealed by the 'rx_enabled' flag so won't be called
		 *		- All pending ingress packets are processed
		 *		- RX interrupt is disabled
		 *	-----------------------------------------------------------------------
		 */

		NXP_LOG_INFO("HIF driver RX path is stopped\n");
	}

	/*	Stop TX */
	if (TRUE == hif_drv->tx_enabled)
	{
		if (unlikely(EOK != oal_mutex_lock(&hif_drv->tx_lock)))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

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

		/*
		 *  ---------------------------------------------------------------------
		 *	Here the TX resource is disabled. No more TX confirmations can be
		 *	generated.
		 *	Run the TX confirmation job to process all pending TX confirmations.
		 *	---------------------------------------------------------------------
		 */

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
		if (EOK != oal_job_run(hif_drv->tx_job))
		{
			NXP_LOG_ERROR("TX job trigger failed\n");
		}

		if (EOK != oal_job_drain(hif_drv->tx_job))
		{
			NXP_LOG_ERROR("Unable to finish TX job\n");
		}
#else
		/*	No deferred job. Process remaining TX confirmations directly here. */
		while (HIF_TX_POLL_BUDGET <= pfe_hif_drv_process_tx(hif_drv, HIF_TX_POLL_BUDGET))
		{
			;
		}
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

		/*	Disallow transmission (and TX confirmation) and ensure the change has been applied */
		hif_drv->tx_enabled = FALSE;

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
		if (EOK != oal_job_run(hif_drv->tx_job))
		{
			NXP_LOG_ERROR("TX job trigger failed\n");
		}

		if (EOK != oal_job_drain(hif_drv->tx_job))
		{
			NXP_LOG_ERROR("Unable to finish TX job\n");
		}
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

		if (unlikely(EOK != oal_mutex_unlock(&hif_drv->tx_lock)))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
#if (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION)
		NXP_LOG_INFO("Disabling channel TX IRQ\n");
		pfe_hif_chnl_tx_irq_mask(hif_drv->channel);
#endif /* HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION */
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

		/*
		 *	-------------------------------------------------------------------------
		 *	Here is ensured that:
		 *		- TX nor TX confirmation tasks will be executed
		 *		- TX routine is sealed by the 'tx_enabled' flag so won't be called
		 *		- All TX confirmations are processed and new ones can't be generated
		 *	-------------------------------------------------------------------------
		 */

		/*	Just a sanity check */
		if (hif_drv->tx_meta_rd_idx != hif_drv->tx_meta_wr_idx)
		{
			NXP_LOG_WARNING("TX confirmation FIFO still contains %d entries\n", hif_drv->tx_meta_wr_idx - hif_drv->tx_meta_rd_idx);
		}
		else
		{
			NXP_LOG_INFO("TX confirmation FIFO is empty\n");
		}

		NXP_LOG_INFO("HIF driver TX path is stopped\n");
	}

	/*
	 *	-----------------------------------------------------
	 *	Now the RX and TX resource of HIF channel are frozen
	 *	-----------------------------------------------------
	 */
}

/**
 * @brief		Exit the HIF driver
 * @details		Terminate the HIF driver and release all allocated
 * 				resources.
 * @param[in]	hif_drv The driver instance
 */
void pfe_hif_drv_exit(pfe_hif_drv_t *hif_drv)
{
	uint32_t ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (FALSE == hif_drv->initialized)
	{
		NXP_LOG_WARNING("HIF is already disabled\n");
		return;
	}

	/*	Check if a client is still registered */
	for (ii=0; ii<HIF_CLIENTS_MAX; ii++)
	{
		if (NULL != hif_drv->clients[ii])
		{
			/*	TODO */
			NXP_LOG_ERROR("A client is still registered within HIF\n");
		}
	}

	NXP_LOG_INFO("HIF exiting\n");

	/*	Stop the traffic */
	pfe_hif_drv_stop(hif_drv);

	/*	Finalize jobs */
	NXP_LOG_INFO("Releasing RX/TX jobs\n");
#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
	if (EOK != oal_job_destroy(hif_drv->tx_job))
	{
		NXP_LOG_WARNING("oal_job_destroy() failed (TX)\n");
	}
	hif_drv->tx_job = NULL;
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

	if (EOK != oal_job_destroy(hif_drv->rx_job))
	{
		NXP_LOG_WARNING("oal_job_destroy() failed (RX)\n");
	}
	hif_drv->rx_job = NULL;

	/*	Release HIF channel and buffers */
	NXP_LOG_INFO("Releasing HIF channel\n");
	pfe_hif_drv_destroy_data_channel(hif_drv);

	/*	Release mutex */
	if (EOK != oal_mutex_destroy(&hif_drv->tx_lock))
	{
		NXP_LOG_ERROR("hif_destroy_mutex() failed (tx_lock)\n");
	}

	if (EOK != oal_mutex_destroy(&hif_drv->cl_api_lock))
	{
		NXP_LOG_ERROR("hif_destroy_mutex() failed (cl_api_lock)\n");
	}

	hif_drv->initialized = FALSE;

	NXP_LOG_INFO("HIF exited\n");
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

bpool_t *pfe_hif_drv_get_rx_pool(pfe_hif_drv_t *hif_drv)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#else
	(void)hif_drv;
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return NULL;
}

/**
 * @brief		Get information if packet is last in frame
 * @param[in]	pkt The packet
 * @return		TRUE if 'pkt' is last packet of a frame. False otherwise.
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_is_last(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.common & HIF_LAST_BUFFER);
}

/**
 * @brief		Get information that IP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if IP checksum has been verified and is valid
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_ipv4_csum_valid(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.rx_flags & HIF_RX_IPV4_CSUM);
}

/**
 * @brief		Get information that UDP checksum within IP fragment has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if UDP checksum has been verified and is valid
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_udpv4_csum_valid(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.rx_flags & HIF_RX_UDPV4_CSUM);
}
/**
 * @brief		Get information that UDP checksum within ipv6 fragment has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if UDP checksum has been verified and is valid
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_udpv6_csum_valid(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.rx_flags & HIF_RX_UDPV6_CSUM);
}
/**
 * @brief		Get information that TCP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if TCP checksum withing ipv4 frame has been verified and is valid
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_tcpv4_csum_valid(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.rx_flags & HIF_RX_TCPV4_CSUM);
}
/**
 * @brief		Get information that TCP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if TCP checksum has been verified and is valid
 */
__attribute__((pure, hot)) bool_t pfe_hif_pkt_tcpv6_csum_valid(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.rx_flags & HIF_RX_TCPV6_CSUM);
}

/**
 * @brief		Get pointer to data buffer
 * @param[in]	pkt The packet
 * @return		Pointer to packet data
 */
__attribute__((pure, hot)) addr_t pfe_hif_pkt_get_data(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return pkt->data;
}

/**
 * @brief		Get packet data length in bytes
 * @param[in]	pkt The packet
 * @return		Number of bytes in data buffer
 */
__attribute__((pure, hot)) uint32_t pfe_hif_pkt_get_data_len(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return pkt->len;
}

/**
 * @brief		Get pointer to packet-related memory
 * @param[in]	pkt The packet
 * @return		Pointer to memory associated with the packet where
 * 				a packet-related data can be stored.
 */
__attribute__((pure, hot)) void *pfe_hif_pkt_get_ref_ptr(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return &pkt->ref_ptr;
}

/**
 * @brief		Get HIF client associated with the packet
 * @param[in]	pkt The packet
 * @return		The HIF client instance
 */
__attribute__((pure, hot)) pfe_hif_drv_client_t *pfe_hif_pkt_get_client(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return *(pkt->client);
}

/**
 * @brief		Get ingress physical interface ID
 * @param[in]	pkt The packet
 * @return		The physical interface ID
 */
__attribute__((pure, hot)) pfe_ct_phy_if_id_t pfe_hif_pkt_get_ingress_phy_id(pfe_hif_pkt_t *pkt)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_PHY_IF_ID_INVALID;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return pkt->i_phy_if;
}

#define STR_TAB "  "

/**
 * @brief		Print ring status in text form
 * @param[in]	client The client instance
 * @param[in]	rx True if rx ring is needed
 * @param[in]	tx True if tx ring is needed
 */
void pfe_hif_drv_show_ring_status(pfe_hif_drv_t *hif_drv, bool_t rx, bool_t tx)
{
	uint32_t ii, qid;
	pfe_hif_drv_client_t *cl;
	struct client_rx_queue *rx_q;
	struct client_tx_queue *tx_q;
	pfe_hif_pkt_t *pkt;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif_drv))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	cl = hif_drv->clients[0]; /* TODO: loop over all */

	NXP_LOG_INFO("client %s\n", pfe_log_if_get_name(cl->log_if));
	NXP_LOG_INFO(STR_TAB "status: %sinitialized\n", (NULL != cl) ? "" : "NOT");
	NXP_LOG_INFO(STR_TAB "queue level: rx %d, tx %d\n", cl->rx_qn, cl->tx_qn);

	/* RX */
	if ((NULL != cl) && (TRUE == rx))
	{
		for (qid=0U; qid<cl->rx_qn; qid++)
		{
			rx_q = &cl->rx_q[qid];
			NXP_LOG_INFO(STR_TAB "RX queue %d: size %d\n", qid, rx_q->size);
			if (likely(NULL == rx_q->rx_fifo))
			{
				NXP_LOG_INFO(STR_TAB STR_TAB "[empty ring]\n");
				continue;
			}

			for (ii=0U; ii<rx_q->size; ii++)
			{
				pkt = (pfe_hif_pkt_t *)fifo_peek(rx_q->rx_fifo, ii);
				if (unlikely(NULL == pkt))
				{
					NXP_LOG_INFO(STR_TAB STR_TAB "%-4d [free]\n", ii);
				}
				else
				{
					NXP_LOG_INFO(STR_TAB STR_TAB "%4d %s:%d:%02x:%02x:0x%03x:%*phD\n",
							ii, pfe_log_if_get_name((*(pkt->client))->log_if), pkt->q_no,
								pkt->flags.common, pkt->flags.rx_flags, pkt->len, 16, (void *)pkt->data);
				}
			} /* for ii */
		} /* for qid */
	}

	/* TX */
	if((NULL != cl) && (TRUE == tx))
	{
		for (qid=0U; qid<cl->rx_qn; qid++)
		{
			tx_q = &cl->tx_q[qid];
			NXP_LOG_INFO(STR_TAB "TX queue %d: size %d\n", qid, tx_q->size);
			if (likely(NULL == tx_q->tx_conf_fifo))
			{
				NXP_LOG_INFO(STR_TAB STR_TAB "[empty ring]\n");
				continue;
			}

			for (ii=0U; ii<tx_q->size; ii++)
			{
				pkt = (pfe_hif_pkt_t *)fifo_peek(tx_q->tx_conf_fifo, ii);
				if (unlikely(NULL == pkt))
				{
					NXP_LOG_INFO(STR_TAB STR_TAB "%-4d [free]\n", ii);
				}
				else
				{
					NXP_LOG_INFO(STR_TAB STR_TAB "%4d %s:%d:%02x:%02x:0x%03x:%*phD\n",
							ii, pfe_log_if_get_name((*(pkt->client))->log_if), pkt->q_no,
							pkt->flags.common, pkt->flags.tx_flags, pkt->len, 16, (void *)pkt->data);
				}
			} /* for ii */
		} /* for qid */
	}
}

/** @}*/
