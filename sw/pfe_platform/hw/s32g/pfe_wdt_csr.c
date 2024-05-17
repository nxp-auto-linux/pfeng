/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2020-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_wdt_csr.h"
#include "pfe_feature_mgr.h"
#include "pfe_hm.h"

#define WDT_INT_SRC_NUMBER_G2 11U
#define WDT_INT_SRC_NUMBER_G3 18U

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		WDT ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * 				Every WDT instance has its own handler. Access to registers is
 * 				protected by mutex implemented within the WDT module (pfe_wdt.c).
 * @param[in]	base_va WDT register space base address (virtual)
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-BMU mutex
 */
errno_t pfe_wdt_cfg_isr(addr_t base_va, addr_t cbus_base_va)
{
	uint8_t index = 0U;
	uint32_t reg_en, reg_src, reg_reen = 0U;
	errno_t ret = ENOENT;

	/* G2 WDT bits */
	static const uint32_t wdt_int_src_arr_g2[WDT_INT_SRC_NUMBER_G2] =
	{
		WDT_BMU1_WDT_INT_G2, WDT_BMU2_WDT_INT_G2, WDT_CLASS_WDT_INT_G2, WDT_EMAC0_GPI_WDT_INT_G2,
		WDT_EMAC1_GPI_WDT_INT_G2, WDT_EMAC2_GPI_WDT_INT_G2, WDT_HIF_GPI_WDT_INT_G2,
		WDT_HIF_NOCPY_WDT_INT_G2, WDT_HIF_WDT_INT_G2, WDT_TLITE_WDT_INT_G2, WDT_UTIL_WDT_INT_G2
	};
	static const uint32_t wdt_int_en_arr_g2[WDT_INT_SRC_NUMBER_G2] =
	{
		WDT_BMU1_WDT_INT_EN_BIT_G2, WDT_BMU2_WDT_INT_EN_BIT_G2, WDT_CLASS_WDT_INT_EN_BIT_G2,
		WDT_EMAC0_GPI_WDT_INT_EN_BIT_G2, WDT_EMAC1_GPI_WDT_INT_EN_BIT_G2, WDT_EMAC2_GPI_WDT_INT_EN_BIT_G2,
		WDT_HIF_GPI_WDT_INT_EN_BIT_G2, WDT_HIF_NOCPY_WDT_INT_EN_BIT_G2, WDT_HIF_WDT_INT_EN_BIT_G2,
		WDT_TLITE_WDT_INT_EN_BIT_G2, WDT_UTIL_PE_WDT_INT_EN_BIT_G2
	};

	static const pfe_hm_evt_t wdt_int_event_id_g2[WDT_INT_SRC_NUMBER_G2] =
	{
		HM_EVT_WDT_BMU1, HM_EVT_WDT_BMU2, HM_EVT_WDT_CLASS, HM_EVT_WDT_EMAC0_GPI,
		HM_EVT_WDT_EMAC1_GPI, HM_EVT_WDT_EMAC2_GPI, HM_EVT_WDT_HIF_GPI, HM_EVT_WDT_HIF_NOCPY,
		HM_EVT_WDT_HIF, HM_EVT_WDT_TLITE, HM_EVT_WDT_UTIL_PE
	};

	/* G3 WDT bits */
	static const uint32_t wdt_int_src_arr_g3[WDT_INT_SRC_NUMBER_G3] =
	{
		WDT_BMU1_WDT_INT, WDT_BMU2_WDT_INT, WDT_CLASS_WDT_INT, WDT_EMAC0_GPI_WDT_INT,
		WDT_EMAC1_GPI_WDT_INT, WDT_EMAC2_GPI_WDT_INT, WDT_HIF_GPI_WDT_INT,
		WDT_HIF_NOCPY_WDT_INT, WDT_HIF_WDT_INT, WDT_TLITE_WDT_INT, WDT_UTIL_PE_WDT_INT,
		WDT_EMAC0_ETGPI_WDT_INT, WDT_EMAC1_ETGPI_WDT_INT, WDT_EMAC2_ETGPI_WDT_INT,
		WDT_EXT_GPT1_WDT_INT, WDT_EXT_GPT2_WDT_INT, WDT_LMEM_WDT_INT, WDT_ROUTE_LMEM_WDT_INT
	};
	static const uint32_t wdt_int_en_arr_g3[WDT_INT_SRC_NUMBER_G3] =
	{
		WDT_BMU1_WDT_INT_EN_BIT, WDT_BMU2_WDT_INT_EN_BIT, WDT_CLASS_WDT_INT_EN_BIT, WDT_EMAC0_GPI_WDT_INT_EN_BIT,
		WDT_EMAC1_GPI_WDT_INT_EN_BIT, WDT_EMAC2_GPI_WDT_INT_EN_BIT, WDT_HIF_GPI_WDT_INT_EN_BIT,
		WDT_HIF_NOCPY_WDT_INT_EN_BIT, WDT_HIF_WDT_INT_EN_BIT, WDT_TLITE_WDT_INT_EN_BIT, WDT_UTIL_PE_WDT_INT_EN_BIT,
		WDT_EMAC0_ETGPI_WDT_INT_EN_BIT, WDT_EMAC1_ETGPI_WDT_INT_EN_BIT, WDT_EMAC2_ETGPI_WDT_INT_EN_BIT,
		WDT_EXT_GPT1_WDT_INT_EN_BIT, WDT_EXT_GPT2_WDT_INT_EN_BIT, WDT_LMEM_WDT_INT_EN_BIT, WDT_ROUTE_LMEM_WDT_INT_EN_BIT
	};

	static const pfe_hm_evt_t wdt_int_event_id_g3[WDT_INT_SRC_NUMBER_G3] =
	{
		HM_EVT_WDT_BMU1, HM_EVT_WDT_BMU2, HM_EVT_WDT_CLASS, HM_EVT_WDT_EMAC0_GPI,
		HM_EVT_WDT_EMAC1_GPI, HM_EVT_WDT_EMAC2_GPI, HM_EVT_WDT_HIF_GPI, HM_EVT_WDT_HIF_NOCPY,
		HM_EVT_WDT_HIF, HM_EVT_WDT_TLITE, HM_EVT_WDT_UTIL_PE, HM_EVT_WDT_EMAC0_ETGPI,
		HM_EVT_WDT_EMAC1_ETGPI, HM_EVT_WDT_EMAC2_ETGPI, HM_EVT_WDT_EXT_GPT1,
		HM_EVT_WDT_EXT_GPT2, HM_EVT_WDT_LMEM, HM_EVT_WDT_ROUTE_LMEM
	};

	uint32_t *int_src_arr, *int_en_arr;
	const pfe_hm_evt_t *int_event_arr;
	uint8_t int_src_nbr = 0;

	(void)cbus_base_va;

	if (FALSE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		int_src_arr = (uint32_t *)wdt_int_src_arr_g2;
		int_en_arr = (uint32_t *)wdt_int_en_arr_g2;
		int_event_arr = wdt_int_event_id_g2;
		int_src_nbr = (uint8_t)WDT_INT_SRC_NUMBER_G2;
	}
	else
	{
		int_src_arr = (uint32_t *)wdt_int_src_arr_g3;
		int_en_arr = (uint32_t *)wdt_int_en_arr_g3;
		int_event_arr = wdt_int_event_id_g3;
		int_src_nbr = (uint8_t)WDT_INT_SRC_NUMBER_G3;
	}

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WDT_INT_EN);
	/*	Mask ALL interrupts */
	hal_write32(0U, base_va + WDT_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WDT_INT_SRC);
	/*	ACK triggered */
	hal_write32(reg_src, base_va + WDT_INT_SRC);

	/*	Process interrupts which are triggered AND enabled */
	for(index = 0U; index < int_src_nbr; index++)
	{
		if (((reg_src & int_src_arr[index]) != 0U) && ((reg_en & int_en_arr[index]) != 0U))
		{
			pfe_hm_report_error(HM_SRC_WDT, int_event_arr[index], "");
			reg_reen |= int_en_arr[index];
			ret = EOK;
		}
	}

	/*	Don't re-enable triggered ones since they can't be cleared until PFE
		is reset. Also don't reset master enable bit which is controlled
		by dedicated API (pfe_wdt_cfg_irq_mask/pfe_wdt_cfg_irq_unmask). */
	hal_write32((reg_en & ~reg_reen), base_va + WDT_INT_EN); /*	Enable the non-triggered ones only */

	return ret;
}

/**
 * @brief		Mask WDT interrupts
 * @param[in]	base_va Base address of the WDT register space
 */
void pfe_wdt_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WDT_INT_EN) & ~(WDT_INT_EN_BIT);
	hal_write32(reg, base_va + WDT_INT_EN);
}

/**
 * @brief		Unmask WDT interrupts
 * @param[in]	base_va Base address of the WDT register space
 */
void pfe_wdt_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WDT_INT_EN) | WDT_INT_EN_BIT;
	hal_write32(reg, base_va + WDT_INT_EN);
}

/**
 * @brief		init WDT interrupts
 * @param[in]	base_va Base address of the wsp register space
 */
void pfe_wdt_cfg_init(addr_t base_va)
{
	uint32_t reg;

	/*	Disable the WDT interrupts */
	reg = hal_read32(base_va + WDT_INT_EN) & ~(WDT_INT_EN_BIT);
	hal_write32(reg, base_va + WDT_INT_EN);

	/*	Clear WDT interrupts */
	reg = hal_read32(base_va + WDT_INT_SRC);
	hal_write32(reg, base_va + WDT_INT_SRC);

	/*	Set default watchdog timer values. */
	/*	TODO: What are real values able to precisely reveal runtime stall? */
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_UPE);
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_BMU);
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_HIF);
	hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_TLITE);

	if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		/*	G3 watchdog default values */
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_HIF_NCPY);
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_CLASS);
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_GPI);
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_GPT);
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_LMEM);
		hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_ROUTE_LMEM);
	}

	/*	Enable ALL particular watchdogs */
	hal_write32(0xFFFFFFU, base_va + CLASS_WDT_INT_EN);
	hal_write32(0xFU, base_va + UPE_WDT_INT_EN);
	hal_write32(0x0ABU, base_va + HGPI_WDT_INT_EN); /* Mask false positive bits */
	hal_write32(0xCU, base_va + HIF_WDT_INT_EN);	/* Mask false positive bits */
	hal_write32(0xFFFFFFU, base_va + TLITE_WDT_INT_EN);
	hal_write32(0x3FU, base_va + HNCPY_WDT_INT_EN);
	hal_write32(0xFU, base_va + BMU1_WDT_INT_EN);
	hal_write32(0xFU, base_va + BMU2_WDT_INT_EN);
	hal_write32(0xFFFU, base_va + EMAC0_WDT_INT_EN);
	hal_write32(0xFFFU, base_va + EMAC1_WDT_INT_EN);
	hal_write32(0xFFFU, base_va + EMAC2_WDT_INT_EN);

	if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		/*	G3 watchdogs */
		hal_write32(0x3U, base_va + EXT_GPT_WDT_INT_EN);
		hal_write32(0x3U, base_va + LMEM_WDT_INT_EN);
	}

	/*	Enable WDT interrupts except of the global enable bit */
	hal_write32((0xffffffffU & ~(WDT_INT_EN_BIT)), base_va + WDT_INT_EN);
}

/**
 * @brief		Clear the WDT interrupt control and status registers
 * @param[in]	base_va Base address of HIF register space (virtual)
 */
void pfe_wdt_cfg_fini(addr_t base_va)
{
	uint32_t reg;

	/*	Disable and clear WDT interrupts */
	hal_write32(0x0U, base_va + WDT_INT_EN);
	reg = hal_read32(base_va + WDT_INT_SRC);
	hal_write32(reg, base_va + WDT_INT_SRC);
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

/**
 * @brief		Get WDT statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the WDT block.
 * @param[in]	base_va 	Base address of WDT register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_wdt_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va) || (NULL == buf))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(verb_level >= 9U)
		{
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "base_va              : 0x%x\n", (uint_t)base_va);
			/*	Get version of wsp (wdt is part of wsp)*/
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WSP Version          : 0x%x\n", hal_read32(base_va + WSP_VERSION));
		}
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_INT_EN           : 0x%x\n", hal_read32(base_va + WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + CLASS_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "UPE_WDT_INT_EN       : 0x%x\n", hal_read32(base_va + UPE_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HGPI_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + HGPI_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_WDT_INT_EN       : 0x%x\n", hal_read32(base_va + HIF_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TLITE_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + TLITE_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HNCPY_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + HNCPY_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU1_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + BMU1_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU2_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + BMU2_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC0_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC0_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC1_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC1_WDT_INT_EN));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC2_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC2_WDT_INT_EN));
			if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
			{
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EXT_GPT_WDT_INT_EN   : 0x%x\n", hal_read32(base_va + EXT_GPT_WDT_INT_EN));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "LMEM_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + LMEM_WDT_INT_EN));
			}
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_INT_SRC          : 0x%x\n", hal_read32(base_va + WDT_INT_SRC));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_UPE    : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_UPE));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_BMU    : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_BMU));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_HIF    : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_HIF));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_TLITE  : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_TLITE));
			if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
			{
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_HIF_NCPY 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_HIF_NCPY));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_CLASS  : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_CLASS));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_GPI    : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_GPI));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_GPT    : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_GPT));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_LMEM   : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_LMEM));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_RT_LMEM: 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_ROUTE_LMEM));
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WSP_DBUG_BUS1_G3     : 0x%x\n", hal_read32(base_va + WSP_DBUG_BUS1_G3));
			}
			else
			{
				len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WSP_DBUG_BUS1        : 0x%x\n", hal_read32(base_va + WSP_DBUG_BUS1));
			}
	}

	return len;
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

