/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>
#include <linux/rtnetlink.h>
#include <linux/list.h>
#include <linux/clk.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"

#include "pfeng.h"

#define pfeng_netif_for_each_chnl(netif, chnl_idx, chnl)			\
	for (chnl_idx = 0, chnl = &netif->priv->hif_chnl[chnl_idx];		\
		chnl_idx < PFENG_PFE_HIF_CHANNELS;				\
		chnl_idx++, chnl = &netif->priv->hif_chnl[chnl_idx])

typedef struct
{
	pfe_mac_addr_t addr;		/* The MAC address */
	struct list_head iterator;	/* List chain entry */
	pfe_drv_id_t owner;		/* Identification of the driver that owns this entry */
} pfeng_netif_mac_db_list_entry_t;

static int pfeng_netif_logif_open(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_hif_chnl *chnl;
	int ret = 0, i;

#ifdef PFE_CFG_PFE_SLAVE
	if (!netif->slave_netif_inited) {
		netdev_err(netif->netdev, "SLAVE init transaction failed.\n");
		return -EINVAL;
	}
#endif /* PFE_CFG_PFE_SLAVE */

	/* Configure real RX and TX queues */
	netif_set_real_num_rx_queues(netdev, netif->cfg->hifs);
	netif_set_real_num_tx_queues(netdev, 1);

	/* start HIF channel(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if (chnl->status == PFENG_HIF_STATUS_ENABLED)
			pfeng_hif_chnl_start(chnl);

		if (chnl->status != PFENG_HIF_STATUS_RUNNING) {
			netdev_err(netif->netdev, "Invalid HIF%u (not running)\n", i);
			return -EINVAL;
		}
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Start PHY */
	if (netif->phylink) {
		ret = pfeng_phylink_start(netif);
		if (ret)
			netdev_warn(netdev, "Error starting phylink: %d\n", ret);
	}
#endif

	/* Enable EMAC logif */
	ret = pfe_log_if_enable(netif->priv->emac[netif->cfg->emac].logif_emac);
	if (ret) {
		netdev_err(netdev, "Cannot enable EMAC: %d\n", ret);
		goto err_mac_ena;
	}

#ifdef PFE_CFG_PFE_SLAVE
	netif_carrier_on(netdev);
#endif

	netif_tx_start_all_queues(netdev);

	return ret;

err_mac_ena:
	return ret;
}

/* Map TX traffic to HIF channel. Currently is used only first HIF channel for TX */
static struct pfeng_hif_chnl *pfeng_netif_map_tx_channel(struct pfeng_netif *netif, struct sk_buff *skb)
{
	u32 id = ffs(netif->cfg->hifmap);

	if (id < 1)
		return NULL;

	//TODO: id = skb_get_queue_mapping(skb);

	return &netif->priv->hif_chnl[id - 1];
}

static int pfeng_netif_logif_txack(struct pfeng_hif_chnl *chnl, int limit)
{
	unsigned int done = 0;

	while(pfe_hif_chnl_get_tx_conf(chnl->priv) == EOK) {

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Check for IHC packet first */
		if (unlikely (pfeng_hif_chnl_txconf_get_flag(chnl) == PFENG_MAP_PKT_IHC)) {
			pfe_hif_drv_client_t *client = &chnl->ihc_client;
			/* IDEX confirmation must return IDEX API compatible data */
			if (!pfe_hif_drv_ihc_put_conf(client)) {
				/* Call IHC TX callback */
				client->event_handler(client, client->priv, EVENT_TXDONE_IND, 0);
			} else {
				dev_err(chnl->dev, "TXconf IHC queuing failed.\n");
			}
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

		pfeng_hif_chnl_txconf_free_map_full(chnl);

		done++;
	}
	return done;
}

static void pfeng_netif_tx_conf(struct work_struct *work)
{
	struct pfeng_netif *netif = container_of(work, struct pfeng_netif, tx_conf_work);
	struct pfeng_hif_chnl *chnl;
	int ring_len;

	/* TODO: replace this with Tx NAPI! */
	chnl = pfeng_netif_map_tx_channel(netif, NULL);
	pfeng_netif_logif_txack(chnl, 0 /* no NAPI */);
	ring_len = pfe_hif_chnl_get_tx_fifo_depth(chnl->priv);

	if (pfe_hif_chnl_can_accept_tx_num(chnl->priv, ring_len >> 1)) {
		/* only one queue per netdev used at this point */
		netif_wake_subqueue(netif->netdev, 0);
	} else {
		schedule_work(&netif->tx_conf_work);
	}
}

static netdev_tx_t pfeng_netif_logif_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	errno_t ret = -EINVAL;
	unsigned int plen;
	u32 nfrags = skb_shinfo(skb)->nr_frags;
	dma_addr_t des = 0;
	int f, ref_num = 0, refid = -1;
	struct pfeng_hif_chnl *chnl;
	pfe_ct_hif_tx_hdr_t *tx_hdr;

	/* Get mapped HIF channel */
	chnl = pfeng_netif_map_tx_channel(netif, skb);
	if (unlikely (!chnl)) {
		net_err_ratelimited("%s: Packet dropped. Map channel failed\n", netdev->name);
		netdev->stats.tx_dropped++;
		return NETDEV_TX_BUSY;
	}
	if (unlikely (chnl->status != PFENG_HIF_STATUS_RUNNING)) {
		net_err_ratelimited("%s: Packet dropped. Channel is not in running state\n", netdev->name);
		netdev->stats.tx_dropped++;
		return NETDEV_TX_BUSY;
	}

	/* Protect shared HIF channel resource */
#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (unlikely(chnl->cl_mode == PFENG_HIF_MODE_SHARED))
#else
	if (unlikely((chnl->cl_mode == PFENG_HIF_MODE_SHARED) || chnl->ihc))
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef LOCK_TX_SPINLOCK
		spin_lock(&chnl->lock_tx);
#else
		mutex_lock(&chnl->lock_tx);
#endif

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	if (unlikely(FALSE == pfe_hif_chnl_can_accept_tx_data(chnl->priv, skb_pagelen(skb) + PFENG_TX_PKT_HEADER_SIZE))) {
		net_err_ratelimited("%s: Packet overlimited.\n", netdev->name);
		goto busy_drop;
	}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	/* Cleanup Tx HIF channel ring(s) first */
	pfeng_netif_logif_txack(chnl, 0 /* no NAPI */);

	/* Check for ring space */
	if (unlikely(!pfe_hif_chnl_can_accept_tx_num(chnl->priv, nfrags + 1))) {
		netif_stop_subqueue(netdev, skb->queue_mapping);
		/* TODO: replace this with Tx NAPI! */
		schedule_work(&netif->tx_conf_work);
		goto busy_drop;
	}

	/* Prepare headroom for TX PFE packet header */
	if (skb_headroom(skb) < PFENG_TX_PKT_HEADER_SIZE) {
		struct sk_buff *skb_new;

		skb_new = skb_realloc_headroom(skb, PFENG_TX_PKT_HEADER_SIZE);
		if (!skb_new)
			goto busy_drop;
		kfree_skb(skb);
		skb = skb_new;
	}

	skb_push(skb, PFENG_TX_PKT_HEADER_SIZE);

	plen = skb_headlen(skb);

	/* Set TX header */
	tx_hdr = (pfe_ct_hif_tx_hdr_t *)skb->data;
	memset(tx_hdr, 0, sizeof(*tx_hdr));
	tx_hdr->chid = chnl->idx;

#ifdef PFE_CFG_HIF_PRIO_CTRL
	/* Firmware will assign queue/priority */
	tx_hdr->queue = 255;
#else
	tx_hdr->queue = 0;
#endif /* PFE_CFG_HIF_PRIO_CTRL */

#ifdef PFE_CFG_ROUTE_HIF_TRAFFIC
	/* Tag the frame with ID of target physical interface */
	tx_hdr->cookie = oal_htonl(netif->cfg->emac);
#else
	tx_hdr->flags |= HIF_TX_INJECT;
	tx_hdr->e_phy_ifs = oal_htonl(1U << netif->cfg->emac);
#endif /* PFE_CFG_ROUTE_HIF_TRAFFIC */

	if (likely(netdev->features & NETIF_F_IP_CSUM))
		tx_hdr->flags |= HIF_TX_IP_CSUM | HIF_TX_TCP_CSUM | HIF_TX_UDP_CSUM;

	/* HW timestamping */
	if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
		     (netif->tshw_cfg.tx_type == HWTSTAMP_TX_ON))) {

		ref_num = pfeng_hwts_store_tx_ref(netif, skb);

		if(likely(-ENOMEM != ref_num)) {
			/* Tell stack to wait for hw timestamp */
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;

			/* Tell HW to make timestamp  with our ref_num */
			tx_hdr->flags |= HIF_TX_ETS;
			tx_hdr->refnum = htons(ref_num);
		}
		/* In error case no warning is necessary, it will come later from the worker. */
	}

	/* Fill linear part of packet */
	des = dma_map_single(netif->dev, skb->data, plen, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(netif->dev, des))) {
		net_err_ratelimited("%s: Frame mapping failed. Packet dropped.\n", netdev->name);
		goto busy_drop;
	}
	refid = pfeng_hif_chnl_txconf_put_map_frag(chnl, skb->data, des, plen, skb, PFENG_MAP_PKT_NORMAL);
	/* Increment to be able to pass number 0 */
	refid++;

	/* Software tx time stamp */
	skb_tx_timestamp(skb);

	/* Put linear part */
	ret = pfe_hif_chnl_tx(chnl->priv, (void *)des, skb->data, plen, !nfrags);
	if (unlikely(EOK != ret)) {
		net_err_ratelimited("%s: HIF channel tx failed. Packet dropped. Error %d\n", netdev->name, ret);
		pfeng_hif_chnl_txconf_unroll_map_full(chnl, refid - 1, 0);
		goto busy_drop;
	}

	/* Process frags */
	for (f = 0; f < nfrags; f++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[f];

		plen = skb_frag_size(frag);
		if (!plen) {
			nfrags--;
			continue;
		}

		des = skb_frag_dma_map(netif->dev, frag, 0, plen, DMA_TO_DEVICE);
		if (dma_mapping_error(netif->dev, des)) {
			net_err_ratelimited("%s: Fragment mapping failed. Packet dropped. Error %d\n", netdev->name, dma_mapping_error(netif->dev, des));
			pfeng_hif_chnl_txconf_unroll_map_full(chnl, refid - 1, f);
			goto busy_drop;
		}

		ret = pfe_hif_chnl_tx(chnl->priv, (void *)des, frag, plen, (f + 1) >= nfrags);
		if (unlikely(EOK != ret)) {
			net_err_ratelimited("%s: HIF channel frag tx failed. Packet dropped. Error %d\n", netdev->name, ret);
			pfeng_hif_chnl_txconf_unroll_map_full(chnl, refid - 1, f);
			goto busy_drop;
		}

		pfeng_hif_chnl_txconf_put_map_frag(chnl, frag, des, plen, NULL, PFENG_MAP_PKT_NORMAL);
	}

#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (unlikely(chnl->cl_mode == PFENG_HIF_MODE_SHARED))
#else
	if (unlikely((chnl->cl_mode == PFENG_HIF_MODE_SHARED) || chnl->ihc))
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef LOCK_TX_SPINLOCK
		spin_unlock(&chnl->lock_tx);
#else
		mutex_unlock(&chnl->lock_tx);
#endif

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += skb->len;

	return NETDEV_TX_OK;

busy_drop:
#ifndef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (unlikely(chnl->cl_mode == PFENG_HIF_MODE_SHARED))
#else
	if (unlikely((chnl->cl_mode == PFENG_HIF_MODE_SHARED) || chnl->ihc))
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef LOCK_TX_SPINLOCK
		spin_unlock(&chnl->lock_tx);
#else
		mutex_unlock(&chnl->lock_tx);
#endif

	netdev->stats.tx_dropped++;
	return NETDEV_TX_BUSY;

}

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

static void dump_idex_rawpkt(const char *label, void *data, u32 dlen)
{
#if 0
#ifdef PFE_CFG_PFE_MASTER
	const char *mode = "MASTER";
#else
	const char *mode = "SLAVE";
#endif
	const uint32_t hdrsize = sizeof(pfe_ct_hif_rx_hdr_t);
	//pfe_ct_phy_if_id_t i_phy_id = pfe_hif_pkt_get_ingress_phy_id(pkt);

	printk("IDEX-%s: %s: %*ph\n", mode, label, hdrsize, data);
	printk("IDEX-%s: %s: %*ph\n", mode, label, dlen - hdrsize, data + hdrsize);
#endif
}

void pfeng_ihc_tx_work_handler(struct work_struct *work)
{
	struct pfeng_priv* priv = container_of(work, struct pfeng_priv, ihc_tx_work);
	struct pfeng_hif_chnl *chnl = priv->ihc_chnl;
	struct pfeng_ihc_tx ihc_tx = { 0 };
	struct sk_buff *skb = NULL;
	dma_addr_t des = 0;
	int refid = -1, ret;

	/* IHC transport requires protection */
#ifdef LOCK_TX_SPINLOCK
	spin_lock(&chnl->lock_tx);
#else
	mutex_lock(&chnl->lock_tx);
#endif

	if (!kfifo_get(&priv->ihc_tx_fifo, &ihc_tx)) {
		dev_err(chnl->dev, "No IHC TX data!\n");
		goto err;
	}
	skb = ihc_tx.skb;

	/* Cleanup Tx HIF channel ring first */
	pfeng_netif_logif_txack(chnl, 0 /* no NAPI */);

	/* Remap skb */
	des = dma_map_single(chnl->dev, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(chnl->dev, des))) {
		dev_err(chnl->dev, "No possible to map IHC frame, dropped.\n");
		goto err;
	}
	refid = pfeng_hif_chnl_txconf_put_map_frag(chnl, skb->data, des, skb_headlen(skb), skb, PFENG_MAP_PKT_IHC);
	/* Increment to be able to pass number 0 */
	refid++;

	ret = pfe_hif_chnl_tx(chnl->priv, (void *)des, skb->data, skb_headlen(skb), true);
	if (unlikely(EOK != ret)) {
		pfeng_hif_chnl_txconf_unroll_map_full(chnl, refid - 1, 0);
		goto err;
	}
#ifdef LOCK_TX_SPINLOCK
	spin_unlock(&chnl->lock_tx);
#else
	mutex_unlock(&chnl->lock_tx);
#endif

	return;

err:
#ifdef LOCK_TX_SPINLOCK
	spin_unlock(&chnl->lock_tx);
#else
	mutex_unlock(&chnl->lock_tx);
#endif
	if(skb)
		kfree_skb(skb);

	return;
}

/**
 * @brief		Transmit IHC packet
 * @param[in]	client Client instance
 * @param[in]	dst Destination physical interface ID. Should by HIFs only.
 * @param[in]	queue TX queue number
 * @param[in]	idex_frame Pointer to the IHC packet with TX header
 * @return		EOK if success, error code otherwise.
 */
errno_t pfe_hif_drv_client_xmit_ihc_pkt(pfe_hif_drv_client_t *client, pfe_ct_phy_if_id_t dst, uint32_t queue, void *idex_frame, uint32_t flen)
{
	struct pfeng_hif_chnl *chnl = container_of(client, struct pfeng_hif_chnl, ihc_client);
	struct pfeng_priv *priv = (struct pfeng_priv *)dev_get_drvdata(chnl->dev);
	struct sk_buff *skb = NULL;
	int ret, pktlen;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
	struct pfeng_ihc_tx ihc_tx = { 0 };

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
	tx_hdr->e_phy_ifs = oal_htonl(1U << dst);
	skb_put(skb, PFENG_TX_PKT_HEADER_SIZE);

	/* Append IDEX frame */
	skb_put_data(skb, idex_frame, flen);
dump_idex_rawpkt("TX", skb->data, skb_headlen(skb));

	/* Free original idex_frame */
	oal_mm_free_contig(idex_frame);

	ihc_tx.chnl = chnl;
	ihc_tx.skb = skb;

	/* Send data to worker */
	ret = kfifo_put(&priv->ihc_tx_fifo, ihc_tx);
	if (ret != 1) {
		dev_err(chnl->dev, "IHC TX kfifo full\n");
		kfree_skb(skb);
		return -ENOMEM;
	}

	schedule_work(&priv->ihc_tx_work);

	return 0;
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

static int pfeng_netif_logif_stop(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	if (!netif || !netif->cfg)
		return 0;

#ifdef PFE_CFG_PFE_MASTER
	/* Stop PHY */
	if (netif->phylink)
		pfeng_phylink_stop(netif);
#endif

	netif_tx_stop_all_queues(netdev);

	return 0;
}

static int pfeng_netif_logif_change_mtu(struct net_device *netdev, int mtu)
{
	netdev->mtu = mtu;
	netdev_update_features(netdev);

	/* Note: Max packet size is not changed on PFE_EMAC */

	return 0;
}

static int pfeng_netif_logif_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	int ret = -EOPNOTSUPP;

	if (!netif_running(netdev))
		return -EINVAL;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		ret = phylink_mii_ioctl(netif->phylink, rq, cmd);
		break;
	case SIOCSHWTSTAMP:
		return pfeng_hwts_ioctl_set(netif, rq);
	case SIOCGHWTSTAMP:
		return pfeng_hwts_ioctl_get(netif, rq);
	default:
		break;
	}

	return ret;
}

static int pfeng_netif_logif_set_mac_address(struct net_device *netdev, void *p)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac];
	struct sockaddr *addr = (struct sockaddr *)p;
	int ret = 0;

	if (is_valid_ether_addr(addr->sa_data)) {
		ether_addr_copy(netdev->dev_addr, addr->sa_data);
	} else {
		netdev_warn(netdev, "No MAC address found, using random\n");
		eth_hw_addr_random(netdev);
	}

	netdev_info(netdev, "setting MAC addr: %pM\n", netdev->dev_addr);

	if (emac->logif_emac) {
		ret = pfe_log_if_flush_mac_addrs(
			emac->logif_emac,
			MAC_DB_CRIT_BY_OWNER_AND_TYPE,
			PFE_TYPE_UC,
			netif->priv->local_drv_id);

		if (ret) {
			netdev_err(netdev, "Can't delete existing unicast MAC address, new address won't be added!\n");
			return ret;
		}

		ret = pfe_log_if_add_mac_addr(emac->logif_emac, netdev->dev_addr, netif->priv->local_drv_id);
	}
	if (!ret)
		return 0;

	return -ENOSPC;
}

#ifdef PFE_CFG_PFE_MASTER
static char_t *mac_to_str(pfe_mac_addr_t addr)
{
	static char_t buf[18];

	scnprintf(buf, sizeof(buf), "%pM", &addr[0]);

	return buf;
}

/**
 * @brief		Add new MAC addresses from list of requested addresses
 * @param[in]	netdev The pointer to the network device structure
 * @param[in]	iface The interface instance
 * @param[in]	req_list List of requested addresses
 * @retval		Execution status, EOK Success, error code otherwise
 */
static int pfeng_netif_add_mac_addrs(struct net_device *netdev, pfe_log_if_t *iface, struct list_head *req_list)
{
	int ret = EOK;
	struct list_head *item;
	pfeng_netif_mac_db_list_entry_t *entry;
	pfe_mac_addr_t addr;
	pfe_ct_phy_if_id_t owner;

	/* Iterate over list of requested addresses */
	list_for_each(item, req_list) {
		entry = list_entry(item, pfeng_netif_mac_db_list_entry_t, iterator);
		(void)memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
		owner = entry->owner;

		netdev_dbg(netdev, "Adding %s to %s\n",
					mac_to_str(addr), pfe_log_if_get_name(iface));

		/* Add address into interface active list */
		ret = pfe_log_if_add_mac_addr(iface, addr, owner);
		if((ret != EOK) && (ret != ENOEXEC)) {
			netdev_warn(netdev, "unable to add %s into %s: %d\n",
						mac_to_str(addr), pfe_log_if_get_name(iface), ret);
			return ret;
		}
	}

	return EOK;
}

/**
 * @brief		Delete unused MAC addresses based on comparison of requested list with
 * 				internal MAC database according to specified rules.
 * @param[in]	netdev The pointer to the network device structure
 * @param[in]	iface The interface instance
 * @param[in]	req_list List of requested addresses
 * @param[in]	mac_db MAC database instance
 * @retval		Execution status, EOK Success, error code otherwise
 */
static int pfeng_netif_del_mac_addrs(struct net_device *netdev, pfe_log_if_t *iface, struct list_head *req_list, pfe_mac_db_t *mac_db)
{
	int ret = EOK;
	pfeng_netif_mac_db_list_entry_t *entry = NULL;
	pfe_mac_addr_t addr_act;
	struct list_head *item;
	bool entry_found;

	if((ret = pfe_mac_db_get_first_addr(mac_db, MAC_DB_CRIT_BY_TYPE, PFE_TYPE_MC, PFE_CFG_LOCAL_IF, addr_act)) != EOK)
		netdev_dbg(netdev, "get first MAC address status: %d\n", ret);

	while(ret == EOK) {
		entry_found = false;

		list_for_each(item, req_list) {
			entry = list_entry(item, pfeng_netif_mac_db_list_entry_t, iterator);
			if (!memcmp(addr_act, entry->addr, sizeof(pfe_mac_addr_t))) {
				entry_found = true;
				break;
			}
		}

		if ((!entry_found) && (pfe_emac_is_multi(addr_act))) {
			/* Del address from interface active list */
			netdev_dbg(netdev, "Removing %s from %s\n",
						mac_to_str(addr_act), pfe_log_if_get_name(iface));

			ret = pfe_log_if_del_mac_addr(iface, addr_act);
			if ((ret != EOK) && (ret != ENOENT)) {
				netdev_warn(netdev, "unable to del %s from %s: %d\n",
							mac_to_str(addr_act), pfe_log_if_get_name(iface), ret);
				return ret;
			}
		}

		ret = pfe_mac_db_get_next_addr(mac_db, addr_act);
	}

	return EOK;
}

/**
 * @brief		Add MAC address into the list of requested addresses
 * @param[in]	netdev The pointer to the network device structure
 * @param[in]	list Pointer to list of requested addresses
 * @param[in]	addr The MAC address to add
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		ENOMEM Not enough memory
 */
static int pfeng_netif_add_mac_to_request_list(struct net_device *netdev, struct list_head *list, pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	int ret = EOK;
	pfeng_netif_mac_db_list_entry_t *entry = NULL;
	struct list_head *item;
	bool is_present = false;

	list_for_each(item, list) {
		entry = list_entry(item, pfeng_netif_mac_db_list_entry_t, iterator);
		if (!memcmp(addr, entry->addr, sizeof(pfe_mac_addr_t))) {
			is_present = true;
			break;
		}
	}

	if(!is_present) {
		/* Add address to the list */
		entry = kmalloc(sizeof(pfeng_netif_mac_db_list_entry_t), GFP_KERNEL);
		if (!entry) {
			netdev_err(netdev, "Memory allocation failed\n");
			ret = ENOMEM;
		} else {
			(void)memcpy(entry->addr, addr, sizeof(pfe_mac_addr_t));
			entry->owner = owner;
			list_add_tail(&entry->iterator, list);
		}
	}

	return ret;
}

/**
 * @brief		Destroy list of requested addresses and free memory
 * @param[in]	list requested list
 * @retval		EOK Success
 * @retval		ENOEXEC Command failed
 */
static int pfeng_netif_destroy_mac_request_list(struct list_head *list)
{
	int ret = EOK;
	struct list_head *item, *aux;
	pfeng_netif_mac_db_list_entry_t *entry;

	/* Destroy list of requested addresses and free memory */
	list_for_each_safe(item, aux, list) {
		entry = list_entry(item, pfeng_netif_mac_db_list_entry_t, iterator);
		if (NULL != entry) {
			list_del_init(&entry->iterator);
			kfree(entry);
			entry = NULL;
		}
	}

	return ret;
}
#endif /* PFE_CFG_PFE_MASTER */

static void pfeng_netif_logif_set_rx_mode(struct net_device *netdev)
{
#ifdef PFE_CFG_PFE_MASTER
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_ct_phy_if_id_t hif_id = netif->priv->local_drv_id;
	pfe_log_if_t *logif_emac = netif->priv->emac[netif->cfg->emac].logif_emac;

	struct list_head req_mac_list; /* List of requested MAC addresses from stack to be written into HW */
	pfe_mac_db_t *mac_db;

	if (netdev->flags & IFF_PROMISC) {
		/* Enable promiscuous mode */
		if (pfe_log_if_promisc_enable(logif_emac) == EOK)
			netdev_dbg(netdev, "promisc enabled\n");
	} else if (netdev->flags & IFF_ALLMULTI) {
		if (pfe_log_if_allmulti_enable(logif_emac) == EOK)
			netdev_dbg(netdev, "allmulti enabled\n");
	} else if (netdev->flags & IFF_MULTICAST) {
		struct netdev_hw_addr *ha;
		INIT_LIST_HEAD(&req_mac_list);
		/* Create list of requested MAC addresses to be written into HW */
		netdev_for_each_mc_addr(ha, netdev) {
			if (EOK != pfeng_netif_add_mac_to_request_list(netdev, &req_mac_list, ha->addr, hif_id)) {
				pfeng_netif_destroy_mac_request_list(&req_mac_list);
				return;
			}
		}

		/* Check if request list is not empty */
		if (!list_empty(&req_mac_list)) {
			mac_db = pfe_log_if_get_mac_db(logif_emac);
			if (!mac_db) {
				netdev_warn(netdev, "no MAC database found\n");
			} else {
				/* Add new addresses from requested list */
				if (EOK != pfeng_netif_add_mac_addrs(netdev, logif_emac, &req_mac_list)) {
					pfeng_netif_destroy_mac_request_list(&req_mac_list);
					return;
				}

				/* Delete all addresses not present in requested list based on given criterion */
				if (EOK != pfeng_netif_del_mac_addrs(netdev, logif_emac, &req_mac_list, mac_db)) {
					pfeng_netif_destroy_mac_request_list(&req_mac_list);
					return;
				}
			}
		}

		/* Destroy requested list */
		pfeng_netif_destroy_mac_request_list(&req_mac_list);
	} else {
		/* Disable promiscuous mode */
		if (pfe_log_if_promisc_disable(logif_emac) == EOK)
			netdev_dbg(netdev, "promisc disabled\n");
		/* Disable allmulti mode */
		if (pfe_log_if_allmulti_disable(logif_emac) == EOK)
			netdev_dbg(netdev, "allmulti disabled\n");
	}
#endif
	return;
}

static const struct net_device_ops pfeng_netdev_ops = {
	.ndo_open		= pfeng_netif_logif_open,
	.ndo_start_xmit		= pfeng_netif_logif_xmit,
	.ndo_stop		= pfeng_netif_logif_stop,
	.ndo_change_mtu		= pfeng_netif_logif_change_mtu,
	.ndo_do_ioctl		= pfeng_netif_logif_ioctl,
	.ndo_set_mac_address	= pfeng_netif_logif_set_mac_address,
	.ndo_set_rx_mode	= pfeng_netif_logif_set_rx_mode,
};

static void pfeng_netif_detach_hifs(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct pfeng_hif_chnl *chnl;
	int ret = -EINVAL, i;

	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		/* Unsubscribe from HIF channel */
		if (chnl->netifs[netif->cfg->emac] != netif) {
			netdev_err(netdev, "Unknown netif registered to HIF%u\n", i);
			ret = -EINVAL;
			return;
		}
		chnl->netifs[netif->cfg->emac] = NULL;
		netdev_err(netdev, "Unsubscribe from HIF%u\n", chnl->idx);
	}
}

static int pfeng_netif_attach_hifs(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct pfeng_hif_chnl *chnl;
	int ret = -EINVAL, i;

	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if ((chnl->status != PFENG_HIF_STATUS_ENABLED) && (chnl->ihc && (chnl->status != PFENG_HIF_STATUS_RUNNING))) {
			netdev_err(netdev, "Invalid HIF%u configuration\n", i);
			ret = -EINVAL;
			goto err;
		}

		/* Subscribe to HIF channel */
		if (chnl->netifs[netif->cfg->emac]) {
			netdev_err(netdev, "Unable to register to HIF%u\n", i);
			ret = -EINVAL;
			goto err;
		}
		chnl->netifs[netif->cfg->emac] = netif;
		netdev_err(netdev, "Subscribe to HIF%u\n", chnl->idx);
	}
	ret = 0;

err:
	return ret;
}

static void pfeng_netif_logif_remove(struct pfeng_netif *netif)
{
	pfe_log_if_t *logif_emac = netif->priv->emac[netif->cfg->emac].logif_emac;

	if (!netif->netdev)
		return;

	unregister_netdev(netif->netdev); /* calls ndo_stop */

#ifdef PFE_CFG_PFE_SLAVE
	cancel_work_sync(&netif->ihc_slave_work);
#endif /* PFE_CFG_PFE_SLAVE */

#ifdef PFE_CFG_PFE_MASTER
	if (netif->phylink)
		pfeng_phylink_destroy(netif);
#endif /* PFE_CFG_PFE_MASTER */

	/* Stop EMAC logif */
	if (logif_emac) {
		pfe_log_if_disable(logif_emac);
		if (EOK != pfe_platform_unregister_log_if(netif->priv->pfe_platform, logif_emac))
			netdev_warn(netif->netdev, "Can't unregister EMAC Logif\n");
		else
			pfe_log_if_destroy(logif_emac);
		netif->priv->emac[netif->cfg->emac].logif_emac = NULL;
	}

	netdev_info(netif->netdev, "unregisted\n");

#ifdef PFE_CFG_PFE_MASTER
	pfeng_ptp_unregister(netif);
#endif /* PFE_CFG_PFE_MASTER */

	/* Release timestamp memory */
	pfeng_hwts_release(netif);

	/* Detach netif from HIF(s) */
	pfeng_netif_detach_hifs(netif);

	free_netdev(netif->netdev);
}

/**
 * @brief	Fetch necessary PFE Platform interfaces
 * @param[in]	netif Net interface instance
 * @return	0 OK
 *
 */
static int pfeng_netif_control_platform_ifs(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct pfeng_priv *priv = netif->priv;
	struct pfeng_emac *emac = &priv->emac[netif->cfg->emac];
	struct pfeng_hif_chnl *chnl;
	int ret, i;

	/* Create PFE platform-wide pool of interfaces */
	if (pfe_platform_create_ifaces(priv->pfe_platform)) {
		netdev_err(netdev, "Can't init platform interfaces\n");
		goto err;
	}

	/* Prefetch linked EMAC interfaces */
	if (!emac->phyif_emac) {
		emac->phyif_emac = pfe_platform_get_phy_if_by_id(priv->pfe_platform, netif->cfg->emac);
		if (!emac->phyif_emac) {
			netdev_err(netdev, "Could not get linked EMAC physical interface\n");
			goto err;
		}
	}
	if (!emac->logif_emac) {
		emac->logif_emac = pfe_log_if_create(emac->phyif_emac, (char *)netif->cfg->name);
		if (!emac->logif_emac) {
			netdev_err(netdev, "EMAC Logif can't be created: %s\n", netif->cfg->name);
			goto err;
		} else {
			ret = pfe_platform_register_log_if(priv->pfe_platform, emac->logif_emac);
			if (ret) {
				netdev_err(netdev, "Can't register EMAC Logif\n");
				goto err;
			}
		}
		netdev_dbg(netdev, "EMAC Logif created: %s @%px\n", netif->cfg->name, emac->logif_emac);
	}
	else
		netdev_dbg(netdev, "EMAC Logif reused: %s @%px\n", netif->cfg->name, emac->logif_emac);

	/* Make sure that EMAC ingress traffic will be forwarded to respective HIF channel */
	i = ffs(netif->cfg->hifmap) - 1;
#ifdef PFE_CFG_PFE_MASTER
	if (netif->cfg->hifs > 1)
		/* Loadbalansing requires routing to PFE_PHY_IF_ID_HIF */
		ret = pfe_log_if_set_egress_ifs(emac->logif_emac, 1 << PFE_PHY_IF_ID_HIF);
	else
		ret = pfe_log_if_set_egress_ifs(emac->logif_emac, 1 << pfeng_hif_ids[i]);
#else
	ret = pfe_log_if_add_egress_if(emac->logif_emac, pfe_platform_get_phy_if_by_id(priv->pfe_platform, pfeng_hif_ids[i]));
#endif /* PFE_CFG_PFE_MASTER */
	if (EOK != ret) {
		netdev_err(netdev, "Can't set EMAC egress interface\n");
		goto err;
	}

	/* Prefetch linked HIF(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if (!chnl->phyif_hif) {
			chnl->phyif_hif = pfe_platform_get_phy_if_by_id(priv->pfe_platform, pfeng_hif_ids[i]);
			if (!chnl->phyif_hif) {
				netdev_err(netdev, "Could not get HIF%u physical interface\n", i);
				goto err;
			}
		}

		if (netif->cfg->hifs > 1) {
#ifdef PFE_CFG_PFE_MASTER
			/* Enable loadbalance for multi-HIF config */
			ret = pfe_phy_if_loadbalance_enable(chnl->phyif_hif);
			if (EOK != ret) {
				netdev_err(netdev, "Can't set loadbalancing mode to HIF%u\n", i);
				goto err;
			} else
				netdev_info(netdev, "add HIF%u loadbalance\n", i);
#else
			netdev_warn(netdev, "Can't set loadbalancing mode to HIF%u on SLAVE instance\n", i);
#endif
		}

		ret = pfe_phy_if_enable(chnl->phyif_hif);
		if (EOK != ret) {
			netdev_err(netdev, "Can't enable HIF%u\n", i);
			goto err;
		}
		netdev_info(netdev, "Enable HIF%u\n", i);
	}

	/* Add rule for local MAC */
	if (!netif->cfg->tx_inject) {
		/* Configure the logical interface to accept frames matching local MAC address */
		ret = pfe_log_if_add_match_rule(emac->logif_emac, IF_MATCH_DMAC, (void *)netif->cfg->macaddr, 6U);
		if (EOK != ret) {
			netdev_err(netdev, "Can't add DMAC match rule\n");
			ret = -ret;
			goto err;
		}
		/* Set parent physical interface to FlexibleRouter mode */
		ret = pfe_phy_if_set_op_mode(emac->phyif_emac, IF_OP_FLEX_ROUTER);
		if (EOK != ret) {
			netdev_err(netdev, "Can't set flexrouter operation mode\n");
			ret = -ret;
			goto err;
		}
		netdev_info(netdev, "receive traffic matching its MAC address\n");
	}

	return 0;

err:
	return -EINVAL;
}

static int pfeng_netif_logif_init_second_stage(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct sockaddr saddr;
	int ret;

	/* Set PFE platform phyifs */
	ret = pfeng_netif_control_platform_ifs(netif);
	if (ret)
		goto err;

	/* Set MAC address */
	if (netif->cfg->macaddr && is_valid_ether_addr(netif->cfg->macaddr))
		memcpy(&saddr.sa_data, netif->cfg->macaddr, sizeof(saddr.sa_data));
	else
		memset(&saddr.sa_data, 0, sizeof(saddr.sa_data));
	pfeng_netif_logif_set_mac_address(netdev, (void *)&saddr);

	/* Init hw timestamp */
	ret = pfeng_hwts_init(netif);
	if (ret) {
		netdev_err(netdev, "Cannot initialize timestamping: %d\n", ret);
		goto err;
	}
#ifdef PFE_CFG_PFE_MASTER
	pfeng_ptp_register(netif);
#endif /* PFE_CFG_PFE_MASTER */

	return 0;

err:
	return ret;
}

#ifdef PFE_CFG_PFE_SLAVE
void pfeng_netif_slave_work_handler(struct work_struct *work)
{
	struct pfeng_netif* netif = container_of(work, struct pfeng_netif, ihc_slave_work);
	int ret;

	ret = pfeng_netif_logif_init_second_stage(netif);
	if (ret)
		goto err;

	netif_carrier_on(netif->netdev);

	netif->slave_netif_inited = true;
err:
	return;
}
#endif /* PFE_CFG_PFE_SLAVE */

static struct pfeng_netif *pfeng_netif_logif_create(struct pfeng_priv *priv, struct pfeng_netif_cfg *netif_cfg)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_netif *netif;
	struct net_device *netdev;
	int ret;

	if (!netif_cfg->name || !strlen(netif_cfg->name)) {
		dev_err(dev, "Interface name is missing: %s\n", netif_cfg->name);
		return NULL;
	}

	/* allocate net device with max RX and max TX queues */
	netdev = alloc_etherdev_mqs(sizeof(*netif), PFENG_PFE_HIF_CHANNELS, PFENG_PFE_HIF_CHANNELS);
	if (!netdev) {
		dev_err(dev, "Error allocating the etherdev\n");
		return NULL;
	}

	/* Set the sysfs physical device reference for the network logical device */
	SET_NETDEV_DEV(netdev, dev);

	/* set ifconfig visible config */
	netdev->mem_start = (unsigned long)priv->pfe_cfg->cbus_base;
	netdev->mem_end = priv->pfe_cfg->cbus_base + priv->pfe_cfg->cbus_len - 1;

	/* Set private structures */
	netif = netdev_priv(netdev);
	netif->dev = dev;
	netif->netdev = netdev;
	netif->priv = priv;
	netif->cfg = netif_cfg;
	netif->phylink = NULL;

	/* Set up explicit device name based on platform names */
	strlcpy(netdev->name, netif_cfg->name, IFNAMSIZ);

	netdev->netdev_ops = &pfeng_netdev_ops;

	/* MTU ranges */
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = ETH_DATA_LEN;

	/* Each packet requires extra buffer for Tx header (metadata) */
	netdev->needed_headroom = PFENG_TX_PKT_HEADER_SIZE;

#ifdef PFE_CFG_PFE_MASTER
	pfeng_ethtool_init(netdev);

	/* Add phylink */
	if (priv->emac[netif_cfg->emac].intf_mode != PHY_INTERFACE_MODE_INTERNAL)
		pfeng_phylink_create(netif);
#endif

	INIT_WORK(&netif->tx_conf_work, pfeng_netif_tx_conf);

	/* Accelerated feature */
	netdev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
	netdev->hw_features |= NETIF_F_SG;
	netdev->features = netdev->hw_features;

	ret = register_netdev(netdev);
	if (ret) {
		dev_err(dev, "Error registering the device: %d\n", ret);
		goto err_netdev_reg;
	}
	netdev_info(netdev, "registered\n");

	/* start without the RUNNING flag, phylink/idex controls it later */
	netif_carrier_off(netdev);

	/* Attach netif to HIF(s) */
	ret = pfeng_netif_attach_hifs(netif);
	if (ret)
		goto err_netdev_reg;

#ifdef PFE_CFG_PFE_SLAVE
	/*
	 * SLAVE mode init = start IHC HIF channel now
	 * and finish the rest in thread
	 * */
	if (!priv->ihc_chnl) {
		netdev_err(netdev, "IHC channel not configured.\n");
		goto err_netdev_reg;
	}
	ret = pfeng_hif_chnl_start(priv->ihc_chnl);
	if (ret) {
		netdev_err(netdev, "IHC channel not started\n");
		goto err_netdev_reg;
	}

	/* Finish device init in deffered work */
	INIT_WORK(&netif->ihc_slave_work, pfeng_netif_slave_work_handler);
	if (!queue_work(priv->ihc_slave_wq, &netif->ihc_slave_work)) {
		netdev_err(netdev, "second stage of netif init failed\n");
		goto err_netdev_reg;
	}

	return netif;
#endif /* PFE_CFG_PFE_SLAVE */

	ret = pfeng_netif_logif_init_second_stage(netif);
	if (ret)
		goto err_netdev_reg;

#ifdef PFE_CFG_PFE_MASTER
	if (netif->phylink) {
		ret = pfeng_phylink_connect_phy(netif);
		if (ret)
			netdev_err(netdev, "Error connecting to the phy: %d\n", ret);
	}
#endif /* PFE_CFG_PFE_MASTER */

	return netif;

err_netdev_reg:
	pfeng_netif_logif_remove(netif);
	return NULL;
}

void pfeng_netif_remove(struct pfeng_priv *priv)
{
	struct pfeng_netif *netif, *tmp;

	list_for_each_entry_safe(netif, tmp, &priv->netif_list, lnode)
		pfeng_netif_logif_remove(netif);
}

int pfeng_netif_create(struct pfeng_priv *priv)
{
	int ret = 0;
	struct pfeng_netif_cfg *netif_cfg, *tmp;
	struct pfeng_netif *netif;

	list_for_each_entry_safe(netif_cfg, tmp, &priv->netif_cfg_list, lnode) {
		netif = pfeng_netif_logif_create(priv, netif_cfg);
		if (netif)
			list_add_tail(&netif->lnode, &priv->netif_list);
	}

	return ret;
}

static int pfeng_netif_logif_suspend(struct pfeng_netif *netif)
{
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac];
	struct pfeng_hif_chnl *chnl;
	int i;

#ifdef PFE_CFG_PFE_MASTER
	pfeng_phylink_mac_change(netif, false);
#endif /* PFE_CFG_PFE_MASTER */

	netif_device_detach(netif->netdev);

	rtnl_lock();

	pfe_log_if_disable(emac->logif_emac);

#ifdef PFE_CFG_PFE_MASTER
	/* Stop PHY */
	if (netif_running(netif->netdev) && netif->phylink)
		pfeng_phylink_stop(netif);

	/* Stop RX/TX EMAC clocks */
	if (emac->tx_clk)
		clk_disable_unprepare(emac->tx_clk);
	if (emac->rx_clk)
		clk_disable_unprepare(emac->rx_clk);
#endif /* PFE_CFG_PFE_MASTER */

	rtnl_unlock();

	/* Reset attached HIF PhyIfs */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Skip in case of IHC channel */
		if (!chnl->ihc)
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
			chnl->phyif_hif = NULL;
	}

	/* Reset linked EMAC IFs */
	emac->phyif_emac = NULL;
	emac->logif_emac = NULL;

	return 0;
}

static int pfeng_netif_logif_resume(struct pfeng_netif *netif)
{
	struct pfeng_priv *priv = netif->priv;
	__maybe_unused struct device *dev = &priv->pdev->dev;
	struct net_device *netdev = netif->netdev;
	struct pfeng_emac *emac = &netif->priv->emac[netif->cfg->emac];
	struct pfeng_hif_chnl *chnl;
	__maybe_unused u64 clk_rate;
	int ret, i;

	rtnl_lock();

#ifdef PFE_CFG_PFE_MASTER
	/* Restart RX/TX EMAC clocks */

	/* retrieve max rate */
	switch (emac->max_speed) {
	case SPEED_10:
		clk_rate = 2500000;
		break;
	case SPEED_100:
		clk_rate = 25000000;
		break;
	case SPEED_1000:
	default:
		clk_rate = 125000000;
		break;
	}

	if (emac->tx_clk) {
		ret = clk_set_rate(emac->tx_clk, clk_rate);
		if (ret)
			dev_err(dev, "Failed to set TX clock on EMAC%d: %d\n", netif->cfg->emac, ret);
		else {
			ret = clk_prepare_enable(emac->tx_clk);
			if (ret) {
				dev_err(dev, "TX clocks restart on EMAC%d failed: %d\n", netif->cfg->emac, ret);
				ret = 0;
				devm_clk_put(dev, emac->tx_clk);
				emac->tx_clk = NULL;
			} else
				dev_info(dev, "TX clocks on EMAC%d restarted\n", netif->cfg->emac);
		}
	}

	if (emac->rx_clk) {
		ret = clk_set_rate(emac->rx_clk, clk_rate);
		if (ret)
			dev_err(dev, "Failed to set RX clock on EMAC%d: %d\n", netif->cfg->emac, ret);
		else {
			ret = clk_prepare_enable(emac->rx_clk);
			if (ret) {
				dev_err(dev, "RX clocks restart on EMAC%d failed: %d\n", netif->cfg->emac, ret);
				ret = 0;
				devm_clk_put(dev, emac->rx_clk);
				emac->rx_clk = NULL;
			} else
				dev_info(dev, "RX clocks on EMAC%d restarted\n", netif->cfg->emac);
		}
	}
#endif /* PFE_CFG_PFE_MASTER */

	ret = pfeng_netif_logif_init_second_stage(netif);

	/* start HIF channel(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if (chnl->status == PFENG_HIF_STATUS_ENABLED)
			pfeng_hif_chnl_start(chnl);

		if (chnl->status != PFENG_HIF_STATUS_RUNNING)
			netdev_warn(netif->netdev, "HIF%u in invalid state: not running\n", i);
	}

	/* Enable EMAC logif */
	ret = pfe_log_if_enable(emac->logif_emac);
	if (ret)
		netdev_warn(netdev, "Cannot enable EMAC: %d\n", ret);

#ifdef PFE_CFG_PFE_SLAVE
	netif_carrier_on(netdev);
#endif

#ifdef PFE_CFG_PFE_MASTER
	if (netif_running(netif->netdev) && netif->phylink) {
		ret = pfeng_phylink_start(netif);
		if (ret)
			netdev_err(netdev, "Error starting phy: %d\n", ret);

		pfeng_phylink_mac_change(netif, true);
	}
#endif /* PFE_CFG_PFE_MASTER */

	rtnl_unlock();

	netif_device_attach(netdev);

	return ret;
}

int pfeng_netif_suspend(struct pfeng_priv *priv)
{
	struct pfeng_netif *netif, *tmp;


	list_for_each_entry_safe(netif, tmp, &priv->netif_list, lnode)
		pfeng_netif_logif_suspend(netif);

	return 0;
}

int pfeng_netif_resume(struct pfeng_priv *priv)
{
	struct pfeng_netif *netif, *tmp;

	list_for_each_entry_safe(netif, tmp, &priv->netif_list, lnode)
		pfeng_netif_logif_resume(netif);

	return 0;
}
