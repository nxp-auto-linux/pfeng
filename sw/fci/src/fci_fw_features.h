/* =========================================================================
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef FCI_FW_FEATURES_H
#define FCI_FW_FEATURES_H

extern errno_t fci_fw_features_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fw_features_cmd_t *reply_buf, uint32_t *reply_len);

#endif

