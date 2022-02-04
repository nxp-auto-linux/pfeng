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
#include "pfe_class.h"
#include "pfe_class_csr.h"
#include "blalloc.h"
#include "fci.h"
#include "fpp.h"
#include "fpp_ext.h"

/* Configures size of the dmem heap allocator chunk (the smallest allocated memory size)
* The size is actually 2^configured value: 1 means 2, 2 means 4, 3 means 8, 4 means 16 etc.
* Do not configure less than 8 bytes (value 3) to avoid alignment problems when allocating structures
* containing uint64_t.
*/
#define PFE_CLASS_HEAP_CHUNK_SIZE 4


struct pfe_classifier_tag
{
	bool_t is_fw_loaded;					/*	Flag indicating that firmware has been loaded */
	bool_t enabled;							/*	Flag indicating that classifier has been enabled */
	addr_t cbus_base_va;						/*	CBUS base virtual address */
	uint32_t pe_num;						/*	Number of PEs */
	pfe_pe_t **pe;							/*	List of particular PEs */
	blalloc_t *heap_context;				/* Heap manager context */
	uint32_t dmem_heap_base;				/* DMEM base address of the heap */
	oal_mutex_t mutex;
    uint32_t current_feature;               /* Index of the feature to return by pfe_class_get_feature_next() */
    pfe_fw_feature_t **fw_features;          /* List of all features*/
    uint32_t fw_features_count;             /* Number of items in fw_features */
};

static errno_t pfe_class_dmem_heap_init(pfe_class_t *class);
static errno_t pfe_class_load_fw_features(pfe_class_t *class);
static void pfe_class_alg_stats_endian(pfe_ct_class_algo_stats_t *stat);
static void pfe_class_pe_stats_endian(pfe_ct_pe_stats_t *stat);
static void pfe_class_sum_pe_algo_stats(pfe_ct_class_algo_stats_t *sum, const pfe_ct_class_algo_stats_t *val);
static uint32_t pfe_class_stat_to_str(const pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level);

/**
 * @brief CLASS ISR
 * @details Checks all PEs whether they report a firmware error
 * @param[in] class The CLASS instance
 */
errno_t pfe_class_isr(const pfe_class_t *class)
{
	uint32_t i;
#ifdef PFE_CFG_FCI_ENABLE
	pfe_ct_buffer_t buf;
	fci_msg_t msg;
#endif /* PFE_CFG_FCI_ENABLE */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == class)
	{
		NXP_LOG_ERROR("NULL argument\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (i=0U; i<class->pe_num; i++)
	{
		/*	Allow safe use of _nolock() functions. We don't call the _mem_lock()
			here as we don't need to have coherent accesses. */
		if (EOK != pfe_pe_lock(class->pe[i]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}

		/*	Read and print the error record from each PE */
		(void)pfe_pe_get_fw_errors_nolock(class->pe[i]);

#ifdef PFE_CFG_FCI_ENABLE
		/*	Check if there is new message */
		if (EOK == pfe_pe_get_data_nolock(class->pe[i], &buf))
		{
			/*	Provide data to user via FCI */
			msg.msg_cmd.code = FPP_CMD_DATA_BUF_AVAIL;
			msg.msg_cmd.length = buf.len;

			if (msg.msg_cmd.length > (uint32_t)sizeof(msg.msg_cmd.payload))
			{
				NXP_LOG_ERROR("FCI buffer is too small\n");
			}
			else
			{
				(void)memcpy(&msg.msg_cmd.payload, buf.payload, buf.len);
				if (EOK != fci_core_client_send_broadcast(&msg, NULL))
				{
					NXP_LOG_ERROR("Can't report data to FCI clients\n");
				}
			}
		}
#endif /* PFE_CFG_FCI_ENABLE */

		if (EOK != pfe_pe_unlock(class->pe[i]))
		{
			NXP_LOG_DEBUG("pfe_pe_unlock() failed\n");
		}
	}

	return EOK;
}

/**
 * @brief		Mask CLASS interrupts
 * @param[in]	class The CLASS instance
 */
void pfe_class_irq_mask(const pfe_class_t *class)
{
	/*	Intentionally empty */
	(void)class;
}

/**
 * @brief		Unmask CLASS interrupts
 * @param[in]	class The CLASS instance
 */
void pfe_class_irq_unmask(const pfe_class_t *class)
{
	/*	Intentionally empty */
	(void)class;
}

/**
 * @brief		Create new classifier instance
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	pe_num Number of PEs to be included
 * @param[in]	cfg The classifier block configuration
 * @return		The classifier instance or NULL if failed
 */
pfe_class_t *pfe_class_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_class_cfg_t *cfg)
{
	pfe_class_t *class;
	pfe_pe_t *pe;
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == cbus_base_va) || (NULL == cfg)))
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
		(void)memset(class, 0, sizeof(pfe_class_t));
		class->cbus_base_va = cbus_base_va;
	}

	if (EOK != oal_mutex_init(&class->mutex))
	{
		oal_mm_free(class);
		return NULL;
	}

	if (pe_num > 0U)
	{
		class->pe = oal_mm_malloc(pe_num * sizeof(pfe_pe_t *));

		if (NULL == class->pe)
		{
			(void)oal_mutex_destroy(&class->mutex);
			oal_mm_free(class);
			return NULL;
		}

		/*	Create PEs */
		for (ii=0U; ii<pe_num; ii++)
		{
			pe = pfe_pe_create(cbus_base_va, PE_TYPE_CLASS, (uint8_t)ii);

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

		class->heap_context = blalloc_create(oal_ntohl(mmap.class_pe.dmem_heap_size), PFE_CLASS_HEAP_CHUNK_SIZE);
		if(NULL == class->heap_context)
		{
			ret = ENOMEM;
		}
		else
		{
			class->dmem_heap_base = oal_ntohl(mmap.class_pe.dmem_heap_base);
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
addr_t pfe_class_dmem_heap_alloc(const pfe_class_t *class, uint32_t size)
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
		NXP_LOG_DEBUG("Failed to allocate memory (size %u)\n", (uint_t)size);
		return 0U;
	}
}

/**
 * @brief		Returns previously allocated memory to the DMEM heap
 * @param[in]	class The classifier instance
 * @param[in]	addr Address of the previously allocated memory by pfe_class_dmem_heap_alloc()
 */
void pfe_class_dmem_heap_free(const pfe_class_t *class, addr_t addr)
{
	if(0U == addr)
	{   /* Ignore "NULL" */
		return;
	}

	if(addr < class->dmem_heap_base)
	{
		NXP_LOG_ERROR("Impossible address 0x%"PRINTADDR_T" (base is 0x%x)\n", addr, (uint_t)class->dmem_heap_base);
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
	uint16_t timeout = 50U;
	pfe_ct_pe_sw_state_t state;

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

	do
	{
		oal_time_usleep(5U);
		timeout--;
		state = pfe_pe_get_fw_state(class->pe[0U]);
	}
	while ((state < PFE_FW_STATE_INIT) && (timeout > 0U));

	if (timeout == 0U)
	{
		NXP_LOG_ERROR("Time-out waiting for classifier to init\n");
	}
	else
	{
		class->enabled = TRUE;
	}

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

	ret = pfe_pe_load_firmware(class->pe, class->pe_num, elf);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Classifier firmware loading the PE failed: %d\n", ret);
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

        ret = pfe_class_load_fw_features(class);
        if(EOK != ret)
        {
            NXP_LOG_ERROR("Failed to initialize FW features\n");
        }

	}

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

static errno_t pfe_class_load_fw_features(pfe_class_t *class)
{
    pfe_ct_pe_mmap_t mmap;
	errno_t ret = EOK;
    pfe_ct_feature_desc_t *entry;
    uint32_t i, j;
    bool_t val_break = FALSE;

	ret = pfe_pe_get_mmap(class->pe[0U], &mmap);
	if(EOK == ret)
	{
        class->fw_features_count = oal_ntohl(mmap.common.version.features_count);
        class->fw_features = NULL;
        if(class->fw_features_count > 0U)
        {
            class->fw_features = oal_mm_malloc(class->fw_features_count * sizeof(pfe_fw_feature_t *));
            if(NULL == class->fw_features)
            {
                class->fw_features_count = 0U;
                NXP_LOG_ERROR("Failed to allocate features storage\n");
                ret = ENOMEM;
            }
            else
            {
                /* Initialize current_feature */
                class->current_feature = 0U;
                for(i = 0U; i < class->fw_features_count; i++)
                {
                    class->fw_features[i] = pfe_fw_feature_create();
                    if(NULL == class->fw_features[i])
                    {
                        NXP_LOG_ERROR("Failed to create feature %u\n", (uint_t)i);
                        /* Destroy previously created and return failure */
                        for(j = 0U; j < i; j++)
                        {
                            pfe_fw_feature_destroy(class->fw_features[j]);
                            class->fw_features[j] = NULL;
                        }
                        oal_mm_free(class->fw_features);
                        class->fw_features = NULL;
                        class->fw_features_count = 0U;
                        ret = ENOMEM;
                        val_break = TRUE;
                    }
                    else
                    {
                        /* Get feature low level data */
                        ret = pfe_pe_get_fw_feature_entry(class->pe[0U], i, &entry);
                        if(EOK != ret)
                        {
                             NXP_LOG_ERROR("Failed get ll data for feature %u\n", (uint_t)i);
                            /* Destroy previously created and return failure */
                            for(j = 0U; j < i; j++)
                            {
                                pfe_fw_feature_destroy(class->fw_features[j]);
                                class->fw_features[j] = NULL;
                            }
                            oal_mm_free(class->fw_features);
                            class->fw_features = NULL;
                            class->fw_features_count = 0U;
                            ret = EINVAL;
                            val_break = TRUE;
                        }
                        else
                        {
                            /* Set the low level data in the feature */
                            (void)pfe_fw_feature_set_ll_data(class->fw_features[i], entry);
                            /* Set the feature string base */
                            ret = pfe_fw_feature_set_string_base(class->fw_features[i], pfe_pe_get_fw_feature_str_base(class->pe[0U]));
                            if(EOK != ret)
                            {
                                NXP_LOG_ERROR("Failed to set string base for feature %u\n", (uint_t)i);
                                /* Destroy previously created and return failure */
                                for(j = 0U; j < i; j++)
                                {
                                    pfe_fw_feature_destroy(class->fw_features[j]);
                                    class->fw_features[j] = NULL;
                                }
                                oal_mm_free(class->fw_features);
                                class->fw_features = NULL;
                                class->fw_features_count = 0U;
                                ret = EINVAL;
                                val_break = TRUE;
                            }
                            else
                            {
                                /* Set functions to read/write DMEM and their data */
                                (void)pfe_fw_feature_set_dmem_funcs(class->fw_features[i], pfe_class_read_dmem, pfe_class_write_dmem, (void *)class);
                            }
                        }
                    }
                    if (TRUE == val_break)
                    {
                        break;
                    }
                }
            }
        } /* Else is OK too */
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
errno_t pfe_class_get_mmap(pfe_class_t *class, int32_t pe_idx, pfe_ct_class_mmap_t *mmap)
{
	errno_t ret;
	pfe_ct_pe_mmap_t mmap_tmp;

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

	ret = pfe_pe_get_mmap(class->pe[pe_idx], &mmap_tmp);
	(void)memcpy(mmap, &mmap_tmp.class_pe, sizeof(pfe_ct_class_mmap_t));

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Write data from host memory to DMEM
 * @param[in]	class_p The classifier instance
 * @param[in]	pe_idx PE index or -1 if all PEs shall be written
 * @param[in]	dst_addr Destination address within DMEM (physical)
 * @param[in]	src_ptr Pointer to data in host memory (virtual address)
 * @param[in]	len Number of bytes to be written
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_write_dmem(void *class_p, int32_t pe_idx, addr_t dst_addr, const void *src_ptr, uint32_t len)
{
	uint32_t ii;
    pfe_class_t *class = (pfe_class_t *)class_p;

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
		pfe_pe_memcpy_from_host_to_dmem_32(class->pe[pe_idx], dst_addr, src_ptr, len);
	}
	else
	{
		/*	All PEs */
		for (ii=0U; ii<class->pe_num; ii++)
		{
			pfe_pe_memcpy_from_host_to_dmem_32(class->pe[ii], dst_addr, src_ptr, len);
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
 * @param[in]	class_p The classifier instance
 * @param[in]	pe_idx PE index
 * @param[in]	dst_ptr Destination address within host memory (virtual)
 * @param[in]	src_addr Source address within DMEM (physical)
 * @param[in]	len Number of bytes to be read
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_read_dmem(void *class_p, int32_t pe_idx, void *dst_ptr, addr_t src_addr, uint32_t len)
{
    pfe_class_t *class = (pfe_class_t *)class_p;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst_ptr)))
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

	pfe_pe_memcpy_from_dmem_to_host_32(class->pe[pe_idx], dst_ptr, src_addr, len);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Read data from DMEM from all PEs atomically to host memory
 * @param[in]	class The classifier instance (All PEs from given class are read)
 * @param[in]	dst_ptr Destination address within host memory (virtual) available memory has to be pe_count * len
 * @param[in]	src_addr Source address within DMEM (physical)
 * @param[in]	buffer_len Destination buffer size
 * @param[in]	read_len Number of bytes to be read (From one PE)
 * @return		EOK or error code in case of failure
 */
errno_t pfe_class_gather_read_dmem(pfe_class_t *class, void *dst_ptr, addr_t src_addr, uint32_t buffer_len, uint32_t read_len)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == dst_ptr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_pe_gather_memcpy_from_dmem_to_host_32(class->pe, (int32_t)class->pe_num, dst_ptr, (addr_t)src_addr, buffer_len, read_len);

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
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

		pfe_pe_destroy(class->pe, class->pe_num);

		if (NULL != class->pe)
		{
			oal_mm_free(class->pe);
		}

		if(NULL != class->fw_features)
		{
			for(ii = 0U; ii < class->fw_features_count; ii++)
			{
				if(NULL != class->fw_features[ii])
				{
					pfe_fw_feature_destroy(class->fw_features[ii]);
					class->fw_features[ii] = NULL;
				}
			}
			oal_mm_free(class->fw_features);
			class->fw_features = NULL;
			class->fw_features_count = 0U;
		}

		class->pe_num = 0U;
		if (NULL != class->heap_context)
		{
			blalloc_destroy(class->heap_context);
			class->heap_context = NULL;
		}

		if (EOK != oal_mutex_destroy(&class->mutex))
		{
			NXP_LOG_WARNING("Could not properly destroy mutex\n");
		}

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
errno_t pfe_class_set_rtable(pfe_class_t *class, addr_t rtable_pa, uint32_t rtable_len, uint32_t entry_size)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL_ADDR == rtable_pa)))
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

		ret = pfe_class_cfg_set_rtable(class->cbus_base_va, rtable_pa, rtable_len, entry_size);

		if (EOK != oal_mutex_unlock(&class->mutex))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Set default VLAN ID
 * @details		Every packet without VLAN tag set received via physical interface will
 * 				be treated as packet with VLAN equal to this default VLAN ID.
 * @param[in]	class The classifier instance
 * @param[in]	vlan The default VLAN ID (12bit)
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_class_set_default_vlan(const pfe_class_t *class, uint16_t vlan)
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

uint32_t pfe_class_get_num_of_pes(const pfe_class_t *class)
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
 * @brief Finds and returns Classifier FW feature by its name
 * @param[in] class The classifier instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @param[in] name Name of the feature to be found
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_class_get_feature(const pfe_class_t *class, pfe_fw_feature_t **feature, const char *name)
{
    uint32_t i;
    const char *fname;
    errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class)||(NULL == feature)||(NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
    for(i = 0U; i < class->fw_features_count; i++)
    {
        ret = pfe_fw_feature_get_name(class->fw_features[i], &fname);
        if(ret == EOK)
        {
            if(0 == strcmp(fname, name))
            {
                *feature = class->fw_features[i];
                return EOK;
            }
        }
    }
    return ENOENT;
}

/**
 * @brief Finds and returns the 1st Classifier FW feature by order of their discovery - used for listing all features
 * @param[in] class The classifier instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_class_get_feature_first(pfe_class_t *class, pfe_fw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class)||(NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
    if(class->fw_features_count > 0U)
    {
        class->current_feature = 0U;
        *feature = class->fw_features[class->current_feature];
        return EOK;
    }

    return ENOENT;
}

/**
 * @brief Finds and returns the next Classifier FW feature by order of their discovery - used for listing all features
 * @param[in] class The classifier instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
errno_t pfe_class_get_feature_next(pfe_class_t *class, pfe_fw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class)||(NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
    if(class->fw_features_count > 0U)
    {
        /* Avoid going out of the array boundaries */
        if((class->current_feature + 1U) < class->fw_features_count)
        {
            class->current_feature += 1U;
            *feature = class->fw_features[class->current_feature];
            return EOK;
        }
    }

    return ENOENT;
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

void pfe_class_flexi_parser_stats_endian(pfe_ct_class_flexi_parser_stats_t *stats)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == stats))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif
	stats->accepted = oal_ntohl(stats->accepted);
	stats->rejected = oal_ntohl(stats->rejected);
}

void pfe_class_sum_flexi_parser_stats(pfe_ct_class_flexi_parser_stats_t *sum, const pfe_ct_class_flexi_parser_stats_t *val)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == sum) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif
	sum->accepted += val->accepted;
	sum->rejected += val->rejected;
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
	for(i = 0U; i < ((uint32_t)PFE_PHY_IF_ID_MAX + 1U); i++)
	{
		stat->replicas[i] = oal_ntohl(stat->replicas[i]);
	}
}

/**
* @brief Function adds statistics value to sum
* @param[in] sum Sum to add the value (results are in HOST endian)
* @param[in] val Value to be added to the sum (it is in HOST endian)
*/
static void pfe_class_sum_pe_algo_stats(pfe_ct_class_algo_stats_t *sum, const pfe_ct_class_algo_stats_t *val)
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
static uint32_t pfe_class_stat_to_str(const pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
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
 * @brief               Converts statistics of a logical interface or classification algorithm into a text form
 * @param[in]   stat            Statistics to convert - expected in HOST endian
 * @param[out]  buf                     Buffer where to write the text
 * @param[in]   buf_len         Buffer length
 * @param[in]   verb_level      Verbosity level
 * @return              Number of bytes written into the output buffer
 */
uint32_t pfe_class_fp_stat_to_str(const pfe_ct_class_flexi_parser_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
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
        len += oal_util_snprintf(buf + len, buf_len - len, "Frames accepted:  %u\n", stat->accepted);
        len += oal_util_snprintf(buf + len, buf_len - len, "Frames rejected:  %u\n", stat->rejected);
        return len;
}

/**
 * @brief		Send data buffer
 * @param[in]	class The CLASS instance
 * @param[in]	buf Buffer to be sent
 * @return		EOK success, error code otherwise
 */
errno_t pfe_class_put_data(const pfe_class_t *class, pfe_ct_buffer_t *buf)
{
	uint32_t ii, tries;
	errno_t ret;

	for (ii=0U; ii<class->pe_num; ii++)
	{
		/*	Allow safe use of _nolock() functions. We don't call the _mem_lock()
		 	here as we don't need to have coherent accesses. */
		if (EOK != pfe_pe_lock(class->pe[ii]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}

		tries = 0U;
		do
		{
			ret = pfe_pe_put_data_nolock(class->pe[ii], buf);
			if (EAGAIN == ret)
			{
				tries++;
				oal_time_usleep(200U);
			}
		} while ((ret == EAGAIN) && (tries < 10U));

		if (EOK != pfe_pe_unlock(class->pe[ii]))
		{
			NXP_LOG_DEBUG("pfe_pe_lock() failed\n");
		}

		if (EOK != ret)
		{
			NXP_LOG_ERROR("Unable to update pe %u\n", (uint_t)ii);
			return EBUSY;
		}
	}

	return EOK;
}

/**
 * @brief		Get class algo statistics
 * @param[in]	class
 * @param[out]	stat Statistic structure
 * @retval		EOK Success
 * @retval		NOMEM Not possible to allocate memory for read
 */
errno_t pfe_class_get_stats(pfe_class_t *class, pfe_ct_classify_stats_t *stat)
{
	pfe_ct_pe_mmap_t mmap;
	uint32_t i = 0U;
	errno_t ret = EOK;
	uint32_t buff_len = 0;
	pfe_ct_classify_stats_t * stats = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == stat)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)memset(stat,0,sizeof(pfe_ct_classify_stats_t));

	/* Prepare memory */
	buff_len = sizeof(pfe_ct_classify_stats_t) * pfe_class_get_num_of_pes(class);
	stats = oal_mm_malloc(buff_len);
	if(NULL == stats)
	{
		return ENOMEM;
	}

	/* Get the memory map - all PEs share the same memory map
	   therefore we can read arbitrary one (in this case 0U) */
	ret = pfe_pe_get_mmap(class->pe[0U], &mmap);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Cannot get PE memory map\n");
		oal_mm_free(stats);
		return ret;
	}

	/* Gather memory from all PEs*/
	ret = pfe_class_gather_read_dmem(class, stats, oal_ntohl(mmap.class_pe.classification_stats), buff_len, sizeof(pfe_ct_classify_stats_t));

	/* Calculate total statistics */
	while(i < pfe_class_get_num_of_pes(class))
	{
		pfe_class_alg_stats_endian(&stats[i].flexible_router);
		pfe_class_alg_stats_endian(&stats[i].ip_router);
		pfe_class_alg_stats_endian(&stats[i].vlan_bridge);
		pfe_class_alg_stats_endian(&stats[i].log_if);
		pfe_class_alg_stats_endian(&stats[i].hif_to_hif);
		pfe_class_flexi_parser_stats_endian(&stats[i].flexible_filter);

		pfe_class_sum_pe_algo_stats(&stat->flexible_router, &stats[i].flexible_router);
		pfe_class_sum_pe_algo_stats(&stat->ip_router, &stats[i].ip_router);
		pfe_class_sum_pe_algo_stats(&stat->vlan_bridge, &stats[i].vlan_bridge);
		pfe_class_sum_pe_algo_stats(&stat->log_if, &stats[i].log_if);
		pfe_class_sum_pe_algo_stats(&stat->hif_to_hif, &stats[i].hif_to_hif);
		pfe_class_sum_flexi_parser_stats(&stat->flexible_filter, &stats[i].flexible_filter);
		++i;
	}

	oal_mm_free(stats);

	return ret;
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
	pfe_ct_classify_stats_t c_alg_stats;
	pfe_ct_version_t fw_ver;

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

	/* FW version */
	if (EOK == pfe_class_get_fw_version(class, &fw_ver))
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "FIRMWARE VERSION\t%u.%u.%u (api:%.32s)\n",
			fw_ver.major, fw_ver.minor, fw_ver.patch, fw_ver.cthdr);
	}
	else
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "FIRMWARE VERSION <unknown>\n");
	}

	len += pfe_class_cfg_get_text_stat(class->cbus_base_va, buf + len, buf_len - len, verb_level);

	/* Allocate memory to copy the statistics from PEs + one position for sums
	   (having sums separate from data allows to print also per PE details) */
	pe_stats = oal_mm_malloc(sizeof(pfe_ct_pe_stats_t) * (class->pe_num + 1U));
	if (NULL == pe_stats)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		if (EOK != oal_mutex_unlock(&class->mutex))
		{
			NXP_LOG_DEBUG("mutex unlock failed\n");
		}

		return len;
	}

	(void)memset(pe_stats, 0, sizeof(pfe_ct_pe_stats_t) * (class->pe_num + 1U));

	/* Get the memory map - all PEs share the same memory map
	   therefore we can read arbitrary one (in this case 0U) */
	ret = pfe_pe_get_mmap(class->pe[0U], &mmap);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Cannot get PE memory map\n");
		oal_mm_free(pe_stats);
		if (EOK != oal_mutex_unlock(&class->mutex))
		{
			NXP_LOG_DEBUG("mutex unlock failed\n");
		}

		return len;
	}

	/* Lock all PEs - they will stop processing frames and wait. This will
	   ensure data coherence. */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		ret = pfe_pe_mem_lock(class->pe[ii]);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PE %u could not be locked\n", (uint_t)ii);
			len += oal_util_snprintf(buf + len, buf_len - len,
					"PE %u could not be locked - statistics are not coherent\n", ii);
		}
	}

	/* Get PE info per PE
	   - leave 1st position in allocated memory empty for sums */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		(void)pfe_pe_get_pe_stats_nolock(
					class->pe[ii],
					oal_ntohl(mmap.class_pe.pe_stats),
					&pe_stats[ii + 1U]);
	}

	/* Unlock all PEs */
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		ret = pfe_pe_mem_unlock(class->pe[ii]);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PE %u could not be unlocked\n", (uint_t)ii);
		}
	}

	if (EOK != oal_mutex_unlock(&class->mutex))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	ret = pfe_class_get_stats(class, &c_alg_stats);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Cannot get class statistics\n");
	}

	/* Process gathered info from all PEs
	   - convert endians
	   - done separately to minimize time when PEs are locked
	   - create sums in the 1st set
	*/
	for (ii = 0U; ii < class->pe_num; ii++)
	{
		pfe_class_pe_stats_endian(&pe_stats[ii + 1U]);

		/* Calculate sums */
		pe_stats[0].processed += pe_stats[ii + 1U].processed;
		pe_stats[0].discarded += pe_stats[ii + 1U].discarded;
		pe_stats[0].injected += pe_stats[ii + 1U].injected;

		for(j = 0U; j < ((uint32_t)PFE_PHY_IF_ID_MAX + 1U); j++)
		{
			pe_stats[0].replicas[j] += pe_stats[ii + 1U].replicas[j];
		}
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

	for (j = 0U; j < ((uint32_t)PFE_PHY_IF_ID_MAX + 1U); j++)
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "Frames with %u replicas: %u\n", j + 1U, pe_stats[0].replicas[j]);
	}

	len += oal_util_snprintf(buf + len, buf_len - len, "Frames with HIF_TX_INJECT: %u\n", pe_stats[0].injected);

	len += oal_util_snprintf(buf + len, buf_len - len, "- Flexible router -\n");
	len += pfe_class_stat_to_str(&c_alg_stats.flexible_router, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- IP Router -\n");
	len += pfe_class_stat_to_str(&c_alg_stats.ip_router, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- VLAN Bridge -\n");
	len += pfe_class_stat_to_str(&c_alg_stats.vlan_bridge, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- Logical Interfaces -\n");
	len += pfe_class_stat_to_str(&c_alg_stats.log_if, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- InterHIF -\n");
	len += pfe_class_stat_to_str(&c_alg_stats.hif_to_hif, buf + len, buf_len - len, verb_level);
	len += oal_util_snprintf(buf + len, buf_len - len, "- Global Flexible filter -\n");
	len += pfe_class_fp_stat_to_str(&c_alg_stats.flexible_filter, buf + len, buf_len - len, verb_level);

	len += oal_util_snprintf(buf + len, buf_len - len, "\nDMEM heap\n---------\n");
	len += blalloc_get_text_statistics(class->heap_context, buf + len, buf_len - len, verb_level);

	/* Free allocated memory */
	oal_mm_free(pe_stats);

	return len;
}

/**
 * @brief		Returns firmware versions
 * @param[in]	class The classifier instance
 * @return		ver Parsed firmware metadata
 */
errno_t pfe_class_get_fw_version(const pfe_class_t *class, pfe_ct_version_t *ver)
{
	pfe_ct_pe_mmap_t pfe_pe_mmap;

	/*	Get mmap base from PE[0] since all PEs have the same memory map */
	if ((NULL == class->pe[0]) || (EOK != pfe_pe_get_mmap(class->pe[0], &pfe_pe_mmap)))
	{
		return EINVAL;
	}

	(void)memcpy(ver, &pfe_pe_mmap.class_pe.common.version, sizeof(pfe_ct_version_t));

	return EOK;
}

/**
* @brief 		Enable HW lookup of routing table
* @param[in] 	class The classifier instance
*/
void pfe_class_rtable_lookup_enable(const pfe_class_t *class)
{
	pfe_class_cfg_rtable_lookup_enable(class->cbus_base_va);
}

/**
* @brief 		Disable HW lookup of routing table
* @param[in] 	class The classifier instance
*/
void pfe_class_rtable_lookup_disable(const pfe_class_t *class)
{
	pfe_class_cfg_rtable_lookup_disable(class->cbus_base_va);
}

