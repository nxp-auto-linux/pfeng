/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2021-2023 NXP
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
#include "pfe_tmu.h"
#include "pfe_hw_feature.h"
#include "pfe_feature_mgr.h"
#include "pfe_fw_feature.h"

/*
--------------         -----------------         -----------
|PFE Platform|--uses-->|pfe_feature_mgr|--uses-->|pfe_class|--uses---------\
--------------         -----------------         -----------               |
                          ^    |      |          ----------                |
-----                     |    |      \---uses-->|pfe_util|---uses------\  |
|FCI|----uses-------------/    |                 ----------             |  |
-----                          |                                        V  V
                               |                                     ----------------
                               |------------------------------uses-->|pfe_fw_feature|
                               |                                     ----------------
                               |
                               |                                     ----------------
                               \------------------------------uses-->|pfe_hw_feature|
                                                                     ----------------
*/

typedef struct
{
	uint32_t *         cbus_base;
	uint32_t           current_hw_feature; /* Index of the hw feature to return by pfe_hw_get_feature_next() */
	pfe_hw_feature_t **hw_features;        /* List of all hw features*/
	uint32_t           hw_features_count;  /* Number of items in hw_features */

	bool_t rewind_flg; /* Internal flag supporting transition walk from hw_feature set to fw_feature set */
	pfe_class_t *class;
	pfe_util_t *util;
	pfe_tmu_t * tmu; /* Included because of err051211_workaround */
} pfe_feature_mgr_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief Feature manager instance
 * @details The feature manager is a single instance only, the instance handle is stored here
 */

static pfe_feature_mgr_t *feature_mgr = NULL;

/**
 *  Internal flag supporting transition walk from cfg table to stats table
 */
static bool_t table_rewind_flag = FALSE;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_hw_get_feature(const pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature, const char *name);
static errno_t pfe_hw_get_feature_first(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature);
static errno_t pfe_hw_get_feature_next(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature);
static errno_t pfe_feature_mgr_configure_driver(const char *feature_name, const uint8_t val);

/**
 * @brief Initializes (the only) feature manager instance
 * @param[in] cbus_base Reference to the Platform config
 * @return EOK or error code in case of failure.
 */
errno_t pfe_feature_mgr_init(uint32_t *cbus_base)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == cbus_base)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			feature_mgr = oal_mm_malloc(sizeof(pfe_feature_mgr_t));
			if (NULL != feature_mgr)
			{
				(void)memset(feature_mgr, 0, sizeof(pfe_feature_mgr_t));
				feature_mgr->cbus_base = cbus_base;
				feature_mgr->hw_features = oal_mm_malloc(2U * sizeof(pfe_hw_feature_t *));
				if (NULL == feature_mgr->hw_features)
				{
					NXP_LOG_ERROR("Unable to allocate memory\n");
					oal_mm_free(feature_mgr);
					ret = ENOMEM;
				}
				else
				{
					table_rewind_flag = FALSE;
					ret = pfe_hw_feature_init_all(cbus_base, feature_mgr->hw_features, &feature_mgr->hw_features_count);
				}
			}
			else
			{
				NXP_LOG_ERROR("Unable to allocate memory\n");
				ret = ENOMEM;
			}
		}
		else
		{
			ret = EPERM;
		}
	}

	return ret;
}

/**
 * @brief Link FW modules class and util
 * @param[in] class Reference to the class module (cannot be NULL - class must be always present)
 * @param[in] util Reference to the util module (value NULL means util is not present)
 * @param[in] tmu Reference to the tmu module  (cannot be NULL - tmu must be always present)
 * @return EOK or error code in case of failure.
 */
errno_t pfe_feature_mgr_add_modules(pfe_class_t *class, pfe_util_t *util, pfe_tmu_t *tmu)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == class) || (NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
/* Note it is OK for "util" to be NULL */
#endif
	{
		if (NULL != feature_mgr)
		{
			feature_mgr->class = class;
			feature_mgr->util = util;
			feature_mgr->tmu = tmu;
			ret = EOK;
		}
		else
		{
			ret = EPERM;
		}
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

	if (NULL == feature_mgr)
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
	pfe_fw_feature_t *fw_feature_util = NULL;
	errno_t           ret_class, ret_util;
	bool_t            class_avail = FALSE;
	bool_t            util_avail = FALSE;
	bool_t            ret = FALSE;
	errno_t           ret_hw;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = FALSE;
		}
		else
		{

			ret_hw = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
			if (EOK == ret_hw)
			{
				/* Descriptor in platform is available */
				if (pfe_hw_feature_enabled(hw_feature))
				{
					/* Feature is enabled thus it is available */
					ret = TRUE;
				}
				else
				{
					/* Feature is disabled thus it is not available */
					ret = FALSE;
				}
			}
			else
			{

				if (NULL == feature_mgr->class)
				{
					/* Class block is not initialized */
					ret = FALSE;
				}
				else
				{

					ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					/* Note if one of ret_class/ret_util is EOK and one is not then the data is inconsistent. In such case the one without EOK will block feature
	   				use. This situation should not happen. */

					/* Analyze Class */
					if (EOK == ret_class)
					{
						/* Descriptor in class is available */

						if (TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
						{
							/* This feature is applicable for class */

							if (pfe_fw_feature_enabled(fw_feature_class))
							{
								/* Feature is enabled thus it is available */
								class_avail = TRUE;
							}
							else
							{
								/* Feature is disabled thus it is not available */
								class_avail = FALSE;
							}
						}
						else
						{
							/* Not applicable for class */
							/* Do not block availability of the feature which is not applicable */
							class_avail = TRUE;
						}
					}
					else
					{
						/* Feature does not exist i.e. it is not available */
						util_avail = FALSE;
					}

					if (NULL != feature_mgr->util)
					{
						/* Util is present */
						ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);

						/* Analyze Util */
						if (EOK == ret_util)
						{
							/* Descriptor in util is available */
							if (TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
							{
								/* This feature is applicable for util */

								if (pfe_fw_feature_enabled(fw_feature_util))
								{
									/* Feature is enabled thus it is available */
									util_avail = TRUE;
								}
								else
								{
									/* Feature is disabled thus it is not available */
									util_avail = FALSE;
								}
							}
							else
							{
								/* Not applicable for util */
								/* Do not block availability of the feature which is not applicable */
								util_avail = TRUE;
							}
						}
						else
						{
							/* Feature does not exist i.e. it is not available */
							util_avail = FALSE;
						}
					}
					else
					{
						/* Util not present */

						if (EOK == ret_class)
						{
							/* Use class information to check whether the feature requires util to be present */
							if (pfe_fw_feature_is_in_util(fw_feature_class))
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
					}
					/* Return TRUE if the feature availability is not blocked by any of class/util pair */
					ret = ((FALSE != class_avail) && (FALSE != util_avail)) ? TRUE : FALSE;
				}
			}
		}
	}

	return ret;
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
	pfe_fw_feature_t *fw_feature_util = NULL;
	pfe_ct_feature_flags_t flags = F_NONE;
	errno_t           ret_class, ret_util;
	errno_t           ret_feature;
	uint8_t           old_val;
	errno_t           ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			ret_feature = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
			if (EOK == ret_feature)
			{ /* Feature exists */
				ret = pfe_hw_feature_get_flags(hw_feature, &flags);
				if (0U != ((uint8_t)flags & (uint8_t)F_RUNTIME))
				{
					ret = pfe_hw_feature_set_val(hw_feature, val);
				}
				else
				{
					ret = EFAULT;
				}
			}
			else
			{

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{
					ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK != ret_class)
					{ /* Feature does not exist or data is inconsistent */
						ret = EINVAL;
					}
					else
					{

						if (NULL != feature_mgr->util)
						{
							ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);
							if (EOK != ret_util)
							{ /* Feature does not exist or data is inconsistent */
								ret = EINVAL;
							}
						}

						if (EOK == ret)
						{
							/* Handle the Class */
							if (TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
							{
								/* Backup the original value for the failure case */
								(void)pfe_fw_feature_get_val(fw_feature_class, &old_val);
								/* Set the new value */
								ret = pfe_fw_feature_set_val(fw_feature_class, val);
							}

							/* Handle the Util */
							if (NULL != feature_mgr->util)
							{ /* Util is present */
								/* Continue only if the previous code succeeded - we need to
		   						keep the class and util coherent */
								if (EOK == ret)
								{
									if (TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
									{
										ret = pfe_fw_feature_set_val(fw_feature_util, val);
										if (EOK != ret)
										{ /* Failure */
											/* Revert the changes already made */
											(void)pfe_fw_feature_set_val(fw_feature_util, old_val);
										}
									}
								}
							}

							/* Check/configure driver (if needed) */
							if (EOK == ret)
							{
								ret = pfe_feature_mgr_configure_driver(feature_name, val);
							}
						}
					}
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
	pfe_hw_feature_t *     hw_feature;
	pfe_fw_feature_t *     fw_feature_class;
	pfe_ct_feature_flags_t tmp;
	errno_t                ret = EINVAL;
	errno_t                ret_class;
	errno_t                ret_flag;
	errno_t                ret_hw_feature;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			/* HW feature first */
			ret_hw_feature = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
			if (EOK == ret_hw_feature)
			{
				ret_flag = pfe_hw_feature_get_flags(hw_feature, &tmp);
				if (EOK == ret_flag)
				{
					if (0U == ((uint8_t)tmp & (uint8_t)F_PRESENT))
					{ /* Feature cannot be enabled */
						NXP_LOG_WARNING("Cannot enable feature %s - not present in Platform\n", feature_name);
						ret = EINVAL;
					}
					else if (0U == ((uint8_t)tmp & (uint8_t)F_RUNTIME))
					{ /* Feature cannot be disabled */
						NXP_LOG_INFO("Feature %s is always enabled in Platform\n", feature_name);
						ret = EOK;
					}
					else
					{ /* Feature needs to be enabled */
						ret = pfe_feature_mgr_set_val(feature_name, 1);
					}
				}

				/* Don't continue with FW features */
			}
			else
			{

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{

					/* Class and util share the same information thus it is enough to use just the class to get it */
					ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK == ret_class)
					{
						ret_flag = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
						if (EOK == ret_flag)
						{
							if (0U == ((uint8_t)tmp & (uint8_t)F_PRESENT))
							{ /* Feature cannot be enabled */
								NXP_LOG_WARNING("Cannot enable feature %s - not present in FW\n", feature_name);
								ret = EINVAL;
							}
							else if (0U == ((uint8_t)tmp & (uint8_t)F_RUNTIME))
							{ /* Feature cannot be disabled */
								NXP_LOG_INFO("Feature %s is always enabled in FW\n", feature_name);
								ret = EOK;
							}
							else
							{ /* Feature needs to be enabled */
								ret = pfe_feature_mgr_set_val(feature_name, 1);
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Disables the given feature
 * @param[in]	feature_name Feature to disable
 * @retval		EOK the feature is disabled
 * @return		Failure code means the feature could not be disabled
 */
errno_t pfe_feature_mgr_disable(const char *feature_name)
{
	pfe_hw_feature_t *     hw_feature;
	pfe_fw_feature_t *     fw_feature_class;
	pfe_ct_feature_flags_t tmp;
	errno_t                ret = EINVAL;
	errno_t                ret_class;
	errno_t                ret_flag;
	errno_t                ret_hw_feature;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			/* HW feature first */
			ret_hw_feature = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
			if (EOK == ret_hw_feature)
			{
				ret_flag = pfe_hw_feature_get_flags(hw_feature, &tmp);
				if (EOK == ret_flag)
				{
					if (0U == ((uint8_t)tmp & (uint8_t)F_PRESENT))
					{ /* Feature cannot be enabled */
						NXP_LOG_INFO("Feature %s is always disabled in Platform\n", feature_name);
						ret = EOK;
					}
					else if (0U == ((uint8_t)tmp & (uint8_t)F_RUNTIME))
					{ /* Feature cannot be disabled */
						NXP_LOG_ERROR("Cannot disabled feature %s - always enabled in Platform\n", feature_name);
						ret = EINVAL;
					}
					else
					{ /* Feature needs to be disabled */
						ret = pfe_feature_mgr_set_val(feature_name, 0);
					}
				}

				/* Don't continue with FW features */
			}
			else
			{

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{

					/* Class and util share the same information thus it is enough to use just the class to get it */
					ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK == ret_class)
					{
						ret_flag = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
						if (EOK == ret_flag)
						{
							if (0U == ((uint8_t)tmp & (uint8_t)F_PRESENT))
							{ /* Feature cannot be enabled */
								NXP_LOG_INFO("Feature %s is always disabled in FW\n", feature_name);
								ret = EOK;
							}
							else if (0U == ((uint8_t)tmp & (uint8_t)F_RUNTIME))
							{ /* Feature cannot be disabled */
								NXP_LOG_ERROR("Cannot disabled feature %s - always enabled in FW\n", feature_name);
								ret = EINVAL;
							}
							else
							{ /* Feature needs to be disabled */
								ret = pfe_feature_mgr_set_val(feature_name, 0);
							}
						}
					}
				}
			}
		}
	}

	return ret;
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
	pfe_fw_feature_t *fw_feature_util = NULL;
	errno_t           ret_class, ret_util;
	errno_t           ret_feature;
	errno_t           ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == feature_name) || (NULL == val))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			ret_feature = pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name);
			if (EOK == ret_feature)
			{ /* Feature exist */
				ret = pfe_hw_feature_get_val(hw_feature, val);
			}
			else
			{

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{

					ret_class = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK != ret_class)
					{ /* Feature does not exist or data is inconsistent */
						ret = EINVAL;
					}
					else
					{

						if (NULL != feature_mgr->util)
						{
							ret_util = pfe_util_get_feature(feature_mgr->util, &fw_feature_util, feature_name);
							if (EOK != ret_util)
							{ /* Data is inconsistent - feature found in class but not in util */
								NXP_LOG_WARNING("Inconsistent feature data for %s\n", feature_name);
								ret = EINVAL;
							}
						}
						if (EOK == ret)
						{
							/* Check the value in class if relates to class */
							if (TRUE == pfe_fw_feature_is_in_class(fw_feature_class))
							{
								ret = pfe_fw_feature_get_val(fw_feature_class, val);
								/* We can stop here because data shall be consistent between class and util
		   						thus it does not matter which value is read */
							}
							else
							{

								/* This is for features related to util only (code above will not read the value)*/
								if (NULL != feature_mgr->util)
								{ /* Util is available */
									/* Check the value in util if relates to util */
									if (TRUE == pfe_fw_feature_is_in_util(fw_feature_util))
									{
										ret = pfe_fw_feature_get_val(fw_feature_util, val);
									}
									else
									{
										/* We can get here only if feature is not present in class nor util */
										NXP_LOG_WARNING("Wrong feature %s (not relevant to any FW)\n", feature_name);
									}
								}
								else
								{
									/* We can get here only if feature is not present in class nor util */
									NXP_LOG_WARNING("Wrong feature %s (not relevant to any FW)\n", feature_name);
								}
							}
						}
					}
				}
			}
		}
	}
	return ret;
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
	errno_t           ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			/* HW feature first */
			ret = pfe_hw_get_feature_first(feature_mgr, &hw_feature);
			if (EOK == ret)
			{
				ret = pfe_hw_feature_get_name(hw_feature, feature_name);
				/* Signal rewind_flg for class/util fw feature walk */
				feature_mgr->rewind_flg = TRUE;
			}
			else
			{
				/* We use the fact that class and util share same list of features and read
		   from only one of them */

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{
					ret = pfe_class_get_feature_first(feature_mgr->class, &fw_feature);
					if (EOK == ret)
					{
						ret = pfe_fw_feature_get_name(fw_feature, feature_name);
					}
					feature_mgr->rewind_flg = FALSE;
				}
			}
		}
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
	errno_t           ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{

			/* HW feature first */
			ret = pfe_hw_get_feature_next(feature_mgr, &hw_feature);
			if (EOK == ret)
			{
				ret = pfe_hw_feature_get_name(hw_feature, feature_name);
			}
			else if (ENOENT == ret)
			{
				/* We use the fact that class and util share same list of features and read
		   from only one of them */

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{

					if (TRUE == feature_mgr->rewind_flg)
					{
						ret = pfe_class_get_feature_first(feature_mgr->class, &fw_feature);
						/* Unset 'rewind_flg' to use real pfe_class_get_feature_next next time */
						feature_mgr->rewind_flg = FALSE;
					}
					else
					{
						ret = pfe_class_get_feature_next(feature_mgr->class, &fw_feature);
					}

					if (EOK == ret)
					{
						ret = pfe_fw_feature_get_name(fw_feature, feature_name);
					}
				}
			}
			else
			{
				; /* No action required */
			}
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
	errno_t           ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{
			/* HW feature first */
			if (EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
			{
				ret = pfe_hw_feature_get_def_val(hw_feature, val);
			}
			else
			{

				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{
					/* The data shall be consistent between util and class thus it is enough to read them from class */

					ret = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK == ret)
					{
						ret = pfe_fw_feature_get_def_val(fw_feature_class, val);
					}
				}
			}
		}
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
	errno_t           ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == desc) || (NULL == feature_name))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{
			/* Platfoorm feature first */
			if (EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
			{
				ret = pfe_hw_feature_get_desc(hw_feature, desc);
			}
			else
			{
				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{
					/* The data shall be consistent between util and class thus it is enough to read
	   				them from class */
					ret = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK == ret)
					{
						ret = pfe_fw_feature_get_desc(fw_feature_class, desc);
					}
				}
			}
		}
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
	pfe_hw_feature_t *     hw_feature;
	pfe_fw_feature_t *     fw_feature_class;
	errno_t                ret;
	pfe_ct_feature_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == feature_name) || (NULL == val))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (NULL == feature_mgr)
		{
			NXP_LOG_ERROR("Feature Mgr not initialized\n");
			ret = EINVAL;
		}
		else
		{
			/* HW feature first */
			if (EOK == pfe_hw_get_feature(feature_mgr, &hw_feature, feature_name))
			{
				ret = pfe_hw_feature_get_flags(hw_feature, &tmp);
				if (EOK == ret)
				{
					*val = (uint8_t)tmp & ((uint8_t)F_PRESENT | (uint8_t)F_RUNTIME);
				}
			}
			else
			{
				if (NULL == feature_mgr->class)
				{ /* Class block is not initialized */
					ret = EINVAL;
				}
				else
				{
					/* The data shall be consistent between util and class thus it is enough to read
	   				them from class */
					ret = pfe_class_get_feature(feature_mgr->class, &fw_feature_class, feature_name);
					if (EOK == ret)
					{
						ret = pfe_fw_feature_get_flags(fw_feature_class, &tmp);
						if (EOK == ret)
						{
							*val = (uint8_t)tmp & ((uint8_t)F_PRESENT | (uint8_t)F_RUNTIME);
						}
					}
				}
			}
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
	uint32_t    i;
	const char *fname;
	errno_t     ret = ENOENT;
	errno_t     ret_val = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		for (i = 0U; i < fmgr->hw_features_count; i++)
		{
			ret_val = pfe_hw_feature_get_name(fmgr->hw_features[i], &fname);
			if (ret_val == EOK)
			{
				if (0 == strcmp(fname, name))
				{
					*feature = fmgr->hw_features[i];
					ret = EOK;
					break;
				}
			}
		}
	}
	return ret;
}

/**
 * @brief Finds and returns the 1st HW feature by order of their discovery - used for listing all features
 * @param[in] fmgr The feature mgr instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
static errno_t pfe_hw_get_feature_first(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (fmgr->hw_features_count > 0U)
		{
			fmgr->current_hw_feature = 0U;
			*feature = fmgr->hw_features[fmgr->current_hw_feature];
			ret = EOK;
		}
		else
		{
			ret = ENOENT;
		}
	}

	return ret;
}

/**
 * @brief Finds and returns the next HW feature by order of their discovery - used for listing all features
 * @param[in] fmgr The feature_mgr instance
 * @param[out] feature Feature found (valid only if EOK is returned)
 * @return EOK when given entry is found, ENOENT when it is not found, error code otherwise
 */
static errno_t pfe_hw_get_feature_next(pfe_feature_mgr_t *fmgr, pfe_hw_feature_t **feature)
{
	errno_t ret = ENOENT;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fmgr) || (NULL == feature)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (fmgr->hw_features_count > 0U)
		{
			/* Avoid going out of the array boundaries */
			if ((fmgr->current_hw_feature + 1U) < fmgr->hw_features_count)
			{
				fmgr->current_hw_feature += 1U;
				*feature = fmgr->hw_features[fmgr->current_hw_feature];
				ret = EOK;
			}
		}
	}

	return ret;
}

/**
 * @brief Executes driver-side checks and configurations (if some are needed) in response to FW feature being enabled/disabled.
 * @param[in] feature_name Name of the feature to be set
 * @param[in] val Enable/disable value of the FW feture which was freshly set.
 * @return EOK or failure code.
 */
static errno_t pfe_feature_mgr_configure_driver(const char *feature_name, const uint8_t val)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		if (0 == strcmp(feature_name, "err051211_workaround"))
		{
			if (0U != val) /* feature got enabled */
			{
				ret = pfe_tmu_queue_err051211_sync(feature_mgr->tmu);
			}
		}
	}
	return ret;
}

static errno_t pfe_feature_mgr_table_parent_inst(const char *feature_name, pfe_fw_feature_t **feature)
{
	errno_t ret = EOK;
	bool_t class_parrent = TRUE;

	if(feature_name[0] == 'u' && feature_name[1] == '_')
	{
		feature_name += 2;
		class_parrent = FALSE;
	}

	if(TRUE == class_parrent)
	{
		if (NULL == feature_mgr->class)
		{ /* Class block is not initialized */
			ret = EINVAL;
		}
		else
		{
			ret = pfe_class_get_feature(feature_mgr->class, feature, feature_name);
		}
	}
	else
	{
		if (NULL == feature_mgr->util)
		{ /* Class block is not initialized */
			ret = EINVAL;
		}
		else
		{
			ret = pfe_util_get_feature(feature_mgr->util, feature, feature_name);
		}
	}

	return ret;
}

/**
 * @brief		Sets a value in the provided feature table element
 * @param[in]	feature_name Name of the feature to set the value
 * @param[in]	table_type In witch table the element is looked for
 * @param[in]	table_el_name Name of the table element to set the value
 * @param[in]	index Index of the value in the table
				index=0 means set the value on all table described by elemnt
				index > 0 means to set the value at a specific index witch
				start from 1.
 * @param[in]	val Value to be set
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_table_set_val(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t index, uint8_t* val)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_entry;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == val)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			switch (table_type)
			{
				case FW_FEATURE_TABLE_DEFAULT:
					ret = pfe_fw_feature_table_cfg_by_name(fw_feature, table_el_name, &fw_feature_table_entry);
					if (ENOENT == ret)
					{
						ret = pfe_fw_feature_table_stats_by_name(fw_feature, table_el_name, &fw_feature_table_entry);
					}
					break;
				case FW_FEATURE_TABLE_CONFIG:
					ret = pfe_fw_feature_table_cfg_by_name(fw_feature, table_el_name, &fw_feature_table_entry);
					break;
				case FW_FEATURE_TABLE_STATS:
					ret = pfe_fw_feature_table_stats_by_name(fw_feature, table_el_name, &fw_feature_table_entry);
					break;
				default:
					ret = EINVAL;
			}

			if (EOK == ret)
			{
				if (index == 0)
				{
					ret = pfe_fw_feature_table_entry_set(fw_feature_table_entry, (void *) val, pfe_fw_feature_table_entry_allocsize(fw_feature_table_entry));
				}
				else
				{
					ret = pfe_fw_feature_table_entry_set_by_idx(fw_feature_table_entry, (void *) val, index-1);
				}
			}
		}
	}

	return ret;
}

/**
 * @brief	Returns the 1st feature table stats element
 *			(resets the features table stts element query)
 * @param[in]	feature_name Name of the feature to be set.
 * @param[out]	Name of the 1st element.
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_stats_first(const char *feature_name, const char **table_el_name)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_stats;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{ 
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_stats_first(fw_feature, &fw_feature_table_stats);
			if (EOK == ret)
			{
				ret = pfe_fw_feature_table_entry_name(fw_feature_table_stats, table_el_name);
			}
		}
	}

	return ret;
}

/**
 * @brief	Returns the next feature element in stats table
 *			(continues the features table stats query)
 * @param[in]	feature_name Name of the feature to be set
 * @param[out]	Name of the next element.
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_stats_next(const char *feature_name, const char **table_el_name)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_stats;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_stats_next(fw_feature, &fw_feature_table_stats);
			if (EOK == ret)
			{
				ret = pfe_fw_feature_table_entry_name(fw_feature_table_stats, table_el_name);
			}
		}
	}

	return ret;
}

/**
 * @brief	Returns the 1st feature table config element 
 *			(resets the features table config element query)
 * @param[in]	feature_name Name of the feature to be set.
 * @param[out]	Name of the 1st element.
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_cfg_first(const char *feature_name, const char **table_el_name)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_cfg;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_cfg_first(fw_feature, &fw_feature_table_cfg);
			if (EOK == ret)
			{
				ret = pfe_fw_feature_table_entry_name(fw_feature_table_cfg, table_el_name);
			}
		}
	}

	return ret;
}

/**
 * @brief	Returns the next feature element in config table 
 * 			(continues the features table config query)
 * @param[in]	feature_name Name of the feature to be set
 * @param[out]	Name of the next element.
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_cfg_next(const char *feature_name, const char **table_el_name)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_cfg;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == *table_el_name)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_cfg_next(fw_feature, &fw_feature_table_cfg);
			if (EOK == ret)
			{
				ret = pfe_fw_feature_table_entry_name(fw_feature_table_cfg, table_el_name);
			}
		}
 	}

	return ret;
}

/**
 * @brief		Returns the 1st feature table element (resets the features table element query)
 * @param[in]	feature_name Name of the feature to be set
 * @param[in]	table_type In witch table the element is looked for
 * @param[out]	Name of the 1st element
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_table_first(const char *feature_name, uint8_t table_type, const char **table_el_name)
{
	errno_t ret = EOK;

	switch(table_type)
	{
		case FW_FEATURE_TABLE_DEFAULT:
			ret = pfe_feature_mgr_table_cfg_first(feature_name, table_el_name);
			table_rewind_flag = TRUE;
			if (EOK != ret)
			{
				ret = pfe_feature_mgr_table_stats_first(feature_name, table_el_name);
				table_rewind_flag = FALSE;
			}
			break;
		case FW_FEATURE_TABLE_CONFIG:
			ret = pfe_feature_mgr_table_cfg_first(feature_name, table_el_name);
			break;
		case FW_FEATURE_TABLE_STATS:
			ret = pfe_feature_mgr_table_stats_first(feature_name, table_el_name);
			break;
		default:
			ret = EINVAL;
	}

	return ret;
}

/**
 * @brief		Returns the next feature element (continues the features element query)
 * @param[in]	feature_name Name of the feature to be set
 * @param[in]	table_type In witch table the element is looked for
 * @param[out]	feature table name of the next element
 * @return		EOK or failure code.
 */

errno_t pfe_feature_mgr_table_next(const char *feature_name, uint8_t table_type, const char **table_el_name)
{
	errno_t ret = EOK;

	switch(table_type)
	{
		case FW_FEATURE_TABLE_DEFAULT:
			ret = pfe_feature_mgr_table_cfg_next(feature_name, table_el_name);
			if (ENOENT == ret)
			{
				if (TRUE == table_rewind_flag)
				{
					ret = pfe_feature_mgr_table_stats_first(feature_name, table_el_name);
					table_rewind_flag = FALSE;
				}
				else
				{
					ret = pfe_feature_mgr_table_stats_next(feature_name, table_el_name);
				}
			}
			break;
		case FW_FEATURE_TABLE_CONFIG:
			ret = pfe_feature_mgr_table_cfg_next(feature_name, table_el_name);
			break;
		case FW_FEATURE_TABLE_STATS:
			ret = pfe_feature_mgr_table_stats_next(feature_name, table_el_name);
			break;
		default:
			ret = EINVAL;
	}

	return ret;
}

/**
 * @brief		Reads the config table element size
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	count The read value of the table element size
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_cfg_get_size(const char *feature_name, const char *table_el_name, uint8_t *size)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_cfg;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == size)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_cfg_by_name(fw_feature, table_el_name, &fw_feature_table_cfg);
			if (EOK == ret)
			{
				*size = pfe_fw_feature_table_entry_size(fw_feature_table_cfg);
			}
		}
	}
	return ret;
}

/**
 * @brief		Reads the config table element multiplicity
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	count The read value of the table element multiplicity
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_cfg_get_multiplicity(const char *feature_name, const char *table_el_name, uint8_t *count)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_cfg;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == count)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_cfg_by_name(fw_feature, table_el_name, &fw_feature_table_cfg);
			if (EOK == ret)
			{
				*count = pfe_fw_feature_table_entry_multiplicity(fw_feature_table_cfg);
			}
		}
	}
	return ret;
}

/**
 * @brief		Reads the config table element payload
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	payload The read value of the table element payload
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_cfg_get_payload(const char *feature_name, const char *table_el_name, uint8_t *payload)
{

	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_cfg;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == payload)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_cfg_by_name(fw_feature, table_el_name, &fw_feature_table_cfg);
			if (EOK == ret)
			{
				pfe_fw_feature_table_entry_get(fw_feature_table_cfg, payload,
						pfe_fw_feature_table_entry_allocsize(fw_feature_table_cfg), FALSE);
			}
		}
	}
 	return ret;
}

/**
 * @brief		Reads the stats table element size
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	size The read value of the table element size
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_stats_get_size(const char *feature_name, const char *table_el_name, uint8_t *size)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_stats;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == size)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_stats_by_name(fw_feature, table_el_name, &fw_feature_table_stats);
			if (EOK == ret)
			{
				*size = pfe_fw_feature_table_entry_size(fw_feature_table_stats);
			}
		}
	}
	return ret;
}

/**
 * @brief		Reads the stats table element multiplicity
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	count The read value of the table element multiplicity
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_stats_get_multiplicity(const char *feature_name, const char *table_el_name, uint8_t *count)
{
	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_stats;
	errno_t				ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == count)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_stats_by_name(fw_feature, table_el_name, &fw_feature_table_stats);
			if (EOK == ret)
			{
				*count = pfe_fw_feature_table_entry_multiplicity(fw_feature_table_stats);
			}
		}
	}
	return ret;
}

/**
 * @brief		Reads the stats table element payload
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[out]	payload The read value of the table element payload
 * @return		EOK or failure code.
 */
static errno_t pfe_feature_mgr_table_stats_get_payload(const char *feature_name, const char *table_el_name, uint8_t *payload)
{

	pfe_fw_feature_t	*fw_feature;
	pfe_fw_tbl_handle_t	fw_feature_table_stats;
	errno_t			ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == payload)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		ret = pfe_feature_mgr_table_parent_inst(feature_name, &fw_feature);
		if (EOK == ret)
		{
			ret = pfe_fw_feature_table_stats_by_name(fw_feature, table_el_name, &fw_feature_table_stats);
			if (EOK == ret)
			{
				pfe_fw_feature_table_entry_get(fw_feature_table_stats, payload,
						pfe_fw_feature_table_entry_allocsize(fw_feature_table_stats), TRUE);
			}
		}
	}
	return ret;
}

/**
 * @brief		Reads the table element size
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]   table_el_name Name of the table element to be read
 * @param[in]   table_type In witch table the element is looked for
 * @param[out]	size The read value of the table element size
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_table_get_size(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *size)
{
	errno_t ret = EOK;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == size)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		switch(table_type)
		{
			case FW_FEATURE_TABLE_DEFAULT:
				ret = pfe_feature_mgr_table_cfg_get_size(feature_name, table_el_name, size);
				if (EOK != ret)
				{
					ret = pfe_feature_mgr_table_stats_get_size(feature_name, table_el_name, size);
				}
				break;
			case FW_FEATURE_TABLE_CONFIG:
				ret = pfe_feature_mgr_table_cfg_get_size(feature_name, table_el_name, size);
				break;
			case FW_FEATURE_TABLE_STATS:
				ret = pfe_feature_mgr_table_stats_get_size(feature_name, table_el_name, size);
				break;
			default:
				ret = EINVAL;
		}
	}
	return ret;
}

/**
 * @brief		Reads the table element multiplicity
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[in]	table_type In witch table the element is looked for
 * @param[out]	count The read value of the table element multiplicity
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_table_get_multiplicity(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *count)
{
	errno_t ret = EOK;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == count)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		switch(table_type)
		{
			case FW_FEATURE_TABLE_DEFAULT:
				ret = pfe_feature_mgr_table_cfg_get_multiplicity(feature_name, table_el_name, count);
				if (EOK != ret)
				{
					ret = pfe_feature_mgr_table_stats_get_multiplicity(feature_name, table_el_name, count);
				}
				break;
			case FW_FEATURE_TABLE_CONFIG:
				ret = pfe_feature_mgr_table_cfg_get_multiplicity(feature_name, table_el_name, count);
				break;
			case FW_FEATURE_TABLE_STATS:
				ret = pfe_feature_mgr_table_stats_get_multiplicity(feature_name, table_el_name, count);
				break;
			default:
				ret = EINVAL;
		}
	}
	return ret;
}

/**
 * @brief		Reads the table element payload
 * @param[in]	feature_name Name of the feature to be read
 * @param[in]	table_el_name Name of the table element to be read
 * @param[in]	table_type In witch table the element is looked for
 * @param[out]	payload The read value of the table element payload
 * @return		EOK or failure code.
 */
errno_t pfe_feature_mgr_table_get_payload(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *payload)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == feature_name || NULL == table_el_name || NULL == payload)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif
	{
		switch(table_type)
		{
			case FW_FEATURE_TABLE_DEFAULT:
				ret = pfe_feature_mgr_table_cfg_get_payload(feature_name, table_el_name, payload);
				if (EOK != ret)
				{
					ret = pfe_feature_mgr_table_stats_get_payload(feature_name, table_el_name, payload);
				}
				break;
			case FW_FEATURE_TABLE_CONFIG:
				ret = pfe_feature_mgr_table_cfg_get_payload(feature_name, table_el_name, payload);
				break;
			case FW_FEATURE_TABLE_STATS:
				ret = pfe_feature_mgr_table_stats_get_payload(feature_name, table_el_name, payload);
				break;
			default:
				ret = EINVAL;
		}
	}
	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

