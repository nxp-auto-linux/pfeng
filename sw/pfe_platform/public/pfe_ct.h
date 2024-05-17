/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_PFE_PLATFORM
 * @{
 *
 * @file		pfe_ct.h
 * @brief		Common types (s32g).
 * @details		This header contains data types shared by host as well as PFE firmware.
 *
 */

#ifndef HW_S32G_PFE_CT_H_
#define HW_S32G_PFE_CT_H_

#include "pfe_compiler.h"
#if (!defined(PFE_COMPILER_BITFIELD_BEHAVIOR))
	#error PFE_COMPILER_BITFIELD_BEHAVIOR is not defined
#endif
#if (!defined(PFE_COMPILER_BITFIELD_HIGH_FIRST))
	#error PFE_COMPILER_BITFIELD_HIGH_FIRST is not defined
#endif
#if (!defined(PFE_COMPILER_BITFIELD_HIGH_LAST))
	#error PFE_COMPILER_BITFIELD_HIGH_LAST is not defined
#endif
#if (PFE_COMPILER_BITFIELD_HIGH_LAST == PFE_COMPILER_BITFIELD_HIGH_FIRST)
	#error PFE_COMPILER_BITFIELD_HIGH_LAST is equal to PFE_COMPILER_BITFIELD_HIGH_FIRST
#endif
#if (!defined(PFE_COMPILER_RESULT))
	#error PFE_COMPILER_RESULT is not defined
#endif
#if (!defined(PFE_COMPILER_RESULT_FW))
	#error PFE_COMPILER_RESULT_FW is not defined
#endif
#if (!defined(PFE_COMPILER_RESULT_DRV))
	#error PFE_COMPILER_RESULT_DRV is not defined
#endif
#if (PFE_COMPILER_RESULT_DRV == PFE_COMPILER_RESULT_FW)
	#error PFE_COMPILER_RESULT_DRV is equal to PFE_COMPILER_RESULT_FW
#endif

#if PFE_COMPILER_RESULT == PFE_COMPILER_RESULT_DRV
	/* Compiling for the driver */
	#include "oal_types.h"
	#define PFE_PTR(type)	uint32_t
#elif PFE_COMPILER_RESULT == PFE_COMPILER_RESULT_FW
	/* Compiling for the firmware */
	#include "fp_types.h"
	#define PFE_PTR(type)	type *
#else
	#error Ambiguous value of PFE_COMPILER_RESULT
#endif

/**
 * @brief	List of available interfaces
 * @details	This is list of identifiers specifying particular available
 * 			(physical) interfaces of the PFE.
 * @note	Current PFE does support max 8-bit IDs.
 */
typedef enum __attribute__((packed))
{
	/*	HW interfaces */
	PFE_PHY_IF_ID_EMAC0 = 0U,
	PFE_PHY_IF_ID_EMAC1 = 1U,
	PFE_PHY_IF_ID_EMAC2 = 2U,
	PFE_PHY_IF_ID_HIF = 3U,
	PFE_PHY_IF_ID_HIF_NOCPY = 4U,
	/*	UTIL PE - FW internal use */
	PFE_PHY_IF_ID_UTIL = 5U,
	/*	Synthetic interfaces */
	PFE_PHY_IF_ID_HIF0 = 6U,
	PFE_PHY_IF_ID_HIF1 = 7U,
	PFE_PHY_IF_ID_HIF2 = 8U,
	PFE_PHY_IF_ID_HIF3 = 9U,
	/*	Internals */
	PFE_PHY_IF_ID_MAX = PFE_PHY_IF_ID_HIF3,
	PFE_PHY_IF_ID_INVALID
} pfe_ct_phy_if_id_t;

typedef pfe_ct_phy_if_id_t pfe_drv_id_t;

/*	We expect given pfe_ct_phy_if_id_t size due to byte order compatibility. In case this
 	assert fails the respective code must be reviewed and every usage of the type must
 	be treated by byte order swap where necessary (typically any place in host software
	where the values are communicated between firmware and the host). */
ct_assert(sizeof(pfe_ct_phy_if_id_t) == sizeof(uint8_t));

/**
 * @brief	Interface matching rules
 * @details	This flags can be used to define matching rules for every logical interface. Every
 * 			packet received via physical interface is classified to get associated logical
 * 			interface. The classification can be done on single, or combination of rules.
 */
typedef enum __attribute__((packed))
{
	/* HW Accelerated Rules */
    IF_MATCH_NONE = 0U,                     /*!< No match rule used */
	IF_MATCH_TYPE_ETH = (1U << 0U),         /*!< Match ETH Packets */
    IF_MATCH_TYPE_VLAN = (1U << 1U),        /*!< Match VLAN Tagged Packets */
    IF_MATCH_TYPE_PPPOE = (1U << 2U),       /*!< Match PPPoE Packets */
    IF_MATCH_TYPE_ARP = (1U << 3U),         /*!< Match ARP Packets */
    IF_MATCH_TYPE_MCAST = (1U << 4U),       /*!< Match Multicast (L2) Packets */
    IF_MATCH_TYPE_IPV4 = (1U << 5U),        /*!< Match IPv4 Packets */
    IF_MATCH_TYPE_IPV6 = (1U << 6U),        /*!< Match IPv6 Packets */
    IF_MATCH_RESERVED7 = (1U << 7U),        /*!< Reserved */
    IF_MATCH_RESERVED8 = (1U << 8U),        /*!< Reserved */
    IF_MATCH_TYPE_IPX = (1U << 9U),         /*!< Match IPX Packets */
    IF_MATCH_TYPE_BCAST = (1U << 10U),      /*!< Match Broadcast (L2) Packets */
    IF_MATCH_TYPE_UDP = (1U << 11U),        /*!< Match UDP Packets */
    IF_MATCH_TYPE_TCP = (1U << 12U),        /*!< Match TCP Packets */
    IF_MATCH_TYPE_ICMP = (1U << 13U),       /*!< Match ICMP Packets */
    IF_MATCH_TYPE_IGMP = (1U << 14U),       /*!< Match IGMP Packets */
    IF_MATCH_VLAN = (1U << 15U),            /*!< Match VLAN ID */
    IF_MATCH_PROTO = (1U << 16U),           /*!< Match IP Protocol */
    IF_MATCH_SPORT = (1U << 20U),           /*!< Match L4 Source Port */
    IF_MATCH_DPORT = (1U << 21U),           /*!< Match L4 Destination Port */
	/* Pure SW */
	IF_MATCH_SIP6 = (1U << 22U),			/*!< Match Source IPv6 Address */
	IF_MATCH_DIP6 = (1U << 23U),			/*!< Match Destination IPv6 Address */
	IF_MATCH_SIP = (1U << 24U),				/*!< Match Source IPv4 Address */
	IF_MATCH_DIP = (1U << 25U),				/*!< Match Destination IPv4 Address */
	IF_MATCH_ETHTYPE = (1U << 26U),			/*!< Match EtherType */
	IF_MATCH_FP0 = (1U << 27U),				/*!< Match Packets Accepted by Flexible Parser 0 */
	IF_MATCH_FP1 = (1U << 28U),				/*!< Match Packets Accepted by Flexible Parser 1 */
	IF_MATCH_SMAC = (1U << 29U),			/*!< Match Source MAC Address */
	IF_MATCH_DMAC = (1U << 30U),			/*!< Match Destination MAC Address */
	IF_MATCH_HIF_COOKIE = (int)(1U << 31U),	/*!< Match HIF header cookie value */
	/* Ensure proper size */
	IF_MATCH_MAX = (int)(1U << 31U)
} pfe_ct_if_m_rules_t;

/*	We expect given pfe_ct_if_m_rules_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_m_rules_t) == sizeof(uint32_t));

typedef enum __attribute__((packed))
{
	/* No flag set */
	FP_FL_NONE = 0U,
	/* Invert match result */
	FP_FL_INVERT = (1U << 0U),
	/* Reject packet in case of match */
	FP_FL_REJECT = (1U << 1U),
	/* Accept packet in case of match */
	FP_FL_ACCEPT = (1U << 2U),
	/* Data offset is relative from start of L3 header */
	FP_FL_L3_OFFSET = (1U << 3U),
	/* Data offset is relative from start of L4 header */
	FP_FL_L4_OFFSET = (1U << 4U),
	/* Just to ensure correct size */
	FP_FL_MAX = (1U << 7U)
} pfe_ct_fp_flags_t;

ct_assert(sizeof(pfe_ct_fp_flags_t) == sizeof(uint8_t));

/**
 * @brief The Flexible Parser rule
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Data to be matched with packet payload */
	uint32_t data;
	/* Mask to be applied to data before comparison */
	uint32_t mask;
	/* Offset within packet where data to be compared is
	located . It is relative value depending on rule
	configuration ( FP_FL_xx_OFFSET ). In case when none
	of FP_FL_xx_OFFSET flags is set the offset is from
	0th byte of the packet . */
	uint16_t offset;
	/* Index within the Flexible Parser table identifying
	next rule to be applied in case the current rule does
	not contain FP_FL_REJECT nor FP_FL_ACCEPT flags . */
	uint8_t next_idx;
	/* Control flags */
	pfe_ct_fp_flags_t flags;
} pfe_ct_fp_rule_t;

ct_assert(sizeof(pfe_ct_fp_rule_t) == 12U);

/*
* @brief Statistics gathered during flexible parser classification
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of frames matching the selection criteria */
	uint32_t accepted;
	/* Number of frames not matching the selection criteria */
	uint32_t rejected;
} pfe_ct_class_flexi_parser_stats_t;

/**
 * @brief The Flexible Parser table
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of rules in the table */
	uint16_t count;
	/* Reserved variables to keep "rules" aligned */
	uint16_t reserved16;
	/* Pointer to the array of "count" rules */
	PFE_PTR (pfe_ct_fp_rule_t) rules;
	pfe_ct_class_flexi_parser_stats_t  __attribute__((aligned(4))) fp_stats; /* Must be aligned at 4 bytes */
} pfe_ct_fp_table_t;

ct_assert(sizeof(pfe_ct_fp_table_t) == 16U);

typedef union __attribute__((packed, aligned(4)))
{
	/*	IPv4 (for IF_MATCH_SIP, IF_MATCH_DIP) */
	struct
	{
		uint32_t sip;
		uint32_t dip;
		uint32_t pad[6U];
	} v4;

	/*	IPv6 (for IF_MATCH_SIP6, IF_MATCH_DIP6) */
	struct
	{
		uint32_t sip[4U];
		uint32_t dip[4U];
	} v6;
} pfe_ct_ip_addresses_t;

/**
 * @brief	Interface matching rules arguments
 * @details	Argument values needed by particular rules given by pfe_ct_if_m_rules_t.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* VLAN ID (IF_MATCH_VLAN) */
	uint16_t vlan;
	/* Ether Type (IF_MATCH_ETHTYPE) */
	uint16_t ethtype;
	/* L4 source port number (IF_MATCH_SPORT) */
	uint16_t sport;
	/* L4 destination port number (IF_MATCH_DPORT) */
	uint16_t dport;
	/* Source and destination addresses */
	pfe_ct_ip_addresses_t ipv;
	/* Flexible Parser 0 table (IF_MATCH_FP0) */
	PFE_PTR(pfe_ct_fp_table_t) fp0_table;
	/* Flexible Parser 1 table (IF_MATCH_FP1) */
	PFE_PTR(pfe_ct_fp_table_t) fp1_table;
	/* HIF header cookie (IF_MATCH_HIF_COOKIE) */
	uint32_t hif_cookie;
	/* Source MAC Address (IF_MATCH_SMAC) */
	uint8_t __attribute__((aligned(4))) smac[6U]; /* Must be aligned at 4 bytes */
	/* IP protocol (IF_MATCH_PROTO) */
	uint8_t proto;
	/* Reserved */
	uint8_t reserved;
	/* Destination MAC Address (IF_MATCH_DMAC) */
	uint8_t __attribute__((aligned(4))) dmac[6U]; /* Must be aligned at 4 bytes */
} pfe_ct_if_m_args_t;

/*
* @brief Statistics gathered during classification (per algorithm and per logical interface)
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of frames processed regardless the result */
	uint32_t processed;
	/* Number of frames matching the selection criteria */
	uint32_t accepted;
	/* Number of frames not matching the selection criteria */
	uint32_t rejected;
	/* Number of frames marked to be dropped */
	uint32_t discarded;
} pfe_ct_class_algo_stats_t;

/*
* @brief Statistics gathered for each physical interface
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of ingress frames for the given interface  */
	uint32_t ingress;
	/* Number of egress frames for the given interface */
	uint32_t egress;
	/* Number of ingress frames with detected error (i.e. checksum) */
	uint32_t malformed;
	/* Number of ingress frames which were discarded */
	uint32_t discarded;
} pfe_ct_phy_if_stats_t;

/*
* @brief Statistic entry for vlan
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of ingress frames for the given vlan */
	uint32_t ingress;
	/* Number of egress frames for the given vlan */
	uint32_t egress;
	/* Number of ingress bytes for the given vlan */
	uint32_t ingress_bytes;
	/* Number of egress bytes for the given vlan */
	uint32_t egress_bytes;
} pfe_ct_vlan_stats_t;

/*
* @brief Statistics gathered for each vlan
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of configured vlan */
	uint16_t vlan_count;
	/* Reserved variables to keep "stats" aligned */
	uint16_t reserved16;
	/* Pointer to vlan stats table */
	PFE_PTR (pfe_ct_vlan_stats_t) vlan;	
} pfe_ct_vlan_statistics_t;

/*
* @brief Statistic entry for conntrack
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of frames that hit the conntrack */
	uint32_t hit;
	/* Number of bytes of frames that hit the conntrack */
	uint32_t hit_bytes;
} pfe_ct_conntrack_stats_t;

/*
* @brief Statistics gathered for each conntrack
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of configured conntracks */
	uint16_t conntrack_count;
	/* Reserved variables to keep "stats" aligned */
	uint16_t reserved16;
	/* Pointer to conntrack stats table */
	PFE_PTR (pfe_ct_conntrack_stats_t) stats_table;
} pfe_ct_conntrack_statistics_t;

/*
* @brief Statistics gathered for the whole processing engine (PE)
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of packets processed by the PE */
	uint32_t processed;
	/* Number of packets discarded by the PE */
	uint32_t discarded;
	/* Count of frames with replicas count 1, 2, ...
	PFE_PHY_IF_ID_MAX+1 (+1 because interfaces are numbered from 0) */
	uint32_t replicas[PFE_PHY_IF_ID_MAX + 1U];
	/* Number of HIF frames with HIF_TX_INJECT flag */
	uint32_t injected;
} pfe_ct_pe_stats_t;

/**
 * @brief	Interface operational flags
 * @details Defines way how ingress packets matching given interface will be processed
 * 			by the classifier.
 */
typedef enum __attribute__((packed))
{
	IF_OP_DEFAULT = 0U,		/*!< Default operational mode */
	IF_OP_VLAN_BRIDGE = 1U,		/*!< L2 bridge with VLAN */
	IF_OP_ROUTER = 2U,		/*!< L3 router */
	IF_OP_FLEX_ROUTER = 3U,		/*!< Flexible router */
	IF_OP_L2L3_VLAN_BRIDGE = 4U,	/*!< L2-L3 bridge with VLAN */
} pfe_ct_if_op_mode_t;

/*	We expect given pfe_ct_if_op_mode_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_op_mode_t) == sizeof(uint8_t));

typedef enum __attribute__((packed))
{
	IF_FL_NONE = 0U,					/*!< No flag set */
	IF_FL_ENABLED = (1U << 0U),			/*!< If set, interface is enabled */
	IF_FL_PROMISC = (1U << 1U),			/*!< If set, interface is promiscuous */
	IF_FL_FF_ALL_TCP = (1U << 2U),		/*!< Enable fast-forwarding of ingress TCP SYN|FIN|RST packets */
	IF_FL_MATCH_OR = (1U << 3U),		/*!< Result of match is logical OR of rules, else AND */
	IF_FL_DISCARD = (1U << 4U),			/*!< Discard packets on rules match */
	IF_FL_LOAD_BALANCE = (1U << 6U),	/*!< HIF channel participates in load balancing */
	IF_FL_VLAN_CONF_CHECK = (1U << 7U),	/*!< Enable VLAN conformance check */
	IF_FL_PTP_CONF_CHECK = (1U << 8U),	/*!< Enable PTP conformance check */
	IF_FL_PTP_PROMISC = (1U << 9U),		/*!< PTP traffic will bypass all ingress checks */
	IF_FL_LOOPBACK = (1U << 10U),		/*!< If set, interface is in loopback mode */
	IF_FL_ALLOW_Q_IN_Q = (1U << 11U),	/*!< If set, QinQ traffic is accepted */
	IF_FL_DISCARD_TTL = (1U << 12U),	/*!< Discard packet with TTL<2 instead of passing to default logical interface */
	IF_FL_MAX = (int)(1U << 31U)
} pfe_ct_if_flags_t;

/*	We expect given pfe_ct_if_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_flags_t) == sizeof(uint32_t));

/**
 * @brief	Acceptable frame types
 */
typedef enum  __attribute__((packed))
{
	IF_AFT_ANY_TAGGING = 0U,
	IF_AFT_TAGGED_ONLY = 1U,
	IF_AFT_UNTAGGED_ONLY = 2U
} pfe_ct_if_aft_t;

/*	We expect given pfe_ct_if_aft_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_aft_t) == sizeof(uint8_t));

/**
 * @brief	Interface blocking state
 */
typedef enum __attribute__((packed))
{
	IF_BS_FORWARDING = 0U,	/*!< Learning and forwarding enabled */
	IF_BS_BLOCKED = 1U,		/*!< Learning and forwarding disabled */
	IF_BS_LEARN_ONLY = 2U,	/*!< Learning enabled, forwarding disabled */
	IF_BS_FORWARD_ONLY = 3U	/*!< Learning disabled, forwarding enabled */
} pfe_ct_block_state_t;

/*	We expect given pfe_ct_block_state_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_block_state_t) == sizeof(uint8_t));

/**
 * @brief	The logical interface structure as seen by firmware
 * @details	This structure is shared between firmware and the driver. It represents
 * 			the logical interface as it is stored in the DMEM.
 * @warning	Do not modify this structure unless synchronization with firmware is ensured.
 */
typedef struct __attribute__((packed, aligned(4))) pfe_ct_log_if_tag
{
	/*	Pointer to next logical interface in the list (DMEM) */
	PFE_PTR(struct pfe_ct_log_if_tag) next;
	/*	List of egress physical interfaces. Bit positions correspond
		to pfe_ct_phy_if_id_t values (1U << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	Flags */
	pfe_ct_if_flags_t flags;
	/*	Match rules. Zero means that matching is disabled and packets
		can be accepted on interface in promiscuous mode only. */
	pfe_ct_if_m_rules_t m_rules;
	/*	Interface identifier */
	uint8_t id;
	/*	Operational mode */
	pfe_ct_if_op_mode_t mode;
	/*	Reserved */
	uint8_t res[2];
	/*	Arguments required by matching rules */
	pfe_ct_if_m_args_t __attribute__((aligned(4))) m_args; /* Must be aligned at 4 bytes */
	/*	Gathered statistics */
	pfe_ct_class_algo_stats_t __attribute__((aligned(4))) class_stats; /* Must be aligned at 4 bytes */
} pfe_ct_log_if_t;

typedef enum __attribute__((packed))
{
	SPD_ACT_INVALID = 0U,			/* Undefined action - configuration is required */
	SPD_ACT_DISCARD,				/* Discard the frame */
	SPD_ACT_BYPASS,					/* Bypass IPsec and forward normally */
	SPD_ACT_PROCESS_ENCODE,			/* Process IPsec */
	SPD_ACT_PROCESS_DECODE			/* Process IPsec */
} pfe_ct_spd_entry_action_t;
ct_assert(sizeof(pfe_ct_spd_entry_action_t) == sizeof(uint8_t));

typedef enum __attribute__((packed))
{
	SPD_FLAG_NONE = 0U,					/* No flag set */
	SPD_FLAG_5T = (1U << 0U),			/* 5-tuple acceleration by HW, if not set the id5t shall be 0 */
	SPD_FLAG_IPv6 = (1U << 1U),			/* IPv4 if not set, IPv6 if set */
	SPD_FLAG_SPORT_OPAQUE = (1U << 2U),	/* Do not match Source PORT */
	SPD_FLAG_DPORT_OPAQUE = (1U << 3U),	/* Do not match Destination PORT */
} pfe_ct_spd_flags_t;
ct_assert(sizeof(pfe_ct_spd_flags_t) == sizeof(uint8_t));

typedef struct __attribute__((packed, aligned(4)))
{
	pfe_ct_spd_flags_t flags;
	/* --- Match criteria ---*/
	/*	IP protocol number */
	uint8_t proto;	/* IP protocol */
	uint16_t pad;	/* align at 4 bytes boundary */
	/*	L4 source port number */
	uint16_t sport;
	/*	L4 destination port number */
	uint16_t dport;
	/*	Source and destination IP addresses */
	pfe_ct_ip_addresses_t ipv;
	uint32_t id5t;	/* 5-tuple ID to speed search, 0 = invalid ID */
	uint32_t spi;	/* SPI value to match - only for action SPD_ACT_PROCESS_DECODE */
	/* --- Action --- */
	uint32_t sad_entry; /* How to process IPsec */
	pfe_ct_spd_entry_action_t action; /* What to do on match */
	uint8_t pad8[3U];
} pfe_ct_spd_entry_t;

typedef struct __attribute__((packed, aligned(4)))
{
	uint32_t entry_count;					/* Count of the entries in the database */
	pfe_ct_spd_entry_action_t no_ip_action;	/* Non-ip traffic action - may not be SPD_ACT_PROCESS */
	uint8_t pad[3U];						/* Align to 4 bytes */
	PFE_PTR(pfe_ct_spd_entry_t) entries;	/* Database entries */
} pfe_ct_ipsec_spd_t;

/**
* @brief Configures mirroring 
*/
typedef struct __attribute__((packed, aligned(4))) pfe_ct_mirror_tag pfe_ct_mirror_t;

/*
* @def PFE_CT_MIRRORS_COUNT
* @brief Number of RX and TX mirrors supported by physical interface.
*/
#define PFE_CT_MIRRORS_COUNT 2U
/**
 * @brief	The physical interface structure as seen by classifier/firmware
 * @details	This structure is shared between firmware and the driver. It represents
 * 			the interface as it is stored in the DMEM.
 * @warning	Do not modify this structure unless synchronization with firmware is ensured.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*	Pointer to head of list of logical interfaces (DMEM) */
	PFE_PTR(pfe_ct_log_if_t) log_ifs;
	/*	Pointer to default logical interface (DMEM) */
	PFE_PTR(pfe_ct_log_if_t) def_log_if;
	/*	Flags */
	pfe_ct_if_flags_t flags;
	/*	Physical port number */
	pfe_ct_phy_if_id_t id;
	/*	Operational mode */
	pfe_ct_if_op_mode_t mode;
	/*	Block state */
	pfe_ct_block_state_t block_state;
	/*	Mirroring to given port */
	PFE_PTR(pfe_ct_mirror_t) rx_mirrors[PFE_CT_MIRRORS_COUNT];
	PFE_PTR(pfe_ct_mirror_t) tx_mirrors[PFE_CT_MIRRORS_COUNT];
	/*	SPD for IPsec */
	PFE_PTR(pfe_ct_ipsec_spd_t) ipsec_spd;
	/*	Flexible Filter */
	PFE_PTR(pfe_ct_fp_table_t) filter;
	/* management interface */
	pfe_ct_phy_if_id_t mgmt_interface;
	/*	Gathered statistics */
	pfe_ct_phy_if_stats_t phy_stats __attribute__((aligned(4))); /* Must be aligned to 4 bytes */
} pfe_ct_phy_if_t;

/**
 * @brief	L2 Bridge Actions
 */
typedef enum __attribute__((packed))
{
	L2BR_ACT_FORWARD = 0U,   /*!< Forward normally */
	L2BR_ACT_FLOOD = 1U,     /*!< Flood */
	L2BR_ACT_PUNT = 2U,      /*!< Punt */
	L2BR_ACT_DISCARD = 3U,   /*!< Discard */
} pfe_ct_l2br_action_t;

#if PFE_COMPILER_BITFIELD_BEHAVIOR == PFE_COMPILER_BITFIELD_HIGH_LAST
/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef union __attribute__((packed))
{
	struct
	{
		/* [19:0] Forward port list */
		uint32_t forward_list : 20;
		/* [25:20] Reserved */
		uint32_t reserved : 6;
		/* [26] Discard on DST MAC match */
		uint32_t dst_discard : 1;
		/* [27] Discard on SRC MAC match */
		uint32_t src_discard : 1;
		/* [28] Local L3 */
		uint32_t local_l3 : 1;
		/* [29] Fresh */
		uint32_t fresh_flag : 1;
		/* [30] Static */
		uint32_t static_flag : 1;
		/* [31] Reserved */
		uint32_t reserved1 : 1;
	} item;

	uint32_t val : 32;
} pfe_ct_mac_table_result_t;

/**
 * @brief	VLAN table lookup result (64-bit)
 */
typedef union __attribute__((packed))
{
	struct
	{
		/*	[17:0]  Forward list (1U << pfe_ct_phy_if_id_t) */
		uint64_t forward_list : 18;
		/*	[35:18] Untag list (1U << pfe_ct_phy_if_id_t) */
		uint64_t untag_list : 18;
		/*	[38:36] Unicast hit action (pfe_ct_l2br_action_t) */
		uint64_t ucast_hit_action : 3;
		/*	[41:39] Multicast hit action (pfe_ct_l2br_action_t) */
		uint64_t mcast_hit_action : 3;
		/*	[44:42] Unicast miss action (pfe_ct_l2br_action_t) */
		uint64_t ucast_miss_action : 3;
		/*	[47:45] Multicast miss action (pfe_ct_l2br_action_t) */
		uint64_t mcast_miss_action : 3;
		/*	[54:48] Stats index */
		uint64_t stats_index : 7;
		/*	[55 : 63] */ /* Reserved */
		uint64_t  hw_reserved : 9;
	} item;

	uint64_t val;
} pfe_ct_vlan_table_result_t;
#elif PFE_COMPILER_BITFIELD_BEHAVIOR == PFE_COMPILER_BITFIELD_HIGH_FIRST
/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef union __attribute__((packed))
{
	struct
	{
		/* [31] Reserved */
		uint32_t reserved1 : 1;
		/* [30] Static */
		uint32_t static_flag : 1;
		/* [29] Fresh */
		uint32_t fresh_flag : 1;
		/* [28] Local L3 */
		uint32_t local_l3 : 1;
		/* [27] Discard on SRC MAC match */
		uint32_t src_discard : 1;
		/* [26] Discard on DST MAC match */
		uint32_t dst_discard : 1;
		/* [25:20] Reserved */
		uint32_t reserved : 6;
		/* [19:0] Forward port list */
		uint32_t forward_list : 20;
	} item;

	uint32_t val : 32;
} pfe_ct_mac_table_result_t;

/**
 * @brief	VLAN table lookup result (64-bit)
 */
typedef union __attribute__((packed))
{
	struct
	{
		/*	[55 : 63] */ /* Reserved */
		uint64_t hw_reserved : 9;
		/*	[54:48] Index in vlan stats table (pfe_ct_vlan_statistics_t) */
		uint64_t stats_index : 7;
		/*	[47:45] Multicast miss action (pfe_ct_l2br_action_t) */
		uint64_t mcast_miss_action : 3;
		/*	[44:42] Unicast miss action (pfe_ct_l2br_action_t) */
		uint64_t ucast_miss_action : 3;
		/*	[41:39] Multicast hit action (pfe_ct_l2br_action_t) */
		uint64_t mcast_hit_action : 3;
		/*	[38:36] Unicast hit action (pfe_ct_l2br_action_t) */
		uint64_t ucast_hit_action : 3;
		/*	[35:18] Untag list (1U << pfe_ct_phy_if_id_t) */
		uint64_t untag_list : 18;  /* List of ports to remove VLAN tag */
		/*	[17:0]  Forward list (1U << pfe_ct_phy_if_id_t) */
		uint64_t forward_list : 18;
	} item;

	uint64_t val;
} pfe_ct_vlan_table_result_t;
#else
	#error Ambiguous definition of PFE_COMPILER_BITFIELD_BEHAVIOR
#endif /* Compiler Behavior */

ct_assert(sizeof(pfe_ct_mac_table_result_t) == sizeof(uint32_t));
ct_assert(sizeof(pfe_ct_vlan_table_result_t) == sizeof(uint64_t));

/**
 * @brief Bridge domain entry
 */
typedef pfe_ct_vlan_table_result_t pfe_ct_bd_entry_t;

/*	Date string type */
typedef uint8_t pfe_date_str_t[16U];

/*	We expect given pfe_date_str_t size. */
ct_assert(sizeof(pfe_date_str_t) > sizeof(__DATE__));

/**
 * @brief Time string type
 */
typedef uint8_t pfe_time_str_t[16U];

/*	We expect given pfe_time_str_t size. */
ct_assert(sizeof(pfe_time_str_t) > sizeof(__TIME__));

/**
 * @brief Version control identifier string type
 */
typedef uint8_t pfe_vctrl_str_t[16U];

/**
 * @brief This header version MD5 checksum string type
 */
typedef char_t pfe_cthdr_str_t[36U]; /* 32 Characters + NULL + 3 padding */

/**
* @brief Identification of PE type the FW is used for
*/
typedef enum __attribute__((packed))
{
	PE_TYPE_INVALID,
	PE_TYPE_CLASS,
	PE_TYPE_TMU,
	PE_TYPE_UTIL,
	PE_TYPE_MAX
} pfe_ct_pe_type_t;
ct_assert(sizeof(pfe_ct_pe_type_t) == 1U);

/**
* @brief Feature flags
* Flags combinations:
* F_PRESENT is missing - the feature is not available
* F_PRESENT is set and F_RUNTIME is missing - the feature is always enabled (cannot be disabled)
* F_PRESENT is set and F_RUNTIME is set - the feature can be enabled/disable at runtime, enabled state must be read out of DMEM 
*/
typedef enum __attribute__((packed))
{
    F_NONE = 0U,
    F_PRESENT = (1U << 0U),     /* Feature not available if not set */
    F_RUNTIME = (1U << 1U),     /* Feature can be enabled/disabled at runtime */
    F_CLASS = (1U << 5U),       /* Feature implemented in Class firmware */
    F_UTIL = (1U << 6U)         /* Feature implemented in Util firmware */
} pfe_ct_feature_flags_t;
ct_assert(sizeof(pfe_ct_feature_flags_t) == 1U);

/**
* @brief Storage for firmware features description
*/
typedef struct __attribute__((packed,aligned(4)))
{
	PFE_PTR(const char)name;               /* Feature name */
	PFE_PTR(const char)description;        /* Feature description */
	PFE_PTR(uint8_t) position;             /* Position of the run-time enable byte */
	const pfe_ct_feature_flags_t flags;    /* Configuration variant: 0 = disabled, 1 = enabled, 2 = runtime configured */
	const uint8_t def_val;                 /* Enable/disable default value used for runtime configuration */
	const uint8_t reserved[2];             /* Pad */
} pfe_ct_feature_desc_t;

ct_assert(sizeof(pfe_ct_feature_desc_t) == 16);

/**
* @brief Version of the HW detected by the FW
*/
typedef enum __attribute__((packed))
{
	HW_VERSION_UNKNOWN = 0U,          /* FW has not recognized the HW version */
	HW_VERSION_S32G2 = 2U,            /* S32G2 */
	HW_VERSION_S32G3 = 3U,            /* S32G3 */
	HW_VERSION_MAX = (int)(1U << 31)  /* Ensure proper size */
} pfe_ct_hw_version_t;

ct_assert(sizeof(pfe_ct_hw_version_t) == sizeof(uint32_t));

/**
 * @brief Firmware version information
 */
typedef struct __attribute__((packed))
{
	/*	ID */
	uint32_t id;
	/*	Revision info */
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	/*	PE type */
	pfe_ct_pe_type_t pe_type;
	/*	Firmware properties */
	uint32_t flags;
	/*	Build date and time */
	pfe_date_str_t build_date;
	pfe_time_str_t build_time;
	/*	Version control ID (e.g. GIT commit) */
	pfe_vctrl_str_t vctrl;
	/*	This header version */
	pfe_cthdr_str_t cthdr;
	/*	Feature descriptions */
	PFE_PTR(pfe_ct_feature_desc_t) features;
	/*	Features count - number of items in features */
	uint32_t features_count;
	/*	Hardware Versions */
	PFE_PTR(pfe_ct_hw_version_t) hw_version;
} pfe_ct_version_t;

/**
	@brief Miscellaneous control commands between host and PE
*/
typedef struct __attribute__ (( packed, aligned (4) ))
{
	/*	Request from host to trigger the PE graceful stop. Writing a non - zero value triggers the stop.
		Once PE entered the stop state it notifies the host via setting the graceful_stop_confirmation
		to a non-zero value. To resume from stop state the host clear the graceful_stop_request to
		zero and waits until PE clears the graceful_stop_confirmation. */
	uint8_t graceful_stop_request;
	/*	Confirmation from PE that has entered or left the graceful stop state */
	uint8_t graceful_stop_confirmation;
} pfe_ct_pe_misc_control_t;

/**
	@brief Miscellaneous config between host and PE
*/
typedef struct __attribute__ (( packed, aligned (4) ))
{
	/*	Timeout of mac aging algorithm of l2 bridge in seconds*/
	uint16_t l2_mac_aging_timeout;
} pfe_ct_misc_config_t;

/*
* @brief Statistics gathered during IHC classification
*/
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of send IHC frames */
	uint32_t tx;
	/* Number of received IHC frames */
	uint32_t rx;
	/* Number of IHC frames marked to be dropped */
	uint32_t discarded;
} pfe_ct_class_ihc_stats_t;

/**
 * @brief Statistics gathered for each classification algorithm
 * @details NULL pointer means that given statistics are no available
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Statistics gathered by Flexible Router algorithm */
	pfe_ct_class_algo_stats_t flexible_router;
	/* Statistics gathered by IP router algorithm (IF_OP_ROUTER) */
	pfe_ct_class_algo_stats_t ip_router;
	/* Statistics gathered by VLAN bridge algorithm (IF_OP_VLAN_BRIDGE) */
	pfe_ct_class_algo_stats_t vlan_bridge;
	/* Statistics gathered by logical interface matching algorithm (IF_OP_DEFAULT) */
	pfe_ct_class_algo_stats_t log_if;
	/* Statistics gathered when hif-to-hif classification is done */
	pfe_ct_class_ihc_stats_t hif_to_hif[PFE_PHY_IF_ID_MAX + 1];
} pfe_ct_classify_stats_t;

/**
 * @brief Number of FW messages which can be stored in pfe_ct_message_record_t
 * @details The value must be power of 2.
 */
#define FP_MESSAGE_RECORD_SIZE 64U

/**
 * @brief	List of message levels
 */
typedef enum __attribute__((packed))
{
	PFE_MESSAGE_EXCEPTION = 0U,
	PFE_MESSAGE_ERROR     = 1U,
	PFE_MESSAGE_WARNING   = 2U,
	PFE_MESSAGE_INFO      = 3U,
	PFE_MESSAGE_DEBUG     = 4U
} pfe_ct_message_level_t;

/**
 * @brief Reported message storage
 * @note Instances of this structure are stored in an elf-file section .messages which
 *       is not loaded into any memory and the driver accesses it only through the
 *       elf-file.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Message content - string in .messages section */
	PFE_PTR(const char_t)message;
	/* File name where message was logged - string in .messages section */
	PFE_PTR(const char_t)file;
	/* Line where message was logged */
	const uint32_t line;
} pfe_ct_message_t;

/**
 * @brief Storage for runtime messeges
 * @note The pointers cannot be dereferenced because the .messages section is not loaded
 *       into memory and the elf-file parsing is needed to translate them.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Next position to write: (write_index & (FP_MESSAGE_RECORD_SIZE - 1)) */
	uint32_t write_index;
	/* Stored messages - pointers point to section .messages which is not
	    part of any memory, just the elf-file */
	PFE_PTR(const pfe_ct_message_t) messages[FP_MESSAGE_RECORD_SIZE];
	uint32_t values[FP_MESSAGE_RECORD_SIZE];
	pfe_ct_message_level_t  level[FP_MESSAGE_RECORD_SIZE];
} pfe_ct_message_record_t;

/**
 * @brief The firmware internal state
 */
typedef enum __attribute__((packed))
{
	PFE_FW_STATE_UNINIT = 0U,        /* FW not started */
	PFE_FW_STATE_INIT,               /* FW passed initialization */
	PFE_FW_STATE_FRAMEWAIT,          /* FW waiting for a new frame arrival */
	PFE_FW_STATE_FRAMEPARSE,         /* FW started parsing a new frame */
	PFE_FW_STATE_FRAMECLASSIFY,      /* FW started classification of parsed frame */
	PFE_FW_STATE_FRAMEDISCARD,       /* FW is discarding the frame */
	PFE_FW_STATE_FRAMEMODIFY,        /* FW is modifying the frame */
	PFE_FW_STATE_FRAMESEND,          /* FW is sending frame out (towards EMAC or HIF) */
	PFE_FW_STATE_STOPPED,            /* FW was gracefully stopped by external request */
	PFE_FW_STATE_EXCEPTION,          /* FW is stopped after an exception */
	PFE_FW_STATE_FAIL_STOP           /* FW is stopped due to a safety fault */
} pfe_ct_pe_sw_state_t;

/**
 * @brief Monitoring of the firmware state (watchdog)
 * @details FW updates the variable with the current state and increments counter
 *          with each state transition. The driver monitors the variable.
 * @note Written only by FW and read by Driver.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	uint32_t counter;               /* Incremented with each state change */
	pfe_ct_pe_sw_state_t state;     /* Reflect the current FW state */
	uint8_t reserved[3U];            /* To make size multiple of 4 bytes */
} pfe_ct_pe_sw_state_monitor_t;

ct_assert(sizeof(pfe_ct_pe_sw_state_monitor_t) == 8U);


/**
 * @brief Storage for measured time intervals used during firmware performance monitoring
 */
typedef struct __attribute__((packed, aligned(4)))
{
	uint32_t min;	/* Minimal measured value */
	uint32_t max;	/* Maximal measured value */
	uint32_t avg;	/* Average of measured values */
	uint32_t cnt;	/* Count of measurements */
} pfe_ct_measurement_t;

/**
 * @brief	Size of buffer defined by pfe_ct_buffer_t in number of bytes
 */
#define PFE_CT_BUFFER_LEN	64U

/**
 * @brief	Generic buffer descriptor
 */
typedef struct __attribute__((packed))
{
	/*	The buffer data area */
	uint8_t payload[PFE_CT_BUFFER_LEN];
	/*	Number of bytes in buffer */
	uint8_t len;
	/*	Non-zero value indicates that the buffer is valid */
	uint8_t flags;
} pfe_ct_buffer_t;

/**
 * @brief Common PE memory map representation type shared between host and PFE
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*	Size of the structure in number of bytes - must be 1st in structure */
	uint32_t size;
	/*	Version information */
	pfe_ct_version_t version;
	/*	Misc. control  */
	PFE_PTR(pfe_ct_pe_misc_control_t) pe_misc_control;
	/*	Misc. config  */
	PFE_PTR(pfe_ct_misc_config_t) misc_config;
	/*	Messages logged by the FW */
	PFE_PTR(pfe_ct_message_record_t) message_record;
	/*	FW state */
	PFE_PTR(pfe_ct_pe_sw_state_monitor_t) state_monitor;
	/*	Count of the measurement storages - 0 = feature not enabled */
	uint32_t measurement_count;
	/*	Performance measurement storages - NULL = none (feature not enabled) */
	PFE_PTR(pfe_ct_measurement_t) measurements;
} pfe_ct_common_mmap_t;

/**
 * @brief Information about configured HIF TMU queue sizes 
 * @details The value is sum of TMU Queue sizes belonging to the HIF channel with number
 *          equal to array index.
 */
typedef struct
{
	uint16_t hif_channel[4U];
} pfe_ct_hif_tmu_queue_sizes_t;

/**
 * @brief Class PE memory map representation type shared between host and PFE
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*	Common part for all PE types - must be 1st in the structure */
	pfe_ct_common_mmap_t common;
	/*	Pointer to DMEM heap */
	PFE_PTR(void) dmem_heap_base;
	/*	DMEM heap size in number of bytes */
	uint32_t dmem_heap_size;
	/*	Pointer to array of physical interfaces */
	PFE_PTR(pfe_ct_phy_if_t) dmem_phy_if_base;
	/*	Physical interfaces memory space size in number of bytes */
	uint32_t dmem_phy_if_size;
	/*	Fall-back bridge domain structure location (DMEM) */
	PFE_PTR(pfe_ct_bd_entry_t) dmem_fb_bd_base;
	/*	Default bridge domain structure location (DMEM) */
	PFE_PTR(pfe_ct_bd_entry_t) dmem_def_bd_base;
	/*	Statistics provided for the PE (by the firmware) */
	PFE_PTR(pfe_ct_pe_stats_t) pe_stats;
	/*	Statistics provided for each classification algorithm */
	PFE_PTR(pfe_ct_classify_stats_t) classification_stats;
	/*	Statistics provided for each vlan */
	PFE_PTR(pfe_ct_vlan_statistics_t) vlan_statistics;
	/*	Statistics provided for each conntrack */
	PFE_PTR(pfe_ct_conntrack_statistics_t) conntrack_statistics;
	/*	Put buffer: FW-to-SW data transfers */
	PFE_PTR(pfe_ct_buffer_t) put_buffer;
	/*	Get buffer: SW-to-FW data transfers */
	PFE_PTR(pfe_ct_buffer_t) get_buffer;
	/*	HIF TMU Queue sizes information for errata ERR051211 workaround*/
	PFE_PTR(pfe_ct_hif_tmu_queue_sizes_t) hif_tmu_queue_sizes;
} pfe_ct_class_mmap_t;

/**
 * @brief IPsec state
 */
typedef struct  {
	uint32_t hse_mu;					/* HSE MU to be used */
	uint32_t hse_mu_chn;				/* HSE MU channel to be used (currently unused) */
	uint32_t response_ok;				/* HSE_SRV_RSP_OK */
	uint32_t verify_failed;				/* HSE_SRV_RSP_VERIFY_FAILED */
	uint32_t ipsec_invalid_data;		/* HSE_SRV_RSP_IPSEC_INVALID_DATA */
	uint32_t ipsec_replay_detected;		/* HSE_SRV_RSP_IPSEC_REPLAY_DETECTED */
	uint32_t ipsec_replay_late;			/* HSE_SRV_RSP_IPSEC_REPLAY_LATE */
	uint32_t ipsec_seqnum_overflow;		/* HSE_SRV_RSP_IPSEC_SEQNUM_OVERFLOW */
	uint32_t ipsec_ce_drop;				/* HSE_SRV_RSP_IPSEC_CE_DROP */
	uint32_t ipsec_ttl_exceeded;		/* HSE_SRV_RSP_IPSEC_TTL_EXCEEDED */
	uint32_t ipsec_valid_dummy_payload;	/* HSE_SRV_RSP_IPSEC_VALID_DUMMY_PAYLOAD */
	uint32_t ipsec_header_overflow;		/* HSE_SRV_RSP_IPSEC_HEADER_LEN_OVERFLOW */
	uint32_t ipsec_padding_check_fail;	/* HSE_SRV_RSP_IPSEC_PADDING_CHECK_FAIL */
	uint32_t handled_error_code;		/* Code of handled error (one of above errors) */
	uint32_t handled_error_said;		/* SAId of handled error (one of above errors) */
	uint32_t unhandled_error_code;		/* default case store code */
	uint32_t unhandled_error_said;		/* default case store code */
} ipsec_state_t;

/**
 * @brief UTIL PE memory map representation type shared between host and PFE
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*  Common part for all PE types - must be 1st in the structure */
	pfe_ct_common_mmap_t common;
	PFE_PTR(ipsec_state_t) ipsec_state;
} pfe_ct_util_mmap_t;

typedef union __attribute__((packed, aligned(4)))
{
	pfe_ct_common_mmap_t common;	/* Common for both */
	pfe_ct_class_mmap_t class_pe;	/* Class PE variant */
	pfe_ct_util_mmap_t util_pe;		/* UTIL PE variant */
} pfe_ct_pe_mmap_t;

typedef enum __attribute__((packed))
{
	/*	No flag being set */
	HIF_RX_NO_FLAG = 0U,
	/*	IPv4 checksum valid */
	HIF_RX_IPV4_CSUM = (1U << 0U),
	/*	TCP of IPv4 checksum valid */
	HIF_RX_TCPV4_CSUM = (1U << 1U),
	/*	TCP of IPv6 checksum valid */
	HIF_RX_TCPV6_CSUM = (1U << 2U),
	/*	UDP of IPv4 checksum valid */
	HIF_RX_UDPV4_CSUM = (1U << 3U),
	/*	UDP of IPv6 checksum valid */
	HIF_RX_UDPV6_CSUM = (1U << 4U),
	/*	PTP packet */
	HIF_RX_PTP = (1U << 5U),
	/*	Timestamp flag. When set, timestamp is valid. */
	HIF_RX_TS = (1U << 7U),
	/*	Inter - HIF communication frame */
	HIF_RX_IHC = (1U << 8U),
	/*	Frame is Egress Timestamp Report */
	HIF_RX_ETS = (1U << 9U),
	/*	IPv6 checksum valid */
	HIF_RX_IPV6_CSUM = (1U << 10U),
	/*	ICMP checksum valid */
	HIF_RX_ICMP_CSUM = (1U << 11U),
	/*      Frame send to HIF0 has vlan tag*/
	HIF_RX_HIF0_VLAN = (1U << 12U),
	/*      Frame send to HIF1 has vlan tag*/
	HIF_RX_HIF1_VLAN = (1U << 13U),
	/*      Frame send to HIF2 has vlan tag*/
	HIF_RX_HIF2_VLAN = (1U << 14U),
	/*      Frame send to HIF3 has vlan tag*/
	HIF_RX_HIF3_VLAN = (1U << 15U),
	/*	Ensure proper size */
	HIF_RX_MAX = (int)(1U << 31U)
} pfe_ct_hif_rx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_hif_rx_flags_t) == sizeof(uint32_t));

/**
 * @brief	HIF RX packet header
 */
typedef struct __attribute__((packed))
{
	/*	Rx frame flags */
	pfe_ct_hif_rx_flags_t flags;
	/*	Ingress physical interface ID */
	pfe_ct_phy_if_id_t i_phy_if;
	/*	Ingress logical interface ID */
	uint8_t i_log_if;
	/*	Queue */
	uint8_t queue;
	/*	Reserved */
	uint8_t reserved;
	/*	RX timestamp */
	uint32_t rx_timestamp_ns;
	uint32_t rx_timestamp_s;
} pfe_ct_hif_rx_hdr_t;

ct_assert(sizeof(pfe_ct_hif_rx_hdr_t) == 16U);

typedef enum __attribute__((packed))
{
	/*	No flag being set */
	HIF_TX_NO_FLAG = 0U,
	HIF_TX_RESERVED0 = (1U << 0U),
	/*	ICMP checksum offload. If set then PFE will calculate and
		insert ICMP header checksum. */
	HIF_TX_ICMP_CSUM = (1U << 1U),    
	/*	Generate egress timestamp */
	HIF_TX_ETS = (1U << 2U),
	/*	IP checksum offload. If set then PFE will calculate and
		insert IP header checksum. */
	HIF_TX_IP_CSUM = (1U << 3U),
	/*	TCP checksum offload. If set then PFE will calculate and
		insert TCP header checksum. */
	HIF_TX_TCP_CSUM = (1U << 4U),
	/*	UDP checksum offload. If set then PFE will calculate and
		insert UDP header checksum. */
	HIF_TX_UDP_CSUM = (1U << 5U),
	/*	Transmit Inject Flag. If not set the packet will be processed
		according to Physical Interface configuration. If set then
		e_phy_ifs is valid and packet will be transmitted via physical
		interfaces given by the list. */
	HIF_TX_INJECT = (1U << 6U),
	/*	Inter-HIF communication frame */
	HIF_TX_IHC = (1U << 7U)

} pfe_ct_hif_tx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_hif_tx_flags_t) == sizeof(uint8_t));

/**
 * @brief	HIF TX packet header
 */
typedef struct __attribute__((packed))
{
	/*	TX flags */
	pfe_ct_hif_tx_flags_t flags;
	/*	Queue number within TMU to be used for packet transmission. Default value
		is zero but can be changed according to current QoS feature configuration. */
	uint8_t queue;
	/*	Source HIF channel ID. To be used to identify HIF channel being originator
		of the frame (0, 1, 2, ...). */
	uint8_t chid;
	/*	Reserved */
	uint8_t reserved1;
	uint16_t reserved2;
	/*	Reference number to match transmitted frame and related egress timestamp
	 	report. Up-most 4 bits must stay 0s. */
	uint16_t refnum;
	/*	List of egress physical interfaces to be used for injection. Bit positions
		correspond to pfe_ct_phy_if_id_t values (1U << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	HIF cookie. Arbitrary 32-bit value to be passed to classifier. Can be used
		as matching rule within logical interface. See pfe_ct_if_m_rules_t. */
	uint32_t cookie;
} pfe_ct_hif_tx_hdr_t;

ct_assert(sizeof(pfe_ct_hif_tx_hdr_t) == 16U);

/*	We expect given pfe_ct_hif_tx_hdr_t size due to byte order compatibility. */
ct_assert(0U == (sizeof(pfe_ct_hif_tx_hdr_t) % sizeof(uint32_t)));

/**
 * @brief	Egress Timestamp Report
 */
typedef struct __attribute__((packed))
{
	uint8_t reserved[3U];
	uint8_t ctrl;
	uint32_t reserved1;
	uint32_t ts_nsec;
	uint32_t ts_sec;
	uint8_t reserved2;
	uint8_t i_phy_if;
	uint16_t ref_num;
} pfe_ct_ets_report_t;

/**
 * @brief	Post-classification header
 */
typedef struct __attribute__((packed))
{
	uint8_t reserved[16U];
} pfe_ct_post_cls_hdr_t;

/**
 * @brief	Routing actions
 * @details	When packet is routed an action or actions can be assigned to be executed
 * 			during the routing process. This can be used to configure the router to do
 *			NAT, update TTL, or insert	VLAN header.
 */
typedef enum __attribute__((packed))
{
	RT_ACT_NONE = 0U,						/*!< No action set */
	RT_ACT_ADD_ETH_HDR = (1U << 0U),		/*!< Construct/Update Ethernet Header */
	RT_ACT_ADD_VLAN_HDR = (1U << 1U),		/*!< Construct/Update outer VLAN Header */
	RT_ACT_ADD_PPPOE_HDR = (1U << 2U),		/*!< Construct/Update PPPOE Header */
	RT_ACT_DEC_TTL = (1U << 7U),			/*!< Decrement TTL */
	RT_ACT_ADD_VLAN1_HDR = (1U << 11U),		/*!< Construct/Update inner VLAN Header */
	RT_ACT_CHANGE_SIP_ADDR = (1U << 17U),	/*!< Change Source IP Address */
	RT_ACT_CHANGE_SPORT = (1U << 18U),		/*!< Change Source Port */
	RT_ACT_CHANGE_DIP_ADDR = (1U << 19U),	/*!< Change Destination IP Address */
	RT_ACT_CHANGE_DPORT = (1U << 20U),		/*!< Change Destination Port */
	RT_ACT_DEL_VLAN_HDR = (1U << 21U),		/*!< Delete outer VLAN Header */
	RT_ACT_MOD_VLAN_HDR = (1U << 22U),		/*!< Modify outer VLAN Header */
	RT_ACT_INVALID = (int)(1U << 31U)		/*!< Invalid value */
}pfe_ct_route_actions_t;

/*	We expect given pfe_ct_route_actions_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_route_actions_t) == sizeof(uint32_t));

/**
 * @brief	Arguments for routing actions
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*	Source MAC address (RT_ACT_ADD_ETH_HDR) */
	uint8_t smac[6U];
	/*	Destination MAC address (RT_ACT_ADD_ETH_HDR) */
	uint8_t dmac[6U];
	/*	PPPOE session ID (RT_ACT_ADD_PPPOE_HDR) */
	uint16_t pppoe_sid;
	/*	VLAN ID (RT_ACT_ADD_VLAN_HDR) */
	uint16_t vlan;
	/*	L4 source port number (RT_ACT_CHANGE_SPORT) */
	uint16_t sport;
	/*	L4 destination port number (RT_ACT_CHANGE_DPORT) */
	uint16_t dport;
	/*	Source and destination IPv4 and IPv6 addresses
		(RT_ACT_CHANGE_SIP_ADDR, RT_ACT_CHANGE_DIP_ADDR) */
	pfe_ct_ip_addresses_t ipv;
	/*	Inner VLAN ID (RT_ACT_ADD_VLAN1_HDR) */
	uint16_t vlan1;
	/*	Egress vlan index in stats table (RT_ACT_ADD_VLAN_HDR) */
	uint16_t vlan_stats_index;
	uint32_t sa;
} pfe_ct_route_actions_args_t;

/**
* @brief Configures mirroring 
*/
struct __attribute__((packed, aligned(4))) pfe_ct_mirror_tag
{
	PFE_PTR(pfe_ct_fp_table_t) flexible_filter;	/* Only accepted frames are mirrored if pointer is set */
	pfe_ct_route_actions_t actions;				/* Action to be done on mirrored frames */
	pfe_ct_route_actions_args_t args;			/* Arguments for modification actions */
	pfe_ct_phy_if_id_t e_phy_if;				/* Destination for mirrored frames (outbound interface) */
};

/**
 * @brief	Routing table entry flags
 */
typedef enum __attribute__((packed))
{
	RT_FL_NONE = 0U,				/*!< No flag set */
	RT_FL_VALID = (1U << 0U),		/*!< Entry is valid */
	RT_FL_IPV6 = (1U << 1U),		/*!< If set entry is IPv6 else it is IPv4 */
	RT_FL_MAX = (int)(1U << 31U)	/* Ensure proper size */
} pfe_ct_rtable_flags_t;

/*	We expect given pfe_ct_rtable_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_rtable_flags_t) == sizeof(uint32_t));

/**
 * @brief	Routing table entry status flags
 */
typedef enum __attribute__((packed))
{
	RT_STATUS_NONE = 0U,			/*!< No bit set */
	RT_STATUS_ACTIVE = (1U << 0U)	/*!< If set, entry has been matched
								     by routing table lookup algorithm */
} pfe_rtable_entry_status_t;

/*	We expect given pfe_rtable_entry_status_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_rtable_entry_status_t) == sizeof(uint8_t));

/**
 * @brief	The physical routing table entry structure
 * @details	This structure is shared between firmware and the driver. It represents
 * 			the routing table entry as it is stored in the memory. In case the QB-RFETCH
 * 			routing table lookup is enabled (see classifier configuration) then the format
 *			of leading 6*8 bytes of the routing table entry is given by PFE HW and shall
 *			not be modified as well as size of the entry should be 128 bytes. In case the
 *			lookup is done by classifier PE (firmware) the format and length can be adjusted
 *			according to application needs.
 *
 * @warning	Do not modify this structure unless synchronization with firmware is ensured.
 */
typedef struct __attribute__((packed, aligned(4))) pfe_ct_rtable_entry_tag
{
	/*	Pointer to next entry in a hash bucket */
	PFE_PTR(struct pfe_ct_rtable_entry_tag) next;
	/*	Flags */
	pfe_ct_rtable_flags_t flags;
	/*	L4 source port number */
	uint16_t sport;
	/*	L4 destination port number */
	uint16_t dport;
	/*	IP protocol number */
	uint8_t proto;
	/*	Ingress physical interface ID */
	pfe_ct_phy_if_id_t i_phy_if;
	/*	Hash storage */
	uint16_t hash;
	/*	Source and destination IP addresses */
	pfe_ct_ip_addresses_t ipv;

	/*	---------- 6x8 byte boundary ---------- */

	/*	*/
	/*	Information updated by the Classifier */
	pfe_rtable_entry_status_t status;
	uint8_t entry_state;
	/*	Egress physical interface ID */
	pfe_ct_phy_if_id_t e_phy_if;
	/*	IPv6 flag */
	uint8_t flag_ipv6;
	/*	Routing actions */
	pfe_ct_route_actions_t actions;
	pfe_ct_route_actions_args_t args;
	/*	General purpose storage */
	uint32_t id5t; /* 5-tuple identifier for the IPsec */
	uint16_t conntrack_stats_index; /*index in the stats table*/
	uint16_t dummy;
	uint32_t rt_orig;
} pfe_ct_rtable_entry_t;

/*	We expect given pfe_ct_rtable_entry_t size due to HW requirement. */
ct_assert(sizeof(pfe_ct_rtable_entry_t) == 128U);

#endif /* HW_S32G_PFE_CT_H_ */
/** @} */

