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
#include <stdbool.h>
#include <stdio.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_phy_if.h"
#include "demo_if_mac.h"
#include "demo_mirror.h"
 
extern int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl);
 
 
/*
 * @brief      Use libFCI to configure advanced properties of physical interfaces.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs), both in the same network subnet.
 *                   Both PCs are connected to PFE, each to one PFE emac physical interface.
 *                   PFE acts as a simple bridge.
 *               [*] MAC address filtering:
 *                   Selected emac physical interfaces should not work in a promiscuous mode,
 *                   but should accept only traffic from a selected range of destination MAC 
 *                   addresses. Use libFCI to configure this MAC address filtering.
 *               [*] Mirroring:
 *                   Use libFCI to create and assign mirroring rules. Task is to mirror 
 *                   a copy of all PC0<->PC1 communication to emac2 physical interface.
 *             PC description:
 *               PC0:
 *                 --> IP address:  10.3.0.2/24
 *                 --> MAC address: 0A:01:23:45:67:89
 *                     (this is just a demo MAC; real MAC of the real PC0 should be used)
 *                 --> Accessible via PFE's emac0 physical interface.
 *               PC1:
 *                 --> IP address:  10.3.0.5/24
 *                 --> MAC address: 0A:FE:DC:BA:98:76
 *                     (this is just a demo MAC; real MAC of the real PC1 should be used)
 *                 --> Accessible via PFE's emac1 physical interface.
 *               
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Physical interfaces should be configured now.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_physical_interface(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* setup PFE to classify traffic (not needed, but done for demo purposes)*/
    /* ===================================================================== */
    rtn = demo_feature_L2_bridge_vlan(p_cl);
    
    
    /* create a mirroring rule */
    /* ======================= */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_add(p_cl, NULL, "MirroringRule0", "emac2");
    }
    
    
    /* configure physical interfaces */
    /* ============================= */
    if (FPP_ERR_OK == rtn)
    {
        /* lock the interface database of PFE */
        rtn = demo_if_session_lock(p_cl);
        if (FPP_ERR_OK == rtn)
        {
            fpp_phy_if_cmd_t phyif = {0};
            
            /* configure physical interface "emac0" */
            /* ------------------------------------ */
            if (FPP_ERR_OK == rtn)
            {
                /* add MAC address filter: accept traffic with dest. MAC == MAC of PC1 */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_if_mac_add(p_cl, (uint8_t[6]){0x0A,0xFE,0xDC,0xBA,0x98,0x76}, 
                                                "emac0");
                }
                
                
                /* get data from PFE and store them in the local variable "phyif" */
                rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac0");
                if (FPP_ERR_OK == rtn)
                {
                    /* modify locally stored data */
                    demo_phy_if_ld_set_rx_mirror(&phyif, 0, "MirroringRule0");  /* mirror */
                    demo_phy_if_ld_set_promisc(&phyif, false);  /* disable promiscuous mode
                                                                   (MAC filters are used) */
                
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
            
            /* configure physical interface "emac1" */
            /* ------------------------------------ */
            if (FPP_ERR_OK == rtn)
            {
                /* add MAC address filter: accept traffic with dest. MAC == MAC of PC0 */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_if_mac_add(p_cl, (uint8_t[6]){0x0A,0x01,0x23,0x45,0x67,0x89}, 
                                                "emac1");
                }
                
                
                /* get data from PFE and store them in the local variable "phyif" */
                rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac1");
                if (FPP_ERR_OK == rtn)
                {
                    /* modify locally stored data */
                    demo_phy_if_ld_set_rx_mirror(&phyif, 0, "MirroringRule0");  /* mirror */
                    demo_phy_if_ld_set_promisc(&phyif, false);  /* disable promiscuous mode
                                                                   (MAC filters are used) */
                    
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
            
            /* configure physical interface "emac2" */
            /* ------------------------------------ */
            if (FPP_ERR_OK == rtn)
            {
                /* get data from PFE and store them in the local variable "phyif" */
                rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac2");
                if (FPP_ERR_OK == rtn)
                {
                    /* modify locally stored data */
                    demo_phy_if_ld_enable(&phyif);
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_DEFAULT);
                    demo_phy_if_ld_set_block_state(&phyif, BS_NORMAL);
                    
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
        }
        
        /* unlock the interface database of PFE */
        rtn = demo_if_session_unlock(p_cl, rtn);
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
