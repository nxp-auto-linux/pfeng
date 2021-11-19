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
#include <string.h>
#include <arpa/inet.h>
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_fwfeat.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested FW feature
 *              from PFE. Identify the FW feature by its name.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_fwfeat Space for data from PFE.
 * @param[in]   p_name       Name of the requested FW feature.
 *                           Names of FW features are hardcoded.
 *                           Use FPP_ACTION_QUERY+FPP_ACTION_QUERY_CONT to get a list of
 *                           available FW features (and their names) from PFE.
 *                           See demo_fwfeat_print_all().
 * @return      FPP_ERR_OK : The requested FW feature was found.
 *                           A copy of its configuration data was stored into p_rtn_fwfeat.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_fwfeat_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_cmd_t* p_rtn_fwfeat, 
                            const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_fwfeat);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    fpp_fw_features_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                        sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (strcmp(p_name, reply_from_fci.name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FW_FEATURE,
                        sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_fwfeat = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_fwfeat_get_by_name() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief      Use FCI calls to enable/disable a target FW feature in PFE.
 * @param[in]  p_cl     FCI client
 * @param[in]  p_name   Name of a FW feature.
 *                      Names of FW features are hardcoded.
 *                      Use FPP_ACTION_QUERY+FPP_ACTION_QUERY_CONT to get a list of
 *                      available FW features (and their names) from PFE.
 *                      See demo_fwfeat_print_all().
 * @param[in]  enable   Request to set/unset the FW feature.
 * @return     FPP_ERR_OK : FW feature was successfully enabled/disabled in PFE.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fwfeat_set(FCI_CLIENT* p_cl, const char* p_name, bool enable)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
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
    
    print_if_error(rtn, "demo_fwfeat_set() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
/*
 * @defgroup    localdata_fwfeat  [localdata_fwfeat]
 * @brief:      Functions marked as [localdata_fwfeat] access only local data. 
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_fwfeat (a struct with configuration data).
 *              Initial data for p_fwfeat can be obtained via demo_fwfeat_get_by_name().
 */
 
 
/*
 * @brief      Query the current status of a FW feature.
 * @details    [localdata_fwfeat]
 * @param[in]  p_fwfeat  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the FW feature:
 *             true  : was enabled
 *             false : was disabled
 */
bool demo_fwfeat_ld_is_enabled(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    return (bool)(p_fwfeat->val);
}
 
 
/*
 * @brief      Query the default status of a FW feature.
 * @details    [localdata_fwfeat]
 * @param[in]  p_fwfeat  Local data to be queried.
 * @return     By default, the FW feature:
 *             true  : is initially enabled
 *             false : is initially disabled
 */
bool demo_fwfeat_ld_is_enabled_by_def(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    return (bool)(p_fwfeat->def_val);
}
 
 
 
 
/*
 * @brief      Query the name of a FW feature.
 * @details    [localdata_fwfeat]
 * @param[in]  p_fwfeat  Local data to be queried.
 * @return     Name of the FW feature.
 */
const char* demo_fwfeat_ld_get_name(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    return (p_fwfeat->name);
}
 
 
/*
 * @brief      Query the description text of a FW feature.
 * @details    [localdata_fwfeat]
 * @param[in]  p_fwfeat  Local data to be queried.
 * @return     Description text of the FW feature.
 */
const char* demo_fwfeat_ld_get_desc(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    return (p_fwfeat->desc);
}
 
 
/*
 * @brief      Query the variant of a FW feature.
 * @details    [localdata_fwfeat]
 * @param[in]  p_fwfeat  Local data to be queried.
 * @return     Flags (bitset) of a FW feature.
 */
fpp_fw_feature_flags_t demo_fwfeat_ld_get_flags(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    return (p_fwfeat->flags);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available FW features in PFE and
 *             execute a callback print function for each reported FW feature.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next FW feature is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available FW features.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fwfeat_print_all(FCI_CLIENT* p_cl, demo_fwfeat_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    if (FPP_ERR_FW_FEATURE_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_fwfeat_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available FW features in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of FW features.
 * @return      FPP_ERR_OK : Successfully counted all available FW features.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_fwfeat_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fw_features_cmd_t cmd_to_fci = {0};
    fpp_fw_features_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
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
    if (FPP_ERR_FW_FEATURE_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_fwfeat_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
