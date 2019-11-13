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
 * @defgroup    dxgr_PFE_HIF_DRV HIF Driver
 * @brief		The HIF driver
 * @details     The HIF driver providing way to send and receive traffic.
 * 				The driver also:
 * 					- Utilizes @link dxgr_PFE_HIF HIF instance @endlink
 * 					- Maintains RX/TX BD rings
 * 					- Handles TX confirmation events
 * 					- Allocates, distributes, and manages RX buffers
 * 					- Handles HIF interrupts
 *
 * @addtogroup  dxgr_PFE_HIF_DRV
 * @{
 *
 * @file		pfe_hif_drv.h
 * @brief		The HIF driver header file (QNX).
 * @details		This is the HIF driver API.
 *
 */

#ifndef _pfe_hif_drv_H_
#define _pfe_hif_drv_H_

#include "bpool.h"
#include "pfe_ct.h"
#include "pfe_log_if.h"

#define HIF_STATS

enum
{
	HIF_STATS_CLIENT_FULL_COUNT,
	HIF_STATS_RX_POOL_EMPTY,
	HIF_STATS_RX_FRAME_DROPS,
	HIF_STATS_TX_CONFIRMATION_DROPS,
	HIF_STATS_MAX_COUNT
};

/**
 * @brief	Maximum number of client's queues
 * @details	Each HIF client instance contains its own RX and TX queues. The
 * 			number of queues used per direction and per instance is given at
 *			instance creation time (pfe_hif_drv_client_register()) but it is
 *			limited by this (HIF_DRV_CLIENT_QUEUES_MAX) value.
 */
#define HIF_DRV_CLIENT_QUEUES_MAX	1U

/**
 * @brief	Scatter-Gather list length
 * @details	Maximum length of SG list represented by hif_drv_sg_list_t.
 */
#define HIF_MAX_SG_LIST_LENGTH		16U

/**
 * @brief	RX poll budget
 * @details	Value specifies number of buffer received from RX HW resource and
 * 			processed by the HIF driver in a row without interruption. Once
 *			the number of processed RX buffers reach this value, the reception
 *			is temporarily interrupted to enable other threads do their jobs
 *			(yield).
 */
#define HIF_RX_POLL_BUDGET			64U

/**
 * @brief	TX poll budget
 * @details	Value specifies number of TX confirmations provided by TX HW
 * 			resource and processed by the HIF driver in a row without
 *			interruption. Once the number of processed TX confirmations reach
 *			this value, the processing is temporarily interrupted to enable
 *			other thread do their jobs (yield).
 */
#define HIF_TX_POLL_BUDGET			128U

/**
 * @brief	HIF common RX/TX packet flags 
 */
typedef	enum
{
	HIF_FIRST_BUFFER = (1U << 0), /* First buffer (contains hif header) */
	HIF_LAST_BUFFER = (1U << 1), /* Last buffer */
} pfe_hif_drv_common_flags_t;
/**
 * @brief	HIF packet flags
 */
typedef struct
{
	pfe_hif_drv_common_flags_t common;
	union
	{
		pfe_ct_hif_rx_flags_t rx_flags;
		pfe_ct_hif_tx_flags_t tx_flags;
	};
} pfe_hif_drv_flags_t;

typedef struct __sg_list_tag
{
	uint32_t size;						/*	Number of valid 'items' entries */

	struct item
	{
		void *data_pa;					/*	Pointer to buffer (PA) */
		void *data_va;					/*	Pointer to buffer (VA) */
		uint32_t len;					/*	Buffer length */
		uint32_t flags;					/*	Flags TODO: Not used, remove me */
	} items[HIF_MAX_SG_LIST_LENGTH];	/*	SG list items */

	/*	Internals */
	pfe_hif_drv_flags_t flags;			/*	Flags */
	pfe_ct_phy_if_id_t dst_phy;			/*	Destination physical interface */
} hif_drv_sg_list_t;

enum
{
	REQUEST_CL_REGISTER = 0,
	REQUEST_CL_UNREGISTER,
	HIF_REQUEST_MAX
};

enum
{
	EVENT_HIGH_RX_WM = 0,	/* Event to indicate that client rx queue is reached water mark level */
	EVENT_RX_PKT_IND,		/* Event to indicate that, packet recieved for client */
	EVENT_TXDONE_IND,		/* Event to indicate that, packet tx done for client */
	EVENT_RX_OOB,			/* Out of RX buffers */
	HIF_EVENT_MAX
};

typedef struct __pfe_hif_drv_tag pfe_hif_drv_t;
typedef struct __pfe_pfe_hif_drv_client_tag pfe_hif_drv_client_t;
typedef struct __pfe_hif_pkt_tag pfe_hif_pkt_t;
typedef errno_t (* pfe_hif_drv_client_event_handler)(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno);

void __hif_xmit_pkt(pfe_hif_drv_t *hif, uint32_t client_id, uint32_t q_no, void *data, uint32_t len, uint32_t flags);
errno_t hif_xmit_pkt(pfe_hif_drv_t *hif, uint32_t client_id, uint32_t q_no, void *data, uint32_t len);
pfe_hif_drv_t *pfe_hif_drv_create(pfe_hif_chnl_t *channel);
void pfe_hif_drv_destroy(pfe_hif_drv_t *hif);
errno_t pfe_hif_drv_init(pfe_hif_drv_t *hif);
errno_t pfe_hif_drv_start(pfe_hif_drv_t *hif_drv);
void pfe_hif_drv_stop(pfe_hif_drv_t *hif);
void pfe_hif_drv_exit(pfe_hif_drv_t *hif);
void pfe_hif_drv_show_ring_status(pfe_hif_drv_t *hif_drv, bool_t rx, bool_t tx);

/*	IHC API */
#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv);
void pfe_hif_drv_ihc_client_unregister(pfe_hif_drv_client_t *client);
errno_t pfe_hif_drv_client_xmit_ihc_sg_pkt(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t dst, uint32_t queue, hif_drv_sg_list_t *sg_list, void *ref_ptr);
errno_t pfe_hif_drv_client_xmit_ihc_pkt(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t dst, uint32_t queue, void *data_pa, void *data_va, uint32_t len, void *ref_ptr);
#endif /* GLOBAL_CFG_MULTI_INSTANCE_SUPPORT */

/*	HIF client */
pfe_hif_drv_client_t * pfe_hif_drv_client_register(pfe_hif_drv_t *hif, pfe_log_if_t *log_if, uint32_t txq_num, uint32_t rxq_num,
		uint32_t txq_depth, uint32_t rxq_depth, pfe_hif_drv_client_event_handler handler, void *priv);
pfe_hif_drv_t *pfe_hif_drv_client_get_drv(pfe_hif_drv_client_t *client);
void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client);
bpool_t *pfe_hif_drv_get_rx_pool(pfe_hif_drv_t *hif);

/*	Packet transmission */
errno_t pfe_hif_drv_client_xmit_pkt(pfe_hif_drv_client_t *client, uint32_t queue, void *data_pa, void *data_va, uint32_t len, void *ref_ptr);
errno_t pfe_hif_drv_client_xmit_sg_pkt(pfe_hif_drv_client_t *client, uint32_t queue, const hif_drv_sg_list_t *const sg_list, void *ref_ptr);
void * pfe_hif_drv_client_receive_tx_conf(pfe_hif_drv_client_t *client, uint32_t queue);

/*	Packet reception */
pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue);
bool_t pfe_hif_drv_client_has_rx_pkt(pfe_hif_drv_client_t *client, uint32_t queue);
void pfe_hif_pkt_free(pfe_hif_pkt_t *desc);

bool_t pfe_hif_pkt_is_last(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
bool_t pfe_hif_pkt_ipv4_csum_valid(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
bool_t pfe_hif_pkt_udpv4_csum_valid(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
bool_t pfe_hif_pkt_udpv6_csum_valid(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
bool_t pfe_hif_pkt_tcpv4_csum_valid(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
bool_t pfe_hif_pkt_tcpv6_csum_valid(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));

addr_t pfe_hif_pkt_get_data(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
uint32_t pfe_hif_pkt_get_data_len(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
void *pfe_hif_pkt_get_ref_ptr(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
pfe_hif_drv_client_t *pfe_hif_pkt_get_client(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));
pfe_ct_phy_if_id_t pfe_hif_pkt_get_ingress_phy_id(pfe_hif_pkt_t *pkt) __attribute__((pure, hot));

#endif /* _pfe_hif_drv_H_ */

/** @}*/
