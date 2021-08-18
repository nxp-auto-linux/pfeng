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
#include "fci_spd.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a SecurityPolicy struct.
 * @param[in,out]  p_rtn_spd  The SecurityPolicy struct to be converted.
 */
static void ntoh_spd(fpp_spd_cmd_t* p_rtn_spd)
{
    assert(NULL != p_rtn_spd);
    
    
    ntoh_enum(&(p_rtn_spd->flags), sizeof(fpp_spd_flags_t));
    p_rtn_spd->position = ntohs(p_rtn_spd->position);
    p_rtn_spd->saddr[0] = ntohl(p_rtn_spd->saddr[0]);
    p_rtn_spd->saddr[1] = ntohl(p_rtn_spd->saddr[1]);
    p_rtn_spd->saddr[2] = ntohl(p_rtn_spd->saddr[2]);
    p_rtn_spd->saddr[3] = ntohl(p_rtn_spd->saddr[3]);
    p_rtn_spd->daddr[0] = ntohl(p_rtn_spd->daddr[0]);
    p_rtn_spd->daddr[1] = ntohl(p_rtn_spd->daddr[1]);
    p_rtn_spd->daddr[2] = ntohl(p_rtn_spd->daddr[2]);
    p_rtn_spd->daddr[3] = ntohl(p_rtn_spd->daddr[3]);
    p_rtn_spd->sport = ntohs(p_rtn_spd->sport);
    p_rtn_spd->dport = ntohs(p_rtn_spd->dport);
    p_rtn_spd->sa_id = ntohl(p_rtn_spd->sa_id);
    p_rtn_spd->spi = ntohl(p_rtn_spd->spi);
    ntoh_enum(&(p_rtn_spd->spd_action), sizeof(fpp_spd_action_t));
}
 
 
/*
 * @brief          Host-to-network (hton) function for a SecurityPolicy struct.
 * @param[in,out]  p_rtn_spd  The SecurityPolicy struct to be converted.
 */
static void hton_spd(fpp_spd_cmd_t* p_rtn_spd)
{
    assert(NULL != p_rtn_spd);
    
    
    hton_enum(&(p_rtn_spd->flags), sizeof(fpp_spd_flags_t));
    p_rtn_spd->position = htons(p_rtn_spd->position);
    p_rtn_spd->saddr[0] = htonl(p_rtn_spd->saddr[0]);
    p_rtn_spd->saddr[1] = htonl(p_rtn_spd->saddr[1]);
    p_rtn_spd->saddr[2] = htonl(p_rtn_spd->saddr[2]);
    p_rtn_spd->saddr[3] = htonl(p_rtn_spd->saddr[3]);
    p_rtn_spd->daddr[0] = htonl(p_rtn_spd->daddr[0]);
    p_rtn_spd->daddr[1] = htonl(p_rtn_spd->daddr[1]);
    p_rtn_spd->daddr[2] = htonl(p_rtn_spd->daddr[2]);
    p_rtn_spd->daddr[3] = htonl(p_rtn_spd->daddr[3]);
    p_rtn_spd->sport = htons(p_rtn_spd->sport);
    p_rtn_spd->dport = htons(p_rtn_spd->dport);
    p_rtn_spd->sa_id = htonl(p_rtn_spd->sa_id);
    p_rtn_spd->spi = htonl(p_rtn_spd->spi);
    hton_enum(&(p_rtn_spd->spd_action), sizeof(fpp_spd_action_t));
}
 
 
/*
 * @brief       Set/unset a bitflag in a SecurityPolicy struct.
 * @param[out]  p_rtn_spd  The SecurityPolicy struct to be modified.
 * @param[in]   enable     New state of the bitflag.
 * @param[in]   flag       The bitflag.
 */
static void set_flag(fpp_spd_cmd_t* p_rtn_spd, bool enable, fpp_spd_flags_t flag)
{
    assert(NULL != p_rtn_spd);
    
    
    if (enable)
    {
        p_rtn_spd->flags |= flag;
    }
    else
    {
        p_rtn_spd->flags &= (fpp_spd_flags_t)(~flag);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested SecurityPolicy
 *              from the PFE. Identify the SecurityPolicy by name of the parent 
 *              physical interface (each physical interface has its own SPD) and by
 *              position of the policy in the SPD.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_spd     Space for data from the PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 *              position      Position of the requested SecurityPolicy in the SPD.
 * @return      FPP_ERR_OK : Requested SecurityPolicy was found.
 *                           A copy of its configuration was stored into p_rtn_spd.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_spd_get_by_position(FCI_CLIENT* p_cl, fpp_spd_cmd_t* p_rtn_spd, 
                            const char* p_phyif_name, uint16_t position)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_spd);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_spd_cmd_t cmd_to_fci = {0};
    fpp_spd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_SPD,
                        sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_spd(&reply_from_fci);  /* set correct byte order */
        
        /* query loop (with the search condition) */
        while ((FPP_ERR_OK == rtn) && (position != (reply_from_fci.position)))
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_SPD,
                            sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_spd(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_spd = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */
 
 
/*
 * @brief       Use FCI calls to create a new SecurityPolicy in the PFE.
 *              The new policy is added into SPD of a provided parent physical interface.
 * @param[in]   p_cl          FCI client instance
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @param[in]   position      Position of the new SecurityPolicy in the SPD.
 * @param[in]   p_spd_data    Configuration data for the new SecurityPolicy.
 *                            To create a new SecurityPolicy, a local data struct must be
 *                            created, configured and then passed to this function.
 *                            See [localdata_spd] functions to learn more.
 * @return      FPP_ERR_OK : New SecurityPolicy was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_spd_add(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position, 
                const fpp_spd_cmd_t* p_spd_data)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_spd_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = (*p_spd_data);
    cmd_to_fci.position = position;
    rtn = set_text((cmd_to_fci.name), p_phyif_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_spd(&cmd_to_fci);  /* set correct byte order */
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_SPD, sizeof(fpp_spd_cmd_t), 
                                          (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target SecurityPolicy in the PFE.
 * @param[in]   p_cl          FCI client instance
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 * @param[in]   position      Position of the target SecurityPolicy in the SPD.
 * @return      FPP_ERR_OK : SecurityPolicy was destroyed.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_spd_del(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_spd_cmd_t cmd_to_fci = {0};
    
    /*  prepare data  */
    cmd_to_fci.position = position;
    rtn = set_text((cmd_to_fci.name), p_phyif_name, IFNAMSIZ);
    
    /*  send data  */
    if (FPP_ERR_OK == rtn)
    {
        hton_spd(&cmd_to_fci);  /*  set correct byte order  */
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_SPD, sizeof(fpp_spd_cmd_t), 
                                          (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_spd  [localdata_spd]
 * @brief:      Functions marked as [localdata_spd] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_spd_add() to 
 *              create a new SecurityPolicy with given configuration in the PFE.
 */
 
 
/*
 * @brief          Set protocol type of a SecurityPolicy.
 * @details        [localdata_spd]
 * @param[in,out]  p_spd  Local data to be modified.
 *                        For SecurityPolicy, there are no "initial data" to be obtained
 *                        from the PFE. Simply declare a local data struct and configure it.
 * @param[in]      protocol  IP protocol ID
 *                           See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_spd_ld_set_protocol(fpp_spd_cmd_t* p_spd, uint8_t protocol)
{
    assert(NULL != p_spd);
    p_spd->protocol = protocol;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set source/destination IP address of a SecurityPolicy.
 * @details        [localdata_spd]
 *                 BEWARE: Address type of 'p_saddr' and 'p_daddr' must be the same!
 * @param[in,out]  p_spd  Local data to be modified.
 *                        For SecurityPolicy, there are no "initial data" to be obtained
 *                        from the PFE. Simply declare a local data struct and configure it.
 * @param[in]      p_saddr  Source IP address (IPv4 or IPv6).
 * @param[in]      p_daddr  Destination IP address (IP4 or IP6).
 * @param[in]      is_ip6   Set if provided addresses are IPv6 type addresses.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_spd_ld_set_ip(fpp_spd_cmd_t* p_spd, const uint32_t p_saddr[4], 
                      const uint32_t p_daddr[4], bool is_ip6)
{
    assert(NULL != p_spd);
    assert(NULL != p_saddr);
    assert(NULL != p_daddr);
    
    
    const uint16_t cnt = ((is_ip6) ? (4u) : (1u));
    memcpy((p_spd->saddr), p_saddr, (cnt * sizeof(uint32_t)));
    memcpy((p_spd->daddr), p_daddr, (cnt * sizeof(uint32_t)));
    set_flag(p_spd, is_ip6, FPP_SPD_FLAG_IPv6);
    
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set source/destination port of the SecurityPolicy.
 * @details        [localdata_spd]
 * @param[in,out]  p_spd  Local data to be modified.
 *                        For SecurityPolicy, there are no "initial data" to be obtained
 *                        from the PFE. Simply declare a local data struct and configure it.
 * @param[in]      use_sport  Prompt to use the source port value of this SecurityPolicy
 *                            during SPD matching process (evaluation which policy to use).
 *                            If false, then source port of the given SecurityPolicy is
 *                            ignored (not tested) when the policy is evaluated.
 * @param[in]      sport      Source port
 * @param[in]      use_dport  Prompt to use the destination port value of this SecurityPolicy
 *                            during SPD matching process (evaluation which policy to use).
 *                            If false, then destination port of the given SecurityPolicy is
 *                            ignored (not tested) when the policy is evaluated.
 * @param[in]      dport      Destination port
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_spd_ld_set_port(fpp_spd_cmd_t* p_spd, bool use_sport, uint16_t sport, 
                        bool use_dport, uint16_t dport)
{
    assert(NULL != p_spd);
    
    
    p_spd->sport = ((use_sport) ? (sport) : (0u));
    p_spd->dport = ((use_dport) ? (dport) : (0u));
    set_flag(p_spd, (!use_sport), FPP_SPD_FLAG_SPORT_OPAQUE);  /* flag has inverted logic */
    set_flag(p_spd, (!use_dport), FPP_SPD_FLAG_DPORT_OPAQUE);  /* flag has inverted logic */
    
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set action of the SecurityPolicy.
 * @details        [localdata_spd]
 * @param[in,out]  p_spd  Local data to be modified.
 *                        For SecurityPolicy, there are no "initial data" to be obtained
 *                        from the PFE. Simply declare a local data struct and configure it.
 * @param[in]      spd_action  Action to do if traffic matches this SecurityPolicy.
 *                             See description of fpp_spd_action_t type in 
 *                             the FCI API Reference.
 * @param[in]      sa_id  Meaningful ONLY if the action is FPP_SPD_ACTION_PROCESS_ENCODE.
 *                        ID of an item in the SAD (Security Association Database).
 *                        SAD is stored in the HSE FW (High Speed Encryption).
 * @param[in]      spi    Meaningful ONLY if the action is FPP_SPD_ACTION_PROCESS_DECODE.
 *                        Security Parameter Index (will be looked for in the traffic data)
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_spd_ld_set_action(fpp_spd_cmd_t* p_spd, fpp_spd_action_t spd_action, 
                          uint32_t sa_id, uint32_t spi)
{
    assert(NULL != p_spd);
    
    
    p_spd->spd_action = spd_action;
    p_spd->sa_id = ((FPP_SPD_ACTION_PROCESS_ENCODE == spd_action) ? (sa_id) : (0uL));
    p_spd->spi = ((FPP_SPD_ACTION_PROCESS_DECODE == spd_action) ? (spi) : (0uL));
    
    return (FPP_ERR_OK);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query address type of the given SecurityPolicy.
 * @details    [localdata_spd]
 * @param[in]  p_spd  Local data to be queried.
 * @return     IP address of the policy:
 *             true  : is IPv6 type.
 *             false : is NOT IPv6 type.
 */
bool fci_spd_ld_is_ip6(const fpp_spd_cmd_t* p_spd)
{
    assert(NULL != p_spd);
    return (bool)(FPP_SPD_FLAG_IPv6 & (p_spd->flags));
}
 
 
/*
 * @brief      Query whether the source port value is used during SPD matching process.
 * @details    [localdata_spd]
 * @param[in]  p_spd  Local data to be queried.
 * @return     Source port value:
 *             true  : is used in a matching process.
 *             false : is NOT used in a matching process.
 */
bool fci_spd_ld_is_used_sport(const fpp_spd_cmd_t* p_spd)
{
    assert(NULL != p_spd);
    return !(FPP_SPD_FLAG_SPORT_OPAQUE & (p_spd->flags));  /* flag has inverted logic */
}
 
 
/*
 * @brief      Query whether the destination port value is used during SPD matching process.
 * @details    [localdata_spd]
 * @param[in]  p_spd  Local data to be queried.
 * @return     Destination port value:
 *             true  : is used in a matching process.
 *             false : is NOT used in a matching process.
 */
bool fci_spd_ld_is_used_dport(const fpp_spd_cmd_t* p_spd)
{
    assert(NULL != p_spd);
    return !(FPP_SPD_FLAG_DPORT_OPAQUE & (p_spd->flags));  /* flag has inverted logic */
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through SecurityPolicies of the given physical
 *             interface and execute a callback print function for each SecurityPolicy.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next physical interface is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name   Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @param[in]  position_init  Start invoking callback print function from 
 *                            this position in the SPD.
 *                            If 0, start from the very first SecurityPolicy in the SPD.
 * @param[in]  count          Print only this count of SecurityPolicies, then stop.
 *                            If 0, keep printing SecurityPolicies till the end of the SPD.
 * @return     FPP_ERR_OK : Successfully iterated through SecrityPolicies of 
 *                          the given physical interface.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_spd_print_by_phyif(FCI_CLIENT* p_cl, fci_spd_cb_print_t p_cb_print, 
                           const char* p_phyif_name, uint16_t position_init, uint16_t count)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_spd_cmd_t cmd_to_fci = {0};
    fpp_spd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_phyif_name, IFNAMSIZ);
    if (0u == count)  /* if 0, set max possible count of items */ 
    {
        count--;  /*  WARNING: intentional use of owf behavior */
    }
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_SPD,
                        sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_spd(&reply_from_fci);  /* set correct byte order */
    
        /* query loop */
        uint16_t position = 0u;
        while ((FPP_ERR_OK == rtn) && (0u != count))
        {
            if (position >= position_init)
            {
                rtn = p_cb_print(&reply_from_fci);
                count--;
            }
            
            position++;
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_SPD,
                                sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
                ntoh_spd(&reply_from_fci);  /* set correct byte order */
            }
        }
        
        /* query loop runs till there are no more SecurityPolicies to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
} 
 
 
/*
 * @brief       Use FCI calls to get a count of all SecurityPolicies in the PFE which are
 *              associated with the given physical interface.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_count   Space to store the count of SecurityPolicies.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted SecurityPolicies.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_spd_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, 
                               const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_spd_cmd_t cmd_to_fci = {0};
    fpp_spd_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_SPD,
                        sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting policies) */
        
        /*  query loop  */
        while (FPP_ERR_OK == rtn)
        {
            count++;
            
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_SPD,
                            sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            /* no need to set correct byte order (we are just counting policies) */
        }
        
        /* query loop runs till there are no more SecurityPolicies to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
        {
            *p_rtn_count = count;
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
