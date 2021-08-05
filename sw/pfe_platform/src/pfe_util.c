/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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

struct pfe_util_tag
{
	bool_t is_fw_loaded;	/*	Flag indicating that firmware has been loaded */
	addr_t cbus_base_va;		/*	CBUS base virtual address */
	uint32_t pe_num;		/*	Number of PEs */
	pfe_pe_t **pe;			/*	List of particular PEs */
};

/**
 * @brief		Set the configuration of the util PE block.
 * @param[in]	util The UTIL instance
 * @param[in]	cfg Pointer to the configuration structure
 */
static void pfe_util_set_config(const pfe_util_t *util, const pfe_util_cfg_t *cfg)
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

/*
 * @brief		Create PEs
 * 
 */
static errno_t pfe_util_create_pe(uint32_t pe_num, addr_t cbus_base_va, pfe_util_t *util)
{
	pfe_pe_t *pe;
	uint32_t ii;
	/*	Create PEs */
	for (ii=0U; ii<pe_num; ii++)
	{
		pe = pfe_pe_create(cbus_base_va, PE_TYPE_UTIL, (uint8_t)ii);

		if (NULL == pe)
		{
			return ECANCELED;
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
	return EOK;
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
pfe_util_t *pfe_util_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_util_cfg_t *cfg)
{
	pfe_util_t *util;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == cbus_base_va) || (NULL == cfg)))
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
		(void)memset(util, 0, sizeof(pfe_util_t));
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
		ret = pfe_util_create_pe(pe_num, cbus_base_va, util);
		if(EOK != ret)
		{
			goto free_and_fail;
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
void pfe_util_reset(const pfe_util_t *util)
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
void pfe_util_enable(const pfe_util_t *util)
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
void pfe_util_disable(const pfe_util_t *util)
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
 * @brief UTIL ISR
 * @details Checks PE whether it reports a firmware error
 * @param[in] util The UTIL instance
 */
errno_t pfe_util_isr(const pfe_util_t *util)
{
	uint32_t i;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == util)
	{
		NXP_LOG_ERROR("NULL argument\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Read the error record from each PE */
	for (i = 0U; i < util->pe_num; i++)
	{
		/*	Allow safe use of _nolock() functions. We don't call the _mem_lock()
			here as we don't need to have coherent accesses. */
		if (EOK != pfe_pe_lock(util->pe[i]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}
		
		(void)pfe_pe_get_fw_errors_nolock(util->pe[i]);
		
		if (EOK != pfe_pe_unlock(util->pe[i]))
		{
			NXP_LOG_DEBUG("pfe_pe_unlock() failed\n");
		}
	}

	/* Acknowledge interrupt */
	(void) pfe_util_cfg_isr(util->cbus_base_va);

	return EOK;
}
/**
 * @brief		Mask UTIL interrupts
 * @param[in]	util The UTIL instance
 */
void pfe_util_irq_mask(const pfe_util_t *util)
{
#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
	/*	Intentionally empty */
	(void)util;
#else
	#error Not supported yet
#endif /* PFE_CFG_IP_VERSION */
}

/**
 * @brief		Unmask UTIL interrupts
 * @param[in]	util The UTIL instance
 */
void pfe_util_irq_unmask(const pfe_util_t *util)
{
#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
	/*	Intentionally empty */
	(void)util;
#else
	#error Not supported yet
#endif /* PFE_CFG_IP_VERSION */
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
uint32_t pfe_util_get_text_statistics(const pfe_util_t *util, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U, ii;
	pfe_ct_version_t fw_ver;
	pfe_ct_pe_mmap_t mmap;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* FW version */
	if (EOK == pfe_util_get_fw_version(util, &fw_ver))
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "FIRMWARE VERSION\t%u.%u.%u (api:%.32s)\n",
			fw_ver.major, fw_ver.minor, fw_ver.patch, fw_ver.cthdr);
	}
	else
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "FIRMWARE VERSION <unknown>\n");
	}

	len += pfe_util_cfg_get_text_stat(util->cbus_base_va, buf + len, buf_len - len, verb_level);

	/*	Get PE info per PE */
	for (ii=0U; ii<util->pe_num; ii++)
	{
		ipsec_state_t state = { 0 };
		len += pfe_pe_get_text_statistics(util->pe[ii], buf + len, buf_len - len, verb_level);

		if (EOK == pfe_pe_get_mmap(util->pe[ii], &mmap))
		{
			/* IPsec statistics */
			pfe_pe_memcpy_from_dmem_to_host_32(util->pe[ii], &state, oal_ntohl(mmap.util_pe.ipsec_state), sizeof(state));
			len += oal_util_snprintf(buf + len, buf_len - len, "\nIPsec\n", state.hse_mu);
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE MU            0x%x\n", oal_ntohl(state.hse_mu));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE MU Channel    0x%x\n", oal_ntohl(state.hse_mu_chn));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_OK                        0x%x\n", oal_ntohl(state.response_ok));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_VERIFY_FAILED             0x%x\n", oal_ntohl(state.verify_failed));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_INVALID_DATA        0x%x\n", oal_ntohl(state.ipsec_invalid_data));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_REPLAY_DETECTED     0x%x\n", oal_ntohl(state.ipsec_replay_detected));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_REPLAY_LATE         0x%x\n", oal_ntohl(state.ipsec_replay_late));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_SEQNUM_OVERFLOW     0x%x\n", oal_ntohl(state.ipsec_seqnum_overflow));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_CE_DROP             0x%x\n", oal_ntohl(state.ipsec_ce_drop));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_TTL_EXCEEDED        0x%x\n", oal_ntohl(state.ipsec_ttl_exceeded));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_VALID_DUMMY_PAYLOAD 0x%x\n", oal_ntohl(state.ipsec_valid_dummy_payload));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_HEADER_LEN_OVERFLOW 0x%x\n", oal_ntohl(state.ipsec_header_overflow));
			len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_PADDING_CHECK_FAIL  0x%x\n", oal_ntohl(state.ipsec_padding_check_fail));
			len += oal_util_snprintf(buf + len, buf_len - len, "Code of handled error    0x%x\n", oal_ntohl(state.handled_error_code));
			len += oal_util_snprintf(buf + len, buf_len - len, "SAId of handled error    0x%x\n", oal_ntohl(state.handled_error_said));
			len += oal_util_snprintf(buf + len, buf_len - len, "Code of unhandled error  0x%x\n", oal_ntohl(state.unhandled_error_code));
			len += oal_util_snprintf(buf + len, buf_len - len, "SAId of unhandled error  0x%x\n", oal_ntohl(state.unhandled_error_said));
		}
	}

	return len;
}

/**
 * @brief		Returns firmware versions
 * @param[in]	util The UTIL instance
 * @return		ver Parsed firmware metadata
 */
errno_t pfe_util_get_fw_version(const pfe_util_t *util, pfe_ct_version_t *ver)
{
	pfe_ct_pe_mmap_t pfe_pe_mmap;

	/*	Get mmap base from PE[0] since all PEs have the same memory map */
	if ((NULL == util->pe[0]) || (EOK != pfe_pe_get_mmap(util->pe[0], &pfe_pe_mmap)))
	{
		return EINVAL;
	}

	(void)memcpy(ver, &pfe_pe_mmap.util_pe.common.version, sizeof(pfe_ct_version_t));

	return EOK;
}
