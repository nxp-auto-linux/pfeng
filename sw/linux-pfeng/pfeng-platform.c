/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

#include "pfeng.h"

void pfeng_hif_client_exit(struct pfeng_priv *priv, const unsigned int index)
{
	if(index >= ARRAY_SIZE(priv->client)) {
		dev_err(priv->device, "Client id out of range (%d > %ld)\n", index, ARRAY_SIZE(priv->client));
		return;
	}

	if(priv->client[index]) {
		pfe_hif_drv_client_unregister(priv->client[index]);
		priv->client[index] = NULL;
	}

	/* if any interface remains up, start hif drv again */
	if(priv->state & PFENG_STATE_NAPI_IF_MASK) {
		pfe_hif_drv_start(priv->hif);
	}

}

void pfeng_hif_exit(struct pfeng_priv *priv)
{
	/* Destroy hif */
	if(priv->hif) {
		priv->hif = NULL;
	}
}

int pfeng_platform_init(struct pfeng_priv *priv, struct pfeng_resources *res)
{
	int ret = 0;

#if GLOBAL_CFG_IP_VERSION == IP_VERSION_FPGA_5_0_4
	priv->pfe_cfg.common_irq_mode = TRUE; /* use common irq mode (FPGA/PCI) */
	priv->pfe_cfg.irq_vector_global = res->irq.hif[0];
#else
	priv->pfe_cfg.common_irq_mode = FALSE; /* don't use common irq mode */
	priv->pfe_cfg.irq_vector_hif_chnls[0] = res->irq.hif[0];
#endif
	priv->pfe_cfg.cbus_base = (addr_t)priv->ioaddr;
	priv->pfe_cfg.cbus_len = res->addr_size;
	priv->pfe_cfg.fw = priv->fw;
	priv->pfe_cfg.hif_chnls_mask = HIF_CHNL_0; /* channel bitmap */
	priv->pfe_cfg.irq_vector_hif_nocpy = 0; /* disable for now */
	priv->pfe_cfg.irq_vector_bmu = res->irq.bmu;

	ret = pfe_platform_init(&priv->pfe_cfg);
	if (ret) {
		dev_err(priv->device, "Could not init PFE platform\n");
		goto end;
	}

	priv->pfe = pfe_platform_get_instance();
	if (!priv->pfe) {
		dev_err(priv->device, "Could not get PFE platform instance\n");
		ret = -EINVAL;
	}

	/* init hif */
	ret = pfeng_hif_init(priv);
	if (ret) {
		dev_err(priv->device, "Error: Cannot init HIF. Err=%d\n", ret);
	}

end:
	return ret;
}

void pfeng_platform_stop(struct pfeng_priv *priv)
{
	if(priv->hif)
		pfe_hif_drv_stop(priv->hif);

	return;
}

void pfeng_platform_exit(struct pfeng_priv *priv)
{
	pfeng_hif_exit(priv);

	if (EOK != pfe_platform_remove())
		dev_err(priv->device, "Unable to remove the PFE platform\n");

	if(priv->fw)
		pfeng_fw_free(priv);

	oal_mm_shutdown();
}

/* HIF */

pfe_hif_pkt_t *pfeng_hif_rx_get(struct pfeng_priv *priv, int ifid)
{
	pfe_hif_pkt_t *pkt;

	if(ifid >= ARRAY_SIZE(priv->client)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->client));
		return NULL;
	}

	if(!priv->client[ifid])
		return NULL;

	while(1) {

		pkt = pfe_hif_drv_client_receive_pkt(priv->client[ifid], 0);
		if (!pkt)
			/* no more packets */
			return NULL;

		if (unlikely(FALSE == pfe_hif_pkt_is_last(pkt))) {
			/*	Currently we only support one packet per buffer */
			netdev_err(priv->ndev[ifid]->netdev, "Unsupported RX buffer received (len: %d)\n", pfe_hif_pkt_get_data_len(pkt));
			pfe_hif_pkt_free(pkt);
			continue;
		}

		return pkt;
	}
}

void pfeng_hif_rx_free(struct pfeng_priv *priv, int ifid, pfe_hif_pkt_t *pkt)
{
	if(ifid >= ARRAY_SIZE(priv->client)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->client));
		return;
	}

	pfe_hif_pkt_free(pkt);
}

void *pfeng_hif_txack_get_ref(struct pfeng_priv *priv, int ifid)
{
	if(ifid >= ARRAY_SIZE(priv->client)) {
		dev_err(priv->device, "Interface id out of range (%d > %ld)\n", ifid, ARRAY_SIZE(priv->client));
		return NULL;
	}

	return pfe_hif_drv_client_receive_tx_conf(priv->client[ifid], 0);
}

/**
 * @brief		HIF client event handler
 * @details		Called by HIF when client-related event happens (packet received, packet
 * 				transmitted).
 * @note		Running within context of HIF driver worker thread.
 */
static int pfeng_hif_event_handler(pfe_hif_drv_client_t *client, void *data, uint32_t event, uint32_t qno)
{
	struct pfeng_ndev *ndev = (struct pfeng_ndev *)data;

	if(!test_bit(ndev->port_id, &ndev->priv->state))
		/* if not up, return silently */
		return 0;

	netdev_dbg(ndev->netdev, " ---> %s if%d ...\n", __func__, qno);

	if(napi_schedule_prep(&ndev->napi)) {
        __napi_schedule(&ndev->napi);
	}

	return 0;
}

int pfeng_hif_init(struct pfeng_priv *priv)
{
	int ret = 0;

	/* if already inited, return */
	if(priv->hif)
		return 0;

	priv->hif = pfe_platform_get_hif_drv(priv->pfe, 0);	
	if (!priv->hif)
		ret = -ENODEV;

	return ret;
}

int pfeng_hif_client_add(struct pfeng_priv *priv, const unsigned int clid)
{
	int ret = 0;
	pfe_log_if_t *log_if;

	if(!priv->hif) {
		dev_err(priv->device, "The HIF has to be inited before channel\n");
		return -EINVAL;
	}

	if(clid >= ARRAY_SIZE(priv->client)) {
		dev_err(priv->device, "Client id out of range (%d > %ld)\n", clid, ARRAY_SIZE(priv->client));
		return -EINVAL;
	}

	log_if = pfe_platform_get_log_if_by_id(priv->pfe, clid);
	if (NULL == log_if) {
		dev_err(priv->device, "Incorrect log if id %d\n", clid);
		return -ENODEV;
	}

	/* Connect to HIF */
	priv->client[clid] = pfe_hif_drv_client_register(
				priv->hif,				/* HIF Driver instance */
				log_if,					/* Client ID */
				1,						/* TX Queue Count */
				1,						/* RX Queue Count */
				1024,					/* TX Queue Depth */
				1024,					/* RX Queue Depth */
				&pfeng_hif_event_handler, /* Client's event handler */
				(void *)priv->ndev[clid]);			/* Meta data */

	if (NULL == priv->client[clid]) 	{
		dev_err(priv->device, "Unable to register HIF client id %d\n", clid);
		return -ENODEV;
	}

	dev_info(priv->device, "Register HIF client id %d for log if %p\n", clid, log_if);

	pfe_hif_drv_start(priv->hif);

	return ret;
}

/* PHY/MAC */

int pfeng_phy_enable(struct pfeng_priv *priv, int num)
{
	if(num >= PFENG_PHY_PORT_NUM) {
		dev_err(priv->device, "Invalid MAC id=%d\n", num);
		return -EINVAL;
	}

	/*	Enable the interface */
	return pfe_log_if_enable(priv->iface[num]);
}

void pfeng_phy_disable(struct pfeng_priv *priv, int num)
{
	if(num >= PFENG_PHY_PORT_NUM) {
		dev_err(priv->device, "Invalid MAC id=%d\n", num);
	}

	pfe_log_if_disable(priv->iface[num]);
}

int pfeng_phy_init(struct pfeng_priv *priv, int num)
{
	if(num >= PFENG_PHY_PORT_NUM) {
		dev_err(priv->device, "Invalid MAC id=%d\n", num);
		return -EINVAL;
	}

	priv->iface[num] = pfe_platform_get_log_if_by_id(priv->pfe, num);

	netdev_dbg(priv->ndev[num]->netdev, "[%s] log if id %p\n", __func__, priv->iface[num]);

	return (priv->iface[num] != NULL) ? 0 : -EINVAL;
}

int pfeng_phy_mac_add(struct pfeng_priv *priv, int num, void *mac)
{
	int ret;

	if(num >= PFENG_PHY_PORT_NUM) {
		dev_err(priv->device, "Invalid MAC id=%d\n", num);
		return -EINVAL;
	}

	/* Try to assign the address as an individual address */
	ret = pfe_log_if_set_mac_addr(priv->iface[num], mac);
	if (ret == EOK)
		return 0;

	/*	Add the address to the hash group */
	//return pfe_emac_add_addr_to_hash_group(priv->emac, mac);
	return -ENOSPC;
}

int pfeng_phy_get_mac(struct pfeng_priv *priv, int num, void *mac_buf)
{
	if(num >= PFENG_PHY_PORT_NUM) {
		dev_err(priv->device, "Invalid MAC id=%d\n", num);
		return -EINVAL;
	}

	if (pfe_log_if_get_mac_addr(priv->iface[num], mac_buf)) {
		netdev_warn(priv->ndev[num]->netdev, "EMAC does not have associated MAC address\n");
		return -EINVAL;
	}

	return 0;
}

char *pfeng_logif_get_name(struct pfeng_priv *priv, int idx)
{
	pfe_log_if_t *log_if;

	/*	Get logical interface */
	log_if = pfe_platform_get_log_if_by_id(priv->pfe, idx);

	return log_if ? pfe_log_if_get_name(log_if) : NULL;
}

/* PM (hint: move to extra file) */

int pfeng_platform_suspend(struct device *dev)
{
	dev_info(dev, "[%s]\n", __func__);

	return 0;
}

int pfeng_platform_resume(struct device *dev)
{
	dev_info(dev, "[%s]\n", __func__);

	return 0;
}
