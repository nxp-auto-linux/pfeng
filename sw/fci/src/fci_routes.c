/* =========================================================================
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_routes.c
 * @brief		IP routes management functions.
 * @details		All IP routes related functionality provided by the FCI should be
 * 				implemented within this file. This includes mainly route-related
 * 				commands.
 *
 */

#include "pfe_cfg.h"
#include "pfe_platform_cfg.h"
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

static void fci_routes_remove_related_connections(fci_rt_db_entry_t *route);

/*
 * @brief		Remove all connections related to the given route
 * @details		When a route becomes invalid or it is being removed, all related connections need
 * 				to be handled. Go therefore through all registered connections and remove ones which
 *				are related to the referenced route (by ID).
 * @param[in]	route The reference route
 */
static void fci_routes_remove_related_connections(fci_rt_db_entry_t *route)
{
	const fci_t *fci_context = (fci_t *)&__context;
	pfe_rtable_entry_t *entry;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == route)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		entry = pfe_rtable_get_first(fci_context->rtable, RTABLE_CRIT_BY_ROUTE_ID, &route->id);
		while (NULL != entry)
		{
			ret = pfe_rtable_del_entry(fci_context->rtable, entry);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("Couldn't properly drop a connection: %d\n", ret);
			}

			/*	Release the entry */
			pfe_rtable_entry_free(fci_context->rtable, entry);

			entry = pfe_rtable_get_next(fci_context->rtable);
		}
	}
}

/**
 * @brief			Process FPP_CMD_IP_ROUTE commands
 * @param[in]		msg FCI message containing the FPP_RCMD_IP_ROUTE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_rt_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with route DB protected against concurrent accesses.
 */
errno_t fci_routes_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_rt_cmd_t *reply_buf, uint32_t *reply_len)
{
	fci_t *fci_context = (fci_t *)&__context;
	fpp_rt_cmd_t *rt_cmd;
	bool_t is_ipv6 = FALSE;
	errno_t ret = EOK;
	pfe_mac_addr_t src_mac;
	pfe_mac_addr_t dst_mac;
	pfe_ip_addr_t ip;
	fci_rt_db_entry_t *rt_entry = NULL;
	pfe_if_db_entry_t *if_entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	uint32_t session_id = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */

	{
		if (*reply_len < sizeof(fpp_rt_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_rt_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_rt_cmd_t));

			rt_cmd = (fpp_rt_cmd_t *)(msg->msg_cmd.payload);
			is_ipv6 = (oal_ntohl(rt_cmd->flags) == 2U) ? TRUE : FALSE;

			/*	Prepare MAC and IP destination address */
			(void)memcpy(dst_mac, rt_cmd->dst_mac, sizeof(pfe_mac_addr_t));
			(void)memset(&ip, 0, sizeof(pfe_ip_addr_t));

			if (is_ipv6)
			{
				/*	Convert to 'known' IPv6 address format */
				(void)memcpy(&ip.v6, &rt_cmd->dst_addr[0], 16);
				ip.is_ipv4 = FALSE;
			}
			else
			{
				/*	Convert to 'known' IPv4 address format */
				(void)memcpy(&ip.v4, &rt_cmd->dst_addr[0], 4);
				ip.is_ipv4 = TRUE;
			}

			switch (rt_cmd->action)
			{
				case FPP_ACTION_REGISTER:
				{
					ret = pfe_if_db_polled_lock(&session_id);
					if (EOK == ret)
					{
						/*	Validate the interface */
						ret = pfe_if_db_get_first(fci_context->phy_if_db, session_id, IF_DB_CRIT_BY_NAME, (void *)rt_cmd->output_device, &if_entry);
						if(EOK != ret)
						{
							NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: DB is locked in different session, entry was not retrieved from DB\n");
						}
					}
					else
					{
						NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: DB lock failed\n");
					}

					if (NULL == if_entry)
					{
						/*	No such interface */
						NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Interface %s not found\n", rt_cmd->output_device);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						break;
					}

					phy_if = pfe_if_db_entry_get_phy_if(if_entry);

					/*	Prepare MAC source address */
					{
						const pfe_mac_addr_t zero_mac = {0u};
						(void)memset(src_mac, 0, sizeof(pfe_mac_addr_t));
						if (0 == memcmp(rt_cmd->src_mac, zero_mac, sizeof(pfe_mac_addr_t)))
						{
							if(EOK != pfe_phy_if_get_mac_addr_first(phy_if, src_mac, MAC_DB_CRIT_ALL, PFE_TYPE_ANY, PFE_CFG_LOCAL_IF))
							{
								NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Get the first MAC address from mac addr db failed\n");
							}
						}
						else
						{
							(void)memcpy(src_mac, rt_cmd->src_mac, sizeof(pfe_mac_addr_t));
						}
					}

					/*	Add entry to database (values in network endian) */
					ret = fci_rt_db_add(&fci_context->route_db,	&ip, &src_mac, &dst_mac,
										phy_if,
										rt_cmd->id,
										msg->client,
										FALSE);

					if (EPERM == ret)
					{
						NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Already registered\n");
						*fci_ret = FPP_ERR_RT_ENTRY_ALREADY_REGISTERED;
						break;
					}
					else if (EOK != ret)
					{
						NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Can't add route entry: %d\n", ret);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						break;
					}
					else
					{
						NXP_LOG_DEBUG("FPP_CMD_IP_ROUTE: Route (ID: %d, IF: %s) added\n", (int_t)oal_ntohl(rt_cmd->id), rt_cmd->output_device);
						*fci_ret = FPP_ERR_OK;
					}

					break;
				}

				case FPP_ACTION_DEREGISTER:
				{
					/*	Validate the route */
					rt_entry = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_BY_ID, (void *)&rt_cmd->id);
					if (NULL == rt_entry)
					{
						NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Requested route %d not found\n", (int_t)oal_ntohl(rt_cmd->id));
						*fci_ret = FPP_ERR_RT_ENTRY_NOT_FOUND;
						break;
					}

					/*	Remove related connections, then the route, and disable the interface if needed */
					ret = fci_routes_drop_one(rt_entry);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("FPP_CMD_IP_ROUTE: Can't remove route entry: %d\n", ret);
						*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
						break;
					}
					else
					{
						NXP_LOG_DEBUG("FPP_CMD_IP_ROUTE: Route %d removed\n", (int_t)oal_ntohl(rt_cmd->id));
					}

					break;
				}

				case FPP_ACTION_UPDATE:
				{
					/*	Not supported yet */
					NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: FPP_ACTION_UPDATE not supported (yet)\n");
					*fci_ret = FPP_ERR_UNKNOWN_COMMAND;
					break;
				}

				case FPP_ACTION_QUERY:
				{
					rt_entry = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_ALL, NULL);
					if (NULL == rt_entry)
					{
						ret = EOK;
						*fci_ret = FPP_ERR_RT_ENTRY_NOT_FOUND;
						break;
					}
					fallthrough;
				}

				case FPP_ACTION_QUERY_CONT:
				{
					if (NULL == rt_entry)
					{
						rt_entry = fci_rt_db_get_next(&fci_context->route_db);
						if (NULL == rt_entry)
						{
							ret = EOK;
							*fci_ret = FPP_ERR_RT_ENTRY_NOT_FOUND;
							break;
						}
					}

					/*	Write the reply buffer */
					*reply_len = sizeof(fpp_rt_cmd_t);

					/*	Build reply structure */
					reply_buf->mtu = rt_entry->mtu;
					(void)memcpy(reply_buf->src_mac, rt_entry->src_mac, sizeof(pfe_mac_addr_t));
					(void)memcpy(reply_buf->dst_mac, rt_entry->dst_mac, sizeof(pfe_mac_addr_t));

					if (rt_entry->dst_ip.is_ipv4)
					{
						/*	IPv4 */
						(void)memcpy(&reply_buf->dst_addr[0], &rt_entry->dst_ip.v4, 4);
						reply_buf->flags = oal_htonl(1U); /* TODO: This is weird (see FCI doc). Some macro should be used instead. */
					}
					else
					{
						/*	IPv6 */
						(void)memcpy(&reply_buf->dst_addr[0], &rt_entry->dst_ip.v6, 16);
						reply_buf->flags = oal_htonl(2U); /* TODO: This is weird (see FCI doc). Some macro should be used instead. */
					}

					reply_buf->id = rt_entry->id;
					(void)strncpy(reply_buf->output_device, pfe_phy_if_get_name(rt_entry->iface), (uint32_t)IFNAMSIZ-1U);

					*fci_ret = FPP_ERR_OK;
					ret = EOK;

					break;
				}

				default:
				{
					NXP_LOG_WARNING("FPP_CMD_IP_ROUTE: Unknown action received: 0x%x\n", reply_buf->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}

			/* Unlock interfaces for required actions */
			if(FPP_ACTION_REGISTER == rt_cmd->action)
			{
				ret = pfe_if_db_unlock(session_id);
				if(EOK != ret)
				{
					NXP_LOG_ERROR("FPP_CMD_IP_ROUTE: DB unlock failed\n");
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Remove single route, inform clients, resolve dependencies
 * @param[in]	route The route to be removed
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
errno_t fci_routes_drop_one(fci_rt_db_entry_t *route)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_msg_t msg;
	fpp_rt_cmd_t *rt_cmd = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == route)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(&msg, 0, sizeof(fci_msg_t));
		msg.type = FCI_MSG_CMD;
		msg.msg_cmd.code = FPP_CMD_IP_ROUTE;

		rt_cmd = (fpp_rt_cmd_t *)msg.msg_cmd.payload;
		rt_cmd->action = FPP_ACTION_REMOVED;

		/*	Inform client about the entry is being removed */
		if (NULL != route->refptr)
		{
			rt_cmd->id = route->id;

			ret = fci_core_client_send((fci_core_client_t *)route->refptr, &msg, NULL);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("Could not notify FCI client\n");
			}
		}

		NXP_LOG_DEBUG("Removing route with ID %d\n", (int_t)oal_ntohl(route->id));

		/*	Remove all associated connections */
		fci_routes_remove_related_connections(route);

		/*	Remove the route */
		ret = fci_rt_db_remove(&fci_context->route_db, route);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't remove route: %d\n", ret);
		}
	}
	return ret;
}

/**
 * @brief		Remove all routes, inform clients, resolve dependencies
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
void fci_routes_drop_all(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *entry = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		NXP_LOG_DEBUG("Removing all routes\n");

		entry = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			ret = fci_routes_drop_one(entry);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Couldn't properly drop a route: %d\n", ret);
			}

			entry = fci_rt_db_get_next(&fci_context->route_db);
		}
	}
}

/**
 * @brief		Remove all IPv4 routes, inform clients, resolve dependencies
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
void fci_routes_drop_all_ipv4(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *entry = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		NXP_LOG_DEBUG("Removing all IPv4 routes\n");

		entry = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			if (entry->dst_ip.is_ipv4)
			{
				ret = fci_routes_drop_one(entry);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Couldn't properly drop a route: %d\n", ret);
				}
			}

			entry = fci_rt_db_get_next(&fci_context->route_db);
		}
	}
}

/**
 * @brief		Remove all IPv6 routes, inform clients, resolve dependencies
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
void fci_routes_drop_all_ipv6(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *entry = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		NXP_LOG_DEBUG("Removing all IPv6 routes\n");

		entry = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			if (!entry->dst_ip.is_ipv4)
			{
				ret = fci_routes_drop_one(entry);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Couldn't properly drop a route: %d\n", ret);
				}
			}

			entry = fci_rt_db_get_next(&fci_context->route_db);
		}
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */
/** @}*/
