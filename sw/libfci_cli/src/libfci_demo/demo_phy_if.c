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
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_phy_if.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief       Set/unset a flag in a physical interface struct.
 * @param[out]  p_rtn_phyif  Struct to be modified.
 * @param[in]   enable       New state of a flag.
 * @param[in]   flag         The flag.
 */
static void set_phyif_flag(fpp_phy_if_cmd_t* p_rtn_phyif, bool enable, fpp_if_flags_t flag)
{
    assert(NULL != p_rtn_phyif);
    
    hton_enum(&flag, sizeof(fpp_if_flags_t));
    
    if (enable)
    {
        p_rtn_phyif->flags |= flag;
    }
    else
    {
        p_rtn_phyif->flags &= (fpp_if_flags_t)(~flag);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested physical interface
 *              from PFE. Identify the interface by its name.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_phyif  Space for data from PFE.
 * @param[in]   p_name       Name of the requested physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : The requested physical interface was found.
 *                           A copy of its configuration data was stored into p_rtn_phyif.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_phy_if_get_by_name(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif, 
                            const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_phyif);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (0 != strcmp((reply_from_fci.name), p_name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_phyif = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_phy_if_get_by_name() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested physical interface
 *              from PFE. Identify the interface by its name.
 * @details     This is a standalone (_sa) function.
 *              It shows how to properly access a physical interface. Namely:
 *              1. Lock the interface database of PFE for exclusive access by this FCI client.
 *              2. Execute one or more FCI calls which access physical or logical interfaces.
 *              3. Unlock the exclusive access lock.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_phyif  Space for data from PFE.
 * @param[in]   p_name       Name of the requested physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : The requested physical interface was found.
 *                           A copy of its configuration data was stored into p_rtn_phyif.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
inline int demo_phy_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif,
                                      const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_phyif);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* lock the interface database of PFE for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    print_if_error(rtn, "demo_phy_if_get_by_name_sa() --> "
                        "fci_write(FPP_CMD_IF_LOCK_SESSION) failed!");
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_phy_if_get_by_name(p_cl, p_rtn_phyif, p_name);
    }
    
    /* unlock the exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));  
    
    print_if_error(rtn_unlock, "demo_phy_if_get_by_name_sa() --> "
                               "fci_write(FPP_CMD_IF_UNLOCK_SESSION) failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target physical interface
 *                 in PFE.
 * @details        To use this function properly, the interface database of PFE must be
 *                 locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *                 an example of a database lock procedure.
 * @param[in]      p_cl     FCI client
 * @param[in,out]  p_phyif  Local data struct which represents a new configuration of
 *                          the target physical interface.
 *                          It is assumed that the struct contains a valid data of some 
 *                          physical interface.
 * @return        FPP_ERR_OK : Configuration of the target physical interface was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct was not updated.
 */
int demo_phy_if_update(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_phy_if_cmd_t cmd_to_fci = (*p_phyif);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_PHY_IF, sizeof(fpp_phy_if_cmd_t), 
                                         (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_phy_if_get_by_name(p_cl, p_phyif, (p_phyif->name));
    }
    
    print_if_error(rtn, "demo_phy_if_update() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_phyif  [localdata_phyif]
 * @brief:      Functions marked as [localdata_phyif] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_phyif (a struct with configuration data).
 *              Initial data for p_phyif can be obtained via demo_phy_if_get_by_name().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_phy_if_update() to update
 *              the configuration of a real physical interface in PFE.
 */
 
 
/*
 * @brief          Enable ("up") a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 */
void demo_phy_if_ld_enable(fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, true, FPP_IF_ENABLED);
}
 
 
/*
 * @brief          Disable ("down") a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 */
void demo_phy_if_ld_disable(fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, false, FPP_IF_ENABLED);
}
 
 
/*
 * @brief          Set/unset a promiscuous mode of a physical interface.
 * @details        [localdata_phyif]
 *                 Promiscuous mode of a physical interface means the interface
 *                 will accept and process all incoming traffic, regardless of
 *                 the traffic's destination MAC.
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the promiscuous mode.
 */
void demo_phy_if_ld_set_promisc(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_PROMISC);
}
 
 
/*
 * @brief          Set/unset Fast Forwarding of all TCP traffic on a physical interface.
 * @details        [localdata_phyif]
 *                 When this flag is set, all TCP traffic is fast-forwarded. This is
 *                 default behavior.
 *                 When this flag is NOT set, TCP SYN|FIN|RST packets are not fast-forwarded.
 *                 Instead, they are passed to default logical interface (to host).
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the Fast Forwarding of all TCP traffic.
 */
void demo_phy_if_ld_set_ff_all_tcp(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_FF_ALL_TCP);
}
 
 
/*
 * @brief          Set/unset a VLAN conformance check on a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the VLAN conformance check.
 */
void demo_phy_if_ld_set_vlan_conf(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_VLAN_CONF_CHECK);
}
 
 
/*
 * @brief          Set/unset a PTP conformance check on a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the PTP conformance check.
 */
void demo_phy_if_ld_set_ptp_conf(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_PTP_CONF_CHECK);
}
 
 
/*
 * @brief          Set/unset a PTP promiscuous mode on a physical interface.
 * @details        [localdata_phyif]
 *                 This flag allows a PTP traffic to pass entry checks even if 
 *                 the strict VLAN conformance check is active.
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the PTP promiscuous mode.
 */
void demo_phy_if_ld_set_ptp_promisc(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_PTP_PROMISC);
}
 
 
/*
 * @brief          Set/unset acceptance of a Q-in-Q traffic on a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the Q-in-Q acceptance.
 */
void demo_phy_if_ld_set_qinq(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_ALLOW_Q_IN_Q);
}
 
 
/*
 * @brief          Set/unset discarding of packets which have TTL<2.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      enable   Request to set/unset discarding of packets which have TTL<2.
 */
void demo_phy_if_ld_set_discard_ttl(fpp_phy_if_cmd_t* p_phyif, bool enable)
{
    assert(NULL != p_phyif);
    set_phyif_flag(p_phyif, enable, FPP_IF_DISCARD_TTL);
}
 
 
/*
 * @brief          Set an operation mode of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      mode     New operation mode.
 *                          For details about physical interface operation modes,
 *                          see description of the fpp_phy_if_op_mode_t type in
 *                          FCI API Reference.
 */
void demo_phy_if_ld_set_mode(fpp_phy_if_cmd_t* p_phyif, fpp_phy_if_op_mode_t mode)
{
    assert(NULL != p_phyif);
    hton_enum(&mode, sizeof(fpp_phy_if_op_mode_t));
    p_phyif->mode = mode;
}
 
 
/*
 * @brief          Set a blocking state of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif      Local data to be modified.
 * @param[in]      block_state  New blocking state
 *                              For details about physical interface blocking states,
 *                              see description of the fpp_phy_if_block_state_t type in
 *                              FCI API Reference.
 */
void demo_phy_if_ld_set_block_state(fpp_phy_if_cmd_t* p_phyif,
                                    fpp_phy_if_block_state_t block_state)
{
    assert(NULL != p_phyif);
    hton_enum(&block_state, sizeof(fpp_phy_if_block_state_t));
    p_phyif->block_state = block_state;
}
 
 
/*
 * @brief          Set rx mirroring rule of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif        Local data to be modified.
 * @param[in]      idx            Index into the array of interface's rx mirroring rules.
 * @param[in]      p_mirror_name  Name of a mirroring rule.
 *                                Can be NULL. If NULL or "" (empty string), then
 *                                this mirroring rule slot is unused (disabled).
 */
void demo_phy_if_ld_set_rx_mirror(fpp_phy_if_cmd_t* p_phyif, uint8_t idx,
                                  const char* p_mirror_name)
{
    assert(NULL != p_phyif);
    /* 'p_mirror_name' is allowed to be NULL */
    
    if (FPP_MIRRORS_CNT > idx)
    {
        set_text(p_phyif->rx_mirrors[idx], p_mirror_name, MIRROR_NAME_SIZE);
    }
}
 
 
/*
 * @brief          Set tx mirroring rule of a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif        Local data to be modified.
 * @param[in]      idx            Index into the array of interface's tx mirroring rules.
 * @param[in]      p_mirror_name  Name of a mirroring rule.
 *                                Can be NULL. If NULL or "" (empty string), then
 *                                this mirroring rule slot is unused (disabled).
 */
void demo_phy_if_ld_set_tx_mirror(fpp_phy_if_cmd_t* p_phyif, uint8_t idx,
                                  const char* p_mirror_name)
{
    assert(NULL != p_phyif);
    /* 'p_mirror_name' is allowed to be NULL */
    
    if (FPP_MIRRORS_CNT > idx)
    {
        set_text(p_phyif->tx_mirrors[idx], p_mirror_name, MIRROR_NAME_SIZE);
    }
}
 
 
/*
 * @brief          Set FlexibleParser table to act as a FlexibleFilter for 
 *                 a physical interface.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif       Local data to be modified.
 * @param[in]      p_table_name  Name of a FlexibleParser table.
 *                               Can be NULL. If NULL or "" (empty string), then
 *                               FlexibleFilter of this physical interface is disabled.
 */
void demo_phy_if_ld_set_flexifilter(fpp_phy_if_cmd_t* p_phyif, const char* p_table_name)
{
    assert(NULL != p_phyif);
    /* 'p_table_name' is allowed to be NULL */
    
    set_text(p_phyif->ftable, p_table_name, IFNAMSIZ);
}

/*
 * @brief          Set physical interface which shall be used as an egress for PTP traffic.
 * @details        [localdata_phyif]
 * @param[in,out]  p_phyif  Local data to be modified.
 * @param[in]      p_name   Name of a physical interface.
 *                          Can be NULL. If NULL or "" (empty string), then
 *                          this feature is disabled and PTP traffic is processed the same
 *                          way as any other traffic.
 */
void demo_phy_if_ld_set_ptp_mgmt_if(fpp_phy_if_cmd_t* p_phyif, const char* p_name)
{
    assert(NULL != p_phyif);
    /* 'p_name' is allowed to be NULL */
    
    set_text(p_phyif->ptp_mgmt_if, p_name, IFNAMSIZ);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query the status of the "enable" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was enabled  ("up")
 *             false : was disabled ("down")
 */
bool demo_phy_if_ld_is_enabled(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_ENABLED);
}
 
 
/*
 * @brief      Query the status of the "enable" flag (inverted logic).
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was disabled ("down")
 *             false : was enabled  ("up)
 */
bool demo_phy_if_ld_is_disabled(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return !demo_phy_if_ld_is_enabled(p_phyif);
}
 
 
/*
 * @brief      Query the status of the "promiscuous mode" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was in a promiscuous mode
 *             false : was NOT in a promiscuous mode
 */
bool demo_phy_if_ld_is_promisc(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_PROMISC);
}
 
 
/*
 * @brief      Query the status of the "Fast Forwarding of all TCP traffic" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was fast-forwarding all TCP traffic
 *             false : was passing TCP SYN/FIN/RST packets to the default logical interface
 */
bool demo_phy_if_ld_is_ff_all_tcp(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);

    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));

    return (bool)(tmp_flags & FPP_IF_FF_ALL_TCP);
}
 
 
/*
 * @brief      Query the status of the "VLAN conformance check" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was checking VLAN conformance of an incoming traffic
 *             false : was NOT checking VLAN conformance of an incoming traffic
 */
bool demo_phy_if_ld_is_vlan_conf(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_VLAN_CONF_CHECK);
}
 
 
/*
 * @brief      Query the status of the "PTP conformance check" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was checking PTP conformance of an incoming traffic
 *             false : was NOT checking PTP conformance of an incoming traffic
 */
bool demo_phy_if_ld_is_ptp_conf(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_PTP_CONF_CHECK);
}
 
 
/*
 * @brief      Query the status of the "PTP promisc" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was using PTP promiscuous mode
 *             false : was NOT using PTP promiscuous mode
 */
bool demo_phy_if_ld_is_ptp_promisc(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_PTP_PROMISC);
}
 
 
/*
 * @brief      Query the status of the "Q-in-Q" flag.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was accepting Q-in-Q traffic
 *             false : was NOT accepting Q-in-Q traffic
 */
bool demo_phy_if_ld_is_qinq(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_ALLOW_Q_IN_Q);
}
 
 
/*
 * @brief      Query the status of the "discard if TTL<2" flag.
 * @details    [localdata_phyif]
 *             This feature applies only if the physical interface is in a mode
 *             which decrements TTL of packets (e.g. L3 Router).
 * @param[in]  p_phyif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the physical interface:
 *             true  : was discarding packets which have TTL<2 (only for some modes)
 *             false : was sending packets which have TTL<2 to a host (only for some modes)
 */
bool demo_phy_if_ld_is_discard_ttl(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_DISCARD_TTL);
}
 
 
/*
 * @brief      Query the name of a physical interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Name of the physical interface.
 */
const char* demo_phy_if_ld_get_name(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (p_phyif->name);
}
 
 
/*
 * @brief      Query the ID of a physical interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     ID of the physical interface.
 */
uint32_t demo_phy_if_ld_get_id(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return ntohl(p_phyif->id);
}
 
 
/*
 * @brief      Query the flags of a physical interface (the whole bitset).
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Flags bitset at time when the data was obtained from PFE.
 */
fpp_if_flags_t demo_phy_if_ld_get_flags(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_if_flags_t tmp_flags = (p_phyif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (tmp_flags);
}
 
 
/*
 * @brief      Query the operation mode of a physical interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Operation mode of the physical interface at time when 
 *             the data was obtained from PFE.
 */
fpp_phy_if_op_mode_t demo_phy_if_ld_get_mode(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_phy_if_op_mode_t tmp_mode = (p_phyif->mode);
    ntoh_enum(&tmp_mode, sizeof(fpp_phy_if_op_mode_t));
    
    return (tmp_mode);
}
 
 
/*
 * @brief      Query the blocking state of a physical interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Blocking state of the physical interface at time when 
 *             the data was obtained from PFE.
 */
fpp_phy_if_block_state_t demo_phy_if_ld_get_block_state(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    
    fpp_phy_if_block_state_t tmp_block_state = (p_phyif->block_state);
    ntoh_enum(&tmp_block_state, sizeof(fpp_phy_if_op_mode_t));
    
    return (tmp_block_state);
}
 
 
/*
 * @brief      Query the name of rx mirroring rule.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @param[in]  idx      Index into the array of interface's tx mirroring rules.
 * @return     Name of the mirroring rule which was assigned to the given slot at time when 
 *             the data was obtained from PFE.
 */
const char* demo_phy_if_ld_get_rx_mirror(const fpp_phy_if_cmd_t* p_phyif, uint8_t idx)
{
    assert(NULL != p_phyif);
    return ((FPP_MIRRORS_CNT > idx) ? (p_phyif->rx_mirrors[idx]) : (""));
}
 
 
/*
 * @brief      Query the name of tx mirroring rule.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @param[in]  idx      Index into the array of interface's tx mirroring rules.
 * @return     Name of the mirroring rule which was assigned to the given slot at time when 
 *             the data was obtained from PFE.
 */
const char* demo_phy_if_ld_get_tx_mirror(const fpp_phy_if_cmd_t* p_phyif, uint8_t idx)
{
    assert(NULL != p_phyif);
    return ((FPP_MIRRORS_CNT > idx) ? (p_phyif->tx_mirrors[idx]) : (""));
}
 
 
/*
 * @brief      Query the name of a FlexibleParser table which is being used as
 *             a FlexibleFilter for a physical interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Name of the FlexibleParser table which was being used as a FlexibleFilter
 *             of the physical interface at time when the data was obtained from PFE.
 */
const char* demo_phy_if_ld_get_flexifilter(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (p_phyif->ftable);
}
 
 
/*
 * @brief      Query the physical interface which is being used as a PTP management interface.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Name of the physical interface which is being used as the PTP management 
 *             interface.
 */
const char* demo_phy_if_ld_get_ptp_mgmt_if(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return (p_phyif->ptp_mgmt_if);
}
 
 
/*
 * @brief      Query the statistics of a physical interface - ingress.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Count of ingress packets at the time when the data was obtained form PFE.
 */
uint32_t demo_phy_if_ld_get_stt_ingress(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return ntohl(p_phyif->stats.ingress);
}
 
 
/*
 * @brief      Query the statistics of a physical interface - egress.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Count of egressed packets at the time when the data was obtained form PFE.
 */
uint32_t demo_phy_if_ld_get_stt_egress(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return ntohl(p_phyif->stats.egress);
}
 
 
/*
 * @brief      Query the statistics of a physical interface - malformed.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Count of malformed packets at the time when the data was obtained form PFE.
 */
uint32_t demo_phy_if_ld_get_stt_malformed(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return ntohl(p_phyif->stats.malformed);
}
 
 
/*
 * @brief      Query the statistics of a physical interface - discarded.
 * @details    [localdata_phyif]
 * @param[in]  p_phyif  Local data to be queried.
 * @return     Count of discarded packets at the time when the data was obtained form PFE.
 */
uint32_t demo_phy_if_ld_get_stt_discarded(const fpp_phy_if_cmd_t* p_phyif)
{
    assert(NULL != p_phyif);
    return ntohl(p_phyif->stats.discarded);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available physical interfaces in PFE and
 *             execute a callback print function for each reported physical interface.
 * @details    To use this function properly, the interface database of PFE must be
 *             locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *             an example of a database lock procedure.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next physical interface is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available physical interfaces.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_phy_if_print_all(FCI_CLIENT* p_cl, demo_phy_if_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                    sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        print_if_error(rtn, "demo_phy_if_print_all() --> "
                            "non-zero return from callback print function!");
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                            sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more physical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_phy_if_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available physical interfaces in PFE.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of physical interfaces.
 * @return      FPP_ERR_OK : Successfully counted all available physical interfaces.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_phy_if_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_phy_if_cmd_t cmd_to_fci = {0};
    fpp_phy_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                    sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_PHY_IF,
                        sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more physical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_phy_if_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
