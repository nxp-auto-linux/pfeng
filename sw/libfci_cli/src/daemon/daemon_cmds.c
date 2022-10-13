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
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "../libfci_cli_common.h"
#include "../libfci_cli_def_cmds.h"
#include "../libfci_cli_def_opts.h"

#include "daemon_shared.h"
#include "daemon_cmds.h"


/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "demo_common.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

void cli_print_error(int errcode, const char* p_txt_errname, const char* p_txt_errmsg, ...);  /* from [libfci_cli.c] */

static int daemon_errno = 0;  /* storage for errno; used if errno occurs during communication between daemon and libfci_cli */

/* ==== PRIVATE FUNCTIONS ================================================== */

static int communicate_with_daemon(daemon_msg_t* p_cmd_for_daemon, daemon_msg_t* p_reply_from_daemon, const uint16_t expected_reply_payload_len)
{
    assert(NULL != p_cmd_for_daemon);
    assert(NULL != p_reply_from_daemon);
    
    int rtn = 0;
    int socket_fd = 0;

    /* set common request data */
    {
        p_cmd_for_daemon->rtn = -1;
        strncpy(p_cmd_for_daemon->version, CLI_VERSION_STRING, DAEMON_VERSION_MAXLN);
    }
    
    /* open a network socket */
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (0 > socket_fd)
        {
            daemon_errno = errno;
            rtn = CLI_ERR_DAEMON_COMM_FAIL_SOCKET;
        }
        else
        {
            rtn = 0;
        }
    }
    
    /* set timeouts, so socket does not wait forever if daemon died (for whatever reason) */
    if (0 == rtn)
    {
        struct timeval timeo = {5, 0};
        
        if (0 > setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(struct timeval)))
        {
            daemon_errno = errno;
            rtn = CLI_ERR_DAEMON_COMM_FAIL_SOCKET;
        }
        else
        {
            if (0 > setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(struct timeval)))
            {
                daemon_errno = errno;
                rtn = CLI_ERR_DAEMON_COMM_FAIL_SOCKET;
            }
            else
            {
                rtn = 0;
            }
        }
    }
    
    /* connect to libfci_cli daemon */
    if (0 == rtn)
    {
        struct sockaddr_in socket_addr_server = {0};
        
        socket_addr_server.sin_family = AF_INET;
        socket_addr_server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socket_addr_server.sin_port = htons(DAEMON_PORT);
        
        if (0 > connect(socket_fd, (struct sockaddr *)&socket_addr_server, sizeof(socket_addr_server)))
        {
            daemon_errno = errno;
            rtn = (ECONNREFUSED == daemon_errno) ? (CLI_ERR_DAEMON_NOT_DETECTED) : (CLI_ERR_DAEMON_COMM_FAIL_CONNECT);
        }
        else
        {
            rtn = 0;
        }
    }
    
    /* send command to libfci_cli daemon */
    if (0 == rtn)
    {
        if (0 > send(socket_fd, p_cmd_for_daemon, sizeof(daemon_msg_t), 0))
        {
            daemon_errno = errno;
            rtn = CLI_ERR_DAEMON_COMM_FAIL_SEND;
        }
        else
        {
            rtn = 0;
        }
    }
    
    /* wait for reply from libfci_cli daemon */
    if (0 == rtn)
    {
        if (0 > read(socket_fd, p_reply_from_daemon, sizeof(daemon_msg_t)))
        {
            daemon_errno = errno;
            rtn = CLI_ERR_DAEMON_COMM_FAIL_RECEIVE;
        }
        else
        {
            rtn = 0;
        }
    }
    
    /* basic check of reply data */
    if (0 == rtn)
    {
        if (strcmp(CLI_VERSION_STRING, (p_reply_from_daemon->version)))
        {
            rtn = CLI_ERR_DAEMON_INCOMPATIBLE;
        }
        else if (0 != (p_reply_from_daemon->rtn))
        {
            rtn = CLI_ERR_DAEMON_REPLY_NONZERO_RTN;
            daemon_errno = p_reply_from_daemon->rtn;  /* errno passing mechanism is utilized for daemon return code as well */
        }
        else
        {
            if (expected_reply_payload_len != (p_reply_from_daemon->payload_len))
            {
                rtn = CLI_ERR_DAEMON_REPLY_BAD_DATA;
            }
        }
    }
    
    /* close the network socket ; do not hide behind rtn check! */
    if (0 < socket_fd)
    {
        if (0 > close(socket_fd))
        {
            const int rtn_close = errno;
            cli_print_error(CLI_ERR_DAEMON_COMM_FAIL_SOCKET, TXT_ERR_NONAME, TXT_ERR_INDENT "Failed to close the network socket for communication with libfci_cli daemon. errno=%d \n", rtn_close);
        }
    }
    
    return rtn;
}

/* ==== PUBLIC FUNCTIONS : errno =========================================== */

/* print stored errno */
void daemon_errno_print(const char* p_txt_indent)
{
    assert(NULL != p_txt_indent);
    printf("%s%d\n", p_txt_indent, daemon_errno);
}

/* clear stored errno */
void daemon_errno_clear(void)
{
    daemon_errno = 0;
}

/* ==== PUBLIC FUNCTIONS : commands for daemon ============================= */

int daemon_stop(void)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_STOP };
    
    rtn = communicate_with_daemon(&msg, &msg, 0u);
    
    return rtn;
}

int daemon_ping(void)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_PING };
    
    rtn = communicate_with_daemon(&msg, &msg, 0u);
    
    return rtn;
}

int daemon_get_cfg(daemon_cfg_t *const p_daemon_cfg)
{
    assert(NULL != p_daemon_cfg);
    
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_GET_CFG };
    
    rtn = communicate_with_daemon(&msg, &msg, sizeof(daemon_cfg_t));
    if (0 == rtn)
    {
        memcpy(p_daemon_cfg, msg.payload, sizeof(daemon_cfg_t));
    }
    
    return rtn;
}

int daemon_cli_cmd_execute(cli_cmd_t cmd, const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_CLI_CMD_EXECUTE, .payload_len = (sizeof(cli_cmd_t) + sizeof(cli_cmdargs_t)) };
    
    memcpy((msg.payload + 0), &cmd, sizeof(cli_cmd_t));
    memcpy((msg.payload + sizeof(cli_cmd_t)), p_cmdargs, sizeof(cli_cmdargs_t));
    
    rtn = communicate_with_daemon(&msg, &msg, sizeof(mandopt_optbuf_t));
    
    /*
        NOTE: This daemon command handles its rtn value a bit different than other daemon commands.
              This daemon command represents "remote procedure call" for cli commands.
              It is expected (by other parts of code) that if the send/receive part of this command passes OK,
              then return value of the command represents return value of the remote procedure call.
              (return of the remote cli_cmd_execute())
    */
    if ((0 == rtn) || (CLI_ERR_DAEMON_REPLY_NONZERO_RTN == rtn))
    {
        /* set mandopt buffer from reply data, so it has correct data in case they are needed for cli_cmd_execute() error printout */
        mandopt_optbuf_t optbuf = {{OPT_NONE}};
        memcpy(&optbuf, msg.payload, sizeof(mandopt_optbuf_t));
        cli_mandopt_setinternal(&optbuf);
        
        rtn = msg.rtn;
    }
    
    return rtn;
}

int daemon_terminal_fciev_set_print(uint8_t is_on)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_TERMINAL_FCIEV_SET_PRINT, .payload_len = 1u, .payload = {is_on} };
    
    rtn = communicate_with_daemon(&msg, &msg, 1u);
    
    return rtn;
}

int daemon_terminal_dbg_set_print(uint8_t is_on)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_TERMINAL_DBG_SET_PRINT, .payload_len = 1u, .payload = {is_on} };
    
    rtn = communicate_with_daemon(&msg, &msg, 1u);
    
    return rtn;
}

int daemon_logfile_fciev_set_print(uint8_t is_on)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_LOGFILE_FCIEV_SET_PRINT, .payload_len = 1u, .payload = {is_on} };
    
    rtn = communicate_with_daemon(&msg, &msg, 1u);
    
    return rtn;
}

int daemon_dbgfile_dbg_set_print(uint8_t is_on)
{
    int rtn = CLI_ERR;
    daemon_msg_t msg = { .cmd = DAEMON_DBGFILE_DBG_SET_PRINT, .payload_len = 1u, .payload = {is_on} };
    
    rtn = communicate_with_daemon(&msg, &msg, 1u);
    
    return rtn;
}

/* ========================================================================= */
