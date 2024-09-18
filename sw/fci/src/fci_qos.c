/* =========================================================================
 *  Copyright 2020-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_qos.c
 * @brief		QoS management
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_phy_if_t *fci_get_phy_if_by_name(char_t *name);
static pfe_gpi_t *fci_qos_get_gpi(const pfe_phy_if_t *phy_if);
static errno_t fci_validate_cmd_params(const fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len, uint32_t cmd_len);
static void fci_qos_flow_entry_convert_to_gpi(const fpp_iqos_flow_spec_t *flow, pfe_iqos_flow_spec_t *gpi_flow);
static void fci_qos_flow_entry_convert_from_gpi(const pfe_iqos_flow_spec_t *gpi_flow, fpp_iqos_flow_spec_t *flow);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef NXP_LOG_ENABLED
#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CONST_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/*
 * 	This is storage of scheduler algorithms ordered in way
 *	as defined by the FCI (see fpp_ext.h::fpp_qos_scheduler_cmd_t)
 */
static const char_t * const sch_algos_str[] = {"SCHED_ALGO_PQ", "SCHED_ALGO_DWRR", "SCHED_ALGO_RR", "SCHED_ALGO_WRR"};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
#endif /* NXP_LOG_ENABLED */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: fci_qos_queue_cmd*/
static const pfe_tmu_queue_mode_t fci_qmode_to_qmode[] =
    {TMU_Q_MODE_INVALID, TMU_Q_MODE_DEFAULT, TMU_Q_MODE_TAIL_DROP, TMU_Q_MODE_WRED};

/* usage scope: fci_qos_scheduler_cmd */
static const pfe_tmu_sched_algo_t sch_algos[] = {SCHED_ALGO_PQ, SCHED_ALGO_DWRR, SCHED_ALGO_RR, SCHED_ALGO_WRR};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_phy_if_t *fci_get_phy_if_by_name(char_t *name)
{
	fci_t *fci = (fci_t *)&__context;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	errno_t ret;
	uint32_t sid;

	ret = pfe_if_db_polled_lock(&sid);
	if (EOK != ret)
	{
		NXP_LOG_WARNING("Could not lock interface DB: %d\n", ret);
	}
	else
	{
		ret = pfe_if_db_get_first(fci->phy_if_db, sid, IF_DB_CRIT_BY_NAME, name, &entry);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Interface DB query failed: %d\n", ret);
		}

		if(NULL != entry)
		{
			phy_if = pfe_if_db_entry_get_phy_if(entry);
		}

		ret = pfe_if_db_unlock(sid);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Interface DB unlock failed: %d\n", ret);
		}
	}

	return phy_if;
}

/**
 * @brief			Process FPP_CMD_QOS_QUEUE command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_qos_queue_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 */
errno_t fci_qos_queue_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_queue_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_queue_cmd_t *q;
	errno_t ret = EOK;
	pfe_phy_if_t *phy_if = NULL;
	const fci_t *fci = (fci_t *)&__context;
	uint8_t cnt, ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (*reply_len < sizeof(fpp_qos_queue_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_qos_queue_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_qos_queue_cmd_t));
			q = (fpp_qos_queue_cmd_t *)msg->msg_cmd.payload;

			switch(q->action)
			{
				case FPP_ACTION_UPDATE:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(q->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}
					else if (PFE_PHY_IF_ID_UTIL == pfe_phy_if_get_id(phy_if))
					{
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
						ret = EOK;
						break;
					}
					else
					{
						; /*	required by MISRA */
					}

					/*	Check queue ID */
					cnt = pfe_tmu_queue_get_cnt(fci->tmu, pfe_phy_if_get_id(phy_if));
					if (q->id > cnt)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						NXP_LOG_WARNING("Queue ID %d out of range. Interface %s implements %d queues\n",
								(int_t)q->id, q->if_name, (int_t)cnt);
						*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
						ret = EOK;
						break;
					}

					NXP_LOG_DEBUG("Setting queue %d mode: %d (min: %d, max: %d)\n",
							(int_t)q->id, (int_t)q->mode, (int_t)oal_ntohl(q->min), (int_t)oal_ntohl(q->max));

					if (q->mode > 3U)
					{
						/* FCI command has wrong data. Respond with FCI error code. */
						NXP_LOG_WARNING("Unsupported queue mode: %d\n", q->mode);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						ret = EOK;
						break;
					}
					else
					{
						if (q->mode == 0U)
						{
							/*	Disable the queue to drop all packets */
							ret = pfe_tmu_queue_set_mode(fci->tmu, pfe_phy_if_get_id(phy_if), q->id,
									TMU_Q_MODE_TAIL_DROP, 0U, 0U);
						}
						else
						{
							ret = pfe_tmu_queue_set_mode(fci->tmu, pfe_phy_if_get_id(phy_if), q->id,
								fci_qmode_to_qmode[q->mode], oal_ntohl(q->min), oal_ntohl(q->max));
						}

						if (EOK != ret)
						{
							if (ENOSPC == ret)
							{
								/* FCI command requested unfulfillable action. Respond with FCI error code. */
								NXP_LOG_WARNING("Refused to set max length of %s queue %d to %u, because then the sum of %s queue lengths would exceed allowed total limit.\n", pfe_phy_if_get_name(phy_if), q->id, (uint_t)oal_ntohl(q->max), pfe_phy_if_get_name(phy_if));
								*fci_ret = FPP_ERR_QOS_QUEUE_SUM_OF_LENGTHS_EXCEEDED;
								ret = EOK;
							}
							else
							{
								/* FCI command has wrong data. Respond with FCI error code. */
								NXP_LOG_WARNING("Could not set queue %d mode %d: %d\n", q->id, q->mode, ret);
								*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
								ret = EOK;
							}
							break;
						}

						if (q->mode == 3U)
						{
							NXP_LOG_DEBUG("Setting WRED zones probabilities\n");

							cnt = pfe_tmu_queue_get_cnt(fci->tmu, pfe_phy_if_get_id(phy_if));

							if (cnt > 32U)
							{
								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Invalid zones count...\n");
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								ret = EINVAL;
								break;
							}

							for (ii=0U; ii<cnt; ii++)
							{
								NXP_LOG_DEBUG("Setting queue %d zone %d probability %d%%\n",
										(int_t)q->id, (int_t)ii, (int_t)q->zprob[ii]);
								ret = pfe_tmu_queue_set_wred_prob(fci->tmu,
										pfe_phy_if_get_id(phy_if), q->id, ii, q->zprob[ii]);
								if (EOK != ret)
								{
									/* FCI command has wrong data. Respond with FCI error code. */
									NXP_LOG_WARNING("Could not set queue %d zone %d probability %d: %d\n",
											(int_t)q->id, (int_t)ii, (int_t)q->zprob[ii], (int_t)ret);
									*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
									ret = EOK;
									break; /* for */
								}
							}
						}
					}

					break;
				}

				case FPP_ACTION_QUERY:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(q->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Check queue */
					ret = pfe_tmu_check_queue(fci->tmu, pfe_phy_if_get_id(phy_if), q->id);
					if (EOK != ret)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Copy original command properties into reply structure */
					reply_buf->action = q->action;
					reply_buf->id = q->id;
					(void)strncpy(reply_buf->if_name, q->if_name, sizeof(reply_buf->if_name));
					reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

					/*	Get queue mode */
					switch (pfe_tmu_queue_get_mode(fci->tmu, pfe_phy_if_get_id(phy_if),
							q->id, &reply_buf->min, &reply_buf->max))
					{
						case TMU_Q_MODE_TAIL_DROP:
						{
							if (reply_buf->max == 0U)
							{
								reply_buf->mode = 0U; /* Disabled */
								reply_buf->max = 0U;
								reply_buf->min = 0U;
							}
							else
							{
								reply_buf->mode = 2U; /* Tail Drop */
								reply_buf->max = oal_htonl(reply_buf->max);
								reply_buf->min = 0U;
							}

							break;
						}

						case TMU_Q_MODE_DEFAULT:
						{
							reply_buf->mode = 1U; /* Default */
							reply_buf->max = oal_htonl(reply_buf->max);
							reply_buf->max = oal_htonl(reply_buf->min);
							break;
						}

						case TMU_Q_MODE_WRED:
						{
							reply_buf->mode = 3U; /* WRED */
							reply_buf->max = oal_htonl(reply_buf->max);
							reply_buf->min = oal_htonl(reply_buf->min);

							/*	Get zone probabilities */
							cnt = pfe_tmu_queue_get_wred_zones(fci->tmu, pfe_phy_if_get_id(phy_if), q->id);
							for (ii=0U; ii<32U; ii++)
							{
								if (ii < cnt)
								{
									ret = pfe_tmu_queue_get_wred_prob(
											fci->tmu, pfe_phy_if_get_id(phy_if), q->id, ii, &reply_buf->zprob[ii]);
									if (EOK != ret)
									{
										/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
										NXP_LOG_ERROR("Could not get queue %d zone %d probability: %d\n", (int_t)q->id, (int_t)ii, (int_t)ret);
										*fci_ret = FPP_ERR_INTERNAL_FAILURE;
										break; /* for */
									}
								}
								else
								{
									reply_buf->zprob[ii] = 255; /* Invalid */
								}
							}

							break;
						}

						default:
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Can't get queue %d mode\n", q->id);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}

					*reply_len = sizeof(fpp_qos_queue_cmd_t);
					break;
				}

				default:
				{
					NXP_LOG_WARNING("FPP_CMD_QOS_QUEUE: Unknown action received: 0x%x\n", q->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief			Process FPP_CMD_QOS_SCHEDULER command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_qos_scheduler_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 */
errno_t fci_qos_scheduler_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_scheduler_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_scheduler_cmd_t *sch;
	errno_t ret = EOK;
	pfe_phy_if_t *phy_if = NULL;
	const fci_t *fci = (fci_t *)&__context;
	uint8_t ii, cnt, queue;
	uint32_t weight;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (*reply_len < sizeof(fpp_qos_scheduler_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_qos_scheduler_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_qos_scheduler_cmd_t));
			sch = (fpp_qos_scheduler_cmd_t *)msg->msg_cmd.payload;

			switch(sch->action)
			{
				case FPP_ACTION_UPDATE:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(sch->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Check scheduler */
					ret = pfe_tmu_check_scheduler(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id);
					if (EOK != ret)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_QOS_SCHEDULER_NOT_FOUND;
						ret = EOK;
						break;
					}
					else if (PFE_PHY_IF_ID_UTIL == pfe_phy_if_get_id(phy_if))
					{
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
						ret = EOK;
						break;
					}
					else
					{
						; /*	required by MISRA */
					}

					/*	Set scheduler mode */
					if (0U == sch->mode)
					{
						NXP_LOG_INFO("Disabling all scheduler %d inputs\n", sch->id);
						/*	Mark all inputs as disabled. Change will be applied below. */
						sch->input_en = 0U;
						ret = EOK;
					}
					else if (1U == sch->mode)
					{
						NXP_LOG_INFO("Setting scheduler %d mode: Data rate\n", sch->id);
						ret = pfe_tmu_sch_set_rate_mode(fci->tmu,
								pfe_phy_if_get_id(phy_if), sch->id, RATE_MODE_DATA_RATE);
					}
					else if (2U == sch->mode)
					{
						NXP_LOG_INFO("Setting scheduler %d mode: Packet rate\n", sch->id);
						ret = pfe_tmu_sch_set_rate_mode(fci->tmu,
								pfe_phy_if_get_id(phy_if), sch->id, RATE_MODE_PACKET_RATE);
					}
					else
					{
						NXP_LOG_WARNING("Unsupported scheduler mode: 0x%x\n", sch->mode);
						ret = EINVAL;
					}

					if (EOK != ret)
					{
						/* FCI command has wrong data. Respond with FCI error code. */
						NXP_LOG_WARNING("Scheduler mode not set: %d\n", ret);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						ret = EOK;
						break;
					}

					/*	Set scheduler algorithm */
					if (sch->algo > 3U)
					{
						/* FCI command has wrong data. Respond with FCI error code. */
						NXP_LOG_WARNING("Unsupported scheduler algorithm: 0x%x\n", sch->algo);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						ret = EOK;
						break;
					}

					NXP_LOG_INFO("Setting scheduler %d algorithm: %s\n",
							sch->id, sch_algos_str[sch->algo]);
					ret = pfe_tmu_sch_set_algo(fci->tmu, pfe_phy_if_get_id(phy_if),
							sch->id, sch_algos[sch->algo]);
					if (EOK != ret)
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						NXP_LOG_WARNING("Scheduler algorithm not set: %d\n", ret);
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						break;
					}

					/*	Configure scheduler inputs */
					cnt = pfe_tmu_sch_get_input_cnt(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id);
					sch->input_en = oal_ntohl(sch->input_en);
					for (ii=0U; ii<cnt; ii++)
					{
						if ((0U == (((uint32_t)1U << ii) & sch->input_en)) || (sch->input_src[ii] == 255U))
						{
							NXP_LOG_DEBUG("Disabling scheduler %d input %d\n", (int_t)sch->id, (int_t)ii);
							ret = pfe_tmu_sch_bind_queue(fci->tmu, pfe_phy_if_get_id(phy_if),
										sch->id, ii, PFE_TMU_INVALID_QUEUE);
							if (EOK != ret)
							{
								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Could not invalidate scheduler input %d: %d\n", (int_t)ii, (int_t)ret);
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break; /* for */
							}
						}
						else
						{
							if (sch->input_src[ii] < 8U)
							{
								NXP_LOG_DEBUG("Connecting source %d to scheduler %d input %d\n",
										(int_t)sch->input_src[ii], (int_t)sch->id, (int_t)ii);
								ret = pfe_tmu_sch_bind_queue(fci->tmu, pfe_phy_if_get_id(phy_if),
										sch->id, ii, sch->input_src[ii]);
								if (EOK != ret)
								{
									/* FCI command has wrong data. Respond with FCI error code. */
									NXP_LOG_WARNING("Could not connect source %d to scheduler input %d\n",
											(int_t)sch->input_src[ii], (int_t)ii);
									*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
									ret = EOK;
									break; /* for */
								}
							}
							else if (sch->input_src[ii] == 8U)
							{
								NXP_LOG_DEBUG("Connecting scheduler %d output to scheduler %d input %d\n",
										(int_t)(sch->id-1U), (int_t)sch->id, (int_t)ii);
								ret = pfe_tmu_sch_bind_sch_output(fci->tmu, pfe_phy_if_get_id(phy_if),
										sch->id-1U, sch->id, ii);
								if (EOK != ret)
								{
									/* FCI command has wrong data. Respond with FCI error code. */
									NXP_LOG_WARNING("Could not connect scheduler %d output to scheduler %d input %d: %d\n",
											(int_t)(sch->id-1U), (int_t)sch->id, (int_t)ii, (int_t)ret);
									*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
									ret = EOK;
									break; /* for */
								}
							}
							else
							{
								/* FCI command has wrong data. Respond with FCI error code. */
								NXP_LOG_WARNING("Unsupported scheduler input %d source: %d\n", (int_t)ii, (int_t)sch->input_src[ii]);
								*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
								ret = EOK;
								break; /* for */
							}

							NXP_LOG_DEBUG("Setting scheduler %d input %d weight: %d\n",
									(int_t)sch->id, (int_t)ii, (int_t)oal_ntohl(sch->input_w[ii]));
							ret = pfe_tmu_sch_set_input_weight(fci->tmu, pfe_phy_if_get_id(phy_if),
									sch->id, ii, oal_ntohl(sch->input_w[ii]));
							if (EOK != ret)
							{
								/* FCI command has wrong data. Respond with FCI error code. */
								NXP_LOG_WARNING("Could not set scheduler %d input %d weight %d: %d\n",
										(int_t)sch->id, (int_t)ii, (int_t)oal_ntohl(sch->input_w[ii]), (int_t)ret);
								*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
								ret = EOK;
								break; /* for */
							}
						}
					}

					break;
				}

				case FPP_ACTION_QUERY:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(sch->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Check scheduler */
					ret = pfe_tmu_check_scheduler(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id);
					if (EOK != ret)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_QOS_SCHEDULER_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Copy original command properties into reply structure */
					reply_buf->action = sch->action;
					reply_buf->id = sch->id;
					(void)strncpy(reply_buf->if_name, sch->if_name, sizeof(reply_buf->if_name));
					reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

					/*	Get scheduler mode */
					switch (pfe_tmu_sch_get_rate_mode(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id))
					{
						case RATE_MODE_DATA_RATE:
						{
							reply_buf->mode = 1U;
							break;
						}

						case RATE_MODE_PACKET_RATE:
						{
							reply_buf->mode = 2U;
							break;
						}

						default:
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Can't get scheduler %d mode or the mode is invalid\n", sch->id);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							ret = EINVAL;
							break;
						}
					}

					/*	Get scheduler algo */
					reply_buf->algo = (uint8_t)pfe_tmu_sch_get_algo(fci->tmu,
							pfe_phy_if_get_id(phy_if), sch->id);
					if (reply_buf->algo == (uint8_t)SCHED_ALGO_INVALID)
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						NXP_LOG_ERROR("Can't get scheduler %d algo or the algo is invalid\n", sch->id);
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						ret = EINVAL;
						break;
					}

					/*	Get enabled inputs and associated sources. See the Egress QoS chapter in FCI doc. */
					cnt = pfe_tmu_sch_get_input_cnt(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id);
					reply_buf->input_en = 0U;
					for (ii=0U; ii<cnt; ii++)
					{
						queue = pfe_tmu_sch_get_bound_queue(fci->tmu, pfe_phy_if_get_id(phy_if),
								sch->id, ii);
						if (PFE_TMU_INVALID_QUEUE == queue)
						{
							if (PFE_TMU_INVALID_SCHEDULER == pfe_tmu_sch_get_bound_sch_output(fci->tmu,
									pfe_phy_if_get_id(phy_if), sch->id, ii))
							{
								/*	Scheduler input 'ii' is not connected */
								reply_buf->input_src[ii] = 255U;
							}
							else
							{
								/*	Scheduler input 'ii' is connected to prepend scheduler output */
								weight = pfe_tmu_sch_get_input_weight(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id, ii);
								reply_buf->input_w[ii] = oal_htonl(weight);
								reply_buf->input_src[ii] = 8U;
								reply_buf->input_en |= ((uint32_t)1U << ii);
							}
						}
						else
						{
							/*	Scheduler input 'ii' is connected to queue */
							weight = pfe_tmu_sch_get_input_weight(fci->tmu, pfe_phy_if_get_id(phy_if), sch->id, ii);
							reply_buf->input_w[ii] = oal_htonl(weight);
							reply_buf->input_src[ii] = queue;
							reply_buf->input_en |= ((uint32_t)1U << ii);
						}
					}

					/*	Maintain endianness as given by FCI doc */
					reply_buf->input_en = oal_htonl(reply_buf->input_en);

					*reply_len = sizeof(fpp_qos_scheduler_cmd_t);
					break;
				}

				default:
				{
					NXP_LOG_WARNING("FPP_CMD_QOS_SCHEDULER: Unknown action received: 0x%x\n", sch->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}

		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_QOS_SHAPER command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_qos_shaper_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 */
errno_t fci_qos_shaper_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_shaper_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_shaper_cmd_t *shp;
	errno_t ret = EOK;
	pfe_phy_if_t *phy_if = NULL;
	const fci_t *fci = (fci_t *)&__context;
	uint32_t isl;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (*reply_len < sizeof(fpp_qos_shaper_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_qos_shaper_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;

			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_qos_shaper_cmd_t));
			shp = (fpp_qos_shaper_cmd_t *)msg->msg_cmd.payload;

			switch(shp->action)
			{
				case FPP_ACTION_UPDATE:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(shp->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}
					else if (PFE_PHY_IF_ID_UTIL == pfe_phy_if_get_id(phy_if))
					{
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
						ret = EOK;
						break;
					}
					else
					{
						; /*	required by MISRA */
					}

					/*	Check shaper */
					ret = pfe_tmu_check_shaper(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);
					if (EOK != ret)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_QOS_SHAPER_NOT_FOUND;
						ret = EOK;
						break;
					}

					if (0U == shp->mode)
					{
						if (255U == shp->position)
						{
							NXP_LOG_DEBUG("Disconnecting shaper %d\n", shp->id);
							ret = pfe_tmu_shp_set_position(fci->tmu, pfe_phy_if_get_id(phy_if),
									shp->id, PFE_TMU_INVALID_POSITION);
							if (EOK != ret)
							{
								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Could not disconnect shaper %d: %d\n", shp->id, ret);
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}
						}

						NXP_LOG_DEBUG("Disabling shaper %d\n", shp->id);
						ret = pfe_tmu_shp_disable(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);
						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Could not disable shaper %d: %d\n", shp->id, ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}
					else
					{
						NXP_LOG_DEBUG("Enabling shaper %d\n", shp->id);
						ret = pfe_tmu_shp_enable(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);
						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Could not enable shaper %d: %d\n", shp->id, ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						NXP_LOG_DEBUG("Setting shaper %d rate mode %d\n", shp->id, shp->mode);
						if (1U == shp->mode)
						{
							ret = pfe_tmu_shp_set_rate_mode(fci->tmu, pfe_phy_if_get_id(phy_if),
								shp->id, RATE_MODE_DATA_RATE);
						}
						else if (2U == shp->mode)
						{
							ret = pfe_tmu_shp_set_rate_mode(fci->tmu, pfe_phy_if_get_id(phy_if),
								shp->id, RATE_MODE_PACKET_RATE);
						}
						else
						{
							/* FCI command has wrong data. Respond with FCI error code. */
							NXP_LOG_WARNING("Invalid shaper rate mode value: %d\n", shp->mode);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Unable to set shaper %d rate mode %d: %d\n",
									shp->id, shp->mode, ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						NXP_LOG_DEBUG("Setting shaper %d credit limits %d-%d\n",
								(int_t)shp->id, (int_t)oal_ntohl(shp->max_credit), (int_t)oal_ntohl(shp->min_credit));
						ret = pfe_tmu_shp_set_limits(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id,
								(int32_t)oal_ntohl(shp->max_credit), (int32_t)oal_ntohl(shp->min_credit));
						if (EOK != ret)
						{
							/* FCI command has wrong data. Respond with FCI error code. */
							NXP_LOG_WARNING("Unable to set shaper %d limits: %d\n", shp->id, ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						NXP_LOG_DEBUG("Setting shaper %d position to %d\n", shp->id, shp->position);
						ret = pfe_tmu_shp_set_position(fci->tmu, pfe_phy_if_get_id(phy_if),
								shp->id, shp->position);
						if (EOK != ret)
						{
							/* FCI command has wrong data. Respond with FCI error code. */
							NXP_LOG_WARNING("Can't set shaper %d at position %d: %d\n",
									shp->id, shp->position, ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						NXP_LOG_DEBUG("Setting shaper %d idle slope: %d\n",
								(int_t)shp->id, (int_t)oal_ntohl(shp->isl));
						ret = pfe_tmu_shp_set_idle_slope(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id,
								oal_ntohl(shp->isl));
						if (EOK != ret)
						{
							/* FCI command has wrong data. Respond with FCI error code. */
							NXP_LOG_WARNING("Can't set shaper %d idle slope %d: %d\n",
									(int_t)shp->id, (int_t)oal_ntohl(shp->isl), (int_t)ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}
					}

					break;
				}

				case FPP_ACTION_QUERY:
				{
					*fci_ret = FPP_ERR_OK;

					/*	Get physical interface ID */
					phy_if = fci_get_phy_if_by_name(shp->if_name);
					if (NULL == phy_if)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Check shaper */
					ret = pfe_tmu_check_shaper(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);
					if (EOK != ret)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_QOS_SHAPER_NOT_FOUND;
						ret = EOK;
						break;
					}

					/*	Copy original command properties into reply structure */
					reply_buf->action = shp->action;
					reply_buf->id = shp->id;
					(void)strncpy(reply_buf->if_name, shp->if_name, sizeof(reply_buf->if_name));
					reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

					/*	Get shaper mode */
					switch (pfe_tmu_shp_get_rate_mode(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id))
					{
						case RATE_MODE_DATA_RATE:
						{
							reply_buf->mode = 1U;
							break;
						}

						case RATE_MODE_PACKET_RATE:
						{
							reply_buf->mode = 2U;
							break;
						}

						default:
						{
							/*	Shaper is disabled or the query failed */
							reply_buf->mode = 0U;
							break;
						}
					}

					/*	Get credit limits */
					ret = pfe_tmu_shp_get_limits(fci->tmu, pfe_phy_if_get_id(phy_if),
							shp->id, &reply_buf->max_credit, &reply_buf->min_credit);
					if (ret != EOK)
					{
						NXP_LOG_ERROR("Could not get shaper %d limits: %d\n", shp->id, ret);
					}
					else
					{
						/*	Ensure expected endianness */
						reply_buf->max_credit = (int32_t)oal_htonl(reply_buf->max_credit);
						reply_buf->min_credit = (int32_t)oal_htonl(reply_buf->min_credit);
					}

					/*	Get idle slope */
					isl = pfe_tmu_shp_get_idle_slope(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);
					reply_buf->isl = oal_htonl(isl);

					/*	Get shaper position */
					reply_buf->position =
							pfe_tmu_shp_get_position(fci->tmu, pfe_phy_if_get_id(phy_if), shp->id);

					*reply_len = sizeof(fpp_qos_shaper_cmd_t);
					break;
				}

				default:
				{
					NXP_LOG_WARNING("FPP_CMD_QOS_SHAPER: Unknown action received: 0x%x\n", shp->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}
		}
	}
	return ret;
}

static pfe_gpi_t *fci_qos_get_gpi(const pfe_phy_if_t *phy_if)
{
	const pfe_emac_t *emac = pfe_phy_if_get_emac(phy_if);
	pfe_gpi_t *gpi = NULL;

	if (NULL != emac)
	{
		  gpi = pfe_emac_get_gpi(emac);
	}

	return gpi;
}

static errno_t fci_validate_cmd_params(const fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len, uint32_t cmd_len)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)msg;
		(void)fci_ret;
		(void)reply_buf;
		if (*reply_len < cmd_len)
		{
			NXP_LOG_WARNING("Buffer length does not match command lenght\n");
			ret = EINVAL;
		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_QOS_POLICER command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_qos_policer_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 */
errno_t fci_qos_policer_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_policer_cmd_t *pol_cmd;
	pfe_phy_if_t *phy_if = NULL;
	pfe_gpi_t *gpi = NULL;
	errno_t ret = EOK;

	ret = fci_validate_cmd_params(msg, fci_ret, reply_buf, reply_len, sizeof(*pol_cmd));
	if (EOK == ret)
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;

		/*	Initialize the reply buffer */
		(void)memset(reply_buf, 0, sizeof(*reply_buf));

		pol_cmd = (fpp_qos_policer_cmd_t *)msg->msg_cmd.payload;

		/* get from phy_if to gpi */
		phy_if = fci_get_phy_if_by_name(pol_cmd->if_name);
		if (NULL == phy_if)
		{
			*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
			ret = ENOENT;
		}
		else
		{
			gpi = fci_qos_get_gpi(phy_if);
			if (NULL == gpi)
			{
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = ENOENT;
			}
			else
			{
				*fci_ret = FPP_ERR_OK;

				switch(pol_cmd->action)
				{
					case FPP_ACTION_UPDATE:
					{
						if (0U != pol_cmd->enable)
						{
							ret = pfe_gpi_qos_enable(gpi);
						}
						else
						{
							ret = pfe_gpi_qos_disable(gpi);
						}

						if (EOK != ret)
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}

						break;
					}

					case FPP_ACTION_QUERY:
					{
						/*	Copy original command properties into reply structure */
						reply_buf->action = pol_cmd->action;
						(void)strncpy(reply_buf->if_name, pol_cmd->if_name, sizeof(reply_buf->if_name));
						reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

						/*	get policer data */
						reply_buf->enable = pfe_gpi_qos_is_enabled(gpi);
						*reply_len = sizeof(*pol_cmd);
						break;
					}

					default:
					{
						NXP_LOG_WARNING("FPP_CMD_QOS_POLICER: Unknown action received: 0x%x\n", pol_cmd->action);
						*fci_ret = FPP_ERR_UNKNOWN_ACTION;
						break;
					}
				}
			}
		}
	}

	return ret;
}

static void fci_qos_flow_entry_convert_to_gpi(const fpp_iqos_flow_spec_t *flow, pfe_iqos_flow_spec_t *gpi_flow)
{
	gpi_flow->type_mask = (pfe_iqos_flow_type_t)oal_ntohs(flow->type_mask);
	gpi_flow->arg_type_mask = (pfe_iqos_flow_arg_type_t)oal_ntohs(flow->arg_type_mask);
	gpi_flow->action = (pfe_iqos_flow_action_t)flow->action;

	gpi_flow->args.vlan = (uint16_t)oal_ntohs(flow->args.vlan);
	gpi_flow->args.vlan_m = (uint16_t)oal_ntohs(flow->args.vlan_m);
	gpi_flow->args.sport_max = (uint16_t)oal_ntohs(flow->args.sport_max);
	gpi_flow->args.sport_min = (uint16_t)oal_ntohs(flow->args.sport_min);
	gpi_flow->args.dport_max = (uint16_t)oal_ntohs(flow->args.dport_max);
	gpi_flow->args.dport_min = (uint16_t)oal_ntohs(flow->args.dport_min);

	gpi_flow->args.sip = (uint32_t)oal_ntohl(flow->args.sip);
	gpi_flow->args.dip = (uint32_t)oal_ntohl(flow->args.dip);

	gpi_flow->args.tos = flow->args.tos;
	gpi_flow->args.tos_m = flow->args.tos_m;
	gpi_flow->args.l4proto = flow->args.l4proto;
	gpi_flow->args.l4proto_m = flow->args.l4proto_m;
	gpi_flow->args.sip_m = flow->args.sip_m;
	gpi_flow->args.dip_m = flow->args.dip_m;
}

static void fci_qos_flow_entry_convert_from_gpi(const pfe_iqos_flow_spec_t *gpi_flow, fpp_iqos_flow_spec_t *flow)
{
	flow->type_mask = (fpp_iqos_flow_type_t)oal_htons(gpi_flow->type_mask);
	flow->arg_type_mask = (fpp_iqos_flow_arg_type_t)oal_htons(gpi_flow->arg_type_mask);
	flow->action = (fpp_iqos_flow_action_t)gpi_flow->action;

	flow->args.vlan = (uint16_t)oal_htons(gpi_flow->args.vlan);
	flow->args.vlan_m = (uint16_t)oal_htons(gpi_flow->args.vlan_m);
	flow->args.sport_max = (uint16_t)oal_htons(gpi_flow->args.sport_max);
	flow->args.sport_min = (uint16_t)oal_htons(gpi_flow->args.sport_min);
	flow->args.dport_max = (uint16_t)oal_htons(gpi_flow->args.dport_max);
	flow->args.dport_min = (uint16_t)oal_htons(gpi_flow->args.dport_min);

	flow->args.sip = (uint32_t)oal_htonl(gpi_flow->args.sip);
	flow->args.dip = (uint32_t)oal_htonl(gpi_flow->args.dip);

	flow->args.tos = gpi_flow->args.tos;
	flow->args.tos_m = gpi_flow->args.tos_m;
	flow->args.l4proto = gpi_flow->args.l4proto;
	flow->args.l4proto_m = gpi_flow->args.l4proto_m;
	flow->args.sip_m = gpi_flow->args.sip_m;
	flow->args.dip_m = gpi_flow->args.dip_m;
}

/**
 * @brief			Process FPP_CMD_QOS_POLICER_FLOW command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_qos_policer_flow_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 */
errno_t fci_qos_policer_flow_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_flow_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_policer_flow_cmd_t *flow_cmd;
	pfe_iqos_flow_spec_t gpi_flow;
	pfe_phy_if_t *phy_if = NULL;
	pfe_gpi_t *gpi = NULL;
	errno_t ret = EOK;

    (void)memset(&gpi_flow, 0, sizeof(pfe_iqos_flow_spec_t));
	ret = fci_validate_cmd_params(msg, fci_ret, reply_buf, reply_len, sizeof(*flow_cmd));
	if (EOK == ret)
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
		/*	Initialize the reply buffer */
		(void)memset(reply_buf, 0, sizeof(*reply_buf));

		flow_cmd = (fpp_qos_policer_flow_cmd_t *)msg->msg_cmd.payload;

		/* get from phy_if to gpi */
		phy_if = fci_get_phy_if_by_name(flow_cmd->if_name);
		if (NULL == phy_if)
		{
			*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
			ret = ENOENT;
		}
		else
		{
			gpi = fci_qos_get_gpi(phy_if);
			if (NULL == gpi)
			{
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = ENOENT;
			}
			else
			{
				*fci_ret = FPP_ERR_OK;

				switch(flow_cmd->action)
				{
					case FPP_ACTION_REGISTER:
					{
						/* populate gpi flow struct */
						fci_qos_flow_entry_convert_to_gpi(&flow_cmd->flow, &gpi_flow);

						/* commit configuration to H/W */
						ret = pfe_gpi_qos_add_flow(gpi, flow_cmd->id, &gpi_flow);
						if (EOVERFLOW == ret)
						{
							*fci_ret = FPP_ERR_QOS_POLICER_FLOW_TABLE_FULL;
							break;
						}
						else if (EINVAL == ret)
						{
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break;
						}
						else if (EOK != ret)
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
						else
						{
							;/* Required by Misra */
						}
						break;
					}
					case FPP_ACTION_DEREGISTER:
					{
						if (flow_cmd->id >= PFE_IQOS_FLOW_TABLE_SIZE)
						{
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break;
						}

						ret = pfe_gpi_qos_rem_flow(gpi, flow_cmd->id);
						if (ret != EOK)
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}

						break;
					}
					case FPP_ACTION_QUERY:
					{
						/*	Copy original command properties into reply structure */
						reply_buf->action = flow_cmd->action;
						(void)strncpy(reply_buf->if_name, flow_cmd->if_name, sizeof(reply_buf->if_name));
						reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

						ret = pfe_gpi_qos_get_first_flow(gpi, &reply_buf->id, &gpi_flow);
						if (ret != EOK)
						{
							*fci_ret = FPP_ERR_QOS_POLICER_FLOW_NOT_FOUND;
							ret = EOK;
							break;
						}

						/* populate fci flow struct */
						fci_qos_flow_entry_convert_from_gpi(&gpi_flow, &reply_buf->flow);

						*reply_len = sizeof(*flow_cmd);
						break;
					}

					case FPP_ACTION_QUERY_CONT:
					{
						/*	Copy original command properties into reply structure */
						reply_buf->action = flow_cmd->action;
						(void)strncpy(reply_buf->if_name, flow_cmd->if_name, sizeof(reply_buf->if_name));
						reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';

						ret = pfe_gpi_qos_get_next_flow(gpi, &reply_buf->id, &gpi_flow);
						if (ret != EOK)
						{
							*fci_ret = FPP_ERR_QOS_POLICER_FLOW_NOT_FOUND;
							ret = EOK;
							break;
						}

						/* populate fci flow struct */
						fci_qos_flow_entry_convert_from_gpi(&gpi_flow, &reply_buf->flow);

						*reply_len = sizeof(*flow_cmd);
						break;
					}
					default:
					{
						NXP_LOG_WARNING("FPP_CMD_QOS_POLICER_FLOW: Unknown action received: 0x%x\n", flow_cmd->action);
						*fci_ret = FPP_ERR_UNKNOWN_ACTION;
						break;
					}
				}
			}
		}
	}

	return ret;
}

errno_t fci_qos_policer_wred_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_wred_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_policer_wred_cmd_t *wred_cmd;
	fpp_iqos_queue_t queue;
	uint16_t wred_thr;
	pfe_phy_if_t *phy_if;
	pfe_gpi_t *gpi;
	errno_t ret;
	uint32_t i;

	ret = fci_validate_cmd_params(msg, fci_ret, reply_buf, reply_len, sizeof(*wred_cmd));
	if (EOK == ret)
	{
		/* no data written to reply buffer (yet) */
		*reply_len = 0U;

		/* initialize the reply buffer */
		(void)memset(reply_buf, 0, sizeof(*reply_buf));

		/* map command structure to message payload (requires casting) */
		wred_cmd = (fpp_qos_policer_wred_cmd_t *)msg->msg_cmd.payload;

		/* get from phy_if to gpi */
		phy_if = fci_get_phy_if_by_name(wred_cmd->if_name);
		if (NULL == phy_if)
		{
			*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
			ret = ENOENT;
		}
		else
		{
			gpi = fci_qos_get_gpi(phy_if);
			if (NULL == gpi)
			{
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = ENOENT;
			}
			else
			{
				/* basic command validations */
				queue = wred_cmd->queue;
				if (queue >= FPP_IQOS_Q_COUNT)
				{
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					ret = EINVAL;
				}
				else
				{
					*fci_ret = FPP_ERR_OK;

					switch(wred_cmd->action)
					{
						case FPP_ACTION_UPDATE:
						{
							if (0U != wred_cmd->enable)
							{
								ret = pfe_gpi_wred_enable(gpi, (pfe_iqos_queue_t)queue);
							}
							else
							{
								/* exit configuration update on disable */
								ret = pfe_gpi_wred_disable(gpi, (pfe_iqos_queue_t)queue);
								if (EOK != ret)
								{
									*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								}
								break;
							}

							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							for (i = 0; i < (uint32_t)FPP_IQOS_WRED_THR_COUNT; i++)
							{
								wred_thr = oal_ntohs(wred_cmd->thr[i]);
								if (PFE_IQOS_WRED_THR_SKIP == wred_thr)
								{
									/* continue to next thr */
								}
								else
								{
									ret = pfe_gpi_wred_set_thr(gpi, (pfe_iqos_queue_t)queue, (pfe_iqos_wred_thr_t)i, wred_thr);
									if (EOK != ret)
									{
										break;
									}
								}
							}

							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							for (i = 0; i < (uint32_t)FPP_IQOS_WRED_ZONES_COUNT; i++)
							{
								if (PFE_IQOS_WRED_ZONE_PROB_SKIP == wred_cmd->zprob[i])
								{
									/* continue to next prob zone */
								}
								else
								{
									ret = pfe_gpi_wred_set_prob(gpi, (pfe_iqos_queue_t)queue, (pfe_iqos_wred_zone_t)i, wred_cmd->zprob[i]);
									if (EOK != ret)
									{
										break;
									}
								}
							}

							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							break;
						}

						case FPP_ACTION_QUERY:
						{
							/* copy original command properties into reply structure */
							reply_buf->action = wred_cmd->action;
							(void)strncpy(reply_buf->if_name, wred_cmd->if_name, sizeof(reply_buf->if_name));
							reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';
							reply_buf->queue = queue;

							/* get WRED data */
							reply_buf->enable = pfe_gpi_wred_is_enabled(gpi, (pfe_iqos_queue_t)queue);

							for (i = 0; i < (uint32_t)FPP_IQOS_WRED_THR_COUNT; i++)
							{
								ret = pfe_gpi_wred_get_thr(gpi, (pfe_iqos_queue_t)queue, (pfe_iqos_wred_thr_t)i, &wred_thr);
								if (EOK != ret)
								{
									*fci_ret = FPP_ERR_INTERNAL_FAILURE;
									break;
								}
								reply_buf->thr[i] = oal_htons(wred_thr);
							}

							for (i = 0; i < (uint32_t)FPP_IQOS_WRED_ZONES_COUNT; i++)
							{
								ret = pfe_gpi_wred_get_prob(gpi, (pfe_iqos_queue_t)queue, (pfe_iqos_wred_zone_t)i, &reply_buf->zprob[i]);
								if (EOK != ret)
								{
									*fci_ret = FPP_ERR_INTERNAL_FAILURE;
									break;
								}
							}

							*reply_len = sizeof(*wred_cmd);
							break;
						}

						default:
						{
							NXP_LOG_WARNING("FPP_CMD_QOS_POLICER_WRED: Unknown action received: 0x%x\n", wred_cmd->action);
							*fci_ret = FPP_ERR_UNKNOWN_ACTION;
							break;
						}
					}
				}
			}
		}
	}

	return ret;
}

errno_t fci_qos_policer_shp_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_shp_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_qos_policer_shp_cmd_t *shp_cmd;
	uint8_t shp_id;
	fpp_iqos_shp_type_t shp_type;
	fpp_iqos_shp_rate_mode_t shp_mode;
	int32_t shp_max_credit = 0;
	int32_t shp_min_credit = 0;
	uint32_t  shp_isl;
	pfe_phy_if_t *phy_if;
	pfe_gpi_t *gpi;
	errno_t ret;

    (void)memset(&shp_type, 0, sizeof(fpp_iqos_shp_type_t));
    (void)memset(&shp_mode, 0, sizeof(fpp_iqos_shp_rate_mode_t));
	ret = fci_validate_cmd_params(msg, fci_ret, reply_buf, reply_len, sizeof(*shp_cmd));
	if (EOK == ret)
	{
		/* no data written to reply buffer (yet) */
		*reply_len = 0U;
		/* initialize the reply buffer */
		(void)memset(reply_buf, 0, sizeof(*reply_buf));

		/* map command structure to message payload (requires casting) */
		shp_cmd = (fpp_qos_policer_shp_cmd_t *)msg->msg_cmd.payload;

		/* get from phy_if to gpi */
		phy_if = fci_get_phy_if_by_name(shp_cmd->if_name);
		if (NULL == phy_if)
		{
			*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
			ret = ENOENT;
		}
		else
		{
			gpi = fci_qos_get_gpi(phy_if);
			if (NULL == gpi)
			{
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = ENOENT;
			}
			else
			{
				/* basic command validations */
				shp_id = shp_cmd->id;
				if (shp_id >= PFE_IQOS_SHP_COUNT)
				{
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					ret = EINVAL;
				}
				else
				{
					*fci_ret = FPP_ERR_OK;

					switch(shp_cmd->action)
					{
						case FPP_ACTION_UPDATE:
						{
							if (0U != shp_cmd->enable)
							{
								ret = pfe_gpi_shp_enable(gpi, shp_id);
							}
							else
							{
								/* exit configuration update on disable */
								ret = pfe_gpi_shp_disable(gpi, shp_id);
								if (EOK != ret)
								{
									*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								}
								break;
							}

							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							shp_type = shp_cmd->type;
							shp_mode = shp_cmd->mode;
							shp_isl = oal_ntohl(shp_cmd->isl);
							shp_max_credit = (int32_t)oal_ntohl(shp_cmd->max_credit);
							shp_min_credit = (int32_t)oal_ntohl(shp_cmd->min_credit);

							/* commit command to h/w */
							ret = pfe_gpi_shp_set_type(gpi, shp_id, (pfe_iqos_shp_type_t)shp_type);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							ret = pfe_gpi_shp_set_mode(gpi, shp_id, (pfe_iqos_shp_rate_mode_t)shp_mode);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							NXP_LOG_DEBUG("Setting shaper %d idle slope: %u\n", shp_id, (uint_t)shp_isl);
							ret = pfe_gpi_shp_set_idle_slope(gpi, shp_id, shp_isl);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							NXP_LOG_DEBUG("Setting shaper %d credit limits: [%d, %d]\n",
									shp_id, (int_t)shp_min_credit, (int_t)shp_max_credit);
							ret = pfe_gpi_shp_set_limits(gpi, shp_id, shp_max_credit, shp_min_credit);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							break;
						}

						case FPP_ACTION_QUERY:
						{
							/* copy original command properties into reply structure */
							reply_buf->action = shp_cmd->action;
							(void)strncpy(reply_buf->if_name, shp_cmd->if_name, sizeof(reply_buf->if_name));
							reply_buf->if_name[sizeof(reply_buf->if_name) - 1U] = '\0';
							reply_buf->id = shp_id;

							/* get shaper data */
							reply_buf->enable = pfe_gpi_shp_is_enabled(gpi, shp_id);

							ret = pfe_gpi_shp_get_type(gpi, shp_id, (pfe_iqos_shp_type_t *)&shp_type);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							ret = pfe_gpi_shp_get_mode(gpi, shp_id, (pfe_iqos_shp_rate_mode_t *)&shp_mode);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							ret = pfe_gpi_shp_get_idle_slope(gpi, shp_id, &shp_isl);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							ret = pfe_gpi_shp_get_limits(gpi, shp_id, &shp_max_credit, &shp_min_credit);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}

							reply_buf->type = shp_type;
							reply_buf->mode = shp_mode;
							reply_buf->isl = oal_htonl(shp_isl);
							reply_buf->max_credit = (int32_t)oal_htonl(shp_max_credit);
							reply_buf->min_credit = (int32_t)oal_htonl(shp_min_credit);

							*reply_len = sizeof(*shp_cmd);
							break;
						}

						default:
						{
							NXP_LOG_WARNING("FPP_CMD_QOS_POLICER_SHP: Unknown action received: 0x%x\n", shp_cmd->action);
							*fci_ret = FPP_ERR_UNKNOWN_ACTION;
							break;
						}
					}
				}
			}
		}
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */
/** @}*/
