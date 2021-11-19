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
#include "demo_qos_pol.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief       Set/unset a flow type flag (from argumentless set) in a policer flow struct.
 * @param[out]  p_rtn_polflow  Struct to be modified.
 * @param[in]   enable         New state of a flag.
 * @param[in]   flag           The flag.
 */
static void set_polflow_m_flag(fpp_qos_policer_flow_cmd_t* p_rtn_polflow, bool enable, 
                               fpp_iqos_flow_type_t flag)
{
    assert(NULL != p_rtn_polflow);
    
    hton_enum(&flag, sizeof(fpp_iqos_flow_type_t));
    
    if (enable)
    {
        p_rtn_polflow->flow.type_mask |= flag;
    }
    else
    {
        p_rtn_polflow->flow.type_mask &= (fpp_iqos_flow_type_t)(~flag);
    }
}
 
 
/*
 * @brief       Set/unset a flow type flag (from argumentful set) in a policer flow struct.
 * @param[out]  p_rtn_polflow  Struct to be modified.
 * @param[in]   enable         New state of a flag.
 * @param[in]   flag           The flag.
 */
static void set_polflow_am_flag(fpp_qos_policer_flow_cmd_t* p_rtn_polflow, bool enable, 
                                fpp_iqos_flow_arg_type_t flag)
{
    assert(NULL != p_rtn_polflow);
    
    hton_enum(&flag, sizeof(fpp_iqos_flow_arg_type_t));
    
    if (enable)
    {
        p_rtn_polflow->flow.arg_type_mask |= flag;
    }
    else
    {
        p_rtn_polflow->flow.arg_type_mask &= (fpp_iqos_flow_arg_type_t)(~flag);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested Ingress QoS policer
 *              from PFE.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_pol      Space for data from PFE.
 * @param[in]   p_phyif_name   Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : The requested Ingress QoS policer was found.
 *                           A copy of its configuration data was stored into p_rtn_pol.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_pol_get(FCI_CLIENT* p_cl, fpp_qos_policer_cmd_t* p_rtn_pol, const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_pol);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the Ingress QoS policer directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER,
                        sizeof(fpp_qos_policer_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_pol = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_pol_get() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested Ingress QoS wred
 *              from PFE. Identify the Ingress QoS wred by the name of a parent 
 *              physical interface and by the associated wred queue.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_polwred  Space for data from PFE.
 * @param[in]   p_phyif_name   Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 *              polwred_que    Associated queue.
 * @return      FPP_ERR_OK : The requested Ingress QoS wred was found.
 *                           A copy of its configuration data was stored into p_rtn_polwred.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_polwred_get_by_que(FCI_CLIENT* p_cl, fpp_qos_policer_wred_cmd_t* p_rtn_polwred, 
                            const char* p_phyif_name, fpp_iqos_queue_t polwred_que)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_polwred);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_wred_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_wred_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.queue = polwred_que;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the Ingress QoS shaper directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_WRED,
                        sizeof(fpp_qos_policer_wred_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_polwred = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_polwred_get_by_que() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested Ingress QoS shaper
 *              from PFE. Identify the Ingress QoS shaper by the name of a parent 
 *              physical interface and by the shaper's ID.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_polshp   Space for data from PFE.
 * @param[in]   p_phyif_name   Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 *              polshp_id      ID of the requested Ingress QoS shaper.
 * @return      FPP_ERR_OK : The requested Ingress QoS shaper was found.
 *                           A copy of its configuration data was stored into p_rtn_polshp.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_polshp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_policer_shp_cmd_t* p_rtn_polshp, 
                          const char* p_phyif_name, uint8_t polshp_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_polshp);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_shp_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_shp_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = polshp_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the Ingress QoS shaper directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_SHP,
                        sizeof(fpp_qos_policer_shp_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_polshp = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_polshp_get_by_id() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested Ingress QoS flow
 *              from PFE. Identify the Ingress QoS flow by the name of a parent 
 *              physical interface and by the flow's ID.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_polflow  Space for data from PFE.
 * @param[in]   p_phyif_name   Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 *              polflow_id     ID of the requested Ingress QoS flow.
 * @return      FPP_ERR_OK : The requested Ingress QoS flow was found.
 *                           A copy of its configuration data was stored into p_rtn_polflow.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_polflow_get_by_id(FCI_CLIENT* p_cl, fpp_qos_policer_flow_cmd_t* p_rtn_polflow, 
                           const char* p_phyif_name, uint8_t polflow_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_polflow);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_flow_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_flow_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = polflow_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                        sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        
        /* query loop (with a search condition) */
        while ((FPP_ERR_OK == rtn) && (reply_from_fci.id != polflow_id))
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                           sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
                           &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_polflow = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_polflow_get_by_id() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief       Use FCI calls to enable/disable Ingress QoS block of a physical interface.
 * @param[in]   p_cl  FCI client
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @param[in]   enable        Enable/disable Ingress QoS block of a physical interface.
 * @return      FPP_ERR_OK : Static MAC table entries of all bridge domains were 
 *                           successfully flushed in PFE.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_pol_enable(FCI_CLIENT* p_cl, const char* p_phyif_name, bool enable)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.enable = enable;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_UPDATE;
        rtn = fci_write(p_cl, FPP_CMD_QOS_POLICER, sizeof(fpp_qos_policer_cmd_t), 
                                                  (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_pol_enable() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target Ingress QoS wred
 *                 in PFE.
 * @param[in]      p_cl   FCI client
 * @param[in,out]  p_polwred  Local data struct which represents a new configuration of 
 *                            the target Ingress QoS wred.
 *                            Initial data can be obtained via demo_polwred_get_by_que().
 * @return        FPP_ERR_OK : Configuration of the target Ingress QoS wred was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct not updated.
 */
int demo_polwred_update(FCI_CLIENT* p_cl, fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(NULL != p_cl);
    assert(NULL != p_polwred);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_policer_wred_cmd_t cmd_to_fci = (*p_polwred);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_POLICER_WRED, sizeof(fpp_qos_policer_wred_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polwred_get_by_que(p_cl, p_polwred, (p_polwred->if_name), 
                                                       (p_polwred->queue));
    }
    
    print_if_error(rtn, "demo_polwred_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target Ingress QoS shaper
 *                 in PFE.
 * @param[in]      p_cl   FCI client
 * @param[in,out]  p_polshp   Local data struct which represents a new configuration of 
 *                            the target Ingress QoS shaper.
 *                            Initial data can be obtained via demo_polshp_get_by_id().
 * @return        FPP_ERR_OK : Configuration of the target Ingress QoS shaper was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct not updated.
 */
int demo_polshp_update(FCI_CLIENT* p_cl, fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(NULL != p_cl);
    assert(NULL != p_polshp);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_policer_shp_cmd_t cmd_to_fci = (*p_polshp);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_POLICER_SHP, sizeof(fpp_qos_policer_shp_cmd_t), 
                                                  (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polshp_get_by_id(p_cl, p_polshp, (p_polshp->if_name), (p_polshp->id));
    }
    
    print_if_error(rtn, "demo_polwred_update() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new Ingress QoS flow for a target physical interface
 *              in PFE.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_polflow  Space for data from PFE.
 *                             This will contain a copy of configuration data of 
 *                             the newly created Ingress QoS flow.
 *                             Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   p_phyif_name   Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 * @param[in]   polflow_id     ID of the requested Ingress QoS flow.
 * @param[in]   p_polflow_data Configuration data of the new Ingress QoS flow.
 *                             To create a new flow, a local data struct must be created,
 *                             configured and then passed to this function.
 *                             See [localdata_polflow] to learn more.
 * @return      FPP_ERR_OK : New Ingress QoS flow was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_polflow.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_polflow_add(FCI_CLIENT* p_cl, const char* p_phyif_name, uint8_t polflow_id, 
                     fpp_qos_policer_flow_cmd_t* p_polflow_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    /* 'p_rtn_polflow' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_policer_flow_cmd_t cmd_to_fci = (*p_polflow_data);
    
    /* prepare data */
    cmd_to_fci.id = polflow_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_QOS_POLICER_FLOW, sizeof(fpp_qos_policer_flow_cmd_t), 
                                                       (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_polflow_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target Ingress QoS flow in PFE.
 * @param[in]  p_cl    FCI client
 * @param[in]  p_phyif_name   Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 *             polflow_id     ID of the Ingress QoS flow to destroy.
 * @return     FPP_ERR_OK : The Ingress QoS flow was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_polflow_del(FCI_CLIENT* p_cl, const char* p_phyif_name, uint8_t polflow_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_policer_flow_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.id = polflow_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_QOS_POLICER_FLOW, sizeof(fpp_qos_policer_flow_cmd_t), 
                                                       (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_polflow_del() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_polflow  [localdata_polflow]
 * @brief:      Functions marked as [localdata_polflow] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_polflow (a struct with configuration data).
 *              Initial data for p_polflow can be obtained via demo_polflow_get_by_id().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_polflow_update() to update
 *              the configuration of a real Ingress QoS flow in PFE.
 */
 
 
/*
 * @brief          Clear all argumentless flow types of an Ingress QoS flow.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      action     Requested action for Ingress QoS flow.
 */
void demo_polflow_ld_set_action(fpp_qos_policer_flow_cmd_t* p_polflow, 
                                fpp_iqos_flow_action_t action)
{
    assert(NULL != p_polflow);
    p_polflow->flow.action = action;
}
 
 
/*
 * @brief          Clear all argumentless flow types of an Ingress QoS flow.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 */
void demo_polflow_ld_clear_m(fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    p_polflow->flow.type_mask = 0u;
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_ETH).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_eth(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_ETH);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_PPPOE).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_pppoe(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_PPPOE);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_ARP).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_arp(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_ARP);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_IP4).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_ip4(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_IPV4);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_IP6).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_ip6(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_IPV6);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_IPX).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_ipx(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_IPX);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_MCAST).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_mcast(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_MCAST);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_BCAST).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_bcast(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_BCAST);
}
 
 
/*
 * @brief          Set/unset the given argumentless flow type (TYPE_VLAN).
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow tpye.
 */
void demo_polflow_ld_set_m_type_vlan(fpp_qos_policer_flow_cmd_t* p_polflow, bool set)
{
    assert(NULL != p_polflow);
    set_polflow_m_flag(p_polflow, set, FPP_IQOS_FLOW_TYPE_VLAN);
}
 
 
 
 
/*
 * @brief          Clear all argumentful flow types of an Ingress QoS flow.
 *                 (also zeroify all associated flow type arguments)
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 */
void demo_polflow_ld_clear_am(fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    p_polflow->flow.arg_type_mask = 0u;
    memset(&(p_polflow->flow.args), 0, sizeof(fpp_iqos_flow_args_t));
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (VLAN) and its argument.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      vlan       New VLAN ID for this flow type.
 *                            When this flow type is active, it compares value of its
 *                            'vlan' argument with the value of traffic's 'VID' field.
 *                            Comparison is bitmasked by value from vlan_m argument.
 * @param[in]      vlan_m     New bitmask for VLAN ID.
 */
void demo_polflow_ld_set_am_vlan(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                 uint16_t vlan, uint16_t vlan_m)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_VLAN);
    p_polflow->flow.args.vlan = htons(vlan);
    p_polflow->flow.args.vlan_m = htons(vlan_m);
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (TOS) and its argument.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      tos        New TOS/TCLASS value for this flow type to match.
 *                            When this flow type is active, it compares value of its
 *                            'tos' argument with the value of traffic's 'TOS' field.
 *                            Comparison is bitmasked by value from tos_m argument.
 * @param[in]      tos_m      New bitmask for TOS/TCLASS.
 */
void demo_polflow_ld_set_am_tos(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                uint8_t tos, uint8_t tos_m)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_TOS);
    p_polflow->flow.args.tos = tos;
    p_polflow->flow.args.tos_m = tos_m;
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (L4PROTO) and its argument.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      tos        New PROTOCOL value for this flow type to match.
 *                            When this flow type is active, it compares value of its
 *                            'l4proto' argument with the value of traffic's 'Protocol' field.
 *                            Comparison is bitmasked by value from l4proto_m argument.
 * @param[in]      tos_m      New bitmask for PROTOCOL.
 */
void demo_polflow_ld_set_am_proto(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                  uint8_t proto, uint8_t proto_m)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_L4PROTO);
    p_polflow->flow.args.l4proto = proto;
    p_polflow->flow.args.l4proto_m = proto_m;
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (SIP) and its argument.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      sip        New source IP address for this flow type to match.
 *                            When this flow type is active, it compares value of its
 *                            'sip' argument with the value of traffic's 
 *                            'source address'.
 *                            Comparison is bitmasked by source address subnet prefix.
 * @param[in]      sip_m      New subnet prefix for source IP address.
 */
void demo_polflow_ld_set_am_sip(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                uint32_t sip, uint8_t sip_m)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_SIP);
    p_polflow->flow.args.sip = htonl(sip);
    p_polflow->flow.args.sip_m = sip_m;
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (DIP) and its argument.
 * @details        [localdata_polflow]
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      dip        New destination IP address for this flow type to match.
 *                            When this flow type is active, it compares value of its
 *                            'dip' argument with the value of traffic's 
 *                            'destination address'.
 *                            Comparison is bitmasked by destination address subnet prefix.
 * @param[in]      dip_m      New subnet prefix for destination IP address.
 */
void demo_polflow_ld_set_am_dip(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                uint32_t dip, uint8_t dip_m)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_DIP);
    p_polflow->flow.args.dip = htonl(dip);
    p_polflow->flow.args.dip_m = dip_m;
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (DIP) and its argument.
 * @details        [localdata_polflow]
 *                 When this flow type is active, it compares traffic's 'source port'
 *                 with a defined range of source ports (from min to max).
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      sport_min  New range of source ports - minimal port.
 * @param[in]      sport_max  New range of source ports - maximal port.
 */
void demo_polflow_ld_set_am_sport(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                  uint16_t sport_min, uint16_t sport_max)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_SPORT);
    p_polflow->flow.args.sport_min = htons(sport_min);
    p_polflow->flow.args.sport_max = htons(sport_max);
}
 
 
/*
 * @brief          Set/unset the given argumentful flow type (DIP) and its argument.
 * @details        [localdata_polflow]
 *                 When this flow type is active, it compares traffic's 'destination port'
 *                 with a defined range of destination ports (from min to max).
 * @param[in,out]  p_polflow  Local data to be modified.
 * @param[in]      set        Request to set/unset the given flow type.
 * @param[in]      dport_min  New range of destination ports - minimal port.
 * @param[in]      dport_max  New range of destination ports - maximal port.
 */
void demo_polflow_ld_set_am_dport(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, 
                                  uint16_t dport_min, uint16_t dport_max)
{
    assert(NULL != p_polflow);
    
    set_polflow_am_flag(p_polflow, set, FPP_IQOS_ARG_DPORT);
    p_polflow->flow.args.dport_min = htons(dport_min);
    p_polflow->flow.args.dport_max = htons(dport_max);
}
 
 
 
 
/*
 * @defgroup    localdata_wred  [localdata_polwred]
 * @brief:      Functions marked as [localdata_polwred] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_polwred (a struct with configuration data).
 *              Initial data for p_polwred can be obtained via demo_polwred_get_by_que().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_polwred_update() to update
 *              the configuration of a real Ingress QoS wred in PFE.
 */
 
 
/*
 * @brief          Enable/disable Ingress QoS wred.
 * @details        [localdata_polwred]
 * @param[in,out]  p_polwred  Local data to be modified.
 * @param[in]      enable     Enable/disable Ingress QoS wred.
 */
void demo_polwred_ld_enable(fpp_qos_policer_wred_cmd_t* p_polwred, bool enable)
{
    assert(NULL != p_polwred);
    p_polwred->enable = enable;
}
 
 
/*
 * @brief          Set a minimal threshold of Ingress QoS wred.
 * @details        [localdata_polwred]
 * @param[in,out]  p_polwred  Local data to be modified.
 * @param[in]      min        Minimal threshold.
 */
void demo_polwred_ld_set_min(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t min)
{
    assert(NULL != p_polwred);
    p_polwred->thr[FPP_IQOS_WRED_MIN_THR] = htons(min);
}
 
 
/*
 * @brief          Set a maximal threshold of Ingress QoS wred.
 * @details        [localdata_polwred]
 * @param[in,out]  p_polwred  Local data to be modified.
 * @param[in]      max        Maximal threshold.
 */
void demo_polwred_ld_set_max(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t max)
{
    assert(NULL != p_polwred);
    p_polwred->thr[FPP_IQOS_WRED_MAX_THR] = htons(max);
}
 
 
/*
 * @brief          Set a queue length ('full' threshold) of Ingress QoS wred.
 * @details        [localdata_polwred]
 * @param[in,out]  p_polwred  Local data to be modified.
 * @param[in]      full       Maximal threshold.
 */
void demo_polwred_ld_set_full(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t full)
{
    assert(NULL != p_polwred);
    p_polwred->thr[FPP_IQOS_WRED_FULL_THR] = htons(full);
}
 
 
/*
 * @brief          Set packet drop probability of a particular Ingress QoS wred's zone.
 * @details        [localdata_polwred]
 * @param[in,out]  p_polwred   Local data to be modified.
 * @param[in]      zprob_id    ID of a probability zone.
 * @param[in]      percentage  Drop probability in [%].
 */
void demo_polwred_ld_set_zprob(fpp_qos_policer_wred_cmd_t* p_polwred, uint8_t zprob_id, 
                               uint8_t percentage)
{
    assert(NULL != p_polwred);
    if (FPP_IQOS_WRED_ZONES_COUNT > zprob_id)
    {
        /* FCI command for Ingress QoS wred expects drop probability in compressed format */
        const uint8_t compressed = (uint8_t)((percentage * 0x0Fu) / 100u);
        
        p_polwred->zprob[zprob_id] = compressed;
    }
}
 
 
 
 
/*
 * @defgroup    localdata_polshp  [localdata_polshp]
 * @brief:      Functions marked as [localdata_polshp] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_polshp (a struct with configuration data).
 *              Initial data for p_polshp can be obtained via demo_polshp_get_by_id().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_polshp_update() to update
 *              the configuration of a real Ingress QoS shaper in PFE.
 */
 
 
/*
 * @brief          Enable/disable Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp  Local data to be modified.
 * @param[in]      enable    Enable/disable Ingress QoS shaper.
 */
void demo_polshp_ld_enable(fpp_qos_policer_shp_cmd_t* p_polshp, bool enable)
{
    assert(NULL != p_polshp);
    p_polshp->enable = enable;
}
 
 
/*
 * @brief          Set a type of Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp  Local data to be modified.
 * @param[in]      shp_type  Shaper type.
 */
void demo_polshp_ld_set_type(fpp_qos_policer_shp_cmd_t* p_polshp, 
                             fpp_iqos_shp_type_t shp_type)
{
    assert(NULL != p_polshp);
    p_polshp->type = shp_type;
}
 
 
/*
 * @brief          Set a mode of Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp  Local data to be modified.
 * @param[in]      shp_mode  Shaper mode.
 */
void demo_polshp_ld_set_mode(fpp_qos_policer_shp_cmd_t* p_polshp, 
                             fpp_iqos_shp_rate_mode_t shp_mode)
{
    assert(NULL != p_polshp);
    p_polshp->mode = shp_mode;
}
 
 
/*
 * @brief          Set an idle slope rate of Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp  Local data to be modified.
 * @param[in]      isl       Idle slope rate (units per second).
 *                           Units depend on the mode of a QoS shaper.
 */
void demo_polshp_ld_set_isl(fpp_qos_policer_shp_cmd_t* p_polshp, uint32_t isl)
{
    assert(NULL != p_polshp);
    p_polshp->isl = htonl(isl);
}
 
 
/*
 * @brief          Set a minimal credit of  Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp    Local data to be modified.
 * @param[in]      min_credit  Minimal credit.
 */
void demo_polshp_ld_set_min_credit(fpp_qos_policer_shp_cmd_t* p_polshp, int32_t min_credit)
{
    assert(NULL != p_polshp);
    p_polshp->min_credit = (int32_t)(htonl(min_credit));
}
 
 
/*
 * @brief          Set a maximal credit of Ingress QoS shaper.
 * @details        [localdata_polshp]
 * @param[in,out]  p_polshp    Local data to be modified.
 * @param[in]      max_credit  Maximal credit.
 */
void demo_polshp_ld_set_max_credit(fpp_qos_policer_shp_cmd_t* p_polshp, int32_t max_credit)
{
    assert(NULL != p_polshp);
    p_polshp->max_credit = (int32_t)(htonl(max_credit));
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query the name of a parent physical interface of Ingress QoS policer.
 * @details    p_pol  Local data of Ingress QoS policer.
 * @return     Name of a parent physical interface.
 */
const char* demo_pol_ld_get_if_name(const fpp_qos_policer_cmd_t* p_pol)
{
    assert(p_pol);
    return (p_pol->if_name);
}
 
 
/*
 * @brief      Query the status of Ingress QoS policer "enable" flag.
 * @details    p_pol  Local data of Ingress QoS policer.
 * @return     At time when the data was obtained from PFE, the Ingress QoS policer:
 *             true  : was enabled
 *             false : was disabled
 */
bool demo_pol_ld_is_enabled(const fpp_qos_policer_cmd_t* p_pol)
{
    assert(p_pol);
    return (p_pol->enable);
}
 
 
 
 
/*
 * @brief      Query the name of a parent physical interface of Ingress QoS flow.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_polflow_ld_get_if_name(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(p_polflow);
    return (p_polflow->if_name);
}
 
 
/*
 * @brief      Query the ID of Ingress QoS flow.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     ID of Ingress QoS flow.
 */
uint8_t demo_polflow_ld_get_id(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(p_polflow);
    return (p_polflow->id);
}
 
 
/*
 * @brief      Query the action of Ingress QoS flow.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Action
 */
fpp_iqos_flow_action_t demo_polflow_ld_get_action(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(p_polflow);
    return (p_polflow->flow.action);
}
 
/*
 * @brief      Query the argumentless flow types bitset of Ingress QoS flow.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitset of argumentless flow types.
 */
fpp_iqos_flow_type_t demo_polflow_ld_get_m_bitset(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    
    fpp_iqos_flow_type_t type_mask = (p_polflow->flow.type_mask);
    ntoh_enum(&type_mask, sizeof(fpp_iqos_flow_type_t));
    
    return (type_mask);
}
 
 
/*
 * @brief      Query the argumentful flow types bitset of Ingress QoS flow.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitset of argumentful flow types.
 */
fpp_iqos_flow_arg_type_t demo_polflow_ld_get_am_bitset(
    const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    
    fpp_iqos_flow_arg_type_t arg_type_mask = (p_polflow->flow.arg_type_mask);
    ntoh_enum(&arg_type_mask, sizeof(fpp_iqos_flow_arg_type_t));
    
    return (arg_type_mask);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type VLAN.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (VLAN ID) of the given flow type.
 */
uint16_t demo_polflow_ld_get_am_vlan(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.vlan);
}
 
 
/*
 * @brief      Query the bitmask of the argumentful flow type VLAN.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitmask for VLAN ID.
 */
uint16_t demo_polflow_ld_get_am_vlan_m(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.vlan_m);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type TOS.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (TOS) of the given flow type.
 */
uint8_t  demo_polflow_ld_get_am_tos(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.tos);
}
 
 
/*
 * @brief      Query the bitmask of the argumentful flow type TOS.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitmask for TOS.
 */
uint8_t demo_polflow_ld_get_am_tos_m(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.tos_m);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type PROTOCOL.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (Protocol ID) of the given flow type.
 */
uint8_t  demo_polflow_ld_get_am_proto(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.l4proto);
}
 
 
/*
 * @brief      Query the bitmask of the argumentful flow type PROTOCOL.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitmask for Protocol ID.
 */
uint8_t demo_polflow_ld_get_am_proto_m(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.l4proto_m);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type SIP.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (source IP address) of the given flow type.
 */
uint32_t demo_polflow_ld_get_am_sip(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohl(p_polflow->flow.args.sip);
}
 
 
/*
 * @brief      Query the bitmask of the argumentful flow type SIP.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitmask for source IP address.
 */
uint8_t demo_polflow_ld_get_am_sip_m(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.sip_m);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type DIP.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (destination IP address) of the given flow type.
 */
uint32_t demo_polflow_ld_get_am_dip(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohl(p_polflow->flow.args.dip);
}
 
 
/*
 * @brief      Query the bitmask of the argumentful flow type SIP.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Bitmask for destination IP address.
 */
uint8_t demo_polflow_ld_get_am_dip_m(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return (p_polflow->flow.args.dip_m);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type SPORT.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (source port range - minimal port) of the given flow type.
 */
uint16_t demo_polflow_ld_get_am_sport_min(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.sport_min);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type SPORT.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (source port range - maximal port) of the given flow type.
 */
uint16_t demo_polflow_ld_get_am_sport_max(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.sport_max);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type DPORT.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (destination port range - minimal port) of the given flow type.
 */
uint16_t demo_polflow_ld_get_am_dport_min(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.dport_min);
}
 
 
/*
 * @brief      Query the argument of the argumentful flow type DPORT.
 * @details    [localdata_polflow]
 * @param[in]  p_polflow  Local data to be queried.
 * @return     Argument (destination port range - maximal port) of the given flow type.
 */
uint16_t demo_polflow_ld_get_am_dport_max(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    assert(NULL != p_polflow);
    return ntohs(p_polflow->flow.args.dport_max);
}
 
 
 
 
/*
 * @brief      Query the name of a parent physical interface of Ingress QoS wred.
 * @details    [localdata_polwred]
 * @param[in]  p_polwred  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_polwred_ld_get_if_name(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return (p_polwred->if_name);
}
 
 
/*
 * @brief      Query the queue of Ingress QoS wred.
 * @details    [localdata_polwred]
 * @param[in]  p_polwred  Local data to be queried.
 * @return     Queue of the given Ingress QoS wred.
 */
fpp_iqos_queue_t demo_polwred_ld_get_que(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return (p_polwred->queue);
}
 
 
/*
 * @brief      Query the status of Ingress QoS wred "enable" flag.
 * @param[in]  p_polwred  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the Ingress QoS wred:
 *             true  : was enabled
 *             false : was disabled
 */
bool demo_polwred_ld_is_enabled(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return (p_polwred->enable);
}
 
 
/*
 * @brief      Query the minimal threshold of Ingress QoS wred.
 * @details    [localdata_polwred]
 * @param[in]  p_polwred  Local data to be queried.
 * @return     Minimal threshold of Ingress QoS wred.
 */
uint16_t demo_polwred_ld_get_min(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return ntohs(p_polwred->thr[FPP_IQOS_WRED_MIN_THR]);
}
 
 
/*
 * @brief      Query the maximal threshold of Ingress QoS wred.
 * @details    [localdata_polwred]
 * @param[in]  p_polwred  Local data to be queried.
 * @return     Maximal threshold of Ingress QoS wred.
 */
uint16_t demo_polwred_ld_get_max(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return ntohs(p_polwred->thr[FPP_IQOS_WRED_MAX_THR]);
}
 
 
/*
 * @brief      Query the queue length (full threshold) of Ingress QoS wred.
 * @details    [localdata_polwred]
 * @param[in]  p_polwred  Local data to be queried.
 * @return     Queue length (full threshold) of Ingress QoS wred.
 */
uint16_t demo_polwred_ld_get_full(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    assert(p_polwred);
    return ntohs(p_polwred->thr[FPP_IQOS_WRED_FULL_THR]);
}
 
 
/*
 * @brief      Query the percentage chance for packet drop.
 * @details    [localdata_polwred]
 * @param[in]  p_que     Local data to be queried.
 * @param[in]  zprob_id  ID of a probability zone.
 *                       There may be less than 32 zones actually implemented in PFE.
 *                       (32 is just the max array limit)
 *                       See FCI API Reference, chapter Egress QoS.
 * @return     Percentage drop chance of the given probability zone.
 */
uint8_t demo_polwred_ld_get_zprob_by_id(const fpp_qos_policer_wred_cmd_t* p_polwred, 
                                        uint8_t zprob_id)
{
    assert(p_polwred);
    
    uint8_t percentage = 255u;  /* default value */
    
    if (FPP_IQOS_WRED_ZONES_COUNT > zprob_id)
    {
        /* FCI command for Ingress QoS wred provides drop probability in compressed format */
        percentage = (uint8_t)((p_polwred->zprob[zprob_id] * 100u) / 0x0Fu);
    }
    
    return (percentage);
}
 
 
 
 
 /*
 * @brief      Query the name of a parent physical interface of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_polshp_ld_get_if_name(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (p_polshp->if_name);
}
 
 
/*
 * @brief      Query the ID of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     ID of Ingress QoS shaper.
 */
uint8_t demo_polshp_ld_get_id(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (p_polshp->id);
}
 
 
/*
 * @brief      Query the status of Ingress QoS shaper "enable" flag.
 * @param[in]  p_polshp  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the Ingress QoS wred:
 *             true  : was enabled
 *             false : was disabled
 */
bool demo_polshp_ld_is_enabled(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (p_polshp->enable);
}
 
 
/*
 * @brief      Query the type of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Type of Ingress QoS shaper.
 */
fpp_iqos_shp_type_t demo_polshp_ld_get_type(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (p_polshp->type);
}
 
 
/*
 * @brief      Query the mode of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Mode of Ingress QoS shaper.
 */
fpp_iqos_shp_rate_mode_t demo_polshp_ld_get_mode(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (p_polshp->mode);
}
 
 
/*
 * @brief      Query the idle slope of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Idle slope of Ingress QoS shaper.
 */
uint32_t demo_polshp_ld_get_isl(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return ntohl(p_polshp->isl);
}
 
 
/*
 * @brief      Query the maximal credit of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Maximal credit of Ingress QoS shaper.
 */
int32_t demo_polshp_ld_get_max_credit(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (int32_t)(ntohl(p_polshp->max_credit));
}
 
 
/*
 * @brief      Query the minimal credit of Ingress QoS shaper.
 * @details    [localdata_polshp]
 * @param[in]  p_polshp  Local data to be queried.
 * @return     Minimal credit of a QoS shaper.
 */
int32_t demo_polshp_ld_get_min_credit(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    assert(p_polshp);
    return (int32_t)(ntohl(p_polshp->min_credit));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available Ingress QoS wreds of 
 *             a given physical interface and execute a callback print function for 
 *             each Ingress QoS wred.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next Ingress QoS wred is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all available Ingress QoS wred of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 *                          No count was stored.
 */
int demo_polwred_print_by_phyif(FCI_CLIENT* p_cl, demo_polwred_cb_print_t p_cb_print, 
                                const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_wred_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_wred_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t wred_queue = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.queue = wred_queue;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_WRED,
                            sizeof(fpp_qos_policer_wred_cmd_t), 
                            (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            
            if (FPP_ERR_OK == rtn)
            {
                rtn = p_cb_print(&reply_from_fci);
            }
            
            wred_queue++;
        }
        
        /* query loop runs till there are no more Ingress QoS wreds to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_INTERNAL_FAILURE == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_polwred_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available Ingress QoS wreds in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of Ingress QoS wreds.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable Ingress QoS wreds.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_polwred_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_wred_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_wred_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t wred_queue = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.queue = wred_queue;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_WRED,
                            sizeof(fpp_qos_policer_wred_cmd_t), 
                            (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            
            wred_queue++;
        }
        
        /* query loop runs till there are no more Ingress QoS wreds to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_INTERNAL_FAILURE == rtn)
        {
            *p_rtn_count = wred_queue;
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_polwred_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available Ingress QoS shapers of 
 *             a given physical interface and execute a callback print function for 
 *             each Ingress QoS shaper.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next Ingress QoS shaper is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all available Ingress QoS shapers of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 *                          No count was stored.
 */
int demo_polshp_print_by_phyif(FCI_CLIENT* p_cl, demo_polshp_cb_print_t p_cb_print, 
                               const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_shp_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_shp_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t shp_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = shp_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_SHP,
                            sizeof(fpp_qos_policer_shp_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            
            if (FPP_ERR_OK == rtn)
            {
                rtn = p_cb_print(&reply_from_fci);
            }
            
            shp_id++;
        }
        
        /* query loop runs till there are no more Ingress QoS shapers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_INTERNAL_FAILURE == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_polshp_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available Ingress QoS shapers in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of Ingress QoS shapers.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable Ingress QoS shapers.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_polshp_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                   const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_shp_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_shp_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t shp_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = shp_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_SHP,
                            sizeof(fpp_qos_policer_shp_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            
            shp_id++;
        }
        
        /* query loop runs till there are no more Ingress QoS shapers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_INTERNAL_FAILURE == rtn)
        {
            *p_rtn_count = shp_id;
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_polshp_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available Ingress QoS flows of 
 *             a given physical interface and execute a callback print function for 
 *             each Ingress QoS flow.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next Ingress QoS flow is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all available Ingress QoS flows of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 *                          No count was stored.
 */
int demo_polflow_print_by_phyif(FCI_CLIENT* p_cl, demo_polflow_cb_print_t p_cb_print, 
                                const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_flow_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_flow_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                        sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        
        /* query loop */
        while (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&reply_from_fci);
            
            print_if_error(rtn, "demo_polflow_print_by_phyif() --> "
                                "non-zero return from callback print function!");
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                                sizeof(fpp_qos_policer_flow_cmd_t), 
                                (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
            }
        }
        
        /* query loop runs till there are no more Ingress QoS flows to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_POLICER_FLOW_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_polflow_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available Ingress QoS flows in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of Ingress QoS flows.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable Ingress QoS flows.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_polflow_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_policer_flow_cmd_t cmd_to_fci = {0};
    fpp_qos_policer_flow_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                        sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        
        /* query loop */
        while (FPP_ERR_OK == rtn)
        {
            count++;
            
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_QOS_POLICER_FLOW,
                            sizeof(fpp_qos_policer_flow_cmd_t),
                            (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_polflow_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
