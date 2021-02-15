/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_TMU_H_
#define PFE_TMU_H_

#define PFE_TMU_INVALID_QUEUE		255U
#define PFE_TMU_INVALID_SCHEDULER	255U
#define PFE_TMU_INVALID_SHAPER		255U
#define PFE_TMU_INVALID_POSITION	255U

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
} pfe_tmu_cfg_t;

errno_t pfe_tmu_queue_get_fill_level(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level);
errno_t pfe_tmu_queue_get_drop_count(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_queue_get_tx_count(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_queue_set_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max);
pfe_tmu_queue_mode_t pfe_tmu_queue_get_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *min, uint32_t *max);
errno_t pfe_tmu_queue_set_wred_prob(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob);
errno_t pfe_tmu_queue_get_wred_prob(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob);
uint8_t pfe_tmu_queue_get_wred_zones(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue);
uint8_t pfe_tmu_queue_get_cnt(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy);

errno_t pfe_tmu_shp_enable(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_disable(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_shp_get_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_idle_slope(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl);
uint32_t pfe_tmu_shp_get_idle_slope(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_set_limits(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t max_credit, int32_t min_credit);
errno_t pfe_tmu_shp_get_limits(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t *max_credit, int32_t *min_credit);
errno_t pfe_tmu_shp_set_position(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos);
uint8_t pfe_tmu_shp_get_position(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t shp);

errno_t pfe_tmu_sch_set_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_sch_get_rate_mode(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_set_algo(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_sched_algo_t algo);
pfe_tmu_sched_algo_t pfe_tmu_sch_get_algo(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
uint8_t pfe_tmu_sch_get_input_cnt(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_set_input_weight(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint32_t weight);
uint32_t pfe_tmu_sch_get_input_weight(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_bind_queue(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint8_t queue);
uint8_t pfe_tmu_sch_get_bound_queue(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_bind_sch_output(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input);
uint8_t pfe_tmu_sch_get_bound_sch_output(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);

pfe_tmu_t *pfe_tmu_create(void *cbus_base_va, uint32_t pe_num, pfe_tmu_cfg_t *cfg);
void pfe_tmu_enable(pfe_tmu_t *tmu);
void pfe_tmu_reset(pfe_tmu_t *tmu);
void pfe_tmu_disable(pfe_tmu_t *tmu);
void pfe_tmu_send(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, void *buf_pa, uint16_t len);
uint32_t pfe_tmu_get_text_statistics(pfe_tmu_t *tmu, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_tmu_destroy(pfe_tmu_t *tmu);

#endif /* PFE_TMU_H_ */
