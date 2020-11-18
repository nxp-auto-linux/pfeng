/* =========================================================================
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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
#include <linux/hashtable.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"

#define PTR_ALIGN(p, a)         ((typeof(p))ALIGN((unsigned long)(p), (a)))

enum pfe_kmem_type
{
	PFE_MEM_INVALID = 0,
	PFE_MEM_KMALLOC,
};

struct pfe_kmem
{
	struct hlist_node node;
	enum pfe_kmem_type type;
	void *addr;
};

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

/* define hash table to store address */
static DEFINE_HASHTABLE(pfe_addr_htable, 8);

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
static void *__oal_mm_kmalloc_htable(const addr_t size)
{
	struct pfe_kmem *hnode;
	void *new_mem;

	new_mem = kmalloc(size, GFP_KERNEL);
	if (!new_mem)
		return NULL;

	hnode = kmalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		kfree(new_mem);
		return NULL;
	}

	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)new_mem);
	hnode->type = PFE_MEM_KMALLOC;
	hnode->addr = new_mem;

	return new_mem;
}

static void __oal_mm_kfree_htable(struct pfe_kmem *hnode)
{
	hnode->type = PFE_MEM_INVALID;
	if (hnode)
	{
		if (hnode->addr)
			kfree(hnode->addr);
		kfree(hnode);
	}
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
	/* All kmalloc memory is automatically aligned to at least 128B(or higher power of two) on arm64.
	 * This should be replaced with genpool align allocator in future.*/
	if (align && 0 != (ARCH_KMALLOC_MINALIGN % align))
	{
		NXP_LOG_ERROR("Alignment not supported\n");
		return NULL;
	}
	return __oal_mm_kmalloc_htable(size);
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
	/* All kmalloc memory is automatically aligned to at least 128B(or higher power of two) on arm64.
	 * This should be replaced with genpool align allocator in future.*/
	if (align && 0 != (ARCH_KMALLOC_MINALIGN % align))
	{
		NXP_LOG_ERROR("Alignment not supported\n");
		return NULL;
	}
	return __oal_mm_kmalloc_htable(size);
}

/**
 *	Release memory allocated by __hwb_malloc_contig()
 */
void oal_mm_free_contig(const void *vaddr)
{
	struct mem_props *props = (struct mem_props *)(vaddr - sizeof(struct mem_props));
	struct pfe_kmem *mem;
	if (NULL == vaddr)
	{
		NXP_LOG_ERROR("Attempt to release NULL-pointed memory\n");
	}
	else
	{
		hash_for_each_possible(pfe_addr_htable, mem, node, (uint64_t)vaddr) {
			if (vaddr == mem->addr)
			{
				if (PFE_MEM_KMALLOC == mem->type) {
					hash_del(&mem->node);
					__oal_mm_kfree_htable(mem);
					return;
				}
				else
				{
					NXP_LOG_ERROR("Invalid oal_mm_free_contig\n");
				}
			}
		}
/* Check only for dma coherent memory */
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
	hash_init(pfe_addr_htable);

	return EOK;
}

void oal_mm_shutdown(void)
{
	__dev = NULL;
	if (!hash_empty(pfe_addr_htable)) {
		NXP_LOG_ERROR("Unfreed memory detected\n");
	}

	return;
}

/** @}*/
