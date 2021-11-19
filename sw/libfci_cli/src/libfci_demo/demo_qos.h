/* =========================================================================
 *  Copyright 2020-2021 NXP
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

#ifndef DEMO_QOS_H_
#define DEMO_QOS_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_qos_que_cb_print_t)(const fpp_qos_queue_cmd_t* p_que);
typedef int (*demo_qos_sch_cb_print_t)(const fpp_qos_scheduler_cmd_t* p_sch);
typedef int (*demo_qos_shp_cb_print_t)(const fpp_qos_shaper_cmd_t* p_shp);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_qos_que_get_by_id(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_rtn_que, const char* p_phyif_name, uint8_t que_id);
int demo_qos_sch_get_by_id(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_rtn_sch, const char* p_phyif_name, uint8_t sch_id);
int demo_qos_shp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_rtn_shp, const char* p_phyif_name, uint8_t shp_id);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */

int demo_qos_que_update(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_que);
int demo_qos_sch_update(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_sch);
int demo_qos_shp_update(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_shp);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

void demo_qos_que_ld_set_mode(fpp_qos_queue_cmd_t* p_que, uint8_t que_mode);
void demo_qos_que_ld_set_min(fpp_qos_queue_cmd_t* p_que, uint32_t min);
void demo_qos_que_ld_set_max(fpp_qos_queue_cmd_t* p_que, uint32_t max);
void demo_qos_que_ld_set_zprob(fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id, uint8_t percentage);

void demo_qos_sch_ld_set_mode(fpp_qos_scheduler_cmd_t* p_sch, uint8_t sch_mode);
void demo_qos_sch_ld_set_algo(fpp_qos_scheduler_cmd_t* p_sch, uint8_t algo);
void demo_qos_sch_ld_set_input(fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id, bool enable, uint8_t src, uint32_t weight);

void demo_qos_shp_ld_set_mode(fpp_qos_shaper_cmd_t* p_shp, uint8_t shp_mode);
void demo_qos_shp_ld_set_position(fpp_qos_shaper_cmd_t* p_shp, uint8_t position);
void demo_qos_shp_ld_set_isl(fpp_qos_shaper_cmd_t* p_shp, uint32_t isl);
void demo_qos_shp_ld_set_min_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t min_credit);
void demo_qos_shp_ld_set_max_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t max_credit);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

const char* demo_qos_que_ld_get_if_name(const fpp_qos_queue_cmd_t* p_que);
uint8_t     demo_qos_que_ld_get_id(const fpp_qos_queue_cmd_t* p_que);
uint8_t     demo_qos_que_ld_get_mode(const fpp_qos_queue_cmd_t* p_que);
uint32_t    demo_qos_que_ld_get_min(const fpp_qos_queue_cmd_t* p_que);
uint32_t    demo_qos_que_ld_get_max(const fpp_qos_queue_cmd_t* p_que);
uint8_t     demo_qos_que_ld_get_zprob_by_id(const fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id);

const char* demo_qos_sch_ld_get_if_name(const fpp_qos_scheduler_cmd_t* p_sch);
uint8_t     demo_qos_sch_ld_get_id(const fpp_qos_scheduler_cmd_t* p_sch);
uint8_t     demo_qos_sch_ld_get_mode(const fpp_qos_scheduler_cmd_t* p_sch);
uint8_t     demo_qos_sch_ld_get_algo(const fpp_qos_scheduler_cmd_t* p_sch);
bool        demo_qos_sch_ld_is_input_enabled(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id);
uint32_t    demo_qos_sch_ld_get_input_weight(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id);
uint8_t     demo_qos_sch_ld_get_input_src(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id);

const char* demo_qos_shp_ld_get_if_name(const fpp_qos_shaper_cmd_t* p_shp);
uint8_t     demo_qos_shp_ld_get_id(const fpp_qos_shaper_cmd_t* p_shp);
uint8_t     demo_qos_shp_ld_get_position(const fpp_qos_shaper_cmd_t* p_shp);
uint8_t     demo_qos_shp_ld_get_mode(const fpp_qos_shaper_cmd_t* p_shp);
uint32_t    demo_qos_shp_ld_get_isl(const fpp_qos_shaper_cmd_t* p_shp);
int32_t     demo_qos_shp_ld_get_max_credit(const fpp_qos_shaper_cmd_t* p_shp);
int32_t     demo_qos_shp_ld_get_min_credit(const fpp_qos_shaper_cmd_t* p_shp);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_qos_que_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_que_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_qos_que_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_parent_name);

int demo_qos_sch_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_sch_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_qos_sch_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_parent_name);

int demo_qos_shp_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_shp_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_qos_shp_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_parent_name);

/* ========================================================================= */

#endif
