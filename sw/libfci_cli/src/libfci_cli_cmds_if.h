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

#ifndef LIBFCI_CLI_CMDS_IF_H_
#define LIBFCI_CLI_CMDS_IF_H_

#include "libfci_cli_common.h"

/* ==== PUBLIC FUNCTIONS =================================================== */

int cli_cmd_phyif_print(const cli_cmdargs_t *p_cmdargs);
int cli_cmd_phyif_update(const cli_cmdargs_t *p_cmdargs);

int cli_cmd_logif_print(const cli_cmdargs_t *p_cmdargs);
int cli_cmd_logif_update(const cli_cmdargs_t *p_cmdargs);
int cli_cmd_logif_add(const cli_cmdargs_t *p_cmdargs);
int cli_cmd_logif_del(const cli_cmdargs_t *p_cmdargs);

/* ========================================================================= */

#endif
