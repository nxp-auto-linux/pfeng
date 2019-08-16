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
 * @brief		The PFE platform management (S32G slave)
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
#include <unistd.h>
#include <stdlib.h>

#include "elf_cfg.h"
#include "elf.h"

#include "oal.h"
#include "hal.h"

#include "pfe_mmap.h"
#include "pfe_cbus.h"
#include "pfe_platform_cfg.h"
#include "pfe_platform.h"
#include "pfe_ct.h"
#include "pfe_idex.h"

/**
 * This is a platform specific file. All routines shall be implemented
 * according to application-given HW setup.
 */

static bool_t pfe_platform_global_isr(void *arg);

/*
 * @brief Platform instance storage
 */
static pfe_platform_t pfe = {.probed = FALSE};

/*
 * @brief Stuff needed to workaround missing IRQ lines on FPGA platform. To
 * 		  be removed.
 */
#define IRQ_WORKER_POLL		1U
#define IRQ_WORKER_QUIT		2U
static oal_mbox_t *mbox;
static oal_thread_t *worker;

/**
 * @brief		IRQ polling thread body
 * @param[in]	arg Argument. See oal_thread_create().
 * @return		Custom return value provided via oal_thread_join().
 */
static void *worker_func(void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;
	oal_mbox_msg_t msg;

	while (TRUE)
	{
		if (EOK == oal_mbox_receive(mbox, &msg))
		{
			if (IRQ_WORKER_POLL == msg.payload.code)
			{
				(void)pfe_platform_global_isr(platform);
			}

			if (IRQ_WORKER_QUIT == msg.payload.code)
			{
				break;
			}

			oal_mbox_ack_msg(&msg);
		}
	}

	return NULL;
}

/**
 * @brief		IDEX RPC callback
 */
static void idex_rpc_cbk(uint32_t id, void *buf, uint16_t buf_len, void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	(void)platform;

	NXP_LOG_INFO("Got IDEX RPC request\n");

	/*	Report execution status to caller */
	if (EOK != pfe_idex_set_rpc_ret_val(EINVAL, NULL, 0U))
	{
		NXP_LOG_ERROR("Could not send RPC response\n");
	}

	return;
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
	uint32_t ii;
	static pfe_hif_chnl_id_t ids[] = {HIF_CHNL_1}; /* Here we handle only HIF ch.1 interrupts */
	pfe_hif_chnl_t *chnls[sizeof(ids)/sizeof(pfe_hif_chnl_id_t)] = {NULL};

	if (NULL != platform->hif)
	{
		for (ii=0U; ii<(sizeof(ids)/sizeof(pfe_hif_chnl_id_t)); ii++)
		{
			chnls[ii] = pfe_hif_get_channel(platform->hif, ids[ii]);
			if (NULL != chnls[ii])
			{
				if (TRUE == pfe_hif_chnl_isr(chnls[ii]))
				{
					;
				}
			}
		}
	}

	/*	Does not matter. We're polling... */
	return FALSE;
}

/**
 * @brief		Assign HIF to the platform
 */
static errno_t pfe_platform_create_hif(pfe_platform_t *platform)
{
	platform->hif = pfe_hif_create(platform->cbus_baseaddr + CBUS_HIF_BASE_ADDR, HIF_CHNL_1);
	if (NULL == platform->hif)
	{
		NXP_LOG_ERROR("Couldn't create HIF instance\n");
		return ENODEV;
	}
	else
	{
		/*	Now particular interrupt sources can be enabled */
		/*	TODO: Enable off FPGA platform. When unmask is done master driver gets stuck due to edge-triggered IRQ line not falling down... */
		/*pfe_hif_chnl_irq_unmask(pfe_hif_get_channel(platform->hif, HIF_CHNL_1)); */
	}

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif(pfe_platform_t *platform)
{
	if (platform->hif)
	{
		pfe_hif_destroy(platform->hif);
		platform->hif = NULL;
	}
}

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
	/*	Create HIF driver instance (HIF channel 1) */
	channel = pfe_hif_get_channel(platform->hif, HIF_CHNL_1);
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	if (NULL != channel)
	{
		platform->hif_drv = pfe_hif_drv_create(pfe_hif_get_channel(platform->hif, HIF_CHNL_1));
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

	return EOK;
}

/**
 * @brief		Release HIF driver(s)
 */
static void pfe_platform_destroy_hif_drv(pfe_platform_t *platform)
{
	if (NULL != platform->hif_drv)
	{
		/*	Shut down IDEX */
		pfe_idex_fini();

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
	return platform->hif_drv;
}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		Assign HIF NOCPY to the platform
 */
static errno_t pfe_platform_create_hif_nocpy(pfe_platform_t *platform)
{
	platform->hif_nocpy = pfe_hif_nocpy_create(pfe.cbus_baseaddr + CBUS_HIF_NOCPY_BASE_ADDR, platform->bmu[1]);

	if (NULL == platform->hif_nocpy)
	{
		NXP_LOG_ERROR("Couldn't create HIF NOCPY instance\n");
		return ENODEV;
	}
	else
	{
		pfe_hif_chnl_irq_unmask(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID));
	}

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif_nocpy(pfe_platform_t *platform)
{
	if (platform->hif_nocpy)
	{
		pfe_hif_nocpy_destroy(platform->hif_nocpy);
		platform->hif_nocpy = NULL;
	}
}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		Get logical interface by its ID
 * @param[in]	platform Platform instance
 * @param[in]	id Logical interface ID. See pfe_log_if_t.
 * @return		Logical interface instance or NULL if failed.
 */
pfe_log_if_t *pfe_platform_get_log_if_by_id(pfe_platform_t *platform, uint8_t id)
{
	/*	TODO */
	return NULL;
}

/**
 * @brief		Assign interfaces to the platform.
 */
static errno_t pfe_platform_create_ifaces(pfe_platform_t *platform)
{
	/*	TODO */
	return EOK;
}

/**
 * @brief	The platform initialization function
 * @details	Initializes the PFE HW platform and prepares it for usage according to configuration.
 */
errno_t pfe_platform_init(pfe_platform_config_t *config)
{
	errno_t ret = EOK;

	memset(&pfe, 0U, sizeof(pfe_platform_t));

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

	/*	Create interrupt polling thread and associated stuff */
	mbox = oal_mbox_create();
	if (NULL == mbox)
	{
		goto exit;
	}

	worker = oal_thread_create(&worker_func, &pfe, "IRQ Poll Thread", 0);
	if (NULL == worker)
	{
		goto exit;
	}

	if (EOK != oal_mbox_attach_timer(mbox, 100, IRQ_WORKER_POLL))
	{
		goto exit;
	}

	ret = pfe_platform_create_hif(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	/*	HIF NOCPY */
	ret = pfe_platform_create_hif_nocpy(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	/*	Interfaces */
	ret = pfe_platform_create_ifaces(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}

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
 * @brief		Destroy PFE platform
 */
errno_t pfe_platform_remove(void)
{
	errno_t ret;

	pfe_platform_destroy_hif_drv(&pfe);

	oal_mbox_send_signal(mbox, IRQ_WORKER_QUIT);
	oal_thread_join(worker, NULL);

	pfe_platform_destroy_hif(&pfe);
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	pfe_platform_destroy_hif_nocpy(&pfe);
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */

	if (NULL != pfe.cbus_baseaddr)
	{
		ret = oal_mm_dev_unmap(pfe.cbus_baseaddr, PFE_CFG_AXI_SLAVE_LEN);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't unmap PPFE CBUS: %d\n", ret);
			return ret;
		}
	}

	pfe.cbus_baseaddr = 0x0ULL;
	pfe.probed = FALSE;

	return EOK;
}

pfe_platform_t * pfe_platform_get_instance(void)
{
	if (pfe.probed)
	{
		return &pfe;
	}
	else
	{
		return NULL;
	}
}

/** @}*/
