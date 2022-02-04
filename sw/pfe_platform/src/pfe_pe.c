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

#include "pfe_platform.h"
#include "elf_cfg.h"
#include "elf.h"
#include "pfe_cbus.h"
#include "pfe_pe.h"

#define BYTES_TO_4B_ALIGNMENT(x)	(4U - ((x) & 0x3U))
#define INVALID_FEATURES_BASE 		0xFFFFFFFFU
/**
 * @brief	Mutex protecting access to common mem_access_* registers
 */
static oal_mutex_t mem_access_lock;

typedef enum
{
	PFE_PE_DMEM,
	PFE_PE_IMEM
} pfe_pe_mem_t;

typedef struct
{
	uint8_t pe_loaded_cnt;
	bool_t can_load_util;
	void (*pe_memset)(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint32_t size);
	void (*pe_memcpy)(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t dst_addr, const void *src_ptr, uint32_t len);
} fw_load_ops_t;

/*	Processing Engine representation */
struct pfe_pe_tag
{
	pfe_ct_pe_type_t type;				/* PE type */
	addr_t cbus_base_va;				/* CBUS base (virtual) */
	uint8_t id;							/* PE HW ID (0..N) */

	/*	DMEM */
	addr_t dmem_elf_base_va;			/* PE's DMEM base address (virtual, as seen by PE) */
	addr_t dmem_size;					/* PE's DMEM region length */

	/*	IMEM */
	addr_t imem_elf_base_va;			/* PE's IMEM base address (virtual, as seen by PE) */
	addr_t imem_size;					/* PE's IMEM size */

	/*	LMEM */
	addr_t lmem_base_addr_pa;			/* PE's LMEM base address (physical, as seen by PE) */
	addr_t lmem_size;					/* PE's LMEM size */

	/*	DDR */
	addr_t ddr_base_addr_pa;			/* PE's DDR base address (physical, as seen by host) */
	addr_t ddr_base_addr_va;			/* PE's DDR base address (virtual) */
	addr_t ddr_size;					/* PE's DDR size */

	/*	Indirect Access */
	addr_t mem_access_wdata;			/* PE's _MEM_ACCESS_WDATA register address (virtual) */
	addr_t mem_access_addr;				/* PE's _MEM_ACCESS_ADDR register address (virtual) */
	addr_t mem_access_rdata;			/* PE's _MEM_ACCESS_RDATA register address (virtual) */

	/* Operations to load FW */
	const fw_load_ops_t *fw_load_ops;

	/* FW Errors*/
	uint32_t error_record_addr;			/* Error record storage address in DMEM */
	uint32_t last_error_write_index;	/* Last seen value of write index in the record */
	void *fw_err_section;				/* Error descriptions elf section storage */
	uint32_t fw_err_section_size;		/* Size of the above section */
	/* FW features */
	void *fw_feature_section;			/* Features descriptions elf section storage */
	uint32_t fw_feature_section_size;	/* Size of the above section */
	uint32_t fw_features_base;          /* Extracted base address of the features table */
	uint32_t fw_features_size;          /* Number of entries in the features table */

	/*	MMap */
	pfe_ct_pe_mmap_t *mmap_data;		/* Buffer containing the memory map data */

	/* Mutex */
	oal_mutex_t lock_mutex;				/* Locking PE API mutex */
	bool_t miflock;						/* When TRUE then PFE memory interface is locked */
};

static bool_t pfe_pe_is_active_nolock(pfe_pe_t *pe);
#if defined(FW_WRITE_CHECK_EN)
static void pfe_pe_memcpy_from_imem_to_host_32_nolock(
		pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len);
#endif /* FW_WRITE_CHECK_EN */
static void pfe_pe_memcpy_from_host_to_dmem_32_nolock(
		pfe_pe_t *pe, addr_t dst_addr, const void *src_ptr, uint32_t len);
/* FW loading functions */
static void pfe_pe_fw_memcpy_bulk(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t dst_addr, const void *src_ptr, uint32_t len);
static void pfe_pe_fw_memset_bulk(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint32_t size);
static void pfe_pe_fw_memcpy_single(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t dst_addr, const void *src_ptr, uint32_t len);
static void pfe_pe_fw_memset_single(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint32_t size);
static void pfe_pe_free_mem(pfe_pe_t **pe, uint32_t pe_num);
static errno_t pfe_pe_upload_sections(pfe_pe_t **pe, uint32_t pe_num, const ELF_File_t *elf_file);
static void print_fw_issue(const pfe_ct_pe_mmap_t *fw_mmap);
static uint8_t pfe_pe_fw_load_cycles(const pfe_pe_t *pe, uint8_t pe_num);
static errno_t pfe_pe_load_elf_section(pfe_pe_t *pe, const void *sdata, addr_t load_addr, addr_t size, uint32_t type);
static addr_t pfe_pe_get_elf_sect_load_addr(const ELF_File_t *elf_file, const Elf32_Shdr *shdr);

/*	This one is not static because a firmware test code is using it but it is
	neither declared in public header because it should not be public. */
void pfe_pe_memcpy_from_dmem_to_host_32_nolock(
		pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len);

static const fw_load_ops_t fw_load_ops[] =
{
	/* These OPs can load 8 CLASS cores only */
	{
		.pe_loaded_cnt = 8U,
		.can_load_util = (bool_t)FALSE,
		.pe_memset = pfe_pe_fw_memset_bulk,
		.pe_memcpy = pfe_pe_fw_memcpy_bulk,
	},
	/* These OPs can load  1 CLASS/UTIL core only */
	{
		.pe_loaded_cnt = 1U,
		.can_load_util = (bool_t)TRUE,
		.pe_memset = pfe_pe_fw_memset_single,
		.pe_memcpy = pfe_pe_fw_memcpy_single,
	},
};

/**
 * @brief		Try to upload all sections of the .elf
 * @param[in]	pe The PE instances
 * @param[in]	pe_num Number of PE instances
 * @param[in]	elf_file The elf file object to be uploaded
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_pe_upload_sections(pfe_pe_t **pe, uint32_t pe_num, const ELF_File_t *elf_file)
{
	uint32_t ii, pe_idx;
	addr_t load_addr;
	const void *buf;
	errno_t ret = EOK;

	for (ii = 0U; ii < elf_file->Header.r32.e_shnum; ii++)
	{
		if (0U == (elf_file->arSectHead32[ii].sh_flags & (uint32_t)(((uint32_t)SHF_WRITE) | ((uint32_t)SHF_ALLOC) | ((uint32_t)SHF_EXECINSTR))))
		{
			/*	Skip the section */
			continue;
		}

		buf = (void*)((addr_t)elf_file->pvData + elf_file->arSectHead32[ii].sh_offset);
		/* Translate elf virtual address to load address */
		load_addr = pfe_pe_get_elf_sect_load_addr(elf_file, &elf_file->arSectHead32[ii]);
		if(0U == load_addr)
		{	/* Failed */
			ret = EINVAL;
			pfe_pe_free_mem(pe, pe_num);
			return ret;
		}

		for(pe_idx = 0; pe_idx < pfe_pe_fw_load_cycles(pe[0], (uint8_t)pe_num); ++pe_idx)
		{
		/*	Upload the section */
			ret = pfe_pe_load_elf_section(pe[pe_idx], buf, load_addr, elf_file->arSectHead32[ii].sh_size, elf_file->arSectHead32[ii].sh_type);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Couldn't upload firmware section %s, %u bytes @ 0x%08x. Reason: %d\n",
								elf_file->acSectNames+elf_file->arSectHead32[ii].sh_name,
								(uint_t)elf_file->arSectHead32[ii].sh_size,
								(uint_t)elf_file->arSectHead32[ii].sh_addr, ret);
				pfe_pe_free_mem(pe, pe_num);
				return ret;
			}
		}
	}

	return ret;
}

/**
 * @brief		Free memory when see failed condition.
 * @param[in]	pe_num number of the PE instance
  * @param[in]	pe	   the PE instance
 */
static void pfe_pe_free_mem(pfe_pe_t **pe, uint32_t pe_num)
{
	uint32_t ii;

	if (NULL != pe[0]->mmap_data)
	{
		oal_mm_free(pe[0]->mmap_data);

	}

	if (NULL != pe[0]->fw_err_section)
	{
		oal_mm_free(pe[0]->fw_err_section);
	}

	if (NULL != pe[0]->fw_feature_section)
	{
		oal_mm_free(pe[0]->fw_feature_section);
	}

	for (ii = 0; ii < pe_num; ++ii)
	{
		if (EOK != pfe_pe_unlock(pe[ii]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}
		pe[ii]->mmap_data = NULL;

		pe[ii]->fw_err_section = NULL;
		pe[ii]->fw_err_section_size = 0U;

		pe[ii]->fw_feature_section = NULL;
		pe[ii]->fw_feature_section_size = 0U;

	}
}

/**
 * @brief		Query if PE is active
 * @details		PE is active if it is running (executing firmware code) and is not gracefully stopped
 * @param[in]	pe The PE instance
 * @return		TRUE if PE is active, FALSE if not
 */
static bool_t pfe_pe_is_active_nolock(pfe_pe_t *pe)
{
	pfe_ct_pe_sw_state_monitor_t state_monitor;
    bool_t ret = FALSE;
    
	if(NULL == pe->mmap_data)
	{
		NXP_LOG_WARNING("PE %u: Firmware not loaded\n", pe->id);
	}
    else
    {
        ret = TRUE;
        /*	Get state */
        pfe_pe_memcpy_from_dmem_to_host_32_nolock(
                pe,
                &state_monitor,
                oal_ntohl(pe->mmap_data->common.state_monitor),
                sizeof(pfe_ct_pe_sw_state_monitor_t));

        if ((PFE_FW_STATE_STOPPED == state_monitor.state) || (PFE_FW_STATE_UNINIT == state_monitor.state))
        {
            ret = FALSE;
        }
        /*	PFE_FW_STATE_INIT == state_monitor.state is considered as running because
		the transition to next state is short */
    }

	return ret;
}

/**
* @brief Lock PE access
* @param[in] pe PE which access shall be locked
* @return EOK if success, error code otherwise
*/
errno_t pfe_pe_lock(pfe_pe_t *pe)
{
	errno_t ret = oal_mutex_lock(&pe->lock_mutex);

	if (unlikely(pe->miflock))
	{
		NXP_LOG_ERROR("Lock already indicated.\n");
	}

	if (likely(EOK == ret))
	{
		/*	Indicate the 'lock' status */
		pe->miflock = TRUE;
	}

	return ret;
}

/**
* @brief Unlock PE access
* @param[in] pe PE which access shall be unlocked
* @return EOK if success, error code otherwise
*/
errno_t pfe_pe_unlock(pfe_pe_t *pe)
{
	/*	Indicate the 'unlock' status */
	pe->miflock = FALSE;

	return oal_mutex_unlock(&pe->lock_mutex);
}

/**
 * @brief		Lock PE memory
 * @details		While locked, the PE can't access internal memory. Invoke the PE graceful
 *				stop request and wait for confirmation. Also lock the PE memory interface.
 * @param[in]	pe The PE instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_mem_lock(pfe_pe_t *pe)
{
	PFE_PTR(pfe_ct_pe_misc_control_t) misc_dmem;
	pfe_ct_pe_misc_control_t misc_ctrl = {0};
	uint32_t timeout = 10;

	if (NULL == pe->mmap_data)
	{
		return ENOEXEC;
	}

	misc_dmem = oal_ntohl(pe->mmap_data->common.pe_misc_control);
	if (0U == misc_dmem)
	{
		return EINVAL;
	}

	if (EOK != pfe_pe_lock(pe))
	{
		NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		return EPERM;
	}

	/*	Read the misc control structure from DMEM */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(
			pe, &misc_ctrl, misc_dmem, sizeof(pfe_ct_pe_misc_control_t));

	if (0U != misc_ctrl.graceful_stop_request)
	{
		if (0U != misc_ctrl.graceful_stop_confirmation)
		{
			NXP_LOG_ERROR("Locking locked memory\n");
		}
		else
		{
			NXP_LOG_ERROR("Duplicate stop request\n");
		}

		if (EOK != pfe_pe_unlock(pe))
		{
			NXP_LOG_ERROR("pfe_pe_unlock() failed\n");
		}

		return EPERM;
	}
	else
	{
		/*	Writing the non-zero value triggers the request */
		misc_ctrl.graceful_stop_request = 0xffU;
		/*	PE will respond with setting this to non-zero value */
		misc_ctrl.graceful_stop_confirmation = 0x0U;
	}

	/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
	pfe_pe_memcpy_from_host_to_dmem_32_nolock(
			pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));

	if (FALSE == pfe_pe_is_active_nolock(pe))
	{
		/*	Access to PE memories is considered to be safe. PE memory
			interface is locked. */
		return EOK;
	}

	/*	Wait for response */
	do
	{
		if (0U == timeout)
		{
			NXP_LOG_ERROR("Timed-out\n");

			/*	Cancel the request */
			misc_ctrl.graceful_stop_request = 0U;

			/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
			pfe_pe_memcpy_from_host_to_dmem_32_nolock(
					pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));

			if (EOK != pfe_pe_unlock(pe))
			{
				NXP_LOG_ERROR("pfe_pe_unlock() failed\n");
			}

			return ETIME;
		}

		oal_time_usleep(10U);
		timeout--;
		pfe_pe_memcpy_from_dmem_to_host_32_nolock(
				pe, &misc_ctrl, misc_dmem, sizeof(pfe_ct_pe_misc_control_t));

	} while (0U == misc_ctrl.graceful_stop_confirmation);

	/*	Access to PE memory interface is locked */
	return EOK;
}

/**
 * @brief		Unlock PE memory
 * @details		While locked, the PE can't access internal memory. Here the memory
 * 				and the memory interface will be unlocked.
 * @param[in]	pe The PE instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_mem_unlock(pfe_pe_t *pe)
{
	PFE_PTR(pfe_ct_pe_misc_control_t) misc_dmem;
	pfe_ct_pe_misc_control_t misc_ctrl = {0};

	if (NULL == pe->mmap_data)
	{
		return ENOEXEC;
	}

	misc_dmem = oal_ntohl(pe->mmap_data->common.pe_misc_control);
	if (0U == misc_dmem)
	{
		return EINVAL;
	}

	/*	Cancel the stop request */
	misc_ctrl.graceful_stop_request = 0U;

	/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
	pfe_pe_memcpy_from_host_to_dmem_32_nolock(
			pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));

	/*	Unlock access to PE memory interface */
	if (EOK != pfe_pe_unlock(pe))
	{
		NXP_LOG_ERROR("pfe_pe_unlock() failed\n");
	}

	return EOK;
}

/**
 * @brief		Get number of cycles to load PEs with configured load ops
 * @param[in]	pe The PE instance
 * @param[in]	pe_num Number of PEs that are being loaded
 * @return		Number of cycles to load all PEs
 */
static uint8_t pfe_pe_fw_load_cycles(const pfe_pe_t *pe, uint8_t pe_num)
{
	uint8_t ret = 1U;

	if (NULL != pe->fw_load_ops)
	{
		if(pe_num >= pe->fw_load_ops->pe_loaded_cnt)
		{
			ret = pe_num/pe->fw_load_ops->pe_loaded_cnt;
		}
	}

	return ret;
}

/**
 * @brief		Compare two PEs with regards of FW loading
 * @param[in]	pe1 The PE instance
 * @param[in]	pe2 The PE instance
 * @return		EOK on success
 */
static errno_t pfe_pe_fw_ops_valid(pfe_pe_t *pe1, const pfe_pe_t *pe2)
{
	errno_t ret = EINVAL;

	if ((pe1->type == pe2->type) &&
		(pe1->mem_access_addr == pe2->mem_access_addr) &&
		(pe1->mem_access_rdata == pe2->mem_access_rdata) &&
		(pe1->mem_access_wdata == pe2->mem_access_wdata))
	{
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get fastest possible FW load operations
 * @param[in]	pe The PE instance
 * @param[in]	pe_num Number of PEs that are being loaded
 * @return		EOK on success
 */
static errno_t pfe_pe_fw_install_ops(pfe_pe_t **pe, uint8_t pe_num)
{
	errno_t ret;
	uint32_t idx = 0U, pe_idx = 0U;
	uint8_t best_pe_loader_cnt = 0U;
	const fw_load_ops_t* pe_loader = NULL;

	for (idx = 0U; idx < (sizeof(fw_load_ops)/sizeof(fw_load_ops[0U])); ++idx)
	{
		ret = EINVAL;
		if (((pe_num == fw_load_ops[idx].pe_loaded_cnt) ||
			 (1U == fw_load_ops[idx].pe_loaded_cnt)) &&
			(fw_load_ops[idx].pe_loaded_cnt > best_pe_loader_cnt) &&
			(((pe[0]->type == PE_TYPE_UTIL) && (fw_load_ops[idx].can_load_util == TRUE)) ||
			(pe[0]->type != PE_TYPE_UTIL)))
		{
			if (1U < fw_load_ops[idx].pe_loaded_cnt)
			{
				for (pe_idx = 1U; pe_idx < pe_num; ++pe_idx)
				{
					/* To be sure that PEs are equivalent compare them here */
					ret = pfe_pe_fw_ops_valid(pe[0], pe[pe_idx]);

					if (EOK != ret)
					{
						NXP_LOG_ERROR("PEs are not identical\n");
						break;
					}
				}

				if (EOK == ret)
				{
					best_pe_loader_cnt = fw_load_ops[idx].pe_loaded_cnt;
					pe_loader = &fw_load_ops[idx];
				}
			}
			else
			{
				best_pe_loader_cnt = fw_load_ops[idx].pe_loaded_cnt;
				pe_loader = &fw_load_ops[idx];
			}
		}
	}

	ret = ENODEV;
	for (pe_idx = 0U; pe_idx < pe_num; ++pe_idx)
	{
		pe[pe_idx]->fw_load_ops = pe_loader;
	}

	if(NULL != pe_loader)
	{
		ret = EOK;
		NXP_LOG_INFO("Selected FW loading OPs to load %d PEs in parallel\n", pe_loader->pe_loaded_cnt);
	}

	return ret;
}

/**
 * @brief		Memcpy FW data to PEs
 * @warning		This is supposed to be called only during initial FW loading.
 * 				Expectation is that everything is 4B aligned and size is divisible by 4.
 * 				This function loads 8 PEs at the same time.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory type
 * @param[in]	dst_addr Destination PE address
 * @param[in]	src_ptr Source host address
 * @param[in]	len Copied length
 */
static void pfe_pe_fw_memcpy_bulk(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	uint32_t mem_addr, addr_temp;
	uint32_t *data = (uint32_t*)src_ptr;
	uint32_t memsel;

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (write).\n");
	}

	addr_temp = PE_IBUS_WRITE | memsel | PE_IBUS_WREN(0xf);

	/*
	 * IF we use gray code order in the unroll we will save very large number of instructions
	 *	So optimal order is
	 *	0 -> 1 -> 3 -> 2 -> 6 -> 7 -> 5 -> 4
	 */

	for (mem_addr = dst_addr; mem_addr < (dst_addr + len); mem_addr += 4U)
	{
		hal_write32(oal_htonl(*data), pe->mem_access_wdata);
		data++;
		/* Just do un-rool manually to save time */
		addr_temp &= (0xff060000U);
		addr_temp |= mem_addr;
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 21U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 22U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 21U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
	}
}

/**
 * @brief		Memset PEs memory
 * @warning		This is supposed to be called only during initial FW loading.
 *				Expectation is that everything is 4B aligned and size is divisible by 4.
 *				This function loads 8 PEs at the same time.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory type
 * @param[in]	val Value to set the memory
 * @param[in]	addr Destination PE address
 * @param[in]	size Copied length
 */
static void pfe_pe_fw_memset_bulk(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint32_t size)
{
	uint32_t mem_addr, addr_temp;
	uint32_t memsel;

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (write).\n");
	}

	hal_write32(oal_htonl(val), pe->mem_access_wdata);

	addr_temp = PE_IBUS_WRITE | memsel | PE_IBUS_WREN(0xf);

	/*
	 * IF we use gray code order in the unroll we will save very large number of instructions
	 *	So optimal order is
	 *	0 -> 1 -> 3 -> 2 -> 6 -> 7 -> 5 -> 4
	 */

	for (mem_addr = addr; mem_addr < (addr + size); mem_addr += 4U)
	{
		/* Just do un-rool to save time manually to save time */
		addr_temp &= (0xff060000U);
		addr_temp |= mem_addr;
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 21U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 22U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp |= ((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 21U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
		addr_temp &= ~((uint32_t)1U << 20U);
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
	}
}

/**
 * @brief		Memset PEs memory
 * @warning		This is supposed to be called only during initial FW loading.
 *				Expectation is that everything is 4B aligned and size is divisible by 4.
 *				This function can load single PE only.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory type
 * @param[in]	dst_addr Destination PE address
 * @param[in]	src_ptr Source host address
 * @param[in]	len Copied length
 */
static void pfe_pe_fw_memcpy_single(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	uint32_t mem_addr, addr_temp;
	uint32_t *data = (uint32_t*)src_ptr;
	uint32_t memsel;

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (write).\n");
	}

	addr_temp = PE_IBUS_WRITE | memsel | PE_IBUS_WREN(0xf) | PE_IBUS_PE_ID(pe->id);

	for (mem_addr = dst_addr; mem_addr < (dst_addr + len); mem_addr += 4U)
	{
		hal_write32(oal_htonl(*data), pe->mem_access_wdata);
		data++;
		addr_temp &= (0xfff60000U);
		addr_temp |= mem_addr;
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
	}
}

/**
 * @brief		Memset PEs memory
 * @warning		This is supposed to be called only during initial FW loading.
 * 				Expectation is that everything is 4B aligned and size is divisible by 4.
 * 				This function can load single PE only.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory type
 * @param[in]	val Value to set the memory
 * @param[in]	addr Destination PE address
 * @param[in]	size Copied length
 */
static void pfe_pe_fw_memset_single(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint32_t size)
{
	uint32_t mem_addr, addr_temp;
	uint32_t memsel;

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (write).\n");
	}

	hal_write32(oal_htonl(val), pe->mem_access_wdata);

	addr_temp = PE_IBUS_WRITE | memsel | PE_IBUS_WREN(0xf) | PE_IBUS_PE_ID(pe->id);

	/* We could potentially do some manual unroll here */
	for (mem_addr = addr; mem_addr < (addr + size); mem_addr += 4U)
	{
		addr_temp &= (0xfff60000U);
		addr_temp |= mem_addr;
		hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
	}
}

/**
 * @brief		Read data from PE memory
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Read address (should be aligned to 32 bits)
 * @param[in]	size Number of bytes to read (maximum 4)
 * @return		The data read (BE).
 */
static uint32_t pfe_pe_mem_read(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t addr, uint8_t size)
{
	uint32_t val;
	uint32_t mask;
	uint32_t memsel;
	uint8_t offset;
	uint8_t size_temp =size;
	addr_t adrr_temp = addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely(addr & 0x3U))
	{
		if (size_temp > BYTES_TO_4B_ALIGNMENT(addr))
		{
			/*	Here we need to split the read into two reads */

			/*	Read 1 (LS bytes). Recursive call. Limited to single recursion. */
			val = pfe_pe_mem_read(pe, mem, addr, (uint8_t)BYTES_TO_4B_ALIGNMENT(addr));
			offset = (uint8_t)(4U - (addr & 0x3U));
			size_temp = size - offset;
			adrr_temp = addr + offset;

			/*	Read 2 (MS bytes). Recursive call. Limited to single recursion. */
			val |= (pfe_pe_mem_read(pe, mem, adrr_temp, size_temp) << (8U * offset));

			return val;
		}
	}

	if (size_temp != 4U)
	{
		mask = ((uint32_t)1U << (size_temp * 8U)) - 1U;
	}
	else
	{
		mask = 0xffffffffU;
	}

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	adrr_temp = (adrr_temp & 0xfffffU)
				| PE_IBUS_READ
				| memsel
				| PE_IBUS_PE_ID(pe->id)
				| PE_IBUS_WREN(0U);

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (read).\n");
	}

	hal_write32((uint32_t)adrr_temp, pe->mem_access_addr);
	val = oal_ntohl(hal_read32(pe->mem_access_rdata));

	if (unlikely(adrr_temp & 0x3U))
	{
		/*	Move the value to the desired address offset */
		val = (val >> (8U * (adrr_temp & 0x3U)));
	}

	return (val & mask);
}

/**
 * @brief		Write data into PE memory
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Write address (should be aligned to 32 bits)
 * @param[in]	val Value to write (BE)
 * @param[in]	size Number of bytes to write (maximum 4)
 */
static void pfe_pe_mem_write(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint8_t size)
{
	uint8_t bytesel = 0U;
	uint32_t memsel;
	uint8_t offset;
	uint32_t val_temp = val;
	uint8_t size_temp = size;
	addr_t addr_temp = addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely((addr_temp & 0x3U) != 0U))
	{
		offset = BYTES_TO_4B_ALIGNMENT(addr_temp);

		if (size_temp <= offset)
		{
			/*	Move the value to the desired address offset */
			val_temp = val << (8U * (addr_temp & 0x3U));
			/*	Enable writes of depicted bytes */
			bytesel = (((1U << size_temp) - 1U) << (offset - size_temp));
		}
		else
		{
			/*	Here we need to split the write into two writes */

			/*	Write 1 (LS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val_temp, addr, offset);
			val_temp >>= 8U * offset;
			size_temp = size - offset;
			addr_temp = addr + offset;

			/*	Write 2 (MS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val_temp, addr_temp, size_temp);

			return;
		}
	}
	else
	{
		/*	Destination is aligned */
		bytesel = (uint8_t)PE_IBUS_BYTES(size_temp);
	}

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	addr_temp = (addr_temp & 0xfffffU)
			| PE_IBUS_WRITE
			| memsel
			| PE_IBUS_PE_ID(pe->id)
			| PE_IBUS_WREN(bytesel);

	/*	Sanity check if we can safely access the memory interface */
	if (unlikely(!pe->miflock))
	{
		NXP_LOG_ERROR("Accessing unlocked PE memory interface (write).\n");
	}

	hal_write32(oal_htonl(val_temp), pe->mem_access_wdata);
	hal_write32((uint32_t)addr_temp, pe->mem_access_addr);
}

/**
 * @brief		Write 'len' bytes to DMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src_ptr Buffer source address (virtual)
 * @param[in]	dst_addr DMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to read
 */
static void pfe_pe_memcpy_from_host_to_dmem_32_nolock(
		pfe_pe_t *pe, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src_ptr;
	addr_t dst_temp = dst_addr;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((dst_temp & 0x3U) != 0U)
	{
		/*	Write unaligned bytes to align the destination address */
		offset = BYTES_TO_4B_ALIGNMENT(dst_temp);
		offset = (len < offset) ? len : offset;
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, dst_temp, (uint8_t)offset);
		src_byteptr += offset;
		dst_temp = dst_addr + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte writes */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst_temp, 4U);
		len_temp-=4U;
		src_byteptr+=4U;
		dst_temp+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst_temp, (uint8_t)len_temp);
	}
}

/**
 * @brief		Write 'len' bytes to DMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src_ptr Buffer source address (virtual)
 * @param[in]	dst_addr DMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to read
 */
void pfe_pe_memcpy_from_host_to_dmem_32(pfe_pe_t *pe, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	errno_t ret;

	ret = pfe_pe_mem_lock(pe);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, dst_addr, src_ptr, len);
	
	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Read 'len' bytes from DMEM
 * @param[in]	pe The PE instance
 * @param[in]	src_addr DMEM source address (must be 32-bit aligned)
 * @param[in]	dst_ptr Destination address (virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_dmem_to_host_32_nolock(pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst_ptr;
	addr_t src_temp = src_addr;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((src_temp & 0x3U) != 0U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src_temp);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, (uint8_t)offset);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, offset);
		dst_byteptr += offset;
		src_temp = src_addr + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, 4U);
		*((uint32_t *)dst_byteptr) = val;
		len_temp-=4U;
		src_temp+=4U;
		dst_byteptr+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, (uint8_t)len_temp);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, len_temp);
	}
}

/**
 * @brief		Read 'len' bytes from DMEM
 * @param[in]	pe The PE instance
 * @param[in]	src_addr DMEM source address (must be 32-bit aligned)
 * @param[in]	dst_ptr Destination address (virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_dmem_to_host_32(pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len)
{
	errno_t ret;

	ret = pfe_pe_mem_lock(pe);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, dst_ptr, src_addr, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Read 'len' bytes from DMEM from each PE
 * @details		Reads PE internal data memory (DMEM) into a host memory through indirect
 *				access registers. The result from each PE are stored consecutively in memory
 *				pointed by dst.
 * @param[in]	pe Array of the PE instances
 * @param[in]	src_addr DMEM source address (physical within PE, must be 32bit aligned)
 * @param[in]	dst_ptr Destination address (virtual) the size required to store the data is pe_count*len
 * @param[in]	buffer_len Destination buffer length
 * @param[in]	read_len Number of bytes to read (from one PE)
 *
 */
errno_t pfe_pe_gather_memcpy_from_dmem_to_host_32(pfe_pe_t **pe, int32_t pe_count, void *dst_ptr, addr_t src_addr, uint32_t buffer_len, uint32_t read_len)
{
	int32_t ii = 0U;
	uint8_t mem_lock_error = 0U;
	errno_t ret = EOK;
	errno_t ret_store = EOK;

	/* Lock all PEs - they will stop processing frames and wait. This will
	   ensure data coherence. */
	for (ii = 0; ii < pe_count; ii++)
	{
		ret = pfe_pe_mem_lock(pe[ii]);
		if (EOK != ret)
		{
			mem_lock_error++;
			NXP_LOG_DEBUG("Memory lock failed for PE instance %d\n", (int_t)ii);
		}
	}

	/* Only read from PEs if all PEs are locked */
	if (0U == mem_lock_error)
	{
		/* Perform the read from required PEs */
		for (ii = 0; ii < pe_count; ii++)
		{
			/* Check if there is still memory  */
			if (buffer_len >= ((read_len * (uint32_t)ii) + read_len))
			{
				pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe[ii],
						(void *)((uint8_t*)dst_ptr + (read_len * (uint32_t)ii)),
							src_addr, read_len);
			}
			else
			{
				ret = ENOMEM;
				break;
			}
		}
	}

	/* Unlock all PEs */
	for (ii = 0; ii < pe_count; ii++)
	{
		/* Backup return from memcpy */
		ret_store = ret;
		ret = pfe_pe_mem_unlock(pe[ii]);
		if(EOK != ret)
		{
			NXP_LOG_DEBUG("Memory unlock failed\n");

			/* Error occurred. Override the return from previous actions. */
			ret_store = ret;
		}
	}

	/* Restore return to before unlock */
	ret = ret_store;

	return ret;
}

/**
 * @brief		Read 'len' bytes from IMEM
 * @param[in]	pe The PE instance
 * @param[in]	src_addr IMEM source address (physical within PE, must be 32-bit aligned)
 * @param[in]	dst_ptr Destination address (host, virtual)
 * @param[in]	len Number of bytes to read
 *
 */
#if defined(FW_WRITE_CHECK_EN)
static void pfe_pe_memcpy_from_imem_to_host_32_nolock(pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst_ptr;
	addr_t src_temp = src_addr;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((src_temp & 0x3U) != 0U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src_temp);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, (uint8_t)offset);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, offset);
		dst_byteptr += offset;
		src_temp = src_addr + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, 4U);
		*((uint32_t *)dst_byteptr) = val;
		len_temp-=4U;
		src_temp+=4U;
		dst_byteptr+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, (uint8_t)len_temp);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, len_temp);
	}
}
#endif /* FW_WRITE_CHECK_EN */

/**
 * @brief		Load an elf section into DMEM
 * @details		Size and load address need to be at least 32-bit aligned
 * @param[in]	pe The PE instance
 * @param[in]	sdata Pointer to the elf section data
 * @param[in]	addr Load address of the section
 * @param[in]	size Size of the section
 * @param[in]	type Section type
 * @retval		EOK Success
 * @retval		EINVAL Unsupported section type or wrong input address alignment
 */
static errno_t pfe_pe_load_dmem_section_nolock(pfe_pe_t *pe, const void *sdata, addr_t addr, addr_t size, uint32_t type)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else
    {
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((addr_t)(sdata) & 0x3U) != (addr & 0x3U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p don't have the same alignment\n", (void *)addr, sdata);
		ret = EINVAL;
	}
	else
	{
		if ((addr & 0x3U) != 0U)
		{
			NXP_LOG_ERROR("Load address 0x%p is not 32bit aligned\n", (void *)addr);
			ret = EINVAL;
		}
		else
		{
			switch (type)
			{
				case 0x7000002aU: /* MIPS.abiflags */
				{
					/* Skip the section */
					break;
				}
				case (uint32_t)SHT_PROGBITS:
				{
		#if defined(FW_WRITE_CHECK_EN)
					void *buf = oal_mm_malloc(size);
		#endif /* FW_WRITE_CHECK_EN */

					/*	Write section data */
					pe->fw_load_ops->pe_memcpy(pe, PFE_PE_DMEM, addr - pe->dmem_elf_base_va, sdata, size);

		#if defined(FW_WRITE_CHECK_EN)
					pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, buf, addr, size);

					if (0 != memcmp(buf, sdata, size))
					{
						NXP_LOG_ERROR("DMEM data inconsistent\n");
					}

					oal_mm_free(buf);
		#endif /* FW_WRITE_CHECK_EN */

					break;
				}

				case (uint32_t)SHT_NOBITS:
				{
					pe->fw_load_ops->pe_memset(pe, PFE_PE_DMEM, 0U, addr, size);
					break;
				}

				default:
				{
					NXP_LOG_ERROR("Unsupported section type: 0x%x\n", (uint_t)type);
					ret = EINVAL;
					break;
				}
			}
		}
	}
#if defined(PFE_CFG_NULL_ARG_CHECK)
    }
#endif

	return ret;
}

/**
 * @brief		Load an elf section into IMEM
 * @details		Code needs to be at least 16bit aligned and only PROGBITS sections are supported
 * @param[in]	pe The PE instance
 * @param[in]	data Pointer to the elf section data
 * @param[in]	addr Load address of the section
 * @param[in]	size Size of the section
 * @param[in]	type Type of the section
 * @retval		EOK Success
 * @retval		EFAULT Wrong input address alignment
 * @retval		EINVAL Unsupported section type
 */
static errno_t pfe_pe_load_imem_section_nolock(pfe_pe_t *pe, const void *data, addr_t addr, addr_t size, uint32_t type)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == data)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check alignment first */
	if (((addr_t)(data) & 0x3U) != (addr & 0x1U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p) don't have the same alignment\n",
				(void *)addr, data);
		return EFAULT;
	}

	if ((addr & 0x1U) != 0U)
	{
		NXP_LOG_ERROR("Load address 0x%p is not 16bit aligned\n", (void *)addr);
		return EFAULT;
	}

	if ((size & 0x1U) != 0U)
	{
		NXP_LOG_ERROR("Load size 0x%p is not 16bit aligned\n", (void *)size);
		return EFAULT;
	}

	switch (type)
	{
		case 0x7000002aU: /* MIPS.abiflags */
		{
			/* Skip the section */
			break;
		}
		case (uint32_t)SHT_PROGBITS:
		{
#if defined(FW_WRITE_CHECK_EN)
			void *buf = oal_mm_malloc(size);
#endif /* FW_WRITE_CHECK_EN */

			/*	Write section data */
			pe->fw_load_ops->pe_memcpy(pe, PFE_PE_IMEM, addr - pe->imem_elf_base_va, data, size);

#if defined(FW_WRITE_CHECK_EN)
			pfe_pe_memcpy_from_imem_to_host_32_nolock(pe, buf, addr, size);

			if (0 != memcmp(buf, data, size))
			{
				NXP_LOG_ERROR("IMEM data inconsistent\n");
			}

			oal_mm_free(buf);
			buf = NULL;
#endif /* FW_WRITE_CHECK_EN */

			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unsupported section type: 0x%x\n", (uint_t)type);
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

/**
 * @brief		Check if memory region belongs to DMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to DMEM
 */
static bool_t pfe_pe_is_dmem(const pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	reg_end = pe->dmem_elf_base_va + pe->dmem_size;

	if ((addr >= pe->dmem_elf_base_va) && ((addr + size) < reg_end))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if memory region belongs to IMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to IMEM
 */
static bool_t pfe_pe_is_imem(const pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	reg_end = pe->imem_elf_base_va + pe->imem_size;

	if ((addr >= pe->imem_elf_base_va) && ((addr + size) < reg_end))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Write elf section to PE memory
 * @details		Function expects the section data is in host endian format
 * @param[in]	pe The PE instance
 * @param[in]	sdata Pointer to the data described by 'shdr'
 * @param[in]	load_addr Address where to load the section
 * @param[in]	size Size of the section to load
 * @param[in]	type Type of the section to load
 */
static errno_t pfe_pe_load_elf_section(pfe_pe_t *pe, const void *sdata, addr_t load_addr, addr_t size, uint32_t type)
{
	errno_t ret_val;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (pfe_pe_is_dmem(pe, load_addr, size))
	{
		/*	Section belongs to DMEM */
		ret_val = pfe_pe_load_dmem_section_nolock(pe, sdata, load_addr, size, type);
	}
	else if (pfe_pe_is_imem(pe, load_addr, size))
	{
		/*	Section belongs to IMEM */
		ret_val = pfe_pe_load_imem_section_nolock(pe, sdata, load_addr, size, type);
	}
	else
	{
		NXP_LOG_ERROR("Unsupported memory range %p\n", (void *)load_addr);
		ret_val = EINVAL;
	}

	return ret_val;
}

/**
 * @brief Translates section virtual address into load address
 * @param[in] elf_file Elf file containing the section to translate the address
 * @param[in] shdr Section header of the section to translate the address
 * @details Elf file section header contains only section virtual address which is used by the
 *          running software. The virtual address needs to be translated to load address which
 *          is address where the section is loaded into memory. In most cases the virtual and
 *          load address are equal.
 * @return Load address of the given section or 0 on failure.
 */
static addr_t pfe_pe_get_elf_sect_load_addr(const ELF_File_t *elf_file, const Elf32_Shdr *shdr)
{
	addr_t virt_addr = shdr->sh_addr;
	addr_t load_addr;
	addr_t offset;
	const Elf32_Phdr *phdr;
	uint_t ii;

	/* Go through all program headers to find one containing the section */
	for (ii=0U; ii<elf_file->Header.r32.e_phnum; ii++)
	{
		phdr = &elf_file->arProgHead32[ii];
		if((virt_addr >= phdr->p_vaddr) &&
		   (virt_addr <= (phdr->p_vaddr + phdr->p_memsz - shdr->sh_size)))
		{   /* Address belongs into this segment */
			/* Calculate the offset between segment load and virtual address */
			offset = phdr->p_vaddr - phdr->p_paddr;
			/* Same offset applies also for sections in the segment */
			load_addr = virt_addr - offset;
			return load_addr;
		}
	}
	/* No segment containing the section was found ! */
	NXP_LOG_ERROR("Translation of 0x%"PRINTADDR_T"x failed, fallback used\n", virt_addr);

	return 0;
}

/**
 * @brief		Create new PE instance
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @param[in]	type Type of PE to create @see pfe_pe_type_t
 * @param[in]	id PE ID
 * @return		The PE instance or NULL if failed
 */
pfe_pe_t * pfe_pe_create(addr_t cbus_base_va, pfe_ct_pe_type_t type, uint8_t id)
{
	static bool_t mem_access_lock_init = FALSE;
	pfe_pe_t *pe = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((type != PE_TYPE_INVALID) && (type < PE_TYPE_MAX))
	{
		pe = oal_mm_malloc(sizeof(pfe_pe_t));

		if (NULL != pe)
		{
			(void)memset(pe, 0, sizeof(pfe_pe_t));
			pe->type = type;
			pe->cbus_base_va = cbus_base_va;
			pe->id = id;
			pe->fw_err_section = NULL;
			pe->mmap_data = NULL;
			if(EOK != oal_mutex_init(&pe->lock_mutex))
			{
				NXP_LOG_DEBUG("Mutex init failed\n");
				oal_mm_free(pe);
				pe = NULL;
			}

			if (FALSE == mem_access_lock_init)
			{
				if (EOK == oal_mutex_init(&mem_access_lock))
				{
					mem_access_lock_init = TRUE;
				}
				else
				{
					NXP_LOG_DEBUG("Mutex (mem_access_lock) init failed\n");
					if (NULL != pe)
					{
						(void)oal_mutex_destroy(&pe->lock_mutex);
						oal_mm_free(pe);
						pe = NULL;
					}
				}
			}
		}
	}

	return pe;
}

/**
 * @brief		Set DMEM base address for .elf mapping
 * @warning		Not intended to be called when PE is running
 * @param[in]	pe The PE instance
 * @param[in]	elf_base DMEM base virtual address within .elf
 * @param[in]	len DMEM memory length
 */
void pfe_pe_set_dmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->dmem_elf_base_va = elf_base;
	pe->dmem_size = len;
}

/**
 * @brief		Set IMEM base address for .elf mapping
 * @warning		Not intended to be called when PE is running
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va IMEM base virtual address within .elf
 * @param[in]	len IMEM memory length
 */
void pfe_pe_set_imem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->imem_elf_base_va = elf_base;
	pe->imem_size = len;
}

/**
 * @brief		Set LMEM base address
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va LMEM base virtual address within .elf
 * @param[in]	len LMEM memory length
 */
void pfe_pe_set_lmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->lmem_base_addr_pa = elf_base;
	pe->lmem_size = len;
}

/**
 * @brief		Set DDR base address
 * @param[in]	pe The PE instance
 * @param[in]	base_pa DDR base physical address as seen by host
 * @param[in]	base_va DDR base virtual address
 * @param[in]	len DDR region length
 */
void pfe_pe_set_ddr(pfe_pe_t *pe, addr_t base_pa, addr_t base_va, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->ddr_base_addr_pa = base_pa;
	pe->ddr_base_addr_va = base_va;
	pe->ddr_size = len;
}

/**
 * @brief		Set indirect access registers
 * @param[in]	pe The PE instance
 * @param[in]	wdata_reg The WDATA register address as appears on CBUS
 * @param[in]	rdata_reg The RDATA register address as appears on CBUS
 * @param[in]	addr_reg The ADDR register address as appears on CBUS
 */
void pfe_pe_set_iaccess(pfe_pe_t *pe, uint32_t wdata_reg, uint32_t rdata_reg, uint32_t addr_reg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->mem_access_addr = (pe->cbus_base_va + addr_reg);
	pe->mem_access_rdata = (pe->cbus_base_va + rdata_reg);
	pe->mem_access_wdata = (pe->cbus_base_va + wdata_reg);
}

static void print_fw_issue(const pfe_ct_pe_mmap_t *fw_mmap)
{
#ifdef NXP_LOG_ENABLED 
	NXP_LOG_ERROR("Unsupported firmware detected: Found revision %d.%d.%d (fwAPI:%s), required fwAPI %s\n",
			fw_mmap->common.version.major, fw_mmap->common.version.minor, fw_mmap->common.version.patch, fw_mmap->common.version.cthdr,
			TOSTRING(PFE_CFG_PFE_CT_H_MD5));
#else
	(void)fw_mmap;
#endif
}

/**
 * @brief		Upload firmware into PEs memory
 * @param[in]	pe The PE instances
 * @param[in]	pe_num Number of PE instances
 * @param[in]	elf The elf file object to be uploaded
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_load_firmware(pfe_pe_t **pe, uint32_t pe_num, const void *elf)
{
	uint32_t ii, pe_idx;
	errno_t ret = EOK;
	uint32_t section_idx = 0U;
	uint32_t mask_sectIdx;
	const ELF_File_t *elf_file = (ELF_File_t *)elf;
	const Elf32_Shdr *shdr = NULL;
	static pfe_ct_pe_mmap_t *tmp_mmap = NULL;
	uint32_t mmap_size;
	uint32_t features_size = 0, errors_size = 0;
	void *features_mem = NULL, *errors_mem = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (pe_idx = 0; pe_idx < pe_num; ++pe_idx)
	{
		if (EOK != pfe_pe_lock(pe[pe_idx]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}
		/* TODO unlock if it fails (maybe it doesn't matter  as we fail init anyway ?)*/
	}

	ret =  pfe_pe_fw_install_ops(pe, (uint8_t)pe_num);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Couldn't find PE load operations: %d\n", ret);
	}

	for(pe_idx = 0; pe_idx < pfe_pe_fw_load_cycles(pe[0], (uint8_t)pe_num); ++pe_idx)
	{
		/* Check that we have the ops */
		if(NULL == pe[pe_idx]->fw_load_ops)
		{
			return ENODEV;
		}

		pe[pe_idx]->fw_load_ops->pe_memset(pe[pe_idx], PFE_PE_DMEM, 0, 0, pe[pe_idx]->dmem_size);
		pe[pe_idx]->fw_load_ops->pe_memset(pe[pe_idx], PFE_PE_IMEM, 0, 0, pe[pe_idx]->imem_size);
	}

	/*	Attempt to get section containing firmware memory map data */
	if (TRUE == ELF_SectFindName(elf_file, ".pfe_pe_mmap", &section_idx, NULL, NULL))
	{
		/* Mask out the flag to get section id */
		mask_sectIdx = (~(ELF_NAMED_SECT_IDX_FLAG) & section_idx);

		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[mask_sectIdx];

		/* Get the mmap size, used to load correct data from FW file*/
		(void)memcpy(
				(void*)&mmap_size,
				(const void*)((addr_t)elf_file->pvData + shdr->sh_offset),
				sizeof(uint32_t));

		/* Convert mmap size endian ! */
		mmap_size = oal_ntohl(mmap_size);

		/* Allocate memory with size of union variable to maintain memory consistency */
		tmp_mmap = (pfe_ct_pe_mmap_t *)oal_mm_malloc(sizeof(*tmp_mmap));
		if (NULL == tmp_mmap)
		{
			ret = ENOMEM;
			pfe_pe_free_mem(pe, pe_num);
			return ret;
		}
		else
		{
			/*  Firmware version check */
			static const char_t mmap_version_str[] = TOSTRING(PFE_CFG_PFE_CT_H_MD5);

			(void)memcpy(
					(void*)tmp_mmap,
					(const void*)((addr_t)elf_file->pvData + shdr->sh_offset),
					mmap_size);

			if(0 != strcmp(mmap_version_str, tmp_mmap->common.version.cthdr))
			{
				ret = EINVAL;
				print_fw_issue(tmp_mmap);
				pfe_pe_free_mem(pe, pe_num);
				return ret;
			}

			NXP_LOG_INFO("pfe_ct.h file version\"%s\"\n", mmap_version_str);
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.pfe_pe_mmap). Memory map will not be available.\n");
	}

	/*	Attempt to get section containing firmware diagnostic data */
	if (TRUE == ELF_SectFindName(elf_file, ".errors", &section_idx, NULL, NULL))
	{
		/* Mask out the flag to get section id */
		mask_sectIdx = (~(ELF_NAMED_SECT_IDX_FLAG) & section_idx);

		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[mask_sectIdx];
		errors_mem = oal_mm_malloc(shdr->sh_size);
		if (NULL == errors_mem)
		{
			ret = ENOMEM;
			pfe_pe_free_mem(pe, pe_num);
			return ret;
		}
		else
		{
			(void)memcpy(errors_mem, (const void *)((uint8_t *)elf_file->pvData + shdr->sh_offset), shdr->sh_size);
			errors_size = shdr->sh_size;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.errors). FW error reporting will not be available.\n");
	}

	/*	Attempt to get section containing firmware supported features */
	if (TRUE == ELF_SectFindName(elf_file, ".features", &section_idx, NULL, NULL))
	{
		/* Mask out the flag to get section id */
		mask_sectIdx = (~(ELF_NAMED_SECT_IDX_FLAG) & section_idx);

		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[mask_sectIdx];
		features_mem = oal_mm_malloc(shdr->sh_size);
		if (NULL == features_mem)
		{
			ret = ENOMEM;
			pfe_pe_free_mem(pe, pe_num);
			return ret;
		}
		else
		{
			(void)memcpy(features_mem, (const void *)((addr_t)elf_file->pvData + shdr->sh_offset), shdr->sh_size);
			features_size = shdr->sh_size;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.features). FW features management will not be available.\n");
	}

	/*	.elf data must be in BIG ENDIAN */
	if (1U == elf_file->Header.e_ident[EI_DATA])
	{
		NXP_LOG_DEBUG("Unexpected .elf format (little endian)\n");
		ret = EINVAL;
		pfe_pe_free_mem(pe, pe_num);
		return ret;
	}

	/*	Try to upload all sections of the .elf */
	ret = pfe_pe_upload_sections(pe, pe_num, elf_file);
	if (EOK != ret)
	{
		return ret;
	}

	for (ii = 0; ii < pe_num; ++ii)
	{
		if (EOK != pfe_pe_unlock(pe[ii]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}

		/*	Indicate that mmap_data is available */
		pe[ii]->mmap_data = tmp_mmap;

		pe[ii]->fw_err_section_size = errors_size;
		/*	Indicate that fw_err_section is available */
		pe[ii]->fw_err_section = errors_mem;

		pe[ii]->fw_feature_section_size = features_size;

		/*	Indicate that fw_feature_section is available */
		pe[ii]->fw_feature_section = features_mem;
		pe[ii]->fw_features_base = INVALID_FEATURES_BASE; /* Invalid value */

		/* Clear the internal copy of the index on each FW load because
		   FW will also start from 0 */
		pe[ii]->last_error_write_index = 0U;
		pe[ii]->error_record_addr = 0U;
	}

	return ret;
}


/**
 * @brief		Get pointer to PE's memory where memory map data is stored
 * @param[in]	pe The PE instance
 * @param[out]	mmap Pointer where memory map shall be written (values are in network byte order)
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Requested data not available
 */
errno_t pfe_pe_get_mmap(const pfe_pe_t *pe, pfe_ct_pe_mmap_t *mmap)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == mmap)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != pe->mmap_data)
	{
		(void)memcpy(mmap, (const void *)pe->mmap_data, sizeof(pfe_ct_pe_mmap_t));
		return EOK;
	}
	else
	{
		return ENOENT;
	}
}

/**
 * @brief		Destroy PE instances
 * @param[in]	pe The list of PE instances
 * @param[in]	pe_num The number of PE instances
 */
void pfe_pe_destroy(pfe_pe_t **pe, uint32_t pe_num)
{
	uint32_t pe_idx;

	if ((NULL != pe) && (0U < pe_num))
	{
		if (NULL != pe[0]->mmap_data)
		{
			oal_mm_free(pe[0]->mmap_data);
		}

		if (NULL != pe[0]->fw_err_section)
		{
			oal_mm_free(pe[0]->fw_err_section);
		}

		if (NULL != pe[0]->fw_feature_section)
		{
			oal_mm_free(pe[0]->fw_feature_section);
		}

		for (pe_idx = 0 ; pe_idx < pe_num; ++pe_idx)
		{
			pe[pe_idx]->mmap_data = NULL;

			pe[pe_idx]->fw_err_section = NULL;
			pe[pe_idx]->fw_err_section_size = 0U;

			pe[pe_idx]->fw_feature_section = NULL;
			pe[pe_idx]->fw_feature_section_size = 0U;

			(void)oal_mutex_destroy(&pe[pe_idx]->lock_mutex);
			oal_mm_free(pe[pe_idx]);

			pe[pe_idx] = NULL;
		}
	}
}

/**
* @brief Returns a string base from the features description section
* @param[in] pe PE to be used
* @return Either the string base or NULL.
*/
char *pfe_pe_get_fw_feature_str_base(const pfe_pe_t *pe)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(INVALID_FEATURES_BASE != pe->fw_features_base)
	{
		return (char *)(pe->fw_feature_section);
	}
	return NULL;
}

/**
* @brief Returns feature description from special .elf section
* @param[in] pe PE to be used
* @param[in] id Id of the feature - its position in the section.
* @param[out] entry Pointer to the description is stored here
* @retun Either error code on failure or EOK
*/
errno_t pfe_pe_get_fw_feature_entry(pfe_pe_t *pe, uint32_t id, pfe_ct_feature_desc_t **entry)
{
	uint32_t entry_ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Check whether the section with features description is available */
	if(NULL == pe->fw_feature_section)
	{	/* Avoid running uninitialized */
		return ENOENT;
	}

	/* Get the pointer to the descriptions and count of the features
	   (do it only once and remember the values) */
	if(INVALID_FEATURES_BASE == pe->fw_features_base)
	{
		pfe_ct_pe_mmap_t pfe_pe_mmap;

		/* The mmap has not been queried for error record yet. Query map for
		   the error record address. */
		if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			return ENOENT;
		}

		/* Remember the features record address and size */
		pe->fw_features_base = oal_ntohl(pfe_pe_mmap.common.version.features);
		if(pe->fw_features_base > pe->fw_feature_section_size)
		{
			NXP_LOG_ERROR("Invalid address of features record 0x%x\n", (uint_t)pe->fw_features_base);
			pe->fw_features_base = INVALID_FEATURES_BASE;
			return EIO;
		}

		pe->fw_features_size = oal_ntohl(pfe_pe_mmap.common.version.features_count);
	}

	/* Check if the requested id does exist */
	if(id < pe->fw_features_size)
	{
		/* Entry at given id does exist so return it */
		entry_ptr = oal_ntohl(*(uint32_t *)(
						(addr_t)pe->fw_feature_section
							+ (pe->fw_features_base
							+ (id * sizeof(PFE_PTR(pfe_ct_feature_desc_t))))));

		*entry = (pfe_ct_feature_desc_t *)((addr_t)pe->fw_feature_section + entry_ptr);

		return EOK;
	}

	return ENOENT;
}

/**
 * @brief		Reads out errors reported by the PE Firmware and prints them on debug console
 * @param[in]	pe PE which error report shall be read out
 * @return		EOK on success or error code
 */
errno_t pfe_pe_get_fw_errors_nolock(pfe_pe_t *pe)
{
#ifdef NXP_LOG_ENABLED
	pfe_ct_error_record_t error_record; /* Copy of the PE error record */
	uint32_t read_start;                /* Starting position in error record to read */
	uint32_t i;
	uint32_t errors_count;
	pfe_ct_pe_mmap_t pfe_pe_mmap;

	if(NULL == pe->fw_err_section)
	{
		/* Avoid running uninitialized */
		return ENOENT;
	}

	if(0U == pe->error_record_addr)
	{
		/* The memory map has not been queried for error record yet. Get
		   the map and query it for the error record address. */
		if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			return ENOENT;
		}

		/* Remember the error record address */
		pe->error_record_addr = oal_ntohl(pfe_pe_mmap.common.error_record);
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(
			pe, &error_record, pe->error_record_addr, sizeof(pfe_ct_error_record_t));

	/* Get the number of new errors */
	errors_count = oal_ntohl(error_record.write_index) - pe->last_error_write_index;

	/* First unread error */
	read_start = pe->last_error_write_index;

	/* Where to continue next time */
	pe->last_error_write_index = oal_ntohl(error_record.write_index);
	if(0U != errors_count)
	{
		/* New errors reported - go through them */
		if(errors_count > FP_ERROR_RECORD_SIZE)
		{
			NXP_LOG_WARNING("FW error log overflow by %u\n",
					(uint_t)errors_count - FP_ERROR_RECORD_SIZE + 1U);

			/* Overflow has occurred - the write_index contains oldest record */
			read_start = oal_ntohl(error_record.write_index);
			errors_count = FP_ERROR_RECORD_SIZE;
		}

		for(i = 0U; i < errors_count; i++)
		{
			uint32_t error_addr;
			uint32_t error_line;
			const pfe_ct_error_t *error_ptr;
			const char_t *error_str;
			const char_t *error_file;
			uint32_t error_val;

			error_addr = oal_ntohl(error_record.errors[(read_start + i)
													  & (FP_ERROR_RECORD_SIZE - 1U)]);
			error_val = oal_ntohl(error_record.values[(read_start + i)
													  & (FP_ERROR_RECORD_SIZE - 1U)]);
			if(error_addr > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid error address from FW 0x%x\n", (uint_t)error_addr);
				break;
			}

			/* Get to the error message through the .errors section */
			error_ptr = (pfe_ct_error_t *)((addr_t)pe->fw_err_section + error_addr);
			if(oal_ntohl(error_ptr->message) > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid error message from FW 0x%x",
						(uint_t)oal_ntohl(error_ptr->message));
				break;
			}

			error_str = (char_t *)((addr_t)pe->fw_err_section + oal_ntohl(error_ptr->message));
			if(oal_ntohl(error_ptr->file) > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid file name from FW 0x%x",
						(uint_t)oal_ntohl(error_ptr->file));
				break;
			}

			error_file =  (char_t *)((addr_t)pe->fw_err_section + oal_ntohl(error_ptr->file));
			error_line = oal_ntohl(error_ptr->line);

			NXP_LOG_ERROR("PE%d: %s line %u: %s (0x%x)\n",
					pe->id, error_file, (uint_t)error_line, error_str, (uint_t)error_val);
		}
	}
#else
    (void)pe;
#endif /* NXP_LOG_ENABLED */
	return EOK;
}

/**
 * @brief Reads and validates PE mmap
 * @param[in] pe The PE instance
 */
errno_t pfe_pe_check_mmap(const pfe_pe_t *pe)
{
	pfe_ct_pe_mmap_t pfe_pe_mmap;

	/*	Get mmap base from PE[0] since all PEs have the same memory map */
	if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
	{
		NXP_LOG_ERROR("Could not get memory map\n");
		return ENOENT;
	}

	NXP_LOG_INFO("[FW VERSION] %d.%d.%d, Build: %s, %s (%s), ID: 0x%x\n",
			pfe_pe_mmap.common.version.major,
			pfe_pe_mmap.common.version.minor,
			pfe_pe_mmap.common.version.patch,
			(char_t *)pfe_pe_mmap.common.version.build_date,
			(char_t *)pfe_pe_mmap.common.version.build_time,
			(char_t *)pfe_pe_mmap.common.version.vctrl,
			(uint_t)pfe_pe_mmap.common.version.id);

	return EOK;
}

/**
 * @brief		Copies PE (global) statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_pe_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_pe_stats_t *stats)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_pe_stats_t));

	return EOK;
}

/**
 * @brief		Copies PE classification algorithms statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_classify_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_classify_stats_t *stats)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_classify_stats_t));

	return EOK;
}

/**
 * @brief		Copies classification algorithm or logical interface statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_class_algo_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_class_algo_stats_t *stats)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_class_algo_stats_t));

	return EOK;
}

/**
 * @brief		Converts statistics of a logical interface or classification algorithm into a text form
 * @param[in]	stat		Statistics to convert
 * @param[out]	buf			Buffer where to write the text
 * @param[in]	buf_len		Buffer length
 * @param[in]	verb_level	Verbosity level
 * @return		Number of bytes written into the output buffer
 */
uint32_t pfe_pe_stat_to_str(const pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

	(void)verb_level;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == stat) || (NULL == buf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif

	len += oal_util_snprintf(buf + len, buf_len - len, "Frames processed: %u\n", oal_ntohl(stat->processed));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames accepted:  %u\n", oal_ntohl(stat->accepted));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames rejected:  %u\n", oal_ntohl(stat->rejected));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames discarded: %u\n", oal_ntohl(stat->discarded));

	return len;
}

/**
 * @brief Translates state pfe_ct_pe_sw_state_t to a string
 * @param[in] state State to be translated
 * @return String representation of the state
 */
static inline const char_t *pfe_pe_get_fw_state_str(pfe_ct_pe_sw_state_t state)
{
	const char *ret;
	switch(state)
	{
		case PFE_FW_STATE_UNINIT:
			ret = "UNINIT";
			break;
		case PFE_FW_STATE_INIT:
			ret = "INIT";
			break;
		case PFE_FW_STATE_FRAMEWAIT:
			ret = "FRAMEWAIT";
			break;
		case PFE_FW_STATE_FRAMEPARSE:
			ret = "FRAMEPARSE";
			break;
		case PFE_FW_STATE_FRAMECLASSIFY:
			ret = "FRAMECLASSIFY";
			break;
		case PFE_FW_STATE_FRAMEDISCARD:
			ret = "FRAMEDISCARD";
			break;
		case PFE_FW_STATE_FRAMEMODIFY:
			ret = "FRAMEMODIFY";
			break;
		case PFE_FW_STATE_FRAMESEND:
			ret = "FRAMESEND";
			break;
		case PFE_FW_STATE_STOPPED:
			ret = "STOPPED";
			break;
		default:
			ret = "Unknown";
			break;
	}
	return ret;
}

/**
* @brief Reads and prints measurements from the PE memory
* @param[in] pe PE which shall be read
* @param[in] count Number of measurements in the PE memory to be read
* @param[in] ptr Location of the measurements record in the PE memory
* @param[in] buf Output text data buffer
* @param[in] buf_len Size of the output text data buffer
* @param[in] verb_level Verbosity level
* @return Number of bytes written into the text buffer
*/
static uint32_t pfe_pe_get_measurements_nolock(pfe_pe_t *pe, uint32_t count, uint32_t ptr, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	pfe_ct_measurement_t *m = NULL;
	uint_t i;
	uint32_t len = 0U;

	(void)verb_level;

	if(0U == ptr)
	{   /* This shall not happen - FW did not initialize data correctly */
		NXP_LOG_ERROR("Inconsistent data in pfe_pe_mmap\n");
	}
    else
    {
        /* Get buffer to read data from DMEM */
        m = oal_mm_malloc(sizeof(pfe_ct_measurement_t) * count);
        if(NULL != m)
        {   /* Memory allocation succeed */
            /* Copy the data into the allocated buffer */
            pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, m, ptr, sizeof(pfe_ct_measurement_t) * count);

            /* Print the data */
            for(i = 0U; i < count; i++)
            {
                /* Variables just to make code more readable */
                uint32_t avg = oal_ntohl(m[i].avg);
                uint32_t min = oal_ntohl(m[i].min);
                uint32_t max = oal_ntohl(m[i].max);
                uint32_t cnt = oal_ntohl(m[i].cnt);

                /* Just print the data without interpreting them */
                len += oal_util_snprintf(buf + len, buf_len - len,
                        "Measurement %u:\tmin %10u\tmax %10u\tavg %10u\tcnt %10u\n",
                            i, min, max, avg, cnt);
            }

            /* Free the allocated buffer */
            oal_mm_free(m);
        }
    }
	
	return len;
}

/**
 * @brief		Provides current state of PE firmware
 * @param[in]	pe 			The PE instance
 * @return		Current state of PE firmware
 *
 */
pfe_ct_pe_sw_state_t pfe_pe_get_fw_state(pfe_pe_t *pe)
{
	pfe_ct_pe_sw_state_monitor_t state_monitor;

	/* Get the pfe_ct_pe_mmap_t structure from PE */
	if (NULL == pe->mmap_data)
	{
		return PFE_FW_STATE_UNINIT;
	}

	/*	We don't need coherent data here so only lock the
	 	memory interface without locking the PE memory. */
	if (EOK != pfe_pe_lock(pe))
	{
		NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &state_monitor,
					oal_ntohl(pe->mmap_data->common.state_monitor), 
					sizeof(pfe_ct_pe_sw_state_monitor_t));

	if (EOK != pfe_pe_unlock(pe))
	{
		NXP_LOG_DEBUG("pfe_pe_unlock() failed\n");
	}

	return state_monitor.state;
}

/**
 * @brief		Read "put" buffer
 * @param[in]	pe The PE instance
 * @param[out]	buf Pointer to memory where buffer shall be written
 * @retval		EOK Success and buffer is valid
 * @retval		EAGAIN Buffer is invalid
 * @retval		ENOENT Buffer not found
 */
errno_t pfe_pe_get_data_nolock(pfe_pe_t *pe, pfe_ct_buffer_t *buf)
{
	uint8_t flags;
	errno_t ret = ENOENT;
	pfe_ct_pe_mmap_t mmap_data;
	const pfe_ct_class_mmap_t *class_mmap_data;

	/*	Get mmap base from PE[0] since all PEs have the same memory map */
	if (EOK != pfe_pe_get_mmap(pe, &mmap_data))
	{
		NXP_LOG_ERROR("Could not get memory map\n");
	}
    else
    {
        class_mmap_data = &mmap_data.class_pe;

        if (0U != class_mmap_data->put_buffer)
        {
            /*	Get "put" buffer status */
            pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &flags,
                    oal_ntohl(class_mmap_data->put_buffer) + offsetof(pfe_ct_buffer_t, flags),
                        sizeof(uint8_t));

            if (0U != flags)
            {
                /*	Copy buffer to local memory */
                pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, buf, oal_ntohl(class_mmap_data->put_buffer), sizeof(pfe_ct_buffer_t));

                /*	Clear flags */
                flags = 0U;

                pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe,
                        oal_ntohl(class_mmap_data->put_buffer) + offsetof(pfe_ct_buffer_t, flags),
                            &flags, sizeof(uint8_t));

                ret = EOK;
            }
            else
            {
                ret = EAGAIN;
            }
        }
    }
	

	return ret;
}

/**
 * @brief		Write "get" buffer
 * @param[in]	pe The PE instance
 * @param[out]	buf Pointer to data to be written
 * @retval		EOK Success and buffer is valid
 * @retval		EAGAIN Buffer is occupied
 * @retval		ENOENT Buffer not found
 */
errno_t pfe_pe_put_data_nolock(pfe_pe_t *pe, pfe_ct_buffer_t *buf)
{
	uint8_t flags;
	errno_t ret = ENOENT;
	pfe_ct_pe_mmap_t mmap_data;
	const pfe_ct_class_mmap_t *class_mmap_data;

	if (EOK != pfe_pe_get_mmap(pe, &mmap_data))
	{
		NXP_LOG_ERROR("Could not get memory map\n");
	}
    else
    {
        class_mmap_data = &mmap_data.class_pe;
        if (0U != class_mmap_data->get_buffer)
        {
            /*	Get "get" buffer status */
            pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &flags,
                    oal_ntohl(class_mmap_data->get_buffer) + offsetof(pfe_ct_buffer_t, flags),
                        sizeof(uint8_t));

            if (0U == flags)
            {
                /*	Send data to PFE */
                buf->flags |= 1U;

                pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe,
                        oal_ntohl(class_mmap_data->get_buffer), buf, sizeof(pfe_ct_buffer_t));

                ret = EOK;
            }
            else
            {
                ret = EAGAIN;
            }
        }
    }

	return ret;
}

/**
 * @brief		Return PE runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	pe 			The PE instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 *
 */
uint32_t pfe_pe_get_text_statistics(pfe_pe_t *pe, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	pfe_ct_pe_sw_state_monitor_t state_monitor;
	errno_t ret;

	if (NULL == pe->mmap_data)
	{
		return 0U;
	}

	len += oal_util_snprintf(buf + len, buf_len - len, "\nPE %u\n----\n", pe->id);
	len += oal_util_snprintf(buf + len, buf_len - len, "- PE state monitor -\n");

	/*	Make the PFE data coherent */
	ret = pfe_pe_mem_lock(pe);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return 0;
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &state_monitor,
			oal_ntohl(pe->mmap_data->common.state_monitor), sizeof(pfe_ct_pe_sw_state_monitor_t));

	len += oal_util_snprintf(buf + len, buf_len - len, "FW State: %u (%s), counter %u\n",
			state_monitor.state, pfe_pe_get_fw_state_str(state_monitor.state),
				oal_ntohl(state_monitor.counter));

	/* This is a class PE therefore we may access the specific data */
	if (0U != oal_ntohl(pe->mmap_data->common.measurement_count))
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "- Measurements -\n");

		/* Read processing time measurements */
		len += pfe_pe_get_measurements_nolock(pe, oal_ntohl(pe->mmap_data->common.measurement_count),
				oal_ntohl(pe->mmap_data->common.measurements), buf + len, buf_len - len, verb_level);
	}

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
		len = 0U;
	}

	return len;
}
