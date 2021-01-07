/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2019-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_SAFETY_H_
#define PUBLIC_PFE_SAFETY_H_

typedef struct __pfe_safety_tag pfe_safety_t;

pfe_safety_t *pfe_safety_create(void *cbus_base_va, void *safety_base);
void pfe_safety_destroy(pfe_safety_t *safety);
errno_t pfe_safety_isr(pfe_safety_t *safety);
void pfe_safety_irq_mask(pfe_safety_t *safety);
void pfe_safety_irq_unmask(pfe_safety_t *safety);

#endif /* PUBLIC_PFE_SAFETY_H_ */
