/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_hm.h"
#include "pfe_cbus.h"
#include "pfe_bmu_csr.h"


#define IS_POWER_OF_2(n) (((n) != 0U) && (((n) & ((n) - 1U)) == 0U))

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static void pfe_bmu_cfg_clear_buf_cnt_memory(addr_t base_va, uint32_t cnt);
static void pfe_bmu_cfg_clear_internal_memory(addr_t base_va, uint32_t cnt);

static void pfe_bmu_cfg_clear_buf_cnt_memory(addr_t base_va, uint32_t cnt)
{
	uint32_t ii;

	for (ii=0U; ii<cnt; ii++)
	{
		hal_write32(ii, base_va + BMU_BUF_CNT_MEM_ACCESS_ADDR);
		hal_write32(0U, base_va + BMU_BUF_CNT_MEM_ACCESS);
		hal_write32(0U, base_va + BMU_BUF_CNT_MEM_ACCESS2);
	}
}

static void pfe_bmu_cfg_clear_internal_memory(addr_t base_va, uint32_t cnt)
{
	uint32_t ii;

	for (ii=0U; ii<cnt; ii++)
	{
		hal_write32(ii, base_va + BMU_INT_MEM_ACCESS_ADDR);
		hal_write32(0U, base_va + BMU_INT_MEM_ACCESS);
		hal_write32(0U, base_va + BMU_INT_MEM_ACCESS2);
	}
}

/**
 * @brief		BMU ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * 				Every BMU instance has its own handler. Access to registers is
 * 				protected by mutex implemented within the BMU module (pfe_bmu.c).
 * @param[in]	base_va BMU register space base address (virtual)
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-BMU mutex
 */
errno_t pfe_bmu_cfg_isr(addr_t base_va, addr_t cbus_base_va)
{
	uint32_t reg_src, reg_en, reg, reg_reen = 0U;
	errno_t ret = ENOENT;
	addr_t bmu_offset = base_va - cbus_base_va;
#ifndef NXP_LOG_ENABLED
	(void)bmu_offset;
#endif /* NXP_LOG_ENABLED */

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + BMU_INT_ENABLE);
	/*	Disable ALL */
	hal_write32(0U, base_va + BMU_INT_ENABLE);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + BMU_INT_SRC);
	/*	ACK triggered */
	hal_write32(reg_src, base_va + BMU_INT_SRC);
	/*	Enable the non-triggered ones. Keep the master and error interrupt bits set. */
	hal_write32((reg_en & ~reg_src)|BMU_FREE_ERR_INT, base_va + BMU_INT_ENABLE);

	/*	Process interrupts which are triggered AND enabled */
	if ((reg_src & reg_en & BMU_EMPTY_INT) != 0U)
	{
		/*	This means that zero buffers are allocated from the BMU pool,
			i.e. all buffers are free, i.e. number of allocated buffers is
			zero. */
		NXP_LOG_INFO("BMU_EMPTY_INT (BMU @ p0x%p). Pool ready.\n", (void *)bmu_offset);

		/*	Stay disabled but re-enable the "threshold" and "full" interrupt */
		reg_reen |= BMU_THRES_INT|BMU_FULL_INT;
		ret = EOK;
	}

	if ((reg_src & reg_en & BMU_FULL_INT) != 0U)
	{
		/*	All BMU buffers are allocated, i.e. no new buffer can be allocated. */
		pfe_hm_report_error(HM_SRC_BMU, HM_EVT_BMU_FULL, "(BMU @ p0x%p)", (void *)bmu_offset);

		/*	Stay disabled but re-enable the "empty" interrupt */
		reg_reen |= BMU_EMPTY_INT;
		ret = EOK;
	}

	if ((reg_src & reg_en & BMU_THRES_INT) != 0U)
	{
		/*	More (or equal) than "threshold" number of buffers have been
			allocated. Read and print the threshold value. Stay disabled. */
		reg = hal_read32(base_va + BMU_THRES);
		NXP_LOG_INFO("BMU_THRES_INT (BMU @ p0x%p). Pool being depleted. Threshold: %u.\n", (void *)bmu_offset, ((uint_t)reg & 0xffffU));

		/*	Stay disabled but re-enable the "empty" interrupt */
		reg_reen |= BMU_EMPTY_INT;
		ret = EOK;
	}

	if ((reg_src & reg_en & BMU_FREE_ERR_INT) != 0U)
	{
		/*	Free error interrupt. Keep this one always enabled */
		pfe_hm_report_error(HM_SRC_BMU, HM_EVT_BMU_FREE_ERR, "(BMU @ p0x%p) address 0x%x", (void *)bmu_offset, (uint_t)hal_read32(base_va + BMU_FREE_ERROR_ADDR));
		ret = EOK;
	}

	if ((reg_src & reg_en & (BMU_MCAST_EMPTY_INT|BMU_MCAST_FULL_INT|BMU_MCAST_THRES_INT|BMU_MCAST_FREE_ERR_INT)) != 0U)
	{
		/*	This should never happen. TRM says that all BMU_MCAST_* flags are reserved and always 0 */
		pfe_hm_report_error(HM_SRC_BMU, HM_EVT_BMU_MCAST, "(BMU @ p0x%p)", (void *)bmu_offset);
		ret = EOK;
	}

	/*	Re-enable requested interrupts */
	if (0U != reg_reen)
	{
		reg = hal_read32(base_va + BMU_INT_ENABLE);
		hal_write32(reg|reg_reen, base_va + BMU_INT_ENABLE);
	}

	return ret;
}

/**
 * @brief		Mask BMU interrupts
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @note		Make sure the call is protected by some per-BMU mutex
 */
void pfe_bmu_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	/*	Mask group */
	reg = hal_read32(base_va + BMU_INT_ENABLE) & ~(BMU_INT);
	hal_write32(reg, base_va + BMU_INT_ENABLE);
}

/**
 * @brief		Unmask BMU interrupts
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @note		Make sure the call is protected by some per-BMU mutex
 */
void pfe_bmu_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	/*	Unmask group */
	reg = hal_read32(base_va + BMU_INT_ENABLE) | BMU_INT;
	hal_write32(reg, base_va + BMU_INT_ENABLE);
}

/**
 * @brief		Initialize and configure the BMU block
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @param[in]	cfg Pointer to the configuration structure
 */
void pfe_bmu_cfg_init(addr_t base_va, const pfe_bmu_cfg_t *cfg)
{
	uint32_t bmu_buf_size_exp;

	if (unlikely(FALSE == IS_POWER_OF_2(cfg->buf_size)))
	{
		NXP_LOG_ERROR("BMU buffer size is not power of 2\n");
	}
	else
	{
		hal_write32(0U, base_va + BMU_CTRL);
		hal_write32(0x0U, base_va + BMU_INT_ENABLE);
		hal_write32(0xffffffffU, base_va + BMU_INT_SRC);

		hal_write32((uint32_t)(cfg->pool_pa & 0xffffffffU), base_va + BMU_UCAST_BASEADDR);
		hal_write32(cfg->max_buf_cnt & 0xffffU, base_va + BMU_UCAST_CONFIG);

		for(bmu_buf_size_exp = 0; bmu_buf_size_exp < (sizeof(cfg->buf_size) * 8U); bmu_buf_size_exp++)
		{
			if(cfg->buf_size == (1UL << bmu_buf_size_exp))
			{
				hal_write32(bmu_buf_size_exp & 0xffffU, base_va + BMU_BUF_SIZE);
				break;
			}
		}

		/*	Thresholds. 75% of maximum number of available buffers. */
		hal_write32((cfg->max_buf_cnt * 75U) / 100U, base_va + BMU_THRES);

		/*	Low Watermark for pause frame generation start 5% of free buffers. */
		hal_write32((cfg->max_buf_cnt * 5U) / 100U, base_va + BMU_LOW_WATERMARK);
		/*	High Watermark for pause frame generation stop 10% of free buffers. */
		hal_write32((cfg->max_buf_cnt * 10U) / 100U, base_va + BMU_HIGH_WATERMARK);

		pfe_bmu_cfg_clear_internal_memory(base_va, cfg->int_mem_loc_cnt);
		pfe_bmu_cfg_clear_buf_cnt_memory(base_va, cfg->buf_mem_loc_cnt);

		/*	Enable BMU interrupts except the global enable bit */
		hal_write32(0xffffffffU & ~(BMU_INT), base_va + BMU_INT_ENABLE);
	}
}

/**
 * @brief		Finalize the BMU
 * @param[in]	base_va Base address of HIF register space (virtual)
 */
void pfe_bmu_cfg_fini(addr_t base_va)
{
	hal_write32(0U, base_va + BMU_CTRL);
	hal_write32(0x0U, base_va + BMU_INT_ENABLE);
	hal_write32(0xffffffffU, base_va + BMU_INT_SRC);
}

/**
 * @brief		BMU reset
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @return		EOK if success or error code otherwise
 */
errno_t pfe_bmu_cfg_reset(addr_t base_va)
{
	uint32_t ii = 0U;
	errno_t ret = EOK;

	hal_write32(0x2U, base_va + BMU_CTRL);
	while ((hal_read32(base_va + BMU_CTRL) & 0x2U) != 0U)
	{
		if (++ii > 1000U)
		{
			ret = ETIMEDOUT;
			break;
		}
		else
		{
			oal_time_usleep(10U);
		}
	}

	return ret;
}

/**
 * @brief		Enable the BMU block
 * @param[in]	base_va Base address of the BMU register space (virtual)
 */
void pfe_bmu_cfg_enable(addr_t base_va)
{
	hal_write32(0x1U, base_va + BMU_CTRL);
}

/**
 * @brief		Disable the BMU block
 * @param[in]	base_va Base address of the BMU register space (virtual)
 */
void pfe_bmu_cfg_disable(addr_t base_va)
{
	hal_write32(0x0U, base_va + BMU_CTRL);
}

/**
 * @brief		Allocate buffer from BMU
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @return		Pointer to the allocated buffer
 */
void * pfe_bmu_cfg_alloc_buf(addr_t base_va)
{
	return (void *)(addr_t)hal_read32(base_va + BMU_ALLOC_CTRL);
}

/**
 * @brief		Free a previously allocated buffer
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @param[in]	buffer Pointer to the buffer to be released
 */
void pfe_bmu_cfg_free_buf(addr_t base_va, addr_t buffer)
{
	hal_write32((uint32_t)(buffer & 0xffffffffU), base_va + BMU_FREE_CTRL);
}

/**
 * @brief		Get BMU statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the BMU block.
 * @param[in]	base_va Base address of BMU register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_bmu_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg, ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received (pfe_bmu_cfg_get_text_stat)\n");
		len = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(verb_level >= 10U)
		{
			seq_printf(seq, "BMU_REM_BUF_CNT     : 0x%x\n", hal_read32(base_va + BMU_REM_BUF_CNT));
			seq_printf(seq, "BMU_FREE_ERROR_ADDR : 0x%x\n", hal_read32(base_va + BMU_FREE_ERROR_ADDR));
			seq_printf(seq, "BMU_CURR_BUF_CNT    : 0x%x\n", hal_read32(base_va + BMU_CURR_BUF_CNT));
			seq_printf(seq, "BMU_DEBUG_BUS       : 0x%x\n", hal_read32(base_va + BMU_DEBUG_BUS));
		}

		if(verb_level >= 9U)
		{
			/*	Get version */
			reg = hal_read32(base_va + BMU_VERSION);
			seq_printf(seq, "Revision             : 0x%x\n", (reg >> 24) & 0xffU);
			seq_printf(seq, "Version              : 0x%x\n", (reg >> 16) & 0xffU);
			seq_printf(seq, "ID                   : 0x%x\n", reg & 0xffffU);
		}
		seq_printf(seq, "Buffer Base (uc)     : p0x%x\n", (uint32_t)hal_read32(base_va + BMU_UCAST_BASEADDR));
		seq_printf(seq, "Buffer Size          : 0x%x\n", 1U << hal_read32(base_va + BMU_BUF_SIZE));
		seq_printf(seq, "Buffers Remaining    : 0x%x\n", hal_read32(base_va + BMU_REM_BUF_CNT));
		seq_printf(seq, "Buffers Allocated    : 0x%x\n", hal_read32(base_va + BMU_CURR_BUF_CNT));
		seq_printf(seq, "Low Watermark        : 0x%x\n", hal_read32(base_va + BMU_LOW_WATERMARK));
		seq_printf(seq, "High Watermark       : 0x%x\n", hal_read32(base_va + BMU_HIGH_WATERMARK));
		seq_printf(seq, "IRQ Threshold (uc)   : 0x%x\n", hal_read32(base_va + BMU_THRES) & 0xffffU);
		seq_printf(seq, "Free Error Address   : 0x%x\n", hal_read32(base_va + BMU_FREE_ERROR_ADDR));
		reg = hal_read32(base_va + BMU_BUF_CNT);
		seq_printf(seq, "Free Error Count     : 0x%x\n", reg >> 16);
		seq_printf(seq, "Active Buffers       : 0x%x\n", reg & 0xffffU);

		seq_printf(seq, "IRQ Source           : 0x%x\n", hal_read32(base_va + BMU_INT_SRC));
		seq_printf(seq, "IRQ Enable           : 0x%x\n", hal_read32(base_va + BMU_INT_ENABLE));

		for (ii=0; ii<32U; ii++)
		{
			reg = hal_read32(base_va + BMU_MAS0_BUF_CNT + (4U*ii));
			if (0U != reg)
			{
				seq_printf(seq, "MASTER%02d Count       : 0x%x\n", ii, reg);
			}
		}
	}
	return len;
}

/**
 * @brief		BMU error detect in polling
 * @details		check and return the interrupt source in polling mode
 * @param[in]	base_va Base address of BMU register space (virtual)
 * @return		return BMU error interrupt source
 * @note		This is polling function and will be called to check the error status of BMU
 * 				Make sure the call is protected by some per-BMU mutex
 */
uint32_t pfe_bmu_cfg_get_err_poll(addr_t base_va)
{
	uint32_t int_src;

	int_src = hal_read32(base_va + BMU_INT_SRC);

	if(0U != int_src)
	{
		/*	Write 1 to clear */
		hal_write32(int_src, base_va + BMU_INT_SRC);

		NXP_LOG_INFO("BMU_ERR_INT_SRC (0x%x)\n", (uint_t)int_src);
	}

	return int_src;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

