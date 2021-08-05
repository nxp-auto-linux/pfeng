/* =========================================================================
 *  Copyright 2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_l2br.c
 * @brief		L2 bridge management functions.
 * @details		All bridge-related functionality provided by the FCI should be
 * 				implemented within this file. This includes mainly bridge-related
 * 				commands.
 *
 */

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_FCI_ENABLE

/**
 * @brief			Process FPP_CMD_L2_FLUSH_* commands
 * @param[in]		msg FCI cmd code
 * @param[out]		fci_ret FCI return code
 * @return			EOK if success, error code otherwise
 */
errno_t fci_l2br_flush_cmd(uint32_t code, uint16_t *fci_ret)
{
	fci_t *context = (fci_t *)&__context;
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
		case FPP_CMD_L2_FLUSH_ALL:
		{
			ret = pfe_l2br_flush_all(context->l2_bridge);
			if (EOK != ret)
			{
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				NXP_LOG_DEBUG("Can't flush MAC table entries: %d\n", ret);
			}

			break;
		}

		case FPP_CMD_L2_FLUSH_LEARNED:
		{
			ret = pfe_l2br_flush_learned(context->l2_bridge);
			if (EOK != ret)
			{
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				NXP_LOG_DEBUG("Can't flush learned MAC table entries: %d\n", ret);
			}
			
			break;
		}

		case FPP_CMD_L2_FLUSH_STATIC:
		{
			ret = pfe_l2br_flush_static(context->l2_bridge);
			if (EOK != ret)
			{
				*fci_ret = FPP_ERR_INTERNAL_FAILURE;
				NXP_LOG_DEBUG("Can't flush static MAC table entries: %d\n", ret);
			}
			
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown L2 bridge command: 0x%x\n", (uint_t)code);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}

	return ret;
}

#endif /* PFE_CFG_FCI_ENABLE */
