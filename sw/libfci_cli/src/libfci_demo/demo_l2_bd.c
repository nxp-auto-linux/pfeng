/* =========================================================================
 *  Copyright 2020-2022 NXP
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
#include "demo_l2_bd.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested bridge domain
 *              from PFE. Identify the domain by its VLAN ID.
 * @param[in]   p_cl      FCI client
 * @param[out]  p_rtn_bd  Space for data from PFE.
 * @param[in]   vlan      VLAN ID of the requested bridge domain.
 * @return      FPP_ERR_OK : The requested bridge domain was found.
 *                           A copy of its configuration data was stored into p_rtn_bd.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_l2_bd_get_by_vlan(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_bd);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (ntohs(reply_from_fci.vlan) != vlan))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                        sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_bd = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_l2_bd_get_by_vlan() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested static entry
 *              from PFE. Identify the entry by VLAN ID of the parent bridge domain and
 *              by MAC address of the entry.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_stent  Space for data from PFE.
 * @param[in]   vlan         VLAN ID of the parent bridge domain.
 * @param[in]   p_mac        MAC address of the requested static entry.
 * @return      FPP_ERR_OK : The requested static entry was found.
 *                           A copy of its configuration data was stored into p_rtn_stent.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_l2_stent_get_by_vlanmac(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent,
                                 uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_stent);
    assert(NULL != p_mac);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) &&
            !(
                (ntohs(reply_from_fci.vlan) == vlan) &&
                (0 == memcmp((reply_from_fci.mac), p_mac, 6))
             )
          )
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                        sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_stent = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_l2_stent_get_by_vlanmac() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target bridge domain
 *                 in PFE.
 * @param[in]      p_cl  FCI client
 * @param[in,out]  p_bd  Local data struct which represents a new configuration of 
 *                       the target bridge domain.
 *                       It is assumed that the struct contains a valid data of some 
 *                       bridge domain.
 * @return        FPP_ERR_OK : Configuration of the target bridge domain was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct was not updated.
 */
int demo_l2_bd_update(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_cl);
    assert(NULL != p_bd);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_bd_cmd_t cmd_to_fci = (*p_bd);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_get_by_vlan(p_cl, p_bd, ntohs(p_bd->vlan));
    }
    
    print_if_error(rtn, "demo_l2_bd_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target static entry
 *                 in PFE.
 * @param[in]      p_cl     FCI client
 * @param[in,out]  p_stent  Local data struct which represents a new configuration of 
 *                          the target static entry.
 *                          It is assumed that the struct contains a valid data of some 
 *                          static entry.
 * @return        FPP_ERR_OK : Configuration of the target static entry was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             Local data struct not updated.
 */
int demo_l2_stent_update(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_cl);
    assert(NULL != p_stent);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_static_ent_cmd_t cmd_to_fci = (*p_stent);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t), 
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_stent_get_by_vlanmac(p_cl, p_stent, 
                                           ntohs(p_stent->vlan), (p_stent->mac));
    }
    
    print_if_error(rtn, "demo_l2_stent_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief         Use FCI calls to flush static entries from MAC tables of 
 *                all bridge domains in PFE.
 * @param[in]     p_cl  FCI client
 * @return        FPP_ERR_OK : Static MAC table entries of all bridge domains were 
 *                             successfully flushed in PFE.
 *                other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_flush_static(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    int rtn = fci_write(p_cl, FPP_CMD_L2_FLUSH_STATIC, 0u, NULL);
    print_if_error(rtn, "demo_l2_flush_static() failed!");
    return (rtn);
}
 
 
/*
 * @brief         Use FCI calls to flush dynamically learned entries from MAC tables of
 *                all bridge domains in PFE.
 * @param[in]     p_cl  FCI client
 * @return        FPP_ERR_OK : Learned MAC table entries of all bridge domains were
 *                             successfully flushed in the PFE.
 *                other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_flush_learned(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    int rtn = fci_write(p_cl, FPP_CMD_L2_FLUSH_LEARNED, 0u, NULL);
    print_if_error(rtn, "demo_l2_flush_learned() failed!");
    return (rtn);
}
 
 
/*
 * @brief         Use FCI calls to flush all entries from MAC tables of 
 *                all bridge domains in PFE.
 * @param[in]     p_cl  FCI client
 * @return        FPP_ERR_OK : All MAC table entries of all bridge domains were
 *                             successfully flushed in the PFE.
 *                other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_flush_all(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    int rtn = fci_write(p_cl, FPP_CMD_L2_FLUSH_ALL, 0u, NULL);
    print_if_error(rtn, "demo_l2_flush_all() failed!");
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new bridge domain in PFE.
 * @param[in]   p_cl      FCI client
 * @param[out]  p_rtn_if  Space for data from PFE.
 *                        This will contain a copy of configuration data of 
 *                        the newly created bridge domain.
 *                        Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   vlan      VLAN ID of the new bridge domain.
 * @return      FPP_ERR_OK : New bridge domain was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_bd.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_l2_bd_add(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan)
{
    assert(NULL != p_cl);
    /* 'p_rtn_bd' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = htons(vlan);
    cmd_to_fci.ucast_hit  = 3u;  /* 3 == discard */
    cmd_to_fci.ucast_miss = 3u;  /* 3 == discard */
    cmd_to_fci.mcast_hit  = 3u;  /* 3 == discard */
    cmd_to_fci.mcast_miss = 3u;  /* 3 == discard */
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_bd))
    {
        rtn = demo_l2_bd_get_by_vlan(p_cl, p_rtn_bd, vlan);
    }
    
    print_if_error(rtn, "demo_l2_bd_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target bridge domain in PFE.
 * @param[in]  p_cl    FCI client
 * @param[in]  vlan    VLAN ID of the bridge domain to destroy.
 *                     NOTE: Bridge domains marked as "default" or "fallback" 
 *                           cannot be destroyed.
 * @return     FPP_ERR_OK : The bridge domain was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_bd_del(FCI_CLIENT* p_cl, uint16_t vlan)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = htons(vlan);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_l2_bd_del() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new static entry in PFE.
 *              The new entry is associated with a provided parent bridge domain.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_stent  Space for data from PFE.
 *                           This will contain a copy of configuration data of 
 *                           the newly created static entry.
 *                           Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   vlan         VLAN ID of the parent bridge domain.
 * @param[in]   p_mac        MAC address of the new static entry.
 * @return      FPP_ERR_OK : New static entry was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_stent.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_l2_stent_add(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent,
                      uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    /* 'p_rtn_stent' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = htons(vlan);
    memcpy(cmd_to_fci.mac, p_mac, 6);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_stent))
    {
        rtn = demo_l2_stent_get_by_vlanmac(p_cl, p_rtn_stent, vlan, p_mac);
    }
    
    print_if_error(rtn, "demo_l2_stent_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target static entry in PFE.
 * @param[in]  p_cl    FCI client
 * @param[in]  vlan    VLAN ID of the parent bridge domain.
 * @param[in]  p_mac   MAC address of the static entry to destroy.
 * @return     FPP_ERR_OK : The static entry was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_stent_del(FCI_CLIENT* p_cl, uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = htons(vlan);
    memcpy(cmd_to_fci.mac, p_mac, 6);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
                                                (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_l2_stent_del() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_bd  [localdata_bd]
 * @brief:      Functions marked as [localdata_bd] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_bd (a struct with configuration data).
 *              Initial data for p_bd can be obtained via demo_l2_bd_get_by_vlan().
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call demo_l2_bd_update() to update 
 *              the configuration of a real bridge domain in PFE.
 */
 
 
/*
 * @brief          Set action to be done if unicast packet's destination MAC is
 *                 found (hit) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd         Local data to be modified.
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions,
 *                              see a description of the ucast_hit in FCI API Reference.
 */
void demo_l2_bd_ld_set_ucast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t hit_action)
{
    assert(NULL != p_bd);
    p_bd->ucast_hit = hit_action;
}
 
 
/*
 * @brief          Set action to be done if unicast packet's destination MAC is NOT
 *                 found (miss) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd         Local data to be modified.
 * @param[in]      miss_action  New action.
 *                              For details about bridge domain hit/miss actions,
 *                              see a description of the ucast_hit in FCI API Reference.
 */
void demo_l2_bd_ld_set_ucast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t miss_action)
{
    assert(NULL != p_bd);
    p_bd->ucast_miss = miss_action;
}
 
 
/*
 * @brief          Set action to be done if multicast packet's destination MAC is 
 *                 found (hit) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd         Local data to be modified.
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions,
 *                              see a description of the ucast_hit in FCI API Reference.
 */
void demo_l2_bd_ld_set_mcast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t hit_action)
{
    assert(NULL != p_bd);
    p_bd->mcast_hit = hit_action;
}
 
 
/*
 * @brief          Set action to be done if multicast packet's destination MAC is NOT
 *                 found (miss) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd         Local data to be modified.
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions,
 *                              see a description of the ucast_hit in FCI API Reference.
 */
void demo_l2_bd_ld_set_mcast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t miss_action)
{
    assert(NULL != p_bd);
    p_bd->mcast_miss = miss_action;
}
 
 
/*
 * @brief          Insert a physical interface into a bridge domain.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd      Local data to be modified.
 * @param[in]      phyif_id  ID of the physical interface.
 *                           IDs of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @param[in]      vlan_tag  Request to add/keep a VLAN tag (true) or to remove 
 *                           the VLAN tag (false) of a traffic egressed through
 *                           the given physical interface.
 */
void demo_l2_bd_ld_insert_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id, bool vlan_tag)
{
    assert(NULL != p_bd);
    
    if (32uL > phyif_id)  /* a check to prevent an undefined behavior */
    {
        const uint32_t phyif_bitmask = (1uL << phyif_id);
        
        p_bd->if_list |= htonl(phyif_bitmask);
        
        if (vlan_tag)
        {
            /* VLAN TAG is desired == physical interface must NOT be on the untag list. */
            p_bd->untag_if_list &= htonl((uint32_t)(~phyif_bitmask));
        }
        else
        {
            /* VLAN TAG is NOT desired == physical interface must BE on the untag list. */
            p_bd->untag_if_list |= htonl(phyif_bitmask);
        }
    }
}
 
 
/*
 * @brief          Remove a physical interface from a bridge domain.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd      Local data to be modified.
 * @param[in]      phyif_id  ID of the physical interface.
 *                           IDs of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 */
void demo_l2_bd_ld_remove_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    
    if (32uL > phyif_id)  /* a check to prevent an undefined behavior */
    {
        const uint32_t phyif_bitmask = (1uL << phyif_id);
        p_bd->if_list &= htonl((uint32_t)(~phyif_bitmask));
    }
}
 
 
 
 
/*
 * @defgroup    localdata_stent  [localdata_stent]
 * @brief:      Functions marked as [localdata_stent] acess only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_stent (a struct with configuration data).
 *              Initial data for p_stent can be obtained via demo_l2_stent_get_by_vlanmac().
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call demo_l2_stent_update() to update 
 *              the configuration of a real static entry in PFE.
 */
 
 
/*
 * @brief          Set target physical interfaces (forwarding list) which 
 *                 shall receive a copy of the accepted traffic.
 * @details        [localdata_stent]
 *                 New forwarding list fully replaces the old one.
 * @param[in,out]  p_stent  Local data to be modified.
 * @param[in]      fwlist   Target physical interfaces (forwarding list). A bitset.
 *                          Each physical interface is represented by one bit.
 *                          Conversion between physical interface ID and a corresponding
 *                          fwlist bit is (1uL << "ID of a target physical interface").
 */
void demo_l2_stent_ld_set_fwlist(fpp_l2_static_ent_cmd_t* p_stent, uint32_t fwlist)
{
    assert(NULL != p_stent);
    p_stent->forward_list = htonl(fwlist);
}
 
 
/*
 * @brief          Set/unset 'local' flag of a static entry.
 * @details        [localdata_stent]
 *                 Related to L2L3 Bridge feature (see FCI API Reference).
 * @param[in,out]  p_stent  Local data to be modified.
 * @param[in]      set      Request to set/unset the flag.
 *                          See description of the fpp_l2_static_ent_cmd_t type 
 *                          in FCI API reference.
 */
void demo_l2_stent_ld_set_local(fpp_l2_static_ent_cmd_t* p_stent, bool set)
{
    assert(NULL != p_stent);
    p_stent->local = set;  /* NOTE: implicit cast from bool to uint8_t */
}
 
 
/*
 * @brief          Set/unset a flag for a frame discarding feature tied with a static entry.
 * @details        [localdata_stent]
 * @param[in,out]  p_stent  Local data to be modified.
 * @param[in]      set      Request to set/unset the flag.
 *                          See description of fpp_l2_static_ent_cmd_t type
 *                          in FCI API reference.
 */
void demo_l2_stent_ld_set_src_discard(fpp_l2_static_ent_cmd_t* p_stent, bool set)
{
    assert(NULL != p_stent);
    p_stent->src_discard = set;  /* NOTE: implicit cast from bool to uint8_t */
}
 
 
/*
 * @brief          Set/unset a flag for a frame discarding feature tied with a static entry.
 * @details        [localdata_stent]
 * @param[in,out]  p_stent  Local data to be modified.
 * @param[in]      set      Request to set/unset the flag.
 *                          See description of fpp_l2_static_ent_cmd_t type
 *                          in FCI API reference.
 */
void demo_l2_stent_ld_set_dst_discard(fpp_l2_static_ent_cmd_t* p_stent, bool set)
{
    assert(NULL != p_stent);
    p_stent->dst_discard = set;  /* NOTE: implicit cast from bool to uint8_t */
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query status of a "default" flag.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the bridge domain:
 *             true  : was set as a default domain.
 *             false : was NOT set as a default domain.
 */
bool demo_l2_bd_ld_is_default(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    
    fpp_l2_bd_flags_t tmp_flags = (p_bd->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_l2_bd_flags_t));
    
    return (bool)(tmp_flags & FPP_L2_BD_DEFAULT);
}
 
 
/*
 * @brief      Query status of a "fallback" flag.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the bridge domain:
 *             true  : was set as a fallback domain.
 *             false : was NOT set as a fallback domain.
 */
bool demo_l2_bd_ld_is_fallback(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    
    fpp_l2_bd_flags_t tmp_flags = (p_bd->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_l2_bd_flags_t));
    
    return (bool)(tmp_flags & FPP_L2_BD_FALLBACK);
}
 
 
/*
 * @brief      Query whether a physical interface is a member of a bridge domain.
 * @details    [localdata_bd]
 * @param[in]  p_bd      Local data to be queried.
 * @param[in]  phyif_id  ID of the physical interface.
 *                       IDs of physical interfaces are hardcoded.
 *                       See FCI API Reference, chapter Interface Management.
 * @return     At time when the data was obtained from PFE, the requested physical interface:
 *             true  : was a member of the given bridge domain.
 *             false : was NOT a member of the given bridge domain.
 */
bool demo_l2_bd_ld_has_phyif(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    
    bool rtn = false;
    
    if (32uL > phyif_id)
    {
        const uint32_t phyif_bitmask = (1uL << phyif_id);
        rtn = (bool)(ntohl(p_bd->if_list) & phyif_bitmask);
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Query whether traffic from a physical interface is tagged by a bridge domain.
 *             This function returns meaningful results only if 
 *             the target physical interface is a member of the bridge domain.
 *             See demo_l2_bd_ld_has_phyif().
 * @details    [localdata_bd]
 * @param[in]  p_bd      Local data to be queried.
 * @param[in]  phyif_id  ID of the physical interface.
 *                       IDs of physical interfaces are hardcoded.
 *                       See FCI API Reference, chapter Interface Management.
 * @return     At time when the data was obtained from PFE, traffic from 
 *             the requested physical interface:
 *             true  : was being VLAN tagged by the given bridge domain.
 *             false : was NOT being VLAN tagged by the given bridge domain.
 */
bool demo_l2_bd_ld_is_phyif_tagged(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    
    bool rtn = false;
    if (32uL > phyif_id)
    {
        /* untag_list uses inverted logic - if interface IS on the list, it is UNTAGGED */
        const uint32_t phyif_bitmask = (1uL << phyif_id);
        rtn = !(ntohl(p_bd->untag_if_list) & phyif_bitmask);
    }
    return (rtn);
}
 
 
 
 
/*
 * @brief      Query the VLAN ID of a bridge domain.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     VLAN ID of the bridge domain.
 */
uint16_t demo_l2_bd_ld_get_vlan(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohs(p_bd->vlan);
}
 
 
/*
 * @brief      Query the bridge action which is executed on unicast hit.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bridge action (see a description of the ucast_hit in FCI API Reference).
 */
uint8_t demo_l2_bd_ld_get_ucast_hit(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (p_bd->ucast_hit);
}
 
 
/*
 * @brief      Query the bridge action which is executed on unicast miss.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bridge action (see a description of the ucast_hit in FCI API Reference).
 */
uint8_t demo_l2_bd_ld_get_ucast_miss(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (p_bd->ucast_miss);
}
 
 
/*
 * @brief      Query the bridge action which is executed on multicast hit.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bridge action (see a description of the ucast_hit in FCI API Reference).
 */
uint8_t demo_l2_bd_ld_get_mcast_hit(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (p_bd->mcast_hit);
}
 
 
/*
 * @brief      Query the bridge action which is executed on multicast miss.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bridge action (see a description of the ucast_hit in FCI API Reference).
 */
uint8_t demo_l2_bd_ld_get_mcast_miss(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (p_bd->mcast_miss);
}
 
 
/*
 * @brief      Query the list of member physical interfaces of a bridge domain.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bitset with physical interfaces being represented as bits.
 */
uint32_t demo_l2_bd_ld_get_if_list(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->if_list);
}
 
 
/*
 * @brief      Query the untag list of a bridge domain.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Bitset with physical interfaces being represented as bits.
 */
uint32_t demo_l2_bd_ld_get_untag_if_list(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->untag_if_list);
}
 
 
/*
 * @brief      Query the flags of a bridge domain (the whole bitset).
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Flags bitset.
 */
fpp_l2_bd_flags_t demo_l2_bd_ld_get_flags(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    
    fpp_l2_bd_flags_t tmp_flags = (p_bd->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_l2_bd_flags_t));
    
    return (tmp_flags);
}


/*
 * @brief      Query the domain traffic statistics - ingress
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Count of ingress packets at the time when the data was obtained from PFE.
 */
uint32_t demo_l2_bd_ld_get_stt_ingress(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->stats.ingress);
}


/*
 * @brief      Query the domain traffic statistics - ingress in bytes
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Number of ingress bytes at the time when the data was obtained from PFE.
 */
uint32_t demo_l2_bd_ld_get_stt_ingress_bytes(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->stats.ingress_bytes);
}


/*
 * @brief      Query the domain traffic statistics - egress
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Count of egress packets at the time when the data was obtained from PFE.
 */
uint32_t demo_l2_bd_ld_get_stt_egress(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->stats.egress);
}


/*
 * @brief      Query the domain traffic statistics - egress in bytes
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 * @return     Number of egress bytes at the time when the data was obtained from PFE.
 */
uint32_t demo_l2_bd_ld_get_stt_egress_bytes(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return ntohl(p_bd->stats.egress_bytes);
}
 
 
 
 
/*
 * @brief      Query whether a physical interface is a member of 
 *             a static entry's forwarding list.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @param[in]  fwlist_bitflag  Queried physical interface. A bitflag.
 *                             Each physical interface is represented by one bit.
 *                             Conversion between physical interface ID and a corresponding
 *                             fwlist bit is (1uL << "ID of a target physical interface").
 *                             Hint: It is recommended to always query only a single bitflag.
 * @return     At time when the data was obtained from PFE, the static entry:
 *             true  : had at least one queried forward list bitflag set
 *             false : had none of the queried forward list bitflags set
 */
bool demo_l2_stent_ld_is_fwlist_phyifs(const fpp_l2_static_ent_cmd_t* p_stent,
                                       uint32_t fwlist_bitflag)
{
    assert(NULL != p_stent);
    return (bool)(ntohl(p_stent->forward_list) & fwlist_bitflag);
}
 
 
/*
 * @brief      Query status of the "local" flag of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the static entry:
 *             true  : was set as local.
 *             false : was NOT set as local.
 */
bool demo_l2_stent_ld_is_local(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return (bool)(p_stent->local);
}
 
 
/*
 * @brief      Query status of the "src_discard" flag of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the static entry:
 *             true  : was set to discard ETH frames with a matching source MAC.
 *             false : was NOT set to discard ETH frames with a matching source MAC.
 */
bool demo_l2_stent_ld_is_src_discard(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return (bool)(p_stent->src_discard);
}
 
 
/*
 * @brief      Query status of the "dst_discard" flag of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the static entry:
 *             true  : was set to discard ETH frames with a matching destination MAC.
 *             false : was NOT set to discard ETH frames with a matching destination MAC.
 */
bool demo_l2_stent_ld_is_dst_discard(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return (bool)(p_stent->dst_discard);
}
 
 
 
 
/*
 * @brief      Query the VLAN ID of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     VLAN ID of the static entry.
 */
uint16_t demo_l2_stent_ld_get_vlan(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return ntohs(p_stent->vlan);
}
 
 
/*
 * @brief      Query the MAC address of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     MAC address of the static entry.
 */
const uint8_t* demo_l2_stent_ld_get_mac(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return (p_stent->mac);
}
 
 
/*
 * @brief      Query the forwarding list (a bitset) of a static entry.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 * @return     Bitset with physical interfaces being represented as bits.
 */
uint32_t demo_l2_stent_ld_get_fwlist(const fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_stent);
    return ntohl(p_stent->forward_list);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available bridge domains in PFE and
 *             execute a callback print function for each bridge domain.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next bridge domain is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available bridge domains.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_bd_print_all(FCI_CLIENT* p_cl, demo_l2_bd_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                            sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more bridge domains to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_BD_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_l2_bd_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available bridge domains in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of bridge domains.
 * @return      FPP_ERR_OK : Successfully counted all available bridge domains.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No value copied.
 */
int demo_l2_bd_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                        sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more bridge domains to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_BD_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_l2_bd_get_count() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available static entries in PFE and
 *             execute a callback print function for each applicable static entry.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next static entry is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]  by_vlan     [optional parameter]
 *                          Request to print only those static entries
 *                          which are associated with a particular bridge domain.
 * @param[in]  vlan        [optional parameter]
 *                          VLAN ID of a bridge domain.
 *                          Applicable only if (true == by_vlan), otherwise ignored.
 * @return     FPP_ERR_OK : Successfully iterated through all available static entries.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_l2_stent_print_all(FCI_CLIENT* p_cl, demo_l2_stent_cb_print_t p_cb_print, 
                            bool by_vlan, uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /*  query loop  */
    while (FPP_ERR_OK == rtn)
    {
        if ((false == by_vlan) ||
            ((true == by_vlan) && (ntohs(reply_from_fci.vlan) == vlan)))
        {
            rtn = p_cb_print(&reply_from_fci);
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more static entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_l2_stent_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all applicable static entries in PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of static entries.
 * @param[in]   by_vlan     [optional parameter]
 *                           Request to count only those static entries
 *                           which are associated with a particular bridge domain.
 * @param[in]   vlan        [optional parameter]
 *                           VLAN ID of a bridge domain.
 *                           Applicable only if (true == by_vlan), otherwise ignored.
 * @return      FPP_ERR_OK : Successfully counted all applicable static entries.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No value copied.
 */
int demo_l2_stent_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                            bool by_vlan, uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if ((false == by_vlan) ||
            ((true == by_vlan) && (ntohs(reply_from_fci.vlan) == vlan)))
        {
            count++;
        }
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more static entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_l2_stent_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
