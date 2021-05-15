/* =========================================================================
 *  Copyright 2018-2021 NXP
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/genalloc.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"

enum pfe_kmem_type {
	PFE_MEM_INVALID = 0,
	PFE_MEM_KMALLOC,
	PFE_MEM_DMA_ALLOC,
	PFE_MEM_RESERVED_ALLOC,
	PFE_MEM_BMU2_RESERVED_ALLOC,
};

struct pfe_kmem {
	struct hlist_node node;
	enum pfe_kmem_type type;
	void *addr;
	addr_t size;
	dma_addr_t dma_addr;
};

struct pfe_reserved_mem {
	struct list_head node;
	char_t *name;
	struct gen_pool *pool_alloc;
	void *map_start_va;
	phys_addr_t map_start_pa;
	addr_t map_size;
};

static LIST_HEAD(pfe_reserved_mem_list);
static void *bmu_start_va = NULL;
static phys_addr_t bmu_start_pa;

#define OAL_CACHE_ALLIGN	64

/* struct device *dev associated with mm */
static struct device *__dev = NULL;

/* define hash table to store address */
static DEFINE_HASHTABLE(pfe_addr_htable, 8);

static struct pfe_reserved_mem *__oal_mm_reserved_mem_get(const char_t *name)
{
	struct pfe_reserved_mem *res_mem = NULL;
	bool_t found = false;

	if (list_empty(&pfe_reserved_mem_list))
		return NULL;

	list_for_each_entry(res_mem, &pfe_reserved_mem_list, node) {
		if (!strcmp(res_mem->name, name)) {
			found = true;
			break;
		}
	}

	if (!found)
		res_mem = NULL;

	return res_mem;
}

/**
 *	Allocate physically contiguous buffer physically aligned to "align" bytes.
 */
static void *__oal_mm_dma_alloc_htable(const addr_t size, const uint32_t align)
{
	struct pfe_kmem *hnode;
	void *vaddr;
	dma_addr_t dma_addr;

	/*	Get memory block */
	vaddr = dma_alloc_coherent(__dev, size, &dma_addr, GFP_KERNEL);
	if (!vaddr)
		return NULL;

	if (!IS_ALIGNED(dma_addr, align ? : OAL_CACHE_ALLIGN)) {
		dma_free_coherent(__dev, size, vaddr, dma_addr);
		NXP_LOG_ERROR("Alignment not supported\n");
		return NULL;
	}

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		dma_free_coherent(__dev, size, vaddr, dma_addr);
		return NULL;
	}

	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);
	hnode->type = PFE_MEM_DMA_ALLOC;
	hnode->addr = vaddr;
	hnode->size = size;
	hnode->dma_addr = dma_addr;

	return vaddr;
}

static void __oal_mm_dma_free_htable(struct pfe_kmem *hnode)
{
	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr)
		dma_free_coherent(__dev, hnode->size, hnode->addr, hnode->dma_addr);
	kfree(hnode);
}

static void *__oal_mm_kmalloc_htable(const addr_t size)
{
	struct pfe_kmem *hnode;
	void *vaddr;

	vaddr = kmalloc(size, GFP_KERNEL);
	if (!vaddr)
		return NULL;

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		kfree(vaddr);
		return NULL;
	}

	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);
	hnode->type = PFE_MEM_KMALLOC;
	hnode->addr = vaddr;

	return vaddr;
}

static void __oal_mm_kfree_htable(struct pfe_kmem *hnode)
{
	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr)
		kfree(hnode->addr);
	kfree(hnode);
}

static void *__oal_mm_reserved_mem_alloc_htable(struct gen_pool *pool_alloc, const addr_t size, const uint32_t align)
{
	struct pfe_kmem *hnode;
	void *vaddr;

	if (align && ((1 << pool_alloc->min_alloc_order) % align)) {
		NXP_LOG_ERROR("Alignment not supported\n");
		return NULL;
	}

	vaddr = (void *)gen_pool_alloc(pool_alloc, size);
	if (!vaddr)
		return NULL;

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		gen_pool_free(pool_alloc, (unsigned long)vaddr, size);
		return NULL;
	}

	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);
	hnode->type = PFE_MEM_RESERVED_ALLOC;
	hnode->addr = vaddr;
	hnode->size = size;

	return vaddr;
}

static void __oal_mm_reserved_mem_free_htable(struct pfe_kmem *hnode)
{
	struct pfe_reserved_mem *res_mem = __oal_mm_reserved_mem_get(PFE_CFG_BD_MEM);

	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr && res_mem->pool_alloc)
		gen_pool_free(res_mem->pool_alloc, (unsigned long)hnode->addr, hnode->size);
	kfree(hnode);
}

static void *__oal_mm_reserved_sys_mem_alloc_htable(struct pfe_reserved_mem *res_mem, const addr_t size, const uint32_t align)
{
	struct pfe_kmem *hnode;

	if (!IS_ALIGNED(res_mem->map_start_pa, align)) {
		NXP_LOG_ERROR("BMU2 buffer pool reserved mem region addr not aligned\n");
		return NULL;
	}

	if (res_mem->map_size < size) {
		NXP_LOG_ERROR("BMU2 buffer pool reserved mem region size exceeded\n");
		/* try default allocation */
		return NULL;
	}

	if (bmu_start_va) {
		NXP_LOG_ERROR("Allocation attempt in BMU2 exclusive zone\n");
		/* try default allocation */
		return NULL;
	}

	bmu_start_va = res_mem->map_start_va;
	bmu_start_pa = res_mem->map_start_pa;

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		bmu_start_va = NULL;
		return NULL;
	}

	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)bmu_start_va);
	hnode->type = PFE_MEM_BMU2_RESERVED_ALLOC;
	hnode->addr = bmu_start_va;
	hnode->size = size;

	return bmu_start_va;
}

static void __oal_mm_reserved_sys_mem_free_htable(struct pfe_kmem *hnode)
{
	bmu_start_va = NULL;
	kfree(hnode);
}

/**
 *	Allocate aligned, contiguous, non-cacheable memory region
 */
void *oal_mm_malloc_contig_aligned_nocache(const addr_t size, const uint32_t align)
{
	return __oal_mm_dma_alloc_htable(size, align);
}

/**
 *	Allocate aligned, contiguous, cacheable memory region
 */
void *oal_mm_malloc_contig_aligned_cache(const addr_t size, const uint32_t align)
{
	/* All kmalloc memory is automatically aligned to at least 128B(or higher power of two) on arm64.
	 * This should be replaced with genpool align allocator in future.*/
	if (align && (ARCH_KMALLOC_MINALIGN % align)) {
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
	struct pfe_reserved_mem *res_mem;

	if (strcmp(pool, PFE_CFG_BD_MEM) && strcmp(pool, PFE_CFG_SYS_MEM))
		goto default_alloc;

	/* use reserved memory */
	res_mem = __oal_mm_reserved_mem_get(pool);
	if (!res_mem)
		goto default_alloc;

	if (!strcmp(pool, PFE_CFG_SYS_MEM))
		return __oal_mm_reserved_sys_mem_alloc_htable(res_mem, size, align);
	else
		return __oal_mm_reserved_mem_alloc_htable(res_mem->pool_alloc, size, align);

default_alloc:
	return __oal_mm_dma_alloc_htable(size, align);
}

/**
 *	Allocate aligned, contiguous, cached memory region from named pool
 */
void *oal_mm_malloc_contig_named_aligned_cache(const char_t *pool, const addr_t size, const uint32_t align)
{
	/* All kmalloc memory is automatically aligned to at least 128B(or higher power of two) on arm64.
	 * This should be replaced with genpool align allocator in future.*/
	if (align && (ARCH_KMALLOC_MINALIGN % align)) {
		NXP_LOG_ERROR("Alignment not supported\n");
		return NULL;
	}
	return __oal_mm_kmalloc_htable(size);
}

static struct pfe_kmem *__oal_mm_get_vaddr_node(const void *vaddr)
{
	struct pfe_kmem *mem = NULL;

	hash_for_each_possible(pfe_addr_htable, mem, node, (uint64_t)vaddr)
		if (vaddr == mem->addr)
			break;
	return mem;
}

/**
 *	Release memory allocated by oal_mm_malloc_contig*()
 */
void oal_mm_free_contig(const void *vaddr)
{
	struct pfe_kmem *mem;

	if (!vaddr) {
		NXP_LOG_ERROR("Attempt to release NULL-pointed memory\n");
		return;
	}

	mem = __oal_mm_get_vaddr_node(vaddr);
	if (!mem) {
		NXP_LOG_WARNING("address not found\n");
		return;
	}

	switch (mem->type) {
		case PFE_MEM_KMALLOC:
			hash_del(&mem->node);
			__oal_mm_kfree_htable(mem);
			break;
		case PFE_MEM_DMA_ALLOC:
			hash_del(&mem->node);
			__oal_mm_dma_free_htable(mem);
			break;
		case PFE_MEM_RESERVED_ALLOC:
			hash_del(&mem->node);
			__oal_mm_reserved_mem_free_htable(mem);
			break;
		case PFE_MEM_BMU2_RESERVED_ALLOC:
			hash_del(&mem->node);
			__oal_mm_reserved_sys_mem_free_htable(mem);
			break;
		default:
			NXP_LOG_ERROR("invalid address node\n");
			return;
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
	struct pfe_kmem *mem = __oal_mm_get_vaddr_node(vaddr);

	if (mem && mem->dma_addr)
		return (void *)mem->dma_addr;

	if (bmu_start_va == vaddr)
		return (void *)bmu_start_pa;

	return (void *)virt_to_phys((volatile void *)vaddr);
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
	return ioremap((resource_size_t)paddr, len);
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

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_reserved_bmu2_pool_region_init(struct device *dev, int idx, struct reserved_mem **rmem_out)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;
	void *base_va;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!mem_node) {
		dev_warn(dev, "No memory-region found at index %d\n", idx);
		goto out;
	}

	if (!of_device_is_compatible(mem_node, "fsl,pfe-bmu2-pool")) {
		/* don't fail probing if node not found */
		dev_warn(dev, "fsl,pfe-bmu2-pool node missing\n");
		goto out;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		goto out;
	}

	of_node_put(mem_node);

	base_va = devm_memremap(dev, rmem->base, rmem->size, MEMREMAP_WC);
	if (!base_va) {
		dev_err(dev, "PFE BMU2 pool mapping failed\n");
		return -EINVAL;
	}

	rmem->priv = base_va;
	*rmem_out = rmem;

	return 0;

out:
	dev_warn(dev, "fallback to default BMU2 pool allocation\n");
	of_node_put(mem_node);

	return 0;
}
#endif /* PFE_CFG_PFE_MASTER */

#if (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a)
static int pfeng_reserved_bdr_pool_region_init(struct device *dev, int idx, struct gen_pool **pool_alloc)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;
	struct gen_pool *p;
	void *base;
	int ret;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!mem_node) {
		dev_warn(dev, "No memory-region found at index %d\n", idx);
		goto out;
	}

	if (!of_device_is_compatible(mem_node, "fsl,pfe-bdr-pool")) {
		/* don't fail probing if node not found */
		dev_warn(dev, "fsl,pfe-bdr-pool node missing\n");
		goto out;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		goto out;
	}

	of_node_put(mem_node);

	base = devm_memremap(dev, rmem->base, rmem->size, MEMREMAP_WB);
	if (!base) {
		dev_err(dev, "PFE BDR pool map failed\n");
		return -EINVAL;
	}

	p = devm_gen_pool_create(dev, L1_CACHE_SHIFT, -1, "pfe-bdr-pool");
	if (!p) {
		dev_err(dev, "gen pool create failed\n");
		memunmap(base);
		return -EINVAL;
	}

	ret = gen_pool_add(p, (unsigned long)base, rmem->size, -1);
	if (ret) {
		dev_err(dev, "gen pool add failed\n");
		memunmap(base);
		gen_pool_destroy(p);
		return ret;
	}

	*pool_alloc = p;

	return 0;

out:
	dev_warn(dev, "allocate BDRs in non-cacheable memory\n");
	of_node_put(mem_node);

	return 0;
}
#endif

errno_t oal_mm_init(const void *devh)
{
	struct device *dev = (struct device *)devh;
	struct pfe_reserved_mem *pfe_res_mem;
	struct gen_pool *pool_alloc = NULL;
	struct reserved_mem *rmem = NULL;
	int idx = 0;
	int ret;

#ifdef PFE_CFG_PFE_MASTER
	/* BMU2 region is required by MASTER only */
	ret = pfeng_reserved_bmu2_pool_region_init(dev, idx, &rmem);
	if (ret) {
		dev_err(dev, "BMU2 pool reservation failed. Error %d\n", ret);
		return -ENOMEM;
	}

	if (rmem)
		idx++;
#endif /* PFE_CFG_PFE_MASTER */

	ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, idx);
	if (ret) {
		dev_err(dev, "shared-dma-pool reservation failed. Error %d\n", ret);
		return -ENOMEM;
	}
	idx++;

#if (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a)
	ret = pfeng_reserved_bdr_pool_region_init(dev, idx, &pool_alloc);
	if (ret) {
		dev_err(dev, "BDR pool reservation failed. Error %d\n", ret);
		return -ENOMEM;
	}
#endif

	if (pool_alloc) {
		pfe_res_mem = devm_kzalloc(dev, sizeof(*pfe_res_mem), GFP_KERNEL);
		if (!pfe_res_mem)
			return -ENOMEM;

		pfe_res_mem->name = PFE_CFG_BD_MEM;
		pfe_res_mem->pool_alloc = pool_alloc;
		INIT_LIST_HEAD(&pfe_res_mem->node);
		list_add(&pfe_res_mem->node, &pfe_reserved_mem_list);
	}

	if (rmem) {
		pfe_res_mem = devm_kzalloc(dev, sizeof(*pfe_res_mem), GFP_KERNEL);
		if (!pfe_res_mem)
			return -ENOMEM;

		pfe_res_mem->name = PFE_CFG_SYS_MEM;
		pfe_res_mem->map_start_va = rmem->priv;
		pfe_res_mem->map_start_pa = rmem->base;
		pfe_res_mem->map_size = rmem->size;
		INIT_LIST_HEAD(&pfe_res_mem->node);
		list_add(&pfe_res_mem->node, &pfe_reserved_mem_list);
	}

	__dev = dev;

	hash_init(pfe_addr_htable);

	return EOK;
}

void oal_mm_shutdown(void)
{
	of_reserved_mem_device_release(__dev);
	/* reserved_mem list nodes will be released by devm_ */
	INIT_LIST_HEAD(&pfe_reserved_mem_list);

	__dev = NULL;
	if (!hash_empty(pfe_addr_htable)) {
		NXP_LOG_ERROR("Unfreed memory detected\n");
	}

	return;
}

/** @}*/
