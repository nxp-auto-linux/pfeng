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
#include "demo_mirror.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief       Set/unset a modification action flag in a mirroring rule struct.
 * @param[out]  p_rtn_mirror  Struct to be modified.
 * @param[in]   enable        New state of a modification action flag.
 * @param[in]   action        The 'modify action' flag.
 */
static void set_mirror_ma_flag(fpp_mirror_cmd_t* p_rtn_mirror, bool enable,
                               fpp_modify_actions_t action)
{
    assert(NULL != p_rtn_mirror);
    
    hton_enum(&action, sizeof(fpp_modify_actions_t));
    
    if (enable)
    {
        p_rtn_mirror->m_actions |= action;
    }
    else
    {
        p_rtn_mirror->m_actions &= (fpp_modify_actions_t)(~action);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested mirroring rule
 *              from PFE. Identify the rule by its name.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_mirror Space for data from PFE.
 * @param[in]   p_name       Name of the requested mirroring rule.
 *                           Names of mirroring rules are user-defined.
 *                           See demo_mirror_add().
 * @return      FPP_ERR_OK : The requested mirrroring rule was found.
 *                           A copy of its configuration data was stored into p_rtn_mirror.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_mirror_get_by_name(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_rtn_mirror, 
                            const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_mirror);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_mirror_cmd_t cmd_to_fci = {0};
    fpp_mirror_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                        sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (0 != strcmp((reply_from_fci.name), p_name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                        sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_mirror = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_mirror_get_by_name() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target mirroring rule
 *                 in PFE.
 * @param[in]      p_cl     FCI client
 * @param[in,out]  p_mirror Local data struct which represents a new configuration of
 *                          the target mirroring rule.
 *                          It is assumed that the struct contains a valid data of some 
 *                          mirroring rule.
 * @return        FPP_ERR_OK : Configuration of the target mirroring rule was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct was not updated.
 */
int demo_mirror_update(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_cl);
    assert(NULL != p_mirror);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_mirror_cmd_t cmd_to_fci = (*p_mirror);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t), 
                                         (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_get_by_name(p_cl, p_mirror, (p_mirror->name));
    }
    
    print_if_error(rtn, "demo_mirror_update() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new mirroring rule in PFE.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_logif    Space for data from PFE.
 *                             This will contain a copy of configuration data of 
 *                             the newly created mirroring rule.
 *                             Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   p_name         Name of the new mirroring rule.
 *                             The name is user-defined.
 * @param[in]   p_phyif_name   Name of an egress physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : New mirroring rule was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_mirror.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_mirror_add(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_rtn_mirror, const char* p_name,
                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    assert(NULL != p_phyif_name);
    /* 'p_rtn_mirror' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_mirror_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, MIRROR_NAME_SIZE);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((cmd_to_fci.egress_phy_if), p_phyif_name, IFNAMSIZ);
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_mirror))
    {
        rtn = demo_mirror_get_by_name(p_cl, p_rtn_mirror, p_name);
    }
    
    print_if_error(rtn, "demo_mirror_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target mirroring rule in PFE.
 * @param[in]  p_cl    FCI client
 * @param[in]  p_name  Name of the mirroring rule to destroy.
 * @return     FPP_ERR_OK : The mirroring rule was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_mirror_del(FCI_CLIENT* p_cl, const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_mirror_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, MIRROR_NAME_SIZE);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_mirror_del() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_mirror  [localdata_mirror]
 * @brief:      Functions marked as [localdata_mirror] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_mirror (a struct with configuration data).
 *              Initial data for p_mirror can be obtained via demo_mirror_get_by_name().
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call demo_mirror_update() to update 
 *              the configuration of a real mirroring rule in PFE.
 */
 
 
/*
 * @brief          Set an egress physical interface of a mirroring rule.
 * @details        [localdata_mirror]
 * @param[in,out]  p_mirror       Local data to be modified.
 * @param[in]      p_mirror_name  Name of a physical interface which shall be used as egress.
 *                                Names of physical interfaces are hardcoded.
 *                                See the FCI API Reference, chapter Interface Management.
 */
void demo_mirror_ld_set_egress_phyif(fpp_mirror_cmd_t* p_mirror, const char* p_phyif_name)
{
    assert(NULL != p_mirror);
    assert(NULL != p_phyif_name);
    set_text((p_mirror->egress_phy_if), p_phyif_name, IFNAMSIZ);
}
 
 
/*
 * @brief          Set FlexibleParser table to act as a filter for a mirroring rule.
 * @details        [localdata_mirror]
 * @param[in,out]  p_mirror      Local data to be modified.
 * @param[in]      p_table_name  Name of a FlexibleParser table.
 *                               Can be NULL. If NULL or "" (empty string), then
 *                               filter of this mirroring rule is disabled.
 */
void demo_mirror_ld_set_filter(fpp_mirror_cmd_t* p_mirror, const char* p_table_name)
{
    assert(NULL != p_mirror);
    /* 'p_table_name' is allowed to be NULL */
    
    set_text(p_mirror->filter_table_name, p_table_name, IFNAMSIZ);
}
 
 
 
 
/*
 * @brief          Clear all modification actions of a mirroring rule.
 *                 (also zeroify all modification action arguments of the mirroring rule)
 * @details        [localdata_mirror]
 * @param[in,out]  p_mirror  Local data to be modified.
 */
void demo_mirror_ld_clear_all_ma(fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    p_mirror->m_actions = 0u;
    memset(&(p_mirror->m_args), 0, sizeof(fpp_modify_args_t));
}
 
 
/*
 * @brief          Set/unset the given modification action (ADD_VLAN_HDR) and its argument.
 * @details        [localdata_mirror]
 * @param[in,out]  p_mirror  Local data to be modified.
 * @param[in]      set       Request to set/unset the given match rule.
 * @param[in]      vlan      New VLAN ID for this match rule.
 *                           When this match rule is active, it compares value of its
 *                           'vlan' argument with the value of traffic's 'VID' field.
 */
void demo_mirror_ld_set_ma_vlan(fpp_mirror_cmd_t* p_mirror, bool set, uint16_t vlan)
{
    assert(NULL != p_mirror);
    
    set_mirror_ma_flag(p_mirror, set, MODIFY_ACT_ADD_VLAN_HDR);
    p_mirror->m_args.vlan = htons(vlan);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query whether a modification action is active or not.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @param[in]  action    Queried 'modify action'.
 * @return     At time when the data was obtained from PFE, the mirroring rule:
 *             true  : had at least one queried 'modify action' bitflags set
 *             false : had none of the queried 'modify action' bitflags set
 */
bool demo_mirror_ld_is_ma(const fpp_mirror_cmd_t* p_mirror, 
                          fpp_modify_actions_t action)
{
    assert(NULL != p_mirror);
    
    fpp_modify_actions_t tmp_actions = (p_mirror->m_actions);
    ntoh_enum(&tmp_actions, sizeof(fpp_modify_actions_t));
    
    return (bool)(tmp_actions & action);
}
 
 
 
 
/*
 * @brief      Query the name of a mirroring rule.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @return     Name of the mirroring rule.
 */
const char* demo_mirror_ld_get_name(const fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    return (p_mirror->name);
}
 
 
/*
 * @brief      Query the egress interface of a mirroring rule.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @return     Name of a physical interface which is used as an egress interface 
 *             of the mirroring rule.
 */
const char* demo_mirror_ld_get_egress_phyif(const fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    return (p_mirror->egress_phy_if);
}
 
 
/*
 * @brief      Query the name of a FlexibleParser table which is being used as
 *             a filter for a mirroring rule.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @return     Name of the FlexibleParser table which is used as a filter
 *             of the mirroring rule.
 */
const char* demo_mirror_ld_get_filter(const fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    return (p_mirror->filter_table_name);
}
 
 
 
 
/*
 * @brief      Query the modification action bitset of a mirroring rule.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @return     'Modify action' bitset.
 */
fpp_modify_actions_t demo_mirror_ld_get_ma_bitset(const fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    
    fpp_modify_actions_t tmp_actions = (p_mirror->m_actions);
    ntoh_enum(&tmp_actions, sizeof(fpp_modify_actions_t));
    
    return (tmp_actions);
}
 
 
/*
 * @brief      Query the argument of the modification action match rule ADD_VLAN_HDR.
 * @details    [localdata_mirror]
 * @param[in]  p_mirror  Local data to be queried.
 * @return     Argument (VLAN ID) of the given modification action.
 */
uint16_t demo_mirror_ld_get_ma_vlan(const fpp_mirror_cmd_t* p_mirror)
{
    assert(NULL != p_mirror);
    return ntohs(p_mirror->m_args.vlan);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available mirroring rules in PFE and
 *             execute a callback print function for each mirroring rule.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next mirroring rule is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available mirroring rules.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_mirror_print_all(FCI_CLIENT* p_cl, demo_mirror_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_mirror_cmd_t cmd_to_fci = {0};
    fpp_mirror_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                    sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                            sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more mirroring rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_MIRROR_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_mirror_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available mirroring rules in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of mirroring rules.
 * @return      FPP_ERR_OK : Successfully counted all available mirroring rules.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No value copied.
 */
int demo_mirror_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_mirror_cmd_t cmd_to_fci = {0};
    fpp_mirror_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                    sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_MIRROR,
                        sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more mirroring rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_MIRROR_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_mirror_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
