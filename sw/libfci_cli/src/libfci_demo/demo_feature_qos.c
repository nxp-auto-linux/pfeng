/* =========================================================================
 *  Copyright 2020-2022 NXP
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
#include "demo_qos.h"
 
 
extern int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl);
 
 
/*
 * @brief      Use libFCI to configure PFE egress QoS feature.
 * @details    Scenario description:
 *               [*] Let there be two computers (PCs), both in the same network subnet.
 *                   Both PCs are connected through PFE. PFE acts as a simple bridge.
 *               [*] Use libFCI to configure PFE egress QoS feature on PFE's emac0 physical
 *                   interface, to prioritize and shape egress communication on emac0.
 *               [*] NOTE:
 *                   Be aware that all Egress QoS queues of a physical interface share 
 *                   a single pool of available slots. This means that sum of all Egress QoS
 *                   queue lengths for every interface must fit within some limit.
 *                   See FCI API Reference (chapter Egress QoS) for interface limits.
 *             PC description:
 *               PC0:
 *                 --> IP address: 10.3.0.2/24
 *                 --> Accessible via PFE's emac0 physical interface.
 *               PC1:
 *                 --> IP address: 10.3.0.5/24
 *                 --> Accessible via PFE's emac1 physical interface.
 *             Additional info:
 *               QoS topology of this example:
 * @verbatim
                           SCH0
                           (WRR)
                        +--------+               SCH1
                  Q0--->| 0      |               (PQ)
                  Q1--->| 1      |            +--------+
                        | ...    +--->SHP0--->| 0      |
                        | 6      |            | 1      |
                        | 7      |            | ...    |
                        +--------+            | 4      +--->SHP2--->
                                              | 5      |
                                 Q6---SHP1--->| 6      |
                                 Q7---------->| 7      |
                                              +--------+
   @endverbatim
 * @note       This code uses a suite of "demo_" functions. The "demo_" functions encapsulate
 *             manipulation of libFCI data structs and calls of libFCI functions.
 *             It is advised to inspect content of these "demo_" functions.
 *              
 * @param[in]  p_cl         FCI client
 *                          To create a client, use libFCI function fci_open().
 * @return     FPP_ERR_OK : All FCI commands were successfully executed.
 *                          Egress QoS should be up and running.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_feature_qos(FCI_CLIENT* p_cl)
{  
    assert(NULL != p_cl);
    int rtn = FPP_ERR_OK;
    
    
    /* setup PFE to classify traffic (not needed by Egress QoS, done for demo purposes)*/
    /* =============================================================================== */
    rtn = demo_feature_L2_bridge_vlan(p_cl);
    
    
    /* configure Egress QoS queues for emac0 */
    /* ===================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_queue_cmd_t que = {0};
        
        /* first shorten and disable unused queues to free some slots in the shared pool */
        
        /* queue 2 (disabled) */
        /* ------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 2u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 0u);  /* 0 == DISABLED */
                demo_qos_que_ld_set_max(&que, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 3 (disabled) */
        /* ------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 3u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 0u);  /* 0 == DISABLED */
                demo_qos_que_ld_set_max(&que, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 4 (disabled) */
        /* ------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 4u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 0u);  /* 0 == DISABLED */
                demo_qos_que_ld_set_max(&que, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 5 (disabled) */
        /* ------------------ */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 5u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 0u);  /* 0 == DISABLED */
                demo_qos_que_ld_set_max(&que, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* now configure used queues ; keep in mind that sum of max lengths must be <255 */
        
        /* queue 0 */
        /* ------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 0u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 3u);  /* 3 == WRED */
                demo_qos_que_ld_set_min(&que, 25u);
                demo_qos_que_ld_set_max(&que, 100u);
                demo_qos_que_ld_set_zprob(&que, 0u, 10u);
                demo_qos_que_ld_set_zprob(&que, 1u, 20u);
                demo_qos_que_ld_set_zprob(&que, 2u, 30u);
                demo_qos_que_ld_set_zprob(&que, 3u, 40u);
                demo_qos_que_ld_set_zprob(&que, 4u, 50u);
                demo_qos_que_ld_set_zprob(&que, 5u, 60u);
                demo_qos_que_ld_set_zprob(&que, 6u, 70u);
                demo_qos_que_ld_set_zprob(&que, 7u, 80u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 1 */
        /* ------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 1u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 2u);  /* 2 == TAIL DROP */
                demo_qos_que_ld_set_max(&que, 50u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 6 */
        /* ------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 6u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 3u);  /* 3 == WRED */
                demo_qos_que_ld_set_min(&que, 10u);
                demo_qos_que_ld_set_max(&que, 50u);
                demo_qos_que_ld_set_zprob(&que, 0u, 20u);
                demo_qos_que_ld_set_zprob(&que, 1u, 20u);
                demo_qos_que_ld_set_zprob(&que, 2u, 40u);
                demo_qos_que_ld_set_zprob(&que, 3u, 40u);
                demo_qos_que_ld_set_zprob(&que, 4u, 60u);
                demo_qos_que_ld_set_zprob(&que, 5u, 60u);
                demo_qos_que_ld_set_zprob(&que, 6u, 80u);
                demo_qos_que_ld_set_zprob(&que, 7u, 80u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
        
        /* queue 7 */
        /* ------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "que" */
            rtn = demo_qos_que_get_by_id(p_cl, &que, "emac0", 7u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_que_ld_set_mode(&que, 2u);  /* 2 == TAIL DROP */
                demo_qos_que_ld_set_max(&que, 50u);
                
                /* update data in PFE */
                rtn = demo_qos_que_update(p_cl, &que);
            }
        }
    }
    
    
    /* configure Egress QoS schedulers for emac0 */
    /* ========================================= */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_scheduler_cmd_t sch = {0};
        
        /* scheduler 0 */
        /* ----------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "sch" */
            rtn = demo_qos_sch_get_by_id(p_cl, &sch, "emac0", 0u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_sch_ld_set_mode(&sch, 2u);  /* 2 == packet rate */
                demo_qos_sch_ld_set_algo(&sch, 3u);  /* 3 == WRR */
                demo_qos_sch_ld_set_input(&sch, 0u, true,    0u, 10000u);
                demo_qos_sch_ld_set_input(&sch, 1u, true,    1u, 20000u);
                demo_qos_sch_ld_set_input(&sch, 2u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 3u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 4u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 5u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 6u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 7u, false, 255u, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_sch_update(p_cl, &sch);
            }
        }
        
        /* scheduler 1 */
        /* ----------- */
        if (FPP_ERR_OK == rtn)
        {
            /* get data from PFE and store them in the local variable "sch" */
            rtn = demo_qos_sch_get_by_id(p_cl, &sch, "emac0", 1u);
            if (FPP_ERR_OK == rtn)
            {
                /* modify locally stored data */
                demo_qos_sch_ld_set_mode(&sch, 1u);  /* 1 == data rate */
                demo_qos_sch_ld_set_algo(&sch, 0u);  /* 0 == PQ */
                demo_qos_sch_ld_set_input(&sch, 0u, true,    8u, 0u);
                demo_qos_sch_ld_set_input(&sch, 1u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 2u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 3u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 4u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 5u, false, 255u, 0u);
                demo_qos_sch_ld_set_input(&sch, 6u, true,    6u, 0u);
                demo_qos_sch_ld_set_input(&sch, 7u, true,    7u, 0u);
                
                /* update data in PFE */
                rtn = demo_qos_sch_update(p_cl, &sch);
            }
        }
    }
    
    
    /* configure Egress QoS shapers for emac0 */
    /* ====================================== */
    if (FPP_ERR_OK == rtn)
    {
        fpp_qos_shaper_cmd_t shp = {0};
        
        /* shaper 0 */
        /* -------- */
        rtn = demo_qos_shp_get_by_id(p_cl, &shp, "emac0", 0u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_qos_shp_ld_set_mode(&shp, 2u);     /* 2 == packet rate */
            demo_qos_shp_ld_set_position(&shp, 1u); /* 1 == input #0 of scheduler 1 */
            demo_qos_shp_ld_set_isl(&shp, 1000u);   /* packets per sec */
            demo_qos_shp_ld_set_min_credit(&shp, -5000);
            demo_qos_shp_ld_set_max_credit(&shp, 10000);
            
            /* update data in PFE */
            rtn = demo_qos_shp_update(p_cl, &shp);
        }
        
        /* shaper 1 */
        /* -------- */
        rtn = demo_qos_shp_get_by_id(p_cl, &shp, "emac0", 1u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_qos_shp_ld_set_mode(&shp, 2u);     /* 2 == packet rate */
            demo_qos_shp_ld_set_position(&shp, 7u); /* 7 == input #6 of scheduler 1 */
            demo_qos_shp_ld_set_isl(&shp, 2000u);   /* packets per sec */
            demo_qos_shp_ld_set_min_credit(&shp, -4000);
            demo_qos_shp_ld_set_max_credit(&shp,  8000);
            
            /* update data in PFE */
            rtn = demo_qos_shp_update(p_cl, &shp);
        }
        
        /* shaper 2 */
        /* -------- */
        rtn = demo_qos_shp_get_by_id(p_cl, &shp, "emac0", 2u);
        if (FPP_ERR_OK == rtn)
        {
            /* modify locally stored data */
            demo_qos_shp_ld_set_mode(&shp, 1u);      /* 1 == data rate */
            demo_qos_shp_ld_set_position(&shp, 0u);  /* 0 == output of scheduler 1 */
            demo_qos_shp_ld_set_isl(&shp, 30000u);    /* bits per sec */
            demo_qos_shp_ld_set_min_credit(&shp, -60000);
            demo_qos_shp_ld_set_max_credit(&shp,  90000);
            
            /* update data in PFE */
            rtn = demo_qos_shp_update(p_cl, &shp);
        }
    }
    
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
