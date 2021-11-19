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
#include "demo_rt_ct.h"
 
 
/*
 * @brief      Use libFCI to configure PFE as VLAN-aware L2L3 bridge.
 * @details    Scenario description:
 *               [*] Let there be four computers (PCs):
 *                     --> Two PCs (PC0_100 and PC0_200) are accessible via
 *                         PFE's emac0 physical interface.
 *                     --> Two PCs (PC1_100 and PC1_200) are accessible via
 *                         PFE's emac1 physical interface.
 *               [*] Use libFCI to configure PFE as VLAN-aware L2L3 bridge, allowing 
 *                   communication between the PCs as follows:
 *                     --> PC0_100 and PC1_100 are both in the VLAN domain 100.
 *                         PFE shall operate as a VLAN-aware L2 bridge, allowing communication 
 *                         between these two PCs.
 *                     --> PC0_200 and PC1_200 are both in the VLAN domain 200.
 *                         PFE shall operate as a VLAN-aware L2 bridge, allowing communication 
 *                         between these two PCs.
 *                     --> PC0_100 and PC1_200 are in different VLAN domains.
 *                         PFE shall operate as a router, allowing ICMP (ping) and 
 *                         TCP (port 4000) communication between these two PCs.
 *               [*] Additional requirements:
 *                     --> Dynamic learning of MAC addresses shall be disabled on 
 *                         emac0 and emac1 interfaces.
 *             PFE emac description:
 *               emac0:
 *                 --> MAC address: 00:01:BE:BE:EF:11
 *               emac1:
 *                 --> MAC address: 00:01:BE:BE:EF:22
 *             PC description:
 *               PC0_100:
 *                 --> IP  address: 10.100.0.2/24
 *                 --> MAC address: 02:11:22:33:44:55
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Configured to send 10.200.0.0 traffic to PFE's emac0.
 *                 --> Belongs to VLAN 100 domain.
 *               PC1_100:
 *                 --> IP  address: 10.100.0.5/24
 *                 --> MAC address: 02:66:77:88:99:AA
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Belongs to VLAN 100 domain.
 *               PC0_200:
 *                 --> IP  address: 10.200.0.2/24
 *                 --> MAC address: 06:CC:BB:AA:99:88
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Belongs to VLAN 200 domain.
 *               PC1_200:
 *                 --> IP  address: 10.200.0.5/24
 *                 --> MAC address: 06:77:66:55:44:33
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Configured to send 10.100.0.0 traffic to PFE's emac1.
 *                 --> Belongs to VLAN 200 domain.
 *              
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          L2L3 bridge should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_L2L3_bridge_vlan(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /*
        configure VLAN-aware L2 bridge
    */
    
    
    /* clear L2 bridge MAC table (not required; done for demo purposes) */
    /* ================================================================ */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_flush_all(p_cl);
    }
    
    
    /* create and configure bridge domains */
    /* =================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_l2_bd_cmd_t bd = {0};
        
        /* bridge domain 100 */
        /* ----------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new bridge domain in PFE */
            rtn = demo_l2_bd_add(p_cl, &bd, 100u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new domain */
                demo_l2_bd_ld_insert_phyif(&bd, 0u, true);  /* 0u == ID of emac0 */
                demo_l2_bd_ld_insert_phyif(&bd, 1u, true);  /* 1u == ID of emac1 */
                demo_l2_bd_ld_set_ucast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
                demo_l2_bd_ld_set_ucast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
                demo_l2_bd_ld_set_mcast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
                demo_l2_bd_ld_set_mcast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
                
                /* update the new bridge domain in PFE */
                rtn = demo_l2_bd_update(p_cl, &bd);
            }
        }
        
        /* bridge domain 200 */
        /* ----------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new bridge domain in PFE */
            rtn = demo_l2_bd_add(p_cl, &bd, 200u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new domain */
                demo_l2_bd_ld_insert_phyif(&bd, 0u, true);  /* 0u == ID of emac0 */
                demo_l2_bd_ld_insert_phyif(&bd, 1u, true);  /* 1u == ID of emac1 */
                demo_l2_bd_ld_set_ucast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
                demo_l2_bd_ld_set_ucast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
                demo_l2_bd_ld_set_mcast_hit(&bd, 0u);   /* 0u == bridge action "FORWARD" */
                demo_l2_bd_ld_set_mcast_miss(&bd, 1u);  /* 1u == bridge action "FLOOD" */
                
                /* update the new bridge domain in PFE */
                rtn = demo_l2_bd_update(p_cl, &bd);
            }
        }
    }
    
    
    /* create and configure static MAC table entries */
    /* ============================================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_l2_static_ent_cmd_t stent = {0};
        
        /* static entry for bridge domain 100 (MAC of PC0_100) */
        /* --------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 100u, 
                                    (uint8_t[6]){0x02,0x11,0x22,0x33,0x44,0x55});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 0u == ID of emac0 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 0u));
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* static entry for bridge domain 100 (MAC of PC1_100) */
        /* --------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 100u, 
                                    (uint8_t[6]){0x02,0x66,0x77,0x88,0x99,0xAA});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 1u == ID of emac1 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 1u));
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* static entry for bridge domain 200 (MAC of PC0_200) */
        /* --------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 200u, 
                                    (uint8_t[6]){0x06,0xCC,0xBB,0xAA,0x99,0x88});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 0u == ID of emac0 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 0u));
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* static entry for bridge domain 200 (MAC of PC1_200) */
        /* --------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 200u, 
                                    (uint8_t[6]){0x06,0x77,0x66,0x55,0x44,0x33});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 1u == ID of emac1 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 1u));
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
    }
    
    
    /* create special 'local' static MAC table entries (required for L2L3 bridge) */
    /* ========================================================================== */
    /* 'local' static MAC table entries are used to select the traffic which should be
       classified by the Router. The rest of the traffic is classified by the L2 bridge. */
    if (FPP_ERR_OK == rtn)
    {
        fpp_l2_static_ent_cmd_t stent = {0};
        
        /* [vlan 100] ; if traffic destination MAC == MAC of emac0, then pass it to Router */
        /* ------------------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 100u, 
                                    (uint8_t[6]){0x00,0x01,0xBE,0xBE,0xEF,0x11});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                demo_l2_stent_ld_set_local(&stent, true);
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* [vlan 100] ; if traffic destination MAC == MAC of emac1, then pass it to Router */
        /* ------------------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 100u, 
                                    (uint8_t[6]){0x00,0x01,0xBE,0xBE,0xEF,0x22});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                demo_l2_stent_ld_set_local(&stent, true);
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* [vlan 200] ; if traffic destination MAC == MAC of emac0, then pass it to Router */
        /* ------------------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 200u, 
                                    (uint8_t[6]){0x00,0x01,0xBE,0xBE,0xEF,0x11});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                demo_l2_stent_ld_set_local(&stent, true);
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
        
        /* [vlan 200] ; if traffic destination MAC == MAC of emac1, then pass it to Router */
        /* ------------------------------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 200u, 
                                    (uint8_t[6]){0x00,0x01,0xBE,0xBE,0xEF,0x22});
            
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                demo_l2_stent_ld_set_local(&stent, true);
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }
    }
    
    
    /*
        configure router
    */
    
    
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
        
        /* route 10 (route to PC0_100) */
        /* --------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x02,0x11,0x22,0x33,0x44,0x55});
            demo_rt_ld_set_egress_phyif(&rt, "emac0");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 10uL, &rt);
        }
        
        /* route 20 (route to PC1_200) */
        /* --------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new route */
            demo_rt_ld_set_as_ip4(&rt);
            demo_rt_ld_set_dst_mac(&rt, (const uint8_t[6]){0x06,0x77,0x66,0x55,0x44,0x33});
            demo_rt_ld_set_egress_phyif(&rt, "emac1");
            
            /* create a new route in PFE */
            rtn = demo_rt_add(p_cl, 20uL, &rt);
        }
    }
    
    
    /* set timeout for conntracks (not necessary; done for demo purposes) */
    /* ================================================================== */
    if (FPP_ERR_OK == rtn)
    {
        demo_ct_timeout_others(p_cl, 0xFFFFFFFFuL);  /* ping is ICMP, that is 'others' */
        demo_ct_timeout_tcp(p_cl, 0xFFFFFFFFuL);  
    }
    
    
    /* create conntracks */
    /* ================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_ct_cmd_t ct = {0};
        
        /* ICMP conntrack from PC0_100 to PC1_200 (and back) */
        /* ------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            /*      This conntrack is configured as a bi-directional conntrack.
                    One FCI command results in two connections being created in PFE -
                    one for the "orig" direction and one for the "reply" direction. 
                    This conntrack also modifies VLAN tag of the routed packet.
            */
            demo_ct_ld_set_protocol(&ct, 1u);  /* 1 == ICMP */
            demo_ct_ld_set_orig_dir(&ct, 0x0A640002u,0x0AC80005u,0u,0u,200u,20uL,false);
            demo_ct_ld_set_reply_dir(&ct,0x0AC80005u,0x0A640002u,0u,0u,100u,10uL,false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
        }
        
        /* TCP conntrack from PC0_100 to PC1_200 (and back) */
        /* ------------------------------------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new conntrack */
            /*      This conntrack is configured as a bi-directional conntrack.
                    One FCI command results in two connections being created in PFE -
                    one for the "orig" direction and one for the "reply" direction. 
                    This conntrack also modifies VLAN tag of the routed packet.
            */
            demo_ct_ld_set_protocol(&ct, 6u);  /* 6 == TCP */
            demo_ct_ld_set_orig_dir(&ct, 0x0A640002u,0x0AC80005u,4000u,4000u,200u,20uL,false);
            demo_ct_ld_set_reply_dir(&ct,0x0AC80005u,0x0A640002u,4000u,4000u,100u,10uL,false);
            
            /* create a new conntrack in PFE */
            rtn = demo_ct_add(p_cl, &ct);
        }
    }
    
    
    /*
        configure physical interfaces
    */
    
    
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_L2L3_VLAN_BRIDGE);
                    demo_phy_if_ld_set_block_state(&phyif, BS_FORWARD_ONLY);
                
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_L2L3_VLAN_BRIDGE);
                    demo_phy_if_ld_set_block_state(&phyif, BS_FORWARD_ONLY);
                    
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
 
