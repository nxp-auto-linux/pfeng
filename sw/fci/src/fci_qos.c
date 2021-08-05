/* =========================================================================
 *  Copyright 2020-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_eqos.c
 * @brief		Egress QoS management
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_FCI_ENABLE

/*
 * 	This is storage of scheduler algorithms ordered in way
 *	as defined by the FCI (see fpp_ext.h::fpp_qos_scheduler_cmd_t)
 */
static const pfe_tmu_sched_algo_t sch_algos[] = {SCHED_ALGO_PQ, SCHED_ALGO_DWRR, SCHED_ALGO_RR, SCHED_ALGO_WRR};
#ifdef NXP_LOG_ENABLED
static const char_t *sch_algos_str[] = {"SCHED_ALGO_PQ", "SCHED_ALGO_DWRR", "SCHED_ALGO_RR", "SCHED_ALGO_WRR"};
#endif /* NXP_LOG_ENABLED */

static pfe_phy_if_t *fci_get_phy_if_by_name(char_t *name)
{
	fci_t *fci = (fci_t *)&__context;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	errno_t ret;
	uint32_t sid;

	ret = pfe_if_db_lock(&sid);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not lock interface DB: %d\n", ret);
		return NULL;
	}

	ret = pfe_if_db_get_first(fci->phy_if_db, sid, IF_DB_CRIT_BY_NAME, name, &entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Interface DB query failed: %d\n", ret);
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
	pfe_platform_t *pfe = pfe_platform_get_instance();
	uint32_t cnt, ii;
	static const pfe_tmu_queue_mode_t fci_qmode_to_qmode[] =
		{TMU_Q_MODE_INVALID, TMU_Q_MODE_DEFAULT, TMU_Q_MODE_TAIL_DROP, TMU_Q_MODE_WRED};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_qos_queue_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_qos_queue_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_qos_queue_cmd_t));
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
				*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
				ret = ENOENT;
				break;
			}

			/*	Check queue ID */
			cnt = pfe_tmu_queue_get_cnt(pfe->tmu, pfe_phy_if_get_id(phy_if));
			if (q->id > cnt)
			{
				NXP_LOG_ERROR("Queue ID %d out of range. Interface %s implements %d queues\n",
						(int_t)q->id, q->if_name, (int_t)cnt);
				*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
				break;
			}

			NXP_LOG_DEBUG("Setting queue %d mode: %d (min: %d, max: %d)\n",
					(int_t)q->id, (int_t)q->mode, (int_t)oal_ntohl(q->min), (int_t)oal_ntohl(q->max));

			if (q->mode > 3U)
			{
				NXP_LOG_ERROR("Unsupported queue mode: %d\n", q->mode);
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				break;
			}
			else
			{
				if (q->mode == 0U)
				{
					/*	Disable the queue to drop all packets */
					ret = pfe_tmu_queue_set_mode(pfe->tmu, pfe_phy_if_get_id(phy_if), q->id,
							TMU_Q_MODE_TAIL_DROP, 0U, 0U);
				}
				else
				{
					ret = pfe_tmu_queue_set_mode(pfe->tmu, pfe_phy_if_get_id(phy_if), q->id,
						fci_qmode_to_qmode[q->mode], oal_ntohl(q->min), oal_ntohl(q->max));
				}

				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not set queue %d mode %d: %d\n", q->id, q->mode, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				if (q->mode == 3)
				{
					NXP_LOG_DEBUG("Setting WRED zones probabilities\n");

					cnt = pfe_tmu_queue_get_cnt(pfe->tmu, pfe_phy_if_get_id(phy_if));

					if (cnt > 32U)
					{
						NXP_LOG_DEBUG("Invalid zones count...\n");
						ret = EINVAL;
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						break;
					}

					for (ii=0U; ii<cnt; ii++)
					{
						NXP_LOG_DEBUG("Setting queue %d zone %d probability %d%%\n",
								(int_t)q->id, (int_t)ii, (int_t)q->zprob[ii]);
						ret = pfe_tmu_queue_set_wred_prob(pfe->tmu,
								pfe_phy_if_get_id(phy_if), q->id, ii, q->zprob[ii]);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Could not set queue %d zone %d probability %d: %d\n",
									(int_t)q->id, (int_t)ii, (int_t)q->zprob[ii], (int_t)ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
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
				*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
				ret = ENOENT;
				break;
			}

			/*	Check queue ID */
			cnt = pfe_tmu_queue_get_cnt(pfe->tmu, pfe_phy_if_get_id(phy_if));
			if (q->id > cnt)
			{
				NXP_LOG_ERROR("Queue ID %d out of range. Interface %s implements %d queues\n",
						(int_t)q->id, q->if_name, (int_t)cnt);
				*fci_ret = FPP_ERR_QOS_QUEUE_NOT_FOUND;
				break;
			}

			/*	Copy original command properties into reply structure */
			reply_buf->action = q->action;
			reply_buf->id = q->id;
			strncpy(reply_buf->if_name, q->if_name, sizeof(reply_buf->if_name));
			reply_buf->if_name[sizeof(reply_buf->if_name) - 1] = '\0';

			/*	Get queue mode */
			switch (pfe_tmu_queue_get_mode(pfe->tmu, pfe_phy_if_get_id(phy_if),
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
					cnt = pfe_tmu_queue_get_wred_zones(pfe->tmu, pfe_phy_if_get_id(phy_if), q->id);
					for (ii=0U; ii<32U; ii++)
					{
						if (ii < cnt)
						{
							ret = pfe_tmu_queue_get_wred_prob(
									pfe->tmu, pfe_phy_if_get_id(phy_if), q->id, ii, &reply_buf->zprob[ii]);
							if (EOK != ret)
							{
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
			NXP_LOG_ERROR("FPP_CMD_QOS_QUEUE: Unknown action received: 0x%x\n", q->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
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
	pfe_platform_t *pfe = pfe_platform_get_instance();
	uint32_t ii, cnt;
	uint8_t queue;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_qos_scheduler_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_qos_scheduler_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_qos_scheduler_cmd_t));
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
				*fci_ret = FPP_ERR_QOS_SCHEDULER_NOT_FOUND;
				ret = ENOENT;
				break;
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
				ret = pfe_tmu_sch_set_rate_mode(pfe->tmu,
						pfe_phy_if_get_id(phy_if), sch->id, RATE_MODE_DATA_RATE);
			}
			else if (2U == sch->mode)
			{
				NXP_LOG_INFO("Setting scheduler %d mode: Packet rate\n", sch->id);
				ret = pfe_tmu_sch_set_rate_mode(pfe->tmu,
						pfe_phy_if_get_id(phy_if), sch->id, RATE_MODE_PACKET_RATE);
			}
			else
			{
				NXP_LOG_ERROR("Unsupported scheduler mode: 0x%x\n", sch->mode);
				ret = EINVAL;
			}

			if (EOK != ret)
			{
				NXP_LOG_WARNING("Scheduler mode not set: %d\n", ret);
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				break;
			}

			/*	Set scheduler algorithm */
			if (sch->algo > 3U)
			{
				NXP_LOG_ERROR("Unsupported scheduler algorithm: 0x%x\n", sch->algo);
				ret = EOK;
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				break;
			}

			NXP_LOG_INFO("Setting scheduler %d algorithm: %s\n",
					sch->id, sch_algos_str[sch->algo]);
			ret = pfe_tmu_sch_set_algo(pfe->tmu, pfe_phy_if_get_id(phy_if),
					sch->id, sch_algos[sch->algo]);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("Scheduler algorithm not set: %d\n", ret);
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				break;
			}

			/*	Configure scheduler inputs */
			cnt = pfe_tmu_sch_get_input_cnt(pfe->tmu, pfe_phy_if_get_id(phy_if), sch->id);
			sch->input_en = oal_ntohl(sch->input_en);
			for (ii=0U; ii<cnt; ii++)
			{
				if ((0U == ((1U << ii) & sch->input_en)) || (sch->input_src[ii] == 255U))
				{
					NXP_LOG_DEBUG("Disabling scheduler %d input %d\n", (int_t)sch->id, (int_t)ii);
					ret = pfe_tmu_sch_bind_queue(pfe->tmu, pfe_phy_if_get_id(phy_if),
								sch->id, ii, PFE_TMU_INVALID_QUEUE);
					if (EOK != ret)
					{
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
						ret = pfe_tmu_sch_bind_queue(pfe->tmu, pfe_phy_if_get_id(phy_if),
								sch->id, ii, sch->input_src[ii]);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Could not connect source %d to scheduler input %d\n",
									(int_t)sch->input_src[ii], (int_t)ii);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break; /* for */
						}
					}
					else if (sch->input_src[ii] == 8U)
					{
						NXP_LOG_DEBUG("Connecting scheduler %d output to scheduler %d input %d\n",
								(int_t)(sch->id-1U), (int_t)sch->id, (int_t)ii);
						ret = pfe_tmu_sch_bind_sch_output(pfe->tmu, pfe_phy_if_get_id(phy_if),
								sch->id-1U, sch->id, ii);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Could not connect scheduler %d output to scheduler %d input %d: %d\n",
									(int_t)(sch->id-1U), (int_t)sch->id, (int_t)ii, (int_t)ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break; /* for */
						}
					}
					else
					{
						NXP_LOG_ERROR("Unsupported scheduler input %d source: %d\n", (int_t)ii, (int_t)sch->input_src[ii]);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						break; /* for */
					}

					NXP_LOG_DEBUG("Setting scheduler %d input %d weight: %d\n",
							(int_t)sch->id, (int_t)ii, (int_t)oal_ntohl(sch->input_w[ii]));
					ret = pfe_tmu_sch_set_input_weight(pfe->tmu, pfe_phy_if_get_id(phy_if),
							sch->id, ii, oal_ntohl(sch->input_w[ii]));
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not set scheduler %d input %d weight %d: %d\n",
								(int_t)sch->id, (int_t)ii, (int_t)oal_ntohl(sch->input_w[ii]), (int_t)ret);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
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
				*fci_ret = FPP_ERR_QOS_SCHEDULER_NOT_FOUND;
				ret = ENOENT;
				break;
			}

			/*	Copy original command properties into reply structure */
			reply_buf->action = sch->action;
			reply_buf->id = sch->id;
			strncpy(reply_buf->if_name, sch->if_name, sizeof(reply_buf->if_name));
			reply_buf->if_name[sizeof(reply_buf->if_name) - 1] = '\0';

			/*	Get scheduler mode */
			switch (pfe_tmu_sch_get_rate_mode(pfe->tmu, pfe_phy_if_get_id(phy_if), sch->id))
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
					NXP_LOG_ERROR("Can't get scheduler %d mode or the mode is invalid\n", sch->id);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					ret = EINVAL;
					break;
				}
			}

			/*	Get scheduler algo */
			reply_buf->algo = (uint8_t)pfe_tmu_sch_get_algo(pfe->tmu,
					pfe_phy_if_get_id(phy_if), sch->id);
			if (reply_buf->algo == (uint8_t)SCHED_ALGO_INVALID)
			{
				NXP_LOG_ERROR("Can't get scheduler %d algo or the algo is invalid\n", sch->id);
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EINVAL;
				break;
			}

			/*	Get enabled inputs and associated sources. See the Egress QoS chapter in FCI doc. */
			cnt = pfe_tmu_sch_get_input_cnt(pfe->tmu, pfe_phy_if_get_id(phy_if), sch->id);
			reply_buf->input_en = 0U;
			for (ii=0U; ii<cnt; ii++)
			{
				queue = pfe_tmu_sch_get_bound_queue(pfe->tmu, pfe_phy_if_get_id(phy_if),
						sch->id, ii);
				if (PFE_TMU_INVALID_QUEUE == queue)
				{
					if (PFE_TMU_INVALID_SCHEDULER == pfe_tmu_sch_get_bound_sch_output(pfe->tmu,
							pfe_phy_if_get_id(phy_if), sch->id, ii))
					{
						/*	Scheduler input 'ii' is not connected */
						reply_buf->input_src[ii] = 255U;
					}
					else
					{
						/*	Scheduler input 'ii' is connected to prepend scheduler output */
						reply_buf->input_src[ii] = 8U;
						reply_buf->input_w[ii] = oal_htonl(pfe_tmu_sch_get_input_weight(pfe->tmu,
								pfe_phy_if_get_id(phy_if), sch->id, ii));
						reply_buf->input_en |= (1U << ii);
					}
				}
				else
				{
					/*	Scheduler input 'ii' is connected to queue */
					reply_buf->input_src[ii] = queue;
					reply_buf->input_w[ii] = oal_htonl(pfe_tmu_sch_get_input_weight(pfe->tmu,
							pfe_phy_if_get_id(phy_if), sch->id, ii));
					reply_buf->input_en |= (1U << ii);
				}
			}

			/*	Maintain endianness as given by FCI doc */
			reply_buf->input_en = oal_htonl(reply_buf->input_en);

			*reply_len = sizeof(fpp_qos_scheduler_cmd_t);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("FPP_CMD_QOS_SCHEDULER: Unknown action received: 0x%x\n", sch->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
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
	pfe_platform_t *pfe = pfe_platform_get_instance();

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

	if (unlikely(FALSE == __context.fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_qos_shaper_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_qos_shaper_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_qos_shaper_cmd_t));
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
				*fci_ret = FPP_ERR_QOS_SHAPER_NOT_FOUND;
				ret = ENOENT;
				break;
			}

			if (0U == shp->mode)
			{
				if (255U == shp->position)
				{
					NXP_LOG_DEBUG("Disconnecting shaper %d\n", shp->id);
					ret = pfe_tmu_shp_set_position(pfe->tmu, pfe_phy_if_get_id(phy_if),
							shp->id, PFE_TMU_INVALID_POSITION);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not disconnect shaper %d: %d\n", shp->id, ret);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						break;
					}
				}

				NXP_LOG_DEBUG("Disabling shaper %d\n", shp->id);
				ret = pfe_tmu_shp_disable(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not disable shaper %d: %d\n", shp->id, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}
			}
			else
			{
				NXP_LOG_DEBUG("Enabling shaper %d\n", shp->id);
				ret = pfe_tmu_shp_enable(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not enable shaper %d: %d\n", shp->id, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				NXP_LOG_DEBUG("Setting shaper %d rate mode %d\n", shp->id, shp->mode);
				if (1U == shp->mode)
				{
					ret = pfe_tmu_shp_set_rate_mode(pfe->tmu, pfe_phy_if_get_id(phy_if),
						shp->id, RATE_MODE_DATA_RATE);
				}
				else if (2U == shp->mode)
				{
					ret = pfe_tmu_shp_set_rate_mode(pfe->tmu, pfe_phy_if_get_id(phy_if),
						shp->id, RATE_MODE_PACKET_RATE);
				}
				else
				{
					NXP_LOG_ERROR("Invalid shaper rate mode value: %d\n", shp->mode);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				if (EOK != ret)
				{
					NXP_LOG_ERROR("Unable to set shaper %d rate mode %d: %d\n",
							shp->id, shp->mode, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				NXP_LOG_DEBUG("Setting shaper %d credit limits %d-%d\n",
						(int_t)shp->id, (int_t)oal_ntohl(shp->max_credit), (int_t)oal_ntohl(shp->min_credit));
				ret = pfe_tmu_shp_set_limits(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id,
						oal_ntohl(shp->max_credit), oal_ntohl(shp->min_credit));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Unable to set shaper %d limits: %d\n", shp->id, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				NXP_LOG_DEBUG("Setting shaper %d position to %d\n", shp->id, shp->position);
				ret = pfe_tmu_shp_set_position(pfe->tmu, pfe_phy_if_get_id(phy_if),
						shp->id, shp->position);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't set shaper %d at position %d: %d\n",
							shp->id, shp->position, ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}

				NXP_LOG_DEBUG("Setting shaper %d idle slope: %d\n",
						(int_t)shp->id, (int_t)oal_ntohl(shp->isl));
				ret = pfe_tmu_shp_set_idle_slope(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id,
						oal_ntohl(shp->isl));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't set shaper %d idle slope %d: %d\n",
							(int_t)shp->id, (int_t)oal_ntohl(shp->isl), (int_t)ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
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
				*fci_ret = FPP_ERR_QOS_SHAPER_NOT_FOUND;
				ret = ENOENT;
				break;
			}

			/*	Copy original command properties into reply structure */
			reply_buf->action = shp->action;
			reply_buf->id = shp->id;
			strncpy(reply_buf->if_name, shp->if_name, sizeof(reply_buf->if_name));
			reply_buf->if_name[sizeof(reply_buf->if_name) - 1] = '\0';

			/*	Get shaper mode */
			switch (pfe_tmu_shp_get_rate_mode(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id))
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
			ret = pfe_tmu_shp_get_limits(pfe->tmu, pfe_phy_if_get_id(phy_if),
					shp->id, &reply_buf->max_credit, &reply_buf->min_credit);
			if (ret != EOK)
			{
				NXP_LOG_ERROR("Could not get shaper %d limits: %d\n", shp->id, ret);
			}
			else
			{
				/*	Ensure expected endianness */
				reply_buf->max_credit = oal_htonl(reply_buf->max_credit);
				reply_buf->min_credit = oal_htonl(reply_buf->min_credit);
			}

			/*	Get idle slope */
			reply_buf->isl = oal_htonl(
					pfe_tmu_shp_get_idle_slope(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id));

			/*	Get shaper position */
			reply_buf->position =
					pfe_tmu_shp_get_position(pfe->tmu, pfe_phy_if_get_id(phy_if), shp->id);

			*reply_len = sizeof(fpp_qos_shaper_cmd_t);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("FPP_CMD_QOS_SHAPER: Unknown action received: 0x%x\n", shp->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}

	return ret;
}

#endif /* PFE_CFG_FCI_ENABLE */
/** @}*/
