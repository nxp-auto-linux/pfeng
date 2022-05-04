/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
/**
 *  @file       pfe_cfg.h
 *  @brief      PFE configuration file
 *  @details    This file needs to be included in all PFE sources before any other.
 *              PFE include.
 */
#ifndef PFE_CFG_H
#define PFE_CFG_H

/*  Find the configuration parameters defined in makefiles  */

/**
 * @def	    PFE_CFG_HIF_IRQ_ENABLED
 * @brief	If TRUE then HIF interrupt will be used.
 */
#define PFE_CFG_HIF_IRQ_ENABLED       TRUE

#endif /*PFE_CFG_H*/
