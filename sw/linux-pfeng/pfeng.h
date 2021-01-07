/*
 * Copyright 2018-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _PFENG_H_
#define _PFENG_H_

#include <linux/version.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/stmmac.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/phylink.h>

#include "pfe_cfg.h"
#include "oal.h"
#include "bpool.h"
#include "pfe_platform.h"
#include "pfe_idex.h"
#include "pfe_hif_drv.h"

#ifdef PFE_CFG_PFE_MASTER
#define PFENG_DRIVER_NAME		"pfeng"
#elif PFE_CFG_PFE_SLAVE
#define PFENG_DRIVER_NAME		"pfeng-slave"
#else
#error Incorrect configuration!
#endif
#define PFENG_DRIVER_VERSION		"1.0.0"

#define PFENG_MAX_RX_QUEUES		1
#define PFENG_MAX_TX_QUEUES		1

#define PFENG_FW_CLASS_NAME		"s32g_pfe_class.fw"
#define PFENG_FW_UTIL_NAME		"s32g_pfe_util.fw"

#define PFENG_LOGIF_OPTS_PHY_CONNECTED	(1 << 1)

static const pfe_hif_chnl_id_t pfeng_chnl_ids[] = {
	HIF_CHNL_0,
	HIF_CHNL_1,
	HIF_CHNL_2,
	HIF_CHNL_3
};

static const pfe_ct_phy_if_id_t pfeng_hif_ids[] = {
	PFE_PHY_IF_ID_HIF0,
	PFE_PHY_IF_ID_HIF1,
	PFE_PHY_IF_ID_HIF2,
	PFE_PHY_IF_ID_HIF3
};

/* HIF channel mode variants */
enum {
	PFENG_HIF_MODE_SC,
	/* PFENG_HIF_MODE_MC, unsupported now */
};

/* represents DT ethernet@ node */
#define PFENG_DT_NODENAME_ETHERNET	"fsl,pfeng-logif"
/* represents DT mdio@ node */
#define PFENG_DT_NODENAME_MDIO		"fsl,pfeng-mdio"

/* config option for ethernet@ node */
struct pfeng_eth {
	struct list_head		lnode;
	const char			*name;
	u32				hif_chnl_sc;
	bool				ihc;
	u8				*addr;
	u8				fixed_link;
	u32				max_speed;
	int				intf_mode;
	u32				emac_id;
	struct device_node		*dn;
	struct clk			*tx_clk;
	struct clk			*rx_clk;
};

/* here comes rest of DT config which not fits to pfe_platform_config_t */
struct pfeng_plat_cfg {
	u32				hif_mode;
	struct resource			syscon;
	u32				hif_chnl_mc;
	struct list_head		eth_list;
	u32				ihc_master_chnl;
};

struct pfeng_priv;

/* HIF channel */
struct pfeng_hif_chnl {
	pfe_hif_chnl_t			*priv;
#ifdef OAL_IRQ_MODE
	oal_irq_t			*irq;
#else
	u32				irqnum;
#endif
	pfe_hif_drv_t			*drv;
	struct dentry			*dentry;
};

/* net interface private data */
struct pfeng_rx_chnl_pool;
struct pfeng_tx_chnl_pool;
struct pfeng_ndev {
	struct list_head		lnode;
	struct napi_struct		napi ____cacheline_aligned_in_smp;
	struct device			*dev;
	struct net_device		*netdev;
	struct phylink			*phylink;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	struct phylink_config		phylink_cfg;
#endif
	struct mii_bus			*mii_bus;
	void				*emac_regs;
	u32				emac_speed;

	struct pfeng_priv		*priv;
	struct pfeng_eth		*eth;
	pfe_hif_drv_client_t		*client;
	pfe_phy_if_t			*phyif_emac;
	pfe_log_if_t			*logif_emac;
	struct pfeng_hif_chnl		chnl_sc;
	struct {
		struct pfeng_rx_chnl_pool	*rx_pool;
		struct pfeng_tx_chnl_pool	*tx_pool;
	} bman;

	u32				opts;

	struct {
		u64			napi_poll;
		u64			napi_poll_onrun;
		u64			napi_poll_completed;
		u64			napi_poll_resched;
		u64			napi_poll_rx;
		u64			txconf_loop;
		u64			tx_busy;
		u64			txconf;
		u64			tx_pkt_frags;
		u64			tx_pkt_frag_deep;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		u64			ihc_rx;
		u64			ihc_tx;
#endif
	} xstats;
};

/* driver private data */
struct pfeng_priv {
	struct platform_device		*pdev;
	struct dentry			*dbgfs;
	struct list_head		ndev_list;
	struct clk			*sys_clk;
	struct pfeng_plat_cfg		plat;
	u32				msg_enable;
	u32				msg_verbosity;

	pfe_platform_config_t		*cfg;
	const char			*fw_class_name;
	const char			*fw_util_name;
	pfe_platform_t			*pfe;

};

/* drv */
struct pfeng_priv *pfeng_drv_alloc(struct platform_device *pdev);
int pfeng_hif_chnl_drv_create(struct pfeng_ndev *ndev);
void pfeng_hif_chnl_drv_remove(struct pfeng_ndev *ndev);
int pfeng_drv_remove(struct pfeng_priv *priv);
int pfeng_drv_probe(struct pfeng_priv *priv);
int pfeng_drv_cfg_get_emac_intf_mode(struct pfeng_priv *priv, u8 id);

/* fw */
int pfeng_fw_load(struct pfeng_priv *priv, const char *fw_class_name, const char *fw_util_name);
void pfeng_fw_free(struct pfeng_priv *priv);

/* debugfs */
int pfeng_debugfs_create(struct pfeng_priv *priv);
void pfeng_debugfs_remove(struct pfeng_priv *priv);
int pfeng_debugfs_add_hif_chnl(struct pfeng_priv *priv, struct pfeng_ndev *ndev);

/* NAPI */
struct pfeng_ndev *pfeng_napi_if_create(struct pfeng_priv *priv, struct pfeng_eth *eth);
void pfeng_napi_if_release(struct pfeng_ndev *ndev);

void pfeng_ethtool_init(struct net_device *netdev);

/* hif */
void pfeng_bman_pool_destroy(struct pfeng_ndev *ndev);
int pfeng_bman_pool_create(struct pfeng_ndev *ndev);
struct sk_buff *pfeng_hif_drv_client_receive_pkt(pfe_hif_drv_client_t *client, uint32_t queue);
int pfeng_hif_chnl_refill_rx_buffer(struct pfeng_ndev *ndev, bool preempt);
int pfeng_hif_chnl_fill_rx_buffers(struct pfeng_ndev *ndev);
bool pfeng_hif_chnl_txconf_check(struct pfeng_ndev *ndev, u32 elems);
int pfeng_hif_chnl_txconf_put_map_frag(struct pfeng_ndev *ndev, void *va_addr, addr_t pa_addr, u32 size, struct sk_buff *skb);
int pfeng_hif_chnl_txconf_free_map_full(struct pfeng_ndev *ndev, u32 idx);
int pfeng_hif_chnl_txconf_unroll_map_full(struct pfeng_ndev *ndev, u32 idx, u32 nfrags);
int pfe_hif_drv_ihc_do_cbk(pfe_hif_drv_t *hif_drv);
int pfe_hif_drv_ihc_put_pkt(pfe_hif_drv_t *hif_drv, void *data, uint32_t len, void *ref);

/* MDIO */
int pfeng_mdio_register(struct pfeng_ndev *ndev);
int pfeng_mdio_unregister(struct pfeng_ndev *ndev);
int pfeng_phylink_create(struct pfeng_ndev *ndev);
int pfeng_phylink_start(struct pfeng_ndev *ndev);
int pfeng_phylink_connect_phy(struct pfeng_ndev *ndev);
int pfeng_phylink_disconnect_phy(struct pfeng_ndev *ndev);
int pfeng_phylink_stop(struct pfeng_ndev *ndev);
int pfeng_phylink_destroy(struct pfeng_ndev *ndev);

#endif
