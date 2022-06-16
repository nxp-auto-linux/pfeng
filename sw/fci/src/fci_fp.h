/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef FCI_FP_H
#define FCI_FP_H

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

extern errno_t fci_fp_table_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_table_cmd_t *reply_buf, uint32_t *reply_len);
extern errno_t fci_fp_rule_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_fp_rule_cmd_t *reply_buf, uint32_t *reply_len);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif

