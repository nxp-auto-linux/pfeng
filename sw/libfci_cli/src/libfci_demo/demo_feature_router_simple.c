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
#include "demo_rt_ct.h"
 
 
/*
 * @brief      Use libFCI to configure PFE as a simple router.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs): PC0_7 and PC1_11.
 *                   Each PC is in a different network subnet.
 *               [*] Use libFCI to configure PFE as a simple router, allowing ICMP (ping)
 *                   communication between PC0_7 and PC1_11.
 *             PC description:
 *               PC0_7:
 *                 --> IP  address: 10.7.0.2/24
 *                 --> MAC address: 0A:01:23:45:67:89
 *                     (this is just a demo MAC; real MAC of the real PC0_7 should be used)
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Configured to send 10.11.0.0 traffic to PFE's emac0.
 *               PC1_11:
 *                 --> IP  address: 10.11.0.5/24
 *                 --> MAC address: 0A:FE:DC:BA:98:76
 *                     (this is just a demo MAC; real MAC of the real PC1_11 should be used)
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
 *                          Router should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_router_simple(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* clear all IPv4 routes and conntracks in PFE (not necessary, done for demo purposes) */
    /* =================================================================================== */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_rtct_reset_ip4(p_cl);
    }
    
    
    /* create routes */
    /* ============= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_rt_cmd_t rt = {0};
        
        /* route 7 (route to PC0_7) */
        /* ------------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x0A,0x01,0x23,0x45,0x67,0x89});
            demo_rt_ld_set_egress_phyif(&rt, "emac0");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 7uL, &rt);
        }
        
        /* route 11 (route to PC1_11) */
        /* -------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x0A,0xFE,0xDC,0xBA,0x98,0x76});
            demo_rt_ld_set_egress_phyif(&rt, "emac1");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 11uL, &rt);
        }
    }
    
    
    /* set timeout for conntracks (not necessary; done for demo purposes) */
    /* ================================================================== */
    if (FPP_ERR_OK == rtn)
    {
        demo_ct_timeout_others(p_cl, 0xFFFFFFFFuL);  /* ping is ICMP, that is 'others' */
    }
    
    
    /* create conntracks */
    /* ================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_ct_cmd_t ct = {0};
        
        /* conntrack from PC0_7 to PC1_11 (and back) */
        /* ----------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            /*  This conntrack is configured as a bi-directional conntrack.
                FCI command to create this conntrack results in two connections being 
                created in PFE:
                    --> one for the "orig" direction
                    --> one for the "reply" direction
            */
            demo_ct_ld_set_protocol(&ct, 1u);  /* 1 == ICMP */
            demo_ct_ld_set_orig_dir(&ct,  0x0A070002u,0x0A0B0005u,0u,0u, 0u,11uL, false);
            demo_ct_ld_set_reply_dir(&ct, 0x0A0B0005u,0x0A070002u,0u,0u, 0u, 7uL, false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
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
                    demo_phy_if_ld_set_promisc(&phyif, false);
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_ROUTER);
                
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
                    demo_phy_if_ld_set_promisc(&phyif, false);
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_ROUTER);
                    
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
 
