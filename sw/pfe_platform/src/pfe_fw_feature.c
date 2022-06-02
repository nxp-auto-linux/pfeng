/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
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
	pfe_ct_feature_desc_t *ll_data;
	const char *string_base;
	dmem_read_func_t dmem_read_func;
	dmem_write_func_t dmem_write_func;
	void *dmem_rw_func_data;
};

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
 * @return EOK or an error code.
 */
errno_t pfe_fw_feature_set_ll_data(pfe_fw_feature_t *feature, pfe_ct_feature_desc_t *ll_data)
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
		feature->ll_data = ll_data;
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
		*name = feature->string_base + oal_ntohl(feature->ll_data->name);
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
		*desc = feature->string_base + oal_ntohl(feature->ll_data->description);
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
		*flags = feature->ll_data->flags;
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
		*def_val = feature->ll_data->def_val;
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
		ret = feature->dmem_read_func(feature->dmem_rw_func_data, 0U, val, (addr_t)oal_ntohl(feature->ll_data->position), sizeof(uint8_t));
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
		ret = feature->dmem_write_func(feature->dmem_rw_func_data, -1, (addr_t)oal_ntohl(feature->ll_data->position), (void *)&val, sizeof(uint8_t));
	}
	return ret;
}
