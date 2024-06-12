/* =========================================================================
 *  
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_ECC_ERR_CSR_H_
#define PFE_ECC_ERR_CSR_H_

#include "pfe_ecc_err.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t pfe_ecc_err_cfg_isr(addr_t base_va);
void pfe_ecc_err_cfg_irq_mask(addr_t base_va);
void pfe_ecc_err_cfg_irq_unmask(addr_t base_va);
void pfe_ecc_err_cfg_irq_unmask_all(addr_t base_va);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_ECC_ERR_CSR_H_ */
