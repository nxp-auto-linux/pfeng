/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_BMU_H_
#define PUBLIC_PFE_BMU_H_

typedef struct pfe_bmu_tag pfe_bmu_t;

typedef struct
{
	addr_t pool_pa;				/*	Buffer pool base (physical, as seen by PFE). Needs to be aligned to buf_cnt * buf_size. */
	addr_t pool_va;				/*  Buffer pool base (virtual) */
	uint32_t max_buf_cnt;		/*	Maximum number of buffers that can be used */
	uint32_t buf_size;			/*	Buffer size of each of the buffers allocated and freed (size = 2^buf_size) */
	uint32_t bmu_ucast_thres;
	uint32_t bmu_mcast_thres;
	uint32_t int_mem_loc_cnt;
	uint32_t buf_mem_loc_cnt;
} pfe_bmu_cfg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_bmu_t *pfe_bmu_create(addr_t cbus_base_va, addr_t bmu_base, const pfe_bmu_cfg_t *cfg) __attribute__((cold, aligned(64)));
errno_t pfe_bmu_isr(pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
void pfe_bmu_irq_mask(pfe_bmu_t *bmu);
void pfe_bmu_irq_unmask(pfe_bmu_t *bmu);
void pfe_bmu_enable(pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
void pfe_bmu_reset(pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
void pfe_bmu_disable(pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
void *pfe_bmu_alloc_buf(const pfe_bmu_t *bmu) __attribute__((hot));
void *pfe_bmu_get_va(const pfe_bmu_t *bmu, addr_t pa) __attribute__((hot, pure));
void *pfe_bmu_get_pa(const pfe_bmu_t *bmu, addr_t va) __attribute__((hot, pure));
uint32_t pfe_bmu_get_buf_size(const pfe_bmu_t *bmu) __attribute__((cold, aligned(64), pure));
void pfe_bmu_free_buf(const pfe_bmu_t *bmu, addr_t buffer) __attribute__((hot));

uint32_t pfe_bmu_get_text_statistics(const pfe_bmu_t *bmu, struct seq_file *seq, uint8_t verb_level) __attribute__((cold, aligned(64)));

void pfe_bmu_destroy(pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
#ifdef PFE_CFG_PFE_MASTER
uint32_t pfe_bmu_get_err_poll(pfe_bmu_t *bmu) __attribute__((hot));
#endif /* #ifdef PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_BMU_H_ */
