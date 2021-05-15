/* =========================================================================
 *  Copyright 2020-2021 NXP
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
#include "pfe_pe.h"
#include "pfe_class.h"
#include "pfe_fw_feature.h"
#include "oal.h"
#include "fci_fw_features.h"

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
	pfe_fw_feature_t *fw_feature = NULL;
	fci_t *context = (fci_t *)&__context;
	fpp_fw_features_cmd_t *fp_cmd;
	const char *str;
	errno_t ret = EOK;

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
	/* Important to initialize to avoid buffer overflows */
	if (*reply_len < sizeof(fpp_fw_features_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_fw_features_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}
	memset(reply_buf, 0, sizeof(fpp_fw_features_cmd_t));
	fp_cmd = (fpp_fw_features_cmd_t *)(msg->msg_cmd.payload);

	switch (fp_cmd->action)
	{
		case FPP_ACTION_UPDATE:
		{
			ret = pfe_class_get_feature(context->class, &fw_feature, fp_cmd->name);

			if(EOK != ret)
			{
				*fci_ret = FPP_ERR_ENTRY_NOT_FOUND;
			}
			else
			{
				ret = pfe_fw_feature_set_val(fw_feature, fp_cmd->val);
				if(EOK != ret)
				{
					*fci_ret = FPP_ERR_ENTRY_NOT_FOUND;
				}
				*fci_ret = FPP_ERR_OK;
			}
			break;
		}

		case FPP_ACTION_QUERY:
		{
            ret = pfe_class_get_feature_first(context->class, &fw_feature);
            if(ret != EOK)
            {
                *fci_ret = FPP_ERR_ENTRY_NOT_FOUND;
            }
            else
            {
				pfe_fw_feature_get_val(fw_feature, &reply_buf->val);
				pfe_fw_feature_get_def_val(fw_feature, &reply_buf->def_val);
				pfe_fw_feature_get_variant(fw_feature, &reply_buf->variant);
                pfe_fw_feature_get_name(fw_feature, &str);
				strncpy(reply_buf->name, str, FPP_FEATURE_NAME_SIZE);
                pfe_fw_feature_get_desc(fw_feature, &str);
				strncpy(reply_buf->desc, str, FPP_FEATURE_DESC_SIZE);
				*reply_len = sizeof(fpp_fw_features_cmd_t);
				*fci_ret = FPP_ERR_OK;
				ret = EOK;
            }
            break;

		}
		case FPP_ACTION_QUERY_CONT:
		{
            ret = pfe_class_get_feature_next(context->class, &fw_feature);
            if(ret != EOK)
            {
                *fci_ret = FPP_ERR_ENTRY_NOT_FOUND;
            }
            else
            {
				pfe_fw_feature_get_val(fw_feature, &reply_buf->val);
				pfe_fw_feature_get_def_val(fw_feature, &reply_buf->def_val);
				pfe_fw_feature_get_variant(fw_feature, &reply_buf->variant);
                pfe_fw_feature_get_name(fw_feature, &str);
				strncpy(reply_buf->name, str, FPP_FEATURE_NAME_SIZE);
                pfe_fw_feature_get_desc(fw_feature, &str);
				strncpy(reply_buf->desc, str, FPP_FEATURE_DESC_SIZE);
				*reply_len = sizeof(fpp_fw_features_cmd_t);
				*fci_ret = FPP_ERR_OK;
				ret = EOK;
            }
			break;
		}

		default:
		{
			NXP_LOG_ERROR("FW Feature Command: Unknown action received: 0x%x\n", fp_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}
	return ret;
}
