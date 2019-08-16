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
#include <linux/kthread.h>
#include <linux/types.h>

#include "oal.h"
#include "oal_time.h"
#include "oal_mbox.h"
#include "fifo.h"

/**
 * @brief	Maximum number of IRQs to be attached to a single mbox
 * @see		oal_mbox_attach_irq()
 */
#define OAL_MBOX_MAX_IRQS 5

/**
 * @brief	Maximum number of timers to be attached to a single mbox
 * @see		oal_mbox_attach_timer()
 */
#define OAL_MBOX_MAX_TIMERS 2

/**
 * @brief	Maximum number of waiting messages in a single mbox
 */
#define OAL_MBOX_LINUX_MSG_DEPTH 128

/**
 * @brief	Maximum time in msec for message acknowledge
 */
#define OAL_MBOX_MSG_ACK_MAX_WAIT 1000

/**
 * @brief	The mbox instance representation
 */
struct __oal_mbox_tag
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
		oal_irq_t *irq;			/* The IRQ instance */
		int32_t code;			/* Message code associated with IRQ */
	} irqs[OAL_MBOX_MAX_IRQS];

	struct
	{
		bool_t used;		/* If true then entry is used */
		int32_t code;		/* Message code associated with timer */
		timer_t timerid;	/* Timer ID */
	} timers[OAL_MBOX_MAX_TIMERS];
};

static atomic_t mbox_cnt = ATOMIC_INIT(0);	/* to serialize workqueue globally */

static void mbox_ack_msg_internal(oal_mbox_msg_t *msg)
{
	oal_mbox_t *mbox = msg->metadata.ptr;

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == msg) || unlikely(NULL == msg->metadata.ptr))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
	if(mutex_lock_interruptible(&mbox->lock))
		return EINTR;

	/* Fill cmd's data */
	mbox->msg.data.metadata.type = mtype;
	mbox->msg.data.metadata.ptr = mbox;
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


/*
	Send message (blocking)
	The 'code' value is used to identify the message by receiver
*/
errno_t oal_mbox_send_message(oal_mbox_t *mbox, int32_t code, void *data, uint32_t len)
{
	return mbox_send_generic(mbox, OAL_MBOX_MSG_MESSAGE, code, data, len);
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

	code = (int32_t)(uint64_t)ptr;
	mbox_send_generic(mbox, OAL_MBOX_MSG_SIGNAL, code, NULL, 0);
}

/*
	Send signal (non-blocking)
	The 'code' value is used to identify the message by receiver
*/
errno_t oal_mbox_send_signal(oal_mbox_t *mbox, int32_t code)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != fifo_put(mbox->intr.fifo, (void *)(uint64_t)code))
	{
		NXP_LOG_ERROR("msg fifo put failed\n");
		return EINVAL;
	}

	queue_work(mbox->intr.queue, &mbox->intr.update);

	return EOK;
}

static bool_t mbox_irq_handler(void *data)
{
	oal_mbox_t *mbox = (oal_mbox_t *)data;
	int32_t ii;
	bool_t ret = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	for (ii=0; ii<OAL_MBOX_MAX_IRQS; ii++)
	{
		if (TRUE == mbox->irqs[ii].used)
		{
			oal_mbox_send_signal(mbox, mbox->irqs[ii].code);
			ret = TRUE;
		}
	}

	return ret;
}

/*
	Attach IRQ to the mbox.
	Once the IRQ is triggered a non-blocking message is sent to the mbox automatically.
 */
errno_t oal_mbox_attach_irq(oal_mbox_t *mbox, oal_irq_t *irq, int32_t code)
{
	int32_t id, ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mbox) || (NULL == irq)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Get IRQ ID */
	id = oal_irq_get_id(irq);
	if (-1 == id)
	{
		NXP_LOG_ERROR("Couldn't get IRQ vector\n");
		return ENODEV;
	}

	/*	Find free slot in IRQ storage */
	for (ii=0; ii<OAL_MBOX_MAX_IRQS; ii++)
	{
		if (FALSE == mbox->irqs[ii].used)
		{
			break;
		}
	}

	if (ii >= OAL_MBOX_MAX_IRQS)
	{
		NXP_LOG_ERROR("No space for new IRQ\n");
		return ENOSPC;
	}

	mbox->irqs[ii].used = TRUE;

	mbox->irqs[ii].code = code;
	mbox->irqs[ii].irq = irq;

	if(oal_irq_add_handler(irq, mbox_irq_handler, (void *)mbox, NULL))
	{
		NXP_LOG_ERROR("Couldn't add IRQ handler\n");
		mbox->irqs[ii].used = FALSE;
		return ENODEV;
	}
	NXP_LOG_INFO("Attach IRQ %d [code:%d] succeeded\n", id, code);

	return EOK;
}

/*
	Detach IRQ from mbox.
 */
errno_t oal_mbox_detach_irq(oal_mbox_t *mbox, oal_irq_t *irq)
{
	int32_t ii;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mbox) || (NULL == irq)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Find free slot in IRQ storage */
	for (ii=0; ii<OAL_MBOX_MAX_IRQS; ii++)
	{
		if ((mbox->irqs[ii].irq == irq) && (TRUE == mbox->irqs[ii].used))
		{
			break;
		}
	}

	if (ii >= OAL_MBOX_MAX_IRQS)
	{
		NXP_LOG_WARNING("IRQ not found\n");
		return ENOENT;
	}

	mbox->irqs[ii].used = FALSE;

	return EOK;
}

/*
	Attach timer to the mbox
 */
int oal_mbox_attach_timer(oal_mbox_t *mbox, unsigned int msec, int code)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/* not implemented yet */
	return ENOSPC;
}

/*
	Detach timer(s) from the mbox
 */
int oal_mbox_detach_timer(oal_mbox_t *mbox)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mbox))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	return ENOEXEC;
}

/*
    Receive message/signal (blocking)
*/
errno_t oal_mbox_receive(oal_mbox_t *mbox, oal_mbox_msg_t *msg)
{
	int ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == mbox) || (NULL == msg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(kthread_should_stop())
	{
		return EINTR;
	}

	/*  Wait for a message (blocking) */
	ret = wait_event_interruptible(mbox->msg.wait, atomic_read(&mbox->msg.up) > 0);
	if(0 != ret)
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
	int ii;

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

	/*	Initialize IRQ storage */
	for (ii=0; ii<OAL_MBOX_MAX_IRQS; ii++)
	{
		mbox->irqs[ii].used = FALSE;
	}

	/*	Initialize timers storage (not needed) */
	;

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
	int ii;

	if (NULL == mbox)
	{
		return;
	}

	/* Wake up reader */
	wake_up_interruptible(&mbox->msg.ack);


		if(mutex_lock_interruptible(&mbox->lock))
		{
			return; /* probably with leak of the mbox and irq */
		}

		/*	Detach all IRQs */
		for (ii=0; ii<OAL_MBOX_MAX_IRQS; ii++)
		{
			if (TRUE == mbox->irqs[ii].used)
			{
				(void)oal_mbox_detach_irq(mbox, mbox->irqs[ii].irq);
			}
		}

	if (mbox->intr.queue)
	{
		drain_workqueue(mbox->intr.queue);
		destroy_workqueue(mbox->intr.queue);
	}

	if (mbox->intr.fifo)
	{
		fifo_destroy(mbox->intr.fifo);
	}

	mutex_unlock(&mbox->lock);

	mutex_destroy(&mbox->lock);

	oal_mm_free(mbox);
}

/** @}*/
