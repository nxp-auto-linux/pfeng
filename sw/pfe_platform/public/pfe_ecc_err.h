/* =========================================================================
 *  
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_ECC_ERR_H_
#define PUBLIC_PFE_ECC_ERR_H_

typedef struct pfe_ecc_err_tag pfe_ecc_err_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_ecc_err_t *pfe_ecc_err_create(addr_t cbus_base_va, addr_t safety_base);
void pfe_ecc_err_destroy(pfe_ecc_err_t *ecc_err);
errno_t pfe_ecc_err_isr(const pfe_ecc_err_t *ecc_err);
void pfe_ecc_err_irq_mask(const pfe_ecc_err_t *ecc_err);
void pfe_ecc_err_irq_unmask(const pfe_ecc_err_t *ecc_err);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_ECC_ERR_H_ */
