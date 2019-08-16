/* =========================================================================
 *  Copyright 2018-2019 NXP
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

/**
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 * 
 * @defgroup    dxgr_PFE_TMU TMU
 * @brief		The Traffic Manager Unit
 * @details     This is the software representation of the TMU block.
 * 
 * @addtogroup  dxgr_PFE_TMU
 * @{
 * 
 * @file		pfe_tmu.h
 * @brief		The TMU module header file.
 * @details		This file contains TMU-related API.
 *
 */

#ifndef PFE_TMU_H_
#define PFE_TMU_H_

/**
 * @brief	Scheduler disciplines
 */
typedef enum
{
	SCHED_ALGO_PQ,		/*!< Priority Queuing */
	SCHED_ALGO_DWRR,	/*!< Deficit Weighted Round Robin */
	SCHED_ALGO_RR,		/*!< Round Robin */
	SCHED_ALGO_WRR		/*!< Weighted Round Robin */
} pfe_tmu_sched_algo_t;

/**
 * @brief	Scheduler/Shaper rate modes
 */
typedef enum
{
	RATE_MODE_DATA_RATE = 0,	/*!< Data rate */
	RATE_MODE_PACKET_RATE = 1	/*!< Packet rate */
} pfe_tmu_rate_mode_t;

/**
 * @brief	Queue modes
 */
typedef enum
{
	/*	Queue in tail drop mode will drop packets if fill level will exceed the 'max' value. */
	TMU_Q_MODE_TAIL_DROP,
	/*	WRED will create probability zones between 'min' and 'max' values. When fill level
	 	is reaching a zone, packets will be dropped as defined by probability defined by
	 	the zone. Drop probability below 'min' is 0%, above 'max' is 100%. */
	TMU_Q_MODE_WRED,
	/*	Default mode (turns off previous modes) */
	TMU_Q_MODE_NONE
} pfe_tmu_queue_mode_t;

typedef struct __pfe_tmu_tag pfe_tmu_t;
typedef struct __pfe_tmu_queue_tag pfe_tmu_queue_t;
typedef struct __pfe_tmu_qset_tag pfe_tmu_qset_t;
typedef struct __pfe_tmu_sch_tag pfe_tmu_sch_t;
typedef struct __pfe_tmu_shp_tag pfe_tmu_shp_t;

typedef struct
{
	uint32_t pe_sys_clk_ratio;		/*	Clock mode ratio for sys_clk and pe_clk */
} pfe_tmu_cfg_t;

errno_t pfe_tmu_queue_get_fill_level(pfe_tmu_queue_t *queue, uint32_t *level);
errno_t pfe_tmu_queue_get_drop_count(pfe_tmu_queue_t *queue, uint32_t *cnt);
errno_t pfe_tmu_queue_get_tx_count(pfe_tmu_queue_t *queue, uint32_t *cnt);
errno_t pfe_tmu_queue_set_mode(pfe_tmu_queue_t *queue, pfe_tmu_queue_mode_t mode, uint32_t min, uint32_t max);
errno_t pfe_tmu_queue_set_wred_prob(pfe_tmu_queue_t *queue, uint8_t zone, uint8_t prob);
uint8_t pfe_tmu_queue_get_wred_zones(pfe_tmu_queue_t *queue);
pfe_tmu_queue_t *pfe_tmu_qset_get_queue(pfe_tmu_qset_t *qset, uint8_t queue);
pfe_tmu_qset_t * pfe_tmu_qset_create(void *cbus_base_va, pfe_ct_phy_if_id_t phy);
void pfe_tmu_qset_destroy(pfe_tmu_qset_t *qset);

pfe_tmu_shp_t *pfe_tmu_shp_create(void *cbus_base_va, void *shp_base_offset);
void pfe_tmu_shp_destroy(pfe_tmu_shp_t *shp);
errno_t pfe_tmu_shp_enable(pfe_tmu_shp_t *shp, pfe_tmu_rate_mode_t mode, uint32_t isl);
errno_t pfe_tmu_shp_disable(pfe_tmu_shp_t *shp);
errno_t pfe_tmu_shp_set_limits(pfe_tmu_shp_t *shp, int32_t max_credit, int32_t min_credit);

pfe_tmu_sch_t *pfe_tmu_sch_create(void *cbus_base_va, void *sch_base_offset);
void pfe_tmu_sch_destroy(pfe_tmu_sch_t *sch);
errno_t pfe_tmu_sch_set_rate_mode(pfe_tmu_sch_t *sch, pfe_tmu_rate_mode_t mode);
errno_t pfe_tmu_sch_set_algo(pfe_tmu_sch_t *sch, pfe_tmu_sched_algo_t algo);
errno_t pfe_tmu_sch_set_input_weight(pfe_tmu_sch_t *sch, uint8_t input, uint32_t weight);
errno_t pfe_tmu_sch_bind_sch_output(pfe_tmu_sch_t *sch, uint8_t input, pfe_tmu_sch_t *sch_out);
errno_t pfe_tmu_sch_bind_queue(pfe_tmu_sch_t *sch, uint8_t input, uint8_t queue);

pfe_tmu_t *pfe_tmu_create(void *cbus_base_va, uint32_t pe_num, pfe_tmu_cfg_t *cfg);
void pfe_tmu_enable(pfe_tmu_t *class);
void pfe_tmu_reset(pfe_tmu_t *class);
void pfe_tmu_disable(pfe_tmu_t *class);
errno_t pfe_tmu_load_firmware(pfe_tmu_t *class, const void *elf);
void pfe_tmu_send(pfe_tmu_t *tmu, pfe_ct_phy_if_id_t phy, uint8_t queue, void *buf_pa, uint16_t len);
uint32_t pfe_tmu_get_text_statistics(pfe_tmu_t *tmu, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_tmu_destroy(pfe_tmu_t *class);

#endif /* PFE_TMU_H_ */

/** @}*/
/** @}*/
