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

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_mirror.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_mirror.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static int mirror_print_aux(const fpp_mirror_cmd_t* p_mirror, bool is_verbose, int indent_of_verbose_info)
{
    assert(NULL != p_mirror);
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*s%s\n", indent, "", demo_mirror_ld_get_name(p_mirror));
    
    if (is_verbose)
    {
        indent += indent_of_verbose_info;
        
        printf("%-*sinterface:       %s\n", indent, "", demo_mirror_ld_get_egress_phyif(p_mirror));
        
        printf("%-*sflexible-filter: ", indent, "");
        cli_print_tablenames(&(p_mirror->filter_table_name), 1u, "", "---");
        printf("\n");
        
        printf("%-*smodify-actions:  ", indent, "");
        cli_print_bitset32(demo_mirror_ld_get_ma_bitset(p_mirror), ",", cli_value2txt_modify_action, "---");
        printf("\n");
        
        /* detailed info - modification action arguments (only if the corresponding action is active) */
        {
            indent += 2u;  /* verbose info is indented even deeper */
            
            const fpp_modify_actions_t modify_actions = demo_mirror_ld_get_ma_bitset(p_mirror);
            
            if (MODIFY_ACT_ADD_VLAN_HDR & modify_actions)
            {
                printf("%-*s"TXT_MODIFY_ACTION__ADD_VLAN_HDR": %"PRIu16"\n", indent, "",
                       demo_mirror_ld_get_ma_vlan(p_mirror));
            }
        }
    }
    
    return (FPP_ERR_OK); 
}

static inline int mirror_print(const fpp_mirror_cmd_t* p_mirror)
{
    return mirror_print_aux(p_mirror, true, 4);
}

/* ==== PUBLIC FUNCTIONS =================================================== */ 

int mirror_print_in_phyif(const fpp_mirror_cmd_t* p_mirror, bool is_verbose)
{
    return mirror_print_aux(p_mirror, is_verbose, 19);  /* 19 is based on gfx design of the phyif-print printout */
}


int cli_cmd_mirror_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_mirror_cmd_t mirror = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    if (p_cmdargs->mirror_name.is_valid)
    {
        /* print a single mirror rule */
        rtn = demo_mirror_get_by_name(cli_p_cl, &mirror, (p_cmdargs->mirror_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            rtn = mirror_print(&mirror);
        }
    }
    else
    {
        /* print all mirroring rules */
        rtn = demo_mirror_print_all(cli_p_cl, mirror_print);
    }
    
    return (rtn);
}

int cli_cmd_mirror_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_mirror_cmd_t mirror = {0};
    const fpp_modify_actions_t modify_actions = ((p_cmdargs->modify_actions.is_valid) ? (p_cmdargs->modify_actions.bitset) : (0));
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_MIRROR, NULL, (p_cmdargs->mirror_name.is_valid)},
        
        /* these are mandatory only if the related modify action is requested */
        {OPT_VLAN,   NULL, ((MODIFY_ACT_ADD_VLAN_HDR & modify_actions) ? (p_cmdargs->vlan.is_valid) : (true))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_get_by_name(cli_p_cl, &mirror, (p_cmdargs->mirror_name.txt));
    }
    
    /* modify local data - 'modify actions' bitset */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->modify_actions.is_valid))
    {
        /* clear any previous modify actions */
        demo_mirror_ld_clear_all_ma(&mirror);
        
        /* set modify actions */
        if (MODIFY_ACT_ADD_VLAN_HDR & modify_actions)
        {
            demo_mirror_ld_set_ma_vlan(&mirror, true, (p_cmdargs->vlan.value));
        }
    }
    
    /* modify local data - misc configuration */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->if_name.is_valid)
        {
            demo_mirror_ld_set_egress_phyif(&mirror, (p_cmdargs->if_name.txt));
        }
        if (p_cmdargs->table0_name.is_valid)
        {
            demo_mirror_ld_set_filter(&mirror, (p_cmdargs->table0_name.txt));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_update(cli_p_cl, &mirror);
    }
    
    return (rtn);
}

int cli_cmd_mirror_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_MIRROR,    NULL, (p_cmdargs->mirror_name.is_valid)},
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_add(cli_p_cl, NULL, (p_cmdargs->mirror_name.txt), (p_cmdargs->if_name.txt));
    }
    
    /* EXTRA: if mirroring rule properly created, then automatically call update as well */
    if (FPP_ERR_OK == rtn)
    {
        rtn = cli_cmd_mirror_update(p_cmdargs);
    }
    
    return (rtn);
}

int cli_cmd_mirror_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_MIRROR, NULL, (p_cmdargs->mirror_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_mirror_del(cli_p_cl, (p_cmdargs->mirror_name.txt));
    }
    
    return (rtn);
}

/* ========================================================================= */
