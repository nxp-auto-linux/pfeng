/* =========================================================================
 *  Copyright 2020-2024 NXP
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
#include <errno.h>
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_fp.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested FP rule
 *              from PFE. Identify the rule by its name.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_rule   Space for data from PFE.
 * @param[out]  p_rtn_idx    Space for index of the requested FP rule.
 *                           This is a generic index of the given rule in a common pool of
 *                           FP rules within PFE. It has no ties to any particular FP table.
 *                           Can be NULL. If NULL, then no index is stored.
 * @param[in]   p_rule_name  Name of the requested FP rule.
 *                           Names of FP rules are user-defined.
 *                           See demo_fp_rule_add().
 * @return      FPP_ERR_OK : The requested FP rule was found.
 *                           A copy of its configuration data was stored into p_rtn_rule.
 *                           Its common pool index was stored into p_rtn_idx.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_fp_rule_get_by_name(FCI_CLIENT* p_cl, fpp_fp_rule_cmd_t* p_rtn_rule, 
                             uint16_t* p_rtn_idx, const char* p_rule_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_rule);
    assert(NULL != p_rule_name);
    /* 'p_rtn_index' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    fpp_fp_rule_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t idx = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                    sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && 
           (0 != strcmp((char*)(reply_from_fci.r.rule_name), p_rule_name)))
    {
        idx++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                        sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_rule = reply_from_fci;
        if (NULL != p_rtn_idx)
        {
            *p_rtn_idx = idx;
        }
    }
    
    print_if_error(rtn, "demo_fp_rule_get_by_name() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new FP rule in PFE.
 * @param[in]   p_cl         FCI client
 * @param[in]   p_rule_name  Name of the new FP rule.
 *                           The name is user-defined.
 * @param[in]   p_rule_data  Configuration data of the new FP rule.
 *                           To create a new FP rule, a local data struct must be created,
 *                           configured and then passed to this function.
 *                           See [localdata_fprule] to learn more.
 * @return      FPP_ERR_OK : New FP rule was created.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_rule_add(FCI_CLIENT* p_cl, const char* p_rule_name,
                     const fpp_fp_rule_cmd_t* p_rule_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rule_name);
    assert(NULL != p_rule_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_rule_data;
    rtn = set_text((char*)(cmd_to_fci.r.rule_name), p_rule_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t), 
                                              (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_rule_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target FP rule in PFE.
 * @param[in]  p_cl         FCI client
 * @param[in]  p_rule_name  Name of the FP rule to destroy.
 * @return     FPP_ERR_OK : The FP rule was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_rule_del(FCI_CLIENT* p_cl, const char* p_rule_name)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.r.rule_name), p_rule_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t), 
                                              (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_rule_del() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new FP table in PFE.
 * @param[in]   p_cl          FCI client
 * @param[in]   p_table_name  Name of the new FP table.
 *                            The name is user-defined.
 * @return      FPP_ERR_OK : New FP table was created.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_table_add(FCI_CLIENT* p_cl, const char* p_table_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_table_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target FP table in PFE.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_table_name  Name of the FP table to destroy.
 * @return     FPP_ERR_OK : The FP table was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_table_del(FCI_CLIENT* p_cl, const char* p_table_name)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_table_del() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to insert a FP rule at a given position of a FP table in PFE.
 * @param[in]   p_cl          FCI client
 * @param[in]   p_table_name  Name of an existing FP table.
 * @param[in]   p_rule_name   Name of an existing FP rule.
 * @param[in]   position      Index where to insert the rule. Starts at 0.
 * @return      FPP_ERR_OK : The rule was successfully inserted into the table.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_table_insert_rule(FCI_CLIENT* p_cl, const char* p_table_name, 
                              const char* p_rule_name, uint16_t position)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    assert(NULL != p_rule_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((char*)(cmd_to_fci.table_info.t.rule_name), p_rule_name, IFNAMSIZ);
    }
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.table_info.t.position = htons(position);
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_USE_RULE;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_table_insert_rule() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to remove a FP rule from a FP table in PFE.
 * @param[in]   p_cl          FCI client
 * @param[in]   p_table_name  Name of an existing FP table.
 * @param[in]   p_rule_name   Name of a FP rule which is present in the FP table.
 * @return      FPP_ERR_OK : The rule was successfully removed from the table.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_table_remove_rule(FCI_CLIENT* p_cl, const char* p_table_name, 
                              const char* p_rule_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    assert(NULL != p_rule_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((char*)(cmd_to_fci.table_info.t.rule_name), p_rule_name, IFNAMSIZ);
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_UNUSE_RULE;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_fp_table_remove_rule() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_fprule  [localdata_fprule]
 * @brief:      Functions marked as [localdata_fprule] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_rule (a struct with configuration data).
 *              For addition of FP rules, there are no "initial data" to be obtained from PFE.
 *              Simply declare a local data struct and configure it.
 *              Then, after all modifications are done and finished,
 *              call demo_fp_rule_add() to create a new FP rule in PFE.
 */
 
 
/*
 * @brief          Set a data "template" of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 * @param[in]      data    Data "template" (a value)
 *                         This value will be compared with a selected value from 
 *                         the inspected traffic.
 */
void demo_fp_rule_ld_set_data(fpp_fp_rule_cmd_t* p_rule, uint32_t data)
{
    assert(NULL != p_rule);
    p_rule->r.data = htonl(data);
}
 
 
/*
 * @brief          Set a bitmask of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 * @param[in]      mask    Bitmask for more precise data selection.
 *                         This bitmask is applied on the selected 32bit value from
 *                         the inspected traffic.
 */
void demo_fp_rule_ld_set_mask(fpp_fp_rule_cmd_t* p_rule, uint32_t mask)
{
    assert(NULL != p_rule);
    p_rule->r.mask = htonl(mask);
}
 
 
/*
 * @brief          Set an offset and a base for the offset ("offset from") of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 * @param[in]      offset  Offset (in bytes) into traffic's data.
 *                         The offset is applied from the respective base ("offset_from").
 *                         Data value (32bit) which lies on the offset is the value selected
 *                         for comparison under the given FP rule.
 * @param[in]      offset_from  Base for an offset calculation.
 *                              See description of the fpp_fp_offset_from_t type 
 *                              in FCI API Reference.
 */
void demo_fp_rule_ld_set_offset(fpp_fp_rule_cmd_t* p_rule, uint16_t offset, 
                                fpp_fp_offset_from_t offset_from)
{
    assert(NULL != p_rule);
    
    p_rule->r.offset = htons(offset);
    
    hton_enum(&offset_from, sizeof(fpp_fp_offset_from_t));
    p_rule->r.offset_from = offset_from;
}
 
 
/*
 * @brief          Set/unset an inverted mode of a FP rule match evaluation.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 * @param[in]      invert  Request to set/unset the inverted mode of evaluation.
 */
void demo_fp_rule_ld_set_invert(fpp_fp_rule_cmd_t* p_rule, bool invert)
{
    assert(NULL != p_rule);
    p_rule->r.invert = invert;  /* NOTE: Implicit cast from bool to uint8_t */
}
 
 
/*
 * @brief          Set action to be done if inspected traffic satisfies a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 * @param[in]      match_action      Action to be done.
 *                                   See description of the fpp_fp_rule_match_action_t type
 *                                   in FCI API Reference.
 * @param[in]      p_next_rule_name  Name of a next FP rule to execute.
 *                                   Meaningful only if the match action is FP_NEXT_RULE.
 *                                   Can be NULL. If NULL or "" (empty string), 
 *                                   then no rule is set as the next rule.
 */
void demo_fp_rule_ld_set_match_action(fpp_fp_rule_cmd_t* p_rule, 
                                      fpp_fp_rule_match_action_t match_action,
                                      const char* p_next_rule_name)
{
    assert(NULL != p_rule);
    /* 'p_next_rule_name' is allowed to be NULL */
    
    hton_enum(&match_action, sizeof(fpp_fp_rule_match_action_t));
    p_rule->r.match_action = match_action;
    
    set_text((char*)(p_rule->r.next_rule_name), p_next_rule_name, IFNAMSIZ);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query the status of an invert mode of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the FP rule:
 *             true  : was running in the inverted mode
 *             false : was NOT running in the inverted mode
 */
bool demo_fp_rule_ld_is_invert(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return (bool)(p_rule->r.invert);
}
 
 
/*
 * @brief      Query the name of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Name of the FP rule.
 */
const char* demo_fp_rule_ld_get_name(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return (const char*)(p_rule->r.rule_name);
}
 
 
/*
 * @brief      Query the name of a "next FP rule".
 * @details    [localdata_fprule]
 *             "Next FP rule" is meaningful only when "match_action == FP_NEXT_RULE"
 * @param[in]  p_rule  Local data to be queried.
 * @return     Name of the "next FP rule".
 */
const char* demo_fp_rule_ld_get_next_name(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return (const char*)(p_rule->r.next_rule_name);
}
 
 
/*
 * @brief      Query the data "template" of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Data "template" used by the FP rule.
 */
uint32_t demo_fp_rule_ld_get_data(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return ntohl(p_rule->r.data);
}
 
 
/*
 * @brief      Query the bitmask of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Bitmask used by the FP rule.
 */
uint32_t demo_fp_rule_ld_get_mask(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return ntohl(p_rule->r.mask);
}
 
 
/*
 * @brief      Query the offset of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Offset where to find the inspected value in the traffic data.
 */
uint16_t demo_fp_rule_ld_get_offset(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    return ntohs(p_rule->r.offset);
}
 
 
/*
 * @brief      Query the offset base ("offset from") of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Base position in traffic data to use for offset calculation.
 */
fpp_fp_offset_from_t demo_fp_rule_ld_get_offset_from(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    
    fpp_fp_offset_from_t tmp_offset_from = (p_rule->r.offset_from);
    ntoh_enum(&tmp_offset_from, sizeof(fpp_fp_offset_from_t));
    
    return (tmp_offset_from);
}
 
 
/*
 * @brief      Query the match action of a FP rule.
 * @details    [localdata_fprule]
 * @param[in]  p_rule  Local data to be queried.
 * @return     Match action of the FP rule.
 */
fpp_fp_rule_match_action_t demo_fp_rule_ld_get_match_action(const fpp_fp_rule_cmd_t* p_rule)
{
    assert(NULL != p_rule);
    
    fpp_fp_rule_match_action_t tmp_match_action = (p_rule->r.match_action);
    ntoh_enum(&tmp_match_action, sizeof(fpp_fp_rule_match_action_t));
    
    return (tmp_match_action);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available FP rules of a given FP table
 *             in PFE. Execute a callback print function for each applicable FP rule.
 * @param[in]  p_cl           FCI client
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns ZERO, then all is OK and 
 *                                a next FP rule in table is picked for a print process.
 *                            --> If the callback returns NON-ZERO, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_table_name   Name of a FP table.
 *                            Names of FP tables are user-defined. See demo_fp_table_add().
 * @param[in]  position_init  Start invoking a callback print function from 
 *                            this position in the FP table.
 *                            If 0, start from the very first FP rule in the table.
 * @param[in]  count          Print only this count of FP rules, then end.
 *                            If 0, keep printing FP rules till the end of the table.
 * @return     FPP_ERR_OK : Successfully iterated through all FP rules of the given FP table.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_table_print(FCI_CLIENT* p_cl, demo_fp_rule_cb_print_t p_cb_print, 
                        const char* p_table_name, uint16_t position_init, uint16_t count)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_table_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    fpp_fp_table_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    if (0u == count)  /* if 0, set max possible count of items */
    {
        count--;  /* WARNING: intentional use of owf behavior */
    }
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_FP_TABLE,
                        sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    
        /* query loop */
        uint16_t position = 0u;
        while ((FPP_ERR_OK == rtn) && (0u != count))
        {
            if (position >= position_init)
            {
                const fpp_fp_rule_cmd_t tmp_rule = {0u, (reply_from_fci.table_info.r)};
                rtn = p_cb_print(&tmp_rule, position);
                count--;
            }
            
            position++;
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_FP_TABLE,
                                sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
            }
        }
        
        /* query loop runs till there are no more FP rules to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (-ENOENT == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_fp_table_print() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available FP rules in PFE (regardless
 *             of table affiliation). Execute a print function for each applicable FP rule.
 * @param[in]  p_cl           FCI client
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns ZERO, then all is OK and 
 *                                a next FP rule is picked for a print process.
 *                            --> If the callback returns NON-ZERO, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  idx_init       Start invoking a callback print function from 
 *                            this index of FP rule query.
 *                            If 0, start from the very first queried FP rule.
 * @param[in]  count          Print only this count of FP rules, then end.
 *                            If 0, keep printing FP rules till there is no more available.
 * @return     FPP_ERR_OK : Successfully iterated through all available FP rules.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_fp_rule_print_all(FCI_CLIENT* p_cl, demo_fp_rule_cb_print_t p_cb_print, 
                           uint16_t idx_init, uint16_t count)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    fpp_fp_rule_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    if (0u == count)  /* if 0, set max possible count of items */
    {
        count--;  /* WARNING: intentional use of owf behavior */
    }
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                    sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));

    /* query loop */
    uint16_t idx = 0u;
    while ((FPP_ERR_OK == rtn) && (0u != count))
    {
        if (idx >= idx_init)
        {
            rtn = p_cb_print(&reply_from_fci, idx);
            count--;
        }
        
        idx++;
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                            sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more FP rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (-ENOENT == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_fp_rule_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available FP rules in PFE (regardless
 *              of table affiliation).
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of FP rules.
 * @return      FPP_ERR_OK : Successfully counted all available FP rules.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_fp_rule_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    fpp_fp_rule_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                    sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                        sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more FP rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (-ENOENT == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_fp_rule_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
