/* =========================================================================
 *  Copyright 2018-2019 NXP
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

/**
 * @addtogroup  dxgr_PFE_CLASS
 * @{
 *
 * @file		pfe_class.c
 * @brief		The CLASS module source file.
 * @details		This file contains CLASS-related functionality.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_mmap.h"
#include "pfe_pe.h"
#include "pfe_class.h"
#include "pfe_class_csr.h"
#include "blalloc.h"

#if !defined (PFE_CFG_FIRMWARE_VARIANT)
#error Please specify firmware variant: PFE_FW_SBL or PFE_FW_FULL
#endif /* PFE_CFG_FIRMWARE_VARIANT */

#if (PFE_CFG_FIRMWARE_VARIANT != PFE_FW_SBL) && (PFE_CFG_FIRMWARE_VARIANT != PFE_FW_FULL)
#error Unsupported firmware variant selected
#endif /* PFE_CFG_FIRMWARE_VARIANT */

#if PFE_CFG_FIRMWARE_VARIANT == PFE_FW_SBL
	#define PEMBOX_ADDR_CLASS		0x890U
#endif /* PFE_FW_SBL */

/* Configures size of the dmem heap allocator chunk (the smallest allocated memory size)
* The size is actually 2^configured value: 1 means 2, 2 means 4, 3 means 8, 4 means 16 etc.
*/
#define PFE_CLASS_HEAP_CHUNK_SIZE 4


struct __pfe_classifier_tag
{
	bool_t is_fw_loaded;					/*	Flag indicating that firmware has been loaded */
	bool_t enabled;							/*	Flag indicating that classifier has been enabled */
	void *cbus_base_va;						/*	CBUS base virtual address */
	uint32_t pe_num;						/*	Number of PEs */
	pfe_pe_t **pe;							/*	List of particular PEs */
	struct blalloc_context *heap_context;	/* Heap manager context */
	uint32_t dmem_heap_base;				/* DMEM base address of the heap */
	oal_mutex_t mutex;
	oal_thread_t *error_poller;
	bool_t poll_fw_errors;
};

static errno_t pfe_class_dmem_heap_init(pfe_class_t *class);

/**
 * @brief Periodically checks all PEs whether they report a firmware error
 * @details Function is intended to be run in a thread
 * @param[in] arg Classifier to check
 * @return Function never returns
 */
static void *class_fw_err_poller_func(void *arg)
{
	pfe_class_t *class = (pfe_class_t *)arg;
	uint32_t i;

	if(NULL == class)
	{
		NXP_LOG_ERROR("NULL argument\n");
		return NULL;
	}

	while (TRUE == class->poll_fw_errors)
	{

		/* Read the error record from each PE */
		for(i = 0U; i < class->pe_num; i++)
		{
			pfe_pe_get_fw_errors(class->pe[i]);		
		}

		/*  Wait for 1 sec and loop again */
		oal_time_mdelay(1000);
	}

	return NULL;

}


/**
 * @brief		Create new classifier instance
 * @details		Creates and initializes classifier instance. After successful
 * 				call the classifier is configured and disabled.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	pe_num Number of PEs to be included
 * @param[in]	cfg The classifier block configuration
 * @return		The classifier instance or NULL if failed
 */
pfe_class_t *pfe_class_create(void *cbus_base_va, uint32_t pe_num, pfe_class_cfg_t *cfg)
{
	pfe_class_t *class;
	pfe_pe_t *pe;
	uint32_t ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	class = oal_mm_malloc(sizeof(pfe_class_t));

	if (NULL == class)
	{
		return NULL;
	}
	else
	{
		memset(class, 0, sizeof(pfe_class_t));
		class->cbus_base_va = cbus_base_va;
	}

	if (pe_num > 0U)
	{
		class->pe = oal_mm_malloc(pe_num * sizeof(pfe_pe_t *));

		if (NULL == class->pe)
		{
			oal_mm_free(class);
			return NULL;
		}

		/*	Create PEs */
		for (ii=0U; ii<pe_num; ii++)
		{
			pe = pfe_pe_create(cbus_base_va, PE_TYPE_CLASS, ii);

			if (NULL == pe)
			{
				goto free_and_fail;
			}
			else
			{
				pfe_pe_set_dmem(pe, PFE_CFG_CLASS_ELF_DMEM_BASE, PFE_CFG_CLASS_DMEM_SIZE);
				pfe_pe_set_imem(pe, PFE_CFG_CLASS_ELF_IMEM_BASE, PFE_CFG_CLASS_IMEM_SIZE);
				pfe_pe_set_lmem(pe, (PFE_CFG_CBUS_PHYS_BASE_ADDR + PFE_MMAP_PE_LMEM_BASE), PFE_MMAP_PE_LMEM_SIZE);
				/* pfe_pe_set_ddr(pe, cfg->ddr_base_pa, cfg->ddr_base_va, cfg->ddr_size); */
				pfe_pe_set_iaccess(pe, CLASS_MEM_ACCESS_WDATA, CLASS_MEM_ACCESS_RDATA, CLASS_MEM_ACCESS_ADDR);

				/*	Initialize DMEM including packet buffer */
#if !defined(GLOBAL_CFG_RUN_ON_VDK)
				NXP_LOG_DEBUG("CLASS PE %d: Initializing DMEM (%lu bytes)+IMEM(%lu bytes)\n", ii, PFE_CFG_CLASS_DMEM_SIZE, PFE_CFG_CLASS_IMEM_SIZE);
				pfe_pe_dmem_memset(pe, 0U, 0U, PFE_CFG_CLASS_DMEM_SIZE);
				pfe_pe_imem_memset(pe, 0U, 0U, PFE_CFG_CLASS_IMEM_SIZE);
#else
				/* PFE TLM WORKAROUND:
				 * Resetting DMEM/IMEM areas affects the TLM model simulation.
				 * So, as workaround, skip it.
				 */
				NXP_LOG_INFO("[RUN_ON_VDK]: TLM WORKAROUND: SKIPPED class PE %d: Initializing DMEM (%lu bytes)+IMEM(%lu bytes)\n", ii, PFE_CFG_CLASS_DMEM_SIZE, PFE_CFG_CLASS_IMEM_SIZE);
#endif /* !GLOBAL_CFG_RUN_ON_VDK */

				class->pe[ii] = pe;
				class->pe_num++;
			}
		}

		if (EOK != oal_mutex_init(&class->mutex))
		{
			goto free_and_fail;
		}

		/*	Issue block reset */
		pfe_class_reset(class);

		/* After soft reset, need to wait for 10us to perform another CSR write/read */
		oal_time_usleep(10);

		/*	Disable the classifier */
		pfe_class_disable(class);

		class->poll_fw_errors = TRUE;
		class->error_poller = oal_thread_create(&class_fw_err_poller_func, class, "FW errors poller", 0);
		if (NULL == class->error_poller)
		{
			NXP_LOG_ERROR("Couldn't start poller thread\n");
		}

		/*	Set new configuration */
		pfe_class_cfg_set_config(class->cbus_base_va, cfg);
	}

	return class;

free_and_fail:
	pfe_class_destroy(class);
	class = NULL;

	return NULL;
}

/**
 * @brief		Initializes the DMEM heap manager
 * @param[in]	class The classifier instance
 */
static errno_t pfe_class_dmem_heap_init(pfe_class_t *class)
{
	pfe_ct_pe_mmap_t mmap;
	errno_t ret = EOK;

	/* All PEs share the same memory map therefore we can read
	   arbitrary one (in this case 0U) */
	ret = pfe_pe_get_mmap(class->pe[0U], &mmap);
	if(EOK == ret)
	{
		class->heap_context = blalloc_create(oal_ntohl(mmap.dmem_heap_size), PFE_CLASS_HEAP_CHUNK_SIZE);
		if(NULL == class->heap_context)
		{
			ret = ENOMEM;
		}
		else
		{
			class->dmem_heap_base = oal_ntohl(mmap.dmem_heap_base);
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Allocates memory from the DMEM heap
 * @param[in]	class The classifier instance
 * @param[in]	size Requested memory size
 * @return		Address of the allocated memory or value 0 on failure.
 */
addr_t pfe_class_dmem_heap_alloc(pfe_class_t *class, uint32_t size)
{
	addr_t addr;
	errno_t ret;

	ret = blalloc_alloc_offs(class->heap_context, size, 0, &addr);
	if(EOK == ret)
	{
		return addr + class->dmem_heap_base;
	}
	else
	{   /* Allocation failed - return "NULL" */
		NXP_LOG_DEBUG("Failed to allocate memory (size %u)\n", size);
		return 0U;
	}
}

/**
 * @brief		Returns previously allocated memory to the DMEM heap
 * @param[in]	class The classifier instance
 * @param[in]	addr Address of the previously allocated memory by pfe_class_dmem_heap_alloc()
 */
void pfe_class_dmem_heap_free(pfe_class_t *class, addr_t addr)
{
	if(0U == addr)
	{   /* Ignore "NULL" */
		return;
	}

	if(addr < class->dmem_heap_base)
	{
		NXP_LOG_ERROR("Impossible address 0x%x (base is 0x%x)\n", addr, class->dmem_heap_base);
		return;
	}

	blalloc_free_offs(class->heap_context, addr - class->dmem_heap_base);
}

/**
 * @brief		Reset the classifier block
 * @param[in]	class The classifier instance
 */
void pfe_class_reset(pfe_class_t *class)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pfe_class_disable(class);

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_class_cfg_reset(class->cbus_base_va);
	class->enabled = FALSE;

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
}

/**
 * @brief		Enable the classifier block
 * @details		Enable all classifier PEs
 * @param[in]	class The classifier instance
 */
void pfe_class_enable(pfe_class_t *class)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (unlikely(FALSE == class->is_fw_loaded))
	{
		NXP_LOG_WARNING("Attempt to enable classifier without previous firmware upload\n");
	}

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_class_cfg_enable(class->cbus_base_va);
	class->enabled = TRUE;

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
}

/**
 * @brief		Disable the classifier block
 * @details		Disable all classifier PEs
 * @param[in]	class The classifier instance
 */
void pfe_class_disable(pfe_class_t *class)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_class_cfg_disable(class->cbus_base_va);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
}

/**
 * @brief		Load firmware elf into PEs memories
 * @param[in]	class The classifier instance
 * @param[in]	elf The elf file object to be uploaded
 * @return		EOK when success or error code otherwise
 */
errno_t pfe_class_load_firmware(pfe_class_t *class, const void *elf)
{
	uint32_t ii;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	for (ii=0U; ii<class->pe_num; ii++)
	{
		ret = pfe_pe_load_firmware(class->pe[ii], elf);

		if (EOK != ret)
		{
			NXP_LOG_ERROR("Classifier firmware loading the PE %u failed: %d\n", ii, ret);
			break;
		}
	}

	if(EOK == ret)
	{
		class->is_fw_loaded = TRUE;

		/* Check the memory map whether it is correct */
		/* All PEs have the same map therefore it is sufficient to check one */
		ret = pfe_pe_check_mmap(class->pe[0U]);

		if(EOK == ret)
		{
			/* Firmware has been loaded and the DMEM heap is known - initialize the allocator */
			ret = pfe_class_dmem_heap_init(class);
			if(EOK != ret)
			{
				NXP_LOG_ERROR("Dmem heap allocator initialization failed\n");
			}
		}
	}

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get pointer to PE's memory where memory map data is stored
 * @param[in]	class The classifier instance
 * @param[in]	pe_idx PE index
 * @param[out]	mmap Pointer where memory map data shall be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_class_get_mmap(pfe_class_t *class, int32_t pe_idx, pfe_ct_pe_mmap_t *mmap)
{
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == mmap)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (pe_idx >= (int32_t)class->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_pe_get_mmap(class->pe[pe_idx], mmap);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Write data from host memory to DMEM
 * @param[in]	class The classifier instance
 * @param[in]	pe_idx PE index or -1 if all PEs shall be written
 * @param[in]	dst Destination address within DMEM (physical)
 * @param[in]	src Source address within host memory (virtual)
 * @param[in]	len Number of bytes to be written
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_write_dmem(pfe_class_t *class, int32_t pe_idx, void *dst, void *src, uint32_t len)
{
	uint32_t ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (pe_idx >= (int32_t)class->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	if (pe_idx >= 0)
	{
		/*	Single PE */
		pfe_pe_memcpy_from_host_to_dmem_32(class->pe[pe_idx], (addr_t)dst, src, len);
	}
	else
	{
		/*	All PEs */
		for (ii=0U; ii<class->pe_num; ii++)
		{
			pfe_pe_memcpy_from_host_to_dmem_32(class->pe[ii], (addr_t)dst, src, len);
		}
	}

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Read data from DMEM to host memory
 * @param[in]	class The classifier instance
 * @param[in]	pe_idx PE index
 * @param[in]	dst Destination address within host memory (virtual)
 * @param[in]	src Source address within DMEM (physical)
 * @param[in]	len Number of bytes to be read
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_read_dmem(pfe_class_t *class, uint32_t pe_idx, void *dst, void *src, uint32_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (pe_idx >= class->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_pe_memcpy_from_dmem_to_host_32(class->pe[pe_idx], dst, (addr_t)src, len);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Read data from PMEM to host memory
 * @param[in]	class The classifier instance
 * @param[in]	pe_idx PE index
 * @param[in]	dst Destination address within host memory (virtual)
 * @param[in]	src Source address within PMEM (physical)
 * @param[in]	len Number of bytes to be read
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_read_pmem(pfe_class_t *class, uint32_t pe_idx, void *dst, void *src, uint32_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (pe_idx >= class->pe_num)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	pfe_pe_memcpy_from_imem_to_host_32(class->pe[pe_idx], dst, (addr_t)src, len);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Destroy classifier instance
 * @param[in]	class The classifier instance
 */
void pfe_class_destroy(pfe_class_t *class)
{
	uint32_t ii;

	if (NULL != class)
	{
#if (PFE_CFG_FIRMWARE_VARIANT == PFE_FW_SBL)
		if (EOK != oal_mutex_lock(&class->mutex))
		{
			NXP_LOG_DEBUG("mutex lock failed\n");
		}

		class->poll_fw_errors = FALSE;
		
		for (ii=0U; ii<class->pe_num; ii++)
		{
			/*	Inform PE to stop */
			pfe_pe_dmem_write(class->pe[ii], cpu_to_be32(1U), PEMBOX_ADDR_CLASS, 4U);
			oal_time_usleep(10);

			/*	Read the 'stopped' status */
			if (0x0U == pfe_pe_dmem_read(class->pe[ii], PEMBOX_ADDR_CLASS+4, 4U))
			{
				NXP_LOG_WARNING("Failed to stop CLASS PE %d\n", ii);
			}
			else
			{
				NXP_LOG_INFO("CLASS PE %d stopped\n", ii);
			}
		}

		if (EOK != oal_mutex_unlock(&class->mutex))
		{
			NXP_LOG_DEBUG("mutex unlock failed\n");
		}
#endif /* PFE_FW_SBL */

		pfe_class_disable(class);

		for (ii=0U; ii<class->pe_num; ii++)
		{
			pfe_pe_destroy(class->pe[ii]);
			class->pe[ii] = NULL;
		}

		class->pe_num = 0U;
		if(NULL != class->heap_context)
		{
			blalloc_destroy(class->heap_context);
			class->heap_context = NULL;
		}
		oal_mutex_destroy(&class->mutex);

		oal_mm_free(class);
	}
}

/**
 * @brief		Set routing table parameters
 * @param[in]	class The classifier instance
 * @param[in]	rtable_pa Physical address of the routing table
 * @param[in]	rtable_len Number of entries in the table
 * @param[in]	entry_size Routing table entry size in number of bytes
 * @return		EOK if success, error code otherwise
 * @note		Must be called before the classifier is enabled.
 */
errno_t pfe_class_set_rtable(pfe_class_t *class, void *rtable_pa, uint32_t rtable_len, uint32_t entry_size)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == rtable_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (class->enabled)
	{
		return EBUSY;
	}
	else
	{
		if (EOK != oal_mutex_lock(&class->mutex))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		pfe_class_cfg_set_rtable(class->cbus_base_va, rtable_pa, rtable_len, entry_size);

		if (EOK != oal_mutex_unlock(&class->mutex))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return EOK;
}

/**
 * @brief		Set default VLAN ID
 * @details		Every packet without VLAN tag set received via physical interface will
 * 				be treated as packet with VLAN equal to this default VLAN ID.
 * @param[in]	class The classifier instance
 * @param[in]	vlan The default VLAN ID (12bit)
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_class_set_default_vlan(pfe_class_t *class, uint16_t vlan)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pfe_class_cfg_set_def_vlan(class->cbus_base_va, vlan);
	return EOK;
}

/**
 * @brief		Returns number of PEs available
 * @param[in]	class The classifier instance
 * @return		Number of available PEs
 */

uint32_t pfe_class_get_num_of_pes(pfe_class_t *class)
{
	return class->pe_num;
}

/**
 * @brief		Return CLASS runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	class 		The CLASS instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_class_get_text_statistics(pfe_class_t *class, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */


	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	len += pfe_class_cfg_get_text_stat(class->cbus_base_va, buf, buf_len, verb_level);

	/*	Get PE info per PE */
	for (ii=0U; ii<class->pe_num; ii++)
	{
		len += pfe_pe_get_text_statistics(class->pe[ii], buf + len, buf_len - len, verb_level);
	}

	len += oal_util_snprintf(buf + len, buf_len - len, "DMEM heap\n---------\n");
	len += blalloc_get_text_statistics(class->heap_context, buf + len, buf_len - len, verb_level);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return len;
}

/** @}*/
