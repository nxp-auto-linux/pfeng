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

#ifndef FCI_COMMON_H_
#define FCI_COMMON_H_

#include <stdint.h>
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

/* return codes (extends libfci 'FPP_ERR_' return code family) */
#define FPP_ERR_FCI           (-1101)
#define FPP_ERR_FCI_INVPTR    (-1102)
#define FPP_ERR_FCI_INVTXTLN  (-1103)

/* ==== PUBLIC FUNCTIONS =================================================== */

void ntoh_enum(void* p_rtn, size_t size);
void hton_enum(void* p_rtn, size_t size);

int set_text(char* p_dst, const char* p_src, const uint_fast16_t dst_ln);

int fci_if_session_lock(FCI_CLIENT* p_cl);
int fci_if_session_unlock(FCI_CLIENT* p_cl, int rtn);

/* ========================================================================= */

#endif
