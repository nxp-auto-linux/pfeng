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
#include "pfe_class.h"
#include "pfe_class_csr.h"
#include "blalloc.h"
#include "fci.h"
#include "fpp.h"
#include "fpp_ext.h"

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
	blalloc_t *heap_context;				/* Heap manager context */
	uint32_t dmem_heap_base;				/* DMEM base address of the heap */
	oal_mutex_t mutex;
};

static errno_t pfe_class_dmem_heap_init(pfe_class_t *class);

/**
 * @brief CLASS ISR
 * @details Checks all PEs whether they report a firmware error
 * @param[in] class The CLASS instance
 */
errno_t pfe_class_isr(pfe_class_t *class)
{
	uint32_t i;
	pfe_ct_buffer_t buf;
	fci_msg_t msg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == class)
	{
		NXP_LOG_ERROR("NULL argument\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Read the error record from each PE */
	for (i = 0U; i < class->pe_num; i++)
	{
		pfe_pe_get_fw_errors(class->pe[i]);
	}

	/*	Read data to be delivered to host */
	for (i=0U; i<class->pe_num; i++)
	{
		/*	Check if there is new message */
		if (EOK == pfe_pe_get_data(class->pe[i], &buf))
		{
#ifdef PFE_CFG_FCI_ENABLE
			/*	Provide data to user via FCI */
			msg.msg_cmd.code = FPP_CMD_DATA_BUF_AVAIL;
			msg.msg_cmd.length = buf.len;

			if (buf.len > sizeof(msg.msg_cmd.payload))
			{
				NXP_LOG_ERROR("FCI buffer is too small\n");
			}
			else
			{
				memcpy(&msg.msg_cmd.payload, buf.payload, buf.len);
				if (EOK != fci_core_client_send_broadcast(&msg, NULL))
				{
					NXP_LOG_ERROR("Can't report data to FCI clients\n");
				}
			}
#endif
		}
	}

	return EOK;
}

/**
 * @brief		Mask CLASS interrupts
 * @param[in]	class The CLASS instance
 */
void pfe_class_irq_mask(pfe_class_t *class)
{
#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
	/*	Intentionally empty */
	(void)class;
#else
	#error Not supported yet
#endif /* PFE_CFG_IP_VERSION */
}

/**
 * @brief		Unmask CLASS interrupts
 * @param[in]	class The CLASS instance
 */
void pfe_class_irq_unmask(pfe_class_t *class)
{
#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
	/*	Intentionally empty */
	(void)class;
#else
	#error Not supported yet
#endif /* PFE_CFG_IP_VERSION */
}

/**
 * @brief		Create new classifier instance
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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
				pfe_pe_set_iaccess(pe, CLASS_MEM_ACCESS_WDATA, CLASS_MEM_ACCESS_RDATA, CLASS_MEM_ACCESS_ADDR);
				pfe_pe_set_dmem(pe, PFE_CFG_CLASS_ELF_DMEM_BASE, PFE_CFG_CLASS_DMEM_SIZE);
				pfe_pe_set_imem(pe, PFE_CFG_CLASS_ELF_IMEM_BASE, PFE_CFG_CLASS_IMEM_SIZE);
				pfe_pe_set_lmem(pe, (PFE_CFG_CBUS_PHYS_BASE_ADDR + PFE_CFG_PE_LMEM_BASE), PFE_CFG_PE_LMEM_SIZE);
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
		NXP_LOG_ERROR("Impossible address 0x%"PRINTADDR_T" (base is 0x%x)\n", addr, class->dmem_heap_base);
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = EINVAL;
	for (ii=0U; ii<class->pe_num; ii++)
	{
		ret = pfe_pe_load_firmware(class->pe[ii], elf);

		if (EOK != ret)
		{
			NXP_LOG_ERROR("Classifier firmware loading the PE %u failed: %d\n", ii, ret);
			break;
		}
	}

	if (EOK == ret)
	{
		class->is_fw_loaded = TRUE;

		/* Check the memory map whether it is correct */
		/* All PEs have the same map therefore it is sufficient to check one */
		ret = pfe_pe_check_mmap(class->pe[0U]);

		if (EOK == ret)
		{
			/* Firmware has been loaded and the DMEM heap is known - initialize the allocator */
			ret = pfe_class_dmem_heap_init(class);
			if (EOK != ret)
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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == mmap)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
 * @brief		Read data from DMEM from all PEs atomically to host memory
 * @param[in]	class The classifier instance (All PEs from given class are read)
 * @param[in]	dst Destination address within host memory (virtual) available memory has to be pe_count * len
 * @param[in]	src Source address within DMEM (physical)
 * @param[in]	buffer_len Destination buffer size
 * @param[in]	read_len Number of bytes to be read (From one PE)
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_gather_read_dmem(pfe_class_t *class, void *dst, void *src, uint32_t buffer_len, uint32_t read_len)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_pe_gather_memcpy_from_dmem_to_host_32(class->pe, class->pe_num, dst, (addr_t)src, buffer_len, read_len);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
		pfe_class_disable(class);

		for (ii=0U; ii<class->pe_num; ii++)
		{
			pfe_pe_destroy(class->pe[ii]);
			class->pe[ii] = NULL;
		}

		class->pe_num = 0U;
		if (NULL != class->heap_context)
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == rtable_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return class->pe_num;
}

/**
* @brief Converts endiannes of the whole structure containing statistics
* @param[in,out] stat Statistics which endiannes shall be converted
*/
static void pfe_class_alg_stats_endian(pfe_ct_class_algo_stats_t *stat)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == stat))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif
	stat->processed = oal_ntohl(stat->processed);
	stat->accepted = oal_ntohl(stat->accepted);
	stat->rejected = oal_ntohl(stat->rejected);
	stat->discarded = oal_ntohl(stat->discarded);
}

/**
* @brief Converts endiannes of the whole structure containing statistics
* @param[in,out] stat Statistics which endiannes shall be converted
*/
static void pfe_class_pe_stats_endian(pfe_ct_pe_stats_t *stat)
{
	uint32_t i;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == stat))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif
	stat->processed = oal_ntohl(stat->processed);
	stat->discarded = oal_ntohl(stat->discarded);
	stat->injected = oal_ntohl(stat->injected);
	for(i = 0U; i < (PFE_PHY_IF_ID_MAX + 1U); i++) 
	{
		stat->replicas[i] = oal_ntohl(stat->replicas[i]);
	}
}

/**
* @brief Function adds statistics value to sum
* @param[in] sum Sum to add the value (results are in HOST endian)
* @param[in] val Value to be added to the sum (it is in HOST endian)
*/
static void pfe_class_sum_pe_algo_stats(pfe_ct_class_algo_stats_t *sum, pfe_ct_class_algo_stats_t *val)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == sum) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif
	sum->processed += val->processed;
	sum->accepted += val->accepted;
	sum->rejected += val->rejected;
	sum->discarded += val->discarded;
}

/**
 * @brief		Converts statistics of a logical interface or classification algorithm into a text form
 * @param[in]	stat		Statistics to convert - expected in HOST endian
 * @param[out]	buf			Buffer where to write the text
 * @param[in]	buf_len		Buffer length
 * @param[in]	verb_level	Verbosity level
 * @return		Number of bytes written into the output buffer
 */
uint32_t pfe_class_stat_to_str(pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
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
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames processed: %u\n", stat->processed);
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames accepted:  %u\n", stat->accepted);
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames rejected:  %u\n", stat->rejected);
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames discarded: %u\n", stat->discarded);
	return len;
}

/**
 * @brief		Send data buffer
 * @param[in]	class The CLASS instance
 * @param[in]	buf Buffer to be sent
 * @return		EOK success, error code otherwise
 */
uint32_t pfe_class_put_data(pfe_class_t *class, pfe_ct_buffer_t *buf)
{
	uint32_t ii, tries;
	errno_t ret;

	for (ii=0U; ii<class->pe_num; ii++)
	{
		do
		{
			tries = 0U;
			ret = pfe_pe_put_data(class->pe[ii], buf);
			if (EAGAIN == ret)
			{
				tries++;
				oal_time_usleep(200U);
			}
		} while ((ret == EAGAIN) && (tries < 10U));

		if (EOK != ret)
		{
			NXP_LOG_ERROR("Unable to update pe %d\n", ii);
			return EBUSY;
		}
	}

	return EOK;
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

	pfe_ct_pe_mmap_t mmap;
	errno_t ret = EOK;
	uint32_t ii, j;
	pfe_ct_pe_stats_t *pe_stats;
	pfe_ct_classify_stats_t *c_alg_stats;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	len += pfe_class_cfg_get_text_stat(class->cbus_base_va, buf, buf_len, verb_level);


	/* Allocate memory to copy the statistics from PEs + one position for sums
	   (having sums separate from data allows to print also per PE details) */
	pe_stats = oal_mm_malloc(sizeof(pfe_ct_pe_stats_t) * (class->pe_num + 1U));
	if(NULL == pe_stats)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return len;
	}
	memset(pe_stats, 0U, sizeof(pfe_ct_pe_stats_t) * (class->pe_num + 1U));
	c_alg_stats = oal_mm_malloc(sizeof(pfe_ct_classify_stats_t) * (class->pe_num + 1U));
	if(NULL == c_alg_stats)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		oal_mm_free(pe_stats);
		return len;
	}	
	memset(c_alg_stats, 0U, sizeof(pfe_ct_classify_stats_t) * (class->pe_num + 1U));
	/* Get the memory map - all PEs share the same memory map
	   therefore we can read arbitrary one (in this case 0U) */
	ret = pfe_pe_get_mmap(class->pe[0U], &mmap);
	if(EOK != ret)
	{
		NXP_LOG_ERROR("Cannot get PE memory map\n");
		oal_mm_free(c_alg_stats);
		oal_mm_free(pe_stats);
		return len;
	}
	/* Lock all PEs - they will stop processing frames and wait */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		ret = pfe_pe_mem_lock(class->pe[ii]);
		if(EOK != ret)
		{
			NXP_LOG_ERROR("PE %u could not be locked\n", ii);
			len += oal_util_snprintf(buf + len, buf_len - len, "PE %u could not be locked - statistics are not coherent\n", ii);
		}
	}
	/* Get PE info per PE 
	   - leave 1st position in allocated memory empty for sums */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		pfe_pe_get_classify_stats(class->pe[ii], oal_ntohl(mmap.classification_stats), &c_alg_stats[ii + 1U]);
		pfe_pe_get_pe_stats(class->pe[ii], oal_ntohl(mmap.pe_stats), &pe_stats[ii + 1U]);
	}
	 /* Enable all PEs */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		ret = pfe_pe_mem_unlock(class->pe[ii]);
		if(EOK != ret)
		{
			NXP_LOG_ERROR("PE %u could not be unlocked\n", ii);
		}
	}
	
	/* Process gathered info from all PEs 
	   - convert endians 
	   - done separately to minimize time when PEs are locked
	   - create sums in the 1st set 
	*/
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		/* First convert endians */
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].flexible_router);
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].ip_router);
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].l2_bridge);
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].vlan_bridge);
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].log_if);
		pfe_class_alg_stats_endian(&c_alg_stats[ii + 1U].hif_to_hif);
		pfe_class_pe_stats_endian(&pe_stats[ii + 1U]);
		/* Calculate sums */
		pe_stats[0].processed += pe_stats[ii + 1U].processed;
		pe_stats[0].discarded += pe_stats[ii + 1U].discarded;
		pe_stats[0].injected += pe_stats[ii + 1U].injected;
		for(j = 0U; j < (PFE_PHY_IF_ID_MAX + 1U); j++)
		{
			pe_stats[0].replicas[j] += pe_stats[ii + 1U].replicas[j];
		}
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].flexible_router, &c_alg_stats[ii + 1U].flexible_router);
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].ip_router, &c_alg_stats[ii + 1U].ip_router);
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].l2_bridge, &c_alg_stats[ii + 1U].l2_bridge);
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].vlan_bridge, &c_alg_stats[ii + 1U].vlan_bridge);
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].log_if, &c_alg_stats[ii + 1U].log_if);
		pfe_class_sum_pe_algo_stats(&c_alg_stats[0U].hif_to_hif, &c_alg_stats[ii + 1U].hif_to_hif);
	}
	
	/* Print results */
	len += oal_util_snprintf(buf + len, buf_len - len, "-- Per PE statistics --\n");
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "PE %u Frames processed: %u\n", ii, pe_stats[ii + 1U].processed);
		len += oal_util_snprintf(buf + len, buf_len - len, "PE %u Frames discarded: %u\n", ii, pe_stats[ii + 1U].discarded);
	}
	len += oal_util_snprintf(buf + len, buf_len - len, "-- Summary statistics --\n");
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames processed: %u\n", pe_stats[0].processed);
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames discarded: %u\n", pe_stats[0].discarded);
	for(j = 0U; j < PFE_PHY_IF_ID_MAX + 1U; j++)
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "Frames with %u replicas: %u\n", j + 1U, pe_stats[0].replicas[j]);
	}
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames with HIF_TX_INJECT: %u\n", pe_stats[0].injected);
	
	len += oal_util_snprintf(buf + len, buf_len - len, "- Flexible router -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].flexible_router, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- IP Router -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].ip_router, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- L2 Bridge -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].l2_bridge, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- VLAN Bridge -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].vlan_bridge, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- Logical Interfaces -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].log_if, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- InterHIF -\n");
	len += pfe_class_stat_to_str(&c_alg_stats[0U].hif_to_hif, buf + len, buf_len - len, verb_level);
	
	len += oal_util_snprintf(buf + len, buf_len - len, "\nDMEM heap\n---------\n");
	len += blalloc_get_text_statistics(class->heap_context, buf + len, buf_len - len, verb_level);
	/* Free allocated memory */
	oal_mm_free(c_alg_stats);
	oal_mm_free(pe_stats);
	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return len;
}
