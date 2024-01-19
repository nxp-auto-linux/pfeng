/* =========================================================================
 *  Copyright 2020-2024 NXP
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
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "../libfci_cli_common.h"
#include "../libfci_cli_def_cmds.h"
#include "../libfci_cli_def_opts.h"

#include "daemon_shared.h"
#include "daemon_fciev2txt.h"
#include "daemon.h"


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

extern FCI_CLIENT* cli_p_cl;  /* from [libfci_cli.c] */

static_assert((sizeof(CLI_VERSION_STRING) <= DAEMON_VERSION_MAXLN), "CLI_VERSION_STRING is too long for daemon data structs!");

/* default daemon configuration data */
static daemon_cfg_t daemon_cfg =
{
    .version = CLI_VERSION_STRING,
    .pid = -1L,
    
    .p_fci_client = NULL,
    
    .terminal.is_fciev_print_on = 1u,
    .terminal.is_dbg_print_on = 0u,
    
    .logfile.p_file = NULL,
    .logfile.name = "daemon_logfile.txt",
    .logfile.is_fciev_print_on = 1u,

    .dbgfile.p_file = NULL,
    .dbgfile.name = "daemon_dbgfile.txt",
    .dbgfile.is_dbg_print_on = 0u,
};

/* ==== PRIVATE FUNCTIONS ================================================== */

/* printout function; utilized to log daemon activities */
static void DBG_PRINTF(const char* p_txt, ...)
{
    assert(NULL != p_txt);

    if ((daemon_cfg.terminal.is_dbg_print_on) || (daemon_cfg.dbgfile.is_dbg_print_on))
    {
        int rtn = 0;
        char txt_buf[1024] = {0};  /* WARNING: owf possibility. The assumption is that debug prints are short. */
        
        va_list args;
        va_start(args, p_txt);
        rtn = vsnprintf(txt_buf, 1024, p_txt, args);
        va_end(args);
        
        if (0 < rtn)
        {
            if (daemon_cfg.terminal.is_dbg_print_on)
            {
                printf("%s", txt_buf);
            }
            
            pthread_mutex_lock(&daemon_cfg.dbgfile.mutex);
            if ((daemon_cfg.dbgfile.is_dbg_print_on) && (NULL != daemon_cfg.dbgfile.p_file))
            {
                fprintf(daemon_cfg.dbgfile.p_file, "%s", txt_buf);
                fflush(daemon_cfg.dbgfile.p_file);
            }
            pthread_mutex_unlock(&daemon_cfg.dbgfile.mutex);
        }
    }
}

/* auxiliary function for data handling (setting booleans in daemon configuration) */
static int set_uint8bool_by_msg(uint8_t* p_bool, const daemon_msg_t* p_daemon_msg, const char* p_txt_boolname)
{
    assert(NULL != p_bool);
    assert(NULL != p_daemon_msg);
    assert(NULL != p_txt_boolname);
    
    int rtn = -1;
    
    DBG_PRINTF(TXT_DAEMON_NAME "Set '%s': ", p_txt_boolname);
    
    if (1u != (p_daemon_msg->payload_len))
    {
        rtn = -1;
        DBG_PRINTF("FAIL  (wrong payload size;exp=%u;act=%u)\n", 1u, (p_daemon_msg->payload_len));
    }
    else
    {
        *p_bool = *((uint8_t*)(p_daemon_msg->payload));
        
        rtn = 0;
        DBG_PRINTF("OK  (current value = %u)\n", (*p_bool));
    }
    
    return rtn;
}

/* auxiliary function for data handling (filling response payload) */
static int msg_for_client__fill_payload(daemon_msg_t* p_daemon_msg, uint8_t* p_payload, uint16_t payload_len)
{
    assert(NULL != p_daemon_msg);
    assert(NULL != p_payload);
    
    int rtn = -1;
    
    DBG_PRINTF(TXT_DAEMON_NAME "Fill msg_for_client with payload data: ");
    
    if (sizeof(p_daemon_msg->payload) < payload_len)
    {
        rtn = -1;
        DBG_PRINTF("FAIL  (payload_len=%u exceeds max msg payload size (%lu))\n", payload_len, sizeof(p_daemon_msg->payload));
    }
    else
    {
        memcpy(p_daemon_msg->payload, p_payload, payload_len);
        p_daemon_msg->payload_len = payload_len;
        
        rtn = 0;
        DBG_PRINTF("OK  \n");
    }
    
    return rtn;
}

/* auxiliary function for cli command remote execution */
static int execute_cli_cmd_and_fill_payload(daemon_msg_t* p_daemon_msg)
{
    assert(NULL != p_daemon_msg);
    
    int rtn = CLI_ERR;
    cli_cmd_t cmd = CMD_00_NO_COMMAND;
    cli_cmdargs_t cmdargs = {0};
    mandopt_optbuf_t optbuf = {{OPT_NONE}};
    
    const uint32_t exp_payload_len = (sizeof(cli_cmd_t) + sizeof(cli_cmdargs_t));
    
    /* assume that command ID is always present in payload data, regardless of payload length */
    memcpy(&cmd, p_daemon_msg->payload, sizeof(cli_cmd_t));
    DBG_PRINTF(TXT_DAEMON_NAME "Execute cli command '%s': ", cli_cmd_cmd2txt(cmd));
    
    if (exp_payload_len != (p_daemon_msg->payload_len))
    {
        rtn = -1;
        DBG_PRINTF("FAIL  (wrong payload size;exp=%u;act=%u)\n", exp_payload_len, (p_daemon_msg->payload_len));
    }
    else
    {
        /* get cmdargs and execute cli command */
        memcpy(&cmdargs, (p_daemon_msg->payload + sizeof(cli_cmd_t)), sizeof(cli_cmdargs_t));
        rtn = cli_cmd_execute(cmd, &cmdargs);
        
        if (CLI_OK == rtn)
        {
            DBG_PRINTF("OK  \n");
        }
        else
        {
            DBG_PRINTF("OK  (but cli command failed with rtn=%d)\n", rtn);
        }
    }
    
    /* fill reply data, regardless of cli_cmd_execute return code */
    /* mandopt buffer needs to be sent back to client, because it may contain additional data for cli_cmd_execute() error printout */
    cli_mandopt_getinternal(&optbuf);
    msg_for_client__fill_payload(p_daemon_msg, (uint8_t*)(&optbuf), sizeof(mandopt_optbuf_t));
    
    return rtn;
}

/* auxiliary function for data handling (sending response back to the requesting libfci_cli) */
static int msg_for_client__send(int client_socket_fd, daemon_msg_t* p_daemon_msg, int rtn_for_client)
{
    assert(NULL != p_daemon_msg);
    
    int rtn = -1;
    
    DBG_PRINTF(TXT_DAEMON_NAME "Send response back to client: ");
    
    /* set common reply data */
    {
        p_daemon_msg->rtn = rtn_for_client;
        strncpy((p_daemon_msg->version), (daemon_cfg.version), DAEMON_VERSION_MAXLN);
    }
    
    if (0 > send(client_socket_fd, p_daemon_msg, sizeof(daemon_msg_t), 0))
    {
        rtn = errno;
        DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
    }
    else
    {
        rtn = 0;
        DBG_PRINTF("OK  \n");
    }
    
    return rtn;
}

/* main loop of the daemon (processing of incoming libfci_cli requests) */
static int daemon_main_loop(int socket_fd)
{
    int rtn = 0;
    int tmp = 0;
    bool keep_running = true;
    
    int client_socket_fd = -1;
    daemon_msg_t daemon_msg = {0u};
    
    while (true == keep_running)
    {
        /* wait for some connection ; accept() is blocking, hence a bit different printout handling */
        {
            client_socket_fd = accept(socket_fd, NULL, NULL);
            if (0 > client_socket_fd)
            {
                rtn = errno;
                DBG_PRINTF(TXT_DAEMON_NAME "New connection: FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF(TXT_DAEMON_NAME "New connection: OK  (client_socket_fd=%d)\n", client_socket_fd);
            }
        }
        
        /* read the incoming command */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Receive a daemon command: ");
            
            errno = 0;
            tmp = read(client_socket_fd, &daemon_msg, sizeof(daemon_msg_t));
            if (0 > tmp)
            {
                rtn = errno;
                DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
            }
            else if (sizeof(daemon_msg_t) != tmp)
            {
                rtn = -1;
                DBG_PRINTF("FAIL  (incomplete command received)\n");
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  (cmd=%u)\n", (daemon_msg.cmd));
            }
        }
        
        /* check version info */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Check version in daemon command: ");
            
            if (strcmp(daemon_cfg.version, daemon_msg.version))
            {
                rtn = -1;
                DBG_PRINTF("FAIL  (command_version=%s;daemon_version=%s;)\n", (daemon_msg.version), (daemon_cfg.version));
                msg_for_client__send(client_socket_fd, &daemon_msg, CLI_ERR_DAEMON_INCOMPATIBLE);  /* loop the message back to caller almost "as is" (just replace the version) */
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  \n");
            }
        }
        
        /* process the command */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Process the daemon command cmd=%u \n", (daemon_msg.cmd));
            
            switch (daemon_msg.cmd)
            {
                case 0:  /* 0 == nothing */
                    ;    /* empty */
                break;
                
                case DAEMON_STOP:
                    msg_for_client__send(client_socket_fd, &daemon_msg, 0);
                    keep_running = false;
                    rtn = 0;
                break;
                
                case DAEMON_PING:
                    msg_for_client__send(client_socket_fd, &daemon_msg, 0);
                break;
                
                case DAEMON_GET_CFG:
                    tmp = msg_for_client__fill_payload(&daemon_msg, (uint8_t*)(&daemon_cfg), sizeof(daemon_cfg_t));
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                case DAEMON_CLI_CMD_EXECUTE:
                    tmp = execute_cli_cmd_and_fill_payload(&daemon_msg);
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                case DAEMON_TERMINAL_FCIEV_SET_PRINT:
                    tmp = set_uint8bool_by_msg(&daemon_cfg.terminal.is_fciev_print_on, &daemon_msg, "terminal.is_fciev_print_on");
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                case DAEMON_TERMINAL_DBG_SET_PRINT:
                    tmp = set_uint8bool_by_msg(&daemon_cfg.terminal.is_dbg_print_on, &daemon_msg, "terminal.is_dbg_print_on");
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                case DAEMON_LOGFILE_FCIEV_SET_PRINT:
                    tmp = set_uint8bool_by_msg(&daemon_cfg.logfile.is_fciev_print_on, &daemon_msg, "logfile.is_fciev_print_on");
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                case DAEMON_DBGFILE_DBG_SET_PRINT:
                    tmp = set_uint8bool_by_msg(&daemon_cfg.dbgfile.is_dbg_print_on, &daemon_msg, "dbgfile.is_dbg_print_on");
                    msg_for_client__send(client_socket_fd, &daemon_msg, tmp);
                break;
                
                default:
                    DBG_PRINTF("FAIL  (unknown command)\n");
                    msg_for_client__send(client_socket_fd, &daemon_msg, -1);
                break;
            }
        }
        
        /* close the temporary socket which was created by accept() */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Close the temporary connection client_socket_fd=%d: ", client_socket_fd);
            
            if (0 >= client_socket_fd)
            {
                DBG_PRINTF("No valid temporary socket detected. Skipping this step.\n");
            }
            else
            {
                if (0 > close(client_socket_fd))
                {
                    rtn = errno;
                    DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
                }
                else
                {
                    rtn = 0;
                    DBG_PRINTF("OK  \n");
                }
            }
        }
        
    }
    
    return rtn;
}

/* FCI event callback function ; this is called from the parallel thread for each caught FCI event */
static fci_cb_retval_t fciev_callback(unsigned short fcode, unsigned short len, unsigned short* payload)
{
    int tmp = 0;
    
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &tmp);  /* guard FCI event processing against abrupt thread cancellation */
    
    DBG_PRINTF(TXT_DAEMON_NAME "Receive FCI event (fcode=0x%04X;len=%hu): OK  \n", fcode, len);
    
    if ((daemon_cfg.terminal.is_fciev_print_on) || (daemon_cfg.logfile.is_fciev_print_on))
    {
        char txt_fciev[2048] = {0};
        
        DBG_PRINTF(TXT_DAEMON_NAME "Print FCI event (fcode=0x%04X;len=%hu): ", fcode, len);
        
        tmp = daemon_fciev2txt_print(txt_fciev, sizeof(txt_fciev), fcode, len, payload);
        if (0 != tmp)
        {
            DBG_PRINTF("FAIL  (rtn=%d)\n", tmp);
        }
        else
        {
            if (daemon_cfg.terminal.is_fciev_print_on)
            {
                printf("%s", txt_fciev);
            }
            if (daemon_cfg.logfile.is_fciev_print_on)
            {
                pthread_mutex_lock(&daemon_cfg.logfile.mutex);
                if (NULL != daemon_cfg.logfile.p_file)
                {
                    fprintf(daemon_cfg.logfile.p_file, "%s", txt_fciev);
                    fflush(daemon_cfg.logfile.p_file);
                }
                pthread_mutex_unlock(&daemon_cfg.logfile.mutex);
            }
            DBG_PRINTF("OK  \n");
        }
    }
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &tmp); /* re-enable thread cancellability */
    
    return FCI_CB_CONTINUE;
}

/* ==== PUBLIC FUNCTIONS =================================================== */ 

int daemon_start(const daemon_cfg_t* p_startup_daemon_cfg)
{
    /* 'p_startup_daemon_cfg' is allowed to be NULL */
    
    int rtn = 0;
    pid_t pid = 0;
    
    /* fork the daemon */
    pid = fork();
    if (pid < 0)
    {
        rtn = errno;
        printf("Fork the " TXT_DAEMON_NAME ": FAIL  (errno=%d)\n", rtn);
    }
    else
    {
        rtn = 0;
    }
    
    /* configure the newly forked daemon process */
    if ((0 == rtn) && (0 == pid))  /* '0 == pid' means this is the forked child process */
    {
        int tmp = 0;
        int socket_fd = -1;
        
        printf("Fork the " TXT_DAEMON_NAME ": OK  (pid=%d)\n", getpid());
        
        umask(0);
        daemon_cfg.pid = getpid();
        
        /* set init cfg */
        if (NULL != p_startup_daemon_cfg)
        {
            daemon_cfg.terminal.is_fciev_print_on = p_startup_daemon_cfg->terminal.is_fciev_print_on;
            daemon_cfg.terminal.is_dbg_print_on = p_startup_daemon_cfg->terminal.is_dbg_print_on;
            
            daemon_cfg.logfile.is_fciev_print_on = p_startup_daemon_cfg->logfile.is_fciev_print_on;
            
            daemon_cfg.dbgfile.is_dbg_print_on = p_startup_daemon_cfg->dbgfile.is_dbg_print_on;
        }
        
        /* MUTEXES: init file mutexes */
        {
            pthread_mutexattr_t mutexattr;
            
            /* prepare attributes */
            rtn = pthread_mutexattr_init(&mutexattr);
            if (0 == rtn)
            {
                rtn = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_NORMAL);
            }
            if (0 == rtn)
            {
                rtn = pthread_mutexattr_setrobust(&mutexattr, PTHREAD_MUTEX_ROBUST);
            }
            
            /* init mutexes */
            if (0 == rtn)
            {
                rtn = pthread_mutex_init(&daemon_cfg.dbgfile.mutex, &mutexattr);
            }
            if (0 == rtn)
            {
                rtn = pthread_mutex_init(&daemon_cfg.logfile.mutex, &mutexattr);
            }
            
            /* destroy attributes (no longer needed) */
            pthread_mutexattr_destroy(&mutexattr);
        }
        
        /* FILE: open a dbgfile */
        if (0 == rtn)
        {
            daemon_cfg.dbgfile.p_file = fopen(daemon_cfg.dbgfile.name, "w");
            if (NULL == daemon_cfg.dbgfile.p_file)
            {
                rtn = errno;
                DBG_PRINTF(TXT_DAEMON_NAME "Open a dbgfile: FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF(TXT_DAEMON_NAME "Open a dbgfile: OK  \n");
            }
        }
        
        /* FILE: open a logfile */
        if (0 == rtn)
        {
            daemon_cfg.logfile.p_file = fopen(daemon_cfg.logfile.name, "w");
            if (NULL == daemon_cfg.logfile.p_file)
            {
                rtn = errno;
                DBG_PRINTF(TXT_DAEMON_NAME "Open a logfile: FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF(TXT_DAEMON_NAME "Open a logfile: OK  \n");
            }
        }
        
        DBG_PRINTF(TXT_DAEMON_NAME "Daemon started  (pid=%d)\n", getpid());
        
        /* SOCKET: open a network socket */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Open a network socket: ");
            
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (0 > socket_fd)
            {
                rtn = errno;
                DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  (socket_fd=%d)\n", socket_fd);
            }
        }
        
        /* SOCKET: set the socket to be reusable */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Set socket parameters: ");
            
            tmp = 1;
            if (0 > setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)))
            {
                rtn = errno;
                DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  \n");
            }
        }
        
        /* SOCKET: bind the socket */
        if (0 == rtn)
        {
            struct sockaddr_in socket_addr = {0};
        
            DBG_PRINTF(TXT_DAEMON_NAME "Bind the socket (sin_addr=0x%08X;sin_port=%u): ", INADDR_ANY, DAEMON_PORT);
            
            socket_addr.sin_family = AF_INET;
            socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            socket_addr.sin_port = htons(DAEMON_PORT);
            
            if (0 > bind(socket_fd, (struct sockaddr *)&socket_addr, sizeof(socket_addr)))
            {
                rtn = errno;
                DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  \n");
            }
        }
        
        /* SOCKET: start listening on the socket */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Start listening on the socket: ");
            
            if (0 > listen(socket_fd, 16))
            {
                rtn = errno;
                DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
            }
            else
            {
                rtn = 0;
                DBG_PRINTF("OK  \n");
            }
        }
        
        /* FCI: open FCI client */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Open FCI client: ");
            
            rtn = demo_client_open_in_cmd_mode(&daemon_cfg.p_fci_client);
            if (0 != rtn)
            {
                DBG_PRINTF("FAIL  (rtn=%d)\n", rtn);
            }
            else
            {
                DBG_PRINTF("OK  (FCI_CLIENT=%p)\n", (daemon_cfg.p_fci_client));
                cli_p_cl = daemon_cfg.p_fci_client;  /* this global var is used in cli cmd executions */
            }
        }
        
        /* FCI: start a parallel thread for FCI events catching */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Start a parallel thread for FCI events catching: ");
            
            rtn = demo_events_catching_init(daemon_cfg.p_fci_client, fciev_callback);
            if (0 != rtn)
            {
                DBG_PRINTF("FAIL  (rtn=%d)\n", rtn);
            }
            else
            {
                DBG_PRINTF("OK  (fci_fd=%d)\n", fci_fd(daemon_cfg.p_fci_client));
            }
        }
        
        /* main loop of the daemon (processing of incoming libfci_cli requests) */
        if (0 == rtn)
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Started main loop\n");
            rtn = daemon_main_loop(socket_fd);
            DBG_PRINTF(TXT_DAEMON_NAME "Finished main loop (rtn=%d)\n", rtn);
            DBG_PRINTF(TXT_DAEMON_NAME "Shutdown initiated\n");
        }
        
        /* FCI: stop the parallel thread */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Stop the parallel thread: ");
            
            rtn = demo_events_catching_fini(daemon_cfg.p_fci_client);
            if (0 != rtn)
            {
                DBG_PRINTF("FAIL  (rtn=%d)\n", rtn);
            }
            else
            {
                DBG_PRINTF("OK  \n");
            }
        }
        
        /* FCI: close FCI client */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Close the FCI client (FCI_CLIENT=%p): ", (daemon_cfg.p_fci_client));
               
            if (NULL == (daemon_cfg.p_fci_client))
            {
                DBG_PRINTF("No valid FCI client found. Skipping this step.\n");
            }
            else
            {
                rtn = demo_client_close(daemon_cfg.p_fci_client);
                if (0 != rtn)
                {
                    DBG_PRINTF("FAIL  (rtn=%d)\n", rtn);
                }
                else
                {
                    DBG_PRINTF("OK  \n");
                }
                
                /* consider FCI client destroyed (regardless of rtn code) */
                daemon_cfg.p_fci_client = NULL;
                cli_p_cl = daemon_cfg.p_fci_client;  /* this global var is used in cli cmd executions */
            }
        }
        
        /* SOCKET: close the network socket */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Close the network socket (socket_fd=%d): ", socket_fd);
            
            if (0 >= socket_fd)
            {
                DBG_PRINTF("No valid network socket detected. Skipping this step.\n");
            }
            else
            {
                if (0 > close(socket_fd))
                {
                    rtn = errno;
                    DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
                }
                else
                {
                    rtn = 0;
                    DBG_PRINTF("OK  \n");
                }
            }
        }
        
        DBG_PRINTF(TXT_DAEMON_NAME "Daemon (pid=%d) stopped\n", getpid());
        
        /* FILE: close the logfile */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Close the logfile: ");
            
            if (NULL == daemon_cfg.logfile.p_file)
            {
                DBG_PRINTF("No valid logfile detected. Skipping this step.\n");
            }
            else
            {
                if (0 != fclose(daemon_cfg.logfile.p_file))
                {
                    rtn = errno;
                    daemon_cfg.logfile.p_file = NULL;
                    DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
                }
                else
                {
                    rtn = 0;
                    daemon_cfg.logfile.p_file = NULL;
                    DBG_PRINTF("OK  \n");
                }
            }
        }
        
        /* FILE: close the dbgfile */
        {
            DBG_PRINTF(TXT_DAEMON_NAME "Close the dbgfile: ");
            
            if (NULL == daemon_cfg.dbgfile.p_file)
            {
                DBG_PRINTF("No valid dbgfile detected. Skipping this step.\n");
            }
            else
            {
                if (0 != fclose(daemon_cfg.dbgfile.p_file))
                {
                    rtn = errno;
                    daemon_cfg.dbgfile.p_file = NULL;
                    DBG_PRINTF("FAIL  (errno=%d)\n", rtn);
                }
                else
                {
                    rtn = 0;
                    daemon_cfg.dbgfile.p_file = NULL;
                    DBG_PRINTF("OK  \n");
                }
            }
        }
        
        /* MUTEXES: destroy file mutexes */
        {
            pthread_mutex_destroy(&daemon_cfg.logfile.mutex);
            pthread_mutex_destroy(&daemon_cfg.dbgfile.mutex);
        }
    }
    
    return rtn;
}


/* ========================================================================= */
