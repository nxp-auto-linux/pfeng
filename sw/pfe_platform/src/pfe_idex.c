/* =========================================================================
 *  Copyright 2019-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

#include "oal.h"
#include "linked_list.h"
#include "pfe_hif_drv.h"
#include "pfe_idex.h"

/* #define IDEX_CFG_VERBOSE */
/* #define IDEX_CFG_VERY_VERBOSE */

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

ct_assert(sizeof(pfe_idex_seqnum_t) == sizeof(uint32_t));

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

ct_assert(sizeof(pfe_idex_frame_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Request types
 */
typedef enum __attribute__((packed))
{
	/*	Master discovery. To find out where master is located. Non-blocking. */
	IDEX_MASTER_DISCOVERY = 0U,
	/*	RPC request. Blocking. */
	IDEX_RPC
} pfe_idex_request_type_t;

ct_assert(sizeof(pfe_idex_request_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Response types
 */
typedef pfe_idex_request_type_t pfe_idex_response_type_t;

ct_assert(sizeof(pfe_idex_request_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Master Discovery Message header
 */
typedef struct __attribute__((packed)) pfe_idex_msg_master_discovery_tag
{
	/*	Physical interface ID where master driver is located */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_idex_msg_master_discovery_t;

ct_assert(sizeof(pfe_idex_msg_master_discovery_t) == sizeof(uint8_t));

/**
 * @brief	IDEX RPC Message header
 */
typedef struct __attribute__((packed)) pfe_idex_rpc_tag
{
	/*	Custom RPC ID */
	uint32_t rpc_id;
	/*	Return value */
	errno_t rpc_ret;
	/*	Payload length */
	uint16_t plen;
} pfe_idex_msg_rpc_t;

ct_assert(sizeof(errno_t) == sizeof(uint32_t));

/**
 * @brief	IDEX Frame Header
 */
typedef struct __attribute__((packed)) pfe_idex_frame_header_tag
{
	/*	Destination physical interface ID */
	pfe_ct_phy_if_id_t dst_phy_if;
	/*	Type of frame */
	pfe_idex_frame_type_t type;
} pfe_idex_frame_header_t;

ct_assert(sizeof(pfe_idex_frame_header_t) == 2);

/**
 * @brief	IDEX request states
 */
typedef enum __attribute__((packed))
{
	/*	New request which is not active. Can't be destroyed or timed-out. */
	IDEX_REQ_STATE_NEW = 0U,
	/*	Request committed for transmission. Can be timed-out. */
	IDEX_REQ_STATE_COMMITTED,
	/*	Transmitted request waiting for response. Can be timed-out. */
	IDEX_REQ_STATE_TRANSMITTED,
	/*	Finished request */
	IDEX_REQ_STATE_COMPLETED,
	/*	Invalid request. Will be destroyed be cleanup task. */
	IDEX_REQ_STATE_INVALID = 0xffU,
} pfe_idex_request_state_t;

ct_assert(sizeof(pfe_idex_request_state_t) == sizeof(uint8_t));

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
typedef struct __attribute__((packed)) pfe_idex_request_tag
{
	/*	Unique sequence number */
	pfe_idex_seqnum_t seqnum;
	/*	Type of message. Specifies format of the payload. */
	pfe_idex_request_type_t type;
	/*	Destination PHY */
	pfe_ct_phy_if_id_t dst_phy_id;
	/*	Request state */
	pfe_idex_request_state_t state;
	/*	Internal linked list hook */
	union { /* Avoids changing struct size between 32/64bit architectures */
        struct __attribute__((packed)) {
        	LLIST_t list_entry;
			/*	Internal timeout value */
			uint32_t timeout;
			void *resp_buf;
			uint16_t resp_buf_len;
        };
        uint8_t padding[30U];
    };
} pfe_idex_request_t;

ct_assert(sizeof(pfe_idex_request_t) == 37);

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
typedef struct __attribute__((packed)) pfe_idex_response_tag
{
	/*	Sequence number matching request which the response is dedicated for */
	pfe_idex_seqnum_t seqnum;
	/*	Type of message. Specifies format of the payload. */
	pfe_idex_response_type_t type;
	/*	Payload length in number of bytes */
	uint16_t plen;
} pfe_idex_response_t;

ct_assert(sizeof(pfe_idex_response_t) == 7);

/**
 * @brief	This is IDEX instance representation type
 */
typedef struct pfe_idex_tag
{
	pfe_hif_drv_client_t *ihc_client;	/*	HIF driver IHC client used for communication */
	pfe_ct_phy_if_id_t master_phy_if;	/*	Physical interface ID where master driver is located */
	pfe_idex_seqnum_t req_seq_num;		/*	Current sequence number */
	LLIST_t req_list;					/*	Internal requests storage */
	oal_mutex_t req_list_lock;			/*	Requests storage sync object */
	bool_t req_list_lock_init;			/*	Flag indicating that mutex is initialized */
	pfe_idex_rpc_cbk_t rpc_cbk;			/*	Callback to be called in case of RPC requests */
	void *rpc_cbk_arg;					/*	RPC callback argument */
	pfe_idex_request_t *cur_req;		/*	Current IDEX request */
	pfe_ct_phy_if_id_t cur_req_phy_id;	/*	Physical interface the current request has been received from */
} pfe_idex_t;

/*	Local IDEX instance storage */
static pfe_idex_t pfe_idex = {0};

static void pfe_idex_do_rx(pfe_hif_drv_client_t *client, pfe_idex_t *idex);
static void pfe_idex_do_tx(pfe_hif_drv_client_t *client, pfe_idex_t *idex);
static pfe_idex_request_t *pfe_idex_request_get_by_id(pfe_idex_seqnum_t seqnum);
static errno_t pfe_idex_request_set_state(pfe_idex_seqnum_t seqnum, pfe_idex_request_state_t state);
static errno_t pfe_idex_request_finalize(pfe_idex_seqnum_t seqnum, pfe_idex_request_result_t res, void *resp_buf, uint16_t resp_len);
static errno_t pfe_idex_send_response(pfe_ct_phy_if_id_t dst_phy, pfe_idex_response_type_t type, pfe_idex_seqnum_t seqnum, void *data, uint16_t data_len);
static errno_t pfe_idex_send_frame(pfe_ct_phy_if_id_t dst_phy, pfe_idex_frame_type_t type, void *data, uint16_t data_len);
#ifdef PFE_CFG_PFE_SLAVE
static errno_t pfe_idex_request_send(pfe_ct_phy_if_id_t dst_phy, pfe_idex_request_type_t type, void *data, uint16_t data_len, void *resp, uint16_t resp_len);
#endif /* PFE_CFG_PFE_SLAVE */

/**
 * @brief		IHC client event handler
 * @details		Called by HIF when client-related event happens (packet received, packet
 * 				transmitted).
 */
static errno_t pfe_idex_ihc_handler(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno)
{
	(void)arg;
	(void)qno;

	switch (event)
	{
		case EVENT_RX_PKT_IND:
		{
			/*	Run RX routine */
			pfe_idex_do_rx(client, &pfe_idex);
			break;
		}

		case EVENT_TXDONE_IND:
		{
			/*	Run TX routine */
			pfe_idex_do_tx(client, &pfe_idex);
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
			return EINVAL;
			break;
		}
	}

	return EOK;
}

/**
 * @brief		RX processing
 */
static void pfe_idex_do_rx(pfe_hif_drv_client_t *client, pfe_idex_t *idex)
{
	pfe_hif_pkt_t *pkt;
	pfe_idex_frame_header_t *idex_header;
	pfe_idex_request_t *idex_req;
	pfe_idex_response_t *idex_resp;
	errno_t ret;
	pfe_ct_phy_if_id_t i_phy_id;

	while (TRUE)
	{
		/*	Get received packet */
		pkt = pfe_hif_drv_client_receive_pkt(client, 0U);
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
							NXP_LOG_ERROR("Not implemented\n");
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
							idex->rpc_cbk(i_phy_id, oal_ntohl(rpc_req->rpc_id), rpc_msg_payload_ptr, oal_ntohs(rpc_req->plen), idex->rpc_cbk_arg);

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
						NXP_LOG_ERROR("Not implemented\n");
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
 * @brief		TX confirmations processing
 */
static void pfe_idex_do_tx(pfe_hif_drv_client_t *client, pfe_idex_t *idex)
{
	void *ref_ptr;
	pfe_idex_frame_header_t *idex_header;

    (void)idex;

	while (TRUE)
	{
		/*	Get the transmitted frame reference */
		ref_ptr = pfe_hif_drv_client_receive_tx_conf(client, 0);
		if (NULL == ref_ptr)
		{
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
	}
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
	pfe_idex_t *idex = (pfe_idex_t *)&pfe_idex;
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
 * 				2.) Pass response data
 * 				3.) Marks request as "completed"
 * @param[in]	seqnum Sequence number identifying the request
 * @param[in]	res Value to be passed to the waiting thread as blocking result
 * @param[in]	resp_buf Pointer to response data buffer. If NULL no response is passed.
 * @param[in]	resp_len Number of byte in response buffer
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_request_finalize(pfe_idex_seqnum_t seqnum, pfe_idex_request_result_t res, void *resp_buf, uint16_t resp_len)
{
	pfe_idex_t *idex = (pfe_idex_t *)&pfe_idex;
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
		/*	2.) Copy response data to buffer associated with request */
		if ((NULL != resp_buf) && (NULL != req->resp_buf))
		{
			if (resp_len <= req->resp_buf_len)
			{
				(void)memcpy(req->resp_buf, resp_buf, resp_len);
				req->resp_buf_len = resp_len;
			}
			else
			{
				NXP_LOG_DEBUG("Response buffer is too small\n");
			}
		}

		/*	3.) Mark request as completed */
		req->state = IDEX_REQ_STATE_COMPLETED;
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
	pfe_idex_t *idex = (pfe_idex_t *)&pfe_idex;
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
	resp = oal_mm_malloc_contig_aligned_nocache((addr_t)(sizeof(pfe_idex_response_t)) + (addr_t)data_len, 0U);
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
	(void)memcpy(payload, data, data_len);

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
	pfe_idex_t *idex = (pfe_idex_t *)&pfe_idex;
	pfe_idex_request_t *req;
	errno_t ret;
	void *payload;
	pfe_idex_seqnum_t seqnum;
	uint32_t timeout_us = 1500U * 1000U;
	/*	Wait 1ms */
	const uint32_t timeout_step = 1000U;

	/*	1.) Create the request instance with room for request payload */
	req = oal_mm_malloc_contig_aligned_nocache((addr_t)(sizeof(pfe_idex_request_t)) + (addr_t)data_len, 0U);
	if (NULL == req)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}
	else
	{
		/*	Only initialize header, payload will be added below */
		(void)memset((void *)req, 0, sizeof(pfe_idex_request_t));
	}

	/*	Assign sequence number, type, and destination PHY ID */
	seqnum = oal_htonl(idex->req_seq_num);
	idex->req_seq_num++;
	req->seqnum = seqnum;
	req->type = type;
	req->dst_phy_id = dst_phy;
	req->timeout = IDEX_CFG_REQ_TIMEOUT_SEC;
	req->state = IDEX_REQ_STATE_NEW;
	req->resp_buf = resp;
	req->resp_buf_len = resp_len;

	/*	Add payload */
	payload = (void *)((addr_t)req + sizeof(pfe_idex_request_t));
	(void)memcpy(payload, data, data_len);

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
	if (EOK != pfe_idex_request_set_state(seqnum, IDEX_REQ_STATE_COMMITTED))
	{
		NXP_LOG_WARNING("Transition to IDEX_REQ_STATE_COMMITTED failed\n");
	}

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

		/*	4.) Block until response is received or timeout occurred. RX and
		 	 	TX processing is expected to be done asynchronously in
		 	 	pfe_idex_ihc_handler(). */
		for ( ; timeout_us>0U; timeout_us-=timeout_step)
		{
			if (IDEX_MASTER_DISCOVERY == type)
			{
				NXP_LOG_ERROR("Not implemented\n");
			}
			else
			{
				/*	This is blocking type. We must wait until the request
				 	is completed. */
				if (IDEX_REQ_STATE_COMPLETED == req->state)
				{
					ret = EOK;
					break;
				}
			}

			/*	Wait a bit */
			oal_time_usleep(timeout_step);
		}

		if (0U == timeout_us)
		{
#ifdef IDEX_CFG_VERY_VERBOSE
			NXP_LOG_DEBUG("IDEX request %d timed-out\n", oal_ntohl(req->seqnum));

			if (IDEX_REQ_STATE_COMMITTED == req->state)
			{
				NXP_LOG_DEBUG("Request %d not transmitted\n", oal_ntohl(req->seqnum));
			}
			else if (IDEX_REQ_STATE_TRANSMITTED == req->state)
			{
				NXP_LOG_DEBUG("Request %d not responded\n", oal_ntohl(req->seqnum));
			}
			else
			{
				NXP_LOG_DEBUG("Request %d state is: %d\n", oal_ntohl(req->seqnum), req->state);
			}
#endif /* IDEX_CFG_VERY_VERBOSE */
			ret = ETIMEDOUT;
		}
		else
		{
			/*	Response data is written in 'resp' */
			;
		}

		/*	Release the blocking request instance here */
		if (EOK != oal_mutex_lock(&idex->req_list_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		LLIST_Remove(&req->list_entry);
		oal_mm_free_contig(req);

		if (EOK != oal_mutex_unlock(&idex->req_list_lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
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
	hif_drv_sg_list_t sg_list = { 0U };

	/*	Get IDX frame buffer */
	idex_hdr = oal_mm_malloc_contig_aligned_nocache((addr_t)(sizeof(pfe_idex_frame_header_t)) + (addr_t)data_len, 0U);
	if (NULL == idex_hdr)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}

	idex_hdr_pa = oal_mm_virt_to_phys_contig(idex_hdr);
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
	(void)memcpy(payload, data, data_len);

	/*	Build SG list
	 	TODO: The SG list could be used as reference to all buffers and used to
	 	release them within TX confirmation task when used as 'ref_ptr' argument of
	 	..._ihc_sg_pkt() instead of idex_hdr. */
	sg_list.size = 1U;
	sg_list.dst_phy = dst_phy;
	sg_list.items[0].data_va = idex_hdr;
	sg_list.items[0].data_pa = idex_hdr_pa;
	sg_list.items[0].len = (uint32_t)(sizeof(pfe_idex_frame_header_t)) + (uint32_t)data_len;

	/*	Send it out */
#ifndef PFE_CFG_TARGET_OS_LINUX
	ret = pfe_hif_drv_client_xmit_sg_pkt(pfe_idex.ihc_client, 0U, &sg_list, (void *)idex_hdr);
#else
	ret = pfe_hif_drv_client_xmit_ihc_sg_pkt(pfe_idex.ihc_client, dst_phy, 0U, &sg_list, (void *)idex_hdr);
#endif
	if (EOK != ret)
	{
		NXP_LOG_ERROR("IDEX frame TX failed. Err %u\n", ret);
		oal_mm_free_contig(idex_hdr);
	}
	else
	{
		/*	Frame transmitted. Will be released once TX confirmation is received. */
		;
	}

	return ret;
}

/**
 * @brief		IDEX initialization routine
 * @param[in]	hif_drv The HIF driver instance to be used to transport the data
 * @param[in]	master Physical interface via which the master driver can be reached
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_init(pfe_hif_drv_t *hif_drv, pfe_ct_phy_if_id_t master)
{
	pfe_idex_t *idex = &pfe_idex;
	errno_t ret;

	if (NULL == hif_drv)
	{
		NXP_LOG_ERROR("HIF driver is mandatory\n");
		return EINVAL;
	}

	(void)memset(idex, 0, sizeof(pfe_idex_t));

	idex->req_seq_num = oal_util_rand();

	/*	Here we don't know even own interface ID... */
	idex->master_phy_if = master;
	idex->cur_req_phy_id = PFE_PHY_IF_ID_INVALID;

#ifdef PFE_CFG_PFE_MASTER
	NXP_LOG_INFO("IDEX-master @ interface %d\n", master);
#elif defined(PFE_CFG_PFE_SLAVE)
	NXP_LOG_INFO("IDEX-slave\n");
#else
#error Impossible configuration
#endif /* PFE_CFG_PFE_MASTER/PFE_CFG_PFE_SLAVE */

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

	return EOK;
}

/**
 * @brief		Finalize IDEX module
 */
void pfe_idex_fini(void)
{
	pfe_idex_t *idex = &pfe_idex;

	if (NULL != idex->ihc_client)
	{
		pfe_hif_drv_client_unregister(idex->ihc_client);
		idex->ihc_client = NULL;
	}

#ifdef PFE_CFG_PFE_SLAVE
	{
		LLIST_t *item, *aux;
		pfe_idex_request_t *req;

		/*	Remove all entries remaining in requests storage */
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
#endif /* PFE_CFG_PFE_SLAVE */

	if (TRUE == idex->req_list_lock_init)
	{
		(void)oal_mutex_destroy(&idex->req_list_lock);
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
	pfe_idex_t *idex = &pfe_idex;

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
	pfe_idex_t *idex = &pfe_idex;

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
	pfe_idex_msg_rpc_t *msg = oal_mm_malloc((addr_t)(sizeof(pfe_idex_msg_rpc_t)) + (addr_t)buf_len);
	uint8_t local_resp_buf[256] = { 0U };
	uint16_t msg_plen;
	void *payload;

	if (NULL == msg)
	{
		return ENOMEM;
	}

	msg->rpc_id = oal_htonl(id);
	msg->plen = oal_htons(buf_len);
	msg->rpc_ret = oal_htonl(EOK);

	payload = (void *)((addr_t)msg + sizeof(pfe_idex_msg_rpc_t));
	(void)memcpy(payload, buf, buf_len);

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
				(void)memcpy(resp, payload, msg_plen);

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
	pfe_idex_t *idex = &pfe_idex;
	pfe_idex_msg_rpc_t *rpc_resp;
	pfe_idex_msg_rpc_t *rpc_req;
	void *payload;
	errno_t ret;

	/*	Construct response message */
	rpc_resp = oal_mm_malloc((addr_t)(sizeof(pfe_idex_msg_rpc_t)) + (addr_t)resp_len);

	rpc_req = (pfe_idex_msg_rpc_t *)((addr_t)idex->cur_req + sizeof(pfe_idex_request_t));

	rpc_resp->rpc_id = rpc_req->rpc_id; /* Already in correct endian */
	rpc_resp->plen = oal_htons(resp_len);
	rpc_resp->rpc_ret = oal_htonl(retval);

	payload = (void *)((addr_t)rpc_resp + sizeof(pfe_idex_msg_rpc_t));
	(void)memcpy(payload, resp, resp_len);

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

#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
