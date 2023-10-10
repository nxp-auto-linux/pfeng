/* =========================================================================
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "pfe_platform.h"
#include "fci_ownership_mask.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
#ifdef PFE_CFG_FCI_ENABLE

/**
 * @brief	This is the FCI ownership representation structure.
 */
typedef struct
{
        pfe_fci_owner_hif_id_t hif_fci_owner_chnls_mask;	/* Bit mask representing allowed FCI ownership */
        pfe_ct_phy_if_id_t lock_owner_if;		/* Current FCI owner lock holder: PFE_PHY_IF_ID_INVALID - no currewnt FCI owner or PFE_PHY_IF_ID_HIFn */
        oal_mutex_t fci_owner_mutex;			/* Mutex protecting the current FCI owner - lock_owner_if member */
} fci_owner_t;

static fci_owner_t fci_owner_context;

static errno_t fci_owner_lock_cmd(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret);
static errno_t fci_owner_unlock_cmd(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret);

/**
 * @brief		Initialize FCI ownership module
 * @param[in]	info Information with bit mask representing allowed FCI ownership
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_init(fci_init_info_t *info)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == info))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Grant FCI ownership to all if none is provided */
	fci_owner_context.hif_fci_owner_chnls_mask = (FCI_OWNER_HIF_INVALID == info->hif_fci_owner_chnls_mask) ?
		(pfe_fci_owner_hif_id_t)(FCI_OWNER_HIF_0 | FCI_OWNER_HIF_1 | FCI_OWNER_HIF_2 | FCI_OWNER_HIF_3 | FCI_OWNER_HIF_NOCPY) :
		(info->hif_fci_owner_chnls_mask);

	NXP_LOG_INFO("FCI ownership mask: 0x%X\n", fci_owner_context.hif_fci_owner_chnls_mask);
	/* Default FCI ownership holder. Beware of availability of OAL_PFE_CFG_MASTER_IF if it is needed here! */
	fci_owner_context.lock_owner_if = PFE_PHY_IF_ID_INVALID;
	ret = oal_mutex_init(&fci_owner_context.fci_owner_mutex);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Mutex initialization failed\n");
	}

	return ret;
}

/**
 * @brief		Deinitialize FCI ownership module
 */
void fci_owner_fini(void)
{
	if (EOK != oal_mutex_destroy(&fci_owner_context.fci_owner_mutex))
	{
		NXP_LOG_ERROR("Mutex destroy failed\n");
	}
}

/**
 * @brief		Process FCI owner lock/unlock commands
 * @details	Call must be protected by FCI owner mutex, it has to be done outside
 * @param[in]	sender HIF interface from where request is originated
 * @param[in]	msg FCI cmd code
 * @param[out]	fci_ret FCI return code
 * @return		EOK if success, error code otherwise
 */
errno_t fci_owner_session_cmd(pfe_ct_phy_if_id_t sender, uint32_t code, uint16_t *fci_ret)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fci_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*fci_ret = FPP_ERR_OK;

	switch (code)
	{
		case FPP_CMD_FCI_OWNERSHIP_LOCK:
		{
			ret = fci_owner_lock_cmd(sender, fci_ret);
			if (EOK != ret)
			{
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				NXP_LOG_WARNING("Can't get FCI lock for sender: %d error: %d\n", sender, ret);
			}

			break;
		}

		case FPP_CMD_FCI_OWNERSHIP_UNLOCK:
		{
			ret = fci_owner_unlock_cmd(sender, fci_ret);
			if (EOK != ret)
			{
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				NXP_LOG_WARNING("Can't release FCI lock for sender: %d error: %d\n", sender, ret);
			}

			break;
		}

		default:
		{
			NXP_LOG_WARNING("Unknown FCI lock/unlock command: 0x%x sender: %d\n", (uint_t)code, sender);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}

	return ret;
}

/**
 * @brief		Authorize FCI ownership request
 * @details	Call must be protected by FCI owner mutex, it has to be done outside
 * 			Authorize denotes sender corresponds to a current FCI ownership holder
 * @param[in]	sender identified by HIF interface
 * @param[out]	auth_ret status of FCI ownership request
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_authorize(pfe_ct_phy_if_id_t sender, bool_t *auth_ret)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == auth_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*auth_ret = (sender == fci_owner_context.lock_owner_if);
	}

	return ret;
}

/**
 * @brief		Get physical interface of sender
 * @details	Sender value is validated, it must correspond to a valid HIF
 * @param[in]	sender interface identification or 0 for Local Sender
 * @param[out]	phy_if_id sender's physical interface
 * @return	EOK if success, error code otherwise
 */
errno_t fci_sender_get_phy_if_id(uint32_t sender, pfe_ct_phy_if_id_t *phy_if_id)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == phy_if_id))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (sender)
		{
			case (uint32_t)PFE_PHY_IF_ID_HIF0:
			case (uint32_t)PFE_PHY_IF_ID_HIF1:
			case (uint32_t)PFE_PHY_IF_ID_HIF2:
			case (uint32_t)PFE_PHY_IF_ID_HIF3:
			case (uint32_t)PFE_PHY_IF_ID_HIF_NOCPY:
			{
				*phy_if_id = (pfe_ct_phy_if_id_t)sender;
				break;
			}

			default:
			{
				ret = EINVAL;
				break;
			}
		}
	}

	return ret;
}

/**
 * @brief		Lock FCI owner mutex
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_mutex_lock(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = oal_mutex_lock(&fci_owner_context.fci_owner_mutex);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Unlock FCI owner mutex
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_mutex_unlock(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = oal_mutex_unlock(&fci_owner_context.fci_owner_mutex);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Acquire FCI ownership
 * @details	Call must be protected by FCI owner mutex, it has to be done outside
 * @param[in]	sender HIF interface from where request is originated
 * @param[out]	fci_ret status of FCI ownership request
 * @return	EOK if success, error code otherwise
 */
static errno_t fci_owner_lock_cmd(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret)
{
	fci_t *fci_context = (fci_t *)&__context;
	pfe_fci_owner_hif_id_t chnl_bit_mask;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fci_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (sender == fci_owner_context.lock_owner_if)
		{
			*fci_ret = FPP_ERR_OK;
		}
		else
		{
			chnl_bit_mask = pfe_fci_owner_hif_from_phy_id(sender);
			if (FCI_OWNER_HIF_INVALID == chnl_bit_mask)
			{
				ret = EINVAL;
			}
			else
			{
				if (0U == (chnl_bit_mask & fci_owner_context.hif_fci_owner_chnls_mask))
				{
					*fci_ret = FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED;
				}
				else
				{
					if (PFE_PHY_IF_ID_INVALID == fci_owner_context.lock_owner_if)
					{
						fci_owner_context.lock_owner_if = sender;
						*fci_ret = FPP_ERR_OK;
					}
					else
					{
						*fci_ret = FPP_ERR_FCI_OWNERSHIP_ALREADY_LOCKED;
					}
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Acquire floating FCI ownership
 * @details	Call must be protected by FCI owner mutex, it has to be done outside.
 *			To be able to get the floating lock, there must not be another current lock owner
 *			and sender must have permission to get FCI owhership.
 *			Floating FCI ownership must be released after current FCI cmd is executed.
 * @param[in]	sender HIF interface from where request is originated
 * @param[out]	fci_ret status of FCI ownership request
 * @param[out]	floating_lock result if floating FCI ownership was granted
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_get_floating_lock(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret, bool_t *floating_lock)
{
	fci_t *fci_context = (fci_t *)&__context;
	pfe_fci_owner_hif_id_t chnl_bit_mask;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fci_ret) || (NULL == floating_lock)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (PFE_PHY_IF_ID_INVALID != fci_owner_context.lock_owner_if)
		{
			*fci_ret = FPP_ERR_FCI_OWNERSHIP_NOT_OWNER;
		}
		else
		{
			chnl_bit_mask = pfe_fci_owner_hif_from_phy_id(sender);
			if (FCI_OWNER_HIF_INVALID == chnl_bit_mask)
			{
				ret = EINVAL;
			}
			else
			{
				if (0U == (chnl_bit_mask & fci_owner_context.hif_fci_owner_chnls_mask))
				{
					*fci_ret = FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED;
				}
				else
				{
					fci_owner_context.lock_owner_if = sender;
					*fci_ret = FPP_ERR_OK;
					*floating_lock = TRUE;
				}
			}
		}
	}

	return ret;
}


/**
 * @brief		Release FCI ownership
 * @details	Call must be protected by FCI owner mutex, it has to be done outside
 * @param[in]	sender HIF interface from where request is originated
 * @param[out]	fci_ret status of FCI ownership release action
 * @return	EOK if success, error code otherwise
 */
static errno_t fci_owner_unlock_cmd(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fci_ret))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (sender == fci_owner_context.lock_owner_if)
		{
			fci_owner_context.lock_owner_if = PFE_PHY_IF_ID_INVALID;
			*fci_ret = FPP_ERR_OK;
		}
		else
		{
			*fci_ret = FPP_ERR_FCI_OWNERSHIP_NOT_OWNER;
		}
	}

	return ret;
}

/**
 * @brief		Clear floating FCI ownership lock
 * @details	Call must be protected by FCI owner mutex, it has to be done outside
 * @return	EOK if success, error code otherwise
 */
errno_t fci_owner_clear_floating_lock(void)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((FALSE == fci_context->fci_initialized) || (FALSE == fci_context->fci_owner_initialized)))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#else
	(void)fci_context;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		fci_owner_context.lock_owner_if = PFE_PHY_IF_ID_INVALID;
	}

	return ret;
}

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
