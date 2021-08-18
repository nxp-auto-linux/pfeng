/* =========================================================================
 *  Copyright 2020-2021 NXP
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
