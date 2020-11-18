/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2019-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_safety.h"
#include "pfe_safety_csr.h"


struct __pfe_safety_tag
{
	void *cbus_base_va;
	void *safety_base_offset;
	void *safety_base_va;
	oal_mutex_t *lock;
};

/**
 * @brief		Create new SAFETY instance
 * @details		Create and initializes SAFETY instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va SAFETY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in safety thread
 */
pfe_safety_t *pfe_safety_create(void *cbus_base_va, void *safety_base)
{
	pfe_safety_t *safety;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	safety = oal_mm_malloc(sizeof(pfe_safety_t));

	if (NULL == safety)
	{
		return NULL;
	}
	else
	{
		memset(safety, 0, sizeof(pfe_safety_t));
		safety->cbus_base_va = cbus_base_va;
		safety->safety_base_offset = safety_base;
		safety->safety_base_va = (void *)((addr_t)safety->cbus_base_va + (addr_t)safety->safety_base_offset);

		/*	Create mutex */
		safety->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == safety->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_safety_destroy(safety);
			return NULL;
		}
		else
		{
			(void)oal_mutex_init(safety->lock);
		}

		/* Unmask all interrupts */
		pfe_safety_cfg_irq_unmask_all(safety->safety_base_va);
	}

	return safety;
}

/**
 * @brief		Destroy SAFETY instance
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_destroy(pfe_safety_t *safety)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Mask safety interrupts */
	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_mask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);

	if (NULL != safety)
	{
		if (NULL != safety->lock)
		{
			(void)oal_mutex_destroy(safety->lock);
			(void)oal_mm_free(safety->lock);
			safety->lock = NULL;
		}

		/* Free memory used for structure */
		(void)oal_mm_free(safety);
	}
}

/**
 * @brief		SAFETY ISR
 * @param[in]	safety The SAFETY instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_safety_isr(pfe_safety_t *safety)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_safety_cfg_isr(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);

	return ret;
}

/**
 * @brief		Mask SAFETY interrupts
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_irq_mask(pfe_safety_t *safety)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_mask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);
}

/**
 * @brief		Unmask SAFETY interrupts
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_irq_unmask(pfe_safety_t *safety)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_unmask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);
}
