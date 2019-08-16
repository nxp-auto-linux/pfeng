/* =========================================================================
 *  Copyright 2019 NXP
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
 * @addtogroup  dxgr_PFE_IDEX
 * @{
 *
 * @file		pfe_idex.c
 * @brief		The inter-driver exchange module source file.
 * @details		This file contains IDEX-related functionality.
 *
 */

#include "oal.h"
#include "linked_list.h"
#include "pfe_hif_drv.h"
#include "pfe_idex.h"

/* #define IDEX_CFG_VERBOSE */
/* #define IDEX_CFG_VERY_VERBOSE */

/**
 * @brief	List of interfaces involved in IDEX functionality.
 */
/*	WARNING:	We must not send packets to a HIF channel which is not able to
				accept data. It leads to whole HIF stall. This means there is
				no way to broadcast master discovery requests, resp. driver must
				somehow know which channels are active.

	TODO:		1.) IDEX broadcast must be allowed only to channels witch are
					enabled (HIF_CTRL_CHn).
				2.) Channel monitoring must be implemented to detect HIF stalls.
				3.) In case that stall is detected by master driver then HIF
					must be reset.
				4.) In case that stall is detected by slave driver then it must
					be synchronized in terms of getting a valid sequence number.

				The monitoring could be done by sending periodic messages and
				watching if HIF is transferring them to PFE (HIF_TX_PKT_CNT1_CHn,
				HIF_TX_PKT_CNT2_CHn).
*/
#define IDEX_CFG_HIF_LIST			{PFE_PHY_IF_ID_HIF0, PFE_PHY_IF_ID_HIF1, /* PFE_PHY_IF_ID_HIF2, PFE_PHY_IF_ID_HIF3, PFE_PHY_IF_ID_HIF_NOCPY*/}

/**
 * @brief	Number of request allowed to be pending/waiting for confirmation at
 * 			the same time.
 */
#define IDEX_CFG_REQ_FIFO_DEPTH		8U

/**
 * @brief	IDEX request timeout in seconds
 */
#define IDEX_CFG_REQ_TIMEOUT_SEC	1U

/*
 *	IDEX worker mbox codes
 */
#define IDEX_WORKER_POLL			(1)
#define IDEX_WORKER_QUIT			(2)

/**
 * @brief	IDEX request return values
 * @details	When IDEX request is finalized, this codes are passed to the
 * 			waiting thread to notify it about request status.
 */
typedef enum
{
	IDEX_REQ_RES_OK,
	IDEX_REQ_RES_TIMEOUT
} pfe_idex_request_result_t;

/**
 * @brief		IDEX sequence number type
 */
typedef uint32_t pfe_idex_seqnum_t;

_ct_assert(sizeof(pfe_idex_seqnum_t) == sizeof(uint32_t));

/**
 * @brief	IDEX Frame types
 */
typedef enum __attribute__((packed))
{
	/*	Request. Frames of this type are expected to be responded
		by a remote instance. Therefore they are not released on TX
		confirmation event but stored in request pool and released
		upon timeout or response is received. */
	IDEX_FRAME_CTRL_REQUEST = 0,
	/*	Response. Released at TX confirmation time. */
	IDEX_FRAME_CTRL_RESPONSE = 1
} pfe_idex_frame_type_t;

_ct_assert(sizeof(pfe_idex_frame_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Request types
 */
typedef enum __attribute__((packed))
{
	/*	Master discovery. To find out where master is located. Non-blocking. */
	IDEX_MASTER_DISCOVERY = 0,
	/*	RPC request. Blocking. */
	IDEX_RPC
} pfe_idex_request_type_t;

/**
 * @brief	IDEX Response types
 */
typedef pfe_idex_request_type_t pfe_idex_response_type_t;

_ct_assert(sizeof(pfe_idex_request_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Master Discovery Message header
 */
typedef struct __attribute__((packed, aligned(16))) __pfe_idex_msg_master_discovery_tag
{
	/*	Physical interface ID where master driver is located */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_idex_msg_master_discovery_t;

/**
 * @brief	IDEX RPC Message header
 */
typedef struct __attribute__((packed, aligned(16))) __pfe_idex_rpc_tag
{
	/*	Custom RPC ID */
	uint32_t rpc_id;
	/*	Return value */
	errno_t rpc_ret;
	/*	Payload length */
	uint16_t plen;
} pfe_idex_msg_rpc_t;

_ct_assert(sizeof(errno_t) == sizeof(uint32_t));

/**
 * @brief	IDEX Frame Header
 */
typedef struct __attribute__((packed, aligned(16))) __pfe_idex_frame_header_tag
{
	/*	Destination physical interface ID */
	pfe_ct_phy_if_id_t dst_phy_if;
	/*	Type of frame */
	pfe_idex_frame_type_t type;
} pfe_idex_frame_header_t;

/**
 * @brief	IDEX request states
 */
typedef enum __attribute__((packed))
{
	/*	New request which is not active. Can't be destroyed or timed-out. */
	IDEX_REQ_STATE_NEW = 0,
	/*	Request committed for transmission. Can be timed-out. */
	IDEX_REQ_STATE_COMMITTED,
	/*	Transmitted request waiting for response. Can be timed-out. */
	IDEX_REQ_STATE_TRANSMITTED,
	/*	Invalid request. Will be destroyed be cleanup task. */
	IDEX_REQ_STATE_INVALID = 0xff,
} pfe_idex_request_state_t;

_ct_assert(sizeof(pfe_idex_request_state_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Request Header. Also used as request instance.
 * @details	IDEX Request Frame:
 * 			+--------------------------------------------------+
 * 			|	IDEX Header (pfe_idex_frame_header_t)		   |
 * 			+--------------------------------------------------+
 * 			|	IDEX Request Header (pfe_idex_request_t)	   |
 * 			+--------------------------------------------------+
 * 			|	IDEX Request message (pfe_idex_msg_*_t)		   |
 * 			+--------------------------------------------------+
 */
typedef struct __attribute__((packed, aligned(16))) __pfe_idex_request_tag
{
	/*	Unique sequence number */
	pfe_idex_seqnum_t seqnum;
	/*	Type of message. Specifies format of the payload. */
	pfe_idex_request_type_t type;
	/*	Destination PHY */
	pfe_ct_phy_if_id_t dst_phy_id;
	/*	Request state */
	pfe_idex_request_state_t state;
	/*	Internal sync object */
	oal_mbox_t *mbox;
	/*	Internal linked list hook */
	LLIST_t list_entry;
	/*	Internal timeout value */
	uint32_t timeout;
} pfe_idex_request_t;

/**
 * @brief	IDEX Response Header. Also used as response instance.
 * @details	IDEX Response Frame:
 * 			+--------------------------------------------------+
 * 			|	IDEX Header (pfe_idex_frame_header_t)		   |
 * 			+--------------------------------------------------+
 * 			|	IDEX Response Header (pfe_idex_response_t)	   |
 * 			+--------------------------------------------------+
 * 			|	IDEX Response message (pfe_idex_msg_*_t)	   |
 * 			+--------------------------------------------------+
 */
typedef struct __attribute__((packed, aligned(16))) __pfe_idex_response_tag
{
	/*	Sequence number matching request which the response is dedicated for */
	pfe_idex_seqnum_t seqnum;
	/*	Type of message. Specifies format of the payload. */
	pfe_idex_response_type_t type;
	/*	Payload length in number of bytes */
	uint16_t plen;
} pfe_idex_response_t;

/**
 * @brief	This is IDEX instance representation type
 */
typedef struct pfe_idex_tag
{
	pfe_hif_drv_client_t *ihc_client;	/*	HIF driver IHC client used for communication */
	oal_job_t *idex_job;				/*	Deferred job to process IDEX protocol */
	pfe_ct_phy_if_id_t master_phy_if;	/*	Physical interface ID where master driver is located */
	pfe_ct_phy_if_id_t local_phy_if;	/*	Local physical interface ID */
	pfe_idex_seqnum_t req_seq_num;		/*	Current sequence number */
	LLIST_t req_list;					/*	Internal requests storage */
	oal_mutex_t req_list_lock;			/*	Requests storage sync object */
	bool_t req_list_lock_init;			/*	Flag indicating that mutex is initialized */
	oal_thread_t *worker;				/*	Worker thread running request timeout logic */
	oal_mbox_t *mbox;					/*	MBox used to periodically trigger the timeout thread */
	pfe_idex_rpc_cbk_t rpc_cbk;			/*	Callback to be called in case of RPC requests */
	void *rpc_cbk_arg;					/*	RPC callback argument */
	pfe_idex_request_t *cur_req;		/*	Current IDEX request */
	pfe_ct_phy_if_id_t cur_req_phy_id;	/*	Physical interface the current request has been received from */
} pfe_idex_t;

/*	Local IDEX instance storage */
static pfe_idex_t __idex = {0};
/*	All HIFs storage */
static const pfe_ct_phy_if_id_t __hifs[] = IDEX_CFG_HIF_LIST;

static void *idex_worker_func(void *arg);
static pfe_idex_request_t *pfe_idex_request_get_by_id(pfe_idex_seqnum_t seqnum);
static errno_t pfe_idex_request_set_state(pfe_idex_seqnum_t seqnum, pfe_idex_request_state_t state);
static errno_t pfe_idex_request_finalize(pfe_idex_seqnum_t seqnum, pfe_idex_request_result_t res, void *resp_buf, uint16_t resp_len);
static errno_t pfe_idex_send_response(pfe_ct_phy_if_id_t dst_phy, pfe_idex_response_type_t type, pfe_idex_seqnum_t seqnum, void *data, uint16_t data_len);
static errno_t pfe_idex_send_frame(pfe_ct_phy_if_id_t dst_phy, pfe_idex_frame_type_t type, void *data, uint16_t data_len);
#ifdef GLOBAL_CFG_PFE_SLAVE
static errno_t pfe_idex_request_send(pfe_ct_phy_if_id_t dst_phy, pfe_idex_request_type_t type, void *data, uint16_t data_len, void *resp, uint16_t resp_len);
static void pfe_idex_do_master_discovery(void);
#endif /* GLOBAL_CFG_PFE_SLAVE */

/**
 * @brief		Worker thread body
 */
static void *idex_worker_func(void *arg)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	oal_mbox_msg_t msg;
	LLIST_t *item, *aux;
	pfe_idex_request_t *req;
	errno_t ret;

#ifdef GLOBAL_CFG_PFE_MASTER
	NXP_LOG_INFO("IDEX worker started (master)\n");
#endif /* GLOBAL_CFG_PFE_MASTER */

#ifdef GLOBAL_CFG_PFE_SLAVE
	NXP_LOG_INFO("IDEX worker started (slave)\n");
#endif /* GLOBAL_CFG_PFE_SLAVE */

	while (TRUE)
	{
		if (EOK == oal_mbox_receive(idex->mbox, &msg))
		{
			if (IDEX_WORKER_QUIT == msg.payload.code)
			{
				break;
			}

			if (IDEX_WORKER_POLL == msg.payload.code)
			{
				/*	Lock request storage access */
				if (EOK != oal_mutex_lock(&idex->req_list_lock))
				{
					NXP_LOG_DEBUG("Mutex lock failed\n");
				}

				/*	Do request timeouts */
				if (FALSE == LLIST_IsEmpty(&idex->req_list))
				{
					LLIST_ForEachRemovable(item, aux, &idex->req_list)
					{
						req = (pfe_idex_request_t *)LLIST_Data(item, pfe_idex_request_t, list_entry);
						if (unlikely(NULL != req))
						{
							switch (req->state)
							{
								case IDEX_REQ_STATE_NEW:
								{
									/*	This state is transient and request shall not remain there */
									break;
								}

								case IDEX_REQ_STATE_COMMITTED:
								case IDEX_REQ_STATE_TRANSMITTED:
								{
									if (req->timeout > 0U)
									{
										req->timeout--;
									}
									else
									{
										if (IDEX_REQ_STATE_COMMITTED == req->state)
										{
											NXP_LOG_WARNING("Non-transmitted IDEX request (dst: %d, sn: %d) timed-out\n", req->dst_phy_id, oal_ntohl(req->seqnum));
										}
										else
										{
#ifdef IDEX_CFG_VERBOSE
											NXP_LOG_DEBUG("IDEX request timed-out (dst: %d, sn: %d)\n", req->dst_phy_id, oal_ntohl(req->seqnum));
#endif /* IDEX_CFG_VERBOSE */
										}

										LLIST_Remove(item);

										if (NULL != req->mbox)
										{
											/*	Unblock the request originator thread */
											ret = oal_mbox_send_signal(req->mbox, IDEX_REQ_RES_TIMEOUT);
											if (EOK != ret)
											{
												NXP_LOG_ERROR("Can't unblock IDEX request %d: %d\n", oal_ntohl(req->seqnum), ret);
											}
										}

										oal_mm_free_contig(req);
									}

									break;
								}

								case IDEX_REQ_STATE_INVALID:
								{
#ifdef IDEX_CFG_VERBOSE
									NXP_LOG_DEBUG("Disposing invalid IDEX request %d\n", oal_ntohl(req->seqnum));
#endif /* IDEX_CFG_VERBOSE */
									LLIST_Remove(item);
									oal_mm_free_contig(req);
									break;
								}

								default:
								{
									NXP_LOG_DEBUG("Unknow request state: %d", req->state);
									break;
								}
							}
						}
					}
				}

				if (EOK != oal_mutex_unlock(&idex->req_list_lock))
				{
					NXP_LOG_DEBUG("Mutex unlock failed\n");
				}
			}

			oal_mbox_ack_msg(&msg);
		}
	}

	NXP_LOG_INFO("IDEX worker shutting down\n");
	return NULL;
}

/**
 * @brief		IHC client event handler
 * @details		Called by HIF when client-related event happens (packet received, packet
 * 				transmitted).
 */
static errno_t pfe_idex_ihc_handler(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	errno_t ret = EOK;
	void *ref_ptr;
	pfe_idex_frame_header_t *idex_header;

	switch (event)
	{
		case EVENT_RX_PKT_IND:
		{
			/*	Trigger deferred job to get and process packets */
			if (EOK != oal_job_run(idex->idex_job))
			{
				NXP_LOG_ERROR("Could not trigger RX job\n");
			}

			break;
		}

		case EVENT_TXDONE_IND:
		{
			/*	Get the transmitted frame reference */
			ref_ptr = pfe_hif_drv_client_receive_tx_conf(client, 0);
			if (NULL == ref_ptr)
			{
				NXP_LOG_DEBUG("NULL reference in TX confirmation.\n");
				break;
			}

			/*	We know that the reference is just pointer to transmitted
			 	buffer containing IDEX Header followed by rest of the IDEX
				frame. */
			idex_header = (pfe_idex_frame_header_t *)ref_ptr;
			switch (idex_header->type)
			{
				case IDEX_FRAME_CTRL_REQUEST:
				{
					pfe_idex_request_t *req_header = (pfe_idex_request_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));

#ifdef IDEX_CFG_VERBOSE
					NXP_LOG_DEBUG("Request %d transmitted\n", oal_ntohl(req_header->seqnum));
#endif /* IDEX_CFG_VERBOSE */

					/*	Change request state */
					if (EOK != pfe_idex_request_set_state(req_header->seqnum, IDEX_REQ_STATE_TRANSMITTED))
					{
#ifdef IDEX_CFG_VERBOSE
						NXP_LOG_DEBUG("Transition to IDEX_REQ_STATE_TRANSMITTED failed\n");
#endif /* IDEX_CFG_VERBOSE */
					}

					/*	Don't release the request instance but release the buffer
					 	used to transmit the request. */
					oal_mm_free_contig(ref_ptr);
					ref_ptr = NULL;
					break;
				}

				case IDEX_FRAME_CTRL_RESPONSE:
				{
#ifdef IDEX_CFG_VERBOSE
					pfe_idex_response_t *resp_header = (pfe_idex_response_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));

					NXP_LOG_DEBUG("Response %d transmitted\n", oal_ntohl(resp_header->seqnum));
#endif /* IDEX_CFG_VERBOSE */

					/*	Responses are released immediately once transmitted */
					oal_mm_free_contig(ref_ptr);
					break;
				}

				default:
				{
					NXP_LOG_ERROR("Unknown IDEX frame transmitted\n");
					break;
				}
			}

			break;
		}

		case EVENT_RX_OOB:
		{
			/*	Out-of-buffers event. Silently ignored. */
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unexpected IHC event: 0x%x\n", event);
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

/**
 * @brief		IDEX deferred job
 * @param[in]	arg Argument. See oal_job.
 */
static void pfe_idex_job_func(void *arg)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	pfe_hif_pkt_t *pkt;
	pfe_idex_frame_header_t *idex_header;
	pfe_idex_request_t *idex_req;
	pfe_idex_response_t *idex_resp;
	pfe_idex_msg_master_discovery_t *idex_md_msg;
	errno_t ret;
	pfe_ct_phy_if_id_t i_phy_id;

	while (TRUE)
	{
		/*	Get received packet */
		pkt = pfe_hif_drv_client_receive_pkt(idex->ihc_client, 0U);
		if (NULL == pkt)
		{
			/*	No more received packets */
			break;
		}

		/*	Get RX packet payload. Also skip HIF header. TODO: Think about removing the HIF header in HIF driver. */
		idex_header = (pfe_idex_frame_header_t *)((addr_t)pfe_hif_pkt_get_data(pkt) + sizeof(pfe_ct_hif_rx_hdr_t));
		i_phy_id = pfe_hif_pkt_get_ingress_phy_id(pkt);

		switch (idex_header->type)
		{
			case IDEX_FRAME_CTRL_REQUEST:
			{
				/*	Frame is IDEX request */
				idex_req = (pfe_idex_request_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));

#ifdef IDEX_CFG_VERBOSE
				NXP_LOG_DEBUG("Request %d received\n", oal_ntohl(idex_req->seqnum));
#endif /* IDEX_CFG_VERBOSE */

				if (PFE_PHY_IF_ID_INVALID == idex->local_phy_if)
				{
					/*	Remember own physical interface ID. This is way how to get information about
					 	on which physical interface THIS IDEX driver is located without need to specify
					 	it via some configuration parameter. */
					idex->local_phy_if = idex_req->dst_phy_id;

#ifdef IDEX_CFG_VERBOSE
					NXP_LOG_DEBUG("IDEX: Local physical interface ID is %d\n", idex_req->dst_phy_id);
#endif /* IDEX_CFG_VERBOSE */
				}

				switch (idex_req->type)
				{
					case IDEX_MASTER_DISCOVERY:
					{
						if (pfe_hif_pkt_get_data_len(pkt) < (sizeof(pfe_idex_frame_header_t)+sizeof(pfe_idex_msg_master_discovery_t)))
						{
							NXP_LOG_ERROR("Invalid payload length\n");
						}
						else
						{
#ifdef GLOBAL_CFG_PFE_MASTER
							/*	Master sends response */
							pfe_idex_msg_master_discovery_t resp = {0};

							/*	Response payload carries the master interface ID */
							resp.phy_if_id = idex->local_phy_if;

							/*	Send the response. Target interface is the one the packet is coming from. */
							ret = pfe_idex_send_response(
															i_phy_id,			/* Destination */
															idex_req->type,		/* Response type */
															idex_req->seqnum,	/* Response sequence number */
															&resp,				/* Response payload */
															sizeof(pfe_idex_msg_master_discovery_t) /* Response payload length */
														);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("IDEX_MASTER_DISCOVERY response failed\n");
							}
#endif /* GLOBAL_CFG_PFE_MASTER */

#ifdef GLOBAL_CFG_PFE_SLAVE
							/*	Slave does nothing. */
							;
#endif /* GLOBAL_CFG_PFE_SLAVE */
						}

						break;
					}

					case IDEX_RPC:
					{
						pfe_idex_msg_rpc_t *rpc_req = (pfe_idex_msg_rpc_t *)((addr_t)idex_req + sizeof(pfe_idex_request_t));
						void *rpc_msg_payload_ptr = (void *)((addr_t)rpc_req + sizeof(pfe_idex_msg_rpc_t));

						if (NULL != idex->rpc_cbk)
						{
							/*	Save source interface and current IDEX request reference */
							idex->cur_req_phy_id = i_phy_id;
							idex->cur_req = idex_req;

							/*	Call RPC callback. Response shall be generated inside the callback using the pfe_idex_set_rpc_ret_val(). */
							idex->rpc_cbk(oal_ntohl(rpc_req->rpc_id), rpc_msg_payload_ptr, oal_ntohs(rpc_req->plen), idex->rpc_cbk_arg);

							/*	Invalidate the current interface ID */
							idex->cur_req_phy_id = PFE_PHY_IF_ID_INVALID;
							idex->cur_req = NULL;
						}
						else
						{
#ifdef IDEX_CFG_VERBOSE
							NXP_LOG_DEBUG("RPC callback not found, request %d ignored\n", oal_ntohl(idex_req->seqnum));
#endif /* IDEX_CFG_VERBOSE */
						}

						break;
					}

					default:
					{
						NXP_LOG_WARNING("Unknown IDEX request type received: 0x%x\n", idex_req->type);
						break;
					}
				}

				break;
			} /* IDEX_FRAME_CTRL_REQUEST */

			case IDEX_FRAME_CTRL_RESPONSE:
			{
				/*	Frame is IDEX response */

				/*	Get response header */
				idex_resp = (pfe_idex_response_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));

#ifdef IDEX_CFG_VERBOSE
				NXP_LOG_DEBUG("Response %d received\n", oal_ntohl(idex_resp->seqnum));
#endif /* IDEX_CFG_VERBOSE */

				/*	Matching request found. Check type. */
				switch (idex_resp->type)
				{
					case IDEX_MASTER_DISCOVERY:
					{
						/*	Finalize the associated request */
						ret = pfe_idex_request_finalize(idex_resp->seqnum, IDEX_REQ_RES_OK, NULL, 0U);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Can't finalize IDEX request %d: %d\n", oal_ntohl(idex_resp->seqnum), ret);
						}

						/*	Master discovery is complete now. Remember the master physical interface ID delivered
							within the master discovery message. */
						idex_md_msg = (pfe_idex_msg_master_discovery_t *)((addr_t)idex_resp + sizeof(pfe_idex_response_t));
						idex->master_phy_if = idex_md_msg->phy_if_id;

						break;
					}

					case IDEX_RPC:
					{
						void *resp_payload = (void *)((addr_t)idex_resp + sizeof(pfe_idex_response_t));

						/*	Finalize the associated request */
						ret = pfe_idex_request_finalize(idex_resp->seqnum, IDEX_REQ_RES_OK, resp_payload, oal_ntohs(idex_resp->plen));
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Can't finalize IDEX request %d: %d\n", oal_ntohl(idex_resp->seqnum), ret);
						}

						break;
					}

					default:
					{
						NXP_LOG_WARNING("Unknown IDEX response type received: 0x%x\n", idex_resp->type);
						break;
					}
				}

				break;
			} /* IDEX_FRAME_CTRL_RESPONSE */

			default:
			{
				/*	Unknown frame */
				NXP_LOG_WARNING("Unknown IDEX frame received\n");
				break;
			}
		} /* switch */

		/*	Release the received packet */
		pfe_hif_pkt_free(pkt);
	};
}

/**
 * @brief		Get request by sequence number
 * @note		Every request can be identified by its unique sequence number.
 * 				This routine is responsible for translation between request
 * 				identifier and request instance. In case of increased performance
 *				demand this function shall be updated to do more efficient lookup.
 * @param[in]	phy_id Associated physical interface ID
 * @param[in]	seqnum Sequence number identifying the request
 * @return		The IDEX request instance or NULL if not found
 * @warning		Does not contain request storage protection. Requires that
 * 				access to request storage is exclusive by the caller.
 */
static pfe_idex_request_t *pfe_idex_request_get_by_id(pfe_idex_seqnum_t seqnum)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	LLIST_t *item;
	pfe_idex_request_t *req;

	LLIST_ForEach(item, &idex->req_list)
	{
		req = LLIST_Data(item, pfe_idex_request_t, list_entry);
		if (seqnum == req->seqnum)
		{
			return req;
		}
	}

	return NULL;
}

/**
 * @brief		Find, acknowledge, remove, and dispose request by sequence number
 * @details		1.) Finds request by sequence number
 * 				2.) Removes found request from request storage
 * 				3.) Releases request related resources
 * 				In case of blocking request:
 * 					4.) Pass response data to the waiting thread
 * 					5.) Unblock the originator thread
 * @param[in]	seqnum Sequence number identifying the request
 * @param[in]	res Value to be passed to the waiting thread as blocking result
 * @param[in]	resp_buf Pointer to response data buffer. If NULL no response is passed.
 * @param[in]	resp_len Number of byte in response buffer
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_request_finalize(pfe_idex_seqnum_t seqnum, pfe_idex_request_result_t res, void *resp_buf, uint16_t resp_len)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	pfe_idex_request_t *req = NULL;
	errno_t ret = EOK;

	/*	Lock request storage access */
	if (EOK != oal_mutex_lock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	1.) Find request instance */
	req = pfe_idex_request_get_by_id(seqnum);
	if (NULL == req)
	{
		ret = ENOENT;
	}
	else
	{
		/*	2.) Remove request from the storage */
		LLIST_Remove(&req->list_entry);

		if (NULL != req->mbox)
		{
			/*	Pass response data and unblock the request originator thread */
			ret = oal_mbox_send_message(req->mbox, res, resp_buf, resp_len);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't unblock IDEX request %d: %d\n", oal_ntohl(req->seqnum), ret);
			}
		}

		/*	3.) Dispose the request instance */
		oal_mm_free_contig(req);
	}

	if (EOK != oal_mutex_unlock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Change request state
 * @details		1.) Find request by given seqnum
 * 				2.) If found set new state
 * @param[in]	seqnum Sequence number identifying the request
 * @param[in]	state New request state
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_request_set_state(pfe_idex_seqnum_t seqnum, pfe_idex_request_state_t state)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	pfe_idex_request_t *req = NULL;
	errno_t ret = EOK;

	/*	Lock request storage access */
	if (EOK != oal_mutex_lock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	1.) Find request instance */
	req = pfe_idex_request_get_by_id(seqnum);
	if (NULL == req)
	{
		ret = ENOENT;
	}
	else
	{
		/*	2.) Set new state */
		req->state = state;
	}

	if (EOK != oal_mutex_unlock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Send IDEX response
 * @param[in]	dst_phy Destination physical interface ID
 * @param[in]	type Response type. Should match request type.
 * @param[in]	seqnum Sequence number in network endian. Should match request.
 * @param[in]	data Response payload buffer
 * @param[in]	data_len Response payload length in number of bytes
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_send_response(pfe_ct_phy_if_id_t dst_phy, pfe_idex_response_type_t type, pfe_idex_seqnum_t seqnum, void *data, uint16_t data_len)
{
	pfe_idex_response_t *resp;
	errno_t ret;
	void *payload;

	/*	Create the request buffer with room for request payload */
	resp = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_idex_response_t) + data_len, 0U);
	if (NULL == resp)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}

	/*	Add seqnum and type */
	resp->seqnum = seqnum;
	resp->type = type;
	resp->plen = oal_htons(data_len);

	/*	Add payload */
	payload = (void *)((addr_t)resp + sizeof(pfe_idex_response_t));
	memcpy(payload, data, data_len);

#ifdef IDEX_CFG_VERBOSE
	NXP_LOG_DEBUG("Sending response %d\n", oal_ntohl(seqnum));
#endif /* IDEX_CFG_VERBOSE */

	/*	Send it out within IDEX frame */
	ret = pfe_idex_send_frame(dst_phy, IDEX_FRAME_CTRL_RESPONSE, resp, (sizeof(pfe_idex_response_t) + data_len));
	if (EOK != ret)
	{
		NXP_LOG_ERROR("IDEX response TX failed\n");
		/*	Release the response instance */
		oal_mm_free_contig(resp);
	}
	else
	{
		/*	Response transmitted. Will be released once it is processed */
		;
	}

	return ret;
}

/**
 * @brief		Create and send IDEX request
 * @details		The call will:
 * 				1.) Create request instance
 * 				2.) Save the request to global request storage
 * 				3.) Send the request to destination physical interface
 * 				4.) In case of blocking do block until request is processed
 * @param[in]	dst_phy Destination physical interface ID
 * @param[in]	type Request type
 * @param[in]	data Request payload buffer
 * @param[in]	data_len Request payload length in number of bytes
 * @param[in]	resp Response buffer. If NULL no response data will be provided.
 * @param[in]	resp_len Response buffer length
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_request_send(pfe_ct_phy_if_id_t dst_phy, pfe_idex_request_type_t type, void *data, uint16_t data_len, void *resp, uint16_t resp_len)
{
	pfe_idex_t *idex = (pfe_idex_t *)&__idex;
	pfe_idex_request_t *req;
	errno_t ret;
	void *payload;
	pfe_idex_seqnum_t seqnum;
	oal_mbox_t *mbox = NULL;
	oal_mbox_msg_t msg;

	/*	1.) Create the request instance with room for request payload */
	req = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_idex_request_t) + data_len, 0U);
	if (NULL == req)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}
	else
	{
		/*	Only initialize header, payload will be added below */
		memset((void *)req, 0, sizeof(pfe_idex_request_t));
	}

	if (IDEX_RPC == type)
	{
		/*	This will be blocking request */
		mbox = oal_mbox_create();
		if (NULL == mbox)
		{
			NXP_LOG_ERROR("Can't create mbox\n");
			oal_mm_free_contig(req);
			return EFAULT;
		}
	}
	else
	{
		/*	This will be non-blocking request */
		if (NULL != resp)
		{
			NXP_LOG_ERROR("Non-blocking request can't return response data\n");
			oal_mm_free_contig(req);
			return EINVAL;
		}
	}

	/*	Assign sequence number, type, and destination PHY ID */
	seqnum = oal_htonl(idex->req_seq_num++);
	req->seqnum = seqnum;
	req->type = type;
	req->dst_phy_id = dst_phy;
	req->timeout = IDEX_CFG_REQ_TIMEOUT_SEC;
	req->state = IDEX_REQ_STATE_NEW;
	req->mbox = mbox;

	/*	Add payload */
	payload = (void *)((addr_t)req + sizeof(pfe_idex_request_t));
	memcpy(payload, data, data_len);

	/*	2.) Save the request to internal storage */
	if (EOK != oal_mutex_lock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	LLIST_AddAtEnd(&req->list_entry, &idex->req_list);

	if (EOK != oal_mutex_unlock(&idex->req_list_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	/*	3.) Send the request */
#ifdef IDEX_CFG_VERBOSE
	NXP_LOG_DEBUG("Sending IDEX request %d\n", oal_ntohl(req->seqnum));
#endif /* IDEX_CFG_VERBOSE */

	/*	Send it out as payload of IDEX frame */
	ret = pfe_idex_send_frame(dst_phy, IDEX_FRAME_CTRL_REQUEST, req, (sizeof(pfe_idex_request_t) + data_len));
	if (EOK != ret)
	{
		NXP_LOG_ERROR("IDEX request TX failed\n");

		/*	Mark the request as INVALID. Will be destroyed by time-out task. */
		if (EOK != pfe_idex_request_set_state(seqnum, IDEX_REQ_STATE_INVALID))
		{
			NXP_LOG_DEBUG("Transition to IDEX_REQ_STATE_INVALID failed\n");
		}
	}
	else
	{
		/*	Request transmitted. Will be released once it is processed. */
		if (EOK != pfe_idex_request_set_state(seqnum, IDEX_REQ_STATE_COMMITTED))
		{
			NXP_LOG_WARNING("Transition to IDEX_REQ_STATE_COMMITTED failed\n");
		}

		/*	4.) Block until request is processed */
		if (NULL != mbox)
		{
			if (EOK == oal_mbox_receive(mbox, &msg))
			{
				if (IDEX_REQ_RES_OK == msg.payload.code)
				{
#ifdef IDEX_CFG_VERY_VERBOSE
					NXP_LOG_DEBUG("IDEX request unblock OK\n");
#endif /* IDEX_CFG_VERY_VERBOSE */

					/*	Get response data */
					if ((NULL != resp) && (NULL != msg.payload.ptr))
					{
						if (msg.payload.len > resp_len)
						{
							NXP_LOG_ERROR("Response buffer too small (%d < %d)\n", resp_len, msg.payload.len);
							ret = ENOMEM;
						}
						else
						{
							memcpy(resp, msg.payload.ptr, msg.payload.len);
							ret = EOK;
						}
					}

					/*	Unblock the message sender */
					oal_mbox_ack_msg(&msg);
				}
				else if (IDEX_REQ_RES_TIMEOUT == msg.payload.code)
				{
#ifdef IDEX_CFG_VERY_VERBOSE
					NXP_LOG_DEBUG("IDEX request timed-out\n");
#endif /* IDEX_CFG_VERY_VERBOSE */
					ret = ETIMEDOUT;
				}
				else
				{
					NXP_LOG_WARNING("Unexpected mbox code: %d\n", msg.payload.code);
					ret = EOK;
				}
			}
			else
			{
				NXP_LOG_ERROR("FATAL: oal_mbox_receive() failed\n");
			}

			oal_mbox_destroy(mbox);
			mbox = NULL;
		}
	}

	return ret;
}

/**
 * @brief		Send IDEX frame
 * @param[in]	dst_phy Destination physical interface ID
 * @param[in]	type Type of frame
 * @param[in]	data Pointer to frame payload
 * @param[in]	data_len Payload length in number of bytes
 * @return		EOK success, error code otherwise
 */
static errno_t pfe_idex_send_frame(pfe_ct_phy_if_id_t dst_phy, pfe_idex_frame_type_t type, void *data, uint16_t data_len)
{
	pfe_idex_frame_header_t *idex_hdr, *idex_hdr_pa;
	void *payload;
	errno_t ret;
	hif_drv_sg_list_t sg_list;

	/*	Get IDX frame buffer */
	idex_hdr = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_idex_frame_header_t) + data_len, 0U);
	if (NULL == idex_hdr)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}

	idex_hdr_pa = oal_mm_virt_to_phys(idex_hdr);
	if (NULL == idex_hdr_pa)
	{
		NXP_LOG_ERROR("VA to PA conversion failed\n");
		oal_mm_free_contig(idex_hdr);
		return ENOMEM;
	}

	/*	Fill the header */
	idex_hdr->dst_phy_if = dst_phy;
	idex_hdr->type = type;

	/*	Add payload */
	payload = (void *)((addr_t)idex_hdr + sizeof(pfe_idex_frame_header_t));
	memcpy(payload, data, data_len);

	/*	Build SG list
	 	TODO: The SG list could be used as reference to all buffers and used to
	 	release them within TX confirmation task when used as 'ref_ptr' argument of
	 	..._ihc_sg_pkt() instead of idex_hdr. */
	sg_list.size = 1U;
	sg_list.items[0].data_va = idex_hdr;
	sg_list.items[0].data_pa = idex_hdr_pa;
	sg_list.items[0].len = sizeof(pfe_idex_frame_header_t) + data_len;

	/*	Send it out */
	ret = pfe_hif_drv_client_xmit_ihc_sg_pkt(__idex.ihc_client, dst_phy, 0U, &sg_list, (void *)idex_hdr);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("IDEX frame TX failed\n");
		oal_mm_free_contig(idex_hdr);
	}
	else
	{
		/*	Frame transmitted. Will be released once TX confirmation is received. */
		;
	}

	return ret;
}

#ifdef GLOBAL_CFG_PFE_SLAVE
/**
 * @brief		Run the master discovery task
 * @details		Routine will try to find master driver by broadcasting
 * 				master discovery requests. Result is writted directly
 * 				to IDEX instance structures.
 */
static void pfe_idex_do_master_discovery(void)
{
	pfe_idex_t *idex =  &__idex;
	uint32_t ii, jj;
	pfe_idex_msg_master_discovery_t msg;

	for (ii=0U; ii<10U; ii++) /* TODO: make this configurable */
	{
		/*	Broadcast master location request until master driver is found */
		for (jj=0U; jj<(sizeof(__hifs)/sizeof(pfe_ct_phy_if_id_t)); jj++)
		{
			if (EOK != pfe_idex_request_send(__hifs[jj], IDEX_MASTER_DISCOVERY, &msg, sizeof(msg), NULL, 0U))
			{
				NXP_LOG_ERROR("pfe_idex_send_request() failed\n");
			}
		}

		if (PFE_PHY_IF_ID_INVALID == idex->master_phy_if)
		{
			oal_time_usleep(1000000);
		}
		else
		{
			/*	Master found */
			break;
		}
	}
}
#endif /* GLOBAL_CFG_PFE_SLAVE */

/**
 * @brief		IDEX initialization routine
 * @param[in]	hif_drv The HIF driver instance to be used to transport the data
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_init(pfe_hif_drv_t *hif_drv)
{
	pfe_idex_t *idex = &__idex;
	errno_t ret;

	if (NULL == hif_drv)
	{
		NXP_LOG_ERROR("HIF driver is mandatory\n");
		return EINVAL;
	}

	memset(idex, 0, sizeof(pfe_idex_t));

	idex->req_seq_num = random();

	/*	Here we don't know even own interface ID... */
	idex->master_phy_if = PFE_PHY_IF_ID_INVALID;
	idex->local_phy_if = PFE_PHY_IF_ID_INVALID;
	idex->cur_req_phy_id = PFE_PHY_IF_ID_INVALID;

	/*	Create mutex */
	ret = oal_mutex_init(&idex->req_list_lock);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Mutex init failed\n");
		pfe_idex_fini();
		return ret;
	}
	else
	{
		idex->req_list_lock_init = TRUE;
	}

	/*	Initialize requests storage */
	LLIST_Init(&idex->req_list);

	/*	Register IHC client */
	idex->ihc_client = pfe_hif_drv_ihc_client_register(hif_drv, &pfe_idex_ihc_handler, NULL);
	if (NULL == idex->ihc_client)
	{
		NXP_LOG_ERROR("Can't register IHC client\n");
		pfe_idex_fini();
		return EFAULT;
	}

	/*	Activate the driver. From now IHC is available. */
	ret = pfe_hif_drv_start(hif_drv);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Unable to start HIF driver\n");
		pfe_idex_fini();
		return ret;
	}

	/*	Create IDEX worker job */
	idex->idex_job = oal_job_create(&pfe_idex_job_func, NULL, "IDEX Job", OAL_PRIO_TOP);
	if (NULL == idex->idex_job)
	{
		NXP_LOG_ERROR("Unable to create IDEX job\n");
		pfe_idex_fini();
		return EFAULT;
	}

	/*	Create MBOX */
	idex->mbox = oal_mbox_create();
	if (NULL == idex->mbox)
	{
		NXP_LOG_ERROR("Could not create MBOX\n");
		pfe_idex_fini();
		return EFAULT;
	}

	/*	Create timeout worker thread */
	idex->worker = oal_thread_create(&idex_worker_func, NULL, "IDEX Worker", 0);
	if (NULL == idex->worker)
	{
		NXP_LOG_ERROR("Could not create IDEX worker thread\n");
		pfe_idex_fini();
		return EFAULT;
	}

	/*	Attach timer to periodically wake up the worker thread every second */
	ret = oal_mbox_attach_timer(idex->mbox, 1000U, IDEX_WORKER_POLL);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Unable to attach timer\n");
		pfe_idex_fini();
		return ret;
	}

#ifdef GLOBAL_CFG_PFE_SLAVE
	/*	Get master driver coordinates. Result will be stored into the idex instance. */
	pfe_idex_do_master_discovery();

	if (PFE_PHY_IF_ID_INVALID == idex->master_phy_if)
	{
		/*	Can't continue */
		NXP_LOG_ERROR("Could not determine master driver location\n");
		pfe_idex_fini();
		return ENXIO;
	}
	else
	{
		NXP_LOG_INFO("Master driver is at physical interface ID %d\n", idex->master_phy_if);
	}
#endif /* GLOBAL_CFG_PFE_SLAVE */

	return EOK;
}

/**
 * @brief		Finalize IDEX module
 */
void pfe_idex_fini(void)
{
	pfe_idex_t *idex = &__idex;
	uint32_t ii;
	LLIST_t *item, *aux;
	pfe_idex_request_t *req;

	if (NULL != idex->ihc_client)
	{
		pfe_hif_drv_ihc_client_unregister(idex->ihc_client);
		idex->ihc_client = NULL;
	}

	if (NULL != idex->worker)
	{
		if (NULL != idex->mbox)
		{
			if (EOK != oal_mbox_detach_timer(idex->mbox))
			{
				NXP_LOG_DEBUG("Could not detach timer\n");
			}

			if (EOK != oal_mbox_send_signal(idex->mbox, IDEX_WORKER_QUIT))
			{
				NXP_LOG_DEBUG("oal_mbox_send_signal() failed\n");
			}
			else
			{
				if (EOK != oal_thread_join(idex->worker, NULL))
				{
					NXP_LOG_DEBUG("oal_thread_join() failed\n");
				}

				idex->worker = NULL;
			}
		}
	}

	if (NULL != idex->mbox)
	{
		(void)oal_mbox_detach_timer(idex->mbox);
		oal_mbox_destroy(idex->mbox);
		idex->mbox = NULL;
	}

	if (NULL != idex->idex_job)
	{
		oal_job_destroy(idex->idex_job);
		idex->idex_job = NULL;
	}

	/*	Remove all entries remaining in requests storage */
	for (ii=0U; ii<(sizeof(__hifs)/sizeof(pfe_ct_phy_if_id_t)); ii++)
	{
		if (FALSE == LLIST_IsEmpty(&idex->req_list))
		{
			LLIST_ForEachRemovable(item, aux, &idex->req_list)
			{
				req = (pfe_idex_request_t *)LLIST_Data(item, pfe_idex_request_t, list_entry);
				if (unlikely(NULL != req))
				{
					LLIST_Remove(item);
					oal_mm_free_contig(req);
				}
			}
		}
	}

	if (TRUE == idex->req_list_lock_init)
	{
		oal_mutex_destroy(&idex->req_list_lock);
		idex->req_list_lock_init = FALSE;
	}
}

/**
 * @brief		Set IDEX RPC callback
 * @details		The callback will be called at any time when RPC request
 * 				will be received.
 * @param[in]	cbk Callback to be called
 * @param[in]	arg Custom argument to be passed to the callback
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_set_rpc_cbk(pfe_idex_rpc_cbk_t cbk, void *arg)
{
	pfe_idex_t *idex = &__idex;

	idex->rpc_cbk_arg = arg;
	idex->rpc_cbk = cbk;

	return EOK;
}

/**
 * @brief		Execute RPC agains IDEX master. Blocking call.
 * @param[in]	id Request identifier to be passed to remote RPC callback
 * @param[in]	buf Buffer containing RPC argument data
 * @param[in]	buf_len Length of RPC argument data in the buffer
 * @param[in]	resp Response buffer. In case of successful call (EOK) the
 *				response data is written here.
 * @param[in]	resp_len Response buffer length. If response is bigger than this
 * 				number of bytes, the buffer will not be written and error code
 * 				indicating no memory condition ENOMEM will be returned.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_master_rpc(uint32_t id, void *buf, uint16_t buf_len, void *resp, uint16_t resp_len)
{
	pfe_idex_t *idex = &__idex;

	return pfe_idex_rpc(idex->master_phy_if, id, buf, buf_len, resp, resp_len);
}

/**
 * @brief		Execute RPC. Blocking call.
 * @param[in]	dst_phy Physical interface ID where the request shall be sent
 * @param[in]	id Request identifier to be passed to remote RPC callback
 * @param[in]	buf Buffer containing RPC argument data
 * @param[in]	buf_len Length of RPC argument data in the buffer
 * @param[in]	resp Response buffer. In case of successful call (EOK) the
 *				response data is written here.
 * @param[in]	resp_len Response buffer length. If response is bigger than this
 * 				number of bytes, the buffer will not be written and error code
 * 				indicating no memory condition ENOMEM will be returned.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_rpc(pfe_ct_phy_if_id_t dst_phy, uint32_t id, void *buf, uint16_t buf_len, void *resp, uint16_t resp_len)
{
	errno_t ret;
	pfe_idex_msg_rpc_t *msg = oal_mm_malloc(sizeof(pfe_idex_msg_rpc_t) + buf_len);
	uint8_t local_resp_buf[256] = {0U};
	uint16_t msg_plen;
	void *payload;

	msg->rpc_id = oal_htonl(id);
	msg->plen = oal_htons(buf_len);
	msg->rpc_ret = oal_htonl(EOK);

	payload = (void *)((addr_t)msg + sizeof(pfe_idex_msg_rpc_t));
	memcpy(payload, buf, buf_len);

	/*	This one is blocking */
	ret = pfe_idex_request_send(dst_phy, IDEX_RPC, msg, sizeof(pfe_idex_msg_rpc_t) + buf_len, local_resp_buf, sizeof(local_resp_buf));

	/*	Dispose the message buffer */
	oal_mm_free(msg);
	msg = NULL;

	if (EOK != ret)
	{
		/*	Transport error */
		NXP_LOG_INFO("RPC transport failed: %d\n", ret);
	}
	else
	{
		/*	Get the remote return value from the response */
		msg = (pfe_idex_msg_rpc_t *)&local_resp_buf[0];

		/*	Sanity checks */
		if (id != oal_ntohl(msg->rpc_id))
		{
			NXP_LOG_ERROR("RPC response ID does not match the request\n");
			ret = EINVAL;
		}
		else
		{
			ret = oal_ntohl(msg->rpc_ret);
			msg_plen = oal_ntohs(msg->plen);

			/*	Copy RPC response data to caller's buffer */
			if ((msg_plen > 0U) && (NULL == local_resp_buf))
			{
				NXP_LOG_WARNING("RPC response data received but there is no buffer supplied\n");
			}
			else if (msg_plen > resp_len)
			{
				NXP_LOG_ERROR("Caller's buffer is too small\n");
				ret = ENOMEM;
			}
			else if (0U == msg_plen)
			{
				/* NXP_LOG_DEBUG("RPC response without payload received\n"); */
			}
			else
			{
				payload = (void *)((addr_t)msg + sizeof(pfe_idex_msg_rpc_t));
				memcpy(resp, payload, msg_plen);

#ifdef IDEX_CFG_VERBOSE
				NXP_LOG_DEBUG("%d bytes of RPC response received\n", msg_plen);
#endif /* IDEX_CFG_VERBOSE */
			}
		}
	}

	return ret;
}

/**
 * @brief		Set RPC response
 * @details		Function can ONLY be called within RPC callback (pfe_idex_rpc_cbk_t)
 * 				to indicate the execution result.
 * @param[in]	retval Error code to be presented to RPC initiator
 * @param[in]	resp Buffer containing response data to be presented to
 * 				the initiator. Can be NULL to return no data.
 * @param[in]	resp_len Size of the response in the buffer. Can be zero.
 * @return		EOK success, error code otherwise
 */
errno_t pfe_idex_set_rpc_ret_val(errno_t retval, void *resp, uint16_t resp_len)
{
	pfe_idex_t *idex = &__idex;
	pfe_idex_msg_rpc_t *rpc_resp;
	pfe_idex_msg_rpc_t *rpc_req;
	void *payload;
	errno_t ret;

	/*	Construct response message */
	rpc_resp = oal_mm_malloc(sizeof(pfe_idex_msg_rpc_t) + resp_len);

	rpc_req = (pfe_idex_msg_rpc_t *)((addr_t)idex->cur_req + sizeof(pfe_idex_request_t));

	rpc_resp->rpc_id = rpc_req->rpc_id; /* Already in correct endian */
	rpc_resp->plen = oal_htons(resp_len);
	rpc_resp->rpc_ret = oal_htonl(retval);

	payload = (void *)((addr_t)rpc_resp + sizeof(pfe_idex_msg_rpc_t));
	memcpy(payload, resp, resp_len);

	/*	Send the response */
	ret = pfe_idex_send_response(
									idex->cur_req_phy_id,	/* Destination */
									idex->cur_req->type,	/* Response type */
									idex->cur_req->seqnum,	/* Response sequence number */
									rpc_resp,				/* Response payload */
									(sizeof(pfe_idex_msg_rpc_t) + resp_len) /* Response payload length */
								);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("IDEX RPC response failed\n");
	}

	/*	Dispose the response buffer */
	oal_mm_free(rpc_resp);
	rpc_resp = NULL;

	return ret;
}

/** @}*/
