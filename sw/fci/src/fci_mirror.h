/* =========================================================================
 *  Copyright 2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef FCI_MIRROR_H
#define FCI_MIRROR_H

errno_t fci_mirror_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_mirror_cmd_t *reply_buf, uint32_t *reply_len);

#endif
