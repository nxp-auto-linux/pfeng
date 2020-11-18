/* =========================================================================
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef FCI_SPD_H
#define FCI_SPD_H
errno_t fci_spd_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_spd_cmd_t *reply_buf, uint32_t *reply_len);
#endif

