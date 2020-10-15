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

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_bmu_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

static void pfe_bmu_cfg_clear_buf_cnt_memory(void *base_va, uint32_t cnt)
{
	uint32_t ii;

	for (ii=0U; ii<cnt; ii++)
	{
		hal_write32(ii, base_va + BMU_BUF_CNT_MEM_ACCESS_ADDR);
		hal_write32(0U, base_va + BMU_BUF_CNT_MEM_ACCESS);
		hal_write32(0U, base_va + BMU_BUF_CNT_MEM_ACCESS2);
	}
}

static void pfe_bmu_cfg_clear_internal_memory(void *base_va, uint32_t cnt)
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
errno_t pfe_bmu_cfg_isr(void *base_va, void *cbus_base_va)
{
	uint32_t reg_src, reg_en, reg, reg_reen = 0U;
	errno_t ret = ENOENT;
	addr_t bmu_offset = (addr_t)base_va - (addr_t)cbus_base_va;

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
	if (reg_src & reg_en & BMU_EMPTY_INT)
	{
		/*	This means that zero buffers are allocated from the BMU pool,
			i.e. all buffers are free, i.e. number of allocated buffers is
			zero. */
		NXP_LOG_INFO("BMU_EMPTY_INT (BMU @ p0x%p). Pool ready.\n", (void *)bmu_offset);

		/*	Stay disabled but re-enable the "threshold" and "full" interrupt */
		reg_reen |= BMU_THRES_INT|BMU_FULL_INT;
		ret = EOK;
	}

	if (reg_src & reg_en & BMU_FULL_INT)
	{
		/*	All BMU buffers are allocated, i.e. no new buffer can be allocated. */
		NXP_LOG_INFO("BMU_FULL_INT (BMU @ p0x%p). Pool depleted.\n", (void *)bmu_offset);

		/*	Stay disabled but re-enable the "empty" interrupt */
		reg_reen |= BMU_EMPTY_INT;
		ret = EOK;
	}

	if (reg_src & reg_en & BMU_THRES_INT)
	{
		/*	More (or equal) than "threshold" number of buffers have been
			allocated. Read and print the threshold value. Stay disabled. */
		reg = hal_read32(base_va + BMU_THRES);
		NXP_LOG_INFO("BMU_THRES_INT (BMU @ p0x%p). Pool being depleted. Threshold: %d.\n", (void *)bmu_offset, (reg & 0xffffU));

		/*	Stay disabled but re-enable the "empty" interrupt */
		reg_reen |= BMU_EMPTY_INT;
		ret = EOK;
	}

	if (reg_src & reg_en & BMU_FREE_ERR_INT)
	{
		/*	Free error interrupt. Keep this one always enabled */
		NXP_LOG_INFO("BMU_FREE_ERR_INT (BMU @ p0x%p) address 0x%x\n", (void *)bmu_offset, hal_read32(base_va + BMU_FREE_ERROR_ADDR));
		ret = EOK;
	}

	if (reg_src & reg_en & (BMU_MCAST_EMPTY_INT|BMU_MCAST_FULL_INT|BMU_MCAST_THRES_INT|BMU_MCAST_FREE_ERR_INT))
	{
		/*	This should never happen. TRM says that all BMU_MCAST_* flags are reserved and always 0 */
		NXP_LOG_INFO("BMU_MCAST_EMTPY_INT or BMU_MCAST_FULL_INT or BMU_MCAST_THRES_INT or BMU_MCAST_FREE_ERR_INT (BMU @ p0x%p)\n", (void *)bmu_offset);
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
void pfe_bmu_cfg_irq_mask(void *base_va)
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
void pfe_bmu_cfg_irq_unmask(void *base_va)
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
void pfe_bmu_cfg_init(void *base_va, pfe_bmu_cfg_t *cfg)
{
	hal_write32(0U, base_va + BMU_CTRL);
	hal_write32(0x0U, base_va + BMU_INT_ENABLE);
	hal_write32(0xffffffffU, base_va + BMU_INT_SRC);

	hal_write32((uint32_t)((addr_t)cfg->pool_pa & 0xffffffffU), base_va + BMU_UCAST_BASEADDR);
	hal_write32(cfg->max_buf_cnt & 0xffffU, base_va + BMU_UCAST_CONFIG);
	hal_write32(cfg->buf_size & 0xffffU, base_va + BMU_BUF_SIZE);

	/*	Thresholds. 75% of maximum number of available buffers. */
	hal_write32((cfg->max_buf_cnt * 75U) / 100U, base_va + BMU_THRES);

	pfe_bmu_cfg_clear_internal_memory(base_va, cfg->int_mem_loc_cnt);
	pfe_bmu_cfg_clear_buf_cnt_memory(base_va, cfg->buf_mem_loc_cnt);

	/*	Enable BMU interrupts except the global enable bit */
	hal_write32(0xffffffffU & ~(BMU_INT), base_va + BMU_INT_ENABLE);
}

/**
 * @brief		Finalize the BMU
 * @param[in]	base_va Base address of HIF register space (virtual)
 */
void pfe_bmu_cfg_fini(void *base_va)
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
errno_t pfe_bmu_cfg_reset(void *base_va)
{
	uint32_t ii = 0U;

	hal_write32(0x2U, base_va + BMU_CTRL);
	while (hal_read32(base_va + BMU_CTRL) & 0x2U)
	{
		if (++ii > 1000U)
		{
			return ETIMEDOUT;
		}
		else
		{
			oal_time_usleep(1000);
		}
	}

	return EOK;
}

/**
 * @brief		Enable the BMU block
 * @param[in]	base_va Base address of the BMU register space (virtual)
 */
void pfe_bmu_cfg_enable(void *base_va)
{
	hal_write32(0x1U, base_va + BMU_CTRL);
}

/**
 * @brief		Disable the BMU block
 * @param[in]	base_va Base address of the BMU register space (virtual)
 */
void pfe_bmu_cfg_disable(void *base_va)
{
	hal_write32(0x0U, base_va + BMU_CTRL);
}

/**
 * @brief		Allocate buffer from BMU
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @return		Pointer to the allocated buffer
 */
void * pfe_bmu_cfg_alloc_buf(void *base_va)
{
	return (void *)(addr_t)hal_read32(base_va + BMU_ALLOC_CTRL);
}

/**
 * @brief		Free a previously allocated buffer
 * @param[in]	base_va Base address of the BMU register space (virtual)
 * @param[in]	buffer Pointer to the buffer to be released
 */
void pfe_bmu_cfg_free_buf(void *base_va, void *buffer)
{
	hal_write32((uint32_t)((addr_t)buffer & 0xffffffffU), base_va + BMU_FREE_CTRL);
}

/**
 * @brief		Get BMU statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the BMU block.
 * @param[in]	base_va Base address of BMU register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_bmu_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg, ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received (pfe_bmu_cfg_get_text_stat)\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(verb_level >= 10U)
	{
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU_REM_BUF_CNT     : 0x%x\n", hal_read32(base_va + BMU_REM_BUF_CNT));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU_FREE_ERROR_ADDR : 0x%x\n", hal_read32(base_va + BMU_FREE_ERROR_ADDR));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU_CURR_BUF_CNT    : 0x%x\n", hal_read32(base_va + BMU_CURR_BUF_CNT));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU_DEBUG_BUS       : 0x%x\n", hal_read32(base_va + BMU_DEBUG_BUS));
	}

	if(verb_level >= 9U)
	{
		/*	Get version */
		reg = hal_read32(base_va + BMU_VERSION);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24) & 0xffU);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16) & 0xffU);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffffU);
	}
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Buffer Base (uc)     : p0x%x\n", (uint32_t)hal_read32(base_va + BMU_UCAST_BASEADDR));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Buffer Size          : 0x%x\n", 1U << hal_read32(base_va + BMU_BUF_SIZE));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Buffers Remaining    : 0x%x\n", hal_read32(base_va + BMU_REM_BUF_CNT));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Buffers Allocated    : 0x%x\n", hal_read32(base_va + BMU_CURR_BUF_CNT));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Low Watermark        : 0x%x\n", hal_read32(base_va + BMU_LOW_WATERMARK));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "High Watermark       : 0x%x\n", hal_read32(base_va + BMU_HIGH_WATERMARK));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "IRQ Threshold (uc)   : 0x%x\n", hal_read32(base_va + BMU_THRES) & 0xffffU);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Free Error Address   : 0x%x\n", hal_read32(base_va + BMU_FREE_ERROR_ADDR));
		reg = hal_read32(base_va + BMU_BUF_CNT);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Free Error Count     : 0x%x\n", reg >> 16);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Active Buffers       : 0x%x\n", reg & 0xffffU);

		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "IRQ Source           : 0x%x\n", hal_read32(base_va + BMU_INT_SRC));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "IRQ Enable           : 0x%x\n", hal_read32(base_va + BMU_INT_ENABLE));

		for (ii=0; ii<32U; ii++)
		{
			reg = hal_read32(base_va + BMU_MAS0_BUF_CNT + (4U*ii));
			if (0U != reg)
			{
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "MASTER%02d Count       : 0x%x\n", ii, reg);
			}
		}
	return len;
}
