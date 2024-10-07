/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>
#include <linux/if_vlan.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

#include "pfeng.h"
#define PFE_DEFAULT_TX_WORK (PFE_CFG_HIF_RING_LENGTH >> 1)

#define pfeng_priv_for_each_chnl(priv, chnl_idx, chnl)			\
	for (chnl_idx = 0, chnl = &priv->hif_chnl[chnl_idx];		\
		chnl_idx < PFENG_PFE_HIF_CHANNELS;			\
		chnl_idx++, chnl = &priv->hif_chnl[chnl_idx])

static const u32 pfeng_hif_id_to_vlan_rx_flag[] = {
	HIF_RX_HIF0_VLAN,
	HIF_RX_HIF1_VLAN,
	HIF_RX_HIF2_VLAN,
	HIF_RX_HIF3_VLAN
};

static int pfeng_hif_chnl_stop(struct pfeng_hif_chnl *chnl)
{
	/* Limit number of the sleep cycles it can wait for TX to complete */
	int timeout = 50000;

	/* Stop enqueuing by changing HIF channel status */
	chnl->status = PFENG_HIF_STATUS_ENABLED;

	/* Wait all tx packets to be transmitted */
	do {
		if (!timeout--) {
			HM_MSG_DEV_ERR(chnl->dev, "HIF%d TX stop timed out\n", chnl->idx);
			break;
		}

		/* Wait to make sure TX are processed */
		oal_time_usleep(10U);
	} while (!pfeng_hif_chnl_txbd_is_empty(chnl));

	/* Disable channel interrupt */
	pfe_hif_chnl_irq_mask(chnl->priv);

	/* Disable the channel RX/TX interrupts */
	pfe_hif_chnl_rx_irq_mask(chnl->priv);
	pfe_hif_chnl_tx_irq_mask(chnl->priv);

	/* Disable RX */
	pfe_hif_chnl_rx_disable(chnl->priv);

	/* Disable TX */
	pfe_hif_chnl_tx_disable(chnl->priv);

	HM_MSG_DEV_INFO(chnl->dev, "HIF%d stopped\n", chnl->idx);

	return 0;
}

int pfeng_hif_chnl_start(struct pfeng_hif_chnl *chnl)
{
	if (chnl->status == PFENG_HIF_STATUS_RUNNING)
		return 0;

	if (chnl->status != PFENG_HIF_STATUS_ENABLED)
		return -EINVAL;

	/* Enable channel interrupt */
	pfe_hif_chnl_irq_unmask(chnl->priv);

	/* Enable RX */
	if (pfe_hif_chnl_rx_enable(chnl->priv) != EOK) {
		HM_MSG_DEV_ERR(chnl->dev, "Couldn't enable RX irq\n");
		return -EINVAL;
	}

	/* Enable TX */
	if (pfe_hif_chnl_tx_enable(chnl->priv) != EOK) {
		HM_MSG_DEV_ERR(chnl->dev, "Couldn't enable TX\n");
		return -EINVAL;
	}

	/* Enable the channel RX/TX interrupts */
	pfe_hif_chnl_rx_irq_unmask(chnl->priv);
	pfe_hif_chnl_tx_irq_unmask(chnl->priv);

	chnl->status = PFENG_HIF_STATUS_RUNNING;

	HM_MSG_DEV_INFO(chnl->dev, "HIF%d started\n", chnl->idx);

	return 0;
}

int pfeng_hif_slave_suspend(struct pfeng_priv *priv)
{
	struct pfeng_hif_chnl *chnl;
	int i;

	pfeng_priv_for_each_chnl(priv, i, chnl)
		if (chnl->status >= PFENG_HIF_STATUS_ENABLED)
			pfeng_hif_chnl_stop(chnl);

	return 0;
}

int pfeng_hif_slave_resume(struct pfeng_priv *priv)
{
	struct pfeng_hif_chnl *chnl;
	int i;

	pfeng_priv_for_each_chnl(priv, i, chnl)
		if (chnl->status >= PFENG_HIF_STATUS_ENABLED)
			pfeng_hif_chnl_start(chnl);

	return 0;
}

/**
 * @brief		HIF channel ISR
 * @details		Will be called by HIF channel instance when an event has occurred
 * @note		To see which context the ISR is running in refer to the
 * 				pfe_hif_chnl module implementation.
 */
static void pfeng_hif_drv_chnl_isr(void *arg)
{
        struct pfeng_hif_chnl *chnl = (struct pfeng_hif_chnl *)arg;

	if(napi_schedule_prep(&chnl->napi)) {

		pfe_hif_chnl_rx_irq_mask(chnl->priv);
		pfe_hif_chnl_tx_irq_mask(chnl->priv);

		__napi_schedule(&chnl->napi);
	}
}

/**
 * @brief		Common HIF channel interrupt service routine
 * @details		Manage common HIF channel interrupt
 * @details		See the oal_irq_handler_t
 */
static irqreturn_t pfeng_hif_chnl_direct_isr(int irq, void *arg)
{
	pfe_hif_chnl_t *chnl = (pfe_hif_chnl_t *)arg;

	/* Disable HIF channel interrupts */
	pfe_hif_chnl_irq_mask(chnl);

	/* Call HIF channel ISR */
	pfe_hif_chnl_isr(chnl);

	/* Enable HIF channel interrupts */
	pfe_hif_chnl_irq_unmask(chnl);

	return IRQ_HANDLED;
}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

void pfe_hif_drv_set_idex_resend_cfg(pfe_hif_drv_t *hif_drv, const uint32_t idex_resend_count, const uint32_t *idex_resend_delays)
{
	hif_drv->idex_resend_count = idex_resend_count;
	hif_drv->idex_resend_delays[0] = idex_resend_delays[0];
	hif_drv->idex_resend_delays[1] = idex_resend_delays[1];
}

void pfe_hif_drv_get_idex_resend_cfg(const pfe_hif_drv_t *hif_drv, uint32_t *idex_resend_count, uint32_t *idex_resend_delays)
{
	*idex_resend_count = hif_drv->idex_resend_count;
	idex_resend_delays[0] = hif_drv->idex_resend_delays[0];
	idex_resend_delays[1] = hif_drv->idex_resend_delays[1];
}

static int pfe_hif_drv_ihc_put_pkt(pfe_hif_drv_client_t *client, void *data, uint32_t len, void *ref)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	pfe_ct_hif_rx_hdr_t *hif_hdr = data;
	pfe_hif_pkt_t *rx_metadata;

	/*	Create the RX metadata */
	rx_metadata = kzalloc(sizeof(*rx_metadata), GFP_ATOMIC);
	if (unlikely(!rx_metadata))
		return -ENOMEM;

	rx_metadata->client = client;
	rx_metadata->data = (addr_t)data;
	rx_metadata->len = len;

	rx_metadata->flags.specific.rx_flags = hif_hdr->flags;
	rx_metadata->i_phy_if = hif_hdr->i_phy_if;
	rx_metadata->ref_ptr = ref;

	if (unlikely(EOK != fifo_put(client->ihc_rx_fifo, rx_metadata))) {
		HM_MSG_DEV_ERR(chnl->dev, "IHC RX fifo full\n");
		kfree(rx_metadata);
		return -EINVAL;
	}

	return 0;
}

/* required by IDEX IHC Rx handler */
void pfe_hif_pkt_free(const pfe_hif_pkt_t *pkt)
{
	if (pkt->ref_ptr)
		dev_kfree_skb(pkt->ref_ptr);
	kfree(pkt);
}

static int pfe_hif_drv_ihc_put_tx_conf(pfe_hif_drv_client_t *client, void *data, uint32_t len)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	void *idex_frame;

	idex_frame = kmalloc(len, GFP_ATOMIC);
	if (unlikely(!idex_frame))
		return -ENOMEM;

	memcpy(idex_frame, data, len);

	if (unlikely(EOK != fifo_put(client->ihc_txconf_fifo, idex_frame))) {
		HM_MSG_DEV_ERR(chnl->dev, "IHC TX fifo full\n");
		kfree(idex_frame);
		return -EINVAL;
	}

	return 0;
}

static void pfeng_tx_conf_free(void *idex_frame)
{
	kfree(idex_frame);
}

void pfeng_ihc_rx_work_handler(struct work_struct *work)
{
	struct pfeng_priv* priv = container_of(work, struct pfeng_priv, ihc_rx_work);
	pfe_hif_drv_client_t *client = &priv->ihc_chnl->ihc_client;
	uint32_t fill_level;

	fifo_get_fill_level(client->ihc_txconf_fifo, &fill_level);

	if (fill_level) {
		/* Call IHC TX callback */
		client->event_handler(client, client->priv, EVENT_TXDONE_IND, 0);
	}

	fifo_get_fill_level(client->ihc_rx_fifo, &fill_level);

	if (fill_level) {
		/* Call IHC RX callback */
		client->event_handler(client, client->priv, EVENT_RX_PKT_IND, 0);
	}
}

static int pfeng_ihc_dispatch_rx_msg(struct pfeng_hif_chnl *chnl, struct sk_buff *skb)
{
	pfe_hif_drv_client_t *client;
	int ret;

	if (unlikely (!chnl->ihc)) {
		HM_MSG_DEV_WARN(chnl->dev, "IHC message on wrong HIF%d channel\n", chnl->idx);
		return 0;
	}

	client = &chnl->ihc_client;

	/* IHC client callback */
	ret = pfe_hif_drv_ihc_put_pkt(client, skb->data, skb->len, skb);
	if (!ret) {
		struct pfeng_priv *priv = dev_get_drvdata(chnl->dev);

		queue_work(priv->ihc_wq, &priv->ihc_rx_work);
	}

	return ret;
}

#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

static inline void pfeng_hwts_skb_set_rx_ts(struct skb_shared_hwtstamps *hwts, u32 rx_timestamp_s, u32 rx_timestamp_ns)
{
	u64 nanos = 0ULL;

	memset(hwts, 0, sizeof(*hwts));
	nanos = rx_timestamp_ns;
	nanos += rx_timestamp_s * 1000000000ULL;
	hwts->hwtstamp = ns_to_ktime(nanos);
}

/**
 * @brief	Process HIF channel receive
 * @details	Read HIF channel data
 * @param[in]	chnl The HIF channel
 * @param[in]	limit The recieve process limit
 * @return	Number of received frames
 */
static int pfeng_hif_chnl_rx(struct pfeng_hif_chnl *chnl, int limit)
{
	pfe_ct_hif_rx_hdr_t *hif_hdr;
	struct net_device *netdev;
	struct pfeng_netif *netif;
	struct sk_buff *skb;
	__be32 vlan_tag = 0;
	int done = 0;

	while (likely(done < limit)) {
		skb = pfeng_hif_chnl_receive_pkt(chnl);
		if (unlikely(!skb))
			/* no more packets */
			break;

		hif_hdr = (pfe_ct_hif_rx_hdr_t *)skb->data;
		hif_hdr->flags = (pfe_ct_hif_rx_flags_t)oal_ntohl(hif_hdr->flags);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Check for IHC frame */
		if (unlikely(hif_hdr->flags & HIF_RX_IHC)) {
			int ret = pfeng_ihc_dispatch_rx_msg(chnl, skb);

			if (unlikely(ret)) {
				HM_MSG_DEV_ERR(chnl->dev, "Failed to dispatch message from PHYIF#%d (err %d)\n",
					hif_hdr->i_phy_if, ret);
				kfree_skb(skb);
			}

			done++;
			continue;
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

		/* get target netdevice */
		netif = pfeng_phy_if_id_to_netif(chnl, hif_hdr->i_phy_if);
		if (unlikely(!netif)) {
			HM_MSG_DEV_DBG(chnl->dev, "Missing netdev for packet from PHYIF#%d\n",
				hif_hdr->i_phy_if);
			consume_skb(skb);
			done++;
			continue;
		}
		/* AUX extra routing
		 * When netif has set config flag 'only_mgmt' and AUX netif exists,
		 * the "non-management" traffic (PTP, egress TS, mirrored) is routed to AUX
		 * and only "management" traffic is passed to regular netif
		 */
		if (likely(!netif->cfg->only_mgmt)) {
			/* Accept all traffic */
		} else if (likely(!chnl->netifs[PFE_PHY_IF_ID_AUX])) {
			/* No AUX */
		} else if (unlikely(hif_hdr->flags & (HIF_RX_PTP | HIF_RX_ETS))) {
			/* Frame is "management" */
		} else {
			/* Frame is "non-management", route to AUX */
			netif = chnl->netifs[PFE_PHY_IF_ID_AUX];
		}

		netdev = netif->netdev;
		skb->dev = netdev;

		if (unlikely(hif_hdr->flags & pfeng_hif_id_to_vlan_rx_flag[chnl->idx]))
		{
			/* handle VLAN tag insertion h/w erratum */
			BUILD_BUG_ON(offsetof(pfe_ct_hif_rx_hdr_t, rx_timestamp_s) != 12U);
			vlan_tag = hif_hdr->rx_timestamp_s;
		}

		if (likely(hif_hdr->flags & HIF_RX_TS)) {
			u32 rx_timestamp_s = hif_hdr->rx_timestamp_s;

			if (unlikely(vlan_tag)) {
				/* retrieve the TS from skb->data */
				rx_timestamp_s = *(u32 *)(skb->data + PFENG_RX_PKT_HEADER_SIZE);
			}

			/* store HW TS to skb */
			pfeng_hwts_skb_set_rx_ts(skb_hwtstamps(skb), rx_timestamp_s, hif_hdr->rx_timestamp_ns);

		} else if(unlikely(hif_hdr->flags & HIF_RX_ETS)) {
			pfe_ct_ets_report_t *etsr = (pfe_ct_ets_report_t *)(skb->data + PFENG_RX_PKT_HEADER_SIZE);

			if (unlikely(vlan_tag)) {
				etsr = (pfe_ct_ets_report_t *)(skb->data + PFENG_RX_PKT_HEADER_SIZE + VLAN_HLEN);
			}

			/* Get tx hw time stamp */
			pfeng_hwts_get_tx_ts(netif, etsr);

			/* Skb has only time stamp report so consume it */
			consume_skb(skb);
			done++;
			continue;
		}

		/* Cksumming support */
		if (likely(netdev->features & NETIF_F_RXCSUM) &&
		    likely(hif_hdr->flags & (HIF_RX_UDPV4_CSUM | HIF_RX_TCPV4_CSUM | HIF_RX_UDPV6_CSUM | HIF_RX_TCPV6_CSUM))) {
			/* we have only OK info, signal it */
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			/* one level csumming support */
			skb->csum_level = 0;
		}

		/* Skip HIF header */
		skb_pull(skb, PFENG_RX_PKT_HEADER_SIZE);

		if (unlikely(vlan_tag)) {
			skb_pull(skb, VLAN_HLEN);
			/* vlan_tag is in big endian format */
			__vlan_hwaccel_put_tag(skb, vlan_tag & 0xffff, be16_to_cpu(vlan_tag >> 16));
		}

		/* Pass to upper layer */

		skb->protocol = eth_type_trans(skb, netdev);

		netdev->stats.rx_packets++;
		netdev->stats.rx_bytes += skb->len;
		napi_gro_receive(&chnl->napi, skb);

		done++;
	}

	return done;
}

static bool pfeng_hif_chnl_tx_conf(struct pfeng_hif_chnl *chnl, int napi_budget)
{
	unsigned int done = 0;
	int ret;

	while (done < PFE_DEFAULT_TX_WORK) {

		/* remove a single frame from the tx ring */
		ret = pfe_hif_chnl_get_tx_conf(chnl->priv);
		if (unlikely(ret == EAGAIN))
			break; /* back to napi polling to try later */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Check for IHC packet first */
		if (unlikely (pfeng_hif_chnl_txconf_get_flag(chnl) == PFENG_MAP_PKT_IHC)) {
			struct sk_buff *skb = pfeng_hif_chnl_txconf_get_skbuf(chnl);
			pfe_hif_drv_client_t *client = &chnl->ihc_client;

			/* IDEX confirmation must return IDEX API compatible data */
			skb_pull(skb, PFENG_TX_PKT_HEADER_SIZE);
			if (!pfe_hif_drv_ihc_put_tx_conf(client, skb->data, skb_headlen(skb))) {
				struct pfeng_priv *priv = dev_get_drvdata(chnl->dev);

				/* process Tx confirmations together Rx buffers */
				queue_work(priv->ihc_wq, &priv->ihc_rx_work);

			} else {
				HM_MSG_DEV_ERR(chnl->dev, "TXconf IHC queuing failed.\n");
			}
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

		ret = pfeng_hif_chnl_txconf_free_map_full(chnl, napi_budget);
		if (unlikely(ret)) {
			HM_MSG_DEV_ERR(chnl->dev, "Bad Tx buffer (skb) mapping, reset transmission!\n");
			/* stop ring cleanup to force Tx timeout and ndev reset */
			return true;
		}

		done++;
	}

	if (pfeng_hif_chnl_txbd_unused(chnl) >= PFE_TXBDS_MAX_NEEDED) {
		int i;

		for (i = 0; i < PFENG_NETIFS_CNT; i++) {
			struct pfeng_netif *netif = chnl->netifs[i];

			if (!netif)
				continue;

			if (__netif_subqueue_stopped(netif->netdev, 0)) {
				smp_rmb();
				if (!test_bit(PFENG_TMU_FULL, &netif->tx_queue_status)) {
					netif_wake_subqueue(netif->netdev, 0);
				}
			}
		}
	}

	return done < PFE_DEFAULT_TX_WORK;
}

static int pfeng_hif_chnl_poll(struct napi_struct *napi, int budget)
{
	struct pfeng_hif_chnl *chnl = container_of(napi, struct pfeng_hif_chnl, napi);
	bool complete;
	int work_done = 0;

	complete = pfeng_hif_chnl_tx_conf(chnl, budget);
	/* Consume RX pkt(s) */
	work_done = pfeng_hif_chnl_rx(chnl, budget);
	if (work_done >= budget)
		complete = false;

	if (!complete)
		return budget;

	if (likely(napi_complete_done(napi, work_done))) {
		/* Enable interrupts */
		pfe_hif_chnl_rx_irq_unmask(chnl->priv);
		pfe_hif_chnl_tx_irq_unmask(chnl->priv);
	}

	return work_done;
}

int pfeng_hif_chnl_set_coalesce(struct pfeng_hif_chnl *chnl, struct clk *clk_sys, u32 usecs, u32 frames)
{
	u32 cycles;
	int ret;

	cycles = usecs * (DIV_ROUND_UP(pfeng_clk_sys_get_rate(clk_sys), USEC_PER_SEC));

	ret = pfe_hif_chnl_set_rx_irq_coalesce(chnl->priv, frames, cycles);
	if (!ret) {
		chnl->cfg_rx_max_coalesced_frames = frames;
		chnl->cfg_rx_coalesce_usecs = usecs;
	}

	return -ret; /* convert platform err code to linux kernel err code */
}

static int pfeng_hif_chnl_drv_stop(struct pfeng_priv *priv, u32 idx)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl;
	int count;

	if (idx >= PFENG_PFE_HIF_CHANNELS) {
		HM_MSG_DEV_ERR(dev, "Invalid HIF instance number: %u\n", idx);
		return -ENODEV;
	}
	chnl = &priv->hif_chnl[idx];

	/* Stop channel interrupt */
	pfeng_hif_chnl_stop(chnl);

	/* Stop NAPI */
	if (chnl->status >= PFENG_HIF_STATUS_ENABLED) {
		napi_disable(&chnl->napi);
		netif_napi_del(&chnl->napi);
	}
	/* Prepare for startup state (in case of STR use) */
	chnl->status = PFENG_HIF_STATUS_REQUESTED;

	/* Confirm all transmitted packets */
	count = 0;
	while (pfe_hif_chnl_get_tx_conf(chnl->priv) == EOK) {
		pfeng_hif_chnl_txconf_free_map_full(chnl, 0);
		count++;
	}

	if (count > 0)
		HM_MSG_DEV_WARN(dev, "Unconfirmed TX packet number: %u\n", count);

	return 0;
}

static int pfeng_hif_chnl_drv_remove(struct pfeng_priv *priv, u32 idx)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl;

	if (idx >= PFENG_PFE_HIF_CHANNELS) {
		HM_MSG_DEV_ERR(dev, "Invalid HIF instance number: %u\n", idx);
		return -ENODEV;
	}
	chnl = &priv->hif_chnl[idx];

	/* Release attached RX/TX pools */
	pfeng_bman_pool_destroy(chnl);

	/* Release IRQ line */
	disable_irq(priv->pfe_cfg->irq_vector_hif_chnls[idx]);
	irq_set_affinity_hint(priv->pfe_cfg->irq_vector_hif_chnls[idx], NULL);
	free_irq(priv->pfe_cfg->irq_vector_hif_chnls[idx], chnl->priv);

	/* Forget HIF channel data */
	chnl->priv = NULL;

	HM_MSG_DEV_INFO(dev, "HIF%d disabled\n", idx);

	return 0;
}

static int pfeng_hif_chnl_drv_create(struct pfeng_priv *priv, u32 idx)
{
	int irq = priv->pfe_cfg->irq_vector_hif_chnls[idx];
	struct device *dev = &priv->pdev->dev;
	struct pfeng_hif_chnl *chnl;
	char irq_name[20];
	int ret = 0;

	if (idx >= PFENG_PFE_HIF_CHANNELS) {
		HM_MSG_DEV_ERR(dev, "Invalid HIF instance number: %u\n", idx);
		return -ENODEV;
	}
	chnl = &priv->hif_chnl[idx];

	chnl->priv = pfe_hif_get_channel(priv->pfe_platform->hif, pfeng_chnl_ids[idx]);
	if (NULL == chnl->priv) {
		HM_MSG_DEV_ERR(dev, "Can't get HIF%d channel instance\n", idx);
		return -ENODEV;
	}
	chnl->dev = dev;
	chnl->idx = idx;

	if (unlikely(chnl->refcount))
		spin_lock_init(&chnl->lock_tx);

	/* Register HIF channel RX/TX callback */
	pfe_hif_chnl_set_event_cbk(chnl->priv, HIF_CHNL_EVT_RX_IRQ | HIF_CHNL_EVT_TX_IRQ,
				   pfeng_hif_drv_chnl_isr, (void *)chnl);

	/* Create interrupt name */
	scnprintf(irq_name, sizeof(irq_name), "pfe-hif-%d:%s", idx, chnl->refcount ? "shared" : "excl");

	/* HIF channel IRQ */
	ret = request_irq(irq, pfeng_hif_chnl_direct_isr, 0,
			  devm_kstrdup(dev, irq_name, GFP_KERNEL), chnl->priv);
	if (ret < 0) {
		HM_MSG_DEV_ERR(dev, "Error allocating the IRQ %d for '%s', error %d\n",
			irq, irq_name, ret);
		return ret;
	}

	/* configure interrupt affinity hint */
	irq_set_affinity_hint(irq, get_cpu_mask(idx % num_online_cpus()));

	/* Create bman for channel */
	if (!chnl->bman.rx_pool) {
		ret = pfeng_bman_pool_create(chnl);
		if (ret) {
			HM_MSG_DEV_ERR(dev, "Unable to attach bman to HIF%d\n", idx);
			goto err;
		}
		/* Fill pool of pages for rx buffers */
		pfeng_hif_chnl_fill_rx_buffers(chnl);
	}

	pfeng_debugfs_add_hif_chnl(priv, idx);

	/* Create dummy netdev required for independent HIF channel support */
	init_dummy_netdev(&chnl->dummy_netdev);

	/* init interrupt coalescing */
	pfeng_hif_chnl_set_coalesce(chnl, priv->clk_sys, PFENG_INT_TIMER_DEFAULT, 0);

	chnl->status = PFENG_HIF_STATUS_ENABLED;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
	netif_napi_add(&chnl->dummy_netdev, &chnl->napi, pfeng_hif_chnl_poll, NAPI_POLL_WEIGHT);
#else
	netif_napi_add(&chnl->dummy_netdev, &chnl->napi, pfeng_hif_chnl_poll);
#endif
	napi_enable(&chnl->napi);

	HM_MSG_DEV_INFO(dev, "HIF%d enabled\n", idx);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* IHC HIF channel must be enabled */
	if (chnl->ihc)
		pfeng_hif_chnl_start(chnl);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	return 0;

err:
	pfeng_hif_chnl_drv_stop(priv, idx);
	pfeng_hif_chnl_drv_remove(priv, idx);
	return ret;
}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
/* Release IDEX object */
static void pfeng_hif_idex_release(struct pfeng_priv *priv)
{
	struct pfeng_hif_chnl *chnl = priv->ihc_chnl;

	if (!priv->ihc_enabled)
		return;

	priv->ihc_enabled = false;

	if (chnl->ihc) {
		priv->ihc_chnl = NULL;
		pfe_idex_fini();
		HM_MSG_DEV_INFO(&priv->pdev->dev, "IDEX RPC released. HIF IHC support disabled\n");
	}
}

/* Create IDEX object */
static int pfeng_hif_idex_create(struct pfeng_priv *priv, int idx)
{
	struct pfeng_hif_chnl *ihc_chnl = &priv->hif_chnl[idx];
	struct device *dev = &priv->pdev->dev;
	int ret = 0;

	/* Install IDEX support */
	priv->ihc_chnl = ihc_chnl;
	pfe_hif_drv_set_idex_resend_cfg(&ihc_chnl->hif_drv, priv->idex_resend_count, priv->idex_resend_delays);
	ret = pfe_idex_init(&ihc_chnl->hif_drv,
			  pfeng_hif_ids[priv->ihc_master_chnl],
			  priv->pfe_platform->hif,
			  pfe_platform_idex_rpc_cbk, priv->pfe_platform,
			  pfeng_tx_conf_free);

	if (!ret) {
		priv->ihc_enabled = true;
		HM_MSG_DEV_INFO(dev, "IDEX RPC installed on HIF%d\n", idx);
		return 0;
	}

	HM_MSG_DEV_ERR(dev, "Can't initialize IDEX, HIF IHC support disabled.\n");
	ihc_chnl->ihc = false;
	priv->ihc_enabled = false;
	priv->ihc_chnl = NULL;
	return -ENODEV;
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief	Create HIF channels
 * @details	Creates and initializes the HIF channels
 * @param[in]	priv The driver main structure
 * @return	> 0 if OK, negative error number if failed
 */
int pfeng_hif_create(struct pfeng_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int idx, ret;

	ret = 0;

	for (idx = 0; idx < PFENG_PFE_HIF_CHANNELS; idx++) {

		if (priv->hif_chnl[idx].status != PFENG_HIF_STATUS_REQUESTED) {
			HM_MSG_DEV_INFO(dev, "HIF%d not configured, skipped\n", idx);
			continue;
		}

		ret = pfeng_hif_chnl_drv_create(priv, idx);
		if (ret) {
			HM_MSG_DEV_ERR(dev, "HIF %d can't be created\n", idx);
			return -EIO;
		}

		/* Set local driver_id */
#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Use lowest managed HIF channel */
		if (priv->local_drv_id > idx)
			priv->local_drv_id = idx;
#else
		/* Use IHC channel */
		if (priv->hif_chnl[idx].ihc) {
			priv->local_drv_id = idx;
			pfeng_hif_idex_create(priv, idx);
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	}

	return ret;
}

/**
 * @brief	Destroy the HIF channels
 * @details	Unregister and destroy the HIF channels
 * @param[in]	priv The driver main structure
 */
void pfeng_hif_remove(struct pfeng_priv *priv)
{
	struct device *dev;
	int idx;
	u8 chnl_stat[PFENG_PFE_HIF_CHANNELS];

	if (!priv)
		return;

	dev = &priv->pdev->dev;

	/*	First stop HIF channels */
	for (idx = (PFENG_PFE_HIF_CHANNELS - 1); idx >= 0; idx--) {
		chnl_stat[idx] = priv->hif_chnl[idx].status;

		if (priv->hif_chnl[idx].status < PFENG_HIF_STATUS_ENABLED) {
			HM_MSG_DEV_INFO(dev, "HIF%d not enabled, skipped\n", idx);
			continue;
		}

		pfeng_hif_chnl_drv_stop(priv, idx);
	}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* Stop IHC RX/TX works */
	cancel_work_sync(&priv->ihc_rx_work);
	cancel_work_sync(&priv->ihc_tx_work);
	flush_workqueue(priv->ihc_wq);

	/*	Unregister IHC client and release IDEX RPC */
	pfeng_hif_idex_release(priv);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	/*	Disable HIFs that have been enabled */
	for (idx = (PFENG_PFE_HIF_CHANNELS - 1); idx >= 0; idx--) {

		if (chnl_stat[idx] < PFENG_HIF_STATUS_ENABLED)
			continue;

		pfeng_hif_chnl_drv_remove(priv, idx);
	}
}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

/* structs pfe_hif_drv_tag and pfe_hif_drv_client_tag are declared in pfeng.h */

void pfe_hif_drv_client_unregister(pfe_hif_drv_client_t *client)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	/*	Release IHC fifo */
	if (client->ihc_rx_fifo) {
		u32 fill_level;
		errno_t err = fifo_get_fill_level(client->ihc_rx_fifo, &fill_level);

		if (unlikely(EOK != err)) {
			HM_MSG_DEV_INFO(chnl->dev, "Unable to get IHC fifo fill level: %d\n", err);
		} else if (fill_level) {
			HM_MSG_DEV_INFO(chnl->dev, "IHC Queue is not empty\n");
		}

		fifo_destroy(client->ihc_rx_fifo);
		client->ihc_rx_fifo = NULL;
	}

	if (client->ihc_txconf_fifo) {
		u32 fill_level;
		errno_t err = fifo_get_fill_level(client->ihc_txconf_fifo, &fill_level);

		if (unlikely(EOK != err)) {
			HM_MSG_DEV_INFO(chnl->dev, "Unable to get IHC tx conf fifo fill level: %d\n", err);
		} else if (fill_level) {
			HM_MSG_DEV_INFO(chnl->dev, "IHC Tx conf Queue is not empty\n");
		}

		fifo_destroy(client->ihc_txconf_fifo);
		client->ihc_txconf_fifo = NULL;
	}

	/*	Cleanup memory */
	memset(client, 0, sizeof(pfe_hif_drv_client_t));

	HM_MSG_DEV_INFO(chnl->dev, "IHC client unregistered\n");
}

pfe_hif_drv_client_t * pfe_hif_drv_ihc_client_register(pfe_hif_drv_t *hif_drv, pfe_hif_drv_client_event_handler handler, void *priv)
{
	struct pfeng_hif_chnl *chnl = container_of(hif_drv, struct pfeng_hif_chnl, hif_drv);
	pfe_hif_drv_client_t *client = &chnl->ihc_client;

	if (client->hif_drv) {
		HM_MSG_DEV_ERR(chnl->dev, "IHC client already registered\n");
		return NULL;
	}

	/* Initialize the instance */
	memset(client, 0, sizeof(pfe_hif_drv_client_t));
	client->ihc_rx_fifo = fifo_create(32);
	if (!client->ihc_rx_fifo) {
		HM_MSG_DEV_ERR(chnl->dev, "Can't create IHC RX fifo.\n");
		return NULL;
	}
	client->ihc_txconf_fifo = fifo_create(32);
	if (!client->ihc_txconf_fifo) {
		HM_MSG_DEV_ERR(chnl->dev, "Can't create IHC TXconf fifo.\n");
		return NULL;
	}
	client->hif_drv = hif_drv;
	client->priv = priv;
	client->event_handler = handler;
	client->inited = true;

	HM_MSG_DEV_INFO(chnl->dev, "IHC client registered\n");
	return client;
}

/**
 * @brief		Get packet from RX queue for IHC data
 * @param[in]	client IHC Client instance
 * @param[in]	queue RX queue number
 * @return		Pointer to SW buffer descriptor containing the packet or NULL
 * 				if the queue does not contain data
 *
 * @warning		Intended to be called for IHC client only
 */
pfe_hif_pkt_t * pfe_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	if (&chnl->ihc_client != client)
	{
		/* Only IHC client supported */
		HM_MSG_DEV_ERR(chnl->dev, "Only HIF IHC client supported\n");
		return NULL;
	}

	/* No resource protection here */
	return fifo_get(client->ihc_rx_fifo);
}

/**
 * @brief		Get TX confirmation
 * @param[in]	client Client instance
 * @param[in]	queue TX queue number
 * @return		Pointer to data associated with the transmitted buffer. See pfe_hif_drv_client_xmit_pkt()
 * 				and pfe_hif_drv_client_xmit_sg_pkt().
 * @note		Only a single thread can call this function for given client+queue
 * 				combination.
 */
void *pfe_hif_drv_client_receive_tx_conf(const pfe_hif_drv_client_t *client, uint32_t queue)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);

	if (&chnl->ihc_client != client)
	{
		/* Only IHC client supported */
		HM_MSG_DEV_ERR(chnl->dev, "Only HIF IHC client supported\n");
		return NULL;
	}

	/* No resource protection here */
	return fifo_get(client->ihc_txconf_fifo);
}

void pfeng_ihc_tx_work_handler(struct work_struct *work)
{
	struct pfeng_priv* priv = container_of(work, struct pfeng_priv, ihc_tx_work);
	struct pfeng_hif_chnl *chnl = priv->ihc_chnl;
	struct sk_buff *skb = NULL;
	dma_addr_t dma;
	int ret;

	while (kfifo_get(&priv->ihc_tx_fifo, &skb)) {

		/* Remap skb */
		dma = dma_map_single(chnl->dev, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(chnl->dev, dma))) {
			HM_MSG_DEV_ERR(chnl->dev, "No possible to map IHC frame, dropped.\n");
			goto err;
		}

		/* IHC transport requires protection */
		spin_lock_bh(&chnl->lock_tx);

		/* Check TX BD ring before sending RPC */
		if (likely(pfeng_hif_chnl_txbd_unused(chnl) >= PFE_TXBDS_NEEDED(1))) {
			pfeng_hif_chnl_txconf_put_map_frag(chnl, dma, skb_headlen(skb), skb, PFENG_MAP_PKT_IHC, 0);

			ret = pfe_hif_chnl_tx(chnl->priv, (void *)dma, skb->data, skb_headlen(skb), true);
			if (unlikely(EOK != ret)) {
				pfeng_hif_chnl_txconf_unroll_map_full(chnl, 0);
				spin_unlock_bh(&chnl->lock_tx);
				goto err;
			}

			pfeng_hif_chnl_txconf_update_wr_idx(chnl, 1);
		} else {
			/* TX BD ring is full, drop the packet */
			dma_unmap_single(chnl->dev, dma, skb_headlen(skb), DMA_TO_DEVICE);
			kfree_skb(skb);
		}

		spin_unlock_bh(&chnl->lock_tx);
	}

	return;

err:
	if(skb)
		kfree_skb(skb);

	return;
}

/**
 * @brief		Transmit IHC packet
 * @param[in]	client Client instance
 * @param[in]	queue TX queue number
 * @param[in]	sg_list Pointer to the SG list and packet metadata
 * @param[in]	idex_frame Pointer to the IHC packet with TX header
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_sg_pkt(pfe_hif_drv_client_t *client, uint32_t queue, const hif_drv_sg_list_t *const sg_list, void *idex_frame)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	struct pfeng_priv *priv = dev_get_drvdata(chnl->dev);
	uint32_t flen = sg_list->items[0].len;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
	struct sk_buff *skb;
	int ret, pktlen;

	/* Find minimal pkt size */
	if ((PFENG_TX_PKT_HEADER_SIZE + flen) < 68)
		pktlen = 68;
	else
		pktlen = PFENG_TX_PKT_HEADER_SIZE + flen;

	/* Copy packet to skb to reuse txconf standard cleaning */
	skb = alloc_skb(pktlen, GFP_ATOMIC);
	if (!skb) {
		oal_mm_free_contig(idex_frame);
		return ENOMEM;
	}

	/* Set TX header */
	tx_hdr = (pfe_ct_hif_tx_hdr_t *)skb->data;
	memset(tx_hdr, 0, PFENG_TX_PKT_HEADER_SIZE);
	tx_hdr->chid = chnl->idx;
	tx_hdr->flags |= HIF_TX_IHC | HIF_TX_INJECT;
	tx_hdr->e_phy_ifs = oal_htonl(1U << sg_list->dst_phy);
	skb_put(skb, PFENG_TX_PKT_HEADER_SIZE);

	/* Append IDEX frame */
	skb_put_data(skb, idex_frame, flen);

	/* Free original idex_frame */
	oal_mm_free_contig(idex_frame);

	/* Send data to worker */
	ret = kfifo_put(&priv->ihc_tx_fifo, skb);
	if (ret != 1) {
		HM_MSG_DEV_ERR(chnl->dev, "IHC TX kfifo full\n");
		kfree_skb(skb);
		return -ENOMEM;
	}

	queue_work(priv->ihc_wq, &priv->ihc_tx_work);

	return 0;
}

#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
