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
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 *
 * @file		pfe_platform.c
 * @brief		The PFE platform management (S32G master)
 * @details		This file contains HW-specific code. It is used to create structured SW representation
 *				of a given PFE HW implementation. File is intended to be created by user
 *				each time a new HW setup with different PFE configuration needs to be
 *				supported.
 * @note		Various variants of this file should exist for various HW implementations (please
 *				keep this file clean, not containing platform-specific preprocessor switches).
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "elf_cfg.h"
#include "elf.h"

#include "oal.h"
#include "hal.h"

#include "pfe_mmap.h"
#include "pfe_cbus.h"
#include "pfe_hif.h"
#include "pfe_platform_cfg.h"
#include "pfe_platform.h"
#include "pfe_ct.h"
#include "pfe_idex.h"
#include "pfe_platform_rpc.h" /* RPC codes and arguments */

/**
 * This is a platform specific file. All routines shall be implemented
 * according to application-given HW setup.
 */

/*
 * @brief The main PFE structure
 */
static pfe_platform_t pfe = {.probed = FALSE};

/**
 * @brief		BMU interrupt service routine
 * @details		Manage BMU interrupt
 * @details		See the oal_irq_handler_t
 */
static bool_t pfe_platform_bmu_isr(void *arg)
{
	pfe_bmu_t *bmu = (pfe_bmu_t *)arg;
	bool_t handled = FALSE;

	/* disable BMU interrups */
	pfe_bmu_irq_mask(bmu);

	/* Handle ISRs */
	if (EOK == pfe_bmu_isr(bmu))
	{
		handled = TRUE;
	}

	/* Re-enable interrups */
	pfe_bmu_irq_unmask(bmu);

	return handled;
}

/**
 * @brief		HIF channel interrupt service routine
 * @details		Manage HIF channel interrupt
 * @details		See the oal_irq_handler_t
 */
static bool_t pfe_platform_hif_chnl_isr(void *arg)
{
	pfe_hif_chnl_t *chnl = (pfe_hif_chnl_t *)arg;
	bool_t handled = FALSE;

	/* Disable HIF channel interrupts */
	pfe_hif_chnl_irq_mask(chnl);

	/* call HIF channel ISR */
	if (EOK == pfe_hif_chnl_isr(chnl))
	{
		handled = TRUE;
	}

	/* Re-nable HIF channel IRQ */
	pfe_hif_chnl_irq_unmask(chnl);

	return handled;
}

/**
 * @brief		HIF global error polling service routine
 * @details		Manage HIF global errors
 * @details		See the pfe_platform_hif_chnl_isr for HIF channel ISR
 */
static void *hif_global_err_poller_func(void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;

	if (NULL == platform)
	{
		NXP_LOG_WARNING("HIF global poller init failed\n");
		return NULL;
	}

	/* Very simple poller. It waits for hard coded delay, then
	   it invokes hif_isr() who checks the HIF global error
	   registers and process them if necessary
	*/

	while (TRUE)
	{
		switch (platform->poller_state)
		{
			case HIF_ERR_POLLER_STATE_DISABLED:
			{
				/*  Do nothing */
				break;
			}

			case HIF_ERR_POLLER_STATE_ENABLED:
			{
				/*  Process HIF global isr */
				pfe_hif_irq_mask(platform->hif);
				pfe_hif_isr(platform->hif);
				pfe_hif_irq_unmask(platform->hif);

				break;
			}

			case HIF_ERR_POLLER_STATE_STOPPED:
			{
				/*  Stop the loop and exit */
				NXP_LOG_WARNING("HIF global poller finished\n");
				return NULL;
			}

			default:
			{
				NXP_LOG_ERROR("Unexpected poller state\n");
				return NULL;
			}
		}

		/*  Wait for 1 sec and loop again */
		oal_time_mdelay(1000);
	}

	return NULL;
}

/**
 * @brief		Global interrupt service routine
 * @details		This must be here on platforms (FPGA...) where all PFE interrupts
 * 				are combined to a single physical interrupt line :(
 * 				Because we want to catch interrupts during platform initialization some
 * 				of platform modules might have not been initialized yet. Therefore the NULL
 * 				checks...
 * @details		See the oal_irq_handler_t
 */
static bool_t pfe_platform_global_isr(void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;
	bool_t handled = FALSE;
	uint32_t ii;
	static pfe_hif_chnl_id_t ids[] = {HIF_CHNL_0, HIF_CHNL_1, HIF_CHNL_2, HIF_CHNL_3};
	pfe_hif_chnl_t *chnls[sizeof(ids)/sizeof(pfe_hif_chnl_id_t)] = {NULL};

	/*	Disable all participating IRQ sources */
	if (NULL != platform->hif)
	{
		pfe_hif_irq_mask(platform->hif);
	}

	if (NULL != platform->bmu[0])
	{
		pfe_bmu_irq_mask(platform->bmu[0]);
	}

	if (NULL != platform->bmu[1])
	{
		pfe_bmu_irq_mask(platform->bmu[1]);
	}

	if (NULL != platform->hif)
	{
		for (ii=0U; ii<(sizeof(ids)/sizeof(pfe_hif_chnl_id_t)); ii++)
		{
			chnls[ii] = pfe_hif_get_channel(platform->hif, ids[ii]);
			if (NULL != chnls[ii])
			{
				pfe_hif_chnl_irq_mask(chnls[ii]);
			}
		}
	}

	if (NULL != platform->safety)
	{
		pfe_safety_irq_mask(platform->safety);
	}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	if (NULL != platform->hif_nocpy)
	{
		pfe_hif_chnl_irq_mask(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID));
	}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	/*	Call modules ISRs */
	if (NULL != platform->hif)
	{
		if (EOK == pfe_hif_isr(platform->hif))
		{
			handled = TRUE;
		}
	}

	if (NULL != platform->bmu[0])
	{
		if (EOK == pfe_bmu_isr(platform->bmu[0]))
		{
			handled = TRUE;
		}
	}

	if (NULL != platform->bmu[1])
	{
		if (EOK == pfe_bmu_isr(platform->bmu[1]))
		{
			handled = TRUE;
		}
	}

	for (ii=0U; ii<(sizeof(ids)/sizeof(pfe_hif_chnl_id_t)); ii++)
	{
		if (NULL != chnls[ii])
		{
			if (EOK == pfe_hif_chnl_isr(chnls[ii]))
			{
				handled = TRUE;
			}
		}
	}
	if (NULL != platform->safety)
	{
		if (EOK == pfe_safety_isr(platform->safety))
		{
			handled = TRUE;
		}
	}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	if (NULL != platform->hif_nocpy)
	{
		if (EOK == pfe_hif_chnl_isr(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID)))
		{
			handled = TRUE;
		}
	}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	/*	Enable all participating IRQ sources */
	if (NULL != platform->hif)
	{
		pfe_hif_irq_unmask(platform->hif);
	}

	if (NULL != platform->bmu[0])
	{
		pfe_bmu_irq_unmask(platform->bmu[0]);
	}

	if (NULL != platform->bmu[1])
	{
		pfe_bmu_irq_unmask(platform->bmu[1]);
	}

	for (ii=0U; ii<(sizeof(ids)/sizeof(pfe_hif_chnl_id_t)); ii++)
	{
		if (NULL != chnls[ii])
		{
			pfe_hif_chnl_irq_unmask(chnls[ii]);
		}
	}

	if (NULL != platform->safety)
	{
		pfe_safety_irq_unmask(platform->safety);
	}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	if (NULL != platform->hif_nocpy)
	{
		pfe_hif_chnl_irq_unmask(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID));
	}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	return handled;
}

/**
 * @brief		IDEX RPC callback
 * @details		All requests from slave drivers are coming and being processed
 *				within this callback. Any request policing should be implemented
 *				here.
 * @warning		Don't block or sleep within the body
 * @param[in]	id Request identifier
 * @param[in]	buf Pointer to request argument. Can be NULL.
 * @param[in]	buf_len Length of request argument. Can be zero.
 * @param[in]	arg Custom argument provided via pfe_idex_set_rpc_cbk()
 * @note		This callback runs in dedicated context/thread.
 */
static void idex_rpc_cbk(uint32_t id, void *buf, uint16_t buf_len, void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
	switch (id)
	{
		case PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE:
		{
			pfe_platform_rpc_pfe_phy_if_create_arg_t *req = (pfe_platform_rpc_pfe_phy_if_create_arg_t *)buf;

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE\n");

			/*	Right now the clients are only allowed to work with interfaces created by the master driver */
			if (NULL == pfe_platform_get_phy_if_by_id(platform, req->phy_if_id))
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = EOK;
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE:
		{
			pfe_platform_rpc_pfe_phy_if_enable_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_enable_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = pfe_phy_if_enable(phy_if);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE:
		{
			pfe_platform_rpc_pfe_phy_if_disable_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_disable_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = pfe_phy_if_disable(phy_if);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE:
		{
			pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = pfe_phy_if_promisc_enable(phy_if);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE:
		{
			pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = pfe_phy_if_promisc_disable(phy_if);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR:
		{
			pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);
			pfe_mac_addr_t mac_addr;

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				_ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg->mac_addr));
				memcpy(&mac_addr, arg->mac_addr, sizeof(pfe_mac_addr_t));
				ret = pfe_phy_if_add_mac_addr(phy_if, mac_addr);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR:
		{
			pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);
			pfe_mac_addr_t mac_addr;

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				_ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg->mac_addr));
				memcpy(&mac_addr, arg->mac_addr, sizeof(pfe_mac_addr_t));
				ret = pfe_phy_if_del_mac_addr(phy_if, mac_addr);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE:
		{
			pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				ret = pfe_phy_if_set_op_mode(phy_if, arg->op_mode);
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		case PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF:
		{
			pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t *arg = (pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t *)buf;
			pfe_phy_if_t *phy_if = pfe_platform_get_phy_if_by_id(platform, arg->phy_if_id);
			pfe_log_if_t *log_if = pfe_platform_get_log_if_by_id(platform, arg->log_if_id);

			NXP_LOG_DEBUG("RPC: PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF\n");

			if (NULL == phy_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else if (NULL == log_if)
			{
				/*	Instance does not exist */
				ret = ENOENT;
			}
			else
			{
				if (pfe_phy_if_has_log_if(phy_if, log_if))
				{
					ret = EOK;
				}
				else
				{
					ret = ENOENT;
				}
			}

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(ret, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}

		default:
		{
			NXP_LOG_WARNING("Unsupported RPC code: %d\n", id);

			/*	Report execution status to caller */
			if (EOK != pfe_idex_set_rpc_ret_val(EINVAL, NULL, 0U))
			{
				NXP_LOG_ERROR("Could not send RPC response\n");
			}

			break;
		}
	}

	return;
}

/**
 * @brief		Assign HIF to the platform
 */
static errno_t pfe_platform_create_hif(pfe_platform_t *platform, pfe_platform_config_t *config)
{
	uint32_t ii;
	static pfe_hif_chnl_id_t ids[HIF_CFG_MAX_CHANNELS] = {HIF_CHNL_0, HIF_CHNL_1, HIF_CHNL_2, HIF_CHNL_3};
	pfe_hif_chnl_t *chnl;

	platform->hif = pfe_hif_create(platform->cbus_baseaddr + CBUS_HIF_BASE_ADDR, config->hif_chnls_mask);
	if (NULL == platform->hif)
	{
		NXP_LOG_ERROR("Couldn't create HIF instance\n");
		return ENODEV;
	}

	if (FALSE == config->common_irq_mode)
	{
		/*	IRQ mode: per channel isr (S32G/VDK) */

		platform->irq_hif_chnls = oal_mm_malloc(sizeof(oal_irq_t *) * HIF_CFG_MAX_CHANNELS);
		if (NULL == platform->irq_hif_chnls)
		{
			NXP_LOG_ERROR("Could not create HIF IRQ storage\n");
			return ENOMEM;
		}

		platform->irq_hif_chnl_isr_handles = oal_mm_malloc(sizeof(oal_irq_isr_handle_t) * HIF_CFG_MAX_CHANNELS);
		if (NULL == platform->irq_hif_chnl_isr_handles)
		{
			NXP_LOG_ERROR("Could not create HIF IRQ handle storage\n");
			return ENOMEM;
		}

		for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
		{
			char_t irq_name[32];

			chnl = pfe_hif_get_channel(platform->hif, ids[ii]);
			if (NULL == chnl)
			{
				/* not requested HIF channel, skipping */
				continue;
			}

			if(0U == config->irq_vector_hif_chnls[ii])
			{
				/* misconfigured channel (requested in config, but irq not set),
				   so report and skip */
				NXP_LOG_WARNING("HIF channel %d is skipped, no IRQ configured\n", ii);
				continue;
			}

			oal_util_snprintf(irq_name, sizeof(irq_name), "PFE HIF%d IRQ", ii);
			platform->irq_hif_chnls[ii] = oal_irq_create(config->irq_vector_hif_chnls[ii], (oal_irq_flags_t)0, irq_name);
			if (NULL == pfe.irq_hif_chnls[ii])
			{
				NXP_LOG_ERROR("Could not create HIF%d IRQ\n", ii);
				return ENODEV;
			}

			if (EOK != oal_irq_add_handler(platform->irq_hif_chnls[ii], &pfe_platform_hif_chnl_isr, chnl, &platform->irq_hif_chnl_isr_handles[ii]))
			{
				NXP_LOG_ERROR("Could not add IRQ handler to the HIF%d\n", ii);
				return ENODEV;
			}

			/*	Now particular channel interrupt sources can be enabled */
			pfe_hif_chnl_irq_unmask(chnl);
		}

		/*	Create poller thread for HIF global (not per-channel) errors */
		platform->hif_global_err_poller = oal_thread_create(&hif_global_err_poller_func, platform->hif, "hif err poller", 0);
		if (NULL == platform->hif_global_err_poller)
		{
			NXP_LOG_ERROR("Couldn't start poller thread\n");
			return ENODEV;
		}

		/*   Enable poller */
		platform->poller_state = HIF_ERR_POLLER_STATE_ENABLED;
	}
	else /* config->common_irq_mode */
	{
		/*	IRQ mode: global isr (FPGA) */

		pfe_hif_irq_unmask(platform->hif);

		/*	Now particular channel interrupt sources can be enabled */
		for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
		{
			chnl = pfe_hif_get_channel(platform->hif, ids[ii]);
			if (NULL == chnl)
			{
				/* not requested HIF channel, skipping */
				continue;
			}

			pfe_hif_chnl_irq_unmask(chnl);
		}
	}

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif(pfe_platform_t *platform)
{
	uint32_t ii;
	pfe_hif_chnl_t *chnl;
	static pfe_hif_chnl_id_t ids[HIF_CFG_MAX_CHANNELS] = {HIF_CHNL_0 , HIF_CHNL_1, HIF_CHNL_2, HIF_CHNL_3};

	if (NULL != platform->hif)
	{
		for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
		{
			chnl = pfe_hif_get_channel(platform->hif, ids[ii]);
			if (NULL == chnl)
			{
				/* not used HIF channel, skipping */
				continue;
			}


			chnl = pfe_hif_get_channel(platform->hif, ids[ii]);
			pfe_hif_chnl_irq_mask(chnl);

			if (NULL != platform->irq_hif_chnls)
			{
				if (NULL != platform->irq_hif_chnls[ii])
				{
					oal_irq_destroy(platform->irq_hif_chnls[ii]);
					platform->irq_hif_chnls[ii] = NULL;
				}
			}
		}

		if(NULL != platform->irq_hif_chnl_isr_handles)
		{
			oal_mm_free(platform->irq_hif_chnl_isr_handles);
			platform->irq_hif_chnl_isr_handles = NULL;
		}

		if(NULL != platform->irq_hif_chnls)
		{
			oal_mm_free(platform->irq_hif_chnls);
			platform->irq_hif_chnls = NULL;
		}

		if (NULL != platform->hif_global_err_poller)
		{
			platform->poller_state = HIF_ERR_POLLER_STATE_STOPPED;

			oal_thread_join(platform->hif_global_err_poller, NULL);
			platform->hif_global_err_poller = NULL;
		}

		pfe_hif_destroy(platform->hif);
		platform->hif = NULL;
	}
}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)

/**
 * @brief		Assign HIF NOCPY to the platform
 */
static errno_t pfe_platform_create_hif_nocpy(pfe_platform_t *platform, pfe_platform_config_t *config)
{
	platform->hif_nocpy = pfe_hif_nocpy_create(pfe.cbus_baseaddr + CBUS_HIF_NOCPY_BASE_ADDR, platform->bmu[1]);

	if (NULL == platform->hif_nocpy)
	{
		NXP_LOG_ERROR("Couldn't create HIF NOCPY instance\n");
		return ENODEV;
	}

	if (FALSE == config->common_irq_mode)
	{
		/*	IRQ mode: per channel isr (S32G/VDK) */

		if (0U != config->irq_vector_hif_nocpy)
		{
				/* misconfigured channel (requested in config, but irq not set),
				   so report and exit */
				NXP_LOG_ERROR("HIF NOCPY has no IRQ configured\n");
				return ENODEV;
		}

		/*	HIF NOCPY interrupt handler */
		platform->irq_hif_nocpy = oal_irq_create(config->irq_vector_hif_nocpy, 0, "PFE HIF NOCPY IRQ");
		if (NULL == platform->irq_hif_nocpy)
		{
			NXP_LOG_ERROR("Could not create HIF NOCPY IRQ vector %d\n", config->irq_vector_hif_nocpy);
			return ENODEV;
		}
		else
		{
			if (EOK != oal_irq_add_handler(platform->irq_hif_nocpy, &pfe_platform_hif_chnl_isr,
									pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID),
									&platform->irq_hif_nocpy_isr_handle))
			{
				NXP_LOG_ERROR("Could not add IRQ handler for the BMU[0]\n");
				return ENODEV;
			}
		}
	}
	else /* config->common_irq_mode */
	{
		/*	IRQ mode: global isr (FPGA) */

		/* Note: used global isr, so do nothing here */
	}

	pfe_hif_chnl_irq_unmask(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID));

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif_nocpy(pfe_platform_t *platform)
{
	if (NULL != platform->hif_nocpy)
	{
		if (NULL != platform->irq_hif_nocpy)
		{
			oal_irq_destroy(platform->irq_hif_nocpy);
			platform->irq_hif_nocpy = NULL;
		}

		pfe_hif_nocpy_destroy(platform->hif_nocpy);
		platform->hif_nocpy = NULL;
	}
}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		Assign HIF driver(s) to the platform
 */
static errno_t pfe_platform_create_hif_drv(pfe_platform_t *platform)
{
	pfe_hif_chnl_t *channel;

	/*	Create HIF driver */
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	/*	Create HIF driver instance (HIF NOCPY) */
	channel = pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID);
#else
	/*	Create HIF driver instance according to configuration */
#if (PFE_CFG_LOCAL_PHY_IF_ID == PFE_PHY_IF_ID_HIF0)
	channel = pfe_hif_get_channel(platform->hif, HIF_CHNL_0);
#elif (PFE_CFG_LOCAL_PHY_IF_ID == PFE_PHY_IF_ID_HIF1)
	channel = pfe_hif_get_channel(platform->hif, HIF_CHNL_1);
#elif (PFE_CFG_LOCAL_PHY_IF_ID == PFE_PHY_IF_ID_HIF2)
	channel = pfe_hif_get_channel(platform->hif, HIF_CHNL_2);
#elif (PFE_CFG_LOCAL_PHY_IF_ID == PFE_PHY_IF_ID_HIF3)
	channel = pfe_hif_get_channel(platform->hif, HIF_CHNL_3);
#else
	#error Wrong channel configuration
#endif /* PFE_CFG_LOCAL_PHY_IF_ID */
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	if (NULL != channel)
	{
		platform->hif_drv = pfe_hif_drv_create(channel);
	}
	else
	{
		NXP_LOG_ERROR("Could not get HIF channel\n");
		return ENODEV;
	}

	if (EOK != pfe_hif_drv_init(platform->hif_drv))
	{
		NXP_LOG_ERROR("HIF driver initialization failed\n");
		return ENODEV;
	}

#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
	if (EOK != pfe_idex_init(platform->hif_drv))
	{
		NXP_LOG_ERROR("Can't initialize IDEX\n");
		return ENODEV;
	}
	else
	{
		if (EOK != pfe_idex_set_rpc_cbk(&idex_rpc_cbk, (void *)platform))
		{
			NXP_LOG_ERROR("Unable to set IDEX RPC callback\n");
			return ENODEV;
		}
	}
#endif

	return EOK;
}

/**
 * @brief		Release HIF driver(s)
 */
static void pfe_platform_destroy_hif_drv(pfe_platform_t *platform)
{
	if (NULL != platform->hif_drv)
	{
#ifdef GLOBAL_CFG_MULTI_INSTANCE_SUPPORT
		/*	Shut down IDEX */
		pfe_idex_fini();
#endif

		/*	Shut down HIF driver */
		pfe_hif_drv_destroy(platform->hif_drv);
		platform->hif_drv = NULL;
	}
}

/**
 * @brief		Get HIF driver instance
 * @param[in]	platform The platform instance
 * @param[in]	id The HIF driver ID (in case there are more drivers available)
 * @return		HIF driver instance or NULL if failed
 */
pfe_hif_drv_t *pfe_platform_get_hif_drv(pfe_platform_t *platform, uint32_t id)
{
	(void)id;
	return platform->hif_drv;
}

/**
 * @brief		Assign BMU to the platform
 */
static errno_t pfe_platform_create_bmu(pfe_platform_t *platform, pfe_platform_config_t *config)
{
	pfe_bmu_cfg_t bmu_cfg;

	/*	Create storage for instances */
	platform->bmu = oal_mm_malloc(platform->bmu_count * sizeof(pfe_bmu_t *));
	if (NULL == platform->bmu)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}

	/*	BMU1 - LMEM buffers */
	bmu_cfg.pool_pa = (void *)(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_LMEM_BASE_ADDR + PFE_MMAP_BMU1_LMEM_BASEADDR);
	NXP_LOG_INFO("BMU1 buffer base: p0x%p\n", bmu_cfg.pool_pa);
	bmu_cfg.max_buf_cnt = PFE_CFG_BMU1_BUF_COUNT;
	bmu_cfg.buf_size = PFE_CFG_BMU1_BUF_SIZE;
	bmu_cfg.bmu_ucast_thres = 0x200U;
	bmu_cfg.bmu_mcast_thres = 0x200U;
	bmu_cfg.int_mem_loc_cnt = 64U;
	bmu_cfg.buf_mem_loc_cnt = 64U;

	platform->bmu[0] = pfe_bmu_create(platform->cbus_baseaddr, (void *)CBUS_BMU1_BASE_ADDR, &bmu_cfg);
	if (NULL == platform->bmu[0])
	{
		NXP_LOG_ERROR("Couldn't create BMU1 instance\n");
		return ENODEV;
	}

	/*	BMU interrupt handling */
	if (FALSE == config->common_irq_mode)
	{
		/*	IRQ mode: per channel isr (S32G/VDK) */

		platform->irq_bmu = oal_irq_create(config->irq_vector_bmu, (oal_irq_flags_t)0, "PFE BMU IRQ");
		if (NULL == platform->irq_bmu)
		{
			NXP_LOG_ERROR("Could not create BMU IRQ vector %d\n", config->irq_vector_bmu);
			return ENODEV;
		}
		else
		{
			if (EOK != oal_irq_add_handler(platform->irq_bmu, &pfe_platform_bmu_isr, platform->bmu[0], &pfe.irq_bmu_isr_handle))
			{
				NXP_LOG_ERROR("Could not add IRQ handler for the BMU[0]\n");
				return ENODEV;
			}
		}
	}
	else /* config->common_irq_mode */
	{
		/*	IRQ mode: global isr (FPGA) */

		/* Note: used global isr, so do nothing here */
	}

	pfe_bmu_irq_unmask(platform->bmu[0]);

	if (2U > platform->bmu_count)
	{
			NXP_LOG_WARNING("Only single BMU was configured.\n");
			return EOK;
	}

	/*	BMU2 - DDR buffers */
	{
		/*
			Due to RTL bug the BMU2 base address must be 2k aligned.
 	 		TRM also says that the base shall be aligned to BUF_COUNT * BUF_SIZE.

			UPDATE: When not aligned to BUF_COUNT * BUF_SIZE the BMU is reporting buffer free errors.
		*/
		platform->bmu_buffers_size = PFE_CFG_BMU2_BUF_COUNT * (1U << PFE_CFG_BMU2_BUF_SIZE);
		platform->bmu_buffers_va = oal_mm_malloc_contig_named_aligned_nocache("pfe_ddr", platform->bmu_buffers_size, platform->bmu_buffers_size);
		if (NULL == platform->bmu_buffers_va)
		{
			NXP_LOG_ERROR("Unable to get BMU2 pool memory\n");
			return ENOMEM;
		}

		bmu_cfg.pool_va = platform->bmu_buffers_va;
		bmu_cfg.pool_pa = oal_mm_virt_to_phys(platform->bmu_buffers_va);
		NXP_LOG_INFO("BMU2 buffer base: p0x%p (%"PRINTADDR_T" bytes)\n", bmu_cfg.pool_pa, platform->bmu_buffers_size);
	}

	bmu_cfg.max_buf_cnt = PFE_CFG_BMU2_BUF_COUNT;
	bmu_cfg.buf_size = PFE_CFG_BMU2_BUF_SIZE;
	bmu_cfg.bmu_ucast_thres = 0x800U;
	bmu_cfg.bmu_mcast_thres = 0x200U;
	bmu_cfg.int_mem_loc_cnt = 1024U;
	bmu_cfg.buf_mem_loc_cnt = 1024U;

	platform->bmu[1] = pfe_bmu_create(platform->cbus_baseaddr, (void *)CBUS_BMU2_BASE_ADDR, &bmu_cfg);

	if (NULL == platform->bmu[1])
	{
		NXP_LOG_ERROR("Couldn't create BMU2 instance\n");
		return ENODEV;
	}

	if (FALSE == config->common_irq_mode)
	{
		/*	IRQ mode: per channel isr (S32G/VDK) */

		if (EOK != oal_irq_add_handler(platform->irq_bmu, &pfe_platform_bmu_isr, platform->bmu[1], &pfe.irq_bmu_isr_handle))
		{
			NXP_LOG_ERROR("Could not add IRQ handler for the BMU[1]\n");
			return ENODEV;
		}
	}
	else /* config->common_irq_mode */
	{
		/*	IRQ mode: global isr (FPGA) */

		/* Note: used global isr, so do nothing here */
	}

	pfe_bmu_irq_unmask(platform->bmu[1]);

	return EOK;
}

/**
 * @brief		Release BMU-related resources
 */
static void pfe_platform_destroy_bmu(pfe_platform_t *platform)
{
	uint32_t ii;

	if (NULL != platform->bmu)
	{
		if (NULL != platform->irq_bmu)
		{
			oal_irq_destroy(platform->irq_bmu);
			platform->irq_bmu = NULL;
		}

		for (ii=0; ii<pfe.bmu_count; ii++)
		{
			if (platform->bmu[ii])
			{
				pfe_bmu_destroy(platform->bmu[ii]);
				platform->bmu[ii] = NULL;
			}
		}

		oal_mm_free(platform->bmu);
		platform->bmu = NULL;
	}

	if (NULL != platform->bmu_buffers_va)
	{
		oal_mm_free_contig(platform->bmu_buffers_va);
		platform->bmu_buffers_va = NULL;
	}
}

/**
 * @brief		Assign GPI to the platform
 */
static errno_t pfe_platform_create_gpi(pfe_platform_t *platform)
{
	pfe_gpi_cfg_t gpi_cfg;

	/*	Create storage for instances */
	platform->gpi = oal_mm_malloc(platform->gpi_count * sizeof(pfe_gpi_t *));
	if (NULL == platform->gpi)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}

	/*	GPI1 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0x178U;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->gpi[0] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_EGPI1_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->gpi[0])
	{
		NXP_LOG_ERROR("Couldn't create GPI1 instance\n");
		return ENODEV;
	}

	/*	GPI2 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0x178U;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->gpi[1] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_EGPI2_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->gpi[1])
	{
		NXP_LOG_ERROR("Couldn't create GPI2 instance\n");
		return ENODEV;
	}

	/*	GPI3 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0x178U;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->gpi[2] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_EGPI3_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->gpi[2])
	{
		NXP_LOG_ERROR("Couldn't create GPI3 instance\n");
		return ENODEV;
	}

	return EOK;
}

/**
 * @brief		Release GPI-related resources
 */
static void pfe_platform_destroy_gpi(pfe_platform_t *platform)
{
	uint32_t ii;

	if (NULL != platform->gpi)
	{
		for (ii=0U; ii<platform->gpi_count; ii++)
		{
			if (NULL != platform->gpi[ii])
			{
				pfe_gpi_destroy(platform->gpi[ii]);
				platform->gpi[ii] = NULL;
			}
		}

		oal_mm_free(platform->gpi);
		platform->gpi = NULL;
	}
}

/**
 * @brief		Assign ETGPI to the platform
 */
static errno_t pfe_platform_create_etgpi(pfe_platform_t *platform)
{
	pfe_gpi_cfg_t gpi_cfg;

	/*	Create storage for instances */
	platform->etgpi = oal_mm_malloc(platform->etgpi_count * sizeof(pfe_gpi_t *));
	if (NULL == platform->etgpi)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}

	/*	ETGPI1 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0xbcU;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->etgpi[0] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_ETGPI1_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->etgpi[0])
	{
		NXP_LOG_ERROR("Couldn't create ETGPI1 instance\n");
		return ENODEV;
	}

	/*	ETGPI2 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0xbcU;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->etgpi[1] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_ETGPI2_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->etgpi[1])
	{
		NXP_LOG_ERROR("Couldn't create ETGPI2 instance\n");
		return ENODEV;
	}

	/*	ETGPI3 */
	gpi_cfg.alloc_retry_cycles = 0x200U;
	gpi_cfg.gpi_tmlf_txthres = 0xbcU;
	gpi_cfg.gpi_dtx_aseq_len = 0x40; /* AAVB-2028 - DTX_ASEQ_LEN shall be 0x40 to have HW CSUMs working */
	gpi_cfg.emac_1588_ts_en = TRUE;

	platform->etgpi[2] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_ETGPI3_BASE_ADDR, &gpi_cfg);
	if (NULL == platform->etgpi[2])
	{
		NXP_LOG_ERROR("Couldn't create ETGPI3 instance\n");
		return ENODEV;
	}

	return EOK;
}

/**
 * @brief		Release ETGPI-related resources
 */
static void pfe_platform_destroy_etgpi(pfe_platform_t *platform)
{
	uint32_t ii;

	if (NULL != platform->etgpi)
	{
		for (ii=0U; ii<platform->etgpi_count; ii++)
		{
			if (platform->etgpi[ii])
			{
				pfe_gpi_destroy(platform->etgpi[ii]);
				platform->etgpi[ii] = NULL;
			}
		}

		oal_mm_free(platform->etgpi);
		platform->etgpi = NULL;
	}
}

/**
 * @brief		Assign HGPI to the platform
 */
static errno_t pfe_platform_create_hgpi(pfe_platform_t *platform)
{
	pfe_gpi_cfg_t hgpi_cfg;

	/*	Create storage for instances */
	platform->hgpi = oal_mm_malloc(platform->hgpi_count * sizeof(pfe_gpi_t *));
	if (NULL == platform->hgpi)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}

	hgpi_cfg.alloc_retry_cycles = 0x200U;
	hgpi_cfg.gpi_tmlf_txthres = 0x178U;
	hgpi_cfg.gpi_dtx_aseq_len = HGPI_ASEQ_LEN;
	hgpi_cfg.emac_1588_ts_en = FALSE;

	platform->hgpi[0] = pfe_gpi_create(platform->cbus_baseaddr, (void *)CBUS_HGPI_BASE_ADDR, &hgpi_cfg);
	if (NULL == platform->hgpi[0])
	{
		NXP_LOG_ERROR("Couldn't create HGPI instance\n");
		return ENODEV;
	}

	return EOK;
}

/**
 * @brief		Release GPI-related resources
 */
static void pfe_platform_destroy_hgpi(pfe_platform_t *platform)
{
	uint32_t ii;

	if (NULL != platform->hgpi)
	{
		for (ii=0U; ii<platform->hgpi_count; ii++)
		{
			if (NULL != platform->hgpi[ii])
			{
				pfe_gpi_destroy(platform->hgpi[ii]);
				platform->hgpi[ii] = NULL;
			}
		}

		oal_mm_free(platform->hgpi);
		platform->hgpi = NULL;
	}
}

/**
 * @brief		Assign CLASS to the platform
 */
static errno_t pfe_platform_create_class(pfe_platform_t *platform)
{
	uint32_t ret;
	pfe_class_cfg_t class_cfg =
	{
		.resume = FALSE,
		.toe_mode = FALSE,
		.pe_sys_clk_ratio = PFE_CFG_CLMODE,
		.pkt_parse_offset = 6U, /* This is actually the sizeof(struct hif_hdr) to skip the HIF header */
	};

	ELF_File_t elf;
	uint8_t *temp;

	if (NULL == platform->fw)
	{
		NXP_LOG_ERROR("The CLASS firmware is NULL\n");
	}

	if ((NULL == platform->fw->class_data) || (0U == platform->fw->class_size))
	{
		NXP_LOG_ERROR("The CLASS firmware is not loaded\n");
		return EIO;
	}

	platform->classifier = pfe_class_create(platform->cbus_baseaddr, platform->class_pe_count, &class_cfg);

	if (NULL == platform->classifier)
	{
		NXP_LOG_ERROR("Couldn't create classifier instance\n");
		return ENODEV;
	}
	else
	{
		/*	Here we need to initialize data structures within the DMEM */
					/*	TODO */
	#if 0
		class_fw_init(platform->classifier, platform->cbus_baseaddr);
	#endif /* 0 */

		temp = (uint8_t *)platform->fw->class_data;

		if ((temp[0] == 0x7fU) &&
			(temp[1] == 'E') &&
			(temp[2] == 'L') &&
			(temp[3] == 'F'))
		{
			/*	FW is ELF file */
			NXP_LOG_INFO("Firmware .elf detected\n");

			if (FALSE == ELF_Open(&elf, platform->fw->class_data, platform->fw->class_size))
			{
				NXP_LOG_ERROR("Can't parse CLASS firmware\n");
				return EIO;
			}
			else
			{
				NXP_LOG_INFO("Uploading CLASS firmware\n");
				ret = pfe_class_load_firmware(platform->classifier, &elf);

				ELF_Close(&elf);

				if (EOK != ret)
				{
					NXP_LOG_ERROR("Error during upload of CLASS firmware: %d\n", ret);
					return EIO;
				}
			}
		}
		else
		{
			NXP_LOG_ERROR("Only ELF format is supported\n");
            return ENODEV;
		}
	}

 	return EOK;
}

/**
 * @brief		Release CLASS-related resources
 */
static void pfe_platform_destroy_class(pfe_platform_t *platform)
{
	if (NULL != platform->classifier)
	{
		pfe_class_destroy(platform->classifier);
		platform->classifier = NULL;
	}
}

/**
 * @brief		Assign L2 Bridge to the platform
 */
static errno_t pfe_platform_create_l2_bridge(pfe_platform_t *platform)
{
	platform->mactab = pfe_l2br_table_create(platform->cbus_baseaddr, PFE_L2BR_TABLE_MAC2F);
	if (NULL == platform->mactab)
	{
		NXP_LOG_ERROR("Couldn't create MAC table instance\n");
		return ENODEV;
	}

	platform->vlantab = pfe_l2br_table_create(platform->cbus_baseaddr, PFE_L2BR_TABLE_VLAN);
	if (NULL == platform->vlantab)
	{
		NXP_LOG_ERROR("Couldn't create VLAN table instance\n");
		return ENODEV;
	}

	platform->l2_bridge = pfe_l2br_create(platform->classifier, 1U, platform->mactab, platform->vlantab);
	if (NULL == platform->l2_bridge)
	{
		NXP_LOG_ERROR("Could not create L2 Bridge\n");
		return ENODEV;
	}

	return EOK;
}

/**
 * @brief		Release L2 Bridge-related resources
 */
static void pfe_platform_destroy_l2_bridge(pfe_platform_t *platform)
{
	if (NULL != platform->l2_bridge)
	{
		pfe_l2br_destroy(platform->l2_bridge);
		platform->l2_bridge = NULL;
	}

	if (NULL != platform->mactab)
	{
		pfe_l2br_table_destroy(platform->mactab);
		platform->mactab = NULL;
	}

	if (NULL != platform->vlantab)
	{
		pfe_l2br_table_destroy(platform->vlantab);
		platform->vlantab = NULL;
	}
}

/**
 * @brief		Assign Routing Table to the platform
 */
static errno_t pfe_platform_create_rtable(pfe_platform_t *platform)
{
	void *htable_mem;
	void *pool_mem;

	platform->rtable_size = 2U * 256U * pfe_rtable_get_entry_size();
	platform->rtable_va = oal_mm_malloc_contig_named_aligned_nocache("pfe_ddr", platform->rtable_size, 2048U);
	if (NULL == platform->rtable_va)
	{
		NXP_LOG_ERROR("Unable to get routing table memory\n");
		return ENOMEM;
	}

	htable_mem = platform->rtable_va;
	pool_mem = (void *)((addr_t)platform->rtable_va + (256U * pfe_rtable_get_entry_size()));

	if (NULL == platform->classifier)
	{
		NXP_LOG_ERROR("Valid classifier instance required\n");
		return ENODEV;
	}

	platform->rtable = pfe_rtable_create(platform->classifier, oal_mm_virt_to_phys(htable_mem), 256U, oal_mm_virt_to_phys(pool_mem), 256U);

	if (NULL == platform->rtable)
	{
		NXP_LOG_ERROR("Couldn't create routing table instance\n");
		return ENODEV;
	}
	else
	{
		NXP_LOG_INFO("Routing table created, Hash Table @ p0x%p, Pool @ p0x%p (%d bytes)\n", oal_mm_virt_to_phys(htable_mem), oal_mm_virt_to_phys(pool_mem), (uint32_t)platform->rtable_size);
	}

	return EOK;
}

/**
 * @brief		Release Routing table-related resources
 */
static void pfe_platform_destroy_rtable(pfe_platform_t *platform)
{
	if (NULL != platform->rtable)
	{
		pfe_rtable_destroy(platform->rtable);
		platform->rtable = NULL;
	}

	if (NULL != platform->rtable_va)
	{
		oal_mm_free_contig(platform->rtable_va);
		platform->rtable_va = NULL;
	}
}

/**
 * @brief		Assign TMU to the platform
 */
static errno_t pfe_platform_create_tmu(pfe_platform_t *platform)
{
	pfe_tmu_cfg_t tmu_cfg =
	{
		.pe_sys_clk_ratio = PFE_CFG_CLMODE,
	};

	platform->tmu = pfe_tmu_create(platform->cbus_baseaddr, platform->tmu_pe_count, &tmu_cfg);

	if (NULL == platform->tmu)
	{
		NXP_LOG_ERROR("Couldn't create TMU instance\n");
		return ENODEV;
	}

	return EOK;
}

/**
 * @brief		Release TMU-related resources
 */
static void pfe_platform_destroy_tmu(pfe_platform_t *platform)
{
	if (NULL != platform->tmu)
	{
		pfe_tmu_destroy(platform->tmu);
		platform->tmu = NULL;
	}
}

#if 0 /* For future use */
/**
 * @brief		Assign UTIL to the platform
 */
static errno_t pfe_platform_create_util(pfe_platform_t *platform)
{
	errno_t ret;
	pfe_util_cfg_t util_cfg =
	{
		.pe_sys_clk_ratio = PFE_CFG_CLMODE,
	};

	platform->util = pfe_util_create(platform->cbus_baseaddr, platform->util_pe_count, &util_cfg);

	if (NULL == platform->util)
	{
		NXP_LOG_ERROR("Couldn't create UTIL instance\n");
		return ENODEV;
	}
	else
	{
		ELF_File_t elf;

		if (NULL == platform->fw->util_data || 0 == platform->fw->util_size)
		{
			NXP_LOG_ERROR("The UTIL firmware is not loaded\n");
			return EIO;
		}

		if (FALSE == ELF_Open(&elf, platform->fw->util_data, platform->fw->util_size))
		{
			NXP_LOG_ERROR("Can't parse UTIL firmware\n");
			return EIO;
		}
		else
		{
			NXP_LOG_INFO("Uploading UTIL firmware\n");
			ret = pfe_util_load_firmware(platform->util, &elf);

			ELF_Close(&elf);

			if (EOK != ret)
			{
				NXP_LOG_ERROR("Error during upload of UTIL firmware: %d\n", ret);
				return EIO;
			}
		}
	}

	return EOK;
}
#endif /* 0 */

/**
 * @brief		Release UTIL-related resources
 */
static void pfe_platform_destroy_util(pfe_platform_t *platform)
{
	if (NULL != platform->util)
	{
		pfe_util_destroy(platform->util);
		platform->util = NULL;
	}
}

/**
 * @brief		Assign EMAC to the platform
 */
static errno_t pfe_platform_create_emac(pfe_platform_t *platform)
{
	/*	Create storage for instances */
	platform->emac = oal_mm_malloc(platform->emac_count * sizeof(pfe_emac_t *));
	if (NULL == platform->emac)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}

	/*	EMAC1 */
	platform->emac[0] = pfe_emac_create(platform->cbus_baseaddr, (void *)CBUS_EMAC1_BASE_ADDR, EMAC_MODE_SGMII, EMAC_SPEED_100_MBPS, EMAC_DUPLEX_FULL);
	if (NULL == platform->emac[0])
	{
		NXP_LOG_ERROR("Couldn't create EMAC1 instance\n");
		return ENODEV;
	}
	else
	{
		pfe_emac_set_max_frame_length(platform->emac[0], 1522);
		pfe_emac_enable_flow_control(platform->emac[0]);
		pfe_emac_enable_broadcast(platform->emac[0]);

		/*	MAC address will be added with phy/log interface */
	}

	/*	EMAC2 */
	platform->emac[1] = pfe_emac_create(platform->cbus_baseaddr, (void *)CBUS_EMAC2_BASE_ADDR, EMAC_MODE_SGMII, EMAC_SPEED_100_MBPS, EMAC_DUPLEX_FULL);
	if (NULL == platform->emac[1])
	{
		NXP_LOG_ERROR("Couldn't create EMAC2 instance\n");
		return ENODEV;
	}
	else
	{
		pfe_emac_set_max_frame_length(platform->emac[1], 1522);
		pfe_emac_enable_flow_control(platform->emac[1]);
		pfe_emac_enable_broadcast(platform->emac[1]);

		/*	MAC address will be added with phy/log interface */
	}

	/*	EMAC3 */
	platform->emac[2] = pfe_emac_create(platform->cbus_baseaddr, (void *)CBUS_EMAC3_BASE_ADDR, EMAC_MODE_SGMII, EMAC_SPEED_100_MBPS, EMAC_DUPLEX_FULL);
	if (NULL == platform->emac[2])
	{
		NXP_LOG_ERROR("Couldn't create EMAC3 instance\n");
		return ENODEV;
	}
	else
	{
		pfe_emac_set_max_frame_length(platform->emac[2], 1522);
		pfe_emac_enable_flow_control(platform->emac[2]);
		pfe_emac_enable_broadcast(platform->emac[2]);

		/*	MAC address will be added with phy/log interface */
	}

	return EOK;
}

/**
 * @brief		Release EMAC-related resources
 */
static void pfe_platform_destroy_emac(pfe_platform_t *platform)
{
	uint32_t ii;

	if (NULL != platform->emac)
	{
		for (ii=0U; ii<platform->emac_count; ii++)
		{
			if (NULL != platform->emac[ii])
			{
				pfe_emac_destroy(platform->emac[ii]);
				platform->emac[ii] = NULL;
			}
		}

		oal_mm_free(platform->emac);
		platform->emac = NULL;
	}
}

/**
 * @brief		Assign SAFETY to the platform
 */
static errno_t pfe_platform_create_safety(pfe_platform_t *platform)
{
	platform->safety = pfe_safety_create(platform->cbus_baseaddr, (void *)CBUS_GLOBAL_CSR_BASE_ADDR);

	if (NULL == platform->safety)
	{
		NXP_LOG_ERROR("Couldn't create SAFETY instance\n");
		return ENODEV;
	}
	else
	{
		NXP_LOG_INFO("SAFETY instance created\n");
	}

	return EOK;
}

/**
 * @brief		Release SAFETY-related resources
 */
static void pfe_platform_destroy_safety(pfe_platform_t *platform)
{
	if (NULL != platform->safety)
	{
		pfe_safety_destroy(platform->safety);
		platform->safety = NULL;
	}
}

/**
 * @brief		Register logical interface
 * @details		Function will crate mapping table between logical interface IDs and
 * 				instances and add the logical interface instance with various validity
 * 				checks.
 */
static errno_t pfe_platform_register_log_if(pfe_platform_t *platform, pfe_log_if_t *log_if)
{
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == platform) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("Null argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Owner of the interface is local driver instance */
	ret = pfe_if_db_add(platform->log_if_db, log_if, PFE_CFG_LOCAL_PHY_IF_ID);

	if (EOK != oal_mutex_unlock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Register physical interface
 * @details		Function will crate mapping table between physical interface IDs and
 * 				instances and add the physical interface instance with various validity
 * 				checks.
 */
static errno_t pfe_platform_register_phy_if(pfe_platform_t *platform, pfe_phy_if_t *phy_if)
{
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == platform) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("Null argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&platform->phy_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Owner of the interface is local driver instance */
	ret = pfe_if_db_add(platform->phy_if_db, phy_if, PFE_CFG_LOCAL_PHY_IF_ID);

	if (EOK != oal_mutex_unlock(&platform->phy_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get logical interface by its ID
 * @param[in]	platform Platform instance
 * @param[in]	id Logical interface ID. See pfe_log_if_t.
 * @return		Logical interface instance or NULL if failed.
 */
pfe_log_if_t *pfe_platform_get_log_if_by_id(pfe_platform_t *platform, uint8_t id)
{
	pfe_if_db_entry_t *entry = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}

	if (unlikely(NULL == platform->log_if_db))
	{
		NXP_LOG_ERROR("Logical interface DB not found\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	entry = pfe_if_db_get_first(platform->log_if_db, IF_DB_CRIT_BY_ID, (void *)(addr_t)id);

	if (EOK != oal_mutex_unlock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return pfe_if_db_entry_get_log_if(entry);
}

/**
 * @brief		Get physical interface by its ID
 * @param[in]	platform Platform instance
 * @param[in]	id Physical interface ID
 * @return		Logical interface instance or NULL if failed.
 */
pfe_phy_if_t *pfe_platform_get_phy_if_by_id(pfe_platform_t *platform, pfe_ct_phy_if_id_t id)
{
	pfe_if_db_entry_t *entry = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}

	if (unlikely(NULL == platform->phy_if_db))
	{
		NXP_LOG_ERROR("Physical interface DB not found\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&platform->phy_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	entry = pfe_if_db_get_first(platform->phy_if_db, IF_DB_CRIT_BY_ID, (void *)(addr_t)id);

	if (EOK != oal_mutex_unlock(&platform->phy_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return pfe_if_db_entry_get_phy_if(entry);
}

/**
 * @brief		Assign interfaces to the platform.
 */
static errno_t pfe_platform_create_ifaces(pfe_platform_t *platform)
{
	int ii;
	uint32_t log_if_id = 0U;
	pfe_phy_if_t *phy_if = NULL;
	pfe_log_if_t *log_if = NULL;
	char_t if_name[16] = {0};
	errno_t ret = EOK;
	pfe_if_db_entry_t *entry;

	struct
	{
		char_t *name;
		pfe_ct_phy_if_id_t id;
		pfe_mac_addr_t mac;
		union
		{
			pfe_emac_t *emac;
			pfe_hif_chnl_t *chnl;
		};
	} phy_ifs[] =
	{
			{"emac0", PFE_PHY_IF_ID_EMAC0, GEMAC0_MAC, {platform->emac[0]}},
			{"emac1", PFE_PHY_IF_ID_EMAC1, GEMAC1_MAC, {platform->emac[1]}},
			{"emac2", PFE_PHY_IF_ID_EMAC2, GEMAC2_MAC, {platform->emac[2]}},
			{"_hif", PFE_PHY_IF_ID_HIF, {0, 0, 0, 0, 0, 0}, {NULL}},
			{"hncpy", PFE_PHY_IF_ID_HIF_NOCPY, {0, 0, 0, 0, 0, 0}, {NULL}},
			{"hif0", PFE_PHY_IF_ID_HIF0, {0, 0, 0, 0, 0, 0}, {NULL}},
			{"hif1", PFE_PHY_IF_ID_HIF1, {0, 0, 0, 0, 0, 0}, {NULL}},
			{"hif2", PFE_PHY_IF_ID_HIF2, {0, 0, 0, 0, 0, 0}, {NULL}},
			{"hif3", PFE_PHY_IF_ID_HIF3, {0, 0, 0, 0, 0, 0}, {NULL}},
			{NULL, PFE_PHY_IF_ID_INVALID, {0, 0, 0, 0, 0, 0}, {NULL}}
	};
	phy_ifs[0].emac = platform->emac[0];
	phy_ifs[1].emac = platform->emac[1];
	phy_ifs[2].emac = platform->emac[2];
	phy_ifs[3].chnl = pfe_hif_get_channel(platform->hif, HIF_CHNL_0);
	phy_ifs[4].chnl = pfe_hif_get_channel(platform->hif, HIF_CHNL_1);
	phy_ifs[5].chnl = pfe_hif_get_channel(platform->hif, HIF_CHNL_2);
	phy_ifs[6].chnl = pfe_hif_get_channel(platform->hif, HIF_CHNL_3);
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	phy_ifs[7].chnl = pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID);
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	/*	Create interfaces */
	for (ii=0; phy_ifs[ii].id!=PFE_PHY_IF_ID_INVALID; ii++)
	{
		/*	Create physical IF */
		phy_if = pfe_phy_if_create(platform->classifier, phy_ifs[ii].id, phy_ifs[ii].name);
		if (NULL == phy_if)
		{
			NXP_LOG_ERROR("Couldn't create %s\n", phy_ifs[ii].name);
			return ENODEV;
		}
		else
		{
			/*	Set default operational mode */
			if (EOK != pfe_phy_if_set_op_mode(phy_if, IF_OP_DEFAULT))
			{
				NXP_LOG_ERROR("Could not set default operational mode (%s)\n", phy_ifs[ii].name);
				return ENODEV;
			}

			if ((pfe_phy_if_get_id(phy_if) == PFE_PHY_IF_ID_EMAC0)
					|| (pfe_phy_if_get_id(phy_if) == PFE_PHY_IF_ID_EMAC1)
					|| (pfe_phy_if_get_id(phy_if) == PFE_PHY_IF_ID_EMAC2))
			{
				/*	Bind EMAC instance with the physical IF */
				if (EOK != pfe_phy_if_bind_emac(phy_if, phy_ifs[ii].emac))
				{
					NXP_LOG_ERROR("Can't bind interface with EMAC (%s)\n", phy_ifs[ii].name);
					return ENODEV;
				}
			}
			else
			{
				/*	Bind HIF channel instance with the physical IF */
				if (EOK != pfe_phy_if_bind_hif(phy_if, phy_ifs[ii].chnl))
				{
					NXP_LOG_ERROR("Can't bind interface with HIF (%s)\n", phy_ifs[ii].name);
					return ENODEV;
				}
			}

			/*	Register in platform */
			if (EOK != pfe_platform_register_phy_if(platform, phy_if))
			{
				NXP_LOG_ERROR("Could not register %s\n", pfe_phy_if_get_name(phy_if));
				if (EOK != pfe_phy_if_destroy(phy_if))
				{
					NXP_LOG_DEBUG("Could not destroy physical interface\n");
				}
				
				phy_if = NULL;
				return ENODEV;
			}

			/*	Create default logical IF */
			snprintf(if_name, 16, "pfe%d", log_if_id);
			log_if = pfe_log_if_create(phy_if, if_name, log_if_id);
			log_if_id++;
			if (NULL == log_if)
			{
				NXP_LOG_ERROR("Could not create %s\n", if_name);
				return ENODEV;
			}
			else
			{
				/*	Configure the logical interface here: */
				if ((pfe_phy_if_get_id(phy_if) == PFE_PHY_IF_ID_EMAC0)
						|| (pfe_phy_if_get_id(phy_if)== PFE_PHY_IF_ID_EMAC1)
						|| (pfe_phy_if_get_id(phy_if) == PFE_PHY_IF_ID_EMAC2))
				{
					/*	Set MAC address */
					(void)pfe_log_if_set_mac_addr(log_if, phy_ifs[ii].mac);
				}
				else
				{
					/*	TODO: Currently not supported for HIF interfaces */
				}

				/*	Register in platform */
				if (EOK != pfe_platform_register_log_if(platform, log_if))
				{
					NXP_LOG_ERROR("Could not register %s\n", pfe_log_if_get_name(log_if));
					pfe_log_if_destroy(log_if);
					log_if = NULL;
					return ENODEV;
				}
			}
		}
	}

	/*	Set up default routing for every logical interface */
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	/*	Send matching packets to HIF NOCPY */
	phy_if = pfe_platform_get_phy_if_by_id(platform, PFE_PHY_IF_ID_HIF_NOCPY);
#else
	/*	Send matching packets to HIF ch.0 */
	phy_if = pfe_platform_get_phy_if_by_id(platform, PFE_PHY_IF_ID_HIF0);
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	if (EOK != oal_mutex_lock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	entry = pfe_if_db_get_first(platform->log_if_db, IF_DB_CRIT_ALL, NULL);
	while (NULL != entry)
	{
		log_if = pfe_if_db_entry_get_log_if(entry);
		if (EOK != pfe_log_if_add_egress_if(log_if, phy_if))
		{
			NXP_LOG_ERROR("Could not set default egress interface for %s\n", pfe_log_if_get_name(log_if));
			ret = ENODEV;
			break;
		}

		entry = pfe_if_db_get_next(platform->log_if_db);
	}

	if (EOK != oal_mutex_unlock(&platform->log_if_db_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Release interface-related resources
 */
static void pfe_platform_destroy_ifaces(pfe_platform_t *platform)
{
	pfe_if_db_entry_t *entry;
	pfe_log_if_t *log_if;
	pfe_phy_if_t *phy_if;

	if (NULL != platform->log_if_db)
	{
		if (EOK != oal_mutex_lock(&platform->log_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		entry = pfe_if_db_get_first(platform->log_if_db, IF_DB_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			log_if = pfe_if_db_entry_get_log_if(entry);

			if (EOK != pfe_if_db_remove(platform->log_if_db, entry))
			{
				NXP_LOG_DEBUG("Could not remove log_if DB entry\n");
			}
			
			pfe_log_if_destroy(log_if);

			entry = pfe_if_db_get_next(platform->log_if_db);
		}

		if (EOK != oal_mutex_unlock(&platform->log_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}

	if (NULL != platform->phy_if_db)
	{
		if (EOK != oal_mutex_lock(&platform->phy_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		entry = pfe_if_db_get_first(platform->phy_if_db, IF_DB_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			phy_if = pfe_if_db_entry_get_phy_if(entry);

			if (EOK != pfe_if_db_remove(platform->phy_if_db, entry))
			{
				NXP_LOG_DEBUG("Could not remove phy_if DB entry\n");
			}

			if (EOK != pfe_phy_if_destroy(phy_if))
			{
				NXP_LOG_DEBUG("Can't destroy %s\n", pfe_phy_if_get_name(phy_if));
			}

			entry = pfe_if_db_get_next(platform->phy_if_db);
		}

		if (EOK != oal_mutex_unlock(&platform->phy_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}
}

/**
 * @brief		Perform PFE soft reset
 */
errno_t pfe_platform_soft_reset(pfe_platform_t *platform)
{
	void *addr;
	uint32_t regval;

	(void)platform;
	addr = (void *)(CBUS_GLOBAL_CSR_BASE_ADDR + 0x20U + (addr_t)(pfe.cbus_baseaddr));
	regval = hal_read32(addr) | (1U << 30);
	hal_write32(regval, addr);

	oal_time_usleep(100000); /* 100ms (taken from reference code) */

	regval &= ~(1U << 30);
	hal_write32(regval, addr);

	return EOK;
}

/**
 * @brief	The platform init function
 * @details	Initializes the PFE HW platform and prepares it for usage according to configuration.
 */
errno_t pfe_platform_init(pfe_platform_config_t *config)
{
	errno_t ret = EOK;
	volatile void *addr;
	uint32_t val, ii;

	memset(&pfe, 0U, sizeof(pfe_platform_t));

	/*	Create interface databases */
	pfe.phy_if_db = pfe_if_db_create(PFE_IF_DB_PHY);
	if (NULL == pfe.phy_if_db)
	{
		NXP_LOG_DEBUG("Can't create physical interface DB\n");
		goto exit;
	}
	else
	{
		if (EOK != oal_mutex_init(&pfe.phy_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex initialization failed\n");
			goto exit;
		}
	}

	pfe.log_if_db = pfe_if_db_create(PFE_IF_DB_LOG);
	if (NULL == pfe.log_if_db)
	{
		NXP_LOG_DEBUG("Can't create logical interface DB\n");
		goto exit;
	}
	else
	{
		if (EOK != oal_mutex_init(&pfe.log_if_db_lock))
		{
			NXP_LOG_DEBUG("Mutex initialization failed\n");
			goto exit;
		}
	}

	pfe.fw = config->fw;

	/*	Map CBUS address space */
	pfe.cbus_baseaddr = oal_mm_dev_map((void *)config->cbus_base, config->cbus_len);
	if (NULL == pfe.cbus_baseaddr)
	{
		NXP_LOG_ERROR("Can't map PPFE CBUS\n");
		goto exit;
	}
	else
	{
		NXP_LOG_INFO("PFE CBUS p0x%p mapped @ v0x%p\n", (void *)config->cbus_base, pfe.cbus_baseaddr);
	}

#if !defined(GLOBAL_CFG_RUN_ON_VDK)
	/*	Initialize LMEM TODO: Get LMEM size from global WSP_LMEM_SIZE register */
	NXP_LOG_DEBUG("Initializing LMEM (%d bytes)\n", CBUS_LMEM_SIZE);
	for (ii=0U; ii<CBUS_LMEM_SIZE; ii+=4U)
	{
		addr = (void *)((addr_t)pfe.cbus_baseaddr + CBUS_LMEM_BASE_ADDR + ii);
		*(uint32_t *)addr = 0U;
	}
#else
	NXP_LOG_INFO("[RUN_ON_VDK]: SKIPPED initializing LMEM (%d bytes)\n", CBUS_LMEM_SIZE);
#endif /* !GLOBAL_CFG_RUN_ON_VDK */

	/*	Create HW components */
	pfe.emac_count = 3U;
	pfe.gpi_count = 3U;
	pfe.etgpi_count = 3U;
	pfe.hgpi_count = 1U;
	pfe.bmu_count = 2U;
	pfe.class_pe_count = 1U;	/* TODO: Current IMG FPGA bitfile implements only 1 CLASS PE */
	pfe.tmu_pe_count = 0U;
	pfe.util_pe_count = 0U;	/* TODO: Current IMG FPGA bitfile does not implement UTIL PE */

	if (TRUE == config->common_irq_mode)
	{
		/*	IRQ mode: global isr (FPGA) */

		NXP_LOG_INFO("Detected Common IRQ mode (FPGA/PCI)\n");

		pfe.irq_global = oal_irq_create(config->irq_vector_global, OAL_IRQ_FLAG_SHARED, "PFE IRQ");
		if (NULL == pfe.irq_global)
		{
			NXP_LOG_ERROR("Could not create global PFE IRQ\n");
			goto exit;
		}
		else
		{
			if (EOK != oal_irq_add_handler(pfe.irq_global, &pfe_platform_global_isr, &pfe, &pfe.irq_global_isr_handle))
			{
				NXP_LOG_ERROR("Could not add global IRQ handler\n");
				goto exit;
			}
		}
	}
	else /* config->common_irq_mode */
	{
		/*	IRQ mode: per block isr (S32G/VDK) */

		NXP_LOG_INFO("Detected per block IRQ mode (S32G/VDK)\n");

		/* Note:
		 *
		 * The irq handlers are created inside corrsponding constructors,
		 * like pfe_platform_create_hif() or pfe_platform_create_bmu()
		 *
		 */
	}

	/*	BMU */
	ret = pfe_platform_create_bmu(&pfe, config);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	GPI */
	ret = pfe_platform_create_gpi(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	ETGPI */
	ret = pfe_platform_create_etgpi(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	HGPI */
	ret = pfe_platform_create_hgpi(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	TMU */
	ret = pfe_platform_create_tmu(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	Classifier */
	ret = pfe_platform_create_class(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
	/*	EMAC */
	ret = pfe_platform_create_emac(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

#ifndef TARGET_OS_LINUX /* temp solution, disabled for Linux now */
	/*	SAFETY */
	ret = pfe_platform_create_safety(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	Routing Table */
	ret = pfe_platform_create_rtable(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	L2 Bridge */
	ret = pfe_platform_create_l2_bridge(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
#endif

#if 0
	/*	UTIL */
	ret = pfe_platform_create_util(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
#endif /* 0 */

#if 0
#warning EXPERIMENTAL STUFF
	/*	This is not optimal. Firmware is sampling the register during initialization (only once)
		to get ports assigned to bridge. If something has changed during runtime (interface added/removed)
		the firmware never gets this information. */
	addr = (addr_t *)(CBUS_GLOBAL_CSR_BASE_ADDR + 0x44U + (addr_t)(pfe.cbus_baseaddr));
	val = *addr;
	val = val | 0x7U; /* this should configure ALL EMAC ports as SWITCH ports */
	*addr = val;
#endif /* 0 */

	/*	SOFT RESET */
	if (EOK != pfe_platform_soft_reset(&pfe))
	{
		NXP_LOG_ERROR("Platform reset failed\n");
	}

	/*	HIF (HIF DOES NOT LIKE SOFT RESET ONCE HAS BEEN CONFIGURED...) */
	ret = pfe_platform_create_hif(&pfe, config);
	if (EOK != ret)
	{
		goto exit;
	}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	/*	HIF NOCPY */
	ret = pfe_platform_create_hif_nocpy(&pfe, config);
	if (EOK != ret)
	{
		goto exit;
	}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	/*	Activate the classifier */
	NXP_LOG_INFO("Enabling the CLASS block\n");
	pfe_class_enable(pfe.classifier);
	/*	Wait a (micro) second to let classifier firmware to initialize */
	oal_time_usleep(50000);

	/*	Interfaces */
	ret = pfe_platform_create_ifaces(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	/*	Activate PFE blocks */
	pfe_bmu_enable(pfe.bmu[0]);
	pfe_bmu_enable(pfe.bmu[1]);
	pfe_gpi_enable(pfe.gpi[0]);
	pfe_gpi_enable(pfe.gpi[1]);
	pfe_gpi_enable(pfe.gpi[2]);
	pfe_gpi_enable(pfe.etgpi[0]);
	pfe_gpi_enable(pfe.etgpi[1]);
	pfe_gpi_enable(pfe.etgpi[2]);
	pfe_gpi_enable(pfe.hgpi[0]);
	pfe_tmu_enable(pfe.tmu);
#if 0
	pfe_util_enable(pfe.util);
#endif /* 0 */

	/*	Enable MAC's TODO: FIXME: Really? What does this write do? (it does not work without this but we need to know
		what is the purpose of this - description is missing in TRM of course... */
	addr = (void *)(CBUS_GLOBAL_CSR_BASE_ADDR + 0x20U + (addr_t)(pfe.cbus_baseaddr));
	val = hal_read32(addr);
	hal_write32((val | 0x80000003U), addr);

	/*	Create HIF driver(s) */
	ret = pfe_platform_create_hif_drv(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

	pfe.probed = TRUE;

	return EOK;

exit:
	(void)pfe_platform_remove();
	return ret;
}

/**
 * @brief		Destroy the platform
 */
errno_t pfe_platform_remove(void)
{
	errno_t ret;

	/*	Remove and disable IRQ just before platform modules are destroyed. */
	if (NULL != pfe.irq_global)
	{
		oal_irq_destroy(pfe.irq_global);
		pfe.irq_global = NULL;
	}

	/*	Clear the generic control register */
	if (NULL != pfe.cbus_baseaddr)
	{
		hal_write32(0U, (void *)(CBUS_GLOBAL_CSR_BASE_ADDR + 0x20U + (addr_t)(pfe.cbus_baseaddr)));
	}

	pfe_platform_destroy_hif_drv(&pfe);
	pfe_platform_destroy_hif(&pfe);
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	pfe_platform_destroy_hif_nocpy(&pfe);
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */
	pfe_platform_destroy_gpi(&pfe);
	pfe_platform_destroy_etgpi(&pfe);
	pfe_platform_destroy_hgpi(&pfe);
	pfe_platform_destroy_bmu(&pfe);
	pfe_platform_destroy_rtable(&pfe);
	pfe_platform_destroy_l2_bridge(&pfe);
	pfe_platform_destroy_ifaces(&pfe); /* Need classifier instance to be available */
	pfe_platform_destroy_class(&pfe);
	pfe_platform_destroy_tmu(&pfe);
	pfe_platform_destroy_util(&pfe);
	pfe_platform_destroy_emac(&pfe);
	pfe_platform_destroy_safety(&pfe);

	if (NULL != pfe.cbus_baseaddr)
	{
		ret = oal_mm_dev_unmap(pfe.cbus_baseaddr, PFE_CFG_AXI_SLAVE_LEN);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't unmap PPFE CBUS: %d\n", ret);
			return ret;
		}
	}
	
	if (NULL != pfe.phy_if_db)
	{
		pfe_if_db_destroy(pfe.phy_if_db);
		pfe.phy_if_db = NULL;
		oal_mutex_destroy(&pfe.phy_if_db_lock);
	}

	if (NULL != pfe.log_if_db)
	{
		pfe_if_db_destroy(pfe.log_if_db);
		pfe.log_if_db = NULL;
		oal_mutex_destroy(&pfe.log_if_db_lock);
	}

	pfe.cbus_baseaddr = 0x0ULL;
	pfe.probed = FALSE;

	return EOK;
}

/**
 * @brief		Read and print PFE HW IP blocks versions
 */
void pfe_platform_print_versions(pfe_platform_t *platform)
{
	NXP_LOG_INFO("CLASS version    : 0x%x\n", hal_read32(platform->cbus_baseaddr + CLASS_VERSION));
	NXP_LOG_INFO("TMU version      : 0x%x\n", hal_read32(platform->cbus_baseaddr + TMU_VERSION));
	NXP_LOG_INFO("BMU1 version     : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_BMU1_BASE_ADDR + BMU_VERSION));
	NXP_LOG_INFO("BMU2 version     : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_BMU2_BASE_ADDR + BMU_VERSION));
	NXP_LOG_INFO("EGPI1 version    : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_EGPI1_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("EGPI2 version    : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_EGPI2_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("EGPI3 version    : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_EGPI3_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("ETGPI1 version   : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_ETGPI1_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("ETGPI2 version   : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_ETGPI2_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("ETGPI3 version   : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_ETGPI3_BASE_ADDR + GPI_VERSION));
	NXP_LOG_INFO("HGPI version     : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_HGPI_BASE_ADDR + GPI_VERSION));
	/* NXP_LOG_INFO("GPT version      : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_GPT_VERSION)); */
	NXP_LOG_INFO("HIF version      : 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_HIF_BASE_ADDR + HIF_VERSION));
	NXP_LOG_INFO("HIF NOPCY version: 0x%x\n", hal_read32(platform->cbus_baseaddr + CBUS_HIF_NOCPY_BASE_ADDR + HIF_NOCPY_VERSION));
	NXP_LOG_INFO("UTIL version     : 0x%x\n", hal_read32(platform->cbus_baseaddr + UTIL_VERSION));
}

/**
 * @brief		Get the platform instance
 */
pfe_platform_t * pfe_platform_get_instance(void)
{
	if (TRUE == pfe.probed)
	{
		return &pfe;
	}
	else
	{
		return NULL;
	}
}

/** @}*/
