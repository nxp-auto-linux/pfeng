/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
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

#define IGQOS_BITMAP_ARR_SZ	2
#define BITMAP_BITS_U32		32
#define DECLARE_BITMAP_U32(name, SIZE) \
	uint32_t name[SIZE]

struct pfe_gpi_tag
{
	addr_t cbus_base_va;		/* CBUS base virtual address */
	addr_t gpi_base_offset;		/* GPI base offset within CBUS space */
	addr_t gpi_base_va;		/* GPI base address (virtual) */

	/* bitmap of all (PFE_IQOS_FLOW_TABLE_SIZE) active classification table entries */
	DECLARE_BITMAP_U32(igqos_active_entries, IGQOS_BITMAP_ARR_SZ);
	uint8_t igqos_entry_iter; /* classification table active entries interator */
	uint32_t sys_clk_mhz;
	uint32_t clk_div_log2;
};
ct_assert(PFE_IQOS_FLOW_TABLE_SIZE <= (BITMAP_BITS_U32 * IGQOS_BITMAP_ARR_SZ));

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
	addr_t gpi_cbus_offset;
	pfe_gpi_t *gpi;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == cbus_base_va) || (NULL == cfg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	gpi = oal_mm_malloc(sizeof(pfe_gpi_t));

	if (NULL == gpi)
	{
		return NULL;
	}
	else
	{
		(void)memset(gpi, 0, sizeof(pfe_gpi_t));
		gpi->cbus_base_va = cbus_base_va;
		gpi->gpi_base_offset = gpi_base;
		gpi->gpi_base_va = (gpi->cbus_base_va + gpi->gpi_base_offset);
		gpi->sys_clk_mhz = pfe_gpi_cfg_get_sys_clk_mhz(cbus_base_va);
		gpi_cbus_offset = gpi->gpi_base_va - cbus_base_va;
	}

	ret = pfe_gpi_reset(gpi);
	if (EOK != ret)
	{
		return NULL;
	}

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
				return NULL;
			}
			break;
		default:
			/* Do Nothing */
			break;
	}

	pfe_gpi_disable(gpi);

	pfe_gpi_cfg_init(gpi->gpi_base_va, cfg);

	return gpi;
}

#if defined(PFE_CFG_NULL_ARG_CHECK)
static errno_t _pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err)
{
	if (unlikely(NULL == gpi))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return err;
	}

	return EOK;
}
#else
static errno_t _pfe_gpi_null_arg_check_return(const pfe_gpi_t *gpi, errno_t err)
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
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	ret = pfe_gpi_cfg_reset(gpi->gpi_base_va);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("GPI reset timed-out\n");
	}

	return ret;
}

/**
 * @brief		Enable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_enable(const pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return;
	}

	pfe_gpi_cfg_enable(gpi->gpi_base_va);
}

/**
 * @brief		Disable the GPI block
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_disable(const pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return;
	}

	pfe_gpi_cfg_disable(gpi->gpi_base_va);
}

/**
 * @brief		Destroy GPI instance
 * @param[in]	gpi The GPI instance
 */
void pfe_gpi_destroy(pfe_gpi_t *gpi)
{
	if (NULL != gpi)
	{
		pfe_gpi_disable(gpi);

		pfe_gpi_qos_reset(gpi);

		pfe_gpi_reset(gpi);

		oal_mm_free(gpi);
	}
}

/* Ingress QoS support */

bool_t pfe_gpi_qos_is_enabled(const pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return FALSE;
	}

	return pfe_gpi_cfg_qos_is_enabled(gpi->gpi_base_va);
}

static void igqos_class_clear_active_all(pfe_gpi_t *gpi)
{
	int i;

	for (i = 0; i < IGQOS_BITMAP_ARR_SZ; i++)
	{
		gpi->igqos_active_entries[i] = 0;
	}

	gpi->igqos_entry_iter = 0;
}

static errno_t igqos_entry_ready_timeout(const pfe_gpi_t *gpi)
{
	uint32_t timeout = 200U;
	bool_t ready;

	for (;;) {
		ready = pfe_gpi_cfg_qos_entry_ready(gpi->gpi_base_va);
		if (TRUE == ready)
		{
			break;
		}
		oal_time_usleep(5U);
		timeout--;
		if (0 == timeout)
		{
			break;
		}
	}

	if (0 == timeout && FALSE == ready)
	{
		ready = pfe_gpi_cfg_qos_entry_ready(gpi->gpi_base_va);
	}

	if (FALSE == ready)
	{
		return ETIMEDOUT;
	}

	return EOK;
}

static errno_t igqos_class_clear_flow_entry_table(pfe_gpi_t *gpi)
{
	uint32_t ii;
	errno_t ret;

	for (ii = 0U; ii < ENTRY_TABLE_SIZE; ii++)
	{
		pfe_gpi_cfg_qos_clear_flow_entry_req(gpi->gpi_base_va, ii);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK != ret)
		{
			return ret;
		}
	}

	return EOK;
}

static errno_t igqos_class_clear_lru_entry_table(pfe_gpi_t *gpi)
{
	uint32_t ii;
	errno_t ret;

	for (ii = 0U; ii < ENTRY_TABLE_SIZE; ii++)
	{
		pfe_gpi_cfg_qos_clear_lru_entry_req(gpi->gpi_base_va, ii);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK != ret)
		{
			return ret;
		}
	}

	return EOK;
}

errno_t pfe_gpi_qos_reset(pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	ret = igqos_class_clear_flow_entry_table(gpi);
	if (EOK != ret)
	{
		return ret;
	}

	ret = igqos_class_clear_lru_entry_table(gpi);
	if (EOK != ret)
	{
		return ret;
	}

	pfe_gpi_cfg_qos_default_init(gpi->gpi_base_va);

	/* clear driver state */
	igqos_class_clear_active_all(gpi);

	return EOK;
}

errno_t pfe_gpi_qos_enable(pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	if (TRUE == pfe_gpi_cfg_qos_is_enabled(gpi->gpi_base_va))
	{
		return EOK;
	}

	ret = pfe_gpi_qos_reset(gpi);
	if (EOK != ret)
	{
		return ret;
	}

	pfe_gpi_cfg_qos_enable(gpi->gpi_base_va);

	return EOK;
}

errno_t pfe_gpi_qos_disable(const pfe_gpi_t *gpi)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_qos_disable(gpi->gpi_base_va);

	return EOK;
}

static void igqos_class_set_active(pfe_gpi_t *gpi, uint8_t id)
{
	gpi->igqos_active_entries[id / BITMAP_BITS_U32] |= ((uint32_t)1 << (id % BITMAP_BITS_U32));
}

static void igqos_class_clear_active(pfe_gpi_t *gpi, uint8_t id)
{
	gpi->igqos_active_entries[id / BITMAP_BITS_U32] &= ~((uint32_t)1 << (id % BITMAP_BITS_U32));
}

static bool_t igqos_class_is_active(const pfe_gpi_t *gpi, uint8_t id)
{
	if ((gpi->igqos_active_entries[id / BITMAP_BITS_U32] & ((uint32_t)1 << (id % BITMAP_BITS_U32))) == 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static uint8_t igqos_class_find_entry(const pfe_gpi_t *gpi, uint8_t start, bool_t is_active)
{
	uint8_t ii;

	if (unlikely(start > PFE_IQOS_FLOW_TABLE_SIZE))
	{
		return PFE_IQOS_FLOW_TABLE_SIZE;
	}

	for (ii = start; ii < PFE_IQOS_FLOW_TABLE_SIZE; ii++)
	{
		if (is_active == igqos_class_is_active(gpi, ii))
		{
			break;
		}
	}
	/* returns PFE_IQOS_FLOW_TABLE_SIZE if not found */
	return ii;
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
	gpi->igqos_entry_iter = igqos_class_find_entry(gpi, gpi->igqos_entry_iter + 1, TRUE);

	return gpi->igqos_entry_iter;
}

static void igqos_convert_entry_to_flow(const uint32_t entry[], pfe_iqos_flow_spec_t *flow)
{
	pfe_iqos_flow_args_t *args = &flow->args;
	uint32_t val;

	/* entry reg0 */
	val = entry[0];
	flow->type_mask = (pfe_iqos_flow_type_t)entry_arg_get(TYPE, val);
	args->vlan = entry_arg_get(VLAN_ID, val);
	args->tos = entry_arg_get(TOS, val);
	args->l4proto = entry_arg_get_lower(PROT, val);

	/* entry reg1 */
	val = entry[1];
	args->l4proto |= entry_arg_get_upper(PROT, val);
	args->sip = entry_arg_get_lower(SIP, val);

	/* entry reg2 */
	val = entry[2];
	args->sip |= entry_arg_get_upper(SIP, val);
	args->dip = entry_arg_get_lower(DIP, val);

	/* entry reg3 */
	val = entry[3];
	args->dip |= entry_arg_get_upper(DIP, val);
	args->sport_max = entry_arg_get(SPORT_MAX, val);
	args->sport_min = entry_arg_get_lower(SPORT_MIN, val);

	/* entry reg4 */
	val = entry[4];
	args->sport_min |= entry_arg_get_upper(SPORT_MIN, val);
	args->dport_max = entry_arg_get(DPORT_MAX, val);
	args->dport_min = entry_arg_get_lower(DPORT_MIN, val);

	/* entry reg5 */
	val = entry[5];
	args->dport_min |= entry_arg_get_upper(DPORT_MIN, val);
	args->vlan_m = entry_arg_get(VLAN_ID_M, val);
	args->tos_m = entry_arg_get_lower(TOS_M, val);

	/* entry reg6 */
	val = entry[6];
	args->tos_m |= entry_arg_get_upper(TOS_M, val);
	args->l4proto_m = entry_arg_get(PROT_M, val);
	args->sip_m = entry_arg_get(SIP_M, val);
	args->dip_m = entry_arg_get(DIP_M, val);

	if (entry_arg_get(ACT_DROP, val) == 1)
	{
		flow->action = PFE_IQOS_FLOW_DROP;
	}

	if (entry_arg_get(ACT_RES, val) == 1)
	{
		flow->action = PFE_IQOS_FLOW_RESERVED;
	}
}

static void igqos_convert_flow_to_entry(const pfe_iqos_flow_spec_t *flow, uint32_t entry[])
{
	const pfe_iqos_flow_args_t *args = &flow->args;
	uint32_t val;

	/* entry reg0 */
	val = entry_arg_set(TYPE, flow->type_mask);
	if (flow->arg_type_mask & PFE_IQOS_ARG_VLAN)
	{
		val |= entry_arg_set(VLAN_ID, args->vlan);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_TOS)
	{
		val |= entry_arg_set(TOS, args->tos);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_L4PROTO)
	{
		val |= entry_arg_set_lower(PROT, args->l4proto);
	}
	entry[0] = val;

	/* entry reg1 */
	val = 0;
	if (flow->arg_type_mask & PFE_IQOS_ARG_L4PROTO)
	{
		val |= entry_arg_set_upper(PROT, args->l4proto);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_SIP)
	{
		val |= entry_arg_set_lower(SIP, args->sip);
	}
	entry[1] = val;

	/* entry reg2 */
	val = 0;
	if (flow->arg_type_mask & PFE_IQOS_ARG_SIP)
	{
		val |= entry_arg_set_upper(SIP, args->sip);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_DIP)
	{
		val |= entry_arg_set_lower(DIP, args->dip);
	}
	entry[2] = val;

	/* entry reg3 */
	val = 0;
	if (flow->arg_type_mask & PFE_IQOS_ARG_DIP)
	{
		val |= entry_arg_set_upper(DIP, args->dip);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_SPORT)
	{
		val |= entry_arg_set(SPORT_MAX, args->sport_max);
		val |= entry_arg_set_lower(SPORT_MIN, args->sport_min);
	}
	entry[3] = val;

	/* entry reg4 */
	val = 0;
	if (flow->arg_type_mask & PFE_IQOS_ARG_SPORT)
	{
		val |= entry_arg_set_upper(SPORT_MIN, args->sport_min);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_DPORT)
	{
		val |= entry_arg_set(DPORT_MAX, args->dport_max);
		val |= entry_arg_set_lower(DPORT_MIN, args->dport_min);
	}
	entry[4] = val;

	/* entry reg5 */
	/* the entry is valid by default */
	val = entry_arg_set(VALID_ENTRY, 1);
	/* set the same as flow type flags */
	val |= entry_arg_set(TYPE_M, flow->type_mask);

	if (flow->arg_type_mask & PFE_IQOS_ARG_DPORT)
	{
		val |= entry_arg_set_upper(DPORT_MIN, args->dport_min);
	}

	if (flow->arg_type_mask & PFE_IQOS_ARG_VLAN)
	{
		val |= entry_arg_set(VLAN_ID_M, args->vlan_m);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_TOS)
	{
		val |= entry_arg_set_lower(TOS_M, args->tos_m);
	}
	entry[5] = val;

	/* entry reg6 */
	val = 0;
	if (flow->arg_type_mask & PFE_IQOS_ARG_TOS)
	{
		val |= entry_arg_set_upper(TOS_M, args->tos_m);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_L4PROTO)
	{
		val |= entry_arg_set(PROT_M, args->l4proto_m);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_SIP)
	{
		val |= entry_arg_set(SIP_M, args->sip_m);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_DIP)
	{
		val |= entry_arg_set(DIP_M, args->dip_m);
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_SPORT)
	{
		/* set source port 'mask' to all '1', as not configurable */
		val |= entry_arg_set(SPORT_M, mask32(GPI_QOS_FLOW_SPORT_M_WIDTH));
	}
	if (flow->arg_type_mask & PFE_IQOS_ARG_DPORT)
	{
		/* set destination port 'mask' to all '1', as not configurable */
		val |= entry_arg_set(DPORT_M, mask32(GPI_QOS_FLOW_DPORT_M_WIDTH));
	}
	if (flow->action == PFE_IQOS_FLOW_DROP)
	{
		val |= entry_arg_set(ACT_DROP, 1);
	}
	else if (flow->action == PFE_IQOS_FLOW_RESERVED)
	{
		val |= entry_arg_set(ACT_RES, 1);
	}
	entry[6] = val;

	/* entry reg7 - unused */
	entry[7] = 0;
}

errno_t pfe_gpi_qos_get_flow(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_flow_spec_t *flow)
{
	uint32_t class_table_entry[8];
	errno_t ret;

	if (id >= PFE_IQOS_FLOW_TABLE_SIZE)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_qos_read_flow_entry_req(gpi->gpi_base_va, id);
	ret = igqos_entry_ready_timeout(gpi);
	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_qos_read_flow_entry_resp(gpi->gpi_base_va, class_table_entry);
	igqos_convert_entry_to_flow(class_table_entry, flow);

	return EOK;
}

errno_t pfe_gpi_qos_rem_flow(pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret;

	if (id >= PFE_IQOS_FLOW_TABLE_SIZE)
	{
		return EINVAL;
	}

	if (igqos_class_is_active(gpi, id))
	{
		pfe_gpi_cfg_qos_clear_flow_entry_req(gpi->gpi_base_va, id);

		ret = igqos_entry_ready_timeout(gpi);
		if (EOK != ret)
		{
			return ret;
		}

		igqos_class_clear_active(gpi, id);
	}
	else
	{
		return EINVAL; /* already removed */
	}

	return EOK;
}

errno_t pfe_gpi_qos_add_flow(pfe_gpi_t *gpi, uint8_t id, const pfe_iqos_flow_spec_t *flow)
{
	uint32_t class_table_entry[8];
	uint8_t entry_id;
	errno_t ret;

	if (id >= PFE_IQOS_FLOW_TABLE_SIZE && id != PFE_IQOS_FLOW_TABLE_ENTRY_SKIP)
	{
		return EINVAL;
	}

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
	if (EOK != ret)
	{
		return ret;
	}

	igqos_class_set_active(gpi, entry_id);

	return ret;
}

errno_t pfe_gpi_qos_get_first_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow)
{
	uint8_t entry_id;

	entry_id = igqos_class_get_first_active(gpi);
	if (entry_id == PFE_IQOS_FLOW_TABLE_SIZE)
	{
		return EOVERFLOW;
	}

	*id = entry_id;
	return pfe_gpi_qos_get_flow(gpi, entry_id, flow);
}

errno_t pfe_gpi_qos_get_next_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow)
{
	uint8_t entry_id;

	entry_id = igqos_class_get_next_active(gpi);
	if (entry_id == PFE_IQOS_FLOW_TABLE_SIZE)
	{
		return EOVERFLOW;
	}

	*id = entry_id;
	return pfe_gpi_qos_get_flow(gpi, entry_id, flow);
}

/* WRED configuration */

bool_t pfe_gpi_wred_is_enabled(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return FALSE;
	}

	if (queue >= PFE_IQOS_Q_COUNT)
	{
		return FALSE;
	}

	return pfe_gpi_cfg_wred_is_enabled(gpi->gpi_base_va, queue);
}

errno_t pfe_gpi_wred_enable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT)
	{
		return EINVAL;
	}

	if (TRUE == pfe_gpi_cfg_wred_is_enabled(gpi->gpi_base_va, queue))
	{
		return EOK;
	}

	pfe_gpi_cfg_wred_enable(gpi->gpi_base_va, queue);

	return EOK;
}

errno_t pfe_gpi_wred_disable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_wred_disable(gpi->gpi_base_va, queue);

	return EOK;
}

errno_t pfe_gpi_wred_set_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t val)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT || zone >= PFE_IQOS_WRED_ZONES_COUNT || val > PFE_IQOS_WRED_ZONE_PROB_MAX)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_wred_set_prob(gpi->gpi_base_va, queue, zone, val);

	return EOK;
}

errno_t pfe_gpi_wred_get_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t *val)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT || zone >= PFE_IQOS_WRED_ZONES_COUNT)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_wred_get_prob(gpi->gpi_base_va, queue, zone, val);

	return EOK;
}

errno_t pfe_gpi_wred_set_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t val)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT || thr >= PFE_IQOS_WRED_THR_COUNT)
	{
		return EINVAL;
	}

	if (queue == PFE_IQOS_Q_DMEM)
	{
		if (val > PFE_IQOS_WRED_DMEM_THR_MAX)
		{
			return EINVAL;
		}
	}
	else
	{
		if (val > PFE_IQOS_WRED_THR_MAX)
		{
			return EINVAL;
		}
	}

	pfe_gpi_cfg_wred_set_thr(gpi->gpi_base_va, queue, thr, val);

	return EOK;
}

errno_t pfe_gpi_wred_get_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t *val)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret != EOK)
	{
		return ret;
	}

	if (queue >= PFE_IQOS_Q_COUNT || thr >= PFE_IQOS_WRED_THR_COUNT)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_wred_get_thr(gpi->gpi_base_va, queue, thr, val);

	return EOK;
}

/* shaper configuration */

static errno_t pfe_gpi_shp_args_checks(const pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);

	if (ret != EOK)
	{
		return ret;
	}

	if (id >= PFE_IQOS_SHP_COUNT)
	{
		return EINVAL;
	}

	return EOK;
}

bool_t pfe_gpi_shp_is_enabled(const pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	return pfe_gpi_cfg_shp_is_enabled(gpi->gpi_base_va, id);
}

errno_t pfe_gpi_shp_enable(pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	if (TRUE == pfe_gpi_cfg_shp_is_enabled(gpi->gpi_base_va, id))
	{
		return EOK;
	}

	gpi->sys_clk_mhz = pfe_gpi_cfg_get_sys_clk_mhz(gpi->cbus_base_va);
	gpi->clk_div_log2 = 0;
	pfe_gpi_cfg_shp_default_init(gpi->gpi_base_va, id);
	pfe_gpi_cfg_shp_enable(gpi->gpi_base_va, id);

	return EOK;
}

errno_t pfe_gpi_shp_disable(const pfe_gpi_t *gpi, uint8_t id)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_shp_disable(gpi->gpi_base_va, id);

	return EOK;
}

errno_t pfe_gpi_shp_set_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t mode)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	if (mode >= PFE_IQOS_SHP_RATE_MODE_COUNT)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_shp_set_mode(gpi->gpi_base_va, id, mode);

	return EOK;
}

errno_t pfe_gpi_shp_get_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t *mode)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_shp_get_mode(gpi->gpi_base_va, id, mode);

	return EOK;
}

errno_t pfe_gpi_shp_set_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t type)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	if (type >= PFE_IQOS_SHP_TYPE_COUNT)
	{
		return EINVAL;
	}

	pfe_gpi_cfg_shp_set_type(gpi->gpi_base_va, id, type);

	return EOK;
}

errno_t pfe_gpi_shp_get_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t *type)
{
	errno_t ret = pfe_gpi_shp_args_checks(gpi, id);

	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_shp_get_type(gpi->gpi_base_va, id, type);

	return EOK;
}

static uint32_t igqos_clk_div(uint32_t clk_div_log2)
{
	return 1U << (clk_div_log2 + 1U);
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

	return wgt;
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

	return isl;
}

static uint32_t igqos_find_optimal_weight(uint32_t isl, uint32_t sys_clk_mhz, bool_t is_bps, uint32_t *wgt)
{
	const uint32_t w_max = IGQOS_PORT_SHP_WEIGHT_MASK;
	uint32_t w, l, r, i;

	r = IGQOS_PORT_SHP_CLKDIV_MASK; /* max clk_div_log2 value */
	l = 0; /* min clk_div_log2 value */

	/* check if 'isl' is out-of-range */
	w = igqos_convert_isl_to_weight(isl, l, sys_clk_mhz, is_bps);
	if (w > w_max)
	{
		NXP_LOG_WARNING("Shaper idle slope too high, weight (%u) exceeds max value\n", w);
		*wgt = w;
		return l;
	}

	w = igqos_convert_isl_to_weight(isl, r, sys_clk_mhz, is_bps);
	if (w == 0)
	{
		NXP_LOG_WARNING("Shaper idle slope too small, computed weight is 0\n");
		*wgt = w;
		return r;
	}

	if (w <= w_max)
	{
		*wgt = w;
		return r; /* optimum found */
	}

	/* binary search, worst case 4 iterations for r == 15 */
	while (l + 1 < r)
	{
		i = (l + r) / 2;
		w = igqos_convert_isl_to_weight(isl, i, sys_clk_mhz, is_bps);

		if (w <= w_max)
		{
			l = i;
		}
		else
		{
			r = i;
		}
	}

	i = (l + r) / 2;

	*wgt = igqos_convert_isl_to_weight(isl, i, sys_clk_mhz, is_bps);
	return i;
}

errno_t pfe_gpi_shp_set_idle_slope(pfe_gpi_t *gpi, uint8_t id, uint32_t isl)
{
	pfe_iqos_shp_rate_mode_t mode;
	uint32_t weight;
	bool_t is_bps;
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret != EOK)
	{
		return ret;
	}

	NXP_LOG_DEBUG("Shaper#%d - Set idle slope of: %u\n", id, isl);

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

	NXP_LOG_DEBUG("Shaper#%d using PFE sys_clk value %u MHz, clkdiv: %u\n", id, gpi->sys_clk_mhz,
		      igqos_clk_div(gpi->clk_div_log2));
	NXP_LOG_DEBUG("Shaper#%d - Write weight of: %u\n", id, weight);

	pfe_gpi_cfg_shp_set_isl_weight(gpi->gpi_base_va, id, gpi->clk_div_log2, weight);

	return EOK;
}

errno_t pfe_gpi_shp_get_idle_slope(const pfe_gpi_t *gpi, uint8_t id, uint32_t *isl)
{
	pfe_iqos_shp_rate_mode_t mode;
	uint32_t weight;
	bool_t is_bps;
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_shp_get_mode(gpi->gpi_base_va, id, &mode);
	if (mode == PFE_IQOS_SHP_BPS)
	{
		is_bps = TRUE;
	}
	else
	{
		is_bps = FALSE;
	}

	NXP_LOG_DEBUG("Shaper#%d using PFE sys_clk value %u MHz, clkdiv: %u\n", id, gpi->sys_clk_mhz,
		      igqos_clk_div(gpi->clk_div_log2));

	pfe_gpi_cfg_shp_get_isl_weight(gpi->gpi_base_va, id, &weight);

	*isl = igqos_convert_weight_to_isl(weight, gpi->clk_div_log2, gpi->sys_clk_mhz, is_bps);

	NXP_LOG_DEBUG("Shaper#%d - Get idle slope of: %u\n", id, *isl);

	return EOK;
}

errno_t pfe_gpi_shp_set_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t max_credit, int32_t min_credit)
{
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret != EOK)
	{
		return ret;
	}

	if (max_credit > IGQOS_PORT_SHP_CREDIT_MAX || max_credit < 0)
	{
		NXP_LOG_ERROR("Max credit value exceeded\n");
		return EINVAL;
	}

	if (min_credit < -IGQOS_PORT_SHP_CREDIT_MAX || min_credit > 0)
	{
		NXP_LOG_ERROR("Min credit value exceeded\n");
		return EINVAL;
	}

	pfe_gpi_cfg_shp_set_limits(gpi->gpi_base_va, id, max_credit, -min_credit);

	return EOK;
}

errno_t pfe_gpi_shp_get_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t *max_credit, int32_t *min_credit)
{
	uint32_t abs_max_cred, abs_min_cred;
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret != EOK)
	{
		return ret;
	}

	pfe_gpi_cfg_shp_get_limits(gpi->gpi_base_va, id, &abs_max_cred, &abs_min_cred);
	*max_credit = (int32_t)abs_max_cred;
	*min_credit = -(int32_t)abs_min_cred;

	return EOK;
}

/* note - the counter is reset to 0 after read (clear on read) */
errno_t pfe_gpi_shp_get_drop_cnt(const pfe_gpi_t *gpi, uint8_t id, uint32_t *cnt)
{
	errno_t ret;

	ret = pfe_gpi_shp_args_checks(gpi, id);
	if (ret != EOK)
	{
		return ret;
	}

	*cnt = pfe_gpi_cfg_shp_get_drop_cnt(gpi->gpi_base_va, id);

	return EOK;
}

/**
 * @brief		Return GPI runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	gpi 		The GPI instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_gpi_get_text_statistics(const pfe_gpi_t *gpi, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	errno_t ret = _pfe_gpi_null_arg_check_return(gpi, EINVAL);
	if (ret != EOK)
	{
		return 0U;
	}

	len += pfe_gpi_cfg_get_text_stat(gpi->gpi_base_va, buf, buf_len, verb_level);


	return len;
}
