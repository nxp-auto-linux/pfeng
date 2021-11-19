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
#include "libfci_cli_cmds_fp.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_fp.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

static bool stt_do_header_print = false;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static void fprule_header_print(unsigned int indent)
{
    printf("%-*s|  pos  | rule name       | data       | mask       | offset | offset-from | invert | match-action              |\n"
           "%-*s|=======|=================|============|============|========|=============|========|===========================|\n", indent, "", indent, "");
}

static int fprule_print_aux(const fpp_fp_rule_cmd_t* p_rule, uint16_t position, unsigned int indent)
{
    assert(NULL != p_rule);
    
    if (stt_do_header_print)
    {
        fprule_header_print(indent);
        stt_do_header_print = false;
    }
    
    printf("%-*s| %5"PRIu16" | %-15s | 0x%08"PRIX32" | 0x%08"PRIX32" |  %5"PRIu16" | %-11s | %-6s | %-9s %-15s |\n", indent, "",
           position,
           demo_fp_rule_ld_get_name(p_rule),
           demo_fp_rule_ld_get_data(p_rule), 
           demo_fp_rule_ld_get_mask(p_rule),
           demo_fp_rule_ld_get_offset(p_rule),
           cli_value2txt_offset_from(demo_fp_rule_ld_get_offset_from(p_rule)),
           cli_value2txt_on_off(demo_fp_rule_ld_is_invert(p_rule)), 
           cli_value2txt_match_action(demo_fp_rule_ld_get_match_action(p_rule)),
           demo_fp_rule_ld_get_next_name(p_rule));
    
    return (FPP_ERR_OK);
}

static inline int fptable_rule_print(const fpp_fp_rule_cmd_t* p_rule, uint16_t position)
{
    return fprule_print_aux(p_rule, position, 2u);
}

static inline int fprule_print(const fpp_fp_rule_cmd_t* p_rule, uint16_t position)
{
    return fprule_print_aux(p_rule, position, 0u);
}

/* ==== PUBLIC FUNCTIONS : fptable ========================================= */

int cli_cmd_fptable_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_TABLE, NULL, (p_cmdargs->table0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        const uint16_t pos = ((p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (0u));
        const uint16_t cnt = ((p_cmdargs->count_ethtype.is_valid) ? (p_cmdargs->count_ethtype.value) : (0u));
        stt_do_header_print = true;
        rtn = demo_fp_table_print(cli_p_cl, fptable_rule_print, (p_cmdargs->table0_name.txt), pos, cnt);
    }
    
    return (rtn);
}

int cli_cmd_fptable_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_TABLE, NULL, (p_cmdargs->table0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_fp_table_add(cli_p_cl, (p_cmdargs->table0_name.txt));
    }
    
    return (rtn);
}

int cli_cmd_fptable_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_TABLE, NULL, (p_cmdargs->table0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_fp_table_del(cli_p_cl, (p_cmdargs->table0_name.txt));
    }
    
    return (rtn);
}

int cli_cmd_fptable_insrule(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_TABLE, NULL, (p_cmdargs->table0_name.is_valid)},
        {OPT_RULE,  NULL, (p_cmdargs->ruleA0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        const uint16_t pos = ((p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (UINT16_MAX));
        rtn = demo_fp_table_insert_rule(cli_p_cl, (p_cmdargs->table0_name.txt), (p_cmdargs->ruleA0_name.txt), pos);
    }
    
    return (rtn);
}

int cli_cmd_fptable_remrule(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_TABLE, NULL, (p_cmdargs->table0_name.is_valid)},
        {OPT_RULE,  NULL, (p_cmdargs->ruleA0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_fp_table_remove_rule(cli_p_cl, (p_cmdargs->table0_name.txt), (p_cmdargs->ruleA0_name.txt));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : fprule ========================================== */

int cli_cmd_fprule_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_fp_rule_cmd_t fprule = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    if (p_cmdargs->ruleA0_name.is_valid)
    {
        /* print a single rule */
        uint16_t idx = 0u;
        rtn = demo_fp_rule_get_by_name(cli_p_cl, &fprule, &idx, (p_cmdargs->ruleA0_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            stt_do_header_print = true;
            rtn = fprule_print(&fprule, idx);
        }
    }
    else
    {
        /* print all rules */
        const uint16_t pos = ((p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (0u));
        const uint16_t cnt = ((p_cmdargs->count_ethtype.is_valid) ? (p_cmdargs->count_ethtype.value) : (0u));
        stt_do_header_print = true;
        rtn = demo_fp_rule_print_all(cli_p_cl, fprule_print, pos, cnt);
    }

    return (rtn);
}

int cli_cmd_fprule_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_fp_rule_cmd_t fprule = {0};
    
    /* check for mandatory opts */
    const mandopt_optbuf_t arn = {{OPT_ACCEPT, OPT_REJECT, OPT_NEXT_RULE}};
    const mandopt_t mandopts[] = 
    {
        {OPT_RULE,   NULL,  (p_cmdargs->ruleA0_name.is_valid)},
        {OPT_DATA,   NULL,  (p_cmdargs->data_hifc_sad.is_valid)},
        {OPT_MASK,   NULL,  (p_cmdargs->mask_spi.is_valid)},
        {OPT_OFFSET, NULL,  (p_cmdargs->offset.is_valid)},
        {OPT_LAYER,  NULL,  (p_cmdargs->layer.is_valid)},
        {OPT_NONE,   &arn, ((p_cmdargs->accept.is_valid) || (p_cmdargs->reject.is_valid) || (p_cmdargs->ruleB0_name.is_valid))}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    /* empty (no 'init data' from the PFE) */
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        demo_fp_rule_ld_set_data(&fprule, (p_cmdargs->data_hifc_sad.value));
        demo_fp_rule_ld_set_mask(&fprule, (p_cmdargs->mask_spi.value));
        demo_fp_rule_ld_set_offset(&fprule, (p_cmdargs->offset.value), (p_cmdargs->layer.value));
        
        {
            fpp_fp_rule_match_action_t match_action = ((p_cmdargs->accept.is_valid) ? (FP_ACCEPT) :
                                                      ((p_cmdargs->ruleB0_name.is_valid) ? (FP_NEXT_RULE) : 
                                                       (FP_REJECT)));
            const char* p_txt = ((p_cmdargs->ruleB0_name.is_valid) ? (p_cmdargs->ruleB0_name.txt) : (NULL));
            demo_fp_rule_ld_set_match_action(&fprule, match_action, p_txt);
        }
        
        /* this param is optional, hence the validity check */
        if (p_cmdargs->invert.is_valid)  
        {
            demo_fp_rule_ld_set_invert(&fprule, (p_cmdargs->invert.is_valid));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_fp_rule_add(cli_p_cl, (p_cmdargs->ruleA0_name.txt), &fprule);
    }
    
    return (rtn);
}

int cli_cmd_fprule_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_RULE, NULL, (p_cmdargs->ruleA0_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_fp_rule_del(cli_p_cl, (p_cmdargs->ruleA0_name.txt));
    }
    
    return (rtn);
}

/* ========================================================================= */
