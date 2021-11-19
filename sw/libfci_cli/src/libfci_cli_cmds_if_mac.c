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
#include "libfci_cli_cmds_mirror.h"  /* needed for mirror_print() */
#include "libfci_cli_cmds_if_mac.h"


/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_common.h"
#include "libfci_demo/demo_if_mac.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

typedef int (*cb_cmdexec_t)(const cli_cmdargs_t* p_cmdargs);

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static int if_mac_print_aux(const fpp_if_mac_cmd_t* p_if_mac, int indent)
{
    assert(NULL != p_if_mac);
    
    printf("%-*s", indent, "");
    cli_print_mac(demo_if_mac_ld_get_mac(p_if_mac));
    printf("\n");
    
    return (FPP_ERR_OK);
}

static inline int if_mac_print(const fpp_if_mac_cmd_t* p_if_mac)
{
    return if_mac_print_aux(p_if_mac, 4);
}

int if_mac_print_in_phyif(const fpp_if_mac_cmd_t* p_if_mac)
{
    return if_mac_print_aux(p_if_mac, 10);  /* 10 is based on gfx design of the phyif-print printout */
}

/* ==== PRIVATE FUNCTIONS : cmds =========================================== */

static int exec_inside_locked_session(cb_cmdexec_t p_cb_cmdexec, const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cb_cmdexec);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    rtn = demo_if_session_lock(cli_p_cl);
    if (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_cmdexec(p_cmdargs);
    }
    rtn = demo_if_session_unlock(cli_p_cl, rtn);
    
    return (rtn);
}

static int stt_cmd_if_mac_print(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_if_mac_print_by_name(cli_p_cl, if_mac_print, (p_cmdargs->if_name.txt));
    }
    
    return (rtn);
}

static int stt_cmd_if_mac_add(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_MAC,       NULL, (p_cmdargs->smac.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_if_mac_add(cli_p_cl, (p_cmdargs->smac.arr), (p_cmdargs->if_name.txt));
    }
    
    return (rtn);
}

static int stt_cmd_if_mac_del(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_MAC,       NULL, (p_cmdargs->smac.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_if_mac_del(cli_p_cl, (p_cmdargs->smac.arr), (p_cmdargs->if_name.txt));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS =================================================== */

inline int cli_cmd_phyif_mac_print(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_if_mac_print, p_cmdargs);
}

inline int cli_cmd_phyif_mac_add(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_if_mac_add, p_cmdargs);
}

inline int cli_cmd_phyif_mac_del(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_if_mac_del, p_cmdargs);
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
/* empty */
#endif

/* ========================================================================= */
