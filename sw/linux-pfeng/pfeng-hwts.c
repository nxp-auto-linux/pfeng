/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>

#include "pfeng.h"

#ifdef PFE_CFG_PFE_MASTER
static inline int pfeng_hwts_check_dup(struct pfeng_ndev *ndev,struct pfeng_ts_skb * new_entry)
{
	struct list_head *tmp = NULL, *curr = NULL;
	struct pfeng_ts_skb *ts_skb = NULL;

	list_for_each_safe(curr, tmp, &ndev->ts_skb_list) {
		ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
		if(new_entry->ref_num == ts_skb->ref_num) {
			netdev_err(ndev->netdev, "Duplicate ref_num %04x dropping skb\n", new_entry->ref_num);
			return -EINVAL;
		}
	}

	return EOK;
}

static void pfeng_hwts_work(struct work_struct *work)
{
	struct pfeng_ndev *ndev = container_of(work, struct pfeng_ndev, ts_tx_work);
	struct pfeng_ts_skb *ts_skb = NULL, *curr_ts_skb = NULL;
	struct list_head *tmp = NULL, *curr = NULL;
	struct pfeng_tx_ts tx_timestamp = { 0 };

	/* First extract all new skbs that are waiting for time stamp */
	if(!kfifo_is_empty(&ndev->ts_skb_fifo)) {
		do {
			if (ts_skb) {
				/* Check for duplicity juts to be sure */
				if (pfeng_hwts_check_dup(ndev, ts_skb)){
					kfree_skb(ts_skb->skb);
					kfree(ts_skb);
					ts_skb = NULL;
					continue;
				}
				list_add(&ts_skb->list, &ndev->ts_skb_list);
			}
			ts_skb = kmalloc(sizeof(struct pfeng_ts_skb), GFP_KERNEL);
		} while (0 != kfifo_get(&ndev->ts_skb_fifo, ts_skb));

		/* Free the unused member */
		kfree(ts_skb);
	}

	/* Now match all time stamps that were received */
	while (!kfifo_is_empty(&ndev->ts_tx_fifo) &&
	       (0 != kfifo_get(&ndev->ts_tx_fifo, &tx_timestamp))) {
		bool match = false;
		list_for_each_safe(curr, tmp, &ndev->ts_skb_list) {
			curr_ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
			if (curr_ts_skb->ref_num == tx_timestamp.ref_num) {
				match = true;
				skb_tstamp_tx(curr_ts_skb->skb, &tx_timestamp.ts);
				consume_skb(curr_ts_skb->skb);
				list_del(&curr_ts_skb->list);
				kfree(curr_ts_skb);
				break;
			}
		}

		if (false == match)
			netdev_err(ndev->netdev, "Dropping unknown TX time stamp with ref_num %04x\n", tx_timestamp.ref_num);
	}

	/* Here do aging (time stamp has to be available in less than 1ms but we will wait for 5ms) */
	list_for_each_safe(curr, tmp, &ndev->ts_skb_list) {
		curr_ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
		if (time_after(jiffies, curr_ts_skb->jif_enlisted + usecs_to_jiffies(5000U))) {
			netdev_warn(ndev->netdev, "Aging TX time stamp with ref_num %04x\n", curr_ts_skb->ref_num);
			kfree_skb(curr_ts_skb->skb);
			list_del(&curr_ts_skb->list);
			kfree(curr_ts_skb);
		}
	}
}

/* Store HW time stamp to skb */
void pfeng_hwts_skb_set_rx_ts(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	pfe_ct_hif_rx_hdr_t *hif_hdr = (pfe_ct_hif_rx_hdr_t *)skb->data;
	struct skb_shared_hwtstamps *hwts = skb_hwtstamps(skb);
	u64 nanos = 0ULL;

	memset(hwts, 0, sizeof(*hwts));
	nanos = hif_hdr->rx_timestamp_ns;
	nanos += hif_hdr->rx_timestamp_s * 1000000000ULL;
	hwts->hwtstamp = ns_to_ktime(nanos);
}

/* Store reference to tx skb that should be time stamped */
int pfeng_hwts_store_tx_ref(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	int ret = 1;
	struct pfeng_ts_skb ts_skb_entry;

	/* Store info for future timestamp */
	ts_skb_entry.skb = skb;
	ts_skb_entry.jif_enlisted = jiffies;
	ts_skb_entry.ref_num = ndev->ts_ref_num++ & 0x0FFFU;

	/* Send data to worker */
	ret = kfifo_put(&ndev->ts_skb_fifo, ts_skb_entry);
	if(0 == ret)
		return -ENOMEM;

	/* Increment reference counter (required to free the skb correctly)*/
	(void)skb_get(skb);
	schedule_work(&ndev->ts_tx_work);

	return ts_skb_entry.ref_num;
}

/* Store time stamp that should be matched with skb */
void pfeng_hwts_get_tx_ts(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	pfe_ct_ets_report_t *etsr = (pfe_ct_ets_report_t *)((addr_t)skb->data + sizeof(pfe_ct_hif_rx_hdr_t));
	struct pfeng_tx_ts tx_timestamp;
	int ret = 1;

	tx_timestamp.ts.hwtstamp = ns_to_ktime(etsr->ts_sec * 1000000000ULL + etsr->ts_nsec);
	tx_timestamp.ref_num = ntohs(etsr->ref_num) & 0x0FFFU;

	/* Send time stamp to worker */
	ret = kfifo_put(&ndev->ts_tx_fifo, tx_timestamp);
	schedule_work(&ndev->ts_tx_work);

	if(0 == ret)
		netdev_err(ndev->netdev, "No more memory. Time stamp dropped.\n");
}
#else /* PFE_CFG_PFE_MASTER */
void pfeng_hwts_skb_set_rx_ts(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	/* NOP */
}

void pfeng_hwts_get_rx_ts(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	/* NOP */
}

int pfeng_hwts_store_tx_ref(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	return -ENOMEM;
}

void pfeng_hwts_get_tx_ts(struct pfeng_ndev *ndev, struct sk_buff *skb)
{
	/* NOP */
}
static void pfeng_hwts_work(struct work_struct *work)
{
	/* NOP */
}
#endif /* else PFE_CFG_PFE_MASTER */

int pfeng_hwts_ioctl_set(struct pfeng_ndev *ndev, struct ifreq *rq)
{
	__maybe_unused struct pfeng_priv *priv = dev_get_drvdata(ndev->dev);
	struct hwtstamp_config cfg = { 0 };

	if (copy_from_user(&cfg, rq->ifr_data, sizeof(cfg)))
		return -EFAULT;

#ifdef PFE_CFG_PFE_MASTER
	if(!priv->ptp_reference_clk)
#endif
	{
		ndev->tshw_cfg.rx_filter = HWTSTAMP_FILTER_NONE;
		ndev->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;
		return copy_to_user(rq->ifr_data, &ndev->tshw_cfg, sizeof(cfg)) ? -EFAULT : 0;
	}
#ifdef PFE_CFG_PFE_MASTER
	else {
		switch (cfg.tx_type) {
			case HWTSTAMP_TX_OFF:
				ndev->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;
				break;
			case HWTSTAMP_TX_ON:
				ndev->tshw_cfg.tx_type = HWTSTAMP_TX_ON;
				break;
			default:
				return -ERANGE;
		}

		/* Following messages are currently time stamped
		 * SYNC, Follow_Up, Delay_Req, Delay_Resp*/
		switch (cfg.rx_filter) {
			case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
			case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
			case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
			case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
			case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
			case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
			case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
			case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
			case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
			case HWTSTAMP_FILTER_PTP_V2_EVENT:
			case HWTSTAMP_FILTER_PTP_V2_SYNC:
			case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
				/* We currently do not support specific filtering */
				ndev->tshw_cfg.rx_filter = cfg.rx_filter;
				break;
			case HWTSTAMP_FILTER_NONE:
				/* We always time stamp some messages on RX */
			case HWTSTAMP_FILTER_NTP_ALL:
				/* No NTP time stamp */
			case HWTSTAMP_FILTER_ALL:
				/* We do not time stamp all incoming packets */
			default:
				return -ERANGE;
		}

		return copy_to_user(rq->ifr_data, &ndev->tshw_cfg, sizeof(cfg)) ? -EFAULT : 0;
	}
#endif
}

int pfeng_hwts_ioctl_get(struct pfeng_ndev *ndev, struct ifreq *rq)
{
	return copy_to_user(rq->ifr_data, &ndev->tshw_cfg, sizeof(ndev->tshw_cfg)) ? -EFAULT : 0;
}

int pfeng_hwts_ethtool(struct pfeng_ndev *ndev, struct ethtool_ts_info *info)
{
	__maybe_unused struct pfeng_priv *priv = dev_get_drvdata(ndev->dev);

#ifdef PFE_CFG_PFE_MASTER
	if(!priv->ptp_reference_clk)
#endif
	{
		info->so_timestamping |= (SOF_TIMESTAMPING_TX_SOFTWARE |
					  SOF_TIMESTAMPING_RX_SOFTWARE |
					  SOF_TIMESTAMPING_SOFTWARE);

		info->tx_types = BIT(HWTSTAMP_TX_OFF);
	}
#ifdef PFE_CFG_PFE_MASTER
	else {
	info->so_timestamping |= (SOF_TIMESTAMPING_TX_HARDWARE |
				  SOF_TIMESTAMPING_RX_HARDWARE |
				  SOF_TIMESTAMPING_RAW_HARDWARE|
				  SOF_TIMESTAMPING_TX_SOFTWARE |
				  SOF_TIMESTAMPING_RX_SOFTWARE |
				  SOF_TIMESTAMPING_SOFTWARE);

	info->tx_types = BIT(HWTSTAMP_TX_ON) | BIT(HWTSTAMP_TX_OFF);

	info->rx_filters = BIT(HWTSTAMP_FILTER_SOME) |
			   BIT(HWTSTAMP_FILTER_NONE) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_EVENT) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_SYNC) |
			   BIT(HWTSTAMP_FILTER_PTP_V2_DELAY_REQ);
	}
#endif
	return 0;
}

int pfeng_hwts_init(struct pfeng_ndev *ndev)
{
#ifdef PFE_CFG_PFE_MASTER

	if (kfifo_alloc(&ndev->ts_skb_fifo, 32, GFP_KERNEL))
		return -ENOMEM;

	if (kfifo_alloc(&ndev->ts_tx_fifo, 32, GFP_KERNEL))
		return -ENOMEM;
#endif /* PFE_CFG_PFE_MASTER */

	/* Initialize for master and slave to have easier cleanup */
	INIT_LIST_HEAD(&ndev->ts_skb_list);
	INIT_WORK(&ndev->ts_tx_work, pfeng_hwts_work);

	/* Store default config */
	ndev->tshw_cfg.flags = 0;
	ndev->tshw_cfg.rx_filter = HWTSTAMP_FILTER_NONE;
	ndev->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;

	return 0;
}

void pfeng_hwts_release(struct pfeng_ndev *ndev)
{
	cancel_work_sync(&ndev->ts_tx_work);

	if (kfifo_initialized(&ndev->ts_skb_fifo))
		kfifo_free(&ndev->ts_skb_fifo);

	if (kfifo_initialized(&ndev->ts_tx_fifo))
		kfifo_free(&ndev->ts_tx_fifo);
}
