/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_feature_mgr.h"
#include "pfe_phy_if.h"
#include "pfe_tmu.h"

struct pfe_tmu_tag
{
	addr_t cbus_base_va;
	pfe_class_t *class;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_tmu_set_queue_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue,
									  pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max, uint16_t sum);

/**
 * @brief		Check whether the provided physical interface ID represents HIF-type interface or not.
 * @details		Optionally, for HIF-type interfaces the function can also provide index of the given HIF
 *				within  pfe_ct_hif_tmu_queue_sizes_t  (for FW feature err051211_workaround).
 * @param[in]	id	Interface ID
 * @param[out]	err051211_hif_id	[optional] Index of the HIF interface in  pfe_ct_hif_tmu_queue_sizes_t. Can be NULL.
 * @return		TRUE if the provided ID represents HIF-type physical interface.
 */
static bool is_hif_by_id(pfe_ct_phy_if_id_t id, uint8_t *err051211_hif_idx)
{
	bool ret = FALSE;
	uint8_t hif_idx;

	/* Check that indexes 0 .. 3 can be assigned */
	ct_assert((sizeof(pfe_ct_hif_tmu_queue_sizes_t) / sizeof(uint16_t)) == 4U);

	switch (id)
	{
		case PFE_PHY_IF_ID_HIF0:
			hif_idx = 0U;
			ret = TRUE;
			break;
		case PFE_PHY_IF_ID_HIF1:
			hif_idx = 1U;
			ret = TRUE;
			break;
		case PFE_PHY_IF_ID_HIF2:
			hif_idx = 2U;
			ret = TRUE;
			break;
		case PFE_PHY_IF_ID_HIF3:
			hif_idx = 3U;
			ret = TRUE;
			break;
		default:
			ret = FALSE;
			hif_idx = 0U;
			break;
	}

	if (NULL != err051211_hif_idx)
	{
		*err051211_hif_idx = hif_idx;
	}

	return ret;
}

/**
 * @brief		Compute new sum of queue lenghts for the given physical interface.
 * 				Also check that the new sum does not exceed any applicable limitations.
 * @param[in]	tmu		The TMU instance
 * @parma[in]	phy		Physical interface ID
 * @param[in]	queue	The queue ID
 * @param[in]	max		New max threshold (number of packets) for the queue
 * @param[out]	sum		Pointer to result of sum calculation (passback)
 * @return		EOK if computation and all applicable checks are OK.
 */
static errno_t get_sum_of_queue_lengths(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t queue, uint32_t max, uint16_t* sum)
{
	errno_t ret_val = ENOSPC;
	uint32_t tmp_sum = 0U;
	uint32_t tmp_min = 0U;
	uint32_t tmp_max = 0U;
	uint8_t i;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == sum)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Compute new sum of queue lengths */
		const uint8_t cnt = pfe_tmu_queue_get_cnt(tmu, phy);
		for (i = 0U; (i < cnt); i++)
		{
			if (i == queue)
			{
				tmp_sum += max;	/* For the queue-to-be-changed, apply its new max threshhold */
			}
			else
			{
				(void)pfe_tmu_queue_get_mode(tmu, phy, i, &tmp_min, &tmp_max);
				tmp_sum += tmp_max;
			}
		}

		/* Check the new sum */
		if (TRUE == is_hif_by_id(phy, NULL))
		{	/* HIF */
			if (TLITE_HIF_MAX_ENTRIES < tmp_sum)
			{
				NXP_LOG_ERROR("Sum of queue lengths (%u) exceeds max allowed sum (%u) for HIF interface.", (uint_t)tmp_sum, TLITE_HIF_MAX_ENTRIES);
				ret_val = ENOSPC;
			}
			else if ((TRUE == pfe_feature_mgr_is_available("err051211_workaround")) &&
					 (PFE_HIF_RX_RING_CFG_LENGTH < (tmp_sum + PFE_TMU_ERR051211_Q_OFFSET)))
			{
				NXP_LOG_ERROR("err051211_workaround is active and \"sum of queue lengths (%u) + Q_OFFSET (%u)\" exceeds HIF RX Ring length (%u).", (uint_t)tmp_sum, PFE_TMU_ERR051211_Q_OFFSET, PFE_HIF_RX_RING_CFG_LENGTH);
				ret_val = ENOSPC;
			}
			else
			{
				ret_val = EOK;
			}
		}
		else
		{	/* EMAC and 'others' */
			if (TLITE_MAX_ENTRIES < tmp_sum)
			{
				NXP_LOG_ERROR("Sum of queue lengths (%u) exceeds max allowed sum (%u) for EMAC/UTIL/HIF_NOCPY interface.", (uint_t)tmp_sum, TLITE_MAX_ENTRIES);
				ret_val = ENOSPC;
			}
			else
			{
				ret_val = EOK;
			}
		}

		/* Set passback value */
		*sum = (uint16_t)tmp_sum;
	}

	return ret_val;
}

/**
 * @brief		Set all TMU queues of the target physical interface to minimal possible lengths.
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @return		EOK if success, error code otherwise
 */
static errno_t set_all_queues_to_min_length(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy)
{
	errno_t ret_val = EINVAL;
	uint8_t queue;
	uint8_t queue_cnt = 0U;

	pfe_tmu_queue_mode_t mode = TMU_Q_MODE_TAIL_DROP;
	uint32_t min = 0UL;
	uint32_t max = 0UL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		queue_cnt = pfe_tmu_queue_get_cnt(tmu, phy);
		for (queue = 0U; (queue_cnt > queue); queue++)
		{
			if (EOK != pfe_tmu_check_queue(tmu, phy, queue))
			{
				ret_val = EINVAL;
			}
			else
			{
				mode = pfe_tmu_queue_get_mode(tmu, phy, queue, &min, &max);
				switch (mode)
				{
					case TMU_Q_MODE_TAIL_DROP:
					{
						ret_val = pfe_tmu_q_mode_set_tail_drop(tmu->cbus_base_va, phy, queue, 1U);
						break;
					}

					case TMU_Q_MODE_WRED:
					{
						ret_val = pfe_tmu_q_mode_set_wred(tmu->cbus_base_va, phy, queue, 0U, 1U);
						break;
					}

					case TMU_Q_MODE_DEFAULT:
					{
						ret_val = pfe_tmu_q_mode_set_default(tmu->cbus_base_va, phy, queue);
						break;
					}

					default:
					{
						NXP_LOG_ERROR("Unknown queue mode: %d\n", mode);
						ret_val = EINVAL;
						break;
					}
				}
			}
		}
	}
	return ret_val;
}

/**
 * @brief		Set the configuration of the TMU block.
 * @param[in]	tmu The TMU instance
 * @param[in]	cfg Pointer to the configuration structure
 */
static void pfe_tmu_init(const pfe_tmu_t *tmu, const pfe_tmu_cfg_t *cfg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_tmu_disable(tmu);

		if (EOK != pfe_tmu_cfg_init(tmu->cbus_base_va, cfg))
		{
			NXP_LOG_ERROR("Couldn't initialize the TMU\n");
		}
	}
}

/**
 * @brief		Set queue mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[in]	mode Mode
 * @param[in]	min Min threshold (number of packets)
 * @param[in]	max Max threshold (number of packets)
 * @param[in]	sum Sum of queue lengths
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_tmu_set_queue_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue,
							  pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max, uint16_t sum)
{
	errno_t ret_val = EOK;
	uint16_t sum_tmp = 0U;
	uint8_t err051211_hif_idx = 0xFF;	/* Index of HIF in  pfe_ct_hif_tmu_queue_sizes_t */

	/* If err051211_workaround is active and queue of some HIF was modified, then update sum of queue lengths in firmware. */
	if ((TRUE == is_hif_by_id(phy, &err051211_hif_idx)) &&
		(TRUE == pfe_feature_mgr_is_available("err051211_workaround")))
	{
		pfe_ct_class_mmap_t mmap = {0};
		ret_val = pfe_class_get_mmap(tmu->class, 0, &mmap);
		if (EOK == ret_val)
		{
			const uint32_t addr = oal_ntohl(mmap.hif_tmu_queue_sizes) + (err051211_hif_idx * sizeof(uint16_t));
			sum_tmp = oal_htons(sum);
			ret_val = pfe_class_write_dmem(tmu->class, -1, addr, (void *)&sum_tmp, sizeof(uint16_t));
		}
	}

	/* Set new tmu configuration */
	if (EOK == ret_val)
	{
		switch (mode)
		{
			case TMU_Q_MODE_TAIL_DROP:
			{
				ret_val = pfe_tmu_q_mode_set_tail_drop(tmu->cbus_base_va, phy, queue, (uint16_t)max);
				break;
			}

			case TMU_Q_MODE_WRED:
			{
				ret_val = pfe_tmu_q_mode_set_wred(tmu->cbus_base_va, phy, queue, (uint16_t)min, (uint16_t)max);
				break;
			}

			case TMU_Q_MODE_DEFAULT:
			{
				ret_val = pfe_tmu_q_mode_set_default(tmu->cbus_base_va, phy, queue);
				break;
			}

			default:
			{
				NXP_LOG_ERROR("Unknown queue mode: %d\n", mode);
				ret_val = EINVAL;
				break;
			}
		}
	}

	return ret_val;
}

/**
* @brief Create new TMU instance
* @details		Creates and initializes TMU instance. After successful
* 				call the TMU is configured and disabled.
* @param[in]	cbus_base_va CBUS base virtual address
* @param[in]	pe_num Number of PEs to be included
* @param[in]	cfg The TMU block configuration
* @param[in]	class Classifier instance
* @return		The TMU instance or NULL if failed
*/
pfe_tmu_t *pfe_tmu_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_tmu_cfg_t *cfg,
						  pfe_class_t *class)
{
	pfe_tmu_t *tmu;
	(void)pe_num;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == cbus_base_va) || (NULL == cfg) || (NULL == class)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		tmu = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		tmu = oal_mm_malloc(sizeof(pfe_tmu_t));

		if (NULL != tmu)
		{
			(void)memset(tmu, 0, sizeof(pfe_tmu_t));
			tmu->cbus_base_va = cbus_base_va;
			tmu->class = class;

			/*	Issue block reset */
			pfe_tmu_reset(tmu);

			/* Initialize reclaim memory */
			pfe_tmu_reclaim_init(cbus_base_va);

			/*	Disable the TMU */
			pfe_tmu_disable(tmu);

			/*	Set new configuration */
			pfe_tmu_init(tmu, cfg);
		}
		else
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
	}

	return tmu;
}

/**
 * @brief		Reset the TMU block
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_reset(const pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_tmu_cfg_reset(tmu->cbus_base_va);
	}
}

/**
 * @brief		Enable the TMU block
 * @details		Enable all TMU PEs
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_enable(const pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_tmu_cfg_enable(tmu->cbus_base_va);
	}
}

/**
 * @brief		Disable the TMU block
 * @details		Disable all TMU PEs
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_disable(const pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_tmu_cfg_disable(tmu->cbus_base_va);
	}
}

/**
 * @brief		Destroy TMU instance
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_destroy(const pfe_tmu_t *tmu)
{
	if (NULL != tmu)
	{
		pfe_tmu_disable(tmu);
		oal_mm_free(tmu);
	}
}

/*
 * @brief		Check if phy+queue combination is valid
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @param[in]	queue Queue ID
 * @return		EOK if the arguments are valid
 */
errno_t pfe_tmu_check_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	const pfe_tmu_phy_cfg_t *pcfg;
	errno_t ret;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_WARNING("Invalid phy: %d\n", phy);
		ret = EINVAL;
	}
	else
	{
		if ((queue >= pcfg->q_cnt) && (queue != PFE_TMU_INVALID_QUEUE))
		{
			NXP_LOG_WARNING("Invalid queue ID (%d). PHY %d implements %d queues\n",
					queue, phy, pcfg->q_cnt);
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}

	return ret;
}

/*
 * @brief		Check if phy+scheduler combination is valid
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @param[in]	sch Scheduler ID
 * @return		EOK if the arguments are valid
 */
errno_t pfe_tmu_check_scheduler(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	const pfe_tmu_phy_cfg_t *pcfg;
	errno_t ret;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_WARNING("Invalid phy: %d\n", (int_t)phy);
		ret = EINVAL;
	}
	else
	{
		if (sch >= pcfg->sch_cnt)
		{
			NXP_LOG_WARNING("Invalid scheduler ID (%d). PHY %d implements %d schedulers\n",
					sch, phy, pcfg->sch_cnt);
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Check if phy+shaper combination is valid
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @param[in]	shp Shaper ID
 * @return		EOK if the arguments are valid
 */
errno_t pfe_tmu_check_shaper(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	const pfe_tmu_phy_cfg_t *pcfg;
	errno_t ret;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_WARNING("Invalid phy: %d\n", (int_t)phy);
		ret = EINVAL;
	}
	else
	{
		if (shp >= pcfg->shp_cnt)
		{
			NXP_LOG_WARNING("Invalid shaper ID (%d). PHY %d implements %d shapers\n",
					shp, phy, pcfg->shp_cnt);
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Get number of packets in the queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the fill level value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_fill_level(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == level)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret = pfe_tmu_q_cfg_get_fill_level(tmu->cbus_base_va, phy, queue, level);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get number of packet dropped by queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_drop_count(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret = pfe_tmu_q_cfg_get_drop_count(tmu->cbus_base_va, phy, queue, cnt);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get number of packet transmitted from queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_tx_count(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret = pfe_tmu_q_cfg_get_tx_count(tmu->cbus_base_va, phy, queue, cnt);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Set queue mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[in]	mode Mode
 * @param[in]	min Min threshold (number of packets)
 * @param[in]	max Max threshold (number of packets)
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_set_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue,
		pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max)
{
	errno_t ret_val = EOK;
	uint16_t sum = 0U;	/* Sum of queue lengths */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Check and set mode + lengths */
		if (min > max)
		{
			NXP_LOG_ERROR("Wrong queue lengths: min queue length (%u) is larger than max queue length (%u)\n", (uint_t)min, (uint_t)max);
			ret_val = EINVAL;
		}
		else if (EOK != pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret_val = EINVAL;
		}
		else if (EOK != get_sum_of_queue_lengths(tmu, phy, queue, max, &sum))
		{
			ret_val = ENOSPC;
		}
		else
		{
			ret_val = pfe_tmu_set_queue_mode(tmu, phy, queue, mode, min, max, sum);
		}
	}

	return ret_val;
}

/**
 * @brief		Get queue mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[in]	mode Mode
 * @param[in]	min Pointer to memory where 'min' value shall be written
 * @param[in]	max Pointer to memory where 'max' value shall be written
 * @return		EOK if success, error code otherwise
 */
pfe_tmu_queue_mode_t pfe_tmu_queue_get_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t queue, uint32_t *min, uint32_t *max)
{
	pfe_tmu_queue_mode_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = TMU_Q_MODE_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret = pfe_tmu_q_get_mode(tmu->cbus_base_va, phy, queue, min, max);
		}
		else
		{
			ret = TMU_Q_MODE_INVALID;
		}
	}
	return ret;
}

/**
 * @brief		Set WRED zone probability
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[in]	zone Zone index
 * @param[in]	prob Drop probability in [%]
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_set_wred_prob(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			if (zone >= pfe_tmu_queue_get_wred_zones(tmu, phy, queue))
			{
				NXP_LOG_WARNING("Zone index out of range\n");
				ret = EINVAL;
			}
			else if (prob > 100U)
			{
				NXP_LOG_WARNING("Probability out of range\n");
				ret = EINVAL;
			}
			else
			{
				ret = pfe_tmu_q_set_wred_probability(tmu->cbus_base_va, phy, queue, zone, prob);
			}
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get WRED zone probability
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[in]	zone Zone index
 * @param[in]	prob Poiter to memory where drop probability in [%] shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_wred_prob(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			if (zone >= pfe_tmu_queue_get_wred_zones(tmu, phy, queue))
			{
				NXP_LOG_WARNING("Zone index out of range\n");
				ret = EINVAL;
			}
			else
			{
				ret = pfe_tmu_q_get_wred_probability(tmu->cbus_base_va, phy, queue, zone, prob);
			}
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get number of WRED probability zones
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @return		Number of zones between 'min' and 'max'
 */
uint8_t pfe_tmu_queue_get_wred_zones(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	uint8_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
		{
			ret = pfe_tmu_q_get_wred_zones(tmu->cbus_base_va, phy, queue);
		}
		else
		{
			ret = 0U;
		}
	}

	return ret;
}

errno_t pfe_tmu_queue_reset_tail_drop_policy(const pfe_tmu_t *tmu)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_tmu_q_reset_tail_drop_policy(tmu->cbus_base_va);
	}

	return ret;
}

/**
 * @brief		Enforce compliance of queue length sums of all HIF interfaces with
 * 				err051211_workaround constraints. Also update data in FW.
 * @param[in]	tmu The TMU instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_err051211_sync(const pfe_tmu_t *tmu)
{
	pfe_ct_phy_if_id_t phy = PFE_PHY_IF_ID_HIF0;
	pfe_tmu_queue_mode_t mode = TMU_Q_MODE_TAIL_DROP;
	uint32_t min = 0UL;
	uint32_t max = 0UL;
	uint32_t default_max = 0UL;
	uint16_t sum = 0U;	/* Sum of queue lengths */
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Pre-compute safe default HIF queue length (in case it is needed). Consider the following two limits:
				--> Size of HIF RX Ring
				--> Max allowed queue size for HIF */
		default_max = (PFE_HIF_RX_RING_CFG_LENGTH >= PFE_TMU_ERR051211_MINIMAL_REQUIRED_RX_RING_LENGTH) ? (((uint32_t)PFE_HIF_RX_RING_CFG_LENGTH - PFE_TMU_ERR051211_Q_OFFSET) / 2U) : (1U);
		default_max = (default_max >= TLITE_HIF_MAX_Q_SIZE) ? (TLITE_HIF_MAX_Q_SIZE) : (default_max);

		/* Check all HIF interfaces and update data in FW. */
		for (phy = PFE_PHY_IF_ID_HIF0; (PFE_PHY_IF_ID_HIF3 >= phy); phy = (pfe_ct_phy_if_id_t)((uint16_t)phy + 1U))
		{
			uint8_t queue = 0U;
			const uint8_t queue_cnt = pfe_tmu_queue_get_cnt(tmu, phy);

			/* Check sum of queue lengths for the given HIF ; 0xFF args ensure that real current sum is returned */
			if (ENOSPC == get_sum_of_queue_lengths(tmu, phy, 0xFF, 0xFF, &sum))
			{
				/* Reset queue lengths and then set them all to default_max length. This will update data in FW as well. */
				(void)set_all_queues_to_min_length(tmu, phy);
				for (queue = 0U; (queue_cnt > queue); queue++)
				{
					mode = pfe_tmu_queue_get_mode(tmu, phy, queue, &min, &max);
					(void)pfe_tmu_queue_set_mode(tmu, phy, queue, mode, min, default_max);
				}

				NXP_LOG_WARNING("Every TMU queue of physical interface id=%d was set to length %u, because err051211_workaround got activated.",
								phy, (uint_t)default_max);
				NXP_LOG_WARNING("\"Original sum of queue lengths (%u) + Q_OFFSET (%u)\" for the given interface was exceeding HIF RX Ring length (%u).",
								(uint_t)sum, (uint_t)PFE_TMU_ERR051211_Q_OFFSET, (uint_t)PFE_HIF_RX_RING_CFG_LENGTH);
			}
			else
			{
				/* Sum is OK. Simply reapply parameters. This will update data in FW as well. */
				for (queue = 0U; (queue_cnt > queue); queue++)
				{
					mode = pfe_tmu_queue_get_mode(tmu, phy, queue, &min, &max);
					(void)pfe_tmu_queue_set_mode(tmu, phy, queue, mode, min, max);
				}
			}
		}

		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get number of queues for given physical interface
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @return		Number of queues
 */
uint8_t pfe_tmu_queue_get_cnt(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy)
{
	const pfe_tmu_phy_cfg_t *pcfg;
	uint8_t ret;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_ERROR("Invalid phy: 0x%x\n", phy);
		ret = 0U;
	}
	else
	{
		ret = pcfg->q_cnt;
	}

	return ret;
}

/**
 * @brief		Set shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	max_credit Maximum credit value
 * @param[in]	min_credit Minimum credit value
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_set_limits(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t max_credit, int32_t min_credit)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_set_limits(tmu->cbus_base_va, phy, shp, max_credit, min_credit);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[out]	max_credit Pointer to memory where maximum credit value shall be written
 * @param[out]	min_credit Pointer to memory where minimum credit value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_get_limits(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t *max_credit, int32_t *min_credit)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu) || unlikely(NULL == max_credit) || unlikely(NULL == min_credit))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_get_limits(tmu->cbus_base_va, phy, shp, max_credit, min_credit);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Set shaper position within the QoS topology
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	pos Shaper position. Setting to PFE_TMU_INVALID_POSITION makes
 *					the shaper unused.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_set_position(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_set_position(tmu->cbus_base_va, phy, shp, pos);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get shaper position within the QoS topology
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	pos Shaper position. Setting to PFE_TMU_INVALID_POSITION makes
 *					the shaper unused.
 * @return		EOK if success, error code otherwise
 */
uint8_t pfe_tmu_shp_get_position(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint8_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = PFE_TMU_INVALID_POSITION;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_get_position(tmu->cbus_base_va, phy, shp);
		}
		else
		{
			ret = PFE_TMU_INVALID_POSITION;
		}
	}

	return ret;
}

/**
 * @brief		Enable shaper
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 */
errno_t pfe_tmu_shp_enable(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_enable(tmu->cbus_base_va, phy, shp);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Set shaper rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	mode Shaper mode
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_set_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_set_rate_mode(tmu->cbus_base_va, phy, shp, mode);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get shaper rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Shaper rate mode
 */
pfe_tmu_rate_mode_t pfe_tmu_shp_get_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	pfe_tmu_rate_mode_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = RATE_MODE_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_get_rate_mode(tmu->cbus_base_va, phy, shp);
		}
		else
		{
			ret = RATE_MODE_INVALID;
		}
	}

	return ret;
}

/**
 * @brief		Set shaper idle slope
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_set_idle_slope(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_set_idle_slope(tmu->cbus_base_va, phy, shp, isl);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get shaper idle slope
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Current idle slope value
 */
uint32_t pfe_tmu_shp_get_idle_slope(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			ret = pfe_tmu_shp_cfg_get_idle_slope(tmu->cbus_base_va, phy, shp);
		}
		else
		{
			ret = 0U;
		}
	}

	return ret;
}

/**
 * @brief		Disable shaper
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 */
errno_t pfe_tmu_shp_disable(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
		{
			pfe_tmu_shp_cfg_disable(tmu->cbus_base_va, phy, shp);
			ret = EOK;
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Set scheduler rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	mode The rate mode to be used by scheduler
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_rate_mode_t mode)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_set_rate_mode(tmu->cbus_base_va, phy, sch, mode);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief	Get scheduler rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Current rate mode or RATE_MODE_INVALID in case of error
 */
pfe_tmu_rate_mode_t pfe_tmu_sch_get_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	pfe_tmu_rate_mode_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = RATE_MODE_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_get_rate_mode(tmu->cbus_base_va, phy, sch);
		}
		else
		{
			ret = RATE_MODE_INVALID;
		}
	}

	return ret;
}

/**
 * @brief		Set scheduler algorithm
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_algo(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_sched_algo_t algo)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_set_algo(tmu->cbus_base_va, phy, sch, algo);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get scheduler algorithm
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Current rate mode or SCHED_ALGO_INVALID in case of error
 */
pfe_tmu_sched_algo_t pfe_tmu_sch_get_algo(const pfe_tmu_t *tmu,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	pfe_tmu_sched_algo_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = SCHED_ALGO_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_get_algo(tmu->cbus_base_va, phy, sch);
		}
		else
		{
			ret = SCHED_ALGO_INVALID;
		}
	}

	return ret;
}

/**
 * @brief		Get number of scheduler inputs
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Number of scheduler inputs
 */
uint8_t pfe_tmu_sch_get_input_cnt(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	uint8_t ret;

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		/*	Number of scheduler inputs is equal to number of available queues */
		ret = pfe_tmu_queue_get_cnt(tmu, phy);
	}
	else
	{
		ret = 0U;
	}

	return ret;
}

/**
 * @brief		Set scheduler input weight
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input
 * @param[in]	weight The weight value to be used by chosen scheduling algorithm
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_input_weight(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, uint8_t input, uint32_t weight)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_set_input_weight(tmu->cbus_base_va,
				phy, sch, input, weight);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get scheduler input weight
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input
 * @return		Input weight
 */
uint32_t pfe_tmu_sch_get_input_weight(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	uint32_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_get_input_weight(tmu->cbus_base_va,
					phy, sch, input);
		}
		else
		{
			ret = 0U;
		}
	}

	return ret;
}

/**
 * @brief		Connect another scheduler output to some scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	src_sch Source scheduler instance/index
 * @param[in]	dst_sch Destination scheduler instance/index
 * @param[in]	input Input of 'dst_sch' where output of 'src_sch' shall be connected
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_bind_sch_output(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((EOK == pfe_tmu_check_scheduler(tmu, phy, src_sch))
				&& (EOK == pfe_tmu_check_scheduler(tmu, phy, dst_sch)))
		{
			ret = pfe_tmu_sch_cfg_bind_sched_output(tmu->cbus_base_va, phy, src_sch, dst_sch, input);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get scheduler which output is connected to given scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input
 * @return		ID of the connected scheduler or PFE_TMU_INVALID_SCHEDULER
 */
uint8_t pfe_tmu_sch_get_bound_sch_output(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	uint8_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = PFE_TMU_INVALID_SCHEDULER;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_get_bound_sched_output(tmu->cbus_base_va, phy, sch, input);
		}
		else
		{
			ret = PFE_TMU_INVALID_SCHEDULER;
		}
	}

	return ret;
}

/**
 * @brief		Connect queue to some scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input the queue shall be connected to
 * @param[in]	queue Queue to be connected to the scheduler input
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_bind_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, uint8_t input, uint8_t queue)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
				&& (EOK == pfe_tmu_check_queue(tmu, phy, queue)))
		{
			ret = pfe_tmu_sch_cfg_bind_queue(tmu->cbus_base_va, phy, sch, input, queue);
		}
		else
		{
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Get queue connected to given scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input to be queried
 * @return		Queue ID connected to the input or PFE_TMU_INVALID_QUEUE if not present
 */
uint8_t pfe_tmu_sch_get_bound_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	uint8_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = PFE_TMU_INVALID_QUEUE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
		{
			ret = pfe_tmu_sch_cfg_get_bound_queue(tmu->cbus_base_va, phy, sch, input);
		}
		else
		{
			ret = PFE_TMU_INVALID_QUEUE;
		}
	}

	return ret;
}

/**
 * @brief		Return TMU runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	tmu The TMU instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_tmu_get_text_statistics(const pfe_tmu_t *tmu, struct seq_file *seq, uint8_t verb_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_tmu_cfg_get_text_stat(tmu->cbus_base_va, seq, verb_level);
	}

	return 0;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

