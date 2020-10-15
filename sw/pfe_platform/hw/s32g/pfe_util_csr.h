/* =========================================================================
 *  Copyright 2015-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2020 NXP
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

#ifndef _UTIL_CSR_H_
#define _UTIL_CSR_H_

#include "pfe_util.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

#define UTIL_VERSION				(CBUS_UTIL_CSR_BASE_ADDR + 0x000U)
#define UTIL_TX_CTRL				(CBUS_UTIL_CSR_BASE_ADDR + 0x004U)
#define UTIL_INQ_PKTPTR				(CBUS_UTIL_CSR_BASE_ADDR + 0x010U)

#define UTIL_HDR_SIZE				(CBUS_UTIL_CSR_BASE_ADDR + 0x014U)

#define UTIL_PE0_QB_DM_ADDR0		(CBUS_UTIL_CSR_BASE_ADDR + 0x020U)
#define UTIL_PE0_QB_DM_ADDR1		(CBUS_UTIL_CSR_BASE_ADDR + 0x024U)
#define UTIL_PE0_RO_DM_ADDR0		(CBUS_UTIL_CSR_BASE_ADDR + 0x060U)
#define UTIL_PE0_RO_DM_ADDR1		(CBUS_UTIL_CSR_BASE_ADDR + 0x064U)

#define UTIL_MEM_ACCESS_ADDR		(CBUS_UTIL_CSR_BASE_ADDR + 0x100U)
#define UTIL_MEM_ACCESS_WDATA		(CBUS_UTIL_CSR_BASE_ADDR + 0x104U)
#define UTIL_MEM_ACCESS_RDATA		(CBUS_UTIL_CSR_BASE_ADDR + 0x108U)

#define UTIL_TM_INQ_ADDR			(CBUS_UTIL_CSR_BASE_ADDR + 0x114U)
#define UTIL_PE_STATUS				(CBUS_UTIL_CSR_BASE_ADDR + 0x118U)

#define UTIL_PE_SYS_CLK_RATIO		(CBUS_UTIL_CSR_BASE_ADDR + 0x200U)
#define UTIL_AFULL_THRES			(CBUS_UTIL_CSR_BASE_ADDR + 0x204U)
#define UTIL_GAP_BETWEEN_READS		(CBUS_UTIL_CSR_BASE_ADDR + 0x208U)
#define UTIL_MAX_BUF_CNT			(CBUS_UTIL_CSR_BASE_ADDR + 0x20cU)
#define UTIL_TSQ_FIFO_THRES			(CBUS_UTIL_CSR_BASE_ADDR + 0x210U)
#define UTIL_TSQ_MAX_CNT			(CBUS_UTIL_CSR_BASE_ADDR + 0x214U)
#define UTIL_IRAM_DATA_0			(CBUS_UTIL_CSR_BASE_ADDR + 0x218U)
#define UTIL_IRAM_DATA_1			(CBUS_UTIL_CSR_BASE_ADDR + 0x21cU)
#define UTIL_IRAM_DATA_2			(CBUS_UTIL_CSR_BASE_ADDR + 0x220U)
#define UTIL_IRAM_DATA_3			(CBUS_UTIL_CSR_BASE_ADDR + 0x224U)

#define UTIL_BUS_ACCESS_ADDR		(CBUS_UTIL_CSR_BASE_ADDR + 0x228U)
#define UTIL_BUS_ACCESS_WDATA		(CBUS_UTIL_CSR_BASE_ADDR + 0x22cU)
#define UTIL_BUS_ACCESS_RDATA		(CBUS_UTIL_CSR_BASE_ADDR + 0x230U)

#define UTIL_INQ_AFULL_THRES		(CBUS_UTIL_CSR_BASE_ADDR + 0x234U)

#define UTIL_PE_IBUS_ACCESS_PMEM	(1U << 17)
#define UTIL_PE_IBUS_ACCESS_DMEM	(1U << 18)
#define UTIL_PE_IBUS_DMEM_BASE(i)	((((i) & 0x3) << 20) | UTIL_PE_IBUS_ACCESS_DMEM)
#define UTIL_PE_IBUS_PMEM_BASE(i)	((((i) & 0x3) << 20) | UTIL_PE_IBUS_ACCESS_PMEM)

uint32_t pfe_util_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level);

#endif /* _UTIL_CSR_H_ */
