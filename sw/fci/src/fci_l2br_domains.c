/* =========================================================================
 *  Copyright 2018-2020 NXP
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

static errno_t fci_l2br_domain_remove(pfe_l2br_domain_t *domain);
static errno_t fci_l2br_domain_remove_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *phy_if);

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
	fci_t *context = (fci_t *)&__context;
	fpp_l2_bd_cmd_t *bd_cmd;
	errno_t ret = EOK;
	pfe_l2br_domain_t *domain = NULL;
	static const pfe_ct_l2br_action_t fci_to_l2br_action[4] = {L2BR_ACT_FORWARD, L2BR_ACT_FLOOD, L2BR_ACT_PUNT, L2BR_ACT_DISCARD};
	uint32_t ii;
	pfe_if_db_entry_t *if_db_entry = NULL;
	bool_t tag;
	pfe_phy_if_t *phy_if;
	uint32_t session_id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
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
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_l2_bd_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_l2_bd_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	bd_cmd = (fpp_l2_bd_cmd_t *)(msg->msg_cmd.payload);

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_l2_bd_cmd_t));

	ret = pfe_if_db_lock(&session_id);

	if(EOK != ret)
	{
		*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
		return EOK;
	}

	switch (bd_cmd->action)
	{
		case FPP_ACTION_REGISTER:
		{
			/*	Check input values. We need to translate integer to pfe_ct_l2br_action_t therefore
			 	some validation needs to be performed first. */
			if ((bd_cmd->ucast_hit > 3) || (bd_cmd->ucast_miss > 3)
					|| (bd_cmd->mcast_hit > 3) || (bd_cmd->mcast_miss > 3))
			{
				NXP_LOG_ERROR("Unsupported action code received\n");
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}

			if (oal_ntohs(bd_cmd->vlan) <= 1U)
			{
				/*	0 - fall-back, 1 - default */
				NXP_LOG_ERROR("VLAN %d is reserved\n", oal_ntohs(bd_cmd->vlan));
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}

			/*	Add new bridge domain */
			ret = pfe_l2br_domain_create(context->l2_bridge, oal_ntohs(bd_cmd->vlan));
			if (EPERM == ret)
			{
				NXP_LOG_ERROR("Domain %d already created\n", oal_ntohs(bd_cmd->vlan));
				*fci_ret = FPP_ERR_L2BRIDGE_DOMAIN_ALREADY_REGISTERED;
				ret = EOK;
				break;
			}
			else if (EOK != ret)
			{
				NXP_LOG_ERROR("Domain creation failed: %d\n", ret);
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				break;
			}
			else
			{
				NXP_LOG_DEBUG("Bridge domain %d created\n", oal_ntohs(bd_cmd->vlan));
			}
		}
		/*	no break */

		case FPP_ACTION_UPDATE:
		{
			/*	Check input values. We need to translate integer to pfe_ct_l2br_action_t therefore
				some validation needs to be performed first. */
			if ((bd_cmd->ucast_hit > 3) || (bd_cmd->ucast_miss > 3)
					|| (bd_cmd->mcast_hit > 3) || (bd_cmd->mcast_miss > 3))
			{
				NXP_LOG_ERROR("Unsupported action code received\n");
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}

			/*	Get the domain instance (by VLAN) */
			domain = pfe_l2br_get_first_domain(context->l2_bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)oal_ntohs(bd_cmd->vlan));
			if (NULL == domain)
			{
				/*	This shall never happen */
				NXP_LOG_DEBUG("New domain not found\n");
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				ret = ENOENT;
				goto finalize_domain_registration;
			}

			/*	Set hit/miss actions */
			ret = pfe_l2br_domain_set_ucast_action(domain, fci_to_l2br_action[bd_cmd->ucast_hit], fci_to_l2br_action[bd_cmd->ucast_miss]);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Could not set uni-cast actions: %d\n", ret);
				goto finalize_domain_registration;
			}

			ret = pfe_l2br_domain_set_mcast_action(domain, fci_to_l2br_action[bd_cmd->mcast_hit], fci_to_l2br_action[bd_cmd->mcast_miss]);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Could not set multi-cast actions: %d\n", ret);
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				goto finalize_domain_registration;
			}

			*fci_ret = FPP_ERR_OK;

			/*	Review if_list and untag_if_list and verify if contains valid interfaces. Valid interfaces
			 	are interfaces, which are known to the internal FCI database. Note that FCI API is using
			 	integer indexes to identify the physical interfaces but rest of SW works with pfe_phy_if_t
			 	instances. */
			for (ii=0U; ((ii < (8U * sizeof(bd_cmd->if_list))) && (ii <= PFE_PHY_IF_ID_MAX)); ii++)
			{
				/*	Check if interface shall be added or removed */
				if (0U != (oal_ntohl(bd_cmd->if_list) & (1U << ii)))
				{
					/*	Only add interfaces which are known to platform interface database */
					ret = pfe_if_db_get_first(context->phy_if_db, session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)ii, &if_db_entry);

					if(EOK != ret)
					{
						NXP_LOG_DEBUG("DB was locked in different session, entry wasn't retrieved from DB\n");
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
								NXP_LOG_INFO("Domain %d: Interface %d updated\n", oal_ntohs(bd_cmd->vlan), ii);
							}
						}
						else if (EOK != ret)
						{
							NXP_LOG_ERROR("Could not add interface to bridge domain: %d\n", ret);
							break; /* break the 'for' loop */
						}
						else
						{
							/*	Added. Enable promiscuous mode. */
							NXP_LOG_INFO("Domain %d: Interface %d added, enabling promiscuous mode\n", oal_ntohs(bd_cmd->vlan), ii);
							ret = pfe_phy_if_promisc_enable(phy_if);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("Could not set promiscuous mode: %d\n", ret);

								/*	Remove interface from the domain */
								if (EOK != fci_l2br_domain_remove_if(domain, phy_if))
								{
									NXP_LOG_ERROR("Interface removal failed\n");
								}

								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break; /* break the 'for' loop */
							}

							/*	Set the interface to be in BRIDGE mode */
							ret = pfe_phy_if_set_op_mode(phy_if, IF_OP_VLAN_BRIDGE);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("Interface could not be configured for BRIDGE mode: %d\n", ret);

								/*	Remove interface from the domain (revert) */
								if (EOK != fci_l2br_domain_remove_if(domain, phy_if))
								{
									NXP_LOG_ERROR("Interface removal failed\n");
								}

								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								break; /* break the 'for' loop */
							}

							/*	Enable the interface */
							ret = fci_enable_if(phy_if);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("Unable to enable interface (%s): %d\n", pfe_phy_if_get_name(phy_if), ret);
								*fci_ret = FPP_ERR_OK;

								/*	Remove interface from the domain (revert) */
								if (EOK != fci_l2br_domain_remove_if(domain, phy_if))
								{
									NXP_LOG_ERROR("Interface removal failed\n");
								}

								break; /* break the 'for' loop */
							}
						}
					}
					else
					{
						/*	Interface list contains interface not found in FCI database */
						NXP_LOG_ERROR("Interface %d not found\n", ii);
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
							break;
						}
						else
						{
							/*	Disable interface */
							ret = fci_disable_if(phy_if);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("Unable to disable interface (%s): %d\n", pfe_phy_if_get_name(phy_if), ret);
								*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							}
							else
							{
								NXP_LOG_INFO("Domain %d: Interface %d removed\n", oal_ntohs(bd_cmd->vlan), ii);
							}
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
				NXP_LOG_ERROR("VLAN %d is reserved\n", oal_ntohs(bd_cmd->vlan));
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}

			/*	Get and delete bridge domain */
			domain = pfe_l2br_get_first_domain(context->l2_bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)oal_ntohs(bd_cmd->vlan));
			if (NULL == domain)
			{
				NXP_LOG_ERROR("Domain %d not found\n", oal_ntohs(bd_cmd->vlan));
				*fci_ret = FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND;
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
			domain = pfe_l2br_get_first_domain(context->l2_bridge, L2BD_CRIT_ALL, NULL);
			if (NULL == domain)
			{
				ret = EOK;
				*fci_ret = FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND;
				break;
			}
		}
		/*	No break */

		case FPP_ACTION_QUERY_CONT:
		{
			if (NULL == domain)
			{
				domain = pfe_l2br_get_next_domain(context->l2_bridge);
				if (NULL == domain)
				{
					ret = EOK;
					*fci_ret = FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND;
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

			if (TRUE == pfe_l2br_domain_is_default(domain))
			{
				bd_cmd->flags |= oal_htonl(FPP_L2BR_DOMAIN_DEFAULT);
			}

			if (TRUE == pfe_l2br_domain_is_fallback(domain))
			{
				bd_cmd->flags |= oal_htonl(FPP_L2BR_DOMAIN_FALLBACK);
			}

			bd_cmd->if_list = oal_htonl(pfe_l2br_domain_get_if_list(domain));
			bd_cmd->untag_if_list = oal_htonl(pfe_l2br_domain_get_untag_if_list(domain));

			fci_ret = FPP_ERR_OK;
			ret = EOK;

			break;
		}

		default:
		{
			NXP_LOG_ERROR("FPP_CMD_L2_BD: Unknown action received: 0x%x\n", bd_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}

	if (EOK != pfe_if_db_unlock(session_id))
	{
		*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
		NXP_LOG_DEBUG("DB unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Remove interface from domain and ensure it is properly re-configured if the interface
 * 				is not member of another domain.
 * @param[in]	domain Domain
 * @param[in]	phy_if Interface instance
 * @retval		EOK Success
 */
static errno_t fci_l2br_domain_remove_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *phy_if)
{
	fci_t *context = (fci_t *)&__context;
	errno_t ret = EOK;
	pfe_ct_phy_if_id_t id = PFE_PHY_IF_ID_INVALID;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain)))
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

	if (NULL != phy_if)
	{
		ret = pfe_l2br_domain_del_if(domain, phy_if);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Could not remove interface from domain\n");
			return ret;
		}
		else
		{
			/*	Find out if there is another domain containing given physical interface */
			if (NULL != pfe_l2br_get_first_domain(context->l2_bridge, L2BD_BY_PHY_IF, phy_if))
			{
				/*	Interface is still member of some bridge domain */
				;
			}
			else
			{
				/*	Get ID (sanity check) */
				id = pfe_phy_if_get_id(phy_if);

				NXP_LOG_INFO("Interface %d is not member of any bridge domain. Setting default operational mode.\n", id);

				ret = pfe_phy_if_set_op_mode(phy_if, IF_OP_DEFAULT);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Could not set interface operational mode\n");
					return ret;
				}
				else
				{
					NXP_LOG_INFO("Interface %d: Disabling promiscuous mode\n", id);
					ret = pfe_phy_if_promisc_disable(phy_if);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not disable promiscuous mode: %d\n", ret);
						return ret;
					}
				}
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
	pfe_phy_if_t *phy_if;
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
				NXP_LOG_WARNING("Interface removal failed\n");
			}
			else
			{
				if (EOK != pfe_l2br_domain_get_vlan(domain, &vlan))
				{
					NXP_LOG_DEBUG("Could not get domain VLAN ID\n");
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


/** @}*/
