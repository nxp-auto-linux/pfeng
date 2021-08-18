/* =========================================================================
 *  Copyright 2020-2021 NXP
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
 
 
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
#include "fci_common.h"
#include "fci_fwfeat.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
/* empty */
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested FW feature
 *              from the PFE. Identify the FW feature by its name.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_fwfeat Space for data from the PFE.
 * @param[in]   p_name       Name of the requested FW feature.
 * @return      FPP_ERR_OK : Requested FW feature was found.
 *                           A copy of its configuration was stored into p_rtn_fwfeat.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_fwfeat_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_cmd_t* p_rtn_fwfeat, 
                           const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_fwfeat);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    fpp_fw_features_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                        sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (strcmp(p_name, reply_from_fci.name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                        sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_fwfeat = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief      Use FCI calls to enable/disable a target FW feature in the PFE.
 * @param[in]  p_cl     FCI client instance
 * @param[in]  p_name   Name of the requested FW feature.
 * @param[in]  enable   A request to set/unset the FW feature.
 * @return     FPP_ERR_OK : FW feature was successfully enabled/disabled in the PFE.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fwfeat_set(FCI_CLIENT* p_cl, const char* p_name, bool enable)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, (FPP_FEATURE_NAME_SIZE + 1));
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.val = enable;  /* NOTE: Implicit cast from bool to uintX_t */
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_UPDATE;
        rtn = fci_write(p_cl, FPP_CMD_FW_FEATURE, sizeof(fpp_fw_features_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all FW features in the PFE and
 *             execute a callback print function for each reported FW feature.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next FW feature is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through FW features.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fwfeat_print_all(FCI_CLIENT* p_cl, fci_fwfeat_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    fpp_fw_features_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                    sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
        
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                            sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more FW features to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all FW features in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of FW features.
 * @return      FPP_ERR_OK : Successfully counted FW features.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_fwfeat_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    fpp_fw_features_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                    sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                        sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more FW features to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
