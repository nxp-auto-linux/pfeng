/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_SAFETY_CSR_H_
#define PFE_SAFETY_CSR_H_

#include "pfe_safety.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

errno_t pfe_safety_cfg_isr(void *base_va);
void pfe_safety_cfg_irq_mask(void *base_va);
void pfe_safety_cfg_irq_unmask(void *base_va);
void pfe_safety_cfg_irq_unmask_all(void *base_va);

#endif /* PFE_SAFETY_CSR_H_ */
