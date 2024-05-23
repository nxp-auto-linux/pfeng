/* =========================================================================
 *  
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_HOST_FAIL_STOP_H_
#define PUBLIC_PFE_HOST_FAIL_STOP_H_

typedef struct pfe_host_fail_stop_tag pfe_host_fail_stop_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_host_fail_stop_t *pfe_host_fail_stop_create(addr_t cbus_base_va, addr_t safety_base);
void pfe_host_fail_stop_destroy(pfe_host_fail_stop_t *host_fail_stop);
errno_t pfe_host_fail_stop_isr(const pfe_host_fail_stop_t *host_fail_stop);
void pfe_host_fail_stop_irq_mask(const pfe_host_fail_stop_t *host_fail_stop);
void pfe_host_fail_stop_irq_unmask(const pfe_host_fail_stop_t *host_fail_stop);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_HOST_FAIL_STOP_H_ */
