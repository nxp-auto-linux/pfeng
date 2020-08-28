/* =========================================================================
 *  Copyright 2018-2020 NXP
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

#ifndef PFE_CBUS_H_
#define PFE_CBUS_H_

#if !defined(PFE_CFG_IP_VERSION) || !defined(PFE_CFG_IP_VERSION_FPGA_5_0_4) || !defined(PFE_CFG_IP_VERSION_NPU_7_14)
#error Missing version define(s)
#endif /* IP version checks */

#if (PFE_CFG_IP_VERSION == 0)
#error PFE_CFG_IP_VERSION shall not be empty
#endif /* PFE_CFG_IP_VERSION */

#define CBUS_EMAC1_BASE_ADDR		(0xA0000U)
#define CBUS_EGPI1_BASE_ADDR		(0xAC000U)
#define CBUS_ETGPI1_BASE_ADDR		(0xB8000U)
#define CBUS_EMAC2_BASE_ADDR		(0xA4000U)
#define CBUS_EGPI2_BASE_ADDR		(0xB0000U)
#define CBUS_ETGPI2_BASE_ADDR		(0xBC000U)
#define CBUS_EMAC3_BASE_ADDR		(0xA8000U)
#define CBUS_EGPI3_BASE_ADDR		(0xB4000U)
#define CBUS_ETGPI3_BASE_ADDR		(0xC0000U)
#define CBUS_BMU1_BASE_ADDR			(0x88000U)
#define CBUS_BMU2_BASE_ADDR			(0x8C000U)
#define CBUS_HIF_BASE_ADDR			(0x98000U)
#define CBUS_HGPI_BASE_ADDR			(0x9C000U)
#define CBUS_LMEM_BASE_ADDR			(0x00000U)
#define CBUS_LMEM_SIZE				(0x20000U)
#define CBUS_LMEM_END				(LMEM_BASE_ADDR + LMEM_SIZE - 1U)
#define CBUS_TMU_CSR_BASE_ADDR		(0x80000U)
#define CBUS_CLASS_CSR_BASE_ADDR	(0x90000U)
#define CBUS_HIF_NOCPY_BASE_ADDR	(0xD0000U)
#define CBUS_UTIL_CSR_BASE_ADDR		(0xCC000U)
#define CBUS_GLOBAL_CSR_BASE_ADDR	(0x94000U)

#define PFE_CORE_DISABLE			0x00000000U
#define PFE_CORE_ENABLE				0x00000001U
#define PFE_CORE_SW_RESET			0x00000002U

#include "pfe_global_wsp.h"
#include "pfe_class_csr.h"
#include "pfe_tmu_csr.h"
#include "pfe_util_csr.h"
#include "pfe_gpi_csr.h"
#include "pfe_hif_csr.h"
#include "pfe_hif_nocpy_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_emac_csr.h"

#endif /* PFE_CBUS_H_ */
