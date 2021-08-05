/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2015-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef UTIL_CSR_H_
#define UTIL_CSR_H_

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
#define UTIL_UPE_GP_REG_ADDR		(CBUS_UTIL_CSR_BASE_ADDR + 0x238U)
#define UTIL_HOST_GP_REG_ADDR		(CBUS_UTIL_CSR_BASE_ADDR + 0x23CU)

#define UTIL_PE_IBUS_ACCESS_PMEM	(1UL << 17U)
#define UTIL_PE_IBUS_ACCESS_DMEM	(1UL << 18U)
#define UTIL_PE_IBUS_DMEM_BASE(i)	((((i) & 0x3) << 20U) | UTIL_PE_IBUS_ACCESS_DMEM)
#define UTIL_PE_IBUS_PMEM_BASE(i)	((((i) & 0x3) << 20U) | UTIL_PE_IBUS_ACCESS_PMEM)

uint32_t pfe_util_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level);
errno_t pfe_util_cfg_isr(addr_t base_va);

#endif /* UTIL_CSR_H_ */
