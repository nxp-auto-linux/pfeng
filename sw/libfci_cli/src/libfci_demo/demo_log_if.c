/* =========================================================================
 *  Copyright 2020-2021,2024 NXP
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
#include "demo_log_if.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief       Set/unset a flag in a logical interface struct.
 * @param[out]  p_rtn_phyif  Struct to be modified.
 * @param[in]   enable       New state of a flag.
 * @param[in]   flag         The flag.
 */
static void set_logif_flag(fpp_log_if_cmd_t* p_rtn_logif, bool enable, fpp_if_flags_t flag)
{
    assert(NULL != p_rtn_logif);
    
    hton_enum(&flag, sizeof(fpp_if_flags_t));
    
    if (enable)
    {
        p_rtn_logif->flags |= flag;
    }
    else
    {
        p_rtn_logif->flags &= (fpp_if_flags_t)(~flag);
    }
}
 
 
/*
 * @brief       Set/unset a match rule flag in a logical interface struct.
 * @param[out]  p_rtn_logif  Struct to be modified.
 * @param[in]   enable       New state of a match rule flag.
 * @param[in]   match_rule   The match rule flag.
 */
static void set_logif_mr_flag(fpp_log_if_cmd_t* p_rtn_logif, bool enable,
                              fpp_if_m_rules_t match_rule)
{
    assert(NULL != p_rtn_logif);
    
    hton_enum(&match_rule, sizeof(fpp_if_m_rules_t));
    
    if (enable)
    {
        p_rtn_logif->match |= match_rule;
    }
    else
    {
        p_rtn_logif->match &= (fpp_if_m_rules_t)(~match_rule);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested logical interface
 *              from PFE. Identify the interface by its name.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_logif  Space for data from PFE.
 * @param[in]   p_name       Name of the requested logical interface.
 *                           Names of logical interfaces are user-defined.
 *                           See demo_log_if_add().
 * @return      FPP_ERR_OK : The requested logical interface was found.
 *                           A copy of its configuration data was stored into p_rtn_logif.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_log_if_get_by_name(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, 
                            const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_logif);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (0 != strcmp((reply_from_fci.name), p_name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_logif = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_log_if_get_by_name() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested logical interface
 *              from PFE. Identify the interface by its name.
 * @details     This is a standalone (_sa) function.
 *              It shows how to properly access a logical interface. Namely:
 *              1. Lock the interface database of PFE for exclusive access by this FCI client.
 *              2. Execute one or more FCI calls which access physical or logical interfaces.
 *              3. Unlock the exclusive access lock.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_logif  Space for data from PFE.
 * @param[in]   p_name       Name of the requested logical interface.
 *                           Names of logical interfaces are user-defined.
 *                           See demo_log_if_add().
 * @return      FPP_ERR_OK : The requested logical interface was found.
 *                           A copy of its configuration data was stored into p_rtn_logif.
 *                           REMINDER: data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
inline int demo_log_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, 
                                     const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_logif);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* lock the interface database of PFE for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    print_if_error(rtn, "demo_log_if_get_by_name_sa() --> "
                        "fci_write(FPP_CMD_IF_LOCK_SESSION) failed!");
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_get_by_name(p_cl, p_rtn_logif, p_name);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    print_if_error(rtn_unlock, "demo_log_if_get_by_name_sa() --> "
                               "fci_write(FPP_CMD_IF_UNLOCK_SESSION) failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target logical interface
 *                 in PFE.
 * @details        To use this function properly, the interface database of PFE must be
 *                 locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *                 an example of a database lock procedure.
 * @param[in]      p_cl     FCI client
 * @param[in,out]  p_phyif  Local data struct which represents a new configuration of
 *                          the target logical interface.
 *                          It is assumed that the struct contains a valid data of some 
 *                          logical interface.
 * @return        FPP_ERR_OK : Configuration of the target logical interface was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                              The local data struct was not updated.
 */
int demo_log_if_update(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_cl);
    assert(NULL != p_logif);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_log_if_cmd_t cmd_to_fci = (*p_logif);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), 
                                         (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_get_by_name(p_cl, p_logif, (p_logif->name));
    }
    
    print_if_error(rtn, "demo_log_if_update() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new logical interface in PFE.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_logif    Space for data from PFE.
 *                             This will contain a copy of configuration data of 
 *                             the newly created logical interface.
 *                             Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   p_name         Name of the new logical interface.
 *                             The name is user-defined.
 * @param[in]   p_parent_name  Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : New logical interface was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_logif.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_log_if_add(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, const char* p_name, 
                   const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    assert(NULL != p_parent_name);
    /* 'p_rtn_logif' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_log_if_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((cmd_to_fci.parent_name), p_parent_name, IFNAMSIZ);
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_logif))
    {
        rtn = demo_log_if_get_by_name(p_cl, p_rtn_logif, p_name);
    }
    
    print_if_error(rtn, "demo_log_if_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target logical interface in PFE.
 * @details    To use this function properly, the interface database of PFE must be
 *             locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *             an example of a database lock procedure.
 * @param[in]  p_cl    FCI client
 * @param[in]  p_name  Name of the logical interface to destroy.
 * @return     FPP_ERR_OK : The logical interface was destroyed.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_log_if_del(FCI_CLIENT* p_cl, const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_log_if_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_log_if_del() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_logif  [localdata_logif]
 * @brief:      Functions marked as [localdata_logif] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_logif (a struct with configuration data).
 *              Initial data for p_logif can be obtained via demo_log_if_get_by_name().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_log_if_update() to update
 *              the configuration of a real logical interface in PFE.
 */
 
 
/*
 * @brief          Enable ("up") a logical interface.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 */
void demo_log_if_ld_enable(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    set_logif_flag(p_logif, true, FPP_IF_ENABLED);
}
 
 
/*
 * @brief          Disable ("down") a logical interface.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 */
void demo_log_if_ld_disable(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    set_logif_flag(p_logif, false, FPP_IF_ENABLED);
}
 
 
/*
 * @brief          Set/unset a promiscuous mode of a logical interface.
 * @details        [localdata_logif]
 *                 Promiscuous mode of a logical interface means the interface
 *                 will accept all incoming traffic, regardless of active match rules.
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      enable   Request to set/unset the promiscuous mode.
 */
void demo_log_if_ld_set_promisc(fpp_log_if_cmd_t* p_logif, bool enable)
{
    assert(NULL != p_logif);
    set_logif_flag(p_logif, enable, FPP_IF_PROMISC);
}
 
 
/*
 * @brief          Set match mode (chaining mode of match rules).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      match_mode_is_or  Request to set match mode.
 *                                   For details about logical interface match modes,
 *                                   see description of the fpp_if_flags_t type
 *                                   in FCI API Reference.
 */
void demo_log_if_ld_set_match_mode_or(fpp_log_if_cmd_t* p_logif, bool match_mode_is_or)
{
    assert(NULL != p_logif);
    set_logif_flag(p_logif, match_mode_is_or, FPP_IF_MATCH_OR);
}
 
 
/*
 * @brief          Set/unset inverted mode of traffic acceptance.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      enable   Request to set/unset inverted mode.
 *                          For details about logical interface inverted mode,
 *                          see description of the fpp_if_flags_t type
 *                          in FCI API Reference.
 */
void demo_log_if_ld_set_discard_on_m(fpp_log_if_cmd_t* p_logif, bool enable)
{
    assert(NULL != p_logif);
    set_logif_flag(p_logif, enable, FPP_IF_DISCARD);
}
 
 
/*
 * @brief          Set target physical interfaces (egress vector) which 
 *                 shall receive a copy of the accepted traffic.
 * @details        [localdata_logif]
 *                 New egress vector fully replaces the old one.
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      egress   Target physical interfaces (egress vector). A bitset.
 *                          Each physical interface is represented by one bit.
 *                          Conversion between physical interface ID and a corresponding
 *                          egress vector bit is (1uL << "ID of a target physical interface").
 */
void demo_log_if_ld_set_egress_phyifs(fpp_log_if_cmd_t* p_logif, uint32_t egress)
{
    assert(NULL != p_logif);
    p_logif->egress = htonl(egress);
}
 
 
/*
 * @brief      Query the flags of a logical interface (the whole bitset).
 * @details    [localdata_phyif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Flags bitset at time when the data was obtained from PFE.
 */
fpp_if_flags_t demo_log_if_ld_get_flags(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_flags_t tmp_flags = (p_logif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (tmp_flags);
}
 
 
 
 
/*
 * @brief          Clear all match rules of a logical interface.
 *                 (also zeroify all match rule arguments of the logical interface)
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 */
void demo_log_if_ld_clear_all_mr(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    p_logif->match = 0u;
    memset(&(p_logif->arguments), 0, sizeof(fpp_if_m_args_t));
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_ETH).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_eth(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_ETH);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_VLAN).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_vlan(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_VLAN);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_PPPOE).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_pppoe(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_PPPOE);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_ARP).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_arp(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_ARP);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_MCAST).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_mcast(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_MCAST);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_IPV4).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_ip4(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_IPV4);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_IPV6).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_ip6(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_IPV6);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_IPX).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_ipx(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_IPX);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_BCAST).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_bcast(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_BCAST);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_UDP).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_udp(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_UDP);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_TCP).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_tcp(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_TCP);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_ICMP).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_icmp(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_ICMP);
}
 
 
/*
 * @brief          Set/unset the given match rule (TYPE_IGMP).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 */
void demo_log_if_ld_set_mr_type_igmp(fpp_log_if_cmd_t* p_logif, bool set)
{
    assert(NULL != p_logif);
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_TYPE_IGMP);
}
 
 
/*
 * @brief          Set/unset the given match rule (VLAN) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      vlan     New VLAN ID for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'vlan' argument with the value of traffic's 'VID' field.
 */
void demo_log_if_ld_set_mr_vlan(fpp_log_if_cmd_t* p_logif, bool set, uint16_t vlan)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_VLAN);
    p_logif->arguments.vlan = htons(vlan);
}
 
 
/*
 * @brief          Set/unset the given match rule (PROTO) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      proto    New IP Protocol Number for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'proto' argument with the value of traffic's 'Protocol' field.
 *                          See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 */
void demo_log_if_ld_set_mr_proto(fpp_log_if_cmd_t* p_logif, bool set, uint8_t proto)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_PROTO);
    p_logif->arguments.proto = proto;
}
 
 
/*
 * @brief          Set/unset the given match rule (SPORT) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      sport    New source port value for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sport' argument with the value of traffic's 'source port' field.
 */
void demo_log_if_ld_set_mr_sport(fpp_log_if_cmd_t* p_logif, bool set, uint16_t sport)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_SPORT);
    p_logif->arguments.sport = htons(sport);
}
 
 
/*
 * @brief          Set/unset the given match rule (DPORT) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      dport    New destination port value for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dport' argument with the value of traffic's 
 *                          'destination port' field.
 */
void demo_log_if_ld_set_mr_dport(fpp_log_if_cmd_t* p_logif, bool set, uint16_t dport)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_DPORT);
    p_logif->arguments.dport = htons(dport);
}
 
 
/*
 * @brief          Set/unset the given match rule (SIP6) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      p_sip6   New source IPv6 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sip' argument with the value of traffic's 
 *                          'source address' (applicable on IPv6 traffic only).
 */
void demo_log_if_ld_set_mr_sip6(fpp_log_if_cmd_t* p_logif, bool set, 
                               const uint32_t p_sip6[4])
{
    assert(NULL != p_logif);
    assert(NULL != p_sip6);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_SIP6);
    
    p_logif->arguments.ipv.v6.sip[0] = htonl(p_sip6[0]);
    p_logif->arguments.ipv.v6.sip[1] = htonl(p_sip6[1]);
    p_logif->arguments.ipv.v6.sip[2] = htonl(p_sip6[2]);
    p_logif->arguments.ipv.v6.sip[3] = htonl(p_sip6[3]);
}
 
 
/*
 * @brief          Set/unset the given match rule (SIP6) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      p_dip6   New destination IPv6 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dip' argument with the value of traffic's 
 *                          'destination address' (applicable on IPv6 traffic only).
 */
void demo_log_if_ld_set_mr_dip6(fpp_log_if_cmd_t* p_logif, bool set, 
                               const uint32_t p_dip6[4])
{
    assert(NULL != p_logif);
    assert(NULL != p_dip6);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_DIP6);
    
    p_logif->arguments.ipv.v6.dip[0] = htonl(p_dip6[0]);
    p_logif->arguments.ipv.v6.dip[1] = htonl(p_dip6[1]);
    p_logif->arguments.ipv.v6.dip[2] = htonl(p_dip6[2]);
    p_logif->arguments.ipv.v6.dip[3] = htonl(p_dip6[3]);
}
 
 
/*
 * @brief          Set/unset the given match rule (SIP) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      sip      New source IPv4 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sip' argument with the value of traffic's 
 *                          'source address' (applicable on IPv4 traffic only).
 */
void demo_log_if_ld_set_mr_sip(fpp_log_if_cmd_t* p_logif, bool set, uint32_t sip)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_SIP);
    p_logif->arguments.ipv.v4.sip = htonl(sip);
}
 
 
/*
 * @brief          Set/unset the given match rule (DIP) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      dip      New destination IPv4 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dip' argument with the value of traffic's 
 *                          'destination address' (applicable on IPv4 traffic only).
 */
void demo_log_if_ld_set_mr_dip(fpp_log_if_cmd_t* p_logif, bool set, uint32_t dip)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_DIP);
    p_logif->arguments.ipv.v4.dip = htonl(dip);
}
 
 
/*
 * @brief          Set/unset the given match rule (ETHTYPE) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      ethtype  New EtherType number for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'ethtype' argument with the value of traffic's 'EtherType' field.
 *                          See "IANA EtherType number (IEEE 802)":
 *                https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
 */
void demo_log_if_ld_set_mr_ethtype(fpp_log_if_cmd_t* p_logif, bool set, uint16_t ethtype)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_ETHTYPE);
    p_logif->arguments.ethtype = htons(ethtype);
}
 
 
/*
 * @brief          Set/unset the given match rule (FP0) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      fp_table0_name  Name of a FlexibleParser table for this match rule.
 *                                 Requested FlexibleParser table must already exist in PFE.
 *                                 When this match rule is active, it inspects traffic
 *                                 according to rules listed in the referenced
 *                                 FlexibleParser table.
 */
void demo_log_if_ld_set_mr_fp0(fpp_log_if_cmd_t* p_logif, bool set, 
                               const char* fp_table0_name)
{
    assert(NULL != p_logif);
    /* 'fp_table0_name' is allowed to be NULL */
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_FP0);
    set_text((p_logif->arguments.fp_table0), fp_table0_name, IFNAMSIZ);
}
 
 
/*
 * @brief          Set/unset the given match rule (FP1) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      fp_table1_name  Name of a FlexibleParser table for this match rule.
 *                                 Requested FlexibleParser table must already exist in PFE.
 *                                 When this match rule is active, it inspects traffic
 *                                 according to rules listed in the referenced
 *                                 FlexibleParser table.
 */
void demo_log_if_ld_set_mr_fp1(fpp_log_if_cmd_t* p_logif, bool set, 
                               const char* fp_table1_name)
{
    assert(NULL != p_logif);
    /* 'fp_table1_name' is allowed to be NULL */
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_FP1);
    set_text((p_logif->arguments.fp_table1), fp_table1_name, IFNAMSIZ);
}
 
 
/*
 * @brief          Set/unset the given match rule (SMAC) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      p_smac   New source MAC address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'smac' argument with the value of traffic's 'source MAC' field.
 */
void demo_log_if_ld_set_mr_smac(fpp_log_if_cmd_t* p_logif, bool set, const uint8_t p_smac[6])
{
    assert(NULL != p_logif);
    assert(NULL != p_smac);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_SMAC);
    memcpy((p_logif->arguments.smac), p_smac, (6 * sizeof(uint8_t)));
}
 
 
/*
 * @brief          Set/unset the given match rule (DMAC) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      p_dmac   New destination MAC address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dmac' argument with the value of traffic's 
 *                          'destination MAC' field.
 */
void demo_log_if_ld_set_mr_dmac(fpp_log_if_cmd_t* p_logif, bool set, const uint8_t p_dmac[6])
{
    assert(NULL != p_logif);
    assert(NULL != p_dmac);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_DMAC);
    memcpy((p_logif->arguments.dmac), p_dmac, (6 * sizeof(uint8_t)));
}
 
 
/*
 * @brief          Set/unset the given match rule (HIF_COOKIE) and its argument.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 * @param[in]      set      Request to set/unset the given match rule.
 * @param[in]      hif_cookie  New hif cookie value for this match rule.
 *                             When this match rule is active, it compares value of its
 *                            'hif_cookie' argument with the value of a hif_cookie tag.
 *                             Hif_cookie tag is a part of internal overhead data, attached
 *                             to traffic by a host's PFE driver.
 */
void demo_log_if_ld_set_mr_hif_cookie(fpp_log_if_cmd_t* p_logif, bool set, 
                                      uint32_t hif_cookie)
{
    assert(NULL != p_logif);
    
    set_logif_mr_flag(p_logif, set, FPP_IF_MATCH_HIF_COOKIE);
    p_logif->arguments.hif_cookie = htonl(hif_cookie);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query the status of the "enable" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : was enabled  ("up")
 *             false : was disabled ("down")
 */
bool demo_log_if_ld_is_enabled(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_flags_t tmp_flags = (p_logif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_ENABLED);
}
 
 
/*
 * @brief      Query the status of the "enable" flag (inverted logic).
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : was disabled ("down")
 *             false : was enabled  ("up)
 */
bool demo_log_if_ld_is_disabled(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return !demo_log_if_ld_is_enabled(p_logif);
}
 
 
/*
 * @brief      Query the status of the "promiscuous mode" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : was in a promiscuous mode
 *             false : was NOT in a promiscuous mode
 */
bool demo_log_if_ld_is_promisc(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_flags_t tmp_flags = (p_logif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_PROMISC);
}
 
 
/*
 * @brief      Query the status of the "match mode" flag (chaining mode of match rules).
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : was using OR match mode
 *             false : was using AND match mode
 */
bool demo_log_if_ld_is_match_mode_or(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_flags_t tmp_flags = (p_logif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_MATCH_OR);
}
 
 
/*
 * @brief      Query the status of the "discard on match" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : was discarding traffic that passed its matching process
 *             false : was NOT discarding traffic that passed its matching process
 */
bool demo_log_if_ld_is_discard_on_m(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_flags_t tmp_flags = (p_logif->flags);
    ntoh_enum(&tmp_flags, sizeof(fpp_if_flags_t));
    
    return (bool)(tmp_flags & FPP_IF_DISCARD);
}
 
 
/*
 * @brief      Query whether a physical interface is a member of 
 *             a logical interface's egress vector.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @param[in]  egress_bitflag  Queried physical interface. A bitflag.
 *                             Each physical interface is represented by one bit.
 *                             Conversion between physical interface ID and a corresponding
 *                             egress vector bit is 
 *                             (1uL << "ID of a target physical interface").
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : had at least one queried egress bitflag set
 *             false : had none of the queried egress bitflags set
 */
bool demo_log_if_ld_is_egress_phyifs(const fpp_log_if_cmd_t* p_logif, uint32_t egress_bitflag)
{
    assert(NULL != p_logif);
    return (bool)(ntohl(p_logif->match) & egress_bitflag);
}
 
 
/*
 * @brief      Query whether a match rule is active or not.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @param[in]  match_rule  Queried match rule.
 * @return     At time when the data was obtained from PFE, the logical interface:
 *             true  : had at least one queried match rule set
 *             false : had none of the queried match rules set
 */
bool demo_log_if_ld_is_match_rule(const fpp_log_if_cmd_t* p_logif, 
                                  fpp_if_m_rules_t match_rule)
{
    assert(NULL != p_logif);
    
    fpp_if_m_rules_t tmp_match = (p_logif->match);
    ntoh_enum(&tmp_match, sizeof(fpp_if_m_rules_t));
    
    return (bool)(tmp_match & match_rule);
}
 
 
 
 
/*
 * @brief      Query the name of a logical interface.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Name of the logical interface.
 */
const char* demo_log_if_ld_get_name(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->name);
}
 
 
/*
 * @brief      Query the ID of a logical interface.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     ID of the logical interface.
 */
uint32_t demo_log_if_ld_get_id(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->id);
}
 
 
/*
 * @brief      Query the name of a logical interface's parent.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Name of the parent physical interface.
 */
const char* demo_log_if_ld_get_parent_name(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->parent_name);
}
 
 
/*
 * @brief      Query the ID of a logical interface's parent.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     ID of the parent physical interface.
 */
uint32_t demo_log_if_ld_get_parent_id(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->parent_id);
}
 
 
/*
 * @brief      Query the target physical interfaces (egress vector) of a logical interface.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Egress vector.
 */
uint32_t demo_log_if_ld_get_egress(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->egress);
}
 
 
 
 
/*
 * @brief      Query the match rule bitset of a logical interface.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Match rule bitset.
 */
fpp_if_m_rules_t demo_log_if_ld_get_mr_bitset(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    
    fpp_if_m_rules_t tmp_match = (p_logif->match);
    ntoh_enum(&tmp_match, sizeof(fpp_if_m_rules_t));
    
    return (tmp_match);
}
 
 
/*
 * @brief      Query the argument of the match rule VLAN.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (VLAN ID) of the given match rule.
 */
uint16_t demo_log_if_ld_get_mr_arg_vlan(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohs(p_logif->arguments.vlan);
}
 
 
/*
 * @brief      Query the argument of the match rule PROTO.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (Protocol ID) of the given match rule.
 */
uint8_t demo_log_if_ld_get_mr_arg_proto(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->arguments.proto);
}
 
 
/*
 * @brief      Query the argument of the match rule SPORT.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (source port ID) of the given match rule.
 */
uint16_t demo_log_if_ld_get_mr_arg_sport(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohs(p_logif->arguments.sport);
}
 
 
/*
 * @brief      Query the argument of the match rule DPORT.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (destination port ID) of the given match rule.
 */
uint16_t demo_log_if_ld_get_mr_arg_dport(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohs(p_logif->arguments.dport);
}
 
 
/*
 * @brief      Query the argument of the match rule SIP6.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (source IPv6) of the given match rule.
 */
const uint32_t* demo_log_if_ld_get_mr_arg_sip6(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    static uint32_t rtn_sip6[4] = {0u};
    
    rtn_sip6[0] = ntohl(p_logif->arguments.ipv.v6.sip[0]);
    rtn_sip6[1] = ntohl(p_logif->arguments.ipv.v6.sip[1]);
    rtn_sip6[2] = ntohl(p_logif->arguments.ipv.v6.sip[2]);
    rtn_sip6[3] = ntohl(p_logif->arguments.ipv.v6.sip[3]);
    
    return (rtn_sip6);
}
 
 
/*
 * @brief      Query the argument of the match rule DIP6.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (destination IPv6) of the given match rule.
 */
const uint32_t* demo_log_if_ld_get_mr_arg_dip6(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    static uint32_t rtn_dip6[4] = {0u};
    
    rtn_dip6[0] = ntohl(p_logif->arguments.ipv.v6.dip[0]);
    rtn_dip6[1] = ntohl(p_logif->arguments.ipv.v6.dip[1]);
    rtn_dip6[2] = ntohl(p_logif->arguments.ipv.v6.dip[2]);
    rtn_dip6[3] = ntohl(p_logif->arguments.ipv.v6.dip[3]);
    
    return (rtn_dip6);
}
 
 
/*
 * @brief      Query the argument of the match rule SIP.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (source IPv4) of the given match rule.
 */
uint32_t demo_log_if_ld_get_mr_arg_sip(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->arguments.ipv.v4.sip);
}
 
 
/*
 * @brief      Query the argument of the match rule DIP.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (destination IPv4) of the given match rule.
 */
uint32_t demo_log_if_ld_get_mr_arg_dip(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->arguments.ipv.v4.dip);
}
 
 
/*
 * @brief      Query the argument of the match rule ETHTYPE.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (EtherType ID) of the given match rule.
 */
uint16_t demo_log_if_ld_get_mr_arg_ethtype(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohs(p_logif->arguments.ethtype);
}
 
 
/*
 * @brief      Query the argument of the match rule FP0.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (name of a FlexibleParser table) of the given match rule.
 */
const char* demo_log_if_ld_get_mr_arg_fp0(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->arguments.fp_table0);
}
 
 
/*
 * @brief      Query the argument of the match rule FP1.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (name of a FlexibleParser table) of the given match rule.
 */
const char* demo_log_if_ld_get_mr_arg_fp1(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->arguments.fp_table1);
}
 
 
/*
 * @brief      Query the argument of the match rule SMAC.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (source MAC address) of the given match rule.
 */
const uint8_t* demo_log_if_ld_get_mr_arg_smac(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->arguments.smac);
}
 
 
/*
 * @brief      Query the argument of the match rule DMAC.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (destination MAC address) of the given match rule.
 */
const uint8_t* demo_log_if_ld_get_mr_arg_dmac(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (p_logif->arguments.dmac);
}
 
 
/*
 * @brief      Query the argument of the match rule HIF_COOKIE.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Argument (hif cookie value) of the given match rule.
 */
uint32_t demo_log_if_ld_get_mr_arg_hif_cookie(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->arguments.hif_cookie);
}
 
 
 
 
/*
 * @brief      Query the statistics of a logical interface - processed.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Count of processed packets at the time when the data was obtained form PFE.
 */
uint32_t demo_log_if_ld_get_stt_processed(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->stats.processed);
}
 
 
/*
 * @brief      Query the statistics of a logical interface - accepted.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Count of accepted packets at the time when the data was obtained form PFE.
 */
uint32_t demo_log_if_ld_get_stt_accepted(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->stats.accepted);
}
 
 
/*
 * @brief      Query the statistics of a logical interface - rejected.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Count of rejected packets at the time when the data was obtained form PFE.
 */
uint32_t demo_log_if_ld_get_stt_rejected(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->stats.rejected);
}
 
 
/*
 * @brief      Query the statistics of a logical interface - discarded.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 * @return     Count of discarded packets at the time when the data was obtained form PFE.
 */
uint32_t demo_log_if_ld_get_stt_discarded(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return ntohl(p_logif->stats.discarded);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available logical interfaces in PFE and
 *             execute a callback print function for each applicable logical interface.
 * @details    To use this function properly, the interface database of PFE must be
 *             locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *             an example of a database lock procedure.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next logical interface is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]  p_parent_name   [optional parameter] Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 *                             Can be NULL.
 *                             If NULL, then all available logical interfaces are printed.
 *                             If non-NULL, then only those logical interfaces which are 
 *                             children of the given physical interface are printed.
 * @return     FPP_ERR_OK : Successfully iterated through all available logical interfaces.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_log_if_print_all(FCI_CLIENT* p_cl, demo_log_if_cb_print_t p_cb_print,
                         const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    /* 'p_parent_name' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if ((NULL == p_parent_name) || 
            (0 == strcmp((reply_from_fci.parent_name), p_parent_name)))
        {
            rtn = p_cb_print(&reply_from_fci);
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                            sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_log_if_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available logical interfaces in PFE.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_log_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl           FCI client
 * @param[out]  p_rtn_count    Space to store the count of logical interfaces.
 * @param[in]   p_parent_name  [optional parameter] Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See FCI API Reference, chapter Interface Management.
 *                             Can be NULL.
 *                             If NULL, then all available logical interfaces are counted.
 *                             If non-NULL, then only those logical interfaces which are 
 *                             children of the given physical interface are counted.
 * @return      FPP_ERR_OK : Successfully counted all applicable logical interfaces.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_log_if_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                         const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    /* 'p_parent_name' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if ((NULL == p_parent_name) || 
            (0 == strcmp((reply_from_fci.parent_name), p_parent_name)))
        {
            count++;
        }
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_log_if_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
