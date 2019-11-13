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
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_interfaces.c
 * @brief		Ethernet interfaces management functions.
 * @details		All interfaces-related functionality provided by the FCI should be
 * 				implemented within this file. This includes commmands dedicated
 *				to register and unregister interface to/from the FCI.
 *
 */

#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"



static void fci_interfaces_remove_related_routes(pfe_phy_if_t *phy_if);
static void fci_interfaces_get_arg_info(fpp_if_m_args_t *m_arg, fpp_if_m_rules_t rule, void **offset, size_t *size);

/*
 * @brief		Remove all routes related to the given interface
 * @details		Routes without active destination interface are not valid. Go therefore through
 *				all registered routes and remove ones which are related to the referenced
 *				interface (by name). Client owning the route will be notified if has provided
 *				its identifier.
 * @param[in]	phy_if Reference to physical interface that changed its MAC address
 */
static void fci_interfaces_remove_related_routes(pfe_phy_if_t *phy_if)
{
	fci_t *context = (fci_t *)&__context;
	fci_msg_t notify_msg;
	fpp_rt_cmd_t *rt_cmd = NULL;
	fci_rt_db_entry_t *route = NULL;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == phy_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	memset(&notify_msg, 0, sizeof(fci_msg_t));
	notify_msg.type = FCI_MSG_CMD;
	notify_msg.msg_cmd.code = FPP_CMD_IP_ROUTE;

	rt_cmd = (fpp_rt_cmd_t *)notify_msg.msg_cmd.payload;
	rt_cmd->action = FPP_ACTION_REMOVED;

	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_IF, (void *)phy_if);
	while (NULL != route)
	{
		ret = fci_routes_drop_one(route);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Couldn't properly drop a route: %d\n", ret);
		}
		route = fci_rt_db_get_next(&context->route_db);
	}
}


/**
 * @brief		Callback to be called on interface-related events
 * @param[in]	iface The interface producing the event
 * @param[in]	event The event
 * @note		Thread safe
 */
static void fci_interfaces_if_cbk(pfe_phy_if_t *iface, pfe_phy_if_event_t event, void *arg)
{
	fci_t *context = (fci_t *)arg;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == context)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PHY_IF_EVT_MAC_ADDR_UPDATE == event)
	{
		/*	MAC address on interface has changed */
		NXP_LOG_DEBUG("FPP_CMD_INTERFACE: Interface %s being updated\n", pfe_phy_if_get_name(iface));

		if (EOK != oal_mutex_lock(&context->db_mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Some interface property is going to be changed. Remove all associated routes. */
		NXP_LOG_DEBUG("FPP_CMD_INTERFACE: Removing routes due to update of interface %s\n", pfe_phy_if_get_name(iface));
		fci_interfaces_remove_related_routes(iface);


		if (EOK != oal_mutex_unlock(&context->db_mutex))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}

	return;
}

/*
 * @brief			Get offset and size of the rule
 * @details			Errors are handled in platform driver
 * @param[in]		m_args pointer to the argument structure
 * @param[in]		rule single rule. See pfe_ct_if_m_rules_t
 * @param[in,out]	offset is set based on the rule to the structure m_arg
 * @param[in,out]	size of the underlying type in the struct based on the rule
 */
static void fci_interfaces_get_arg_info(fpp_if_m_args_t *m_arg, fpp_if_m_rules_t rule, void **offset, size_t *size)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == m_arg) || (NULL == offset) || (NULL == size)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	switch (rule)
	{
		case IF_MATCH_VLAN:
		{
			*size = sizeof(m_arg->vlan);
			*offset = &m_arg->vlan;
			break;
		}

		case IF_MATCH_PROTO:
		{
			*size = sizeof(m_arg->proto);
			*offset = &m_arg->proto;
			break;
		}

		case IF_MATCH_SPORT:
		{
			*size = sizeof(m_arg->sport);
			*offset = &m_arg->sport;
			break;
		}

		case IF_MATCH_DPORT:
		{
			*size = sizeof(m_arg->dport);
			*offset = &m_arg->dport;
			break;
		}

		case IF_MATCH_SIP6:
		{
			*size = sizeof(m_arg->v6.sip);
			*offset = &m_arg->v6.sip;
			break;
		}

		case IF_MATCH_DIP6:
		{
			*size = sizeof(m_arg->v6.dip);
			*offset = &m_arg->v6.dip;
			break;
		}

		case IF_MATCH_SIP:
		{
			*size = sizeof(m_arg->v4.sip);
			*offset = &m_arg->v4.sip;
			break;
		}

		case IF_MATCH_DIP:
		{
			*size = sizeof(m_arg->v4.dip);
			*offset = &m_arg->v4.dip;
			break;
		}

		case IF_MATCH_ETHTYPE:
		{
			*size = sizeof(m_arg->ethtype);
			*offset = &m_arg->ethtype;
			break;
		}

#if 0 /* Waiting for implementation of Flexible parser (AAVB-1899 will add these)*/
		case IF_MATCH_FP0:
		{
			if (arg_len == sizeof(m_args.fp0_table))
			{
				iface->log_if_class.m_args.fp0_table = *(PFE_PTR(pfe_ct_fp_table_t) *)arg;

			}
			/*TODO after FP is imeplemented*/
			break;

			break;
		}

		case IF_MATCH_FP1:
		{
			if (arg_len == sizeof(m_args.fp1_table))
			{
				iface->log_if_class.m_args.fp1_table = *(PFE_PTR(pfe_ct_fp_table_t) *)arg;

			}
			/*TODO after FP is imeplemented*/
			break;
		}
#endif /* 0 */

		case IF_MATCH_SMAC:
		{
			*size = sizeof(m_arg->smac);
			*offset = &m_arg->smac;
			break;
		}

		case IF_MATCH_DMAC:
		{
			*size = sizeof(m_arg->dmac);
			*offset = &m_arg->dmac;
			break;
		}

		default:
		{
			*size = 0U;
			*offset = 0U;
		}
	}
}

/**
 * @brief		Install callback for MAC change to each phy_if
 * @note		Thread safe
 */
void fci_interfaces_init_callback(void)
{
	fci_t *context = (fci_t *)&__context;
	errno_t error;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	uint32_t session_id;
	errno_t ret;

	/* Lock DB */
	if(EOK != pfe_if_db_lock(&session_id))
	{
		NXP_LOG_DEBUG("DB lock failed\n");
	}

	/* Set callback to each phy_if */
	ret = pfe_if_db_get_first(context->phy_if_db, session_id, IF_DB_CRIT_ALL, NULL, &entry);
	if(NULL != entry)
	{
		do
		{
			phy_if = pfe_if_db_entry_get_phy_if(entry);

			error = pfe_phy_if_set_callback(phy_if,&fci_interfaces_if_cbk, context);
			if (EOK != error)
			{
				NXP_LOG_ERROR("Unable to set interface callback\n");
				break;
			}
			ret = pfe_if_db_get_next(context->phy_if_db, session_id, &entry);
		} while(NULL != entry);
	}

	if(ret != EOK)
	{
		NXP_LOG_ERROR("MAC callbacks were not initialized\n");
	}

	/* Unlock  DB */
	if(EOK != pfe_if_db_unlock(session_id))
	{
		NXP_LOG_DEBUG("DB unlock failed\n");
	}
}

/**
 * @brief			Process interface atomic session related commands
 * @param[in]		msg FCI cmd code
 * @param[out]		fci_ret FCI return code
 * @return			EOK if success, error code otherwise
 */
errno_t fci_interfaces_session_cmd(uint32_t code, uint16_t *fci_ret)
{
	fci_t *context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fci_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	switch(code)
	{
		case FPP_CMD_IF_LOCK_SESSION:
		{
			*fci_ret = FPP_ERR_OK;
			if (EOK != pfe_if_db_lock(&context->if_session_id))
			{
				*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
				NXP_LOG_DEBUG("DB lock failed\n");
			}
			break;
		}
		case FPP_CMD_IF_UNLOCK_SESSION:
		{
			*fci_ret = FPP_ERR_OK;
			if (EOK != pfe_if_db_unlock(context->if_session_id))
			{
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				NXP_LOG_DEBUG("DB unlock failed due to incorrect session ID\n");
			}
			break;
		}
		default:
		{
			NXP_LOG_ERROR("Unknown Interface Session Command Received\n");
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_LOG_INTERFACE commands
 * @param[in]		msg FCI message containing the FPP_CMD_LOG_INTERFACE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_log_if_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with interface DB session lock.
 */
errno_t fci_interfaces_log_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_log_if_cmd_t *reply_buf, uint32_t *reply_len)
{
	fci_t *context = (fci_t *)&__context;
	fpp_log_if_cmd_t *if_cmd;
	errno_t ret = EOK;
	pfe_ct_if_m_args_t args;
	pfe_ct_if_m_rules_t rules;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	pfe_log_if_t *log_if = NULL;
	uint32_t index = 0U, egress = 0U;
	size_t size = 0U;
	void *offset = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_log_if_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_if_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_log_if_cmd_t));

	if_cmd = (fpp_log_if_cmd_t *)msg->msg_cmd.payload;

	switch(if_cmd->action)
	{
		case FPP_ACTION_UPDATE:
		{
			*fci_ret = FPP_ERR_OK;
			*reply_len = sizeof(fpp_log_if_cmd_t);

			ret = pfe_if_db_get_first(context->log_if_db, context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->name, &entry);

			if(EOK != ret)
			{
				NXP_LOG_ERROR("Incorrect session ID detected\n");
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				break;
			}

			/* Check if entry is not NULL and get logical interface */
			if(NULL != entry)
			{
				log_if = pfe_if_db_entry_get_log_if(entry);
			}

			/* Check if the entry exits*/
			if((NULL == entry) || (NULL == log_if))
			{
				/* Interface doesn't exist or couldn't be extracted from the entry */
				*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
				break;
			}

			/* drop all unset rules */
			ret = pfe_log_if_del_match_rule(log_if, ~if_cmd->match);
			if(EOK == ret)
			{
				NXP_LOG_INFO("All match rules were dropped on %s before match rule update.\n",  pfe_log_if_get_name(log_if));
			}
			else
			{
				NXP_LOG_DEBUG("Dropping of all match rules on logical interface %s failed !!\n",  pfe_log_if_get_name(log_if));
				*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
			}

			/* Update each rule one by one */
			for(index = 0U; 8U * sizeof(if_cmd->match) > index;  ++index)
			{
				if(0U != (if_cmd->match & (1U << index)))
				{
					/* Resolve position of data and size */
					fci_interfaces_get_arg_info(&if_cmd->arguments, if_cmd->match & (1U << index), &offset, &size);

					/* Add match rule and arguments */
					ret = pfe_log_if_add_match_rule(log_if, if_cmd->match & (1U << index), offset, size);

					if(EOK != ret)
					{
						NXP_LOG_DEBUG("Updating single rule on logical inteface %s failed !!\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
					}
				}
			}

			/* Update egress in case atleast one is set (old egress is dropped) */
			if(0 != if_cmd->egress)
			{
				NXP_LOG_INFO("Updating egress interfaces on %s\n",  pfe_log_if_get_name(log_if));
				for(index = 0U; PFE_PHY_IF_ID_INVALID > index;  ++index)
				{
					if((PFE_PHY_IF_ID_HIF == index) || (PFE_PHY_IF_ID_HIF_NOCPY == index))
					{
						/* Skip currently not used interfaces */
						continue;
					}
					/* For each bit in egress mask search if the if exists*/
					ret = pfe_if_db_get_first(context->log_if_db, context->if_session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)index, &entry);
					if((EOK == ret) && (NULL != entry))
					{
						phy_if = pfe_if_db_entry_get_phy_if(entry);

						if(0U != (if_cmd->egress & (1U << index)))
						{
							/* If the ID exits add corresponding phy_if as egress to log_if*/
							if (EOK != pfe_log_if_add_egress_if(log_if, phy_if))
							{
								NXP_LOG_ERROR("Could not set egress interface for %s\n", pfe_log_if_get_name(log_if));
								*fci_ret = FPP_ERR_IF_EGRESS_UPDATE_FAILED;
							}
						}
						else
						{
							/* Get current egress interfaces */
							ret = pfe_log_if_get_egress_ifs(log_if, &egress);
							/* Clear all currently enabled egress interfaces in case they are set */
							if(EOK == ret)
							{
								if(0U != (egress && (1U << index)))
								{
									ret= pfe_log_if_del_egress_if(log_if, phy_if);
								}
							}

							if (EOK != ret)
							{
								NXP_LOG_ERROR("Could not get and clear egress interface for %s\n", pfe_log_if_get_name(log_if));
								*fci_ret = FPP_ERR_IF_EGRESS_UPDATE_FAILED;
							}
						}
					}
					else
					{
						NXP_LOG_ERROR("Egress %u on %s is not set because it doesn't exist\n", index,  pfe_log_if_get_name(log_if));

						/* Error in input do not continue */
						*fci_ret = FPP_ERR_IF_EGRESS_DOESNT_EXIST;
						break;
					}
				}
			}

			/* AND/OR rules */
			if(0U != (if_cmd->flags & FPP_IF_MATCH_OR))
			{
				ret = pfe_log_if_set_match_or(log_if);
			}
			else
			{
				ret = pfe_log_if_set_match_and(log_if);
			}

			if(EOK != ret)
			{
				NXP_LOG_ERROR("AND/OR flag wans't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
				*fci_ret = FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED;
			}

			/* enable/disable*/
			if(0U != (if_cmd->flags & FPP_IF_ENABLED))
			{
				ret = pfe_log_if_enable(log_if);
			}
			else
			{
				ret = pfe_log_if_disable(log_if);
			}

			if(EOK != ret)
			{
				NXP_LOG_ERROR("ENABLE flag wans't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
				*fci_ret = FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED;
			}

			/* promisc */
			if(0U != (if_cmd->flags & FPP_IF_PROMISC))
			{
				ret = pfe_log_if_promisc_enable(log_if);
			}
			else
			{
				ret = pfe_log_if_promisc_disable(log_if);
			}

			if(EOK != ret)
			{
				NXP_LOG_ERROR("PROMISC flag wans't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
				*fci_ret = FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED;
			}

			break;
		}
		case FPP_ACTION_QUERY:
			{
			    ret = pfe_if_db_get_first(context->log_if_db, context->if_session_id, IF_DB_CRIT_ALL, NULL, &entry);
				if (NULL == entry)
				{
					*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					}
					ret = EOK;
					break;
				}
			}
			/*	No break */
		case FPP_ACTION_QUERY_CONT:
		{
			if (NULL == entry)
			{
				ret = pfe_if_db_get_next(context->log_if_db, context->if_session_id, &entry);
				if (NULL == entry)
				{
					*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					}
					ret = EOK;
					break;
				}
			}

			log_if = pfe_if_db_entry_get_log_if(entry);

			if(NULL != log_if)
			{
				phy_if = pfe_log_if_get_parent(log_if);
			}
			/* Store names */
			if(NULL != phy_if)
			{
				strncpy(reply_buf->name, pfe_log_if_get_name(log_if), IFNAMSIZ-1);
				strncpy(reply_buf->parent_name, pfe_phy_if_get_name(phy_if), IFNAMSIZ-1);
			}
			else
			{
				NXP_LOG_DEBUG("Was not possible to resolve DB entry to log_if or parent phy_if");
				*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
				break;
			}

			/* Get important flag values */
			reply_buf->flags = 0U;
			if(pfe_log_if_is_enabled(log_if))
			{
				reply_buf->flags |= FPP_IF_ENABLED;
			}

			if(pfe_log_if_is_promisc(log_if))
			{
				reply_buf->flags |= FPP_IF_PROMISC;
			}

			if(pfe_log_if_is_match_or(log_if))
			{
				reply_buf->flags |= FPP_IF_MATCH_OR;
			}

			/* Store egress interfaces */
			if(EOK != pfe_log_if_get_egress_ifs(log_if, &egress))
			{
				NXP_LOG_DEBUG("Was not possible to get egress interfaces\n");
			}
			reply_buf->egress = egress;

			/* Store rules for FCI*/
			if(EOK != pfe_log_if_get_match_rules(log_if, &rules, &args))
			{
				NXP_LOG_DEBUG("Was not possible to get match rules and arguments\n");
			}
			reply_buf->match = rules;

			/* Store arguments for FCI*/
			memcpy(&reply_buf->arguments, &args, sizeof(args));

			/* Set ids */
			reply_buf->id = pfe_log_if_get_id(log_if);
			reply_buf->parent_id = pfe_phy_if_get_id(pfe_log_if_get_parent(log_if));

			*reply_len = sizeof(fpp_log_if_cmd_t);
			*fci_ret = FPP_ERR_OK;
			break;
		}
	}

	return ret;
}

/**
 * @brief			Process FPP_CMD_PHY_INTERFACE commands
 * @param[in]		msg FCI message containing the FPP_CMD_PHY_INTERFACE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_phy_if_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with interface DB session lock.
 */
errno_t fci_interfaces_phy_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_phy_if_cmd_t *reply_buf, uint32_t *reply_len)
{
	fci_t *context = (fci_t *)&__context;
	fpp_phy_if_cmd_t *if_cmd;
	errno_t ret = EOK;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_phy_if_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_if_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_phy_if_cmd_t));
	
	if_cmd = (fpp_phy_if_cmd_t *)msg->msg_cmd.payload;

	switch (if_cmd->action)
	{
		case FPP_ACTION_QUERY:
		{
			ret = pfe_if_db_get_first(context->phy_if_db, context->if_session_id, IF_DB_CRIT_ALL, NULL, &entry);

			if(EOK != ret)
			{
				NXP_LOG_ERROR("Incorrect session ID detected\n");
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				break;
			}

			if (NULL == entry)
			{
				ret = EOK;
				*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
				break;
			}
		}
		/*	No break */

		case FPP_ACTION_QUERY_CONT:
		{
			if (NULL == entry)
			{
				ret = pfe_if_db_get_next(context->phy_if_db, context->if_session_id, &entry);
				if(EOK != ret)
				{
					ret = EOK;
					*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					break;
				}

				if (NULL == entry)
				{
					ret = EOK;
					*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
					break;
				}
			}

			phy_if = pfe_if_db_entry_get_phy_if(entry);
			if (NULL == phy_if)
			{
				NXP_LOG_DEBUG("Was not possible to resolve DB entry to phy_if");
				*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
				break;
			}

			/*	Store phy_if name and MAC */
			strncpy(reply_buf->name, pfe_phy_if_get_name(phy_if), IFNAMSIZ-1);
			if (EOK != pfe_phy_if_get_mac_addr(phy_if, reply_buf->mac_addr))
			{
				NXP_LOG_DEBUG("Could not get interface MAC address\n");
			}

			/* Store phy_if id */
			reply_buf->id = pfe_phy_if_get_id(phy_if);

			/* Set reply length end return OK */
			*reply_len = sizeof(fpp_phy_if_cmd_t);
			*fci_ret = FPP_ERR_OK;
			ret = EOK;
			break;
		}
	
		default:
		{
			NXP_LOG_ERROR("Interface Command: Unknown action received: 0x%x\n", if_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}

	return ret;
}
