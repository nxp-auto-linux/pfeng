/* =========================================================================
 *  
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_fail_stop.h"
#include "pfe_fail_stop_csr.h"


struct pfe_fail_stop_tag
{
	addr_t cbus_base_va;
	addr_t fail_stop_base_offset;
	addr_t fail_stop_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new FAIL_STOP instance
 * @details		Create and initializes FAIL_STOP instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va FAIL_STOP register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in SAFETY thread
 */
pfe_fail_stop_t *pfe_fail_stop_create(addr_t cbus_base_va, addr_t fail_stop_base)
{
	pfe_fail_stop_t *fail_stop;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	fail_stop = oal_mm_malloc(sizeof(pfe_fail_stop_t));

	if (NULL == fail_stop)
	{
		return NULL;
	}
	else
	{
		(void)memset(fail_stop, 0, sizeof(pfe_fail_stop_t));
		fail_stop->cbus_base_va = cbus_base_va;
		fail_stop->fail_stop_base_offset = fail_stop_base;
		fail_stop->fail_stop_base_va = (fail_stop->cbus_base_va + fail_stop->fail_stop_base_offset);

		/*	Create mutex */
		fail_stop->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == fail_stop->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_fail_stop_destroy(fail_stop);
			return NULL;
		}
		else
		{
			if (EOK != oal_mutex_init(fail_stop->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
			}
		}

		/* Unmask all interrupts */
		pfe_fail_stop_cfg_irq_unmask_all(fail_stop->fail_stop_base_va);
	}

	return fail_stop;
}

/**
 * @brief		Destroy FAIL_STOP instance
 * @param[in]	fail_stop The FAIL_STOP instance
 */
void pfe_fail_stop_destroy(pfe_fail_stop_t *fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != fail_stop->lock)
	{
		/* Mask fail_stop interrupts */
		if (EOK != oal_mutex_lock(fail_stop->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		pfe_fail_stop_cfg_irq_mask(fail_stop->fail_stop_base_va);
		if (EOK != oal_mutex_unlock(fail_stop->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
		(void)oal_mutex_destroy(fail_stop->lock);
		(void)oal_mm_free(fail_stop->lock);
		fail_stop->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(fail_stop);
}

/**
 * @brief		FAIL_STOP ISR
 * @param[in]	fail_stop The FAIL_STOP instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_fail_stop_isr(const pfe_fail_stop_t *fail_stop)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_fail_stop_cfg_isr(fail_stop->fail_stop_base_va);
	if (EOK != oal_mutex_unlock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Mask FAIL_STOP interrupts
 * @param[in]	fail_stop The FAIL_STOP instance
 */
void pfe_fail_stop_irq_mask(const pfe_fail_stop_t *fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_fail_stop_cfg_irq_mask(fail_stop->fail_stop_base_va);
	if (EOK != oal_mutex_unlock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask FAIL_STOP interrupts
 * @param[in]	fail_stop The FAIL_STOP instance
 */
void pfe_fail_stop_irq_unmask(const pfe_fail_stop_t *fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_fail_stop_cfg_irq_unmask(fail_stop->fail_stop_base_va);
	if (EOK != oal_mutex_unlock(fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
