/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_GLOBAL_WSP_CSR_H_
#define PFE_GLOBAL_WSP_CSR_H_

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

#define WSP_VERSION				(0x00U)
#define WSP_CLASS_PE_CNT		(0x04U)
#define WSP_PE_IMEM_DMEM_SIZE	(0x08U)
#define WSP_LMEM_SIZE			(0x0cU)
#define WSP_TMU_EMAC_PORT_COUNT	(0x10U)
#define WSP_EGPIS_PHY_NO		(0x14U)
#define WSP_HIF_SUPPORT_PHY_NO	(0x18U)
#define WSP_CLASS_HW_SUPPORT	(0x1cU)
#define WSP_SYS_GENERIC_CONTROL	(0x20U)
#define WSP_SYS_GENERIC_STATUS	(0x24U)
#define WSP_SYS_GEN_CON0		(0x28U)
#define WSP_SYS_GEN_CON1		(0x2cU)
#define WSP_SYS_GEN_CON2		(0x30U)
#define WSP_SYS_GEN_CON3		(0x34U)
#define WSP_SYS_GEN_CON4		(0x38U)
#define WSP_DBUG_BUS			(0x3cU)
#define WSP_CLK_FRQ				(0x40U)
#define WSP_EMAC_CLASS_CONFIG	(0x44U)
#define WSP_EGPIS_PHY_NO1		(0x48U)
#define WSP_SAFETY_INT_SRC		(0x4cU)
#define WSP_SAFETY_INT_EN		(0x50U)
#define WDT_INT_EN				(0x54U)

#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
#define CLASS_WDT_INT_EN		(0x58U)
#define UPE_WDT_INT_EN			(0x5cU)
#define HGPI_WDT_INT_EN			(0x60U)
#define HIF_WDT_INT_EN			(0x64U)
#define TLITE_WDT_INT_EN		(0x68U)
#define HNCPY_WDT_INT_EN		(0x6cU)
#define BMU1_WDT_INT_EN			(0x70U)
#define BMU2_WDT_INT_EN			(0x74U)
#define EMAC0_WDT_INT_EN		(0x78U)
#define EMAC1_WDT_INT_EN		(0x7cU)
#define EMAC2_WDT_INT_EN		(0x80U)
#define WDT_INT_SRC				(0x84U)
#define WDT_TIMER_VAL_1			(0x88U)
#define WDT_TIMER_VAL_2			(0x8cU)
#define WDT_TIMER_VAL_3			(0x90U)
#define WDT_TIMER_VAL_4			(0x94U)
#define WSP_DBUG_BUS1			(0x98U)
#endif /* PFE_CFG_IP_VERSION_NPU_7_14 */

#if (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_FPGA_5_0_4)
#define WDT_INT_SRC				(0x58U)
#define WDT_TIMER_VAL_1			(0x5cU)
#define WDT_TIMER_VAL_2			(0x60U)
#define WDT_TIMER_VAL_3			(0x64U)
#define WDT_TIMER_VAL_4			(0x68U)
#define WSP_DBUG_BUS1			(0x70U)
#endif /* PFE_CFG_IP_VERSION_FPGA_5_0_4 */

/*	WDT_IN_EN bits */
#define WDT_INT_EN_BIT					(1UL << 0U)
#define WDT_CLASS_WDT_INT_EN_BIT		(1UL << 1U)
#define WDT_UTIL_PE_WDT_INT_EN_BIT		(1UL << 2U)
#define WDT_HIF_GPI_WDT_INT_EN_BIT		(1UL << 3U)
#define WDT_HIF_WDT_INT_EN_BIT			(1UL << 4U)
#define WDT_TLITE_WDT_INT_EN_BIT		(1UL << 5U)
#define WDT_HIF_NOCPY_WDT_INT_EN_BIT	(1UL << 6U)
#define WDT_BMU1_WDT_INT_EN_BIT			(1UL << 7U)
#define WDT_BMU2_WDT_INT_EN_BIT			(1UL << 8U)
#define WDT_EMAC0_GPI_WDT_INT_EN_BIT	(1UL << 9U)
#define WDT_EMAC1_GPI_WDT_INT_EN_BIT	(1UL << 10U)
#define WDT_EMAC2_GPI_WDT_INT_EN_BIT	(1UL << 11U)

/*	WDT_INT_SRC bits*/
#define WDT_INT					(1UL << 0U)
#define WDT_BMU1_WDT_INT		(1UL << 1U)
#define WDT_BMU2_WDT_INT		(1UL << 2U)
#define WDT_CLASS_WDT_INT		(1UL << 3U)
#define WDT_EMAC0_GPI_WDT_INT	(1UL << 4U)
#define WDT_EMAC1_GPI_WDT_INT	(1UL << 5U)
#define WDT_EMAC2_GPI_WDT_INT	(1UL << 6U)
#define WDT_HIF_GPI_WDT_INT		(1UL << 7U)
#define WDT_HIF_NOCPY_WDT_INT	(1UL << 8U)
#define WDT_HIF_WDT_INT			(1UL << 9U)
#define WDT_TLITE_WDT_INT		(1UL << 10U)
#define WDT_UTIL_WDT_INT		(1UL << 11U)

/* WSP_SAFETY_INT_SRC bits*/
#define	SAFETY_INT				(1UL << 0U)
#define	MASTER1_INT				(1UL << 1U)
#define	MASTER2_INT				(1UL << 2U)
#define	MASTER3_INT				(1UL << 3U)
#define	MASTER4_INT				(1UL << 4U)
#define	EMAC_CBUS_INT			(1UL << 5U)
#define	EMAC_DBUS_INT			(1UL << 6U)
#define	CLASS_CBUS_INT			(1UL << 7U)
#define	CLASS_DBUS_INT			(1UL << 8U)
#define	TMU_CBUS_INT			(1UL << 9U)
#define	TMU_DBUS_INT			(1UL << 10U)
#define	HIF_CBUS_INT			(1UL << 11U)
#define	HIF_DBUS_INT			(1UL << 12U)
#define	HIF_NOCPY_CBUS_INT		(1UL << 13U)
#define	HIF_NOCPY_DBUS_INT		(1UL << 14U)
#define	UPE_CBUS_INT			(1UL << 15U)
#define	UPE_DBUS_INT			(1UL << 16U)
#define	HRS_CBUS_INT			(1UL << 17U)
#define	BRIDGE_CBUS_INT			(1UL << 18U)
#define EMAC_SLV_INT			(1UL << 19U)
#define	BMU1_SLV_INT			(1UL << 20U)
#define	BMU2_SLV_INT			(1UL << 21U)
#define	CLASS_SLV_INT			(1UL << 22U)
#define	HIF_SLV_INT				(1UL << 23U)
#define	HIF_NOCPY_SLV_INT		(1UL << 24U)
#define	LMEM_SLV_INT			(1UL << 25U)
#define	TMU_SLV_INT				(1UL << 26U)
#define	UPE_SLV_INT				(1UL << 27U)
#define	WSP_GLOBAL_SLV_INT		(1UL << 28U)

/* WSP_SAFETY_INT_EN bits*/
#define	SAFETY_INT_EN			(1UL << 0U)
#define	MASTER1_INT_EN 			(1UL << 1U)
#define	MASTER2_INT_EN			(1UL << 2U)
#define	MASTER3_INT_EN			(1UL << 3U)
#define	MASTER4_INT_EN			(1UL << 4U)
#define	EMAC_CBUS_INT_EN 		(1UL << 5U)
#define	EMAC_DBUS_INT_EN 		(1UL << 6U)
#define	CLASS_CBUS_INT_EN 		(1UL << 7U)
#define	CLASS_DBUS_INT_EN 		(1UL << 8U)
#define	TMU_CBUS_INT_EN 		(1UL << 9U)
#define	TMU_DBUS_INT_EN 		(1UL << 10U)
#define	HIF_CBUS_INT_EN 		(1UL << 11U)
#define	HIF_DBUS_INT_EN 		(1UL << 12U)
#define	HIF_NOCPY_CBUS_INT_EN 	(1UL << 13U)
#define	HIF_NOCPY_DBUS_INT_EN 	(1UL << 14U)
#define	UPE_CBUS_INT_EN 		(1UL << 15U)
#define	UPE_DBUS_INT_EN 		(1UL << 16U)
#define	HRS_CBUS_INT_EN 		(1UL << 17U)
#define	BRIDGE_CBUS_INT_EN 		(1UL << 18U)
#define EMAC_SLV_INT_EN 		(1UL << 19U)
#define	BMU1_SLV_INT_EN 		(1UL << 20U)
#define	BMU2_SLV_INT_EN 		(1UL << 21U)
#define	CLASS_SLV_INT_EN 		(1UL << 22U)
#define	HIF_SLV_INT_EN 			(1UL << 23U)
#define	HIF_NOCPY_SLV_INT_EN 	(1UL << 24U)
#define	LMEM_SLV_INT_EN 		(1UL << 25U)
#define	TMU_SLV_INT_EN 			(1UL << 26U)
#define	UPE_SLV_INT_EN 			(1UL << 27U)
#define	WSP_GLOBAL_SLV_INT_EN 	(1UL << 28U)

#define	SAFETY_INT_ENABLE_ALL	0x1FFFFFFFU

#endif /* PFE_GLOBAL_WSP_CSR_H_ */
