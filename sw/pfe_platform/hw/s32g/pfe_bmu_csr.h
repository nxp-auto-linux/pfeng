/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_BMU_CSR_H_
#define PFE_BMU_CSR_H_

#include "pfe_bmu.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#define BMU_VERSION					0x000U
#define BMU_CTRL					0x004U
#define BMU_UCAST_CONFIG			0x008U
#define BMU_UCAST_BASEADDR			0x00cU
#define BMU_BUF_SIZE				0x010U
#define BMU_BUF_CNT					0x014U
#define BMU_THRES					0x018U
#define BMU_LOW_WATERMARK			0x050U
#define BMU_HIGH_WATERMARK			0x054U
#define BMU_MCAST_CNT				0x040U
#define BMU_REM_BUF_CNT				0x048U
#define BMU_INT_SRC					0x020U
#define BMU_INT_ENABLE				0x024U
#define BMU_ALLOC_CTRL				0x030U
#define BMU_FREE_CTRL				0x034U
#define BMU_MCAST_ALLOC_CTRL		0x044U
#define BMU_FREE_ERROR_ADDR			0x038U
#define BMU_CURR_BUF_CNT			0x03cU
#define BMU_MAS0_BUF_CNT			0x060U
#define BMU_MAS1_BUF_CNT			0x064U
#define BMU_MAS2_BUF_CNT			0x068U
#define BMU_MAS3_BUF_CNT			0x06cU
#define BMU_MAS4_BUF_CNT			0x070U
#define BMU_MAS5_BUF_CNT			0x074U
#define BMU_MAS6_BUF_CNT			0x078U
#define BMU_MAS7_BUF_CNT			0x07cU
#define BMU_MAS8_BUF_CNT			0x080U
#define BMU_MAS9_BUF_CNT			0x084U
#define BMU_MAS10_BUF_CNT			0x088U
#define BMU_MAS11_BUF_CNT			0x08cU
#define BMU_MAS12_BUF_CNT			0x090U
#define BMU_MAS13_BUF_CNT			0x094U
#define BMU_MAS14_BUF_CNT			0x098U
#define BMU_MAS15_BUF_CNT			0x09cU
#define BMU_MAS16_BUF_CNT			0x0a0U
#define BMU_MAS17_BUF_CNT			0x0a4U
#define BMU_MAS18_BUF_CNT			0x0a8U
#define BMU_MAS19_BUF_CNT			0x0acU
#define BMU_MAS20_BUF_CNT			0x0b0U
#define BMU_MAS21_BUF_CNT			0x0b4U
#define BMU_MAS22_BUF_CNT			0x0b8U
#define BMU_MAS23_BUF_CNT			0x0bcU
#define BMU_MAS24_BUF_CNT			0x0c0U
#define BMU_MAS25_BUF_CNT			0x0c4U
#define BMU_MAS26_BUF_CNT			0x0c8U
#define BMU_MAS27_BUF_CNT			0x0ccU
#define BMU_MAS28_BUF_CNT			0x0d0U
#define BMU_MAS29_BUF_CNT			0x0d4U
#define BMU_MAS30_BUF_CNT			0x0d8U
#define BMU_MAS31_BUF_CNT			0x0dcU
#define BMU_DEBUG_BUS				0x0e0U
#define BMU_INT_MEM_ACCESS			0x100U
#define BMU_INT_MEM_ACCESS2			0x104U
#define BMU_INT_MEM_ACCESS_ADDR		0x108U
#define BMU_BUF_CNT_MEM_ACCESS		0x10cU
#define BMU_BUF_CNT_MEM_ACCESS2		0x110U
#define BMU_BUF_CNT_MEM_ACCESS_ADDR	0x114U

#define BMU_INT						(1UL << 0U)
#define BMU_EMPTY_INT				(1UL << 1U)
#define BMU_FULL_INT				(1UL << 2U)
#define BMU_THRES_INT				(1UL << 3U)
#define BMU_FREE_ERR_INT			(1UL << 4U)
#define BMU_MCAST_EMPTY_INT			(1UL << 5U)
#define BMU_MCAST_FULL_INT			(1UL << 6U)
#define BMU_MCAST_THRES_INT			(1UL << 7U)
#define BMU_MCAST_FREE_ERR_INT		(1UL << 8U)

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t pfe_bmu_cfg_isr(addr_t base_va, addr_t cbus_base_va);
 void pfe_bmu_cfg_irq_mask(addr_t base_va);
 void pfe_bmu_cfg_irq_unmask(addr_t base_va);
void pfe_bmu_cfg_init(addr_t base_va, const pfe_bmu_cfg_t *cfg);
void pfe_bmu_cfg_fini(addr_t base_va);
errno_t pfe_bmu_cfg_reset(addr_t base_va);
void pfe_bmu_cfg_enable(addr_t base_va);
void pfe_bmu_cfg_disable(addr_t base_va);
void * pfe_bmu_cfg_alloc_buf(addr_t base_va);
void pfe_bmu_cfg_free_buf(addr_t base_va, addr_t buffer);

uint32_t pfe_bmu_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level);

uint32_t pfe_bmu_cfg_get_err_poll(addr_t base_va);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_BMU_CSR_H_ */
