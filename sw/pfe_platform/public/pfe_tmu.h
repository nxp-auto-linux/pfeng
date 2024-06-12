/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_TMU_H_
#define PFE_TMU_H_

#include "pfe_class.h"

#define PFE_TMU_INVALID_QUEUE		255U
#define PFE_TMU_INVALID_SCHEDULER	255U
#define PFE_TMU_INVALID_SHAPER		255U
#define PFE_TMU_INVALID_POSITION	255U

#define PFE_TMU_ERR051211_Q_OFFSET	(40U + 1U)	/* (S2+S3+8 +1) See ERR051211 errata ; extra '+1' compensates for the fact that HIF BD Rings have one permanently disabled 'terminator' BD. */
#define PFE_TMU_ERR051211_MINIMAL_REQUIRED_RX_RING_LENGTH	((PFE_TMU_ERR051211_Q_OFFSET) + 2U)	/* See ERR051211 errata ; extra '+2' is for two theoretical minimalistic HIF queues (each with one slot). */

/**
 * @brief	Scheduler disciplines
 */
typedef enum __attribute__((packed))
{
	SCHED_ALGO_PQ,		/*!< Priority Queuing */
	SCHED_ALGO_DWRR,	/*!< Deficit Weighted Round Robin */
	SCHED_ALGO_RR,		/*!< Round Robin */
	SCHED_ALGO_WRR,		/*!< Weighted Round Robin */
	SCHED_ALGO_INVALID
} pfe_tmu_sched_algo_t;

/**
 * @brief	Scheduler/Shaper rate modes
 */
typedef enum __attribute__((packed))
{
	RATE_MODE_DATA_RATE = 0,	/*!< Data rate */
	RATE_MODE_PACKET_RATE = 1,	/*!< Packet rate */
	RATE_MODE_INVALID
} pfe_tmu_rate_mode_t;

/**
 * @brief	Queue modes
 */
typedef enum __attribute__((packed))
{
	/*	Queue in tail drop mode will drop packets if fill level will exceed the 'max' value. */
	TMU_Q_MODE_TAIL_DROP,
	/*	WRED will create probability zones between 'min' and 'max' values. When fill level
	 	is reaching a zone, packets will be dropped as defined by probability defined by
	 	the zone. Drop probability below 'min' is 0%, above 'max' is 100%. */
	TMU_Q_MODE_WRED,
	/*	Default mode (turns off previous modes) */
	TMU_Q_MODE_DEFAULT,
	/*	Invalid queue mode */
	TMU_Q_MODE_INVALID
} pfe_tmu_queue_mode_t;

typedef struct pfe_tmu_tag pfe_tmu_t;
typedef struct pfe_tmu_phy_cfg_tag pfe_tmu_phy_cfg_t;

typedef struct
{
	uint32_t pe_sys_clk_ratio;		/*	Clock mode ratio for sys_clk and pe_clk */
	bool_t on_g3;
} pfe_tmu_cfg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t pfe_tmu_check_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue);
errno_t pfe_tmu_queue_get_fill_level(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level);
errno_t pfe_tmu_queue_get_drop_count(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_queue_get_tx_count(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_queue_set_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max);
pfe_tmu_queue_mode_t pfe_tmu_queue_get_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *min, uint32_t *max);
errno_t pfe_tmu_queue_set_wred_prob(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob);
errno_t pfe_tmu_queue_get_wred_prob(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob);
uint8_t pfe_tmu_queue_get_wred_zones(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue);
uint8_t pfe_tmu_queue_get_cnt(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy);
errno_t pfe_tmu_queue_reset_tail_drop_policy(const pfe_tmu_t *tmu);
errno_t pfe_tmu_queue_err051211_sync(const pfe_tmu_t *tmu);

errno_t pfe_tmu_check_shaper(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_enable(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_disable(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_shp_get_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_idle_slope(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl);
uint32_t pfe_tmu_shp_get_idle_slope(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_limits(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t max_credit, int32_t min_credit);
errno_t pfe_tmu_shp_get_limits(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t *max_credit, int32_t *min_credit);
errno_t pfe_tmu_shp_set_position(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos);
uint8_t pfe_tmu_shp_get_position(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);

errno_t pfe_tmu_check_scheduler(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_set_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_sch_get_rate_mode(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_set_algo(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_sched_algo_t algo);
pfe_tmu_sched_algo_t pfe_tmu_sch_get_algo(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
uint8_t pfe_tmu_sch_get_input_cnt(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_set_input_weight(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint32_t weight);
uint32_t pfe_tmu_sch_get_input_weight(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_bind_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint8_t queue);
uint8_t pfe_tmu_sch_get_bound_queue(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_bind_sch_output(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input);
uint8_t pfe_tmu_sch_get_bound_sch_output(const pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);

pfe_tmu_t *pfe_tmu_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_tmu_cfg_t *cfg, pfe_class_t *class);
void pfe_tmu_enable(const pfe_tmu_t *tmu);
void pfe_tmu_reset(const pfe_tmu_t *tmu);
void pfe_tmu_disable(const pfe_tmu_t *tmu);

uint32_t pfe_tmu_get_text_statistics(const pfe_tmu_t *tmu, struct seq_file *seq, uint8_t verb_level);

void pfe_tmu_destroy(const pfe_tmu_t *tmu);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_TMU_H_ */
