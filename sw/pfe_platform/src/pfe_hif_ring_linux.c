/* =========================================================================
 *
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_hif_ring.c
 * @brief		The HIF BD ring driver.
 * @details		This is the HW BD ring interface providing basic manipulation
 * 				possibilities for HIF's RX and TX buffer descriptor rings.
 * 				Each ring is treated as a single instance therefore module can
 * 				be used to handle HIF with multiple channels (RX/TX ring pairs).
 *
 * @note		BD and WB BD rings are non-cached entities.
 *
 * @warning		No concurrency prevention is implemented here. User shall
 *				therefore ensure correct protection	of ring instance manipulation
 *				at application level.
 *
 */
#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_hif_ring_linux.h"

#define RING_LEN						PFE_CFG_HIF_RING_LENGTH
#define RING_LEN_MASK					(PFE_CFG_HIF_RING_LENGTH - 1U)

/* Buffer descriptor WORD0 */
#define HIF_RING_BD_W0_DESC_EN				(1U << 31U)
/* 30 .. 21 reserved */
#define HIF_RING_BD_W0_DIR					(1U << 20U)
#define HIF_RING_BD_W0_LAST_BD				(1U << 19U)
#define HIF_RING_BD_W0_LIFM					(1U << 18U)
#define HIF_RING_BD_W0_CBD_INT_EN			(1U << 17U)
#define HIF_RING_BD_W0_PKT_INT_EN			(1U << 16U)

#define HIF_RING_BD_W0_BD_SEQNUM_MASK		(0xFFFFU)
#define HIF_RING_BD_W0_BD_SEQNUM_OFFSET		(0U)
#define HIF_RING_BD_W0_BD_CTRL_MASK			(0xFFFFU)
#define HIF_RING_BD_W0_BD_CTRL_OFFSET		(16U)

#define HIF_RING_BD_W0_BD_SEQNUM(seqnum)	\
		(((seqnum) & HIF_RING_BD_W0_BD_SEQNUM_MASK)	<< \
					 HIF_RING_BD_W0_BD_SEQNUM_OFFSET)
#define HIF_RING_BD_W0_BD_SEQNUM_GET(w0)	\
		(((w0) >> HIF_RING_BD_W0_BD_SEQNUM_OFFSET) & \
					  HIF_RING_BD_W0_BD_SEQNUM_MASK)

#define HIF_RING_BD_W0_BD_CTRL(ctrl)	\
		(((ctrl) & HIF_RING_BD_W0_BD_CTRL_MASK)	<< \
					 HIF_RING_BD_W0_BD_CTRL_OFFSET)
#define HIF_RING_BD_W0_BD_CTRL_GET(w0)	\
		(((w0) >> HIF_RING_BD_W0_BD_CTRL_OFFSET) & \
					  HIF_RING_BD_W0_BD_CTRL_MASK)

/* Buffer descriptor WORD1 */
#define HIF_RING_BD_W1_BD_BUFFLEN_MASK		(0xFFFFU)
#define HIF_RING_BD_W1_BD_BUFFLEN_OFFSET	(0U)
#define HIF_RING_BD_W1_BD_RSVD_STAT_MASK	(0xFFFFU)
#define HIF_RING_BD_W1_BD_RSVD_STAT_OFFSET	(16U)

#define HIF_RING_BD_W1_BD_BUFFLEN(buflen)	\
		(((buflen) & HIF_RING_BD_W1_BD_BUFFLEN_MASK)	<< \
					 HIF_RING_BD_W1_BD_BUFFLEN_OFFSET)
#define HIF_RING_BD_W1_BD_BUFFLEN_GET(w1)	\
		(((w1) >> HIF_RING_BD_W1_BD_BUFFLEN_OFFSET) & \
					  HIF_RING_BD_W1_BD_BUFFLEN_MASK)

#define HIF_RING_BD_W1_BD_RSVD_STAT(stat)	\
		(((stat) & HIF_RING_BD_W1_BD_RSVD_STAT_MASK)	<< \
				   HIF_RING_BD_W1_BD_RSVD_STAT_OFFSET)

/* Write back Buffer descriptor WORD0 */
#define HIF_RING_WB_BD_W0_DESC_EN			(1U << 9U)
#define HIF_RING_WB_BD_W0_DIR				(1U << 8U)
#define HIF_RING_WB_BD_W0_LAST_BD			(1U << 7U)
#define HIF_RING_WB_BD_W0_LIFM				(1U << 6U)
#define HIF_RING_WB_BD_W0_CBD_INT_EN		(1U << 5U)
#define HIF_RING_WB_BD_W0_PKT_INT_EN		(1U << 4U)
/* 3.. 0 Reserved */

/* Write back Buffer descriptor WORD1 */
#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_MASK	(0xFFFFU)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM_MASK		(0xFFFFU)
#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_OFFSET	(0U)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET	(16U)

#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN(buflen)	\
		(((buflen) & HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_MASK)	<< \
					 HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_OFFSET)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM(seqnum)	\
		(((seqnum) & HIF_RING_WB_BD_W1_WB_BD_SEQNUM_MASK)	<< \
				HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET)

#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(w1)	\
		(((w1) >> HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_OFFSET) & \
					  HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_MASK)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM_GET(w1)	\
		(((w1) >> HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET) & \
					  HIF_RING_WB_BD_W1_WB_BD_SEQNUM_MASK)

/**
 * @brief	The BD as seen by HIF
 * @details	Properly pack to form the structure as expected by HIF.
 * @note	Don't use the 'aligned' attribute here since behavior
 * 			is implementation-specific (due to the bitfields). Still
 * 			applies that BD shall be aligned to 64-bits and in
 * 			ideal case to cache line size.
 * @warning	Do not touch the structure (even types) unless you know
 * 			what you're doing.
 */
typedef struct __attribute__((packed)) pfe_hif_bd_tag
{
	volatile uint32_t ctrl_seqnum_w0;
	volatile uint32_t rsvd_buflen_w1;
	volatile uint32_t data;
	volatile uint32_t next;
} pfe_hif_bd_t;


/**
 * @brief	The write-back BD as seen by HIF
 * @note	Don't use the 'aligned' attribute here since behavior
 * 			is implementation-specific (due to the bitfields). Still
 * 			applies that BD shall be aligned to 64-bits and in
 * 			ideal case to cache line size.
 * @warning	Do not touch the structure (even types) unless you know
 * 			what you're doing.
 */
typedef struct __attribute__((packed)) pfe_hif_wb_bd_tag
{
	volatile uint32_t rsvd_ctrl_w0;
	volatile uint32_t seqnum_buflen_w1;
} pfe_hif_wb_bd_t;

/**
 * @brief	The BD ring structure
 * @note	The attribute 'aligned' is here just to ensure proper alignment
 * 			when instance will be created automatically without dynamic memory
 * 			allocation.
 */
struct __attribute__((aligned (HAL_CACHE_LINE_SIZE), packed)) pfe_hif_ring_tag
{
	/*	Put often used data from beginning to improve cache locality */

	/*	Every 'enqueue' and 'dequeue' access */
	void *base_va;				/*	Ring base address (virtual) */
	void *wb_tbl_base_va;			/*	Write-back table base address (virtual) */

	/*	Every 'enqueue' access */
	uint32_t write_idx;			/*	BD index to be written */
	pfe_hif_bd_t *wr_bd;			/*	Pointer to BD to be written */

#if (TRUE == HAL_HANDLE_CACHE)
	pfe_hif_bd_t *wr_bd_pa;			/*	Pointer to BD to be written (PA). Only due to CACHE_* macros in QNX... */
#endif /* HAL_HANDLE_CACHE */
	pfe_hif_wb_bd_t *wr_wb_bd;		/*	Pointer to WB BD to be written */
	bool_t is_rx;				/*	If TRUE then ring is RX ring */

	/*	Every 'dequeue' access */
	uint32_t read_idx;			/*	BD index to be read */
	pfe_hif_bd_t *rd_bd;			/*	Pointer to BD to be read */

	pfe_hif_wb_bd_t *rd_wb_bd;		/*	Pointer to WB BD to be read */
	bool_t heavy_data_mark;			/*	To enable getting size of heavily accessed data */

	/*	Initialization time only */
	void *base_pa;				/*	Ring base address (physical) */
	void *wb_tbl_base_pa;			/*	Write-back table base address (physical) */
};

__attribute__((hot)) static inline void inc_write_index_std(pfe_hif_ring_t *ring);
__attribute__((hot)) static inline void dec_write_index_std(pfe_hif_ring_t *ring);
__attribute__((hot)) static inline void inc_read_index_std(pfe_hif_ring_t *ring);
__attribute__((cold, aligned(64))) static pfe_hif_ring_t *pfe_hif_ring_create_std(bool_t rx);
static inline errno_t pfe_hif_ring_enqueue_buf_std(pfe_hif_ring_t *ring, const void *buf_pa, uint32_t length, bool_t lifm);
static inline errno_t pfe_hif_ring_dequeue_buf_std(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm);
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm);
__attribute__((cold, aligned(64))) static void pfe_hif_ring_invalidate_std(const pfe_hif_ring_t *ring);

__attribute__((hot)) static inline void inc_write_index_std(pfe_hif_ring_t *ring)
{
	ring->write_idx = (ring->write_idx + 1) & RING_LEN_MASK;
	ring->wr_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->write_idx];
	ring->wr_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->write_idx];
}

__attribute__((hot)) static inline void dec_write_index_std(pfe_hif_ring_t *ring)
{
	ring->write_idx = (ring->write_idx - 1) & RING_LEN_MASK;
	ring->wr_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->write_idx];
	ring->wr_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->write_idx];
}

__attribute__((hot)) static inline void inc_read_index_std(pfe_hif_ring_t *ring)
{
	ring->read_idx = (ring->read_idx + 1) & RING_LEN_MASK;
	ring->rd_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->read_idx];
	ring->rd_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->read_idx];
}

/**
 * @brief		Get fill level
 * @param[in]	ring The ring instance
 * @return		Number of occupied entries within the ring
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) uint32_t pfe_hif_ring_get_fill_level(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return RING_LEN;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return (ring->write_idx - ring->read_idx) & RING_LEN_MASK;
}

/**
 * @brief		Get physical address of the start of the ring
 * @param[in]	ring The ring instance
 * @return		Pointer to the beginning address of the ring
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, cold, aligned(64))) void *pfe_hif_ring_get_base_pa(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ring->base_pa;
}

/**
 * @brief		Get physical address of the write-back table
 * @param[in]	ring The ring instance
 * @return		Pointer to the table
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, cold, aligned(64))) void *pfe_hif_ring_get_wb_tbl_pa(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ring->wb_tbl_base_pa;
}

/**
 * @brief		Get length of the write-back table
 * @param[in]	ring The ring instance
 * @return		Length of the table in number of entries. Only valid when
 *				pfe_hif_ring_get_wb_tbl_pa() is not NULL.
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, cold, aligned(64))) uint32_t pfe_hif_ring_get_wb_tbl_len(const pfe_hif_ring_t *ring)
{
	(void)ring;

	return RING_LEN;
}

/**
 * @brief		Invalidate the explicit entry in the ring
 * @param[in]	ring The ring instance
 * @param[in]	index The position in the ring
 */
__attribute__((cold, aligned(64))) void pfe_hif_ring_invalidate_direct(const pfe_hif_ring_t *ring, uint32_t index)
{
        /* Clear BD enable flag */
        (&((pfe_hif_bd_t *)ring->base_va)[index])->ctrl_seqnum_w0 &= ~HIF_RING_BD_W0_DESC_EN;

        /* Reset the write-back descriptor */
        (&((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[index])->seqnum_buflen_w1 |= HIF_RING_WB_BD_W1_WB_BD_SEQNUM(0xffffU);
        (&((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[index])->rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
}

/**
 * @brief		Revalidate the explicit entry in the ring
 * @param[in]	ring The ring instance
 * @param[in]	index The position in the ring
 * @note		It is expected that BD has valid set-up
 */
__attribute__((cold, aligned(64))) void pfe_hif_ring_revalidate_direct(const pfe_hif_ring_t *ring, uint32_t index)
{
        /* Set BD enable flag */
        (&((pfe_hif_bd_t *)ring->base_va)[index])->ctrl_seqnum_w0 |= HIF_RING_BD_W0_DESC_EN;

        /* Set the write-back descriptor */
        (&((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[index])->rsvd_ctrl_w0 &= ~HIF_RING_WB_BD_W0_DESC_EN;
}

/**
 * @brief		Set ring to explicit position
 * @param[in]	ring The ring instance
 * @param[in]	index The new position of the ring
 * @return		0 if OK
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_ring_force_index(pfe_hif_ring_t *ring, uint32_t index)
{
	uint32_t offset;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	offset = index % RING_LEN;

	ring->read_idx = offset;
	ring->rd_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->read_idx & RING_LEN_MASK];
	ring->rd_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->read_idx & RING_LEN_MASK];

	ring->write_idx = offset;
	ring->wr_bd = ring->rd_bd;
	ring->wr_wb_bd = ring->rd_wb_bd;

	return 0;
}

/**
 * @brief		Search for valid entry in WB ring
 * @param[in]	ring The ring instance
 * @param[in]	valid TRUE if searhing for valid entry, FALSE otherwise
 * @param[out]	index The position of found entry in the ring
 * @return		TRUE if found
 * @note		Requires invalidated all entries before search
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_ring_find_wb_entry(pfe_hif_ring_t *ring, bool_t valid, uint32_t *index)
{
	uint32_t valid_id, invalid_id, ii;
	pfe_hif_wb_bd_t *wb;

	for (ii = 0U; ii < RING_LEN; ii++)
	{
		wb = &(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ii]);

		/* valid entry: WB BD must be DISABLED. The entry was processed by HW */
		if (0U == (wb->rsvd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
		{
			valid_id = ii;
			break;
		}
	}

	if (RING_LEN == ii)
		return ENOENT; /* not found */

	if (TRUE == valid)
	{
		*index = valid_id;
		return EOK;
	}

	for (ii = 0U; ii < RING_LEN; ii++)
	{

		invalid_id = (ii + valid_id + 1) % RING_LEN;
		wb = &(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[invalid_id]);

		/* invalid entry: WB BD must be ENABLED. The entry was not touched by HW
		 *                For invalid search we need to detect first valid entry
		 *                and only then search for invalid. Reason: Invalid search
		 *                is used on RX ring, which can contain multiple (cached)
		 *                entries.
		 */
		if ((0U != (wb->rsvd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN)))
			break;
	}

	if (RING_LEN == ii)
		return ENOENT; /* not found */

	*index = invalid_id;

	return EOK;
}

/**
 * @brief		Check if the ring is on the head
 * @param[in]	ring The ring instance
 * @return		TRUE if the ring is on the head
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) bool_t pfe_hif_ring_is_on_head(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ring->rd_wb_bd == ring->wb_tbl_base_va;
}

/**
 * @brief		Get length of the ring
 * @param[in]	ring The ring instance
 * @return		Ring length in number of entries
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) uint32_t pfe_hif_ring_get_len(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#else
	(void)ring;
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return RING_LEN;
}

/**
 * @brief		Add buffer to the ring
 * @details		Add buffer at current write position within the ring and increment
 *          	the write index. If the current position is already occupied by an
 *          	enabled buffer the call will fail.
 * @param[in]	ring The ring instance
 * @param[in]	buf_pa Physical address of buffer to be enqueued
 * @param[in]	length Length of the buffer
 * @param[in]	lifm TRUE means that the buffer is last buffer of a frame (last-in-frame)
 * @retval		EOK Success
 * @retval		EIO The slot is already occupied
 * @retval		EPERM Ring is locked and does not accept enqueue requests
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((hot)) errno_t pfe_hif_ring_enqueue_buf(pfe_hif_ring_t *ring, const void *buf_pa, uint32_t length, bool_t lifm)
{
	return pfe_hif_ring_enqueue_buf_std(ring, buf_pa, length, lifm);
}

/**
 * @brief		The "standard" HIF variant
 */
static inline errno_t pfe_hif_ring_enqueue_buf_std(pfe_hif_ring_t *ring, const void *buf_pa, uint32_t length, bool_t lifm)
{
	uint32_t tmp_ctrl_seq_w0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) | (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	We do not perform this check due to performance reasons. We expect that this
		enqueue functions is only called when there is room in the ring. */
#if 0
	/*	WB BD must be ENABLED. This is indication that SW finished processing the BD. */
	tmp_wb_bd_ctrl_w0 = ring->wr_wb_bd->rsvd_ctrl_w0;
	if (0U == (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
	{
		return EAGAIN;
	}
#endif /* 0 */

	/*	BD must be DISABLED. This indicates that BD is not going to be used by HW = is empty/unused. */
	tmp_ctrl_seq_w0 = ring->wr_bd->ctrl_seqnum_w0;
	if (unlikely(0U != (tmp_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN)))
	{
		NXP_LOG_ERROR("Can't insert buffer to %s ring since the BD entry #%d is already used\n", ring->is_rx? "RX" : "TX", ring->write_idx);
		return EIO;
	}
	else
	{
		/*	1.) Process the BD (write new data). */
		ring->wr_bd->data = (uint32_t)(addr_t)buf_pa;
		ring->wr_bd->rsvd_buflen_w1 = HIF_RING_BD_W1_BD_RSVD_STAT(0U) |
					      HIF_RING_BD_W1_BD_BUFFLEN((uint16_t)length);

		if (lifm)
		{
			tmp_ctrl_seq_w0 |= HIF_RING_BD_W0_LIFM;
		}
		else
		{
			tmp_ctrl_seq_w0 &= ~HIF_RING_BD_W0_LIFM;
		}

#ifdef EQ_DQ_RX_DEBUG
		if (ring->is_rx)
		{
			NXP_LOG_INFO("EQ: IDX:%02d, BD@p0x%p, WB@p0x%p, BUF@p0x%p\n",
				(ring->write_idx & RING_LEN_MASK),
				(void *)((addr_t)ring->wr_bd - ((addr_t)ring->base_va - (addr_t)ring->base_pa)),
				(void *)((addr_t)ring->wr_wb_bd - ((addr_t)ring->wb_tbl_base_va - (addr_t)ring->wb_tbl_base_pa)),
				(void *)buf_pa);
		}
#endif /* EQ_DQ_RX_DEBUG */

		/* Wait until wr_wb_bd is written */
		hal_wmb();

		/*	2.) Set the BD enable flag. */
		ring->wr_bd->ctrl_seqnum_w0 = (tmp_ctrl_seq_w0 | HIF_RING_BD_W0_DESC_EN);

		/*	3.) Increment the write pointer. This will indicate fill level increase. */
		inc_write_index_std(ring);
	}

	return EOK;
}

/**
 * @brief		Dequeue buffer form the ring
 * @details		Remove next buffer from the ring and increment the read index. If the
 * 				buffer is empty then the call fails and no operation is performed.
 * @param[in]	ring The ring instance
 * @param[out]	buf_pa Pointer where pointer to the dequeued buffer shall be written
 * @param[out]	length Pointer where length of the buffer shall be written
 * @param[out]	lifm Pointer where last-in-frame information shall be written
 * @retval		EOK Buffer dequeued
 * @retval		EAGAIN Current BD is busy
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((hot)) errno_t pfe_hif_ring_dequeue_buf(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm)
{
	return pfe_hif_ring_dequeue_buf_std(ring, buf_pa, length, lifm);
}

/**
 * @brief		The "standard" HIF variant
 */
static inline errno_t pfe_hif_ring_dequeue_buf_std(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm)
{
	uint32_t tmp_bd_ctrl_seq_w0;
	uint32_t tmp_wb_bd_ctrl_w0;
	uint32_t tmp_wb_bd_seq_buf_w1;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == buf_pa) || (NULL == length) || (NULL == lifm)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	WB BD must be DISABLED. This indicates that it is not being used by HW. */
	tmp_wb_bd_ctrl_w0 = ring->rd_wb_bd->rsvd_ctrl_w0;
	if(0U != (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
	{
		/* Immediate return (do not waste time reading non-cached memory)
		 * as the buffer is still used by HW */
		return EAGAIN;
	}

	/*	BD must be ENABLED. This indicates that SW has previously enqueued it. */
	tmp_bd_ctrl_seq_w0 = ring->rd_bd->ctrl_seqnum_w0;

	if (unlikely(0U == (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN)))
	{
		return EAGAIN;
	}
	else
	{
		tmp_wb_bd_seq_buf_w1 = ring->rd_wb_bd->seqnum_buflen_w1;

		/*	1.) Process the BD data. */
		*buf_pa = (void *)(addr_t)(ring->rd_bd->data);

#ifdef EQ_DQ_RX_DEBUG
		if (ring->is_rx)
		{
			NXP_LOG_INFO("DQ: IDX:%02d, BD@p0x%p, WB@p0x%p, BUF@p0x%p\n",
				(ring->read_idx & RING_LEN_MASK),
				(void *)((addr_t)ring->rd_bd - ((addr_t)ring->base_va - (addr_t)ring->base_pa)),
				(void *)((addr_t)ring->rd_wb_bd - ((addr_t)ring->wb_tbl_base_va - (addr_t)ring->wb_tbl_base_pa)),
				(void *)*buf_pa);
		}
#endif /* EQ_DQ_RX_DEBUG */

		*length = HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(tmp_wb_bd_seq_buf_w1);
		*lifm = (0 != (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_LIFM));

		/*	2.) Clear the BD ENABLE flag. This step invalidates the BD so HW can't use it again. */
		ring->rd_bd->ctrl_seqnum_w0 = (tmp_bd_ctrl_seq_w0 & ~HIF_RING_BD_W0_DESC_EN);

		/*	3.) Set the WB BD ENABLE flag. This is indication that the BD is DISABLED and can be reused by SW. */
		ring->rd_wb_bd->rsvd_ctrl_w0 = (tmp_wb_bd_ctrl_w0 | HIF_RING_WB_BD_W0_DESC_EN);

		/*	After steps 2.) and 3.) the BD can be reused by SW again (enqueue). */

		/*	4.) Increment the read pointer to next BD in the ring. */
		inc_read_index_std(ring);
	}

	return EOK;
}

/**
 * @brief		Dequeue buffer from the ring without response
 * @details		Remove next buffer from the ring and increment the read index. If the
 * 				buffer is empty then the call fails and no operation is performed. Can
 * 				be used to receive TX confirmations.
 * @param[in]	ring The ring instance
 * @param[out]	lifm Pointer where last-in-frame information shall be written
 * @param[out]	len Number of transmitted bytes
 * @retval		EOK Buffer dequeued
 * @retval		EAGAIN Current BD is busy
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((hot)) errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm)
{
	return pfe_hif_ring_dequeue_plain_std(ring, lifm);
}

/**
 * @brief		The "standard" HIF variant
 */
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm)
{
	uint32_t tmp_bd_ctrl_seq_w0;
	uint32_t tmp_wb_bd_ctrl_w0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	WB BD must be DISABLED. This indicates that it is not being used by HW. */
	tmp_wb_bd_ctrl_w0 = ring->rd_wb_bd->rsvd_ctrl_w0;
	if(0U != (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
	{
		/* Immediate return (do not waste time reading non-cached memory)
		 * as the buffer is still used by HW */
		return EAGAIN;
	}

	/* Perform single read */
	tmp_bd_ctrl_seq_w0 = ring->rd_bd->ctrl_seqnum_w0;

	/*	BD must be ENABLED. This indicates that SW has previously enqueued it. */
	if ((0U == (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN)))
	{
		return EAGAIN;
	}
	else
	{
		/*	1.) Process the BD data. */
		*lifm = (0U != (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_LIFM));

		/*	2.) Clear the BD ENABLE flag. This step invalidates the BD so HW can't use it again. */
		ring->rd_bd->ctrl_seqnum_w0 = (tmp_bd_ctrl_seq_w0 & ~HIF_RING_BD_W0_DESC_EN);

		/*	3.) Set the WB BD ENABLE flag. This is indication that the BD is DISABLED and can be reused by SW. */
		ring->rd_wb_bd->rsvd_ctrl_w0 = (tmp_wb_bd_ctrl_w0 | HIF_RING_WB_BD_W0_DESC_EN);

		/*	After steps 2.) and 3.) the BD can be reused by SW again (enqueue). */

		/*	4.) Increment the read pointer to next BD in the ring. */
		inc_read_index_std(ring);
	}

	return EOK;
}

/**
 * @brief		Drain buffer from ring
 * @details		This call dequeues previously enqueued buffer from a ring regardless it
 *				has been processed by the HW or not. Function is intended to properly
 *				shut-down the ring in terms of possibility to retrieve all currently
 *				enqueued entries. In case of RX ring this will return enqueued RX buffer.
 *				In case of TX ring the enqueued TX buffer will be returned.
 * @param[in]	ring The ring instance
 * @param[out]	buf_pa buf_pa Pointer where pointer to the dequeued buffer shall be written
 * @retval		EOK Buffer has been dequeued
 * @retval		ENOENT No more buffers in the ring
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_ring_drain_buf(pfe_hif_ring_t *ring, void **buf_pa)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (0U != pfe_hif_ring_get_fill_level(ring))
	{
		/*	Draining introduces sequence number corruption. Every enqueued
			BD increments sequence number in SW and every processed BD
			increments it in HW. In case when non-processed BDs are dequeued
			the new ones will be enqueued with sequence number not matching
			the current HW one. We need to adjust the SW value when draining
			non-processed BDs. */
		if ( 0 != (HIF_RING_WB_BD_W0_DESC_EN & ring->wr_wb_bd->rsvd_ctrl_w0))
		{
			/*	This BD has not been processed yet. Revert the enqueue. */
			*buf_pa = (void *)(addr_t)ring->wr_bd->data;
			ring->wr_bd->ctrl_seqnum_w0 &= ~HIF_RING_BD_W0_DESC_EN;
			ring->wr_wb_bd->rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
			dec_write_index_std(ring);
		}
		else
		{
			/*	Processed BD. Do standard dequeue. */
			*buf_pa = (void *)(addr_t)ring->rd_bd->data;
			ring->rd_bd->ctrl_seqnum_w0 &= ~HIF_RING_BD_W0_DESC_EN;
			ring->rd_wb_bd->rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
			inc_read_index_std(ring);
		}
	}
	else
	{
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief       Check if ring contains less than watermark-specified
 *              number of free entries
 * @param[in]   ring The ring instance
 * @return      TRUE if ring contains less than watermark-specified number
 *              of free entries
 * @note        Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) bool_t pfe_hif_ring_is_below_wm(const pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(NULL == ring))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return FALSE;
    }
#endif /* PFE_CFG_NULL_ARG_CHECK */

    /*	TODO: Make the water-mark value configurable */
    if (pfe_hif_ring_get_fill_level(ring) >= (RING_LEN / 2))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief		Invalidate the ring
 * @details		Disable all buffer descriptors in the ring
 * @param[in]	ring The ring instance
 * @note		Must not be preempted by: pfe_hif_ring_enqueue_buf(), pfe_hif_ring_destroy()
 */
__attribute__((cold, aligned(64))) void pfe_hif_ring_invalidate(const pfe_hif_ring_t *ring)
{
	pfe_hif_ring_invalidate_std(ring);
}

/**
 * @brief		The "standard" HIF variant
 */
__attribute__((cold, aligned(64))) static void pfe_hif_ring_invalidate_std(const pfe_hif_ring_t *ring)
{
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (ii=0U; ii<RING_LEN; ii++)
	{
		/*	Mark the descriptor as last BD and clear enable flag */
		(((pfe_hif_bd_t *)ring->base_va)[ii]).ctrl_seqnum_w0 &= ~HIF_RING_BD_W0_DESC_EN;
		(((pfe_hif_bd_t *)ring->base_va)[ii]).ctrl_seqnum_w0 |= HIF_RING_BD_W0_LAST_BD;

		/*	Reset the write-back descriptor */
		(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ii]).seqnum_buflen_w1 |= HIF_RING_WB_BD_W1_WB_BD_SEQNUM(0xffffU);
		(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ii]).rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
	}
}

/**
 * @brief		Dump of HW rings
 * @details		Dumps particular ring
 * @param[in]	ring The ring instance
 * @param[in]	name The ring name
 * @param[in]	dev Printing device instance
 * @param[in]	dev_print Device dependent print method
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 * @return		Number of bytes written to the buffer
 * @note		Must not be preempted by: pfe_hif_ring_enqueue_buf(), pfe_hif_ring_destroy()
 */
__attribute__((cold, aligned(64))) void pfe_hif_ring_dump(pfe_hif_ring_t *ring, char_t *name, void *dev, void (*dev_print)(void *dev, const char *fmt, ...), uint8_t verb_level)
{
	char_t *idx_str;
	bool_t pr_out;
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	dev_print(dev, "Ring %s: len %d\n", name, RING_LEN);
	dev_print(dev, "  Type: %s\n", ring->is_rx ? "RX" : "TX");
	dev_print(dev, "  Index w/r: %d/%d (%d/%d)\n", ring->write_idx & RING_LEN_MASK, ring->read_idx & RING_LEN_MASK, ring->write_idx, ring->read_idx);

	if (verb_level >= PFE_CFG_VERBOSITY_LEVEL) {
		/* BD ring */
		for (ii=0U; ii<RING_LEN; ii++)
		{
			pfe_hif_bd_t *bd = &(((pfe_hif_bd_t *)ring->base_va)[ii]);

			pr_out = FALSE;

			if (0 == ii)
			{
				dev_print(dev, "  BD va/pa v0x%px/p0x%px\n", ring->base_va, ring->base_pa);
				dev_print(dev, "            pa           idx: bufl:ctrl:  data  :  next  :seqn\n");
				pr_out = TRUE;
			}

			if ((ring->write_idx & RING_LEN_MASK) == ii)
			{
				idx_str = "<-- WR";
				pr_out = TRUE;
			}
			else if ((ring->read_idx & RING_LEN_MASK) == ii)
			{
				idx_str = "<-- RD";
				pr_out = TRUE;
			}
			else
			{
				idx_str = "";
			}

			if ((ii == 1) || (ii >= (RING_LEN - 2)) ||
				((ii > 1) && (((ring->read_idx & RING_LEN_MASK) - 1) == ii)) ||
				((ii < (RING_LEN - 2)) && (((ring->read_idx & RING_LEN_MASK) + 1) == ii)))
			{
				pr_out = TRUE;
			}

			if (TRUE == pr_out)
			{
				dev_print(dev, "    p0x%px%5d: %04x:%04x:%08x:%08x:%04x%s\n",(void *)&((pfe_hif_bd_t *)ring->base_pa)[ii], ii,
					  HIF_RING_BD_W1_BD_BUFFLEN_GET(bd->rsvd_buflen_w1), HIF_RING_BD_W0_BD_CTRL_GET(bd->ctrl_seqnum_w0),
					  bd->data, bd->next, HIF_RING_BD_W0_BD_SEQNUM_GET(bd->ctrl_seqnum_w0), idx_str);
			}
		}

		/* WB ring */
		{
			for (ii=0U; ii<RING_LEN; ii++)
			{
				pfe_hif_wb_bd_t *wb = &(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ii]);

				pr_out = FALSE;

				if (0 == ii)
				{
					dev_print(dev, "  WB va/pa v0x%px/p0x%px\n", ring->wb_tbl_base_va, ring->wb_tbl_base_pa);
					dev_print(dev, "            pa           idx: ctrl:bufl:seqn\n");
					pr_out = TRUE;
				}

				if ((ring->read_idx & RING_LEN_MASK) == ii)
				{
					idx_str = "<-- RD";
					pr_out = TRUE;
				}
				else
				{
					idx_str = "";
				}

				if ((ii == 1) || (ii >= (RING_LEN - 2)) ||
					((ii > 1) && (((ring->read_idx & RING_LEN_MASK) - 1) == ii)) ||
					((ii < (RING_LEN - 2)) && (((ring->read_idx & RING_LEN_MASK) + 1) == ii)))
				{
					pr_out = TRUE;
				}

				if (TRUE == pr_out)
				{
					dev_print(dev, "    p0x%px%5d: %04x:%04x:%04x%s\n", (void *)&((pfe_hif_wb_bd_t *)ring->wb_tbl_base_pa)[ii], ii, wb->rsvd_ctrl_w0,
						  HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(wb->seqnum_buflen_w1), HIF_RING_WB_BD_W1_WB_BD_SEQNUM_GET(wb->seqnum_buflen_w1), idx_str);
				}
			}
		}
	}
}

/**
 * @brief		Create new PFE buffer descriptor ring
 * @param[in]	rx If TRUE the ring is RX, if FALSE the the ring is TX
 * @param[in]	nocpy If TRUE then ring will be treated as HIF NOCPY variant
 * @return		The new ring instance or NULL if the call has failed
 * @note		Must not be preempted by any of the remaining API functions
 */
__attribute__((cold, aligned(64))) pfe_hif_ring_t *pfe_hif_ring_create(bool_t rx, bool_t nocpy)
{
	if (TRUE == nocpy)
	{
		NXP_LOG_ERROR("HIF NOCPY not supported\n");
		return NULL;
	}

	return pfe_hif_ring_create_std(rx);
}


/**
 * @brief		The "standard" HIF variant
 */
__attribute__((cold, aligned(64))) static pfe_hif_ring_t *pfe_hif_ring_create_std(bool_t rx)
{
	pfe_hif_ring_t *ring;
	uint32_t ii, size;
	pfe_hif_bd_t *hw_desc_va, *hw_desc_pa;
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
	char_t *variant_str;
#endif /* PFE_CFG_VERBOSITY_LEVEL */

	/*	Allocate the ring structure */
	ring = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_hif_ring_t), HAL_CACHE_LINE_SIZE);
	if (NULL == ring)
	{
		NXP_LOG_ERROR("Can't create BD ring; oal_mm_malloc_contig_aligned_cache() failed\n");
		return NULL;
	}

	memset(ring, 0, sizeof(pfe_hif_ring_t));
	ring->base_va = NULL;
	ring->wb_tbl_base_va = NULL;

	/*	Just a debug check */
	if (((addr_t)&ring->heavy_data_mark - (addr_t)ring) > HAL_CACHE_LINE_SIZE)
	{
		NXP_LOG_DEBUG("Suboptimal: Data split between two cache lines\n");
	}

	/*	Allocate memory for buffer descriptors. Should be DMA safe, contiguous, and 64-bit aligned. */
	if (0 != (HAL_CACHE_LINE_SIZE % 8))
	{
		NXP_LOG_DEBUG("Suboptimal: Cache line size is not 64-bit aligned\n");
		ii = 8U;
	}
	else
	{
		ii = HAL_CACHE_LINE_SIZE;
	}

	size = RING_LEN * sizeof(pfe_hif_bd_t);
	ring->base_va = oal_mm_malloc_contig_named_aligned_nocache(PFE_CFG_BD_MEM, size, ii);

	if (unlikely(NULL == ring->base_va))
	{
		NXP_LOG_ERROR("BD memory allocation failed\n");
		goto free_and_fail;
	}

	/*	It shall be ensured that a single BD does not split across 4k boundary */
	if (0 != (sizeof(pfe_hif_bd_t) % 8))
	{
		if ((((addr_t)ring->base_va + size) & (MAX_ADDR_T_VAL << 12)) > ((addr_t)ring->base_va & (MAX_ADDR_T_VAL << 12)))
		{
			NXP_LOG_ERROR("A buffer descriptor is crossing 4k boundary\n");
			goto free_and_fail;
		}
	}

	ring->base_pa = oal_mm_virt_to_phys_contig(ring->base_va);

	/*	Allocate memory for write-back descriptors */
	size = RING_LEN * sizeof(pfe_hif_wb_bd_t);
	ring->wb_tbl_base_va = oal_mm_malloc_contig_named_aligned_nocache(PFE_CFG_BD_MEM, size, ii);

	if (unlikely(NULL == ring->wb_tbl_base_va))
	{
		NXP_LOG_ERROR("WB BD memory allocation failed\n");
		goto free_and_fail;
	}

	/*	It shall be ensured that a single WB BD does not split across 4k boundary */
	if (0 != (sizeof(pfe_hif_bd_t) % 8))
	{
		if ((((addr_t)ring->wb_tbl_base_va + size) & (MAX_ADDR_T_VAL << 12)) > ((addr_t)ring->wb_tbl_base_va & (MAX_ADDR_T_VAL << 12)))
		{
			NXP_LOG_ERROR("A write-back buffer descriptor is crossing 4k boundary\n");
			goto free_and_fail;
		}
	}

	ring->wb_tbl_base_pa = oal_mm_virt_to_phys_contig(ring->wb_tbl_base_va);

	/*	Initialize state variables */
	ring->write_idx = 0U;
	ring->read_idx = 0U;
	ring->is_rx = rx;
	ring->rd_bd = (pfe_hif_bd_t *)ring->base_va;
	ring->wr_bd = (pfe_hif_bd_t *)ring->base_va;

	/*	Initialize memory */
	memset(ring->base_va, 0, RING_LEN * sizeof(pfe_hif_bd_t));

	/*	Chain the buffer descriptors */
	hw_desc_va = (pfe_hif_bd_t *)ring->base_va;
	hw_desc_pa = (pfe_hif_bd_t *)ring->base_pa;

	for (ii=0; ii<RING_LEN; ii++)
	{
		if (TRUE == ring->is_rx)
		{
			/*	Mark BD as RX */
			hw_desc_va[ii].ctrl_seqnum_w0 |= HIF_RING_BD_W0_DIR;
		}

		/*	Enable BD interrupt */
		hw_desc_va[ii].ctrl_seqnum_w0 |= HIF_RING_BD_W0_CBD_INT_EN;
		hw_desc_va[ii].next = (uint32_t)((addr_t)&hw_desc_pa[ii + 1U] & 0xffffffffU);
	}

	/*	Chain last one with the first one */
	hw_desc_va[ii-1].next = (uint32_t)((addr_t)&hw_desc_pa[0] & 0xffffffffU);
	hw_desc_va[ii-1].ctrl_seqnum_w0 |= HIF_RING_BD_W0_LAST_BD;

	/*	Initialize write-back descriptors */
	{
		pfe_hif_wb_bd_t *wb_bd_va;

		ring->rd_wb_bd = (pfe_hif_wb_bd_t *)ring->wb_tbl_base_va;
		ring->wr_wb_bd = (pfe_hif_wb_bd_t *)ring->wb_tbl_base_va;

		memset(ring->wb_tbl_base_va, 0, RING_LEN * sizeof(pfe_hif_wb_bd_t));

		wb_bd_va = (pfe_hif_wb_bd_t *)ring->wb_tbl_base_va;
		for (ii=0U; ii<RING_LEN; ii++)
		{
			wb_bd_va->seqnum_buflen_w1 |= HIF_RING_WB_BD_W1_WB_BD_SEQNUM(0xffffU);

			/*	Initialize WB BD descriptor enable flag. Once descriptor is processed,
				the PFE HW will clear it. */
			wb_bd_va->rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
			wb_bd_va++;
		}

	}

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
	if (ring->is_rx)
	{
		variant_str = "RX";
	}
	else
	{
		variant_str = "TX";
	}

	NXP_LOG_DEBUG("%s ring created. %d entries.\nBD @ p0x%p/v0x%p.\nWB @ p0x%p/v0x%p.\n",
					variant_str,
					RING_LEN,
					(void *)ring->base_pa,
					(void *)ring->base_va,
					(void *)ring->wb_tbl_base_pa,
					(void *)ring->wb_tbl_base_va);
#endif /* PFE_CFG_VERBOSITY_LEVEL */

	return ring;

free_and_fail:
	(void)pfe_hif_ring_destroy(ring);
	return NULL;
}

/**
 * @brief		Destroy BD ring
 * @param[in]	ring The ring instance
 * @note		Must not be preempted by any of the remaining API functions
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_ring_destroy(pfe_hif_ring_t *ring)
{
	if (NULL != ring)
	{
		/*	Invalidate and release the BD ring */
		if (NULL != ring->base_va)
		{
			pfe_hif_ring_invalidate(ring);
			oal_mm_free_contig(ring->base_va);
			ring->base_va = NULL;
		}

		/*	Release WB BD ring */
		if (NULL != ring->wb_tbl_base_va)
		{
			oal_mm_free_contig(ring->wb_tbl_base_va);
			ring->wb_tbl_base_va = NULL;
		}

		/*	Release the ring structure */
		oal_mm_free_contig(ring);
		ring = NULL;
	}

	return EOK;
}

