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
#include <stdarg.h>
#include <stdio.h>
#include "libfci_cli_common.h"
#include "libfci_cli_def_help.h"
#include "libfci_cli_parser.h"
#include "fci_ep.h"

/* ==== TYPEDEFS & DATA ==================================================== */

FCI_CLIENT* cli_p_cl = NULL;

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_error(int rtncode, const char* p_txt_err, ...)
{
    assert(NULL != p_txt_err);

    printf("ERROR (%d): ", rtncode);
    
    va_list args;
    va_start(args, p_txt_err);
    vprintf(p_txt_err, args);
    va_end(args);
    
    printf("\n");
}

int main(int argc, char* argv[])
{
    #if !defined(NDEBUG)
        #warning "DEBUG build"
        printf("\nWARNING: DEBUG build\n");
    #endif
    
    
    int rtn = CLI_ERR;
    
    printf("DISCLAIMER: This is a DEMO application. It is not part of the production code deliverables.\n");
    
    if (1 >= argc)
    {
        cli_print_app_version();
        cli_print_help(0);
    }
    
    rtn = fci_ep_open_in_cmd_mode(&cli_p_cl);
    if (CLI_OK != rtn)
    {
        cli_print_error(rtn, "FCI endpoint failed to open.");
    }
    else
    {
        rtn = cli_parse_and_execute(argv, argc);
    }
    
    /* close FCI (do not hide behind rtn check) */
    if (NULL != cli_p_cl)
    {
        const int rtn_close = fci_ep_close(cli_p_cl);
        rtn = ((CLI_OK == rtn) ? (rtn_close) : (rtn));
        if (CLI_OK != rtn_close)
        {
            cli_print_error(rtn_close, "FCI endpoint failed to close.");
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
