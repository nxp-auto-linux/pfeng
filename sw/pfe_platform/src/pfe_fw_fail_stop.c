/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_fw_fail_stop.h"
#include "pfe_fw_fail_stop_csr.h"


struct pfe_fw_fail_stop_tag
{
	addr_t cbus_base_va;
	addr_t fw_fail_stop_base_offset;
	addr_t fw_fail_stop_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new SAFETY_FW_FAIL_STOP instance
 * @details		Create and initializes SAFETY_FW_FAIL_STOP instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va SAFETY_FW_FAIL_STOP register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in SAFETY thread
 */
pfe_fw_fail_stop_t *pfe_fw_fail_stop_create(addr_t cbus_base_va, addr_t fw_fail_stop_base)
{
	pfe_fw_fail_stop_t *fw_fail_stop;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	fw_fail_stop = oal_mm_malloc(sizeof(pfe_fw_fail_stop_t));

	if (NULL == fw_fail_stop)
	{
		return NULL;
	}
	else
	{
		(void)memset(fw_fail_stop, 0, sizeof(pfe_fw_fail_stop_t));
		fw_fail_stop->cbus_base_va = cbus_base_va;
		fw_fail_stop->fw_fail_stop_base_offset = fw_fail_stop_base;
		fw_fail_stop->fw_fail_stop_base_va = (fw_fail_stop->cbus_base_va + fw_fail_stop->fw_fail_stop_base_offset);

		/*	Create mutex */
		fw_fail_stop->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == fw_fail_stop->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_fw_fail_stop_destroy(fw_fail_stop);
			return NULL;
		}
		else
		{
			(void)oal_mutex_init(fw_fail_stop->lock);
		}

		/* Unmask all interrupts */
		pfe_fw_fail_stop_cfg_irq_unmask_all(fw_fail_stop->fw_fail_stop_base_va);
	}

	return fw_fail_stop;
}

/**
 * @brief		Destroy SAFETY_FW_FAIL_STOP instance
 * @param[in]	fw_fail_stop The SAFETY_FW_FAIL_STOP instance
 */
void pfe_fw_fail_stop_destroy(pfe_fw_fail_stop_t *fw_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fw_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != fw_fail_stop->lock)
	{
		/* Mask fw_fail_stop interrupts */
		(void)oal_mutex_lock(fw_fail_stop->lock);
		pfe_fw_fail_stop_cfg_irq_mask(fw_fail_stop->fw_fail_stop_base_va);
		(void)oal_mutex_unlock(fw_fail_stop->lock);
		(void)oal_mutex_destroy(fw_fail_stop->lock);
		(void)oal_mm_free(fw_fail_stop->lock);
		fw_fail_stop->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(fw_fail_stop);
}

/**
 * @brief		SAFETY_FW_FAIL_STOP ISR
 * @param[in]	fw_fail_stop The SAFETY_FW_FAIL_STOP instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_fw_fail_stop_isr(const pfe_fw_fail_stop_t *fw_fail_stop)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fw_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(fw_fail_stop->lock);
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_fw_fail_stop_cfg_isr(fw_fail_stop->fw_fail_stop_base_va);
	(void)oal_mutex_unlock(fw_fail_stop->lock);

	return ret;
}

/**
 * @brief		Mask SAFETY_FW_FAIL_STOP interrupts
 * @param[in]	fw_fail_stop The SAFETY_FW_FAIL_STOP instance
 */
void pfe_fw_fail_stop_irq_mask(const pfe_fw_fail_stop_t *fw_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fw_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(fw_fail_stop->lock);
	pfe_fw_fail_stop_cfg_irq_mask(fw_fail_stop->fw_fail_stop_base_va);
	(void)oal_mutex_unlock(fw_fail_stop->lock);
}

/**
 * @brief		Unmask SAFETY_FW_FAIL_STOP interrupts
 * @param[in]	fw_fail_stop The SAFETY_FW_FAIL_STOP instance
 */
void pfe_fw_fail_stop_irq_unmask(const pfe_fw_fail_stop_t *fw_fail_stop)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fw_fail_stop))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(fw_fail_stop->lock);
	pfe_fw_fail_stop_cfg_irq_unmask(fw_fail_stop->fw_fail_stop_base_va);
	(void)oal_mutex_unlock(fw_fail_stop->lock);
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
