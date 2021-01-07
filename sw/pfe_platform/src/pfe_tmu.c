/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_pe.h"
#include "pfe_tmu.h"

struct __pfe_tmu_tag
{
	void *cbus_base_va;
};

/*	Scheduler instance */
struct __pfe_tmu_sch_tag
{
	void *cbus_base_va;			/*	CBUS base virtual address */
	void *sch_base_va;			/*	Scheduler base address */
};

/*	Shaper instance */
struct __pfe_tmu_shp_tag
{
	void *cbus_base_va;			/*	CBUS base virtual address */
	void *shp_base_va;			/*	Shaper base address */
};

/**
 * @brief		Set the configuration of the TMU block.
 * @param[in]	tmu The TMU instance
 * @param[in]	cfg Pointer to the configuration structure
 */
static void pfe_tmu_init(pfe_tmu_t *tmu, pfe_tmu_cfg_t *cfg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_disable(tmu);
	oal_time_mdelay(10);
	
	if (EOK != pfe_tmu_cfg_init(tmu->cbus_base_va, cfg))
	{
		NXP_LOG_ERROR("Couldn't initialize the TMU\n");
		return;
	}
}

/**
 * @brief		Create new TMU instance
 * @details		Creates and initializes TMU instance. After successful
 * 				call the TMU is configured and disabled.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	pe_num Number of PEs to be included
 * @param[in]	cfg The TMU block configuration
 * @return		The TMU instance or NULL if failed
 */
pfe_tmu_t *pfe_tmu_create(void *cbus_base_va, uint32_t pe_num, pfe_tmu_cfg_t *cfg)
{
	pfe_tmu_t *tmu;
	(void)pe_num;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	tmu = oal_mm_malloc(sizeof(pfe_tmu_t));
	
	if (NULL == tmu)
	{
		return NULL;
	}
	else
	{
		memset(tmu, 0, sizeof(pfe_tmu_t));
		tmu->cbus_base_va = cbus_base_va;
	}
	
	/*	Issue block reset */
	pfe_tmu_reset(tmu);
	
	/*	Disable the TMU */
	pfe_tmu_disable(tmu);
	
	/*	Set new configuration */
	pfe_tmu_init(tmu, cfg);
	
	return tmu;
}

/**
 * @brief		Reset the TMU block
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_reset(pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_cfg_reset(tmu->cbus_base_va);
}

/**
 * @brief		Enable the TMU block
 * @details		Enable all TMU PEs
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_enable(pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_cfg_enable(tmu->cbus_base_va);
}

/**
 * @brief		Disable the TMU block
 * @details		Disable all TMU PEs
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_disable(pfe_tmu_t *tmu)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_cfg_disable(tmu->cbus_base_va);
}

/**
 * @brief		Send packet to HIF directly via TMU
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface identifier
 * @param[in]	queue TX queue identifier
 * @param[in]	buf_pa Buffer physical address
 * @param[in]	len Number of bytes to send
 */
void pfe_tmu_send(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, void *buf_pa, uint16_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_cfg_send_pkt(tmu->cbus_base_va, phy, queue, buf_pa, len);
}

/**
 * @brief		Destroy TMU instance
 * @param[in]	tmu The TMU instance
 */
void pfe_tmu_destroy(pfe_tmu_t *tmu)
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
static errno_t pfe_tmu_check_queue(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	const pfe_tmu_phy_cfg_t *pcfg;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_ERROR("Invalid phy: %d\n", phy);
		return EINVAL;
	}
	else
	{
		if ((queue >= pcfg->q_cnt) && (queue != PFE_TMU_INVALID_QUEUE))
		{
			NXP_LOG_ERROR("Invalid queue ID (%d). PHY %d implements %d queues\n",
					queue, phy, pcfg->q_cnt);
			return EINVAL;
		}
	}

	return EOK;
}

/*
 * @brief		Check if phy+scheduler combination is valid
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @param[in]	sch Scheduler ID
 * @return		EOK if the arguments are valid
 */
static errno_t pfe_tmu_check_scheduler(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	const pfe_tmu_phy_cfg_t *pcfg;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_ERROR("Invalid phy: %d\n", (uint32_t)phy);
		return EINVAL;
	}
	else
	{
		if (sch >= pcfg->sch_cnt)
		{
			NXP_LOG_ERROR("Invalid scheduler ID (%d). PHY %d implements %d schedulers\n",
					sch, phy, pcfg->sch_cnt);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Check if phy+shaper combination is valid
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @param[in]	shp Shaper ID
 * @return		EOK if the arguments are valid
 */
static errno_t pfe_tmu_check_shaper(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	const pfe_tmu_phy_cfg_t *pcfg;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_ERROR("Invalid phy: %d\n", (uint32_t)phy);
		return EINVAL;
	}
	else
	{
		if (shp >= pcfg->shp_cnt)
		{
			NXP_LOG_ERROR("Invalid shaper ID (%d). PHY %d implements %d shapers\n",
					shp, phy, pcfg->shp_cnt);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Get number of packets in the queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the fill level value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_fill_level(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == level)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		return pfe_tmu_q_cfg_get_fill_level(tmu->cbus_base_va, phy, queue, level);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get number of packet dropped by queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_drop_count(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		return pfe_tmu_q_cfg_get_drop_count(tmu->cbus_base_va, phy, queue, cnt);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get number of packet transmitted from queue
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_tx_count(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == tmu) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		return pfe_tmu_q_cfg_get_tx_count(tmu->cbus_base_va, phy, queue, cnt);
	}
	else
	{
		return EINVAL;
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
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_set_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue,
		pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max)
{
    errno_t ret_val;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		switch (mode)
		{
			case TMU_Q_MODE_TAIL_DROP:
			{
				ret_val = pfe_tmu_q_mode_set_tail_drop(tmu->cbus_base_va, phy, queue, max);
				break;
			}

			case TMU_Q_MODE_WRED:
			{
				ret_val = pfe_tmu_q_mode_set_wred(tmu->cbus_base_va, phy, queue, min, max);
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
	else
	{
		ret_val = EINVAL;
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
pfe_tmu_queue_mode_t pfe_tmu_queue_get_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t queue, uint32_t *min, uint32_t *max)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TMU_Q_MODE_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		return pfe_tmu_q_get_mode(tmu->cbus_base_va, phy, queue, min, max);
	}
	else
	{
		return TMU_Q_MODE_INVALID;
	}
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
errno_t pfe_tmu_queue_set_wred_prob(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		if (zone >= pfe_tmu_queue_get_wred_zones(tmu, phy, queue))
		{
			NXP_LOG_DEBUG("Zone index out of range\n");
			return EINVAL;
		}

		if (prob > 100U)
		{
			NXP_LOG_DEBUG("Probability out of range\n");
			return EINVAL;
		}

		return pfe_tmu_q_set_wred_probability(tmu->cbus_base_va, phy, queue, zone, prob);
	}
	else
	{
		return EINVAL;
	}
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
errno_t pfe_tmu_queue_get_wred_prob(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		if (zone >= pfe_tmu_queue_get_wred_zones(tmu, phy, queue))
		{
			NXP_LOG_DEBUG("Zone index out of range\n");
			return EINVAL;
		}

		return pfe_tmu_q_get_wred_probability(tmu->cbus_base_va, phy, queue, zone, prob);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get number of WRED probability zones
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	queue The queue ID
 * @return		Number of zones between 'min' and 'max'
 */
uint8_t pfe_tmu_queue_get_wred_zones(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_queue(tmu, phy, queue))
	{
		return pfe_tmu_q_get_wred_zones(tmu->cbus_base_va, phy, queue);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get number of queues for given physical interface
 * @param[in]	tmu The TMU instance
 * @param[in]	phy Physical interface ID
 * @return		Number of queues
 */
uint8_t pfe_tmu_queue_get_cnt(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy)
{
	const pfe_tmu_phy_cfg_t *pcfg;

	(void)tmu;

	pcfg = pfe_tmu_cfg_get_phy_config(phy);
	if (NULL == pcfg)
	{
		NXP_LOG_ERROR("Invalid phy: 0x%x\n", phy);
		return 0U;
	}
	else
	{
		return pcfg->q_cnt;
	}
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
errno_t pfe_tmu_shp_set_limits(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t max_credit, int32_t min_credit)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_set_limits(tmu->cbus_base_va, phy, shp, max_credit, min_credit);
	}
	else
	{
		return EINVAL;
	}
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
errno_t pfe_tmu_shp_get_limits(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t *max_credit, int32_t *min_credit)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu) || unlikely(NULL == max_credit) || unlikely(NULL == min_credit))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_get_limits(tmu->cbus_base_va, phy, shp, max_credit, min_credit);
	}
	else
	{
		return EINVAL;
	}
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
errno_t pfe_tmu_shp_set_position(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_set_position(tmu->cbus_base_va, phy, shp, pos);
	}
	else
	{
		return EINVAL;
	}
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
uint8_t pfe_tmu_shp_get_position(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_TMU_INVALID_POSITION;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_get_position(tmu->cbus_base_va, phy, shp);
	}
	else
	{
		return PFE_TMU_INVALID_POSITION;
	}
}

/**
 * @brief		Enable shaper
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 */
errno_t pfe_tmu_shp_enable(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_enable(tmu->cbus_base_va, phy, shp);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Set shaper rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	mode Shaper mode
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_set_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_set_rate_mode(tmu->cbus_base_va, phy, shp, mode);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get shaper rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Shaper rate mode
 */
pfe_tmu_rate_mode_t pfe_tmu_shp_get_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return RATE_MODE_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_get_rate_mode(tmu->cbus_base_va, phy, shp);
	}
	else
	{
		return RATE_MODE_INVALID;
	}
}

/**
 * @brief		Set shaper idle slope
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_set_idle_slope(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_set_idle_slope(tmu->cbus_base_va, phy, shp, isl);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get shaper idle slope
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Current idle slope value
 */
uint32_t pfe_tmu_shp_get_idle_slope(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		return pfe_tmu_shp_cfg_get_idle_slope(tmu->cbus_base_va, phy, shp);
	}
	else
	{
		return 0U;
	}
}

/**
 * @brief		Disable shaper
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 */
errno_t pfe_tmu_shp_disable(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_shaper(tmu, phy, shp))
	{
		pfe_tmu_shp_cfg_disable(tmu->cbus_base_va, phy, shp);
		return EOK;
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Set scheduler rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	mode The rate mode to be used by scheduler
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_rate_mode_t mode)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_set_rate_mode(tmu->cbus_base_va, phy, sch, mode);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief	Get scheduler rate mode
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Current rate mode or RATE_MODE_INVALID in case of error
 */
pfe_tmu_rate_mode_t pfe_tmu_sch_get_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return RATE_MODE_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_get_rate_mode(tmu->cbus_base_va, phy, sch);
	}
	else
	{
		return RATE_MODE_INVALID;
	}
}

/**
 * @brief		Set scheduler algorithm
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_algo(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_sched_algo_t algo)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_set_algo(tmu->cbus_base_va, phy, sch, algo);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get scheduler algorithm
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Current rate mode or SCHED_ALGO_INVALID in case of error
 */
pfe_tmu_sched_algo_t pfe_tmu_sch_get_algo(pfe_tmu_t *tmu,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return SCHED_ALGO_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_get_algo(tmu->cbus_base_va, phy, sch);
	}
	else
	{
		return SCHED_ALGO_INVALID;
	}
}

/**
 * @brief		Get number of scheduler inputs
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @return		Number of scheduler inputs
 */
uint8_t pfe_tmu_sch_get_input_cnt(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		/*	Number of scheduler inputs is equal to number of available queues */
		return pfe_tmu_queue_get_cnt(tmu, phy);
	}
	else
	{
		return 0U;
	}
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
errno_t pfe_tmu_sch_set_input_weight(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, uint8_t input, uint32_t weight)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_set_input_weight(tmu->cbus_base_va,
			phy, sch, input, weight);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get scheduler input weight
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input
 * @return		Input weight
 */
uint32_t pfe_tmu_sch_get_input_weight(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_get_input_weight(tmu->cbus_base_va,
				phy, sch, input);
	}
	else
	{
		return 0U;
	}
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
errno_t pfe_tmu_sch_bind_sch_output(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((EOK == pfe_tmu_check_scheduler(tmu, phy, src_sch))
			&& (EOK == pfe_tmu_check_scheduler(tmu, phy, dst_sch)))
	{
		return pfe_tmu_sch_cfg_bind_sched_output(tmu->cbus_base_va, phy, src_sch, dst_sch, input);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get scheduler which output is connected to given scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input
 * @return		ID of the connected scheduler or PFE_TMU_INVALID_SCHEDULER
 */
uint8_t pfe_tmu_sch_get_bound_sch_output(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_TMU_INVALID_SCHEDULER;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_get_bound_sched_output(tmu->cbus_base_va, phy, sch, input);
	}
	else
	{
		return PFE_TMU_INVALID_SCHEDULER;
	}
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
errno_t pfe_tmu_sch_bind_queue(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy,
		uint8_t sch, uint8_t input, uint8_t queue)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
			&& (EOK == pfe_tmu_check_queue(tmu, phy, queue)))
	{
		return pfe_tmu_sch_cfg_bind_queue(tmu->cbus_base_va, phy, sch, input, queue);
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief		Get queue connected to given scheduler input
 * @param[in]	tmu The TMU instance
 * @parma[in]	phy Physical interface ID
 * @param[in]	sch The scheduler ID
 * @param[in]	input Scheduler input to be queried
 * @return		Queue ID connected to the input or PFE_TMU_INVALID_QUEUE if not present
 */
uint8_t pfe_tmu_sch_get_bound_queue(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_TMU_INVALID_QUEUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK == pfe_tmu_check_scheduler(tmu, phy, sch))
	{
		return pfe_tmu_sch_cfg_get_bound_queue(tmu->cbus_base_va, phy, sch, input);
	}
	else
	{
		return PFE_TMU_INVALID_QUEUE;
	}
}

/**
 * @brief		Return TMU runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	tmu The TMU instance
 * @param[in]	buf Pointer to buffer to be written
 * @param[in]	buf_len Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_tmu_get_text_statistics(pfe_tmu_t *tmu, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == tmu))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	
	len += pfe_tmu_cfg_get_text_stat(tmu->cbus_base_va, buf, buf_len, verb_level);

	return len;
}
