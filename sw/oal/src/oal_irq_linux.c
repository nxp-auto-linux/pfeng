/* =========================================================================
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_IRQ
 * @{
 *
 * @file		oal_irq_linux.c
 * @brief		The oal_irq module source file (Linux).
 * @details		This file contains Linux-specific irq implementation.
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_irq.h"
#include "linked_list.h"

/**
 * @brief	The IRQ instance representation
 */
struct oal_irq_tag
{
	int32_t id;					/* The logical IRQ ID as seen by OS */
	oal_irq_flags_t flags;		/* Interrupt type flags */
	LLIST_t handlers;			/* List of handlers */
	uint32_t cnt;
	rwlock_t lock;				/* General spinlock */
	char_t *name;				/* Interrupt name as seen by OS */
};

typedef struct
{
	oal_irq_t *irq;
	void *data;
	oal_irq_handler_t handler;
	struct workqueue_struct *queue;
	struct work_struct work;
	oal_irq_isr_handle_t handle;
	LLIST_t list_entry;
} oal_irq_list_entry_t;

/**
 * @brief	Irq Linux handler
 */
static irqreturn_t linux_irq_handler(int32_t id, void *ctx)
{
	oal_irq_t *irq = (oal_irq_t *)ctx;
	oal_irq_list_entry_t *entry;
	LLIST_t *item, *aux;
	unsigned long flags;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return IRQ_HANDLED;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	read_lock_irqsave(&irq->lock, flags);

	LLIST_ForEachRemovable(item, aux, &irq->handlers)
	{
		entry = LLIST_Data(item, oal_irq_list_entry_t, list_entry);
		if(entry && entry->handler)
		{
			queue_work(entry->queue, &entry->work);
		}
	}

	read_unlock_irqrestore(&irq->lock, flags);

	return IRQ_HANDLED;
}

oal_irq_t * oal_irq_create(int32_t id, oal_irq_flags_t flags, const char_t *name)
{
	oal_irq_t *irq;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == name))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	irq = oal_mm_malloc(sizeof(oal_irq_t));
	if (NULL == irq)
	{
		NXP_LOG_ERROR("failed to allocate memory\n");
		return NULL;
	}

	irq->id = id;
	LLIST_Init(&irq->handlers);
	irq->cnt = 0;
	rwlock_init(&irq->lock);
	irq->name = oal_mm_malloc(strlen(name) + 1);
	if (NULL != irq->name)
	{
		memcpy(irq->name, name, strlen(name) + 1);
	}
	else
	{
		NXP_LOG_ERROR("failed to allocate memory\n");
		oal_mm_free(irq);
		return NULL;
	}

	ret = request_irq(id, linux_irq_handler, (flags & ~OAL_IRQ_FLAG_SHARED) ? IRQF_SHARED : 0, irq->name, (void *)irq);
	if (ret)
	{
		NXP_LOG_ERROR("failed to register IRQ %d (ret=%d)\n", id, ret);
		oal_mm_free(irq);
		return NULL;
	}
	NXP_LOG_INFO("register IRQ %d by name '%s'\n", id, name == NULL ? "<empty>" : name);

#if 0 /*TODO: disabled until introduction of hif_chnl_cbk_enable/disable api */
	if(flags & OAL_IRQ_FLAG_DISABLED)
	{
		oal_irq_mask(irq);
	}
#endif

	return irq;
}

static void irq_work_fn(struct work_struct *w)
{
	oal_irq_list_entry_t *entry = container_of(w, oal_irq_list_entry_t, work);

	if((NULL != entry) && (NULL != entry->handler))
	{
		entry->handler(entry->data);
	}
}

errno_t oal_irq_add_handler(oal_irq_t *irq, oal_irq_handler_t handler, void *data, oal_irq_isr_handle_t *handle)
{
	errno_t ret;
	oal_irq_list_entry_t *entry;
	unsigned long flags;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry = oal_mm_malloc(sizeof(oal_irq_list_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");

		ret = ENOMEM;
		goto end;
	}

	/* set up handler's entry */
	entry->irq = irq;
	entry->data = data;
	entry->handler = handler;
	entry->handle = atomic_inc_return((atomic_t *)&irq->cnt);

	/* single wq per handler */
	entry->queue = alloc_workqueue("%s/%i", WQ_UNBOUND | WQ_MEM_RECLAIM, 1, irq->name, entry->handle);
	if(NULL == entry->queue)
	{
		NXP_LOG_ERROR("interrupt handler queue alloc failed\n");
		ret = ENOMEM;
		goto err_workqueue;
	}
	INIT_WORK(&entry->work, irq_work_fn);

	write_lock_irqsave(&irq->lock, flags);

	LLIST_AddAtEnd(&entry->list_entry, &irq->handlers);

	if (NULL != handle)
	{
		*handle = entry->handle;
	}
	ret = EOK;

	write_unlock_irqrestore(&irq->lock, flags);

end:
	return ret;

err_workqueue:
	oal_mm_free(entry);

	goto end;
}

errno_t oal_irq_mask(oal_irq_t *irq)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	disable_irq_nosync(irq->id);

	return EOK;
}

errno_t oal_irq_del_handler(oal_irq_t *irq, oal_irq_isr_handle_t handle)
{
	errno_t ret;
	oal_irq_list_entry_t *entry = NULL;
	LLIST_t *item, *aux;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Search the handle */
	ret = ENOENT;
	write_lock(&irq->lock);

	LLIST_ForEachRemovable(item, aux, &irq->handlers)
	{

		entry = LLIST_Data(item, oal_irq_list_entry_t, list_entry);
		if ((NULL != entry) && (entry->handle == handle))
		{
			LLIST_Remove(&entry->list_entry);

			break; /* ForEachRemovable */
		}

	} /* ForEachRemovable */

	write_unlock(&irq->lock);

	if (entry && entry->handle == handle)
	{
		drain_workqueue(entry->queue);
		destroy_workqueue(entry->queue);

		oal_mm_free(entry);
		ret = EOK;
	}

	return ret;
}

void oal_irq_destroy(oal_irq_t *irq)
{
	LLIST_t *item, *aux;
	oal_irq_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Destroy list of handlers */
	LLIST_ForEachRemovable(item, aux, &irq->handlers)
	{
		errno_t ret;
		entry = LLIST_Data(item, oal_irq_list_entry_t, list_entry);

		if(NULL != entry)
		{
			ret = oal_irq_del_handler(irq, entry->handle);
			if(EOK != ret)
			{
				NXP_LOG_ERROR("Irq '%s' was't able to remove some handler\n", irq->name);
			}
		}
	}

	free_irq(irq->id, (void *)irq);

	if(NULL != irq->name)
	{
		oal_mm_free(irq->name);
	}

	oal_mm_free(irq);
}

errno_t oal_irq_unmask(oal_irq_t *irq)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	enable_irq(irq->id);

	return EOK;
}

int32_t oal_irq_get_id(const oal_irq_t *irq)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == irq))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return -1;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return irq->id;
}

/** @}*/
