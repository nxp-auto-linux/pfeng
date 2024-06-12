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
#include "pfe_ct.h"
#include "oal.h"
#include "pfe_class.h"
#include "pfe_fw_feature.h"

struct pfe_fw_feature_tag
{
	union {
		pfe_ct_feature_desc_t *data;
		pfe_ct_feature_desc_ext_t *data_ext;
	} ll;
	const char *string_base;
	dmem_read_func_t dmem_read_func;
	dmem_write_func_t dmem_write_func;
	void *dmem_rw_func_data;
	uint8_t current_cfg;
	uint8_t current_stats;
	uint8_t instances;
};

#define UINT_8_SIZE		1
#define UINT_16_SIZE	2
#define UINT_32_SIZE	4

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief Creates a feature instance
 * @return The created feature instance or NULL in case of failure
 */
pfe_fw_feature_t *pfe_fw_feature_create(void)
{
	pfe_fw_feature_t *feature;
	feature = oal_mm_malloc(sizeof(pfe_fw_feature_t));
	if (NULL != feature)
	{
		(void)memset(feature, 0U, sizeof(pfe_fw_feature_t));
	}
	else
	{
		NXP_LOG_ERROR("Cannot allocate %u bytes of memory for feature\n", (uint_t)sizeof(pfe_fw_feature_t));
	}
	return feature;
}

/**
 * @brief Destroys a feature instance previously created by pfe_fw_feature_create()
 * @param[in] feature Previously created feature
 */
void pfe_fw_feature_destroy(const pfe_fw_feature_t *feature)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == feature))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		oal_mm_free(feature);
	}
}

/**
 * @brief Sets reference to low level data in obtained from PE
 * @param[in] feature Feature to set the low level data
 * @param[in] ll_data Low level data to set
 * @param[in] instances of processing
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_set_ll_data(pfe_fw_feature_t *feature, pfe_ct_feature_desc_t *ll_data, uint8_t instances)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == ll_data)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		feature->ll.data = ll_data;
		feature->instances = instances;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Sets the base address for the strings
 * @param[in] feature Feature which string base address shall be set
 * @param[in] string_base String base address to be set
 * @return EOK or an error code.
 * @details All features use the same base address which is actually pointer to copy of elf-section
 *          .features loaded by PE. All strings are stored there and their addresses are stored in
 *          the low level data set by pfe_fw_feature_set_ll_data().
 */
errno_t pfe_fw_feature_set_string_base(pfe_fw_feature_t *feature, const char *string_base)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == string_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		feature->string_base = string_base;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Sets the functions to access PEs DMEM
 * @param[in] feature Feature to set the functions
 * @param[in] read_func Function to read the PE DMEM data
 * @param[in] write_func Function to write PE DMEM data
 * @param[in] data Class/Util reference used by read_func/write_func.
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_set_dmem_funcs(pfe_fw_feature_t *feature, dmem_read_func_t read_func, dmem_write_func_t write_func, void *data)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == read_func) || (NULL == write_func) || (NULL == data)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		feature->dmem_read_func = read_func;
		feature->dmem_write_func = write_func;
		feature->dmem_rw_func_data = data;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Returns name of the feature
 * @param[in] feature Feature to be read.
 * @param[out] name The feature name to be read.
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_get_name(const pfe_fw_feature_t *feature, const char **name)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*name = feature->string_base + oal_ntohl(feature->ll.data->name);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Returns the feature description provide by the firmware.
 * @param[in] feature Feature to be read.
 * @param[out] desc Descripton of the feature
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_get_desc(const pfe_fw_feature_t *feature, const char **desc)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == desc)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*desc = feature->string_base + oal_ntohl(feature->ll.data->description);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Reads the flags of the feature
 * @param[in] feature Feature to be read
 * @param[out] flags Value of the feature flags
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_get_flags(const pfe_fw_feature_t *feature, pfe_ct_feature_flags_t *flags)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == flags)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*flags = feature->ll.data->flags;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Checks whether the feature is available in Class
 * @param[in] feature Feature to check
 * @retval TRUE Feature is implemented in Class
 * @retval FALSE Feature is not implemented in Class
 */
bool_t pfe_fw_feature_is_in_class(const pfe_fw_feature_t *feature)
{
	pfe_ct_feature_flags_t flags;
	bool_t ret;
	flags = F_NONE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == feature))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)pfe_fw_feature_get_flags(feature, &flags);

		ret = ((uint8_t)F_NONE == ((uint8_t)flags & (uint8_t)F_CLASS)) ? FALSE : TRUE;
	}
	return ret;
}

/**
 * @brief Checks whether the feature is available in Util
 * @param[in] feature Feature to check
 * @retval TRUE Feature is implemented in Util
 * @retval FALSE Feature is not implemented in Util
 */
bool_t pfe_fw_feature_is_in_util(const pfe_fw_feature_t *feature)
{
	pfe_ct_feature_flags_t flags;
	bool_t ret;
	flags = F_NONE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == feature))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)pfe_fw_feature_get_flags(feature, &flags);

		ret = ((uint8_t)F_NONE == ((uint8_t)flags & (uint8_t)F_UTIL)) ? FALSE : TRUE;
	}
	return ret;
}

/**
 * @brief Reads the default value of the feature i.e. initial value set by the FW
 * @param[in] feature Feature to read the value
 * @param[out] def_val The read default value.
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_get_def_val(const pfe_fw_feature_t *feature, uint8_t *def_val)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == def_val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*def_val = feature->ll.data->def_val;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Reads value of the feature enable variable
 * @param[in] Feature to read the value
 * @param[out] val Value read from the DMEM
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_get_val(const pfe_fw_feature_t *feature, uint8_t *val)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == feature) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = feature->dmem_read_func(feature->dmem_rw_func_data, 0U, val, (addr_t)oal_ntohl(feature->ll.data->position), sizeof(uint8_t));
	}
	return ret;
}

/**
 * @brief Checks whether the given feature is in enabled state
 * @param[in] feature Feature to check the enabled state
 * @retval TRUE Feature is enabled (the enable variable value is not 0)
 * @retval FALSE Feature is disabled (or its state could not be read)
 */
bool_t pfe_fw_feature_enabled(const pfe_fw_feature_t *feature)
{
	uint8_t val;
	errno_t ret;
	bool_t feature_enabled;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == feature))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		feature_enabled = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_fw_feature_get_val(feature, &val);

		if (EOK != ret)
		{
			feature_enabled = FALSE;
		}
		else
		{
			if (0U != val)
			{
				feature_enabled = TRUE;
			}
			else
			{
				feature_enabled = FALSE;
			}
		}
	}
	return feature_enabled;
}

/**
 * @brief Sets value of the feature enable variable in the DMEM
 * @param[in] feature Feature to set the value
 * @param[in] val Value to be set
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_set_val(const pfe_fw_feature_t *feature, uint8_t val)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == feature))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = feature->dmem_write_func(feature->dmem_rw_func_data, -1, (addr_t)oal_ntohl(feature->ll.data->position), (void *)&val, sizeof(uint8_t));
	}
	return ret;
}

/**
 * @brief Search feature entry table by name
 * @param[in] handle to the first table element
 * @param[in] name to be searched
 * @return EOK or an error code
 */
static errno_t pfe_fw_feature_table_entry_by_name(pfe_fw_tbl_handle_t *handle, const char_t *name)
{
	pfe_ct_feature_tbl_entry_t * tbl_curr;
	errno_t ret = ENOENT;

	if(NULL != handle->tbl_curr)
	{
		tbl_curr = handle->tbl_curr;

		while(tbl_curr->name[0] != '\0')
		{
			if(0 == strcmp(name, tbl_curr->name))
			{
				handle->tbl_curr = tbl_curr;
				ret = EOK;
			}
			tbl_curr++;
		}
	}

	return ret;
}

/**
 * @brief Search feature entry in config table by name
 * @param[in] feature Feature to be searched
 * @param[in] name to be searched
 * @param[out] entry table handle of the search element
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_cfg_by_name(const pfe_fw_feature_t *feature, const char *name, pfe_fw_tbl_handle_t *entry)
{
	pfe_fw_tbl_handle_t handle;
	errno_t ret;

	handle.tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->cfg));
	handle.feature = feature;

	ret = pfe_fw_feature_table_entry_by_name(&handle, name);

	if (ret == EOK)
	{
		*entry = handle;
	}

	return ret;
}

/*
 * @brief Search feature entry in stats table by name
 * @param[in] feature Feature to be searched
 * @param[in] name to be searched
 * @param[out] entry table handle of the search element
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_stats_by_name(const pfe_fw_feature_t *feature, const char* name, pfe_fw_tbl_handle_t *entry)
{
	pfe_fw_tbl_handle_t handle;
	errno_t ret;

	handle.tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->stats));
	handle.feature = feature;

	ret = pfe_fw_feature_table_entry_by_name(&handle, name);

	if(ret == EOK)
	{
		*entry = handle;
	}
	return ret;
}

/*
 * @brief Returns first entry of the config table
 * @param[in] feature Feature from witch to get config table
 * @param[out] feature_table table handle of the first entry
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_cfg_first(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table)
{
	errno_t ret = ENOENT;

	if(0 != feature->ll.data_ext->cfg)
	{
		feature_table->feature = feature;
		feature->current_cfg = 0;
		feature_table->tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->cfg));
		ret = EOK;
	}

	return ret;
}

/*
 * @brief Returns next entry of the config table
 * @param[in] feature Feature from witch to get config table
 * @param[out] feature_table table handle of the next entry
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_cfg_next(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table)
{
	errno_t ret = ENOENT;

	if(0 != feature->ll.data_ext->cfg)
	{
		feature_table->feature = feature;
		feature->current_cfg ++;
		feature_table->tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->cfg) + feature->current_cfg*sizeof(pfe_ct_feature_tbl_entry_t));
		if(feature_table->tbl_curr->name[0] != '\0')
		{
			ret = EOK;
		}
	}

	return ret;
}

/*
 * @brief Returns first entry of the stats table
 * @param[in] feature Feature from witch to get stats table
 * @param[out] feature_table table handle of the first entry
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_stats_first(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table)
{
	errno_t ret = ENOENT;

	if(0 != feature->ll.data_ext->stats)
	{
		feature_table->feature = feature;
		feature->current_stats = 0;
		feature_table->tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->stats));
		ret = EOK;
	}

	return ret;
}

/*
 * @brief Returns next entry of the stats table
 * @param[in] feature Feature from witch to get stats table
 * @param[out] feature_table table handle of the next entry
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_stats_next(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table)
{
	errno_t ret = ENOENT;

	if(0 != feature->ll.data_ext->stats)
	{
		feature_table->feature = feature;
		feature->current_stats ++;
		feature_table->tbl_curr = (pfe_ct_feature_tbl_entry_t *)(uintptr_t)(feature->string_base + oal_ntohl(feature->ll.data_ext->stats) + feature->current_stats*sizeof(pfe_ct_feature_tbl_entry_t));
		if(feature_table->tbl_curr->name[0] != '\0')
		{
			ret = EOK;
		}
	}

	return ret;
}

/*
 * @brief Returns the name of the element handle
 * @param[in] handle Table element handle
 * @param[out] table_name name of the element
 * @return EOK
 */
errno_t pfe_fw_feature_table_entry_name(pfe_fw_tbl_handle_t handle, const char **table_name)
{
	*table_name = handle.tbl_curr->name;

	return EOK;
}

/*
 * @brief Returns the size of the element handle
 * @param[in] handle Table element handle
 * @return size of the element
 */
uint32_t pfe_fw_feature_table_entry_size(pfe_fw_tbl_handle_t handle)
{
	return handle.tbl_curr->size;
}

/*
 * @brief Returns the multiplicity of the element handle
 * @param[in] handle Table element handle
 * @return multiplicity of the element
 */
uint32_t pfe_fw_feature_table_entry_multiplicity(pfe_fw_tbl_handle_t handle)
{
	return handle.tbl_curr->multiplicity;
}

/*
 * @brief Returns the allocation size of the payload
 * @param[in] handle Table element handle
 * @return allocation size of the payload
 */
uint32_t pfe_fw_feature_table_entry_allocsize(pfe_fw_tbl_handle_t handle)
{
	return handle.tbl_curr->size * handle.tbl_curr->multiplicity;
}

/*
 * @brief Sets the table entry payload
 * @param[in] handle Feature table entry on witch to set the value
 * @param[in] val    Value to be set
 * @param[in] size   Size of the value to be set
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_entry_set(pfe_fw_tbl_handle_t handle, void *val, uint16_t size)
{
	errno_t ret;
	uint8_t idx;
	void   *ptr;

	switch (pfe_fw_feature_table_entry_size(handle))
	{
		case UINT_16_SIZE: 
			for (idx = 0; idx < size/pfe_fw_feature_table_entry_size(handle); idx++)
			{
				ptr = val + idx * sizeof(uint16_t);
				*(uint16_t *)ptr = oal_ntohs(*(uint16_t *)ptr);
			}
			break;
		case UINT_32_SIZE:
			for (idx = 0; idx < size/pfe_fw_feature_table_entry_size(handle); idx++)
 			{
				ptr = val + idx * sizeof(uint32_t);
				*(uint32_t *)ptr = oal_ntohl(*(uint32_t *) ptr);
			}
			break;
 	}

	ret = handle.feature->dmem_write_func(handle.feature->dmem_rw_func_data, -1, (addr_t)oal_ntohl(handle.tbl_curr->data), (void *)val, size);

	return ret;
}

/*
 * @brief Gets the table entry payload
 * @param[in] handle Feature table entry from witch to get the payload
 * @param[in] mem    memory where we write the read values
 * @param[in] size   Size of the value to be read
 * @param[in] collect Read the values from one/all PE cores 
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_entry_get(pfe_fw_tbl_handle_t handle, void *mem, uint16_t size, bool_t collect)
{
	errno_t ret;
	uint8_t idx;
	void  *ptr;

	if(FALSE == collect)
	{
		ret =  handle.feature->dmem_read_func(handle.feature->dmem_rw_func_data, 0U, mem, (addr_t)oal_ntohl(handle.tbl_curr->data), size);

		if (EOK == ret)
		{
			for (idx = 0; idx < size/pfe_fw_feature_table_entry_size(handle); idx++)
			{
				ptr = mem + idx * pfe_fw_feature_table_entry_size(handle);
				switch (pfe_fw_feature_table_entry_size(handle))
				{
					case UINT_16_SIZE:
							*(uint16_t *)ptr = oal_ntohs(*(uint16_t *)ptr);
						break;
					case UINT_32_SIZE:
							*(uint32_t *)ptr = oal_ntohl(*(uint32_t *) ptr);
						break;
				}
			}
		}
	}
	else
	{
		for(idx = 0; idx < size/pfe_fw_feature_table_entry_size(handle); idx++)
		{
			ptr = mem + idx * pfe_fw_feature_table_entry_size(handle);
			ret = pfe_fw_feature_table_entry_get_by_idx(handle, ptr, idx, collect);
		}
	}

	return ret;
}

/*
 * @brief Sets the table entry payload at a specific index
 * @param[in] handle Feature table entry on witch to set the value
 * @param[in] val    Value to be set
 * @param[in] idx    Index of the vector to set the value
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_entry_set_by_idx(pfe_fw_tbl_handle_t handle, void *val, uint16_t idx)
{
	errno_t ret = EINVAL;

	if( idx < pfe_fw_feature_table_entry_multiplicity(handle))
	{
		switch (pfe_fw_feature_table_entry_size(handle))
		{
			case UINT_16_SIZE:
				*(uint16_t*)val = oal_htons(*(uint16_t *) val);
				break;
			case UINT_32_SIZE:
				*(uint32_t*)val = oal_htonl(*(uint32_t *) val);
				break;
		}
		ret = handle.feature->dmem_write_func(handle.feature->dmem_rw_func_data, -1, (addr_t)oal_ntohl(handle.tbl_curr->data) + idx * handle.tbl_curr->size, (void *)val, handle.tbl_curr->size);
	}

	return ret;
}

/*
 * @brief Gets the table entry payload at a specific index
 * @param[in] handle Feature table entry on witch to get the vale
 * @param[in] mem    Memory where to put the value
 * @param[in] idx    Index of the vector to set the value
 * @param[in] collect Read the data from one/all PE cores
 * @return EOK or an error code
 */
errno_t pfe_fw_feature_table_entry_get_by_idx(pfe_fw_tbl_handle_t handle, void *mem, uint16_t idx, bool_t collect)
{
	errno_t ret = EINVAL;
	uint8_t pe_idx = 0;
	uint8_t pe_idx_max = 1;
	uint8_t val8 = 0;
	uint16_t val16 = 0;
	uint32_t val32 = 0;

	if(TRUE == collect)
	{
		pe_idx_max = handle.feature->instances;
	}

	if( idx < pfe_fw_feature_table_entry_multiplicity(handle))
	{
		for(pe_idx = 0; pe_idx < pe_idx_max; pe_idx++)
		{
			ret = handle.feature->dmem_read_func(handle.feature->dmem_rw_func_data, pe_idx, mem, (addr_t)oal_ntohl(handle.tbl_curr->data) + idx * handle.tbl_curr->size, handle.tbl_curr->size);

			switch (pfe_fw_feature_table_entry_size(handle))
			{
				case UINT_8_SIZE:
					val8 += *(uint8_t *)mem;
					break;
				case UINT_16_SIZE:
					val16 += oal_htons(*(uint16_t *) mem);
					break;
				case UINT_32_SIZE:
					val32 += oal_htonl(*(uint32_t *) mem);
					break;
			}
		}

		switch (pfe_fw_feature_table_entry_size(handle))
		{
			case UINT_8_SIZE:
				*(uint8_t*)mem = val8;
				break;
			case UINT_16_SIZE:
				*(uint16_t*)mem = val16;
				break;
			case UINT_32_SIZE:
				*(uint32_t*)mem = val32;
				break;
		}
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

