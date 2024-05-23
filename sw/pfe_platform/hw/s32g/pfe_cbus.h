/* =========================================================================
 *
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_CBUS_H_
#define PFE_CBUS_H_

#define CBUS_LMEM_BASE_ADDR		(0x00000U)
#define CBUS_LMEM_SIZE			(0x20000U)
#define CBUS_TMU_CSR_BASE_ADDR		(0x80000U)
#define CBUS_BMU1_BASE_ADDR		(0x88000U)
#define CBUS_BMU2_BASE_ADDR		(0x8C000U)
#define CBUS_CLASS_CSR_BASE_ADDR	(0x90000U)
#define CBUS_GLOBAL_CSR_BASE_ADDR	(0x94000U)
#define CBUS_HIF_BASE_ADDR		(0x98000U)
#define CBUS_HGPI_BASE_ADDR		(0x9C000U)
#define CBUS_EMAC1_BASE_ADDR		(0xA0000U)
#define CBUS_EMAC2_BASE_ADDR		(0xA4000U)
#define CBUS_EMAC3_BASE_ADDR		(0xA8000U)
#define CBUS_EGPI1_BASE_ADDR		(0xAC000U)
#define CBUS_EGPI2_BASE_ADDR		(0xB0000U)
#define CBUS_EGPI3_BASE_ADDR		(0xB4000U)
#define CBUS_ETGPI1_BASE_ADDR		(0xB8000U)
#define CBUS_ETGPI2_BASE_ADDR		(0xBC000U)
#define CBUS_ETGPI3_BASE_ADDR		(0xC0000U)
#define CBUS_UTIL_CSR_BASE_ADDR		(0xCC000U)
#define CBUS_HIF_NOCPY_BASE_ADDR	(0xD0000U)
#define CBUS_ROUTE_LMEM_ADDR		(0xE0000U)
#define CBUS_ROUTE_LMEM_SIZE		(0x20000U)

#define PFE_CORE_DISABLE		0x00000000U
#define PFE_CORE_ENABLE			0x00000001U
#define PFE_CORE_SW_RESET		0x00000002U

#include "pfe_global_wsp.h"
#include "pfe_class_csr.h"
#include "pfe_tmu_csr.h"
#include "pfe_util_csr.h"
#include "pfe_gpi_csr.h"
#include "pfe_hif_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_emac_csr.h"

#endif /* PFE_CBUS_H_ */
