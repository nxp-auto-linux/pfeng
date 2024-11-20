/* =========================================================================
 *  Copyright 2018-2021,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_MBOX
 * @{
 *
 * @file		oal_mbox_linux.c
 * @brief		The oal_mbox module source file (Linux).
 * @details		This file contains Linux-specific mbox implementation.
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/timer.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_time.h"
#include "oal_mbox.h"
#include "fifo.h"

/**
 * @brief	Maximum number of timers to be attached to a single mbox
 * @see		oal_mbox_attach_timer()
 */
#define OAL_MBOX_MAX_TIMERS 1

/**
 * @brief	Maximum number of waiting messages in a single mbox
 */
#define OAL_MBOX_LINUX_MSG_DEPTH 128

/**
 * @brief	Maximum time in msec for message acknowledge
 */
#define OAL_MBOX_MSG_ACK_MAX_WAIT 1000

/** @brief	We are reusing fifo data pointers as uint32_t values stored in.
 *			So to be able to detect fifo_get() errors, which are signalled
 *			as NULL, we have to move signal codes out of zero.
 */
#define MBOX_FIFO_TRANSITION 1

/**
 * @brief	The mbox instance representation
 */
struct oal_mbox_tag
{
	uint32_t id;
	struct mutex lock;

	struct
	{
		wait_queue_head_t wait;		/* Consumer (oal_mbox_receive) wait queue */
		wait_queue_head_t ack;		/* Producer (oal_mbox_send_xxx) wait queue */
		atomic_t up;
		atomic_t fin;
		oal_mbox_msg_t data;
	} msg;

	struct
	{
		struct workqueue_struct *queue;	/* Interrupt botom half */
		struct work_struct update;		/* Bottom half work */
		fifo_t *fifo;					/* BH code cache */
	} intr;

	struct
	{
		bool_t used;		/* If true then entry is used */
		int32_t code;		/* Message code associated with timer */
		struct timer_list timer;		/* Timer instance */
		uint32_t tmout;			/* Time timeout */
	} timer;
};

static atomic_t mbox_cnt = ATOMIC_INIT(0);	/* to serialize workqueue globally */

static void mbox_ack_msg_internal(oal_mbox_msg_t *msg)
{
	oal_mbox_t *mbox = msg->metadata.msg_info.ptr;

	/* signal the cmd is done */
	atomic_set(&mbox->msg.fin, 1);
	atomic_dec(&mbox->msg.up);
	wake_up_interruptible(&mbox->msg.ack);
}

/*
    Acknowledge the message (unblock the sender)
*/
void oal_mbox_ack_msg(oal_mbox_msg_t *msg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == msg) || unlikely(NULL == msg->metadata.msg_info.ptr))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (OAL_MBOX_MSG_MESSAGE == msg->metadata.type)
	{
		mbox_ack_msg_internal(msg);
	}
	else
	{
		; /* nothing to do */
	}
}

static errno_t mbox_send_generic(oal_mbox_t *mbox, oal_mbox_msg_type mtype, int32_t code, void *data, uint32_t len)
{
	int ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(mutex_lock_interruptible(&mbox->lock))
		return EINTR;

	/* Fill cmd's data */
	mbox->msg.data.metadata.type = mtype;
	mbox->msg.data.metadata.msg_info.ptr = mbox;
	mbox->msg.data.payload.code = code;
	mbox->msg.data.payload.ptr = (mtype == OAL_MBOX_MSG_MESSAGE) ? data : NULL;
	mbox->msg.data.payload.len = (mtype == OAL_MBOX_MSG_MESSAGE) ? len : 0;
	atomic_set(&mbox->msg.fin, 0);
	/* Wake up consumer */
	atomic_inc(&mbox->msg.up);
	wake_up_interruptible(&mbox->msg.wait);

	/*  Wait for an ACK (blocking) */
	ret = wait_event_interruptible_timeout(mbox->msg.ack, atomic_read(&mbox->msg.fin), msecs_to_jiffies(OAL_MBOX_MSG_ACK_MAX_WAIT));
	if(0 == ret)
	{
		NXP_LOG_ERROR("internal msg %d/%d timeouted\n", code, mtype);
		mutex_unlock(&mbox->lock);
		return ETIME;
	}
	else if(0 > ret)
	{
		NXP_LOG_DEBUG("internal msg %d/%d failed, err=%d\n", code, mtype, ret);
		mutex_unlock(&mbox->lock);
		return EBADMSG;
	}

	/* Unlocking queue for other waiters */
	mutex_unlock(&mbox->lock);

	return EOK;
}

static inline uint32_t mbox_fifo_transcode_read(uint32_t code)
{
	return code - MBOX_FIFO_TRANSITION;
}

static inline uint32_t mbox_fifo_transcode_write(uint32_t code)
{
	return code + MBOX_FIFO_TRANSITION;
}

static void mbox_handle_signal(struct work_struct *work)
{
	oal_mbox_t *mbox = container_of(work, oal_mbox_t, intr.update);
	void *ptr;
	int32_t code;

	if(mutex_lock_interruptible(&mbox->lock))
	{
		return;
	}
	ptr = fifo_get(mbox->intr.fifo);
	mutex_unlock(&mbox->lock);

	if (NULL == ptr)
	{
		NXP_LOG_ERROR("No signal data in fifo\n");
		return;
	}

	code = mbox_fifo_transcode_read((uint32_t)(addr_t)ptr);
	mbox_send_generic(mbox, OAL_MBOX_MSG_SIGNAL, code, NULL, 0);
}

/*
	Send signal (non-blocking)
	The 'code' value is used to identify the message by receiver
*/
errno_t oal_mbox_send_signal(oal_mbox_t *mbox, int32_t code)
{
	uint32_t transcode;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (code < 0)
	{
		NXP_LOG_ERROR("Invalid value for signal code: %d\n", code);
		return EINVAL;
	}

	transcode = mbox_fifo_transcode_write(code);

	if (EOK != fifo_put(mbox->intr.fifo, (void *)(addr_t)transcode))
	{
		NXP_LOG_ERROR("msg fifo put failed\n");
		return EINVAL;
	}

	queue_work(mbox->intr.queue, &mbox->intr.update);

	return EOK;
}

static void mbox_timer_handler(struct timer_list *t)
{
	oal_mbox_t *mbox = from_timer(mbox, t, timer.timer);
	errno_t err;

	if (NULL == mbox || FALSE == mbox->timer.used)
	{
		return;
	}

	err = oal_mbox_send_signal(mbox, mbox->timer.code);

	if (TRUE == mbox->timer.used)
	{
		mod_timer(&mbox->timer.timer, jiffies + msecs_to_jiffies(mbox->timer.tmout));
	}
}

/*
	Attach timer to the mbox
 */
errno_t oal_mbox_attach_timer(oal_mbox_t *mbox, unsigned int msec, int code)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(mutex_lock_interruptible(&mbox->lock))
	{
		return EAGAIN;
	}

	if (TRUE == mbox->timer.used)
	{
		NXP_LOG_ERROR("No space for new timer\n");
		mutex_unlock(&mbox->lock);
		return ENOSPC;
	}

	mbox->timer.used = TRUE;
	mbox->timer.code = code;
	mbox->timer.tmout = msec;
	timer_setup(&mbox->timer.timer, mbox_timer_handler, 0);
	add_timer(&mbox->timer.timer);
	mod_timer(&mbox->timer.timer, jiffies + msecs_to_jiffies(msec));

	mutex_unlock(&mbox->lock);

	return EOK;
}

/*
	Detach timer(s) from the mbox
 */
errno_t oal_mbox_detach_timer(oal_mbox_t *mbox)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(mutex_lock_interruptible(&mbox->lock))
	{
		return EAGAIN;
	}

	if (FALSE == mbox->timer.used)
	{
		NXP_LOG_ERROR("No timer was running\n");
		mutex_unlock(&mbox->lock);
		return ENOSPC;
	}

	del_timer_sync(&mbox->timer.timer);
	mbox->timer.used = FALSE;

	mutex_unlock(&mbox->lock);

	return EOK;
}

/*
    Receive message/signal (blocking)
*/
errno_t oal_mbox_receive(oal_mbox_t *mbox, oal_mbox_msg_t *msg)
{
	int ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mbox) || (NULL == msg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = wait_event_interruptible(mbox->msg.wait,
								   atomic_read(&mbox->msg.up) > 0 || TRUE == kthread_should_stop());
	if(0 != ret || TRUE == kthread_should_stop())
	{
		return EINTR;
	}

	/*  Build MBOX message */
	memcpy(msg, &mbox->msg.data, sizeof(oal_mbox_msg_t));

	if (OAL_MBOX_MSG_SIGNAL == msg->metadata.type)
	{
		mbox_ack_msg_internal(msg);
	}

	return EOK;
}

/*
	Create new mbox
*/
oal_mbox_t * oal_mbox_create(void)
{
	oal_mbox_t *mbox;

	/*  Allocate new mbox */
	mbox = oal_mm_malloc(sizeof(oal_mbox_t));
	if (NULL == mbox)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return NULL;
	}

	memset(mbox, 0, sizeof(oal_mbox_t));

	/* Initialize msg fifo */
	mbox->intr.fifo = fifo_create(OAL_MBOX_LINUX_MSG_DEPTH);
	if(NULL == mbox->intr.fifo)
	{
		NXP_LOG_ERROR("mbox msg fifo alloc failed\n");
		goto err_fifo;
	}

	mbox->id = atomic_inc_return(&mbox_cnt);
	mbox->intr.queue = alloc_ordered_workqueue("pfe_mbox_intr/%i", WQ_MEM_RECLAIM | WQ_HIGHPRI, mbox->id);
	if(NULL == mbox->intr.queue)
	{
		NXP_LOG_ERROR("mbox msg intr queue alloc failed\n");
		goto err_workqueue;
	}
	INIT_WORK(&mbox->intr.update, mbox_handle_signal);

	/*	Initialize Linux waitqueues */
	init_waitqueue_head(&mbox->msg.wait);
	init_waitqueue_head(&mbox->msg.ack);

	atomic_set(&mbox->msg.up, 0);
	atomic_set(&mbox->msg.fin, 0);

	mutex_init(&mbox->lock);

	return mbox;

err_workqueue:
	fifo_destroy(mbox->intr.fifo);

err_fifo:

	oal_mm_free(mbox);
	return NULL;
}

void oal_mbox_destroy(oal_mbox_t *mbox)
{
	if (NULL == mbox)
	{
		return;
	}

	/* Wake up reader */
	wake_up_interruptible(&mbox->msg.ack);

	/*  Destroy timer */
	if (TRUE == mbox->timer.used)
	{
		(void)oal_mbox_detach_timer(mbox);
	}

	if(mutex_lock_interruptible(&mbox->lock))
	{
		NXP_LOG_ERROR("mbox locking failed\n");
		return; /* probably with leak of the mbox */
	}

	if (mbox->intr.queue)
	{
		cancel_work_sync(&mbox->intr.update);
		flush_workqueue(mbox->intr.queue);
		destroy_workqueue(mbox->intr.queue);
	}

	if (mbox->intr.fifo)
	{
		fifo_destroy(mbox->intr.fifo);
		mbox->intr.fifo = NULL;
	}

	mutex_unlock(&mbox->lock);

	mutex_destroy(&mbox->lock);

	oal_mm_free(mbox);
}

/** @}*/
