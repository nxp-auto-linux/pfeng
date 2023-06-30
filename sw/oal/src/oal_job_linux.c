/* =========================================================================
 *  Copyright 2019-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_JOB
 * @{
 *
 * @file		oal_job_linux.c
 * @brief		The oal_job module source file (LINUX).
 * @details		This file contains LINUX-specific deferred job implementation.
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/types.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"
#include "oal_job.h"

struct oal_job_tag
{
	struct workqueue_struct *queue;
	struct work_struct work;
	oal_job_func func;
	void *arg;
	oal_spinlock_t lock;
};

errno_t oal_job_run(oal_job_t *job)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == job))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	queue_work(job->queue, &job->work);

	return ret;
}

errno_t oal_job_drain(const oal_job_t *job)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == job))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}

	if (unlikely((NULL == job) || (NULL == job->queue)))
	{
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	drain_workqueue(job->queue);

	return EOK;
}

static void job_work_fn(struct work_struct *w)
{
	oal_job_t *job = container_of(w, oal_job_t, work);

	if(NULL != job->func)
	{
		job->func(job->arg);
	}
}

oal_job_t *oal_job_create(oal_job_func func, void *arg, const char_t *name, oal_prio_t prio)
{
	oal_job_t *job;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == name))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == func)
	{
		NXP_LOG_ERROR("Job function must not be NULL\n");
		return NULL;
	}

	job = oal_mm_malloc(sizeof(oal_job_t));
	if (NULL == job)
	{
		NXP_LOG_ERROR("failed to allocate memory\n");
		return NULL;
	}

	job->func = func;
	job->arg = arg;

	if (EOK != oal_spinlock_init(&job->lock))
	{
		NXP_LOG_ERROR("oal_spinlock_init() failed\n");
		oal_mm_free(job);
		return NULL;
	}

	job->queue = alloc_workqueue("%s", WQ_UNBOUND | WQ_MEM_RECLAIM, 1, name);
	if(NULL == job->queue)
	{
		NXP_LOG_ERROR("Can't create job queue\n");
		oal_job_destroy(job);
		return NULL;
	}
	INIT_WORK(&job->work, job_work_fn);

	return job;
}

errno_t oal_job_destroy(oal_job_t *job)
{
	if (NULL != job)
	{
		if (NULL != job->queue)
		{
			oal_job_drain(job);

			destroy_workqueue(job->queue);

			job->queue = NULL;
		}

		if (EOK != oal_spinlock_destroy(&job->lock))
		{
			NXP_LOG_ERROR("oal_spinlock_destroy() failed\n");
		}

		/*	Release the instance */
		oal_mm_free(job);
	}

	return EOK;
}

/** @}*/
