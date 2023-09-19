/* =========================================================================
 *  Copyright 2020-2023 NXP
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
#include "pfe_feature_mgr.h"
#include "fci_fw_features.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief			Processes FPP_CMD_FW_FEATURES commands
 * @param[in]		msg FCI message containing the FPP_CMD_FP_FEATURES command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_fw_features_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fw_features_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fw_features_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_fw_features_cmd_t *fp_cmd;
	const char *str;
	const char *feature_name;
	errno_t ret;

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

		*fci_ret = FPP_ERR_OK;

		/* Important to initialize to avoid buffer overflows */
		if (*reply_len < sizeof(fpp_fw_features_cmd_t))
		{

			/*	Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_fw_features_cmd_t)\n");
			*fci_ret = FPP_ERR_INTERNAL_FAILURE;
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			(void)memset(reply_buf, 0, sizeof(fpp_fw_features_cmd_t));
			fp_cmd = (fpp_fw_features_cmd_t *)(msg->msg_cmd.payload);

			switch (fp_cmd->action)
			{
				case FPP_ACTION_UPDATE:
				{
					ret = pfe_feature_mgr_set_val(fp_cmd->name, fp_cmd->val);
					if(EOK != ret)
					{
						if (EFAULT == ret)
						{
							/*  FCI command try to change value of an ignore state feature. Respond with FCI error code. */
							*fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
						}
						else
						{
							/*	FCI command requested nonexistent entity. Respond with FCI error code. */
							*fci_ret = FPP_ERR_FW_FEATURE_NOT_FOUND;
						}
						ret = EOK;
					}

					break;
				}

				case FPP_ACTION_QUERY:
				{
					ret = pfe_feature_mgr_get_first(&feature_name);
					if(ret != EOK)
					{
						/*	End of the query process (no more entities to report). Respond with FCI error code. */
						*fci_ret = FPP_ERR_FW_FEATURE_NOT_FOUND;
						ret = EOK;
					}
					else
					{
						ret = pfe_feature_mgr_get_val(feature_name, &reply_buf->val);
						if(EOK == ret)
						{
							ret = pfe_feature_mgr_get_def_val(feature_name, &reply_buf->def_val);
						}
						if(EOK == ret)
						{
							ret = pfe_feature_mgr_get_variant(feature_name, &reply_buf->flags);
						}
						if(EOK == ret)
						{
							(void)strncpy(reply_buf->name, feature_name, FPP_FEATURE_NAME_SIZE);
							ret = pfe_feature_mgr_get_desc(feature_name, &str);
						}

						if(EOK == ret)
						{
							(void)strncpy(reply_buf->desc, str, FPP_FEATURE_DESC_SIZE);
							*reply_len = sizeof(fpp_fw_features_cmd_t);
							*fci_ret = FPP_ERR_OK;
						}
						else
						{
							/*	Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							*reply_len = sizeof(fpp_fw_features_cmd_t);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
					}
					break;

				}
				case FPP_ACTION_QUERY_CONT:
				{
					ret = pfe_feature_mgr_get_next(&feature_name);
					if(ret != EOK)
					{
						/*	End of the query process (no more entities to report). Respond with FCI error code. */
						*fci_ret = FPP_ERR_FW_FEATURE_NOT_FOUND;
						ret = EOK;
					}
					else
					{
						ret = pfe_feature_mgr_get_val(feature_name, &reply_buf->val);
						if(EOK == ret)
						{
							ret = pfe_feature_mgr_get_def_val(feature_name, &reply_buf->def_val);
						}
						if(EOK == ret)
						{
							ret = pfe_feature_mgr_get_variant(feature_name, &reply_buf->flags);
						}
						if(EOK == ret)
						{
							(void)strncpy(reply_buf->name, feature_name, FPP_FEATURE_NAME_SIZE);
							ret = pfe_feature_mgr_get_desc(feature_name, &str);
						}

						if(EOK == ret)
						{
							(void)strncpy(reply_buf->desc, str, FPP_FEATURE_DESC_SIZE);
							*reply_len = sizeof(fpp_fw_features_cmd_t);
							*fci_ret = FPP_ERR_OK;
						}
						else
						{
							/*	Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
							*reply_len = sizeof(fpp_fw_features_cmd_t);
							*fci_ret = FPP_ERR_INTERNAL_FAILURE;
						}
					}
					break;
				}

				default:
				{
					/*	Unknown action. Respond with FCI error code. */
					NXP_LOG_WARNING("FPP_CMD_FW_FEATURE: Unknown action received: 0x%x\n", fp_cmd->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					ret = EOK;
					break;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief			Processes FPP_CMD_FW_FEATURE_ELEMENT commands
 * @param[in]		msg FCI message containing the FPP_CMD_FW_FEATURE_ELEMENT command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_fw_features_element_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fw_features_element_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fw_features_element_cmd_t *reply_buf, uint32_t *reply_len)
{
	fpp_fw_features_element_cmd_t *fp_cmd;
	char *feature_name;
	const char *table_el_name;
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
		*fci_ret = FPP_ERR_OK;

		/* Important to initialize to avoid buffer overflows */
		if (*reply_len < sizeof(fpp_fw_features_element_cmd_t))
		{
			/*      Internal problem. Set fci_ret, but respond with detected internal error code (ret). */
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_fw_features_element_cmd_t)\n");
			*fci_ret = FPP_ERR_INTERNAL_FAILURE;
			ret = EINVAL;
		}
		else
		{
			/*      No data written to reply buffer (yet) */
			*reply_len = 0U;
			(void)memset(reply_buf, 0, sizeof(fpp_fw_features_element_cmd_t));
			fp_cmd = (fpp_fw_features_element_cmd_t *)(msg->msg_cmd.payload);


			if (fp_cmd->fw_feature_name[0] == '\0')
			{
				NXP_LOG_WARNING("Feature invalid name (fpp_fw_features_element_cmd_t)\n");
				*fci_ret = FPP_ERR_FW_FEATURE_NOT_FOUND;
				ret = EOK;
			}
			else
			{
				feature_name = fp_cmd->fw_feature_name;
				if(feature_name[0] == 'u' && feature_name[1] == '_')
				{
					feature_name += 2;
				}

				switch (fp_cmd->action)
				{
					case FPP_ACTION_UPDATE:
					{
						if (pfe_feature_mgr_is_available(feature_name))
						{
							ret = pfe_feature_mgr_table_set_val(fp_cmd->fw_feature_name, fp_cmd->group, fp_cmd->element_name, fp_cmd->index, (uint8_t *)fp_cmd->payload);
							if (EOK != ret)
							{
								*fci_ret = FPP_ERR_FW_FEATURE_ELEMENT_NOT_FOUND;
							}
						}
						else
						{
							 *fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
						}
						ret = EOK;
						break;
					}
					case FPP_ACTION_QUERY:
					{
						if (pfe_feature_mgr_is_available(feature_name))
						{
							if (fp_cmd->element_name[0] == '\0')
							{
								ret = pfe_feature_mgr_table_first(fp_cmd->fw_feature_name, fp_cmd->group, &table_el_name);
								if (ret != EOK)
								{
									/*      End of the query process (no more entities to report). Respond with FCI error code. */
									*fci_ret = FPP_ERR_FW_FEATURE_ELEMENT_NOT_FOUND;
									ret = EOK;
								}
								else
								{
									ret = pfe_feature_mgr_table_get_size(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, &reply_buf->unit_size);
									if (ret == EOK)
									{
										ret = pfe_feature_mgr_table_get_multiplicity(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, &reply_buf->count);
									}
									if (ret == EOK)
									{
										ret = pfe_feature_mgr_table_get_payload(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, (uint8_t *)reply_buf->payload);
									}
									if(EOK == ret)
									{
										(void)strncpy(reply_buf->element_name, table_el_name, FPP_FEATURE_NAME_SIZE);
										(void)strncpy(reply_buf->fw_feature_name, fp_cmd->fw_feature_name, FPP_FEATURE_NAME_SIZE);
										*reply_len = sizeof(fpp_fw_features_element_cmd_t);
										*fci_ret = FPP_ERR_OK;
									}
									else
									{
									/*      Internal problem. */
										*reply_len = sizeof(fpp_fw_features_element_cmd_t);
										*fci_ret = FPP_ERR_INTERNAL_FAILURE;
									}
								}
							}
							else
							{
								ret = pfe_feature_mgr_table_get_size(fp_cmd->fw_feature_name, fp_cmd->group, fp_cmd->element_name, &reply_buf->unit_size);
								if (ret == EOK)
								{
									ret = pfe_feature_mgr_table_get_multiplicity(fp_cmd->fw_feature_name, fp_cmd->group, fp_cmd->element_name, &reply_buf->count);
								}
								if (ret == EOK)
								{
									ret = pfe_feature_mgr_table_get_payload(fp_cmd->fw_feature_name, fp_cmd->group, fp_cmd->element_name, (uint8_t *)reply_buf->payload);
								}
								if(EOK == ret)
								{
									(void)strncpy(reply_buf->element_name, fp_cmd->element_name, FPP_FEATURE_NAME_SIZE);
									(void)strncpy(reply_buf->fw_feature_name, fp_cmd->fw_feature_name, FPP_FEATURE_NAME_SIZE);
									*reply_len = sizeof(fpp_fw_features_element_cmd_t);
									*fci_ret = FPP_ERR_OK;
								}
								else
								{
									*reply_len = sizeof(fpp_fw_features_element_cmd_t);
									*fci_ret = FPP_ERR_FW_FEATURE_ELEMENT_NOT_FOUND;
									ret = EOK;
								}
							}
						}
						else
						{
							*fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
							ret = EOK;
						}
						break;
					}
					case FPP_ACTION_QUERY_CONT:
					{
						if (pfe_feature_mgr_is_available(feature_name))
						{
							ret = pfe_feature_mgr_table_next(fp_cmd->fw_feature_name, fp_cmd->group, &table_el_name);
							if(ret != EOK)
							{
								/*      End of the query process (no more entities to report). Respond with FCI error code. */
								*fci_ret = FPP_ERR_FW_FEATURE_ELEMENT_NOT_FOUND;
								ret = EOK;
							}
							else
							{
								ret = pfe_feature_mgr_table_get_size(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, &reply_buf->unit_size);
								if (ret == EOK)
								{
									ret = pfe_feature_mgr_table_get_multiplicity(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, &reply_buf->count);
								}
								if (ret == EOK)
								{
									ret = pfe_feature_mgr_table_get_payload(fp_cmd->fw_feature_name, fp_cmd->group, table_el_name, (uint8_t *)reply_buf->payload);
								}
								if(EOK == ret)
								{
									(void)strncpy(reply_buf->element_name, table_el_name, FPP_FEATURE_NAME_SIZE);
									(void)strncpy(reply_buf->fw_feature_name, fp_cmd->fw_feature_name, FPP_FEATURE_NAME_SIZE);
									*reply_len = sizeof(fpp_fw_features_element_cmd_t);
									*fci_ret = FPP_ERR_OK;
								}
								else
								{
									/*      Internal problem. */
									*reply_len = sizeof(fpp_fw_features_element_cmd_t);
									*fci_ret = FPP_ERR_INTERNAL_FAILURE;
								}
							}
						}
						else
						{
							*fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
							ret = EOK;
						}
						break;
					}
					default:
					{
						/*      Unknown action. Respond with FCI error code. */
						NXP_LOG_WARNING("FPP_CMD_FW_FEATURE: Unknown action received: 0x%x\n", fp_cmd->action);
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
