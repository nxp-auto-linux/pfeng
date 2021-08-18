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
#include "fci_l2_bd.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a bridge domain struct.
 * @param[in,out]  p_rtn_bd  The bridge domain struct to be converted.
 */
static void ntoh_bd(fpp_l2_bd_cmd_t* p_rtn_bd)
{
    assert(NULL != p_rtn_bd);
    
    
    p_rtn_bd->vlan = ntohs(p_rtn_bd->vlan);
    p_rtn_bd->if_list = ntohl(p_rtn_bd->if_list);
    p_rtn_bd->untag_if_list = ntohl(p_rtn_bd->untag_if_list);
    ntoh_enum(&(p_rtn_bd->flags), sizeof(fpp_l2_bd_flags_t));
}
 
 
/*
 * @brief          Host-to-network (hton) function for a bridge domain struct.
 * @param[in,out]  p_rtn_bd  The bridge domain struct to be converted.
 */
static void hton_bd(fpp_l2_bd_cmd_t* p_rtn_bd)
{
    assert(NULL != p_rtn_bd);
    
    
    p_rtn_bd->vlan = htons(p_rtn_bd->vlan);
    p_rtn_bd->if_list = htonl(p_rtn_bd->if_list);
    p_rtn_bd->untag_if_list = htonl(p_rtn_bd->untag_if_list);
    hton_enum(&(p_rtn_bd->flags), sizeof(fpp_l2_bd_flags_t));
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for a static entry struct.
 * @param[in,out]  p_rtn_stent  The static entry struct to be converted.
 */
static void ntoh_stent(fpp_l2_static_ent_cmd_t* p_rtn_stent)
{
    assert(NULL != p_rtn_stent);
    
    
    p_rtn_stent->vlan = ntohs(p_rtn_stent->vlan);
    p_rtn_stent->forward_list = ntohl(p_rtn_stent->forward_list);
}
 
 
/*
 * @brief          Host-to-network (hton) function for a static entry struct.
 * @param[in,out]  p_rtn_bd  The static entry struct to be converted.
 */
static void hton_stent(fpp_l2_static_ent_cmd_t* p_rtn_stent)
{
    assert(NULL != p_rtn_stent);
    
    
    p_rtn_stent->vlan = htons(p_rtn_stent->vlan);
    p_rtn_stent->forward_list = htonl(p_rtn_stent->forward_list);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested bridge domain
 *              from the PFE. Identify the domain by its VLAN ID.
 * @param[in]   p_cl      FCI client instance
 * @param[out]  p_rtn_bd  Space for data from the PFE.
 * @param[in]   vlan      VLAN ID of the requested bridge domain.
 * @return      FPP_ERR_OK : Requested bridge domain was found.
 *                           A copy of its configuration was stored into p_rtn_bd.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_l2_bd_get_by_vlan(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_bd);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_bd(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (vlan != (reply_from_fci.vlan)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                        sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_bd(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_bd = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested static entry
 *              from the PFE. Identify the entry by VLAN ID of the parent bridge domain and
 *              by MAC address of the entry.
 * @param[in]   p_cl      FCI client instance
 * @param[out]  p_rtn_stent  Space for data from the PFE.
 * @param[in]   vlan         VLAN ID of the parent bridge domain.
 * @param[in]   p_mac        MAC address of the requested static entry.
 * @return      FPP_ERR_OK : Requested static entry was found.
 *                           A copy of its configuration was stored into p_rtn_stent.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_l2_stent_get_by_vlanmac(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent,
                                uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_stent);
    assert(NULL != p_mac);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_stent(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) &&
          !((vlan == reply_from_fci.vlan) && (0 == memcmp(p_mac, reply_from_fci.mac, 6))))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                        sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_stent(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_stent = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target bridge domain
 *                 in the PFE.
 * @param[in]      p_cl  FCI client instance
 * @param[in,out]  p_bd  Data struct which represents a new configuration of 
 *                       the target bridge domain.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @return         FPP_ERR_OK : Configuration of the target bridge domain was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_l2_bd_update(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_cl);
    assert(NULL != p_bd);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_bd_cmd_t cmd_to_fci = (*p_bd);
    
    /* send data */
    hton_bd(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_get_by_vlan(p_cl, p_bd, (p_bd->vlan));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target static entry
 *                 in the PFE.
 * @param[in]      p_cl  FCI client instance
 * @param[in,out]  p_stent  Data struct which represents a new configuration of 
 *                          the target static entry.
 *                          Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @return         FPP_ERR_OK : Configuration of the target static entry was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_l2_stent_update(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_stent)
{
    assert(NULL != p_cl);
    assert(NULL != p_stent);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_static_ent_cmd_t cmd_to_fci = (*p_stent);
    
    /* send data */
    hton_stent(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t), 
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_stent_get_by_vlanmac(p_cl, p_stent, (p_stent->vlan), (p_stent->mac));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to flush static entries from MAC tables of 
 *                 all bridge domains in the PFE.
 * @param[in]      p_cl  FCI client instance
 * @return         FPP_ERR_OK : Static entries of all bridge domains were 
 *                              successfully flushed in the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 */
int fci_l2_flush_static(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_write(p_cl, FPP_CMD_L2_FLUSH_STATIC, 0u, NULL);
}
 
 
/*
 * @brief          Use FCI calls to flush dynamically learned entries from MAC tables of
 *                 all bridge domains in the PFE.
 * @param[in]      p_cl  FCI client instance
 * @return         FPP_ERR_OK : Learned entries of all bridge domains were
 *                              successfully flushed in the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 */
int fci_l2_flush_learned(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_write(p_cl, FPP_CMD_L2_FLUSH_LEARNED, 0u, NULL);
}
 
 
/*
 * @brief          Use FCI calls to flush all entries from MAC tables of 
 *                 all bridge domains in the PFE.
 * @param[in]      p_cl  FCI client instance
 * @return         FPP_ERR_OK : All entries of all bridge domains were
 *                              successfully flushed in the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 */
int fci_l2_flush_all(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_write(p_cl, FPP_CMD_L2_FLUSH_ALL, 0u, NULL);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */
 
 
/*
 * @brief       Use FCI calls to create a new bridge domain in the PFE.
 * @param[in]   p_cl      FCI client instance
 * @param[out]  p_rtn_if  Space for data from the PFE.
 *                        Will contain a copy of configuration data of 
 *                        the newly created bridge domain.
 *                        Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   vlan      VLAN ID of the new bridge domain.
 * @return      FPP_ERR_OK : New bridge domain was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_bd.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_l2_bd_add(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan)
{
    assert(NULL != p_cl);
    /* 'p_rtn_bd' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = vlan;
    cmd_to_fci.ucast_hit  = 3u;  /* 3 == discard */
    cmd_to_fci.ucast_miss = 3u;  /* 3 == discard */
    cmd_to_fci.mcast_hit  = 3u;  /* 3 == discard */
    cmd_to_fci.mcast_miss = 3u;  /* 3 == discard */
    
    /* send data */
    hton_bd(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_bd))
    {
        rtn = fci_l2_bd_get_by_vlan(p_cl, p_rtn_bd, vlan);
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target bridge domain in the PFE.
 * @param[in]  p_cl    FCI client instance
 * @param[in]  vlan    VLAN ID of the bridge domain to destroy.
 * @return     FPP_ERR_OK : Bridge domain was destroyed.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_l2_bd_del(FCI_CLIENT* p_cl, uint16_t vlan)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = vlan;
    
    /* send data */
    hton_bd(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new static entry in the PFE.
 *              The new entry is associated with a provided parent bridge domain.
 * @param[in]   p_cl      FCI client instance
 * @param[out]  p_rtn_stent  Space for data from the PFE.
 *                           Will contain a copy of configuration data of 
 *                           the newly created static entry.
 *                           Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   vlan         VLAN ID of the parent bridge domain.
 * @param[in]   p_mac        MAC address of the new static entry.
 * @return      FPP_ERR_OK : New static entry was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_stent.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_l2_stent_add(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent,
                     uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    /* 'p_rtn_stent' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan  = vlan;
    memcpy(cmd_to_fci.mac, p_mac, 6);
    
    /* send data */
    hton_stent(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_stent))
    {
        rtn = fci_l2_stent_get_by_vlanmac(p_cl, p_rtn_stent, vlan, p_mac);
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target static entry in the PFE.
 * @param[in]  p_cl    FCI client instance
 * @param[in]  vlan    VLAN ID of the parent bridge domain.
 * @param[in]  p_mac   MAC address of the static entry to be destroyed.
 * @return     FPP_ERR_OK : Static entry was destroyed.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_l2_stent_del(FCI_CLIENT* p_cl, uint16_t vlan, const uint8_t p_mac[6])
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.vlan = vlan;
    memcpy(cmd_to_fci.mac, p_mac, 6);
    
    /* send data */
    hton_stent(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
                                                (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_bd  [localdata_bd]
 * @brief:      Functions marked as [localdata_bd] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_l2_bd_update() to
 *              update configuration of the real bridge domain in the PFE.
 */
 
 
/*
 * @brief          Set action to be done if unicast packet's destination MAC is
 *                 found (hit) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions, see
 *                              description of ucast_hit in the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_set_ucast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t hit_action)
{
    assert(NULL != p_bd);
    p_bd->ucast_hit = hit_action;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set action to be done if unicast packet's destination MAC is NOT
 *                 found (miss) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      miss_action  New action.
 *                              For details about bridge domain hit/miss actions, see
 *                              description of ucast_hit in the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_set_ucast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t miss_action)
{
    assert(NULL != p_bd);
    p_bd->ucast_miss = miss_action;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set action to be done if multicast packet's destination MAC is 
 *                 found (hit) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions, see
 *                              description of ucast_hit in the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_set_mcast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t hit_action)
{
    assert(NULL != p_bd);
    p_bd->mcast_hit = hit_action;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set action to be done if multicast packet's destination MAC is NOT
 *                 found (miss) in a bridge domain's MAC table.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      hit_action   New action.
 *                              For details about bridge domain hit/miss actions, see
 *                              description of ucast_hit in the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_set_mcast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t miss_action)
{
    assert(NULL != p_bd);
    p_bd->mcast_miss = miss_action;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Insert a physical interface into a bridge domain.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      phyif_id  Physical interface ID
 *                           IDs of physical interfaces are hardcoded.
 *                           See the FCI API Reference, chapter Interface Management.
 * @param[in]      add_vlan_tag  A request to tag (true) or untag (false) a traffic from 
 *                               the given physical interface.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_insert_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id, bool add_vlan_tag)
{
    assert(NULL != p_bd);
    
    
    int rtn = FPP_ERR_FCI;
    if (32uL > phyif_id)  /* a check to prevent undefined behavior */
    {
        const uint32_t phyif_bitmask = (1uL << phyif_id);
        p_bd->if_list |= phyif_bitmask;
        if (add_vlan_tag)  
        {
            /* VLAN TAG is desired == physical interface must NOT be on the untag list. */
            p_bd->untag_if_list &= ~phyif_bitmask;
        }
        else
        {
            /* VLAN TAG is NOT desired == physical interface must be on the untag list. */
            p_bd->untag_if_list |= phyif_bitmask;
        }
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Remove given physical interface from a bridge domain.
 * @details        [localdata_bd]
 * @param[in,out]  p_bd  Local data to be modified.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]      phyif_id  Physical interface ID
 *                           IDs of physical interfaces are hardcoded.
 *                           See the FCI API Reference, chapter Interface Management.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_bd_ld_remove_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    
    
    int rtn = FPP_ERR_FCI;
    if (32uL > phyif_id)  /* a check to prevent undefined behavior */
    {
        p_bd->if_list &= (uint32_t)(~(1uL << phyif_id));
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @defgroup    localdata_stent  [localdata_stent]
 * @brief:      Functions marked as [localdata_stent] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_l2_stent_update() to
 *              update configuration of the real static entry in the PFE.
 */
 
 
/*
 * @brief          Set target physical interfaces (forwarding list) which 
 *                 shall receive a copy of the accepted traffic.
 * @details        [localdata_stent]
 *                 New forwarding list fully replaces the old one.
 * @param[in,out]  p_stent  Local data to be modified.
 *                          Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @param[in]      fwlist   Target physical interfaces (forwarding list). A bitset.
 *                          Each physical interface is represented by one bit.
 *                          Conversion between physical interface ID and a corresponding
 *                          egress vector bit is (1u << "physical interface ID").
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_stent_ld_set_fwlist(fpp_l2_static_ent_cmd_t* p_stent, uint32_t fwlist)
{
    assert(NULL != p_stent);
    p_stent->forward_list = fwlist;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set flag 'local' in a static entry.
 * @details        [localdata_stent]
 * @param[in,out]  p_stent  Local data to be modified.
 *                          Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @param[in]      local    A request to set/unset the flag.
 *                          See description of fpp_l2_static_ent_cmd_t type in 
 *                          the FCI API reference.
 *                          Related topic: L2L3 Bridge mode of a physical interface.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_stent_ld_set_local(fpp_l2_static_ent_cmd_t* p_stent, bool local)
{
    assert(NULL != p_stent);
    p_stent->local = local;  /* NOTE: Implicit cast from bool to uintX_t */
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set flag 'discard on source MAC match' in a static entry.
 * @details        [localdata_stent]
 * @param[in,out]  p_stent  Local data to be modified.
 *                          Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @param[in]      src_discard  A request to set/unset the flag.
 *                              See description of fpp_l2_static_ent_cmd_t type in 
 *                              the FCI API reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_stent_ld_set_src_discard(fpp_l2_static_ent_cmd_t* p_stent, bool src_discard)
{
    assert(NULL != p_stent);
    p_stent->src_discard = src_discard;  /* NOTE: Implicit cast from bool to uintX_t */
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set flag 'discard on destination MAC match' in a static entry.
 * @details        [localdata_stent]
 * @param[in,out]  p_stent  Local data to be modified.
 *                          Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @param[in]      dst_discard  A request to set/unset the flag.
 *                              See description of fpp_l2_static_ent_cmd_t type in 
 *                              the FCI API reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_l2_stent_ld_set_dst_discard(fpp_l2_static_ent_cmd_t* p_stent, bool dst_discard)
{
    assert(NULL != p_stent);
    p_stent->dst_discard = dst_discard;  /* NOTE: Implicit cast from bool to uintX_t */
    return (FPP_ERR_OK);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query status of a "default" flag.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 *                   Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @return     At time when the data was obtained, the bridge domain:
 *             true  : was set as a default domain.
 *             false : was NOT set as a default domain.
 */
bool fci_l2_bd_ld_is_default(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (bool)(FPP_L2_BD_DEFAULT & (p_bd->flags));
}
 
 
/*
 * @brief      Query status of a "fallback" flag.
 * @details    [localdata_bd]
 * @param[in]  p_bd  Local data to be queried.
 *                   Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @return     At time when the data was obtained, the bridge domain:
 *             true  : was set as a fallback domain.
 *             false : was NOT set as a fallback domain.
 */
bool fci_l2_bd_ld_is_fallback(const fpp_l2_bd_cmd_t* p_bd)
{
    assert(NULL != p_bd);
    return (bool)(FPP_L2_BD_FALLBACK & (p_bd->flags));
}
 
 
/*
 * @brief      Query whether the given physical interface is a member of a bridge domain.
 * @details    [localdata_bd]
 * @param[in]  p_bd      Local data to be queried.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]  phyif_id  phyif_id  Physical interface ID
 *                       IDs of physical interfaces are hardcoded.
 *                       See the FCI API Reference, chapter Interface Management.
 * @return     At time when the data was obtained, the given physical interface:
 *             true  : was a member of the given bridge domain.
 *             false : was NOT a member of the given bridge domain.
 */
bool fci_l2_bd_ld_is_phyif(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    bool rtn = false;
    if (32uL > phyif_id)
    {
        rtn = (bool)((1uL << phyif_id) & (p_bd->if_list));
    }
    return (rtn);
}
 
 
/*
 * @brief      Query whether the requested physical interface is 
 *             tagged by the bridge domain (or not).
 * @details    [localdata_bd]
 * @param[in]  p_bd      Local data to be queried.
 *                       Initial data can be obtained via fci_l2_bd_get_by_vlan().
 * @param[in]  phyif_id  Physical interface ID
 *                       IDs of physical interfaces are hardcoded.
 *                       See the FCI API Reference, chapter Interface Management.
 * @return     At time when the data was obtained, the requested physical interface:
 *             true  : was being tagged by the given bridge domain.
 *             false : was NOT being tagged by the given bridge domain.
 */
bool fci_l2_bd_ld_is_tagged(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id)
{
    assert(NULL != p_bd);
    bool rtn = false;
    if (32uL > phyif_id)
    {
        rtn = (bool)(!((1uL << phyif_id) & (p_bd->untag_if_list)));
    }
    return (rtn);
}
 
 
/*
 * @brief      Query whether a physical interface is a member of 
 *             the static entry's forwarding list or not.
 * @details    [localdata_stent]
 * @param[in]  p_stent  Local data to be queried.
 *                      Initial data can be obtained via fci_l2_stent_get_by_vlanmac().
 * @param[in]  fwlist_bitflag  Queried physical interface. A bitflag.
 *                             Each physical interface is represented by one bit.
 *                             Conversion between physical interface ID and a corresponding
 *                             fwlist bit is (1u << "physical interface ID").
 *                             Hint: It is recommended to always query only a single bitflag.
 * @return     At time when the data was obtained, the logical interface:
 *             true  : had at least one queried forward list bitflag set
 *             false : had none of the queried forward list bitflags set
 */
bool fci_l2_stent_ld_is_fwlist_phyifs(const fpp_l2_static_ent_cmd_t* p_stent,
                                      uint32_t fwlist_bitflag)
{
    assert(NULL != p_stent);
    return (bool)(fwlist_bitflag & (p_stent->forward_list));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all bridge domains in the PFE and
 *             execute a callback print function for each reported bridge domain.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next bridge domain is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all bridge domains.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_l2_bd_print_all(FCI_CLIENT* p_cl, fci_l2_bd_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_bd(&reply_from_fci);  /* set correct byte order */
    
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
            ntoh_bd(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more bridge domains to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_BD_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available bridge domains in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of bridge domains.
 * @return      FPP_ERR_OK : Successfully counted bridge domains.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No value copied.
 */
int fci_l2_bd_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_bd_cmd_t cmd_to_fci = {0};
    fpp_l2_bd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                    sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting bridge domains) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_BD,
                        sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting bridge domains) */
    }
    
    /* query loop runs till there are no more bridge domains to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_BD_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all static entries in the PFE and
 *             execute a callback print function for each reported static entry.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next static entry is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all static entries.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_l2_stent_print_all(FCI_CLIENT* p_cl, fci_l2_stent_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_stent(&reply_from_fci);  /* set correct byte order */
    
    /*  query loop  */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_stent(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more sttic entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all static entries in the PFE which
 *             are children of a given bridge domain. Execute a print function
 *             for each reported static entry.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next static entry is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]  vlan        VLAN ID of the parent bridge domain.
 * @return     FPP_ERR_OK : Successfully iterated through all suitable static entries.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_l2_stent_print_by_vlan(FCI_CLIENT* p_cl, fci_l2_stent_cb_print_t p_cb_print, 
                               uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_stent(&reply_from_fci);  /* set correct byte order */
    
    /*  query loop  */
    while (FPP_ERR_OK == rtn)
    {
        if (vlan == (reply_from_fci.vlan))
        {
            rtn = p_cb_print(&reply_from_fci);
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_stent(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more sttic entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available static entries in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of static entries.
 * @return      FPP_ERR_OK : Successfully counted static entries.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No value copied.
 */
int fci_l2_stent_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting static entries) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting static entries) */
    }
    
    /* query loop runs till there are no more sttic entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available static entries in the PFE which
 *              are children of a given parent bridge domain.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of static entries.
 * @param[in]   vlan         VLAN ID of the parent bridge domain.
 * @return      FPP_ERR_OK : Successfully counted static entries.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No value copied.
 */
int fci_l2_stent_get_count_by_vlan(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, uint16_t vlan)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_l2_static_ent_cmd_t cmd_to_fci = {0};
    fpp_l2_static_ent_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting static entries) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if (vlan == ntohs(reply_from_fci.vlan))  /* NOTE: vlan needs correct byte order */
        {
            count++;
        }
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_L2_STATIC_ENT,
                    sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting static entries) */
    }
    
    /* query loop runs till there are no more sttic entries to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_L2_STATIC_EN_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
