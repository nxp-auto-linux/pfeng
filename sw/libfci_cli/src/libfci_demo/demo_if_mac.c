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
#include "demo_if_mac.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
/*
 * @brief       Use FCI calls to add a new MAC address to an interface.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl       FCI client
 * @param[out]  p_mac      New MAC address.
 * @param[in]   p_name     Name of a target physical interface.
 *                         Names of physical interfaces are hardcoded.
 *                         See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : New MAC address was added to the target physical interface.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_if_mac_add(FCI_CLIENT* p_cl, const uint8_t p_mac[6], const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_if_mac_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        memcpy(cmd_to_fci.mac, p_mac, (6 * sizeof(uint8_t)));
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_IF_MAC, sizeof(fpp_if_mac_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_if_mac_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to remove the target MAC address from an interface.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl       FCI client
 * @param[out]  p_mac      MAC address to be remove.
 * @param[in]   p_name     Name of a target physical interface.
 *                         Names of physical interfaces are hardcoded.
 *                         See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : The MAC address was removed from the target physical interface.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_if_mac_del(FCI_CLIENT* p_cl, const uint8_t p_mac[6], const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_mac);
    assert(NULL != p_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_if_mac_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        memcpy(cmd_to_fci.mac, p_mac, (6 * sizeof(uint8_t)));
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_IF_MAC, sizeof(fpp_if_mac_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    print_if_error(rtn, "demo_if_mac_del() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
/*
 * @defgroup    localdata_if_mac  [localdata_if_mac]
 * @brief:      Functions marked as [localdata_if_mac] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_if_mac (a struct with MAC data).
 */
 
 
/*
 * @brief      Query the name of a target interface.
 * @details    [localdata_if_mac]
 * @param[in]  p_if_mac  Local data to be queried.
 * @return     Name of the target interface.
 */
const char* demo_if_mac_ld_get_name(const fpp_if_mac_cmd_t* p_if_mac)
{
    assert(NULL != p_if_mac);
    return (p_if_mac->name);
}
 
 
/*
 * @brief      Query the MAC address of a target interface.
 * @details    [localdata_if_mac]
 * @param[in]  p_if_mac  Local data to be queried.
 * @return     MAC address of the target interface.
 */
const uint8_t* demo_if_mac_ld_get_mac(const fpp_if_mac_cmd_t* p_if_mac)
{
    assert(NULL != p_if_mac);
    return (p_if_mac->mac);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all MAC addresses of a target interface 
 *             in PFE. Execute a callback print function for each MAC address.
 * @details    To use this function properly, the interface database of PFE must be
 *             locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *             an example of a database lock procedure.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next physical interface is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]   p_name     Name of a target physical interface.
 *                         Names of physical interfaces are hardcoded.
 *                         See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all MAC addresses.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_if_mac_print_by_name(FCI_CLIENT* p_cl, demo_if_mac_cb_print_t p_cb_print,
                              const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_if_mac_cmd_t cmd_to_fci = {0};
    fpp_if_mac_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_IF_MAC,
                        sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        
        /* query loop */
        while (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&reply_from_fci);
            
            print_if_error(rtn, "demo_if_mac_print_by_name() --> "
                                "non-zero return from callback print function!");
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_IF_MAC,
                                sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
            }
        }
        
        /* query loop runs till there are no more MAC addresses to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_IF_MAC_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_if_mac_print_by_name() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all MAC addresses of a target interface
 *              in PFE.
 * @details     To use this function properly, the interface database of PFE must be
 *              locked for exclusive access. See demo_phy_if_get_by_name_sa() for
 *              an example of a database lock procedure.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of MAC addresses.
 * @param[in]   p_name       Name of a target physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all MAC addresses of the target interface.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_if_mac_get_count_by_name(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                  const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_if_mac_cmd_t cmd_to_fci = {0};
    fpp_if_mac_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_IF_MAC,
                        sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        
        /* query loop */
        while (FPP_ERR_OK == rtn)
        {
            count++;
            
            if (FPP_ERR_OK == rtn)
            {
                cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
                rtn = fci_query(p_cl, FPP_CMD_IF_MAC,
                                sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
                                &reply_length, (unsigned short*)(&reply_from_fci));
            }
        }
        
        /* query loop runs till there are no more MAC addresses to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_IF_MAC_NOT_FOUND == rtn)
        {
            *p_rtn_count = count;
            rtn = FPP_ERR_OK;
        }
    }
    
    print_if_error(rtn, "demo_if_mac_get_count_by_name() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
  