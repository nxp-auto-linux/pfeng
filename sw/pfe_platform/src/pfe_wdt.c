/* =========================================================================
 *  Copyright 2020 NXP
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
#include "pfe_wdt.h"
#include "pfe_wdt_csr.h"

struct __pfe_wdt_tag
{
	void *cbus_base_va;
	void *wdt_base_offset;
	void *wdt_base_va;
	oal_mutex_t lock;
};

pfe_wdt_t *pfe_wdt_create(void *cbus_base_va, void *wdt_base)
{
	pfe_wdt_t *wdt;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	wdt = oal_mm_malloc(sizeof(pfe_wdt_t));

	if (NULL == wdt)
	{
		return NULL;
	}
	else
	{
		memset(wdt, 0, sizeof(pfe_wdt_t));
		wdt->cbus_base_va = cbus_base_va;
		wdt->wdt_base_offset = wdt_base;
		wdt->wdt_base_va = (void *)((addr_t)wdt->cbus_base_va + (addr_t)wdt->wdt_base_offset);

		/*	Resource protection */
		if (EOK != oal_mutex_init(&wdt->lock))
		{
			NXP_LOG_DEBUG("Mutex initialization failed\n");
			oal_mm_free(wdt);
			return NULL;
		}
	}
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	pfe_wdt_cfg_init(wdt->wdt_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

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
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		pfe_wdt_cfg_fini(wdt->wdt_base_va);

		if (EOK != oal_mutex_unlock(&wdt->lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		if (EOK != oal_mutex_destroy(&wdt->lock))
		{
			NXP_LOG_DEBUG("Mutex destroy failed\n");
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
	bool_t ret = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == wdt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Run the low-level ISR to identify and process the interrupt */
	if (EOK == pfe_wdt_cfg_isr(wdt->wdt_base_va, wdt->cbus_base_va))
	{
		/*	IRQ handled */
		ret = TRUE;
	}

	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	pfe_wdt_cfg_irq_mask(wdt->wdt_base_va);

	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	pfe_wdt_cfg_irq_unmask(wdt->wdt_base_va);

	if (EOK != oal_mutex_unlock(&wdt->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
}

/**
 * @brief		Return WDT runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	wdt 		The WDT instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_wdt_get_text_statistics(pfe_wdt_t *wdt, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == wdt))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

		len += pfe_wdt_cfg_get_text_stat(wdt->wdt_base_va, buf, buf_len, verb_level);

	return len;
}
