/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
 * ========================================================================= */

#ifndef FCI_SPD_H_
#define FCI_SPD_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_qos_que_cb_print_t)(const fpp_qos_queue_cmd_t* p_que);
typedef int (*fci_qos_sch_cb_print_t)(const fpp_qos_scheduler_cmd_t* p_sch);
typedef int (*fci_qos_shp_cb_print_t)(const fpp_qos_shaper_cmd_t* p_shp);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_qos_que_get_by_id(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_rtn_que, const char* p_phyif_name, uint8_t que_id);
int fci_qos_sch_get_by_id(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_rtn_sch, const char* p_phyif_name, uint8_t sch_id);
int fci_qos_shp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_rtn_shp, const char* p_phyif_name, uint8_t shp_id);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */

int fci_qos_que_update(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_que);
int fci_qos_sch_update(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_sch);
int fci_qos_shp_update(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_shp);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_qos_que_ld_set_mode(fpp_qos_queue_cmd_t* p_que, uint8_t que_mode);
int fci_qos_que_ld_set_min(fpp_qos_queue_cmd_t* p_que, uint32_t min);
int fci_qos_que_ld_set_max(fpp_qos_queue_cmd_t* p_que, uint32_t max);
int fci_qos_que_ld_set_zprob(fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id, uint8_t percentage);

int fci_qos_sch_ld_set_mode(fpp_qos_scheduler_cmd_t* p_sch, uint8_t sch_mode);
int fci_qos_sch_ld_set_algo(fpp_qos_scheduler_cmd_t* p_sch, uint8_t algo);
int fci_qos_sch_ld_set_input(fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id, bool enable, uint8_t src, uint32_t weight);

int fci_qos_shp_ld_set_mode(fpp_qos_shaper_cmd_t* p_shp, uint8_t shp_mode);
int fci_qos_shp_ld_set_position(fpp_qos_shaper_cmd_t* p_shp, uint8_t position);
int fci_qos_shp_ld_set_isl(fpp_qos_shaper_cmd_t* p_shp, uint32_t isl);
int fci_qos_shp_ld_set_min_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t min_credit);
int fci_qos_shp_ld_set_max_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t max_credit);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool fci_qos_sch_ld_is_input_enabled(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_qos_que_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_que_cb_print_t p_cb_print, const char* p_phyif_name);
int fci_qos_que_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, const char* p_parent_name);

int fci_qos_sch_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_sch_cb_print_t p_cb_print, const char* p_phyif_name);
int fci_qos_sch_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, const char* p_parent_name);

int fci_qos_shp_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_shp_cb_print_t p_cb_print, const char* p_phyif_name);
int fci_qos_shp_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, const char* p_parent_name);

/* ========================================================================= */

#endif
