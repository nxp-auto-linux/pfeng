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
 * @file		fci_interfaces.c
 * @brief		Ethernet interfaces management functions.
 * @details		All interfaces-related functionality provided by the FCI should be
 * 				implemented within this file. This includes commmands dedicated
 *				to register and unregister interface to/from the FCI.
 *
 */

#include "pfe_cfg.h"
#include "pfe_platform_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "fci_internal.h"
#include "fci_fp_db.h"
#include "fci.h"
#include "pfe_mirror.h"
#include "pfe_feature_mgr.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t fci_interfaces_get_arg_info(fpp_if_m_args_t *m_arg, pfe_ct_if_m_rules_t rule, void **offset, size_t *size, uint32_t *fp_table_addr);
static errno_t fci_interfaces_destroy_fptables(const fpp_if_m_rules_t match, const pfe_ct_if_m_args_t* args);

/*
 * @brief			Get offset and size of the rule
 * @details			Errors are handled in platform driver
 * @param[in]		m_args pointer to the argument structure
 * @param[in]		rule single rule. See pfe_ct_if_m_rules_t
 * @param[in,out]	offset is set based on the rule to the structure m_arg
 * @param[in,out]	size of the underlying type in the struct based on the rule
 */
static errno_t fci_interfaces_get_arg_info(fpp_if_m_args_t *m_arg, pfe_ct_if_m_rules_t rule, void **offset, size_t *size, uint32_t *fp_table_addr)
{
	errno_t retval = EOK; /* Function return value */
	uint32_t table_addr;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == m_arg) || (NULL == offset) || (NULL == size)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		retval = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
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
				*size = sizeof(m_arg->ipv.v6.sip);
				*offset = &m_arg->ipv.v6.sip;
				break;
			}

			case IF_MATCH_DIP6:
			{
				*size = sizeof(m_arg->ipv.v6.dip);
				*offset = &m_arg->ipv.v6.dip;
				break;
			}

			case IF_MATCH_SIP:
			{
				*size = sizeof(m_arg->ipv.v4.sip);
				*offset = &m_arg->ipv.v4.sip;
				break;
			}

			case IF_MATCH_DIP:
			{
				*size = sizeof(m_arg->ipv.v4.dip);
				*offset = &m_arg->ipv.v4.dip;
				break;
			}

			case IF_MATCH_ETHTYPE:
			{
				*size = sizeof(m_arg->ethtype);
				*offset = &m_arg->ethtype;
				break;
			}

			case IF_MATCH_FP0:
			{
				/* Get the table address in the HW */
				table_addr = fci_fp_db_get_table_dmem_addr(m_arg->fp_table0);
				*fp_table_addr = oal_htonl(table_addr);
				if(0U == *fp_table_addr)
				{
					retval = ENOENT;
				}
				*offset = fp_table_addr;
				*size = sizeof(uint32_t);
				break;
			}

			case IF_MATCH_FP1:
			{
				/* Get the table address in the HW */
				table_addr = fci_fp_db_get_table_dmem_addr(m_arg->fp_table1);
				*fp_table_addr = oal_htonl(table_addr);
				if(0U == *fp_table_addr)
				{
					retval = ENOENT;
				}
				*offset = fp_table_addr;
				*size = sizeof(uint32_t);
				break;
			}

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

			case IF_MATCH_HIF_COOKIE:
			{
				*size = sizeof(m_arg->hif_cookie);
				*offset = &m_arg->hif_cookie;
				break;
			}

			default:
			{
				*size = 0U;
				*offset = NULL;
				break;
			}
		}
	}
	return retval;
}

/**
 * @brief			Destroy FP tables if they are used.
 * 					Auxiliary function for logical interface processing.
 * @param[in]		match	Match rules of a logical interface.
 * @param[in]		args	Match rule arguments of a logical interface.
 * @return			EOK if success, error code otherwise
 */
static errno_t fci_interfaces_destroy_fptables(const fpp_if_m_rules_t match, const pfe_ct_if_m_args_t* args)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == args)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		char_t *table_name = NULL;

		if((uint32_t)FPP_IF_MATCH_FP0 == ((uint32_t)match & (uint32_t)FPP_IF_MATCH_FP0))
		{	/* A flexible parser table was dropped - it needs to be destroyed if it existed */
			if(0U != args->fp0_table)
			{	/* Table existed */
				(void)fci_fp_db_get_table_from_addr(args->fp0_table, &table_name);
				(void)fci_fp_db_pop_table_from_hw(table_name);
			}
		}
		if((uint32_t)FPP_IF_MATCH_FP1 == ((uint32_t)match & (uint32_t)FPP_IF_MATCH_FP1))
		{	/* A flexible parser table was dropped - it needs to be destroyed if it existed */
			if(0U != args->fp1_table)
			{	/* Table existed */
				(void)fci_fp_db_get_table_from_addr(args->fp1_table, &table_name);
				(void)fci_fp_db_pop_table_from_hw(table_name);
			}
		}
		ret = EOK;
	}
	return ret;
}

/**
 * @brief			Process interface atomic session related commands
 * @param[in]		msg FCI cmd code
 * @param[out]		fci_ret FCI return code
 * @return			EOK if success, error code otherwise
 */
errno_t fci_interfaces_session_cmd(uint32_t code, uint16_t *fci_ret)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fci_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch(code)
		{
			case FPP_CMD_IF_LOCK_SESSION:
			{
				*fci_ret = FPP_ERR_OK;
				if (EOK != pfe_if_db_polled_lock(&fci_context->if_session_id))
				{
					*fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
					NXP_LOG_WARNING("DB lock failed\n");
				}
				break;
			}
			case FPP_CMD_IF_UNLOCK_SESSION:
			{
				*fci_ret = FPP_ERR_OK;
				if (EOK != pfe_if_db_unlock(fci_context->if_session_id))
				{
					*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					NXP_LOG_WARNING("DB unlock failed due to incorrect session ID\n");
				}
				break;
			}
			default:
			{
				NXP_LOG_WARNING("Unknown Interface Session Command Received\n");
				*fci_ret = FPP_ERR_UNKNOWN_ACTION;
				break;
			}
		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_LOG_IF commands
 * @param[in]		msg FCI message containing the FPP_CMD_LOG_IF command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_log_if_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with interface DB session lock.
 */
errno_t fci_interfaces_log_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_log_if_cmd_t *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
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
	uint32_t fp_table_addr;
	uint32_t fp_table_destroy[2];
	char_t *table_name;
	pfe_ct_class_algo_stats_t stats = { 0 };

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
		if (*reply_len < sizeof(fpp_log_if_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_if_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_log_if_cmd_t));

			if_cmd = (fpp_log_if_cmd_t *)msg->msg_cmd.payload;

			/* Check if IF_DB lock is acquired */
			ret = pfe_if_db_lock_check(fci_context->if_session_id);
			if(EOK != ret)
			{
				/* FCI command requested unfulfillable action. Respond with FCI error code. */
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				return EOK;
			}

			switch(if_cmd->action)
			{
				case FPP_ACTION_REGISTER:
				{
					/* Get the intended parent physical interface */
					ret = pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->parent_name, &entry);
					if(EOK != ret)
					{
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}
					phy_if = pfe_if_db_entry_get_phy_if(entry);
					if(NULL == phy_if)
					{
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = ENOENT;
						break;
					}
					/* Create the logical interface */
					log_if = pfe_log_if_create(phy_if, if_cmd->name);
					if(NULL == log_if)
					{
						*fci_ret = FPP_ERR_IF_OP_CANNOT_CREATE;
						ret = ENOENT;
						break;
					}
					/* Add the interface into the database */
					ret = pfe_if_db_add(fci_context->log_if_db, fci_context->if_session_id, log_if, pfe_phy_if_get_id(phy_if));
					if(EOK != ret)
					{
						pfe_log_if_destroy(log_if);
						*fci_ret = FPP_ERR_IF_OP_CANNOT_CREATE;
						break;
					}
					NXP_LOG_INFO("Added logical interface %s to physical interface %s\n", if_cmd->name, if_cmd->parent_name);
					break;
				}

				case FPP_ACTION_DEREGISTER:
				{
					ret = pfe_if_db_get_first(fci_context->log_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->name, &entry);

					if(EOK != ret)
					{
						NXP_LOG_WARNING("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
						break;
					}

					/* Check if entry is not NULL and get logical interface */
					if(NULL != entry)
					{
						log_if = pfe_if_db_entry_get_log_if(entry);
					}

					/* Check if the entry exists */
					if((NULL == entry) || (NULL == log_if))
					{
						/* Interface doesn't exist or couldn't be extracted from the entry */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = ENOENT;
						break;
					}

					/* Destroy FP tables if they were used by this interface. */
					if(EOK == pfe_log_if_get_match_rules(log_if, &rules, &args))
					{
						/* Fix endians of FP tables */
						args.fp0_table = oal_ntohl(args.fp0_table);
						args.fp1_table = oal_ntohl(args.fp1_table);

						/* Destroy FP tables */
						(void)fci_interfaces_destroy_fptables((fpp_if_m_rules_t)rules, &args);
					}

					/* Remove interface from the database */
					(void)pfe_if_db_remove(fci_context->log_if_db, fci_context->if_session_id, entry);
					/* Destroy the interface */
					pfe_log_if_destroy(log_if);
					break;
				}

				case FPP_ACTION_UPDATE:
				{
					*fci_ret = FPP_ERR_OK;
					*reply_len = sizeof(fpp_log_if_cmd_t);

					ret = pfe_if_db_get_first(fci_context->log_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->name, &entry);

					if(EOK != ret)
					{
						NXP_LOG_WARNING("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
						break;
					}

					/* Check if entry is not NULL and get logical interface */
					if(NULL != entry)
					{
						log_if = pfe_if_db_entry_get_log_if(entry);
					}

					/* Check if the entry exists */
					if((NULL == entry) || (NULL == log_if))
					{
						/* Interface doesn't exist or couldn't be extracted from the entry */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}

					/* Get the currently set rules */
					ret = pfe_log_if_get_match_rules(log_if, &rules, &args);
					if(ret != EOK)
					{
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}

					/* Do not allow simultaneous use of IPv4 and IPv6 match rules. */
					if (((uint32_t)(FPP_IF_MATCH_SIP  | FPP_IF_MATCH_DIP)  & oal_ntohl(if_cmd->match)) &&
						((uint32_t)(FPP_IF_MATCH_SIP6 | FPP_IF_MATCH_DIP6) & oal_ntohl(if_cmd->match)))
					{
						/* FCI command requested unfulfillable action. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
						break;
					}

					/* Fix endians of FP tables */
					args.fp0_table = oal_ntohl(args.fp0_table);
					args.fp1_table = oal_ntohl(args.fp1_table);

					rules = (pfe_ct_if_m_rules_t)(~oal_ntohl(if_cmd->match));

					/* Drop all unset rules (if any) */
					ret = pfe_log_if_del_match_rule(log_if, rules);

					/* Destroy FP tables if they are not used by new rules */
					(void)fci_interfaces_destroy_fptables((fpp_if_m_rules_t)rules, &args);

					if(EOK == ret)
					{
						NXP_LOG_INFO("All match rules were dropped on %s before match rule update.\n",  pfe_log_if_get_name(log_if));
					}
					else
					{
						NXP_LOG_WARNING("Dropping of all match rules on logical interface %s failed !!\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
					}
					/* Clear the storage for queues to be destroyed */
					fp_table_destroy[0] = 0U;
					fp_table_destroy[1] = 0U;
					/* We are going to configure Flexible parser - prepare table(s) */
					if((uint32_t)FPP_IF_MATCH_FP0 == (oal_ntohl(if_cmd->match) & (uint32_t)FPP_IF_MATCH_FP0))
					{
						/* Get the newly configured table address */
						fp_table_addr = fci_fp_db_get_table_dmem_addr(if_cmd->arguments.fp_table0);
						if(0U == fp_table_addr)
						{   /* Table has not been created yet */
							ret = fci_fp_db_push_table_to_hw(fci_context->class, if_cmd->arguments.fp_table0);
							if(EOK != ret)
							{   /* Failed to write */
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
								break;
							}
							/* We have just created the table therefore the existing one must be different
							and it needs to be destroyed before we overwrite the reference */
							if(0U != args.fp0_table)
							{
								/* Table is still in use therefore it cannot be destroyed,
								just remember it */
								fp_table_destroy[0] = args.fp0_table;
							}
						}
						else
						{   /* Table does exist */
							/* Check whether it is already configured */
							if(fp_table_addr != args.fp0_table)
							{   /* Different table is configured thus the new one must be in use
								somewhere else (because it does have the address) and cannot be
								used here */
								NXP_LOG_WARNING("Table %s already in use.\n", if_cmd->arguments.fp_table0);
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
								break;
							}

						}
					}
					if((uint32_t)FPP_IF_MATCH_FP1 == (oal_ntohl(if_cmd->match) & (uint32_t)FPP_IF_MATCH_FP1))
					{
						/* Get the newly configured table address */
						fp_table_addr = fci_fp_db_get_table_dmem_addr(if_cmd->arguments.fp_table1);
						if(0U == fp_table_addr)
						{   /* Table has not been created yet */
							ret = fci_fp_db_push_table_to_hw(fci_context->class, if_cmd->arguments.fp_table1);
							if(EOK != ret)
							{   /* Failed to write */
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
								break;
							}
							/* We have just created the table therefore the existing one must be different
							and it needs to be destroyed before we overwrite the reference */
							if(0U != args.fp1_table)
							{
								/* Table is still in use therefore it cannot be destroyed,
								just remember it */
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
								break;
							}
						}
						else
						{   /* Table does exist */
							/* Check whether it is already configured */
							if(fp_table_addr != args.fp1_table)
							{   /* Different table is configured thus the new one must be in use
								somewhere else (because it does have the address) and cannot be
								used here */
								NXP_LOG_WARNING("Table %s already in use.\n", if_cmd->arguments.fp_table1);
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
								break;
							}

						}
					}

					/* Update each rule one by one */
					for(index = 0U; (8U * sizeof(if_cmd->match)) > index;  ++index)
					{
						if(0U != (oal_ntohl(if_cmd->match) & (1UL << index)))
						{
							/* Resolve position of data and size */
							ret = fci_interfaces_get_arg_info(&if_cmd->arguments, (pfe_ct_if_m_rules_t)(oal_ntohl(if_cmd->match) & (1UL << index)), &offset, &size, &fp_table_addr);
							if(EOK != ret)
							{
								NXP_LOG_WARNING("Failed to get update argument\n");
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
							}

							/* Add match rule and arguments */
							ret = pfe_log_if_add_match_rule(log_if, (pfe_ct_if_m_rules_t)(oal_ntohl(if_cmd->match) & (1UL << index)), offset, size);

							if(EOK != ret)
							{
								NXP_LOG_WARNING("Updating single rule on logical interface %s failed !!\n",  pfe_log_if_get_name(log_if));
								*fci_ret = FPP_ERR_IF_MATCH_UPDATE_FAILED;
							}
						}
					}

					/* Now is the time to destroy Flexible Parser tables no longer in use */
					if(0U != fp_table_destroy[0])
					{
						(void)fci_fp_db_get_table_from_addr(fp_table_destroy[0], &table_name);
						(void)fci_fp_db_pop_table_from_hw(table_name);
					}
					if(0U != fp_table_destroy[1])
					{
						(void)fci_fp_db_get_table_from_addr(fp_table_destroy[1], &table_name);
						(void)fci_fp_db_pop_table_from_hw(table_name);
					}

					/* Update egress in case at least one is set (old egress is dropped) */
					if(0U != if_cmd->egress)
					{
						NXP_LOG_INFO("Updating egress interfaces on %s (0x%x)\n",  pfe_log_if_get_name(log_if), (uint_t)oal_ntohl(if_cmd->egress));
						for(index = 0U; (uint32_t)PFE_PHY_IF_ID_INVALID > index;  ++index)
						{

							/* Linux PFE Driver does not implement HIF NOCPY. This is by design. */
							if(((uint32_t)PFE_PHY_IF_ID_HIF == index) || ((uint32_t)PFE_PHY_IF_ID_HIF_NOCPY == index))
							{
								/* Skip currently not used interfaces */
								continue;
							}

							/* For each bit in egress mask search if the phy if exists */
							ret = pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)index, &entry);
							if((EOK == ret) && (NULL != entry))
							{   /* phy if does exist */
								phy_if = pfe_if_db_entry_get_phy_if(entry);

								/* Check whether the phy if shall be added
								We are getting inputs in network order thus conversion is needed */
								if(0U != (oal_ntohl(if_cmd->egress) & (1UL << index)))
								{   /* Add */
									/* If the ID exits add corresponding phy_if as egress to log_if*/
									if (EOK != pfe_log_if_add_egress_if(log_if, phy_if))
									{
										NXP_LOG_ERROR("Could not set egress interface for %s\n", pfe_log_if_get_name(log_if));
										*fci_ret = FPP_ERR_IF_EGRESS_UPDATE_FAILED;
									}
								}
								else
								{   /* Do not add (drop from the list if already on the list) */
									/* Get current egress interfaces */
									ret = pfe_log_if_get_egress_ifs(log_if, &egress);
									if(EOK == ret)
									{
										if(0U != (egress & ((uint32_t)1U << index)))
										{   /* Interface is on the current list but not on the requested list - drop it */
											ret = pfe_log_if_del_egress_if(log_if, phy_if);
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
								NXP_LOG_WARNING("Egress %u on %s is not set because it doesn't exist\n", (uint_t)index,  pfe_log_if_get_name(log_if));

								/* Error in input do not continue */
								*fci_ret = FPP_ERR_IF_EGRESS_DOESNT_EXIST;
							}
						}
					}

					/* AND/OR rules */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_MATCH_OR))
					{
						ret = pfe_log_if_set_match_or(log_if);
					}
					else
					{
						ret = pfe_log_if_set_match_and(log_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("AND/OR flag wasn't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/* enable/disable */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_ENABLED))
					{
						ret = pfe_log_if_enable(log_if);
					}
					else
					{
						ret = pfe_log_if_disable(log_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("ENABLE flag wasn't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("ENABLE flag wasn't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/* promisc */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_PROMISC))
					{
						ret = pfe_log_if_promisc_enable(log_if);
					}
					else
					{
						ret = pfe_log_if_promisc_disable(log_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("PROMISC flag wasn't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/* discard */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_DISCARD))
					{
						ret = pfe_log_if_discard_enable(log_if);
					}
					else
					{
						ret = pfe_log_if_discard_disable(log_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("DISCARD flag wasn't updated correctly on %s\n",  pfe_log_if_get_name(log_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					break;
				}
				case FPP_ACTION_QUERY:
				{
					ret = pfe_if_db_get_first(fci_context->log_if_db, fci_context->if_session_id, IF_DB_CRIT_ALL, NULL, &entry);
					if (NULL == entry)
					{
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						if(EOK != ret)
						{
							NXP_LOG_WARNING("Incorrect session ID detected\n");
							*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
						}
						ret = EOK;
						break;
					}
					fallthrough;
				}
				case FPP_ACTION_QUERY_CONT:
				{
					if (NULL == entry)
					{
						ret = pfe_if_db_get_next(fci_context->log_if_db, fci_context->if_session_id, &entry);
						if (NULL == entry)
						{
							*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
							if(EOK != ret)
							{
								NXP_LOG_WARNING("Incorrect session ID detected\n");
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
						(void)strncpy(reply_buf->name, pfe_log_if_get_name(log_if), (uint32_t)IFNAMSIZ-1U);
						(void)strncpy(reply_buf->parent_name, pfe_phy_if_get_name(phy_if), (uint32_t)IFNAMSIZ-1U);
					}
					else
					{
						NXP_LOG_WARNING("Was not possible to resolve DB entry to log_if or parent phy_if\n");
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}
					ret = pfe_log_if_get_stats(log_if,&stats);
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Could not get interface statistics\n");
						break;
					}

					/* Copy the log if statistics to reply */
					(void)memcpy(&reply_buf->stats, &stats, sizeof(reply_buf->stats));

					/* Get important flag values */
					(void)memset(&reply_buf->flags, 0, sizeof(reply_buf->flags));
					if(pfe_log_if_is_enabled(log_if))
					{
						reply_buf->flags |= oal_htonl(FPP_IF_ENABLED);
					}

					if(pfe_log_if_is_promisc(log_if))
					{
						reply_buf->flags |= oal_htonl(FPP_IF_PROMISC);
					}

					if(pfe_log_if_is_discard(log_if))
					{
						reply_buf->flags |= oal_htonl(FPP_IF_DISCARD);
					}

					if(pfe_log_if_is_match_or(log_if))
					{
						reply_buf->flags |= oal_htonl(FPP_IF_MATCH_OR);
					}

					/* Store egress interfaces */
					if(EOK != pfe_log_if_get_egress_ifs(log_if, &egress))
					{
						NXP_LOG_ERROR("Was not possible to get egress interfaces\n");
					}
					reply_buf->egress = oal_htonl(egress);

					(void)memset(&rules, 0, sizeof(pfe_ct_if_m_rules_t));
					(void)memset(&args, 0, sizeof(pfe_ct_if_m_args_t));
					/* Store rules for FCI */
					if(EOK != pfe_log_if_get_match_rules(log_if, &rules, &args))
					{
						NXP_LOG_ERROR("Was not possible to get match rules and arguments\n");
					}

					/* Fix endians of FP tables */
					args.fp0_table = oal_ntohl(args.fp0_table);
					args.fp1_table = oal_ntohl(args.fp1_table);
					reply_buf->match = (fpp_if_m_rules_t)(oal_htonl(rules));

					/* Store match rule arguments for FCI */
					reply_buf->arguments.vlan = args.vlan;
					reply_buf->arguments.ethtype = args.ethtype;
					reply_buf->arguments.sport = args.sport;
					reply_buf->arguments.dport = args.dport;
					reply_buf->arguments.proto = args.proto;
					reply_buf->arguments.hif_cookie = args.hif_cookie;

					/* Copy IPv4 or IPv6 addresses, based on data from logical interface instance. */
					if ((uint32_t)(FPP_IF_MATCH_SIP6 | FPP_IF_MATCH_DIP6) & (uint32_t)rules)
					{
						memcpy(&reply_buf->arguments.ipv.v6, &args.ipv.v6, sizeof(reply_buf->arguments.ipv.v6));
					}
					else
					{
						memcpy(&reply_buf->arguments.ipv.v4, &args.ipv.v4, sizeof(reply_buf->arguments.ipv.v4));
					}

					(void)memcpy(reply_buf->arguments.smac, args.smac, 6U);
					(void)memcpy(reply_buf->arguments.dmac, args.dmac, 6U);

					/* Translate names of flexible parser tables from addresses to strings. */
					(void)memset(reply_buf->arguments.fp_table0, 0, IFNAMSIZ);
					(void)memset(reply_buf->arguments.fp_table1, 0, IFNAMSIZ);
					if(EOK == fci_fp_db_get_table_from_addr(args.fp0_table, &table_name))
					{
						(void)strcpy(reply_buf->arguments.fp_table0, table_name);
					}
					if(EOK == fci_fp_db_get_table_from_addr(args.fp1_table, &table_name))
					{
						(void)strcpy(reply_buf->arguments.fp_table1, table_name);
					}

					/* Set ids */
					reply_buf->id = oal_htonl(pfe_log_if_get_id(log_if));
					reply_buf->parent_id = oal_htonl(pfe_phy_if_get_id(pfe_log_if_get_parent(log_if)));

					*reply_len = sizeof(fpp_log_if_cmd_t);
					*fci_ret = FPP_ERR_OK;
					break;
				}
				default:
				{
					/*Do Nothing*/
					break;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief			Process FPP_CMD_PHY_IF commands
 * @param[in]		msg FCI message containing the FPP_CMD_PHY_IF command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_phy_if_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with interface DB session lock.
 */
errno_t fci_interfaces_phy_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_phy_if_cmd_t *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
	fpp_phy_if_cmd_t *if_cmd;
	errno_t ret = EOK;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;
	pfe_ct_block_state_t block_state;
	pfe_ct_phy_if_stats_t stats = { 0 };
	pfe_mirror_t *mirror = NULL;
	uint32_t addr = 0U;
	uint32_t i;
	const char *str;
	char_t *name;
	bool flag_in_cmd;
	bool flag_in_drv;
	pfe_if_db_entry_t *mgmt_entry = NULL;
	pfe_phy_if_t *mgmt_if = NULL;
	pfe_ct_phy_if_id_t mgmt_if_id = PFE_PHY_IF_ID_INVALID;

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
		if (*reply_len < sizeof(fpp_phy_if_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_if_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_phy_if_cmd_t));

			if_cmd = (fpp_phy_if_cmd_t *)msg->msg_cmd.payload;

			/* Check if IF_DB lock is acquired */
			ret = pfe_if_db_lock_check(fci_context->if_session_id);
			if(EOK != ret)
			{
				/* FCI command requested unfulfillable action. Respond with FCI error code. */
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				return EOK;
			}

			switch (if_cmd->action)
			{
				case FPP_ACTION_UPDATE:
				{
					/* Get the requested interface */
					ret = pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->name, &entry);

					if(EOK != ret)
					{
						NXP_LOG_WARNING("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
						break;
					}

					/* Check if entry is not NULL and get physical interface */
					if(NULL != entry)
					{
						phy_if = pfe_if_db_entry_get_phy_if(entry);
					}

					/* Check if the entry exits*/
					if((NULL == entry) || (NULL == phy_if))
					{
						/* Interface doesn't exist or couldn't be extracted from the entry */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}

					/*	Set the interface block state - use the fact the enumerations
						have same values */
					ret = pfe_phy_if_set_block_state(phy_if, (pfe_ct_block_state_t)if_cmd->block_state);
					if(EOK != ret)
					{
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
						break;
					}

					/* Set the interface mode */
					ret = pfe_phy_if_set_op_mode(phy_if, (pfe_ct_if_op_mode_t)(if_cmd->mode));
					if(EOK != ret)
					{
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
						break;
					}

					for(i = 0U; i < (uint32_t)FPP_MIRRORS_CNT; i++)
					{
						/* RX */
						if('\0' == if_cmd->rx_mirrors[i][0])
						{	/* Mirror is disabled */
							if (EOK != pfe_phy_if_set_rx_mirror(phy_if, i, NULL))
							{
								NXP_LOG_ERROR("Configures the selected RX mirror failed\n");
								*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
								break;
							}
						}
						else
						{	/* Mirror is enabled and to be configured */
							/* Get requested mirror handle */
							mirror = pfe_mirror_get_first(MIRROR_BY_NAME, if_cmd->rx_mirrors[i]);
							if(NULL == mirror)
							{
								/* FCI command requested nonexistent entity. Respond with FCI error code. */
								NXP_LOG_WARNING("Mirror %s cannot be found\n", if_cmd->rx_mirrors[i]);
								*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
								ret = EOK;
								break;
							}
							/* Set the mirror */
							if (EOK != pfe_phy_if_set_rx_mirror(phy_if, i, mirror))
							{
								/*	Notify mirror module we are done working with the mirror instance. */
								pfe_mirror_put(mirror);
								mirror = NULL;
							
								NXP_LOG_ERROR("Configures the selected RX mirror failed\n");
								*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
								break;
							}

							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

						}
						/* TX */
						if('\0' == if_cmd->tx_mirrors[i][0])
						{	/* Mirror is disabled */
							if (EOK != pfe_phy_if_set_tx_mirror(phy_if, i, NULL))
							{
								NXP_LOG_ERROR("Configures the selected TX mirror failed\n");
								*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
								break;
							}
						}
						else
						{	/* Mirror is enabled and to be configured */
							/* Get requested mirror handle */
							mirror = pfe_mirror_get_first(MIRROR_BY_NAME, if_cmd->tx_mirrors[i]);
							if(NULL == mirror)
							{
								/* FCI command requested nonexistent entity. Respond with FCI error code. */
								NXP_LOG_WARNING("Mirror %s cannot be found\n", if_cmd->rx_mirrors[i]);
								*fci_ret = FPP_ERR_MIRROR_NOT_FOUND;
								ret = EOK;
								break;
							}
							/* Set the mirror */
							if (EOK != pfe_phy_if_set_tx_mirror(phy_if, i, mirror))
							{
								/*	Notify mirror module we are done working with the mirror instance. */
								pfe_mirror_put(mirror);
								mirror = NULL;

								NXP_LOG_ERROR("Configures the selected TX mirror failed\n");
								*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
								break;
							}

							/*	Notify mirror module we are done working with the mirror instance. */
							pfe_mirror_put(mirror);
							mirror = NULL;

						}
					}

					/*	Enable/Disable */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_ENABLED))
					{
						ret = pfe_phy_if_enable(phy_if);
					}
					else
					{
						ret = pfe_phy_if_disable(phy_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("ENABLE flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/* promisc */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_PROMISC))
					{
						ret = pfe_phy_if_promisc_enable(phy_if);
					}
					else
					{
						ret = pfe_phy_if_promisc_disable(phy_if);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("PROMISC flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/*	VLAN conformance check */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_VLAN_CONF_CHECK))
					{
						flag_in_cmd = true;
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_VLAN_CONF_CHECK);
					}
					else
					{
						flag_in_cmd = false;
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_VLAN_CONF_CHECK);
					}

					if(EOK != ret)
					{
						flag_in_drv = (IF_FL_NONE != pfe_phy_if_get_flag(phy_if, IF_FL_VLAN_CONF_CHECK));

						if (EPERM == ret)
						{
							if(flag_in_cmd == flag_in_drv)
							{
								/* Unavailable feature and FCI command didn't modify it. Continue through. */
								ret = EOK;
							}
							else
							{
								/* Unavailable feature and FCI command tried to modify it. Respond with FCI error code. */
								*fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
								ret = EOK;
								break;
							}
						}
						else
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("VLAN_CONF_CHECK flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}

					/*	PTP conformance check */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_PTP_CONF_CHECK))
					{
						flag_in_cmd = true;
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_PTP_CONF_CHECK);
					}
					else
					{
						flag_in_cmd = false;
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_PTP_CONF_CHECK);
					}

					if(EOK != ret)
					{
						flag_in_drv = (IF_FL_NONE != pfe_phy_if_get_flag(phy_if, IF_FL_PTP_CONF_CHECK));

						if (EPERM == ret)
						{
							if(flag_in_cmd == flag_in_drv)
							{
								/* Unavailable feature and FCI command didn't modify it. Continue through. */
								ret = EOK;
							}
							else
							{
								/* Unavailable feature and FCI command tried to modify it. Respond with FCI error code. */
								*fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
								ret = EOK;
								break;
							}
						}
						else
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("PTP_CONF_CHECK flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}

					/*	PTP promiscuous mode */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_PTP_PROMISC))
					{
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_PTP_PROMISC);
					}
					else
					{
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_PTP_PROMISC);
					}

					if(EOK != ret)
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						NXP_LOG_ERROR("PTP_PROMISC flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						break;
					}

					/* Fast-Forward of ingress TCP SYN, RST, FIN */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_FF_ALL_TCP))
					{
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_FF_ALL_TCP);
					}
					else
					{
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_FF_ALL_TCP);
					}

					if(EOK != ret)
					{
						/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
						NXP_LOG_ERROR("FF_ALL_TCP flag wasn't updated correctly on %s",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						break;
					}

					/*	QinQ support control */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_ALLOW_Q_IN_Q))
					{
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_ALLOW_Q_IN_Q);
					}
					else
					{
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_ALLOW_Q_IN_Q);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("ALLOW_Q_IN_Q flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/*	TTL discard control */
					if(0U != (oal_ntohl(if_cmd->flags) & (uint32_t)FPP_IF_DISCARD_TTL))
					{
						ret = pfe_phy_if_set_flag(phy_if, IF_FL_DISCARD_TTL);
					}
					else
					{
						ret = pfe_phy_if_clear_flag(phy_if, IF_FL_DISCARD_TTL);
					}

					if(EOK != ret)
					{
						NXP_LOG_ERROR("DISCARD_TTL flag wasn't updated correctly on %s\n",  pfe_phy_if_get_name(phy_if));
						*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
					}

					/*	Flexible Filter */
					if (0U != strlen((char_t *)if_cmd->ftable))
					{
						/*	Validate table */
						if (NULL == fci_fp_db_get_first(FP_TABLE_CRIT_NAME, (void *)if_cmd->ftable))
						{
							/*	Table not found */
							NXP_LOG_WARNING("%s: FP table %s not found\n",
									pfe_phy_if_get_name(phy_if), if_cmd->ftable);
						}
						else
						{
							/*	If not already done, write the table to HW */
							addr = fci_fp_db_get_table_dmem_addr((char_t *)if_cmd->ftable);
							if (0U == addr)
							{
								(void)fci_fp_db_push_table_to_hw(fci_context->class, (char_t *)if_cmd->ftable);
								addr = fci_fp_db_get_table_dmem_addr((char_t *)if_cmd->ftable);
							}

							/*	Assign the table to the physical interface */
							/*	TODO: Temporary way. Pass table instance or name but not the DMEM address :( */
							ret = pfe_phy_if_set_ftable(phy_if, addr);
							if (EOK != ret)
							{
								NXP_LOG_ERROR("%s: Could not set filter table: %d\n", pfe_phy_if_get_name(phy_if), ret);
								*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
							}
						}
					}
					else
					{
						/*	Disable the filter. Get table entry from DB first. */
						addr = pfe_phy_if_get_ftable(phy_if);
						if (EOK == fci_fp_db_get_table_from_addr(addr, &name))
						{
							/* Delete the table from DMEM - no longer in use, copy is in database */
							(void)fci_fp_db_pop_table_from_hw(name);
						}

						/*	Assign NULL-table to the physical interface */
						/*	TODO: Temporary way. Pass table instance or name but not the DMEM address :( */
						ret = pfe_phy_if_set_ftable(phy_if, 0U);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("%s: Could not set filter table: %d\n", pfe_phy_if_get_name(phy_if), ret);
							*fci_ret = FPP_ERR_IF_OP_UPDATE_FAILED;
						}
					}

					/* PTP mgmt interface */
					if ('\0' == if_cmd->ptp_mgmt_if[0])
					{
						/* Disable mgmt interface */
						ret = pfe_phy_if_set_mgmt_interface(phy_if, PFE_PHY_IF_ID_INVALID);
						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("%s: Could not disable mgmt interface\n", pfe_phy_if_get_name(phy_if));
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}
					else
					{
						ret = pfe_if_db_get_single(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_cmd->ptp_mgmt_if, &mgmt_entry);
						if (EOK != ret)
						{
							/* FCI command requested unfulfillable action. Respond with FCI error code. */
							NXP_LOG_WARNING("Incorrect session ID detected\n");
							*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
							ret = EOK;
							break;
						}

						/* Check if entry is not NULL and get physical interface */
						if (NULL != mgmt_entry)
						{
							mgmt_if = pfe_if_db_entry_get_phy_if(mgmt_entry);
						}
						/* Check if the entry exists */
						if ((NULL == mgmt_entry) || (NULL == mgmt_if))
						{
							/* FCI command requested nonexistent entity. Respond with FCI error code. */
							*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
							ret = EOK;
							break;
						}

						/* Enable mgmt interface and set its target physical interface */
						ret = pfe_phy_if_set_mgmt_interface(phy_if, pfe_phy_if_get_id(mgmt_if));
						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("%s: Could not set new mgmt interface %s\n", pfe_phy_if_get_name(phy_if), pfe_phy_if_get_name(mgmt_if));
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}

					break;
				}

				case FPP_ACTION_QUERY:
				{
					ret = pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_ALL, NULL, &entry);

					if(EOK != ret)
					{
						NXP_LOG_WARNING("Incorrect session ID detected\n");
						*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
						break;
					}

					if (NULL == entry)
					{
						ret = EOK;
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}
					fallthrough;
				}

				case FPP_ACTION_QUERY_CONT:
				{
					if (NULL == entry)
					{
						ret = pfe_if_db_get_next(fci_context->phy_if_db, fci_context->if_session_id, &entry);
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
						NXP_LOG_WARNING("Was not possible to resolve DB entry to phy_if\n");
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						break;
					}
					ret = pfe_phy_if_get_stats(phy_if, &stats);
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Could not get interface statistics\n");
						break;
					}
					/* Copy the phy if statistics to reply */
					(void)memcpy(&reply_buf->stats, &stats, sizeof(reply_buf->stats));

					/* Store phy_if name */
					(void)strncpy(reply_buf->name, pfe_phy_if_get_name(phy_if), (uint32_t)IFNAMSIZ-1U);

					/* Store phy_if id */
					reply_buf->id = oal_htonl(pfe_phy_if_get_id(phy_if));

					reply_buf->flags |= (TRUE == pfe_phy_if_is_promisc(phy_if)) ? oal_htonl(FPP_IF_PROMISC) : 0U;
					reply_buf->flags |= (TRUE == pfe_phy_if_is_enabled(phy_if)) ? oal_htonl(FPP_IF_ENABLED) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_VLAN_CONF_CHECK)) ? oal_htonl(FPP_IF_VLAN_CONF_CHECK) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_PTP_CONF_CHECK)) ? oal_htonl(FPP_IF_PTP_CONF_CHECK) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_PTP_PROMISC)) ? oal_htonl(FPP_IF_PTP_PROMISC) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_FF_ALL_TCP)) ? oal_htonl(FPP_IF_FF_ALL_TCP) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_ALLOW_Q_IN_Q)) ? oal_htonl(FPP_IF_ALLOW_Q_IN_Q) : 0U;
					reply_buf->flags |= ((uint32_t)IF_FL_NONE != (uint32_t)pfe_phy_if_get_flag(phy_if, IF_FL_DISCARD_TTL)) ? oal_htonl(FPP_IF_DISCARD_TTL) : 0U;

					/* Get the mode - use the fact enums have same values */
					reply_buf->mode = (fpp_phy_if_op_mode_t) pfe_phy_if_get_op_mode(phy_if);

					/* Get the block state */
					(void)pfe_phy_if_get_block_state(phy_if, &block_state);
					/* Use the fact that the enums have same values */
					reply_buf->block_state = (fpp_phy_if_block_state_t)block_state;

					for(i = 0U; i < (uint32_t)FPP_MIRRORS_CNT; i++)
					{
						/* RX */
						mirror = pfe_phy_if_get_rx_mirror(phy_if, i);
						if(NULL != mirror)
						{
							str = pfe_mirror_get_name(mirror);
							if(NULL != str)
							{
								(void)strncpy(&reply_buf->rx_mirrors[i][0], str, 16);
								reply_buf->rx_mirrors[i][15] = '\0'; /* Ensure correct string end */
							}
							else
							{
								NXP_LOG_WARNING("Could not obtain mirror name for %u\n", (uint_t)addr);
							}
						}
						/*	Notify mirror module we are done working with the mirror instance. This releases the reference obtained from pfe_phy_if_get_rx_mirror(). */
						pfe_mirror_put(mirror);
						mirror = NULL;

						/* TX */
						mirror = pfe_phy_if_get_tx_mirror(phy_if, i);
						if(NULL != mirror)
						{
							str = pfe_mirror_get_name(mirror);
							if(NULL != str)
							{
								(void)strncpy(&reply_buf->tx_mirrors[i][0], str, 16);
								reply_buf->tx_mirrors[i][15] = '\0'; /* Ensure correct string end */
							}
							else
							{
								NXP_LOG_WARNING("Could not obtain mirror name for %u\n", (uint_t)addr);
							}
						}
						/*	Notify mirror module we are done working with the mirror instance. This releases the reference obtained from pfe_phy_if_get_tx_mirror(). */
						pfe_mirror_put(mirror);
						mirror = NULL;
					}

					/*	Get filter info */
					addr = pfe_phy_if_get_ftable(phy_if);
					if (0U != addr)
					{
						ret = fci_fp_db_get_table_from_addr(addr, &name);
						if (EOK == ret)
						{
							(void)strncpy(reply_buf->ftable, name, sizeof(reply_buf->ftable) - 1U);
						}
						else
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Can't get table name from DB: %d\n", ret);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}
					}
					else
					{
						(void)memset(reply_buf->ftable, 0, sizeof(reply_buf->ftable));
					}

					/* Get PTP mgmt interface */
					mgmt_if_id = pfe_phy_if_get_mgmt_interface(phy_if);
					if (PFE_PHY_IF_ID_INVALID <= mgmt_if_id)
					{
						(void)memset(reply_buf->ptp_mgmt_if, 0, sizeof(reply_buf->ptp_mgmt_if));
					}
					else
					{
						ret = pfe_if_db_get_single(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_ID, (void*)mgmt_if_id, &mgmt_entry);
						if (EOK != ret)
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_WARNING("Incorrect session ID detected\n");
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						/* Check if entry is not NULL and get physical interface */
						if (NULL != mgmt_entry)
						{
							mgmt_if = pfe_if_db_entry_get_phy_if(mgmt_entry);
						}
						/* Check if the entry exists */
						if ((NULL == mgmt_entry) || (NULL == mgmt_if))
						{
							/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							NXP_LOG_ERROR("Unexpected NULL mgmt_if\n");
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
							break;
						}

						(void)strncpy(reply_buf->ptp_mgmt_if, pfe_phy_if_get_name(mgmt_if), (uint32_t)IFNAMSIZ-1U);
					}

					/* Set reply length end return OK */
					*reply_len = sizeof(fpp_phy_if_cmd_t);
					*fci_ret = FPP_ERR_OK;
					ret = EOK;
					break;
				}

				default:
				{
					NXP_LOG_WARNING("Interface Command: Unknown action received: 0x%x\n", if_cmd->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}
		}
	}
	return ret;
}

/**
 * @brief			Process FPP_CMD_IF_MAC commands
 * @param[in]		msg FCI message containing the FPP_CMD_IF_MAC command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_if_mac_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with interface DB session lock.
 */
errno_t fci_interfaces_mac_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_if_mac_cmd_t *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
	fpp_if_mac_cmd_t *if_mac_cmd;
	errno_t ret = EOK;
	pfe_if_db_entry_t *entry = NULL;
	pfe_phy_if_t *phy_if = NULL;


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

		if (*reply_len < sizeof(fpp_if_mac_cmd_t))
		{
			/* Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_if_mac_cmd_t)\n");
			*fci_ret = FPP_ERR_INTERNAL_FAILURE;
			ret = EINVAL;
		}
		else
		{
			/* No data written to reply buffer (yet) */
			*reply_len = 0U;
			/* Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_if_mac_cmd_t));

			/* Initialize pointer to the command data */
			if_mac_cmd = (fpp_if_mac_cmd_t *)msg->msg_cmd.payload;

			/* Check if IF_DB lock is acquired */
			ret = pfe_if_db_lock_check(fci_context->if_session_id);
			if(EOK != ret)
			{
				/* FCI command requested unfulfillable action. Respond with FCI error code. */
				*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
				return EOK;
			}

			/*	Preparation: get the requested interface */
			{
				ret = pfe_if_db_get_single(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, if_mac_cmd->name, &entry);

				if (EOK != ret)
				{
					/* DB not locked or locked by some other FCI user.*/
					/* FCI command requested unfulfillable action. Respond with FCI error code. */
					NXP_LOG_WARNING("Incorrect session ID detected\n");
					*fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
					ret = EOK;
				}
				else
				{

					/* Check if entry is not NULL and get physical interface */
					if (NULL != entry)
					{
						phy_if = pfe_if_db_entry_get_phy_if(entry);
					}

					/* Check if the entry exists*/
					if ((NULL == entry) || (NULL == phy_if))
					{
						/* Parent physical interface doesn't exist or cannot be extracted from the entry. */
						/* FCI command requested nonexistent entity. Respond with FCI error code. */
						*fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
						ret = EOK;
					}
				}
			}

			if ((uint16_t)FPP_ERR_OK == *fci_ret)
			{
				/* Process the command */
				switch (if_mac_cmd->action)
				{
					case FPP_ACTION_REGISTER:
					{
						ret = pfe_phy_if_add_mac_addr(phy_if, if_mac_cmd->mac, PFE_CFG_LOCAL_IF);
						if (EOK != ret)
						{
							if (EEXIST == ret)
							{
								/* FCI command attempted to register already registered entity. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_MAC_ALREADY_REGISTERED;
								ret = EOK;
							}
							if (EINVAL == ret)
							{
								/* FCI command requested unfulfillable action. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
								ret = EOK;
							}
						}

						/* No further actions. */
						break;
					}

					case FPP_ACTION_DEREGISTER:
					{
						ret = pfe_phy_if_del_mac_addr(phy_if, if_mac_cmd->mac, PFE_CFG_LOCAL_IF);
						if (EOK != ret)
						{
							if (ENOENT == ret)
							{
								/* FCI command requested nonexistent entity. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_MAC_NOT_FOUND;
								ret = EOK;
							}
							if (EINVAL == ret)
							{
								/* FCI command requested unfulfillable action. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
								ret = EOK;
							}
						}

						/*	No further actions. */
						break;
					}

					case FPP_ACTION_QUERY:
					{
						ret = pfe_phy_if_get_mac_addr_first(phy_if, reply_buf->mac, MAC_DB_CRIT_ALL, PFE_TYPE_ANY, PFE_CFG_LOCAL_IF);
						if (EOK != ret)
						{
							if (ENOENT == ret)
							{
								/* End of the query process (no more entities to report). Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_MAC_NOT_FOUND;
								ret = EOK;
							}
							if (EINVAL == ret)
							{
								/* FCI command requested unfulfillable action. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
								ret = EOK;
							}
						}

						if ((uint16_t)FPP_ERR_OK == *fci_ret)
						{
							/* Store phy_if name into reply message */
							(void)strncpy(reply_buf->name, pfe_phy_if_get_name(phy_if), (uint32_t)IFNAMSIZ - 1U);

							/* Set reply length and return OK */
							*reply_len = sizeof(fpp_if_mac_cmd_t);
							*fci_ret = FPP_ERR_OK;
							ret = EOK;
						}
						break;
					}

					case FPP_ACTION_QUERY_CONT:
					{
						ret = pfe_phy_if_get_mac_addr_next(phy_if, reply_buf->mac);
						if (EOK != ret)
						{
							if (ENOENT == ret)
							{
								/* End of the query process (no more entities to report). Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_MAC_NOT_FOUND;
								ret = EOK;
							}
							if (EINVAL == ret)
							{
								/* FCI command requested unfulfillable action. Respond with FCI error code. */
								*fci_ret = FPP_ERR_IF_NOT_SUPPORTED;
								ret = EOK;
							}
						}

						if ((uint16_t)FPP_ERR_OK == *fci_ret)
						{ /* Store phy_if name into reply message */
							(void)strncpy(reply_buf->name, pfe_phy_if_get_name(phy_if), (uint32_t)IFNAMSIZ - 1U);

							/* Set reply length and return OK */
							*reply_len = sizeof(fpp_if_mac_cmd_t);
							*fci_ret = FPP_ERR_OK;
							ret = EOK;
						}
						break;
					}

					default: 
					{
						/* Unknown action. Respond with FCI error code. */
						NXP_LOG_WARNING("FPP_CMD_IF_MAC: Unknown action received: 0x%x\n", if_mac_cmd->action);
						*fci_ret = FPP_ERR_UNKNOWN_ACTION;
						ret = EOK;
						break;
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
