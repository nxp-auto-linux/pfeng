/* =========================================================================
 *  Copyright 2018-2020 NXP
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/*	Queue instance */
struct __pfe_tmu_queue_tag
{
	void *cbus_base_va;			/*	CBUS base virtual address */
	pfe_ct_phy_if_id_t phy;		/*	Associated physical interface ID */
	uint8_t qid;				/*	Queue ID */
};

/*	Queue set instance */
struct __pfe_tmu_qset_tag
{
	void *cbus_base_va;			/*	CBUS base virtual address */
	pfe_ct_phy_if_id_t phy;		/*	Associated physical interface ID */
	uint8_t count;				/*	Number of queues */
	pfe_tmu_queue_t **queues;	/*	The queues */
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

/**
 * @brief		Get number of packets in the queue
 * @param[in]	queue The queue instance
 * @param[out]	level Pointer to memory where the fill level value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_fill_level(pfe_tmu_queue_t *queue, uint32_t *level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == queue) || (NULL == level)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_q_cfg_get_fill_level(queue->cbus_base_va, queue->phy, queue->qid, level);
}

/**
 * @brief		Get number of packet dropped by queue
 * @param[in]	queue The queue instance
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_drop_count(pfe_tmu_queue_t *queue, uint32_t *cnt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == queue) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_q_cfg_get_drop_count(queue->cbus_base_va, queue->phy, queue->qid, cnt);
}

/**
 * @brief		Get number of packet transmitted from queue
 * @param[in]	queue The queue instance
 * @param[out]	level Pointer to memory where the count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_get_tx_count(pfe_tmu_queue_t *queue, uint32_t *cnt)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == queue) || (NULL == cnt)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_q_cfg_get_tx_count(queue->cbus_base_va, queue->phy, queue->qid, cnt);
}

/**
 * @brief		Set queue mode
 * @param[in]	queue The queue instance
 * @param[in]	mode Mode
 * @param[in]	min Min threshold (number of packets)
 * @param[in]	max Max threshold (number of packets)
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_set_mode(pfe_tmu_queue_t *queue, pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max)
{
    errno_t ret_val;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == queue))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (mode)
	{
		case TMU_Q_MODE_TAIL_DROP:
		{
			ret_val = pfe_tmu_q_mode_set_tail_drop(queue->cbus_base_va, queue->phy, queue->qid, max);
			break;
		}

		case TMU_Q_MODE_WRED:
		{
			ret_val = pfe_tmu_q_mode_set_wred(queue->cbus_base_va, queue->phy, queue->qid, min, max);
			break;
		}

		case TMU_Q_MODE_NONE:
		{
			ret_val = pfe_tmu_q_mode_set_default(queue->cbus_base_va, queue->phy, queue->qid);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown queue mode: %d\n", mode);
			ret_val = EINVAL;
            break;
		}
	}

	return ret_val;
}

/**
 * @brief		Set WRED zone probability
 * @param[in]	queue The queue instance
 * @param[in]	zone Zone index
 * @param[in]	prob Drop probability in [%]
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_queue_set_wred_prob(pfe_tmu_queue_t *queue, uint8_t zone, uint8_t prob)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == queue))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (zone >= pfe_tmu_queue_get_wred_zones(queue))
	{
		NXP_LOG_DEBUG("Zone index out of range\n");
		return EINVAL;
	}

	if (prob > 100U)
	{
		NXP_LOG_DEBUG("Probability out of range\n");
		return EINVAL;
	}

	return pfe_tmu_q_set_wred_probability(queue->cbus_base_va, queue->phy, queue->qid, zone, prob);
}

/**
 * @brief		Get number of WRED probability zones
 * @param[in]	queue The queue instance
 * @return		Number of zones between 'min' and 'max'
 */
uint8_t pfe_tmu_queue_get_wred_zones(pfe_tmu_queue_t *queue)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == queue))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_q_get_wred_zones(queue->cbus_base_va, queue->phy, queue->qid);
}

/**
 * @brief		Get queue from queue set
 * @param[in]	qset The queue set instance
 * @param[in]	queue Queue index
 * @return		Queue instance or NULL if failed
 */
pfe_tmu_queue_t *pfe_tmu_qset_get_queue(pfe_tmu_qset_t *qset, uint8_t queue)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == qset))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (queue < qset->count)
	{
		return qset->queues[queue];
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Create set of queues for given physical interface
 * @param[in]	phy Physical interface ID
 * @return		Queue set instance or NULL if failed
 */
pfe_tmu_qset_t * pfe_tmu_qset_create(void *cbus_base_va, pfe_ct_phy_if_id_t phy)
{
	uint8_t count, ii;
	pfe_tmu_qset_t *qset;

	count = pfe_tmu_cfg_get_q_count(phy);
	if (0U == count)
	{
		NXP_LOG_WARNING("No queues for PHY ID %d\n", phy);
		return NULL;
	}

	qset = oal_mm_malloc(sizeof(pfe_tmu_qset_t));
	if (NULL == qset)
	{
		return NULL;
	}

	qset->queues = oal_mm_malloc(count * sizeof(pfe_tmu_queue_t));
	if (NULL == qset->queues)
	{
		oal_mm_free(qset);
		return NULL;
	}

	qset->count = count;
	qset->cbus_base_va = cbus_base_va;
	qset->phy = phy;

	for (ii=0U; ii<count; ii++)
	{
		qset->queues[ii]->cbus_base_va = cbus_base_va;
		qset->queues[ii]->phy = phy;
		qset->queues[ii]->qid = ii;
	}

	return qset;
}

/**
 * @brief		Destroy set of queues
 * @param[in]	qset The queue set instance
 */
void pfe_tmu_qset_destroy(pfe_tmu_qset_t *qset)
{
	uint32_t ii;

	if (qset != NULL)
	{
		for (ii=0U; ii<qset->count; ii++)
		{
			if (NULL != qset->queues[ii])
			{
				oal_mm_free(qset->queues[ii]);
				qset->queues[ii] = NULL;
			}
		}

		oal_mm_free(qset);
	}
}

/**
 * @brief		Set shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	shp_base_va Shaper base address (VA)
 * @param[in]	max_credit Maximum credit value
 * @param[in]	min_credit Minimum credit value
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_set_limits(pfe_tmu_shp_t *shp, int32_t max_credit, int32_t min_credit)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == shp))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_shp_cfg_set_limits(shp->shp_base_va, max_credit, min_credit);
}

/**
 * @brief		Enable shaper
 * @param[in]	shp The shaper instance
 * @param[in]	mode Shaper mode
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_enable(pfe_tmu_shp_t *shp, pfe_tmu_rate_mode_t mode, uint32_t isl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == shp))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_shp_cfg_enable(shp->cbus_base_va, shp->shp_base_va, mode, isl);
}

/**
 * @brief		Disable shaper
 * @param[in]	shp The shaper instance
 */
errno_t pfe_tmu_shp_disable(pfe_tmu_shp_t *shp)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == shp))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_tmu_shp_cfg_disable(shp->shp_base_va);

	return EOK;
}

/**
 * @brief		Create shaper instance
 * @details		This will bind HW shaper implementation with SW representation.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	shp_base_offset Shaper base address offset within CBUS address space
 * @return		Shaper instance or NULL if failed
 */
pfe_tmu_shp_t *pfe_tmu_shp_create(void *cbus_base_va, void *shp_base_offset)
{
	pfe_tmu_shp_t *shp;

	shp = oal_mm_malloc(sizeof(pfe_tmu_shp_t));

	if (NULL == shp)
	{
		return NULL;
	}
	else
	{
		shp->cbus_base_va = cbus_base_va;
		shp->shp_base_va = (void *)((addr_t)cbus_base_va + (addr_t)shp_base_offset);

		/*	Disable and initialize the shaper */
		pfe_tmu_shp_cfg_disable(shp->shp_base_va);
		pfe_tmu_shp_cfg_init(shp->shp_base_va);
	}

	return shp;
}

/**
 * @brief		Destroy shaper instance
 * @param[in]	shp The shaper instance
 */
void pfe_tmu_shp_destroy(pfe_tmu_shp_t *shp)
{
	if (NULL != shp)
	{
		oal_mm_free(shp);
	}
}

/**
 * @brief		Set rate mode
 * @param[in]	sch Scheduler instance
 * @param[in]	mode The rate mode to be used by scheduler
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_rate_mode(pfe_tmu_sch_t *sch, pfe_tmu_rate_mode_t mode)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == sch))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_sch_cfg_set_rate_mode(sch->sch_base_va, mode);
}

/**
 * @brief		Set scheduler algorithm
 * @param[in]	sch Scheduler instance
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_algo(pfe_tmu_sch_t *sch, pfe_tmu_sched_algo_t algo)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == sch))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_sch_cfg_set_algo(sch->sch_base_va, algo);
}

/**
 * @brief		Set input weight
 * @param[in]	sch Scheduler instance
 * @param[in]	input Scheduler input
 * @param[in]	weight The weight value to be used by chosen scheduling algorithm
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_set_input_weight(pfe_tmu_sch_t *sch, uint8_t input, uint32_t weight)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == sch))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_sch_cfg_set_input_weight(sch->sch_base_va, input, weight);
}

/**
 * @brief		Connect scheduler output to another scheduler input
 * @param[in]	sch Scheduler instance
 * @param[in]	input Scheduler where the other scheduler output shall be connected to
 * @param[in]	sch_out The other scheduler instance providing the output to be connected
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_bind_sch_output(pfe_tmu_sch_t *sch, uint8_t input, pfe_tmu_sch_t *sch_out)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == sch) || (NULL == sch_out)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_sch_cfg_bind_sch_output(sch->sch_base_va, input, sch_out->sch_base_va, sch->cbus_base_va);
}

/**
 * @brief		Connect queue to some scheduler input
 * @param[in]	sch Scheduler instance
 * @param[in]	input Scheduler input the queue shall be connected to
 * @param[in]	queue Queue to be connected to the scheduler input
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_bind_queue(pfe_tmu_sch_t *sch, uint8_t input, uint8_t queue)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == sch))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_tmu_sch_cfg_bind_queue(sch->sch_base_va, input, queue);
}

/**
 * @brief		Create scheduler instance
 * @details		This will bind HW scheduler implementation with SW representation.
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	sch_base_offset Scheduler base address offset within CBUS address space
 * @return		Scheduler instance or NULL if failed
 */
pfe_tmu_sch_t *pfe_tmu_sch_create(void *cbus_base_va, void *sch_base_offset)
{
	pfe_tmu_sch_t *sch;

	sch = oal_mm_malloc(sizeof(pfe_tmu_sch_t));

	if (NULL == sch)
	{
		return NULL;
	}
	else
	{
		sch->cbus_base_va = cbus_base_va;
		sch->sch_base_va = (void *)((addr_t)cbus_base_va + (addr_t)sch_base_offset);
	}

	return sch;
}

/**
 * @brief		Destroy scheduler instance
 * @param[in]	sch The scheduler instance
 */
void pfe_tmu_sch_destroy(pfe_tmu_sch_t *sch)
{
	if (NULL != sch)
	{
		oal_mm_free(sch);
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
