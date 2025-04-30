/*
 * Copyright 2020-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/prefetch.h>

#include "pfe_cfg.h"
#include "pfeng.h"

#define PFE_RXB_TRUESIZE	2048 /* PAGE_SIZE >> 1 */
#define PFE_RXB_PAD		NET_SKB_PAD /* add extra space if needed */
#define PFE_RXB_DMA_SIZE	(SKB_WITH_OVERHEAD(PFE_RXB_TRUESIZE) - PFE_RXB_PAD)

#define PFENG_BMAN_REFILL_THR	32

/* sanity check: we need RX buffering internal support disabled */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
#error "Invalid PFE HIF channel mode"
#endif

struct pfeng_rx_map {
	dma_addr_t dma;
	struct page *page;
	u16 page_offset;
};

struct pfeng_rx_chnl_pool {
	struct device			*dev;
	pfe_hif_chnl_t			*ll_chnl;
	struct sk_buff			*skb;
	u32				id;
	u32				depth;

	/* mappings of hif_drv rx ring */
	struct pfeng_rx_map 		*rx_tbl;
	u32				rd_idx;
	u32				wr_idx;
	u32				alloc_idx;
	u32				idx_mask;
};

struct pfeng_tx_map {
	addr_t				pa_addr;
	u32				size;
	struct sk_buff			*skb;
	u8				flags;
};

struct pfeng_tx_chnl_pool {
	int				rd_idx;
	int				wr_idx;
	int				idx_mask;
	int				depth;

	/* mappings for hif_drv tx ring */
	struct pfeng_tx_map		*tx_tbl;
};

int pfeng_bman_pool_create(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_rx_chnl_pool *rx_pool;
	struct pfeng_tx_chnl_pool *tx_pool;

	/* RX pool */
	rx_pool = kzalloc(sizeof(*rx_pool), GFP_KERNEL);
	if (!rx_pool) {
		HM_MSG_DEV_ERR(chnl->dev, "chnl%d: No mem for bman rx_pool\n", pfe_hif_chnl_get_id(chnl->priv));
		return -ENOMEM;
	}

	chnl->bman.rx_pool = rx_pool;
	rx_pool->ll_chnl = chnl->priv;
	rx_pool->dev = chnl->dev;
	rx_pool->id = pfe_hif_chnl_get_id(chnl->priv);
	rx_pool->depth = PFE_CFG_HIF_RING_LENGTH;
	rx_pool->idx_mask = PFE_CFG_HIF_RING_LENGTH - 1;

	rx_pool->rx_tbl = kcalloc(rx_pool->depth, sizeof(struct pfeng_rx_map), GFP_KERNEL);
	if (!rx_pool->rx_tbl) {
		HM_MSG_DEV_ERR(chnl->dev, "chnl%d: failed. No mem\n", rx_pool->id);
		goto err;
	}

	/* TX pool */
	tx_pool = kzalloc(sizeof(*tx_pool), GFP_KERNEL);
	if (!tx_pool) {
		HM_MSG_DEV_ERR(chnl->dev, "chnl%d: No mem for bman tx_pool\n", pfe_hif_chnl_get_id(chnl->priv));
		goto err;
	}

	chnl->bman.tx_pool = tx_pool;
	tx_pool->depth = PFE_CFG_HIF_RING_LENGTH;
	tx_pool->idx_mask = PFE_CFG_HIF_RING_LENGTH - 1;

	tx_pool->tx_tbl = kcalloc(tx_pool->depth, sizeof(struct pfeng_tx_map), GFP_KERNEL);
	if (!tx_pool->tx_tbl) {
		HM_MSG_DEV_ERR(chnl->dev, "chnl%d: failed. No mem\n", rx_pool->id);
		goto err;
	}

	return 0;

err:
	pfeng_bman_pool_destroy(chnl);
	return -ENOMEM;
}

void pfeng_bman_tx_pool_dump(struct pfeng_hif_chnl *chnl, struct net_device *ndev, void (*dbg_print)(void *ndev, const char *fmt, ...))
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	u32 rd = READ_ONCE(pool->rd_idx) & pool->idx_mask;
	u32 wr = READ_ONCE(pool->wr_idx) & pool->idx_mask;

	dbg_print(ndev, "depth: %d, indexes rd: %d, wr: %d\n", pool->depth, rd, wr);
}

int pfeng_hif_chnl_txbd_unused(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int wr_idx = READ_ONCE(pool->wr_idx);
	int rd_idx = READ_ONCE(pool->rd_idx);

	if (likely(wr_idx >= rd_idx))
		return pool->depth - wr_idx + rd_idx - 1;

	return rd_idx - wr_idx - 1;
}

bool pfeng_hif_chnl_txbd_is_empty(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;

	return pool->wr_idx == pool->rd_idx;
}

void pfeng_hif_chnl_txconf_put_map_frag(struct pfeng_hif_chnl *chnl, addr_t pa_addr, u32 size, struct sk_buff *skb, u8 flags, int i)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int idx = pool->wr_idx;

	idx = (idx + i) & pool->idx_mask;
	pool->tx_tbl[idx].pa_addr = pa_addr;
	pool->tx_tbl[idx].size = size;
	pool->tx_tbl[idx].skb = skb;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	pool->tx_tbl[idx].flags = flags;
#endif
}

void pfeng_hif_chnl_txconf_update_wr_idx(struct pfeng_hif_chnl *chnl, int count)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int wr_idx = pool->wr_idx;

	wr_idx = (wr_idx + count) & pool->idx_mask;
	WRITE_ONCE(pool->wr_idx, wr_idx);
}

u8 pfeng_hif_chnl_txconf_get_flag(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int idx = pool->rd_idx;

	return pool->tx_tbl[idx].flags;
}

struct sk_buff *pfeng_hif_chnl_txconf_get_skbuf(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int idx = pool->rd_idx;

	return pool->tx_tbl[idx].skb;
}

int pfeng_hif_chnl_txconf_free_map_full(struct pfeng_hif_chnl *chnl, int napi_budget)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int idx = READ_ONCE(pool->rd_idx);
	int idx_mask = pool->idx_mask;
	struct sk_buff *skb;
	int nfrags;

	skb = pool->tx_tbl[idx].skb;

	if (unlikely(!skb))
		return -EINVAL;

	nfrags = skb_shinfo(skb)->nr_frags;

	/* Unmap linear part */
	dma_unmap_single_attrs(chnl->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE, 0);

	/* Unmap frags */
	idx = (idx + 1) & idx_mask;
	while (nfrags--) {
		dma_unmap_page(chnl->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE);

		idx = (idx + 1) & idx_mask;
	}
	WRITE_ONCE(pool->rd_idx, idx);

	napi_consume_skb(skb, napi_budget);
	return 0;
}

void pfeng_hif_chnl_txconf_unroll_map_full(struct pfeng_hif_chnl *chnl, int i)
{
	struct pfeng_tx_chnl_pool *pool = chnl->bman.tx_pool;
	int idx = READ_ONCE(pool->wr_idx);

	/* unrolling from last to first */
	idx += i;
	while (i) {
		dma_unmap_page(chnl->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE);
		pool->tx_tbl[idx].size = 0;

		idx = (idx > 0) ? idx - 1 : pool->depth - 1;
		i--;
	}

	/* Unmap linear part */
	dma_unmap_single_attrs(chnl->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE, 0);
	pool->tx_tbl[idx].size = 0;
}

static inline int pfeng_bman_rx_chnl_pool_unused(struct pfeng_rx_chnl_pool *pool)
{
	return pool->depth - pool->wr_idx + pool->rd_idx - 1;
}

static inline struct pfeng_rx_map *pfeng_bman_get_rx_map(struct pfeng_rx_chnl_pool *pool, u32 idx)
{
	return &pool->rx_tbl[idx & pool->idx_mask];
}

static bool pfeng_bman_buf_alloc_and_map(struct pfeng_rx_chnl_pool *pool, struct pfeng_rx_map *rx_map)
{
	struct page *page;
	dma_addr_t dma;

	/* Request page from DMA safe region */
	page = __dev_alloc_page(GFP_DMA32 | GFP_ATOMIC | __GFP_NOWARN);
	if (unlikely(!page))
		return false;

	/* do dma map */
	dma = dma_map_page(pool->dev, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(pool->dev, dma))) {
		__free_page(page);
		return false;
	}

	rx_map->dma = dma;
	rx_map->page = page;
	rx_map->page_offset = PFE_RXB_PAD;

	return true;
}

static void pfeng_bman_free_rx_buffers(struct pfeng_rx_chnl_pool *pool)
{
	struct pfeng_rx_map *rx_map;
	int i;

	for (i = 0; i < pool->depth; i++) {
		rx_map = pfeng_bman_get_rx_map(pool, i);

		if (!rx_map->page)
			continue;

		dma_unmap_page(pool->dev, rx_map->dma, PAGE_SIZE, DMA_FROM_DEVICE);

		__free_page(rx_map->page);

		rx_map->dma = 0;
		rx_map->page = NULL;
		rx_map->page_offset = 0;
	}
}

static int pfeng_hif_chnl_refill_rx_buffer(struct pfeng_rx_chnl_pool *pool, struct pfeng_rx_map *rx_map)
{
	int err;

	/*	Ask for new buffer */
	if (unlikely(!rx_map->page))
		if (unlikely(!pfeng_bman_buf_alloc_and_map(pool, rx_map))) {
			HM_MSG_DEV_ERR(pool->dev, "buffer allocation error\n");
			return -ENOMEM;
		}

	/* Add new buffer to ring */
	err = pfe_hif_chnl_supply_rx_buf(pool->ll_chnl, (void *)(rx_map->dma + rx_map->page_offset), PFE_RXB_DMA_SIZE);
	if (unlikely(err))
		return err;

	return 0;
}

static int pfeng_hif_chnl_refill_rx_pool(struct pfeng_rx_chnl_pool *pool, int count)
{
	struct pfeng_rx_map *rx_map;
	int i, ret = 0;

	for (i = 0; i < count; i++) {
		rx_map = pfeng_bman_get_rx_map(pool, pool->wr_idx);
		ret = pfeng_hif_chnl_refill_rx_buffer(pool, rx_map);
		if (unlikely(ret))
			break;
		/* push rx map */
		pool->wr_idx++;
	}

	pool->alloc_idx = pool->wr_idx;

	return ret;
}

static bool pfeng_page_reusable(struct page *page)
{
	return (!page_is_pfmemalloc(page) && page_ref_count(page) == 1);
}

static void pfeng_reuse_page(struct pfeng_rx_chnl_pool *pool,
			     struct pfeng_rx_map *old)
{
	struct pfeng_rx_map *new;

	/* next buf mapping that may reuse a page */
	new = pfeng_bman_get_rx_map(pool, pool->alloc_idx);

	/* copy page reference */
	*new = *old;

	/* advance page allocation idx */
	pool->alloc_idx++;
}

static struct pfeng_rx_map *pfeng_get_rx_buff(struct pfeng_rx_chnl_pool *pool, u32 i, u32 size)
{
	struct pfeng_rx_map *rx_map = pfeng_bman_get_rx_map(pool, i);

	dma_sync_single_range_for_cpu(pool->dev, rx_map->dma,
				      rx_map->page_offset,
				      size, DMA_FROM_DEVICE);

	return rx_map;
}

static void pfeng_put_rx_buff(struct pfeng_rx_chnl_pool *pool, struct pfeng_rx_map *rx_map, u32 size)
{
	if (likely(pfeng_page_reusable(rx_map->page))) {
		rx_map->page_offset ^= PFE_RXB_TRUESIZE;
		page_ref_inc(rx_map->page);

		pfeng_reuse_page(pool, rx_map);

		/* dma sync for use by device */
		dma_sync_single_range_for_device(pool->dev, rx_map->dma,
						 rx_map->page_offset,
						 size, DMA_FROM_DEVICE);
	} else {
		dma_unmap_page(pool->dev, rx_map->dma, PAGE_SIZE, DMA_FROM_DEVICE);
	}

	/* drop reference since page has been recycled (@alloc_idx), or unmaped */
	rx_map->page = NULL;
}

static struct sk_buff *pfeng_rx_map_buff_to_skb(struct pfeng_rx_chnl_pool *pool, u32 rx_len)
{
	struct pfeng_rx_map *rx_map;
	struct sk_buff *skb;
	void *va;

	/* get rx buffer */
	rx_map = pfeng_get_rx_buff(pool, pool->rd_idx, rx_len);

	va = page_address(rx_map->page) + rx_map->page_offset;
	skb = build_skb(va - PFE_RXB_PAD, PFE_RXB_TRUESIZE);
	if (unlikely(!skb)) {
		/* We're OOM: release the page (drop the frame) and
		 * advance the pool consumer index to the next frame to keep
		 * it in sync with the BD ring consumer index. Do this until
		 * the OOM condtion is gone or there's no more space left in
		 * the BD ring, in which case the HW will stop receiving frames.*/
		dma_unmap_page(pool->dev, rx_map->dma, PAGE_SIZE, DMA_FROM_DEVICE);
		__free_page(rx_map->page);

		memset(rx_map, 0, sizeof(*rx_map));
		/* pull rx map */
		pool->rd_idx++;
		return NULL;
	}

	skb_reserve(skb, PFE_RXB_PAD);
	__skb_put(skb, rx_len);

	/* put rx buffer */
	pfeng_put_rx_buff(pool, rx_map, rx_len);

	/* pull rx map */
	pool->rd_idx++;

	return skb;
}

static void pfeng_rx_add_buff_to_skb(struct pfeng_rx_chnl_pool *pool, u32 rx_len)
{
	struct pfeng_rx_map *rx_map;

	/* get rx buffer */
	rx_map = pfeng_get_rx_buff(pool, pool->rd_idx, rx_len);

	skb_add_rx_frag(pool->skb, skb_shinfo(pool->skb)->nr_frags, rx_map->page,
			rx_map->page_offset, rx_len, PFE_RXB_TRUESIZE);

	/* put rx buffer */
	pfeng_put_rx_buff(pool, rx_map, rx_len);

	/* pull rx map */
	pool->rd_idx++;
}

struct sk_buff *pfeng_hif_chnl_receive_pkt(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_rx_chnl_pool *pool = chnl->bman.rx_pool;
	bool_t lifm = false;
	struct sk_buff *skb;
	void *buf_pa;
	u32 rx_len;

	if (unlikely(pfeng_bman_rx_chnl_pool_unused(pool) >= PFENG_BMAN_REFILL_THR)) {
		pfeng_hif_chnl_refill_rx_pool(pool, PFENG_BMAN_REFILL_THR);
		/* Re-trigger the RX DMA */
		pfe_hif_chnl_rx_dma_start(pool->ll_chnl);
	}

	while (!lifm) {
		/* get frame buffer info from the RX BD and move to the next BD in the ring */
		if (EOK != pfe_hif_chnl_rx(pool->ll_chnl, &buf_pa, &rx_len, &lifm)) {
			return NULL;
		}

		if (!pool->skb) {
			/* map the corresponding buffer (frame) to an skb and advance
			 * the pool consumer index, to keep it in sync with the BD ring
			 * consumer index */
			skb = pfeng_rx_map_buff_to_skb(pool, rx_len);
			if (unlikely(!skb)) {
				HM_MSG_DEV_ERR(chnl->dev, "chnl%d: Rx skb mapping failed\n", chnl->idx);
				return NULL;
			}

			pool->skb = skb;
		} else {
			pfeng_rx_add_buff_to_skb(pool, rx_len);
		}
	}

	skb = pool->skb;
	pool->skb = NULL;
	prefetch(skb->data);

	return skb;
}

int pfeng_hif_chnl_fill_rx_buffers(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_rx_chnl_pool *pool = chnl->bman.rx_pool;
	int cnt = 0;
	int ret;

	while (pfe_hif_chnl_can_accept_rx_buf(pool->ll_chnl)) {

		ret = pfeng_hif_chnl_refill_rx_pool(pool, 1);
		if (ret)
			break;
		cnt++;
	}

	return cnt;
}

void pfeng_bman_pool_destroy(struct pfeng_hif_chnl *chnl)
{
	struct pfeng_rx_chnl_pool *rx_pool = (struct pfeng_rx_chnl_pool *)chnl->bman.rx_pool;
	struct pfeng_tx_chnl_pool *tx_pool = (struct pfeng_tx_chnl_pool *)chnl->bman.tx_pool;

	if (rx_pool) {
		if(rx_pool->rx_tbl) {
			pfe_hif_chnl_reset_fifos(chnl->priv);
			pfeng_bman_free_rx_buffers(rx_pool);
			kfree(rx_pool->rx_tbl);
			rx_pool->rx_tbl = NULL;
		}

		kfree(rx_pool);
		chnl->bman.rx_pool = NULL;
	}

	if (tx_pool) {
		if(tx_pool->tx_tbl) {
			kfree(tx_pool->tx_tbl);
			tx_pool->tx_tbl = NULL;
		}

		kfree(tx_pool);
		chnl->bman.tx_pool = NULL;
	}

	return;
}
