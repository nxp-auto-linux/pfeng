/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/prefetch.h>

#include "pfe_cfg.h"
#include "pfeng.h"

#define SKB_VA_SIZE (sizeof(void *))
#define RX_BUF_SIZE (2048 - SKB_VA_SIZE)

/*
			BMan

	Bman covers buffer managemment for pfe_hid_drv in mode, when
	pfe_hif_chnl is used without internal buffering support.

	It is necessary for supporting zero-copy data passing
	between RX DMA channel and Linux stack.

	The core idea is to use prebuilt skbuf which data buffer
	is fed into channel RX ring, so we got native skbuf
	of packet arrival.

	To optimize additional processing, the skbuf is prebuilt
	with extra area in head of data buffer where the skbuf
	pointer is saved.

	Here is the simple map of prepended data:

	skb ptr prepends BD buff:

		[*skb][ BUFF ]
*/

struct pfeng_rx_chnl_pool {
	pfe_hif_chnl_t	*chnl;
	struct device	*dev;
	struct napi_struct		*napi;
	u32				id;
	u32				depth;
	u32				avail;
	u32				buf_size;

	/* skb VA table for hif_drv rx ring */
	void				**rx_tbl;
	u32				rd_idx;
	u32				wr_idx;
	u32				idx_mask;
};

struct pfeng_tx_map {

	void				*va_addr;
	addr_t				pa_addr;
	u32				size;
	bool				pages;
	struct sk_buff			*skb;
};

struct pfeng_tx_chnl_pool {
	u32				depth;

	/* mappings for hif_drv tx ring */
	struct pfeng_tx_map		*tx_tbl;
	u32				rd_idx;
	u32				wr_idx;
	u32				idx_mask;
};

#define HEADROOM (NET_SKB_PAD + NET_IP_ALIGN)

static struct sk_buff *pfeng_bman_build_skb(struct pfeng_rx_chnl_pool *pool, bool preempt)
{
	const u32 truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		SKB_DATA_ALIGN(NET_SKB_PAD + NET_IP_ALIGN + RX_BUF_SIZE + SKB_VA_SIZE);
	struct sk_buff *skb;

	/* Request skb from DMA safe region */
	if (likely(preempt))
		preempt_disable();
	skb = __napi_alloc_skb(pool->napi, truesize, GFP_DMA32 | GFP_ATOMIC);
	if (likely(preempt))
		preempt_enable();
	if (!skb) {
		dev_err(pool->dev, "chnl%d: No skb created\n", pool->id);
		return NULL;
	}

	/* embed skb pointer */
	*(struct sk_buff **)(skb->data) = skb;
	/* forward skb->data to save skb ptr */
	skb_put(skb, SKB_VA_SIZE);
	skb_pull(skb, SKB_VA_SIZE);

	return skb;
}

void pfeng_bman_pool_destroy(struct pfeng_ndev *ndev)
{
	struct pfeng_rx_chnl_pool *rx_pool = (struct pfeng_rx_chnl_pool *)ndev->bman.rx_pool;
	struct pfeng_tx_chnl_pool *tx_pool = (struct pfeng_tx_chnl_pool *)ndev->bman.tx_pool;

	if (rx_pool) {
		if(rx_pool->rx_tbl) {
			kfree(rx_pool->rx_tbl);
			rx_pool->rx_tbl = NULL;
		}

		kfree(rx_pool);
	}

	if (tx_pool) {
		if(tx_pool->tx_tbl) {
			kfree(tx_pool->tx_tbl);
			tx_pool->tx_tbl = NULL;
		}

		kfree(tx_pool);
	}

	return;
}

int pfeng_bman_pool_create(struct pfeng_ndev *ndev)
{
	pfe_hif_chnl_t *chnl = ndev->chnl_sc.priv;
	struct pfeng_rx_chnl_pool *rx_pool;
	struct pfeng_tx_chnl_pool *tx_pool;

	/* RX pool */
	rx_pool = kzalloc(sizeof(*rx_pool), GFP_KERNEL);
	if (!rx_pool) {
		dev_err(ndev->dev, "chnl%d: No mem for bman rx_pool\n", pfe_hif_chnl_get_id(chnl));
		return -ENOMEM;
	}

	rx_pool->id = pfe_hif_chnl_get_id(chnl);
	rx_pool->depth = pfe_hif_chnl_get_rx_fifo_depth(chnl);
	rx_pool->avail = rx_pool->depth;
	rx_pool->chnl = chnl;
	rx_pool->dev = ndev->dev;
	rx_pool->napi = &ndev->napi;
	rx_pool->buf_size = RX_BUF_SIZE;

	rx_pool->rx_tbl = kzalloc(sizeof(void *) * rx_pool->depth, GFP_KERNEL);
	if (!rx_pool->rx_tbl) {
		dev_err(ndev->dev, "chnl%d: failed. No mem\n", rx_pool->id);
		goto err;
	}
	rx_pool->rd_idx = 0;
	rx_pool->wr_idx = 0;
	rx_pool->idx_mask = pfe_hif_chnl_get_rx_fifo_depth(chnl) - 1;

	ndev->bman.rx_pool = rx_pool;

	/* TX pool */
	tx_pool = kzalloc(sizeof(*tx_pool), GFP_KERNEL);
	if (!rx_pool) {
		dev_err(ndev->dev, "chnl%d: No mem for bman tx_pool\n", pfe_hif_chnl_get_id(chnl));
		goto err;
	}

	tx_pool->depth = pfe_hif_chnl_get_tx_fifo_depth(chnl);
	tx_pool->tx_tbl = kzalloc(sizeof(struct pfeng_tx_map) * tx_pool->depth, GFP_KERNEL);
	if (!tx_pool->tx_tbl) {
		dev_err(ndev->dev, "chnl%d: failed. No mem\n", rx_pool->id);
		goto err;
	}
	tx_pool->rd_idx = 0;
	tx_pool->wr_idx = 0;
	tx_pool->idx_mask = pfe_hif_chnl_get_tx_fifo_depth(chnl) - 1;

	ndev->bman.tx_pool = tx_pool;

	return 0;

err:
	pfeng_bman_pool_destroy(ndev);
	return -ENOMEM;
}

static inline addr_t pfeng_bman_buf_map(void *ctx, void *paddr)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	return dma_map_single_attrs(pool->dev, paddr, RX_BUF_SIZE, DMA_FROM_DEVICE, 0);
}

static inline void *pfeng_bman_buf_alloc_and_map(void *ctx, addr_t *paddr, bool preempt)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;
	struct sk_buff *skb;
	addr_t map;

	skb = pfeng_bman_build_skb(pool, preempt);
	if (!skb)
		return NULL;

	/* do dma map */
	map = dma_map_single(pool->dev, skb->data, RX_BUF_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(pool->dev, map)) {
		kfree_skb(skb);
		dev_err(pool->dev, "chnl%d: dma map error\n", pool->id);
		return NULL;
	}
	*paddr = map;

	return skb->data;
}

static inline void pfeng_bman_buf_unmap(void *ctx, addr_t paddr)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	dma_unmap_single(pool->dev, paddr, RX_BUF_SIZE, DMA_FROM_DEVICE);
}

static inline void *pfeng_bman_buf_pull_va(void *ctx)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	return pool->rx_tbl[pool->rd_idx++ & pool->idx_mask];
}

static inline int pfeng_bman_buf_push_va(void *ctx, void *vaddr)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	pool->rx_tbl[pool->wr_idx++ & pool->idx_mask] = vaddr;
	return 0;
}

static inline void pfeng_bman_buf_free(void *ctx, void *vaddr)
{
	/* FIXME: recycle back to fifo pool */
}

static inline u32 pfeng_bman_buf_size(void *ctx)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	return pool->buf_size;
}

bool pfeng_hif_chnl_txconf_check(struct pfeng_ndev *ndev, u32 elems)
{
	struct pfeng_tx_chnl_pool *pool = ndev->bman.tx_pool;
	u32 idx = pool->wr_idx;

	if(unlikely(elems >= pool->depth))
		return false;

	/* Check if last element is free */
	idx = (pool->wr_idx + elems) & pool->idx_mask;
	return !pool->tx_tbl[idx].size;
}

int pfeng_hif_chnl_txconf_put_map_frag(struct pfeng_ndev *ndev, void *va_addr, addr_t pa_addr, u32 size, struct sk_buff *skb)
{
	struct pfeng_tx_chnl_pool *pool = ndev->bman.tx_pool;
	u32 idx = pool->wr_idx;

	pool->tx_tbl[idx].va_addr = va_addr;
	pool->tx_tbl[idx].pa_addr = pa_addr;
	pool->tx_tbl[idx].size = size;
	pool->tx_tbl[idx].skb = skb;

	pool->wr_idx = (pool->wr_idx + 1) & pool->idx_mask;

	return idx;
}

int pfeng_hif_chnl_txconf_free_map_full(struct pfeng_ndev *ndev, u32 idx)
{
	struct pfeng_tx_chnl_pool *pool = ndev->bman.tx_pool;
	struct sk_buff *skb = pool->tx_tbl[idx].skb;
	u32 nfrags;

	BUG_ON(!skb);
	BUG_ON(idx != pool->rd_idx);

	nfrags = skb_shinfo(skb)->nr_frags;

	/* Unmap linear part */
	dma_unmap_single_attrs(ndev->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE, 0);
	pool->tx_tbl[idx].size = 0;

	idx = pool->rd_idx = (pool->rd_idx + 1 ) & pool->idx_mask;

	/* Unmap frags */
	while (nfrags--) {
		dma_unmap_page(ndev->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE);
		pool->tx_tbl[idx].size = 0;

		idx = pool->rd_idx = (pool->rd_idx + 1 ) & pool->idx_mask;
	}

	dev_consume_skb_any(skb);

	return 0;
}

int pfeng_hif_chnl_txconf_unroll_map_full(struct pfeng_ndev *ndev, u32 idx, u32 nfrags)
{
	struct pfeng_tx_chnl_pool *pool = ndev->bman.tx_pool;
	struct sk_buff *skb = pool->tx_tbl[idx].skb;

	BUG_ON(!skb);
	BUG_ON(idx != ((pool->wr_idx - 1) & pool->idx_mask));

	/* Unmap frags */
	while (nfrags--) {
		dma_unmap_page(ndev->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE);
		pool->tx_tbl[idx].size = 0;

		idx = pool->wr_idx = (pool->wr_idx - 1 ) & pool->idx_mask;
	}

	/* Unmap linear part */
	dma_unmap_single_attrs(ndev->dev, pool->tx_tbl[idx].pa_addr, pool->tx_tbl[idx].size, DMA_TO_DEVICE, 0);
	pool->tx_tbl[idx].size = 0;

	idx = pool->wr_idx = (pool->wr_idx - 1 ) & pool->idx_mask;

	dev_consume_skb_any(skb);

	return 0;
}

/*

	The following library is pfeng driver re-implementation
	of pfe_hif_drv RX calls, with support of bman.

*/

struct sk_buff *pfeng_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue)
{
	void *buf_pa, *buf_va;
	uint32_t rx_len;
	bool_t lifm;
	struct sk_buff *skb;
	struct pfeng_ndev *ndev = (struct pfeng_ndev *)pfe_hif_drv_client_get_priv(client);

	/*	Get RX buffer */
	if (EOK != pfe_hif_chnl_rx(ndev->chnl_sc.priv, &buf_pa, &rx_len, &lifm))
	{
		return NULL;
	}

	/*  Get buffer VA */
	buf_va = pfeng_bman_buf_pull_va(ndev->bman.rx_pool);
	if (unlikely(!buf_va)) {
		netdev_err(ndev->netdev, "chnl%d: pull VA failed\n", pfe_hif_chnl_get_id(ndev->chnl_sc.priv));
		pfeng_bman_buf_unmap(ndev->bman.rx_pool, (addr_t)buf_pa);
		return NULL;
	}
	prefetch(buf_va - SKB_VA_SIZE);

	/*  Unmap DMAed area */
	pfeng_bman_buf_unmap(ndev->bman.rx_pool, (addr_t)buf_pa);

	/* Retrieve saved skb address */
	skb = *((struct sk_buff **)(buf_va - SKB_VA_SIZE));
	__skb_put(skb, rx_len);

	return skb;
}

int pfeng_hif_chnl_refill_rx_buffer(struct pfeng_ndev *ndev, bool preempt)
{
	pfe_hif_chnl_t *chnl = ndev->chnl_sc.priv;
	void *buf_va;
	addr_t buf_pa = 0;
	errno_t ret;

	/*	Ask for new buffer */
	buf_va = pfeng_bman_buf_alloc_and_map(ndev->bman.rx_pool, &buf_pa, preempt);
	if (unlikely((NULL == buf_va) || (NULL == (void *)buf_pa))) {
		netdev_err(ndev->netdev, "No skb buffer available to fetch\n");
		return -ENOMEM;
	}

	/*	Add new buffer to ring */
	ret = pfe_hif_chnl_supply_rx_buf(chnl, (void *)buf_pa, pfeng_bman_buf_size(ndev->bman.rx_pool));
	if (unlikely(ret)) {
		pfeng_bman_buf_unmap(ndev->bman.rx_pool, buf_pa);
		netdev_err(ndev->netdev, "chnl%d: Impossible to feed new buffer to the ring\n", pfe_hif_chnl_get_id(chnl));
		return -ret;
	}
	pfeng_bman_buf_push_va(ndev->bman.rx_pool, buf_va);

	return 0;
}

int pfeng_hif_chnl_fill_rx_buffers(struct pfeng_ndev *ndev)
{
	errno_t ret;
	int cnt = 0;

	while (pfe_hif_chnl_can_accept_rx_buf(ndev->chnl_sc.priv)) {
		ret = pfeng_hif_chnl_refill_rx_buffer(ndev, true);
		if (ret)
			break;
		cnt++;
	}

	return cnt;
}
