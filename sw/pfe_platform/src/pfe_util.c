/* =========================================================================
 *  Copyright 2018-2020 NXP
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

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_pe.h"
#include "pfe_util.h"

/* Configuration check */
#if ((PFE_CFG_PE_LMEM_BASE + PFE_CFG_PE_LMEM_SIZE) > CBUS_LMEM_SIZE)
	#error PE memory area exceeds LMEM capacity
#endif

struct __pfe_util_tag
{
	bool_t is_fw_loaded;	/*	Flag indicating that firmware has been loaded */
	void *cbus_base_va;		/*	CBUS base virtual address */
	uint32_t pe_num;		/*	Number of PEs */
	pfe_pe_t **pe;			/*	List of particular PEs */
};

/**
 * @brief		Set the configuration of the util PE block.
 * @param[in]	util The UTIL instance
 * @param[in]	cfg Pointer to the configuration structure
 */
static void pfe_util_set_config(pfe_util_t *util, pfe_util_cfg_t *cfg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	hal_write32(cfg->pe_sys_clk_ratio, util->cbus_base_va + UTIL_PE_SYS_CLK_RATIO);
}

/**
 * @brief		Create new UTIL instance
 * @details		Creates and initializes UTIL instance. After successful
 * 				call the UTIL is configured and disabled.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	pe_num Number of PEs to be included
 * @param[in]	cfg The UTIL block configuration
 * @return		The UTIL instance or NULL if failed
 */
pfe_util_t *pfe_util_create(void *cbus_base_va, uint32_t pe_num, pfe_util_cfg_t *cfg)
{
	pfe_util_t *util;
	pfe_pe_t *pe;
	uint32_t ii;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	util = oal_mm_malloc(sizeof(pfe_util_t));
	
	if (NULL == util)
	{
		return NULL;
	}
	else
	{
		memset(util, 0, sizeof(pfe_util_t));
		util->cbus_base_va = cbus_base_va;
	}
	
	if (pe_num > 0U)
	{
		util->pe = oal_mm_malloc(pe_num * sizeof(pfe_pe_t *));
		
		if (NULL == util->pe)
		{
			oal_mm_free(util);
			return NULL;
		}
		
		/*	Create PEs */
		for (ii=0U; ii<pe_num; ii++)
		{
			pe = pfe_pe_create(cbus_base_va, PE_TYPE_UTIL, ii);
			
			if (NULL == pe)
			{
				goto free_and_fail;
			}
			else
			{
				pfe_pe_set_iaccess(pe, UTIL_MEM_ACCESS_WDATA, UTIL_MEM_ACCESS_RDATA, UTIL_MEM_ACCESS_ADDR);
				pfe_pe_set_dmem(pe, PFE_CFG_UTIL_ELF_DMEM_BASE, PFE_CFG_UTIL_DMEM_SIZE);
				pfe_pe_set_imem(pe, PFE_CFG_UTIL_ELF_IMEM_BASE, PFE_CFG_UTIL_IMEM_SIZE);
				
				util->pe[ii] = pe;
				util->pe_num++;
			}
		}
		
		/*	Issue block reset */
		pfe_util_reset(util);
		
		/*	Disable the UTIL block */
		pfe_util_disable(util);
		
		/*	Set new configuration */
		pfe_util_set_config(util, cfg);
	}
	
	return util;
	
free_and_fail:
	pfe_util_destroy(util);
	util = NULL;
	
	return NULL;
}

/**
 * @brief		Reset the UTIL block
 * @param[in]	util The UTIL instance
 */
void pfe_util_reset(pfe_util_t *util)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	hal_write32(PFE_CORE_SW_RESET, util->cbus_base_va + UTIL_TX_CTRL);
}

/**
 * @brief		Enable the UTIL block
 * @details		Enable all UTIL PEs
 * @param[in]	util The UTIL instance
 */
void pfe_util_enable(pfe_util_t *util)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely(FALSE == util->is_fw_loaded))
	{
		NXP_LOG_WARNING("Attempt to enable UTIL PE(s) without previous firmware upload\n");
	}
	
	hal_write32(PFE_CORE_ENABLE, util->cbus_base_va + UTIL_TX_CTRL);
}

/**
 * @brief		Disable the UTIL block
 * @param[in]	util The UTIL instance
 */
void pfe_util_disable(pfe_util_t *util)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	hal_write32(PFE_CORE_DISABLE, util->cbus_base_va + UTIL_TX_CTRL);
}

/**
 * @brief		Load firmware elf into PEs memories
 * @param[in]	util The UTIL instance
 * @param[in]	elf The elf file object to be uploaded
 * @return		EOK when success or error code otherwise
 */
errno_t pfe_util_load_firmware(pfe_util_t *util, const void *elf)
{
	uint32_t ii;
	errno_t ret;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (ii=0U; ii<util->pe_num; ii++)
	{
		ret = pfe_pe_load_firmware(util->pe[ii], elf);
		
		if (EOK != ret)
		{
			NXP_LOG_ERROR("UTIL firmware loading failed: %d\n", ret);
			return ret;
		}
	}
	
	util->is_fw_loaded = TRUE;
	
	return EOK;
}

/**
 * @brief		Destroy util block instance
 * @param[in]	util The util block instance
 */
void pfe_util_destroy(pfe_util_t *util)
{
	uint32_t ii;
	
	if (NULL != util)
	{
		for (ii=0U; ii<util->pe_num; ii++)
		{
			pfe_pe_destroy(util->pe[ii]);
			util->pe[ii] = NULL;
		}
		
		pfe_util_disable(util);
		
		util->pe_num = 0U;
		
		oal_mm_free(util);
	}
}

/**
 * @brief		Return UTIL runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	util 		The UTIL instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_util_get_text_statistics(pfe_util_t *util, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U, ii;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	
	len += pfe_util_cfg_get_text_stat(util->cbus_base_va, buf + len, buf_len - len, verb_level);

	/*	Get PE info per PE */
	for (ii=0U; ii<util->pe_num; ii++)
	{
		len += pfe_pe_get_text_statistics(util->pe[ii], buf + len, buf_len - len, verb_level);
	}
	
	return len;
}
