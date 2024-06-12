/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_GPI_H_
#define PUBLIC_PFE_GPI_H_

/* Ingress QoS flow specification */

#define PFE_IQOS_FLOW_TABLE_SIZE	64U
#define PFE_IQOS_FLOW_TABLE_ENTRY_SKIP	0xFFU

/* flow type to match */
typedef enum __attribute__((packed))
{
    PFE_IQOS_FLOW_TYPE_ETH = (1 << 0),     /**< match ETH packets */
    PFE_IQOS_FLOW_TYPE_PPPOE = (1 << 1),   /**< match PPPoE packets */
    PFE_IQOS_FLOW_TYPE_ARP = (1 << 2),     /**< match ARP packets */
    PFE_IQOS_FLOW_TYPE_IPV4 = (1 << 3),    /**< match IPv4 packets */
    PFE_IQOS_FLOW_TYPE_IPV6 = (1 << 4),    /**< match IPv6 packets */
    PFE_IQOS_FLOW_TYPE_IPX = (1 << 5),     /**< match IPX packets */
    PFE_IQOS_FLOW_TYPE_MCAST = (1 << 6),   /**< match multicast (L2) packets */
    PFE_IQOS_FLOW_TYPE_BCAST = (1 << 7),   /**< match L2 broadcast packets */
    PFE_IQOS_FLOW_TYPE_VLAN = (1 << 8),    /**< match VLAN tagged packets */

    PFE_IQOS_FLOW_TYPE_MAX = PFE_IQOS_FLOW_TYPE_VLAN,
    /* Ensure proper size */
    PFE_IQOS_FLOW_TYPE_RESERVED = (uint16_t)(1U << 15U)
} pfe_iqos_flow_type_t;
ct_assert(sizeof(pfe_iqos_flow_type_t) == sizeof(uint16_t));

/* header field argument type to match */
typedef enum __attribute__((packed))
{
    PFE_IQOS_ARG_VLAN = (1 << 0),     /**< match on VLAN ID range arguments */
    PFE_IQOS_ARG_TOS = (1 << 1),      /**< match on TOS range arguments */
    PFE_IQOS_ARG_L4PROTO = (1 << 2),  /**< match on L4 PROTO range arguments */
    PFE_IQOS_ARG_SIP = (1 << 3),      /**< match on source IPv4/IPv6 address range arguments */
    PFE_IQOS_ARG_DIP = (1 << 4),      /**< match on destination IPv4/IPv6 address range arguments */
    PFE_IQOS_ARG_SPORT = (1 << 5),    /**< match on L4 source port range arguments */
    PFE_IQOS_ARG_DPORT = (1 << 6),    /**< match on L4 destination port range arguments */

    PFE_IQOS_ARG_MAX = PFE_IQOS_ARG_DPORT,
    /* Ensure proper size */
    PFE_IQOS_ARG_RESERVED = (uint16_t)(1U << 15U)
} pfe_iqos_flow_arg_type_t;
ct_assert(sizeof(pfe_iqos_flow_arg_type_t) == sizeof(uint16_t));

/* header fields arguments to match */
typedef struct __attribute__((packed, aligned(4)))
{
    uint16_t vlan;      /*< VLAN ID (12b) */
    uint16_t vlan_m;    /*< VLAN ID mask (12b) */
    uint8_t tos;        /*< TOS field for IPv4, TCLASS for IPv6 (8b) */
    uint8_t tos_m;      /*< TOS mask (8b) */
    uint8_t l4proto;    /*< L4 protocol field for IPv4 and IPv6 (8b)*/
    uint8_t l4proto_m;  /*< L4 protocol mask (8b)*/
    uint32_t sip;       /*< source IP address for IPv4 and IPv6 (32b) */
    uint32_t dip;       /*< destination IP address for IPv4 and IPv6 (32b) */
    uint8_t sip_m;      /*< source IP address mask, "6 bit encoded" */
    uint8_t dip_m;      /*< destination IP address mask, "6 bit encoded" */
    uint16_t sport_max; /*< max L4 source port (16b) */
    uint16_t sport_min; /*< min L4 source port (16b) */
    uint16_t dport_max; /*< max L4 destination port (16b) */
    uint16_t dport_min; /*< min L4 destination port (16b) */
} pfe_iqos_flow_args_t;

#define PFE_IQOS_VLAN_ID_MASK	0xFFFU
#define PFE_IQOS_TOS_MASK	0xFF
#define PFE_IQOS_L4PROTO_MASK	0xFF
#define PFE_IQOS_SDIP_MASK	0x3FU

typedef enum __attribute__((packed))
{
    PFE_IQOS_FLOW_MANAGED = 0,	/**< Mark flow as Managed. Default action on flow match */
    PFE_IQOS_FLOW_DROP,		/**< Drop flow */
    PFE_IQOS_FLOW_RESERVED,	/**< Mark flow as Reserved. */

    PFE_IQOS_FLOW_COUNT         /* must be last */
} pfe_iqos_flow_action_t;
ct_assert(sizeof(pfe_iqos_flow_action_t) == sizeof(uint8_t));

typedef struct __attribute__((packed, aligned(4)))
{
    pfe_iqos_flow_type_t type_mask;
    pfe_iqos_flow_arg_type_t arg_type_mask;
    pfe_iqos_flow_args_t args;
    pfe_iqos_flow_action_t action;
} pfe_iqos_flow_spec_t;

/* Ingress QoS WRED specification */
typedef enum __attribute__((packed))
{
    PFE_IQOS_Q_DMEM = 0,/**< select DMEM for WRED configuration */
    PFE_IQOS_Q_LMEM,    /**< select LMEM for WRED configuration */
    PFE_IQOS_Q_RXF,     /**< select RXF for WRED configuration */

    PFE_IQOS_Q_COUNT    /* must be last */
} pfe_iqos_queue_t;

typedef enum __attribute__((packed))
{
    PFE_IQOS_WRED_ZONE1 = 0,   /**< WRED probability zone1 */
    PFE_IQOS_WRED_ZONE2,       /**< WRED probability zone2 */
    PFE_IQOS_WRED_ZONE3,       /**< WRED probability zone3 */
    PFE_IQOS_WRED_ZONE4,       /**< WRED probability zone4 */

    PFE_IQOS_WRED_ZONES_COUNT  /* must be last */
} pfe_iqos_wred_zone_t;

#define PFE_IQOS_WRED_ZONE_PROB_MAX        15U /* increments of 1/16 */
#define PFE_IQOS_WRED_ZONE_PROB_SKIP      255U /* ignore provided prop value, preserve exiting one */
#define PFE_IQOS_WRED_ZONE1_PROB_DEFAULT    1U
#define PFE_IQOS_WRED_ZONE2_PROB_DEFAULT    2U
#define PFE_IQOS_WRED_ZONE3_PROB_DEFAULT    4U
#define PFE_IQOS_WRED_ZONE4_PROB_DEFAULT    8U

typedef enum __attribute__((packed))
{
    PFE_IQOS_WRED_MIN_THR = 0, /**< WRED queue threshold min level */
    PFE_IQOS_WRED_MAX_THR,     /**< WRED queue threshold max level */
    PFE_IQOS_WRED_FULL_THR,    /**< WRED queue threshold full level */

    PFE_IQOS_WRED_THR_COUNT    /* must be last */
} pfe_iqos_wred_thr_t;

#define PFE_IQOS_WRED_THR_MAX                0x200U
#define PFE_IQOS_WRED_DMEM_THR_MAX           0x2000U
#define PFE_IQOS_WRED_THR_SKIP               0xFFFFU /* ignore provided prop value, preserve exiting one */

#define PFE_IQOS_WRED_MIN_THR_DEFAULT        0x100U
#define PFE_IQOS_WRED_DMEM_MIN_THR_DEFAULT   0x1000U
#define PFE_IQOS_WRED_MAX_THR_DEFAULT        0x1F0U
#define PFE_IQOS_WRED_DMEM_MAX_THR_DEFAULT   0x1FF0U
#define PFE_IQOS_WRED_FULL_THR_DEFAULT       PFE_IQOS_WRED_THR_MAX
#define PFE_IQOS_WRED_DMEM_FULL_THR_DEFAULT  PFE_IQOS_WRED_DMEM_THR_MAX

#define PFE_IQOS_WRED_WEIGHT_MAX             0xFFFFU
#define PFE_IQOS_WRED_WEIGHT_DEFAULT         0x1U

/* Ingress QoS rate shaping specification */
#define PFE_IQOS_SHP_COUNT  2U   /* number of available shapers */

typedef enum __attribute__((packed))
{
    PFE_IQOS_SHP_PORT_LEVEL = 0,    /* port level data rate shaper */
    PFE_IQOS_SHP_BCAST,             /* shaper for broadcast packets */
    PFE_IQOS_SHP_MCAST,             /* shaper for multicast packets */

    PFE_IQOS_SHP_TYPE_COUNT         /* must be last */
} pfe_iqos_shp_type_t;

typedef enum __attribute__((packed))
{
    PFE_IQOS_SHP_BPS = 0,          /* specify data rate in bits per second */
    PFE_IQOS_SHP_PPS,              /* specify data rate in packets per second */

    PFE_IQOS_SHP_RATE_MODE_COUNT   /* must be last */
} pfe_iqos_shp_rate_mode_t;

typedef struct pfe_gpi_tag pfe_gpi_t;

typedef struct
{
	uint32_t alloc_retry_cycles;
	uint32_t gpi_tmlf_txthres;
	uint32_t gpi_dtx_aseq_len;
	uint16_t lmem_header_size;
	bool_t emac_1588_ts_en;
	bool_t g2_ordered_class_writes;
} pfe_gpi_cfg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_gpi_t *pfe_gpi_create(addr_t cbus_base_va, addr_t gpi_base, const pfe_gpi_cfg_t *cfg);
void pfe_gpi_enable(const pfe_gpi_t *gpi);
errno_t pfe_gpi_reset(const pfe_gpi_t *gpi);
void pfe_gpi_disable(const pfe_gpi_t *gpi);

uint32_t pfe_gpi_get_text_statistics(const pfe_gpi_t *gpi, struct seq_file *seq, uint8_t verb_level);

void pfe_gpi_destroy(pfe_gpi_t *gpi);
uint32_t pfe_gpi_cfg_get_sys_clk_mhz(addr_t cbus_base_va);

/* Ingress QoS API */
/* global ingress QoS block enable/disable */
errno_t	pfe_gpi_qos_enable(pfe_gpi_t *gpi);
errno_t pfe_gpi_qos_disable(const pfe_gpi_t *gpi);
bool_t pfe_gpi_qos_is_enabled(const pfe_gpi_t *gpi);
errno_t pfe_gpi_qos_reset(pfe_gpi_t *gpi);

/* flow add/remove/get */
errno_t pfe_gpi_qos_add_flow(pfe_gpi_t *gpi, uint8_t id, pfe_iqos_flow_spec_t *flow);
errno_t pfe_gpi_qos_rem_flow(pfe_gpi_t *gpi, uint8_t id);
errno_t pfe_gpi_qos_get_flow(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_flow_spec_t *flow);
errno_t pfe_gpi_qos_get_first_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow);
errno_t pfe_gpi_qos_get_next_flow(pfe_gpi_t *gpi, uint8_t *id, pfe_iqos_flow_spec_t *flow);

/* WRED API */
errno_t pfe_gpi_wred_enable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue); /* enable with default settings */
errno_t pfe_gpi_wred_disable(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue);
bool_t pfe_gpi_wred_is_enabled(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue);
/* WRED modifiers */
errno_t pfe_gpi_wred_set_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t val);
errno_t pfe_gpi_wred_get_prob(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t *val);
errno_t pfe_gpi_wred_set_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t val);
errno_t pfe_gpi_wred_get_thr(const pfe_gpi_t *gpi, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t *val);

/* shaper API*/
errno_t pfe_gpi_shp_enable(pfe_gpi_t *gpi, uint8_t id); /* enable with default settings */
errno_t pfe_gpi_shp_disable(const pfe_gpi_t *gpi, uint8_t id);
bool_t pfe_gpi_shp_is_enabled(const pfe_gpi_t *gpi, uint8_t id);
/* shaper modifiers */
errno_t pfe_gpi_shp_set_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t mode);
errno_t pfe_gpi_shp_get_mode(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_rate_mode_t *mode);
errno_t pfe_gpi_shp_set_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t type);
errno_t pfe_gpi_shp_get_type(const pfe_gpi_t *gpi, uint8_t id, pfe_iqos_shp_type_t *type);
errno_t pfe_gpi_shp_set_idle_slope(pfe_gpi_t *gpi, uint8_t id, uint32_t isl);
errno_t pfe_gpi_shp_get_idle_slope(const pfe_gpi_t *gpi, uint8_t id, uint32_t *isl);
errno_t pfe_gpi_shp_set_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t max_credit, int32_t min_credit);
errno_t pfe_gpi_shp_get_limits(const pfe_gpi_t *gpi, uint8_t id, int32_t *max_credit, int32_t *min_credit);
errno_t pfe_gpi_shp_get_drop_cnt(const pfe_gpi_t *gpi, uint8_t id, uint32_t *cnt);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_GPI_H_ */
