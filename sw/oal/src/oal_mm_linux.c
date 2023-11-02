/* =========================================================================
 *  Copyright 2018-2023 NXP
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
#include <linux/module.h>
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

/* managed memory allocation types */
enum pfe_kmem_type {
	PFE_MEM_INVALID = 0,
	PFE_MEM_KMALLOC,
	PFE_MEM_DMA_ALLOC,
	PFE_MEM_RESERVED_ALLOC,
	PFE_MEM_BMU2_RESERVED_ALLOC,
	PFE_MEM_RT_RESERVED_ALLOC,
};

/*
 * Internal structure for managing oal_mm memory allocations
 * One entry per (addr, size) range, stored in a hash table
 * indexed by the virtual start address of the entry. May store
 * a DMA coherent addr range, a special reserved memory range,
 * or a stadard kmalloc'ed range perfromed via the oal_mm API.
 */
struct pfe_kmem {
	struct hlist_node node;
	void *addr;
	addr_t size;
	union {
		dma_addr_t dma_addr;
		phys_addr_t phys_addr;
	};
	enum pfe_kmem_type type;
	bool_t is_dma;
};

/* 'no-map' reserved memory region types */
enum pfeng_res_no_map_reg_id {
	PFE_REG_BMU2 = 0,
	PFE_REG_RT,
	PFE_REG_COUNT
};

/*
 * Reserved memory region configuration entry, populated based
 * on info parsed from 'reserved-memory' device tree nodes.
 * May hold system memory mapped ranges that require a gen pool
 * allocator, or exclusive 'no-map' regions for single allocations,
 * of type @pfeng_res_no_map_reg_id.
 */
struct pfe_reserved_mem {
	struct list_head node;
	char_t *name;
	struct gen_pool *pool_alloc;
	void *map_start_va;
	phys_addr_t map_start_pa;
	addr_t map_size;
};

#define OAL_CACHE_ALLIGN	64

/*
 * Device associated with memory management, passed to @oal_mm_init().
 * Must be the parent pfe device reference.
 */
static struct device *__dev = NULL;

/* hash table for 'struct pfe_kmem' entries, indexed by virtual address */
static DEFINE_HASHTABLE(pfe_addr_htable, 8);

/* list of reserved memory ranges config params */
static LIST_HEAD(pfe_reserved_mem_list);

#ifdef PFE_CFG_PFE_MASTER
static const char pfeng_res_no_map_name[PFE_REG_COUNT][16] = {
	"pfe-bmu2-pool",
	"pfe-rt-pool",
};
#endif

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

static struct pfe_kmem *__oal_mm_get_vaddr_node(const void *vaddr)
{
	struct pfe_kmem *mem = NULL;
	bool_t found = false;

	hash_for_each_possible(pfe_addr_htable, mem, node, (uint64_t)vaddr) {
		if ((uint64_t)vaddr == (uint64_t)mem->addr) {
			found = true;
			break;
		}
	}

	if (!found)
		mem = NULL;

	return mem;
}

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

	hnode->type = PFE_MEM_DMA_ALLOC;
	hnode->addr = vaddr;
	hnode->size = size;
	hnode->is_dma = true;
	hnode->dma_addr = dma_addr;
	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);

	return vaddr;
}

static void __oal_mm_dma_free_htable(struct pfe_kmem *hnode)
{
	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr)
		dma_free_coherent(__dev, hnode->size, hnode->addr, hnode->dma_addr);
}

static void *__oal_mm_kmalloc_htable(const addr_t size, const uint32_t align)
{
	struct pfe_kmem *hnode;
	void *vaddr;

	vaddr = kmalloc(size, GFP_KERNEL);
	if (!vaddr)
		return NULL;

	/* Check that requested alignment matches what kmalloc provides.
	 *
	 * From kmalloc documentation:
	 * The allocated object address is aligned to at least ARCH_KMALLOC_MINALIGN
	 * bytes (128B on AMR64). For @size of power of two bytes, the alignment is also
	 * guaranteed to be at least to the size.
	 */
	if (align && !IS_ALIGNED((unsigned long) vaddr, align)) {
		NXP_LOG_ERROR("Requested allocation of size: 0x%llx not aligned to: 0x%x\n",
			      size, align);
		kfree(vaddr);
		return NULL;
	}

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		kfree(vaddr);
		return NULL;
	}

	hnode->type = PFE_MEM_KMALLOC;
	hnode->addr = vaddr;
	hnode->phys_addr = virt_to_phys(vaddr);
	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);

	return vaddr;
}

static void __oal_mm_kfree_htable(struct pfe_kmem *hnode)
{
	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr)
		kfree(hnode->addr);
}

/*
 * Allocate inside a reserved memory region that is mapped in Linux's system memory, via memremap.
 * The memory region is shared with PFE, so it must be in PFE's DMA domain. Objects allocated
 * in this memory region need also to be efficiently accessed by the CPU, so, contrary to what
 * the higher level plaform API suggests, this memory region is cacheable and H/W coherency
 * b/w the CPUs and PFE is enabled on it. It's used to store buffer descriptors and PFE specific
 * per-packet headers (i.e. in-band metadata that is read or written by PFE) whose processing on the
 * CPU is performance critical. Since there are multiple such ojects (i.e. BD rings and headers)
 * a generic allocator is used to manage allocation inside this reserved memory region.
 */
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

	hnode->type = PFE_MEM_RESERVED_ALLOC;
	hnode->addr = vaddr;
	hnode->size = size;
	hnode->phys_addr = gen_pool_virt_to_phys(pool_alloc, (unsigned long)vaddr);
	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)vaddr);

	return vaddr;
}

static void __oal_mm_reserved_mem_free_htable(struct pfe_kmem *hnode)
{
	struct pfe_reserved_mem *res_mem = __oal_mm_reserved_mem_get(PFE_CFG_BD_MEM);

	hnode->type = PFE_MEM_INVALID;
	if (hnode->addr && res_mem->pool_alloc)
		gen_pool_free(res_mem->pool_alloc, (unsigned long)hnode->addr, hnode->size);
}

/*
 * Return a "no-map" reserved region (not mapped in Linux's system memory) as the CPU
 * should not access it at runtime. The region is mapped via ioremap, so it's contiguous
 * and non-cacheable.
 * These regions are reserved for PFE usage alone, and preconfigured by design (via DT).
 * Such a region is destined for a single, usually large, object used by the accelerator,
 * like the BMU2 buffer pool or the routing table.
 */
static void *__oal_mm_reserved_nomap_mem_alloc_htable(struct pfe_reserved_mem *res_mem, enum pfe_kmem_type type, const addr_t size, const uint32_t align)
{
	struct pfe_kmem *hnode;

	if (align && !IS_ALIGNED(res_mem->map_start_pa, align)) {
		NXP_LOG_ERROR("%s reserved mem region addr not aligned\n", res_mem->name);
		return NULL;
	}

	if (res_mem->map_size < size) {
		NXP_LOG_ERROR("%s reserved mem region size exceeded\n", res_mem->name);
		/* try default allocation */
		return NULL;
	}

	if (__oal_mm_get_vaddr_node(res_mem->map_start_va)) {
		NXP_LOG_ERROR("Allocation attempt in %s exclusive zone\n", res_mem->name);
		/* try default allocation */
		return NULL;
	}

	hnode = kzalloc(sizeof(struct pfe_kmem), GFP_KERNEL);
	if (!hnode) {
		return NULL;
	}

	hnode->type = type;
	hnode->addr = res_mem->map_start_va;
	hnode->size = size;
	hnode->phys_addr = res_mem->map_start_pa;
	hash_add(pfe_addr_htable, &hnode->node, (uint64_t)res_mem->map_start_va);

	return res_mem->map_start_va;
}

/* External API */

struct device *oal_mm_get_dev(void)
{
	return __dev;
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
	return __oal_mm_kmalloc_htable(size, align);
}

/**
 *	Allocate aligned, contiguous, non-cacheable memory region from named pool
 */
void *oal_mm_malloc_contig_named_aligned_nocache(const char_t *pool, const addr_t size, const uint32_t align)
{
	struct pfe_reserved_mem *res_mem;

	/* try reserved memory */
	res_mem = __oal_mm_reserved_mem_get(pool);
	if (!res_mem)
		goto default_alloc;

	if (!strcmp(pool, PFE_CFG_SYS_MEM))
		return __oal_mm_reserved_nomap_mem_alloc_htable(res_mem, PFE_MEM_BMU2_RESERVED_ALLOC, size, align);
	else if (!strcmp(pool, PFE_CFG_RT_MEM))
		return __oal_mm_reserved_nomap_mem_alloc_htable(res_mem, PFE_MEM_RT_RESERVED_ALLOC, size, align);
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
	return __oal_mm_kmalloc_htable(size, align);
}

void *oal_mm_virt_to_phys_contig(void *vaddr)
{
	struct pfe_kmem *mem = __oal_mm_get_vaddr_node(vaddr);

	if (mem) {
		if (mem->is_dma)
			return (void *)mem->dma_addr;
		else
			return (void *)mem->phys_addr;
	}

	return (void *)virt_to_phys((volatile void *)vaddr);
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

	hash_del(&mem->node);

	switch (mem->type) {
		case PFE_MEM_KMALLOC:
			__oal_mm_kfree_htable(mem);
			break;
		case PFE_MEM_DMA_ALLOC:
			__oal_mm_dma_free_htable(mem);
			break;
		case PFE_MEM_RESERVED_ALLOC:
			__oal_mm_reserved_mem_free_htable(mem);
			break;
		case PFE_MEM_BMU2_RESERVED_ALLOC:
		case PFE_MEM_RT_RESERVED_ALLOC:
			break;
		default:
			NXP_LOG_ERROR("invalid address node\n");
	}

	kfree(mem);
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

static int init_io_region(struct reserved_mem *rmem)
{
	void __iomem *base_va;

	base_va = ioremap_wc(rmem->base, rmem->size);
	if (!base_va)
		return -EINVAL;

	memset_io(base_va, 0, rmem->size);

	iounmap(base_va);
	return 0;
}

static int pfeng_reserved_bdr_pool_region_init(struct device *dev, struct gen_pool **pool_alloc, int rmem_idx)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;
	struct gen_pool *p;
	void *base;
	bool nomap;
	int idx;
	int ret;

	idx = of_property_match_string(dev->of_node, "memory-region-names", "pfe-bdr-pool");
	if (idx < 0)
		idx = rmem_idx;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!mem_node) {
		dev_warn(dev, "No memory-region found at index %d\n", idx);
		goto out;
	}

	if (!of_device_is_compatible(mem_node, "nxp,s32g-pfe-bdr-pool")) {
		/* don't fail probing if node not found */
		dev_warn(dev, "nxp,s32g-pfe-bdr-pool node missing\n");
		goto out;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		goto out;
	}

	nomap = of_find_property(mem_node, "no-map", NULL) != NULL;

	of_node_put(mem_node);

	if (nomap && init_io_region(rmem)) {
		dev_err(dev, "failed to init reserved region %s @ phys addr: 0x%llx\n",
			rmem->name, rmem->base);
		return -EINVAL;
	}

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

	ret = gen_pool_add_owner(p, (unsigned long)base, rmem->base, rmem->size, -1, NULL);
	if (ret) {
		dev_err(dev, "gen pool add failed\n");
		memunmap(base);
		gen_pool_destroy(p);
		return ret;
	}

	*pool_alloc = p;

	dev_info(dev, "assigned reserved memory node %s\n", rmem->name);

	return 0;

out:
	dev_warn(dev, "allocate BDRs in non-cacheable memory\n");
	of_node_put(mem_node);

	return 0;
}

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_reserved_no_map_region_init(struct device *dev, struct reserved_mem **rmem_out, int rmem_id, int *rmem_idx)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;
	void __iomem *base_va;
	char compatible[32];
	int idx;

	idx = of_property_match_string(dev->of_node, "memory-region-names", pfeng_res_no_map_name[rmem_id]);
	if (idx < 0)
		idx = *rmem_idx;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!mem_node) {
		dev_warn(dev, "No memory-region found at index %d\n", idx);
		goto out;
	}

	scnprintf(compatible, sizeof(compatible), "nxp,s32g-%s", pfeng_res_no_map_name[rmem_id]);
	if (!of_device_is_compatible(mem_node, compatible)) {
		/* don't fail probing if node not found */
		dev_warn(dev, "memory-region: %s node missing\n", compatible);
		goto out;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		/* advance rmem iterator */
		*rmem_idx = idx + 1;
		goto out;
	}

	of_node_put(mem_node);

	base_va = devm_ioremap_wc(dev, rmem->base, rmem->size);
	if (!base_va) {
		dev_err(dev, "%s mapping failed\n", pfeng_res_no_map_name[rmem_id]);
		return -EINVAL;
	}

	memset_io(base_va, 0, rmem->size);

	rmem->priv = base_va;
	*rmem_out = rmem;
	/* advance rmem iterator */
	*rmem_idx = idx + 1;

	dev_info(dev, "assigned reserved memory node %s\n", rmem->name);

	return 0;

out:
	dev_warn(dev, "fall back to default pool allocation\n");
	of_node_put(mem_node);

	return 0;
}

static int pfeng_reserved_dma_shared_pool_region_init(struct device *dev, int *rmem_idx)
{
	int idx = of_property_match_string(dev->of_node, "memory-region-names", "pfe-shared-pool");
	int ret;

	if (idx < 0)
		idx = *rmem_idx;

	ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, idx);
	if (ret) {
		return ret;
	}

	/* advance rmem iterator */
	*rmem_idx = idx + 1;

	return 0;
}

#endif /* PFE_CFG_PFE_MASTER */

int __oal_mm_wakeup_reinit(void)
{
	struct pfe_reserved_mem *res_mem = NULL;

	if (list_empty(&pfe_reserved_mem_list))
		return 0;

	list_for_each_entry(res_mem, &pfe_reserved_mem_list, node) {
		/* PFE_CFG_RT_MEM and PFE_CFG_SYS_MEM regions could be mapped to SRAM
		 * Reinit is required when returning from SUSPEND mode
		 */
		if (!strcmp(res_mem->name, PFE_CFG_SYS_MEM) || !strcmp(res_mem->name, PFE_CFG_RT_MEM)) {
			NXP_LOG_DEBUG("Reserved memory re-inited: %s\n", res_mem->name);
			memset_io(res_mem->map_start_va, 0, res_mem->map_size);
		}
	}

	return 0;
}

static errno_t __oal_mm_init_regions(struct device *dev)
{
	struct pfe_reserved_mem *pfe_res_mem;
	struct gen_pool *pool_alloc = NULL;
#ifdef PFE_CFG_PFE_MASTER
	struct reserved_mem *rmem[PFE_REG_COUNT] = {NULL, NULL};
	int rmem_idx = 0, i;
#endif /* PFE_CFG_PFE_MASTER */
	int ret;

#ifdef PFE_CFG_PFE_MASTER
	/* BMU2 and RT regions are required by MASTER only */
	for (i = 0; i < PFE_REG_COUNT; i++) {
		ret = pfeng_reserved_no_map_region_init(dev, &rmem[i], i, &rmem_idx);
		if (ret) {
			dev_err(dev, "%s reservation failed. Error %d\n",
				pfeng_res_no_map_name[i], ret);
			return ret;
		}
	}


	ret = pfeng_reserved_dma_shared_pool_region_init(dev, &rmem_idx);
	if (ret) {
		dev_err(dev, "shared-dma-pool reservation failed. Error %d\n", ret);
		return ret;
	}
#endif /* PFE_CFG_PFE_MASTER */

	ret = pfeng_reserved_bdr_pool_region_init(dev, &pool_alloc, 0); // only one reserved reg on slave!
	if (ret) {
		dev_err(dev, "BDR pool reservation failed. Error %d\n", ret);
		goto err_bdr_pool_region_init;
	}

	if (pool_alloc) {
		pfe_res_mem = devm_kzalloc(dev, sizeof(*pfe_res_mem), GFP_KERNEL);
		if (!pfe_res_mem) {
			ret = -ENOMEM;
			goto err_alloc;
		}

		pfe_res_mem->name = PFE_CFG_BD_MEM;
		pfe_res_mem->pool_alloc = pool_alloc;
		INIT_LIST_HEAD(&pfe_res_mem->node);
		list_add(&pfe_res_mem->node, &pfe_reserved_mem_list);
	}

#ifdef PFE_CFG_PFE_MASTER
	for (i = 0; i < PFE_REG_COUNT; i++) {
		if (!rmem[i])
			continue;

		pfe_res_mem = devm_kzalloc(dev, sizeof(*pfe_res_mem), GFP_KERNEL);
		if (!pfe_res_mem) {
			ret = -ENOMEM;
			goto err_alloc;
		}

		pfe_res_mem->name = (i == PFE_REG_RT) ? PFE_CFG_RT_MEM : PFE_CFG_SYS_MEM;
		pfe_res_mem->map_start_va = rmem[i]->priv;
		pfe_res_mem->map_start_pa = rmem[i]->base;
		pfe_res_mem->map_size = rmem[i]->size;
		INIT_LIST_HEAD(&pfe_res_mem->node);
		list_add(&pfe_res_mem->node, &pfe_reserved_mem_list);
	}
#endif /* PFE_CFG_PFE_MASTER */

	return 0;

err_bdr_pool_region_init:
err_alloc:
	of_reserved_mem_device_release(dev);

	return ret;
}

errno_t oal_mm_wakeup_reinit(void)
{
	return __oal_mm_wakeup_reinit();
}

errno_t oal_mm_init(const void *devh)
{
	struct device *dev = (struct device *)devh;
	int ret;

	ret = __oal_mm_init_regions(dev);
	if (ret)
		return ret;

	__dev = dev;

	hash_init(pfe_addr_htable);

	return EOK;
}

void oal_mm_shutdown(void)
{
	of_reserved_mem_device_release(__dev);

	/* reserved_mem list nodes will be released by devm_ */
	INIT_LIST_HEAD(&pfe_reserved_mem_list);

	if (!hash_empty(pfe_addr_htable)) {
		dev_warn(__dev, "Unfreed memory detected\n");
	}

	__dev = NULL;

	return;
}

/** @}*/
