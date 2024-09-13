/* =========================================================================
 *  Copyright 2019-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#include "pfe_cfg.h"
#include "oal.h"

#include "linked_list.h"
#include "pfe_hif_drv.h"
#include "pfe_hif.h"
#include "pfe_idex.h"
#include "pfe_platform_cfg.h"

#define IDEX_MAX_CLIENTS	4	/*	Maximum HIF clients to handle in IDEX server */

/*	RESET request/response RPC_ID for IDEX 2.0
	Using for synchronization of sequence number and for IDEX version negotiation 	*/
#define IDEX_RESET_RPC_ID	0xFFFFFFFF

/**
 * @brief		IDEX sequence number type
 */
typedef uint32_t pfe_idex_seqnum_t;

ct_assert(sizeof(pfe_idex_seqnum_t) == sizeof(uint32_t));

/**
 * @brief		IDEX version number for features improvement
 */
typedef enum __attribute__((packed))
{
	IDEX_VERSION_1 = 1U,
	IDEX_VERSION_2 = 2U
} pfe_idex_version_t;

ct_assert(sizeof(pfe_idex_version_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Frame types
 */
typedef enum __attribute__((packed))
{
	/*	Request. Frames of this type are expected to be responded by a remote instance */
	IDEX_FRAME_CTRL_REQUEST = 0U,
	/*	Response. Contains information about remote result */
	IDEX_FRAME_CTRL_RESPONSE = 1U
} pfe_idex_frame_type_t;

ct_assert(sizeof(pfe_idex_frame_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Request types
 */
typedef enum __attribute__((packed))
{
	/*	Master discovery message. Not used or implemented */
	IDEX_MASTER_DISCOVERY = 0U,
	/*	RPC request. Blocking. */
	IDEX_RPC = 1U,
} pfe_idex_request_type_t;

ct_assert(sizeof(pfe_idex_request_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX Response types
 */
typedef pfe_idex_request_type_t pfe_idex_response_type_t;

ct_assert(sizeof(pfe_idex_response_type_t) == sizeof(uint8_t));

/**
 * @brief	IDEX RESET Request/Response for IDEX 2.0
 */
typedef struct __attribute__((packed))
{
	/*	Reset seq number to this value */
	pfe_idex_seqnum_t seqnum;
	/*	Version of IDEX	for backward and forward compability */
	pfe_idex_version_t version;
} pfe_idex_msg_reset_t;

ct_assert(sizeof(pfe_idex_msg_reset_t) == sizeof(uint32_t) + sizeof(uint8_t));

/**
 * @brief	IDEX RPC Message header
 */
typedef struct __attribute__((packed))
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
typedef struct __attribute__((packed))
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
typedef struct __attribute__((packed))
{
	/*	Unique sequence number */
	pfe_idex_seqnum_t seqnum;
	/*	Type of message. Specifies format of the payload. */
	pfe_idex_request_type_t type;
	/*	Destination PHY */
	pfe_ct_phy_if_id_t dst_phy_id;
	/*	Request state */
	pfe_idex_request_state_t state;
	/*	Padding only to keep compatibility, not used */
	uint8_t padding[30U];
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
typedef struct __attribute__((packed))
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
 * @brief	This is IDEX Client structure for Master to save information about Slave
 */
typedef struct
{
	pfe_idex_seqnum_t seqnum;			/*	Current sequence number */
	pfe_idex_version_t version;			/*	IDEX version, for backward compability */
	pfe_ct_phy_if_id_t phy_id;			/*	Physical interface of the client */
	pfe_idex_response_t *response;		/*	Last IDEX response for resending in case of same seqnum */
	pfe_idex_msg_rpc_t rpc_msg;			/*	Current IDEX RPC message request */
} pfe_remote_client_t;

/**
 * @brief	This is IDEX Server structure for Client to save information about Master
 */
typedef struct
{
	pfe_idex_seqnum_t seqnum;			/*	Current sequence number */
	pfe_idex_version_t version;			/*	IDEX version, for backward compability */
	pfe_ct_phy_if_id_t phy_id;			/*	Physical interface of the server */
	oal_mutex_t lock;					/*	Concurrency lock for request/response tasks */
	pfe_idex_request_t *request;		/*	Current IDEX request */
	pfe_idex_msg_rpc_t *rpc_msg;		/*	Current IDEX RPC message response */
	bool_t is_up;						/*	TRUE if IDEX Server connection is UP */
} pfe_remote_server_t;

/**
 * @brief	This is IDEX instance representation type
 */
typedef struct
{
	pfe_hif_drv_client_t *ihc_client;	/*	HIF driver IHC client used for communication */
	pfe_idex_tx_conf_free_cbk_t txc_free_cbk;	/*	Callback to release frame buffers on Tx confirmation */
	pfe_idex_rpc_cbk_t rpc_cbk;			/*	Callback to be called in case of RPC requests */
	void *rpc_cbk_arg;					/*	RPC callback argument */
	pfe_hif_t *hif;						/*	HIF module, for Master-up signaling */
	bool_t is_server;					/*	IDEX is acting as server when TRUE	*/
	bool_t mark_down;					/*	TRUE if driver is shutting down */
	union {
		pfe_remote_server_t server;			/*	Client has Server information */
		pfe_remote_client_t clients[IDEX_MAX_CLIENTS];	/*	Server has information about every client */
	} remote;
	oal_mutex_t rpc_req_lock;			/*	Requests mutex blocking communication */
	bool_t rpc_req_lock_init;			/*	Flag indicating that mutex is initialized */
	uint32_t resend_count;				/*	Transport retransmission count, configuration value */
	uint32_t resend_delays[2];			/*	Transport retransmission delays, configuration value */
	struct {
		uint64_t rx_count;
		uint64_t rx_aliens;
		uint64_t rx_dups;
		uint64_t rx_unknowns;
		uint64_t rx_fails;
		uint64_t tx_count;
		uint64_t tx_retries;
		uint32_t tx_max_retries;
		uint32_t tx_skips;
		uint32_t proto_resets;
	} stats;
} pfe_idex_t;

/*	Local IDEX instance storage */
static pfe_idex_t pfe_idex = {0};
/*	Current client that is waiting for response	*/
static pfe_remote_client_t *idex_current_client;

static void pfe_idex_do_rx(pfe_hif_drv_client_t *client, pfe_idex_t *idex);
static void pfe_idex_do_tx_conf(const pfe_hif_drv_client_t *client, const pfe_idex_t *idex);
static errno_t pfe_idex_send_response(pfe_ct_phy_if_id_t dst_phy, pfe_idex_response_type_t type, pfe_idex_seqnum_t seqnum, const void *data, uint16_t data_len);
static errno_t pfe_idex_send_frame(pfe_ct_phy_if_id_t dst_phy, pfe_idex_frame_type_t type, const void *data, uint16_t data_len);
static errno_t pfe_idex_request_send(pfe_ct_phy_if_id_t dst_phy, pfe_idex_request_type_t type, const uint32_t resend_count, const void *data, uint16_t data_len);
static errno_t pfe_idex_ihc_handler(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno);
static errno_t pfe_idex_set_rpc_cbk(pfe_idex_rpc_cbk_t cbk, void *arg);

/**
 * @brief		IHC client event handler
 * @details		Called by HIF when client-related event happens (packet received, packet
 * 				transmitted).
 */
static errno_t pfe_idex_ihc_handler(pfe_hif_drv_client_t *client, void *arg, uint32_t event, uint32_t qno)
{
	errno_t ret;
	(void)arg;
	(void)qno;
	ret = EOK;

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
			pfe_idex_do_tx_conf(client, &pfe_idex);
			break;
		}

		case EVENT_RX_OOB:
		{
			/*	Out-of-buffers event. Silently ignored. */
			break;
		}

		default:
		{
			NXP_LOG_WARNING("Unexpected IHC event: 0x%x\n", (uint_t)event);
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

/**
 * @brief		RX processing
 */
static void pfe_idex_do_rx(pfe_hif_drv_client_t *hif_client, pfe_idex_t *idex)
{
	pfe_hif_pkt_t *pkt;
	pfe_idex_frame_header_t *idex_header;
	pfe_ct_phy_if_id_t i_phy_id;

	while (TRUE)
	{
		/*	Get received packet */
		pkt = pfe_hif_drv_client_receive_pkt(hif_client, 0U);
		if (NULL == pkt)
		{
			/*	No more received packets */
			break;
		}

		pfe_idex.stats.rx_count++;

		/*	Get RX packet payload. Also skip HIF header. */
		idex_header = (pfe_idex_frame_header_t *)((addr_t)pfe_hif_pkt_get_data(pkt) + sizeof(pfe_ct_hif_rx_hdr_t));

		i_phy_id = pfe_hif_pkt_get_ingress_phy_id(pkt);

		/*	IDEX frames originate from HIF channels exclusively */
		if((i_phy_id < PFE_PHY_IF_ID_HIF0 || i_phy_id > PFE_PHY_IF_ID_HIF3) && i_phy_id != PFE_PHY_IF_ID_HIF_NOCPY)
		{
#ifdef PFE_CFG_PFE_SLAVE
			if (i_phy_id != pfe_idex.remote.server.phy_id)
#endif /* PFE_CFG_PFE_SLAVE */
			{
				NXP_LOG_WARNING("IDEX: Allien IDEX frame type 0x%x with PHY_IF %d", idex_header->type, i_phy_id);
				pfe_idex.stats.rx_aliens++;
				break;
			}
		}

		switch (idex_header->type)
		{
#ifdef PFE_CFG_PFE_MASTER
			case IDEX_FRAME_CTRL_REQUEST:
			{
				pfe_idex_request_t *idex_req;
				pfe_remote_client_t* client;
				pfe_idex_seqnum_t seqnum;
				errno_t ret;

				/*	Received frame is IDEX request */
				idex_req = (pfe_idex_request_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));
				seqnum = (pfe_idex_seqnum_t)oal_ntohl(idex_req->seqnum);
				/*	Current client which we are communicating with */
				client = &idex->remote.clients[i_phy_id - PFE_PHY_IF_ID_HIF0];
				client->phy_id = i_phy_id;

				/*	Save current IDEX client reference to global pointer */
				idex_current_client = client;

				NXP_LOG_DEBUG("Request %u received\n", (uint_t)oal_ntohl(idex_req->seqnum));

				switch (idex_req->type)
				{

					case IDEX_RPC:
					{
						pfe_idex_msg_rpc_t *rpc_req;
						uint32_t rpc_id;
						void *rpc_msg_payload_ptr;

						if (pfe_hif_pkt_get_data_len(pkt) < (sizeof(pfe_ct_hif_rx_hdr_t)
															+ sizeof(pfe_idex_frame_header_t)
															+ sizeof(pfe_idex_request_t)
															+ sizeof(pfe_idex_msg_rpc_t)))
						{
							NXP_LOG_WARNING("Invalid RPC request message length");
							pfe_idex.stats.rx_fails++;
							break;
						}

						rpc_req = (pfe_idex_msg_rpc_t *)((addr_t)idex_req + sizeof(pfe_idex_request_t));
						rpc_id = (uint32_t)oal_ntohl(rpc_req->rpc_id);
						rpc_msg_payload_ptr = (void *)((addr_t)rpc_req + sizeof(pfe_idex_msg_rpc_t));

						/*	IDEX_RESET REQUEST received. Used for seqnumber and version synchronization */
						if(rpc_id == IDEX_RESET_RPC_ID)
						{
							pfe_idex_msg_reset_t *reset_req = (pfe_idex_msg_reset_t *)(rpc_msg_payload_ptr);

							client->seqnum = (pfe_idex_seqnum_t)oal_ntohl(reset_req->seqnum);
							client->version = reset_req->version;
							NXP_LOG_DEBUG("IDEX: RESET Request received: seqnum=%u, version=%u, phy_id=%u",
									(uint_t)client->seqnum, client->version, i_phy_id);

							/*	Send response with same data to acknowledge server version */
							ret = pfe_idex_send_response(client->phy_id, IDEX_RPC, seqnum, rpc_req, sizeof(pfe_idex_msg_rpc_t) + sizeof(pfe_idex_msg_reset_t));
							if(ret != EOK)
							{
								NXP_LOG_WARNING("Problem to send RESET response");
							}

							break;
						}

						NXP_LOG_DEBUG("IDEX: RPC Request received: cmd=%u, plen=%u, seqnum=%u, phy_id=%u",
									rpc_id, (uint16_t)oal_ntohs(rpc_req->plen), (uint_t)oal_ntohl(idex_req->seqnum), i_phy_id);

						/*	In IDEX 2.0 check sequence number */
						if(client->version >= IDEX_VERSION_2)
						{
							/* Duplicated request received, only resend last response */
							if(client->seqnum == seqnum)
							{
								NXP_LOG_DEBUG("IDEX Duplicated RPC request seqnum received: seqnum=%u, phy_id=%u", (uint_t)seqnum, client->phy_id);
								pfe_idex.stats.rx_dups++;
								if(client->response != NULL)
								{
									/*	Resend last saved IDEX frame */
									ret = pfe_idex_send_frame(client->phy_id, IDEX_FRAME_CTRL_RESPONSE, client->response,
												(uint16_t)sizeof(pfe_idex_response_t) + (uint16_t)oal_ntohs(client->response->plen));
									if (EOK != ret)
									{
										NXP_LOG_WARNING("Problem to resend RPC response PHY: %u", client->phy_id);
									}
								}

								break;
							}
							/*	New sequence number received, should be +1. Continue processing */
							else if(client->seqnum +1 == seqnum)
							{
								client->seqnum = seqnum;
							}
							/* Wrong sequence number received */
							else
							{
								NXP_LOG_WARNING("Wrong sequence number %u", (uint_t)seqnum);
								pfe_idex.stats.rx_fails++;
								break;
							}
						}
						else
						{
							client->seqnum = seqnum;
						}

						if (NULL != idex->rpc_cbk)
						{
							/*	Save RPC message for later response generation	*/
							(void)memcpy(&client->rpc_msg, rpc_req, sizeof(pfe_idex_msg_rpc_t));

							/*	Call RPC callback. Response shall be generated inside the callback using the pfe_idex_set_rpc_ret_val(). */
							idex->rpc_cbk(i_phy_id, rpc_id, rpc_msg_payload_ptr, (uint16_t)oal_ntohs(rpc_req->plen), idex->rpc_cbk_arg);
						}
						else
						{
							NXP_LOG_WARNING("RPC callback not found, request %u ignored", (uint_t)oal_ntohl(idex_req->seqnum));
						}

						break;
					}

					default:
					{
						NXP_LOG_WARNING("Unknown IDEX request type received: 0x%x\n", idex_req->type);
						pfe_idex.stats.rx_unknowns++;
						break;
					}
				}

				break;
			} /* IDEX_FRAME_CTRL_REQUEST */

#else

			case IDEX_FRAME_CTRL_RESPONSE:
			{
				pfe_idex_response_t *idex_resp;
				pfe_remote_server_t* server;

				/*	Get response header */
				idex_resp = (pfe_idex_response_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));

				server = &idex->remote.server;

				switch (idex_resp->type)
				{
					/* IDEX_RPC RESPONSE */
					case IDEX_RPC:
					{
						pfe_idex_msg_rpc_t *rpc_msg;
						pfe_idex_msg_rpc_t *rpc_resp;
						pfe_idex_seqnum_t seqnum;
						uint16_t payload_length;

						if (pfe_hif_pkt_get_data_len(pkt) < (sizeof(pfe_ct_hif_rx_hdr_t)
															+ sizeof(pfe_idex_frame_header_t)
															+ sizeof(pfe_idex_response_t)
															+ sizeof(pfe_idex_msg_rpc_t)))
						{
							NXP_LOG_WARNING("Invalid RPC response message length");
							break;
						}

						/*	Response on RCP request. Finalize the associated request message */
						rpc_resp = (pfe_idex_msg_rpc_t*)((addr_t)idex_resp + sizeof(pfe_idex_response_t));

						seqnum = (uint32_t)oal_ntohl(idex_resp->seqnum);
						payload_length = (uint16_t)oal_ntohs(rpc_resp->plen);

						NXP_LOG_DEBUG("IDEX: RPC Response received: cmd=%u, return=%u, plen=%u, seqnum=%u, phy_id=%u",
								(uint32_t)oal_ntohl(rpc_resp->rpc_id), (uint32_t)oal_ntohl(rpc_resp->rpc_ret), payload_length, (uint_t)seqnum, i_phy_id);

						oal_mutex_lock_sleep(&server->lock);

						/*	If there is not waiting any RPC request in buffer, write error and skip processing */
						if(NULL == server->request || NULL == server->rpc_msg)
						{
							NXP_LOG_WARNING("Received IDEX Response without valid RPC Request");
							oal_mutex_unlock(&server->lock);
							break;
						}
						rpc_msg = server->rpc_msg;

						/*	Sequence number in response must be the same as in request */
						if(server->version >= IDEX_VERSION_2 && server->seqnum != seqnum)
						{
							NXP_LOG_DEBUG("IDEX: Wrong sequence number in RPC response: %u!=%u. Skipped", (uint_t)seqnum, (uint_t)server->seqnum);
							if (server->seqnum > seqnum)
							{
								pfe_idex.stats.rx_dups++;
							}
							else
							{
								pfe_idex.stats.rx_fails++;
							}
							oal_mutex_unlock(&server->lock);
							break;
						}

						/*	In rpc_msg->plen is temporarily saved buffer length */
						if (payload_length <= rpc_msg->plen)
						{
							/*	Copy payload */
							(void)memcpy((void *)((addr_t)rpc_msg + sizeof(pfe_idex_msg_rpc_t)),
										(void *)((addr_t)rpc_resp + sizeof(pfe_idex_msg_rpc_t)), payload_length);

							rpc_msg->rpc_id = (uint32_t)oal_ntohl(rpc_resp->rpc_id);
							rpc_msg->rpc_ret = (uint32_t)oal_ntohl(rpc_resp->rpc_ret);
							rpc_msg->plen = payload_length;

							server->request->state = IDEX_REQ_STATE_COMPLETED;
						}
						else
						{
							/*	Don't send response if there is no room for required payload */
							NXP_LOG_WARNING("RPC Response buffer is too small! %u < %u", payload_length, rpc_msg->plen);
							server->request->state = IDEX_REQ_STATE_INVALID;
							pfe_idex.stats.rx_fails++;
						}

						oal_mutex_unlock(&server->lock);

						break;
					}/* IDEX_RPC RESPONSE */

					default:
					{
						NXP_LOG_WARNING("Unknown IDEX response type received: 0x%x\n", idex_resp->type);
						break;
					}
				}

				break;
			} /* IDEX_FRAME_CTRL_RESPONSE */
#endif /* PFE_CFG_PFE_MASTER/SLAVE */

			default:
			{
				/*	Unknown frame */
				NXP_LOG_WARNING("Unknown IDEX frame ctrl type 0x%x received\n", idex_header->type);
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
static void pfe_idex_do_tx_conf(const pfe_hif_drv_client_t *client, const pfe_idex_t *idex)
{
	const pfe_idex_frame_header_t *idex_header;
	void *ref_ptr;

	while (TRUE)
	{
		/*	Get the transmitted frame reference */
		ref_ptr = pfe_hif_drv_client_receive_tx_conf(client, 0);
		if (NULL == ref_ptr)
		{
			break;
		}

		idex_header = (pfe_idex_frame_header_t *)ref_ptr;
		switch (idex_header->type)
		{
			case IDEX_FRAME_CTRL_REQUEST:
			{
				pfe_idex_request_t *req_header = (pfe_idex_request_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));
				NXP_LOG_DEBUG("Request %u transmitted\n", (uint_t)oal_ntohl(req_header->seqnum));
				break;
			}

			case IDEX_FRAME_CTRL_RESPONSE:
			{
				pfe_idex_response_t *resp_header = (pfe_idex_response_t *)((addr_t)idex_header + sizeof(pfe_idex_frame_header_t));
				NXP_LOG_DEBUG("Response %u transmitted\n", (uint_t)oal_ntohl(resp_header->seqnum));
				break;
			}

			default:
			{
				NXP_LOG_WARNING("Unknown IDEX frame transmitted\n");
				break;
			}
		}

		/*	Free packet memory */
		if (NULL == idex->txc_free_cbk)
		{
			oal_mm_free_contig(ref_ptr);
		}
		else
		{
			idex->txc_free_cbk(ref_ptr);
		}
	}
}

/**
 * @brief		Send IDEX response message to client with data
 * @param[in]	dst_phy Destination physical interface ID of client
 * @param[in]	type Response type. Should match request type.
 * @param[in]	seqnum Sequence number. Should match request sequence number.
 * @param[in]	data Response payload buffer to send
 * @param[in]	data_len Response payload length in number of bytes
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_send_response(pfe_ct_phy_if_id_t dst_phy, pfe_idex_response_type_t type, pfe_idex_seqnum_t seqnum, const void *data, uint16_t data_len)
{
	pfe_idex_response_t *resp;
	errno_t ret;
	void *payload;

	/*	If there is response in buffer from old request, clean it */
	if(idex_current_client->response != NULL){
		/*	Release the response instance */
		oal_mm_free_contig(idex_current_client->response);
		idex_current_client->response = NULL;
	}

	/*	Create the response buffer with room for request payload */
	resp = oal_mm_malloc_contig_aligned_nocache((addr_t)(sizeof(pfe_idex_response_t)) + (addr_t)data_len, 0U);
	if (NULL == resp)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		ret = ENOMEM;
	}
	else
	{
		/*	Add seqnum and type */
		resp->seqnum = (uint32_t)oal_htonl(seqnum);
		resp->type = type;
		resp->plen = (uint16_t)oal_htons(data_len);

		/*	Add payload */
		payload = (void *)((addr_t)resp + sizeof(pfe_idex_response_t));
		(void)memcpy(payload, data, data_len);

		/*	Send it out within IDEX frame */
		ret = pfe_idex_send_frame(dst_phy, IDEX_FRAME_CTRL_RESPONSE, resp, ((uint16_t)sizeof(pfe_idex_response_t) + data_len));
		if (EOK != ret)
		{
			NXP_LOG_WARNING("IDEX response TX failed\n");
		}

		/*	Save response in case of not successful delivery */
		idex_current_client->response = resp;
	}
	return ret;
}

/**
 * @brief		Create, send IDEX request and wait for response
 * @details		THIS IS BLOCKING FUNCTION!
 * 				Slave will create IDEX request and send it to Master
 * 				Function is waiting for response with Pooling mode on request status *
 * @param[in]	dst_phy Destination physical interface ID
 * @param[in]	resend_count Number of retries for sending request
 * @param[in]	type Request type
 * @param[in]	data Request payload buffer
 * @param[in]	data_len Request payload length in number of bytes
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_request_send(pfe_ct_phy_if_id_t dst_phy, pfe_idex_request_type_t type, const uint32_t resend_count, const void *data, uint16_t data_len)
{
	pfe_remote_server_t	*server = (pfe_remote_server_t*)&pfe_idex.remote.server;
	void *				payload;
	uint32_t			timeout_ms;
	uint32_t			sending_counter;
	uint32_t			resend_delay;
	pfe_idex_request_t *request;
	errno_t				ret;

	/*	Create the request instance with room for request payload */
	request = oal_mm_malloc_contig_aligned_nocache((addr_t)(sizeof(pfe_idex_request_t)) + (addr_t)data_len, 0U);
	if (NULL == request)
	{
		NXP_LOG_ERROR("Unable to allocate memory");
		return ENOMEM;
	}

	oal_mutex_lock_sleep(&server->lock);

	/*	Only initialize header, payload will be added below */
	(void)memset((void *)request, 0, sizeof(pfe_idex_request_t));

	/*	Assign sequence number, type, and destination PHY ID */
	request->seqnum = (uint32_t)oal_htonl(server->seqnum);
	request->type = type;
	request->dst_phy_id = dst_phy;
	request->state = IDEX_REQ_STATE_NEW;

	/*	Add payload */
	payload = (void *)((addr_t)request + sizeof(pfe_idex_request_t));
	(void)memcpy(payload, data, data_len);

	server->request = request;

	oal_mutex_unlock(&server->lock);

	/*	Mark request as commited and start sending */
	request->state = IDEX_REQ_STATE_COMMITTED;

	/*	Sending request. Try to send configured number of times	 */
	for(sending_counter = 0; sending_counter < resend_count; sending_counter++)
	{
		/*	Request transmitted. Will be released once it is processed. */
		ret = pfe_idex_send_frame(dst_phy, IDEX_FRAME_CTRL_REQUEST, request, ((uint16_t)sizeof(pfe_idex_request_t) + data_len));
		if (EOK != ret)
		{
			/*	Sending of request failed. Should return ERROR */
			NXP_LOG_ERROR("IDEX request %d TX failed", request->seqnum);
			goto end_sending;
		}

		/*	Block until response is received or timeout occurred. RX and
			TX processing is expected to be done asynchronously in pfe_idex_ihc_handler(). */

		/* Progressive retry delay: Fist 3 tries can wait independently of the rest retries */
		if (3U > sending_counter)
		{
			resend_delay = pfe_idex.resend_delays[0];
		}
		else
		{
			resend_delay = pfe_idex.resend_delays[1];
		}

		/*	Wait 1ms between every check */
		for (timeout_ms = resend_delay; timeout_ms > 0U; timeout_ms -= 1)
		{
			/*	Check the status of request */
			if (IDEX_REQ_STATE_COMPLETED == request->state)
			{
				/* Request succesfully completed, we should increment counter and stop sending */
				ret = EOK;
				goto end_sending;
			}
			else if (IDEX_REQ_STATE_INVALID == request->state)
			{
				/* Request failed */
				NXP_LOG_ERROR("IDEX request %d TX in invalid state", request->seqnum);
				ret = EFAULT;
				goto end_sending;
			}
			else
			{
				/*	Wait 1ms */
				oal_time_udelay(1000U);
			}
		}

		NXP_LOG_DEBUG("IDEX RESENDING REQUEST seqnum=%d counter=%d state=%d", server->seqnum, sending_counter + 1, request->state);

		/*	IDEX protocol stats */
		pfe_idex.stats.tx_retries++;
		if (pfe_idex.stats.tx_max_retries < (sending_counter + 1))
		{
			pfe_idex.stats.tx_max_retries = sending_counter + 1;
		}
	}

	/*	Sending was not succesfull, timeout occured */
	if (0U == timeout_ms || resend_count == sending_counter)
	{
		NXP_LOG_WARNING("IDEX request %u timed-out, retransmitted %u times", (uint_t)oal_ntohl(request->seqnum), sending_counter);
		ret = ETIMEDOUT;
	}

end_sending:
	oal_mutex_lock_sleep(&server->lock);

	/*	End of sending, increment seqnum */
	server->seqnum += 1;

	if (EOK != ret)
	{
		/* Send was unsuccessful, mark connection down */
		pfe_idex.remote.server.is_up = FALSE;
	}
	server->request = NULL;
	oal_mm_free_contig(request);

	oal_mutex_unlock(&server->lock);

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
static errno_t pfe_idex_send_frame(pfe_ct_phy_if_id_t dst_phy, pfe_idex_frame_type_t type, const void *data, uint16_t data_len)
{
	pfe_idex_frame_header_t *idex_hdr, *idex_hdr_pa;
	void *payload;
	errno_t ret;
	hif_drv_sg_list_t sg_list = { 0U };
	uint16_t data_len_tmp = data_len;

	/*	Get IDEX frame buffer */
	idex_hdr = oal_mm_malloc_contig_named_aligned_nocache(PFE_CFG_TX_MEM, (addr_t)(sizeof(pfe_idex_frame_header_t)) + (addr_t)data_len_tmp, 0U);
	if (NULL == idex_hdr)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		ret = ENOMEM;
	}
	else
	{
		idex_hdr_pa = oal_mm_virt_to_phys_contig(idex_hdr);
		if (NULL == idex_hdr_pa)
		{
			NXP_LOG_ERROR("IDEX frame VA to PA conversion failed\n");
			oal_mm_free_contig(idex_hdr);
			ret = ENOMEM;
		}
		else
		{
			/*	Fill the header */
			idex_hdr->dst_phy_if = dst_phy;
			idex_hdr->type = type;

			/*	Add payload */
			payload = (void *)((addr_t)idex_hdr + sizeof(pfe_idex_frame_header_t));
			(void)memcpy(payload, data, data_len_tmp);

			/*	Build SG list
				TODO: The SG list could be used as reference to all buffers and used to
				release them within TX confirmation task when used as 'ref_ptr' argument of
				..._ihc_sg_pkt() instead of idex_hdr. */
			sg_list.size = 1U;
			sg_list.dst_phy = dst_phy;
			sg_list.items[0].data_va = idex_hdr;
			sg_list.items[0].data_pa = idex_hdr_pa;
			sg_list.items[0].len = (uint32_t)(sizeof(pfe_idex_frame_header_t)) + (uint32_t)data_len_tmp;

			/*	Send it out */
			ret = pfe_hif_drv_client_xmit_sg_pkt(pfe_idex.ihc_client, 0U, &sg_list, (void *)idex_hdr);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("IDEX frame TX failed. Code %u\n", ret);
				oal_mm_free_contig(idex_hdr);
			}
			else
			{
				/*	Frame transmitted. Will be released once TX confirmation is received. */
				pfe_idex.stats.tx_count++;
			}
		}
	}

	return ret;
}

/**
 * @brief		Set IDEX RPC callback
 * @details		The callback will be called at any time when RPC request
 * 				will be received.
 * @param[in]	cbk Callback to be called
 * @param[in]	arg Custom argument to be passed to the callback
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_set_rpc_cbk(pfe_idex_rpc_cbk_t cbk, void *arg)
{
	pfe_idex_t *idex = &pfe_idex;

	idex->rpc_cbk_arg = arg;
	idex->rpc_cbk = cbk;

	return EOK;
}

#ifdef PFE_CFG_PFE_SLAVE
/**
 * @brief		IDEX client send RESET request message
 * @details		Client will synchronize current sequence number
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_idex_send_reset_request(pfe_idex_t *idex)
{
	errno_t ret;
	pfe_idex_msg_reset_t rst_msg;

	rst_msg.seqnum = (uint32_t)oal_htonl(idex->remote.server.seqnum);
	rst_msg.version = IDEX_VERSION_2;

	NXP_LOG_DEBUG("IDEX: RESET Request sending: seqnum=%u, version=%u, phy_id=%u",
								(uint_t)idex->remote.server.seqnum, rst_msg.version, idex->remote.server.phy_id);

	/*	Sending RESET request. This is blocking communication	*/
	ret = pfe_idex_rpc(idex->remote.server.phy_id, IDEX_RESET_RPC_ID, &rst_msg, sizeof(pfe_idex_msg_reset_t), &rst_msg, sizeof(pfe_idex_msg_reset_t));
	if(EOK != ret)
	{
		/* IDEX Reset was not succesfull. Client will use Legacy configuration */
		NXP_LOG_WARNING("IDEX: RESET Request was not successful [%d]", ret);
	}
	else
	{
		idex->remote.server.version = rst_msg.version;
		idex->stats.proto_resets++;
		NXP_LOG_DEBUG("IDEX: RESET Response received: %d IDEX v%d protocol in use", (uint_t)idex->remote.server.seqnum, rst_msg.version);
	}

	return ret;
}
#endif

/**
 * @brief		IDEX initialization routine
 * @details		The callback will be called at any time when RPC request
 * 				will be received.
 * @param[in]	hif_drv The HIF driver instance to be used to transport the data
 * @param[in]	master Physical interface via which the master driver can be reached
 * @param[in]	hif The Platform HIF instance
 * @param[in]	cbk Callback to be called
 * @param[in]	arg Custom argument to be passed to the callback
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_idex_init(pfe_hif_drv_t *hif_drv, pfe_ct_phy_if_id_t master, pfe_hif_t *hif,
			pfe_idex_rpc_cbk_t cbk, void *arg, pfe_idex_tx_conf_free_cbk_t txcf_cbk)
{
	pfe_idex_t *idex = &pfe_idex;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == hif_drv) || (NULL == hif))
	{
		NXP_LOG_ERROR("NULL argument received");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)memset(idex, 0, sizeof(pfe_idex_t));

#ifdef PFE_CFG_PFE_MASTER
	/*	IDEX is in Server mode */
	NXP_LOG_INFO("IDEX-master @ interface %d", master);

	idex->is_server = TRUE;
	idex->hif = hif;

#elif defined(PFE_CFG_PFE_SLAVE)
	/* IDEX is Client	*/
	NXP_LOG_INFO("IDEX-slave @ master-interface %d", master);

	idex->is_server = FALSE;
	/*	Set init seqnum to 0	*/
	idex->remote.server.seqnum = 0;
	idex->remote.server.phy_id = master;
	idex->remote.server.version = IDEX_VERSION_1;
	idex->hif = hif;

#else
#error Impossible configuration
#endif /* PFE_CFG_PFE_MASTER/PFE_CFG_PFE_SLAVE */

	pfe_hif_drv_get_idex_resend_cfg(hif_drv, &idex->resend_count, idex->resend_delays);
	idex->txc_free_cbk = txcf_cbk;

	/*	Create mutex */
	ret = oal_mutex_init(&idex->rpc_req_lock);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Mutex init failed");
		pfe_idex_fini();
		return ret;
	}
	ret = oal_mutex_init(&idex->remote.server.lock);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Mutex init failed");
		pfe_idex_fini();
		return ret;
	}

	idex->rpc_req_lock_init = TRUE;

	/*	Register IHC client */
	idex->ihc_client = pfe_hif_drv_ihc_client_register(hif_drv, &pfe_idex_ihc_handler, NULL);
	if (NULL == idex->ihc_client)
	{
		NXP_LOG_ERROR("Can't register IHC client");
		pfe_idex_fini();
		ret = EFAULT;
	}
	else
	{
		ret = pfe_idex_set_rpc_cbk(cbk, arg);
		if (EOK == ret)
		{
#ifdef PFE_CFG_PFE_MASTER
			/*	Mark MASTER with ready flag */
			pfe_hif_set_master_up(hif);

#elif defined(PFE_CFG_PFE_SLAVE)
			idex->remote.server.is_up = TRUE;
			idex->mark_down = FALSE;

			/*	Send RESET request message to server */
			pfe_idex_send_reset_request(idex);

			if (IDEX_VERSION_2 == idex->remote.server.version)
			{
				NXP_LOG_INFO("IDEX: v2 protocol used, Resend:count=%d,delay=%d,%dms\n", idex->resend_count, idex->resend_delays[0], idex->resend_delays[1]);
			}
			else
			{
				NXP_LOG_INFO("IDEX: v1 (legacy) protocol used\n");
			}

#endif /* PFE_CFG_PFE_MASTER/PFE_CFG_PFE_SLAVE */
		}
	}

	return ret;
}

/**
 * @brief		Finalize IDEX module
 */
void pfe_idex_fini(void)
{
	pfe_idex_t *idex = &pfe_idex;
	uint8_t i;

#ifdef PFE_CFG_PFE_MASTER
	pfe_hif_clear_master_up(idex->hif);
	idex->hif = NULL;
#endif /* PFE_CFG_PFE_MASTER */

	idex->rpc_cbk = NULL;
	idex->rpc_cbk_arg = NULL;
	idex->txc_free_cbk = NULL;

	if (NULL != idex->ihc_client)
	{
		pfe_hif_drv_client_unregister(idex->ihc_client);
		idex->ihc_client = NULL;
	}

	/*	Free IDEX Server buffer for every client response */
	if(idex->is_server == TRUE)
	{
		for(i = 0; i < IDEX_MAX_CLIENTS; i++)
		{
			if(idex->remote.clients[i].response != NULL)
			{
				oal_mm_free_contig(idex->remote.clients[i].response);
				idex->remote.clients[i].response = NULL;
			}
		}
	}

	if (TRUE == idex->rpc_req_lock_init)
	{
		(void)oal_mutex_destroy(&idex->remote.server.lock);
		(void)oal_mutex_destroy(&idex->rpc_req_lock);
		idex->rpc_req_lock_init = FALSE;
	}
}

/**
 * @brief		Execute RPC against IDEX master. Blocking call.
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
errno_t pfe_idex_master_rpc(uint32_t id, const void *buf, uint16_t buf_len, void *resp, uint16_t resp_len)
{
	pfe_idex_t *idex = &pfe_idex;
	errno_t ret = EOK;

#ifdef PFE_CFG_PFE_SLAVE
	/* Check if Master IP flag is up to prevent communication without Server running */
	if (NULL == idex->hif || FALSE == pfe_hif_get_master_up(idex->hif))
	{
		NXP_LOG_WARNING("IDEX: Master driver is not running, skip IDEX request");
		idex->stats.tx_skips++;
		ret = EPERM;
	}
	/* Check if IDEX had error in last communication */
	else if (FALSE == idex->remote.server.is_up)
	{
		if(TRUE == idex->mark_down)
		{
			/* Driver is shutting down and Server was not reachable in last request */
			/* Skip RPC and return error */
			idex->stats.tx_skips++;
			ret = EPERM;
		}
		else {
			/* Try to send RESET request for recovery of communication */
			ret = pfe_idex_send_reset_request(idex);
			if(EOK == ret)
			{
				/* Server is now responding, continue with communication */
				idex->remote.server.is_up = TRUE;
			}
			else
			{
				/* Server did not respond on RESET, skip this RPC and return error */
				idex->stats.tx_skips++;
			}
		}
	}
#endif

	if(EOK == ret)
	{
		/* RPC message can be sent to Master only from IDEX client */
		if(FALSE == idex->is_server)
		{
			ret = pfe_idex_rpc(idex->remote.server.phy_id, id, buf, buf_len, resp, resp_len);
		}
		else
		{
			ret = EPERM;
		}
	}

	return ret;
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
errno_t pfe_idex_rpc(pfe_ct_phy_if_id_t dst_phy, uint32_t id, const void *buf, uint16_t buf_len, void *resp, uint16_t resp_len)
{
	errno_t ret;
	void *payload;
	const uint16_t request_buf_size = (uint16_t)sizeof(pfe_idex_msg_rpc_t) + buf_len;
	const uint16_t response_buf_size = (uint16_t)sizeof(pfe_idex_msg_rpc_t) + resp_len;
	/*	Allocate memory for request and also response */
	pfe_idex_msg_rpc_t *msg_req = (pfe_idex_msg_rpc_t *)oal_mm_malloc((addr_t)request_buf_size);
	pfe_idex_msg_rpc_t *msg_resp = (pfe_idex_msg_rpc_t *)oal_mm_malloc((addr_t)response_buf_size);
	/*	If we have version 2 or RESET request message, try to resend multiple times */
	const uint32_t resend_count = ((pfe_idex.remote.server.version >= IDEX_VERSION_2) || (id == IDEX_RESET_RPC_ID)) ? pfe_idex.resend_count : 1U;

	if (NULL == msg_req || NULL == msg_resp)
	{
		NXP_LOG_ERROR("Unable to allocate memory");
		return ENOMEM;
	}

	/*	Blocking RCP communication for all threads during processing */
	oal_mutex_lock_sleep(&pfe_idex.rpc_req_lock);

	msg_req->rpc_id = (uint32_t)oal_htonl(id);
	msg_req->plen = (uint16_t)oal_htons(buf_len);
	msg_req->rpc_ret = (uint32_t)oal_htonl(EOK);

	/*	Set buffer for expected RPC response */
	msg_resp->plen = resp_len;
	pfe_idex.remote.server.rpc_msg = msg_resp;

	/*	Copy data to payload of request message */
	payload = (void *)((addr_t)msg_req + sizeof(pfe_idex_msg_rpc_t));
	(void)memcpy(payload, buf, buf_len);

	NXP_LOG_DEBUG("IDEX: RPC Request sending: cmd=%u, seqnum=%u, phy_id=%u, size:%u",
				id, pfe_idex.remote.server.seqnum, dst_phy, buf_len);

	/*	This one is BLOCKING function */
	ret = pfe_idex_request_send(dst_phy, IDEX_RPC, resend_count, msg_req, request_buf_size);
	if (EOK != ret)
	{
		/*	Transport error */
		NXP_LOG_WARNING("RPC transport failed: %d", ret);
	}
	else
	{
		/*	Sanity checks */
		if (id != msg_resp->rpc_id)
		{
			NXP_LOG_WARNING("RPC response ID does not match the request %u != %u", id, msg_req->rpc_id);
			pfe_idex.stats.rx_fails++;
			ret = EINVAL;
		}
		else
		{
			/*	Check the remote return value from the response */
			ret = msg_resp->rpc_ret;

			/*	Copy RPC response data to caller's buffer */
			if (0U == msg_resp->plen)
			{
				NXP_LOG_DEBUG("RPC response without payload received");
			}
			else if (msg_resp->plen > resp_len) /* if the response is too big */
			{
				NXP_LOG_ERROR("Caller's buffer is too small");
				ret = ENOMEM;
			}
			else /* there is response, it is not too big and we have buffer */
			{
				payload = (void *)((addr_t)msg_resp + sizeof(pfe_idex_msg_rpc_t));
				(void)memcpy(resp, payload, msg_resp->plen);

				NXP_LOG_DEBUG("%d bytes of RPC response received", msg_resp->plen);
			}
		}
	}

	oal_mm_free(msg_req);
	oal_mm_free(msg_resp);
	pfe_idex.remote.server.rpc_msg = NULL;

	/*	Unlock mutex and unblock driver	*/
	if (EOK != oal_mutex_unlock(&pfe_idex.rpc_req_lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed");
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
	pfe_remote_client_t *client = idex_current_client;
	pfe_idex_msg_rpc_t *rpc_resp;
	void *payload;
	errno_t ret;

	rpc_resp = oal_mm_malloc((addr_t)(sizeof(pfe_idex_msg_rpc_t)) + (addr_t)resp_len);
	if (NULL == rpc_resp)
	{
		NXP_LOG_ERROR("Unable to allocate memory");
		ret = ENOMEM;
	}
	else
	{
		/*	Construct response message */
		rpc_resp->rpc_id = client->rpc_msg.rpc_id; /* Already in correct endian */
		rpc_resp->plen = oal_htons(resp_len);
		rpc_resp->rpc_ret = oal_htonl(retval);

		payload = (void *)((addr_t)rpc_resp + sizeof(pfe_idex_msg_rpc_t));
		(void)memcpy(payload, resp, resp_len);

		NXP_LOG_DEBUG("IDEX: RPC Response sending: cmd=%u, seqnum=%u, resp_len=%u, retval=%u",
				(uint_t)oal_ntohl(rpc_resp->rpc_id), (uint_t)client->seqnum, resp_len, retval);

		/*	Send the response */
		ret = pfe_idex_send_response(
										client->phy_id,		/* Destination */
										IDEX_RPC,			/* Response type */
										client->seqnum,	/* Response sequence number */
										rpc_resp,				/* Response payload */
										((uint16_t)sizeof(pfe_idex_msg_rpc_t) + resp_len) /* Response payload length */
									);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("IDEX RPC response failed");
		}

		/*	Dispose the response buffer */
		oal_mm_free(rpc_resp);
	}
	return ret;
}

/**
 * @brief		Mark IDEX communication channel as DOWN
 */
void pfe_idex_down(void)
{
	oal_mutex_lock_sleep(&pfe_idex.remote.server.lock);
	pfe_idex.remote.server.is_up = FALSE;
	oal_mutex_unlock(&pfe_idex.remote.server.lock);
}

/**
 * @brief		Get IDEX channel status for testing
 */
bool_t pfe_idex_status(void)
{
	return pfe_idex.remote.server.is_up;
}

/**
 * @brief		Inform IDEX communication that driver is shutting down
 * @details		This flag is usable when Master driver crashes and Slave
 * 				needs to shutdown faster without RPC communication
 */
void pfe_idex_mark_down(void)
{
	pfe_idex.mark_down = TRUE;
}

/**
 * @brief               Return IDEX runtime statistics in text form
 * @param[in]   seq                     Pointer to debugfs seq_file
 * @param[in]   verb_level      Verbosity level, number of data written to the buffer
 * @return              Number of bytes written to the buffe
 */
__attribute__((cold, aligned(64))) void pfe_idex_get_text_statistics(struct seq_file *seq, uint8_t verb_level)
{
        seq_printf(seq, "rx_count:       %llu\n", pfe_idex.stats.rx_count);
        seq_printf(seq, "rx_aliens:      %llu\n", pfe_idex.stats.rx_aliens);
        seq_printf(seq, "rx_dups:        %llu\n", pfe_idex.stats.rx_dups);
        seq_printf(seq, "rx_unknowns:    %llu\n", pfe_idex.stats.rx_unknowns);
        seq_printf(seq, "rx_fails:       %llu\n", pfe_idex.stats.rx_fails);
        seq_printf(seq, "tx_count:       %llu\n", pfe_idex.stats.tx_count);
#ifdef PFE_CFG_PFE_SLAVE
        seq_printf(seq, "tx_retries:     %llu\n", pfe_idex.stats.tx_retries);
        seq_printf(seq, "tx_max_retries: %u\n", pfe_idex.stats.tx_max_retries);
        seq_printf(seq, "tx_skips:       %u\n", pfe_idex.stats.tx_skips);
        seq_printf(seq, "proto_resets :  %u\n", pfe_idex.stats.proto_resets);
#endif /* PFE_CFG_PFE_SLAVE */
}
