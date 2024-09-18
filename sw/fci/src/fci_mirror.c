 /* =========================================================================
 *  Copyright 2021-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "fci_fp_db.h"
#include "fci_msg.h"
#include "fci.h"
#include "fci_internal.h"
#include "oal.h"
#include "fci_mirror.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief			Processes FPP_CMD_MIRROR commands
 * @param[in]		msg FCI message containing the FPP_CMD_MIRROR command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_mirror_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 */
errno_t fci_mirror_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_mirror_cmd_t *reply_buf, uint32_t *reply_len)
{
	fci_t *fci_context = (fci_t *)&__context;
	fpp_mirror_cmd_t *mirror_cmd;
	const char *str;
	errno_t ret = EOK;
	pfe_mirror_t *mirror = NULL;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	pfe_ct_phy_if_id_t egress_id;
	uint32_t addr;
	pfe_ct_route_actions_t m_actions;
	pfe_ct_route_actions_args_t m_args;

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
		*fci_ret = FPP_ERR_OK;

		/* Important to initialize to avoid buffer overflows */
		if (*reply_len < sizeof(fpp_mirror_cmd_t))
		{
			/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_mirror_cmd_t)\n");
			*fci_ret = FPP_ERR_INTERNAL_FAILURE;
			ret = EINVAL;
		}
		else
		{
			/* No data written to reply buffer (yet) */
			*reply_len = 0U;

			(void)memset(reply_buf, 0, sizeof(fpp_mirror_cmd_t));
			mirror_cmd = (fpp_mirror_cmd_t *)(msg->msg_cmd.payload);

			switch (mirror_cmd->action)
			{
				case FPP_ACTION_REGISTER:
				{
					/* Check that the requested mirror name is not already registered */
					mirror = pfe_mirror_get_first(MIRROR_BY_NAME, mirror_cmd->name);
					if (NULL != mirror)
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;

						/* FCI command attempted to register already registered entity. Respond with FCI error code. */
						NXP_LOG_WARNING("Mirror '%s' is already registered.\n", mirror_cmd->name);
						*fci_ret = FPP_ERR_MIRROR_ALREADY_REGISTERED;
						ret = EOK;
						break;
					}

					/* Create mirror */
					mirror = pfe_mirror_create(mirror_cmd->name);
					if (NULL == mirror)
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						NXP_LOG_WARNING("Cannot create mirror '%s'\n", mirror_cmd->name);
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						ret = EPERM;
						break;
					}
					fallthrough;
				}

				case FPP_ACTION_UPDATE:
				{
					if(NULL == mirror)
					{	/* Not the FALLTHRU case - obtain the mirror */
						/* Get mirror */
						mirror = pfe_mirror_get_first(MIRROR_BY_NAME, mirror_cmd->name);
						if(NULL == mirror)
						{
							/* FCI command requested nonexistent entity. Respond with FCI error code. */
							NXP_LOG_WARNING("No mirror with name '%s'\n", mirror_cmd->name);
							*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
							ret = EINVAL;
							break;
						}
					}

					/* 1) Set egress port */

					/* Lock interface db and get the requested interface */
					ret = pfe_if_db_polled_lock(&fci_context->if_session_id);
					if(EOK != ret)
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;
						
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
						ret = EOK;
						break;
					}

					(void)pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, mirror_cmd->egress_phy_if, &entry);
					if (NULL != entry)
					{
						phy_if = pfe_if_db_entry_get_phy_if(entry);
					}

					if((NULL == entry) || (NULL == phy_if))
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;

						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						(void)pfe_if_db_unlock(fci_context->if_session_id);
						NXP_LOG_WARNING("No interface '%s'\n", mirror_cmd->egress_phy_if);
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
						break;
					}

					/* Set the interface as mirror's egress port */
					egress_id = pfe_phy_if_get_id(phy_if);
					ret = pfe_mirror_set_egress_port(mirror, egress_id);
					if(EOK != ret)
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;

						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						(void)pfe_if_db_unlock(fci_context->if_session_id);
						NXP_LOG_ERROR("Cannot set egress port for '%s'\n", mirror_cmd->name);
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						break;
					}

					(void)pfe_if_db_unlock(fci_context->if_session_id);

					/* 2) Set filter to select frames */

					if('\0' == mirror_cmd->filter_table_name[0U])
					{	/* FCI command requests that the filter shall be disabled */

						/* Check if the mirror currently uses some filter. */
						addr = pfe_mirror_get_filter(mirror);
						if(0U != addr)
						{	/* Some filter (Flexible Parser table) is used. Get it and remove it from DMEM. */
							ret = fci_fp_db_get_table_from_addr(addr, (char **)&str);
							if(EOK != ret)
							{
								/*	Notify mirror module we are done working with the mirror instance. */
								pfe_mirror_put(mirror);
								mirror = NULL;

								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Cannot obtain filter name.\n");
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}
							else
							{
								(void)fci_fp_db_pop_table_from_hw((char *)str);
							}
						}

						/* Disable the filter */
						ret = pfe_mirror_set_filter(mirror, 0U);
						if(EOK != ret)
						{
							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_WARNING("Failed to disable filter on mirror '%s'.\n", mirror_cmd->name);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							ret = EOK;
							break;
						}
					}
					else
					{	/* FCI command requests that the filter shall be enabled or replaced by another one. */

						/* Check that the newly requested filter exists */
						if (NULL == fci_fp_db_get_first(FP_TABLE_CRIT_NAME, mirror_cmd->filter_table_name))
						{
							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

							/* FCI command requested nonexistent entity. Respond with FCI error code. */
							NXP_LOG_WARNING("Requested filter table '%s' does not exist.\n", mirror_cmd->filter_table_name);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;  /* TODO_BOB: Replace this with FPP_ERR_FP_TABLE_NOT_FOUND after AAVB-3369 gets implemented. */
							ret = EOK;
							break;
						}

						/* Check if the mirror currently uses some filter. */
						addr = pfe_mirror_get_filter(mirror);
						if(0U != addr)
						{	/* Some filter (Flexible Parser table) is used. Get it and remove it from DMEM. */
							ret = fci_fp_db_get_table_from_addr(addr, (char **)&str);
							if(EOK != ret)
							{
								/*	Notify mirror module we are done working with the mirror instance. */
								pfe_mirror_put(mirror);
								mirror = NULL;

								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Cannot obtain filter name.\n");
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}
							else
							{
								(void)fci_fp_db_pop_table_from_hw((char *)str);
							}
						}

						/* Set the filter */
						addr = fci_fp_db_get_table_dmem_addr((char_t *)mirror_cmd->filter_table_name);
						if(0U == addr)
						{	/* Requested filter table (from FCI command) is not used anywhere yet. Good. Use it as filter. */

							/* Add filter table to HW */
							ret = fci_fp_db_push_table_to_hw(fci_context->class, (char_t *)mirror_cmd->filter_table_name);
							addr = fci_fp_db_get_table_dmem_addr((char_t *)mirror_cmd->filter_table_name);

							/* Update filter address of mirror */
							ret = pfe_mirror_set_filter(mirror, addr);
							if(EOK != ret)
							{
								/*	Notify mirror module we are done working with the mirror instance. */
								pfe_mirror_put(mirror);
								mirror = NULL;

								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Failed to set filter %s to mirror %s\n", mirror_cmd->filter_table_name, mirror_cmd->name);
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}
						}
						else
						{	/* Requested filter table (from FCI command) is already used somewhere and cannot be used here. */

							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

							/* FCI command requested unfulfillable action. Respond with FCI error code. */
							NXP_LOG_WARNING("Filter '%s' already in use, but it should not be!\n", mirror_cmd->filter_table_name);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;  /* TODO_BOB: Replace this with FPP_ERR_FP_TABLE_ALREADY_IN_USE after AAVB-3369 gets implemented. */
							ret = EOK;
							break;
						}
					}

					/* 3) Set modification actions */

					mirror_cmd->m_actions = (fpp_modify_actions_t)oal_ntohl(mirror_cmd->m_actions);
					if(MODIFY_ACT_NONE == mirror_cmd->m_actions)
					{	/* No modifications */
						ret = pfe_mirror_set_actions(mirror, RT_ACT_NONE, NULL);
						if(EOK != ret)
						{
							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

							NXP_LOG_ERROR("Failed to set modification action: MODIFY_ACT_NONE.\n");
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}
					else
					{	/* Some actions to be set - add one by one */

						/* Initialize */
						(void)memset(&m_args, 0, sizeof(pfe_ct_route_actions_args_t));
						m_actions = RT_ACT_NONE;

						/* Start adding */
						if(0U != ((uint32_t)mirror_cmd->m_actions & (uint32_t)MODIFY_ACT_ADD_VLAN_HDR))
						{	/* VLAN header add/replace */
							m_args.vlan = mirror_cmd->m_args.vlan;
							m_actions |= RT_ACT_ADD_VLAN_HDR;
						}

						/* Apply */
						m_actions = (pfe_ct_route_actions_t) oal_htonl(m_actions);  /* PFE has modification actions in big endian. */
						ret = pfe_mirror_set_actions(mirror, m_actions, &m_args);
						if(EOK != ret)
						{
							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

							NXP_LOG_ERROR("Failed to set modification actions.\n");
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}

					/*	Notify mirror module we are done working with the mirror instance. */
					pfe_mirror_put(mirror);
					mirror = NULL;

					break;
				}

				case FPP_ACTION_DEREGISTER:
				{
					/* Get mirror */
					mirror = pfe_mirror_get_first(MIRROR_BY_NAME, mirror_cmd->name);
					if(NULL == mirror)
					{
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						NXP_LOG_WARNING("No mirror with name '%s'\n", mirror_cmd->name);
						*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
						ret = EOK;
						break;
					}

					/* Note filter address (if any). */
					addr = pfe_mirror_get_filter(mirror);

					/* Destroy the mirror. */
					ret = pfe_mirror_destroy(mirror);
					if (EBUSY == ret)
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;
						
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						NXP_LOG_WARNING("Cannot destroy a mirror (it is currently utilized).\n");
						*fci_ret = FPP_ERR_MIRROR_CURRENTLY_UTILIZED;
						ret = EOK;
						break;  /* Intentionally skip destruction of filter. We are keeping the mirror completely intact. */
					}
					else
					{
						mirror = NULL;  /* Mirror was destroyed. */

						/* Check if the mirror had some filter. */
						if(0U != addr)
						{	/* Some filter (Flexible Parser table) is used. Get it and remove it from DMEM. */
							ret = fci_fp_db_get_table_from_addr(addr, (char **)&str);
							if(EOK != ret)
							{
								/* No need to release the mirror instance here. The instance is already destroyed. */

								/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
								NXP_LOG_ERROR("Cannot obtain filter name.\n");
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break;
							}
							else
							{
								(void)fci_fp_db_pop_table_from_hw((char *)str);
							}
						}
					}

					/* No need to release the mirror instance here. The instance is already destroyed. */

					break;
				}

				case FPP_ACTION_QUERY:
				{
					/* Get the first mirror */
					mirror = pfe_mirror_get_first(MIRROR_ANY, NULL);
					if(NULL == mirror)
					{
						/* End of the query process (no more entities to report). Respond with FCI error code. */
						*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
						ret = EOK;
						break;
					}
					fallthrough;
				}

				case FPP_ACTION_QUERY_CONT:
				{
					/* If not fallthrough, then get the next mirror */
					if(NULL == mirror)
					{
						mirror = pfe_mirror_get_next();
						if(NULL == mirror)
						{
							/* End of the query process (no more entities to report). Respond with FCI error code. */
							*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
							ret = EOK;
							break;
						}
					}

					/* Get mirror name */
					str = pfe_mirror_get_name(mirror);
					(void)strncpy(reply_buf->name, str, sizeof(reply_buf->name) - 1U);

					/* Get egress port name, step #1 - find the egress interface in the interface db */
					egress_id = pfe_mirror_get_egress_port(mirror);
					ret = pfe_if_db_polled_lock(&fci_context->if_session_id);
					if(EOK != ret)
					{
						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;

						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
						ret = EOK;
						break;
					}

					(void)pfe_if_db_get_single(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)egress_id, &entry);
					if (NULL != entry)
					{
						phy_if = pfe_if_db_entry_get_phy_if(entry);
					}

					if((NULL == entry) || (NULL == phy_if))
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						(void)pfe_if_db_unlock(fci_context->if_session_id);
						NXP_LOG_ERROR("Cannot get egress interface of the mirror '%s'.\n", pfe_mirror_get_name(mirror));

						/*	Notify mirror module we are done working with the mirror instance. */
						pfe_mirror_put(mirror);
						mirror = NULL;

						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						ret = ENOENT;
						break;
					}

					/* Get egress port name, step #2 - get name of the egress interface */
					str = pfe_phy_if_get_name(phy_if);
					(void)strncpy(reply_buf->egress_phy_if, str, (uint32_t)IFNAMSIZ - 1U);
					reply_buf->egress_phy_if[(uint32_t)IFNAMSIZ - 1U] = '\0';  /* Ensure termination */
					(void)pfe_if_db_unlock(fci_context->if_session_id);

					/* Get filter name */
					(void)memset(reply_buf->filter_table_name, 0, IFNAMSIZ);
					addr = pfe_mirror_get_filter(mirror);
					if(0U != addr)
					{
						ret = fci_fp_db_get_table_from_addr(addr, (char **)&str);
						if(EOK == ret)
						{
							(void)strncpy(reply_buf->filter_table_name, str, 15);
							reply_buf->filter_table_name[15] = '\0';  /* Ensure termination */
						}
					}

					/* Initialize */
					(void)memset(&m_args, 0, sizeof(pfe_ct_route_actions_args_t));
					m_actions = RT_ACT_NONE;

					/* Get modification actions */
					reply_buf->m_actions = MODIFY_ACT_NONE;
					(void)pfe_mirror_get_actions(mirror, &m_actions, &m_args);
					m_actions = (pfe_ct_route_actions_t) oal_ntohl(m_actions);  /* PFE has modification actions in big endian. */
					if(0U != ((uint32_t)m_actions & (uint32_t)RT_ACT_ADD_VLAN_HDR))
					{
						reply_buf->m_actions |= MODIFY_ACT_ADD_VLAN_HDR;
						reply_buf->m_args.vlan = m_args.vlan;
					}
					reply_buf->m_actions = (fpp_modify_actions_t)oal_htonl(reply_buf->m_actions);

					/*	Notify mirror module we are done working with the mirror instance. */
					pfe_mirror_put(mirror);
					mirror = NULL;

					/* Set reply length end return OK */
					*reply_len = sizeof(fpp_mirror_cmd_t);
					ret = EOK;

					break;
				}

				default:
				{
					/* Unknown command. Respond with FCI error code. */
					NXP_LOG_WARNING("FPP_CMD_MIRROR command: Unknown action received: 0x%x\n", mirror_cmd->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					ret = EOK;
					break;
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
