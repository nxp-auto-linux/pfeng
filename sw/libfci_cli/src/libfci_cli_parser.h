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

#ifndef LIBFCI_CLI_PARSER_H_
#define LIBFCI_CLI_PARSER_H_

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_app_version(void);

int cli_parse_and_execute(char* pp_txtarr[], int arrln);

/* ========================================================================= */

#endif
