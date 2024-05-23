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
#include "pfe_host_fail_stop.h"
#include "pfe_host_fail_stop_csr.h"


struct pfe_host_fail_stop_tag
{
	addr_t cbus_base_va;
	addr_t host_fail_stop_base_offset;
	addr_t host_fail_stop_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new SAFETY_HOST_FAIL_STOP instance
 * @details		Create and initializes SAFETY_HOST_FAIL_STOP instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va SAFETY_HOST_FAIL_STOP register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in SAFETY thread
 */
pfe_host_fail_stop_t *pfe_host_fail_stop_create(addr_t cbus_base_va, addr_t host_fail_stop_base)
{
	pfe_host_fail_stop_t *host_fail_stop;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	host_fail_stop = oal_mm_malloc(sizeof(pfe_host_fail_stop_t));

	if (NULL == host_fail_stop)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		(void)memset(host_fail_stop, 0, sizeof(pfe_host_fail_stop_t));
		host_fail_stop->cbus_base_va = cbus_base_va;
		host_fail_stop->host_fail_stop_base_offset = host_fail_stop_base;
		host_fail_stop->host_fail_stop_base_va = (host_fail_stop->cbus_base_va + host_fail_stop->host_fail_stop_base_offset);

		/*	Create mutex */
		host_fail_stop->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == host_fail_stop->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_host_fail_stop_destroy(host_fail_stop);
			return NULL;
		}
		else
		{
			if (EOK != oal_mutex_init(host_fail_stop->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
			}
		}

		/* Unmask all interrupts */
		pfe_host_fail_stop_cfg_irq_unmask_all(host_fail_stop->host_fail_stop_base_va);
	}

	return host_fail_stop;
}

/**
 * @brief		Destroy SAFETY_HOST_FAIL_STOP instance
 * @param[in]	host_fail_stop The SAFETY_HOST_FAIL_STOP instance
 */
void pfe_host_fail_stop_destroy(pfe_host_fail_stop_t *host_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == host_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != host_fail_stop->lock)
	{
		/* Mask host_fail_stop interrupts */
		if (EOK != oal_mutex_lock(host_fail_stop->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		pfe_host_fail_stop_cfg_irq_mask(host_fail_stop->host_fail_stop_base_va);
		if (EOK != oal_mutex_unlock(host_fail_stop->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
		(void)oal_mutex_destroy(host_fail_stop->lock);
		(void)oal_mm_free(host_fail_stop->lock);
		host_fail_stop->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(host_fail_stop);
}

/**
 * @brief		SAFETY_HOST_FAIL_STOP ISR
 * @param[in]	host_fail_stop The SAFETY_HOST_FAIL_STOP instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_host_fail_stop_isr(const pfe_host_fail_stop_t *host_fail_stop)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == host_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_host_fail_stop_cfg_isr(host_fail_stop->host_fail_stop_base_va);
	if (EOK != oal_mutex_unlock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Mask SAFETY_HOST_FAIL_STOP interrupts
 * @param[in]	host_fail_stop The SAFETY_HOST_FAIL_STOP instance
 */
void pfe_host_fail_stop_irq_mask(const pfe_host_fail_stop_t *host_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == host_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_host_fail_stop_cfg_irq_mask(host_fail_stop->host_fail_stop_base_va);
	if (EOK != oal_mutex_unlock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask SAFETY_HOST_FAIL_STOP interrupts
 * @param[in]	host_fail_stop The SAFETY_HOST_FAIL_STOP instance
 */
void pfe_host_fail_stop_irq_unmask(const pfe_host_fail_stop_t *host_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == host_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_host_fail_stop_cfg_irq_unmask(host_fail_stop->host_fail_stop_base_va);
	if (EOK != oal_mutex_unlock(host_fail_stop->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
