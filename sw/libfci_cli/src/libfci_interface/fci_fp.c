/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
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
#include "fci_fp.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a FP rule properties struct.
 * @param[in,out]  p_rtn_rule_props  The FP rule properties struct to be converted.
 */
static void ntoh_rule_props(fpp_fp_rule_props_t* p_rtn_rule_props)
{
    assert(NULL != p_rtn_rule_props);
    
    
    p_rtn_rule_props->data = ntohl(p_rtn_rule_props->data);
    p_rtn_rule_props->mask = ntohl(p_rtn_rule_props->mask);
    p_rtn_rule_props->offset = ntohs(p_rtn_rule_props->offset);
    ntoh_enum(&(p_rtn_rule_props->match_action), sizeof(fpp_fp_rule_match_action_t));
    ntoh_enum(&(p_rtn_rule_props->offset_from), sizeof(fpp_fp_offset_from_t));
}
 
 
/*
 * @brief          Host-to-network (hton) function for a FP rule properties struct.
 * @param[in,out]  p_rtn_rule_props  The FP rule properties struct to be converted.
 */
static void hton_rule_props(fpp_fp_rule_props_t* p_rtn_rule_props)
{
    assert(NULL != p_rtn_rule_props);
    
    
    p_rtn_rule_props->data = htonl(p_rtn_rule_props->data);
    p_rtn_rule_props->mask = htonl(p_rtn_rule_props->mask);
    p_rtn_rule_props->offset = htons(p_rtn_rule_props->offset);
    hton_enum(&(p_rtn_rule_props->match_action), sizeof(fpp_fp_rule_match_action_t));
    hton_enum(&(p_rtn_rule_props->offset_from), sizeof(fpp_fp_offset_from_t));
}
 
 
/*
 * @brief          Host-to-network (hton) function for a FP table struct when a rule is
 *                 inserted into the table.
 * @param[in,out]  p_rtn_table  The FP table struct to be converted.
 */
static void hton_table(fpp_fp_table_cmd_t* p_rtn_table)
{
    assert(NULL != p_rtn_table);
    p_rtn_table->table_info.t.position = htons(p_rtn_table->table_info.t.position);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested FP rule
 *              from the PFE. Identify the rule by its name.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_rule   Space for data from the PFE.
 * @param[out]  p_rtn_idx    Space for index of the requested FP rule (from the PFE).
 *                           This is a generic index of the given rule in a common pool of
 *                           FP rules. It has no ties to any FP table.
 *                           Can be NULL. If NULL, then no index is stored.
 * @param[in]   p_rule_name  Name of the requested FP rule.
 *                           Names of FP rules are user-defined. See fci_fp_rule_add().
 * @return      FPP_ERR_OK : Requested FP rule was found.
 *                           A copy of its configuration was stored into p_rtn_rule.
 *                           Its index in a common pool of FP rules was stored into p_rtn_idx.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_fp_rule_get_by_name(FCI_CLIENT* p_cl, fpp_fp_rule_cmd_t* p_rtn_rule, 
                            uint16_t* p_rtn_idx, const char* p_rule_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_rule);
    assert(NULL != p_rule_name);
    /* 'p_rtn_index' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    fpp_fp_rule_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t idx = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                    sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_rule_props(&(reply_from_fci.r));  /* set correct byte order of rule properties */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && strcmp(p_rule_name, (char*)(reply_from_fci.r.rule_name)))
    {
        idx++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                        sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_rule_props(&(reply_from_fci.r));  /* set correct byte order of rule properties */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_rule = reply_from_fci;
        if (NULL != p_rtn_idx)
        {
            *p_rtn_idx = idx;
        }
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */
 
 
/*
 * @brief       Use FCI calls to create a new FP rule in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[in]   p_rule_name  Name of the new FP rule.
 *                           The name is user-defined.
 * @param[in]   p_rule_data  Configuration data for the new FP rule.
 *                           To create a new FP rule, a local data struct must be created,
 *                           configured and then passed to this function.
 *                           See [localdata] functions to learn more.
 * @return      FPP_ERR_OK : New FP rule was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_fp_rule_add(FCI_CLIENT* p_cl, const char* p_rule_name,
                    const fpp_fp_rule_cmd_t* p_rule_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rule_name);
    assert(NULL != p_rule_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_rule_data;
    rtn = set_text((char*)(cmd_to_fci.r.rule_name), p_rule_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_rule_props(&(cmd_to_fci.r));  /* set correct byte order of rule properties */
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t), 
                                              (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target FP rule in the PFE.
 * @param[in]  p_cl         FCI client instance
 * @param[in]  p_rule_name  Name of the FP rule to destroy.
 * @return     FPP_ERR_OK : FP rule was destroyed.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fp_rule_del(FCI_CLIENT* p_cl, const char* p_rule_name)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.r.rule_name), p_rule_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_rule_props(&(cmd_to_fci.r));  /* set correct byte order of rule properties */
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t), 
                                              (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new FP table in the PFE.
 * @param[in]   p_cl          FCI client instance
 * @param[in]   p_table_name  Name of the new FP table.
 *                            The name is user-defined.
 * @return      FPP_ERR_OK : New FP table was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_fp_table_add(FCI_CLIENT* p_cl, const char* p_table_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        /* only text was set, no need to change byte order */
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target FP table in the PFE.
 * @param[in]  p_cl          FCI client instance
 * @param[in]  p_table_name  Name of the FP table to destroy.
 * @return     FPP_ERR_OK : FP table was destroyed.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fp_table_del(FCI_CLIENT* p_cl, const char* p_table_name)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        /* only text was set, no need to change byte order */
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to insert a FP rule at given position of a FP table in the PFE.
 * @param[in]   p_cl          FCI client instance
 * @param[in]   p_table_name  Name of an existing FP table.
 * @param[in]   p_rule_name   Name of an existing FP rule.
 * @param[in]   position      Index where to insert the rule. Starts at 0.
 * @return      FPP_ERR_OK : The rule was successfully inserted into the table.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_fp_table_insert_rule(FCI_CLIENT* p_cl, const char* p_table_name, 
                             const char* p_rule_name, uint16_t position)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    assert(NULL != p_rule_name);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((char*)(cmd_to_fci.table_info.t.rule_name), p_rule_name, IFNAMSIZ);
    }
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.table_info.t.position = position;
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_table(&cmd_to_fci);  /* set correct byte order */
        cmd_to_fci.action = FPP_ACTION_USE_RULE;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to remove a FP rule from a FP table in the PFE.
 * @param[in]   p_cl          FCI client instance
 * @param[in]   p_table_name  Name of an existing FP table.
 * @param[in]   p_rule_name   Name of an existing FP rule.
 * @return      FPP_ERR_OK : The rule was successfully removed from the table.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_fp_table_remove_rule(FCI_CLIENT* p_cl, const char* p_table_name, 
                             const char* p_rule_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_table_name);
    assert(NULL != p_rule_name);
    
    
    int rtn = FPP_ERR_FCI;
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
        hton_table(&cmd_to_fci);  /* set correct byte order */
        cmd_to_fci.action = FPP_ACTION_UNUSE_RULE;
        rtn = fci_write(p_cl, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t), 
                                               (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_fprule  [localdata_fprule]
 * @brief:      Functions marked as [localdata_fprule] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_fp_rule_add() to 
 *              create a new FP rule with given configuration in the PFE.
 */
 
 
/*
 * @brief          Set data template of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 *                         For FP rules, there are no "initial data" to be obtained from PFE.
 *                         Simply declare a local data struct and configure it.
 * @param[in]      data    Data template (value)
 *                         This value will be compared with a selected value from 
 *                         the inspected traffic.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_fp_rule_ld_set_data(fpp_fp_rule_cmd_t* p_rule, uint32_t data)
{
    assert(NULL != p_rule);
    p_rule->r.data = data;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set bitmask of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 *                         For FP rules, there are no "initial data" to be obtained from PFE.
 *                         Simply declare a local data struct and configure it.
 * @param[in]      mask    Bitmask for more precise data selection.
 *                         This bitmask is applied on the selected 32bit value from
 *                         the inspected traffic.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_fp_rule_ld_set_mask(fpp_fp_rule_cmd_t* p_rule, uint32_t mask)
{
    assert(NULL != p_rule);
    p_rule->r.mask = mask;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set offset and base for offset ("offset from") of a FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 *                         For FP rules, there are no "initial data" to be obtained from PFE.
 *                         Simply declare a local data struct and configure it.
 * @param[in]      offset  Offset (in bytes) into traffic's data.
 *                         This offset is applied from the respective base ("offset_from").
 *                         A 32bit data value which lies on the offset is the value selected
 *                         for comparison under the given FP rule.
 * @param[in]      offset_from  The base for offset calculation.
 *                              See description of fpp_fp_offset_from_t type 
 *                              in the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_fp_rule_ld_set_offset(fpp_fp_rule_cmd_t* p_rule, uint16_t offset, 
                              fpp_fp_offset_from_t offset_from)
{
    assert(NULL != p_rule);
    p_rule->r.offset = offset;
    p_rule->r.offset_from = offset_from;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset inverted mode of FP rule match evaluation.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 *                         For FP rules, there are no "initial data" to be obtained from PFE.
 *                         Simply declare a local data struct and configure it.
 * @param[in]      invert  A request to set/unset the inverted mode of evaluation.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_fp_rule_ld_set_invert(fpp_fp_rule_cmd_t* p_rule, bool invert)
{
    assert(NULL != p_rule);
    p_rule->r.invert = invert;  /* NOTE: Implicit cast of bool to uintX_t */
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set action to be done if inspected traffic satisfies the given FP rule.
 * @details        [localdata_fprule]
 * @param[in,out]  p_rule  Local data to be modified.
 *                         For FP rules, there are no "initial data" to be obtained from PFE.
 *                         Simply declare a local data struct and configure it.
 * @param[in]      match_action      An action to be done.
 *                                   See description of fpp_fp_rule_match_action_t type
 *                                   in the FCI API Reference.
 * @param[in]      p_next_rule_name  Name of the next FP rule to execute.
 *                                   Is meaningful only if the match action is FP_NEXT_RULE.
 *                                   Can be NULL. If NULL or "" (empty string), 
 *                                   then no rule is set as the next rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_fp_rule_ld_set_match_action(fpp_fp_rule_cmd_t* p_rule, 
                                    fpp_fp_rule_match_action_t match_action,
                                    const char* p_next_rule_name)
{
    assert(NULL != p_rule);
    /* 'p_next_rule_name' is allowed to be NULL */
    
    p_rule->r.match_action = match_action;
    return set_text((char*)(p_rule->r.next_rule_name), p_next_rule_name, IFNAMSIZ);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all FP rules of a given FP table in the PFE.
 *             Execute a print function for each reported FP rule.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next FP rule in table is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_table_name   Name of a FP table.
 *                            Names of FP tables are user-defined. See fci_fp_table_add().
 * @param[in]  position_init  Start invoking callback print function from 
 *                            this position in the table.
 *                            If 0, start from the very first FP rule in the table.
 * @param[in]  count          Print only this count of FP rules, then end.
 *                            If 0, keep printing FP rules till the end of the table.
 * @return     FPP_ERR_OK : Successfully iterated through FP rules of the given FP table.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fp_table_print(FCI_CLIENT* p_cl, fci_fp_rule_cb_print_t p_cb_print, 
                       const char* p_table_name, uint16_t position_init, uint16_t count)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_table_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fp_table_cmd_t cmd_to_fci = {0};
    fpp_fp_table_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((char*)(cmd_to_fci.table_info.t.table_name), p_table_name, IFNAMSIZ);
    if (0u == count)  /* if 0, set max possible count of items */
    {
        count--;  /* WARNING: intentional use of owf behavior */
    }
    
    /*  do the query  */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_FP_TABLE,
                        sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_rule_props(&(reply_from_fci.table_info.r));  /* set correct byte order */
    
        /* query loop */
        uint16_t position = 0u;
        while ((FPP_ERR_OK == rtn) && (0u != count))
        {
            if (position >= position_init)
            {
                rtn = p_cb_print(&(reply_from_fci.table_info.r), position);
                count--;
            }
            
            position++;
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_FP_TABLE,
                                sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
                ntoh_rule_props(&(reply_from_fci.table_info.r));  /* set correct byte order */
            }
        }
        
        /* query loop runs till there are no more FP rules to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_FP_RULE_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all existing FP rules in the PFE (regardless
 *             of table affiliation). Execute a print function for each reported FP rule.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next FP rule is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  idx_init       Start invoking callback print function from 
 *                            this index of FP rule query.
 *                            If 0, start from the very first queried FP rule.
 * @param[in]  count          Print only this count of FP rules, then end.
 *                            If 0, keep printing FP rules till there is no more available.
 * @return     FPP_ERR_OK : Successfully iterated through FP rules.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_fp_rule_print_all(FCI_CLIENT* p_cl, fci_fp_rule_cb_print_t p_cb_print, 
                          uint16_t idx_init, uint16_t count)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
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
    ntoh_rule_props(&(reply_from_fci.r));  /* set correct byte order */

    /* query loop */
    uint16_t idx = 0u;
    while ((FPP_ERR_OK == rtn) && (0u != count))
    {
        if (idx >= idx_init)
        {
            rtn = p_cb_print(&(reply_from_fci.r), idx);
            count--;
        }
        
        idx++;
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                            sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_rule_props(&(reply_from_fci.r));  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more FP rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_FP_RULE_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all FP rules in the PFE (regardless
 *              of table affiliation).
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of FP rules.
 * @return      FPP_ERR_OK : Successfully counted FP rules.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_fp_rule_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_fp_rule_cmd_t cmd_to_fci = {0};
    fpp_fp_rule_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                    sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting FP rules) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_FP_RULE,
                        sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting FP rules) */
    }
    
    /* query loop runs till there are no more FP rules to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_FP_RULE_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
