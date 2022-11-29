/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_THREAD
 * @{
 * 
 * @file		oal_thread_linux.c
 * @brief		The oal_thread module source file (Linux).
 * @details		This file contains Linux-specific thread management implementation.
 *
 */

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/signal.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"
#include "oal_thread.h"

struct __oal_thread_tag
{
	struct task_struct *thread;
	oal_thread_func func;
	void *func_arg;
	char_t *name;
};

static int thread_func(void *arg)
{
	oal_thread_t *thread = (oal_thread_t *)arg;
	void *err;
	int ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == thread))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*  Set thread killable */
	allow_signal(SIGKILL);

	/*  Run custom thread worker */
	err = thread->func(thread->func_arg);
	ret = (long int)err;

	/* Registered function already terminated. Wait for oal_thread_join() */
	while(!kthread_should_stop()) {
		oal_time_usleep(1000U);
	}

	return ret;
}

oal_thread_t *oal_thread_create(oal_thread_func func, void *func_arg, const char_t *name, uint32_t attrs)
{
	oal_thread_t *thread;
	
	(void)attrs;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == name))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	thread = oal_mm_malloc(sizeof(oal_thread_t));
	if (NULL == thread)
	{
		NXP_LOG_ERROR("failed to allocate memory\n");
		return NULL;
	}

	thread->func = func;
	thread->func_arg = func_arg;
	thread->name = oal_mm_malloc(strlen(name) + 1);
	if (NULL != thread->name)
	{
		memcpy(thread->name, name, strlen(name) + 1);
	}
	else
	{
		NXP_LOG_ERROR("failed to allocate memory\n");
		oal_mm_free(thread);
		return NULL;
	}

	thread->thread = kthread_run(thread_func, thread, name);
	if (unlikely(IS_ERR(thread->thread)))
	{
		NXP_LOG_ERROR("Can't create a thread '%s': %ld\n", thread->name, PTR_ERR(thread->thread));
		oal_mm_free(thread->name);
		oal_mm_free(thread);
		thread = NULL;
		return NULL;
	}

	return thread;
}

errno_t oal_thread_join(oal_thread_t *thread, void **retval)
{
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == thread))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	wake_up_process(thread->thread);
	err = kthread_stop(thread->thread);

	if(unlikely(err)) {
		NXP_LOG_ERROR("Can't stop thread '%s': %d\n", thread->name, err);
		return err;
	}
	
	if(NULL != thread->name)
		oal_mm_free(thread->name);
	oal_mm_free(thread);
	
	return err;
}

errno_t oal_thread_cancel(oal_thread_t *thread) {
	(void)thread;

	/* No cancellation in Linux */
	return EOK;
}


/** @}*/
