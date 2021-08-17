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

typedef int (*fci_spd_cb_print_t)(const fpp_spd_cmd_t* p_spd);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_spd_get_by_position(FCI_CLIENT* p_cl, fpp_spd_cmd_t* p_rtn_spd, const char* p_phyif_name, uint16_t position);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */

int fci_spd_add(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position, const fpp_spd_cmd_t* p_spd_data);
int fci_spd_del(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_spd_ld_set_protocol(fpp_spd_cmd_t* p_spd, uint8_t protocol);
int fci_spd_ld_set_ip(fpp_spd_cmd_t* p_spd, const uint32_t p_saddr[4], const uint32_t p_daddr[4], bool is_ip6);
int fci_spd_ld_set_port(fpp_spd_cmd_t* p_spd, bool use_sport, uint16_t sport, bool use_dport, uint16_t dport);
int fci_spd_ld_set_action(fpp_spd_cmd_t* p_spd, fpp_spd_action_t spd_action, uint32_t sa_id, uint32_t spi);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool fci_spd_ld_is_ip6(const fpp_spd_cmd_t* p_spd);
bool fci_spd_ld_is_used_sport(const fpp_spd_cmd_t* p_spd);
bool fci_spd_ld_is_used_dport(const fpp_spd_cmd_t* p_spd);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_spd_print_by_phyif(FCI_CLIENT* p_cl, fci_spd_cb_print_t p_cb_print, const char* p_phyif_name,
                           uint16_t position_init, uint16_t count);
int fci_spd_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, const char* p_phyif_name);

/* ========================================================================= */

#endif
