/*
 * 2020 NXP
 *
 * SPDX-License-Identifier: BSD OR GPL-2.0
 *
 */

#include <linux/phylink.h>

#include "pfeng.h"

/* sanity check: we need RX buffering internal support disabled */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
#error "Invalid PFE HIF channel mode"
#endif

/* The dma mapping info, embedded to the TX skbuff used in RXCONF cleanup */
struct pfeng_qdesc {
	dma_addr_t			map;
	u32				len;
};

static int pfeng_logif_set_mac_address(struct net_device *netdev, void *p)
{
	struct pfeng_ndev *ndata = netdev_priv(netdev);
	struct sockaddr *addr = (struct sockaddr *)p;
	int ret;

	if (is_valid_ether_addr(addr->sa_data)) {
		ether_addr_copy(netdev->dev_addr, addr->sa_data);
	} else {
		netdev_warn(netdev, "No MAC address found, using random\n");
		eth_hw_addr_random(netdev);
	}

	netdev_info(netdev, "setting MAC addr: %pM\n", netdev->dev_addr);

	if (!ndata->logif_emac)
		return 0;

	ret = pfe_log_if_set_mac_addr(ndata->logif_emac, netdev->dev_addr);
	if (!ret)
		return 0;

	return -ENOSPC;
}

static void pfeng_hif_client_remove(struct pfeng_ndev *ndev)
{

	/* EMAC */
	if(ndev->logif_emac) {
		if (EOK != pfe_platform_unregister_log_if(ndev->priv->pfe, ndev->logif_emac))
			netdev_warn(ndev->netdev, "Can't unregister EMAC Logif\n");
		else
			pfe_log_if_destroy(ndev->logif_emac);
		ndev->logif_emac = NULL;
	}
	ndev->phyif_emac = NULL; /* Don't destroy, just forget */

	if(ndev->client) {
		pfe_hif_drv_client_unregister(ndev->client);
		ndev->client = NULL;
	}

	/* Uninstall HIF SC channel */
	if (ndev->chnl_sc.drv)
		pfeng_hif_chnl_drv_remove(ndev);
}

/**
 * @brief		HIF client event handler
 * @details		Called by HIF when client-related event happens (packet received, packet
 * 				transmitted).
 */
static int pfeng_hif_event_handler(pfe_hif_drv_client_t *client, void *data, uint32_t event, uint32_t qno)
{
	struct pfeng_ndev *ndev = (struct pfeng_ndev *)data;

	if (event == EVENT_RX_PKT_IND) {

		if(napi_schedule_prep(&ndev->napi)) {

			pfe_hif_chnl_rx_irq_mask(ndev->chnl_sc.priv);

			__napi_schedule_irqoff(&ndev->napi);
		} else
			ndev->xstats.napi_poll_onrun++;
	}

	return 0;
}

static int pfeng_hif_client_add(struct pfeng_ndev *ndev)
{
	int ret = 0;
	struct sockaddr saddr;

	if (ndev->eth->hif_chnl_sc >= HIF_CFG_MAX_CHANNELS) {
		netdev_err(ndev->netdev, "Unsupported channel index: %u\n", ndev->eth->hif_chnl_sc);
		return -ENODEV;
	}

	/* Create SC HIF channel */
	ret = pfeng_hif_chnl_drv_create(ndev);
	if (ret)
		return ret;

	/* Create bman for channel */
	if (!ndev->bman.rx_pool) {
		ndev->bman.rx_pool = pfeng_bman_pool_create(ndev->chnl_sc.priv, ndev->dev);
		if (!ndev->bman.rx_pool) {
			netdev_err(ndev->netdev, "Unable to attach bman\n");
			return -ENODEV;
		}
		/* Fill by prebuilt RX skbuf */
		pfeng_hif_chnl_fill_rx_buffers(ndev->chnl_sc.priv, ndev);
	}

	/* Connect to HIF */
	ndev->client = pfe_hif_drv_client_register(
				ndev->chnl_sc.drv,		/* HIF Driver instance */
				HIF_CLIENTS_MAX,		/* Client ID - unused for now */
				1,				/* TX Queue Count */
				1,				/* RX Queue Count */
				PFE_HIF_RING_CFG_LENGTH,	/* TX Queue Depth */
				PFE_HIF_RING_CFG_LENGTH,	/* RX Queue Depth */
				&pfeng_hif_event_handler,	/* Client's event handler */
				(void *)ndev);			/* Meta data */

	if (!ndev->client) {
		netdev_err(ndev->netdev, "Unable to register HIF client: %s\n", ndev->eth->name);
		return -ENODEV;
	}

#ifdef PFE_CFG_PFE_SLAVE
	/* start HIF channel driver */
	napi_enable(&ndev->napi);
	pfe_hif_drv_start(ndev->chnl_sc.drv);
#endif

	/* 
	 * Create platform-wide pool of interfaces. Must be done here where HIF channel
	 * is already initialized to allow slave driver create the instances via IDEX.
	 */
	if (pfe_platform_create_ifaces(ndev->priv->pfe)) {
		netdev_err(ndev->netdev, "Can't init platform interfaces\n");
		return -ENODEV;
	}

	/*	Get EMAC physical interface */
	ndev->phyif_emac = pfe_platform_get_phy_if_by_id(ndev->priv->pfe, ndev->eth->emac_id);
	if (NULL == ndev->phyif_emac) {
		netdev_err(ndev->netdev, "Could not get EMAC physical interface\n");
		return -ENODEV;
	}

	/*	Create EMAC logical interface */
	ndev->logif_emac = pfe_log_if_create(ndev->phyif_emac, (char *)ndev->eth->name);
	if (!ndev->logif_emac) {
		netdev_err(ndev->netdev, "EMAC Logif doesn't exist: %s\n", ndev->eth->name);
			return -ENODEV;
	} else {
		ret = pfe_platform_register_log_if(ndev->priv->pfe, ndev->logif_emac);
		if (ret) {
			netdev_err(ndev->netdev, "Can't register EMAC Logif\n");
			goto err;
		}
	}

	/* Set MAC address */
	if (ndev->eth->addr && is_valid_ether_addr(ndev->eth->addr))
		memcpy(&saddr.sa_data, ndev->eth->addr, sizeof(saddr.sa_data));
	else
		memset(&saddr.sa_data, 0, sizeof(saddr.sa_data));
	pfeng_logif_set_mac_address(ndev->netdev, (void *)&saddr);

	/* Add debugfs entry for HIF channel */
	pfeng_debugfs_add_hif_chnl(ndev->priv, ndev);


	if (EOK != pfe_hif_drv_client_set_inject_if(ndev->client,
					pfe_phy_if_get_id(ndev->phyif_emac)))
	{
		netdev_err(ndev->netdev, "Can't set inject interface\n");
		goto err;
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Send packets received via 'log_if' to exclusively associated HIF channel */
	ret = pfe_log_if_set_egress_ifs(ndev->logif_emac, 1 << pfeng_hif_ids[ndev->eth->hif_chnl_sc]);
	if (EOK != ret) {
		netdev_err(ndev->netdev, "Can't set egress interface %s\n", pfe_log_if_get_name(ndev->logif_emac));
		ret = -ret;
		goto err;
	}
#elif PFE_CFG_PFE_SLAVE
	/* Make sure that EMAC ingress traffic will be forwarded to respective HIF channel */
	ret = pfe_log_if_add_egress_if(ndev->logif_emac, pfe_platform_get_phy_if_by_id(ndev->priv->pfe, pfeng_hif_ids[ndev->eth->hif_chnl_sc]));
	if (EOK != ret) {
		netdev_err(ndev->netdev, "Can't set egress interface %s\n", pfe_log_if_get_name(ndev->logif_emac));
		ret = -ret;
		goto err;
	}
	/* Configure the logical interface to accept frames matching local MAC address */
	ret = pfe_log_if_add_match_rule(ndev->logif_emac, IF_MATCH_DMAC, (void *)ndev->netdev->dev_addr, 6U);
	if (EOK != ret) {
		netdev_err(ndev->netdev, "Can't add match rule for %s\n", pfe_log_if_get_name(ndev->logif_emac));
		ret = -ret;
		goto err;
	}
	/* Set parent physical interface to FlexibleRouter mode */
	ret = pfe_phy_if_set_op_mode(ndev->phyif_emac, IF_OP_FLEX_ROUTER);
	if (EOK != ret) {
		netdev_err(ndev->netdev, "Can't set operation mode for %s\n", pfe_log_if_get_name(ndev->logif_emac));
		ret = -ret;
		goto err;
	}
	netdev_info(ndev->netdev, "receive traffic matching its MAC address\n");
#endif

	netdev_info(ndev->netdev, "Register HIF client %s on logif %d\n", ndev->eth->name, pfe_log_if_get_id(ndev->logif_emac));

	return 0;

err:
	pfeng_hif_client_remove(ndev);

	/* convert to linux style */
	if (ret > 0)
		ret = -ret;

	return ret;
}

/**
 *  pfeng_release - close entry point of the driver
 *  @dev : device pointer.
 *  Description:
 *  This is the stop entry point of the driver.
 */
static int pfeng_logif_release(struct net_device *netdev)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	netdev_info(netdev, "%s\n", __func__);

	netif_tx_stop_queue(netdev_get_tx_queue(netdev, 0));

	/* Stop log if */
	pfe_log_if_disable(ndev->logif_emac);

#ifdef PFE_CFG_PFE_MASTER
	/* stop napi */
	napi_disable(&ndev->napi);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	/*
	 * Note: phylink_stop() causes backtraces on 5.4
	 *       but worked correctly on 4.19
	 */
	/* stop phylink */
	if (ndev->phylink)
		pfeng_phylink_stop(ndev);

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0) */

	pfe_hif_drv_stop(ndev->chnl_sc.drv);
#else
	netif_carrier_off(netdev);
#endif /* PFE_CFG_PFE_MASTER */

	return 0;
}

/**
 *  pfeng_open - open entry point of the driver
 *  @dev : pointer to the device structure.
 *  Description:
 *  This function is the open entry point of the driver.
 *  Return value:
 *  0 on success and an appropriate integer as defined in errno.h
 *  file on failure.
 */
static int pfeng_logif_open(struct net_device *netdev)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
	int ret;

	netdev_dbg(netdev, "%s: %s\n", __func__, ndev ? ndev->eth->name : "???");

	if(!ndev) {
		netdev_err(netdev, "Cannot init NAPI. NO <ndata>\n");
		return -ENODEV;
	}

	/* clear xstats */
	ndev->xstats.napi_poll = 0;
	ndev->xstats.napi_poll_onrun = 0;
	ndev->xstats.napi_poll_resched = 0;
	ndev->xstats.napi_poll_completed = 0;
	ndev->xstats.napi_poll_rx = 0;
	ndev->xstats.txconf_loop = 0;
	ndev->xstats.txconf = 0;
	ndev->xstats.tx_busy = 0;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	ndev->xstats.ihc_rx = 0;
	ndev->xstats.ihc_tx = 0;
#endif

#ifdef PFE_CFG_PFE_MASTER
	/* start HIF channel driver */
	pfe_hif_drv_start(ndev->chnl_sc.drv);

	/* start phylink */
	if (ndev->phylink) {
		if(!(ndev->opts & PFENG_LOGIF_OPTS_PHY_CONNECTED)) {
			ret = pfeng_phylink_connect_phy(ndev);
			if (ret)
				netdev_err(netdev, "Error connecting to the phy: %d\n", ret);
			else
				ndev->opts |= PFENG_LOGIF_OPTS_PHY_CONNECTED;
		}
		if((ndev->opts & PFENG_LOGIF_OPTS_PHY_CONNECTED)) {
			ret = pfeng_phylink_start(ndev);
			if (ret)
				netdev_warn(netdev, "Error starting phylink: %d\n", ret);
		}
	}
#endif

	/* Enable EMAC logif */
	ret = pfe_log_if_enable(ndev->logif_emac);
	if (ret) {
		netdev_err(netdev, "Cannot enable EMAC: %d\n", ret);
		goto err_mac_ena;
	}

#ifdef PFE_CFG_PFE_MASTER
	napi_enable(&ndev->napi);
#elif PFE_CFG_PFE_SLAVE
	netif_carrier_on(netdev);
#endif

	netif_tx_start_queue(netdev_get_tx_queue(netdev, 0));

	return ret;

err_mac_ena:

	return ret;
}

static int pfeng_napi_txack(struct pfeng_ndev *ndev, int limit)
{
	unsigned int done = 0;
	void *ref;

	while((ref = pfe_hif_drv_client_receive_tx_conf(ndev->client, 0))) {
		/* release skbuf of TX packet */
		struct sk_buff *skb = (struct sk_buff *)ref;
		struct pfeng_qdesc *qdesc = (struct pfeng_qdesc *)&skb->cb;

		dma_unmap_single(ndev->dev, qdesc->map, qdesc->len, DMA_TO_DEVICE);
		dev_consume_skb_any(skb);

		done++;
	}
	ndev->xstats.txconf += done;

	if (likely(done))
		ndev->xstats.txconf_loop++;

	return done;
}

/**
 *  pfeng_xmit - Tx entry point of the driver
 *  @skb : the socket buffer
 *  @dev : device pointer
 *  Description : this is the tx entry point of the driver.
 *  It programs the chain or the ring [TODO: and supports oversized frames
 *  and SG feature].
 */
static netdev_tx_t pfeng_logif_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	hif_drv_sg_list_t sg_list = { 0 };
	errno_t ret = -EINVAL;
	unsigned int plen = skb_headlen(skb);
	u32 nfrags = skb_shinfo(skb)->nr_frags;
	dma_addr_t des = 0;
	struct pfeng_qdesc *qdesc = (struct pfeng_qdesc *)&skb->cb;

	qdesc->map = 0;

	/* Cleanup Tx ring first */
	pfeng_napi_txack(ndev, 0 /* no NAPI */);

	/* Not supporting S/G yet, so max value can be 1+1 */
	if ((nfrags + 1) >= 2 /*HIF_MAX_SG_LIST_LENGTH*/) {
		netdev_err(netdev, "So big frame for xmit. Frame dropped.\n");
		goto pkt_drop;
	}

	/* Fill first part of packet */
	des = dma_map_single(ndev->dev, skb->data, plen, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(ndev->dev, des))) {
		netdev_err(netdev, "No possible to map frame, dropped.\n");
		goto pkt_drop;
	}

	sg_list.items[0].data_pa = (void *)des;
	sg_list.items[0].data_va = skb->data;
	sg_list.items[0].len = plen;
	sg_list.size = 1;
	qdesc->map = des;
	qdesc->len = plen;

#if 0
	// TODO: S/G
	for (i = 0; i < nfrags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
                //bool last_segment = (i == (nfrags - 1));

                plen = skb_frag_size(frag);
		des = skb_frag_dma_map(ndev->dev, frag, 0, plen, DMA_TO_DEVICE);
		if (dma_mapping_error(ndev->dev, des)) {
			netdev_err(netdev, "No possible to map frame, dropped.\n");
			goto pkt_drop;
		}
		sg_list->items[i].data_pa = (void *)des;
		sg_list->items[i].data_va = frag;
		sg_list->items[i].len = plen;
		sg_list->size++;

	}
#endif

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	sg_list.total_bytes += skb->len;
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	ret = pfe_hif_drv_client_xmit_sg_pkt(ndev->client, 0, &sg_list, skb);

	if (unlikely(EOK != ret)) {
		ndev->xstats.tx_busy++;
		ret = NETDEV_TX_BUSY;
		goto pkt_busy;
	}

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += skb->len;

	return NETDEV_TX_OK;

pkt_busy:
	/* the same like drop but not free skbuf */
pkt_drop:
	if (qdesc->map) {
		dma_unmap_single(ndev->dev, qdesc->map, qdesc->len, DMA_TO_DEVICE);
		qdesc->map = 0;
	}

	if (ret == NETDEV_TX_BUSY)
		return NETDEV_TX_BUSY;

	netdev_info(netdev, "Packet dropped (skb len=0x%x)\n", skb->len);
	dev_kfree_skb_any(skb);
	netdev->stats.tx_dropped++;

	return NET_XMIT_DROP;
}

static int pfeng_napi_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);
	int ret = -EOPNOTSUPP;

	if (!netif_running(netdev))
		return -EINVAL;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		ret = phylink_mii_ioctl(ndev->phylink, rq, cmd);
		break;
	default:
		break;
	}

	return ret;
}

static int pfeng_napi_change_mtu(struct net_device *netdev, int mtu)
{
#ifdef PFE_CFG_PFE_MASTER
	struct pfeng_ndev *ndev = netdev_priv(netdev);
	pfe_emac_t *emac = ndev->priv->pfe->emac[ndev->eth->emac_id];
#endif

	netdev_info(netdev, "%s: mtu change to %d\n", __func__, mtu);

	if (mtu < (ETH_ZLEN - ETH_HLEN) || mtu > (SKB_MAX_HEAD(NET_SKB_PAD + NET_IP_ALIGN))) {
		netdev_err(netdev, "Error: Invalid MTU value requested: %d\n", mtu);
		return -EINVAL;
	}

	if (netif_running(netdev)) {
		netdev_err(netdev, "Error: Must be stopped to change its MTU\n");
		return -EBUSY;
	}

#ifdef PFE_CFG_PFE_MASTER
	if (pfe_emac_set_max_frame_length(emac, mtu) != EOK) {
		netdev_err(netdev, "Error: Invalid MTU value requested: %d\n", mtu);
		return -EINVAL;
	}
#endif

	netdev->mtu = mtu;

	netdev_update_features(netdev);

	return 0;
}

static void pfeng_logif_set_rx_mode(struct net_device *netdev)
{
	struct pfeng_ndev *ndev = netdev_priv(netdev);

	if (netdev->flags & IFF_PROMISC) {
		/* Enable promiscuous mode */
		if (pfe_log_if_promisc_enable(ndev->logif_emac) == EOK)
			netdev_dbg(netdev, "promisc enabled\n");
	} else {
		/* Disable promiscuous mode */
		if (pfe_log_if_promisc_disable(ndev->logif_emac) == EOK)
			netdev_dbg(netdev, "promisc disabled\n");
	}
}

static const struct net_device_ops pfeng_netdev_ops = {
	.ndo_open		= pfeng_logif_open,
	.ndo_start_xmit		= pfeng_logif_xmit,
	.ndo_stop		= pfeng_logif_release,
	.ndo_change_mtu		= pfeng_napi_change_mtu,
	.ndo_do_ioctl		= pfeng_napi_ioctl,
	.ndo_set_mac_address	= pfeng_logif_set_mac_address,
	.ndo_set_rx_mode	= pfeng_logif_set_rx_mode,
};

static struct sk_buff *pfeng_hif_rx_get(struct pfeng_ndev *ndev, unsigned int *ihc_processed)
{
	struct sk_buff *skb;
	pfe_ct_hif_rx_hdr_t *hif_hdr;

	if(!ndev->client)
		return NULL;

	while(1) {

		skb = pfeng_hif_drv_client_receive_pkt(ndev->client, 0);
		if (unlikely(!skb))
			/* no more packets */
			return NULL;

		hif_hdr = (pfe_ct_hif_rx_hdr_t *)skb->data;
		hif_hdr->flags = (pfe_ct_hif_rx_flags_t)oal_ntohs(hif_hdr->flags);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Check for IHC frame */
		if (unlikely (hif_hdr->flags & HIF_RX_IHC)) {
			(*ihc_processed)++;

			/* IHC client, deffer work */
			if (!pfe_hif_drv_ihc_put_pkt(ndev->chnl_sc.drv, skb->data, skb->len, skb)) {
				ndev->priv->q_elems++;
				wake_up_interruptible(&ndev->priv->q_ihc);
			} else {
				netdev_err(ndev->netdev, "RX IHC queuing failed. Origin phyif %d\n", hif_hdr->i_phy_if);
				kfree_skb(skb);
			}

			continue;
		}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

		/* Skip HIF header */
		skb_pull(skb, 16);

		return skb;
	}

	return NULL;
}

/**
 * pfeng_rx - manage the receive process
 * @priv: driver private structure
 * @limit: napi budget
 * @queue: RX client id
 * Description :  this the function called by the napi poll method.
 * It gets all the frames inside the ring.
 */
static int pfeng_napi_rx(struct pfeng_ndev *ndev, int limit)
{
	struct net_device *netdev = ndev->netdev;
	unsigned int done = 0;
	unsigned int ihc_processed = 0;
	struct sk_buff *skb;

	while((skb = pfeng_hif_rx_get(ndev, &ihc_processed))) {

		if (likely(netdev->features & NETIF_F_RXCSUM)) {
			/* we have only OK info, signal it */
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		}

		/* Pass to upper layer */
		skb->protocol = eth_type_trans(skb, netdev);

		if (unlikely(skb->ip_summed == CHECKSUM_NONE))
			netif_receive_skb(skb);
		else
			napi_gro_receive(&ndev->napi, skb);

		netdev->stats.rx_packets++;
		netdev->stats.rx_bytes += skb_headlen(skb);

		pfeng_hif_chnl_refill_rx_buffer(ndev->chnl_sc.priv, ndev);

		done++;
		if(unlikely(done == limit))
			break;
	}

	if (likely(done))
		ndev->xstats.napi_poll_rx++;

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	if (likely(ihc_processed))
		ndev->xstats.ihc_rx += ihc_processed;

	return done + ihc_processed;
#else
	return done;
#endif
}

/**
 *  pfeng_poll - pfeng poll method (NAPI)
 *  @napi : pointer to the napi structure.
 *  @budget : maximum number of packets that the current CPU can receive from
 *        all interfaces.
 *  Description :
 *  To look at the incoming frames and clear the tx resources.
 */
static int pfeng_napi_poll(struct napi_struct *napi, int budget)
{
	struct pfeng_ndev *ndev = container_of(napi, struct pfeng_ndev, napi);
	int done = 0;

	/* Consume RX pkt(s) */
	done = pfeng_napi_rx(ndev, budget);

	ndev->xstats.napi_poll++;

	if (done < budget && napi_complete_done(napi, done)) {
		ndev->xstats.napi_poll_completed++;

		if (done) {
			/* We might have more RX packets in queue */
			napi_reschedule(napi);
			ndev->xstats.napi_poll_resched++;
		} else {
			/* Indicate end of RX event (required for SC mode, empty for MC) */
			pfe_hif_drv_client_rx_done(ndev->client);
		}
	}

	return done;
}

struct pfeng_ndev *pfeng_napi_if_create(struct pfeng_priv *priv, struct pfeng_eth *eth)
{
	struct device *dev = &priv->pdev->dev;
	struct net_device *netdev;
	struct pfeng_ndev *ndev;
	int ret;

	if (!eth->name || !strlen(eth->name)) {
		dev_err(dev, "Interface name is missing: %s\n", eth->name);
		return NULL;
	}

	/* allocate net device with one RX and one TX queue */
	netdev = alloc_etherdev_mqs(sizeof(*ndev), 1, 1);
	if (!netdev) {
		dev_err(dev, "Error allocating the etherdev\n");
		return NULL;
	}

	/* Set the sysfs physical device reference for the network logical device */
	SET_NETDEV_DEV(netdev, dev);

	/* set ifconfig visible config */
	netdev->mem_start = (unsigned long)priv->cfg->cbus_base;
	netdev->mem_end = priv->cfg->cbus_base + priv->cfg->cbus_len;

	/* Set private structures */
	ndev = netdev_priv(netdev);
	ndev->dev = dev;
	ndev->netdev = netdev;
	ndev->priv = priv;
	ndev->eth = eth;
	ndev->client = NULL;
	ndev->emac_regs = NULL;
	ndev->emac_speed = 0;
	ndev->phylink = NULL;

	/* Set netdev IRQ */
	netdev->irq = priv->cfg->irq_vector_hif_chnls[eth->hif_chnl_sc];

	/* Configure real RX and TX queues */
	netif_set_real_num_rx_queues(netdev, 1);
	netif_set_real_num_tx_queues(netdev, 1);

	/* Set up explicit device name based on platform names */
	strlcpy(netdev->name, eth->name, IFNAMSIZ);

	netdev->netdev_ops = &pfeng_netdev_ops;

	/* MTU ranges */
	netdev->min_mtu = ETH_ZLEN - ETH_HLEN;

#ifdef PFE_CFG_PFE_MASTER
	pfeng_ethtool_init(netdev);

	/* Add phylink */
	if (eth->intf_mode != PHY_INTERFACE_MODE_INTERNAL)
		pfeng_phylink_create(ndev);
#endif

	/* Accelerated feature */
	netdev->hw_features |= /*NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |*/ NETIF_F_RXCSUM;
	netdev->features = netdev->hw_features;
	//netdev->hw_features |=NETIF_F_SG;

	netif_napi_add(netdev, &ndev->napi, pfeng_napi_poll, NAPI_POLL_WEIGHT);

	ret = register_netdev(netdev);
	if (ret) {
		dev_err(dev, "Error registering the device: %d\n", ret);
		goto err_ndev_reg;
	}
	netdev_info(netdev, "registered\n");

	/* start without the RUNNING flag, phylink controls it later */
	netif_carrier_off(netdev);

	/* attach to the hif channel */
	ret = pfeng_hif_client_add(ndev);
	if (ret) {
		netdev_err(netdev, "Cannot add HIF client: %d)\n", ret);
		goto err_ndev_reg;
	}

	return ndev;

err_ndev_reg:
	pfeng_napi_if_release(ndev);
	return NULL;
}

void pfeng_napi_if_release(struct pfeng_ndev *ndev)
{
	if (!ndev)
		return;

	netdev_info(ndev->netdev, "unregisted\n");

	unregister_netdev(ndev->netdev); /* calls ndo_stop */

	/* Remove HIF client */
	pfeng_hif_client_remove(ndev);

	/* Detach Bman */
	if (ndev->bman.rx_pool) {
		pfeng_bman_pool_destroy(ndev->bman.rx_pool);
		ndev->bman.rx_pool = NULL;
	}

#ifdef PFE_CFG_PFE_MASTER
	if (ndev->phylink)
		pfeng_phylink_destroy(ndev);
#endif

	netif_napi_del(&ndev->napi);
	free_netdev(ndev->netdev);
}
