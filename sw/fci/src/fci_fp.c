/* =========================================================================
 *  Copyright 2019 NXP
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
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "fci_fp_db.h"
#include "pfe_fp.h"
#include "fci_msg.h"
#include "fci.h"
#include "fci_internal.h"
#include "pfe_class.h"
#include "pfe_flexible_filter.h"

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
static void fci_fp_construct_rule_reply(fpp_flexible_parser_rule_props *r, char *rule_name, char *next_rule,
                                   uint32_t data, uint32_t mask, uint16_t offset, pfe_ct_fp_flags_t flags)
{
    strncpy((char_t *)r->rule_name, rule_name, 15);
    r->data = data;
    r->mask = mask;
    r->offset = offset;
    if(NULL != next_rule)
    {
        strncpy((char_t *)r->next_rule_name, next_rule, 15);
    }
    if(flags & FP_FL_ACCEPT)
    {
        r->match_action = FP_ACCEPT;
    }
    else if(flags & FP_FL_REJECT)
    {
        r->match_action = FP_REJECT;
    }
    else
    {
        r->match_action = FP_NEXT_RULE;
    }
    if(flags & FP_FL_INVERT)
    {
        r->invert = TRUE;
    }
    else
    {
        r->invert = FALSE;
    }

    if(flags & FP_FL_L3_OFFSET)
    {
        r->offset_from = FP_OFFSET_FROM_L3_HEADER;
    }
    else if(flags & FP_FL_L4_OFFSET)
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
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_flexible_parser_table_cmd)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fp_table_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_flexible_parser_table_cmd *reply_buf, uint32_t *reply_len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
    fci_t *context = (fci_t *)&__context;
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
    fpp_flexible_parser_table_cmd *fp_cmd;
    errno_t ret = EOK;

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

	if (*reply_len < sizeof(fpp_flexible_parser_table_cmd))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_flexible_parser_table_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}
    fp_cmd = (fpp_flexible_parser_table_cmd *)(msg->msg_cmd.payload);
    switch (fp_cmd->action)
	{
		case FPP_ACTION_REGISTER:
        {
            ret = fci_fp_db_create_table((char_t *)fp_cmd->t.table_name);
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
            ret = fci_fp_db_destroy_table((char_t *)fp_cmd->t.table_name, FALSE);
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
            ret = fci_fp_db_add_rule_to_table((char_t *)fp_cmd->t.table_name, (char_t *)fp_cmd->t.rule_name, fp_cmd->t.position);
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
            fci_fp_db_remove_rule_from_table((char_t *)fp_cmd->t.rule_name);
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
            char *rule_name = NULL;
            char *next_rule = NULL;
            uint32_t data, mask;
            uint16_t offset;
            pfe_ct_fp_flags_t flags;

            ret = fci_fp_db_get_table_first_rule((char_t *)fp_cmd->t.table_name, &rule_name, &data, &mask, &offset, &flags, &next_rule);
            if(EOK == ret)
            {
            	fci_fp_construct_rule_reply(&reply_buf->r, rule_name, next_rule, data, mask, offset, flags);
                *fci_ret = FPP_ERR_OK;
                *reply_len = sizeof(fpp_flexible_parser_table_cmd);
            }
            else
            {
                *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
            }
            break;
        }
        case FPP_ACTION_QUERY_CONT:
        {
            char *rule_name;
            char *next_rule;
            uint32_t data, mask;
            uint16_t offset;
            pfe_ct_fp_flags_t flags;

            ret = fci_fp_db_get_table_next_rule((char_t *)fp_cmd->t.table_name, &rule_name, &data, &mask, &offset, &flags, &next_rule);
            if(EOK == ret)
            {
            	fci_fp_construct_rule_reply(&reply_buf->r, rule_name, next_rule, data, mask, offset, flags);
                *fci_ret = FPP_ERR_OK;
                *reply_len = sizeof(fpp_flexible_parser_table_cmd);
            }
            else
            {
                *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
            }
            break;
        }
		default:
		{
			NXP_LOG_ERROR("FPP_CMD_L2BRIDGE_DOMAIN: Unknown action received: 0x%x\n", fp_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
    }
    return ret;
}

/**
 * @brief			Processes FPP_CMD_FP_RULE commands
 * @param[in]		msg FCI message containing the FPP_CMD_FP_RULE command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_flexible_parser_rule_cmd)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_fp_rule_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_flexible_parser_rule_cmd *reply_buf, uint32_t *reply_len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
    fci_t *context = (fci_t *)&__context;
#endif
    fpp_flexible_parser_rule_cmd *fp_cmd;
    errno_t ret = EOK;

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

    if (*reply_len < sizeof(fpp_flexible_parser_rule_cmd))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_flexible_parser_rule_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

    fp_cmd = (fpp_flexible_parser_rule_cmd *)(msg->msg_cmd.payload);
    switch (fp_cmd->action)
	{
		case FPP_ACTION_REGISTER:
        {
            pfe_ct_fp_flags_t flags = 0U;
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
                    NXP_LOG_ERROR("Impossible happened\n");
                    *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    return EINVAL;
                    break;
            }
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
                    NXP_LOG_ERROR("Impossible happened\n");
                    *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
                    return EINVAL;
                    break;

            }
            if(TRUE == fp_cmd->r.invert)
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
            char *rule_name;
            char *next_rule;
            uint32_t data, mask;
            uint16_t offset;
            pfe_ct_fp_flags_t flags;

            ret = fci_fp_db_get_first_rule(&rule_name, &data, &mask, &offset, &flags, &next_rule);
            if(EOK == ret)
            {
                fci_fp_construct_rule_reply(&reply_buf->r, rule_name, next_rule, data, mask, offset, flags);
                *fci_ret = FPP_ERR_OK;
                *reply_len = sizeof(fpp_flexible_parser_rule_cmd);
            }
            else
            {
                *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
            }
            break;
        }
        case FPP_ACTION_QUERY_CONT:
        {
            char *rule_name;
            char *next_rule;
            uint32_t data, mask;
            uint16_t offset;
            pfe_ct_fp_flags_t flags;

            ret = fci_fp_db_get_next_rule(&rule_name, &data, &mask, &offset, &flags, &next_rule);
            if(EOK == ret)
            {
                fci_fp_construct_rule_reply(&reply_buf->r, rule_name, next_rule, data, mask, offset, flags);
                *fci_ret = FPP_ERR_OK;
                *reply_len = sizeof(fpp_flexible_parser_rule_cmd);
            }
            else
            {
                *fci_ret = FPP_ERR_FP_RULE_NOT_FOUND;
            }
            break;
        }

		default:
		{
			NXP_LOG_ERROR("FPP_CMD_L2BRIDGE_DOMAIN: Unknown action received: 0x%x\n", fp_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
    }
    return ret;
}


