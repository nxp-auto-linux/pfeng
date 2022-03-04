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
 * @brief      Use libFCI to configure PFE as a VLAN-aware L2 bridge.
 * @details    Scenario description:
 *               [*] Let there be six computers (PCs):
 *                     --> Three PCs (PC0_NOVLAN, PC0_100 and PC0_200) are accessible via
 *                         PFE's emac0 physical interface.
 *                     --> Three PCs (PC1_NOVLAN, PC1_100 and PC1_200) are accessible via
 *                         PFE's emac1 physical interface.
 *               [*] Use libFCI to configure PFE as a VLAN-aware L2 bridge, allowing the PCs 
 *                   to communicate as follows:
 *                     --> PC0_NOVLAN and PC1_NOVLAN  (untagged traffic)
 *                     --> PC0_100 and PC1_100        (VLAN 100 tagged traffic)
 *                     --> PC0_200 and PC1_200        (VLAN 200 tagged traffic)
 *               [*] Additional requirements:
 *                     --> Dynamic learning of MAC addresses shall be disabled on 
 *                         emac0 and emac1 interfaces.
 *                     --> In VLAN 200 domain, a replica of all passing traffic shall be sent 
 *                         to a host.
 *             PC description:
 *               PC0_NOVLAN
 *                 --> IP  address: 10.3.0.2/24
 *                 --> MAC address: 0A:01:23:45:67:89
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Sends untagged traffic
 *               PC1_NOVLAN
 *                 --> IP  address: 10.3.0.5/24
 *                 --> MAC address: 0A:FE:DC:BA:98:76
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Sends untagged traffic
 *               PC0_100:
 *                 --> IP  address: 10.100.0.2/24
 *                 --> MAC address: 02:11:22:33:44:55
 *                 --> Accessible via PFE's emac0 physical interface.
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
 *                 --> Belongs to VLAN 200 domain.
 *                 
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *             
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          VLAN-aware L2 bridge should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
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
        
        /* Default BD (VLAN == 1) */
        /* ---------------------- */
        /* This bridge domain already exists (automatically created at driver startup). */
        /* It is used by PFE to process untagged traffic. */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "bd" */
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

        /* static entry for bridge domain 1 (MAC of PC0_NOVLAN) */
        /* ---------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 1u,
                                    (uint8_t[6]){0x0A,0x01,0x23,0x45,0x67,0x89});

            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 0u == ID of emac0 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 0u));

                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }

        /* static entry for bridge domain 1 (MAC of PC1_NOVLAN) */
        /* ---------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* create a new static entry in PFE */
            rtn = demo_l2_stent_add(p_cl, &stent, 1u,
                                    (uint8_t[6]){0x0A,0xFE,0xDC,0xBA,0x98,0x76});

            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data of the new static entry */
                /* 1u == ID of emac1 */
                demo_l2_stent_ld_set_fwlist(&stent, (1uL << 1u));

                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
        }

        
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
                /* 0u == ID of emac0 ; 7u == hif1 */
                demo_l2_stent_ld_set_fwlist(&stent, ((1uL << 0u) | (1uL << 7u)));
            
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
                /* 1u == ID of emac1 ; 7u == hif1 */
                demo_l2_stent_ld_set_fwlist(&stent, ((1uL << 1u) | (1uL << 7u)));
            
                /* update the new static entry in PFE */
                rtn = demo_l2_stent_update(p_cl, &stent);
            }
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_VLAN_BRIDGE);
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_VLAN_BRIDGE);
                    demo_phy_if_ld_set_block_state(&phyif, BS_FORWARD_ONLY);
                    
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
        }
        
        /* unlock the interface database of PFE */
        rtn = demo_if_session_unlock(p_cl, rtn);
    }
    
    
    /* clear dynamic (learned) entries from L2 bridge MAC table */
    /* ========================================================= */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_flush_learned(p_cl);
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
