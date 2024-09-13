/* =========================================================================
 *  Copyright 2017-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup	dxgrBPOOL Buffer Pool
 * @brief		Buffer pool management
 * @details
 * 				Purpose
 * 				-------
 * 				The bpool modules' task is to provide pre-allocated buffers in form of a pool with
 * 				fast access to minimize buffer allocation overhead.
 *
 *				Initialization
 *				--------------
 *				A buffer pool instance is created by bpool_create() specifying depth of the pool as
 *				well as size of included buffers.
 *
 *				Operation
 *				---------
 *				When needed, user can request buffer from the pool via bpool_get(). Buffers are
 *				released back to the pool using the bpool_put(). To get current number of buffers
 *				in the pool the bpool_get_fill_level() can be called.
 *
 *				@note	Currently, the resource protection is handled by the pool instance itself
 *						and one does not need to implement custom concurrent access protection.
 *
 * 				Shutdown
 * 				--------
 * 				Pool can be released via bpool_destroy() call. Pool memory will be released
 * 				regardless all buffers are returned to the pool or not.	But it is highly
 * 				recommended before the destroy attempt to return all the buffers to prevent
 * 				possible crashes upon the pool is released and an entity will try to access
 * 				already	released buffer.
 *
 * 				@todo	If there is only one thread requesting buffers from the pool and only
 * 						one returning them back, the resource protection is not needed. Add
 * 						possibility to turn the internal resource protection off to improve
 * 						performance.
 *
 * @addtogroup	dxgrBPOOL
 * @{
 *
 * @file		bpool.h
 * @brief		The main BPOOL header file
 * @details		Use this header to include all the BPOOL-provided functionality
 *
 */

#ifndef SRC_BPOOL_H_
#define SRC_BPOOL_H_

/* Just for debugging */
#define NXP_MAGICINT 0x4e58505f /* NXP_ */
#define BPOOL_CFG_METADATA_LENGTH	64U

/*	This is buffer pool representation type */
typedef struct __attribute__((aligned(HAL_CACHE_LINE_SIZE)))
{
	/*	Whole block */
	void *block_origin_va;		/*	Origin VA of the memory block */
	void *block_origin_pa;		/*	Origin PA of the memory block */
	addr_t block_size;			/*	Reserved block size */
	addr_t block_pa_offset;		/*  Offset PA from VA (block_origin_va - block_origin_pa) */

	/* Buffers */
	addr_t buffer_pa_start;
	addr_t buffer_va_start;
	addr_t buffer_pa_end;	    /*	Upper boundary of the buffers region */
	addr_t buffer_va_end;	    /*	Upper boundary of the buffers region */

	/* Descriptors */
	addr_t bd_pa_start;
	addr_t bd_va_start;
	addr_t bd_pa_end;
	addr_t bd_va_end;

	/*	Pool */
	uint32_t buffer_raw_size;	/*	Per-buffer size */
	uint32_t buffer_align;		/*	Alignment per-buffer */
	uint32_t buffer_num;		/*	Number of buffers in the pool */
	volatile void *free_fifo;	/*	Here all the free buffers are stored */
	oal_mutex_t fifo_lock;		/*	Mutex protecting the FIFO */
} bpool_t;

typedef struct
{
	addr_t ustorage0;
	addr_t ustorage1;
} bpool_complex_storage_t;

typedef struct __attribute__((aligned(HAL_CACHE_LINE_SIZE)))
{
	void *vaddr;									/*	Buffer address (virtual) */
	void *paddr; 									/*	Buffer address (physical) */
	uint32_t len;									/*	Buffer length */
	uint32_t storage;								/*	General purpose storage associated with buffer */
	bpool_complex_storage_t cstorage;				/*	General purpose storage associated with buffer (complex) */
	uint8_t metadata[BPOOL_CFG_METADATA_LENGTH];	/*	General purpose storage associated with buffer (metadata) */
#ifdef BPOOL_CFG_MEM_BUF_WATCH
	uint32_t magicword;
#endif /* BPOOL_CFG_MEM_BUF_WATCH */
} bpool_rx_buf_t;

/**
 * @brief		Returns buffer size of any buffer in the bpool
 * @param[in]	pool The bpool instance
 * @retval		On success buffer size, on error (NULL pointer) -1 is returned
 */
__attribute__((pure, hot)) static inline int32_t bpool_get_buf_len(const bpool_t *const pool)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return -1;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	return (int32_t)pool->buffer_raw_size;
}

/**
 * @brief		Calculates pointer of bd belonging to va in pool
 * @param[in]	pool The bpool instance
 * @param[in]	va Virtual address of a buffer from a pool
 * @retval		Pointer to bd of the buffer if found or NULL when not found
 */
__attribute__((pure, hot)) static inline bpool_rx_buf_t *bpool_get_bd(const bpool_t *const pool, const void *const va)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef BPOOL_CFG_MEM_REGION_WATCH
	if (((addr_t)va >= pool->buffer_va_start) && ((addr_t)va <= pool->buffer_va_end))
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
	{
		/* MATCH, now calculate the buffer's BD */
		uint32_t buf_idx = ((addr_t)va - pool->buffer_va_start) / pool->buffer_align;
		bpool_rx_buf_t *bd = (bpool_rx_buf_t *)(pool->bd_va_start + (buf_idx * sizeof(bpool_rx_buf_t)));

#ifdef BPOOL_CFG_MEM_BUF_WATCH
		if (NXP_MAGICINT != bd->magicword)
			NXP_LOG_ERROR("%s: Memory region check failure\n", __func__);
#endif /* BPOOL_CFG_MEM_BUF_WATCH */

		return bd;
	}
#ifdef BPOOL_CFG_MEM_REGION_WATCH
	else
	{
		return NULL;
	}
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
}

/**
 * @brief		Get unsigned storage
 * @details		Each buffer within pool has associated a storage which can be used by
 *				application for its purposes (store a value associated with buffer).
 *				This function returns pointer to the pre-allocated memory. Its type is
 *				'unsigned'.
 * @param[in]	pool The bpool instance
 * @param[in]	va Virtual address of a buffer from the pool which storage will be returned
 * @return		Pointer to the storage (virtual) or NULL if failed
 */
__attribute__((pure, hot)) static inline uint32_t * bpool_get_unsigned_storage(const bpool_t *const pool, const void *va)
{
	bpool_rx_buf_t *bd;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bd = bpool_get_bd(pool, va);

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bd))
	{
		NXP_LOG_ERROR("Can't get BD\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return &(bd->storage);
}

/**
 * @brief		Get complex storage
 * @details		Each buffer within pool has associated a storage which can be used by
 *				application for its purposes (store a value associated with buffer).
 *				This function returns pointer to the pre-allocated memory. Its type is
 *				'bpool_complex_storage_t'.
 * @param[in]	pool The bpool instance
 * @param[in]	va Virtual address of a buffer from the pool which storage will be returned
 * @return		Pointer to the storage (virtual) or NULL if failed
 */
__attribute__((pure, hot)) static inline bpool_complex_storage_t * bpool_get_complex_storage(const bpool_t *const pool, const void *va)
{
	bpool_rx_buf_t *bd;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bd = bpool_get_bd(pool, va);

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bd))
	{
		NXP_LOG_ERROR("Can't get BD\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return &(bd->cstorage);
}

/**
 * @brief		Get metadata storage
 * @details		Each buffer within pool has associated a storage which can be used by
 *				application for its purposes (store a value associated with buffer).
 *				This function returns pointer to the pre-allocated memory of size
 *				bpool_get_meta_storage_size().
 * @param[in]	pool The bpool instance
 * @param[in]	va Virtual address of a buffer from the pool which storage will be returned
 * @return		Pointer to the storage (virtual) or NULL if failed
 */
__attribute__((pure, hot)) static inline void * bpool_get_meta_storage(const bpool_t *const pool, const void *va)
{
	bpool_rx_buf_t *bd;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bd = bpool_get_bd(pool, va);

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bd))
	{
		NXP_LOG_ERROR("Can't get BD\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return (void *)(bd->metadata);
}

/**
 * @brief		Get metadata storage size in bytes
 * @return		Size of the storage in bytes
 */
__attribute__((pure, cold, aligned(64))) static inline addr_t bpool_get_meta_storage_size(void)
{
	return BPOOL_CFG_METADATA_LENGTH;
}

/**
 * @brief		Convert physical address of a buffer to virtual one
 * @param[in]	pool The bpool instance
 * @param[in]	pa The physical address to convert
 * @return		Virtual address associated with 'pa' or NULL if not found
 */
__attribute__((pure, hot)) static inline void * bpool_get_va(const bpool_t *const pool, void *pa)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef BPOOL_CFG_MEM_REGION_WATCH
	/*	Check if address belongs to THIS block memory range */
	if (((addr_t)pa <= pool->buffer_pa_end) && ((addr_t)pa >= pool->buffer_pa_start))
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
	{
		return (void*)((addr_t)pa + pool->block_pa_offset);
	}
#ifdef BPOOL_CFG_MEM_REGION_WATCH
	else
	{
		return NULL;
	}
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
}

/**
 * @brief		Convert virtual address or a buffer to physical one
 * @param[in]	pool The bpool instance
 * @param[in]	va The virtual address to convert
 * @return		Physical address associated with 'va' or NULL if not found
 */
__attribute__((pure, hot)) static inline void * bpool_get_pa(const bpool_t *const pool, void *va)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pool))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef BPOOL_CFG_MEM_REGION_WATCH
	/*	Check if address belongs to THIS block memory range */
	if (((addr_t)va <= pool->buffer_va_end) && ((addr_t)va >= pool->buffer_va_start))
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
	{
		return (void*)((addr_t)va - pool->block_pa_offset);
	}
#ifdef BPOOL_CFG_MEM_REGION_WATCH
	else
	{
		return NULL;
	}
#endif /* BPOOL_CFG_MEM_REGION_WATCH */
}

bpool_t * bpool_create(uint32_t depth, uint32_t buf_size, uint32_t align, bool_t cached) __attribute__((cold, aligned(64)));
void * bpool_get(bpool_t *pool) __attribute__((hot));
void bpool_put(bpool_t *pool, const void *va) __attribute__((hot));
errno_t bpool_get_fill_level(bpool_t *pool, uint32_t *fill_level) __attribute__((hot));
uint32_t bpool_get_depth(const bpool_t *pool) __attribute__((pure, hot));
errno_t bpool_destroy(bpool_t * pool) __attribute__((cold, aligned(64)));
errno_t bpool_clear(bpool_t * pool) __attribute__((cold, aligned(64)));

#endif /* SRC_BPOOL_H_ */

/** @}*/
