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
#include "demo_l2_bd.h"
 
 
/*
 * @brief      Use libFCI to configure PFE as a simple (non-VLAN aware) L2 bridge.
 * @details    Scenario description:
 *              [*] Let there be two computers (PCs).
 *                  Both PCs are in the same network subnet.
 *              [*] Use libFCI to configure PFE as a simple (non-VLAN aware) L2 bridge, 
 *                  allowing the PCs to communicate with each other.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.3.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *               PC1:
 *                 --> IP address: 10.3.0.5/24
 *                 --> Accessible via PFE's emac1 physical interface.
 *             Additional info:
 *               For simple (non-VLAN aware) bridge, the "default BD" (default bridge domain)
 *               must always be used. This is hardcoded behavior of PFE.
 *               
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Simple (non-VLAN aware) L2 bridge should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_L2_bridge_simple(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* clear L2 bridge MAC table (not required; done for demo purposes) */
    /* ================================================================ */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_flush_all(p_cl);
    }
    
    
    /* configure the "default BD" */
    /* ========================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_l2_bd_cmd_t bd = {0};
        
        /* get data from PFE and store them in the local variable "bd" */
        /* 1u == vlan ID of the "default BD" */
        rtn = demo_l2_bd_get_by_vlan(p_cl, &bd, 1u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_l2_bd_ld_insert_phyif(&bd, 0u, false);  /* 0u == ID of emac0 */
            demo_l2_bd_ld_insert_phyif(&bd, 1u, false);  /* 1u == ID of emac1 */
            demo_l2_bd_ld_set_ucast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
            demo_l2_bd_ld_set_ucast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
            demo_l2_bd_ld_set_mcast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
            demo_l2_bd_ld_set_mcast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
        
            /* update data in PFE */
            rtn = demo_l2_bd_update(p_cl, &bd);
        }
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
                /* get data from PFE and store them in the local variable "phyif" */
                rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac0");
                if (FPP_ERR_OK == rtn)
                {
                    /* modify locally stored data */
                    demo_phy_if_ld_enable(&phyif);
                    demo_phy_if_ld_set_promisc(&phyif, true);
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_BRIDGE);
                    demo_phy_if_ld_set_block_state(&phyif, BS_NORMAL);
                
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
            
            /* configure physical interface "emac1" */
            /* ------------------------------------ */
            if (FPP_ERR_OK == rtn)
            {
                /* get data from PFE and store them in the local variable "phyif" */
                rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac1");
                if (FPP_ERR_OK == rtn)
                {
                    /* modify locally stored data */
                    demo_phy_if_ld_enable(&phyif);
                    demo_phy_if_ld_set_promisc(&phyif, true);
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_BRIDGE);
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
 
