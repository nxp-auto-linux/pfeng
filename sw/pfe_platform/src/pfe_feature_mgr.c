/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_class.h"
#include "pfe_util.h"
#include "pfe_hw_feature.h"
#include "pfe_feature_mgr.h"

/*
--------------         -----------------         -----------
|PFE Platform|--uses-->|pfe_feature_mgr|--uses-->|pfe_class|--uses---------\
--------------         -----------------         -----------               |
                          ^    |      |          ----------                |
-----                     |    |      \---uses-->|pfe_util|---uses------\  |
|FCI|----uses-------------/    |                 ----------             |  |
-----                          |                                        V  V
                               |                                     ----------------
                               \------------------------------uses-->|pfe_fw_feature|
                                                                     ----------------
*/

typedef struct
{
	uint32_t *cbus_base;
	uint32_t current_hw_feature;	/* Index of the hw feature to return by pfe_hw_get_feature_next() */
	pfe_hw_feature_t **hw_features;	/* List of all hw features*/
	uint32_t hw_features_count;		/* Number of items in hw_features */

	bool_t rewind;					/* Internal flag supporting transition walk from hw_feature set to fw_feature set */
	pfe_class_t *class;
	pfe_util_t *util;
} pfe_feature_mgr_t;

static errno_t pfe_hw_get_feature(const pfe_feature_mgr_t *feature_mgr, pfe_hw_feature_t **feature, const char *name);
static errno_t pfe_hw_get_feature_first(pfe_feature_mgr_t *feature_mgr, pfe_hw_feature_t **feature);
static errno_t pfe_hw_get_feature_next(pfe_feature_mgr_t *feature_mgr, pfe_hw_feature_t **feature);

/**
 * @brief Feature manager instance
 * @details The feature manager is a single instance only, the instance handle is stored here
 */

static pfe_feature_mgr_t *feature_mgr = NULL;

/**
 * @brief Initializes (the only) feature manager instance
 * @param[in] cbus_base Reference to the Platform config
 * @return EOK or error code in case of failure.
 */
errno_t pfe_feature_mgr_init(uint32_t *cbus_base)
{
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == cbus_base)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		feature_mgr = oal_mm_malloc(sizeof(pfe_feature_mgr_t));
		if(NULL != feature_mgr)
		{
			memset(feature_mgr, 0, sizeof(pfe_feature_mgr_t));
			feature_mgr->cbus_base = cbus_base;

			ret = pfe_hw_feature_init_all(cbus_base, &feature_mgr->hw_features, &feature_mgr->hw_features_count);
		}
		else
		{
			ret = ENOMEM;
		}
	}
	else
	{
		ret = EPERM;
	}

	return ret;
}

/**
 * @brief Link FW modules class and util
 * @param[in] class Reference to the class module (cannot be NULL - class must be always present)
 * @param[in] util Reference to the util module (value NULL means util is not present)
 * @return EOK or error code in case of failure.
 */
errno_t pfe_feature_mgr_add_fw_modules(pfe_class_t *class, pfe_util_t *util)
{
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == class)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	/* Note it is OK for "util" to be NULL */
	#endif

	if(NULL != feature_mgr)
	{
			feature_mgr->class = class;
			feature_mgr->util = util;
	}
	else
	{
		ret = EPERM;
	}

	return ret;
}

/**
 * @brief Deinitializes the feature manager instance
 * @return EOK or error code in case of failure. 
 */
errno_t pfe_feature_mgr_fini(void)
{
	errno_t ret;

	if(NULL == feature_mgr)
	{
		ret = EEXIST;
	}
	else
	{
		oal_mm_free(feature_mgr);
		feature_mgr = NULL;
		ret = EOK;
	}

	return ret;
}

/**
 * @brief 		Checks whether the firmware feature with given name is available to be used
 * @param[in]	name Name of the feature to check
 * @retval		TRUE Feature is available
 * @retval		FALSE Feature is not available
 * @details		It is checked whether the feature is applicable for hw, class, util or both and then
 *				it is checked whether it is enabled at all places it is applicable for.
 */
bool_t pfe_feature_mgr_is_available(const char *feature_name)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	pfe_fw_feature_t *fw_feature_util;
	errno_t ret_class, ret_util;
	bool_t class_avail = FALSE;
	bool_t util_avail = FALSE;
	errno_t ret_hw;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return FALSE;
	}


	ret_hw = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
	if(EOK == ret_hw)
	{	/* Descriptor in platform is available */
		if(pfe_hw_feature_enabled(hw_feature))
		{	/* Feature is enabled thus it is available */
			return TRUE;
		}
		else
		{	/* Feature is disabled thus it is not available */
			return FALSE;
		}
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return FALSE;
	}

	ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
	/* Note if one of ret_class/ret_util is EOK and one is not then the data
	   is inconsistent. In such case the one without EOK will block feature
	   use. This situation should not happen. */

	/* Analyze Class */
	if(EOK == ret_class)
	{	/* Descriptor in class is available */

		if(TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
		{	/* This feature is applicable for class */

			if(pfe_fw_feature_enabled(fw_feature_class))
			{	/* Feature is enabled thus it is available */
				class_avail = TRUE;
			}
			else
			{	/* Feature is disabled thus it is not available */
				class_avail = FALSE;
			}
		}
		else
		{	/* Not applicable for class */
			/* Do not block availability of the feature which is not applicable */
			class_avail = TRUE;
		}
	}
	else
	{	/* Feature does not exist i.e. it is not available */
		util_avail = FALSE;
	}

	if(NULL != feature_mgr->util)
	{	/* Util is present */
		ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);

		/* Analyze Util */
		if(EOK == ret_util)
		{	/* Descriptor in util is available */
			if(TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
			{	/* This feature is applicable for util */

				if(pfe_fw_feature_enabled(fw_feature_util))
				{	/* Feature is enabled thus it is available */
					util_avail = TRUE;
				}
				else
				{	/* Feature is disabled thus it is not available */
					util_avail = FALSE;
				}
			}
			else
			{	/* Not applicable for util */
				/* Do not block availability of the feature which is not applicable */
				util_avail = TRUE;
			}
		}
		else
		{	/* Feature does not exist i.e. it is not available */
			util_avail = FALSE;
		}
	}
	else
	{	/* Util not present */

		/* Use class information to check whether the feature requires util to be present */
		if(pfe_fw_feature_is_in_util(fw_feature_class))
		{
			/* No firmware = no feature */
			util_avail = FALSE;
		}
		else
		{
			/* Feature does not need util to be present */
			util_avail = TRUE;
		}
	}

	/* Return TRUE if the feature availability is not blocked by any of class/util pair */
	return ((FALSE != class_avail) && (FALSE != util_avail)) ? TRUE : FALSE;
}

/**
 * @brief Sets the value of the feature enable variable
 * @param[in] feature_name Name of the feature to be set
 * @param[in] val Value to be set
 * @return EOK or failure code.
 */
errno_t pfe_feature_mgr_set_val(const char *feature_name, const uint8_t val)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	pfe_fw_feature_t *fw_feature_util;
	errno_t ret_class, ret_util;
	uint8_t old_val;
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	ret = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
	if(EOK == ret)
	{	/* Feature exists */
		ret = pfe_hw_feature_set_val(hw_feature, val);
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
	if(EOK != ret_class)
	{	/* Feature does not exist or data is inconsistent */
		return EINVAL;
	}

	if(NULL != feature_mgr->util)
	{
		ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);
		if(EOK != ret_util)
		{	/* Feature does not exist or data is inconsistent */
			ret = EINVAL;
			return ret;
		}
	}

	/* Handle the Class */
	if(TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
	{
		/* Backup the original value for the failure case */
		(void )pfe_fw_feature_get_val(fw_feature_class, &old_val);
		/* Set the new value */
		ret = pfe_fw_feature_set_val(fw_feature_class, val);
	}

	/* Handle the Util */
	if(NULL != feature_mgr->util)
	{	/* Util is present */
		/* Continue only if the previous code succeeded - we need to
		   keep the class and util coherent */
		if(EOK == ret)
		{
			if(TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
			{
				ret = pfe_fw_feature_set_val(fw_feature_util, val);
				if(EOK != ret)
				{	/* Failure */
					/* Revert the changes already made */
					pfe_fw_feature_set_val(fw_feature_util, old_val);
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Enables the given feature
 * @param[in]	feature_name Feature to enabled
 * @retval		EOK the feature is enabled
 * @return		Failure code means the feature could not be enabled
 */
errno_t pfe_feature_mgr_enable(const char *feature_name)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	pfe_ct_feature_flags_t tmp;
	errno_t ret;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	ret = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
	if(EOK == ret)
	{
		ret = pfe_hw_feature_get_flags(hw_feature, &tmp);
		if(EOK == ret)
		{
			if(0 == (tmp & F_PRESENT))
			{	/* Feature cannot be enabled */
				NXP_LOG_WARNING("Cannot enable feature %s - not present in Platform\n", feature_name);
				return EINVAL;
			}
			else if(0 == (tmp & F_RUNTIME))
			{	/* Feature cannot be disabled */
				NXP_LOG_INFO("Feature %s is always enabled in Platform\n", feature_name);
				return EOK;
			}
			else
			{	/* Feature needs to be disabled */
				return pfe_feature_mgr_set_val(feature_name, 1);
			}
		}

		/* Don't continue with FW features */
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	/* Class and util share the same information thus it is
	   enough to use just the class to get it */
	ret = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
	if(EOK == ret)
	{
		ret = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
		if(EOK == ret)
		{
			if(0 == (tmp & F_PRESENT))
			{	/* Feature cannot be enabled */
				NXP_LOG_WARNING("Cannot enable feature %s - not present in FW\n", feature_name);
				return EINVAL;
			}
			else if(0 == (tmp & F_RUNTIME))
			{	/* Feature cannot be disabled */
				NXP_LOG_INFO("Feature %s is always enabled in FW\n", feature_name);
				return EOK;
			}
			else
			{	/* Feature needs to be disabled */
				return pfe_feature_mgr_set_val(feature_name, 1);
			}
		}
	}

	return EINVAL;
}

/**
 * @brief		Disables the given feature
 * @param[in]	feature_name Feature to disable
 * @retval		EOK the feature is disabled
 * @return		Failure code means the feature could not be disabled
 */
errno_t pfe_feature_mgr_disable(const char *feature_name)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	pfe_ct_feature_flags_t tmp;
	errno_t ret;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	ret = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
	if(EOK == ret)
	{
		ret = pfe_hw_feature_get_flags(hw_feature, &tmp);
		if(EOK == ret)
		{
			if(0 == (tmp & F_PRESENT))
			{	/* Feature cannot be enabled */
				NXP_LOG_INFO("Feature %s is always disabled in Platform\n", feature_name);
				return EOK;
			}
			else if(0 == (tmp & F_RUNTIME))
			{	/* Feature cannot be disabled */
				NXP_LOG_ERROR("Cannot disabled feature %s - always enabled in Platform\n", feature_name);
				return EINVAL;
			}
			else
			{	/* Feature needs to be disabled */
				return pfe_feature_mgr_set_val(feature_name, 0);
			}
		}

		/* Don't continue with FW features */
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	/* Class and util share the same information thus it is
	   enough to use just the class to get it */
	ret = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
	if(EOK == ret)
	{
		ret = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
		if(EOK == ret)
		{
			if(0 == (tmp & F_PRESENT))
			{	/* Feature cannot be enabled */
				NXP_LOG_INFO("Feature %s is always disabled in FW\n", feature_name);
				return EOK;
			}
			else if(0 == (tmp & F_RUNTIME))
			{	/* Feature cannot be disabled */
				NXP_LOG_ERROR("Cannot disabled feature %s - always enabled in FW\n", feature_name);
				return EINVAL;
			}
			else
			{	/* Feature needs to be disabled */
				return pfe_feature_mgr_set_val(feature_name, 0);
			}
		}
	}

	return EINVAL;
}



/**
 * @brief		Reads the feature value
 * @param[in]	feature_name Name of the feature to be read
 * @param[out]	val The read value of the feature enable variable
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_val(const char *feature_name, uint8_t *val)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	pfe_fw_feature_t *fw_feature_util;
	errno_t ret_class, ret_util;
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if((NULL == feature_name)||(NULL == val))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	ret = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
	if(EOK == ret)
	{	/* Feature exist */
		return pfe_hw_feature_get_val(hw_feature, val);
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
	if(EOK != ret_class)
	{	/* Feature does not exist or data is inconsistent */
		return EINVAL;
	}

	if(NULL != feature_mgr->util)
	{
		ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);
		if(EOK != ret_util)
		{	/* Data is inconsistent - feature found in class but not in util */
			NXP_LOG_WARNING("Inconsistent feature data for %s\n", feature_name);
			return EINVAL;
		}
	}

	/* Check the value in class if relates to class */
	if(TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
	{
		ret = pfe_fw_feature_get_val(fw_feature_class, val);
		/* We can stop here because data shall be consistent between class and util
		   thus it does not matter which value is read */
		return ret;
	}

	/* This is for features related to util only (code above will not read the value)*/
	if(NULL != feature_mgr->util)
	{	/* Util is available */
		/* Check the value in util if relates to util */
		if(TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
		{
			return pfe_fw_feature_get_val(fw_feature_util, val);
		}
	}

	/* We can get here only if feature is not present in class nor util */
	NXP_LOG_WARNING("Wrong feature %s (not relevant to any FW)\n", feature_name);
	return EINVAL;
}

/**
 * @brief		Returns the 1st feature (resets the features query)
 * @param[out]	feature_name Name of the 1st feature
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_first(const char **feature_name)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature;
	errno_t ret;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	ret = pfe_hw_get_feature_first(feature_mgr, &hw_feature);
	if(EOK == ret)
	{
		ret = pfe_hw_feature_get_name(hw_feature, feature_name);
		/* Signal rewind for class/util fw feature walk */
		feature_mgr->rewind = TRUE;
	}
	else
	{
		/* We use the fact that class and util share same list of features and read
		   from only one of them */

		if (NULL == feature_mgr->class)
		{	/* Class block is not initialized */
			return EINVAL;
		}

		ret = pfe_class_get_feature_first(feature_mgr->class, &fw_feature);
		if(EOK == ret)
		{
			ret = pfe_fw_feature_get_name(fw_feature, feature_name);
		}
		feature_mgr->rewind = FALSE;
	}

	return ret;
}

/**
 * @brief		Returns the next feature (continues the features query)
 * @param[out]	feature_name Name of the next feature
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_next(const char **feature_name)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature;
	errno_t ret;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	ret = pfe_hw_get_feature_next(feature_mgr, &hw_feature);
	if(EOK == ret)
	{
		ret = pfe_hw_feature_get_name(hw_feature, feature_name);
	}
	else if (ENOENT == ret)
	{
		/* We use the fact that class and util share same list of features and read
		   from only one of them */

		if (NULL == feature_mgr->class)
		{	/* Class block is not initialized */
			return EINVAL;
		}

		if (TRUE == feature_mgr->rewind)
		{
			ret = pfe_class_get_feature_first(feature_mgr->class, &fw_feature);
			/* Unset 'rewind' to use real pfe_class_get_feature_next next time */
			feature_mgr->rewind = FALSE;
		}
		else
		{
			ret = pfe_class_get_feature_next(feature_mgr->class, &fw_feature);
		}

		if(EOK == ret)
		{
			ret = pfe_fw_feature_get_name(fw_feature, feature_name);
		}
	}

	return ret;
}

/**
 * @brief		Returns the feature default value
 * @param[in]	feature_name Name of the feature
 * @param[out]	val Default (initial) value of feature enable variable
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_def_val(const char *feature_name, uint8_t *val)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	if(EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
	{
		ret = pfe_hw_feature_get_def_val(hw_feature, val);
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	/* The data shall be consistent between util and class thus it is enough to read
	   them from class */
	if(EOK == pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name))
	{
		ret = pfe_fw_feature_get_def_val(fw_feature_class, val);
	}

	return ret;
}

/**
 * @brief		Returns the feature default description text
 * @param[in]	feature_name Name of the feature
 * @param[out]	desc Feature description text (string)
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_desc(const char *feature_name, const char **desc)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	errno_t ret = EOK;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if((NULL == desc)||(NULL == feature_name))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* Platfoorm feature first */
	if(EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
	{
		ret = pfe_hw_feature_get_desc(hw_feature, desc);
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	/* The data shall be consistent between util and class thus it is enough to read
	   them from class */
	if(EOK == pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name))
	{
		ret = pfe_fw_feature_get_desc(fw_feature_class, desc);
	}

	return ret;
}

/**
 * @brief		Returns the feature variant
 * @param[in]	feature_name Name of the feature
 * @param[out]	val Value of the flags defining feature variant
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_get_variant(const char *feature_name, uint8_t *val)
{
	pfe_hw_feature_t *hw_feature;
	pfe_fw_feature_t *fw_feature_class;
	errno_t ret = EOK;
	pfe_ct_feature_flags_t tmp;

	#if defined(PFE_CFG_NULL_ARG_CHECK)
	if((NULL == feature_name)||(NULL == val))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	#endif

	if(NULL == feature_mgr)
	{
		NXP_LOG_ERROR("Feature Mgr not initialized\n");
		return EINVAL;
	}

	/* HW feature first */
	if(EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
	{
		ret = pfe_hw_feature_get_flags(hw_feature, &tmp);
		if(EOK == ret)
		{
			*val = tmp & (F_PRESENT | F_RUNTIME);
		}
		return ret;
	}

	if (NULL == feature_mgr->class)
	{	/* Class block is not initialized */
		return EINVAL;
	}

	/* The data shall be consistent between util and class thus it is enough to read
	   them from class */
	if(EOK == pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name))
	{
		ret = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
		if(EOK == ret)
		{
			*val = tmp & (F_PRESENT | F_RUNTIME);
		}
	}

	return ret;
}

/**
 * @brief Finds and returns HW feature by its name
 * @param[in] fmgr The feature_mgr instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @param[in] name Name of the feature to be found
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
static errno_t pfe_hw_get_feature(const pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature, const char *name)
{
	uint32_t i;
	const char *fname;
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	for(i = 0U; i < fmgr->hw_features_count; i++)
	{
		ret = pfe_hw_feature_get_name(fmgr->hw_features[i], &fname);
		if(ret == EOK)
		{
			if(0 == strcmp(fname, name))
			{
				*feature = fmgr->hw_features[i];
				return EOK;
			}
		}
	}
	return ENOENT;
}

/**
 * @brief Finds and returns the 1st HW feature by order of their discovery - used for listing all features
 * @param[in] fmgr The feature mgr instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
static errno_t pfe_hw_get_feature_first(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(fmgr->hw_features_count > 0U)
	{
		fmgr->current_hw_feature = 0U;
		*feature = fmgr->hw_features[fmgr->current_hw_feature];
		return EOK;
	}

	return ENOENT;
}

/**
 * @brief Finds and returns the next HW feature by order of their discovery - used for listing all features
 * @param[in] fmgr The feature_mgr instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
static errno_t pfe_hw_get_feature_next(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature)
{
 #if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(fmgr->hw_features_count > 0U)
	{
		/* Avoid going out of the array boundaries */
		if((fmgr->current_hw_feature + 1U) < fmgr->hw_features_count)
		{
			fmgr->current_hw_feature += 1U;
			*feature = fmgr->hw_features[fmgr->current_hw_feature];
			return EOK;
		}
	}

	return ENOENT;
}
