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
 * @defgroup	dxgrPFE_PLATFORM PFE Platform
 * @brief		The PFE Platform
 * @details		This SW module is intended to provide full PFE HW abstraction layer. Using
 * 				this module's API one is able to configure, control and encapsulate particular
 * 				PFE HW components like classifier, GPI or HIF as a single object.
 * 				
 * 				Module can be used to create and maintain various PFE instances depending on
 * 				currently used HW platform or HW configuration without need to modify upper
 * 				or application-level SW layers.
 * 				
 * 				Module consists of following components:
 * 				- pfe_pe - A generic processing engine representation
 * 				- pfe_class - The CLASS block
 * 				- pfe_rtable - The routing table
 * 				- pfe_tmu - The TMU block
 * 				- pfe_util - The UTIL block
 * 				- pfe_bmu - The BMU block
 * 				- pfe_gpi - The GPI block
 * 				- pfe_util - The UTIL block
 * 				- pfe_emac - The EMAC block
 * 				- pfe_hif - The HIF block
 * 				- pfe_hif_chnl - The HIF channel
 * 				- pfe_hif_ring - The HIF BD ring
 * 				
 * 				These components can be used to synthesize SW representation of a PPFE HW block
 * 				using hierarchical structure where top level is the PFE platform object. 
 * 
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 * 
 * @file		pfe_platform.h
 * @brief		The PFE platform management for LS1012a
 * @details		Header for the HW-specific code
 *
 */

#ifndef SRC_PFE_PLATFORM_H_
#define SRC_PFE_PLATFORM_H_

#include <stdint.h>
#include <stdbool.h>
#include "pfe_platform_cfg.h"
#include "pfe_pe.h"
#include "pfe_gpi.h"
#include "pfe_bmu.h"
#include "pfe_class.h"
#include "pfe_rtable.h"
#include "pfe_tmu.h"
#include "pfe_util.h"
#include "pfe_hif.h"
#include "pfe_hif_nocpy.h"
#include "pfe_safety.h"
#include "pfe_emac.h"
#include "pfe_l2br_table.h"
#include "pfe_l2br.h"
#include "pfe_phy_if.h"
#include "pfe_log_if.h"
#include "pfe_hif_drv.h"
#include "pfe_if_db.h"

#define GEMAC0_MAC						{ 0x00U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU }
#define GEMAC1_MAC						{ 0x00U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU }
#define GEMAC2_MAC						{ 0x00U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU }

typedef enum
{
	HIF_ERR_POLLER_STATE_DISABLED,
	HIF_ERR_POLLER_STATE_ENABLED,
	HIF_ERR_POLLER_STATE_STOPPED
} pfe_hif_err_poller_state_t;

typedef struct
/*	The PFE firmware data type */
{
	char_t *version;				/* free text: version */
	char_t *source;				/* free text: filename, filepath, ... etc */
	void *class_data;			/* The CLASS fw data buffer */
	uint32_t class_size;	/* The CLASS fw data size */
	void *tmu_data;				/* The TMU fw data buffer */
	uint32_t tmu_size;		/* The TMU fw data size */
	void *util_data;			/* The UTIL fw data buffer */
	uint32_t util_size;		/* The UTIL fw data size */
} pfe_fw_t;

/*	The PFE platform config */
typedef struct
{
	addr_t cbus_base;		/* PFE control bus base address */
	addr_t cbus_len;		/* PFE control bus size */
	pfe_fw_t *fw;			/* Required firmware, embedded */
	bool_t common_irq_mode;	/* True if FPGA specific common irq is used */
	uint32_t irq_vector_global;		/* Global IRQ number */
	uint32_t irq_vector_bmu;		/* BMU IRQ number */
	pfe_hif_chnl_id_t hif_chnls_mask; /* The bitmap list of the requested HIF channels */
	uint32_t irq_vector_hif_chnls[HIF_CFG_MAX_CHANNELS];	/* HIF channels IRQ number */
	uint32_t irq_vector_hif_nocpy;	/* HIF nocopy channel IRQ number */
} pfe_platform_config_t;

/*	The PFE platform data type */
typedef struct
{
	volatile bool_t probed;		/* Flag indicating that instance has been successfully probed */
	void *cbus_baseaddr;
	void *bmu_buffers_va;
	addr_t bmu_buffers_size;
	void *rtable_va;
	addr_t rtable_size;
	void *apb_baseaddr;
	uint32_t iram_phys_baseaddr;
	void *iram_baseaddr;
	uint32_t ipsec_phys_baseaddr;
	void *ipsec_baseaddr;
	oal_irq_t *irq_global;		/* Common PFE IRQ TODO: FPGA only, remove on S32G */
	oal_irq_isr_handle_t irq_global_isr_handle;
	oal_thread_t *hif_global_err_poller;	/* Thread polling for HIF global errors */
	pfe_hif_err_poller_state_t poller_state;
	oal_irq_t *irq_bmu;			/* BMU IRQ */
	oal_irq_isr_handle_t irq_bmu_isr_handle;
	uint32_t hif_chnl_count;	/* Number of HIF channels */
	oal_irq_t **irq_hif_chnls;	/* array of the HIF channel IRQs */
	oal_irq_isr_handle_t *irq_hif_chnl_isr_handles;
#if defined(GLOBAL_CFG_HIF_NOCPY_SUPPORT)
	oal_irq_t *irq_hif_nocpy;	/* HIF nocopy channel IRQ */
	oal_irq_isr_handle_t irq_hif_nocpy_isr_handle;
#endif /* GLOBAL_CFG_HIF_NOCPY_SUPPORT */
	uint32_t emac_count;		/* Number of EMAC blocks */
	uint32_t gpi_count;			/* Number of GPI blocks */
	uint32_t etgpi_count;		/* Number of ETGPI blocks */
	uint32_t hgpi_count;		/* Number of HGPI blocks */
	uint32_t bmu_count;			/* Number of BMU blocks */
	uint32_t class_pe_count; 	/* Number of CLASS PEs */
	uint32_t util_pe_count; 	/* Number of UTIL PEs */
	uint32_t tmu_pe_count;		/* Number of TMU PEs */
	pfe_fw_t *fw;
	pfe_rtable_t *rtable;		/* The routing table */
	pfe_l2br_table_t *mactab;	/* The MAC table */
	pfe_l2br_table_t *vlantab;	/* The VLAN table */
	pfe_l2br_t *l2_bridge;		/* The L2 bridge */
	pfe_class_t *classifier;	/* The classifier block */
	pfe_tmu_t *tmu;				/* The TMU block */
	pfe_util_t *util;			/* The UTIL block */
	pfe_bmu_t **bmu;			/* The BMU blocks */
	pfe_gpi_t **gpi;			/* The GPI blocks */
	pfe_gpi_t **etgpi;			/* The ETGPI blocks */
	pfe_gpi_t **hgpi;			/* The HGPI blocks */
	pfe_hif_t *hif;				/* The HIF block */
	pfe_hif_nocpy_t *hif_nocpy;	/* The HIF_NOCPY block */
	pfe_emac_t **emac;			/* The EMAC blocks */
	pfe_safety_t *safety;		/* The SAFETY block */
	pfe_if_db_t *phy_if_db;		/* The PFE physical interfaces */
	oal_mutex_t phy_if_db_lock; /* The physical interfaces database mutex */
	pfe_if_db_t *log_if_db;		/* The PFE logical interfaces */
	oal_mutex_t log_if_db_lock; /* The logical interfaces database mutex */
	pfe_hif_drv_t *hif_drv;		/* The HIF driver instance */
	struct dentry *dentry;
	int32_t wake;
	struct clk *hfe_clock;
} pfe_platform_t;

pfe_fw_t *pfe_fw_load(char_t *name);
errno_t pfe_platform_init(pfe_platform_config_t *config);
errno_t pfe_platform_soft_reset(pfe_platform_t *platform);
errno_t pfe_platform_remove(void);
void pfe_platform_print_versions(pfe_platform_t *platform);
pfe_platform_t *pfe_platform_get_instance(void);
pfe_hif_drv_t *pfe_platform_get_hif_drv(pfe_platform_t *platform, uint32_t id);
pfe_log_if_t *pfe_platform_get_log_if_by_id(pfe_platform_t *platform, uint8_t id);
pfe_phy_if_t *pfe_platform_get_phy_if_by_id(pfe_platform_t *platform, pfe_ct_phy_if_id_t id);

#endif /* SRC_PFE_PLATFORM_H_ */

/** @}*/
