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
#include "pfe_ecc_err.h"
#include "pfe_ecc_err_csr.h"


struct pfe_ecc_err_tag
{
	addr_t cbus_base_va;
	addr_t ecc_err_base_offset;
	addr_t ecc_err_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new ECC_ERR instance
 * @details		Create and initializes ECC_ERR instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va ECC_ERR register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in SAFETY thread
 */
pfe_ecc_err_t *pfe_ecc_err_create(addr_t cbus_base_va, addr_t ecc_err_base)
{
	pfe_ecc_err_t *ecc_err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ecc_err = oal_mm_malloc(sizeof(pfe_ecc_err_t));

	if (NULL == ecc_err)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		(void)memset(ecc_err, 0, sizeof(pfe_ecc_err_t));
		ecc_err->cbus_base_va = cbus_base_va;
		ecc_err->ecc_err_base_offset = ecc_err_base;
		ecc_err->ecc_err_base_va = (ecc_err->cbus_base_va + ecc_err->ecc_err_base_offset);

		/*	Create mutex */
		ecc_err->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == ecc_err->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_ecc_err_destroy(ecc_err);
			return NULL;
		}
		else
		{
			if (EOK != oal_mutex_init(ecc_err->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
			}
		}

		/* Unmask all interrupts */
		pfe_ecc_err_cfg_irq_unmask_all(ecc_err->ecc_err_base_va);
	}

	return ecc_err;
}

/**
 * @brief		Destroy ECC_ERR instance
 * @param[in]	ecc_err The ECC_ERR instance
 */
void pfe_ecc_err_destroy(pfe_ecc_err_t *ecc_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ecc_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != ecc_err->lock)
	{
		/* Mask ecc_err interrupts */
		if (EOK != oal_mutex_lock(ecc_err->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		pfe_ecc_err_cfg_irq_mask(ecc_err->ecc_err_base_va);
		if (EOK != oal_mutex_unlock(ecc_err->lock))
		{
			 NXP_LOG_ERROR("Mutex unlock failed\n");
		}
		(void)oal_mutex_destroy(ecc_err->lock);
		(void)oal_mm_free(ecc_err->lock);
		ecc_err->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(ecc_err);
}

/**
 * @brief		ECC_ERR ISR
 * @param[in]	ecc_err The ECC_ERR instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_ecc_err_isr(const pfe_ecc_err_t *ecc_err)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ecc_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_ecc_err_cfg_isr(ecc_err->ecc_err_base_va);
	if (EOK != oal_mutex_unlock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Mask ECC_ERR interrupts
 * @param[in]	ecc_err The ECC_ERR instance
 */
void pfe_ecc_err_irq_mask(const pfe_ecc_err_t *ecc_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ecc_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_ecc_err_cfg_irq_mask(ecc_err->ecc_err_base_va);
	if (EOK != oal_mutex_unlock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask ECC_ERR interrupts
 * @param[in]	ecc_err The ECC_ERR instance
 */
void pfe_ecc_err_irq_unmask(const pfe_ecc_err_t *ecc_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ecc_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_ecc_err_cfg_irq_unmask(ecc_err->ecc_err_base_va);
	if (EOK != oal_mutex_unlock(ecc_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
