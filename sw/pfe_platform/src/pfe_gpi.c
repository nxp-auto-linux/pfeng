/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_gpi.h"

struct __pfe_gpi_tag
{
	void *cbus_base_va;		/*	CBUS base virtual address */
	void *gpi_base_offset;	/*	GPI base offset within CBUS space */
	void *gpi_base_va;		/*	GPI base address (virtual) */
};

static void pfe_gpi_set_config(pfe_gpi_t *gpi, pfe_gpi_cfg_t *cfg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == gpi) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_gpi_cfg_init(gpi->cbus_base_va, gpi->gpi_base_va, cfg))
	{
		NXP_LOG_ERROR("Wrong GPI configuration detected\n");
	}
}

/**
 * @brief		Create new GPI instance
 * @details		Creates and initializes GPI instance. The new instance is disabled and needs
 * 				to be enabled by pfe_gpi_enable().
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	gpi_base BMU base address offset within CBUS address space
 * @param[in]	cfg The BMU block configuration
 * @return		The BMU instance or NULL if failed
 */
pfe_gpi_t *pfe_gpi_create(void *cbus_base_va, void *gpi_base, pfe_gpi_cfg_t *cfg)
{
	pfe_gpi_t *gpi;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	gpi = oal_mm_malloc(sizeof(pfe_gpi_t));
		
	if (NULL == gpi)
	{
		return NULL;
	}
	else
	{
		memset(gpi, 0, sizeof(pfe_gpi_t));
		gpi->cbus_base_va = cbus_base_va;
		gpi->gpi_base_offset = gpi_base;
		gpi->gpi_base_va = (void *)((addr_t)gpi->cbus_base_va + (addr_t)gpi->gpi_base_offset);
	}
	
	pfe_gpi_reset(gpi);
	
	pfe_gpi_disable(gpi);

	pfe_gpi_set_config(gpi, cfg);
	
	return gpi;
}

/**
 * @brief		Reset the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_reset(pfe_gpi_t *gpi)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_gpi_cfg_reset(gpi->gpi_base_va))
	{
		NXP_LOG_ERROR("GPI reset timed-out\n");
	}
}

/**
 * @brief		Enable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_enable(pfe_gpi_t *gpi)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_gpi_cfg_enable(gpi->gpi_base_va);
}

/**
 * @brief		Disable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_disable(pfe_gpi_t *gpi)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_gpi_cfg_disable(gpi->gpi_base_va);
}

/**
 * @brief		Destroy GPI instance
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_destroy(pfe_gpi_t *gpi)
{
	if (NULL != gpi)
	{
		pfe_gpi_disable(gpi);
		
		pfe_gpi_reset(gpi);
		
		oal_mm_free(gpi);
	}
}

/**
 * @brief		Return GPI runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	gpi 		The GPI instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_gpi_get_text_statistics(pfe_gpi_t *gpi, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	
	len += pfe_gpi_cfg_get_text_stat(gpi->gpi_base_va, buf, buf_len, verb_level);
	
	
	return len;
}
