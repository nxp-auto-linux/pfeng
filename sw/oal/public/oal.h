/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup	dxgrOAL OAL
 * @brief		The OS Abstraction Layer
 * @details		OAL is intended to provide OS abstraction. To write portable SW one shall use
 * 				OAL calls instead of OS-specific ones. This OAL module incorporates following
 * 				functionality:
 * 				
 * 				- oal_irq - Interrupt management
 * 				- oal_mbox - Message-based IPC
 * 				- oal_mm - Memory management
 * 				- oal_types - Abstraction of standard types
 * 				- oal_thread - Threading support
 * 				- oal_sync - Thread synchronization
 * 				- oal_util - Simplification utility
 * 				- oaj_job - Job context abstraction
 * 				
 * 
 * @addtogroup	dxgrOAL
 * @{
 * 
 * @file		oal.h
 * @brief		The main OAL header file
 * @details		Use this header to include all the OAL-provided functionality
 *
 */

#ifndef OAL_H_
#define OAL_H_

#ifndef PFE_CFG_H
	#error Please include the pfe_cfg.h first.
#endif

#include "oal_types.h"
#include "oal_mm.h"
#include "oal_util.h"
#include "oal_sync.h"
#include "oal_master_if.h"
#if !defined(PFE_CFG_DETACHED_MINIHIF)
#include "oal_mbox.h"
#include "oal_irq.h"
#include "oal_thread.h"
#include "oal_time.h"
#include "oal_job.h"
#endif /* PFE_CFG_DETACHED_MINIHIF */

#endif /* OAL_H_ */

/** @}*/
