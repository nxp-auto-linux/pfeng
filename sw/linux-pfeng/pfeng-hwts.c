/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/net.h>
#include <linux/net_tstamp.h>

#include "pfeng.h"

static inline int pfeng_hwts_check_dup(struct pfeng_netif *netif,struct pfeng_ts_skb * new_entry)
{
	struct list_head *tmp = NULL, *curr = NULL;
	struct pfeng_ts_skb *ts_skb = NULL;

	list_for_each_safe(curr, tmp, &netif->ts_skb_list) {
		ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
		if(new_entry->ref_num == ts_skb->ref_num) {
			HM_MSG_NETDEV_ERR(netif->netdev, "Duplicate ref_num %04x dropping skb\n", new_entry->ref_num);
			return -EINVAL;
		}
	}

	return EOK;
}

static void pfeng_hwts_work(struct work_struct *work)
{
	struct pfeng_netif *netif = container_of(work, struct pfeng_netif, ts_tx_work);
	struct pfeng_ts_skb *ts_skb = NULL, *curr_ts_skb = NULL;
	struct list_head *tmp = NULL, *curr = NULL;
	struct pfeng_tx_ts tx_timestamp = { 0 };

	/* First extract all new skbs that are waiting for time stamp */
	if(!kfifo_is_empty(&netif->ts_skb_fifo)) {
		do {
			if (ts_skb) {
				/* Check for duplicity juts to be sure */
				if (pfeng_hwts_check_dup(netif, ts_skb)){
					/* Free socket buffer */
					kfree_skb(ts_skb->skb);
					/* Continue to get data from fifo into recycled ts_skb */
					continue;
				}
				list_add(&ts_skb->list, &netif->ts_skb_list);
			}
			ts_skb = kmalloc(sizeof(struct pfeng_ts_skb), GFP_KERNEL);
		} while (0 != kfifo_get(&netif->ts_skb_fifo, ts_skb));

		/* Free the unused member */
		kfree(ts_skb);
	}

	/* Now match all time stamps that were received */
	while (!kfifo_is_empty(&netif->ts_tx_fifo) &&
	       (0 != kfifo_get(&netif->ts_tx_fifo, &tx_timestamp))) {
		bool match = false;
		list_for_each_safe(curr, tmp, &netif->ts_skb_list) {
			curr_ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
			if (curr_ts_skb->ref_num == tx_timestamp.ref_num) {
				match = true;
				skb_pull(curr_ts_skb->skb, PFENG_TX_PKT_HEADER_SIZE);
				/* Pass skb to the kernel stack */
				skb_tstamp_tx(curr_ts_skb->skb, &tx_timestamp.ts);

				consume_skb(curr_ts_skb->skb);
				list_del(&curr_ts_skb->list);
				kfree(curr_ts_skb);
				break;
			}
		}

		if (false == match)
			HM_MSG_NETDEV_ERR(netif->netdev, "Dropping unknown TX time stamp with ref_num %04x\n", tx_timestamp.ref_num);
	}

	/* Here do aging (time stamp has to be available in less than 1ms but we will wait for 5ms) */
	list_for_each_safe(curr, tmp, &netif->ts_skb_list) {
		curr_ts_skb = list_entry(curr, struct pfeng_ts_skb, list);
		if (time_after(jiffies, curr_ts_skb->jif_enlisted + usecs_to_jiffies(5000U))) {
			HM_MSG_NETDEV_WARN(netif->netdev, "Aging TX time stamp with ref_num %04x\n", curr_ts_skb->ref_num);
			kfree_skb(curr_ts_skb->skb);
			list_del(&curr_ts_skb->list);
			kfree(curr_ts_skb);
		}
	}
}

/* Store reference to tx skb that should be time stamped */
int pfeng_hwts_store_tx_ref(struct pfeng_netif *netif, struct sk_buff *skb)
{
	int ret = 1;
	/* Store info for future timestamp */
	struct pfeng_ts_skb ts_skb_entry = {
		.skb = skb,
		.jif_enlisted = jiffies,
		.ref_num = netif->ts_ref_num++ & 0x0FFFU
	};

	if (!netif->ts_work_on)
		return -EINVAL;

	/* Send data to worker */
	ret = kfifo_put(&netif->ts_skb_fifo, ts_skb_entry);
	if(0 == ret)
		return -ENOMEM;

	/* Increment reference counter (required to free the skb correctly)*/
	(void)skb_get(skb);
	schedule_work(&netif->ts_tx_work);

	return ts_skb_entry.ref_num;
}

/* Store time stamp that should be matched with skb */
void pfeng_hwts_get_tx_ts(struct pfeng_netif *netif, pfe_ct_ets_report_t *etsr)
{
	struct pfeng_tx_ts tx_timestamp;
	int ret = 1;

	if (!netif->ts_work_on)
		return;

	tx_timestamp.ts.hwtstamp = ns_to_ktime(etsr->ts_sec * 1000000000ULL + etsr->ts_nsec);
	tx_timestamp.ref_num = ntohs(etsr->ref_num) & 0x0FFFU;

	/* Send time stamp to worker */
	ret = kfifo_put(&netif->ts_tx_fifo, tx_timestamp);
	schedule_work(&netif->ts_tx_work);

	if(0 == ret)
		HM_MSG_NETDEV_ERR(netif->netdev, "No more memory. Time stamp dropped.\n");
}

int pfeng_hwts_ioctl_set(struct pfeng_netif *netif, struct ifreq *rq)
{
	struct hwtstamp_config cfg = { 0 };

	if (!netif->ts_work_on)
		return -EINVAL;

	if (copy_from_user(&cfg, rq->ifr_data, sizeof(cfg)))
		return -EFAULT;

	if (!netif->priv->clk_ptp_reference)
	{
		netif->tshw_cfg.rx_filter = HWTSTAMP_FILTER_NONE;
		netif->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;
		return copy_to_user(rq->ifr_data, &netif->tshw_cfg, sizeof(struct hwtstamp_config)) ? -EFAULT : 0;
	}

	switch (cfg.tx_type) {
	case HWTSTAMP_TX_OFF:
		netif->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;
		break;
	case HWTSTAMP_TX_ON:
		netif->tshw_cfg.tx_type = HWTSTAMP_TX_ON;
		break;
	default:
		return -ERANGE;
	}

	/* Following messages are currently time stamped
	 * SYNC, Follow_Up, Delay_Req, Delay_Resp*/
	switch (cfg.rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		netif->tshw_cfg.rx_filter = HWTSTAMP_FILTER_NONE;
		break;
	default:
		netif->tshw_cfg.rx_filter = HWTSTAMP_FILTER_ALL;
		break;
	}

	return copy_to_user(rq->ifr_data, &netif->tshw_cfg, sizeof(cfg)) ? -EFAULT : 0;
}

int pfeng_hwts_ioctl_get(struct pfeng_netif *netif, struct ifreq *rq)
{
	if (!netif->ts_work_on)
		return -EINVAL;

	return copy_to_user(rq->ifr_data, &netif->tshw_cfg, sizeof(netif->tshw_cfg)) ? -EFAULT : 0;
}

int pfeng_hwts_ethtool(struct pfeng_netif *netif, struct ethtool_ts_info *info)
{
	if (!netif->priv->clk_ptp_reference || pfeng_netif_is_aux(netif)) {
		info->so_timestamping |= SOF_TIMESTAMPING_TX_SOFTWARE |
					 SOF_TIMESTAMPING_RX_SOFTWARE |
					 SOF_TIMESTAMPING_SOFTWARE;
		info->tx_types = BIT(HWTSTAMP_TX_OFF);
	} else {
		info->so_timestamping |= SOF_TIMESTAMPING_TX_HARDWARE |
					 SOF_TIMESTAMPING_RX_HARDWARE |
					 SOF_TIMESTAMPING_RAW_HARDWARE |
					 SOF_TIMESTAMPING_TX_SOFTWARE |
					 SOF_TIMESTAMPING_RX_SOFTWARE |
					 SOF_TIMESTAMPING_SOFTWARE;
		info->tx_types = BIT(HWTSTAMP_TX_ON) | BIT(HWTSTAMP_TX_OFF);
		info->rx_filters = BIT(HWTSTAMP_FILTER_ALL) |
				   BIT(HWTSTAMP_FILTER_NONE);
	}

	return 0;
}

int pfeng_hwts_init(struct pfeng_netif *netif)
{
	if (kfifo_alloc(&netif->ts_skb_fifo, 32, GFP_KERNEL))
		return -ENOMEM;

	if (kfifo_alloc(&netif->ts_tx_fifo, 32, GFP_KERNEL))
		return -ENOMEM;

	/* Initialize for master and slave to have easier cleanup */
	INIT_LIST_HEAD(&netif->ts_skb_list);
	INIT_WORK(&netif->ts_tx_work, pfeng_hwts_work);
	netif->ts_work_on = true;

	/* Store default config */
	netif->tshw_cfg.flags = 0;
	netif->tshw_cfg.rx_filter = HWTSTAMP_FILTER_NONE;
	netif->tshw_cfg.tx_type = HWTSTAMP_TX_OFF;

	return 0;
}

void pfeng_hwts_release(struct pfeng_netif *netif)
{
	if (netif->ts_work_on) {
		cancel_work_sync(&netif->ts_tx_work);
		netif->ts_work_on = false;
	}

	if (kfifo_initialized(&netif->ts_skb_fifo))
		kfifo_free(&netif->ts_skb_fifo);

	if (kfifo_initialized(&netif->ts_tx_fifo))
		kfifo_free(&netif->ts_tx_fifo);
}
