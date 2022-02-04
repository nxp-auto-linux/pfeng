/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>
#include <linux/rtnetlink.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/if_vlan.h>

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

static struct pfeng_emac *pfeng_netif_get_emac(struct pfeng_netif *netif)
{
	if (netif->cfg->aux)
		return NULL;

	return &netif->priv->emac[netif->cfg->emac];
}

static pfe_log_if_t *pfeng_netif_get_emac_logif(struct pfeng_netif *netif)
{
	if (!pfeng_netif_get_emac(netif))
		return NULL;

	return pfeng_netif_get_emac(netif)->logif_emac;
}

static pfe_phy_if_t *pfeng_netif_get_emac_phyif(struct pfeng_netif *netif)
{
	if (!pfeng_netif_get_emac(netif))
		return NULL;

	return pfeng_netif_get_emac(netif)->phyif_emac;
}

static u8 *__mac_to_str(const u8 *addr, u8 *buf, int len)
{
	scnprintf(buf, len, "%pM", addr);

	return buf;
}

#define mac_to_str(addr, buf) __mac_to_str((addr), (buf), sizeof(buf))

static int pfeng_uc_list_sync(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);
	struct netdev_hw_addr *ha;
	errno_t ret;
	u8 buf[18];

	if (!phyif_emac)
		return 0;

	ret = pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					 PFE_TYPE_UC, netif->priv->local_drv_id);
	if (ret != EOK) {
		netdev_err(netdev, "failed to flush multicast MAC addresses\n");
		return -ret;
	}

	ret = pfe_phy_if_add_mac_addr(phyif_emac, netdev->dev_addr,
				      netif->priv->local_drv_id);
	if (ret != EOK) {
		netdev_err(netdev, "failed to add %s to %s: %d\n",
			   mac_to_str(netdev->dev_addr, buf),
			   pfe_phy_if_get_name(phyif_emac), ret);
		return -ret;
	}

	netdev_for_each_uc_addr(ha, netdev) {
		if (!is_unicast_ether_addr(ha->addr))
			continue;

		ret = pfe_phy_if_add_mac_addr(phyif_emac, ha->addr, netif->priv->local_drv_id);
		if (ret != EOK)
			netdev_warn(netdev, "failed to add %s to %s: %d\n",
				    mac_to_str(ha->addr, buf), pfe_phy_if_get_name(phyif_emac), ret);
	}

	return -ret;
}

static int pfeng_netif_logif_open(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_hif_chnl *chnl;
	int ret = 0, i;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(netif->dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

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

		if (!netif->cfg->tx_inject) {
			/* PFENG_LOGIF_MODE_TX_CLASS mode requires logIf config */
			if (!pfe_log_if_is_enabled(chnl->logif_hif)) {
				ret = pfe_log_if_enable(chnl->logif_hif);
				if (ret)
					netdev_warn(netdev, "Cannot enable logif HIF%i: %d\n", i, ret);
			} else
				netdev_info(netdev, "Logif HIF%i already enabled\n", i);

			if (!pfe_log_if_is_promisc(chnl->logif_hif)) {
				ret = pfe_log_if_promisc_enable(chnl->logif_hif);
				if (ret)
					netdev_warn(netdev, "Cannot set promisc mode for logif HIF%i: %d\n", i, ret);
			} else
				netdev_dbg(netdev, "Logif HIF%i already in promisc mode\n", i);
		}
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Start PHY */
	if (netif->phylink) {
		ret = pfeng_phylink_start(netif);
		if (ret)
			netdev_warn(netdev, "Error starting phylink: %d\n", ret);
	} else
		netif_carrier_on(netdev);
#endif

	/* Enable EMAC logif */
	if (pfeng_netif_get_emac(netif)) {
		ret = pfe_log_if_enable(pfeng_netif_get_emac(netif)->logif_emac);
		if (ret) {
			netdev_err(netdev, "Cannot enable EMAC: %d\n", ret);
			goto err_mac_ena;
		}
	}

	pfeng_uc_list_sync(netdev);

#ifdef PFE_CFG_PFE_SLAVE
	netif_carrier_on(netdev);
#endif

	netif_tx_start_all_queues(netdev);

	return ret;

err_mac_ena:
#ifdef PFE_CFG_PFE_MASTER
	pm_runtime_put(netif->dev);
#endif /* PFE_CFG_PFE_MASTER */
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

static netdev_tx_t pfeng_netif_logif_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	u32 nfrags = skb_shinfo(skb)->nr_frags;
	struct pfeng_hif_chnl *chnl;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
	unsigned int plen;
	dma_addr_t dma;
	int f, i = 1;
	errno_t ret;

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
	pfeng_hif_shared_chnl_lock_tx(chnl);

	/* Check for ring space */
	if (unlikely(pfeng_hif_chnl_txbd_unused(chnl) < PFE_TXBDS_NEEDED(nfrags + 1))) {
		netif_stop_subqueue(netdev, skb->queue_mapping);
		chnl->queues_stopped = true;
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

	/* Use correct TX mode */
	if (likely(netif->cfg->tx_inject)) {
		/* Set INJECT flag and bypass classifier */
		tx_hdr->flags |= HIF_TX_INJECT;
		tx_hdr->e_phy_ifs = oal_htonl(1U << netif->cfg->emac);
	} else {
		/* Tag the frame with ID of target physical interface */
		tx_hdr->cookie = oal_htonl(netif->cfg->emac);
	}

	if (likely(netdev->features & NETIF_F_IP_CSUM))
		tx_hdr->flags |= HIF_TX_IP_CSUM | HIF_TX_TCP_CSUM | HIF_TX_UDP_CSUM;

	/* HW timestamping */
	if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
		    (netif->tshw_cfg.tx_type == HWTSTAMP_TX_ON))) {
		int ref_num = pfeng_hwts_store_tx_ref(netif, skb);

		if(likely(-ENOMEM != ref_num)) {
			/* Tell stack to wait for hw timestamp */
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;

			/* Tell HW to make timestamp with our ref_num */
			tx_hdr->flags |= HIF_TX_ETS;
			tx_hdr->refnum = htons(ref_num);
		}
		/* In error case no warning is necessary, it will come later from the worker. */
	}

	/* Fill linear part of packet */
	dma = dma_map_single(netif->dev, skb->data, plen, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(netif->dev, dma))) {
		net_err_ratelimited("%s: Frame mapping failed. Packet dropped.\n", netdev->name);
		goto busy_drop;
	}

	/* store the linear part info */
	pfeng_hif_chnl_txconf_put_map_frag(chnl, skb->data, dma, plen, skb, PFENG_MAP_PKT_NORMAL, 0);

	/* Software tx time stamp */
	skb_tx_timestamp(skb);

	/* Put linear part */
	ret = pfe_hif_chnl_tx(chnl->priv, (void *)dma, skb->data, plen, !nfrags);
	if (unlikely(EOK != ret)) {
		net_err_ratelimited("%s: HIF channel tx failed. Packet dropped. Error %d\n",
				    netdev->name, ret);
		goto busy_drop_unroll;
	}

	/* Process frags */
	for (f = 0; f < nfrags; f++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[f];
		plen = skb_frag_size(frag);

		dma = skb_frag_dma_map(netif->dev, frag, 0, plen, DMA_TO_DEVICE);
		if (dma_mapping_error(netif->dev, dma)) {
			net_err_ratelimited("%s: Fragment mapping failed. Packet dropped. Error %d\n",
					    netdev->name, dma_mapping_error(netif->dev, dma));
			goto busy_drop_unroll;
		}

		ret = pfe_hif_chnl_tx(chnl->priv, (void *)dma, frag, plen, f == nfrags - 1);
		if (unlikely(EOK != ret)) {
			net_err_ratelimited("%s: HIF channel frag tx failed. Packet dropped. Error %d\n",
					    netdev->name, ret);
			goto busy_drop_unroll;
		}

		pfeng_hif_chnl_txconf_put_map_frag(chnl, frag, dma, plen, NULL, PFENG_MAP_PKT_NORMAL, i);
		i++;
	}

	pfeng_hif_chnl_txconf_update_wr_idx(chnl, nfrags + 1);
	pfeng_hif_shared_chnl_unlock_tx(chnl);

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += skb->len;

	return NETDEV_TX_OK;

busy_drop_unroll:
	pfeng_hif_chnl_txconf_unroll_map_full(chnl, i - 1);
busy_drop:
	pfeng_hif_shared_chnl_unlock_tx(chnl);

	netdev->stats.tx_dropped++;
	return NETDEV_TX_BUSY;

}

static int pfeng_netif_logif_stop(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);

	if (phyif_emac) {
		pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					   PFE_TYPE_MC, netif->priv->local_drv_id);

		pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					   PFE_TYPE_UC, netif->priv->local_drv_id);
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Stop PHY */
	if (netif->phylink)
		pfeng_phylink_stop(netif);
#endif

	netif_tx_stop_all_queues(netdev);

	pm_runtime_put(netif->dev);

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

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_addr_sync(struct net_device *netdev, const u8 *addr)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);
	errno_t ret;
	u8 buf[18];

	ret = pfe_phy_if_add_mac_addr(phyif_emac, addr, netif->priv->local_drv_id);
	if (ret != EOK)
		netdev_warn(netdev, "failed to add %s to %s: %d\n",
			    mac_to_str(addr, buf), pfe_phy_if_get_name(phyif_emac), ret);

	return -ret;
}

static int pfeng_addr_mc_sync(struct net_device *netdev, const u8 *addr)
{
	if (!is_multicast_ether_addr(addr))
		return 0;

	return pfeng_addr_sync(netdev, addr);
}

static int pfeng_addr_mc_unsync(struct net_device *netdev, const u8 *addr)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	netif->mc_unsynced = true;

	return 0;
}

static int pfeng_addr_uc_sync(struct net_device *netdev, const u8 *addr)
{
	if (!is_unicast_ether_addr(addr))
		return 0;

	return pfeng_addr_sync(netdev, addr);
}

static int pfeng_addr_uc_unsync(struct net_device *netdev, const u8 *addr)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	netif->uc_unsynced = true;

	return 0;
}

static int pfeng_mc_list_sync(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);
	struct netdev_hw_addr *ha;
	errno_t ret;
	u8 buf[18];

	if (!phyif_emac)
		return 0;

	ret = pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					 PFE_TYPE_MC, netif->priv->local_drv_id);
	if (ret != EOK) {
		netdev_err(netdev, "failed to flush multicast MAC addresses\n");
		return -ret;
	}

	netdev_for_each_mc_addr(ha, netdev) {
		if (!is_multicast_ether_addr(ha->addr))
			continue;

		ret = pfe_phy_if_add_mac_addr(phyif_emac, ha->addr, netif->priv->local_drv_id);
		if (ret != EOK)
			netdev_warn(netdev, "failed to add %s to %s: %d\n",
				    mac_to_str(ha->addr, buf), pfe_phy_if_get_name(phyif_emac), ret);
	}

	return -ret;
}

static int pfeng_phyif_is_bridge(pfe_phy_if_t *phyif)
{
	bool on_bridge;

	switch (pfe_phy_if_get_op_mode(phyif)) {
	case IF_OP_VLAN_BRIDGE:
	case IF_OP_L2L3_VLAN_BRIDGE:
		on_bridge = true;
		break;
	default:
		on_bridge = false;
		break;
	}

	return on_bridge;
}

static void pfeng_netif_set_rx_mode(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);
	bool uprom = false, mprom = false;

	if (!phyif_emac)
		return;

	if (netdev->flags & IFF_PROMISC) {
		/* Enable promiscuous mode */
		if (pfe_phy_if_promisc_enable(phyif_emac) != EOK)
			netdev_warn(netdev, "failed to enable promisc mode\n");

		uprom = true;
		mprom = true;
	} else if (netdev->flags & IFF_ALLMULTI) {
		if (pfe_phy_if_allmulti_enable(phyif_emac) != EOK)
			netdev_warn(netdev, "failed to enable promisc mode\n");

		mprom = true;
	}

	__dev_uc_sync(netdev, pfeng_addr_uc_sync, pfeng_addr_uc_unsync);
	__dev_mc_sync(netdev, pfeng_addr_mc_sync, pfeng_addr_mc_unsync);

	if (netif->uc_unsynced) {
		pfeng_uc_list_sync(netdev);
		netif->uc_unsynced = false;
	}

	if (netif->mc_unsynced) {
		pfeng_mc_list_sync(netdev);
		netif->mc_unsynced = false;
	}

	if (!mprom) {
		if (pfe_phy_if_allmulti_disable(phyif_emac) != EOK)
			netdev_warn(netdev, "failed to disable allmulti mode\n");
	}

	if (!uprom) {
		if (pfeng_phyif_is_bridge(phyif_emac)) {
			netdev_dbg(netdev, "bridge op: ignore to disable promisc mode\n");
		} else if (pfe_phy_if_is_promisc(phyif_emac)) {
				if (pfe_phy_if_promisc_disable(phyif_emac) != EOK)
					netdev_warn(netdev, "failed to disable promisc mode\n");
		}
	}

	return;
}
#else /* PFE_CFG_PFE_MASTER */
#define pfeng_netif_set_rx_mode NULL
#endif

static int pfeng_netif_set_mac_address(struct net_device *netdev, void *p)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_emac *emac = pfeng_netif_get_emac(netif);
	struct sockaddr *addr = (struct sockaddr *)p;

	if (is_valid_ether_addr(addr->sa_data)) {
		ether_addr_copy(netdev->dev_addr, addr->sa_data);
	} else {
		netdev_warn(netdev, "No MAC address found, using random\n");
		eth_hw_addr_random(netdev);
	}

	if (!emac)
		return 0;

	netdev_info(netdev, "setting MAC addr: %pM\n", netdev->dev_addr);

#ifdef PFE_CFG_PFE_SLAVE
	{
		errno_t ret = pfe_log_if_add_match_rule(emac->logif_emac, IF_MATCH_DMAC, (void *)netdev->dev_addr, 6U);
		if (EOK != ret) {
			netdev_err(netdev, "Can't add DMAC match rule\n");
			return -ret;
		}
	}
#endif

	return pfeng_uc_list_sync(netdev);
}

static netdev_features_t pfeng_netif_fix_features(struct net_device *netdev, netdev_features_t features)
{
	struct pfeng_netif *netif = netdev_priv(netdev);

	/* Don't enable hw checksumming for AUX interface */
	if (netif->cfg->aux) {
		features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM);
		netdev_info(netdev, "checksum offload not possible for AUX interface\n");
	}

	return features;
}

static const struct net_device_ops pfeng_netdev_ops = {
	.ndo_open		= pfeng_netif_logif_open,
	.ndo_start_xmit		= pfeng_netif_logif_xmit,
	.ndo_stop		= pfeng_netif_logif_stop,
	.ndo_change_mtu		= pfeng_netif_logif_change_mtu,
	.ndo_do_ioctl		= pfeng_netif_logif_ioctl,
	.ndo_set_mac_address	= pfeng_netif_set_mac_address,
	.ndo_set_rx_mode	= pfeng_netif_set_rx_mode,
	.ndo_fix_features	= pfeng_netif_fix_features,
};

static void pfeng_netif_detach_hifs(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct pfeng_hif_chnl *chnl;
	int ret = -EINVAL, i;

	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if (netif->cfg->aux) {
			chnl->netifs[HIF_CLIENTS_AUX_IDX] = NULL;
			netdev_info(netdev, "AUX unsubscribe from HIF%u\n", chnl->idx);
			continue;
		}

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

		if (netif->cfg->aux) {
			chnl->netifs[HIF_CLIENTS_AUX_IDX] = netif;
			netdev_info(netdev, "AUX subscribe to HIF%u\n", chnl->idx);
			continue;
		}

		/* Subscribe to HIF channel */
		if (chnl->netifs[netif->cfg->emac]) {
			netdev_err(netdev, "Unable to register to HIF%u\n", i);
			ret = -EINVAL;
			goto err;
		}
		chnl->netifs[netif->cfg->emac] = netif;
		netdev_info(netdev, "Subscribe to HIF%u\n", chnl->idx);
	}
	ret = 0;

err:
	return ret;
}

static void pfeng_netif_logif_remove(struct pfeng_netif *netif)
{
	pfe_log_if_t *logif_emac;

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
	logif_emac = pfeng_netif_get_emac_logif(netif);
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
	struct pfeng_emac *emac = pfeng_netif_get_emac(netif);
	struct pfeng_hif_chnl *chnl;
	int ret, i;

	/* Create PFE platform-wide pool of interfaces */
	if (pfe_platform_create_ifaces(priv->pfe_platform)) {
		netdev_err(netdev, "Can't init platform interfaces\n");
		goto err;
	}

	/* Prefetch linked EMAC interfaces */
	if (emac) {
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
#ifdef PFE_CFG_PFE_MASTER
			ret = pfe_log_if_promisc_enable(emac->logif_emac);
			if (ret) {
				netdev_err(netdev, "Can't set EMAC Logif promiscuous mode\n");
				goto err;
			}
#endif /* PFE_CFG_PFE_MASTER */
			netdev_dbg(netdev, "EMAC Logif created: %s @%px\n", netif->cfg->name, emac->logif_emac);
		} else
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
	}

	/* Prefetch linked HIF(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		char hifname[16];

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

		if (!chnl->logif_hif) {
			scnprintf(hifname, sizeof(hifname) - 1, "%s-logif", pfe_phy_if_get_name(chnl->phyif_hif));
			chnl->logif_hif = pfe_log_if_create(chnl->phyif_hif, hifname);
			if (!chnl->logif_hif) {
				netdev_err(netdev, "HIF Logif can't be created: %s\n", hifname);
				goto err;
			}

			ret = pfe_platform_register_log_if(priv->pfe_platform, chnl->logif_hif);
			if (ret) {
				netdev_err(netdev, "Can't register HIF Logif\n");
				goto err;
			}
			netdev_dbg(netdev, "HIF Logif created: %s @%px\n", hifname, chnl->logif_hif);
		} else
			netdev_dbg(netdev, "HIF Logif reused: %s @%px\n", hifname, chnl->logif_hif);

		if (emac) {
			if (!netif->cfg->tx_inject) {
				/* Make sure that HIF ingress traffic will be forwarded to respective EMAC */
#ifdef PFE_CFG_PFE_MASTER
				ret = pfe_log_if_set_egress_ifs(chnl->logif_hif, 1 << pfeng_emac_ids[netif->cfg->emac]);
#else
				ret = pfe_log_if_add_egress_if(chnl->logif_hif, pfe_platform_get_phy_if_by_id(priv->pfe_platform, pfeng_emac_ids[netif->cfg->emac]));
#endif
				if (EOK != ret) {
					netdev_err(netdev, "Can't set HIF egress interface\n");
					goto err;
				}
			}
		}
	}

#ifdef PFE_CFG_PFE_SLAVE
	/* Add rule for local MAC */
	if (netif->cfg->tx_inject && emac) {
		/* Configure the logical interface to accept frames matching local MAC address */
		ret = pfe_log_if_add_match_rule(emac->logif_emac, IF_MATCH_DMAC, (void *)netif->cfg->macaddr, 6U);
		if (EOK != ret) {
			netdev_err(netdev, "Can't add DMAC match rule\n");
			ret = -ret;
			goto err;
		}
		if (netif->cfg->emac_router) {
			/* Set parent physical interface to FlexibleRouter mode */
			ret = pfe_phy_if_set_op_mode(emac->phyif_emac, IF_OP_FLEX_ROUTER);
			if (EOK != ret) {
				netdev_err(netdev, "Can't set flexrouter operation mode\n");
				ret = -ret;
				goto err;
			}
		}
		netdev_info(netdev, "receive traffic matching its MAC address\n");
	}
#endif /* PFE_CFG_PFE_SLAVE */

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

	pfeng_netif_set_mac_address(netdev, (void *)&saddr);

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
	netdev->dev.of_node = netif_cfg->dn; /* required by of_find_net_device_by_node(), AAVB-3196 */

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
	netdev->max_mtu = ETH_DATA_LEN + VLAN_HLEN; /* account for 8021q DSA tag length, AAVB-3196 */

	/* Each packet requires extra buffer for Tx header (metadata) */
	netdev->needed_headroom = PFENG_TX_PKT_HEADER_SIZE;

#ifdef PFE_CFG_PFE_MASTER
	pfeng_ethtool_init(netdev);

	/* Add phylink */
	if (!netif_cfg->aux && priv->emac[netif_cfg->emac].intf_mode != PHY_INTERFACE_MODE_INTERNAL)
		pfeng_phylink_create(netif);
#endif

	/* Accelerated feature */
	if (!netif_cfg->aux) {
		/* Chksumming can be enabled only if no AUX involved */
		netdev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
	}
	netdev->hw_features |= NETIF_F_SG;
	netdev->features = netdev->hw_features;
#ifdef PFE_CFG_PFE_MASTER
	netdev->priv_flags |= IFF_UNICAST_FLT;
#endif

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
	struct pfeng_emac *emac = pfeng_netif_get_emac(netif);
	struct pfeng_hif_chnl *chnl;
	int i;

#ifdef PFE_CFG_PFE_MASTER
	if (emac)
		pfeng_phylink_mac_change(netif, false);
#endif /* PFE_CFG_PFE_MASTER */

	netif_device_detach(netif->netdev);

	rtnl_lock();

	if (emac) {
		/* Save EMAC pause */
		pfeng_ethtool_params_save(netif);

		/* Disable EMAC */
		pfe_log_if_disable(emac->logif_emac);
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Stop PHY */
	if (netif_running(netif->netdev) && netif->phylink)
		pfeng_phylink_stop(netif);

	/* Stop RX/TX EMAC clocks */
	if (emac) {
		if (emac->tx_clk)
			clk_disable_unprepare(emac->tx_clk);
		if (emac->rx_clk)
			clk_disable_unprepare(emac->rx_clk);
	}
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
		{
			chnl->phyif_hif = NULL;
			if (chnl->logif_hif) {
				pfe_log_if_disable(chnl->logif_hif);
				chnl->logif_hif = NULL;
			}
		}
	}

	/* Reset linked EMAC IFs */
	if (emac) {
		emac->phyif_emac = NULL;
		emac->logif_emac = NULL;
	}

	return 0;
}

static int pfeng_netif_logif_resume(struct pfeng_netif *netif)
{
	struct pfeng_priv *priv = netif->priv;
	__maybe_unused struct device *dev = &priv->pdev->dev;
	struct net_device *netdev = netif->netdev;
	struct pfeng_emac *emac = pfeng_netif_get_emac(netif);
	struct pfeng_hif_chnl *chnl;
	int ret, i;

	rtnl_lock();

#ifdef PFE_CFG_PFE_MASTER

	/* Restart RX/TX EMAC clocks */

	if (emac) {
		u64 clk_rate;

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
				if (ret)
					dev_err(dev, "TX clocks restart on EMAC%d failed: %d\n", netif->cfg->emac, ret);
				else
					dev_info(dev, "TX clocks on EMAC%d restarted\n", netif->cfg->emac);
			}
			if (ret) {
				devm_clk_put(dev, emac->tx_clk);
				emac->tx_clk = NULL;
			}
		}

		if (emac->rx_clk) {
			ret = clk_set_rate(emac->rx_clk, clk_rate);
			if (ret)
				dev_err(dev, "Failed to set RX clock on EMAC%d: %d\n", netif->cfg->emac, ret);
			else {
				ret = clk_prepare_enable(emac->rx_clk);
				if (ret)
					dev_err(dev, "RX clocks restart on EMAC%d failed: %d\n", netif->cfg->emac, ret);
				else
					dev_info(dev, "RX clocks on EMAC%d restarted\n", netif->cfg->emac);
			}
			if (ret) {
				devm_clk_put(dev, emac->rx_clk);
				emac->rx_clk = NULL;
			}
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

		if (!netif->cfg->tx_inject) {
			/* PFENG_LOGIF_MODE_TX_CLASS mode requires logIf config */
			if (!pfe_log_if_is_enabled(chnl->logif_hif)) {
				ret = pfe_log_if_enable(chnl->logif_hif);
				if (ret)
					netdev_warn(netdev, "Cannot enable logif HIF%i: %d\n", i, ret);
			} else
				netdev_info(netdev, "Logif HIF%i already enabled\n", i);

			if (!pfe_log_if_is_promisc(chnl->logif_hif)) {
				ret = pfe_log_if_promisc_enable(chnl->logif_hif);
				if (ret)
					netdev_warn(netdev, "Cannot set promisc mode for logif HIF%i: %d\n", i, ret);
			} else
				netdev_dbg(netdev, "Logif HIF%i already in promisc mode\n", i);
		}
	}

	/* Enable EMAC logif */
	if (emac) {
		ret = pfe_log_if_enable(emac->logif_emac);
		if (ret)
			netdev_warn(netdev, "Cannot enable EMAC: %d\n", ret);

#ifdef PFE_CFG_PFE_MASTER
		/* Restore RX mode: promisc & UC/MC addresses */
		pfeng_netif_set_rx_mode(netdev);
#endif

		/* Restore EMAC pause and coalesce */
		pfeng_ethtool_params_restore(netif);
	}

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
