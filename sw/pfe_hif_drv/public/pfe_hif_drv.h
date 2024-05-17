/* =========================================================================
 *
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup    dxgr_PFE_HIF_DRV HIF Driver
 * @brief		The HIF driver
 * @details     The HIF driver providing way to send and receive traffic.
 * 				The driver also:
 * 					- Utilizes @link dxgr_PFE_HIF HIF instance @endlink
 * 					- Maintains RX/TX BD rings
 * 					- Handles TX confirmation events
 * 					- Allocates, distributes, and manages RX buffers,
 * 					  by default, it's disableable, see NOTE.
 * 					- Handles HIF interrupts
 *
 * 		NOTE:	If pfe_hif_chnl is build without internal buffering support
 * 			(PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED = FALSE in pfe_hif_chnl.h),
 * 			then OS driver has to implement RX buffering on its own.
 * 			In general, it is requested to implement two disabled API calls:
 *				1) pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt()
 *				2) void pfe_hif_pkt_free()
 * 			See linux driver for the reference.
 *
 * @addtogroup  dxgr_PFE_HIF_DRV
 * @{
 *
 * @file		pfe_hif_drv.h
 * @brief		The HIF driver header file (QNX).
 * @details		This is the HIF driver API.
 *
 */

#ifndef PFE_HIF_DRV_H_
#define PFE_HIF_DRV_H_

#include "pfe_ct.h"
#include "pfe_log_if.h"
#include "pfe_hif_chnl_linux.h"

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
#define HIF_DRV_CLIENT_QUEUES_MAX	8U

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

#if defined(PFE_CFG_MULTI_INSTANCE_SUPPORT) || defined(PFE_CFG_IEEE1588_SUPPORT)
/*	When there is no need to modify HIF TX header with every TX frame then only
	single static HIF TX header instance (client-owned) will be created and used for
	each transmission. When HIF TX header modification is needed to be done with
	every transmitted frame then multiple HIF TX headers are needed and therefore
	they will be allocated within dedicated storage. */
	#define HIF_CFG_USE_DYNAMIC_TX_HEADERS
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifndef PFE_CFG_CSUM_ALL_FRAMES
/*	Enable dynamic tx headers for individual CSUM
	(on demand) calculation if it is not already enabled */
	#ifndef HIF_CFG_USE_DYNAMIC_TX_HEADERS
		#define HIF_CFG_USE_DYNAMIC_TX_HEADERS
	#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
#endif /* PFE_CFG_CSUM_ALL_FRAMES */

/**
 * @def	    HIF_CFG_DETACH_TX_CONFIRMATION_JOB
 * @brief	If TRUE the TX confirmation procedure will be executed within deferred job.
 * 			If FALSE the TX confirmation will be executed with every pfe_hif_drv_client_xmit call.
 */
#define HIF_CFG_DETACH_TX_CONFIRMATION_JOB		FALSE

/**
 * @def	    HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION
 * @brief	If TRUE then TX confirmation job will be triggered as response to
 * 			TX interrupt/event. If FALSE the TX confirmation job will be triggered
 * 			from within the pfe_hif_drv_client_xmit call.
 */
#define	HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION	FALSE

/**
 * @def		HIF_CFG_DETACH_RX_JOB
 * @brief	If TRUE the RX procedure will be executed within deferred job.
 * 			If FALSE the RX procedure will be executed within RX ISR.
 */
#define HIF_CFG_DETACH_RX_JOB						FALSE

/**
 * @brief	Maximum number of HIF clients. Right now it is set to cover all possible
 * 			physical interfaces and two additional 'special' clients.
 */
#define HIF_CLIENTS_MAX							(((uint8_t)PFE_PHY_IF_ID_MAX + 1U) + 2U)
#define HIF_CLIENTS_IHC_IDX						(((uint8_t)PFE_PHY_IF_ID_MAX + 1U) + 0U)
#define HIF_CLIENTS_AUX_IDX						(((uint8_t)PFE_PHY_IF_ID_MAX + 1U) + 1U)

#if ((FALSE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB) && (TRUE == HIF_CFG_IRQ_TRIGGERED_TX_CONFIRMATION))
#error Impossible configuration
#endif

/**
 * @brief	HIF common RX/TX packet flags
 */
typedef	enum
{
	HIF_FIRST_BUFFER = (1U << 0U), /* First buffer (contains hif header) */
	HIF_LAST_BUFFER = (1U << 1U), /* Last buffer */
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
	} specific;
} pfe_hif_drv_flags_t;

typedef struct
{
	uint32_t size;						/*	Number of valid 'items' entries */

	struct
	{
		void *data_pa;					/*	Pointer to buffer (PA) */
		void *data_va;					/*	Pointer to buffer (VA) */
		uint32_t len;					/*	Buffer length */
	} items[HIF_MAX_SG_LIST_LENGTH];	/*	SG list items */

	/*	Internals */
	pfe_hif_drv_flags_t flags;			/*	Flags */
	pfe_ct_phy_if_id_t dst_phy;			/*	Destination physical interface */
	uint16_t est_ref_num;				/*	EST reference number */
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
	EVENT_ETS,				/* Indicates that new Egress Time Stamp is available */
	HIF_EVENT_MAX
};

typedef struct pfe_hif_drv_client_tag pfe_hif_drv_client_t;

/**
 * @brief	Packet representation struct
 */
struct __attribute__((packed)) pfe_hif_pkt_tag
{
	/*	When every transmitted frame needs to contain customized HIF TX header then
	 	multiple HIF TX header instances are needed. For this purpose the TX metadata
	 	storage is used. */
#ifdef HIF_CFG_USE_DYNAMIC_TX_HEADERS
	pfe_ct_hif_tx_hdr_t *hif_tx_header;
	void *hif_tx_header_pa;
#endif /* HIF_CFG_USE_DYNAMIC_TX_HEADERS */
	pfe_hif_drv_client_t *client;
	addr_t data;
	uint16_t len;
	uint8_t q_no;
	pfe_hif_drv_flags_t flags;
	pfe_ct_phy_if_id_t i_phy_if;
	void *ref_ptr; /* Reference pointer (keep the original mbuf pointer here) */
};

typedef struct pfe_hif_drv_tag pfe_hif_drv_t;
typedef struct pfe_hif_pkt_tag pfe_hif_pkt_t;
typedef errno_t (* pfe_hif_drv_client_event_handler)(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno);

pfe_hif_drv_t *pfe_hif_drv_create(pfe_hif_chnl_t *channel);
void pfe_hif_drv_destroy(pfe_hif_drv_t *hif_drv);
errno_t pfe_hif_drv_init(pfe_hif_drv_t *hif_drv);
errno_t pfe_hif_drv_start(pfe_hif_drv_t *hif_drv);
void pfe_hif_drv_stop(pfe_hif_drv_t *hif_drv);
void pfe_hif_drv_exit(pfe_hif_drv_t *hif_drv);
pfe_hif_chnl_t *pfe_hif_drv_get_chnl(const pfe_hif_drv_t *hif_drv);
void pfe_hif_drv_rx_job(void *arg);
#if (TRUE == HIF_CFG_DETACH_TX_CONFIRMATION_JOB)
void pfe_hif_drv_tx_job(void *arg);
#endif /* HIF_CFG_DETACH_TX_CONFIRMATION_JOB */

#ifdef PFE_CFG_MC_HIF
void pfe_hif_drv_show_ring_status(pfe_hif_drv_t *hif_drv, bool_t rx, bool_t tx);
#endif /*PFE_CFG_MC_HIF*/

/*	IHC API */
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(
		pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/*	AUX API */
#ifdef PFE_CFG_MC_HIF
pfe_hif_drv_client_t * pfe_hif_drv_aux_client_register(
		pfe_hif_drv_t *hif_drv, uint32_t txq_num, uint32_t rxq_num, uint32_t txq_depth,
			uint32_t rxq_depth, pfe_hif_drv_client_event_handler handler, void *priv);
#endif /*PFE_CFG_MC_HIF*/

void pfe_hif_drv_set_idex_resend_cfg(pfe_hif_drv_t *hif_drv, const uint32_t idex_resend_count, const uint32_t *idex_resend_delays);
void pfe_hif_drv_get_idex_resend_cfg(const pfe_hif_drv_t *hif_drv, uint32_t *idex_resend_count, uint32_t *idex_resend_delays);

/*	HIF client */
pfe_hif_drv_client_t * pfe_hif_drv_client_register(
		pfe_hif_drv_t *hif_drv, pfe_ct_phy_if_id_t phy_if_id, uint32_t txq_num,
			uint32_t rxq_num, uint32_t txq_depth, uint32_t rxq_depth, bool_t promisc,
				pfe_hif_drv_client_event_handler handler, void *priv);
errno_t pfe_hif_drv_client_set_inject_if(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t phy_if_id);
pfe_hif_drv_t *pfe_hif_drv_client_get_drv(const pfe_hif_drv_client_t *client);
void *pfe_hif_drv_client_get_priv(const pfe_hif_drv_client_t *client);
void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client);
void pfe_hif_drv_client_rx_done(const pfe_hif_drv_client_t *client);
void pfe_hif_drv_client_tx_done(const pfe_hif_drv_client_t *client);

/*	Packet transmission */
errno_t pfe_hif_drv_client_xmit_pkt(pfe_hif_drv_client_t *client, uint32_t queue, void *data_pa, void *data_va, uint32_t len, void *ref_ptr);
errno_t pfe_hif_drv_client_xmit_sg_pkt(pfe_hif_drv_client_t *client, uint32_t queue, const hif_drv_sg_list_t *const sg_list, void *ref_ptr);
void * pfe_hif_drv_client_receive_tx_conf(const pfe_hif_drv_client_t *client, uint32_t queue);

/*	Packet reception */
bool_t pfe_hif_drv_client_has_rx_pkt(const pfe_hif_drv_client_t *client, uint32_t queue);
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED) || defined(PFE_CFG_TARGET_OS_LINUX)
pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue);
void pfe_hif_pkt_free(const pfe_hif_pkt_t *pkt);
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

/*	PTP Timestamps */
errno_t pfe_hif_drv_client_get_ts(pfe_hif_drv_client_t * const client, bool_t rx,
		uint8_t type, uint16_t port, uint16_t seq_id, uint32_t * const ts_sec, uint32_t * const ts_nsec);

/**
 * @brief		Get information if packet is last in frame
 * @param[in]	pkt The packet
 * @return		TRUE if 'pkt' is last packet of a frame. False otherwise.
 */
static inline bool_t pfe_hif_pkt_is_last(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.common & HIF_LAST_BUFFER);
}

/**
 * @brief		Get information that IP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if IP checksum has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_ipv4_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.specific.rx_flags & HIF_RX_IPV4_CSUM);
}

/**
 * @brief		Get information that UDP checksum within IP fragment has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if UDP checksum has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_udpv4_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.specific.rx_flags & HIF_RX_UDPV4_CSUM);
}
/**
 * @brief		Get information that UDP checksum within ipv6 fragment has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if UDP checksum has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_udpv6_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.specific.rx_flags & HIF_RX_UDPV6_CSUM);
}
/**
 * @brief		Get information that TCP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if TCP checksum withing ipv4 frame has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_tcpv4_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.specific.rx_flags & HIF_RX_TCPV4_CSUM);
}
/**
 * @brief		Get information that TCP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if TCP checksum has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_tcpv6_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!(pkt->flags.specific.rx_flags & HIF_RX_TCPV6_CSUM);
}
/**
 * @brief		Get information that ICMP checksum has been verified by PFE
 * @param[in]	pkt The packet
 * @return		TRUE if ICMP checksum has been verified and is valid
 */
static inline bool_t pfe_hif_pkt_icmp_csum_valid(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return !!((uint16_t)pkt->flags.specific.rx_flags & (uint16_t)HIF_RX_ICMP_CSUM);
}
/**
 * @brief		Get pointer to data buffer
 * @param[in]	pkt The packet
 * @return		Pointer to packet data
 */
static inline addr_t pfe_hif_pkt_get_data(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pkt->data;
}

/**
 * @brief		Get packet data length in bytes
 * @param[in]	pkt The packet
 * @return		Number of bytes in data buffer
 */
static inline uint32_t pfe_hif_pkt_get_data_len(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pkt->len;
}

/**
 * @brief		Get pointer to packet-related memory
 * @param[in]	pkt The packet
 * @return		Pointer to memory associated with the packet where
 * 				a packet-related data can be stored.
 */
static inline void *pfe_hif_pkt_get_ref_ptr(pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return &pkt->ref_ptr;
}

/**
 * @brief		Get HIF client associated with the packet
 * @param[in]	pkt The packet
 * @return		The HIF client instance
 */
static inline pfe_hif_drv_client_t *pfe_hif_pkt_get_client(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pkt->client;
}

/**
 * @brief		Get ingress physical interface ID
 * @param[in]	pkt The packet
 * @return		The physical interface ID
 */
static inline pfe_ct_phy_if_id_t pfe_hif_pkt_get_ingress_phy_id(const pfe_hif_pkt_t *pkt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pkt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_PHY_IF_ID_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pkt->i_phy_if;
}



#endif /* PFE_HIF_DRV_H_ */

/** @}*/
