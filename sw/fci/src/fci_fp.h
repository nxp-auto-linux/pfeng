/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef FCI_FP_H
#define FCI_FP_H

extern errno_t fci_fp_table_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_table_cmd_t *reply_buf, uint32_t *reply_len);
extern errno_t fci_fp_rule_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_rule_cmd_t *reply_buf, uint32_t *reply_len);

#endif

