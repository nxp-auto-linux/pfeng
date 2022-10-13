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
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_daemon.h"

#include "daemon/daemon.h"
#include "daemon/daemon_cmds.h"
#include "daemon/daemon_shared.h"


/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_common.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

void cli_print_error(int errcode, const char* p_txt_errname, const char* p_txt_errmsg, ...);  /* from [libfci_cli.c] */
extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */


/* ==== PUBLIC FUNCTIONS =================================================== */ 

int cli_cmd_daemon_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    daemon_cfg_t daemon_cfg = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    rtn = daemon_get_cfg(&daemon_cfg);
    if (FPP_ERR_OK == rtn)
    {
        printf("Daemon reported the following configuration: \n"
               "  ==================== \n"
               "  version           : %s \n"
               "  pid               : %d \n"
               "  p_fci_client      : %p \n"
               "  ---------- \n"
               "  print-to-terminal : %s \n"
               "  dbg-to-terminal   : %s \n"
               "  ---------- \n"
               "  logfile name      : %s \n"
               "  is logfile open?  : %d \n"
               "  print-to-logfile  : %s \n"
               "  ---------- \n"
               "  dbgfile name      : %s \n"
               "  is dbgfile open?  : %d \n"
               "  dbg-to-dbgfile    : %s \n"
               "  ==================== \n",
               daemon_cfg.version,
               daemon_cfg.pid,
               daemon_cfg.p_fci_client,
               cli_value2txt_on_off(daemon_cfg.terminal.is_fciev_print_on),
               cli_value2txt_on_off(daemon_cfg.terminal.is_dbg_print_on),
               daemon_cfg.logfile.name,
               (NULL != daemon_cfg.logfile.p_file),
               cli_value2txt_on_off(daemon_cfg.logfile.is_fciev_print_on),
               daemon_cfg.dbgfile.name,
               (NULL != daemon_cfg.dbgfile.p_file),
               cli_value2txt_on_off(daemon_cfg.dbgfile.is_dbg_print_on)
        );
    }
    
    return (rtn);
}

int cli_cmd_daemon_update(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    /* empty */
    
    /* send daemon commands (if applicable) */
    rtn = daemon_ping();  /* this is here just to make daemon-update throw an error if no daemon exists */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->print_to_terminal.is_valid)
        {
            rtn = daemon_terminal_fciev_set_print(p_cmdargs->print_to_terminal.is_on);
        }
        if (p_cmdargs->dbg_to_terminal.is_valid)
        {
            rtn = daemon_terminal_dbg_set_print(p_cmdargs->dbg_to_terminal.is_on);
        }
        if (p_cmdargs->print_to_logfile.is_valid)
        {
            rtn = daemon_logfile_fciev_set_print(p_cmdargs->print_to_logfile.is_on);
        }
        if (p_cmdargs->dbg_to_dbgfile.is_valid)
        {
            rtn = daemon_dbgfile_dbg_set_print(p_cmdargs->dbg_to_dbgfile.is_on);
        }
    }
    
    return (rtn);
}

int cli_cmd_daemon_start(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* startup values were taken from defaults in [daemon.c] */
    daemon_cfg_t startup_cfg = {
        .terminal.is_fciev_print_on = 1u,
        .terminal.is_dbg_print_on = 0u,
        
        .logfile.is_fciev_print_on = 1u,
        
        .dbgfile.is_dbg_print_on = 0u,
    };
    
    /* check for mandatory opts */
    /* empty */
    
    /* check whether some daemon is already running */
    rtn = daemon_ping();
    if (FPP_ERR_OK == rtn)
    {
        rtn = CLI_ERR_DAEMON_ALREADY_EXISTS;  /* some daemon detected, do not create another one */
    }
    else if (CLI_ERR_DAEMON_NOT_DETECTED == rtn)
    {
        rtn = FPP_ERR_OK;  /* no daemon detected, can proceed with creating a new one */
    }
    else
    {
        /* empty ; keep the reported error code */
    }
    
    /* start the daemon */
    if (FPP_ERR_OK == rtn) 
    {
        /* close the global FCI client before the daemon is forked */
        /* this prevents any hypothetical forking-related issues with FCI client */
        if (NULL != cli_p_cl)
        {
            const int rtn_close = demo_client_close(cli_p_cl);
            rtn = ((CLI_OK == rtn) ? (rtn_close) : (rtn));
            if (CLI_OK != rtn_close)
            {
                cli_print_error(rtn_close, TXT_ERR_NONAME, TXT_ERR_INDENT "FCI endpoint failed to close.\n");
            }
            cli_p_cl = NULL;
        }
        
        /* modify startup data */
        if (p_cmdargs->print_to_terminal.is_valid)
        {
            startup_cfg.terminal.is_fciev_print_on = p_cmdargs->print_to_terminal.is_on;
        }
        if (p_cmdargs->dbg_to_terminal.is_valid)
        {
            startup_cfg.terminal.is_dbg_print_on = p_cmdargs->dbg_to_terminal.is_on;
        }
        if (p_cmdargs->print_to_logfile.is_valid)
        {
            startup_cfg.logfile.is_fciev_print_on = p_cmdargs->print_to_logfile.is_on;
        }
        if (p_cmdargs->dbg_to_dbgfile.is_valid)
        {
            startup_cfg.dbgfile.is_dbg_print_on = p_cmdargs->dbg_to_dbgfile.is_on;
        }
        
        /* exec */
        rtn = daemon_start(&startup_cfg);
    }
    
    return (rtn);
}

int cli_cmd_daemon_stop(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    rtn = daemon_stop();
    
    return (rtn);
}

/* ========================================================================= */
