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
#include "demo_log_if.h"
 
 
/*
 * @brief      Use libFCI to configure PFE as a Flexible Router.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs).
 *                   Each PC is in a different network subnet.
 *               [*] Use libFCI to configure PFE as a Flexible Router, allowing the PCs 
 *                   to communicate with each other.
 *               [*] Only a specific traffic is allowed through PFE (the rest is discarded).
 *                   Criteria for the allowed traffic:
 *                     --> Only ARP and ICMP traffic is allowed through PFE.
 *                     --> No further limitations for ARP traffic.
 *                     --> For ICMP traffic, only IPs of PC0 and PC1 are allowed 
 *                         to communicate with each other. ICMP traffic from 
 *                         any other IP must be blocked.
 *                     --> EXTRA: All traffic which passes through PFE must also be mirrored
 *                                to the emac2 physical interface.
 *               [*] NOTE:
 *                   Flexible Router is best used for special, non-standard requirements.
 *                   Scanning of traffic data and chaining of logical interfaces presents 
 *                   an additional overhead.
 *                   PFE features such as L2 bridge or L3 router offer a better performance
 *                   and are recommended over the Flexible Router in all cases where
 *                   they can be used to satisfy the given requirements.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.7.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Configured to send 10.11.0.0 traffic to PFE's emac0.
 *               PC1:
 *                 --> IP address: 10.11.0.5/24
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Configured to send 10.7.0.0 traffic to PFE's emac1.
 *              
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Flexible Router should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_flexible_router(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* lock the interface database of PFE */
    rtn = demo_if_session_lock(p_cl);
    
    
    /* create and configure logical interfaces on emac0 */
    /* ================================================ */
    /* NOTE: creation order of logical interfaces is IMPORTANT */
    if (FPP_ERR_OK == rtn)
    {
        fpp_log_if_cmd_t logif = {0};
        
        /* create a "sinkhole" logical interface for unsuitable ingress traffic */
        /* -------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create new logical interface in PFE and store a copy of its data in "logif" */
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif0_sink", "emac0");
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_log_if_ld_set_promisc(&logif, true);  /* promisc == accept everything */
                demo_log_if_ld_set_discard_on_m(&logif, true);
                demo_log_if_ld_enable(&logif);
                
                /* update data in PFE */
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
        
        /* create and configure a logical interface for ARP ingress traffic */
        /* ---------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif0_arp", "emac0");
            if (FPP_ERR_OK == rtn)
            {
                /* NOTE: 1u == ID of emac1 ; 2u == ID of emac2 */
                demo_log_if_ld_set_promisc(&logif, false);
                demo_log_if_ld_set_egress_phyifs(&logif, ((1uL << 1u) | (1uL << 2u)));
                demo_log_if_ld_set_match_mode_or(&logif, false);
                demo_log_if_ld_clear_all_mr(&logif);
                demo_log_if_ld_set_mr_type_arp(&logif, true);
                demo_log_if_ld_enable(&logif);
            
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
        
        /* create and configure a logical interface for ICMP ingress traffic */
        /* ----------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif0_icmp", "emac0");
            if (FPP_ERR_OK == rtn)
            {
                /* NOTE: 1u == ID of emac1 ; 2u == ID of emac2 */
                demo_log_if_ld_set_promisc(&logif, false);
                demo_log_if_ld_set_egress_phyifs(&logif, ((1uL << 1u) | (1uL << 2u)));
                demo_log_if_ld_set_match_mode_or(&logif, false);
                demo_log_if_ld_clear_all_mr(&logif);
                demo_log_if_ld_set_mr_type_icmp(&logif, true);
                demo_log_if_ld_set_mr_sip(&logif, true, 0x0A070002);
                demo_log_if_ld_set_mr_dip(&logif, true, 0x0A0B0005);
                demo_log_if_ld_enable(&logif);
                
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
    }
    
    
    /* create and configure logical interfaces on emac1 */
    /* ================================================ */
    /* NOTE: creation order of logical interfaces is IMPORTANT */
    if (FPP_ERR_OK == rtn)
    {
        fpp_log_if_cmd_t logif = {0};
        
        /* create a "sinkhole" logical interface for unsuitable ingress traffic */
        /* -------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create new logical interface in PFE and store a copy of its data in "logif" */
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif1_sink", "emac1");
            if (FPP_ERR_OK == rtn)
            {
                demo_log_if_ld_set_promisc(&logif, true);  /* promisc == accept everything */
                demo_log_if_ld_set_discard_on_m(&logif, true);
                demo_log_if_ld_enable(&logif);
            
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
        
        
        /* create and configure a logical interface for ARP ingress traffic */
        /* ---------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif1_arp", "emac1");
            if (FPP_ERR_OK == rtn)
            {
                /* NOTE: 0u == ID of emac0 ; 2u == ID of emac2 */
                demo_log_if_ld_set_promisc(&logif, false);
                demo_log_if_ld_set_egress_phyifs(&logif, ((1uL << 0u) | (1uL << 2u)));
                demo_log_if_ld_set_match_mode_or(&logif, false);
                demo_log_if_ld_clear_all_mr(&logif);
                demo_log_if_ld_set_mr_type_arp(&logif, true);
                demo_log_if_ld_enable(&logif);
                
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
        
        /* create and configure a logical interface for ICMP ingress traffic */
        /* ----------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_log_if_add(p_cl, &logif, "MyLogif1_icmp", "emac1");
            if (FPP_ERR_OK == rtn)
            {
                /* NOTE: 0u == ID of emac0 ; 2u == ID of emac2 */
                demo_log_if_ld_set_promisc(&logif, false);
                demo_log_if_ld_set_egress_phyifs(&logif, ((1uL << 0u) | (1uL << 2u)));
                demo_log_if_ld_set_match_mode_or(&logif, false);
                demo_log_if_ld_clear_all_mr(&logif);
                demo_log_if_ld_set_mr_type_icmp(&logif, true);
                demo_log_if_ld_set_mr_sip(&logif, true, 0x0A0B0005);
                demo_log_if_ld_set_mr_dip(&logif, true, 0x0A070002);
                demo_log_if_ld_enable(&logif);
                
                rtn = demo_log_if_update(p_cl, &logif);
            }
        }
    }
    
    
    /* configure physical interfaces */
    /* ============================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_phy_if_cmd_t phyif = {0};
        
        /* configure physical interface "emac0" */
        /* -----------------------------------  */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "phyif" */
            rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac0");
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_phy_if_ld_enable(&phyif);
                demo_phy_if_ld_set_promisc(&phyif, true);
                demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_FLEXIBLE_ROUTER);
                
                /* update data in PFE */
                rtn = demo_phy_if_update(p_cl, &phyif);
            }
        }
        
        /* configure physical interface "emac1" */
        /* -----------------------------------  */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "phyif" */
            rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac1");
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_phy_if_ld_enable(&phyif);
                demo_phy_if_ld_set_promisc(&phyif, true);
                demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_FLEXIBLE_ROUTER);
                
                /* update data in PFE */
                rtn = demo_phy_if_update(p_cl, &phyif);
            }
        }
    }
    
    
    /* unlock the interface database of PFE */
    rtn = demo_if_session_unlock(p_cl, rtn);
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
