/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include <linux/version.h>
#include <linux/module.h>

#include <linux/rtnetlink.h>

#include "pfeng.h"

#define PFE_FW_NAME "pfe-s32g-class.fw"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("NXP");
MODULE_DESCRIPTION("PFEng driver");
MODULE_VERSION(PFENG_DRIVER_VERSION);
MODULE_FIRMWARE(PFE_FW_NAME);

static const u32 default_msg_level = (
				NETIF_MSG_DRV | NETIF_MSG_PROBE |
				NETIF_MSG_LINK | NETIF_MSG_IFUP |
				NETIF_MSG_IFDOWN | NETIF_MSG_TIMER
);

static char *fw_name = PFE_FW_NAME;
module_param(fw_name, charp, 0444);
#ifdef OPT_FW_EMBED
#define FW_DESC_TXT ", use - for built-in variant"
#else
#define FW_DESC_TXT ""
#endif
MODULE_PARM_DESC(fw_name, "\t The name of firmware file (default: " PFE_FW_NAME FW_DESC_TXT ")");

/**
 *  pfeng_tx_timeout
 *  @dev : Pointer to net device structure
 *  Description: this function is called when a packet transmission
 *   fails to complete within a reasonable time. The driver will mark
 *   the error in the netdev structure and arrange for the device to be
 *   reset to a sane state in order to transmit a new packet.
 */
static void pfeng_napi_tx_timeout(struct net_device *dev)
{
	struct pfeng_ndev *ndata = netdev_priv(dev);

	netif_carrier_off(ndata->netdev);

	netdev_info(ndata->netdev, "[%s] TODO: reset\n", __func__);
}

static int pfeng_napi_set_mac_address(struct net_device *ndev, void *p)
{
	struct pfeng_ndev *ndata = netdev_priv(ndev);
	struct pfeng_priv *priv = ndata->priv;
	struct sockaddr *addr = (struct sockaddr *)p;

	if (is_valid_ether_addr(addr->sa_data)) {
		memcpy(ndev->dev_addr, addr->sa_data, ETH_ALEN);
	} else {
		netdev_warn(ndev, "No MAC address found, using random\n");
		eth_hw_addr_random(ndev);
	}

	netdev_dbg(ndev, "[%s] addr %pM\n", __func__, ndev->dev_addr);

	return pfeng_phy_mac_add(priv, ndata->port_id, ndev->dev_addr);
}

/**
 * pfeng_napi_stop_if - Stop the interface
 * @priv: driver private structure
 * @ifid: interface id
 */
static void pfeng_napi_stop_if(struct pfeng_priv *priv, int ifid)
{
	struct net_device *ndev;

	if(ifid >= ARRAY_SIZE(priv->ndev)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->ndev));
		return;
	}

	ndev = priv->ndev[ifid]->netdev;

	netdev_dbg(ndev, "%s: idx %d [state: 0x%lx]...\n", __func__, ifid, priv->state);

	clear_bit(ifid, &priv->state);

	netif_carrier_off(ndev);
	netif_tx_stop_queue(netdev_get_tx_queue(ndev, 0/*ifid*/));

}

/**
 * pfeng_napi_start_if - Start the interface
 * @priv: driver private structure
 * @ifid: interface id
 */
static void pfeng_napi_start_if(struct pfeng_priv *priv, int ifid)
{
	struct net_device *ndev;

	if(ifid >= ARRAY_SIZE(priv->ndev)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->ndev));
		return;
	}

	ndev = priv->ndev[ifid]->netdev;

	netdev_dbg(ndev, "%s: idx %d [state: 0x%lx]...\n", __func__, ifid, priv->state);

	set_bit(ifid, &priv->state);

	netif_carrier_on(ndev);
	netif_tx_start_queue(netdev_get_tx_queue(ndev, 0/*ifid*/));
}

/**
 * pfeng_napi_disable_if - Disable the interface
 * @priv: driver private structure
 * @ifid: interface id
 */
static void pfeng_napi_disable_if(struct pfeng_priv *priv, int ifid)
{
	struct net_device *ndev;

	if(ifid >= ARRAY_SIZE(priv->ndev)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->ndev));
		return;
	}

	ndev = priv->ndev[ifid]->netdev;

	netdev_dbg(ndev, "%s: idx %d [state: 0x%lx]...\n", __func__, ifid, priv->state);

	napi_disable(&priv->ndev[ifid]->napi);
}

/**
 * pfeng_napi_enable_if - Enable the interface
 * @priv: driver private structure
 * @ifid: interface id
 */
static void pfeng_napi_enable_if(struct pfeng_priv *priv, int ifid)
{
	struct net_device *ndev;

	if(ifid >= ARRAY_SIZE(priv->ndev)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->ndev));
		return;
	}

	ndev = priv->ndev[ifid]->netdev;

	netdev_dbg(ndev, "%s: idx %d [state: 0x%lx]...\n", __func__, ifid, priv->state);

	napi_enable(&priv->ndev[ifid]->napi);
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
static int pfeng_napi_open(struct net_device *ndev)
{
	struct pfeng_ndev *ndata = netdev_priv(ndev);
	struct pfeng_priv *priv;
	int ret;
	struct sockaddr mac_addr;
	int ifid;

	netdev_dbg(ndev, "%s: if%d\n", __func__, ndata ? ndata->port_id : -1);

	if(!ndata) {
		netdev_err(ndev, "Error: Cannot init NAPI. NO <ndata>!!!\n");
		ret = -ENODEV;
		goto end;
	}

	priv = ndata->priv;
	ifid = ndata->port_id;

	/* init hif channel (per interface) */
	ret = pfeng_hif_client_add(priv, ifid);
	if (ret) {
		netdev_err(ndev, "Error: Cannot add HIF client to if%d. Err=%d)\n", ifid, ret);
		goto end;
	}

	/* phy */
	ret = pfeng_phy_init(priv, ifid);
	if (ret) {
		netdev_err(ndev, "Error: Cannot init PHY layer on if%d. Eerr=%d\n", ifid, ret);
		goto err_phy_init;
	}

	/* init mac */
	ret = pfeng_phy_enable(priv, ifid);
	if (ret) {
		netdev_err(ndev, "Error: Cannot init mac%d. Err=%d\n", ifid + 1, ret);
		goto err_mac_ena;
	}

	/* set mac addr */
	ret = pfeng_phy_get_mac(priv, ifid, &mac_addr.sa_data);
	if (ret)
		/* clear mac addr to signalize non valid value */
		memset(&mac_addr.sa_data, 0, sizeof(mac_addr.sa_data));
	pfeng_napi_set_mac_address(ndev, (void *)&mac_addr);
	netdev_info(ndev, "eth addr: %pM\n", ndev->dev_addr);

	pfeng_napi_enable_if(priv, ifid);
	pfeng_napi_start_if(priv, ifid);

end:
	return ret;

err_phy_init:
	pfeng_phy_disable(priv, ifid);

err_mac_ena:
	pfeng_hif_client_exit(priv, ifid);

	goto end;
}

/**
 *  pfeng_xmit - Tx entry point of the driver
 *  @skb : the socket buffer
 *  @dev : device pointer
 *  Description : this is the tx entry point of the driver.
 *  It programs the chain or the ring and supports oversized frames
 *  and SG feature.
 */
static netdev_tx_t pfeng_napi_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct pfeng_ndev *ndata = netdev_priv(ndev);
	struct pfeng_priv *priv = ndata->priv;
	int ifid = ndata->port_id;

	hif_drv_sg_list_t sg_list = { 0 };
	errno_t ret;
	uint32_t ii, max_chunk_idx = skb_shinfo(skb)->nr_frags;
	void * addr_va, *addr_pa;

	netdev_dbg(ndev, "[%s] if%d skb len 0x%x (nfrags=%d)\n", __func__, ifid, skb ? skb->len : 0, max_chunk_idx);

	if (max_chunk_idx >= 1 /*HIF_MAX_SG_LIST_LENGTH*/) {
		/* no SG yet */
		netdev_err(ndev, "-----> TODO: TX SG <-----.\n");
		goto pkt_drop;
	}

	ii = 0;

	/* TODO: get buffer from preallocated area */
	addr_va = oal_mm_malloc_contig_aligned_nocache(skb->len, 64);
	if(!addr_va) {
		netdev_err(ndev, "No more mem for transmit request. Frame dropped.\n");
		goto pkt_drop;
	}
	addr_pa = oal_mm_virt_to_phys_contig(addr_va);
	if(!addr_pa) {
		netdev_err(ndev, "No more mem for transmit request. Frame dropped.\n");
		oal_mm_free_contig(addr_va);
		goto pkt_drop;
	}

	memcpy(addr_va, skb->data, skb->len);

	sg_list.items[ii].data_pa = addr_pa;
	sg_list.items[ii].data_va = addr_va;
	sg_list.items[ii].len = skb->len;
	sg_list.items[ii].flags = 0U;

	sg_list.items[ii].flags |= HIF_FIRST_BUFFER;
	sg_list.items[ii].flags |= HIF_LAST_BUFFER;

	sg_list.size = 1;

	//print_hex_dump(KERN_ERR, "pkt-tx: ", DUMP_PREFIX_NONE, 16, 1, (void *)skb->data, skb->len, true);

	ret = pfe_hif_drv_client_xmit_sg_pkt(priv->client[ifid], 0, &sg_list, addr_va);

	if (unlikely(EOK != ret))
	{
		/*	Drop the frame */
		netdev_err(ndev, "Error: HIF did not accept a transmit request (err=%d)\n", ret);
		goto pkt_drop;
	}

	ret = NETDEV_TX_OK;
	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += skb->len;

end:
	dev_kfree_skb_any(skb);
	return ret;

pkt_drop:
	netdev_info(ndev, "Error: packet dropped (skb len=0x%x)\n", skb->len);
	ndev->stats.tx_dropped++;
	ret = NET_XMIT_DROP;

	goto end;
}

/**
 *  pfeng_release - close entry point of the driver
 *  @dev : device pointer.
 *  Description:
 *  This is the stop entry point of the driver.
 */
static int pfeng_napi_release(struct net_device *ndev)
{
	struct pfeng_ndev *ndata = netdev_priv(ndev);
	struct pfeng_priv *priv = ndata->priv;
	int ifid = ndata->port_id;

	netdev_dbg(ndev, "%s\n", __func__);

	pfeng_napi_stop_if(priv, ifid);
	pfeng_napi_disable_if(priv, ifid);

#if 0
    /* Stop and disconnect the PHY */
    if (ndev->phydev) {
        phy_stop(ndev->phydev);
        phy_disconnect(ndev->phydev);
    }
#endif

	pfeng_phy_disable(priv, ifid);
	pfeng_hif_client_exit(priv, ifid);

	return 0;
}

/**
 *  pfeng_change_mtu - entry point to change MTU size for the device.
 *  @dev : device pointer.
 *  @new_mtu : the new MTU size for the device.
 *  Description: the Maximum Transfer Unit (MTU) is used by the network layer
 *  to drive packet transmission. Ethernet has an MTU of 1500 octets
 *  (ETH_DATA_LEN). This value can be changed with ifconfig.
 *  Return value:
 *  0 on success and an appropriate integer as defined in errno.h
 *  file on failure.
 */
static int pfeng_napi_change_mtu(struct net_device *ndev, int new_mtu)
{
	netdev_dbg(ndev, "%s: mtu change to %d\n", __func__, new_mtu);

	if (netif_running(ndev)) {
		netdev_err(ndev, "Error: Must be stopped to change its MTU\n");
	return -EBUSY;
	}

	ndev->mtu = new_mtu;

	netdev_update_features(ndev);

	return 0;
}

static netdev_features_t pfeng_fix_features(struct net_device *ndev,
				netdev_features_t features)
{
	netdev_dbg(ndev, "%s\n", __func__);

	/* TODO */

	return features;
}

static int pfeng_set_features(struct net_device *ndev,
				netdev_features_t features)
{
	netdev_dbg(ndev, "%s\n", __func__);

	/* TODO */

	return 0;
}

/**
 *  pfeng_ioctl - Entry point for the Ioctl
 *  @dev: Device pointer.
 *  @rq: An IOCTL specefic structure, that can contain a pointer to
 *  a proprietary structure used to pass information to the driver.
 *  @cmd: IOCTL command
 *  Description:
 *  Currently it supports the phy_mii_ioctl(...) and HW time stamping.
 */
static int pfeng_napi_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd)
{
	int ret = -EOPNOTSUPP;

	netdev_dbg(ndev, "%s: cmd=0x%x\n", __func__, cmd);

	if (!netif_running(ndev))
		return -EINVAL;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		if (!ndev->phydev)
			return -EINVAL;
		ret = phy_mii_ioctl(ndev->phydev, rq, cmd);
		break;

	default:
		break;
	}

	return ret;
}

static const struct net_device_ops pfeng_netdev_ops = {
	.ndo_open = pfeng_napi_open,
	.ndo_start_xmit = pfeng_napi_xmit,
	.ndo_stop = pfeng_napi_release,
	.ndo_change_mtu = pfeng_napi_change_mtu,
	.ndo_fix_features = pfeng_fix_features,
	.ndo_set_features = pfeng_set_features,
	/* TODO: .ndo_set_rx_mode = pfeng_set_rx_mode, */
	.ndo_tx_timeout = pfeng_napi_tx_timeout,
	.ndo_do_ioctl = pfeng_napi_ioctl,
	/* TODO: .ndo_setup_tc = pfeng_setup_tc, */
#ifdef CONFIG_NET_POLL_CONTROLLER
	/* TODO: .ndo_poll_controller = pfeng_poll_controller, */
#endif
	.ndo_set_mac_address = pfeng_napi_set_mac_address,
};

/**
 * pfeng_mod_init
 * @device: device pointer
 * @: platform data pointer
 * Description:
 */
struct pfeng_priv *pfeng_mod_init(struct device *device)
{
	struct pfeng_priv *priv;

	dev_info(device, "%s, ethernet driver loading ...\n", PFENG_DRIVER_NAME);

	priv = devm_kzalloc(device, sizeof(*priv), GFP_KERNEL);
	if(!priv)
		return NULL;

	priv->device = device;

	return priv;
}

/**
 * pfeng_mod_get_setup
 * @device: device pointer
 * @plat_dat: platform data pointer
 * Description: Currently static, but is intended to read device tree
 */
int pfeng_mod_get_setup(struct device *device, struct pfeng_plat_data *plat)
{

	/* Set the maxmtu to a default of JUMBO_LEN */
	plat->max_mtu = JUMBO_LEN;

	/* Set default number of RX and TX queues to use */
	plat->tx_queues_to_use = 1;
	plat->rx_queues_to_use = 1;

	/* */
	plat->ifaces = PFENG_PHY_PORT_NUM;

	return 0;
}

/**
 * pfeng_rx - manage the receive process
 * @priv: driver private structure
 * @limit: napi budget
 * @queue: RX client id
 * Description :  this the function called by the napi poll method.
 * It gets all the frames inside the ring.
 */
int pfeng_napi_rx(struct pfeng_priv *priv, int limit, int ifid)
{
	struct net_device *ndev = priv->ndev[ifid]->netdev;
	unsigned int done = 0;
	pfe_hif_pkt_t *pkt;

	netdev_dbg(ndev, "---> %s: if%d ...\n", __func__, ifid);

	while((pkt = pfeng_hif_rx_get(priv, ifid))) {
		struct sk_buff *skb;

		//print_hex_dump(KERN_ERR, "pkt-rx: ", DUMP_PREFIX_NONE, 16, 1, (void *)pfe_hif_pkt_get_data(pkt), pfe_hif_pkt_get_data_len(pkt), true);

		skb = netdev_alloc_skb_ip_align(ndev, pfe_hif_pkt_get_data_len(pkt));
		if (!skb) {
				ndev->stats.rx_dropped++;
				pfeng_hif_rx_free(priv, ifid, pkt);
				continue;
		}
		/* Copy packet from buffer */
		skb_put_data(skb, (void *)pfe_hif_pkt_get_data(pkt) + /*HEADER LEN???*/ 16, pfe_hif_pkt_get_data_len(pkt));

		/* Pass to upper layer */
		skb->protocol = eth_type_trans(skb, ndev);
		netif_receive_skb(skb);

		pfeng_hif_rx_free(priv, ifid, pkt);

		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += pfe_hif_pkt_get_data_len(pkt);

		done++;
		if(done == limit)
			break;
	}

	netdev_dbg(ndev, "---> %s done = %d\n", __func__, done);

	return done;
}

static int pfeng_napi_txack(struct pfeng_priv *priv, int limit, int ifid)
{
	struct net_device *ndev = priv->ndev[ifid]->netdev;
	unsigned int done = 0;
	void *ref;

	netdev_dbg(ndev, "---> %s if%d ...\n", __func__, ifid);

	while((ref = pfeng_hif_txack_get_ref(priv, ifid))) {

		/* release dma buffer of TX packet */
		/* TODO: return to preallocated buffer */
		oal_mm_free_contig(ref);

		done++;
		if(done == limit)
			break;
	}

	netdev_dbg(ndev, "---> %s done = %d\n", __func__, done);

	return done;
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
	struct pfeng_priv *priv = ndev->priv;
	int ifid = ndev->port_id;
	int done = 0;

	dev_dbg(priv->device, "%s: napi %p if%d ...\n", __func__, napi, ifid);

	/* consume RX pkt(s) */
	done = pfeng_napi_rx(priv, NAPI_POLL_WEIGHT, ifid);

	/* Cleanup Tx ring first */
	pfeng_napi_txack(priv, NAPI_POLL_WEIGHT, ifid);

	/* TODO: priv->xstats.napi_poll++; */

	napi_complete_done(napi, done);

	return done;
}

/**
 * pfeng_mod_probe
 * @device: device pointer
 * @plat_dat: platform data pointer
 * @res: pfe resource pointer
 * Description: this is the main probe function used to
 * call the alloc_etherdev, allocate the priv structure.
 * It gets called on driver load only.
 * Return:
 * returns 0 on success, otherwise errno.
 */
int pfeng_mod_probe(struct device *device,
				struct pfeng_priv *priv,
				struct pfeng_plat_data *plat_dat,
				struct pfeng_resources *res)
{
	struct net_device *ndev = NULL;
	int ret = 0;
	int i;

	/* PFE platform layer init */
	oal_mm_init(device);

	dev_set_drvdata(device, priv);

	/* Load firmware */
	ret = pfeng_fw_load(priv, fw_name);
	if (ret) {
		dev_err(priv->device, "Failed to load firmware\n");
		goto err_hw_init;
	}

	/* driver priv init */
	priv->plat = plat_dat;
	priv->ioaddr = (void __iomem *)res->addr;
	priv->irq_mode = res->irq_mode;
	for (i = 0; i < ARRAY_SIZE(priv->irq_hif_num); i++) {
		priv->irq_hif_num[i] = res->irq.hif[i];
	}
	priv->irq_bmu_num = res->irq.bmu;
	priv->state = 0;

	/* PFE platform hw init */
	ret = pfeng_platform_init(priv, res);
	if (ret) {
		dev_err(priv->device, "failed to setup pfe subsystem\n");
		goto err_hw_init;
	}

	for (i = 0; i < priv->plat->ifaces; i++) {
		struct pfeng_ndev *ndata;

		/* allocate net device with one RX and one TX queue */
		ndev = alloc_etherdev_mqs(sizeof(*ndata), 1, 1);
		if (!ndev) {
			dev_err(device, "Error registering the device (err=%d)\n", ret);
			//TODO goto platform_stop
			return -ENOMEM;
		}

		/* Set the sysfs physical device reference for the network logical device */
		SET_NETDEV_DEV(ndev, device);

		ndev->mem_start = (unsigned long)res->addr;
		ndev->mem_end = ndev->mem_start + res->addr_size;
		ndev->irq = res->irq.hif[0]; /* multi hif not supported yet */

		/* Set private structures */
		ndata = netdev_priv(ndev);
		ndata->netdev = ndev;
		ndata->priv = priv;
		ndata->port_id = i;

		pfeng_ethtool_set_ops(ndev);

		/* Configure real RX and TX queues */
		netif_set_real_num_rx_queues(ndev, priv->plat->rx_queues_to_use);
		netif_set_real_num_tx_queues(ndev, priv->plat->tx_queues_to_use);

		/* Set up explicit device name based on platform names */
		strlcpy(ndev->name, pfeng_logif_get_name(priv, i), IFNAMSIZ);

		ndev->netdev_ops = &pfeng_netdev_ops;

		/* TODO: ndev->hw_features = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM; */

		/* MTU ranges */
		ndev->min_mtu = ETH_ZLEN - ETH_HLEN;
		ndev->max_mtu = SKB_MAX_HEAD(NET_SKB_PAD + NET_IP_ALIGN);

		netif_napi_add(ndev, &ndata->napi, pfeng_napi_poll, NAPI_POLL_WEIGHT);

		ret = register_netdev(ndev);
		if (ret) {
			dev_err(priv->device, "Error registering the device (err=%d)\n", ret);
			goto err_napi_init;
		}
		dev_info(priv->device, "%s registered\n", ndev->name);

		/* start without the RUNNING flag, phylib controls it later */
		//netif_carrier_off(ndev);

		priv->ndev[i] = ndata;

	}

	mutex_init(&priv->lock);

	ret = pfeng_debugfs_init(priv);
	if(ret)
		dev_warn(priv->device, "Warning: debugfs node was not created (err=%d)\n", ret);
	ret = 0;

	/* sysfs */
	ret = pfeng_sysfs_init(priv);
	if(ret) {
		dev_warn(priv->device, "Warning: sysfs node was not created (error=%d)\n", ret);
		ret = 0;
	}

end:
	return ret;

err_napi_init:
	pfeng_fw_free(priv);

	dev_set_drvdata(device, NULL);

	// oal_mm_release

	for (i = priv->plat->ifaces-1; i >= 0; i--) {
		struct net_device *ndev;

		if(!priv->ndev[i])
			continue;

		ndev = priv->ndev[i]->netdev;

		unregister_netdev(ndev);
		netif_napi_del(&priv->ndev[i]->napi);
		free_netdev(ndev);
	}


err_hw_init:
	//TODO: pfeng_platform_release

	goto end;
}

/**
 * pfeng_dvr_remove
 * @dev: device pointer
 * Description: this function resets the TX/RX processes,
 *   disables the MAC RX/TX, changes the link status,
 *   releases the DMA descriptor rings.
 */
void pfeng_mod_exit(struct device *dev)
{
	struct pfeng_priv *priv = dev_get_drvdata(dev);
	int i;

	if(!priv) {
		dev_warn(dev, "%s: driver unloading impossible, no private data.", __func__);
		return;
	}

	pfeng_debugfs_exit(priv);

	pfeng_sysfs_exit(priv);

	pfeng_platform_stop(priv);

	/* NAPI removal */
	for (i = priv->plat->ifaces-1; i >= 0; i--) {
		struct net_device *ndev = priv->ndev[i]->netdev;

		netdev_info(ndev, "removing driver ...");

		unregister_netdev(ndev);
		free_netdev(ndev);
	}

	mutex_destroy(&priv->lock);

	pfeng_platform_exit(priv);

}
