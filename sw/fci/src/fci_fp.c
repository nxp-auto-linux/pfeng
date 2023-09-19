/* =========================================================================
 *  Copyright 2019-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "fci_fp_db.h"
#include "pfe_fp.h"
#include "fci_msg.h"
#include "fci.h"
#include "fci_internal.h"
#include "pfe_class.h"
#include "fci_fp.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static void fci_fp_construct_rule_reply(fpp_fp_rule_props_t *r, char *rule_name, char *next_rule,
                                   uint32_t data, uint32_t mask, uint16_t offset, pfe_ct_fp_flags_t flags);

/**
* @brief Constructs a query reply with specified rule parameters in the specified buffer
* @param[in] r Buffer where to construct the query reply
* @param[in] rule_name Name of the rule which is being replied to the query
* @param[in] next_rule Value of the next_rule parameter of the replied rule
* @param[in] data Value of the data parameter of the replied rule
* @param[in] mask Value of the mask parameter of the replied rule
* @param[in] offset Value of the offset parameter of the replied rule
* @param[in] flags Flags being applied to the replied rule
*/
static void fci_fp_construct_rule_reply(fpp_fp_rule_props_t *r, char *rule_name, char *next_rule,
                                   uint32_t data, uint32_t mask, uint16_t offset, pfe_ct_fp_flags_t flags)
{
    (void)strncpy((char_t *)r->rule_name, rule_name, 15);
    r->data = data;
    r->mask = mask;
    r->offset = offset;
    if(NULL != next_rule)
    {
        (void)strncpy((char_t *)r->next_rule_name, next_rule, 15);
    }
    if(((uint8_t)flags & (uint8_t)FP_FL_ACCEPT) != 0U)
    {
        /*  Ensure correct endianess */
        r->match_action = FP_ACCEPT;
    }
    else if(((uint8_t)flags & (uint8_t)FP_FL_REJECT) != 0U)
    {
        r->match_action = FP_REJECT;
    }
    else
    {
        r->match_action = FP_NEXT_RULE;
    }
    if(((uint8_t)flags & (uint8_t)FP_FL_INVERT) != 0U)
    {
        r->invert = TRUE;
    }
    else
    {
        r->invert = FALSE;
    }

    if(((uint8_t)flags & (uint8_t)FP_FL_L3_OFFSET) != 0U)
    {
        r->offset_from = FP_OFFSET_FROM_L3_HEADER;
    }
    else if(((uint8_t)flags & (uint8_t)FP_FL_L4_OFFSET) != 0U)
    {
        r->offset_from = FP_OFFSET_FROM_L4_HEADER;
    }
    else
    {
        r->offset_from = FP_OFFSET_FROM_L2_HEADER;
    }
}

/**
 * @brief			Processes FPP_CMD_FP_TABLE commands
 * @param[in]		msg FCI message containing the FPP_CMD_FP_TABLE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_fp_table_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fp_table_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_table_cmd_t *reply_buf, uint32_t *reply_len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
    const fci_t *fci_context = (fci_t *)&__context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
    fpp_fp_table_cmd_t *fp_cmd;
    errno_t ret = EOK;
    fci_fp_rule_info_t rule;
    char *next_rule;

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
        /* Important to initialize to avoid buffer overflows */
        if (*reply_len < sizeof(fpp_fp_table_cmd_t))
        {
            NXP_LOG_WARNING("Buffer length does not match expected value (fpp_fp_table_cmd_t)\n");
            ret = EINVAL;
        }
        else
        {
            /*	No data written to reply buffer (yet) */
            *reply_len = 0U;
            fp_cmd = (fpp_fp_table_cmd_t *)(msg->msg_cmd.payload);
            switch (fp_cmd->action)
            {
                case FPP_ACTION_REGISTER:
                {
                    ret = fci_fp_db_create_table((char_t *)fp_cmd->table_info.t.table_name);
                    if(EOK == ret)
                    {
                        *fci_ret = FPP_ERR_OK;
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    }
                    break;
                }
                case FPP_ACTION_DEREGISTER:
                {
                    ret = fci_fp_db_destroy_table((char_t *)fp_cmd->table_info.t.table_name, FALSE);
                    if(EOK == ret)
                    {
                        *fci_ret = FPP_ERR_OK;
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    }
                    break;
                }
                case FPP_ACTION_USE_RULE:
                {
                    ret = fci_fp_db_add_rule_to_table((char_t *)fp_cmd->table_info.t.table_name, (char_t *)fp_cmd->table_info.t.rule_name, oal_ntohs(fp_cmd->table_info.t.position));
                    if(EOK == ret)
                    {
                        *fci_ret = FPP_ERR_OK;
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    }
                    break;
                }
                case FPP_ACTION_UNUSE_RULE:
                {
                    ret = fci_fp_db_remove_rule_from_table((char_t *)fp_cmd->table_info.t.rule_name);
                    if(EOK == ret)
                    {
                        *fci_ret = FPP_ERR_OK;
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    }
                    break;
                }
                case FPP_ACTION_QUERY:
                {
                    rule.rule_name = NULL;
                    next_rule = NULL;

                    ret = fci_fp_db_get_table_first_rule((char_t *)fp_cmd->table_info.t.table_name, &rule, &next_rule);
                    if(EOK == ret)
                    {
                        fci_fp_construct_rule_reply(&reply_buf->table_info.r, rule.rule_name, next_rule, rule.data, rule.mask, rule.offset, rule.flags);
                        *fci_ret = FPP_ERR_OK;
                        *reply_len = sizeof(fpp_fp_table_cmd_t);
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
                    }
                    break;
                }
                case FPP_ACTION_QUERY_CONT:
                {
                    ret = fci_fp_db_get_table_next_rule((char_t *)fp_cmd->table_info.t.table_name, &rule, &next_rule);
                    if(EOK == ret)
                    {
                        fci_fp_construct_rule_reply(&reply_buf->table_info.r, rule.rule_name, next_rule, rule.data, rule.mask, rule.offset, rule.flags);
                        *fci_ret = FPP_ERR_OK;
                        *reply_len = sizeof(fpp_fp_table_cmd_t);
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
                    }
                    break;
                }
                default:
                {
                    NXP_LOG_WARNING("FPP_CMD_L2_BD: Unknown action received: 0x%x\n", fp_cmd->action);
                    *fci_ret = FPP_ERR_UNKNOWN_ACTION;
                    break;
                }
            }
        }
    }

    return ret;
}

/**
 * @brief			Processes FPP_CMD_FP_RULE commands
 * @param[in]		msg FCI message containing the FPP_CMD_FP_RULE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_fp_rule_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fp_rule_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_rule_cmd_t *reply_buf, uint32_t *reply_len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
    const fci_t *fci_context = (fci_t *)&__context;
#endif
    fpp_fp_rule_cmd_t *fp_cmd;
    errno_t ret = EOK;

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
        if (*reply_len < sizeof(fpp_fp_rule_cmd_t))
        {
            NXP_LOG_WARNING("Buffer length does not match expected value (fpp_fp_rule_cmd_t)\n");
            ret = EINVAL;
        }
        else
        {
            /*	No data written to reply buffer (yet) */
            *reply_len = 0U;

            fp_cmd = (fpp_fp_rule_cmd_t *)(msg->msg_cmd.payload);
            switch (fp_cmd->action)
            {
                case FPP_ACTION_REGISTER:
                {
                    pfe_ct_fp_flags_t flags = FP_FL_NONE;
                    switch(fp_cmd->r.match_action)
                    {
                        case FP_ACCEPT:
                            flags |= FP_FL_ACCEPT;
                            break;
                        case FP_REJECT:
                            flags |= FP_FL_REJECT;
                            break;
                        case FP_NEXT_RULE:
                            break;
                        default:
                            NXP_LOG_WARNING("Impossible happened\n");
                            *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                            ret = EINVAL;
                    }
                    if(EOK == ret)
                    {
                        switch(fp_cmd->r.offset_from)
                        {
                            case FP_OFFSET_FROM_L2_HEADER:
                                break;
                            case FP_OFFSET_FROM_L3_HEADER:
                                flags |= FP_FL_L3_OFFSET;
                                break;
                            case FP_OFFSET_FROM_L4_HEADER:
                                flags |= FP_FL_L4_OFFSET;
                                break;
                            default:
                                NXP_LOG_WARNING("Impossible happened\n");
                                *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                                ret = EINVAL;
                        }
                        if(EOK == ret)
                        {
                            if((uint8_t)TRUE == fp_cmd->r.invert)
                            {
                                flags |= FP_FL_INVERT;
                            }

                            ret = fci_fp_db_create_rule((char_t *)fp_cmd->r.rule_name, fp_cmd->r.data, fp_cmd->r.mask,
                                                    fp_cmd->r.offset, flags,
                                                    (char_t *)fp_cmd->r.next_rule_name);
                            if(EOK == ret)
                            {
                                *fci_ret = FPP_ERR_OK;
                            }
                            else
                            {
                                *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                            }
                        }
                    }
                    break;
                }
                case FPP_ACTION_DEREGISTER:
                {
                    ret = fci_fp_db_destroy_rule((char_t*)fp_cmd->r.rule_name);
                    if(EOK == ret)
                    {
                        *fci_ret = FPP_ERR_OK;
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    }
                    break;
                }
                case FPP_ACTION_QUERY:
                {
                    fci_fp_rule_info_t rule;
                    char *next_rule;

                    ret = fci_fp_db_get_first_rule(&rule, &next_rule);
                    if(EOK == ret)
                    {
                        fci_fp_construct_rule_reply(&reply_buf->r, rule.rule_name, next_rule, rule.data, rule.mask, rule.offset, rule.flags);
                        *fci_ret = FPP_ERR_OK;
                        *reply_len = sizeof(fpp_fp_rule_cmd_t);
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
                    }
                    break;
                }
                case FPP_ACTION_QUERY_CONT:
                {
                    fci_fp_rule_info_t rule;
                    char *next_rule;

                    ret = fci_fp_db_get_next_rule(&rule, &next_rule);
                    if(EOK == ret)
                    {
                        fci_fp_construct_rule_reply(&reply_buf->r, rule.rule_name, next_rule, rule.data, rule.mask, rule.offset, rule.flags);
                        *fci_ret = FPP_ERR_OK;
                        *reply_len = sizeof(fpp_fp_rule_cmd_t);
                    }
                    else
                    {
                        *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
                    }
                    break;
                }

                default:
                {
                    NXP_LOG_WARNING("FPP_CMD_L2_BD: Unknown action received: 0x%x\n", fp_cmd->action);
                    *fci_ret = FPP_ERR_UNKNOWN_ACTION;
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

