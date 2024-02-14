/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#include <linux/limits.h>

#include "pfeng.h"
#include "pfe_platform.h"

#define NS_IN_S (1000000000ULL)

#define PTP_DEBUG(n, f, ...)

static errno_t pfeng_ptp_get_phc_emac_id(struct pfeng_netif *netif, u32 *phc_emac_id)
{
	/* select proper PHC */
	switch (netif->cfg->phyif_id) {
	case PFE_PHY_IF_ID_EMAC0:
	case PFE_PHY_IF_ID_EMAC1:
	case PFE_PHY_IF_ID_EMAC2:
		*phc_emac_id = netif->cfg->phyif_id;
		break;
	case PFE_PHY_IF_ID_HIF0:
	case PFE_PHY_IF_ID_HIF1:
	case PFE_PHY_IF_ID_HIF2:
	case PFE_PHY_IF_ID_HIF3:
		*phc_emac_id = netif->priv->hif_phc_emac_id;
		break;
	default:
		/* only AUX should come here */
		return -EINVAL;
	}

	return 0;
}

static int pfeng_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
{
	struct pfeng_netif *netif = container_of(ptp, struct pfeng_netif, ptp_ops);
	struct pfeng_priv *priv = netif->priv;
	pfe_emac_t *emac;
	bool_t sgn = TRUE;
	u32 phc_emac_id;
	s32 delta = scaled_ppm_to_ppb(scaled_ppm);
	errno_t ret = 0;

	if (pfeng_ptp_get_phc_emac_id(netif, &phc_emac_id))
		return -ENODEV;

	PTP_DEBUG(netif->netdev, "%s, delta %d\n",__func__, delta);

	emac = priv->pfe_platform->emac[phc_emac_id];

	if (delta < 0) {
		delta = -delta;
		sgn = FALSE;
	}

	ret = pfe_emac_set_ts_freq_adjustment(emac, delta, sgn);

	if (ret == EPERM) {
		HM_MSG_NETDEV_WARN(netif->netdev, "Frequency adjustment failed on EMAC%u\n", phc_emac_id);
		ret = -EOPNOTSUPP;
	} else if (ret != 0){
		HM_MSG_NETDEV_ERR(netif->netdev, "Frequency adjustment failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct pfeng_netif *netif = container_of(ptp, struct pfeng_netif, ptp_ops);
	struct pfeng_priv *priv = netif->priv;
	pfe_emac_t *emac;
	errno_t ret = 0;
	bool_t sgn = TRUE;
	uint32_t sec = 0, nsec = 0;
	u32 phc_emac_id;

	if (pfeng_ptp_get_phc_emac_id(netif, &phc_emac_id))
		return -ENODEV;

	emac = priv->pfe_platform->emac[phc_emac_id];

	PTP_DEBUG(netif->netdev, "%s, delta %lld\n",__func__, delta);

	if (delta < 0) {
		delta = -delta;
		sgn = FALSE;
	}

	sec = delta / NS_IN_S;
	nsec = delta % NS_IN_S;

	ret = pfe_emac_adjust_ts_time(emac, sec, nsec, sgn);

	if (ret == EPERM) {
		HM_MSG_NETDEV_WARN(netif->netdev, "Time adjustment failed on EMAC%u\n", phc_emac_id);
		ret = -EOPNOTSUPP;
	} else if (ret != 0) {
		HM_MSG_NETDEV_ERR(netif->netdev, "Time adjustment failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_gettime64(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct pfeng_netif *netif = container_of(ptp, struct pfeng_netif, ptp_ops);
	struct pfeng_priv *priv = netif->priv;
	pfe_emac_t *emac;
	uint32_t sec = 0, nsec = 0;
	uint16_t sec_hi = 0;
	errno_t ret;
	u32 phc_emac_id;

	if (pfeng_ptp_get_phc_emac_id(netif, &phc_emac_id))
		return -ENODEV;

	emac = priv->pfe_platform->emac[phc_emac_id];

	ret = pfe_emac_get_ts_time(emac, &sec, &nsec, &sec_hi);

	ts->tv_nsec = nsec;
	ts->tv_sec = ((uint64_t)sec_hi << 32U) + sec;

	PTP_DEBUG(netif->netdev, "%s, returned s %lld ns %ld \n",__func__, ts->tv_sec, ts->tv_nsec);

	if (ret != 0) {
		HM_MSG_NETDEV_ERR(netif->netdev, "Get time failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_settime64(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
	struct pfeng_netif *netif = container_of(ptp, struct pfeng_netif, ptp_ops);
	struct pfeng_priv *priv = netif->priv;
	pfe_emac_t *emac;
	errno_t ret;
	uint32_t sec = (uint64_t)ts->tv_sec & 0x00000000FFFFFFFFU;
	uint16_t sec_hi = (uint16_t)(ts->tv_sec >> 32);
	u32 phc_emac_id;

	if (pfeng_ptp_get_phc_emac_id(netif, &phc_emac_id))
		return -ENODEV;

	emac = priv->pfe_platform->emac[phc_emac_id];

	PTP_DEBUG(netif->netdev, "%s, s %lld ns %ld \n",__func__, ts->tv_sec, ts->tv_nsec);

	ret = pfe_emac_set_ts_time(emac, sec, ts->tv_nsec, sec_hi);

	if (ret == EPERM) {
		HM_MSG_NETDEV_WARN(netif->netdev, "Set time failed on EMAC%u\n", phc_emac_id);
		ret = -EOPNOTSUPP;
	} else if (ret != 0) {
		HM_MSG_NETDEV_ERR(netif->netdev, "Set time failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_enable(struct ptp_clock_info __always_unused *ptp,
		     struct ptp_clock_request __always_unused *request,
		     int __always_unused on)
{
	/* Clocks are enabled in platform */
	return -EOPNOTSUPP;
}

static struct ptp_clock_info pfeng_ptp_ops = {
	.owner = THIS_MODULE,
	.name = "pfeng ptp",
	.max_adj = 6500000, /* in ppb */
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0,
	.n_pins = 0,
	.pps = 0,
	.adjfine = pfeng_ptp_adjfine,
	.adjtime = pfeng_ptp_adjtime,
	.gettime64 = pfeng_ptp_gettime64,
	.settime64 = pfeng_ptp_settime64,
	.enable = pfeng_ptp_enable,
};

static void pfeng_ptp_prepare_clock_adjustement(struct pfeng_netif *netif, unsigned long ptp_ref_hz) {
	u32 ptp_ref_clk = ptp_ref_hz;
	u32 ptp_out_clk = ptp_ref_hz/2UL;
	u32 nil_addend = 0, max_addend = 0, max_freq_delta = 0;

	/* Calculate max ppb adjustment */
	nil_addend = (u64)(((u64)ptp_out_clk) << 32) / ptp_ref_clk;
	max_addend = 0xffffffffUL - nil_addend;
	max_freq_delta = ptp_ref_clk - ptp_out_clk;
	pfeng_ptp_ops.max_adj = (u64)((u64)max_freq_delta * 1000000000ULL) / max_addend;

	HM_MSG_NETDEV_INFO(netif->netdev, "PTP HW addend 0x%08x, max_adj configured to %d ppb\n",nil_addend, pfeng_ptp_ops.max_adj);
}

void pfeng_ptp_register(struct pfeng_netif *netif)
{
	struct pfeng_priv *priv = netif->priv;
	pfe_emac_t *emac;
	errno_t ret;
	bool ext_ts;
	u32 phc_emac_id;

	if (pfeng_ptp_get_phc_emac_id(netif, &phc_emac_id))
		/* no PHC, valid only for AUX */
		return;

	emac = priv->pfe_platform->emac[phc_emac_id];

	/* Set PTP clock to null in case of error */
	netif->ptp_clock = NULL;

	/* Check if we have reference clock */
	if((!priv->clk_ptp_reference) || (!priv->clk_ptp))
		return;

	/* Calculate max possible adjustment by controller */
	pfeng_ptp_prepare_clock_adjustement(netif, priv->clk_ptp_reference);

	/* Get EMAC's timestamping mode external / internal */
	ext_ts = priv->pfe_platform->emac_ext_ts_mask & (1 << phc_emac_id);
	/* Start PTP clock and enable time stamping in platform */
	ret = pfe_emac_enable_ts(emac, priv->clk_ptp_reference,
				 ext_ts ? 0 : (priv->clk_ptp_reference / 2LLU));

	if(ret) {
		HM_MSG_DEV_ERR(netif->dev, "Failed to register PTP clock on EMAC%d\n", phc_emac_id);
		return;
	}

	/* Register clock and ops */
	netif->ptp_ops = pfeng_ptp_ops;
	netif->ptp_clock = ptp_clock_register(&netif->ptp_ops, netif->dev);

	if (IS_ERR(netif->ptp_clock))
		HM_MSG_NETDEV_ERR(netif->netdev, "Failed to register PTP clock on EMAC%d\n", phc_emac_id);
	else if (netif->ptp_clock)
		HM_MSG_NETDEV_INFO(netif->netdev, "Registered PTP HW clock successfully on EMAC%d\n", phc_emac_id);
}

void pfeng_ptp_unregister(struct pfeng_netif *netif)
{
	if (netif->ptp_clock) {
		ptp_clock_unregister(netif->ptp_clock);
		netif->ptp_clock = NULL;
		HM_MSG_NETDEV_INFO(netif->netdev, "Unregistered PTP HW clock successfully on EMAC%d\n", netif->priv->hif_phc_emac_id);
	}
}
