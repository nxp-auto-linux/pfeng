/* =========================================================================
 *  Copyright 2019 NXP
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
 * @addtogroup  dxgr_PFE_GLOBAL
 * @{
 * 
 * @file		pfe_global_csr.h
 * @brief		The global WSP registers definition file (s32g).
 * @details		Applicable for IP versions listed below.
 *
 */

#ifndef PFE_GLOBAL_WSP_CSR_H_
#define PFE_GLOBAL_WSP_CSR_H_

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

/*	Supported IPs. Defines are validated within pfe_cbus.h. */
#if (GLOBAL_CFG_IP_VERSION != IP_VERSION_FPGA_5_0_4) && (GLOBAL_CFG_IP_VERSION != IP_VERSION_NPU_7_14)
#error Unsupported IP version
#endif /* GLOBAL_CFG_IP_VERSION */

/*	CBUS offsets */
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

#if (GLOBAL_CFG_IP_VERSION == IP_VERSION_NPU_7_14)
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
#endif /* IP_VERSION_NPU_7_14 */

#if (GLOBAL_CFG_IP_VERSION == IP_VERSION_FPGA_5_0_4)
/*	TODO: Taken from NPU 7.6. Remove this comment once confirmed
 	that FPGA is based on 7.6. Ticket no.: 123703. */
#define WDT_INT_SRC				(0x58U)
#define WDT_TIMER_VAL_1			(0x5cU)
#define WDT_TIMER_VAL_2			(0x60U)
#define WDT_TIMER_VAL_3			(0x64U)
#define WDT_TIMER_VAL_4			(0x68U)
#define WSP_DBUG_BUS1			(0x70U)
#endif /* IP_VERSION_FPGA_5_0_4 */

/* WSP_SAFETY_INT_SRC bits*/
#define	SAFETY_INT				(1U << 0)
#define	MASTER1_INT				(1U << 1)
#define	MASTER2_INT				(1U << 2)
#define	MASTER3_INT				(1U << 3)
#define	MASTER4_INT				(1U << 4)
#define	EMAC_CBUS_INT			(1U << 5)
#define	EMAC_DBUS_INT			(1U << 6)
#define	CLASS_CBUS_INT			(1U << 7)
#define	CLASS_DBUS_INT			(1U << 8)
#define	TMU_CBUS_INT			(1U << 9)
#define	TMU_DBUS_INT			(1U << 10)
#define	HIF_CBUS_INT			(1U << 11)
#define	HIF_DBUS_INT			(1U << 12)
#define	HIF_NOCPY_CBUS_INT		(1U << 13)
#define	HIF_NOCPY_DBUS_INT		(1U << 14)
#define	UPE_CBUS_INT			(1U << 15)
#define	UPE_DBUS_INT			(1U << 16)
#define	HRS_CBUS_INT			(1U << 17)
#define	BRIDGE_CBUS_INT			(1U << 18)
#define EMAC_SLV_INT			(1U << 19)
#define	BMU1_SLV_INT			(1U << 20)
#define	BMU2_SLV_INT			(1U << 21)
#define	CLASS_SLV_INT			(1U << 22)
#define	HIF_SLV_INT				(1U << 23)
#define	HIF_NOCPY_SLV_INT		(1U << 24)
#define	LMEM_SLV_INT			(1U << 25)
#define	TMU_SLV_INT				(1U << 26)
#define	UPE_SLV_INT				(1U << 27)
#define	WSP_GLOBAL_SLV_INT		(1U << 28)

/* WSP_SAFETY_INT_EN bits*/
#define	SAFETY_INT_EN			(1U << 0)
#define	MASTER1_INT_EN 			(1U << 1)
#define	MASTER2_INT_EN			(1U << 2)
#define	MASTER3_INT_EN			(1U << 3)
#define	MASTER4_INT_EN			(1U << 4)
#define	EMAC_CBUS_INT_EN 		(1U << 5)
#define	EMAC_DBUS_INT_EN 		(1U << 6)
#define	CLASS_CBUS_INT_EN 		(1U << 7)
#define	CLASS_DBUS_INT_EN 		(1U << 8)
#define	TMU_CBUS_INT_EN 		(1U << 9)
#define	TMU_DBUS_INT_EN 		(1U << 10)
#define	HIF_CBUS_INT_EN 		(1U << 11)
#define	HIF_DBUS_INT_EN 		(1U << 12)
#define	HIF_NOCPY_CBUS_INT_EN 	(1U << 13)
#define	HIF_NOCPY_DBUS_INT_EN 	(1U << 14)
#define	UPE_CBUS_INT_EN 		(1U << 15)
#define	UPE_DBUS_INT_EN 		(1U << 16)
#define	HRS_CBUS_INT_EN 		(1U << 17)
#define	BRIDGE_CBUS_INT_EN 		(1U << 18)
#define EMAC_SLV_INT_EN 		(1U << 19)
#define	BMU1_SLV_INT_EN 		(1U << 20)
#define	BMU2_SLV_INT_EN 		(1U << 21)
#define	CLASS_SLV_INT_EN 		(1U << 22)
#define	HIF_SLV_INT_EN 			(1U << 23)
#define	HIF_NOCPY_SLV_INT_EN 	(1U << 24)
#define	LMEM_SLV_INT_EN 		(1U << 25)
#define	TMU_SLV_INT_EN 			(1U << 26)
#define	UPE_SLV_INT_EN 			(1U << 27)
#define	WSP_GLOBAL_SLV_INT_EN 	(1U << 28)

#define	SAFETY_INT_ENABLE_ALL	0x1FFFFFFFU

#endif /* PFE_GLOBAL_WSP_CSR_H_ */

/** @}*/
