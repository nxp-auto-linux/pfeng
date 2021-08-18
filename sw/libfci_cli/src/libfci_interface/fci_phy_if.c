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
#include "fci_phy_if.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a physical interface struct.
 * @param[in,out]  p_rtn_phyif  The physical interface struct to be converted.
 */
static void ntoh_phyif(fpp_phy_if_cmd_t* p_rtn_phyif)
{
    assert(NULL != p_rtn_phyif);
    
    
    p_rtn_phyif->id = ntohl(p_rtn_phyif->id);
    ntoh_enum(&(p_rtn_phyif->flags), sizeof(fpp_if_flags_t));
    ntoh_enum(&(p_rtn_phyif->mode), sizeof(fpp_phy_if_op_mode_t));
    ntoh_enum(&(p_rtn_phyif->block_state), sizeof(fpp_phy_if_block_state_t));
    p_rtn_phyif->stats.ingress = ntohl(p_rtn_phyif->stats.ingress);
    p_rtn_phyif->stats.egress = ntohl(p_rtn_phyif->stats.egress);
    p_rtn_phyif->stats.malformed = ntohl(p_rtn_phyif->stats.malformed);
    p_rtn_phyif->stats.discarded = ntohl(p_rtn_phyif->stats.discarded);
}
 
 
/*
 * @brief          Host-to-network (hton) function for a physical interface struct.
 * @param[in,out]  p_rtn_phyif  The physical interface struct to be converted.
 */
static void hton_phyif(fpp_phy_if_cmd_t* p_rtn_phyif)
{
    assert(NULL != p_rtn_phyif);
    
    
    p_rtn_phyif->id = htonl(p_rtn_phyif->id);
    hton_enum(&(p_rtn_phyif->flags), sizeof(fpp_if_flags_t));
    hton_enum(&(p_rtn_phyif->mode), sizeof(fpp_phy_if_op_mode_t));
    hton_enum(&(p_rtn_phyif->block_state), sizeof(fpp_phy_if_block_state_t));
    p_rtn_phyif->stats.ingress = htonl(p_rtn_phyif->stats.ingress);
    p_rtn_phyif->stats.egress = htonl(p_rtn_phyif->stats.egress);
    p_rtn_phyif->stats.malformed = htonl(p_rtn_phyif->stats.malformed);
    p_rtn_phyif->stats.discarded = htonl(p_rtn_phyif->stats.discarded);
}
 
 
/*
 * @brief       Set/unset a bitflag in a physical interface struct.
 * @param[out]  p_rtn_phyif  The physical interface struct to be modified.
 * @param[in]   enable       New state of the bitflag.
 * @param[in]   flag         The bitflag.
 */
static void set_flag(fpp_phy_if_cmd_t* p_rtn_phyif, bool enable, fpp_if_flags_t flag)
{
    assert(NULL != p_rtn_phyif);
    
    
    if (enable)
    {
        p_rtn_phyif->flags |= flag;
    }
    else
    {
        p_rtn_phyif->flags &= (fpp_if_flags_t)(~flag);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested physical interface
 *              from the PFE. Identify the interface by its name.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_phy_if_get_by_name_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_phyif  Space for data from the PFE.
 * @param[in]   p_name       Name of the requested physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Requested physical interface was found.
 *                           A copy of its configuration was stored into p_rtn_phyif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_phy_if_get_by_name(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif,
                           const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_phyif);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_phyif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (strcmp(p_name, reply_from_fci.name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_phyif(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_phyif = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested physical interface
 *              from the PFE. Identify the interface by its name.
 * @details     This is a standalone (_sa) function.
 *              It shows how to properly access a physical interface. Namely:
 *              1. Lock the interface database for exclusive access by this FCI client.
 *              2. Execute one or more FCI calls which access 
 *                 physical or logical interfaces.
 *              3. Unlock the interface database's exclusive access lock.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_phyif  Space for data from the PFE.
 * @param[in]   p_name       Name of the requested physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Requested physical interface was found.
 *                           A copy of its configuration was stored into p_rtn_phyif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
inline int fci_phy_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif,
                                     const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_phyif);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_get_by_name(p_cl, p_rtn_phyif, p_name);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));  
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested physical interface
 *              from the PFE. Identify the interface by its ID.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_phy_if_get_by_name_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_phyif  Space for data from the PFE.
 * @param[in]   p_id         ID of the requested physical interface.
 *                           IDs of physical interfaces are hardcoded.
 *                           See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Requested physical interface was found. 
 *                           A copy of its configuration was stored into p_rtn_phyif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_phy_if_get_by_id(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif, uint32_t id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_phyif);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_phyif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (id != (reply_from_fci.id)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_phyif(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_phyif = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target physical interface 
 *                 in the PFE.
 * @details        To use this function properly, the PFE interface database must be
 *                 locked for exclusive access. See fci_phy_if_update_sa() for 
 *                 an example how to lock the database.
 * @param[in]      p_cl     FCI client instance
 * @param[in,out]  p_phyif  Data struct which represents a new configuration of
 *                          the target physical interface.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or
 *                          fci_phy_if_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the target physical interface was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_phy_if_update(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_phy_if_cmd_t cmd_to_fci = (*p_phyif);
    
    /* send data */
    hton_phyif(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_PHY_IF, sizeof(fpp_phy_if_cmd_t), 
                                         (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_get_by_id(p_cl, p_phyif, (p_phyif->id));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target physical interface 
 *                 in the PFE.
 * @details        This is a standalone (_sa) function.
 *                 It shows how to properly access a physical interface. Namely:
 *                 1. Lock the interface database for exclusive access by this FCI client.
 *                 2. Execute one or more FCI calls which access 
 *                    physical or logical interfaces.
 *                 3. Unlock the interface database's exclusive access lock.
 * @param[in]      p_cl     FCI client instance
 * @param[in,out]  p_phyif  Data struct which represents a new configuration of
 *                          the target physical interface.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                          fci_phy_if_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the target physical interface was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
inline int fci_phy_if_update_sa(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_update(p_cl, p_phyif);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_phyif  [localdata_phyif]
 * @brief:      Functions marked as [localdata_phyif] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_phy_if_update() or fci_phy_if_update_sa() to
 *              update configuration of the real physical interface in the PFE.
 */
 
 
/*
 * @brief          Enable ("up") a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or
 *                          fci_phy_if_get_by_id().
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_enable(fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    p_phyif->flags |= FPP_IF_ENABLED;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Disable ("down") a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or
 *                          fci_phy_if_get_by_id().
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_disable(fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    p_phyif->flags &= (fpp_if_flags_t)(~FPP_IF_ENABLED);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset promiscuous mode of a physical interface.
 * @details        [localdata_phyif]
 *                 Promiscuous mode of a physical interface means the interface
 *                 will accept and process all incoming traffic, regardless of
 *                 the traffic's destination MAC.
 * @param[in,out]  p_phyif  Local data to be modified.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                          fci_phy_if_get_by_id().
 * @param[in]      promisc  A request to set/unset the promiscuous mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_promisc(fpp_phy_if_cmd_t* p_phyif, bool promisc)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, promisc, FPP_IF_PROMISC);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset this physical interface as a part of a loadbalancing bucket.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 *                              Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                              fci_phy_if_get_by_id().
 * @param[in]      loadbalance  A request to add/remove this interface to/from
 *                              a loadbalancing bucket.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_loadbalance(fpp_phy_if_cmd_t* p_phyif, bool loadbalance)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, loadbalance, FPP_IF_LOADBALANCE);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset VLAN conformance check in a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif    Local data to be modified.
 *                            Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                            fci_phy_if_get_by_id().
 * @param[in]      vlan_conf  A request to set/unset the VLAN conformance check.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_vlan_conf(fpp_phy_if_cmd_t* p_phyif, bool vlan_conf)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, vlan_conf, FPP_IF_VLAN_CONF_CHECK);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset PTP conformance check in a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif   Local data to be modified.
 *                           Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                           fci_phy_if_get_by_id().
 * @param[in]      ptp_conf  A request to set/unset the PTP conformance check.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_ptp_conf(fpp_phy_if_cmd_t* p_phyif, bool ptp_conf)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, ptp_conf, FPP_IF_PTP_CONF_CHECK);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset PTP promiscuous mode in a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 *                              Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                              fci_phy_if_get_by_id().
 * @param[in]      ptp_promisc  A request to set/unset the PTP promiscuous mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_ptp_promisc(fpp_phy_if_cmd_t* p_phyif, bool ptp_promisc)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, ptp_promisc, FPP_IF_PTP_PROMISC);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset Q-in-Q mode in a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 *                              Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                              fci_phy_if_get_by_id().
 * @param[in]      qinq         A request to set/unset the Q-in-Q mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_qinq(fpp_phy_if_cmd_t* p_phyif, bool qinq)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, qinq, FPP_IF_ALLOW_Q_IN_Q);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset discarding of packets which have TTL<2.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 *                              Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                              fci_phy_if_get_by_id().
 * @param[in]      discard_ttl  A request to set/unset the Q-in-Q mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_discard_ttl(fpp_phy_if_cmd_t* p_phyif, bool discard_ttl)
{
    assert(NULL != p_phyif);
    set_flag(p_phyif, discard_ttl, FPP_IF_DISCARD_TTL);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set operation mode of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 *                          Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                          fci_phy_if_get_by_id().
 * @param[in]      mode     New operation mode
 *                          For details about physical interface operation modes, see
 *                          the description of a fpp_phy_if_op_mode_t type in
 *                          the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_mode(fpp_phy_if_cmd_t* p_phyif, fpp_phy_if_op_mode_t mode)
{
    assert(NULL != p_phyif);
    p_phyif->mode = mode;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set blocking state of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 *                              Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                              fci_phy_if_get_by_id().
 * @param[in]      block_state  New blocking state
 *                              For details about physical interface blocking states, see
 *                              description of a fpp_phy_if_block_state_t type in
 *                              the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_block_state(fpp_phy_if_cmd_t* p_phyif,
                                  fpp_phy_if_block_state_t block_state)
{
    assert(NULL != p_phyif);
    p_phyif->block_state = block_state;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set traffic mirroring from this physical interface to 
 *                 another physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif        Local data to be modified.
 *                                Initial data can be obtained via fci_phy_if_get_by_name() or
 *                                fci_phy_if_get_by_id().
 * @param[in]      p_mirror_name  Name of a physical interface which shall be receiving
 *                                a copy of traffic.
 *                                Names of physical interfaces are hardcoded.
 *                                See the FCI API Reference, chapter Interface Management.
 *                                Can be NULL. If NULL or "" (empty string), then
 *                                traffic mirorring is disabled.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_mirror(fpp_phy_if_cmd_t* p_phyif, const char* p_mirror_name)
{
    assert(NULL != p_phyif);
    /* 'p_mirror_name' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    rtn = set_text(p_phyif->mirror, p_mirror_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        const bool enable_mirroring = ((NULL != p_mirror_name) && ('\0' != p_mirror_name[0]));
        set_flag(p_phyif, enable_mirroring, FPP_IF_MIRROR);
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Set FlexibleParser table to act as a FlexibleFilter for 
 *                 this physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif        Local data to be modified.
 *                                Initial data can be obtained via fci_phy_if_get_by_name() or
 *                                fci_phy_if_get_by_id().
 * @param[in]      p_table_name   Name of a FlexibleParser table.
 *                                Can be NULL. If NULL or "" (empty string), then
 *                                FlexibleFilter of this physical interface is disabled.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_phy_if_ld_set_flexifilter(fpp_phy_if_cmd_t* p_phyif, const char* p_table_name)
{
    assert(NULL != p_phyif);
    /* 'p_table_name' is allowed to be NULL */
    
    int rtn = FPP_ERR_FCI;
    rtn = set_text(p_phyif->ftable, p_table_name, IFNAMSIZ);
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query status of an "enable" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or 
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was enabled ("up")
 *             false : was disabled ("down")
 */
bool fci_phy_if_ld_is_enabled(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_ENABLED & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of an "enable" flag (inverted logic).
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was disabled ("down")
 *             false : was enabled ("up")
 */
bool fci_phy_if_ld_is_disabled(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return !fci_phy_if_ld_is_enabled(p_phyif);
}
 
 
/*
 * @brief      Query status of a "promiscuous mode" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was in a promiscuous mode
 *             false : was NOT in a promiscuous mode
 */
bool fci_phy_if_ld_is_promisc(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_PROMISC & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "loadbalance" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was a part of a loadbalance bucket
 *             false : was NOT a part of a loadbalance bucket
 */
bool fci_phy_if_ld_is_loadbalance(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_LOADBALANCE & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "VLAN conformance check" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was checking VLAN conformance
 *             false : was NOT checking VLAN conformance
 */
bool fci_phy_if_ld_is_vlan_conf(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_VLAN_CONF_CHECK & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "PTP conformance check" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was checking PTP conformance
 *             false : was NOT checking PTP conformance
 */
bool fci_phy_if_ld_is_ptp_conf(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_PTP_CONF_CHECK & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "PTP promisc" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was using PTP promiscuous mode
 *             false : was NOT using PTP promiscuous mode
 */
bool fci_phy_if_ld_is_ptp_promisc(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_PTP_PROMISC & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "Q-in-Q" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was using Q-in-Q feature
 *             false : was NOT using Q-in-Q feature
 */
bool fci_phy_if_ld_is_qinq(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_ALLOW_Q_IN_Q & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "Q-in-Q" flag.
 * @details    [localdata_phyif]
 *             This feature applies only if the physical interface is in a mode
 *             which decrements TTL of packets (e.g. L3 Router).
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : was discarding packets which have TTL<2 (only for some modes)
 *             false : was sending packets which have TTL<2 to a host (only for some modes)
 */
bool fci_phy_if_ld_is_discard_ttl(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_DISCARD_TTL & (p_phyif->flags));
}
 
 
/*
 * @brief      Query status of a "mirror" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 *                      Initial data can be obtained via fci_phy_if_get_by_name() or
 *                      fci_phy_if_get_by_id().
 * @return     At time when the data was obtained, the physical interface:
 *             true  : had the mirroring feature enabled
 *             false : had the mirroring feature disabled
 */
bool fci_phy_if_ld_is_mirror(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (bool)(FPP_IF_MIRROR & (p_phyif->flags));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all physical interfaces in the PFE and
 *             execute a callback print function for each reported physical interface.
 * @details    To use this function properly, the PFE interface database must be
 *             locked for exclusive access. See fci_phy_if_print_all_sa() for 
 *             an example how to lock the database.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next physical interface is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all physical interfaces.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_phy_if_print_all(FCI_CLIENT* p_cl, fci_phy_if_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                    sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_phyif(&reply_from_fci);  /* set correct byte order */
        
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                            sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_phyif(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more physical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all physical interfaces in the PFE and
 *             execute a callback print function for each reported physical interface.
 * @details    This is a standalone (_sa) function.
 *             It shows how to properly access a physical interface. Namely:
 *             1. Lock the interface database for exclusive access by this FCI client.
 *             2. Execute one or more FCI calls which access 
 *                physical or logical interfaces.
 *             3. Unlock the interface database's exclusive access lock.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next physical interface is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all physical interfaces.
 *             other      : Some error occured (represented by the respective error code).
 */
inline int fci_phy_if_print_all_sa(FCI_CLIENT* p_cl, fci_phy_if_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_print_all(p_cl, p_cb_print);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all physical interfaces in the PFE.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_phy_if_print_all_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of physical interfaces.
 * @return      FPP_ERR_OK : Successfully counted physical interfaces.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_phy_if_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                    sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting interfaces) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting interfaces) */
    }
    
    /* query loop runs till there are no more physical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
