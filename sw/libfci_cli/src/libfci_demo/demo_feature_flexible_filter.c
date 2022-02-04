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
#include "demo_fp.h"
 
 
extern int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl);
 
 
/*
 * @brief      Use libFCI to configure a Flexible Filter in PFE.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs), both in the same network subnet.
 *                   Both PCs are connected through PFE. PFE acts as a simple bridge.
 *               [*] Use libFCI to configure a Flexible Filter on PFE's emac0 physical
 *                   interface, allowing only a specific type of ingress traffic to pass
 *                   for further classification. Non-compliant traffic is discarded.
 *               [*] Criteria for the allowed ingress traffic on PFE's emac0:
 *                     --> Type of the traffic is either ARP or ICMP.
 *                     --> Source IP address is always the IP address of the PC0.
 *                     --> Destination IP address is always the IP address of the PC1.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.3.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Has static ARP entry for PC1.
 *               PC1:
 *                 --> IP address: 10.3.0.5/24
 *                 --> Accessible via PFE's emac1 physical interface.
 *                 --> Has static ARP entry for PC0.
 *             Additional info:
 *               Pseudocode of the comparison process done by this demo's FP table:
 *               [0] r_arp_ethtype : (ethtype != ARP)  ? (GOTO r_icmp_ethtype) : (next_line)
 *               [1] r_arp_sip     : (sip != 10.3.0.2) ? (REJECT)              : (next_line)
 *               [2] r_arp_dip     : (dip == 10.3.0.5) ? (ACCEPT)              : (next_line)
 *               [3] r_arp_discard : (true)            ? (REJECT)              : (REJECT)
 *               [4] r_icmp_ethtype: (ethtype != IPv4) ? (REJECT)              : (next_line)
 *               [5] r_icmp_proto  : (proto != ICMP)   ? (REJECT)              : (next_line)
 *               [6] r_icmp_sip    : (sip != 10.3.0.2) ? (REJECT)              : (next_line)
 *               [7] r_icmp_dip    : (sip == 10.3.0.5) ? (ACCEPT)              : (next_line)
 *               [8] r_icmp_discard: (true)            ? (REJECT)              : (REJECT)
 *              
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Flexible Parser table should be set in PFE.
 *                          Flexible Filter on PFE's emac0 should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_flexible_filter(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* setup PFE to classify traffic (not needed by Flexible Filter, done for demo purposes)*/
    /* ==================================================================================== */
    rtn = demo_feature_L2_bridge_vlan(p_cl);
    
    
    /* create FP rules */
    /* =============== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_fp_rule_cmd_t rule = {0};
        
        /* rule [0] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new rule */
            demo_fp_rule_ld_set_data(&rule, 0x08060000);  /* 0x0806 == EtherType for ARP */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFF0000);
            demo_fp_rule_ld_set_offset(&rule, 12u, FP_OFFSET_FROM_L2_HEADER);
            demo_fp_rule_ld_set_invert(&rule, true);
            demo_fp_rule_ld_set_match_action(&rule, FP_NEXT_RULE, "r_icmp_ethtype");
            
            /* create a new rule in PFE */
            rtn = demo_fp_rule_add(p_cl, "r_arp_ethtype", &rule);
        }
        
        /* rule [1] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x0A030002);  /* ARP protocol: sender IP */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFFFFFF);
            demo_fp_rule_ld_set_offset(&rule, 28u, FP_OFFSET_FROM_L2_HEADER);
            demo_fp_rule_ld_set_invert(&rule, true);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_arp_sip", &rule);
        }
        
        /* rule [2] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x0A030005);  /* ARP protocol: target IP */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFFFFFF);
            demo_fp_rule_ld_set_offset(&rule, 38u, FP_OFFSET_FROM_L2_HEADER);
            demo_fp_rule_ld_set_invert(&rule, false);
            demo_fp_rule_ld_set_match_action(&rule, FP_ACCEPT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_arp_dip", &rule);
        }
        
        /* rule [3] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x00);
            demo_fp_rule_ld_set_mask(&rule, 0x00);
            demo_fp_rule_ld_set_offset(&rule, 0u, FP_OFFSET_FROM_L2_HEADER);
            demo_fp_rule_ld_set_invert(&rule, false);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_arp_discard", &rule);
        }
        
        /* rule [4] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x08000000);  /* 0x0800 == EtherType for IPv4 */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFF0000);
            demo_fp_rule_ld_set_offset(&rule, 12u, FP_OFFSET_FROM_L2_HEADER);
            demo_fp_rule_ld_set_invert(&rule, true);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_icmp_ethtype", &rule);
        }
        
        /* rule [5] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x01000000);  /* 0x01 == ICMP protocol type */
            demo_fp_rule_ld_set_mask(&rule, 0xFF000000);
            demo_fp_rule_ld_set_offset(&rule, 9u, FP_OFFSET_FROM_L3_HEADER);  /* from L3 */
            demo_fp_rule_ld_set_invert(&rule, true);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_icmp_proto", &rule);
        }
        
        /* rule [6] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x0A030002);  /* IP protocol: source IP */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFFFFFF);
            demo_fp_rule_ld_set_offset(&rule, 12u, FP_OFFSET_FROM_L3_HEADER);  /* from L3 */
            demo_fp_rule_ld_set_invert(&rule, true);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_icmp_sip", &rule);
        }
        
        /* rule [7] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x0A030005);  /* IP protocol: destination IP */
            demo_fp_rule_ld_set_mask(&rule, 0xFFFFFFFF);
            demo_fp_rule_ld_set_offset(&rule, 16u, FP_OFFSET_FROM_L3_HEADER);  /* from L3 */
            demo_fp_rule_ld_set_invert(&rule, false);
            demo_fp_rule_ld_set_match_action(&rule, FP_ACCEPT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_icmp_dip", &rule);
        }
        
        /* rule [8] */
        /* -------- */
        if (FPP_ERR_OK == rtn)
        {
            demo_fp_rule_ld_set_data(&rule, 0x00);
            demo_fp_rule_ld_set_mask(&rule, 0x00);
            demo_fp_rule_ld_set_offset(&rule, 0u, FP_OFFSET_FROM_L3_HEADER);
            demo_fp_rule_ld_set_invert(&rule, false);
            demo_fp_rule_ld_set_match_action(&rule, FP_REJECT, NULL);
            
            rtn = demo_fp_rule_add(p_cl, "r_icmp_discard", &rule);
        }
    }
    
    
    /* create (and fill) FP table */
    /* ========================== */
    if (FPP_ERR_OK == rtn)
    {
        /* create FP table */
        /* --------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_add(p_cl, "my_filter_table");
        }
        
        /* fill the table with rules */
        /* ------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_arp_ethtype",  0u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_arp_sip", 1u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_arp_dip", 2u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_arp_discard", 3u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_icmp_ethtype", 4u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_icmp_proto", 5u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_icmp_sip", 6u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_icmp_dip", 7u);
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = demo_fp_table_insert_rule(p_cl, "my_filter_table", "r_icmp_discard", 8u);
        }
    }
    
    
    /* assign the created FP table as a Flexible Filter for emac0 */
    /* ========================================================== */
    if (FPP_ERR_OK == rtn)
    {
        /* lock the interface database of PFE */
        rtn = demo_if_session_lock(p_cl);
        if (FPP_ERR_OK == rtn)
        {
            fpp_phy_if_cmd_t phyif = {0};
            
            /* get data from PFE and store them in the local variable "phyif" */
            rtn = demo_phy_if_get_by_name(p_cl, &phyif, "emac0");
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_phy_if_ld_set_flexifilter(&phyif, "my_filter_table");
            
                /* update data in PFE */
                rtn = demo_phy_if_update(p_cl, &phyif);
            }
        }
        
        /* unlock the interface database of PFE */
        rtn = demo_if_session_unlock(p_cl, rtn);
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
