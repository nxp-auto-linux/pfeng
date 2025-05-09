/*
 * Copyright 2018-2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _PFENG_H_
#define _PFENG_H_

#include <linux/version.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/phylink.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/pcs/nxp-s32cc-xpcs.h>
#include <linux/phy/phy.h>
#include "pfe_cfg.h"
#include "oal.h"
#include "bpool.h"
#include "fifo.h"
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
#define PFENG_DRIVER_VERSION		"1.9.0"

#define PFENG_FW_CLASS_NAME		"s32g_pfe_class.fw"
#define PFENG_FW_UTIL_NAME		"s32g_pfe_util.fw"

#define PFENG_DRIVER_COMMIT_HASH	"M4_DRIVER_COMMIT_HASH"

/* PFE SYS CLK is 300MHz */
#define PFE_CLK_SYS_RATE		300000000

/* PFE TS CLK is 200MHz */
#define PFE_CLK_TS_RATE			200000000

/* IDEX resend delay for first 3 attempts */
#define PFE_CFG_IDEX_FIRST3_RESEND_TIME PFE_CFG_IDEX_RESEND_TIME

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
	PFE_PHY_IF_ID_HIF3,
	/* HIF NOCPY is unsupported, the id can be used
	 * only for addressing master IDEX HIF channel
	 * or linked HIF netdev
	 */
	PFE_PHY_IF_ID_HIF_NOCPY
};

#define PFENG_PFE_HIF_CHANNELS		(ARRAY_SIZE(pfeng_hif_ids) - 1)
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
#define PFENG_RX_PKT_HEADER_SIZE	(sizeof(pfe_ct_hif_rx_hdr_t))

#define PFENG_INT_TIMER_DEFAULT		256 /* usecs */

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
	u8				macaddr[ETH_ALEN];
	u8				phyif_id;
	u8				hifs;
	u32				hifmap;
	bool				pause_rx;
	bool				pause_tx;
#ifdef PFE_CFG_PFE_SLAVE
	bool				emac_router;
#endif /* PFE_CFG_PFE_SLAVE */
	bool				only_mgmt;
};

enum tx_queue_status {
	PFENG_TMU_FULL
};

#define PFENG_TMU_LLTX_DISABLE_MODE_Q_ID	255U

struct pfeng_tmu_q_cfg {
	u8 q_id;
	pfe_ct_phy_if_id_t phy_id;
	u8 q_size; /* cannot exceed 255 */
	u8 min_thr;
};

struct pfeng_tmu_q {
	u32 pkts;
	u8 cap;
};

/* net interface private data */
struct pfeng_netif {
	struct work_struct		tmu_status_check ____cacheline_aligned_in_smp;
	unsigned long 			tx_queue_status;
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
#ifdef PFE_CFG_PFE_SLAVE_PHY
	struct phy_device *slave_phydev;
#endif /* PFE_CFG_PFE_SLAVE_PHY */
#endif /* PFE_CFG_PFE_SLAVE */
	/* if set, the multicast/ unicast MAC addr list needs to be sync'ed with the hw */
	bool mc_unsynced;
	bool uc_unsynced;

	pfe_tmu_t 			*tmu; /* fast access to the TMU handle */
	struct pfeng_tmu_q_cfg 		tmu_q_cfg;
	struct pfeng_tmu_q 		tmu_q;

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
	bool				dbg_info_dumped;
	struct work_struct              ndev_reset_work;
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
	uint32_t			idex_resend_count;
	uint32_t 			idex_resend_delays[2];
};
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/* netif array maps every phy_if to netif */
#define PFENG_NETIFS_CNT	(PFE_PHY_IF_ID_MAX + 1)
/* PHY_IF id hole of HIF block is used for AUX */
#define PFE_PHY_IF_ID_AUX	PFE_PHY_IF_ID_HIF

struct pfeng_rx_chnl_pool;
struct pfeng_tx_chnl_pool;
struct pfeng_hif_chnl {
	struct napi_struct		napi ____cacheline_aligned_in_smp;
	spinlock_t			lock_tx;
	struct net_device		dummy_netdev;
	struct device			*dev;
	pfe_hif_chnl_t			*priv;
	u8				refcount;
	bool				ihc;
	u8				status;
	u8				idx;
	u32				features;

	struct pfeng_netif		*netifs[PFENG_NETIFS_CNT];

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
	refcount_t			logif_hif_count;

	u32				cfg_rx_max_coalesced_frames;
	u32				cfg_rx_coalesce_usecs;
};

static inline struct pfeng_netif *pfeng_phy_if_id_to_netif(struct pfeng_hif_chnl *chnl,
							   pfe_ct_phy_if_id_t phy_if_id)
{
	if (likely((phy_if_id <= PFE_PHY_IF_ID_MAX) &&
		chnl->netifs[phy_if_id])) {
		return chnl->netifs[phy_if_id];
	}

	return chnl->netifs[PFE_PHY_IF_ID_AUX];
}

/* leave out one BD to ensure minimum gap */
#define PFE_TXBDS_NEEDED(val)	((val) + 1)
#define PFE_TXBDS_MAX_NEEDED	PFE_TXBDS_NEEDED(MAX_SKB_FRAGS + 1)

static inline void pfeng_hif_shared_chnl_lock_tx(struct pfeng_hif_chnl *chnl)
{
	if (unlikely(chnl->refcount))
		spin_lock(&chnl->lock_tx);
}

static inline void pfeng_hif_shared_chnl_unlock_tx(struct pfeng_hif_chnl *chnl)
{
	if (unlikely(chnl->refcount))
		spin_unlock(&chnl->lock_tx);
}

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
	bool				rx_clk_pending;
	struct device_node		*dn_mdio;
	struct mii_bus			*mii_bus;
	/* XPCS */
	struct phy			*serdes_phy;
	struct phylink_pcs				*pcs;
	struct phylink_link_state 	xpcs_link;
	u32				serdes_an_speed;
	bool				sgmii_link;

	pfe_phy_if_t			*phyif_emac;
};

/* driver private data */
struct pfeng_priv {
	struct pfeng_hif_chnl		hif_chnl[PFENG_PFE_HIF_CHANNELS]; /* datapath hot, keep first */
	struct platform_device		*pdev;
	struct reset_control		*rst;
	struct list_head		netif_cfg_list;
	struct list_head		netif_list;
	struct clk			*clk_sys;
	struct clk			*clk_pe;
	struct clk                      *clk_ptp;
	uint64_t                        clk_ptp_reference;
	u32				msg_enable;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	struct pfeng_hif_chnl		*ihc_chnl;
	u32				ihc_master_chnl;
	bool				ihc_enabled;
	struct workqueue_struct		*ihc_wq;
	struct work_struct		ihc_tx_work;
	struct work_struct		ihc_rx_work;
	DECLARE_KFIFO_PTR(ihc_tx_fifo, struct sk_buff *);
#ifdef PFE_CFG_PFE_SLAVE
	struct task_struct		*deferred_probe_task;
	struct workqueue_struct		*ihc_slave_wq;
#endif /* PFE_CFG_PFE_SLAVE */
	uint32_t			idex_resend_count;
	uint32_t 			idex_resend_delays[2];
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
	struct pfeng_emac		emac[PFENG_PFE_EMACS];
	struct resource			syscon;
	u8				local_drv_id;
	bool				in_suspend;
	bool				on_g3;
	u8				hif_phc_emac_id;

	struct notifier_block		upper_notifier;
	struct net_device		*lower_ndev;
	pfe_platform_t			*pfe_platform;
	pfe_platform_config_t		*pfe_cfg;
	const char			*fw_class_name;
	const char			*fw_util_name;
	struct dentry			*dbgfs;
	u32				msg_verbosity;
};

static inline bool pfeng_netif_cfg_is_aux(struct pfeng_netif_cfg *cfg)
{
	return cfg->phyif_id == PFE_PHY_IF_ID_AUX;
}

static inline bool pfeng_netif_is_aux(struct pfeng_netif *netif)
{
	return pfeng_netif_cfg_is_aux(netif->cfg);
}

static inline struct pfeng_emac *__pfeng_netif_get_emac(struct pfeng_netif *netif)
{
	return &netif->priv->emac[netif->cfg->phyif_id];
}

static inline bool pfeng_netif_cfg_has_emac(struct pfeng_netif_cfg *cfg)
{
	return cfg->phyif_id <= PFE_PHY_IF_ID_EMAC2;
}

static inline bool pfeng_netif_has_emac(struct pfeng_netif *netif)
{
	return pfeng_netif_cfg_has_emac(netif->cfg);
}

static inline struct pfeng_emac *pfeng_netif_get_emac(struct pfeng_netif *netif)
{
	if (!pfeng_netif_has_emac(netif))
		return NULL;

	return __pfeng_netif_get_emac(netif);
}

static inline pfe_phy_if_t *pfeng_netif_get_emac_phyif(struct pfeng_netif *netif)
{
	if (!pfeng_netif_has_emac(netif))
		return NULL;

	return __pfeng_netif_get_emac(netif)->phyif_emac;
}

/* fw */
int pfeng_fw_load(struct pfeng_priv *priv, const char *fw_class_name, const char *fw_util_name);
void pfeng_fw_free(struct pfeng_priv *priv);

/* dt */
int pfeng_dt_create_config(struct pfeng_priv *priv);
int pfeng_dt_release_config(struct pfeng_priv *priv);

/* gpr */
int pfeng_gpr_check_nvmem_cells(struct device *dev);
int pfeng_gpr_set_port_coherency(struct pfeng_priv *priv);
int pfeng_gpr_clear_port_coherency(struct pfeng_priv *priv);
int pfeng_gpr_set_emac_interfaces(struct pfeng_priv *priv);
int pfeng_gpr_ip_ready_get(struct device *dev, bool *is_on);
int pfeng_gpr_ip_ready_set(struct device *dev, bool on);

/* debugfs */
int pfeng_debugfs_create(struct pfeng_priv *priv);
void pfeng_debugfs_remove(struct pfeng_priv *priv);
int pfeng_debugfs_add_hif_chnl(struct pfeng_priv *priv, u32 idx);

/* mdio */
int pfeng_mdio_register(struct pfeng_priv *priv);
void pfeng_mdio_unregister(struct pfeng_priv *priv);
int pfeng_mdio_suspend(struct pfeng_priv *priv);
int pfeng_mdio_resume(struct pfeng_priv *priv);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
int pfeng_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg);
int pfeng_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg, u16 phydata);
#else
int pfeng_mdio_read_c22(struct mii_bus *bus, int phyaddr, int phyreg);
int pfeng_mdio_write_c22(struct mii_bus *bus, int phyaddr, int phyreg, u16 phydata);
int pfeng_mdio_read_c45(struct mii_bus *bus, int phyaddr, int devad, int phyreg);
int pfeng_mdio_write_c45(struct mii_bus *bus, int phyaddr, int devad, int phyreg, u16 phydata);
#endif

/* hif */
int pfeng_hif_create(struct pfeng_priv *priv);
void pfeng_hif_remove(struct pfeng_priv *priv);
struct sk_buff *pfeng_hif_chnl_receive_pkt(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_event_handler(pfe_hif_drv_client_t *client, void *data, uint32_t event, uint32_t qno);
int pfeng_hif_chnl_start(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_set_coalesce(struct pfeng_hif_chnl *chnl, struct clk *clk_sys, u32 usecs, u32 frames);
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
void pfeng_ihc_tx_work_handler(struct work_struct *work);
void pfeng_ihc_rx_work_handler(struct work_struct *work);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef PFE_CFG_PFE_SLAVE
int pfeng_hif_slave_suspend(struct pfeng_priv *priv);
int pfeng_hif_slave_resume(struct pfeng_priv *priv);
#endif /* PFE_CFG_PFE_SLAVE */

/* bman */
int pfeng_bman_pool_create(struct pfeng_hif_chnl *chnl);
void pfeng_bman_pool_destroy(struct pfeng_hif_chnl *chnl);
int pfeng_hif_chnl_fill_rx_buffers(struct pfeng_hif_chnl *chnl);
void pfeng_hif_chnl_txconf_put_map_frag(struct pfeng_hif_chnl *chnl, addr_t pa_addr, u32 size, struct sk_buff *skb, u8 flags, int i);
u8 pfeng_hif_chnl_txconf_get_flag(struct pfeng_hif_chnl *chnl);
struct sk_buff *pfeng_hif_chnl_txconf_get_skbuf(struct pfeng_hif_chnl *chnl);
void pfeng_hif_chnl_txconf_unroll_map_full(struct pfeng_hif_chnl *chnl, int i);
int pfeng_hif_chnl_txconf_free_map_full(struct pfeng_hif_chnl *chnl, int napi_budget);
int pfeng_hif_chnl_txbd_unused(struct pfeng_hif_chnl *chnl);
bool pfeng_hif_chnl_txbd_is_empty(struct pfeng_hif_chnl *chnl);
void pfeng_hif_chnl_txconf_update_wr_idx(struct pfeng_hif_chnl *chnl, int count);
void pfeng_bman_tx_pool_dump(struct pfeng_hif_chnl *chnl, struct net_device *ndev, void (*dbg_print)(void *ndev, const char *fmt, ...));

/* netif */
int pfeng_netif_create(struct pfeng_priv *priv);
void pfeng_netif_remove(struct pfeng_priv *priv);
int pfeng_netif_suspend(struct pfeng_priv *priv);
int pfeng_netif_resume(struct pfeng_priv *priv);
void pfeng_ethtool_init(struct net_device *netdev);
int pfeng_ethtool_params_save(struct pfeng_netif *netif);
int pfeng_ethtool_params_restore(struct pfeng_netif *netif);
int pfeng_phylink_create(struct pfeng_netif *netif);
int pfeng_phylink_connect_phy(struct pfeng_netif *netif);
int pfeng_phylink_start(struct pfeng_netif *netif);
void pfeng_phylink_stop(struct pfeng_netif *netif);
void pfeng_phylink_disconnect_phy(struct pfeng_netif *netif);
void pfeng_phylink_destroy(struct pfeng_netif *netif);
void pfeng_phylink_mac_change(struct pfeng_netif *netif, bool up);
#ifdef PFE_CFG_PFE_SLAVE_PHY
int pfeng_phy_connect(struct pfeng_netif *netif);
void pfeng_phy_disconnect(struct pfeng_netif *netif);
void pfeng_phy_start(struct pfeng_netif *netif);
void pfeng_phy_stop(struct pfeng_netif *netif);
#endif /* PFE_CFG_PFE_SLAVE_PHY */

/* ptp */
void pfeng_ptp_register(struct pfeng_netif *netif);
void pfeng_ptp_unregister(struct pfeng_netif *netif);

/* hw timestamp */
int pfeng_hwts_init(struct pfeng_netif *netif);
void pfeng_hwts_release(struct pfeng_netif *netif);

static inline unsigned long pfeng_clk_sys_get_rate(struct clk *clk)
{
	if (!clk)
		return PFE_CLK_SYS_RATE;

	return clk_get_rate(clk);
}

void pfeng_hwts_get_tx_ts(struct pfeng_netif *netif, pfe_ct_ets_report_t *etsr);
int pfeng_hwts_store_tx_ref(struct pfeng_netif *netif, struct sk_buff *skb);
int pfeng_hwts_ioctl_set(struct pfeng_netif *netif, struct ifreq *rq);
int pfeng_hwts_ioctl_get(struct pfeng_netif *netif, struct ifreq *rq);
int pfeng_hwts_ethtool(struct pfeng_netif *netif, struct ethtool_ts_info *info);

#endif
