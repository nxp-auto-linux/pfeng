/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_platform_cfg.h"
#include "pfe_bmu.h"

/* Configuration check */
#if ((PFE_CFG_BMU1_LMEM_BASEADDR + PFE_CFG_BMU1_LMEM_SIZE) > CBUS_LMEM_SIZE)
	#error BMU1 buffers exceed LMEM capacity
#endif

struct pfe_bmu_tag
{
	addr_t cbus_base_va;		/*	CBUS base virtual address */
	addr_t bmu_base_va;		/*	BMU base address (virtual) */
	addr_t pool_va_offset;	/*	Pre-calculated VA-PA conversion offset */
	addr_t pool_base_va;
	addr_t pool_base_pa;
	addr_t pool_size;
#ifdef PFE_CFG_PARANOID_IRQ
	oal_mutex_t lock;
#endif /* PFE_CFG_PARANOID_IRQ */
	addr_t bmu_base_offset;	/*	BMU base offset within CBUS space */
	uint32_t buf_size;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		BMU ISR
 * @param[in]	bmu The BMU instance
 * @return		EOK if interrupt has been handled
 */
__attribute__((cold, aligned(64))) errno_t pfe_bmu_isr(pfe_bmu_t *bmu)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */

		/*	Run the low-level ISR to identify and process the interrupt */
		ret = pfe_bmu_cfg_isr(bmu->bmu_base_va, bmu->cbus_base_va);

	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */
	}

	return ret;
}

/**
 * @brief		Mask BMU interrupts
 * @param[in]	bmu The BMU instance
 */
void pfe_bmu_irq_mask(pfe_bmu_t *bmu)
{
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&bmu->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	pfe_bmu_cfg_irq_mask(bmu->bmu_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&bmu->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */
}

/**
 * @brief		Unmask BMU interrupts
 * @param[in]	hif The BMU instance
 */
void pfe_bmu_irq_unmask(pfe_bmu_t *bmu)
{
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&bmu->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	pfe_bmu_cfg_irq_unmask(bmu->bmu_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&bmu->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */
}

/**
 * @brief		Create new BMU instance
 * @details		Creates and initializes BMU instance. New instance is disabled and needs
 * 				to be enabled by pfe_bmu_enable().
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	bmu_base BMU base address offset within CBUS address space
 * @param[in]	cfg The BMU block configuration
 * @return		The BMU instance or NULL if failed
 */
__attribute__((cold, aligned(64))) pfe_bmu_t *pfe_bmu_create(addr_t cbus_base_va, addr_t bmu_base, const pfe_bmu_cfg_t *cfg)
{
	pfe_bmu_t *bmu;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cfg) || (NULL_ADDR == cbus_base_va)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		bmu = NULL;
	}
	else if (unlikely(NULL_ADDR == cfg->pool_pa))
	{
		NXP_LOG_ERROR("Buffer pool base is NULL\n");
		bmu = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		bmu = oal_mm_malloc(sizeof(pfe_bmu_t));

		if(NULL == bmu)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
		else
		{
			(void)memset(bmu, 0, sizeof(pfe_bmu_t));
			bmu->cbus_base_va = cbus_base_va;
			bmu->bmu_base_offset = bmu_base;
			bmu->bmu_base_va = (bmu->cbus_base_va + bmu->bmu_base_offset);
			bmu->pool_base_pa = cfg->pool_pa;
			bmu->pool_base_va = cfg->pool_va;
			bmu->pool_va_offset = bmu->pool_base_va - bmu->pool_base_pa;
			bmu->pool_size = cfg->buf_size * cfg->max_buf_cnt;
			bmu->buf_size = cfg->buf_size;

	#ifdef PFE_CFG_PARANOID_IRQ
			/*	Resource protection */
			if (EOK != oal_mutex_init(&bmu->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
				oal_mm_free(bmu);
				bmu = NULL;
			}
			else
	#endif /* PFE_CFG_PARANOID_IRQ */
			{
				pfe_bmu_reset(bmu);

			#ifdef PFE_CFG_PARANOID_IRQ
				if (EOK != oal_mutex_lock(&bmu->lock))
				{
					NXP_LOG_ERROR("Mutex lock failed\n");
				}
			#endif /* PFE_CFG_PARANOID_IRQ */

				pfe_bmu_cfg_disable(bmu->bmu_base_va);
				pfe_bmu_cfg_init(bmu->bmu_base_va, cfg);

			#ifdef PFE_CFG_PARANOID_IRQ
				if (EOK != oal_mutex_unlock(&bmu->lock))
				{
					NXP_LOG_ERROR("Mutex unlock failed\n");
				}
			#endif /* PFE_CFG_PARANOID_IRQ */
			}
		}
	}

	return bmu;
}

/**
 * @brief		Reset the BMU block
 * @param[in]	bmu The BMU instance
 */
__attribute__((cold, aligned(64))) void pfe_bmu_reset(pfe_bmu_t *bmu)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */

		ret = pfe_bmu_cfg_reset(bmu->bmu_base_va);

		if (ETIMEDOUT == ret)
		{
			NXP_LOG_ERROR("BMU reset timed-out\n");
		}
		else if (EOK != ret)
		{
			NXP_LOG_ERROR("BMU reset failed: 0x%x\n", ret);
		}
		else
		{
			/*Do Nothing*/
			;
		}

	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */
	}
}

/**
 * @brief		Enable the BMU block
 * @param[in]	bmu The BMU instance
 */
__attribute__((cold, aligned(64))) void pfe_bmu_enable(pfe_bmu_t *bmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */

		pfe_bmu_cfg_enable(bmu->bmu_base_va);

	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */
	}
}

/**
 * @brief		Disable the BMU block
 * @param[in]	bmu The BMU instance
 */
__attribute__((cold, aligned(64))) void pfe_bmu_disable(pfe_bmu_t *bmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */

		pfe_bmu_cfg_disable(bmu->bmu_base_va);

	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */
	}
}

/**
 * @brief		Allocate buffer via BMU
 * @param[in]	bmu The BMU instance
 * @return		Allocated buffer pointer (physical)
 * @note		Thread safe
 */
__attribute__((hot)) void *pfe_bmu_alloc_buf(const pfe_bmu_t *bmu)
{
	void *ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	No resource protection here since it is done by register read */
		ret = (void *)pfe_bmu_cfg_alloc_buf(bmu->bmu_base_va);
	}

	return ret;
}

/**
 * @brief		Convert physical buffer address to virtual one
 * @param[in]	bmu The BMU instance
 * @param[in]	pa The address to be converted
 * @return		Associated virtual address or NULL if failed
 */
__attribute__((hot, pure)) void *pfe_bmu_get_va(const pfe_bmu_t *bmu, addr_t pa)
{
	void * ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((bmu->pool_base_pa + bmu->pool_size) < pa)
		{
			/*	TODO: The condition is not sufficient and need to consider buffer size... */
			NXP_LOG_DEBUG("PA out of range\n");
		}
		ret = (void *)(pa + bmu->pool_va_offset);
	}

	return ret;
}

/**
 * @brief		Convert virtual buffer address to physical one
 * @param[in]	bmu The BMU instance
 * @param[in]	pa The address to be converted
 * @return		Associated virtual address or NULL if failed
 */
__attribute__((hot, pure)) void *pfe_bmu_get_pa(const pfe_bmu_t *bmu, addr_t va)
{
	void * ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((bmu->pool_base_va + bmu->pool_size) < va)
		{
			/*	TODO: The condition is not sufficient and need to consider buffer size... */
			NXP_LOG_DEBUG("VA out of range\n");
		}
		ret = (void *)(va - bmu->pool_va_offset);
	}

	return ret;
}

/**
 * @brief		Get BMU buffer
 * @param[in]	bmu The BMU instance
 * @return		Buffer size in number of bytes
 */
__attribute__((cold, aligned(64), pure)) uint32_t pfe_bmu_get_buf_size(const pfe_bmu_t *bmu)
{
	uint32_t len;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		len = bmu->buf_size;
	}
	return len;
}

/**
 * @brief		Free buffer via BMU
 * @param[in]	bmu The BMU instance
 * @param[in]	buffer Pointer (physical) to the buffer to be freed.
 * @note		Thread safe
 */
__attribute__((hot)) void pfe_bmu_free_buf(const pfe_bmu_t *bmu, addr_t buffer)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bmu) || (NULL_ADDR == buffer)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	No resource protection here since it is done by register write */
		pfe_bmu_cfg_free_buf(bmu->bmu_base_va, PFE_CFG_MEMORY_PHYS_TO_PFE(buffer));
	}
}

/**
 * @brief		Destroy BMU instance
 * @param[in]	bmu The BMU instance
 */
__attribute__((cold, aligned(64))) void pfe_bmu_destroy(pfe_bmu_t *bmu)
{
	if (NULL != bmu)
	{
#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */

		pfe_bmu_cfg_disable(bmu->bmu_base_va);
		pfe_bmu_cfg_fini(bmu->bmu_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (EOK != oal_mutex_destroy(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex destroy failed\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */

		oal_mm_free(bmu);
	}
}

/**
 * @brief		Return BMU runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	bmu 		The BMU instance
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
__attribute__((cold, aligned(64))) uint32_t pfe_bmu_get_text_statistics(const pfe_bmu_t *bmu, struct seq_file *seq, uint8_t verb_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_bmu_cfg_get_text_stat(bmu->bmu_base_va, seq, verb_level);
	}
	return 0;
}

#ifdef PFE_CFG_PFE_MASTER
/**
 * @brief		BMU error polling
 * @param[in]	bmu The BMU instance
 * @return		Return BMU interrupt source error
 */
__attribute__((hot)) uint32_t pfe_bmu_get_err_poll(pfe_bmu_t *bmu)
{
	uint32_t int_src = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		int_src = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */

		/*	Run the low-level polling to identify and process the interrupt source error */
		int_src = pfe_bmu_cfg_get_err_poll(bmu->bmu_base_va);

	#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&bmu->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	#endif /* PFE_CFG_PARANOID_IRQ */
	}

	return int_src;
}
#endif /* #ifdef PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

