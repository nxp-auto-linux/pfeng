/* =========================================================================
 *  Copyright 2018-2020 NXP
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

#ifndef SRC_PFE_PLATFORM_H_
#define SRC_PFE_PLATFORM_H_

#include "pfe_platform_cfg.h"
#include "pfe_pe.h"
#include "pfe_gpi.h"
#include "pfe_bmu.h"
#include "pfe_class.h"
#if defined(PFE_CFG_RTABLE_ENABLE)
	#include "pfe_rtable.h"
#endif /* PFE_CFG_RTABLE_ENABLE */
#include "pfe_tmu.h"
#include "pfe_util.h"
#include "pfe_hif.h"
#include "pfe_hif_nocpy.h"
#include "pfe_safety.h"
#include "pfe_emac.h"
#if defined(PFE_CFG_L2BRIDGE_ENABLE)
	#include "pfe_l2br_table.h"
	#include "pfe_l2br.h"
#endif /* PFE_CFG_L2BRIDGE_ENABLE */
#include "pfe_phy_if.h"
#include "pfe_log_if.h"
#include "pfe_if_db.h"
#include "pfe_wdt.h"

#define GEMAC0_MAC						{ 0x00U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU }
#define GEMAC1_MAC						{ 0x00U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU }
#define GEMAC2_MAC						{ 0x00U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU }

typedef enum
{
	POLLER_STATE_DISABLED,
	POLLER_STATE_ENABLED,
	POLLER_STATE_STOPPED
} pfe_poller_state_t;

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
	char_t *fw_name;		/* FW name */
	pfe_fw_t *fw;			/* Required firmware, embedded */
	bool_t common_irq_mode;	/* True if FPGA specific common irq is used */
	uint32_t irq_vector_global;		/* Global IRQ number */
	uint32_t irq_vector_bmu;		/* BMU IRQ number */
	pfe_hif_chnl_id_t hif_chnls_mask; /* The bitmap list of the requested HIF channels */
	pfe_ct_phy_if_id_t master_if; /* Interface where master driver is located */
	uint32_t irq_vector_hif_chnls[HIF_CFG_MAX_CHANNELS];	/* HIF channels IRQ number */
	uint32_t irq_vector_hif_nocpy;	/* HIF nocopy channel IRQ number */
	uint32_t irq_vector_upe_gpt; /* UPE + GPT IRQ number */
	uint32_t irq_vector_safety; /* Safety IRQ number */
	bool_t enable_util;			/* Shall be UTIL enabled? */
} pfe_platform_config_t;

typedef struct
{
	volatile bool_t probed;
	void *cbus_baseaddr;
	void *bmu_buffers_va;
	addr_t bmu_buffers_size;
	void *rtable_va;
	addr_t rtable_size;
	oal_irq_t *irq_global;
#if defined(PFE_CFG_GLOB_ERR_POLL_WORKER)
	oal_thread_t *poller;	/* Global poller thread */
#endif /* PFE_CFG_GLOB_ERR_POLL_WORKER */
	pfe_poller_state_t poller_state;
	oal_irq_t *irq_bmu;			/* BMU IRQ */
	uint32_t hif_chnl_count;	/* Number of HIF channels */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	pfe_hif_nocpy_t *hif_nocpy;	/* The HIF_NOCPY block */
	oal_irq_t *irq_hif_nocpy;	/* HIF nocopy channel IRQ */
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	uint32_t emac_count;
	uint32_t gpi_count;
	uint32_t etgpi_count;
	uint32_t hgpi_count;
	uint32_t bmu_count;
	uint32_t class_pe_count;
	uint32_t util_pe_count;
	uint32_t tmu_pe_count;
	pfe_fw_t *fw;
#if defined(PFE_CFG_RTABLE_ENABLE)
	pfe_rtable_t *rtable;
#endif /* PFE_CFG_RTABLE_ENABLE */
#if defined(PFE_CFG_L2BRIDGE_ENABLE)
	pfe_l2br_table_t *mactab;
	pfe_l2br_table_t *vlantab;
	pfe_l2br_t *l2_bridge;
#endif /* PFE_CFG_L2BRIDGE_ENABLE */
	pfe_class_t *classifier;
	pfe_tmu_t *tmu;
	pfe_util_t *util;
	pfe_bmu_t **bmu;
	pfe_gpi_t **gpi;
	pfe_gpi_t **etgpi;
	pfe_gpi_t **hgpi;
	pfe_hif_t *hif;
	pfe_emac_t **emac;
	pfe_safety_t *safety;
	pfe_wdt_t 	*wdt;
	pfe_if_db_t *phy_if_db;
	pfe_if_db_t *log_if_db;
	bool_t fci_created;
} pfe_platform_t;

pfe_fw_t *pfe_fw_load(char_t *class_fw_name, char_t *util_fw_name);
errno_t pfe_platform_init(pfe_platform_config_t *config);
errno_t pfe_platform_create_ifaces(pfe_platform_t *platform);
errno_t pfe_platform_soft_reset(pfe_platform_t *platform);
errno_t pfe_platform_remove(void);
void pfe_platform_print_versions(pfe_platform_t *platform);
pfe_platform_t *pfe_platform_get_instance(void);
errno_t pfe_platform_register_log_if(pfe_platform_t *platform, pfe_log_if_t *log_if);
errno_t pfe_platform_unregister_log_if(pfe_platform_t *platform, pfe_log_if_t *log_if);
pfe_log_if_t *pfe_platform_get_log_if_by_id(pfe_platform_t *platform, uint8_t id);
pfe_log_if_t *pfe_platform_get_log_if_by_name(pfe_platform_t *platform, char_t *name);
pfe_phy_if_t *pfe_platform_get_phy_if_by_id(pfe_platform_t *platform, pfe_ct_phy_if_id_t id);
void pfe_platform_idex_rpc_cbk(pfe_ct_phy_if_id_t sender, uint32_t id, void *buf, uint16_t buf_len, void *arg);

#endif /* SRC_PFE_PLATFORM_H_ */
