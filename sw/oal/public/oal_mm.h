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
 * @addtogroup	dxgrOAL
 * @{
 *
 * @defgroup    dxgr_OAL_MM MM
 * @brief		Memory management abstraction
 * @details
 * 				Purpose
 * 				-------
 * 				Purpose of the oal_mm is to abstract some memory management tasks like:
 *
 * 				- Memory allocation (physically contiguous, aligned, cached)
 * 				- Virtual to physical and vice versa address conversion
 * 				- Cache maintenance
 *
 * 				Initialization
 * 				--------------
 * 				The oal_mm needs to be initialized by the oal_mm_init().
 *
 * 				Operation
 * 				---------
 * 				The provided API can be used to manage memory as defined by API.
 *
 * 				Shutdown
 * 				--------
 * 				Library should be properly terminated when no more needed via the
 * 				oal_mm_shutdown() call.
 *
 *
 * @addtogroup  dxgr_OAL_MM
 * @{
 *
 * @file		oal_mm.h
 * @brief		The oal_mm module header file.
 * @details		This file contains generic memory management-related API.
 *
 */

#ifndef PUBLIC_OAL_MM_H_
#define PUBLIC_OAL_MM_H_

/**
 * @brief		Initialize the memory management library
 * @details		The oal_mm must be initialized by this call before it can be used.
 * @param[in]	dev The OS specific device structure associated with the memory management. Only for Linux supported.
 * @return		EOK if success
 */
errno_t oal_mm_init(void *dev);

/**
 * @brief		Shut the memory management library down
 * @details		Call this function when the oal_mm is no more needed. The call
 * 				will properly release all internal resources.
 */
void oal_mm_shutdown(void);

/**
 * @brief		Allocate contiguous aligned and non-cache-able memory region
 * @param[in]	size Length of the requested region in bytes
 * @param[in]	align The alignment value in bytes the physical address of the new allocated
 * 					  region shall be aligned to.
 * @return		Pointer (virtual) to the start of allocated region or NULL if the call has failed.
 */
void *oal_mm_malloc_contig_aligned_nocache(const addr_t size, const uint32_t align);

/**
 * @brief		Allocate contiguous aligned and cache-enabled memory region
 * @param[in]	size Length of the requested region in bytes
 * @param[in]	align The alignment value in bytes the physical address of the new allocated
 * 					  region shall be aligned to.
 * @return		Pointer (virtual) to the start of allocated region or NULL if the call has failed.
 */
void *oal_mm_malloc_contig_aligned_cache(const addr_t size, const uint32_t align);

/**
 * @brief		Allocate contiguous aligned and non-cache-able memory region from named memory pool
 * @param[in]	pool Name of the memory pool
 * @param[in]	size Length of the requested region in bytes
 * @param[in]	align The alignment value in bytes the physical address of the new allocated
 * 					  region shall be aligned to.
 * @return		Pointer (virtual) to the start of allocated region or NULL if the call has failed.
 */
void *oal_mm_malloc_contig_named_aligned_nocache(const char_t *pool, const addr_t size, const uint32_t align);

/**
 * @brief		Allocate contiguous aligned and cache-enabled memory region from named memory pool
 * @param[in]	pool Name of the memory pool
 * @param[in]	size Length of the requested region in bytes
 * @param[in]	align The alignment value in bytes the physical address of the new allocated
 * 					  region shall be aligned to.
 * @return		Pointer (virtual) to the start of allocated region or NULL if the call has failed.
 */
void *oal_mm_malloc_contig_named_aligned_cache(const char_t *pool, const addr_t size, const uint32_t align);

/**
 * @brief		Release a previously allocated memory
 * @details		Dispose memory region previously allocated by oal_mm_malloc_contig_*() calls.
 * @param		vaddr Pointer to the memory to be released (virtual)
 */
void oal_mm_free_contig(const void *vaddr);

/**
 * @brief		Convert virtual address to physical
 * @details		Only applicable to memory allocated by oal_mm_alloc_config_xxx variants
 * @param[in]	vaddr The virtual address to be converted
 * @return		Physical address associated with the virtual one or NULL if failed
 */
void *oal_mm_virt_to_phys_contig(void *vaddr);

/**
 * @brief		Allocate memory
 * @details		This is intended to perform standard memory allocations (malloc-like).
 * @param[in]	size Number of bytes to allocate
 * @return		Pointer (virtual) to the allocated space or NULL if failed
 */
void *oal_mm_malloc(const addr_t size);

/**
 * @brief		Free allocated memory
 * @details		Dispose memory region previously allocated by oal_mm_malloc() (free-like).
 * @param[in]	vaddr Pointer to the memory to be released (virtual)
 */
void oal_mm_free(const void *vaddr);

/**
 * @brief		Convert virtual address to physical
 * @details		Only applicable to memory managed by oal_mm module.
 * @param[in]	vaddr The virtual address to be converted
 * @return		Physical address associated with the virtual one or NULL if failed
 */
void *oal_mm_virt_to_phys(void *vaddr);

/**
 * @brief		Convert physical address to virtual
 * @details		Only applicable to memory managed by oal_mm module.
 * @param[in]	paddr The physical address to be converted
 * @return		Virtual address associated with the physical one or NULL if failed
 */
void *oal_mm_phys_to_virt(void *paddr);

/**
 * @brief		Map a physical memory region into a process's address space
 * @details		Enable access to the device's registers
 * @param[in]	paddr The physical address to be mapped
 * @param[in]	len Memory region length
 * @return		Virtual address associated with the physical one or NULL if failed
 */
void *oal_mm_dev_map(void *paddr, const addr_t len);

/**
 * @brief		Map a physical memory region into a process's address space
 * @details		Enable access to the device's registers, cachable variant
 * @param[in]	paddr The physical address to be mapped
 * @param[in]	len Memory region length
 * @return		Virtual address associated with the physical one or NULL if failed
 */
void *oal_mm_dev_map_cache(void *paddr, const addr_t len);

/**
 * @brief		Unmap previously mapped physical memory region
 * @details		Removes any mappings
 * @param[in]	paddr The physical address to be unmapped
 * @param[in]	len Memory region length
 * @return		EOK if success
 */
errno_t oal_mm_dev_unmap(void *paddr, const addr_t len);

/**
 * @brief		Invalidate cache
 * @param[in]	vaddr Memory region start (virtual)
 * @param[in]	paddr Memory region start (physical)
 * @param[in]	len Memory region length
 */
void oal_mm_cache_inval(const void *vaddr, const void *paddr, const addr_t len);

/**
 * @brief		Flush cache
 * @param[in]	vaddr Memory region start (virtual)
 * @param[in]	paddr Memory region start (physical)
 * @param[in]	len Memory region length
 */
void oal_mm_cache_flush(const void *vaddr, const void *paddr, const addr_t len);

/**
 * @brief		Get cache line size in bytes
 * @return		Cache line size in bytes
 */
uint32_t oal_mm_cache_get_line_size(void);


#endif /* PUBLIC_OAL_MM_H_ */

/** @}*/
/** @}*/
