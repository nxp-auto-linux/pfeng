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
#include "pfe_bus_err.h"
#include "pfe_bus_err_csr.h"


struct pfe_bus_err_tag
{
	addr_t cbus_base_va;
	addr_t bus_err_base_offset;
	addr_t bus_err_base_va;
	oal_mutex_t *lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new BUS_ERR instance
 * @details		Create and initializes BUS_ERR instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	bus_err_base BUS_ERR base address offset within CBUS address space
 * @return		EOK if interrupt has been handled, error code otherwise
 * @return		The BUS_ERR instance or NULL if failed
 */
pfe_bus_err_t *pfe_bus_err_create(addr_t cbus_base_va, addr_t bus_err_base)
{
	pfe_bus_err_t *bus_err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bus_err = oal_mm_malloc(sizeof(pfe_bus_err_t));

	if (NULL == bus_err)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		(void)memset(bus_err, 0, sizeof(pfe_bus_err_t));
		bus_err->cbus_base_va = cbus_base_va;
		bus_err->bus_err_base_offset = bus_err_base;
		bus_err->bus_err_base_va = (bus_err->cbus_base_va + bus_err->bus_err_base_offset);

		/*	Create mutex */
		bus_err->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == bus_err->lock)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
			pfe_bus_err_destroy(bus_err);
			return NULL;
		}
		else
		{
			if (EOK != oal_mutex_init(bus_err->lock))
			{
				NXP_LOG_ERROR("Mutex initialization failed\n");
			}
		}

		/* Unmask all interrupts */
		pfe_bus_err_cfg_irq_unmask_all(bus_err->bus_err_base_va);
	}

	return bus_err;
}

/**
 * @brief		Destroy BUS_ERR instance
 * @param[in]	bus_err The BUS_ERR instance
 */
void pfe_bus_err_destroy(pfe_bus_err_t *bus_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bus_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != bus_err->lock)
	{
		/* Mask bus_err interrupts */
		if (EOK != oal_mutex_lock(bus_err->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		pfe_bus_err_cfg_irq_mask(bus_err->bus_err_base_va);
		if (EOK != oal_mutex_unlock(bus_err->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
		(void)oal_mutex_destroy(bus_err->lock);
		(void)oal_mm_free(bus_err->lock);
		bus_err->lock = NULL;
	}

	/* Free memory used for structure */
	(void)oal_mm_free(bus_err);
}

/**
 * @brief		BUS_ERR ISR
 * @param[in]	bus_err The BUS_ERR instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_bus_err_isr(const pfe_bus_err_t *bus_err)
{
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bus_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_bus_err_cfg_isr(bus_err->bus_err_base_va);
	if (EOK != oal_mutex_unlock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Mask BUS_ERR interrupts
 * @param[in]	bus_err The BUS_ERR instance
 */
void pfe_bus_err_irq_mask(const pfe_bus_err_t *bus_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bus_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_bus_err_cfg_irq_mask(bus_err->bus_err_base_va);
	if (EOK != oal_mutex_unlock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask BUS_ERR interrupts
 * @param[in]	bus_err The BUS_ERR instance
 */
void pfe_bus_err_irq_unmask(const pfe_bus_err_t *bus_err)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bus_err))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
	pfe_bus_err_cfg_irq_unmask(bus_err->bus_err_base_va);
	if (EOK != oal_mutex_unlock(bus_err->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
