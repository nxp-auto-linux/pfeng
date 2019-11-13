/* =========================================================================
 *  Copyright 2019 NXP
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
 * @addtogroup  dxgr_PFE_SAFETY
 * @{
 *
 * @file		pfe_safety.c
 * @brief		The SAFETY module source file.
 * @details		This file contains SAFETY-related functionality.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_mmap.h"
#include "pfe_safety.h"
#include "pfe_safety_csr.h"


struct __pfe_safety_tag
{
	void *cbus_base_va;		    /* CBUS base virtual address */
	void *safety_base_offset;	/* SAFETY base offset within CBUS space (SAFETY is member of WSP global CSR)*/
	void *safety_base_va;		/* SAFETY base address (virtual) (It is actually WSP global CSR base address)*/
	oal_mutex_t *lock;          /* Mutex for resource protection */
#if	defined(GLOBAL_CFG_SAFETY_WORKER)
	oal_thread_t *worker;		/* Worker thread */
	oal_mbox_t *mbox;			/* Message box to communicate with the worker thread */
#endif /* GLOBAL_CFG_SAFETY_WORKER */
};

#if	defined(GLOBAL_CFG_SAFETY_WORKER)
/**
 * @brief	Worker thread signals
 * @details	Driver is sending signals to the worker thread to request specific
 * 			operations.
 */
enum pfe_safety_worker_signals
{
	SIG_WORKER_STOP,	/*	!< Stop the thread  */
	SIG_TIMER_TICK		/*	!< Pulse from timer */
};

static void safety_unmask_all_irq(pfe_safety_t *safety);
static void *safety_worker_func(void *arg);

/**
 * @brief		Worker function running within internal thread
 * @note        Thread is used for periodic unmasking safety interrupts
 */
static void *safety_worker_func(void *arg)
{
	pfe_safety_t *safety = (pfe_safety_t *)arg;
	errno_t err;
	oal_mbox_msg_t msg;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	while (1)
	{
		err = oal_mbox_receive(safety->mbox, &msg);
		if (EOK != err)
		{
			NXP_LOG_WARNING("mbox: Problem receiving message: %d", err);
		}
		else
		{
			switch (msg.payload.code)
			{
				case SIG_WORKER_STOP:
				{
					/*	Exit the thread */
					oal_mbox_ack_msg(&msg);
					return NULL;
				}

				case SIG_TIMER_TICK:
				{
					safety_unmask_all_irq(safety);
					break;
				}
			}
		}
		oal_mbox_ack_msg(&msg);
	}
	return NULL;
}

/**
 * @brief		Unmask all safety interrupts in WSP_SAFETY_INT_EN register
 * @details		Used for unmasking all safety interrupt which were masked in ISR.
 * @param[in]	safety SAFETY instance
 * @note		This function is called from thread.
 */
static void safety_unmask_all_irq(pfe_safety_t *safety)
{

	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_unmask_all(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);
	return;
}
#endif /* GLOBAL_CFG_SAFETY_WORKER */
/**
 * @brief		Create new SAFETY instance
 * @details		Create and initializes SAFETY instance. New instance is always enabled.
 * 				Use mask and unmask function to control interrupts.
 * @param[in]	base_va SAFETY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in safety thread
 */
pfe_safety_t *pfe_safety_create(void *cbus_base_va, void *safety_base)
{
	pfe_safety_t *safety;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	safety = oal_mm_malloc(sizeof(pfe_safety_t));

	if (NULL == safety)
	{
		return NULL;
	}
	else
	{
		memset(safety, 0, sizeof(pfe_safety_t));
		safety->cbus_base_va = cbus_base_va;
		safety->safety_base_offset = safety_base;
		safety->safety_base_va = (void *)((addr_t)safety->cbus_base_va + (addr_t)safety->safety_base_offset);

		/*	Create mutex */
		safety->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == safety->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			pfe_safety_destroy(safety);
			return NULL;
		}
		else
		{
			(void)oal_mutex_init(safety->lock);
		}

#if defined(GLOBAL_CFG_SAFETY_WORKER)
		/*	Create mbox */
		safety->mbox = oal_mbox_create();
		if (NULL == safety->mbox)
		{
			NXP_LOG_ERROR("Mbox creation failed\n");
			pfe_safety_destroy(safety);
			return NULL;
		}

		/*	Create worker thread */
		safety->worker = oal_thread_create(&safety_worker_func, safety, "PFE safety worker", 0);
		if (NULL == safety->worker)
		{
			NXP_LOG_ERROR("Couldn't start worker thread\n");
			pfe_safety_destroy(safety);
			return NULL;
		}
		else
		{
			if (EOK != oal_mbox_attach_timer(safety->mbox, 30000, SIG_TIMER_TICK))
			{
				NXP_LOG_ERROR("Unable to attach timer\n");
				pfe_safety_destroy(safety);
				return NULL;
			}
		}
#endif /* GLOBAL_CFG_SAFETY_WORKER */

		/* Unmask all interrupts */
		pfe_safety_cfg_irq_unmask_all(safety->safety_base_va);
	}
	return safety;
}

/**
 * @brief		Destroy SAFETY instance
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_destroy(pfe_safety_t *safety)
{
#if defined(GLOBAL_CFG_SAFETY_WORKER)
	errno_t err;
#endif /* GLOBAL_CFG_SAFETY_WORKER */

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/* Mask safety interrupts */
	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_mask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);

	if (NULL != safety)
	{
#if defined(GLOBAL_CFG_SAFETY_WORKER)
		if (NULL != safety->mbox)
		{
			if (EOK != oal_mbox_detach_timer(safety->mbox))
			{
				NXP_LOG_DEBUG("Could not detach timer\n");
			}
		}

		if (NULL != safety->worker)
		{
			NXP_LOG_INFO("Stopping safety worker...\n");

			err = oal_mbox_send_signal(safety->mbox, SIG_WORKER_STOP);
			if (EOK != err)
			{
				NXP_LOG_ERROR("Signal failed: %d\n", err);
			}
			else
			{
				err = oal_thread_join(safety->worker, NULL);
				if (EOK != err)
				{
					NXP_LOG_ERROR("Can't join the worker thread: %d\n", err);
				}
				else
				{
					NXP_LOG_INFO("Safety worker stopped\n");
				}
			}
		}

		if (NULL != safety->mbox)
		{
			oal_mbox_destroy(safety->mbox);
			safety->mbox = NULL;
		}
#endif /* GLOBAL_CFG_SAFETY_WORKER */

		if (NULL != safety->lock)
		{
			(void)oal_mutex_destroy(safety->lock);
			(void)oal_mm_free(safety->lock);
			safety->lock = NULL;
		}

		/* Free memory used for structure */
		(void)oal_mm_free(safety);
	}
}

/**
 * @brief		SAFETY ISR
 * @param[in]	safety The SAFETY instance
 * @return		EOK if interrupt has been handled
 */
errno_t pfe_safety_isr(pfe_safety_t *safety)
{
	errno_t ret = ENOENT;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return ENOMEM;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_safety_cfg_isr(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);

	return ret;
}

/**
 * @brief		Mask SAFETY interrupts
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_irq_mask(pfe_safety_t *safety)
{

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_mask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);
}

/**
 * @brief		Unmask SAFETY interrupts
 * @param[in]	safety The SAFETY instance
 */
void pfe_safety_irq_unmask(pfe_safety_t *safety)
{

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == safety))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	(void)oal_mutex_lock(safety->lock);
	pfe_safety_cfg_irq_unmask(safety->safety_base_va);
	(void)oal_mutex_unlock(safety->lock);
}

/** @}*/
