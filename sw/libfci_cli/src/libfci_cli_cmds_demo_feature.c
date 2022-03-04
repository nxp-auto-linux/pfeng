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
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libfci.h"

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

typedef struct demo_menu_item_tt {
    int (*p_cb)(FCI_CLIENT* p_cl);
    const char* p_txt_name;
} demo_feature_t;

extern int demo_feature_physical_interface(FCI_CLIENT* p_cl);
extern int demo_feature_L2_bridge_vlan(FCI_CLIENT* p_cl);
extern int demo_feature_router_simple(FCI_CLIENT* p_cl);
extern int demo_feature_router_nat(FCI_CLIENT* p_cl);
extern int demo_feature_L2L3_bridge_vlan(FCI_CLIENT* p_cl);
extern int demo_feature_flexible_filter(FCI_CLIENT* p_cl);
extern int demo_feature_flexible_router(FCI_CLIENT* p_cl);
extern int demo_feature_spd(FCI_CLIENT* p_cl);
extern int demo_feature_qos(FCI_CLIENT* p_cl);
extern int demo_feature_qos_policer(FCI_CLIENT* p_cl);
 
static const demo_feature_t demo_features[] = 
{
    { demo_feature_physical_interface,
                  "physical_interface" },
    
    { demo_feature_L2_bridge_vlan,
                  "L2_bridge_vlan" },
    
    { demo_feature_router_simple,
                  "router_simple" },
     
    { demo_feature_router_nat,
                  "router_nat" },
     
    { demo_feature_L2L3_bridge_vlan,
                  "L2L3_bridge_vlan" },
    
    { demo_feature_flexible_filter,
                  "flexible_filter" },
    
    { demo_feature_flexible_router,
                  "flexible_router" },
    
    { demo_feature_spd,
                  "spd" },
    
    { demo_feature_qos,
                  "qos" },
    
    { demo_feature_qos_policer,
                  "qos_policer" },
}; 

#define DEMO_FEATURES_LN (uint8_t)(sizeof(demo_features) / sizeof(demo_feature_t))

/* ==== PUBLIC FUNCTIONS =================================================== */ 

int cli_cmd_demo_feature_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    printf("Available demo feature scenarios:\n");
    for (uint8_t i = 0u; (DEMO_FEATURES_LN > i); (++i))
    {
        printf("  %s\n", demo_features[i].p_txt_name);
    }
    
    return (FPP_ERR_OK);
}

int cli_cmd_demo_feature_run(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_FEATURE, NULL, (p_cmdargs->feature_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* find and execute the demo feature */
    if (FPP_ERR_OK == rtn)
    {
        const char *const p_txt_name = (p_cmdargs->feature_name.txt);
        uint8_t i = UINT8_MAX;  /* WARNING: intentional use of owf behavior */ 
        while ((DEMO_FEATURES_LN > (++i)) && (0 != strcmp(demo_features[i].p_txt_name, p_txt_name))) { /* empty */ };
        if (DEMO_FEATURES_LN <= i)
        {
            rtn = CLI_ERR_INV_DEMO_FEATURE;
        }
        else
        {
            rtn = demo_features[i].p_cb(cli_p_cl);
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
