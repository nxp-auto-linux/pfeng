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
 * @file		fci_l2br_domains.c
 * @brief		L2 bridge domains management functions.
 * @details		All bridge domains-related functionality provided by the FCI should be
 * 				implemented within this file. This includes mainly bridge domain-related
 * 				commands.
 *
 */

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: fci_l2br_domain_cmd */
static const pfe_ct_l2br_action_t fci_to_l2br_action[4] = {L2BR_ACT_FORWARD, L2BR_ACT_FLOOD, L2BR_ACT_PUNT, L2BR_ACT_DISCARD};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t fci_l2br_domain_remove(pfe_l2br_domain_t *domain);
static errno_t fci_l2br_domain_remove_if(pfe_l2br_domain_t *domain, const pfe_phy_if_t *phy_if);
uint32_t fci_l2br_static_entry_get_valid_fw_list(void);

/**
 * @brief			Process FPP_CMD_L2_BD commands
 * @param[in]		msg FCI message containing the FPP_CMD_L2_BD command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_l2_bd_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_l2br_domain_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_bd_cmd_t *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
	fpp_l2_bd_cmd_t *bd_cmd;
	errno_t ret = EOK;
	pfe_l2br_domain_t *domain = NULL;
	uint32_t ii;
	pfe_if_db_entry_t *if_db_entry = NULL;
	bool_t tag;
	pfe_phy_if_t *phy_if;
	uint32_t session_id;
	pfe_ct_vlan_stats_t stats = {0};

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
		if (*reply_len < sizeof(fpp_l2_bd_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_l2_bd_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			bd_cmd = (fpp_l2_bd_cmd_t *)(msg->msg_cmd.payload);

			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_l2_bd_cmd_t));

			ret = pfe_if_db_polled_lock(&session_id);

			if(EOK != ret)
			{
				NXP_LOG_WARNING("DB lock failed\n");
				*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
				ret = EOK;
			}
			else
			{
				switch (bd_cmd->action)
				{
					case FPP_ACTION_REGISTER:
					{
						/*	Check input values. We need to translate integer to pfe_ct_l2br_action_t therefore
							some validation needs to be performed first. */
						if ((bd_cmd->ucast_hit > 3U) || (bd_cmd->ucast_miss > 3U)
								|| (bd_cmd->mcast_hit > 3U) || (bd_cmd->mcast_miss > 3U))
						{
							NXP_LOG_WARNING("Unsupported action code received\n");
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						if (oal_ntohs(bd_cmd->vlan) <= 1U)
						{
							/*	0 - fall-back, 1 - default */
							NXP_LOG_WARNING("VLAN %d is reserved\n", oal_ntohs(bd_cmd->vlan));
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						/*	Add new bridge domain */
						ret = pfe_l2br_domain_create(fci_context->l2_bridge, oal_ntohs(bd_cmd->vlan));
						if (EPERM == ret)
						{
							NXP_LOG_WARNING("Domain %d already created\n", oal_ntohs(bd_cmd->vlan));
							*fci_ret = FPP_ERR_L2_BD_ALREADY_REGISTERED;
							ret = EOK;
							break;
						}
						else if (EOK != ret)
						{
							NXP_LOG_WARNING("Domain creation failed: %d\n", ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
						else
						{
							NXP_LOG_DEBUG("Bridge domain %d created\n", oal_ntohs(bd_cmd->vlan));
						}
						fallthrough;
					}

					case FPP_ACTION_UPDATE:
					{
						/*	Check input values. We need to translate integer to pfe_ct_l2br_action_t therefore
							some validation needs to be performed first. */
						if ((bd_cmd->ucast_hit > 3U) || (bd_cmd->ucast_miss > 3U)
								|| (bd_cmd->mcast_hit > 3U) || (bd_cmd->mcast_miss > 3U))
						{
							NXP_LOG_WARNING("Unsupported action code received\n");
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						/*	Get the domain instance (by VLAN) */
						domain = pfe_l2br_get_first_domain(fci_context->l2_bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)oal_ntohs(bd_cmd->vlan));
						if (NULL == domain)
						{
							/*	This shall never happen */
							NXP_LOG_WARNING("New domain not found\n");
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							ret = ENOENT;
							goto finalize_domain_registration;
						}

						/*	Set hit/miss actions */
						ret = pfe_l2br_domain_set_ucast_action(domain, fci_to_l2br_action[bd_cmd->ucast_hit], fci_to_l2br_action[bd_cmd->ucast_miss]);
						if (EOK != ret)
						{
							NXP_LOG_WARNING("Could not set uni-cast actions: %d\n", ret);
							goto finalize_domain_registration;
						}

						ret = pfe_l2br_domain_set_mcast_action(domain, fci_to_l2br_action[bd_cmd->mcast_hit], fci_to_l2br_action[bd_cmd->mcast_miss]);
						if (EOK != ret)
						{
							NXP_LOG_WARNING("Could not set multi-cast actions: %d\n", ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							goto finalize_domain_registration;
						}

						*fci_ret = FPP_ERR_OK;

						/*	Review if_list and untag_if_list and verify if contains valid interfaces. Valid interfaces
							are interfaces, which are known to the internal FCI database. Note that FCI API is using
							integer indexes to identify the physical interfaces but rest of SW works with pfe_phy_if_t
							instances. */
						for (ii=0U; ((ii < (8U * sizeof(bd_cmd->if_list))) && (ii <= (uint32_t)PFE_PHY_IF_ID_MAX)); ii++)
						{
							/*	Check if interface shall be added or removed */
							if (0U != (oal_ntohl(bd_cmd->if_list) & (1U << ii)))
							{
								/*	Only add interfaces which are known to platform interface database */
								ret = pfe_if_db_get_first(fci_context->phy_if_db, session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)ii, &if_db_entry);

								if(EOK != ret)
								{
									NXP_LOG_WARNING("DB was locked in different session, entry wasn't retrieved from DB\n");
									*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
									break;
								}

								if (NULL != if_db_entry)
								{
									/*	Got valid physical interface */
									tag = (0U == (oal_ntohl(bd_cmd->untag_if_list) & (1U << ii)));

									/* Get physical interface */
									phy_if = pfe_if_db_entry_get_phy_if(if_db_entry);

									/*	Add it to domain */
									ret = pfe_l2br_domain_add_if(domain, phy_if, tag);
									if (EEXIST == ret)
									{
										/*	Already added. Update = remove old -> add new. Update is only due
											to tag/untag flag. */
										ret = pfe_l2br_domain_del_if(domain, phy_if);
										if (EOK != ret)
										{
											NXP_LOG_ERROR("Could not update interface within bridge domain: %d\n", ret);
											break; /* break the 'for' loop */
										}

										ret = pfe_l2br_domain_add_if(domain, phy_if, tag);
										if (EOK != ret)
										{
											NXP_LOG_ERROR("Could not update interface within bridge domain: %d\n", ret);
											break; /* break the 'for' loop */
										}
										else
										{
											NXP_LOG_INFO("Domain %d: Interface %d updated\n", (int_t)oal_ntohs(bd_cmd->vlan), (int_t)ii);
										}
									}
									else if (EOK != ret)
									{
										NXP_LOG_ERROR("Could not add interface to bridge domain: %d\n", ret);
										break; /* break the 'for' loop */
									}
									else
									{
										/*	Added */
										NXP_LOG_INFO("Domain %d: Interface %d added\n", (int_t)oal_ntohs(bd_cmd->vlan), (int_t)ii);
									}
								}
								else
								{
									/*	Interface list contains interface not found in FCI database */
									NXP_LOG_WARNING("Interface %d not found\n", (int_t)ii);
									*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
									ret = EOK;
									break; /* break the 'for' loop */
								}
							}
							else
							{
								/*	Remove the interface if domain does contain it */
								phy_if = pfe_l2br_domain_get_first_if(domain, L2BD_IF_BY_PHY_IF_ID, (void *)(addr_t)ii);
								if (NULL != phy_if)
								{
									ret = fci_l2br_domain_remove_if(domain, phy_if);
									if (EOK != ret)
									{
										*fci_ret = FPP_ERR_INTERNAL_FAILURE;
										NXP_LOG_ERROR("Domain %d: Failed to remove interface %d\n", (int_t)oal_ntohs(bd_cmd->vlan), (int_t)ii);
										break;
									}
									else
									{
										NXP_LOG_INFO("Domain %d: Interface %d removed\n", (int_t)oal_ntohs(bd_cmd->vlan), (int_t)ii);
									}
								}
							}
						}

			finalize_domain_registration:
						if ((EOK != ret) && (FPP_ACTION_REGISTER == bd_cmd->action))
						{
							/*	New domain has not been properly created. Gracefully revert here. */
							if (EOK != fci_l2br_domain_remove(domain))
							{
								NXP_LOG_ERROR("Could not revert domain creation\n");
							}

							/*	Report failure */
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}

						break;
					}

					case FPP_ACTION_DEREGISTER:
					{
						if (oal_ntohs(bd_cmd->vlan) <= 1U)
						{
							/*	0 - fall-back, 1 - default */
							NXP_LOG_WARNING("VLAN %d is reserved\n", oal_ntohs(bd_cmd->vlan));
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							ret = EOK;
							break;
						}

						/*	Get and delete bridge domain */
						domain = pfe_l2br_get_first_domain(fci_context->l2_bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)oal_ntohs(bd_cmd->vlan));
						if (NULL == domain)
						{
							NXP_LOG_WARNING("Domain %d not found\n", oal_ntohs(bd_cmd->vlan));
							*fci_ret = FPP_ERR_L2_BD_NOT_FOUND;
							ret = EOK;
						}
						else
						{
							/*	Remove domain, release interfaces */
							ret = fci_l2br_domain_remove(domain);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("Could not destroy bridge domain: %d\n", ret);
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
							else
							{
								NXP_LOG_DEBUG("Bridge domain %d removed\n", oal_ntohs(bd_cmd->vlan));
							}
						}

						break;
					}

					case FPP_ACTION_QUERY:
					{
						domain = pfe_l2br_get_first_domain(fci_context->l2_bridge, L2BD_CRIT_ALL, NULL);
						if (NULL == domain)
						{
							ret = EOK;
							*fci_ret = FPP_ERR_L2_BD_NOT_FOUND;
							break;
						}
						fallthrough;
					}

					case FPP_ACTION_QUERY_CONT:
					{
						if (NULL == domain)
						{
							domain = pfe_l2br_get_next_domain(fci_context->l2_bridge);
							if (NULL == domain)
							{
								ret = EOK;
								*fci_ret = FPP_ERR_L2_BD_NOT_FOUND;
								break;
							}
						}

						/*	Write the reply buffer */
						bd_cmd = reply_buf;
						*reply_len = sizeof(fpp_l2_bd_cmd_t);

						/*	Build reply structure */
						if (EOK != pfe_l2br_domain_get_vlan(domain, &bd_cmd->vlan))
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
						else
						{
							/*	Endian... */
							bd_cmd->vlan = oal_htons(bd_cmd->vlan);
						}

						if (EOK != pfe_l2br_domain_get_ucast_action(domain, (pfe_ct_l2br_action_t *)&bd_cmd->ucast_hit, (pfe_ct_l2br_action_t *)&bd_cmd->ucast_miss))
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						if (EOK != pfe_l2br_domain_get_mcast_action(domain, (pfe_ct_l2br_action_t *)&bd_cmd->mcast_hit, (pfe_ct_l2br_action_t *)&bd_cmd->mcast_miss))
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						if (EOK != pfe_l2br_get_domain_stats(fci_context->l2_bridge, &stats, pfe_l2br_get_vlan_stats_index(domain)))
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						/* Copy the domain statistics to reply */
						(void)memcpy(&bd_cmd->stats, &stats, sizeof(bd_cmd->stats));

						if (TRUE == pfe_l2br_domain_is_default(domain))
						{
							bd_cmd->flags |= FPP_L2_BD_DEFAULT;
						}

						if (TRUE == pfe_l2br_domain_is_fallback(domain))
						{
							bd_cmd->flags |= FPP_L2_BD_FALLBACK;
						}

						bd_cmd->if_list = oal_htonl(pfe_l2br_domain_get_if_list(domain));
						bd_cmd->untag_if_list = oal_htonl(pfe_l2br_domain_get_untag_if_list(domain));

						*fci_ret = FPP_ERR_OK;
						ret = EOK;

						break;
					}

					default:
					{
						NXP_LOG_WARNING("FPP_CMD_L2_BD: Unknown action received: 0x%x\n", bd_cmd->action);
						*fci_ret = FPP_ERR_UNKNOWN_ACTION;
						break;
					}
				}

				if (EOK != pfe_if_db_unlock(session_id))
				{
					*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					NXP_LOG_ERROR("DB unlock failed\n");
				}
			}

		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_L2_STATIC_ENT commands
 * @param[in]		msg FCI message containing the FPP_CMD_L2_STATIC_ENT command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_l2_static_ent_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 */
errno_t fci_l2br_static_entry_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_static_ent_cmd_t *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;
	fpp_l2_static_ent_cmd_t *br_ent_cmd;
	pfe_l2br_static_entry_t *entry = NULL;
	pfe_mac_addr_t mac = { 0 };
	uint32_t valid_if_list = 0;

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
		if (*reply_len < sizeof(fpp_l2_static_ent_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_l2_static_ent_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			br_ent_cmd = (fpp_l2_static_ent_cmd_t *)(msg->msg_cmd.payload);

			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_l2_static_ent_cmd_t));

			switch (br_ent_cmd->action)
			{
				case FPP_ACTION_REGISTER:
				{
					valid_if_list = fci_l2br_static_entry_get_valid_fw_list();
					/* Check if valid logical interfaces are requested */
					if (oal_ntohl(br_ent_cmd->forward_list) != (oal_ntohl(br_ent_cmd->forward_list) & valid_if_list))
					{
						NXP_LOG_WARNING("Invalid interfaces in forward list\n");
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						ret = EOK;
					}
					else
					{
						(void)memcpy(mac, br_ent_cmd->mac, sizeof(pfe_mac_addr_t));
						ret = pfe_l2br_static_entry_create(fci_context->l2_bridge, oal_ntohs(br_ent_cmd->vlan), mac, oal_ntohl(br_ent_cmd->forward_list));
						if (EOK == ret) {
							NXP_LOG_DEBUG("Static entry %02x:%02x:%02x:%02x:%02x:%02x added to vlan %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], oal_ntohs(br_ent_cmd->vlan));
							*fci_ret = FPP_ERR_OK;
						}
						else if (EPERM == ret)
						{
							NXP_LOG_WARNING("Duplicit static entry %02x:%02x:%02x:%02x:%02x:%02x wasn't added to vlan %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], oal_ntohs(br_ent_cmd->vlan));
							*fci_ret = FPP_ERR_L2_STATIC_ENT_ALREADY_REGISTERED;
						}
						else
						{
							NXP_LOG_WARNING("Static entry %02x:%02x:%02x:%02x:%02x:%02x wasn't added to vlan %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], oal_ntohs(br_ent_cmd->vlan));
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
					}
					break;
				}
				case FPP_ACTION_UPDATE:
				{
					valid_if_list = fci_l2br_static_entry_get_valid_fw_list();
					/* Check if valid logical interfaces are requested */
					if (oal_ntohl(br_ent_cmd->forward_list) != (oal_ntohl(br_ent_cmd->forward_list) & valid_if_list))
					{
						NXP_LOG_WARNING("Invalid interfaces in forward list\n");
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						ret = EOK;
					}
					else
					{
						/* search for entry update fw list */
						entry = pfe_l2br_static_entry_get_first(fci_context->l2_bridge, L2SENT_CRIT_BY_MAC_VLAN, (void*)(addr_t)oal_ntohs(br_ent_cmd->vlan), (void*)br_ent_cmd->mac);
						if (NULL == entry)
						{
							*fci_ret = FPP_ERR_L2_STATIC_EN_NOT_FOUND;
						}
						else
						{
							*fci_ret = FPP_ERR_OK;

							if (EOK != pfe_l2br_static_entry_replace_fw_list(fci_context->l2_bridge, entry, oal_ntohl(br_ent_cmd->forward_list)))
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
							if (EOK != pfe_l2br_static_entry_set_local_flag(fci_context->l2_bridge, entry, br_ent_cmd->local))
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
							if (EOK != pfe_l2br_static_entry_set_src_discard_flag(fci_context->l2_bridge, entry, br_ent_cmd->src_discard))
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
							if (EOK != pfe_l2br_static_entry_set_dst_discard_flag(fci_context->l2_bridge, entry, br_ent_cmd->dst_discard))
							{
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
						}
					}
					break;
				}
				case FPP_ACTION_DEREGISTER:
				{
					/* search for entry and delete if the entery exists */
					entry = pfe_l2br_static_entry_get_first(fci_context->l2_bridge, L2SENT_CRIT_BY_MAC_VLAN, (void*)(addr_t)oal_ntohs(br_ent_cmd->vlan), (void*)br_ent_cmd->mac);
					if (NULL == entry)
					{
						*fci_ret = FPP_ERR_L2_STATIC_EN_NOT_FOUND;
					}
					else
					{
						*fci_ret = FPP_ERR_OK;

						if (EOK != pfe_l2br_static_entry_destroy(fci_context->l2_bridge, entry))
						{
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
					}
					break;
				}
				case FPP_ACTION_QUERY:
				{
					entry = pfe_l2br_static_entry_get_first(fci_context->l2_bridge, L2SENT_CRIT_ALL, NULL, NULL);
					if (NULL == entry)
					{
						ret = EOK;
						*fci_ret = FPP_ERR_L2_STATIC_EN_NOT_FOUND;
						break;
					}
					fallthrough;
				}
				case FPP_ACTION_QUERY_CONT:
				{
					if (NULL == entry)
					{
						entry = pfe_l2br_static_entry_get_next(fci_context->l2_bridge);
						if (NULL == entry)
						{
							ret = EOK;
							*fci_ret = FPP_ERR_L2_STATIC_EN_NOT_FOUND;
							break;
						}
					}

					/*	Write the reply buffer */
					br_ent_cmd = reply_buf;
					*reply_len = sizeof(fpp_l2_static_ent_cmd_t);

					/*	Build reply structure */
					/* VLAN */
					br_ent_cmd->vlan = pfe_l2br_static_entry_get_vlan(entry);
					br_ent_cmd->vlan = oal_htons(br_ent_cmd->vlan);
					/* MAC */
					pfe_l2br_static_entry_get_mac(entry, br_ent_cmd->mac);
					/* FW list */
					br_ent_cmd->forward_list = oal_htonl(pfe_l2br_static_entry_get_fw_list(entry));
					/* misc flags */
					(void)pfe_l2br_static_entry_get_local_flag(fci_context->l2_bridge, entry, (bool_t *)&br_ent_cmd->local);
					(void)pfe_l2br_static_entry_get_src_discard_flag(fci_context->l2_bridge, entry, (bool_t *)&br_ent_cmd->src_discard);
					(void)pfe_l2br_static_entry_get_dst_discard_flag(fci_context->l2_bridge, entry, (bool_t *)&br_ent_cmd->dst_discard);
					*fci_ret = FPP_ERR_OK;
					break;
				}
				default:
				{
					NXP_LOG_WARNING("FPP_CMD_L2_STATIC_ENT: Unknown action received: 0x%x\n", br_ent_cmd->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}

		}
	}
	return ret;
}

/**
 * @brief		Get valid inteface list
 * @retval		valid interface list
 */
uint32_t fci_l2br_static_entry_get_valid_fw_list(void)
{
	uint32_t ii;
	uint32_t session_id, valid_if_list = 0U;
	errno_t ret = EOK;
	const fci_t *fci_context = (fci_t *)&__context;
	pfe_if_db_entry_t *if_db_entry = NULL;

	ret = pfe_if_db_polled_lock(&session_id);
	if(EOK != ret)
	{
		NXP_LOG_WARNING("DB lock failed\n");
		valid_if_list = 0;
	}
	else
	{
		for (ii=0U; ((ii < (8U * sizeof(uint32_t))) && (ii <= (uint32_t)PFE_PHY_IF_ID_MAX)); ii++)
		{
				/*	Only add interfaces which are known to platform interface database */
				if_db_entry = NULL;
				ret = pfe_if_db_get_first(fci_context->phy_if_db, session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)ii, &if_db_entry);
				if (EOK != ret)
				{
					valid_if_list = 0;
					break;
				}
				if (NULL != if_db_entry)
				{
					valid_if_list |= ((uint32_t)1U << ii);
				}
		}

		if (EOK != pfe_if_db_unlock(session_id))
		{
			NXP_LOG_ERROR("DB unlock failed\n");
			valid_if_list = 0;
		}
	}

	return valid_if_list;
}

/**
 * @brief		Remove interface from domain and ensure it is properly re-configured if the interface
 * 				is not member of another domain.
 * @param[in]	domain Domain
 * @param[in]	phy_if Interface instance
 * @retval		EOK Success
 */
static errno_t fci_l2br_domain_remove_if(pfe_l2br_domain_t *domain, const pfe_phy_if_t *phy_if)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (NULL != phy_if)
		{
			ret = pfe_l2br_domain_del_if(domain, phy_if);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not remove interface from domain\n");
			}
			else
			{
				ret = pfe_l2br_domain_flush_by_if(domain, phy_if);
			}
		}
	}

	return ret;
}

/**
 * @brief		Gracefully remove bridge domain
 * @param[in]	domain The domain instance
 * @retval		EOK Success
 */
static errno_t fci_l2br_domain_remove(pfe_l2br_domain_t *domain)
{
	const pfe_phy_if_t *phy_if;
	errno_t ret = EOK;
	uint16_t vlan;

	if (NULL != domain)
	{
		/*	Remove all physical interfaces from the domain and adjust their properties. */
		phy_if = pfe_l2br_domain_get_first_if(domain, L2BD_IF_CRIT_ALL, NULL);
		while (NULL != phy_if)
		{
			/*	Remove from domain */
			if (EOK != fci_l2br_domain_remove_if(domain, phy_if))
			{
				NXP_LOG_ERROR("Interface removal failed\n");
			}
			else
			{
				if (EOK != pfe_l2br_domain_get_vlan(domain, &vlan))
				{
					NXP_LOG_ERROR("Could not get domain VLAN ID\n");
				}

				NXP_LOG_INFO("Domain %d: Interface %d removed\n", vlan, pfe_phy_if_get_id(phy_if));
			}

			/*	Get next */
			phy_if = pfe_l2br_domain_get_next_if(domain);
		}

		/*	Remove the domain instance */
		ret = pfe_l2br_domain_destroy(domain);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Fatal: Could not destroy bridge domain: %d\n", ret);
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
