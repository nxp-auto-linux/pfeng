/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_PARITY_H_
#define PUBLIC_PFE_PARITY_H_


typedef struct pfe_parity_tag pfe_parity_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_parity_t *pfe_parity_create(addr_t cbus_base_va, addr_t safety_base);
void pfe_parity_destroy(pfe_parity_t *safety);
errno_t pfe_parity_isr(const pfe_parity_t *safety);
void pfe_parity_irq_mask(const pfe_parity_t *safety);
void pfe_parity_irq_unmask(const pfe_parity_t *safety);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_PARITY_H_ */
