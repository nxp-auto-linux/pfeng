/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
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
	u32				id;
	u32				depth;
	u32				avail;
	u32				buf_size;

	void				**va_tbl;	/* skb VA table in hif drv rx ring */
	u32				va_tbl_rd_idx;
	u32				va_tbl_wr_idx;
	u32				va_tbl_idx_mask;
};

#define HEADROOM (NET_SKB_PAD + NET_IP_ALIGN)

static struct sk_buff *pfeng_bman_build_skb(struct pfeng_rx_chnl_pool *pool)
{
	const u32 truesize = SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) +
		SKB_DATA_ALIGN(NET_SKB_PAD + NET_IP_ALIGN + RX_BUF_SIZE + SKB_VA_SIZE);
	struct sk_buff *skb;
	u8 *buf;

	buf = napi_alloc_frag(truesize);
	if (!buf) {
		dev_err(pool->dev, "chnl%d: No mem for skb\n", pool->id);
		return NULL;
	}

	/* build an skb around the page buffer */
	skb = build_skb(buf, truesize);
	if (!skb) {
		dev_err(pool->dev, "chnl%d: No skb created\n", pool->id);
		skb_free_frag(buf);
		return NULL;
	}

	/* embed skb pointer */
	*(struct sk_buff **)(skb->data) = skb;
	/* forward skb->data to save skb ptr */
	skb_put(skb, SKB_VA_SIZE);
	skb_pull(skb, SKB_VA_SIZE);

	return skb;
}

void pfeng_bman_pool_destroy(void *ctx)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	if (pool) {
		if(pool->va_tbl) {
			kfree(pool->va_tbl);
			pool->va_tbl = NULL;
		}

		kfree(pool);
	}

	return;
}

void *pfeng_bman_pool_create(pfe_hif_chnl_t *chnl, void *ref)
{
	struct device *dev = (struct device *)ref;
	struct pfeng_rx_chnl_pool *pool;

	pool = kzalloc(sizeof(*pool), GFP_KERNEL);

	if (!pool) {
		dev_err(dev, "chnl%d: No mem for bman pool\n", pfe_hif_chnl_get_id(chnl));
		return NULL;
	}

	pool->id = pfe_hif_chnl_get_id(chnl);
	pool->depth = pfe_hif_chnl_get_rx_fifo_depth(chnl);
	pool->avail = pool->depth;
	pool->chnl = chnl;
	pool->dev = dev;
	pool->buf_size = RX_BUF_SIZE;

	pool->va_tbl = kzalloc(sizeof(void *) * pool->depth, GFP_KERNEL);
	if (!pool->va_tbl) {
		dev_err(dev, "chnl%d: failed. No mem\n", pool->id);
		goto err;
	}
	pool->va_tbl_rd_idx = 0;
	pool->va_tbl_wr_idx = 0;
	pool->va_tbl_idx_mask = pfe_hif_chnl_get_tx_fifo_depth(chnl) - 1;

	return pool;

err:
	pfeng_bman_pool_destroy(pool);
	return NULL;
}

static inline void *pfeng_bman_buf_alloc_and_map(void *ctx, addr_t *paddr)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;
	struct sk_buff *skb;
	addr_t map;

	skb = pfeng_bman_build_skb(pool);
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

	return pool->va_tbl[pool->va_tbl_rd_idx++ & pool->va_tbl_idx_mask];
}

static inline int pfeng_bman_buf_push_va(void *ctx, void *vaddr)
{
	struct pfeng_rx_chnl_pool *pool = (struct pfeng_rx_chnl_pool *)ctx;

	pool->va_tbl[pool->va_tbl_wr_idx++ & pool->va_tbl_idx_mask] = vaddr;
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

int pfeng_hif_chnl_refill_rx_buffer(pfe_hif_chnl_t *chnl, struct pfeng_ndev *ndev)
{
	void *buf_va;
	addr_t buf_pa = 0;
	errno_t ret;

	/*	Ask for new buffer */
	buf_va = pfeng_bman_buf_alloc_and_map(ndev->bman.rx_pool, &buf_pa);
	if (unlikely((NULL == buf_va) || (NULL == (void *)buf_pa))) 		{
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

int pfeng_hif_chnl_fill_rx_buffers(pfe_hif_chnl_t *chnl, struct pfeng_ndev *ndev)
{
	errno_t ret;
	int cnt = 0;

	while (pfe_hif_chnl_can_accept_rx_buf(chnl)) {
		ret = pfeng_hif_chnl_refill_rx_buffer(chnl, ndev);
		if (ret)
			break;
		cnt++;
	}

	return cnt;
}
