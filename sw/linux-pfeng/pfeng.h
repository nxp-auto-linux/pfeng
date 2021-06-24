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
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/phylink.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
#include <linux/pcs/fsl-s32gen1-xpcs.h>
#include <linux/phy/phy.h>
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */
#include "pfe_cfg.h"
#include "oal.h"
#include "bpool.h"
#include "fifo.h"
#include "pfe_platform.h"
#include "pfe_idex.h"
#include "pfe_hif_drv.h"

#ifdef PFE_CFG_PFE_MASTER
#if (PFE_CFG_IP_VERSION < PFE_CFG_IP_VERSION_NPU_7_14a)
#define PFENG_DRIVER_NAME		"pfeng-cut1"
#else
#define PFENG_DRIVER_NAME		"pfeng"
#endif /* PFE_CFG_IP_VERSION_NPU_7_14a */
#elif PFE_CFG_PFE_SLAVE
#define PFENG_DRIVER_NAME		"pfeng-slave"
#else
#error Incorrect configuration!
#endif
#define PFENG_DRIVER_VERSION		"BETA 0.9.4 CD1 FORD"

#define PFENG_FW_CLASS_NAME		"s32g_pfe_class.fw"
#define PFENG_FW_UTIL_NAME		"s32g_pfe_util.fw"

static const pfe_ct_phy_if_id_t pfeng_emac_ids[] = {
	PFE_PHY_IF_ID_EMAC0,
	PFE_PHY_IF_ID_EMAC1,
	PFE_PHY_IF_ID_EMAC2
};

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

#define PFENG_PFE_HIF_CHANNELS		(ARRAY_SIZE(pfeng_hif_ids))
#define PFENG_PFE_EMACS			(ARRAY_SIZE(pfeng_emac_ids))

/* HIF channel mode variants */
enum {
	PFENG_HIF_MODE_EXCLUSIVE,
	PFENG_HIF_MODE_SHARED
};

enum {
	PFENG_HIF_STATUS_DISABLED,
	PFENG_HIF_STATUS_REQUESTED,
	PFENG_HIF_STATUS_ENABLED,
	PFENG_HIF_STATUS_RUNNING
};

enum {
	PFENG_MAP_PKT_NORMAL,
	PFENG_MAP_PKT_IHC
};

#define PFENG_TX_PKT_HEADER_SIZE	(sizeof(pfe_ct_hif_tx_hdr_t))

/* skbs waiting for time stamp */
struct pfeng_ts_skb {
	struct list_head		list;
	struct sk_buff			*skb;
	unsigned long			jif_enlisted;
	u16				ref_num;
};

/* timestamp data */
struct pfeng_tx_ts {
	u16				ref_num;
	struct skb_shared_hwtstamps	ts;
};

/* config option for ethernet@ node */
struct pfeng_netif_cfg {
	struct list_head		lnode;
	const char			*name;
	struct device_node		*dn;
	u8				*macaddr;
	u8				emac;
	u8				hifs;
	u32				hifmap;
	bool				tx_inject;
};

/* net interface private data */
struct pfeng_netif {
	struct list_head		lnode;
	struct device			*dev;
	struct net_device		*netdev;
	struct phylink			*phylink;
	struct phylink_config		phylink_cfg;
	struct delayed_work		xpcs_poll_work;
	struct pfeng_netif_cfg		*cfg;
	struct pfeng_priv		*priv;
#ifdef PFE_CFG_PFE_SLAVE
	struct work_struct		ihc_slave_work;
	bool				slave_netif_inited;
#endif /* PFE_CFG_PFE_SLAVE */

	struct work_struct		tx_conf_work;
	/* PTP/Time stamping*/
	struct ptp_clock_info           ptp_ops;
	struct ptp_clock                *ptp_clock;
	struct hwtstamp_config          tshw_cfg;
	DECLARE_KFIFO_PTR(ts_skb_fifo, struct pfeng_ts_skb);
	DECLARE_KFIFO_PTR(ts_tx_fifo, struct pfeng_tx_ts);
	struct work_struct              ts_tx_work;
	struct list_head                ts_skb_list;
	uint16_t                        ts_ref_num;
	bool				ts_work_on;
};

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
struct pfe_hif_drv_client_tag
{
	pfe_ct_phy_if_id_t		phy_if_id;
	pfe_hif_drv_client_event_handler	event_handler;
	void				*priv;
	pfe_hif_drv_t			*hif_drv;
	fifo_t				*ihc_rx_fifo;
	fifo_t				*ihc_txconf_fifo;
	bool				inited;
};

struct pfe_hif_drv_tag
{
	pfe_hif_drv_client_t		*ihc_client;
};
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

struct pfeng_rx_chnl_pool;
struct pfeng_tx_chnl_pool;
struct pfeng_hif_chnl {
	struct napi_struct		napi ____cacheline_aligned_in_smp;
	struct mutex			lock_tx;
	struct net_device		dummy_netdev;
	struct device			*dev;
	pfe_hif_chnl_t			*priv;
	struct dentry			*dentry;
	int				cl_mode;
	bool				ihc;
	u8				status;
	u8				idx;
	u8				netif_q_idx;
	u32				features;

	struct pfeng_netif		*netifs[HIF_CLIENTS_MAX];

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	/* For IDEX support only */
	pfe_hif_drv_t			hif_drv;
	pfe_hif_drv_client_t		ihc_client;
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	struct {
		struct pfeng_rx_chnl_pool	*rx_pool;
		struct pfeng_tx_chnl_pool	*tx_pool;
	} bman;

	pfe_phy_if_t			*phyif_hif;
	pfe_log_if_t			*logif_hif;
};

struct pfeng_emac {
	struct clk			*tx_clk;
	struct clk			*rx_clk;
	phy_interface_t			intf_mode;
	u32				link_an;
	u32				max_speed;
	u32				speed;
	u32				duplex;
	bool				enabled;
	bool				phyless;
	struct device_node		*dn_mdio;
	struct mii_bus			*mii_bus;
#if !defined(PFENG_CFG_LINUX_NO_SERDES_SUPPORT)
	/* XPCS */
	struct phy			*serdes_phy;
	struct s32gen1_xpcs		*xpcs;
	const struct s32gen1_xpcs_ops	*xpcs_ops;
	struct phylink_link_state 	xpcs_link;
	u32				serdes_an_speed;
	bool				sgmii_link;
#endif /* PFENG_CFG_LINUX_NO_SERDES_SUPPORT */

	pfe_phy_if_t			*phyif_emac;
	pfe_log_if_t			*logif_emac;
};

struct pfeng_ihc_tx {
	struct pfeng_hif_chnl		*chnl;
	struct sk_buff			*skb;
};

/* driver private data */
struct pfeng_priv {
	struct platform_device		*pdev;
	struct reset_control		*rst;
	struct dentry			*dbgfs;
	struct list_head		netif_cfg_list;
	struct list_head		netif_list;
	struct clk			*clk_sys;
	struct clk			*clk_pe;
	struct clk                      *clk_ptp;
	uint64_t                        clk_ptp_reference;
	bool				msg_enable;
	u32				msg_verbosity;
	struct pfeng_emac		emac[PFENG_PFE_EMACS];
	struct pfeng_hif_chnl		hif_chnl[PFENG_PFE_HIF_CHANNELS];
	pfe_ct_phy_if_id_t		drv_id;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	struct pfeng_hif_chnl		*ihc_chnl;
	u32				ihc_master_chnl;
	bool				ihc_enabled;
	struct workqueue_struct		*ihc_tx_wq;
	struct work_struct		ihc_tx_work;
	DECLARE_KFIFO_PTR(ihc_tx_fifo, struct pfeng_ihc_tx);
#ifdef PFE_CFG_PFE_SLAVE
	struct workqueue_struct		*ihc_slave_wq;
#endif /* PFE_CFG_PFE_SLAVE */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
	struct resource			syscon;
	u8				local_drv_id;
	bool				in_suspend;

	pfe_platform_t			*pfe_platform;
	pfe_platform_config_t		*pfe_cfg;
	const char			*fw_class_name;
	const char			*fw_util_name;
};

/* fw */
int pfeng_fw_load(struct pfeng_priv *priv, const char *fw_class_name, const char *fw_util_name);
void pfeng_fw_free(struct pfeng_priv *priv);

/* debugfs */
int pfeng_debugfs_create(struct pfeng_priv *priv);
void pfeng_debugfs_remove(struct pfeng_priv *priv);
int pfeng_debugfs_add_hif_chnl(struct pfeng_priv *priv, u32 idx);

/* mdio */
int pfeng_mdio_register(struct pfeng_priv *priv);
void pfeng_mdio_unregister(struct pfeng_priv *priv);
int pfeng_mdio_suspend(struct pfeng_priv *priv);
int pfeng_mdio_resume(struct pfeng_priv *priv);

/* hif */
int pfeng_hif_create(struct pfeng_priv *priv);
void pfeng_hif_remove(struct pfeng_priv *priv);
struct sk_buff *pfeng_hif_chnl_receive_pkt(struct pfeng_hif_chnl *chnl, uint32_t queue);
int pfeng_hif_chnl_event_handler(pfe_hif_drv_client_t *client, void *data, uint32_t event, uint32_t qno);
int pfe_hif_drv_ihc_put_pkt(pfe_hif_drv_client_t *client, void *data, uint32_t len, void *ref);
int pfe_hif_drv_ihc_put_conf(pfe_hif_drv_client_t *client);
int pfeng_hif_chnl_start(struct pfeng_hif_chnl *chnl);
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
void pfeng_ihc_tx_work_handler(struct work_struct *work);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/* bman */
int pfeng_bman_pool_create(struct pfeng_hif_chnl *chnl);
void pfeng_bman_pool_destroy(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_fill_rx_buffers(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_txconf_put_map_frag(struct pfeng_hif_chnl *chnl, void *va_addr, addr_t pa_addr, u32 size, struct sk_buff *skb, u8 flags);
u8 pfeng_hif_chnl_txconf_get_flag(struct pfeng_hif_chnl *chnl);
struct sk_buff *pfeng_hif_chnl_txconf_get_skbuf(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_txconf_unroll_map_full(struct pfeng_hif_chnl *chnl, u32 idx, u32 nfrags);
int pfeng_hif_chnl_txconf_free_map_full(struct pfeng_hif_chnl *chnl);
bool pfeng_hif_chnl_txconf_check(struct pfeng_hif_chnl *chnl, u32 elems);

/* netif */
int pfeng_netif_create(struct pfeng_priv *priv);
void pfeng_netif_remove(struct pfeng_priv *priv);
int pfeng_netif_suspend(struct pfeng_priv *priv);
int pfeng_netif_resume(struct pfeng_priv *priv);
void pfeng_ethtool_init(struct net_device *netdev);
int pfeng_phylink_create(struct pfeng_netif *netif);
int pfeng_phylink_connect_phy(struct pfeng_netif *netif);
int pfeng_phylink_start(struct pfeng_netif *netif);
void pfeng_phylink_stop(struct pfeng_netif *netif);
void pfeng_phylink_disconnect_phy(struct pfeng_netif *netif);
void pfeng_phylink_destroy(struct pfeng_netif *netif);
void pfeng_phylink_mac_change(struct pfeng_netif *netif, bool up);

/* ptp */
void pfeng_ptp_register(struct pfeng_netif *netif);
void pfeng_ptp_unregister(struct pfeng_netif *netif);

/* hw timestamp */
int pfeng_hwts_init(struct pfeng_netif *netif);
void pfeng_hwts_release(struct pfeng_netif *netif);
void pfeng_hwts_skb_set_rx_ts(struct pfeng_netif *netif, struct sk_buff *skb);
void pfeng_hwts_get_tx_ts(struct pfeng_netif *netif, struct sk_buff *skb);
int pfeng_hwts_store_tx_ref(struct pfeng_netif *netif, struct sk_buff *skb);
int pfeng_hwts_ioctl_set(struct pfeng_netif *netif, struct ifreq *rq);
int pfeng_hwts_ioctl_get(struct pfeng_netif *netif, struct ifreq *rq);
int pfeng_hwts_ethtool(struct pfeng_netif *netif, struct ethtool_ts_info *info);

#endif
