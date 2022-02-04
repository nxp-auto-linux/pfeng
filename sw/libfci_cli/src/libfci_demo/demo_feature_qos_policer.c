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
#include <string.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_phy_if.h"
#include "demo_qos_pol.h"
 
 
extern int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl);
 
 
/*
 * @brief      Use libFCI to configure PFE ingress QoS feature.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs), both in the same network subnet.
 *                   Both PCs are connected through PFE. PFE acts as a simple bridge.
 *               [*] Use libFCI to configure PFE ingress QoS feature on PFE's emac0 physical
 *                   interface, to prioritize and shape ingress communication on emac0.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.3.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *               PC1:
 *                 --> IP address: 10.3.0.5/24
 *                 --> Accessible via PFE's emac1 physical interface.
 *             Additional info (parameters of emac0 ingress QoS policing):
 *               [+] Ingressing ARP traffic shall be classified as Managed.
 *               [+] Ingressing IPv4 TCP traffic from PC0 IP shall be classified as Reserved.
 *               [+] Ingressing IPv4 UDP traffic (from any source) shall be dropped.
 *               [+] One WRED queue is required, with maximal depth of 255 and with linear
 *                   rise of drop probability for Unmanaged traffic.
 *               [+] One port-level shaper is required.
 *
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Ingress QoS policer should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_qos_policer(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* setup PFE to classify traffic (not needed by Egress QoS, done for demo purposes)*/
    /* =============================================================================== */
    rtn = demo_feature_L2_bridge_vlan(p_cl);
    
    /* enable Ingress QoS policer on emac0 */
    /* =================================== */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_pol_enable(p_cl, "emac0", true);
    }
    
    /* configure Ingress QoS flows for emac0 */
    /* ===================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_policer_flow_cmd_t polflow = {0};
        
        /* flow 0 - ARP traffic shall be Managed */
        /* ------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new flow */
            memset(&polflow, 0, sizeof(fpp_qos_policer_flow_cmd_t));
            demo_polflow_ld_set_m_type_arp(&polflow, true);
            demo_polflow_ld_set_action(&polflow, FPP_IQOS_FLOW_MANAGED);
            
            /* create a new flow in PFE */
            rtn = demo_polflow_add(p_cl, "emac0", 0u, &polflow);
        }
        
        /* flow 1 - specific TCP traffic shall be Reserved */
        /* ----------------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new flow */
            memset(&polflow, 0, sizeof(fpp_qos_policer_flow_cmd_t));
            demo_polflow_ld_set_m_type_ip4(&polflow, true);
            demo_polflow_ld_set_am_proto(&polflow, true, 6u, FPP_IQOS_L4PROTO_MASK);
            demo_polflow_ld_set_am_sip(&polflow, true, 0x0A030002, FPP_IQOS_SDIP_MASK);
            demo_polflow_ld_set_action(&polflow, FPP_IQOS_FLOW_RESERVED);
            
            /* create a new flow in PFE */
            rtn = demo_polflow_add(p_cl, "emac0", 1u, &polflow);
        }
        
        /* flow 2 - UDP traffic shall be dropped */
        /* ------------------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* locally prepare data for a new flow */
            memset(&polflow, 0, sizeof(fpp_qos_policer_flow_cmd_t));
            demo_polflow_ld_set_am_proto(&polflow, true, 17u, FPP_IQOS_L4PROTO_MASK);
            demo_polflow_ld_set_action(&polflow, FPP_IQOS_FLOW_DROP);
            
            /* create a new flow in PFE */
            rtn = demo_polflow_add(p_cl, "emac0", 2u, &polflow);
        }
    }
    
    
    /* configure Ingress QoS WRED queues for emac0 */
    /* =========================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_policer_wred_cmd_t polwred = {0};
        
        /* WRED queue LMEM */
        /* --------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "polwred" */
            rtn = demo_polwred_get_by_que(p_cl, &polwred, "emac0", FPP_IQOS_Q_LMEM);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_polwred_ld_enable(&polwred, true);
                demo_polwred_ld_set_min(&polwred, 0u);
                demo_polwred_ld_set_max(&polwred, 200u);  /* over 200 == drop all Unmanaged */
                demo_polwred_ld_set_full(&polwred, 255u); /* over 255 == drop everything */
                demo_polwred_ld_set_zprob(&polwred, FPP_IQOS_WRED_ZONE1,  0u);
                demo_polwred_ld_set_zprob(&polwred, FPP_IQOS_WRED_ZONE2, 30u);
                demo_polwred_ld_set_zprob(&polwred, FPP_IQOS_WRED_ZONE3, 60u);
                demo_polwred_ld_set_zprob(&polwred, FPP_IQOS_WRED_ZONE4, 90u);
                
                /* update data in PFE */
                rtn = demo_polwred_update(p_cl, &polwred);
            }
        }
        
        /* WRED queue DMEM (disabled) */
        /* -------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "polwred" */
            rtn = demo_polwred_get_by_que(p_cl, &polwred, "emac0", FPP_IQOS_Q_DMEM);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_polwred_ld_enable(&polwred, false);
                
                /* update data in PFE */
                rtn = demo_polwred_update(p_cl, &polwred);
            }
        }
        
        /* WRED queue RXF (disabled) */
        /* ------------------------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "polwred" */
            rtn = demo_polwred_get_by_que(p_cl, &polwred, "emac0", FPP_IQOS_Q_RXF);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_polwred_ld_enable(&polwred, false);
                
                /* update data in PFE */
                rtn = demo_polwred_update(p_cl, &polwred);
            }
        }
    }
    
    
    /* configure Ingress QoS shapers for emac0 */
    /* ======================================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_policer_shp_cmd_t polshp = {0};
        
        /* Ingress QoS shaper 0 */
        /* -------------------- */
        rtn = demo_polshp_get_by_id(p_cl, &polshp, "emac0", 0u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_polshp_ld_enable(&polshp, true);
            demo_polshp_ld_set_type(&polshp, FPP_IQOS_SHP_PORT_LEVEL);
            demo_polshp_ld_set_mode(&polshp, FPP_IQOS_SHP_PPS);
            demo_polshp_ld_set_isl(&polshp, 1000u);
            demo_polshp_ld_set_min_credit(&polshp, -5000u);
            demo_polshp_ld_set_max_credit(&polshp, 10000u);
            
            /* update data in PFE */
            rtn = demo_polshp_update(p_cl, &polshp);
        }
        
        /* Ingress QoS shaper 1 (disabled) */
        /* ------------------------------- */
        rtn = demo_polshp_get_by_id(p_cl, &polshp, "emac0", 1u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_polshp_ld_enable(&polshp, false);
            
            /* update data in PFE */
            rtn = demo_polshp_update(p_cl, &polshp);
        }
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
