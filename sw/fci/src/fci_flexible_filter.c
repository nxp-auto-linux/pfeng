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
#include "fci_msg.h"
#include "fci.h"
#include "fci_internal.h"
#include "pfe_class.h"
#include "pfe_flexible_filter.h"

/**
 * @brief			Processes FPP_FP_CMD_FLEXIBLE_FILTER commands
 * @param[in]		msg FCI message containing the FPP_FP_CMD_FLEXIBLE_FILTER command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_flexible_filter_cmd)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with domain DB protected against concurrent accesses.
 */
errno_t fci_flexible_filter_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_flexible_filter_cmd *reply_buf, uint32_t *reply_len)
{
    fci_t *context = (fci_t *)&__context;

    fpp_flexible_filter_cmd *fp_cmd;
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
    fp_cmd = (fpp_flexible_filter_cmd *)(msg->msg_cmd.payload);
    switch (fp_cmd->action)
	{
		case FPP_ACTION_REGISTER:
        {
            uint32_t addr;
            /* Write the finished table into the DMEM */
            ret = fci_fp_db_push_table_to_hw(context->class, (char_t *)fp_cmd->table_name);
            if(EOK == ret)
            {
                /* Get the DMEM address */
                addr = fci_fp_db_get_table_dmem_addr((char_t *)fp_cmd->table_name);
                if(0 != addr)
                {
                    /* Let the classifier use the address of the table as flexible filter */
                    ret = pfe_flexible_filter_set(context->class, addr);
                }
                else
                {
                    ret = ENOMEM;
                }
            }
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
            /* Write zero (NULL) into the classifier to prevent table being used */
            ret = pfe_flexible_filter_set(context->class, 0U);
            if(EOK == ret)
            {
                /* Delete the table from DMEM - no longer in use, copy is in database */
                fci_fp_db_pop_table_from_hw((char_t *)fp_cmd->table_name);
                *fci_ret = FPP_ERR_OK;
            }
            else
            {
                *fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
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

