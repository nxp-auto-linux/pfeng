/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2020-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_wdt.h"
#include "pfe_wdt_csr.h"

struct pfe_wdt_tag
{
	addr_t cbus_base_va;
	addr_t wdt_base_offset;
	addr_t wdt_base_va;
	oal_mutex_t lock;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_wdt_t *pfe_wdt_create(addr_t cbus_base_va, addr_t wdt_base)
{
	pfe_wdt_t *wdt;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		wdt = NULL;
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
	    wdt = oal_mm_malloc(sizeof(pfe_wdt_t));

		if (NULL == wdt)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
		else
	    {
	    	(void)memset(wdt, 0, sizeof(pfe_wdt_t));
	    	wdt->cbus_base_va = cbus_base_va;
	    	wdt->wdt_base_offset = wdt_base;
	    	wdt->wdt_base_va = (wdt->cbus_base_va + wdt->wdt_base_offset);

	    	/*	Resource protection */
	    	if (EOK != oal_mutex_init(&wdt->lock))
	    	{
	    		NXP_LOG_ERROR("Mutex initialization failed\n");
	    		oal_mm_free(wdt);
	    		wdt = NULL;
	    	}
            else
            {
#ifdef PFE_CFG_PARANOID_IRQ
	            if (EOK != oal_mutex_lock(&wdt->lock))
	            {
	            	NXP_LOG_ERROR("Mutex lock failed\n");
	            }
#endif /* PFE_CFG_PARANOID_IRQ */

	            pfe_wdt_cfg_init(wdt->wdt_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	            if (EOK != oal_mutex_unlock(&wdt->lock))
	            {
	            	NXP_LOG_ERROR("Mutex unlock failed\n");
	            }
#endif /* PFE_CFG_PARANOID_IRQ */
            }
	    }
    }
	return wdt;
}

/**
 * @brief		Destroy WDT instance
 * @param[in]	wdt The WDT instance
 */
void pfe_wdt_destroy(pfe_wdt_t *wdt)
{
	if (NULL != wdt)
	{
		if (EOK != oal_mutex_lock(&wdt->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		pfe_wdt_cfg_fini(wdt->wdt_base_va);

		if (EOK != oal_mutex_unlock(&wdt->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (EOK != oal_mutex_destroy(&wdt->lock))
		{
			NXP_LOG_ERROR("Mutex destroy failed\n");
		}

		/* Free memory used for structure */
		oal_mm_free(wdt);
	}
}

/**
 * @brief		WDT ISR
 * @param[in]	wdt The WDT instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_wdt_isr(pfe_wdt_t *wdt)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == wdt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
	    if (EOK != oal_mutex_lock(&wdt->lock))
	    {
	    	NXP_LOG_ERROR("Mutex lock failed\n");
	    }

	    /*	Run the low-level ISR to identify and process the interrupt */
	    if (EOK == pfe_wdt_cfg_isr(wdt->wdt_base_va, wdt->cbus_base_va))
	    {
	    	/*	IRQ handled */
	    	ret = EOK;
	    }
		else
		{
			ret = EINVAL;
		}

	    if (EOK != oal_mutex_unlock(&wdt->lock))
	    {
	    	NXP_LOG_ERROR("Mutex unlock failed\n");
	    }
    }
	return ret;
}

/**
 * @brief		Mask WDT interrupts
 * @param[in]	The WDT instance
 */
void pfe_wdt_irq_mask(pfe_wdt_t *wdt)
{
	if (EOK != oal_mutex_lock(&wdt->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_wdt_cfg_irq_mask(wdt->wdt_base_va);

	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask WDT interrupts
 * @param[in]	The WDT instance
 */
void pfe_wdt_irq_unmask(pfe_wdt_t * wdt)
{
	if (EOK != oal_mutex_lock(&wdt->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_wdt_cfg_irq_unmask(wdt->wdt_base_va);

	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

/**
 * @brief		Return WDT runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	wdt 		The WDT instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_wdt_get_text_statistics(const pfe_wdt_t *wdt, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == wdt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
		len += pfe_wdt_cfg_get_text_stat(wdt->wdt_base_va, buf, buf_len, verb_level);
    }
	return len;
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

