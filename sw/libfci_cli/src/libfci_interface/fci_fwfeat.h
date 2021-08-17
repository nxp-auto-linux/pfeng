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

#ifndef FCI_SPD_H_
#define FCI_SPD_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_fwfeat_cb_print_t)(const fpp_fw_features_cmd_t* p_fwfeat);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_fwfeat_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_cmd_t* p_rtn_fwfeat, const char* p_name);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */

int fci_fwfeat_set(FCI_CLIENT* p_cl, const char* p_name, bool enable);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_fwfeat_print_all(FCI_CLIENT* p_cl, fci_fwfeat_cb_print_t p_cb_print);
int fci_fwfeat_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count);

/* ========================================================================= */

#endif
