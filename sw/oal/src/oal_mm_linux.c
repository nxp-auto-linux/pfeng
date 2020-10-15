/* =========================================================================
 *  Copyright 2018-2020 NXP
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
 * @addtogroup  dxgr_OAL_MM
 * @{
 *
 * @file		oal_mm_linux.c
 * @brief		The oal_mm module source file (Linux).
 * @details		This file contains Linux-specific memory management implementation.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"

#define PTR_ALIGN(p, a)         ((typeof(p))ALIGN((unsigned long)(p), (a)))

/**
 * Memory range properties
 */
#define HWB_CFG_MEM_BUF_WATCH
#define NXP_MAGICINT 0x5f4e5850 /* _NXP */
struct mem_props
{
#ifdef HWB_CFG_MEM_BUF_WATCH
	uint32_t magicword;
#endif /* HWB_CFG_MEM_BUF_WATCH */
	addr_t map_start_va;		/*	Memory region start virt(kernel) address */
	addr_t map_start_pa;		/*	Memory region start phys  address */
	addr_t map_length;

	addr_t buf_start_va;		/*	Original VA given by process calling the __hwb_malloc_contig() */
	dma_addr_t handle;
};

#define OAL_CACHE_ALLIGN	64

/* struct device *dev associated with mm */
static struct device *__dev = NULL;

/**
 *	Allocate physically contiguous buffer physically aligned to "align" bytes. If
 *	"align" is zero, no alignment will be performed.
 */
static void *__oal_mm_malloc_contig(const addr_t size, const uint32_t align, const bool_t cacheable)
{
	void *vaddr, *paddr;
	addr_t offset;
	addr_t map_start_va;
	addr_t map_start_pa;
	const addr_t map_length = (size + align + sizeof(struct mem_props));
	struct mem_props *props;
	dma_addr_t handle;

	/*	Get memory block */
	vaddr = dma_alloc_coherent(__dev, map_length, &handle, GFP_ATOMIC);
	if (!vaddr)
	{
		return NULL;
	}
	paddr = (void*)handle;

	map_start_va = (unsigned long)(vaddr);
	map_start_pa = (unsigned long)(paddr);

	/*	Align the PA */
	paddr += sizeof(struct mem_props);

	if (align)
	{
		paddr = (void *)PTR_ALIGN(paddr, align);
	}

	offset = (addr_t)(paddr) - map_start_pa;

	/*	Move VA by the same offset than the aligned PA to point to same location */
	vaddr += offset;

	/*	Set buffer properties */
	props = (struct mem_props *)(vaddr - sizeof(struct mem_props));
	props->map_length = map_length;
	props->map_start_va = map_start_va;
	props->map_start_pa = map_start_pa;
	props->buf_start_va = (addr_t)vaddr;
	props->handle = handle; //FIXME: use map_start_pa instead and add 'offs'
#ifdef HWB_CFG_MEM_BUF_WATCH
	props->magicword = NXP_MAGICINT;
#endif /* HWB_CFG_MEM_BUF_WATCH */

	if (TRUE == cacheable)
	{
		/*	Flush the cache over props */
		//TODO: needed on ARM
	}

	return vaddr;
}

/**
 *	Allocate aligned, contiguous, non-cacheable memory region
 */
void *oal_mm_malloc_contig_aligned_nocache(const addr_t size, const uint32_t align)
{
	return __oal_mm_malloc_contig(size, align, FALSE);
}

/**
 *	Allocate aligned, contiguous, cacheable memory region
 */
void *oal_mm_malloc_contig_aligned_cache(const addr_t size, const uint32_t align)
{
	return __oal_mm_malloc_contig(size, align, TRUE);
}

/**
 *	Allocate aligned, contiguous, non-cacheable memory region from named pool
 */
void *oal_mm_malloc_contig_named_aligned_nocache(const char_t *pool, const addr_t size, const uint32_t align)
{
	// TODO: switch to reserved memory
	return __oal_mm_malloc_contig(size, align, FALSE);
}

/**
 *	Allocate aligned, contiguous, cached memory region from named pool
 */
void *oal_mm_malloc_contig_named_aligned_cache(const char_t *pool, const addr_t size, const uint32_t align)
{
	// TODO: switch to reserved memory
	return __oal_mm_malloc_contig(size, align, TRUE);
}

/**
 *	Release memory allocated by __hwb_malloc_contig()
 */
void oal_mm_free_contig(const void *vaddr)
{
	struct mem_props *props = (struct mem_props *)(vaddr - sizeof(struct mem_props));

	if (NULL == vaddr)
	{
		NXP_LOG_ERROR("Attempt to release NULL-pointed memory\n");
	}
	else
	{
#ifdef HWB_CFG_MEM_BUF_WATCH
		if (NXP_MAGICINT != props->magicword)
		{
			NXP_LOG_ERROR("%s: Memory region check failure\n", __func__);
		}
#endif /* HWB_CFG_MEM_BUF_WATCH */
		dma_free_coherent(__dev, props->map_length, (void *)props->map_start_va, props->handle);
	}
}

/**
 * Standard memory allocation
 */
void *oal_mm_malloc(const addr_t size)
{
	return kzalloc(size, GFP_KERNEL);
}

/**
 * Standard memory release
 */
void oal_mm_free(const void *vaddr)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == vaddr))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	kfree((void *)vaddr);
}

void *oal_mm_virt_to_phys_contig(void *vaddr)
{
	struct mem_props *props = (struct mem_props *)(vaddr - sizeof(struct mem_props));
	int64_t offs;

#ifdef HWB_CFG_MEM_BUF_WATCH
		if (NXP_MAGICINT != props->magicword)
		{
			NXP_LOG_ERROR("%s: Memory region check failure\n", __func__);
		}
#endif /* HWB_CFG_MEM_BUF_WATCH */

	offs = (int64_t)vaddr - props->buf_start_va;
	offs += props->buf_start_va - props->map_start_va;
	if(offs < 0) {
		NXP_LOG_ERROR("%s: @@@!!!@@@ Virt to phys calculation failed (offset=%lld \n", __func__, offs);
		return NULL;
	}

	return (void *)(props->map_start_pa + offs);
}

/**
 *	Try to find physical address associated with mapped virtual range
 */
void *oal_mm_virt_to_phys(void *vaddr)
{
	return (void *)virt_to_phys((volatile void *)vaddr);
}

/**
 * Get virtual address based on physical address. Function assumes that region containing the 'paddr'
 * is inside kernel address space.
 */
void *oal_mm_phys_to_virt(void *paddr)
{
	return phys_to_virt((phys_addr_t)paddr);
}

void *oal_mm_dev_map(void *paddr, const addr_t len)
{
	return ioremap_nocache((resource_size_t)paddr, len);
}

void *oal_mm_dev_map_cache(void *paddr, const addr_t len)
{
	return ioremap((resource_size_t)paddr, len);
}

errno_t oal_mm_dev_unmap(void *paddr, const addr_t len)
{
	iounmap(paddr);
	return 0;
}

void oal_mm_cache_inval(const void *vaddr, const void *paddr, const addr_t len)
{
	//TODO
	return;
}

void oal_mm_cache_flush(const void *vaddr, const void *paddr, const addr_t len)
{
	//TODO
	return;
}

uint32_t oal_mm_cache_get_line_size(void)
{
	return OAL_CACHE_ALLIGN; //oal_cache_context.cache_line_size;
}

errno_t oal_mm_init(void *dev)
{
	__dev = (struct device *)dev;

	return EOK;
}

void oal_mm_shutdown(void)
{
	__dev = NULL;

	return;
}

/** @}*/
