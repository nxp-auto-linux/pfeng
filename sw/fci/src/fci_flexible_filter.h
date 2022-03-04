/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef FCI_FLEXIBLE_FILTER_H
#define FCI_FLEXIBLE_FILTER_H

extern errno_t fci_flexible_filter_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_flexible_filter_cmd_t *reply_buf, uint32_t *reply_len);

#endif

