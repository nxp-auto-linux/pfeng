/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
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
#include "pfe_hif_ring.h"

#define RING_LEN						PFE_HIF_RING_CFG_LENGTH
#define RING_LEN_MASK					(PFE_HIF_RING_CFG_LENGTH - 1U)

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
#define HIF_RING_BD_W0_BD_CTRL_OFFSET		(15U)

#define HIF_RING_BD_W0_BD_SEQNUM(seqnum)	\
		(((seqnum) & HIF_RING_BD_W0_BD_SEQNUM_MASK)	<< \
					 HIF_RING_BD_W0_BD_SEQNUM_OFFSET)
#define HIF_RING_BD_W0_BD_SEQNUM_GET(seqnum)	\
		(((seqnum) >> HIF_RING_BD_W0_BD_SEQNUM_OFFSET) & \
					  HIF_RING_BD_W0_BD_SEQNUM_MASK)

#define HIF_RING_BD_W0_BD_CTRL(ctrl)	\
		(((ctrl) & HIF_RING_BD_W0_BD_CTRL_MASK)	<< \
					 HIF_RING_BD_W0_BD_CTRL_OFFSET)
#define HIF_RING_BD_W0_BD_CTRL_GET(ctrl)	\
		(((ctrl) >> HIF_RING_BD_W0_BD_CTRL_OFFSET) & \
					  HIF_RING_BD_W0_BD_CTRL_MASK)


/* Buffer descriptor WORD1 */
#define HIF_RING_BD_W1_BD_BUFFLEN_MASK		(0xFFFFU)
#define HIF_RING_BD_W1_BD_BUFFLEN_OFFSET	(0U)
#define HIF_RING_BD_W1_BD_RSVD_STAT_MASK	(0xFFFFU)
#define HIF_RING_BD_W1_BD_RSVD_STAT_OFFSET	(15U)

#define HIF_RING_BD_W1_BD_BUFFLEN(buflen)	\
		(((buflen) & HIF_RING_BD_W1_BD_BUFFLEN_MASK)	<< \
					 HIF_RING_BD_W1_BD_BUFFLEN_OFFSET)
#define HIF_RING_BD_W1_BD_BUFFLEN_GET(buflen)	\
		(((buflen) >> HIF_RING_BD_W1_BD_BUFFLEN_OFFSET) & \
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
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET	(15U)

#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN(buflen)	\
		(((buflen) & HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_MASK)	<< \
					 HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_OFFSET)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM(seqnum)	\
		(((seqnum) & HIF_RING_WB_BD_W1_WB_BD_SEQNUM_MASK)	<< \
				HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET)

#define HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(buflen)	\
		(((buflen) >> HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_OFFSET) & \
					  HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_MASK)
#define HIF_RING_WB_BD_W1_WB_BD_SEQNUM_GET(seqnum)	\
		(((seqnum) >> HIF_RING_WB_BD_W1_WB_BD_SEQNUM_OFFSET) & \
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
typedef struct __attribute__((packed)) __pfe_hif_bd_tag
{
	volatile uint32_t ctrl_seqnum_w0;
	volatile uint32_t rsvd_buflen_w1;
	volatile uint32_t data;
	volatile uint32_t next;
} pfe_hif_bd_t;

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief	The BD as seen by HIF NOCPY
 * @details	Properly pack to form the structure as expected by HIF NOCPY.
 * @note	Don't use the 'aligned' attribute here since behavior
 * 			is implementation-specific (due to the bitfields). Still
 * 			applies that BD shall be aligned to 64-bits and in
 * 			ideal case to cache line size.
 * @warning	Do not touch the structure (even types) unless you know
 * 			what you're doing.
 */
typedef struct __attribute__((packed)) __pfe_hif_nocpy_bd_tag
{
	union
	{
		struct
		{
			union
			{
				volatile uint16_t rx_reserved;
				volatile uint16_t tx_buflen;
			};

			union
			{
				volatile uint16_t ctrl;
				struct
				{
					volatile uint16_t cbd_int_en	: 1;
					volatile uint16_t pkt_int_en	: 1;
					volatile uint16_t lifm			: 1;
					volatile uint16_t last_bd		: 1;	/*	Not used */
					volatile uint16_t dir			: 1;
					volatile uint16_t lmem_cpy		: 1;
					volatile uint16_t reserved1		: 2;
					volatile uint16_t pkt_xfer		: 1;
					volatile uint16_t reserved2		: 6;
					volatile uint16_t desc_en		: 1;
				};
			};
		};
		volatile uint32_t ctrl_txlen_w0;
	};

	union
	{
		struct
		{
			union
			{
				volatile uint16_t rx_buflen;
				volatile uint16_t tx_status;
			};

			union
			{
				volatile uint16_t rx_status;
				struct
				{
					uint16_t tx_portno		: 3;
					uint16_t tx_queueno		: 4;
					uint16_t tx_reserved4	: 9;
				};
			};
		};
		volatile uint32_t stat_rxlen_txstat_w1;
	};

	volatile uint32_t data;
	volatile uint32_t next;

} pfe_hif_nocpy_bd_t;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief	The write-back BD as seen by HIF
 * @note	Don't use the 'aligned' attribute here since behavior
 * 			is implementation-specific (due to the bitfields). Still
 * 			applies that BD shall be aligned to 64-bits and in
 * 			ideal case to cache line size.
 * @warning	Do not touch the structure (even types) unless you know
 * 			what you're doing.
 */
typedef struct __attribute__((packed)) __pfe_hif_wb_bd_tag
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
struct __attribute__((aligned (HAL_CACHE_LINE_SIZE), packed)) __pfe_hif_ring_tag
{
	/*	Put often used data from beginning to improve cache locality */

	/*	Every 'enqueue' and 'dequeue' access */
	void *base_va;				/*	Ring base address (virtual) */
	void *wb_tbl_base_va;		/*	Write-back table base address (virtual) */
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	uint16_t seqnum;			/*	Current sequence number */
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

	/*	Every 'enqueue' access */
	uint32_t write_idx;			/*	BD index to be written */
	union						/* Pointer to BD to be written */
	{
		pfe_hif_bd_t *wr_bd;
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		pfe_hif_nocpy_bd_t *wr_bd_nocpy;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	};

#if (TRUE == HAL_HANDLE_CACHE)
	union						/*	Pointer to BD to be written (PA). Only due to CACHE_* macros in QNX... */
	{
		pfe_hif_bd_t *wr_bd_pa;
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		pfe_hif_nocpy_bd_t *wr_bd_nocpy_pa;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	};
#endif /* HAL_HANDLE_CACHE */
	pfe_hif_wb_bd_t *wr_wb_bd;	/*	Pointer to WB BD to be written */
	bool_t is_rx;				/*	If TRUE then ring is RX ring */
	bool_t is_nocpy;			/*	If TRUE then ring is HIF NOCPY variant */

	/*	Every 'dequeue' access */
	uint32_t read_idx;			/*	BD index to be read */
	union						/*	Pointer to BD to be read */
	{
		pfe_hif_bd_t *rd_bd;
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		pfe_hif_nocpy_bd_t *rd_bd_nocpy;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	};

	pfe_hif_wb_bd_t *rd_wb_bd;	/*	Pointer to WB BD to be read */
	bool_t heavy_data_mark;		/*	To enable getting size of heavily accessed data */

	/*	Initialization time only */
	void *base_pa;				/*	Ring base address (physical) */
	void *wb_tbl_base_pa;		/*	Write-back table base address (physical) */
};

__attribute__((hot)) static inline void inc_write_index_std(pfe_hif_ring_t *ring);
__attribute__((hot)) static inline void dec_write_index_std(pfe_hif_ring_t *ring);
__attribute__((hot)) static inline void inc_read_index_std(pfe_hif_ring_t *ring);
__attribute__((cold)) static pfe_hif_ring_t *pfe_hif_ring_create_std(uint16_t seqnum, bool_t rx);
static inline errno_t pfe_hif_ring_enqueue_buf_std(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm);
static inline errno_t pfe_hif_ring_dequeue_buf_std(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm);
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len);
#else
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm);
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
__attribute__((cold)) static void pfe_hif_ring_invalidate_std(pfe_hif_ring_t *ring);
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
__attribute__((hot)) static inline void inc_write_index_nocpy(pfe_hif_ring_t *ring);
__attribute__((hot)) static inline void inc_read_index_nocpy(pfe_hif_ring_t *ring);
__attribute__((cold)) static pfe_hif_ring_t *pfe_hif_ring_create_nocpy(uint16_t seqnum, bool_t rx);
static inline errno_t pfe_hif_ring_enqueue_buf_nocpy(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm);
static inline errno_t pfe_hif_ring_dequeue_buf_nocpy(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm);
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
static inline errno_t pfe_hif_ring_dequeue_plain_nocpy(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len);
#else
static inline errno_t pfe_hif_ring_dequeue_plain_nocpy(pfe_hif_ring_t *ring, bool_t *lifm);
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
__attribute__((cold)) static void pfe_hif_ring_invalidate_nocpy(pfe_hif_ring_t *ring);
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

__attribute__((hot)) static inline void inc_write_index_std(pfe_hif_ring_t *ring)
{
	ring->write_idx++;
	ring->wr_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->write_idx & RING_LEN_MASK];
	ring->wr_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->write_idx & RING_LEN_MASK];
}

__attribute__((hot)) static inline void dec_write_index_std(pfe_hif_ring_t *ring)
{
	ring->write_idx--;
	ring->wr_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->write_idx & RING_LEN_MASK];
	ring->wr_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->write_idx & RING_LEN_MASK];
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
__attribute__((hot)) static inline void inc_write_index_nocpy(pfe_hif_ring_t *ring)
{
	ring->write_idx++;
	ring->wr_bd_nocpy = &((pfe_hif_nocpy_bd_t *)ring->base_va)[ring->write_idx & RING_LEN_MASK];
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

__attribute__((hot)) static inline void inc_read_index_std(pfe_hif_ring_t *ring)
{
	ring->read_idx++;
	ring->rd_bd = &((pfe_hif_bd_t *)ring->base_va)[ring->read_idx & RING_LEN_MASK];
	ring->rd_wb_bd = &((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ring->read_idx & RING_LEN_MASK];
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
__attribute__((hot)) static inline void inc_read_index_nocpy(pfe_hif_ring_t *ring)
{
	ring->read_idx++;
	ring->rd_bd_nocpy = &((pfe_hif_nocpy_bd_t *)ring->base_va)[ring->read_idx & RING_LEN_MASK];
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		Check if ring contains less than watermark-specified
 * 				number of free entries
 * @param[in]	ring The ring instance
 * @return		TRUE if ring contains less than watermark-specified number
 * 				of free entries
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) bool_t pfe_hif_ring_is_below_wm(pfe_hif_ring_t *ring)
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
 * @brief		Get fill level
 * @param[in]	ring The ring instance
 * @return		Number of occupied entries within the ring
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) uint32_t pfe_hif_ring_get_fill_level(pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return RING_LEN;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	/*	In case of HIF NOCPY, the HW does not use external RX buffers but internal
	 	BMU-provided buffers. Thus the RX ring fill level can't be other value
	 	than zero. */

	if ((ring->is_nocpy) && (ring->is_rx))
	{
		return 0U;
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		return (ring->write_idx - ring->read_idx);
	}
}

/**
 * @brief		Get physical address of the start of the ring
 * @param[in]	ring The ring instance
 * @return		Pointer to the beginning address of the ring
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, cold)) void *pfe_hif_ring_get_base_pa(pfe_hif_ring_t *ring)
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
__attribute__((pure, cold)) void *pfe_hif_ring_get_wb_tbl_pa(pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (ring->is_nocpy)
	{
		/*	NOCPY ring does not use write-back descriptors */
		return NULL;
	}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

	return ring->wb_tbl_base_pa;
}

/**
 * @brief		Get length of the write-back table
 * @param[in]	ring The ring instance
 * @return		Length of the table in number of entries. Only valid when
 *				pfe_hif_ring_get_wb_tbl_pa() is not NULL.
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, cold)) uint32_t pfe_hif_ring_get_wb_tbl_len(pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ring->is_nocpy)
	{
		/*	NOCPY ring does not use write-back descriptors */
		return 0U;
	}
#else /* PFE_CFG_HIF_NOCPY_SUPPORT */
    (void)ring;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

	return RING_LEN;
}

/**
 * @brief		Get length of the ring
 * @param[in]	ring The ring instance
 * @return		Ring length in number of entries
 * @note		Must not be preempted by: pfe_hif_ring_destroy()
 */
__attribute__((pure, hot)) uint32_t pfe_hif_ring_get_len(pfe_hif_ring_t *ring)
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
__attribute__((hot)) errno_t pfe_hif_ring_enqueue_buf(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm)
{
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ring->is_nocpy)
	{
		return pfe_hif_ring_enqueue_buf_nocpy(ring, buf_pa, length, lifm);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		return pfe_hif_ring_enqueue_buf_std(ring, buf_pa, length, lifm);
	}
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
/**
 * @brief		Set egress interface ID
 * @details		This function allows to modify egress interface to be used to transmit
 * 				frame given by subsequent pfe_hif_ring_enqueue_buf() call. Only reason
 * 				why this exists is the HIF NOCPY 'direct' mode where the egress interface
 * 				must be present in the TX buffer descriptor because the post classification
 * 				header which is required to be prepend to the frame and contains the
 * 				information is partially ignored and the egress interface identifier is
 * 				overwritten with the one programmed within the TX descriptor. Enjoy.
 * @param[in]	ring The ring instance
 * @param[in]	id The physical interface ID
 * @note		Only valid for HIF NOCPY TX ring manipulation. All other usages will be
 * 				ignored.
 */
__attribute__((hot)) void pfe_hif_ring_set_egress_if(pfe_hif_ring_t *ring, pfe_ct_phy_if_id_t id)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ring->wr_bd_nocpy->tx_portno = id;
}
#endif /* PFE_CFG_HIF_NOCPY_DIRECT */

/**
 * @brief		The HIF NOCPY variant
 * @param[in]	buf_pa This must be BMU2 allocated physical address
 */
static inline errno_t pfe_hif_ring_enqueue_buf_nocpy(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) | (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely(ring->is_rx))
	{
		NXP_LOG_ERROR("There is nothing to enqueue into RX ring in case of HIF NOCPY\n");
		return EPERM;
	}
	else
	{
		/*	Write the HW BD. Always write all control word bits since
		 	the HIF NOCPY is clearing the flags once BD is processed... */
		ring->wr_bd_nocpy->data = (uint32_t)((addr_t)buf_pa & 0xffffffffU);
		ring->wr_bd_nocpy->tx_buflen = (uint16_t)length;
		ring->wr_bd_nocpy->tx_status = 0xf0U; /* This is from reference code. Not documented. */

#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
		/*	In case the HIF NOCPY TX works in 'direct' mode (packet is not passed to classifier but
		 	directly transmitted via physical interface. We must deliver the physical interface
		 	identifier here (because of course the headers we're putting together within the HIF
			channel layer and which contains the interface identifier, is being overwritten by
			information taken from the TX BD... So we need to get the egress interface and configure
			the TX BD here...

			For this purpose the pfe_hif_ring_set_egress_if() has been implemented to not completely
			break the HIF ring API logic. Therefore we do not touch the tx_portno here and expect
			that caller has already called the pfe_hif_ring_set_egress_if().
		*/

		/* ring->wr_bd_nocpy->tx_portno = 0U; */
#else
#error LMEM copy mode not implemented yet
#endif /* PFE_HIF_RING_CFG_NOCPY_DIRECT_MODE */


		ring->wr_bd_nocpy->tx_queueno = 0U;
		ring->wr_bd_nocpy->lmem_cpy = 0U;

		if (lifm)
		{
			ring->wr_bd_nocpy->lifm = 1U;
		}
		else
		{
			ring->wr_bd_nocpy->lifm = 0U;
		}

		/*	Write the BD 'enable' bit */
		ring->wr_bd_nocpy->desc_en = 1U;

		/*	Increment the write pointer */
		inc_write_index_nocpy(ring);
	}

	return EOK;
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		The "standard" HIF variant
 */
static inline errno_t pfe_hif_ring_enqueue_buf_std(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm)
{
	uint32_t tmp_ctrl_seq_w0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) | (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	tmp_ctrl_seq_w0 = ring->wr_bd->ctrl_seqnum_w0;

	if (unlikely(0U != (tmp_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN)))
	{
		NXP_LOG_ERROR("Can't insert buffer since the BD entry is already used\n");
		return EIO;
	}
	else
	{
		/*	Write the HW BD */
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

#ifdef PFE_CFG_HIF_SEQNUM_CHECK
		/* Discard old SEQ */
		tmp_ctrl_seq_w0 &= ~(HIF_RING_BD_W0_BD_SEQNUM_MASK << HIF_RING_BD_W0_BD_SEQNUM_OFFSET);
		/* Set new SEQ counter */
		tmp_ctrl_seq_w0 |= HIF_RING_BD_W0_BD_SEQNUM(ring->seqnum);
		ring->seqnum++;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

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

		/*	Write the BD 'enable' bit */

		ring->wr_wb_bd->rsvd_ctrl_w0 |= HIF_RING_WB_BD_W0_DESC_EN;
		/* Wait until wr_wb_bd is written */
		hal_wmb();
		ring->wr_bd->ctrl_seqnum_w0 = (tmp_ctrl_seq_w0 | HIF_RING_BD_W0_DESC_EN);

		/*	Increment the write pointer */
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
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ring->is_nocpy)
	{
		return pfe_hif_ring_dequeue_buf_nocpy(ring, buf_pa, length, lifm);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		return pfe_hif_ring_dequeue_buf_std(ring, buf_pa, length, lifm);
	}
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		The HIF NOCPY variant
 */
static inline errno_t pfe_hif_ring_dequeue_buf_nocpy(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == buf_pa) || (NULL == length) || (NULL == lifm)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* if (unlikely(0U != ring->rd_bd_nocpy->desc_en)) */
	if (1U != ring->rd_bd_nocpy->pkt_xfer)
	{
		return EAGAIN;
	}
	else
	{
		*buf_pa = (void *)(addr_t)PFE_CFG_MEMORY_PFE_TO_PHYS(ring->rd_bd_nocpy->data);

		if (ring->is_rx)
		{
			*length = ring->rd_bd_nocpy->rx_buflen;
		}
		else
		{
			*length = ring->rd_bd_nocpy->tx_buflen;
		}

		*lifm = (0U != ring->rd_bd_nocpy->lifm);

		/*	Re-enable the descriptor so HIF can write another RX buffer there */
		ring->rd_bd_nocpy->pkt_xfer = 0U;
		ring->rd_bd_nocpy->desc_en = 1U;

		/*	Increment the read pointer */
		inc_read_index_nocpy(ring);
	}

	return EOK;
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

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


	tmp_wb_bd_ctrl_w0 = ring->rd_wb_bd->rsvd_ctrl_w0;
	if(0U != (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
	{
		/* Immediate return (do not waste time reading non-cached memory)
		 * as the buffer is still used by HW */
		return EAGAIN;
	}
	/* Single read buffer descriptor */
	tmp_bd_ctrl_seq_w0 = ring->rd_bd->ctrl_seqnum_w0;
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	tmp_wb_bd_seq_buf_w1 = ring->rd_wb_bd->seqnum_buflen_w1;
#endif

	if ((0U == (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN))
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
		|| (HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(tmp_wb_bd_seq_buf_w1) != HIF_RING_BD_W0_BD_SEQNUM_GET(tmp_ctrl_seq_w0))
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */
		)
	{
		return EAGAIN;
	}
	else
	{
#ifndef PFE_CFG_HIF_SEQNUM_CHECK
		tmp_wb_bd_seq_buf_w1 = ring->rd_wb_bd->seqnum_buflen_w1;
#endif
		/*	Reset BD EN flag so it is not reused by HW. */
		ring->rd_bd->ctrl_seqnum_w0 = (tmp_bd_ctrl_seq_w0 & ~HIF_RING_BD_W0_DESC_EN);

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
		/*	Increment the read pointer */
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
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
__attribute__((hot)) errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len)
#else
__attribute__((hot)) errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm)
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
{
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ring->is_nocpy)
	{
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		return pfe_hif_ring_dequeue_plain_nocpy(ring, lifm, len);
#else
		return pfe_hif_ring_dequeue_plain_nocpy(ring, lifm);
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		return pfe_hif_ring_dequeue_plain_std(ring, lifm, len);
#else
		return pfe_hif_ring_dequeue_plain_std(ring, lifm);
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
	}
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		The HIF NOCPY variant
 */
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
static inline errno_t pfe_hif_ring_dequeue_plain_nocpy(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len)
#else
static inline errno_t pfe_hif_ring_dequeue_plain_nocpy(pfe_hif_ring_t *ring, bool_t *lifm)
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* if (unlikely((0U != ring->rd_bd_nocpy->desc_en) || (0U == pfe_hif_ring_get_fill_level(ring)))) */
	if (0U == ring->rd_bd_nocpy->pkt_xfer) /* TODO: Is this OK also within RX ring? */
	{
		/*	Nothing to dequeue */
		return EAGAIN;
	}
	else
	{
		/*
			Return the LIFM flag

		 	HIF NOCPY TX BDP will always overwrite the BD so the LIFM
			flag will be set to zero (very smart...). It must be ensured
			that the HIF NOCPY ring will be used in the one-frame=one-BD
			manner.
		*/
		*lifm = TRUE;

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		/*	Return transmitted buffer length */
		*len = ring->rd_bd_nocpy->tx_buflen;
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

		/*	Clear the 'TX done' flag */
		ring->rd_bd_nocpy->pkt_xfer = 0U;
		ring->rd_bd_nocpy->desc_en = 0U;

		/*	Increment the read pointer */
		inc_read_index_nocpy(ring);
	}

	return EOK;
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		The "standard" HIF variant
 */
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len)
#else
static inline errno_t pfe_hif_ring_dequeue_plain_std(pfe_hif_ring_t *ring, bool_t *lifm)
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
{
	uint32_t tmp_bd_ctrl_seq_w0;
	uint32_t tmp_wb_bd_ctrl_w0;
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	uint32_t tmp_wb_bd_seq_buf_w1;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	tmp_wb_bd_ctrl_w0 = ring->rd_wb_bd->rsvd_ctrl_w0;
	if(0U != (tmp_wb_bd_ctrl_w0 & HIF_RING_WB_BD_W0_DESC_EN))
	{
		/* Immediate return (do not waste time reading non-cached memory)
		 * as the buffer is still used by HW */
		return EAGAIN;
	}

	/* Perform single read */
	tmp_bd_ctrl_seq_w0 = ring->rd_bd->ctrl_seqnum_w0;

#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	tmp_wb_bd_seq_buf_w1 = ring->rd_wb_bd->seqnum_buflen_w1;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

	if ((0U == (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_DESC_EN))
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
		|| (HIF_RING_WB_BD_W1_WB_BD_BUFFLEN_GET(tmp_wb_bd_seq_buf_w1) != HIF_RING_BD_W0_BD_SEQNUM_GET(tmp_ctrl_seq_w0))
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */
		)
	{
		return EAGAIN;
	}
	else
	{
		/*	Return LIFM */
		*lifm = (0U != (tmp_bd_ctrl_seq_w0 & HIF_RING_BD_W0_LIFM));

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		/*	Return transmitted buffer length */
		*len = HIF_RING_BD_W1_BD_BUFFLEN_GET(ring->rd_bd->rsvd_buflen_w1);
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

		ring->rd_bd->ctrl_seqnum_w0 = (tmp_bd_ctrl_seq_w0  & ~HIF_RING_BD_W0_DESC_EN);

		/*	Increment the read pointer */
		inc_read_index_std(ring);
	}

	return EOK;
}

/**
 * @brief		Drain buffer from ring
 * @details		This call dequeues previously enqueued buffer from a ring regardless it
 *				has been processed by the HW or not. Function is intended to properly
 *				shut-down the ring in terms of possibility to retrieve all currently
 *				enqueued entries.
 * @param[in]	ring The ring instance
 * @param[out]	buf_pa buf_pa Pointer where pointer to the dequeued buffer shall be written
 * @retval		EOK Buffer has been dequeued
 * @retval		ENOENT No more buffers in the ring
 */
__attribute__((cold)) errno_t pfe_hif_ring_drain_buf(pfe_hif_ring_t *ring, void **buf_pa)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (ring->is_nocpy && ring->is_rx)
	{
		bool_t lifm;
		uint32_t len;

		/*	In this case we will do standard dequeue until the ring is empty. This
			will ensure that application can drain RX buffers and return all BMU
			buffers back to the HW pool. */
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		if (EOK == pfe_hif_ring_dequeue_plain_nocpy(ring, &lifm, &len))
#else
		if (EOK == pfe_hif_ring_dequeue_plain_nocpy(ring, &lifm))
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
		{
			return EOK;
		}
		else
		{
			return ENOENT;
		}
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	if (0U != pfe_hif_ring_get_fill_level(ring))
	{
		/*	In case of RX ring this will return enqueued RX buffer. In
			case of TX ring the enqueued TX buffer will be returned. */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		if (ring->is_nocpy)
		{
			*buf_pa = (void *)(addr_t)ring->rd_bd_nocpy->data;
			ring->rd_bd_nocpy->desc_en = 0U;
			inc_read_index_nocpy(ring);
		}
		else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
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
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
				ring->seqnum--;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */
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
	}
	else
	{
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief		Invalidate the ring
 * @details		Disable all buffer descriptors in the ring
 * @param[in]	ring The ring instance
 * @note		Must not be preempted by: pfe_hif_ring_enqueue_buf(), pfe_hif_ring_destroy()
 */
__attribute__((cold)) void pfe_hif_ring_invalidate(pfe_hif_ring_t *ring)
{
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ring))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ring->is_nocpy)
	{
		pfe_hif_ring_invalidate_nocpy(ring);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_ring_invalidate_std(ring);
	}

	return;
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		The HIF NOCPY variant
 */
__attribute__((cold)) static void pfe_hif_ring_invalidate_nocpy(pfe_hif_ring_t *ring)
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
		/*	Zero-out the EN flag */
		(((pfe_hif_nocpy_bd_t *)ring->base_va)[ii]).desc_en = 0U;

		/*	Mark the descriptor as last BD */
		(((pfe_hif_nocpy_bd_t *)ring->base_va)[ii]).last_bd = 1U;
	}
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		The "standard" HIF variant
 */
__attribute__((cold)) static void pfe_hif_ring_invalidate_std(pfe_hif_ring_t *ring)
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
		/*	Mark the descriptor as last BD and set enable flag */
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
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 * @return		Number of bytes written to the buffer
 * @note		Must not be preempted by: pfe_hif_ring_enqueue_buf(), pfe_hif_ring_destroy()
 */
__attribute__((cold)) uint32_t pfe_hif_ring_dump(pfe_hif_ring_t *ring, char_t *name, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t ii;
	uint32_t len = 0U;
	char_t *idx_str;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ring) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Ring %s: len %d\n", name, RING_LEN);
	len += (uint32_t)oal_util_snprintf(buf + len, size - len, "  Type: %s\n", ring->is_rx ? "RX" : "TX");
	len += (uint32_t)oal_util_snprintf(buf + len, size - len, "  Index w/r: %d/%d (%d/%d)\n", ring->write_idx & RING_LEN_MASK, ring->read_idx & RING_LEN_MASK, ring->write_idx, ring->read_idx);
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	len += (uint32_t)oal_util_snprintf(buf + len, size - len, "  Seqn: 0x%x\n", ring->seqnum);
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

	if(verb_level >= 8) {
		/* BD ring */
		for (ii=0U; ii<RING_LEN; ii++)
		{

			pfe_hif_bd_t *bd = &(((pfe_hif_bd_t *)ring->base_va)[ii]);
			if (0 == ii)
			{
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "  BD va/pa v0x%px/p0x%px\n", ring->base_va, ring->base_pa);
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "            pa           idx: bufl:ctrl: status :  data  :  next  :seqn\n");
			}

			if ((ring->write_idx & RING_LEN_MASK) == ii)
			{
				idx_str = "<-- WR";
			}
			else if ((ring->read_idx & RING_LEN_MASK) == ii)
			{
				idx_str = "<-- RD";
			}
			else
			{
				idx_str = "";
			}

			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "    p0x%px%5d: %04x:%04x:%08x:%08x:%04x%s\n",(void *)&((pfe_hif_bd_t *)ring->base_pa)[ii], ii, HIF_RING_BD_W1_BD_BUFFLEN_GET(bd->rsvd_buflen_w1), HIF_RING_BD_W0_BD_CTRL_GET(bd->ctrl_seqnum_w0), bd->data, bd->next, HIF_RING_BD_W0_BD_SEQNUM_GET(bd->ctrl_seqnum_w0), idx_str);
		}

		/* WB ring */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		if (FALSE == ring->is_nocpy)
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
		{
			for (ii=0U; ii<RING_LEN; ii++)
			{
				pfe_hif_wb_bd_t *wb = &(((pfe_hif_wb_bd_t *)ring->wb_tbl_base_va)[ii]);
				if (0 == ii)
				{
					len += (uint32_t)oal_util_snprintf(buf + len, size - len, "  WB va/pa v0x%px/p0x%px\n", ring->wb_tbl_base_va, ring->wb_tbl_base_pa);
					len += (uint32_t)oal_util_snprintf(buf + len, size - len, "    pa:      idx:  ctl: rsvd :bufl:seqn\n");
				}

				if ((ring->read_idx & RING_LEN_MASK) == ii)
				{
					idx_str = "<-- RD";
				}
				else
				{
					idx_str = "";
				}

				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "    p0x%px%5d: %04x:%06x:%04x:%04x%s\n", (void *)&((pfe_hif_wb_bd_t *)ring->wb_tbl_base_pa)[ii], ii, HIF_RING_BD_W0_BD_CTRL(wb->rsvd_ctrl_w0), HIF_RING_WB_BD_W1_WB_BD_BUFFLEN(wb->seqnum_buflen_w1), HIF_RING_WB_BD_W1_WB_BD_SEQNUM(wb->seqnum_buflen_w1), idx_str);
			}
		}
	}

	return len;
}

/**
 * @brief		Create new PFE buffer descriptor ring
 * @param[in]	rx If TRUE the ring is RX, if FALSE the the ring is TX
 * @param[in]	seqnum Initial sequence number
 * @param[in]	nocpy If TRUE then ring will be treated as HIF NOCPY variant
 * @return		The new ring instance or NULL if the call has failed
 * @note		Must not be preempted by any of the remaining API functions
 */
__attribute__((cold)) pfe_hif_ring_t *pfe_hif_ring_create(bool_t rx, uint16_t seqnum, bool_t nocpy)
{
#if !defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (TRUE == nocpy)
	{
		NXP_LOG_ERROR("HIF NOCPY support not enabled\n");
		return NULL;
	}
#else
	if (TRUE == nocpy)
	{
		return pfe_hif_ring_create_nocpy(seqnum, rx);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		return pfe_hif_ring_create_std(seqnum, rx);
	}
}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		The HIF NOCPY variant
 */
__attribute__((cold)) static pfe_hif_ring_t *pfe_hif_ring_create_nocpy(uint16_t seqnum, bool_t rx)
{
	pfe_hif_ring_t *ring;
	uint32_t ii, size;
	pfe_hif_nocpy_bd_t *hw_desc_va, *hw_desc_pa;

	(void)seqnum;

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
	ring->wb_tbl_base_pa = NULL;
	ring->rd_wb_bd = NULL;
	ring->is_nocpy = TRUE;

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

	size = RING_LEN * sizeof(pfe_hif_nocpy_bd_t);
	ring->base_va = oal_mm_malloc_contig_aligned_nocache(size, ii);

	if (unlikely(NULL == ring->base_va))
	{
		NXP_LOG_ERROR("BD memory allocation failed\n");
		goto free_and_fail;
	}

	/*	It shall be ensured that a single BD does not split across 4k boundary */
	if (0 != (sizeof(pfe_hif_nocpy_bd_t) % 8))
	{
		if ((((addr_t)ring->base_va + size) & (MAX_ADDR_T_VAL << 12)) > ((addr_t)ring->base_va & (MAX_ADDR_T_VAL << 12)))
		{
			NXP_LOG_ERROR("A buffer descriptor is crossing 4k boundary\n");
			goto free_and_fail;
		}
	}

	ring->base_pa = oal_mm_virt_to_phys_contig(ring->base_va);

	/*	S32G HIFNCPY AXI MASTER can only access range 0x00000000 - 0xbfffffff */
	if (unlikely((addr_t)ring->base_pa > (addr_t)0xBFFFFFFFU))
	{
		NXP_LOG_WARNING("Descriptor ring memory not in required range: starts @ p0x%p\n", ring->base_pa);
	}

	/*	Initialize state variables */
	ring->write_idx = 0U;
	ring->read_idx = 0U;
	ring->is_rx = rx;
	ring->rd_bd_nocpy = (pfe_hif_nocpy_bd_t *)ring->base_va;
	ring->wr_bd_nocpy = (pfe_hif_nocpy_bd_t *)ring->base_va;

	/*	Initialize memory */
	memset(ring->base_va, 0, RING_LEN * sizeof(pfe_hif_nocpy_bd_t));

	/*	Chain the buffer descriptors */
	hw_desc_va = (pfe_hif_nocpy_bd_t *)ring->base_va;
	hw_desc_pa = (pfe_hif_nocpy_bd_t *)ring->base_pa;

	for (ii=0; ii<RING_LEN; ii++)
	{
		if (TRUE == ring->is_rx)
		{
			/*	Mark BD as RX */
			hw_desc_va[ii].dir = 1U;
			/*	Enable the descriptor */
			hw_desc_va[ii].desc_en = 1U;
		}
		else
		{
			hw_desc_va[ii].dir = 0U;
			hw_desc_va[ii].desc_en = 0U;
		}

		/*	Enable BD interrupt */
		hw_desc_va[ii].cbd_int_en = 1U;

		hw_desc_va[ii].next = (uint32_t)((addr_t)(&hw_desc_pa[ii + 1U]) & 0xffffffffU);
	}

	/*	Chain last one with the first one */
	hw_desc_va[ii-1].next = (uint32_t)((addr_t)(&hw_desc_pa[0]) & 0xffffffffU);
	hw_desc_va[ii-1].last_bd = 1U;

	return ring;

free_and_fail:
	(void)pfe_hif_ring_destroy(ring);
	return NULL;
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		The "standard" HIF variant
 */
__attribute__((cold)) static pfe_hif_ring_t *pfe_hif_ring_create_std(uint16_t seqnum, bool_t rx)
{
	pfe_hif_ring_t *ring;
	uint32_t ii, size;
	pfe_hif_bd_t *hw_desc_va, *hw_desc_pa;
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
	char_t *variant_str;
#endif /* PFE_CFG_VERBOSITY_LEVEL */

#ifndef PFE_CFG_HIF_SEQNUM_CHECK
	(void)seqnum;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

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
	ring->is_nocpy = FALSE;

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

#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	ring->seqnum = seqnum;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */

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
__attribute__((cold)) errno_t pfe_hif_ring_destroy(pfe_hif_ring_t *ring)
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
