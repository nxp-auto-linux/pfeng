/* =========================================================================
 *  Copyright 2017-2022 NXP
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_help.h"
#include "libfci_cli_parser.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_common.h"

/* ==== TYPEDEFS & DATA ==================================================== */

FCI_CLIENT* cli_p_cl = NULL;

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_error(int errcode, const char* p_txt_errname, const char* p_txt_errmsg, ...)
{
    assert((NULL != p_txt_errname) && (NULL != p_txt_errmsg));

    printf("ERROR (%d)%s\n", errcode, p_txt_errname);
    
    va_list args;
    va_start(args, p_txt_errmsg);
    vprintf(p_txt_errmsg, args);
    va_end(args);
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
        cli_print_app_version(false);
        cli_print_help(0);
        rtn = CLI_OK;
    }
    else if (0 == strcmp(argv[1], "--version"))
    {
        const bool is_verbose = ((2 < argc) && (0 == strcmp(argv[2], "--verbose")));
        cli_print_app_version(is_verbose);
        rtn = CLI_OK;
    }
    else
    {
        rtn = demo_client_open_in_cmd_mode(&cli_p_cl);
        if (CLI_OK != rtn)
        {
            cli_print_error(rtn, TXT_ERR_NONAME, TXT_ERR_INDENT "FCI endpoint failed to open.\n");
        }
        else
        {
            rtn = cli_parse_and_execute(argv, argc);
        }
        
        /* close FCI (do not hide behind rtn check) */
        if (NULL != cli_p_cl)
        {
            const int rtn_close = demo_client_close(cli_p_cl);
            rtn = ((CLI_OK == rtn) ? (rtn_close) : (rtn));
            if (CLI_OK != rtn_close)
            {
                cli_print_error(rtn_close, TXT_ERR_NONAME, TXT_ERR_INDENT "FCI endpoint failed to close.\n");
            }
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
