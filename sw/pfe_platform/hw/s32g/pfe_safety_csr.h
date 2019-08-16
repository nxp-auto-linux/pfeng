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
 * @addtogroup  dxgr_PFE_SAFETY
 * @{
 *
 * @file		pfe_safety_csr.h
 * @brief		The safety module registers definition file (s32g).
 * @details
 *
 */

#ifndef PFE_SAFETY_CSR_H_
#define PFE_SAFETY_CSR_H_

#include "pfe_safety.h"


/*WSP_SAFETY_INT_SRC bits*/
#define	SAFETY_INT			(1U << 0)
#define	MASTER1_INT 		(1U << 1)
#define	MASTER2_INT			(1U << 2)
#define	MASTER3_INT			(1U << 3)
#define	MASTER4_INT			(1U << 4)
#define	EMAC_CBUS_INT 		(1U << 5)
#define	EMAC_DBUS_INT 		(1U << 6)
#define	CLASS_CBUS_INT 		(1U << 7)
#define	CLASS_DBUS_INT 		(1U << 8)
#define	TMU_CBUS_INT 		(1U << 9)
#define	TMU_DBUS_INT 		(1U << 10)
#define	HIF_CBUS_INT 		(1U << 11)
#define	HIF_DBUS_INT 		(1U << 12)
#define	HIF_NOCPY_CBUS_INT 	(1U << 13)
#define	HIF_NOCPY_DBUS_INT 	(1U << 14)
#define	UPE_CBUS_INT 		(1U << 15)
#define	UPE_DBUS_INT 		(1U << 16)
#define	HRS_CBUS_INT 		(1U << 17)
#define	BRIDGE_CBUS_INT 	(1U << 18)
#define EMAC_SLV_INT 		(1U << 19)
#define	BMU1_SLV_INT 		(1U << 20)
#define	BMU2_SLV_INT 		(1U << 21)
#define	CLASS_SLV_INT 		(1U << 22)
#define	HIF_SLV_INT 		(1U << 23)
#define	HIF_NOCPY_SLV_INT 	(1U << 24)
#define	LMEM_SLV_INT 		(1U << 25)
#define	TMU_SLV_INT 		(1U << 26)
#define	UPE_SLV_INT 		(1U << 27)
#define	WSP_GLOBAL_SLV_INT 	(1U << 28)

/* WSP_SAFETY_INT_EN bits*/
#define	SAFETY_INT_EN			(1U << 0)
#define	MASTER1_INT_EN 			(1U << 1)
#define	MASTER2_INT_EN			(1U << 2)
#define	MASTER3_INT_EN			(1U << 3)
#define	MASTER4_INT_EN			(1U << 4)
#define	EMAC_CBUS_INT_EN 		(1U << 5)
#define	EMAC_DBUS_INT_EN 		(1U << 6)
#define	CLASS_CBUS_INT_EN 		(1U << 7)
#define	CLASS_DBUS_INT_EN 		(1U << 8)
#define	TMU_CBUS_INT_EN 		(1U << 9)
#define	TMU_DBUS_INT_EN 		(1U << 10)
#define	HIF_CBUS_INT_EN 		(1U << 11)
#define	HIF_DBUS_INT_EN 		(1U << 12)
#define	HIF_NOCPY_CBUS_INT_EN 	(1U << 13)
#define	HIF_NOCPY_DBUS_INT_EN 	(1U << 14)
#define	UPE_CBUS_INT_EN 		(1U << 15)
#define	UPE_DBUS_INT_EN 		(1U << 16)
#define	HRS_CBUS_INT_EN 		(1U << 17)
#define	BRIDGE_CBUS_INT_EN 		(1U << 18)
#define EMAC_SLV_INT_EN 		(1U << 19)
#define	BMU1_SLV_INT_EN 		(1U << 20)
#define	BMU2_SLV_INT_EN 		(1U << 21)
#define	CLASS_SLV_INT_EN 		(1U << 22)
#define	HIF_SLV_INT_EN 			(1U << 23)
#define	HIF_NOCPY_SLV_INT_EN 	(1U << 24)
#define	LMEM_SLV_INT_EN 		(1U << 25)
#define	TMU_SLV_INT_EN 			(1U << 26)
#define	UPE_SLV_INT_EN 			(1U << 27)
#define	WSP_GLOBAL_SLV_INT_EN 	(1U << 28)


#define	SAFETY_INT_ENABLE_ALL	0x1FFFFFFFU

errno_t pfe_safety_cfg_isr(void *base_va);
void pfe_safety_cfg_irq_mask(void *base_va);
void pfe_safety_cfg_irq_unmask(void *base_va);
void pfe_safety_cfg_irq_unmask_all(void *base_va);

#endif /* PFE_SAFETY_CSR_H_ */

/** @}*/
