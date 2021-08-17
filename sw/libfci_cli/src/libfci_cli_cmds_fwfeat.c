/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
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
#include "libfci_cli_cmds_fwfeat.h"

#include "libfci_interface/fci_fwfeat.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static int fwfeat_print(const fpp_fw_features_cmd_t* p_fwfeat)
{
    assert(NULL != p_fwfeat);
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*s%s\n", indent, "", (p_fwfeat->name));
    
    indent += 4;
    
    {
        const char* p_txt_ignored = (2u != (p_fwfeat->variant)) ? (" (ignored)") : ("");
        printf("%-*sstate%s: %s\n", indent, "", 
               p_txt_ignored,
               cli_value2txt_en_dis(p_fwfeat->val));
    }
    
    {
        const char* p_txt_variant_descr = "__INVALID_ITEM__";
        switch (p_fwfeat->variant)
        {
            case 0:
                p_txt_variant_descr = "ignore state and always act as DISABLED";
            break;
            
            case 1:
                p_txt_variant_descr = "ignore state and always act as ENABLED";
            break;
            
            case 2:
                p_txt_variant_descr = "feature is runtime-configurable";
            break;
            
            default:
                p_txt_variant_descr = "__INVALID_ITEM__";
            break;
        }
        printf("%-*svariant: %"PRIu8" (%s)\n", indent, "", 
               (p_fwfeat->variant),
               (p_txt_variant_descr));
    }
    
    {
        printf("%-*s%s\n", indent, "", (p_fwfeat->desc));
    }
    
    return (FPP_ERR_OK); 
}

/* ==== PUBLIC FUNCTIONS =================================================== */ 

int cli_cmd_fwfeat_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_fw_features_cmd_t fwfeat = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    if (p_cmdargs->feature_name.is_valid)
    {
        /* print a single feature */
        rtn = fci_fwfeat_get_by_name(cli_p_cl, &fwfeat, (p_cmdargs->feature_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            rtn = fwfeat_print(&fwfeat);
        }
    }
    else
    {
        /* print all FW features */
        rtn = fci_fwfeat_print_all(cli_p_cl, fwfeat_print);
    }
    
    return (rtn);
}

int cli_cmd_fwfeat_set(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_optbuf_t endis = {{OPT_ENABLE, OPT_DISABLE}};
    const mandopt_t mandopts[] = 
    {
        {OPT_FEATURE, NULL,    (p_cmdargs->feature_name.is_valid)},
        {OPT_NONE,    &endis, ((p_cmdargs->enable_noreply.is_valid) || (p_cmdargs->disable_noorig.is_valid))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        /* NOTE: enable and disable opts are mutually exclusive */
        rtn = fci_fwfeat_set(cli_p_cl, (p_cmdargs->feature_name.txt), (p_cmdargs->enable_noreply.is_valid));
    }
    
    return (rtn);
}

/* ========================================================================= */
