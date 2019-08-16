/* =========================================================================
 *  Copyright 2019 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================= */

/**
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 *
 * @defgroup    dxgr_PFE_SAFETY SAFETY
 * @brief		The safety interrupts unit
 * @details     This is the safety interrupts unit.
 *
 * @addtogroup  dxgr_PFE_SAFETY
 * @{
 *
 * @file		pfe_safety.h
 * @brief		The SAFETY module header file.
 * @details		This file contains SAFETY-related API.
 *
 */

#ifndef PUBLIC_PFE_SAFETY_H_
#define PUBLIC_PFE_SAFETY_H_

typedef struct __pfe_safety_tag pfe_safety_t;

pfe_safety_t *pfe_safety_create(void *cbus_base_va, void *safety_base);
void pfe_safety_destroy(pfe_safety_t *safety);
errno_t pfe_safety_isr(pfe_safety_t *safety);
void pfe_safety_irq_mask(pfe_safety_t *safety);
void pfe_safety_irq_unmask(pfe_safety_t *safety);

#endif /* PUBLIC_PFE_SAFETY_H_ */

/** @}*/
