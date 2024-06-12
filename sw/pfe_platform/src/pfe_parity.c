/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_parity.h"
#include "pfe_parity_csr.h"


struct pfe_parity_tag
{
	addr_t cbus_base_va;
	addr_t parity_base_offset;
	addr_t parity_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new PARITY instance
 * @details		Create and initializes PARITY instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va PARITY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in SAFETY thread
 */
pfe_parity_t *pfe_parity_create(addr_t cbus_base_va, addr_t parity_base)
{
	pfe_parity_t *parity;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	parity = oal_mm_malloc(sizeof(pfe_parity_t));

	if (NULL == parity)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		(void)memset(parity, 0, sizeof(pfe_parity_t));
		parity->cbus_base_va = cbus_base_va;
		parity->parity_base_offset = parity_base;
		parity->parity_base_va = (parity->cbus_base_va + parity->parity_base_offset);

		/*	Create mutex */
		parity->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == parity->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_parity_destroy(parity);
			return NULL;
		}
		else
		{
			if (EOK != oal_mutex_init(parity->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
			}
		}

		/* Unmask all interrupts */
		pfe_parity_cfg_irq_unmask_all(parity->parity_base_va);
	}

	return parity;
}

/**
 * @brief		Destroy PARITY instance
 * @param[in]	parity The PARITY instance
 */
void pfe_parity_destroy(pfe_parity_t *parity)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == parity))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != parity->lock)
	{
		/* Mask parity interrupts */
		if (EOK != oal_mutex_lock(parity->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		pfe_parity_cfg_irq_mask(parity->parity_base_va);
		if (EOK != oal_mutex_unlock(parity->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
		(void)oal_mutex_destroy(parity->lock);
		(void)oal_mm_free(parity->lock);
		parity->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(parity);
}

/**
 * @brief		PARITY ISR
 * @param[in]	parity The PARITY instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_parity_isr(const pfe_parity_t *parity)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == parity))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_parity_cfg_isr(parity->parity_base_va);
	if (EOK != oal_mutex_unlock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Mask PARITY interrupts
 * @param[in]	parity The PARITY instance
 */
void pfe_parity_irq_mask(const pfe_parity_t *parity)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == parity))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_parity_cfg_irq_mask(parity->parity_base_va);
	if (EOK != oal_mutex_unlock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask PARITY interrupts
 * @param[in]	parity The PARITY instance
 */
void pfe_parity_irq_unmask(const pfe_parity_t *parity)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == parity))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_parity_cfg_irq_unmask(parity->parity_base_va);
	if (EOK != oal_mutex_unlock(parity->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
