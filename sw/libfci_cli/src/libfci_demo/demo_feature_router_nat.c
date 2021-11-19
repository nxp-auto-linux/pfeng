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
 * @brief      Use libFCI to configure PFE as a router (with one-to-many NAT).
 * @details    Scenario description:
 *               [*] Let there be three computers (PCs):
 *                     --> PC0_20, which acts as a server
 *                     --> PC1_2, which acts as a client
 *                     --> PC1_5, which acts as a client
 *               [*] Use libFCI to configure PFE as a router (with one-to-many NAT), allowing 
 *                   TCP communication between the server PC and client PCs.
 *               [*] Client PCs can communicate with the server PC via TCP port 4000.
 *                   This scenario requires both source and destination port to be 4000.
 *                   (no use of ephemeral ports)
 *               [*] PC0_20 (server) has a public IP address (200.201.202.20/16).
 *               [*] PC1_2 and PC1_5 (clients) have private IP addresses from 10.x.x.x range.
 *                   They both share one public IP address (100.101.102.10/16) to communicate
 *                   with the outside world (NAT+PAT "one-to-many" mapping).
 *             PC description:
 *               PC0_20 (server):
 *                 --> IP address:  200.201.202.20/16
 *                 --> MAC address: 0A:BB:CC:DD:EE:FF  
 *                     (this is just a demo MAC; real MAC of the real PC0 should be used)
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Configured to send 100.101.0.0 traffic to PFE's emac0.
 *                 --> Listens on TCP port 4000.
 *               PC1_2 (client_2):
 *                 --> IP address:  10.11.0.2/24
 *                 --> MAC address: 0A:11:33:55:77:99
 *                     (this is just a demo MAC; real MAC of the real PC1_2 should be used)
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Configured to send 200.201.0.0 traffic to PFE's emac1.
 *                 --> Hidden behind NAT.
 *               PC1_5 (client_5):
 *                 --> IP address: 10.11.0.5/24
 *                 --> MAC address: 0A:22:44:66:88:AA
 *                     (this is just a demo MAC; real MAC of the real PC1_5 should be used)
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Configured to send 200.201.0.0 traffic to PFE's emac1.
 *                 --> Hidden behind NAT.
 *             Additional info:
 *               [+] Conntrack struct has data members for an "orig" direction and for
 *                   a "reply" direction. See FPP_CMD_IPV4_CONNTRACK.
 *                   The "reply" direction data can be used for two purposes:
 *                     - To automatically create a reply direction conntrack together with
 *                       the orig direction conntrack in one FCI command.
 *                     - To modify parts of the "orig" direction packet (IPs/ports), 
 *                       effectively creating NAT/PAT behavior.
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
int demo_feature_router_nat(FCI_CLIENT* p_cl)
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
        
        /* route 20 (route to PC0_20) */
        /* -------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x0A,0xBB,0xCC,0xDD,0xEE,0xFF});
            demo_rt_ld_set_egress_phyif(&rt, "emac0");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 20uL, &rt);
        }
        
        /* route 2 (route to PC1_2) */
        /* ------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x0A,0x11,0x33,0x55,0x77,0x99});
            demo_rt_ld_set_egress_phyif(&rt, "emac1");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 2uL, &rt);
        }
        
        /* route 5 (route to PC1_5) */
        /* ------------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x0A,0x22,0x44,0x66,0x88,0xAA});
            demo_rt_ld_set_egress_phyif(&rt, "emac1");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 5uL, &rt);
        }
    }
    
    
    /* set timeout for conntracks (not necessary; done for demo purposes) */
    /* ================================================================== */
    if (FPP_ERR_OK == rtn)
    {
        demo_ct_timeout_tcp(p_cl, 0xFFFFFFFFuL);
    }
    
    
    /* create conntracks between PC1_2 (client_2) and PC0_20 (server) */
    /* ============================================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_ct_cmd_t ct = {0};
        
        /* from PC1_2 (client_2) to PC0_20 (server) */
        /* ---------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            /*  This conntrack is configured as an unidirectional NAT/PAT conntrack.
                FCI command to create this conntrack results in one connection being
                created in PFE - a connection from PC1_2 to PC0_20 ("orig" direction only).
                Packets routed by this conntrack are modified by PFE as follows:
                    --> Source IP of the routed packet is replaced with the conntrack's 
                        "reply" dir destination IP address (NAT behavior).
                    --> Source port of the routed packet is replaced with the conntrack's
                        "reply" dir destination port (PAT behavior).
            */
            demo_ct_ld_set_protocol(&ct, 6u);  /* 6 == TCP */
            demo_ct_ld_set_orig_dir(&ct, 0x0A0B0003u,0xC8C9CA14u,4000u,4000u, 0u,20uL, true);
            demo_ct_ld_set_reply_dir(&ct,0xC8C9CA14u,0x6465660Au,4000u,40003u,0u, 0uL, false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
        }
        
        /* from PC0_20 (server) back to PC1_2 (client_2) */
        /* --------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            /*  This conntrack is a complement to the previous one - it represents
                connection from PC0_20 back to PC1_2.
                Notice that this conntrack translates source IP / source port of 
                the routed packet back to the values expected by the PC1_2.
            */
            demo_ct_ld_set_protocol(&ct, 6u);  /* 6 == TCP */
            demo_ct_ld_set_orig_dir(&ct, 0xC8C9CA14u,0x6465660Au,4000u,40003u,0u,2uL, true);
            demo_ct_ld_set_reply_dir(&ct,0x0A0B0003u,0xC8C9CA14u,4000u,4000u, 0u,0uL, false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
        }
    }
    
    
    /* create conntracks between PC1_5 (client_5) and PC0_20 (server) */
    /* ============================================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_ct_cmd_t ct = {0};
        
        /* from PC1_5 (client_5) to PC0_20 (server) */
        /* ---------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            demo_ct_ld_set_protocol(&ct, 6u);  /* 6 == TCP */
            demo_ct_ld_set_orig_dir(&ct, 0x0A0B0005u,0xC8C9CA14u,4000u,4000u, 0u,20uL, true);
            demo_ct_ld_set_reply_dir(&ct,0xC8C9CA14u,0x6465660Au,4000u,40005u,0u, 0uL, false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
        }
        
        /* from PC0_20 (server) back to PC1_5 (client_5) */
        /* --------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            demo_ct_ld_set_protocol(&ct, 6u);  /* 6 == TCP */
            demo_ct_ld_set_orig_dir(&ct, 0xC8C9CA14u,0x6465660Au,4000u,40005u,0u,5uL, true);
            demo_ct_ld_set_reply_dir(&ct,0x0A0B0005u,0xC8C9CA14u,4000u,4000u, 0u,0uL, false);
            
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
 
