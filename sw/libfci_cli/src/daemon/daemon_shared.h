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

#ifndef DAEMON_SHARED_H_
#define DAEMON_SHARED_H_

#include <stdint.h>
#include <pthread.h>
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */
/* symbols which are shared by daemon internals and libfci_cli commands for daemon */

/* misc */
#define TXT_DAEMON_NAME       "[libfci_cli daemon] "
#define DAEMON_PORT           (26000u)  /* Network port for communication with libfci_cli daemon. */
#define DAEMON_VERSION_MAXLN  (16u)

/* daemon configuration data struct */
typedef struct daemon_cfg_tt {
    const char version[DAEMON_VERSION_MAXLN];
    int32_t pid;
    
    FCI_CLIENT* p_fci_client;
    
    struct {
        uint8_t is_fciev_print_on;
        uint8_t is_dbg_print_on;
    } terminal;
    
    struct {
        FILE* p_file;
        pthread_mutex_t mutex;
        const char name[32];
        uint8_t is_fciev_print_on;
    } logfile;
    
    struct {
        FILE* p_file;
        pthread_mutex_t mutex;
        const char name[32];
        uint8_t is_dbg_print_on;
    } dbgfile;
    
} daemon_cfg_t;

/* cli<->daemon communication : communication struct */
typedef struct daemon_msg_tt {
    char     version[DAEMON_VERSION_MAXLN];
    int32_t  rtn;
    uint16_t cmd;
    uint16_t payload_len;
    uint8_t  payload[1000];
} daemon_msg_t;

/* cli<->daemon communication : commands */
#define DAEMON_STOP                      ( (uint16_t) 11u )
#define DAEMON_PING                      ( (uint16_t) 12u )
#define DAEMON_GET_CFG                   ( (uint16_t) 13u )
#define DAEMON_CLI_CMD_EXECUTE           ( (uint16_t) 14u )
#define DAEMON_TERMINAL_FCIEV_SET_PRINT  ( (uint16_t) 31u )
#define DAEMON_TERMINAL_DBG_SET_PRINT    ( (uint16_t) 32u )
#define DAEMON_LOGFILE_FCIEV_SET_PRINT   ( (uint16_t) 41u )
#define DAEMON_DBGFILE_DBG_SET_PRINT     ( (uint16_t) 51u )

/* ========================================================================= */

#endif
