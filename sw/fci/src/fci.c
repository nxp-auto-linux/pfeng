/* =========================================================================
 *  Copyright 2017-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#include "pfe_cfg.h"
#include "fci_internal.h"
#include "fci.h"
#include "fci_rt_db.h" /* The 'routes' database */
#include "fci_fp.h"
#include "fci_fp_db.h"
#include "fci_flexible_filter.h"
#include "fci_spd.h"
 /* Global variable used across all fci files */
fci_t __context = {0};

/**
 * @brief		Process FCI IPC message
 * @details		Interpret the IPC message and perform related configuration/management actions.
 * 				Generate response in form of fci_msg_t to be sent back to FCI client.
 * @param[in]	msg The input message from FCI client
 * @param[out]	rep_msg The reply message
 * @return		EOK if success, error code otherwise
 */
errno_t fci_process_ipc_message(fci_msg_t *msg, fci_msg_t *rep_msg)
{
	fci_t *context = (fci_t *)&__context;
	errno_t ret = EOK; /* Return value */
	uint16_t fci_ret = FPP_ERR_OK; /* FCI command return value */

	uint8_t *reply_buf_ptr = NULL;
	uint32_t *reply_buf_len_ptr = NULL;
	uint16_t *reply_retval_ptr = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == context) || (NULL == msg) || (NULL == rep_msg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if (FALSE == FCI_CFG_FORCE_LEGACY_API)
	/*	Allocate space for return value by skipping first two bytes */
	reply_buf_ptr = ((uint8_t *)&rep_msg->msg_cmd.payload[0] + 2);
	reply_buf_len_ptr = &rep_msg->msg_cmd.length;

	/*	Available reply buffer is two bytes less than maximum FCI message payload length */
	*reply_buf_len_ptr = FCI_CFG_MAX_CMD_PAYLOAD_LEN - 2U;
#else
	/*	Don't allocate space for return value. First two bytes of payload buffer will be overwritten... */
	reply_buf_ptr = rep_msg->msg_cmd.payload;
	reply_buf_len_ptr = &rep_msg->msg_cmd.length;
	*reply_buf_len_ptr = FCI_CFG_MAX_CMD_PAYLOAD_LEN;
#endif /* FCI_CFG_FORCE_LEGACY_API */

	switch (msg->type)
	{
		case FCI_MSG_CMD:
		{
			switch (msg->msg_cmd.code)
			{
				case FPP_CMD_DATA_BUF_PUT:
				{
					pfe_ct_buffer_t buf;
					fpp_buf_cmd_t *fci_buf = (fpp_buf_cmd_t *)msg->msg_cmd.payload;

					if (sizeof(buf.payload) < fci_buf->len)
					{
						NXP_LOG_ERROR("Put buffer is too small\n");
						ret = EINVAL;
						fci_ret = FPP_ERR_INTERNAL_FAILURE;
					}
					else
					{
						buf.flags = 1;
						buf.len = fci_buf->len;
						memcpy(&buf.payload, fci_buf->payload, fci_buf->len);

						ret = pfe_class_put_data(context->class, &buf);
						if (EOK != ret)
						{
							NXP_LOG_DEBUG("pfe_class_buf_put() failed: %d\n", ret);
							fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
					}

					break;
				}

				case FPP_CMD_IF_LOCK_SESSION:
				case FPP_CMD_IF_UNLOCK_SESSION:
				{
					ret = fci_interfaces_session_cmd(msg->msg_cmd.code, &fci_ret);

					break;
				}

				case FPP_CMD_LOG_IF:
				{
					/*	Process 'interface' commands (add/del/update/query/...) */
					ret = fci_interfaces_log_cmd(msg, &fci_ret, (fpp_log_if_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);

					break;
				}

				case FPP_CMD_PHY_IF:
				{
					/*	Process 'interface' commands (add/del/update/query/...) */
					ret = fci_interfaces_phy_cmd(msg, &fci_ret, (fpp_phy_if_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);

					break;
				}

				case FPP_CMD_IP_ROUTE:
				{
					/*	Process 'route' commands (add/del/update/query/...) */
					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						ret = fci_routes_cmd(msg, &fci_ret, (fpp_rt_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_IPV4_SET_TIMEOUT:
				{
					/*	Update default timeouts for connections */

					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						ret = fci_connections_ipv4_timeout_cmd(msg, &fci_ret, (fpp_timeout_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_IPV4_CONNTRACK:
				{
					/*	Process 'ipv4 connection' commands (add/del/updated/query/...) */
					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						ret = fci_connections_ipv4_ct_cmd(msg, &fci_ret, (fpp_ct_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_IPV6_CONNTRACK:
				{
					/*	Process 'ipv6 connection' commands (add/del/updated/query/...) */
					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						ret = fci_connections_ipv6_ct_cmd(msg, &fci_ret, (fpp_ct6_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_IPV4_RESET:
				{
					/*	Remove all IPv4 routes, including connections */
					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						fci_routes_drop_all_ipv4();
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_IPV6_RESET:
				{
					/*	Remove all IPv6 routes, including connections */
					ret = oal_mutex_lock(&context->db_mutex);
					if (EOK == ret)
					{
						fci_routes_drop_all_ipv6();
						oal_mutex_unlock(&context->db_mutex);
					}

					break;
				}

				case FPP_CMD_L2_BD:
				{
					/*	Manage L2 bridge domains */
					ret = fci_l2br_domain_cmd(msg, &fci_ret, (fpp_l2_bd_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);

					break;
				}

                case FPP_CMD_FP_TABLE:
                {
                    ret = fci_fp_table_cmd(msg, &fci_ret, (fpp_fp_table_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
                    break;
                }

                case FPP_CMD_FP_RULE:
                {
                    ret = fci_fp_rule_cmd(msg, &fci_ret, (fpp_fp_rule_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
                    break;
                }

                case FPP_CMD_FP_FLEXIBLE_FILTER:
                {
                    /* Configure Flexible filter */
                    ret = fci_flexible_filter_cmd(msg, &fci_ret, (fpp_flexible_filter_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
                    break;
                }

                case FPP_CMD_SPD:
                {
                    ret = fci_spd_cmd(msg, &fci_ret, (fpp_spd_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
                    break;
                }

				case FPP_CMD_QOS_QUEUE:
				{
					ret = fci_qos_queue_cmd(msg, &fci_ret, (fpp_qos_queue_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
					break;
				}

				case FPP_CMD_QOS_SCHEDULER:
				{
					ret = fci_qos_scheduler_cmd(msg, &fci_ret, (fpp_qos_scheduler_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
					break;
				}

				case FPP_CMD_QOS_SHAPER:
				{
					ret = fci_qos_shaper_cmd(msg, &fci_ret, (fpp_qos_shaper_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
					break;
				}

				default:
				{
					NXP_LOG_WARNING("Unknown CMD code received: 0x%x\n", msg->msg_cmd.code);
					ret = EINVAL;
					fci_ret = FPP_ERR_UNKNOWN_COMMAND;
					break;
				}
			}

			/*	Inform client about command execution status */
#if (FALSE == FCI_CFG_FORCE_LEGACY_API)
			/*	We're adding another two bytes at the beginning of the FCI message payload area */
			rep_msg->msg_cmd.length = *reply_buf_len_ptr + 2U;
#else
			/*	Pass reply buffer length as is. First two bytes will be overwritten by the return value. */
			rep_msg->msg_cmd.length = *reply_buf_len_ptr;
#endif /* FCI_CFG_FORCE_LEGACY_API */
			reply_retval_ptr = (uint16_t *)rep_msg->msg_cmd.payload;
			*reply_retval_ptr = (uint16_t)fci_ret;

			break;
		}

		default:
		{
			NXP_LOG_WARNING("Unknown message type\n");
			return EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Create and start FCI endpoint
 * @param[in]	info Additional FCI endpoint configuration or NULL if not required
 * @param[in]	identifier Text to be used to identify namespace node associated with
 * 				the context
 * @retval		EOK Success
 * @retval		EINVAL invalid argument received
 * @retval		ENOMEM initialization failed
 */
errno_t fci_init(fci_init_info_t *info, const char_t *const identifier)
{
	fci_t *context = (fci_t *)&__context;
	errno_t err = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == info) || (NULL == identifier)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(TRUE == context->fci_initialized)
	{
		NXP_LOG_ERROR("FCI has already been initialized!\n");
		return EINVAL;
	}

	memset(context, 0, sizeof(fci_t));

	context->db_mutex_initialized = FALSE;
	context->log_if_db_initialized = FALSE;
	context->phy_if_db_initialized = FALSE;
	context->rt_db_initialized = FALSE;
	context->rtable_initialized = FALSE;

	/*	Sanity check */
	if (6 != sizeof(pfe_mac_addr_t))
	{
		err = EINVAL;
		goto free_and_fail;
	}

	/*	Create communication core */
	err = fci_core_init(identifier);
	if (err != EOK)
	{
		NXP_LOG_ERROR("Could not create FCI core\n");
		goto free_and_fail;
	}

	err = oal_mutex_init(&context->db_mutex);
	if (EOK != err)
	{
		goto free_and_fail;
	}
	else
	{
		context->db_mutex_initialized = TRUE;
	}

	/*	Initialize the Flexible Parser databases */
	fci_fp_db_init();
	context->class = info->class;

	/*	Initialize the physical interface database */
	context->phy_if_db = info->phy_if_db;

	if(NULL != context->log_if_db)
	{
		context->phy_if_db_initialized = TRUE;
	}

	/*	Initialize the logical interface database */
	context->log_if_db = info->log_if_db;

	if(NULL != context->log_if_db)
	{
		context->log_if_db_initialized = TRUE;
	}

	/*	Initialize the route database */
	fci_rt_db_init(&context->route_db);
	context->rt_db_initialized = TRUE;

	/*	Store the routing table and bridge reference */
	if (NULL != info)
	{
		if (NULL != info->rtable)
		{
			context->rtable = info->rtable;
			context->rtable_initialized = TRUE;
		}

		if (NULL != info->l2_bridge)
		{
			context->l2_bridge = info->l2_bridge;
			context->l2_bridge_initialized = TRUE;
		}
	}

	context->default_timeouts.timeout_tcp = 5U * 24U * 60U * 60U; 	/* 5 days */
	context->default_timeouts.timeout_udp = 300U; 					/* 5 min */
	context->default_timeouts.timeout_other = 240U; 				/* 4 min */
	context->fci_initialized = TRUE;
	return err;

free_and_fail:
	fci_fini();
	return err;
}

/**
 * @brief		Destroy FCI context
 */
void fci_fini(void)
{
	fci_t *context = (fci_t *)&__context;
	uint32_t session_id;

	if (FALSE == context->fci_initialized)
	{
		return;
	}

	/*	Shut down the endpoint */
	if (NULL != context->core)
	{
		fci_core_fini();
		context->core = NULL;
	}

	pfe_if_db_lock(&session_id);
	/*	Shutdown the logical IF DB */
	if (TRUE == context->log_if_db_initialized)
	{
		/* Freeing of the DB is handled by platfrom driver*/
		context->log_if_db = NULL;
		context->log_if_db_initialized = FALSE;
	}

	/*  Shutdown the physical IF DB */
	if (TRUE == context->phy_if_db_initialized)
	{
		/* Freeing of the DB is handled by platfrom driver*/
		context->phy_if_db = NULL;
		context->phy_if_db_initialized = FALSE;
	}
	pfe_if_db_unlock(session_id);

	/*	Shutdown the RT DB */
	if (TRUE == context->rt_db_initialized)
	{
		if (TRUE == context->db_mutex_initialized)
		{
			oal_mutex_lock(&context->db_mutex);
			fci_routes_drop_all();
			oal_mutex_unlock(&context->db_mutex);
		}

		context->rt_db_initialized = FALSE;
	}

	/*	Invalidate the routing table */
	context->rtable = NULL;
	context->rtable_initialized = FALSE;

	if (TRUE == context->db_mutex_initialized)
	{
		(void)oal_mutex_destroy(&context->db_mutex);
	}

	memset(context, 0, sizeof(fci_t));
	context->fci_initialized = FALSE;
}

/**
 * @brief		Enable interface to receive/transmit data
 * @param[in]	phy_if The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t fci_enable_if(pfe_phy_if_t *phy_if)
{

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == phy_if))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_phy_if_enable(phy_if);
}

/**
 * @brief		Disable transmission/reception on interface
 * @param[in]	phy_if The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t fci_disable_if(pfe_phy_if_t *phy_if)
{
	fci_t *context = (fci_t *)&__context;
	fci_rt_db_entry_t *route_entry;
	pfe_rtable_entry_t *rtable_entry;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == phy_if))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

	if (unlikely(FALSE == context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Don't disable the interface if some routing table entry is using it */
	route_entry = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_IF, phy_if);
	while (NULL != route_entry)
	{
		rtable_entry = pfe_rtable_get_first(context->rtable, RTABLE_CRIT_BY_ROUTE_ID, &route_entry->id);
		if (NULL != rtable_entry)
		{
			/*	There is routing table entry using the interface */
			return EOK;
		}

		route_entry = fci_rt_db_get_next(&context->route_db);
	}

	/*	Also don't disable it when interface is in bridge */
	if (NULL != pfe_l2br_get_first_domain(context->l2_bridge, L2BD_BY_PHY_IF, (void *)phy_if))
	{
		/*	Interface is assigned to some L2 bridge domain */
		return EOK;
	}

	/*	Interface is not being used by FCI logic, disable it. Note that
	 	if some logical interface associated with this physical one is
	 	active, the interface will not be disabled. */
	ret = pfe_phy_if_disable(phy_if);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Can't disable interface (%s)\n", pfe_phy_if_get_name(phy_if));
	}

	return ret;
}
