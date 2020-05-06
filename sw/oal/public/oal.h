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

#ifndef __OAL__
#define __OAL__

#ifndef PFE_CFG_H
	#error Please include the pfe_cfg.h first.
#endif

#include "oal_types.h"
#include "oal_mbox.h"
#include "oal_irq.h"
#include "oal_mm.h"
#include "oal_thread.h"
#include "oal_sync.h"
#include "oal_time.h"
#include "oal_util.h"
#include "oal_job.h"

#endif /* __OAL__ */

/** @}*/
