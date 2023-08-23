/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>
#include <linux/rtnetlink.h>
#include <linux/list.h>
#include <linux/refcount.h>
#include <linux/clk.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/dsa.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_platform.h"
#include "pfe_feature_mgr.h"

#include "pfeng.h"

#define pfeng_netif_for_each_chnl(netif, chnl_idx, chnl)			\
	for (chnl_idx = 0, chnl = &netif->priv->hif_chnl[chnl_idx];		\
		chnl_idx < PFENG_PFE_HIF_CHANNELS;				\
		chnl_idx++, chnl = &netif->priv->hif_chnl[chnl_idx])

#define TMU_RES_Q_MAX_SIZE	0xFFU
#define TMU_RES_Q_W_FACT	2U
#define TMU_RES_Q_MIN_TX_THR	8U

typedef struct
{
	pfe_mac_addr_t addr;		/* The MAC address */
	struct list_head iterator;	/* List chain entry */
	pfe_drv_id_t owner;		/* Identification of the driver that owns this entry */
} pfeng_netif_mac_db_list_entry_t;

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
		return -ENODEV;

	ret = pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					 PFE_TYPE_UC, netif->priv->local_drv_id);
	if (ret != EOK) {
		HM_MSG_NETDEV_ERR(netdev, "failed to flush multicast MAC addresses\n");
		return -ret;
	}

	ret = pfe_phy_if_add_mac_addr(phyif_emac, netdev->dev_addr,
				      netif->priv->local_drv_id);
	if (ret != EOK) {
		HM_MSG_NETDEV_ERR(netdev, "failed to add %s to %s: %d\n",
			   mac_to_str(netdev->dev_addr, buf),
			   pfe_phy_if_get_name(phyif_emac), ret);
		return -ret;
	}

	netdev_for_each_uc_addr(ha, netdev) {
		if (!is_unicast_ether_addr(ha->addr))
			continue;

		ret = pfe_phy_if_add_mac_addr(phyif_emac, ha->addr, netif->priv->local_drv_id);
		if (ret != EOK)
			HM_MSG_NETDEV_WARN(netdev, "failed to add %s to %s: %d\n",
				    mac_to_str(ha->addr, buf), pfe_phy_if_get_name(phyif_emac), ret);
	}

	return -ret;
}

static int pfeng_netif_logif_open(struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	struct pfeng_hif_chnl *chnl;
	pfe_log_if_t *logif_emac;
	int ret = 0, i;

#ifdef PFE_CFG_PFE_MASTER
	ret = pm_runtime_resume_and_get(netif->dev);
	if (ret < 0)
		return ret;
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_PFE_SLAVE
	if (!netif->slave_netif_inited)
		return -EINVAL;
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
			HM_MSG_NETDEV_ERR(netif->netdev, "Invalid HIF%u (not running)\n", i);
			return -EINVAL;
		}

		if (pfeng_netif_is_aux(netif)) {
			/* PFENG_LOGIF_MODE_TX_CLASS mode requires logIf config */
			if (!pfe_log_if_is_enabled(chnl->logif_hif)) {
				ret = pfe_log_if_enable(chnl->logif_hif);
				if (ret)
					HM_MSG_NETDEV_WARN(netdev, "Cannot enable logif HIF%i: %d\n", i, ret);
			} else
				HM_MSG_NETDEV_INFO(netdev, "Logif HIF%i already enabled\n", i);

			if (!pfe_log_if_is_promisc(chnl->logif_hif)) {
				ret = pfe_log_if_promisc_enable(chnl->logif_hif);
				if (ret)
					HM_MSG_NETDEV_WARN(netdev, "Cannot set promisc mode for logif HIF%i: %d\n", i, ret);
			} else
				HM_MSG_NETDEV_DBG(netdev, "Logif HIF%i already in promisc mode\n", i);
		}
	}

#ifdef PFE_CFG_PFE_MASTER
	if (netif->phylink) {
		ret = pfeng_phylink_connect_phy(netif);
		if (ret) {
			HM_MSG_NETDEV_ERR(netdev, "Error connecting to the phy: %d\n", ret);
			goto err_pl_con;
		} else {
			/* Start PHY */
			ret = pfeng_phylink_start(netif);
			if (ret) {
				HM_MSG_NETDEV_ERR(netdev, "Error starting phylink: %d\n", ret);
				goto err_pl_start;
			}
		}
	} else
		netif_carrier_on(netdev);
#endif /* PFE_CFG_PFE_MASTER */

	/* Enable EMAC logif */
	logif_emac = pfeng_netif_get_emac_logif(netif);
	if (logif_emac) {
		ret = pfe_log_if_enable(logif_emac);
		if (ret) {
			HM_MSG_NETDEV_ERR(netdev, "Cannot enable EMAC: %d\n", ret);
			goto err_mac_ena;
		}
	}

	if (!pfeng_netif_is_aux(netif))
		pfeng_uc_list_sync(netdev);

#ifdef PFE_CFG_PFE_SLAVE
	netif_carrier_on(netdev);
#endif

	netif_tx_start_all_queues(netdev);

	return ret;

#ifdef PFE_CFG_PFE_MASTER
err_pl_start:
	pfeng_phylink_disconnect_phy(netif);
err_pl_con:
#endif /* PFE_CFG_PFE_MASTER */
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

#ifdef PFE_CFG_PFE_MASTER
static int pfe_get_tmu_pkts_conf(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy_id, u8 tx_queue, u32 *pkts_conf)
{
	int ret;

	ret = pfe_tmu_queue_get_tx_count(tmu, phy_id, tx_queue, pkts_conf);
	if (unlikely(ret != 0)) {
		*pkts_conf = 0;
		return -ret;
	}

	return 0;
}

static int pfe_get_tmu_fill(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy_id, u8 tx_queue, u8 *fill)
{
	u32 level;
	int ret;

	ret = pfe_tmu_queue_get_fill_level(tmu, phy_id, tx_queue, &level);
	if (unlikely(ret != 0)) {
		*fill = 0;
		return -ret;
	}

	if (likely(level < U8_MAX)) {
		*fill = level;
	} else {
		*fill = U8_MAX;
	}

	return 0;
}

#else
static int pfe_get_tmu_pkts_conf(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy_id, u8 tx_queue, u32 *pkts_conf)
{
	return 0;
}

static int pfe_get_tmu_fill(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy_id, u8 tx_queue, u8 *fill)
{
	return 0;
}

#endif

static u8 pfeng_tmu_q_window_size(struct pfeng_tmu_q_cfg *cfg)
{
	return cfg->q_size >> TMU_RES_Q_W_FACT;
}

static bool pfeng_tmu_lltx_enabled(struct pfeng_tmu_q_cfg* cfg)
{
#ifdef PFE_CFG_PFE_MASTER
	return cfg->q_id != PFENG_TMU_LLTX_DISABLE_MODE_Q_ID;
#else
	return false; /* LLTX disable for Slave (compile time optimization) */
#endif
}

static void pfeng_tmu_disable_lltx(struct pfeng_tmu_q_cfg* cfg)
{
	cfg->q_id = PFENG_TMU_LLTX_DISABLE_MODE_Q_ID;
}

static u8 pfeng_tmu_get_q_id(struct pfeng_tmu_q_cfg* cfg)
{
	if (likely(pfeng_tmu_lltx_enabled(cfg))) {
		 return cfg->q_id;
	} else {
#ifdef PFE_CFG_HIF_PRIO_CTRL
		/* Firmware will assign queue/priority */
		return PFENG_TMU_LLTX_DISABLE_MODE_Q_ID;
#else
		return 0;
#endif
	}
}

static bool pfeng_tmu_can_tx(pfe_tmu_t *tmu, struct pfeng_tmu_q_cfg* tmu_q_cfg, struct pfeng_tmu_q *tmu_q)
{
	u8 w = pfeng_tmu_q_window_size(tmu_q_cfg);
	u32 pkts = tmu_q->pkts;
	u8 fill, cap, delta;
	bool can_tx = true;
	u32 pkts_conf;
	int err;

	if (likely(tmu_q->cap)) {
		tmu_q->cap--;
		tmu_q->pkts++;
		return true;
	}

	err = pfe_get_tmu_pkts_conf(tmu, tmu_q_cfg->phy_id, tmu_q_cfg->q_id, &pkts_conf);
	if (unlikely(err != 0)) {
		return false;
	}

	delta = (u8)((pkts - pkts_conf) & 0xFFU);

	/*
	 * External perturbation handling, i.e.:
	 * - fast-path flow sharing the same queue, causing
	 *   pkts_conf increase; (1)
	 * - cumulative errors in 'pkts' due to unexpected drops. (2)
	 * Re-adjust 'pkts' for robustness.
	 */

	if (unlikely(pkts_conf > pkts && delta > w)) {
		pkts = pkts_conf; /* (1) */
		delta = 0;
	}

	if (unlikely(pkts > (pkts_conf + w))) {
		pkts = pkts_conf; /* (2) */
		delta = 0;
	}

	cap = w - delta;

	if (unlikely(cap <= tmu_q_cfg->min_thr)) {
		can_tx = false;
		goto out;
	}

	err = pfe_get_tmu_fill(tmu, tmu_q_cfg->phy_id, tmu_q_cfg->q_id, &fill);
	if (unlikely(err != 0)) {
		can_tx = false;
		goto out;
	}

	if (unlikely(cap > tmu_q_cfg->q_size - delta - fill)) {
		can_tx = false;
		goto out;
	}

	/* store the available capacity for next iterations */
	tmu_q->cap = cap;

out:
	tmu_q->pkts = pkts;

	return can_tx;
}

static void pfeng_tmu_status_check(struct work_struct *work)
{
	struct pfeng_netif* netif = container_of(work, struct pfeng_netif, tmu_status_check);
	bool tmu_full = !pfeng_tmu_can_tx(netif->tmu, &netif->tmu_q_cfg, &netif->tmu_q);

	if (tmu_full) {
		schedule_work(&netif->tmu_status_check);
		return;
	}

	if (test_and_clear_bit(PFENG_TMU_FULL, &netif->tx_queue_status)) {
		netif_wake_subqueue(netif->netdev, 0);
	}
}

static netdev_tx_t pfeng_netif_logif_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	u32 nfrags = skb_shinfo(skb)->nr_frags;
	unsigned int len, pktlen = skb->len;
	struct pfeng_hif_chnl *chnl;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
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

		/* mb() to see the txbd ring updates from the NAPI thread after queue stop */
		smp_mb();

		/* prevent a (unlikely but possible) race condition with the NAPI thread,
		 * which may have just finished cleaning up the ring
		 */
		if (pfeng_hif_chnl_txbd_unused(chnl) >= PFE_TXBDS_MAX_NEEDED) {
			netif_start_subqueue(netif->netdev, skb->queue_mapping);
		} else {
			goto busy_drop;
		}
	}

	if (likely(pfeng_tmu_lltx_enabled(&netif->tmu_q_cfg)) &&
		   !pfeng_tmu_can_tx(netif->tmu, &netif->tmu_q_cfg, &netif->tmu_q)) {
		set_bit(PFENG_TMU_FULL, &netif->tx_queue_status);
		smp_wmb();
		netif_stop_subqueue(netdev, skb->queue_mapping);
		schedule_work(&netif->tmu_status_check);
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

	/* record sw tx timestamp before pushing PFE metadata to skb->data */
	skb_tx_timestamp(skb);

	skb_push(skb, PFENG_TX_PKT_HEADER_SIZE);

	len = skb_headlen(skb);

	/* Set TX header */
	tx_hdr = (pfe_ct_hif_tx_hdr_t *)skb->data;
	memset(tx_hdr, 0, sizeof(*tx_hdr));
	tx_hdr->chid = chnl->idx;

	tx_hdr->queue = pfeng_tmu_get_q_id(&netif->tmu_q_cfg);

	/* Use correct TX mode */
	if (unlikely(!pfeng_netif_is_aux(netif))) {
		/* Set INJECT flag and bypass classifier */
		tx_hdr->flags |= HIF_TX_INJECT;
		tx_hdr->e_phy_ifs = oal_htonl(1U << netif->cfg->phyif_id);
	} else {
		/* Tag the frame with ID of target physical interface */
		tx_hdr->cookie = oal_htonl(netif->cfg->phyif_id);
	}

	if (likely(skb->ip_summed == CHECKSUM_PARTIAL)) {
		if (likely(skb->csum_offset == offsetof(struct udphdr, check) &&
			   pktlen <= PFENG_CSUM_OFF_PKT_LIMIT)) {
			tx_hdr->flags |= HIF_TX_UDP_CSUM;
		}
		else if (likely(skb->csum_offset == offsetof(struct tcphdr, check) &&
				pktlen <= PFENG_CSUM_OFF_PKT_LIMIT)) {
			tx_hdr->flags |= HIF_TX_TCP_CSUM;
		} else {
			skb_checksum_help(skb);
		}
	}

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
	dma = dma_map_single(netif->dev, skb->data, len, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(netif->dev, dma))) {
		net_err_ratelimited("%s: Frame mapping failed. Packet dropped.\n", netdev->name);
		goto busy_drop;
	}

	/* store the linear part info */
	pfeng_hif_chnl_txconf_put_map_frag(chnl, dma, len, skb, PFENG_MAP_PKT_NORMAL, 0);

	/* Put linear part */
	ret = pfe_hif_chnl_tx(chnl->priv, (void *)dma, skb->data, len, !nfrags);
	if (unlikely(EOK != ret)) {
		net_err_ratelimited("%s: HIF channel tx failed. Packet dropped. Error %d\n",
				    netdev->name, ret);
		goto busy_drop_unroll;
	}

	/* Process frags */
	for (f = 0; f < nfrags; f++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[f];
		len = skb_frag_size(frag);

		dma = skb_frag_dma_map(netif->dev, frag, 0, len, DMA_TO_DEVICE);
		if (dma_mapping_error(netif->dev, dma)) {
			net_err_ratelimited("%s: Fragment mapping failed. Packet dropped. Error %d\n",
					    netdev->name, dma_mapping_error(netif->dev, dma));
			goto busy_drop_unroll;
		}

		/* save dma map data for tx_conf cleanup before triggering the H/W DMA */
		pfeng_hif_chnl_txconf_put_map_frag(chnl, dma, len, NULL, PFENG_MAP_PKT_NORMAL, i);

		ret = pfe_hif_chnl_tx(chnl->priv, (void *)dma, frag, len, f == nfrags - 1);
		if (unlikely(EOK != ret)) {
			net_err_ratelimited("%s: HIF channel frag tx failed. Packet dropped. Error %d\n",
					    netdev->name, ret);
			goto busy_drop_unroll;
		}

		i++;
	}

	pfeng_hif_chnl_txconf_update_wr_idx(chnl, nfrags + 1);
	pfeng_hif_shared_chnl_unlock_tx(chnl);

	netdev->stats.tx_packets++;
	netdev->stats.tx_bytes += pktlen;

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

	if (pfeng_tmu_lltx_enabled(&netif->tmu_q_cfg)) {
		cancel_work_sync(&netif->tmu_status_check);
		netif->tx_queue_status = 0;
	}

	if (phyif_emac) {
		pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					   PFE_TYPE_MC, netif->priv->local_drv_id);

		pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					   PFE_TYPE_UC, netif->priv->local_drv_id);
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Stop PHY */
	if (netif->phylink) {
		pfeng_phylink_stop(netif);
		pfeng_phylink_disconnect_phy(netif);
	}
#endif /* PFE_CFG_PFE_MASTER */

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
	struct mii_ioctl_data *mii = if_mii(rq);
	int val, phyaddr, phyreg;

	if (pfeng_netif_is_aux(netif))
		return -EOPNOTSUPP;

	if (mdio_phy_id_is_c45(mii->phy_id)) {
		phyaddr = mdio_phy_id_prtad(mii->phy_id);
		phyreg = MII_ADDR_C45 | (mdio_phy_id_devad(mii->phy_id) << 16) | mii->reg_num;
	} else {
		phyaddr = mii->phy_id;
		phyreg = mii->reg_num;
	}

	switch (cmd) {
	case SIOCGMIIPHY:
		if (!pfeng_netif_has_emac(netif) || !netdev->phydev)
			return -EOPNOTSUPP;

		phyaddr = mii->phy_id = netdev->phydev->mdio.addr;
		fallthrough;
	case SIOCGMIIREG:
		if (!pfeng_netif_has_emac(netif))
			return -EOPNOTSUPP;

		if (netdev->phydev)
			return phy_mii_ioctl(netdev->phydev, rq, cmd);
		/* If no phydev, use direct MDIO call */
		val = pfeng_mdio_read(pfeng_netif_get_emac(netif)->mii_bus, phyaddr, phyreg);
		if (val > -1) {
			mii->val_out = val;
			return 0;
		}
		return val;
	case SIOCSMIIREG:
		if (!pfeng_netif_has_emac(netif))
			return -EOPNOTSUPP;

		if (netdev->phydev)
			return phy_mii_ioctl(netdev->phydev, rq, cmd);
		/* If no phydev, use direct MDIO call */
		return pfeng_mdio_write(pfeng_netif_get_emac(netif)->mii_bus, phyaddr, phyreg, mii->val_in);
	case SIOCGHWTSTAMP:
		if (phy_has_hwtstamp(netdev->phydev))
			return phy_mii_ioctl(netdev->phydev, rq, cmd);
		else
			return pfeng_hwts_ioctl_get(netif, rq);
	break;
	case SIOCSHWTSTAMP:
		if (phy_has_hwtstamp(netdev->phydev))
			return phy_mii_ioctl(netdev->phydev, rq, cmd);
		else
			return pfeng_hwts_ioctl_set(netif, rq);
	break;
	}

	return -EOPNOTSUPP;
}

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_addr_sync(struct net_device *netdev, const u8 *addr)
{
	struct pfeng_netif *netif = netdev_priv(netdev);
	pfe_phy_if_t *phyif_emac = pfeng_netif_get_emac_phyif(netif);
	errno_t ret;
	u8 buf[18];

	if (!phyif_emac)
		return -ENODEV;

	ret = pfe_phy_if_add_mac_addr(phyif_emac, addr, netif->priv->local_drv_id);
	if (ret != EOK)
		HM_MSG_NETDEV_WARN(netdev, "failed to add %s to %s: %d\n",
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
		return -ENODEV;

	ret = pfe_phy_if_flush_mac_addrs(phyif_emac, MAC_DB_CRIT_BY_OWNER_AND_TYPE,
					 PFE_TYPE_MC, netif->priv->local_drv_id);
	if (ret != EOK) {
		HM_MSG_NETDEV_ERR(netdev, "failed to flush multicast MAC addresses\n");
		return -ret;
	}

	netdev_for_each_mc_addr(ha, netdev) {
		if (!is_multicast_ether_addr(ha->addr))
			continue;

		ret = pfe_phy_if_add_mac_addr(phyif_emac, ha->addr, netif->priv->local_drv_id);
		if (ret != EOK)
			HM_MSG_NETDEV_WARN(netdev, "failed to add %s to %s: %d\n",
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
			HM_MSG_NETDEV_WARN(netdev, "failed to enable promisc mode\n");

		uprom = true;
		mprom = true;
	} else if (netdev->flags & IFF_ALLMULTI) {
		if (pfe_phy_if_allmulti_enable(phyif_emac) != EOK)
			HM_MSG_NETDEV_WARN(netdev, "failed to enable promisc mode\n");

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
			HM_MSG_NETDEV_WARN(netdev, "failed to disable allmulti mode\n");
	}

	if (!uprom) {
		if (pfeng_phyif_is_bridge(phyif_emac)) {
			HM_MSG_NETDEV_DBG(netdev, "bridge op: ignore to disable promisc mode\n");
		} else if (pfe_phy_if_is_promisc(phyif_emac)) {
				if (pfe_phy_if_promisc_disable(phyif_emac) != EOK)
					HM_MSG_NETDEV_WARN(netdev, "failed to disable promisc mode\n");
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
		HM_MSG_NETDEV_WARN(netdev, "No MAC address found, using random\n");
		eth_hw_addr_random(netdev);
	}

	if (!emac)
		return 0;

	HM_MSG_NETDEV_INFO(netdev, "setting MAC addr: %pM\n", netdev->dev_addr);

#ifdef PFE_CFG_PFE_SLAVE
	{
		errno_t ret = pfe_log_if_add_match_rule(emac->logif_emac, IF_MATCH_DMAC, (void *)netdev->dev_addr, 6U);
		if (EOK != ret) {
			HM_MSG_NETDEV_ERR(netdev, "Can't add DMAC match rule\n");
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
	if (pfeng_netif_is_aux(netif)) {
		features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM);
		HM_MSG_NETDEV_INFO(netdev, "checksum offload not possible for AUX interface\n");
	}

	return features;
}

static void pfeng_ndev_print(void *dev, const char *fmt, ...)
{
	struct net_device *ndev = (struct net_device *)dev;
	struct pfeng_netif *netif = netdev_priv(ndev);
	static char buf[256];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	netif_crit(netif->priv, drv, ndev, "%s", buf);
	va_end(args);
}

static void pfeng_netif_tx_timeout(struct net_device *ndev, unsigned int txq)
{
	struct netdev_queue *dev_queue = netdev_get_tx_queue(ndev, txq);
	struct pfeng_netif *netif = netdev_priv(ndev);
	struct pfeng_hif_chnl *chnl;
	int i;

	if (netif->dbg_info_dumped)
		return;

	netif->dbg_info_dumped = true;

	pfeng_ndev_print(ndev, "-----[ Tx queue #%u timed out: debug info start ]-----", txq);
	pfeng_ndev_print(ndev, "netdev state: 0x%lx, Tx queue state: 0x%lx, pkts: %lu, dropped: %lu (%u ms)",
			 ndev->state, dev_queue->state, ndev->stats.tx_packets, ndev->stats.tx_dropped,
			 jiffies_to_msecs(jiffies - dev_trans_start(ndev)));

	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		pfeng_ndev_print(ndev, "chid: %d, txbd_unused: %d, napi: 0x%lx",
				 i, pfeng_hif_chnl_txbd_unused(chnl), chnl->napi.state);

		pfeng_bman_tx_pool_dump(chnl, ndev, pfeng_ndev_print);

		pfe_hif_chnl_dump_tx_ring_to_ndev(chnl->priv, ndev, pfeng_ndev_print);
	}

	pfeng_ndev_print(ndev, "-----[ Tx queue #%u timed out: debug info stop  ]-----", txq);

	if (netif_running(ndev)) {
		/* try timeout recovery */
		netif_info(netif->priv, drv, ndev, "Resetting netdevice for Tx queue %d", txq);
		schedule_work(&netif->ndev_reset_work);
	}
}

static const struct net_device_ops pfeng_netdev_ops = {
	.ndo_open		= pfeng_netif_logif_open,
	.ndo_start_xmit		= pfeng_netif_logif_xmit,
	.ndo_stop		= pfeng_netif_logif_stop,
	.ndo_change_mtu		= pfeng_netif_logif_change_mtu,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0)
	.ndo_eth_ioctl		= pfeng_netif_logif_ioctl,
#else
	.ndo_do_ioctl		= pfeng_netif_logif_ioctl,
#endif
	.ndo_set_mac_address	= pfeng_netif_set_mac_address,
	.ndo_set_rx_mode	= pfeng_netif_set_rx_mode,
	.ndo_fix_features	= pfeng_netif_fix_features,
	.ndo_tx_timeout		= pfeng_netif_tx_timeout,
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
		if (chnl->netifs[netif->cfg->phyif_id] != netif) {
			HM_MSG_NETDEV_ERR(netdev, "Unknown netif registered to HIF%u\n", i);
			ret = -EINVAL;
			return;
		}
		chnl->netifs[netif->cfg->phyif_id] = NULL;
		HM_MSG_NETDEV_INFO(netdev, "Unsubscribe from HIF%u\n", chnl->idx);
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
			HM_MSG_NETDEV_ERR(netdev, "Invalid HIF%u configuration\n", i);
			ret = -EINVAL;
			goto err;
		}

		/* Subscribe to HIF channel */
		if (chnl->netifs[netif->cfg->phyif_id]) {
			HM_MSG_NETDEV_ERR(netdev, "Unable to register to HIF%u\n", i);
			ret = -EINVAL;
			goto err;
		}
		chnl->netifs[netif->cfg->phyif_id] = netif;
		HM_MSG_NETDEV_INFO(netdev, "Subscribe to HIF%u\n", chnl->idx);
	}
	ret = 0;

err:
	return ret;
}

static void pfeng_netif_logif_remove(struct pfeng_netif *netif)
{
	pfe_log_if_t *logif;
	struct pfeng_hif_chnl *chnl;
	int i;

	if (!netif->netdev)
		return;

	if (netif->priv->lower_ndev) {
		unregister_netdevice_notifier(&netif->priv->upper_notifier);
		netif->priv->lower_ndev = NULL;
	}

	cancel_work_sync(&netif->ndev_reset_work);
	unregister_netdev(netif->netdev); /* calls ndo_stop */

#ifdef PFE_CFG_PFE_SLAVE
	cancel_work_sync(&netif->ihc_slave_work);
#endif /* PFE_CFG_PFE_SLAVE */

#ifdef PFE_CFG_PFE_MASTER
	if (netif->phylink)
		pfeng_phylink_destroy(netif);
#endif /* PFE_CFG_PFE_MASTER */

	/* Stop EMAC logif */
	logif = pfeng_netif_get_emac_logif(netif);
	if (logif) {
		pfe_log_if_disable(logif);
		if (EOK != pfe_platform_unregister_log_if(netif->priv->pfe_platform, logif))
			HM_MSG_NETDEV_WARN(netif->netdev, "Can't unregister EMAC Logif\n");
		else
			pfe_log_if_destroy(logif);
		netif->priv->emac[netif->cfg->phyif_id].logif_emac = NULL;
	}

	/* Remove created HIF logif(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {

		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		logif = chnl->logif_hif;
		if (logif && refcount_dec_and_test(&chnl->logif_hif_count)) {
			pfe_log_if_disable(logif);
			if (EOK != pfe_platform_unregister_log_if(netif->priv->pfe_platform, logif))
				HM_MSG_NETDEV_WARN(netif->netdev, "Can't unregister HIF Logif\n");
			else
				pfe_log_if_destroy(logif);

			chnl->logif_hif = NULL;
		}
	}

	HM_MSG_NETDEV_INFO(netif->netdev, "unregisted\n");

	if (!pfeng_netif_is_aux(netif)) {
		pfeng_ptp_unregister(netif);

		/* Release timestamp memory */
		pfeng_hwts_release(netif);
	}

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
		HM_MSG_NETDEV_ERR(netdev, "Can't init platform interfaces\n");
		goto err;
	}

	/* Prefetch linked EMAC interfaces */
	if (emac) {
		if (!emac->phyif_emac) {
			emac->phyif_emac = pfe_platform_get_phy_if_by_id(priv->pfe_platform, netif->cfg->phyif_id);
			if (!emac->phyif_emac) {
				HM_MSG_NETDEV_ERR(netdev, "Could not get linked EMAC physical interface\n");
				goto err;
			}
		}
		if (!emac->logif_emac) {
			emac->logif_emac = pfe_log_if_create(emac->phyif_emac, (char *)netif->cfg->name);
			if (!emac->logif_emac) {
				HM_MSG_NETDEV_ERR(netdev, "EMAC Logif can't be created: %s\n", netif->cfg->name);
				goto err;
			} else {
				ret = pfe_platform_register_log_if(priv->pfe_platform, emac->logif_emac);
				if (ret) {
					HM_MSG_NETDEV_ERR(netdev, "Can't register EMAC Logif\n");
					goto err;
				}
			}
#ifdef PFE_CFG_PFE_MASTER
			ret = pfe_log_if_promisc_enable(emac->logif_emac);
			if (ret) {
				HM_MSG_NETDEV_ERR(netdev, "Can't set EMAC Logif promiscuous mode\n");
				goto err;
			}
#endif /* PFE_CFG_PFE_MASTER */
			HM_MSG_NETDEV_DBG(netdev, "EMAC Logif created: %s @%px\n", netif->cfg->name, emac->logif_emac);
		} else
			HM_MSG_NETDEV_DBG(netdev, "EMAC Logif reused: %s @%px\n", netif->cfg->name, emac->logif_emac);

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
			HM_MSG_NETDEV_ERR(netdev, "Can't set EMAC egress interface\n");
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
				HM_MSG_NETDEV_ERR(netdev, "Could not get HIF%u physical interface\n", i);
				goto err;
			}
		}

		if (netif->cfg->hifs > 1) {
#ifdef PFE_CFG_PFE_MASTER
			/* Enable loadbalance for multi-HIF config */
			ret = pfe_phy_if_loadbalance_enable(chnl->phyif_hif);
			if (EOK != ret) {
				HM_MSG_NETDEV_ERR(netdev, "Can't set loadbalancing mode to HIF%u\n", i);
				goto err;
			} else
				HM_MSG_NETDEV_INFO(netdev, "add HIF%u loadbalance\n", i);
#else
			HM_MSG_NETDEV_WARN(netdev, "Can't set loadbalancing mode to HIF%u on SLAVE instance\n", i);
#endif
		}

		ret = pfe_phy_if_enable(chnl->phyif_hif);
		if (EOK != ret) {
			HM_MSG_NETDEV_ERR(netdev, "Can't enable HIF%u\n", i);
			goto err;
		}
		HM_MSG_NETDEV_INFO(netdev, "Enable HIF%u\n", i);

		if (!chnl->logif_hif) {
			scnprintf(hifname, sizeof(hifname) - 1, "%s-logif", pfe_phy_if_get_name(chnl->phyif_hif));
			chnl->logif_hif = pfe_log_if_create(chnl->phyif_hif, hifname);
			if (!chnl->logif_hif) {
				HM_MSG_NETDEV_ERR(netdev, "HIF Logif can't be created: %s\n", hifname);
				goto err;
			}

			ret = pfe_platform_register_log_if(priv->pfe_platform, chnl->logif_hif);
			if (ret) {
				HM_MSG_NETDEV_ERR(netdev, "Can't register HIF Logif\n");
				goto err;
			}
			refcount_set(&chnl->logif_hif_count, 1);
			HM_MSG_NETDEV_DBG(netdev, "HIF Logif created: %s @%px\n", hifname, chnl->logif_hif);
		} else {
			refcount_inc(&chnl->logif_hif_count);
			HM_MSG_NETDEV_DBG(netdev, "HIF Logif reused: %s @%px\n", hifname, chnl->logif_hif);
		}

		if (emac) {
			if (pfeng_netif_is_aux(netif)) {
				/* Make sure that HIF ingress traffic will be forwarded to respective EMAC */
#ifdef PFE_CFG_PFE_MASTER
				ret = pfe_log_if_set_egress_ifs(chnl->logif_hif, 1 << pfeng_emac_ids[netif->cfg->phyif_id]);
#else
				ret = pfe_log_if_add_egress_if(chnl->logif_hif, pfe_platform_get_phy_if_by_id(priv->pfe_platform, pfeng_emac_ids[netif->cfg->phyif_id]));
#endif
				if (EOK != ret) {
					HM_MSG_NETDEV_ERR(netdev, "Can't set HIF egress interface\n");
					goto err;
				}
			}
		}
	}

#ifdef PFE_CFG_PFE_SLAVE
	/* Add rule for local MAC */
	if (!pfeng_netif_is_aux(netif) && emac) {
		/* Configure the logical interface to accept frames matching local MAC address */
		ret = pfe_log_if_add_match_rule(emac->logif_emac, IF_MATCH_DMAC, (void *)netif->cfg->macaddr, 6U);
		if (EOK != ret) {
			HM_MSG_NETDEV_ERR(netdev, "Can't add DMAC match rule\n");
			ret = -ret;
			goto err;
		}
		if (netif->cfg->emac_router) {
			/* Set parent physical interface to FlexibleRouter mode */
			ret = pfe_phy_if_set_op_mode(emac->phyif_emac, IF_OP_FLEX_ROUTER);
			if (EOK != ret) {
				HM_MSG_NETDEV_ERR(netdev, "Can't set flexrouter operation mode\n");
				ret = -ret;
				goto err;
			}
		}
		HM_MSG_NETDEV_INFO(netdev, "receive traffic matching its MAC address\n");
	}
#endif /* PFE_CFG_PFE_SLAVE */

	return 0;

err:
	return -EINVAL;
}

#ifdef PFE_CFG_PFE_MASTER
static u32 pfeng_tmu_get_q_size(struct pfeng_netif *netif)
{
	struct pfeng_tmu_q_cfg *cfg = &netif->tmu_q_cfg;
	u32 min, max;
	int err;

	err = pfe_tmu_queue_get_mode(netif->tmu, cfg->phy_id, cfg->q_id, &min, &max);
	if (err) {
		HM_MSG_NETDEV_ERR(netif->netdev, "TMU queue mode read error for PHY_ID#%u/ Q_ID#%u (err: %d)\n",
				  cfg->phy_id, cfg->q_id, err);
		return 0;
	}

	return max;
}
#else
static u32 pfeng_tmu_get_q_size(struct pfeng_netif *netif)
{
	return 0;
}
#endif

static void pfeng_netif_tmu_lltx_init(struct pfeng_netif *netif)
{
	struct pfeng_tmu_q_cfg *cfg = &netif->tmu_q_cfg;
	const struct pfeng_priv *priv = netif->priv;
	struct pfeng_tmu_q *tmu_q = &netif->tmu_q;
	u8 cap, min_thr;
	u32 q_size;

	cfg->q_id = (u8)priv->pfe_cfg->lltx_res_tmu_q_id;

	if (!pfeng_tmu_lltx_enabled(cfg))
		goto out_disabled;

	if (pfeng_netif_is_aux(netif))
		goto disable_lltx;

	netif->tmu = priv->pfe_platform->tmu;
	cfg->phy_id = PFE_PHY_IF_ID_EMAC0 + netif->cfg->phyif_id;

	q_size = pfeng_tmu_get_q_size(netif);
	if (q_size == 0 || q_size > TMU_RES_Q_MAX_SIZE) {
		HM_MSG_NETDEV_ERR(netif->netdev, "TMU returned invalid size for PHY_ID#%u/ Q_ID#%u (size: %u)\n", cfg->phy_id, cfg->q_id, q_size);
		goto disable_lltx;
	}

	cfg->q_size = q_size;

	cap = pfeng_tmu_q_window_size(cfg);
	min_thr = cap >> TMU_RES_Q_W_FACT;
	if (min_thr > TMU_RES_Q_MIN_TX_THR)
		min_thr = TMU_RES_Q_MIN_TX_THR;

	cfg->min_thr = min_thr;
	tmu_q->cap = cap;

	INIT_WORK(&netif->tmu_status_check, pfeng_tmu_status_check);

	HM_MSG_NETDEV_INFO(netif->netdev, "Host LLTX enabled for TMU PHY_ID#%u/ Q_ID#%u\n",
			   netif->tmu_q_cfg.phy_id, netif->tmu_q_cfg.q_id);

	return;

disable_lltx:
	pfeng_tmu_disable_lltx(cfg);
out_disabled:
	HM_MSG_NETDEV_INFO(netif->netdev, "Host LLTX disabled\n");

	return;
}

static int pfeng_netif_logif_init_second_stage(struct pfeng_netif *netif)
{
	struct net_device *netdev = netif->netdev;
	struct sockaddr saddr;
	int ret;

	pfeng_netif_tmu_lltx_init(netif);

	/* Set PFE platform phyifs */
	ret = pfeng_netif_control_platform_ifs(netif);
	if (ret)
		goto err;

	/* Set MAC address */
	if (is_valid_ether_addr(netif->cfg->macaddr))
		memcpy(&saddr.sa_data, netif->cfg->macaddr, ARRAY_SIZE(netif->cfg->macaddr));
	else
		memset(&saddr.sa_data, 0, sizeof(saddr.sa_data));

	pfeng_netif_set_mac_address(netdev, (void *)&saddr);

	if (!pfeng_netif_is_aux(netif)) {
		/* Init hw timestamp */
		ret = pfeng_hwts_init(netif);
		if (ret) {
			HM_MSG_NETDEV_ERR(netdev, "Cannot initialize timestamping: %d\n", ret);
			goto err;
		}
		pfeng_ptp_register(netif);
	}

	if (!netif->priv->in_suspend) {
		ret = register_netdev(netdev);
		if (ret) {
			HM_MSG_NETDEV_ERR(netdev, "Error registering the device: %d\n", ret);
			goto err;
		}

		/* start without the RUNNING flag, phylink/idex controls it later */
		netif_carrier_off(netdev);

		HM_MSG_NETDEV_INFO(netdev, "registered\n");
	}

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

#ifdef PFE_CFG_PFE_MASTER
static int pfeng_netif_event(struct notifier_block *nb,
			     unsigned long event, void *ptr)
{
	struct net_device *ndev = netdev_notifier_info_to_dev(ptr);
	struct netdev_notifier_changeupper_info *info = ptr;
	struct pfeng_priv *priv;
	int ret = 0;

	priv = container_of(nb, struct pfeng_priv, upper_notifier);
	if (priv->lower_ndev != ndev)
		return NOTIFY_DONE;

	switch (event) {
	case NETDEV_CHANGEUPPER:
		if (info->linking) {
			struct pfeng_netif *netif = netdev_priv(ndev);
			struct pfeng_emac *emac = pfeng_netif_get_emac(netif);

			if (!emac)
				goto out;

			if (!emac->rx_clk_pending)
				goto out;

			ret = clk_prepare_enable(emac->rx_clk);
			if (ret) {
				HM_MSG_DEV_ERR(netif->dev, "Failed to enable RX clock on EMAC%d for interface %s (err %d)\n",
					netif->cfg->phyif_id, phy_modes(emac->intf_mode), ret);
				goto out;
			}

			emac->rx_clk_pending = false;

			HM_MSG_DEV_INFO(netif->dev, "RX clock on EMAC%d for interface %s installed\n",
				 netif->cfg->phyif_id, phy_modes(emac->intf_mode));
		}

		break;
	}

out:
	return notifier_from_errno(ret);
}

static int pfeng_netif_register_dsa_notifier(struct pfeng_netif *netif)
{
	struct pfeng_emac *emac = pfeng_netif_get_emac(netif);
	struct pfeng_priv *priv = netif->priv;
	int ret;

	if (emac && emac->rx_clk_pending) {
		if (!priv->lower_ndev) {
			priv->upper_notifier.notifier_call = pfeng_netif_event;

			ret = register_netdevice_notifier(&priv->upper_notifier);
			if (ret) {
				HM_MSG_DEV_ERR(netif->dev, "Error registering the DSA notifier\n");
				return ret;
			}

			priv->lower_ndev = netif->netdev;

		} else {
			HM_MSG_DEV_WARN(netif->dev, "DSA master notifier already registered\n");
		}
	}

	return 0;
}
#else
#define pfeng_netif_register_dsa_notifier(netif) 0
#endif

static void pfeng_reset_ndev(struct work_struct *work)
{
	struct pfeng_netif *netif = container_of(work, struct pfeng_netif, ndev_reset_work);
	struct net_device *ndev = netif->netdev;
	bool reset = false;

	rtnl_lock();
	if (netif_running(ndev)) {
		dev_close(ndev);
		dev_open(ndev, NULL);
		reset = true;
	}
	rtnl_unlock();

	netif->dbg_info_dumped = false; /* re-arm debug dump */
	netif_info(netif->priv, drv, ndev, "netdevice reset %s", reset ? "done" : "skipped");
}

static struct pfeng_netif *pfeng_netif_logif_create(struct pfeng_priv *priv, struct pfeng_netif_cfg *netif_cfg)
{
	struct device *dev = &priv->pdev->dev;
	struct pfeng_netif *netif;
	struct net_device *netdev;
	int ret;

	if (!netif_cfg->name || !strlen(netif_cfg->name)) {
		HM_MSG_DEV_ERR(dev, "Interface name is missing: %s\n", netif_cfg->name);
		return NULL;
	}

	/* allocate net device with max RX and max TX queues */
	netdev = alloc_etherdev_mqs(sizeof(*netif), PFENG_PFE_HIF_CHANNELS, PFENG_PFE_HIF_CHANNELS);
	if (!netdev) {
		HM_MSG_DEV_ERR(dev, "Error allocating the etherdev\n");
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

#ifdef PFE_CFG_PFE_MASTER
	if (pfe_feature_mgr_is_available("jumbo_frames")) {
		netdev->max_mtu = PFE_EMAC_JUMBO_MTU + PFE_MIN_DSA_OVERHEAD;
	} else {
		netdev->max_mtu = PFE_EMAC_STD_MTU + PFE_MIN_DSA_OVERHEAD; /* account for 8021q DSA tag length */
	}
#else
	netdev->max_mtu = PFE_EMAC_JUMBO_MTU + PFE_MIN_DSA_OVERHEAD; /* account for 8021q DSA tag length */
#endif

	/* Each packet requires extra buffer for Tx header (metadata) */
	netdev->needed_headroom = PFENG_TX_PKT_HEADER_SIZE;

	pfeng_ethtool_init(netdev);

#ifdef PFE_CFG_PFE_MASTER
	/* Add phylink */
	if (pfeng_netif_cfg_has_emac(netif->cfg) && priv->emac[netif_cfg->phyif_id].intf_mode != PHY_INTERFACE_MODE_INTERNAL)
		pfeng_phylink_create(netif);
#endif

	/* Accelerated feature */
	if (!pfeng_netif_is_aux(netif)) {
		/* Chksumming can be enabled only if no AUX involved */
		netdev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
	}
	netdev->hw_features |= NETIF_F_SG;
	netdev->features = netdev->hw_features;
#ifdef PFE_CFG_PFE_MASTER
	netdev->priv_flags |= IFF_UNICAST_FLT;
#endif
	INIT_WORK(&netif->ndev_reset_work, pfeng_reset_ndev);

	ret = pfeng_netif_register_dsa_notifier(netif);
	if (ret) {
		HM_MSG_DEV_ERR(dev, "Error registering the DSA notifier: %d\n", ret);
		goto err_netdev_reg;
	}

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
		HM_MSG_NETDEV_ERR(netdev, "IHC channel not configured.\n");
		goto err_netdev_reg;
	}
	ret = pfeng_hif_chnl_start(priv->ihc_chnl);
	if (ret) {
		HM_MSG_NETDEV_ERR(netdev, "IHC channel not started\n");
		goto err_netdev_reg;
	}

	/* Finish device init in deffered work */
	INIT_WORK(&netif->ihc_slave_work, pfeng_netif_slave_work_handler);
	if (!queue_work(priv->ihc_slave_wq, &netif->ihc_slave_work)) {
		HM_MSG_NETDEV_ERR(netdev, "second stage of netif init failed\n");
		goto err_netdev_reg;
	}

	return netif;
#else

	ret = pfeng_netif_logif_init_second_stage(netif);
	if (ret)
		goto err_netdev_reg;

	return netif;
#endif

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

	if (pfeng_tmu_lltx_enabled(&netif->tmu_q_cfg)) {
		cancel_work_sync(&netif->tmu_status_check);
		netif->tx_queue_status = 0;
	}

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
#ifdef PFE_CFG_PFE_MASTER
			/* On Standalone/Master we disable HIF logif instances */
			chnl->phyif_hif = NULL;
			if (chnl->logif_hif) {
				pfe_log_if_disable(chnl->logif_hif);
				chnl->logif_hif = NULL;
			}
#else
			/* On Slave we only stop HIF instances */
			if (chnl->logif_hif)
				pfe_log_if_disable(chnl->logif_hif);
#endif /* PFE_CFG_PFE_MASTER */
		}
	}

#ifdef PFE_CFG_PFE_MASTER
	/* Reset linked EMAC IFs */
	if (emac) {
		emac->phyif_emac = NULL;
		emac->logif_emac = NULL;
	}
#endif /* PFE_CFG_PFE_MASTER */

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
				HM_MSG_DEV_ERR(dev, "Failed to set TX clock on EMAC%d: %d\n", netif->cfg->phyif_id, ret);
			else {
				ret = clk_prepare_enable(emac->tx_clk);
				if (ret)
					HM_MSG_DEV_ERR(dev, "TX clocks restart on EMAC%d failed: %d\n", netif->cfg->phyif_id, ret);
				else
					HM_MSG_DEV_INFO(dev, "TX clocks on EMAC%d restarted\n", netif->cfg->phyif_id);
			}
			if (ret) {
				devm_clk_put(dev, emac->tx_clk);
				emac->tx_clk = NULL;
			}
		}

		if (emac->rx_clk) {
			ret = clk_set_rate(emac->rx_clk, clk_rate);
			if (ret)
				HM_MSG_DEV_ERR(dev, "Failed to set RX clock on EMAC%d: %d\n", netif->cfg->phyif_id, ret);
			else {
				ret = clk_prepare_enable(emac->rx_clk);
				if (ret)
					HM_MSG_DEV_ERR(dev, "RX clocks restart on EMAC%d failed: %d\n", netif->cfg->phyif_id, ret);
				else
					HM_MSG_DEV_INFO(dev, "RX clocks on EMAC%d restarted\n", netif->cfg->phyif_id);
			}
			if (ret) {
				devm_clk_put(dev, emac->rx_clk);
				emac->rx_clk = NULL;
			}
		}
	}

	ret = pfeng_netif_logif_init_second_stage(netif);
#endif /* PFE_CFG_PFE_MASTER */

	/* start HIF channel(s) */
	pfeng_netif_for_each_chnl(netif, i, chnl) {
		if (!(netif->cfg->hifmap & (1 << i)))
			continue;

		if (chnl->status == PFENG_HIF_STATUS_ENABLED)
			pfeng_hif_chnl_start(chnl);

		if (chnl->status != PFENG_HIF_STATUS_RUNNING)
			HM_MSG_NETDEV_WARN(netif->netdev, "HIF%u in invalid state: not running\n", i);

		if (pfeng_netif_is_aux(netif)) {
			/* PFENG_LOGIF_MODE_TX_CLASS mode requires logIf config */
			if (!pfe_log_if_is_enabled(chnl->logif_hif)) {
				ret = pfe_log_if_enable(chnl->logif_hif);
				if (ret)
					HM_MSG_NETDEV_WARN(netdev, "Cannot enable logif HIF%i: %d\n", i, ret);
			} else
				HM_MSG_NETDEV_INFO(netdev, "Logif HIF%i already enabled\n", i);

			if (!pfe_log_if_is_promisc(chnl->logif_hif)) {
				ret = pfe_log_if_promisc_enable(chnl->logif_hif);
				if (ret)
					HM_MSG_NETDEV_WARN(netdev, "Cannot set promisc mode for logif HIF%i: %d\n", i, ret);
			} else
				HM_MSG_NETDEV_DBG(netdev, "Logif HIF%i already in promisc mode\n", i);
		}
	}

	/* Enable EMAC logif */
	if (emac) {
		ret = pfe_log_if_enable(emac->logif_emac);
		if (ret)
			HM_MSG_NETDEV_WARN(netdev, "Cannot enable EMAC: %d\n", ret);

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
			HM_MSG_NETDEV_ERR(netdev, "Error starting phy: %d\n", ret);

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
