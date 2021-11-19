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
#include "demo_spd.h"
 
 
/*
 * @brief      Use libFCI to configure PFE IPsec support.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs):
 *                     --> PC0, which uses encrypted communication.
 *                     --> PC1, which uses unencrypted communication.
 *               [*] Use libFCI to configure PFE IPsec support, allowing ICMP (ping) and 
 *                   TCP (port 4000) communication between PC0 and PC1.
 *                     --> Traffic from PC0 should be decrypted by PFE, then sent to PC1.
 *                     --> Traffic from PC1 should be encrypted by PFE, then sent to PC0.
 *               [*] NOTE:
 *                   To fully enable PFE IPsec support, it is required to configure 
 *                   the underlying HSE (Hardware Security Engine). HSE configuration 
 *                   is not done by the FCI API and is outside the scope of this demo.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.7.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *                 --> Configured to send 10.11.0.0 traffic to PFE's emac0.
 *                 --> Requires IPsec-encrypted communication.
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
 *                          IPsec support should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_spd(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* configure SPD database entries on emac0 */
    /* ======================================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_spd_cmd_t spd = {0};
        uint32_t src_ip[4] = {0};
        uint32_t dst_ip[4] = {0};
        
        /* create SPD entry for ICMP traffic (ping) from PC0 to PC1 */
        /* -------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new SPD entry */
            /* SPI passed in the demo_spd_ld_set_action() should be known by HSE */
            src_ip[0] = 0x0A070002;
            dst_ip[0] = 0x0A0B0005;
            demo_spd_ld_set_protocol(&spd, 1u);  /* 1 == ICMP */ 
            demo_spd_ld_set_ip(&spd, src_ip, dst_ip, false);
            demo_spd_ld_set_port(&spd, false, 0u, false, 0u);
            demo_spd_ld_set_action(&spd, FPP_SPD_ACTION_PROCESS_DECODE, 0u, 0x11335577);
            
            /* create a new SPD entry in PFE */
            rtn = demo_spd_add(p_cl, "emac0", 0u, &spd);
        }
        
        /* create SPD entry for TCP traffic from PC0 to PC1 */
        /* ------------------------------------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new SPD entry */
            /* SPI passed in the demo_spd_ld_set_action() should be known by HSE */
            src_ip[0] = 0x0A070002;
            dst_ip[0] = 0x0A0B0005;
            demo_spd_ld_set_protocol(&spd, 6u);  /* 6 == TCP */ 
            demo_spd_ld_set_ip(&spd, src_ip, dst_ip, false);
            demo_spd_ld_set_port(&spd, true, 4000u, true, 4000u);
            demo_spd_ld_set_action(&spd, FPP_SPD_ACTION_PROCESS_DECODE, 0u, 0x22446688);
            
            /* create a new SPD entry in PFE */
            rtn = demo_spd_add(p_cl, "emac0", 1u, &spd);
        }
    }
    
    
    /* configure SPD database entries on emac1 */
    /* ======================================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_spd_cmd_t spd = {0};
        uint32_t src_ip[4] = {0};
        uint32_t dst_ip[4] = {0};
        
        /* create SPD entry for ICMP traffic (ping) from PC1 to PC0 */
        /* -------------------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new SPD entry */
            /* SA_ID passed in the demo_spd_ld_set_action() should be 
               a valid index to some SAD entry in HSE */
            src_ip[0] = 0x0A0B0005;
            dst_ip[0] = 0x0A070002;
            demo_spd_ld_set_protocol(&spd, 1u);  /* 1 == ICMP */ 
            demo_spd_ld_set_ip(&spd, src_ip, dst_ip, false);
            demo_spd_ld_set_port(&spd, false, 0u, false, 0u);
            demo_spd_ld_set_action(&spd, FPP_SPD_ACTION_PROCESS_ENCODE, 1u, 0u);
            
            /* create a new SPD entry in PFE */
            rtn = demo_spd_add(p_cl, "emac1", 0u, &spd);
        }
        
        /* create SPD entry for TCP traffic from PC1 to PC0 */
        /* ------------------------------------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new SPD entry */
            /* SA_ID passed in the demo_spd_ld_set_action() should be 
               a valid index to some SAD entry in HSE */
            src_ip[0] = 0x0A0B0005;
            dst_ip[0] = 0x0A070002;
            demo_spd_ld_set_protocol(&spd, 6u);  /* 6 == TCP */ 
            demo_spd_ld_set_ip(&spd, src_ip, dst_ip, false);
            demo_spd_ld_set_port(&spd, true, 4000u, true, 4000u);
            demo_spd_ld_set_action(&spd, FPP_SPD_ACTION_PROCESS_ENCODE, 2u, 0);
            
            /* create a new SPD entry in PFE */
            rtn = demo_spd_add(p_cl, "emac1", 1u, &spd);
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_DEFAULT);
                
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
                    demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_DEFAULT);
                    
                    /* update data in PFE */
                    rtn = demo_phy_if_update(p_cl, &phyif);
                }
            }
            
            /* configure physical interface "util" */
            /* ----------------------------------- */
            /* This interface represents interaction between PFE and HSE.
               This example configures util in Flexible Router mode to allow for distribution
               of the traffic which arrives from HSE. */
            if (FPP_ERR_OK == rtn)
            {
                fpp_log_if_cmd_t logif = {0};
                fpp_phy_if_cmd_t phyif = {0};
                
                /* create and configure a logical interface for traffic from PC0 to PC1 */
                /* -------------------------------------------------------------------- */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_log_if_add(p_cl, &logif, "From-PC0_to-PC1", "util");
                    if (FPP_ERR_OK == rtn)
                    {
                        /* NOTE: 1u == ID of emac1 */
                        demo_log_if_ld_set_promisc(&logif, false);
                        demo_log_if_ld_set_egress_phyifs(&logif, (1uL << 1u));
                        demo_log_if_ld_set_match_mode_or(&logif, false);
                        demo_log_if_ld_clear_all_mr(&logif);
                        demo_log_if_ld_set_mr_sip(&logif, true, 0x0A070002);
                        demo_log_if_ld_set_mr_dip(&logif, true, 0x0A0B0005);
                        demo_log_if_ld_enable(&logif);
                        
                        rtn = demo_log_if_update(p_cl, &logif);
                    }
                }
                
                /* create and configure a logical interface for traffic from PC1 to PC0 */
                /* -------------------------------------------------------------------- */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_log_if_add(p_cl, &logif, "From-PC1_to-PC0", "util");
                    if (FPP_ERR_OK == rtn)
                    {
                        /* NOTE: 0u == ID of emac0 */
                        demo_log_if_ld_set_promisc(&logif, false);
                        demo_log_if_ld_set_egress_phyifs(&logif, (1uL << 0u));
                        demo_log_if_ld_set_match_mode_or(&logif, false);
                        demo_log_if_ld_clear_all_mr(&logif);
                        demo_log_if_ld_set_mr_sip(&logif, true, 0x0A0B0005);
                        demo_log_if_ld_set_mr_dip(&logif, true, 0x0A070002);
                        demo_log_if_ld_enable(&logif);
                        
                        rtn = demo_log_if_update(p_cl, &logif);
                    }
                }
                
                /* configure physical interface "util" */
                /* ----------------------------------- */
                if (FPP_ERR_OK == rtn)
                {
                    /* get data from PFE and store them in the local variable "phyif" */
                    rtn = demo_phy_if_get_by_name(p_cl, &phyif, "util");
                    if (FPP_ERR_OK == rtn)
                    {
                        /* modify locally stored data */
                        demo_phy_if_ld_enable(&phyif);
                        demo_phy_if_ld_set_promisc(&phyif, false);
                        demo_phy_if_ld_set_mode(&phyif, FPP_IF_OP_FLEXIBLE_ROUTER);
                        
                        /* update data in PFE */
                        rtn = demo_phy_if_update(p_cl, &phyif);
                    }
                }
            }
        }
        
        /* unlock the interface database of PFE */
        rtn = demo_if_session_unlock(p_cl, rtn);
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
