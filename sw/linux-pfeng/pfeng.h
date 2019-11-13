/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:     BSD OR GPL-2.0
 *
 */

#ifndef _PFENG_H_
#define _PFENG_H_

#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/stmmac.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "oal.h"
#include "pfe_platform.h"
#include "pfe_hif_drv.h"

#define PFENG_DRIVER_NAME		"pfeng"
#define PFENG_DRIVER_VERSION	"1.0.0"

#define PFENG_MAX_RX_QUEUES	1
#define PFENG_MAX_TX_QUEUES	1

/* no jumbo for now */
#define JUMBO_LEN 1522

/* fpga implements only two for now */
#define PFENG_PHY_PORT_NUM	3

enum pfeng_irq_mode {
	PFENG_IRQ_MODE_UNDEFINED = 0,
	PFENG_IRQ_MODE_SHARED,
	PFENG_IRQ_MODE_PRIVATE,
};

#define PFENG_STATE_NAPI_IF0_INDEX		0
#define PFENG_STATE_NAPI_IF0_BIT		(1 << PFENG_STATE_NAPI_IF0_INDEX)
#define PFENG_STATE_NAPI_IF1_INDEX		1
#define PFENG_STATE_NAPI_IF1_BIT		(1 << PFENG_STATE_NAPI_IF1_INDEX)
#define PFENG_STATE_NAPI_IF2_INDEX		2
#define PFENG_STATE_NAPI_IF2_BIT		(1 << PFENG_STATE_NAPI_IF2_INDEX)
#define PFENG_STATE_NAPI_IF_MASK		(PFENG_STATE_NAPI_IF0_BIT | PFENG_STATE_NAPI_IF1_BIT | PFENG_STATE_NAPI_IF2_BIT)
#define PFENG_STATE_ON_INIT				(1 << 3)
#define PFENG_STATE_ON_ERROR			(1 << 4)

#define PFENG_STATE_MAX_BITS			5

struct pfeng_resources {
	u64 addr;
	u32 addr_size;
	char mac[ETH_ALEN];
	struct {
		u32 hif[HIF_CFG_MAX_CHANNELS];
		u32 bmu;
	} irq;
	enum pfeng_irq_mode irq_mode;
};

struct pfeng_mac {
	//struct mii_regs mii;    /* MII register Addresses */
	unsigned int pcs;
	char eth_addr[ETH_ALEN];
};

struct pfeng_priv;
struct pfeng_ndev {
	struct napi_struct napi ____cacheline_aligned_in_smp;
	struct net_device *netdev;
	struct pfeng_priv *priv;
	uint8_t port_id;
};

struct pfeng_priv {
	/* state bitmap */
	volatile long unsigned int state;

	/* devices */
	struct device *device;
	struct mutex lock;
	struct pfeng_ndev *ndev[PFENG_PHY_PORT_NUM];

	/* hw related stuff */
	void __iomem *ioaddr;
	uint32_t irq_hif_num[HIF_CFG_MAX_CHANNELS];
	uint32_t irq_bmu_num;
	enum pfeng_irq_mode irq_mode;
	struct pfeng_plat_data *plat;
	struct pfeng_mac *hw;

	/* pfe platform members */
	pfe_platform_config_t pfe_cfg;
	pfe_fw_t *fw;
	pfe_platform_t *pfe;
	pfe_log_if_t *iface[PFENG_PHY_PORT_NUM];
	pfe_hif_drv_t *hif;
	pfe_hif_chnl_t *channel;
	pfe_hif_drv_client_t *client[PFENG_PHY_PORT_NUM];

#ifdef CONFIG_DEBUG_FS
	struct dentry *dbgfs_dir;
#endif

};

struct pfeng_plat_data {
	u8 ifaces;
	u8 rx_queues_to_use;
	u8 tx_queues_to_use;
	u16 max_mtu;
};

/* module */
struct pfeng_priv *pfeng_mod_init(struct device *device);
void pfeng_mod_exit(struct device *device);
int pfeng_mod_get_setup(struct device *device,
			struct pfeng_plat_data *plat);
int pfeng_mod_probe(struct device *device,
			struct pfeng_priv *priv,
			struct pfeng_plat_data *plat_dat,
			struct pfeng_resources *res);

/* fw */
int pfeng_fw_load(struct pfeng_priv *priv, char *fw_name);
void pfeng_fw_free(struct pfeng_priv *priv);

/* platform */
int pfeng_platform_init(struct pfeng_priv *priv, struct pfeng_resources *res);
void pfeng_platform_exit(struct pfeng_priv *priv);
void pfeng_platform_stop(struct pfeng_priv *priv);
int pfeng_platform_suspend(struct device *dev);
int pfeng_platform_resume(struct device *dev);

/* napi [hif client callback handler] */
int pfeng_napi_rx(struct pfeng_priv *priv, int limit, int clid);

/* hif */
int pfeng_hif_init(struct pfeng_priv *priv);
void pfeng_hif_exit(struct pfeng_priv *priv);
int pfeng_hif_client_add(struct pfeng_priv *priv, const unsigned int index);
void pfeng_hif_client_exit(struct pfeng_priv *priv, const unsigned int index);
void pfeng_hif_irq_enable(struct pfeng_priv *priv);
void pfeng_hif_irq_disable(struct pfeng_priv *priv);
pfe_hif_pkt_t *pfeng_hif_rx_get(struct pfeng_priv *priv, int idx);
void pfeng_hif_rx_free(struct pfeng_priv *priv, int idx, pfe_hif_pkt_t *bd);
void *pfeng_hif_txack_get_ref(struct pfeng_priv *priv, int idx);
void pfeng_hif_txack_free(struct pfeng_priv *priv, int idx, void *ref);

char *pfeng_logif_get_name(struct pfeng_priv *priv, int idx);

/* ethtool */
void pfeng_ethtool_set_ops(struct net_device *netdev);

/* phy/mac */
int pfeng_phy_init(struct pfeng_priv *priv, int num);
int pfeng_phy_enable(struct pfeng_priv *priv, int num);
void pfeng_phy_disable(struct pfeng_priv *priv, int num);
int pfeng_phy_get_mac(struct pfeng_priv *priv, int num, void *mac_buf);
int pfeng_phy_mac_add(struct pfeng_priv *priv, int num, void *mac);

/* sysfs */
int pfeng_sysfs_init(struct pfeng_priv *priv);
void pfeng_sysfs_exit(struct pfeng_priv *priv);

/* debugfs */
int pfeng_debugfs_init(struct pfeng_priv *priv);
void pfeng_debugfs_exit(struct pfeng_priv *priv);

/* fci */
errno_t pfeng_fci_init(pfe_platform_t *pfe);
void pfeng_fci_exit(void);

#endif
