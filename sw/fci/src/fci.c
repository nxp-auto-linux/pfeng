/* =========================================================================
 *  Copyright 2017-2025 NXP
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
#include "fci_fw_features.h"
#include "fci_mirror.h"
#include "fci_spd.h"

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_FCI_ENABLE

/*==================================================================================================
*                                     GLOBAL VARIABLES
==================================================================================================*/

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* Global variable used across all fci files */
fci_t __context = {0};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/

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
#if !defined(PFE_CFG_PFE_MASTER)
	/* Slave FCI proxy support */

	errno_t ret = EOK; /* Return value */
	pfe_platform_rpc_pfe_fci_proxy_arg_t proxy_cmd = {msg->type, msg->msg_cmd};
	pfe_platform_rpc_pfe_fci_proxy_ret_t proxy_rep = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == rep_msg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		NXP_LOG_DEBUG("Send FCI proxy message (type=0x%02x, code=0x%02x)\n", (uint_t)msg->type, (uint_t)msg->msg_cmd.code);
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_FCI_PROXY, &proxy_cmd, sizeof(proxy_cmd), &proxy_rep, sizeof(proxy_rep));
		rep_msg->msg_cmd = proxy_rep.msg_cmd;
	}

#else
	/* Normal FCI processing */

	errno_t ret = EOK; /* Return value */
	fci_t *fci_context = (fci_t *)&__context;
	uint16_t fci_ret = FPP_ERR_OK; /* FCI command return value */
	uint32_t *reply_buf_ptr = NULL;
	uint32_t *reply_buf_len_ptr = NULL;
	uint16_t *reply_retval_ptr = NULL;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	pfe_ct_phy_if_id_t sender_phy_if_id = PFE_PHY_IF_ID_INVALID;
	bool_t fci_cmd_execute = FALSE;
	bool_t fci_floating_lock = FALSE;
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fci_context) || (NULL == msg) || (NULL == rep_msg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Allocate space for return value ( + padding) by skipping first 4 bytes */
		reply_buf_ptr = ((uint32_t *)&rep_msg->msg_cmd.payload[4]);
		reply_buf_len_ptr = &rep_msg->msg_cmd.length;

		/*	Available reply buffer is 4 bytes less than maximum FCI message payload length */
		*reply_buf_len_ptr = FCI_CFG_MAX_CMD_PAYLOAD_LEN - 4U;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/*	HANDLE FCI OWNERSHIP */
		if (FCI_MSG_CMD == msg->type)
		{
			/*	Get mutex when handling FCI Ownership */
			ret = fci_owner_mutex_lock();
			if (EOK == ret)
			{
				/*	Authentication - get validated sender's HIF */
				ret = fci_sender_get_phy_if_id(msg->msg_cmd.sender, &sender_phy_if_id);
				if (EOK == ret)
				{
					if ((FPP_CMD_FCI_OWNERSHIP_LOCK == msg->msg_cmd.code) ||
						(FPP_CMD_FCI_OWNERSHIP_UNLOCK == msg->msg_cmd.code))
					{
						/*	Handle FCI lock/unlock commands */
						ret = fci_owner_session_cmd(sender_phy_if_id, msg->msg_cmd.code, &fci_ret);
					}
					else
					{
						/*	Authorization - check if sender is matching current FCI owner lock holder */
						ret = fci_owner_authorize(sender_phy_if_id, &fci_cmd_execute);
						if (EOK == ret)
						{
							if (FALSE == fci_cmd_execute)
							{
								/* 	Do not execute not authorized command and try to get floating FCI ownership */
								ret = fci_owner_get_floating_lock(sender_phy_if_id, &fci_ret, &fci_floating_lock);
								if (EOK == ret)
								{
									/* 	Execute FCI command as we have got the floating FCI owner lock */
									fci_cmd_execute = fci_floating_lock;
								}
							}
						}
						if ((EOK == ret) && (FALSE == fci_cmd_execute))
						{
							/* 	Clear reply buf len as FCI command is not going to be executed */
							*reply_buf_len_ptr = 0;
						}
					}
				}

				/*	fci_owner_mutex_unlock overrides ret, so set internal failure here */
				if (EOK != ret)
				{
					fci_ret = FPP_ERR_INTERNAL_FAILURE;
				}

				/*	Release FCI Ownership mutex if the floating FCI lock was not granted */
				if (FALSE == fci_floating_lock)
				{
					ret = fci_owner_mutex_unlock();
				}

			}
			if (EOK != ret)
			{
				fci_ret = FPP_ERR_INTERNAL_FAILURE;
			}

			/*	Exit fci_process_ipc_message if: */
			/*		1) Handled FCI lock/unlock command */
			/*		2) Current sender is not authorized to execute FCI command due to missing Ownership / lock */
			/*		3) An internal error occurred */
			if (FALSE == fci_cmd_execute)
			{
				/*	Inform client about command execution status */
				/*	We're adding another 4 bytes at the beginning of the FCI message payload area */
				rep_msg->msg_cmd.length = *reply_buf_len_ptr + 4U;

				reply_retval_ptr = (uint16_t *)rep_msg->msg_cmd.payload;
				*reply_retval_ptr = (uint16_t)fci_ret;

				return ret;
			}
		}
		NXP_LOG_DEBUG("Process FCI message (type=0x%02x, code=0x%02x, sender=0x%02x)\n", (uint_t)msg->type, (uint_t)msg->msg_cmd.code, (uint_t)sender_phy_if_id);
#else
		NXP_LOG_DEBUG("Process FCI message (type=0x%02x, code=0x%02x)\n", (uint_t)msg->type, (uint_t)msg->msg_cmd.code);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

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
							NXP_LOG_WARNING("Put buffer is too small\n");
							ret = EINVAL;
							fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
						else
						{
							buf.flags = 1;
							buf.len = fci_buf->len;
							(void)memcpy(&buf.payload, fci_buf->payload, fci_buf->len);

							ret = pfe_class_put_data(fci_context->class, &buf);
							if (EOK != ret)
							{
								NXP_LOG_WARNING("pfe_class_buf_put() failed: %d\n", ret);
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

					case FPP_CMD_IF_MAC:
					{
						/*	Process 'MAC address of interface' commands (add/del/query) */
						ret = fci_interfaces_mac_cmd(msg, &fci_ret, (fpp_if_mac_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_IP_ROUTE:
					{
						/*	Process 'route' commands (add/del/update/query/...) */
						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							ret = fci_routes_cmd(msg, &fci_ret, (fpp_rt_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_IPV4_SET_TIMEOUT:
					{
						/*	Update default timeouts for connections */

						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							ret = fci_connections_ipv4_timeout_cmd(msg, &fci_ret, (fpp_timeout_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_IPV4_CONNTRACK:
					{
						/*	Process 'ipv4 connection' commands (add/del/updated/query/...) */
						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							ret = fci_connections_ipv4_ct_cmd(msg, &fci_ret, (fpp_ct_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_IPV6_CONNTRACK:
					{
						/*	Process 'ipv6 connection' commands (add/del/updated/query/...) */
						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							ret = fci_connections_ipv6_ct_cmd(msg, &fci_ret, (fpp_ct6_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_IPV4_RESET:
					{
						/*	Remove all IPv4 routes, including connections */
						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							fci_routes_drop_all_ipv4();
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_IPV6_RESET:
					{
						/*	Remove all IPv6 routes, including connections */
						ret = oal_mutex_lock(&fci_context->db_mutex);
						if (EOK == ret)
						{
							fci_routes_drop_all_ipv6();
							(void)oal_mutex_unlock(&fci_context->db_mutex);
						}
						else
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						break;
					}

					case FPP_CMD_L2_BD:
					{
						/*	Manage L2 bridge domains */
						ret = fci_l2br_domain_cmd(msg, &fci_ret, (fpp_l2_bd_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_L2_STATIC_ENT:
					{
						/*	Manage L2 bridge domains */
						ret = fci_l2br_static_entry_cmd(msg, &fci_ret, (fpp_l2_static_ent_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
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

					case FPP_CMD_FW_FEATURE:
					{
						ret = fci_fw_features_cmd(msg, &fci_ret, (fpp_fw_features_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_FW_FEATURE_ELEMENT:
					{
						ret = fci_fw_features_element_cmd(msg, &fci_ret, (fpp_fw_features_element_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
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

					case FPP_CMD_MIRROR:
					{
						ret = fci_mirror_cmd(msg, &fci_ret, (fpp_mirror_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}
					case FPP_CMD_QOS_POLICER:
					{
						ret = fci_qos_policer_cmd(msg, &fci_ret, (fpp_qos_policer_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_QOS_POLICER_FLOW:
					{
						ret = fci_qos_policer_flow_cmd(msg, &fci_ret, (fpp_qos_policer_flow_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_QOS_POLICER_WRED:
					{
						ret = fci_qos_policer_wred_cmd(msg, &fci_ret, (fpp_qos_policer_wred_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_QOS_POLICER_SHP:
					{
						ret = fci_qos_policer_shp_cmd(msg, &fci_ret, (fpp_qos_policer_shp_cmd_t *)reply_buf_ptr, reply_buf_len_ptr);
						break;
					}

					case FPP_CMD_L2_FLUSH_ALL:
					case FPP_CMD_L2_FLUSH_LEARNED:
					case FPP_CMD_L2_FLUSH_STATIC:
					{
						ret = fci_l2br_flush_cmd(msg->msg_cmd.code, &fci_ret);
						break;
					}

					case FPP_CMD_FCI_OWNERSHIP_LOCK:
					case FPP_CMD_FCI_OWNERSHIP_UNLOCK:
					{
						NXP_LOG_WARNING("Received FCI ownership command: 0x%x. It is not supported in standalone mode.\n", (uint_t)msg->msg_cmd.code);
						fci_ret = FPP_ERR_FCI_OWNERSHIP_NOT_ENABLED;
						break;
					}

					default:
					{
						NXP_LOG_WARNING("Unknown CMD code received: 0x%x\n", (uint_t)msg->msg_cmd.code);
						ret = EINVAL;
						fci_ret = FPP_ERR_UNKNOWN_COMMAND;
						break;
					}
				}

				/*
				 * WARNING
				 * Return value of fci_process_ipc_message() is checked by the libFCI static library.
				 * The library assumes that the return value of fci_process_ipc_message() represents internal result of the FCI message processing.
				 * For this reason, do NOT modify 'ret' of fci_process_ipc_message() after it was set by some FCI message processing function in the 'big switch'.
				 * (the modification in the 'Unknown message type' default case is the only exception)
				 */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
				if (TRUE == fci_floating_lock)
				{
					/* Release floating FCI ownership lock */
					fci_owner_clear_floating_lock();
					fci_owner_mutex_unlock();
				}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

				/*	Inform client about command execution status */
				/*	We're adding another 4 bytes at the beginning of the FCI message payload area */
				rep_msg->msg_cmd.length = *reply_buf_len_ptr + 4U;

				reply_retval_ptr = (uint16_t *)rep_msg->msg_cmd.payload;
				*reply_retval_ptr = (uint16_t)fci_ret;

				break;
			}

			default:
			{
				NXP_LOG_WARNING("Unknown message type\n");
				ret = EINVAL;
				break;
			}
		}
	}
#endif /* PFE_CFG_PFE_MASTER */
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
	fci_t *fci_context = (fci_t *)&__context;
	errno_t err = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == identifier))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		err = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (TRUE == fci_context->fci_initialized)
		{
			NXP_LOG_ERROR("FCI has already been initialized!\n");
			err = EINVAL;
		}
		else
		{
			(void)memset(fci_context, 0, sizeof(fci_t));

			fci_context->db_mutex_initialized = FALSE;
			fci_context->log_if_db_initialized = FALSE;
			fci_context->phy_if_db_initialized = FALSE;
			fci_context->rt_db_initialized = FALSE;
			fci_context->rtable_initialized = FALSE;
			fci_context->tmu_initialized = FALSE;
			fci_context->hm_cb_registered = FALSE;
			fci_context->is_some_client = FALSE;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
			fci_context->fci_owner_initialized = FALSE;
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

			/*	Sanity check */
			if (6U != sizeof(pfe_mac_addr_t))
			{
				err = EINVAL;
                fci_fini();
			}
            else
            {
			    /*	Create communication core */
			    err = fci_core_init(identifier);
			    if (EOK != err)
			    {
			    	NXP_LOG_ERROR("Could not create FCI core\n");
                    fci_fini();
			    }
                else
                {
#ifdef PFE_CFG_PFE_MASTER
			        err = oal_mutex_init(&fci_context->db_mutex);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
				if (EOK == err)
				{
					/*	Initialize FCI ownership */
					err = fci_owner_init(info);
					if (EOK == err)
					{
						fci_context->fci_owner_initialized = TRUE;
					}
				}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

			        if (EOK != err)
			        {
					NXP_LOG_ERROR("Mutex initialization failed\n");
                        fci_fini();
			        }
			        else
			        {
			        	fci_context->db_mutex_initialized = TRUE;

			            /*	Initialize the Flexible Parser databases */
			            fci_fp_db_init();
			            if (NULL != info)
			            {
			            	fci_context->class = info->class;
			            }

			            /*	Initialize the physical interface database */
			            if (NULL != info)
			            {
			            	fci_context->phy_if_db = info->phy_if_db;
			            }

			            if (NULL != fci_context->log_if_db)
			            {
			            	fci_context->phy_if_db_initialized = TRUE;
			            }

			            /*	Initialize the logical interface database */
			            if (NULL != info)
			            {
			            	fci_context->log_if_db = info->log_if_db;
			            }

			            if (NULL != fci_context->log_if_db)
			            {
			            	fci_context->log_if_db_initialized = TRUE;
			            }

			            /*	Initialize the route database */
			            fci_rt_db_init(&fci_context->route_db);
			            fci_context->rt_db_initialized = TRUE;

			            /*	Store the routing table and bridge reference */
			            if (NULL != info)
			            {
			            	if (NULL != info->rtable)
			            	{
			            		fci_context->rtable = info->rtable;
			            		fci_context->rtable_initialized = TRUE;
			            	}

			            	if (NULL != info->l2_bridge)
			            	{
			            		fci_context->l2_bridge = info->l2_bridge;
			            		fci_context->l2_bridge_initialized = TRUE;
			            	}
			            }

			            if (NULL != info)
			            {
			            	/*	Initialize the TMU  */
			            	fci_context->tmu = info->tmu;
			            }

			            if (NULL != fci_context->tmu)
			            {
			            	fci_context->tmu_initialized = TRUE;
			            }
#else
		    	        (void)info;
#endif /* PFE_CFG_PFE_MASTER */

						if (EOK == fci_hm_cb_register())
						{
							fci_context->hm_cb_registered = TRUE;
						}

		    	        fci_context->default_timeouts.timeout_tcp = 5U * 24U * 60U * 60U; 	/* 5 days */
		    	        fci_context->default_timeouts.timeout_udp = 300U; 					/* 5 min */
		    	        fci_context->default_timeouts.timeout_other = 240U; 				/* 4 min */
		    	        fci_context->fci_initialized = TRUE;
#ifdef PFE_CFG_PFE_MASTER
                    }
#endif
                }
            }
		}
	}
	return err;
}

/**
 * @brief		Destroy FCI context
 */
void fci_fini(void)
{
	fci_t *fci_context = (fci_t *)&__context;
#ifdef PFE_CFG_PFE_MASTER
	uint32_t session_id = 0U;
#endif /* PFE_CFG_PFE_MASTER */

	if (TRUE == fci_context->fci_initialized)
	{

#ifdef PFE_CFG_PFE_MASTER
		/*	Drop all content of RT DB (needs operational endpoint; may send FCI events) */
		if (TRUE == fci_context->rt_db_initialized)
		{
			if (TRUE == fci_context->db_mutex_initialized)
			{
				if (EOK != oal_mutex_lock(&fci_context->db_mutex))
				{
					NXP_LOG_ERROR("Mutex lock failed\n");
				}
				fci_routes_drop_all();
				if (EOK != oal_mutex_unlock(&fci_context->db_mutex))
				{
					NXP_LOG_ERROR("Mutex unlock failed\n");
				}
			}
		}
#endif /* PFE_CFG_PFE_MASTER */

		/* Deregister HM callback function */
		if (TRUE == fci_context->hm_cb_registered)
		{
			fci_hm_cb_deregister();
			fci_context->hm_cb_registered = FALSE;
			fci_context->is_some_client = FALSE;
		}

		/*	Shut down the endpoint */
		if (NULL != fci_context->core)
		{
			fci_core_fini();
			fci_context->core = NULL;
		}

	#ifdef PFE_CFG_PFE_MASTER
		if (EOK != pfe_if_db_polled_lock(&session_id))
		{
			NXP_LOG_ERROR("DB lock failed\n");
		}
		/*	Shutdown the logical IF DB */
		if (TRUE == fci_context->log_if_db_initialized)
		{
			/* Freeing of the DB is handled by platfrom driver*/
			fci_context->log_if_db = NULL;
			fci_context->log_if_db_initialized = FALSE;
		}

		/*  Shutdown the physical IF DB */
		if (TRUE == fci_context->phy_if_db_initialized)
		{
			/* Freeing of the DB is handled by platfrom driver*/
			fci_context->phy_if_db = NULL;
			fci_context->phy_if_db_initialized = FALSE;
		}
		if (EOK != pfe_if_db_unlock(session_id))
		{
			NXP_LOG_ERROR("DB unlock failed\n");
		}

		/*	Shutdown the RT DB (paranoia clean) */
		if (TRUE == fci_context->rt_db_initialized)
		{
			if (TRUE == fci_context->db_mutex_initialized)
			{
				if (EOK != oal_mutex_lock(&fci_context->db_mutex))
				{
					NXP_LOG_ERROR("Mutex lock failed\n");
				}
				fci_routes_drop_all();
				if (EOK != oal_mutex_unlock(&fci_context->db_mutex))
				{
					NXP_LOG_ERROR("Mutex unlock failed\n");
				}
			}

			fci_context->rt_db_initialized = FALSE;
		}

		/*	Invalidate the routing table */
		fci_context->rtable = NULL;
		fci_context->rtable_initialized = FALSE;

		if (TRUE == fci_context->db_mutex_initialized)
		{
			(void)oal_mutex_destroy(&fci_context->db_mutex);
		}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		if (TRUE == fci_context->fci_owner_initialized)
		{
			fci_owner_fini();
			fci_context->fci_owner_initialized = FALSE;
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
	#endif /* PFE_CFG_PFE_MASTER */

		(void)memset(fci_context, 0, sizeof(fci_t));
		fci_context->fci_initialized = FALSE;
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
