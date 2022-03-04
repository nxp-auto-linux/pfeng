/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
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
	oal_mutex_t mutex;
	uint32_t current_feature;			/* Index of the feature to return by pfe_util_get_feature_next() */
	pfe_fw_feature_t **fw_features;		/* List of all features*/
	uint32_t fw_features_count;			/* Number of items in fw_features */
};

static errno_t pfe_util_read_dmem(void *util_p, int32_t pe_idx, void *dst_ptr, addr_t src_addr, uint32_t len);
static errno_t pfe_util_write_dmem(void *util_p, int32_t pe_idx, addr_t dst_addr, const void *src_ptr, uint32_t len);

static errno_t pfe_util_load_fw_features(pfe_util_t *util)
{
	pfe_ct_pe_mmap_t mmap;
	errno_t ret = EOK;
	pfe_ct_feature_desc_t *entry;
	uint32_t i = 0U, j = 0U;
	bool_t val_break = FALSE;

	ret = pfe_pe_get_mmap(util->pe[0U], &mmap);
	if(EOK == ret)
	{
		util->fw_features_count = oal_ntohl(mmap.common.version.features_count);
		util->fw_features = NULL;
		if(util->fw_features_count > 0U)
		{
			util->fw_features = oal_mm_malloc(util->fw_features_count * sizeof(pfe_fw_feature_t *));
			if(NULL == util->fw_features)
			{
				util->fw_features_count = 0U;
				NXP_LOG_ERROR("Failed to allocate features storage\n");
				ret = ENOMEM;
			}
			else
			{
				/* Initialize current_feature */
				util->current_feature = 0U;
				while(i < util->fw_features_count)
				{
					util->fw_features[i] = pfe_fw_feature_create();
					if(NULL == util->fw_features[i])
					{
						NXP_LOG_ERROR("Failed to create feature %u\n", (uint_t)i);
						/* Destroy previously created and return failure */
						for(j = 0U; j < i; j++)
						{
							pfe_fw_feature_destroy(util->fw_features[j]);
							util->fw_features[j] = NULL;
						}
						oal_mm_free(util->fw_features);
						util->fw_features = NULL;
						util->fw_features_count = 0U;
						ret = ENOMEM;
						val_break = TRUE;
					}
					else
					{
						/* Get feature low level data */
						ret = pfe_pe_get_fw_feature_entry(util->pe[0U], i, &entry);
						if(EOK != ret)
						{
							 NXP_LOG_ERROR("Failed get ll data for feature %u\n", (uint_t)i);
							/* Destroy previously created and return failure */
							for(j = 0U; j < i; j++)
							{
								pfe_fw_feature_destroy(util->fw_features[j]);
								util->fw_features[j] = NULL;
							}
							oal_mm_free(util->fw_features);
							util->fw_features = NULL;
							util->fw_features_count = 0U;
							ret = EINVAL;
							val_break = TRUE;
						}
						else
						{
							/* Set the low level data in the feature */
							(void)pfe_fw_feature_set_ll_data(util->fw_features[i], entry);
							/* Set the feature string base */
							ret = pfe_fw_feature_set_string_base(util->fw_features[i], pfe_pe_get_fw_feature_str_base(util->pe[0U]));
							if(EOK != ret)
							{
								NXP_LOG_ERROR("Failed to set string base for feature %u\n", (uint_t)i);
								/* Destroy previously created and return failure */
								for(j = 0U; j < i; j++)
								{
									pfe_fw_feature_destroy(util->fw_features[j]);
									util->fw_features[j] = NULL;
								}
								oal_mm_free(util->fw_features);
								util->fw_features = NULL;
								util->fw_features_count = 0U;
								ret = EINVAL;
								val_break = TRUE;
							}
							else
							{
								/* Set functions to read/write DMEM and their data */
								(void)pfe_fw_feature_set_dmem_funcs(util->fw_features[i], pfe_util_read_dmem, pfe_util_write_dmem, (void *)util);
							}
						}
					}
					if (TRUE == val_break)
					{
						break;
					}

					++i;
				}
			}
		} /* Else is OK too */
	}
	return ret;
}



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

	if (EOK != oal_mutex_init(&util->mutex))
	{
		oal_mm_free(util);
		return NULL;
	}
    /* No need to lock mutex. No other function can be called before
       we return util handle from this function. */

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
void pfe_util_reset(pfe_util_t *util)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}
	hal_write32(PFE_CORE_SW_RESET, util->cbus_base_va + UTIL_TX_CTRL);
	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
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
	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}
	hal_write32(PFE_CORE_ENABLE, util->cbus_base_va + UTIL_TX_CTRL);
	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
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
	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}
	hal_write32(PFE_CORE_DISABLE, util->cbus_base_va + UTIL_TX_CTRL);
	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
}

/**
 * @brief		Load firmware elf into PEs memories
 * @param[in]	util The UTIL instance
 * @param[in]	elf The elf file object to be uploaded
 * @return		EOK when success or error code otherwise
 */
errno_t pfe_util_load_firmware(pfe_util_t *util, const void *elf)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_pe_load_firmware(util->pe, util->pe_num, elf);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("UTIL firmware loading failed: %d\n", ret);
	}

	util->is_fw_loaded = TRUE;
	ret = pfe_util_load_fw_features(util);
	if(EOK != ret)
	{
		NXP_LOG_ERROR("Failed to initialize FW features\n");
	}
	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
	return ret;
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
		pfe_pe_destroy(util->pe, util->pe_num);

		if (NULL != util->pe)

		{
			oal_mm_free(util->pe);
		}

		if(NULL != util->fw_features)
		{
			for(ii = 0U; ii < util->fw_features_count; ii++)
			{
				if(NULL != util->fw_features[ii])
				{
					pfe_fw_feature_destroy(util->fw_features[ii]);
					util->fw_features[ii] = NULL;
				}
			}
			oal_mm_free(util->fw_features);
			util->fw_features = NULL;
			util->fw_features_count = 0U;
		}

		util->pe_num = 0U;
		(void)oal_mutex_destroy(&util->mutex);
		oal_mm_free(util);
	}
}

/**
 * @brief Finds and returns Util FW feature by its name
 * @param[in] util The Util instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @param[in] name Name of the feature to be found
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_util_get_feature(const pfe_util_t *util, pfe_fw_feature_t **feature, const char *name)
{
	uint32_t i;
	const char *fname;
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util)||(NULL == feature)||(NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	for(i = 0U; i < util->fw_features_count; i++)
	{
		ret = pfe_fw_feature_get_name(util->fw_features[i], &fname);
		if(ret == EOK)
		{
			if(0 == strcmp(fname, name))
			{
				*feature = util->fw_features[i];
				return EOK;
			}
		}
	}
	return ENOENT;
}

/**
 * @brief Finds and returns the 1st Util FW feature by order of their discovery - used for listing all features
 * @param[in] util The Util instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_util_get_feature_first(pfe_util_t *util, pfe_fw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util)||(NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(util->fw_features_count > 0U)
	{
		util->current_feature = 0U;
		*feature = util->fw_features[util->current_feature];
		return EOK;
	}

	return ENOENT;
}

/**
 * @brief Finds and returns the next Util FW feature by order of their discovery - used for listing all features
 * @param[in] util The Util instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_util_get_feature_next(pfe_util_t *util, pfe_fw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util)||(NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(util->fw_features_count > 0U)
	{
		/* Avoid going out of the array boundaries */
		if((util->current_feature + 1U) < util->fw_features_count)
		{
			util->current_feature += 1U;
			*feature = util->fw_features[util->current_feature];
			return EOK;
		}
	}

	return ENOENT;
}

/**
 * @brief		Write data from host memory to DMEM
 * @param[in]	util_p The util instance
 * @param[in]	pe_idx PE index or -1 if all PEs shall be written
 * @param[in]	dst_addr Destination address within DMEM (physical)
 * @param[in]	src_ptr Pointer to data in host memory (virtual address)
 * @param[in]	len Number of bytes to be written
 * @return		EOK or error code in case of failure
 */
static errno_t pfe_util_write_dmem(void *util_p, int32_t pe_idx, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	uint32_t ii;
    pfe_util_t *util = (pfe_util_t *)util_p;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == util))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (pe_idx >= (int32_t)util->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	if (pe_idx >= 0)
	{
		/*	Single PE */
		pfe_pe_memcpy_from_host_to_dmem_32(util->pe[pe_idx], dst_addr, src_ptr, len);
	}
	else
	{
		/*	All PEs */
		for (ii=0U; ii<util->pe_num; ii++)
		{
			pfe_pe_memcpy_from_host_to_dmem_32(util->pe[ii], dst_addr, src_ptr, len);
		}
	}

	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Read data from DMEM to host memory
 * @param[in]	util_p The util instance
 * @param[in]	pe_idx PE index
 * @param[in]	dst_ptr Destination address within host memory (virtual)
 * @param[in]	src_addr Source address within DMEM (physical)
 * @param[in]	len Number of bytes to be read
 * @return		EOK or error code in case of failure
 */
static errno_t pfe_util_read_dmem(void *util_p, int32_t pe_idx, void *dst_ptr, addr_t src_addr, uint32_t len)
{
    pfe_util_t *util = (pfe_util_t *)util_p;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == util) || (NULL == dst_ptr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (pe_idx >= (int32_t)util->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_pe_memcpy_from_dmem_to_host_32(util->pe[pe_idx], dst_ptr, src_addr, len);

	if (EOK != oal_mutex_unlock(&util->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
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

		(void)pfe_pe_get_fw_messages_nolock(util->pe[i]);

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
	/*	Intentionally empty */
	(void)util;
}

/**
 * @brief		Unmask UTIL interrupts
 * @param[in]	util The UTIL instance
 */
void pfe_util_irq_unmask(const pfe_util_t *util)
{
	/*	Intentionally empty */
	(void)util;
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
		uint32_t text_stat_len = 0U;

		if (EOK == pfe_pe_get_mmap(util->pe[ii], &mmap))
		{
			text_stat_len = pfe_pe_get_text_statistics(util->pe[ii], buf + len, buf_len - len, verb_level);
			if (0U == text_stat_len)
			{
				len = 0U;
				break;
			}
			else
			{
				len += text_stat_len;
				/* IPsec statistics */
				pfe_pe_memcpy_from_dmem_to_host_32(util->pe[ii], &state, oal_ntohl(mmap.util_pe.ipsec_state), sizeof(state));
				len += oal_util_snprintf(buf + len, buf_len - len, "\nIPsec\n");
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE MU			0x%x\n", oal_ntohl(state.hse_mu));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE MU Channel    0x%x\n", oal_ntohl(state.hse_mu_chn));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_OK						0x%x\n", oal_ntohl(state.response_ok));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_VERIFY_FAILED			 0x%x\n", oal_ntohl(state.verify_failed));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_INVALID_DATA		0x%x\n", oal_ntohl(state.ipsec_invalid_data));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_REPLAY_DETECTED     0x%x\n", oal_ntohl(state.ipsec_replay_detected));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_REPLAY_LATE		 0x%x\n", oal_ntohl(state.ipsec_replay_late));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_SEQNUM_OVERFLOW     0x%x\n", oal_ntohl(state.ipsec_seqnum_overflow));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_CE_DROP			 0x%x\n", oal_ntohl(state.ipsec_ce_drop));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_TTL_EXCEEDED		0x%x\n", oal_ntohl(state.ipsec_ttl_exceeded));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_VALID_DUMMY_PAYLOAD 0x%x\n", oal_ntohl(state.ipsec_valid_dummy_payload));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_HEADER_LEN_OVERFLOW 0x%x\n", oal_ntohl(state.ipsec_header_overflow));
				len += oal_util_snprintf(buf + len, buf_len - len, "HSE_SRV_RSP_IPSEC_PADDING_CHECK_FAIL  0x%x\n", oal_ntohl(state.ipsec_padding_check_fail));
				len += oal_util_snprintf(buf + len, buf_len - len, "Code of handled error    0x%x\n", oal_ntohl(state.handled_error_code));
				len += oal_util_snprintf(buf + len, buf_len - len, "SAId of handled error    0x%x\n", oal_ntohl(state.handled_error_said));
				len += oal_util_snprintf(buf + len, buf_len - len, "Code of unhandled error  0x%x\n", oal_ntohl(state.unhandled_error_code));
				len += oal_util_snprintf(buf + len, buf_len - len, "SAId of unhandled error  0x%x\n", oal_ntohl(state.unhandled_error_said));
			}
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
