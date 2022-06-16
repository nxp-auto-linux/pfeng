/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_SAFETY_H_
#define PUBLIC_PFE_SAFETY_H_

typedef struct pfe_safety_tag pfe_safety_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_safety_t *pfe_safety_create(addr_t cbus_base_va, addr_t safety_base);
void pfe_safety_destroy(pfe_safety_t *safety);
errno_t pfe_safety_isr(const pfe_safety_t *safety);
void pfe_safety_irq_mask(const pfe_safety_t *safety);
void pfe_safety_irq_unmask(const pfe_safety_t *safety);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_SAFETY_H_ */
