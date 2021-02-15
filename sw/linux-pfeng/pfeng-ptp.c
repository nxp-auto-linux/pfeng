/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#include <linux/limits.h>

#include "pfeng.h"
#include "pfe_platform.h"

#define NS_IN_S (1000000000ULL)

#define PTP_DEBUG(n, f, ...)

int pfeng_ptp_adjfreq(struct ptp_clock_info *ptp, s32 delta)
{
	struct pfeng_ndev *priv = container_of(ptp, struct pfeng_ndev, ptp_ops);
	pfe_emac_t *emac = pfe_phy_if_get_emac(priv->phyif_emac);
	bool_t sgn = TRUE;
	errno_t ret = 0;

	PTP_DEBUG(priv->netdev, "%s, delta %d\n",__func__, delta);

	if (delta < 0) {
		delta = -delta;
		sgn = FALSE;
	}

	ret = pfe_emac_set_ts_freq_adjustment(emac, delta, sgn);

	if (ret != 0){
		netdev_err(priv->netdev, "Frequency adjustment failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct pfeng_ndev *priv = container_of(ptp, struct pfeng_ndev, ptp_ops);
	pfe_emac_t *emac = pfe_phy_if_get_emac(priv->phyif_emac);
	errno_t ret = 0;
	bool_t sgn = TRUE;
	uint32_t sec = 0, nsec = 0;

	PTP_DEBUG(priv->netdev, "%s, delta %lld\n",__func__, delta);

	if (delta < 0) {
		delta = -delta;
		sgn = FALSE;
	}

	sec = delta / NS_IN_S;
	nsec = delta % NS_IN_S;

	ret = pfe_emac_adjust_ts_time(emac, sec, nsec, sgn);

	if (ret != 0) {
		netdev_err(priv->netdev, "Time adjustment failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_gettime64(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct pfeng_ndev *priv = container_of(ptp, struct pfeng_ndev, ptp_ops);
	pfe_emac_t *emac = pfe_phy_if_get_emac(priv->phyif_emac);
	uint32_t sec = 0, nsec = 0;
	uint64_t nsts = 0;
	errno_t ret;

	ret = pfe_emac_get_ts_time(emac, &sec, &nsec);
	nsts = nsec + sec * NS_IN_S;
	*ts = ns_to_timespec64(nsts);

	PTP_DEBUG(priv->netdev, "%s, returned s %lld ns %ld \n",__func__, ts->tv_sec, ts->tv_nsec);

	if (ret != 0) {
		netdev_err(priv->netdev, "Get time failed (err %d)\n", ret);
		ret = -EINVAL;
	}

	return ret;
}

int pfeng_ptp_settime64(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
	struct pfeng_ndev *priv = container_of(ptp, struct pfeng_ndev, ptp_ops);
	pfe_emac_t *emac = pfe_phy_if_get_emac(priv->phyif_emac);
	errno_t ret;

	PTP_DEBUG(priv->netdev, "%s, s %lld ns %ld \n",__func__, ts->tv_sec, ts->tv_nsec);

	ret = pfe_emac_set_ts_time(emac, ts->tv_sec, ts->tv_nsec);

	if (ret != 0) {
		netdev_err(priv->netdev, "Set time failed (err %d)\n", ret);
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
	.adjfreq = pfeng_ptp_adjfreq,
	.adjtime = pfeng_ptp_adjtime,
	.gettime64 = pfeng_ptp_gettime64,
	.settime64 = pfeng_ptp_settime64,
	.enable = pfeng_ptp_enable,
};

static void pfeng_ptp_prepare_clock_adjustement(struct pfeng_ndev *ndev, unsigned long ptp_ref_hz) {
	u32 ptp_ref_clk = ptp_ref_hz;
	u32 ptp_out_clk = ptp_ref_hz/2UL;
	u32 nil_addend = 0, max_addend = 0, max_freq_delta = 0;

	/* Calculate max ppb adjustment */
	nil_addend = (u64)(((u64)ptp_out_clk) << 32) / ptp_ref_clk;
	max_addend = 0xffffffffUL - nil_addend;
	max_freq_delta = ptp_ref_clk - ptp_out_clk;
	pfeng_ptp_ops.max_adj = (u64)((u64)max_freq_delta * 1000000000ULL) / max_addend;

	netdev_info(ndev->netdev, "PTP HW addend 0x%08x, max_adj configured to %d ppb\n",nil_addend, pfeng_ptp_ops.max_adj);
}

void pfeng_ptp_register(struct pfeng_ndev *ndev)
{
	struct pfeng_priv *priv = dev_get_drvdata(ndev->dev);
	pfe_emac_t *emac = ndev->priv->pfe->emac[ndev->eth->emac_id];
	errno_t ret;

	/* Set PTP clock to null in case of error */
	ndev->ptp_clock = NULL;

	/* Check if we have reference clock */
	if((!priv->ptp_reference_clk) || (NULL == priv->ptp_clk))
		return;

	/* Calculate max possible adjustment by controller */
	pfeng_ptp_prepare_clock_adjustement(ndev, priv->ptp_reference_clk);

	/* Start PTP clock and enable time stamping in platform */
	ret = pfe_emac_enable_ts(emac, priv->ptp_reference_clk,
				 priv->ptp_reference_clk / 2LLU);

	if(ret) {
		dev_err(ndev->dev, "Failed to register PTP clock on emac%d\n", ndev->eth->emac_id);
		return;
	}

	/* Register clock and ops */
	ndev->ptp_ops = pfeng_ptp_ops;
	ndev->ptp_clock = ptp_clock_register(&ndev->ptp_ops, ndev->dev);

	if (IS_ERR(ndev->ptp_clock))
		netdev_err(ndev->netdev, "Failed to register PTP clock on emac%d\n", ndev->eth->emac_id);
	else if (ndev->ptp_clock)
		netdev_info(ndev->netdev, "Registered PTP HW clock successfully on emac%d\n", ndev->eth->emac_id);
}

void pfeng_ptp_unregister(struct pfeng_ndev *ndev)
{
	if (ndev->ptp_clock) {
		ptp_clock_unregister(ndev->ptp_clock);
		ndev->ptp_clock = NULL;
		netdev_info(ndev->netdev, "Unregistered PTP HW clock successfully\n");
	}
}
