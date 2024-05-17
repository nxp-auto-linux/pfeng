/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_gpi.h"

#define IGQOS_BITMAP_ARR_SZ 2U
#define BITMAP_BITS_U32     32U
#define DECLARE_BITMAP_U32(name, SIZE) \
	uint32_t name[SIZE]

/* PFE uses the value of 32 to represent the 6 bit encoding of the IP address mask of 0 */
#define IGQOS_IP_MASK_0 32U

struct pfe_gpi_tag
{
	addr_t cbus_base_va;    /* CBUS base virtual address */
	addr_t gpi_base_offset; /* GPI base offset within CBUS space */
	addr_t gpi_base_va;     /* GPI base address (virtual) */

	/* bitmap of all (PFE_IQOS_FLOW_TABLE_SIZE) active classification table entries */
	DECLARE_BITMAP_U32(igqos_active_entries, IGQOS_BITMAP_ARR_SZ);
	uint8_t  igqos_entry_iter; /* classification table active entries interator */
	uint32_t sys_clk_mhz;
	uint32_t clk_div_log2;
};
ct_assert(PFE_IQOS_FLOW_TABLE_SIZE <= (BITMAP_BITS_U32 * IGQOS_BITMAP_ARR_SZ));

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err);
static errno_t pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err);
static void igqos_class_clear_active_all(pfe_gpi_t *gpi);
static errno_t igqos_entry_ready_timeout(const pfe_gpi_t *gpi);
static errno_t igqos_class_clear_flow_entry_table(const pfe_gpi_t *gpi);
static errno_t igqos_class_clear_lru_entry_table(const pfe_gpi_t *gpi);
static void igqos_class_set_active(pfe_gpi_t *gpi, uint8_t id);
static void igqos_class_clear_active(pfe_gpi_t *gpi, uint8_t id);
static bool_t igqos_class_is_active(const pfe_gpi_t *gpi, uint8_t id);
static uint8_t igqos_class_find_entry(const pfe_gpi_t *gpi, uint8_t start, bool_t is_active);
static uint8_t igqos_class_find_first_free(const pfe_gpi_t *gpi);
static uint8_t igqos_class_get_first_active(pfe_gpi_t *gpi);
static uint8_t igqos_class_get_next_active(pfe_gpi_t *gpi);
static uint8_t igqos_ip_mask_hw_encode(uint8_t ip_m);
static uint8_t igqos_ip_mask_hw_decode(uint8_t ip_m);
static void igqos_convert_entry_to_flow(const uint32_t entry[], pfe_iqos_flow_spec_t *flow);
static void igqos_convert_flow_to_entry(const pfe_iqos_flow_spec_t *flow, uint32_t entry[]);
static errno_t pfe_gpi_shp_args_checks(const pfe_gpi_t *gpi, uint8_t id);
static uint32_t igqos_clk_div(uint32_t clk_div_log2);
static uint32_t igqos_convert_isl_to_weight(uint32_t isl, uint32_t clk_div_log2, uint32_t sys_clk_mhz, bool_t is_bps);
static uint32_t igqos_convert_weight_to_isl(uint32_t wgt, uint32_t clk_div_log2, uint32_t sys_clk_mhz, bool_t is_bps);
static uint32_t igqos_find_optimal_weight(uint32_t isl, uint32_t sys_clk_mhz, bool_t is_bps, uint32_t *wgt);

/**
 * @brief		Create new GPI instance
 * @details		Creates and initializes GPI instance. The new instance is disabled and needs
 * 				to be enabled by pfe_gpi_enable().
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	gpi_base BMU base address offset within CBUS address space
 * @param[in]	cfg The BMU block configuration
 * @return		The BMU instance or NULL if failed
 */
pfe_gpi_t *pfe_gpi_create(addr_t cbus_base_va, addr_t gpi_base, const pfe_gpi_cfg_t *cfg)
{
	addr_t     gpi_cbus_offset;
	pfe_gpi_t *gpi;
	errno_t    ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		gpi = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		gpi = (pfe_gpi_t *)oal_mm_malloc(sizeof(pfe_gpi_t));

		if (NULL == gpi)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
		else
		{
			(void)memset(gpi, 0, sizeof(pfe_gpi_t));
			gpi->cbus_base_va    = cbus_base_va;
			gpi->gpi_base_offset = gpi_base;
			gpi->gpi_base_va     = (gpi->cbus_base_va + gpi->gpi_base_offset);
			gpi->sys_clk_mhz     = pfe_gpi_cfg_get_sys_clk_mhz(cbus_base_va);
			gpi_cbus_offset      = gpi->gpi_base_va - cbus_base_va;

			ret = pfe_gpi_reset(gpi);
			if (EOK != ret)
			{
				oal_mm_free(gpi);
				gpi = NULL;
			}
			else
			{
				switch (gpi_cbus_offset)
				{
					case CBUS_EGPI1_BASE_ADDR:
					case CBUS_EGPI2_BASE_ADDR:
					case CBUS_EGPI3_BASE_ADDR:
						/*
						* includes initialization of CLASS tables
						* required by the ECC module init
						*/
						ret = pfe_gpi_qos_reset(gpi);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("GPI QOS reset timed-out\n");
							oal_mm_free(gpi);
							gpi = NULL;
						}
						break;
					default:
						/* Do Nothing */
						break;
				}

				if (NULL != gpi)
				{
					pfe_gpi_disable(gpi);

					pfe_gpi_cfg_init(gpi->gpi_base_va, cfg);
				}
			}
		}
	}

	return gpi;
}

#if defined(PFE_CFG_NULL_ARG_CHECK)
static errno_t pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err)
{
	errno_t ret = EOK;

	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = err;
	}

	return ret;
}
#else
static errno_t pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err)
{
	(void)gpi;
	(void)err;
	return EOK;
}
#endif /* PFE_CFG_NULL_ARG_CHECK */
/**
 * @brief		Reset the GPI block
 * @param[in]	gpi The GPI instance
 */
errno_t pfe_gpi_reset(const pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		ret = pfe_gpi_cfg_reset(gpi->gpi_base_va);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("GPI reset timed-out\n");
		}
	}

	return ret;
}

/**
 * @brief		Enable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_enable(const pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		pfe_gpi_cfg_enable(gpi->gpi_base_va);
	}
}

/**
 * @brief		Disable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_disable(const pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		pfe_gpi_cfg_disable(gpi->gpi_base_va);
	}
}

/**
 * @brief		Destroy GPI instance
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_destroy(pfe_gpi_t *gpi)
{
	errno_t ret;
	if (NULL != gpi)
	{
		pfe_gpi_disable(gpi);

		if ((gpi->gpi_base_offset == CBUS_EGPI1_BASE_ADDR) || (gpi->gpi_base_offset == CBUS_EGPI2_BASE_ADDR) || (gpi->gpi_base_offset == CBUS_EGPI3_BASE_ADDR))
		{
			ret = pfe_gpi_qos_reset(gpi);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("GPI QOS reset timed-out\n");
			}
		}

		ret = pfe_gpi_reset(gpi);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("GPI reset timed-out\n");
		}

		oal_mm_free(gpi);
	}
}

/* Ingress QoS support */

bool_t pfe_gpi_qos_is_enabled(const pfe_gpi_t *gpi)
{
	bool_t  is_enabled;
	errno_t ret        = pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret == EOK)
	{
		is_enabled = pfe_gpi_cfg_qos_is_enabled(gpi->gpi_base_va);
	}
	else
	{
		is_enabled = FALSE;
	}

	return is_enabled;
}

static void igqos_class_clear_active_all(pfe_gpi_t *gpi)
{
	uint32_t i;

	for (i = 0U; i < IGQOS_BITMAP_ARR_SZ; i++)
	{
		gpi->igqos_active_entries[i] = 0;
	}

	gpi->igqos_entry_iter = 0;
}

static errno_t igqos_entry_ready_timeout(const pfe_gpi_t *gpi)
{
	errno_t  ret     = EOK;
	uint32_t timeout = 200U;
	bool_t   ready;

	while (timeout > 0U)
	{
		ready = pfe_gpi_cfg_qos_entry_ready(gpi->gpi_base_va);
		if (TRUE == ready)
		{
			break;
		}
		oal_time_usleep(5U);
		timeout--;
	}

	if ((0U == timeout) && (FALSE == ready))
	{
		ready = pfe_gpi_cfg_qos_entry_ready(gpi->gpi_base_va);
	}

	if (FALSE == ready)
	{
		ret = ETIMEDOUT;
	}

	return ret;
}

static errno_t igqos_class_clear_flow_entry_table(const pfe_gpi_t *gpi)
{
	uint32_t ii;
	errno_t  ret;

	for (ii = 0U; ii < ENTRY_TABLE_SIZE; ii++)
	{
		pfe_gpi_cfg_qos_clear_flow_entry_req(gpi->gpi_base_va, ii);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK != ret)
		{
			break;
		}
	}

	return ret;
}

static errno_t igqos_class_clear_lru_entry_table(const pfe_gpi_t *gpi)
{
	uint32_t ii;
	errno_t  ret;

	for (ii = 0U; ii < ENTRY_TABLE_SIZE; ii++)
	{
		pfe_gpi_cfg_qos_clear_lru_entry_req(gpi->gpi_base_va, ii);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK != ret)
		{
			break;
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_reset(pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		ret = igqos_class_clear_flow_entry_table((const pfe_gpi_t *)gpi);
		if (EOK == ret)
		{
			ret = igqos_class_clear_lru_entry_table((const pfe_gpi_t *)gpi);
			if (EOK == ret)
			{
				pfe_gpi_cfg_qos_default_init(gpi->gpi_base_va);

				/* clear driver state */
				igqos_class_clear_active_all(gpi);
			}
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_enable(pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		if (TRUE != pfe_gpi_cfg_qos_is_enabled(gpi->gpi_base_va))
		{
			ret = pfe_gpi_qos_reset(gpi);
			if (EOK == ret)
			{
				pfe_gpi_cfg_qos_enable(gpi->gpi_base_va);
			}
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_disable(const pfe_gpi_t *gpi)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		pfe_gpi_cfg_qos_disable(gpi->gpi_base_va);
	}

	return ret;
}

static void igqos_class_set_active(pfe_gpi_t *gpi, uint8_t id)
{
	gpi->igqos_active_entries[id / BITMAP_BITS_U32] |= ((uint32_t)1 << (id % BITMAP_BITS_U32));
}

static void igqos_class_clear_active(pfe_gpi_t *gpi, uint8_t id)
{
	gpi->igqos_active_entries[id / BITMAP_BITS_U32] &= (uint32_t)(~((uint32_t)1 << (id % BITMAP_BITS_U32)));
}

static bool_t igqos_class_is_active(const pfe_gpi_t *gpi, uint8_t id)
{
	bool_t ret = TRUE;

	if ((gpi->igqos_active_entries[id / BITMAP_BITS_U32] & ((uint32_t)1 << (id % BITMAP_BITS_U32))) == 0U)
	{
		ret = FALSE;
	}

	return ret;
}

static uint8_t igqos_class_find_entry(const pfe_gpi_t *gpi, uint8_t start, bool_t is_active)
{
	uint8_t ii;
	uint8_t ret;

	if (unlikely(start > PFE_IQOS_FLOW_TABLE_SIZE))
	{
		ret = PFE_IQOS_FLOW_TABLE_SIZE;
	}
	else
	{
		for (ii = start; ii < PFE_IQOS_FLOW_TABLE_SIZE; ii++)
		{
			if (is_active == igqos_class_is_active(gpi, ii))
			{
				break;
			}
		}

		ret = ii; /* returns PFE_IQOS_FLOW_TABLE_SIZE if not found */
	}

	return ret;
}

static uint8_t igqos_class_find_first_free(const pfe_gpi_t *gpi)
{
	return igqos_class_find_entry(gpi, 0, FALSE);
}

static uint8_t igqos_class_get_first_active(pfe_gpi_t *gpi)
{
	gpi->igqos_entry_iter = igqos_class_find_entry(gpi, 0, TRUE);

	return gpi->igqos_entry_iter;
}

static uint8_t igqos_class_get_next_active(pfe_gpi_t *gpi)
{
	gpi->igqos_entry_iter = igqos_class_find_entry(gpi, gpi->igqos_entry_iter + 1U, TRUE);

	return gpi->igqos_entry_iter;
}

/*
 * convert from the stadard IP address mask encoding to the PFE hardware
 * representation
 */
static uint8_t igqos_ip_mask_hw_encode(uint8_t ip_m)
{
	uint8_t hw_encoded;
	if (0U != ip_m)
	{
		hw_encoded  = ip_m - 1U;
	}
	else
	{
		hw_encoded  = IGQOS_IP_MASK_0;
	}
	return hw_encoded ;
}

static uint8_t igqos_ip_mask_hw_decode(uint8_t ip_m)
{
	uint8_t hw_decoded;
	if (IGQOS_IP_MASK_0 != ip_m)
	{
		hw_decoded = ip_m + 1U;
	}
	else
	{
		hw_decoded = 0U;
	}
	return hw_decoded;
}

static void igqos_convert_entry_to_flow(const uint32_t entry[], pfe_iqos_flow_spec_t *flow)
{
	pfe_iqos_flow_args_t *args = &flow->args;
	uint32_t              val;

	/* entry reg0 */
	val             = entry[0];
	flow->type_mask = (pfe_iqos_flow_type_t)entry_arg_get(TYPE, val);
	args->vlan      = (uint16_t)entry_arg_get(VLAN_ID, val);
	args->tos       = (uint8_t)entry_arg_get(TOS, val);
	args->l4proto   = (uint8_t)entry_arg_get_lower(PROT, val);

	/* entry reg1 */
	val = entry[1];
	args->l4proto |= (uint8_t)entry_arg_get_upper(PROT, val);
	args->sip = entry_arg_get_lower(SIP, val);

	/* entry reg2 */
	val = entry[2];
	args->sip |= entry_arg_get_upper(SIP, val);
	args->dip = entry_arg_get_lower(DIP, val);

	/* entry reg3 */
	val = entry[3];
	args->dip |= entry_arg_get_upper(DIP, val);
	args->sport_min = (uint16_t)entry_arg_get(SPORT_MIN, val);
	args->sport_max = (uint16_t)entry_arg_get_lower(SPORT_MAX, val);

	/* entry reg4 */
	val = entry[4];
	args->sport_max |= (uint16_t)entry_arg_get_upper(SPORT_MAX, val);
	args->dport_min = (uint16_t)entry_arg_get(DPORT_MIN, val);
	args->dport_max = (uint16_t)entry_arg_get_lower(DPORT_MAX, val);

	/* entry reg5 */
	val = entry[5];
	args->dport_max |= (uint16_t)entry_arg_get_upper(DPORT_MAX, val);
	args->vlan_m = (uint16_t)entry_arg_get(VLAN_ID_M, val);
	args->tos_m  = (uint8_t)entry_arg_get_lower(TOS_M, val);

	/* entry reg6 */
	val = entry[6];
	args->tos_m |= (uint8_t)entry_arg_get_upper(TOS_M, val);
	args->l4proto_m = (uint8_t)entry_arg_get(PROT_M, val);
	args->sip_m     = igqos_ip_mask_hw_decode((uint8_t)entry_arg_get(SIP_M, val));
	args->dip_m     = igqos_ip_mask_hw_decode((uint8_t)entry_arg_get(DIP_M, val));

	if (entry_arg_get(ACT_DROP, val) == 1U)
	{
		flow->action = PFE_IQOS_FLOW_DROP;
	}

	if (entry_arg_get(ACT_RES, val) == 1U)
	{
		flow->action = PFE_IQOS_FLOW_RESERVED;
	}

	/* revert h/w fixups from returned flow params */
	args->sport_max <<= 1; /* AAVB-5836 */
	args->dport_max <<= 1; /* AAVB-5836 */
}


static bool igqos_l4_port_range_is_valid(uint16_t min, uint16_t max)
{
	if (min > max)
	{
		return false;
	}

	/* AAVB-5836 */
	if (min > 0x7FFFU)
	{
		return false;
	}

	/* AAVB-5836 */
	if ((max > 0x7FFEU) || (max & 0x1U))
	{
		return false;
	}

	return true;
}

static errno_t igqos_flow_entry_validate_and_fixup(pfe_iqos_flow_spec_t *flow)
{
	pfe_iqos_flow_args_t *args = &flow->args;
	errno_t ret = EOK;

	if (((uint16_t)flow->type_mask >= ((uint16_t)PFE_IQOS_FLOW_TYPE_MAX << 1)) ||
	    ((uint16_t)flow->arg_type_mask >= ((uint16_t)PFE_IQOS_ARG_MAX << 1)) ||
	    (flow->action >= PFE_IQOS_FLOW_COUNT))
	{
		ret = EINVAL;
	}
	else
	{

		if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_VLAN) != 0U)
		{
			if ((args->vlan > PFE_IQOS_VLAN_ID_MASK) || (args->vlan_m > PFE_IQOS_VLAN_ID_MASK))
			{
				ret = EINVAL;
			}
			else
			{
				/* fixup */
				if (args->vlan_m == 0U)
				{
					/* mask not specified */
					args->vlan_m = PFE_IQOS_VLAN_ID_MASK;
				}
			}
		}

		if (EOK == ret)
		{
			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_TOS) != 0U)
			{
				/* fixup */
				if (args->tos_m == 0U)
				{
					/* mask not specified */
					args->tos_m = PFE_IQOS_TOS_MASK;
				}
			}

			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_L4PROTO) != 0U)
			{
				/* fixup */
				if (args->l4proto_m == 0U)
				{
					/* mask not specified */
					args->l4proto_m = PFE_IQOS_L4PROTO_MASK;
				}
			}

			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_SIP) != 0U)
			{
				if (args->sip_m > PFE_IQOS_SDIP_MASK)
				{
					ret = EINVAL;
				}
			}
		}

		if (EOK == ret)
		{
			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_DIP) != 0U)
			{
				if (args->dip_m > PFE_IQOS_SDIP_MASK)
				{
					ret = EINVAL;
				}
			}
		}

		if (EOK == ret)
		{
			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_SPORT) != 0U)
			{
				if (!igqos_l4_port_range_is_valid(args->sport_min, args->sport_max))
				{
					ret = EINVAL;
				}

				/* fixup - AAVB-5836 */
				args->sport_max >>= 1;
			}
		}

		if (EOK == ret)
		{
			if (((uint16_t)flow->arg_type_mask & (uint16_t)PFE_IQOS_ARG_DPORT) != 0U)
			{
				if (!igqos_l4_port_range_is_valid(args->dport_min, args->dport_max))
				{
					ret = EINVAL;
				}

				/* fixup - AAVB-5836 */
				args->dport_max >>= 1;
			}
		}
	}

	return ret;
}

static void igqos_convert_flow_to_entry(const pfe_iqos_flow_spec_t *flow, uint32_t entry[])
{
	const pfe_iqos_flow_args_t *args = &flow->args;
	uint32_t                    val;

	/* entry reg0 */
	val = entry_arg_set(TYPE, (uint32_t)flow->type_mask);
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_VLAN))
	{
		val |= entry_arg_set(VLAN_ID, (uint32_t)args->vlan);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_TOS))
	{
		val |= entry_arg_set(TOS, (uint32_t)args->tos);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_L4PROTO))
	{
		val |= entry_arg_set_lower(PROT, (uint32_t)args->l4proto);
	}
	entry[0] = val;

	/* entry reg1 */
	val = 0;
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_L4PROTO))
	{
		val |= entry_arg_set_upper(PROT, (uint32_t)args->l4proto);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SIP))
	{
		val |= entry_arg_set_lower(SIP, args->sip);
	}
	entry[1] = val;

	/* entry reg2 */
	val = 0;
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SIP))
	{
		val |= entry_arg_set_upper(SIP, args->sip);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DIP))
	{
		val |= entry_arg_set_lower(DIP, args->dip);
	}
	entry[2] = val;

	/* entry reg3 */
	val = 0;
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DIP))
	{
		val |= entry_arg_set_upper(DIP, args->dip);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SPORT))
	{
		val |= entry_arg_set(SPORT_MIN, (uint32_t)args->sport_min);
		val |= entry_arg_set_lower(SPORT_MAX, (uint32_t)args->sport_max);
	}
	entry[3] = val;

	/* entry reg4 */
	val = 0;
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SPORT))
	{
		val |= entry_arg_set_upper(SPORT_MAX, (uint32_t)args->sport_max);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DPORT))
	{
		val |= entry_arg_set(DPORT_MIN, (uint32_t)args->dport_min);
		val |= entry_arg_set_lower(DPORT_MAX, (uint32_t)args->dport_max);
	}
	entry[4] = val;

	/* entry reg5 */
	/* the entry is valid by default */
	val = entry_arg_set(VALID_ENTRY, 1U);
	/* set the same as flow type flags */
	val |= entry_arg_set(TYPE_M, (uint32_t)flow->type_mask);

	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DPORT))
	{
		val |= entry_arg_set_upper(DPORT_MAX, (uint32_t)args->dport_max);
	}

	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_VLAN))
	{
		val |= entry_arg_set(VLAN_ID_M, (uint32_t)args->vlan_m);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_TOS))
	{
		val |= entry_arg_set_lower(TOS_M, (uint32_t)args->tos_m);
	}
	entry[5] = val;

	/* entry reg6 */
	val = 0;
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_TOS))
	{
		val |= entry_arg_set_upper(TOS_M, (uint32_t)args->tos_m);
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_L4PROTO))
	{
		val |= entry_arg_set(PROT_M, (uint32_t)args->l4proto_m);
	}

	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SIP))
	{
		val |= entry_arg_set(SIP_M, (uint32_t)igqos_ip_mask_hw_encode(args->sip_m));
	}
	else
	{
		val |= entry_arg_set(SIP_M, (uint32_t)igqos_ip_mask_hw_encode(0));
	}

	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DIP))
	{
		val |= entry_arg_set(DIP_M, (uint32_t)igqos_ip_mask_hw_encode(args->dip_m));
	}
	else
	{
		val |= entry_arg_set(DIP_M, (uint32_t)igqos_ip_mask_hw_encode(0));
	}

	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_SPORT))
	{
		/* set source port 'mask' to all '1', as not configurable */
		val |= entry_arg_set(SPORT_M, mask32(GPI_QOS_FLOW_SPORT_M_WIDTH));
	}
	if (0U != ((uint32_t)flow->arg_type_mask & (uint32_t)PFE_IQOS_ARG_DPORT))
	{
		/* set destination port 'mask' to all '1', as not configurable */
		val |= entry_arg_set(DPORT_M, mask32(GPI_QOS_FLOW_DPORT_M_WIDTH));
	}
	if (flow->action == PFE_IQOS_FLOW_DROP)
	{
		val |= entry_arg_set(ACT_DROP, 1U);
	}
	else if (flow->action == PFE_IQOS_FLOW_RESERVED)
	{
		val |= entry_arg_set(ACT_RES, 1U);
	}
	else
	{
		/* Required by Misra */
	}
	entry[6] = val;

	/* entry reg7 - unused */
	entry[7] = 0;
}

errno_t pfe_gpi_qos_get_flow(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_flow_spec_t *flow)
{
	uint32_t class_table_entry[8] = { 0U };
	errno_t  ret;

	if (id >= PFE_IQOS_FLOW_TABLE_SIZE)
	{
		ret = EINVAL;
	}
	else
	{
		pfe_gpi_cfg_qos_rd_fl_entry_req(gpi->gpi_base_va, id);
		ret = igqos_entry_ready_timeout(gpi);
		if (ret == EOK)
		{
			pfe_gpi_cfg_qos_rd_fl_entry_resp(gpi->gpi_base_va, class_table_entry);
			igqos_convert_entry_to_flow(class_table_entry, flow);
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_rem_flow(pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret;

	if (id >= PFE_IQOS_FLOW_TABLE_SIZE)
	{
		ret = EINVAL;
	}
	else
	{
		if (igqos_class_is_active(gpi, id))
		{
			pfe_gpi_cfg_qos_clear_flow_entry_req(gpi->gpi_base_va, id);

			ret = igqos_entry_ready_timeout(gpi);
			if (EOK == ret)
			{
				igqos_class_clear_active(gpi, id);
			}
		}
		else
		{
			ret = EINVAL; /* already removed */
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_add_flow(pfe_gpi_t *gpi, uint8_t id, pfe_iqos_flow_spec_t *flow)
{
	uint32_t class_table_entry[8];
	uint8_t  entry_id;
	errno_t  ret;

	if ((id >= PFE_IQOS_FLOW_TABLE_SIZE) && (id != PFE_IQOS_FLOW_TABLE_ENTRY_SKIP))
	{
		ret = EINVAL;
	}
	else if (EOK != igqos_flow_entry_validate_and_fixup(flow))
	{
		ret = EINVAL;
	}
	else
	{
		if (id == PFE_IQOS_FLOW_TABLE_ENTRY_SKIP)
		{
			entry_id = igqos_class_find_first_free(gpi);
		}
		else
		{
			entry_id = id;
		}

		igqos_convert_flow_to_entry(flow, class_table_entry);

		pfe_gpi_cfg_qos_write_flow_entry_req(gpi->gpi_base_va, entry_id, class_table_entry);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK == ret)
		{
			igqos_class_set_active(gpi, entry_id);
		}
	}

	return ret;
}

errno_t pfe_gpi_qos_get_first_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow)
{
	errno_t ret;
	uint8_t entry_id;

	entry_id = igqos_class_get_first_active(gpi);
	if (entry_id == PFE_IQOS_FLOW_TABLE_SIZE)
	{
		ret = EOVERFLOW;
	}
	else
	{
		*id = entry_id;
		ret = pfe_gpi_qos_get_flow(gpi, entry_id, flow);
	}

	return ret;
}

errno_t pfe_gpi_qos_get_next_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow)
{
	errno_t ret;
	uint8_t entry_id;

	entry_id = igqos_class_get_next_active(gpi);
	if (entry_id == PFE_IQOS_FLOW_TABLE_SIZE)
	{
		ret = EOVERFLOW;
	}
	else
	{
		*id = entry_id;
		ret = pfe_gpi_qos_get_flow(gpi, entry_id, flow);
	}

	return ret;
}

/* WRED configuration */

bool_t pfe_gpi_wred_is_enabled(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	bool_t  is_enabled;
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		is_enabled = FALSE;
	}
	else if (queue >= PFE_IQOS_Q_COUNT)
	{
		is_enabled = FALSE;
	}
	else
	{
		is_enabled = pfe_gpi_cfg_wred_is_enabled(gpi->gpi_base_va, queue);
	}

	return is_enabled;
}

errno_t pfe_gpi_wred_enable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		if (queue >= PFE_IQOS_Q_COUNT)
		{
			ret = EINVAL;
		}
		else if (TRUE == pfe_gpi_cfg_wred_is_enabled(gpi->gpi_base_va, queue))
		{
			ret = EOK;
		}
		else
		{
			pfe_gpi_cfg_wred_enable(gpi->gpi_base_va, queue);
		}
	}

	return ret;
}

errno_t pfe_gpi_wred_disable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		if (queue >= PFE_IQOS_Q_COUNT)
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_wred_disable(gpi->gpi_base_va, queue);
		}
	}

	return ret;
}

errno_t pfe_gpi_wred_set_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t val)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		if ((queue >= PFE_IQOS_Q_COUNT) || (zone >= PFE_IQOS_WRED_ZONES_COUNT) || (val > PFE_IQOS_WRED_ZONE_PROB_MAX))
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_wred_set_prob(gpi->gpi_base_va, queue, zone, val);
		}
	}

	return ret;
}

errno_t pfe_gpi_wred_get_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t *val)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		if ((queue >= PFE_IQOS_Q_COUNT) || (zone >= PFE_IQOS_WRED_ZONES_COUNT))
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_wred_get_prob(gpi->gpi_base_va, queue, zone, val);
		}
	}

	return ret;
}

errno_t pfe_gpi_wred_set_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t val)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret == EOK)
	{
		if ((queue >= PFE_IQOS_Q_COUNT) || (thr >= PFE_IQOS_WRED_THR_COUNT))
		{
			ret = EINVAL;
		}
		else if ((queue == PFE_IQOS_Q_DMEM) && (val > PFE_IQOS_WRED_DMEM_THR_MAX))
		{
			ret = EINVAL;
		}
		else if ((queue != PFE_IQOS_Q_DMEM) && (val > PFE_IQOS_WRED_THR_MAX))
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_wred_set_thr(gpi->gpi_base_va, queue, thr, val);
		}
	}

	return ret;
}

errno_t pfe_gpi_wred_get_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t *val)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret == EOK)
	{
		if ((queue >= PFE_IQOS_Q_COUNT) || (thr >= PFE_IQOS_WRED_THR_COUNT))
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_wred_get_thr(gpi->gpi_base_va, queue, thr, val);
		}
	}

	return ret;
}

/* shaper configuration */

static errno_t pfe_gpi_shp_args_checks(const pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret == EOK)
	{
		if (id >= PFE_IQOS_SHP_COUNT)
		{
			ret = EINVAL;
		}
	}

	return ret;
}

bool_t pfe_gpi_shp_is_enabled(const pfe_gpi_t *gpi, uint8_t id)
{
	bool_t  is_enabled = FALSE;
	errno_t ret        = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		is_enabled = pfe_gpi_cfg_shp_is_enabled(gpi->gpi_base_va, id);
	}

	return is_enabled;
}

errno_t pfe_gpi_shp_enable(pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		if (TRUE != pfe_gpi_cfg_shp_is_enabled(gpi->gpi_base_va, id))
		{
			gpi->sys_clk_mhz  = pfe_gpi_cfg_get_sys_clk_mhz(gpi->cbus_base_va);
			gpi->clk_div_log2 = 0;
			pfe_gpi_cfg_shp_default_init(gpi->gpi_base_va, id);
			pfe_gpi_cfg_shp_enable(gpi->gpi_base_va, id);
		}
	}

	return ret;
}

errno_t pfe_gpi_shp_disable(const pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		pfe_gpi_cfg_shp_disable(gpi->gpi_base_va, id);
	}

	return ret;
}

errno_t pfe_gpi_shp_set_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t mode)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		if (mode >= PFE_IQOS_SHP_RATE_MODE_COUNT)
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_shp_set_mode(gpi->gpi_base_va, id, mode);
		}
	}

	return ret;
}

errno_t pfe_gpi_shp_get_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t *mode)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		pfe_gpi_cfg_shp_get_mode(gpi->gpi_base_va, id, mode);
	}

	return ret;
}

errno_t pfe_gpi_shp_set_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t type)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		if (type >= PFE_IQOS_SHP_TYPE_COUNT)
		{
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_shp_set_type(gpi->gpi_base_va, id, type);
		}
	}

	return ret;
}

errno_t pfe_gpi_shp_get_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t *type)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret == EOK)
	{
		pfe_gpi_cfg_shp_get_type(gpi->gpi_base_va, id, type);
	}

	return ret;
}

static uint32_t igqos_clk_div(uint32_t clk_div_log2)
{
	return ((uint32_t)1U << (clk_div_log2 + 1U));
}

static uint32_t igqos_convert_isl_to_weight(uint32_t isl, uint32_t clk_div_log2, uint32_t sys_clk_mhz, bool_t is_bps)
{
	uint64_t clk_div = igqos_clk_div(clk_div_log2);
	uint64_t wgt;

	wgt = (uint64_t)isl;
	wgt *= clk_div;
	wgt *= (1ULL << IGQOS_PORT_SHP_FRACW_WIDTH);
	wgt /= (uint64_t)sys_clk_mhz * 1000000ULL; /* sys clk in Hz */
	if (TRUE == is_bps)
	{
		wgt /= 8ULL;
	}

	return (uint32_t)wgt;
}

static uint32_t igqos_convert_weight_to_isl(uint32_t wgt, uint32_t clk_div_log2, uint32_t sys_clk_mhz, bool_t is_bps)
{
	uint64_t clk_div = igqos_clk_div(clk_div_log2);
	uint64_t isl;

	isl = (uint64_t)wgt;
	if (is_bps)
	{
		isl *= 8ULL;
	}
	isl *= (uint64_t)sys_clk_mhz * 1000000ULL; /* sys clk in Hz */
	isl /= (1ULL << IGQOS_PORT_SHP_FRACW_WIDTH);
	isl /= clk_div;

	return (uint32_t)isl;
}

static uint32_t igqos_find_optimal_weight(uint32_t isl, uint32_t sys_clk_mhz, bool_t is_bps, uint32_t *wgt)
{
	const uint32_t w_max = IGQOS_PORT_SHP_WEIGHT_MASK;
	uint32_t       w, l, r, k;
	uint32_t       ret;

	r = IGQOS_PORT_SHP_CLKDIV_MASK; /* max clk_div_log2 value */
	l = 0;                          /* min clk_div_log2 value */

	/* check if 'isl' is out-of-range */
	w = igqos_convert_isl_to_weight(isl, l, sys_clk_mhz, is_bps);
	if (w > w_max)
	{
		NXP_LOG_WARNING("Shaper idle slope too high, weight (%u) exceeds max value\n", (uint_t)w);
		*wgt = w;
		ret  = l;
	}
	else
	{

		w = igqos_convert_isl_to_weight(isl, r, sys_clk_mhz, is_bps);
		if (w == 0U)
		{
			NXP_LOG_WARNING("Shaper idle slope too small, computed weight is 0\n");
			*wgt = w;
			ret  = r;
		}
		else
		{

			if (w <= w_max)
			{
				*wgt = w;
				ret  = r; /* optimum found */
			}
			else
			{

				/* binary search, worst case 4 iterations for r == 15 */
				while ((l + 1U) < r)
				{
					k = (l + r) / 2U;
					w = igqos_convert_isl_to_weight(isl, k, sys_clk_mhz, is_bps);

					if (w <= w_max)
					{
						l = k;
					}
					else
					{
						r = k;
					}
				}

				k = (l + r) / 2U;

				*wgt = igqos_convert_isl_to_weight(isl, k, sys_clk_mhz, is_bps);
				ret = k;
			}
		}
	}
	return ret;
}

errno_t pfe_gpi_shp_set_idle_slope(pfe_gpi_t *gpi, uint8_t id, uint32_t isl)
{
	pfe_iqos_shp_rate_mode_t mode;
	uint32_t                 weight;
	bool_t                   is_bps;
	errno_t                  ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret == EOK)
	{
		NXP_LOG_DEBUG("Shaper#%d - Set idle slope of: %u\n", id, (uint_t)isl);

		pfe_gpi_cfg_shp_get_mode(gpi->gpi_base_va, id, &mode);
		if (mode == PFE_IQOS_SHP_BPS)
		{
			is_bps = TRUE;
		}
		else
		{
			is_bps = FALSE;
		}

		gpi->clk_div_log2 = igqos_find_optimal_weight(isl, gpi->sys_clk_mhz, is_bps, &weight);

		NXP_LOG_DEBUG("Shaper#%d using PFE sys_clk value %u MHz, clkdiv: %u\n", id, (uint_t)(gpi->sys_clk_mhz), (uint_t)igqos_clk_div(gpi->clk_div_log2));
		NXP_LOG_DEBUG("Shaper#%d - Write weight of: %u\n", id, (uint_t)weight);

		pfe_gpi_cfg_shp_set_isl_weight(gpi->gpi_base_va, id, gpi->clk_div_log2, weight);
	}

	return ret;
}

errno_t pfe_gpi_shp_get_idle_slope(const pfe_gpi_t *gpi, uint8_t id, uint32_t *isl)
{
	pfe_iqos_shp_rate_mode_t mode;
	uint32_t                 weight;
	bool_t                   is_bps;
	errno_t                  ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret == EOK)
	{
		pfe_gpi_cfg_shp_get_mode(gpi->gpi_base_va, id, &mode);
		if (mode == PFE_IQOS_SHP_BPS)
		{
			is_bps = TRUE;
		}
		else
		{
			is_bps = FALSE;
		}

		NXP_LOG_DEBUG("Shaper#%d using PFE sys_clk value %u MHz, clkdiv: %u\n", id, (uint_t)(gpi->sys_clk_mhz), (uint_t)igqos_clk_div(gpi->clk_div_log2));

		pfe_gpi_cfg_shp_get_isl_weight(gpi->gpi_base_va, id, &weight);

		*isl = igqos_convert_weight_to_isl(weight, gpi->clk_div_log2, gpi->sys_clk_mhz, is_bps);

		NXP_LOG_DEBUG("Shaper#%d - Get idle slope of: %u\n", id, (uint_t)(*isl));
	}

	return ret;
}

errno_t pfe_gpi_shp_set_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t max_credit, int32_t min_credit)
{
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret == EOK)
	{
		if ((max_credit > IGQOS_PORT_SHP_CREDIT_MAX) || (max_credit < 0))
		{
			NXP_LOG_WARNING("Max credit value exceeded\n");
			ret = EINVAL;
		}
		else if ((min_credit < -IGQOS_PORT_SHP_CREDIT_MAX) || (min_credit > 0))
		{
			NXP_LOG_WARNING("Min credit value exceeded\n");
			ret = EINVAL;
		}
		else
		{
			pfe_gpi_cfg_shp_set_limits(gpi->gpi_base_va, id, (uint32_t)max_credit, (uint32_t)-min_credit);
		}
	}

	return ret;
}

errno_t pfe_gpi_shp_get_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t *max_credit, int32_t *min_credit)
{
	uint32_t abs_max_cred, abs_min_cred;
	errno_t  ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret == EOK)
	{
		pfe_gpi_cfg_shp_get_limits(gpi->gpi_base_va, id, &abs_max_cred, &abs_min_cred);
		*max_credit = (int32_t)abs_max_cred;
		*min_credit = -(int32_t)abs_min_cred;
	}

	return ret;
}

/* note - the counter is reset to 0 after read (clear on read) */
errno_t pfe_gpi_shp_get_drop_cnt(const pfe_gpi_t *gpi, uint8_t id, uint32_t *cnt)
{
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret == EOK)
	{
		*cnt = pfe_gpi_cfg_shp_get_drop_cnt(gpi->gpi_base_va, id);
	}

	return ret;
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

/**
 * @brief		Return GPI runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	gpi 		The GPI instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_gpi_get_text_statistics(const pfe_gpi_t *gpi, struct seq_file *seq, uint8_t verb_level)
{
	errno_t  ret = pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret == EOK)
	{
		pfe_gpi_cfg_get_text_stat(gpi->gpi_base_va, seq, verb_level);
	}

	return 0;
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

