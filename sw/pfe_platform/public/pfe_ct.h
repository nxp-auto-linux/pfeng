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
	PFE_PHY_IF_ID_EMAC0 = 0,
	PFE_PHY_IF_ID_EMAC1 = 1,
	PFE_PHY_IF_ID_EMAC2 = 2,
	PFE_PHY_IF_ID_HIF = 3,
	PFE_PHY_IF_ID_HIF_NOCPY = 4,
	/*	UTIL PE - FW internal use */
	PFE_PHY_IF_ID_UTIL = 5,
	/*	Synthetic interfaces */
	PFE_PHY_IF_ID_HIF0 = 6,
	PFE_PHY_IF_ID_HIF1 = 7,
	PFE_PHY_IF_ID_HIF2 = 8,
	PFE_PHY_IF_ID_HIF3 = 9,
	/*	Internals */
	PFE_PHY_IF_ID_MAX = PFE_PHY_IF_ID_HIF3,
	PFE_PHY_IF_ID_INVALID
} pfe_ct_phy_if_id_t;

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
	IF_MATCH_TYPE_ETH = (1U << 0),		/*!< Match ETH Packets */
	IF_MATCH_TYPE_VLAN = (1U << 1),		/*!< Match VLAN Tagged Packets */
	IF_MATCH_TYPE_PPPOE = (1U << 2),		/*!< Match PPPoE Packets */
	IF_MATCH_TYPE_ARP = (1U << 3),		/*!< Match ARP Packets */
	IF_MATCH_TYPE_MCAST = (1U << 4),		/*!< Match Multicast (L2) Packets */
	IF_MATCH_TYPE_IPV4 = (1U << 5),		/*!< Match IPv4 Packets */
	IF_MATCH_TYPE_IPV6 = (1U << 6),		/*!< Match IPv6 Packets */
	IF_MATCH_RESERVED7 = (1U << 7),		/*!< Reserved */
	IF_MATCH_RESERVED8 = (1U << 8),		/*!< Reserved */
	IF_MATCH_TYPE_IPX = (1U << 9),		/*!< Match IPX Packets */
	IF_MATCH_TYPE_BCAST = (1U << 10),	/*!< Match Broadcast (L2) Packets */
	IF_MATCH_TYPE_UDP = (1U << 11),		/*!< Match UDP Packets */
	IF_MATCH_TYPE_TCP = (1U << 12),		/*!< Match TCP Packets */
	IF_MATCH_TYPE_ICMP = (1U << 13),		/*!< Match ICMP Packets */
	IF_MATCH_TYPE_IGMP = (1U << 14),		/*!< Match IGMP Packets */
	IF_MATCH_VLAN = (1U << 15),			/*!< Match VLAN ID */
	IF_MATCH_PROTO = (1U << 16),			/*!< Match IP Protocol */
	IF_MATCH_SPORT = (1U << 20),			/*!< Match L4 Source Port */
	IF_MATCH_DPORT = (1U << 21),			/*!< Match L4 Destination Port */
	/* Pure SW */
	IF_MATCH_SIP6 = (1U << 22),			/*!< Match Source IPv6 Address */
	IF_MATCH_DIP6 = (1U << 23),			/*!< Match Destination IPv6 Address */
	IF_MATCH_SIP = (1U << 24),			/*!< Match Source IPv4 Address */
	IF_MATCH_DIP = (1U << 25),			/*!< Match Destination IPv4 Address */
	IF_MATCH_ETHTYPE = (1U << 26),		/*!< Match EtherType */
	IF_MATCH_FP0 = (1U << 27),			/*!< Match Packets Accepted by Flexible Parser 0 */
	IF_MATCH_FP1 = (1U << 28),			/*!< Match Packets Accepted by Flexible Parser 1 */
	IF_MATCH_SMAC = (1U << 29),			/*!< Match Source MAC Address */
	IF_MATCH_DMAC = (1U << 30),			/*!< Match Destination MAC Address */
	IF_MATCH_HIF_COOKIE = (1U << 31),	/*!< Match HIF header cookie value */
	/* Ensure proper size */
	IF_MATCH_MAX = (1U << 31)
} pfe_ct_if_m_rules_t;

/*	We expect given pfe_ct_if_m_rules_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_m_rules_t) == sizeof(uint32_t));

typedef enum __attribute__((packed))
{
	/* Invert match result */
	FP_FL_INVERT = (1U << 0),
	/* Reject packet in case of match */
	FP_FL_REJECT = (1U << 1),
	/* Accept packet in case of match */
	FP_FL_ACCEPT = (1U << 2),
	/* Data offset is relative from start of L3 header */
	FP_FL_L3_OFFSET = (1U << 3),
	/* Data offset is relative from start of L4 header */
	FP_FL_L4_OFFSET = (1U << 4),
	/* Just to ensure correct size */
	FP_FL_MAX = (1U << 7)
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

/**
 * @brief The Flexible Parser table
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Number of rules in the table */
	uint8_t count;
	/* Reserved (to keep "rules" aligned ) */
	uint8_t reserved8;
	uint16_t reserved16;
	/* Pointer to the array of "count" rules */
	PFE_PTR (pfe_ct_fp_rule_t) rules;
} pfe_ct_fp_table_t;

ct_assert(sizeof(pfe_ct_fp_table_t) == 8U);

/**
 * @brief	Interface matching rules arguments
 * @details	Argument values needed by particular rules given by pfe_ct_if_m_rules_t.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* VLAN ID (IF_MATCH_VLAN) */
	uint16_t vlan;
	/* EtherType (IF_MATCH_ETHTYPE) */
	uint16_t ethtype;
	/* L4 source port number (IF_MATCH_SPORT) */
	uint16_t sport;
	/* L4 destination port number (IF_MATCH_DPORT) */
	uint16_t dport;
	/* Source and destination addresses */
	struct
	{
		/*	IPv4 (for IF_MATCH_SIP, IF_MATCH_DIP) */
		struct
		{
			uint32_t sip;
			uint32_t dip;
		} v4;

		/*	IPv6 (for IF_MATCH_SIP6, IF_MATCH_DIP6) */
		struct
		{
			uint32_t sip[4];
			uint32_t dip[4];
		} v6;
	};
	/* Flexible Parser 0 table (IF_MATCH_FP0) */
	PFE_PTR(pfe_ct_fp_table_t) fp0_table;
	/* Flexible Parser 1 table (IF_MATCH_FP1) */
	PFE_PTR(pfe_ct_fp_table_t) fp1_table;
	/* HIF header cookie (IF_MATCH_HIF_COOKIE) */
	uint32_t hif_cookie;
	/* Source MAC Address (IF_MATCH_SMAC) */
	uint8_t __attribute__((aligned(4))) smac[6]; /* Must be aligned at 4 bytes */
	/* IP protocol (IF_MATCH_PROTO) */
	uint8_t proto;
	/* Reserved */
	uint8_t reserved;
	/* Destination MAC Address (IF_MATCH_DMAC) */
	uint8_t __attribute__((aligned(4))) dmac[6]; /* Must be aligned at 4 bytes */
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
	IF_OP_DISABLED = 0,		/*!< Disabled */
	IF_OP_DEFAULT = 1,		/*!< Default operational mode */
	IF_OP_BRIDGE = 2,		/*!< L2 bridge */
	IF_OP_ROUTER = 3,		/*!< L3 router */
	IF_OP_VLAN_BRIDGE = 4,	/*!< L2 bridge with VLAN */
	IF_OP_FLEX_ROUTER = 5	/*!< Flexible router */
} pfe_ct_if_op_mode_t;

/*	We expect given pfe_ct_if_op_mode_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_op_mode_t) == sizeof(uint8_t));

typedef enum __attribute__((packed))
{
	IF_FL_ENABLED = (1U << 0),		/*!< If set, interface is enabled */
	IF_FL_PROMISC = (1U << 1),		/*!< If set, interface is promiscuous */
	IF_FL_FF_ALL_TCP = (1U << 2),	/*!< Enable fast-forwarding of ingress TCP SYN|FIN|RST packets */
	IF_FL_MATCH_OR = (1U << 3),		/*!< Result of match is logical OR of rules, else AND */
	IF_FL_DISCARD = (1U << 4)		/*!< Discard packets on rules match */
} pfe_ct_if_flags_t;

/*	We expect given pfe_ct_if_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_flags_t) == sizeof(uint8_t));

/**
 * @brief	Acceptable frame types
 */
typedef enum  __attribute__((packed))
{
	IF_AFT_ANY_TAGGING = 0,
	IF_AFT_TAGGED_ONLY = 1,
	IF_AFT_UNTAGGED_ONLY = 2
} pfe_ct_if_aft_t;

/*	We expect given pfe_ct_if_aft_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_if_aft_t) == sizeof(uint8_t));

/**
 * @brief	Interface blocking state
 */
typedef enum __attribute__((packed))
{
	IF_BS_FORWARDING = 0,	/*!< Learning and forwarding enabled */
	IF_BS_BLOCKED = 1,		/*!< Learning and forwarding disabled */
	IF_BS_LEARN_ONLY = 2,	/*!< Learning enabled, forwarding disabled */
	IF_BS_FORWARD_ONLY = 3	/*!< Learning disabled, forwarding enabled */
} pfe_ct_block_state_t;

/*	We expect given pfe_ct_block_state_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_block_state_t) == sizeof(uint8_t));

/**
 * @brief	The logical interface structure as seen by firmware
 * @details	This structure is shared between firmware and the driver. It represents
 * 			the logical interface as it is stored in the DMEM.
 * @warning	Do not modify this structure unless synchronization with firmware is ensured.
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_log_if_tag
{
	/*	Pointer to next logical interface in the list (DMEM) */
	PFE_PTR(struct __pfe_ct_log_if_tag) next;
	/*	List of egress physical interfaces. Bit positions correspond
		to pfe_ct_phy_if_id_t values (1U << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	Arguments required by matching rules */
	pfe_ct_if_m_args_t __attribute__((aligned(4))) m_args; /* Must be aligned at 4 bytes */
	/*	Interface identifier */
	uint8_t id;
	/*	Operational mode */
	pfe_ct_if_op_mode_t mode;
	/*	Flags */
	pfe_ct_if_flags_t flags;
	/*	Match rules. Zero means that matching is disabled and packets
		can be accepted on interface in promiscuous mode only. */
	pfe_ct_if_m_rules_t m_rules;
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
	union
	{
		struct
		{
			uint32_t sip;
			uint32_t dip;
		} v4;

		struct
		{
			uint32_t sip[4];
			uint32_t dip[4];
		} v6;
	} u;
	uint32_t id5t;	/* 5-tuple ID to speed search, 0 = invalid ID */
	uint32_t spi;	/* SPI value to match - only for action SPD_ACT_PROCESS_DECODE */
	/* --- Action --- */
	uint32_t sad_entry; /* How to process IPsec */
	pfe_ct_spd_entry_action_t action; /* What to do on match */
	uint8_t pad8[3];
} pfe_ct_spd_entry_t;

typedef struct __attribute__((packed, aligned(4)))
{
	uint32_t entry_count;					/* Count of the entries in the database */
	pfe_ct_spd_entry_action_t no_ip_action;	/* Non-ip traffic action - may not be SPD_ACT_PROCESS */
	uint8_t pad[3U];						/* Align to 4 bytes */
	PFE_PTR(pfe_ct_spd_entry_t) entries;	/* Database entries */
} pfe_ct_ipsec_spd_t;

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
	/*	Physical port number */
	pfe_ct_phy_if_id_t id;
	/*	Operational mode */
	pfe_ct_if_op_mode_t mode;
	/*	Flags */
	pfe_ct_if_flags_t flags;
	/*	Block state */
	pfe_ct_block_state_t block_state;
	/*	Mirroring to given port */
	pfe_ct_phy_if_id_t mirror;
	/*	Reserved */
	uint8_t reserved[3];
	/*	SPD for IPsec */
	PFE_PTR(pfe_ct_ipsec_spd_t) ipsec_spd;
	/*	Gathered statistics */
	pfe_ct_phy_if_stats_t __attribute__((aligned(4))) phy_stats; /* Must be aligned to 4 bytes */
} pfe_ct_phy_if_t;

/**
 * @brief	L2 Bridge Actions
 */
typedef enum __attribute__((packed))
{
	L2BR_ACT_FORWARD = 0,   /*!< Forward normally */
	L2BR_ACT_FLOOD = 1,     /*!< Flood */
	L2BR_ACT_PUNT = 2,      /*!< Punt */
	L2BR_ACT_DISCARD = 3,   /*!< Discard */
} pfe_ct_l2br_action_t;

#if PFE_COMPILER_BITFIELD_BEHAVIOR == PFE_COMPILER_BITFIELD_HIGH_LAST
/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef struct __attribute__((packed))
{
	union
	{
		struct
		{
			/* [19:0] Forward port list */
			uint32_t forward_list : 20;
			/* [28:20] Reserved */
			uint32_t reserved : 9;
			/* [29] Fresh */
			uint32_t fresh_flag : 1;
			/* [30] Static */
			uint32_t static_flag : 1;
			/* [31] Reserved */
			uint32_t reserved1 : 1;
		};

		uint32_t val : 32;
	};
} pfe_ct_mac_table_result_t;

/**
 * @brief	VLAN table lookup result (64-bit)
 */
typedef struct __attribute__((packed))
{
	union
	{
		struct
		{
			/*	[19:0]  Forward list (1U << pfe_ct_phy_if_id_t) */
			uint64_t forward_list : 20;
			/*	[39:20] Untag list (1U << pfe_ct_phy_if_id_t) */
			uint64_t untag_list : 20;
			/*	[42:40] Unicast hit action (pfe_ct_l2br_action_t) */
			uint64_t ucast_hit_action : 3;
			/*	[45:43] Multicast hit action (pfe_ct_l2br_action_t) */
			uint64_t mcast_hit_action : 3;
			/*	[48:46] Unicast miss action (pfe_ct_l2br_action_t) */
			uint64_t ucast_miss_action : 3;
			/*	[51:49] Multicast miss action (pfe_ct_l2br_action_t) */
			uint64_t mcast_miss_action : 3;
			/*	[54:52] Reserved */
			uint64_t reserved : 3;
			/*	[55 : 63] */ /* Reserved */
			uint64_t  hw_reserved : 9;
		};

		uint64_t val;
	};
} pfe_ct_vlan_table_result_t;
#elif PFE_COMPILER_BITFIELD_BEHAVIOR == PFE_COMPILER_BITFIELD_HIGH_FIRST
/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef struct __attribute__((packed))
{
	union
	{
		struct
		{
			/* [31] Reserved */
			uint32_t reserved1 : 1;
			/* [30] Static */
			uint32_t static_flag : 1;
			/* [29] Fresh */
			uint32_t fresh_flag : 1;
			/* [28:20] Reserved */
			uint32_t reserved : 9;
			/* [19:0] Forward port list */
			uint32_t forward_list : 20;
		};

		uint32_t val : 32;
	};
} pfe_ct_mac_table_result_t;

/**
 * @brief	VLAN table lookup result (64-bit)
 */
typedef struct __attribute__((packed))
{
	union
	{
		struct
		{
			/*	[55 : 63] */ /* Reserved */
			uint64_t  hw_reserved : 9;
			/*	[54:52] Reserved */
			uint64_t reserved : 3;
			/*	[51:49] Multicast miss action (pfe_ct_l2br_action_t) */
			uint64_t mcast_miss_action : 3;
			/*	[48:46] Unicast miss action (pfe_ct_l2br_action_t) */
			uint64_t ucast_miss_action : 3;
			/*	[45:43] Multicast hit action (pfe_ct_l2br_action_t) */
			uint64_t mcast_hit_action : 3;
			/*	[42:40] Unicast hit action (pfe_ct_l2br_action_t) */
			uint64_t ucast_hit_action : 3;
			/*	[39:20] Untag list (1U << pfe_ct_phy_if_id_t) */
			uint64_t untag_list : 20;  /* List of ports to remove VLAN tag */
			/*	[19:0]  Forward list (1U << pfe_ct_phy_if_id_t) */
			uint64_t forward_list : 20;
		};

		uint64_t val;
	};
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
typedef uint8_t pfe_date_str_t[16];

/*	We expect given pfe_date_str_t size. */
ct_assert(sizeof(pfe_date_str_t) > sizeof(__DATE__));

/**
 * @brief Time string type
 */
typedef uint8_t pfe_time_str_t[16];

/*	We expect given pfe_time_str_t size. */
ct_assert(sizeof(pfe_time_str_t) > sizeof(__TIME__));

/**
 * @brief Version control identifier string type
 */
typedef uint8_t pfe_vctrl_str_t[16];

/**
 * @brief This header version MD5 checksum string type
 */
typedef char_t pfe_cthdr_str_t[36]; /* 32 Characters + NULL + 3 padding */

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
	uint8_t res;
	/*	Firmware properties */
	uint32_t flags;
	/*	Build date and time */
	pfe_date_str_t build_date;
	pfe_time_str_t build_time;
	/*	Version control ID (e.g. GIT commit) */
	pfe_vctrl_str_t vctrl;
	/*  This header version */
	pfe_cthdr_str_t cthdr;
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
 * @brief Statistics gathered for each classification algorithm
 * @details NULL pointer means that given statistics are no available
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Statistics gathered by Flexible Router algorithm */
	pfe_ct_class_algo_stats_t flexible_router;
	/* Statistics gathered by IP router algorithm (IF_OP_ROUTER) */
	pfe_ct_class_algo_stats_t ip_router;
	/* Statistics gathered by L2 bridge algorithm (IF_OP_BRIDGE) */
	pfe_ct_class_algo_stats_t l2_bridge;
	/* Statistics gathered by VLAN bridge algorithm (IF_OP_VLAN_BRIDGE) */
	pfe_ct_class_algo_stats_t vlan_bridge;
	/* Statistics gathered by logical interface matching algorithm (IF_OP_DEFAULT) */
	pfe_ct_class_algo_stats_t log_if;
	/* Statistics gathered when hif-to-hif classification is done */
	pfe_ct_class_algo_stats_t hif_to_hif;
} pfe_ct_classify_stats_t;

/**
 * @brief Number of FW error reports which can be stored in pfe_ct_error_record_t
 * @details The value must be power of 2.
 */
#define FP_ERROR_RECORD_SIZE 64

/**
 * @brief Reported error storage
 * @note Instances of this structure are stored in an elf-file section .errors which
 *       is not loaded into any memory and the driver accesses it only through the
 *       elf-file.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Error description - string in .errors section */
	PFE_PTR(const char_t)message;
	/* File name where error occurred - string in .errors section */
	PFE_PTR(const char_t)file;
	/* Line where error occurred */
	const uint32_t line;
} pfe_ct_error_t;

/**
 * @brief Storage for runtime errors
 * @note The pointers cannot be dereferenced because the .errors section is not loaded
 *       into memory and the elf-file parsing is needed to translate them.
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/* Next position to write: (write_index & (FP_ERROR_RECORD_SIZE - 1)) */
	uint32_t write_index;
	/* Stored errors - pointers point to section .errors which is not
	    part of any memory, just the elf-file */
	PFE_PTR(const pfe_ct_error_t) errors[FP_ERROR_RECORD_SIZE];
	uint32_t values[FP_ERROR_RECORD_SIZE];
} pfe_ct_error_record_t;

/**
 * @brief The firmware internal state
 */
typedef enum __attribute__((packed))
{
	PFE_FW_STATE_UNINIT = 0,        /* FW not started */
	PFE_FW_STATE_INIT,              /* FW passed initialization */
	PFE_FW_STATE_FRAMEWAIT,         /* FW waiting for a new frame arrival */
	PFE_FW_STATE_FRAMEPARSE,        /* FW started parsing a new frame */
	PFE_FW_STATE_FRAMECLASSIFY,     /* FW started classification of parsed frame */
	PFE_FW_STATE_FRAMEDISCARD,      /* FW is discarding the frame */
	PFE_FW_STATE_FRAMEMODIFY,       /* FW is modifying the frame */
	PFE_FW_STATE_FRAMESEND,         /* FW is sending frame out (towards EMAC or HIF) */
	PFE_FW_STATE_STOPPED,           /* FW was gracefully stopped by external request */
	PFE_FW_STATE_EXCEPTION,         /* FW is stopped after an exception */
	PFE_FW_STATE_FAIL_STOP          /* FW is stopped due to a safety fault */
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
	uint8_t reserved[3];            /* To make size multiple of 4 bytes */
} pfe_ct_pe_sw_state_monitor_t;

ct_assert(sizeof(pfe_ct_pe_sw_state_monitor_t) == 8);


/**
 * @brief Storage for measured time intervals used during firmware performance monitoring
 */
typedef struct __attribute__((packed, aligned(4)))
{
	uint32_t min; /* Minimal measured value */
	uint32_t max; /* Maximal measured value */
	uint32_t avg; /* Average of measured values */
	uint32_t cnt; /* Count of measurements */
} pfe_ct_measurement_t;


/**
 * @brief Configuration of flexible filter
 * @details Value 0 (NULL) means disabled filter, any other value is a pointer to the
 *          flexible parser table to be used as filter. Frames rejected by the filter
 *          are discarded.
 */
typedef PFE_PTR(pfe_ct_fp_table_t) pfe_ct_flexible_filter_t;
ct_assert(sizeof(pfe_ct_flexible_filter_t) == sizeof(uint32_t));

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
 * @brief PE memory map representation type shared between host and PFE
 */
typedef struct __attribute__((packed, aligned(4)))
{
	/*	Size of the structure in number of bytes */
	uint32_t size;
	/*	Version information */
	pfe_ct_version_t version;
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
	/*	Misc. control  */
	PFE_PTR(pfe_ct_pe_misc_control_t) pe_misc_control;
	/*	Statistics provided for the PE (by the firmware) */
	PFE_PTR(pfe_ct_pe_stats_t) pe_stats;
	/*	Statistics provided for each classification algorithm */
	PFE_PTR(pfe_ct_classify_stats_t) classification_stats;
	/*	Errors reported by the FW */
	PFE_PTR(pfe_ct_error_record_t) error_record;
	/*	FW state */
	PFE_PTR(pfe_ct_pe_sw_state_monitor_t) state_monitor;
	/*	Count of the measurement storages - 0 = feature not enabled */
	uint32_t measurement_count;
	/*	Performance measurement storages - NULL = none (feature not enabled) */
	PFE_PTR(pfe_ct_measurement_t) measurements;
	/*	Flexible Filter */
	PFE_PTR(pfe_ct_flexible_filter_t) flexible_filter;
	/*	PE ID */
	PFE_PTR(uint8_t) pe_id;
	/*	Put buffer: FW-to-SW data transfers */
	PFE_PTR(pfe_ct_buffer_t) put_buffer;
	/*	Get buffer: SW-to-FW data transfers */
	PFE_PTR(pfe_ct_buffer_t) get_buffer;
} pfe_ct_pe_mmap_t;

typedef enum __attribute__((packed))
{
	/*	Punt by snooping feature */
	PUNT_SNOOP = (1U << 0),
	/*	Ensure proper size */
	PUNT_MAX = (1U << 15)
} pfe_ct_punt_reasons_t;

/*	We expect given pfe_ct_punt_reasons_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_punt_reasons_t) == sizeof(uint16_t));

typedef enum __attribute__((packed))
{
	/*	No flag being set */
	HIF_RX_NO_FLAG = 0,
	/*	IP checksum valid */
	HIF_RX_IPV4_CSUM = (1U << 0),
	/*	TCP of IPv4 checksum valid */
	HIF_RX_TCPV4_CSUM = (1U << 1),
	/*	TCP of IPv6 checksum valid */
	HIF_RX_TCPV6_CSUM = (1U << 2),
	/*	UDP of IPv4 checksum valid */
	HIF_RX_UDPV4_CSUM = (1U << 3),
	/*	UDP of IPv6 checksum valid */
	HIF_RX_UDPV6_CSUM = (1U << 4),
	/*	PTP packet */
	HIF_RX_PTP = (1U << 5),
	/*	Punt flag. If set then punt reason is valid. */
	HIF_RX_PUNT = (1U << 6),
	/*	Timestamp flag. When set, timestamp is valid. */
	HIF_RX_TS = (1U << 7),
	/*	Inter - HIF communication frame */
	HIF_RX_IHC = (1U << 8),
	/*	Frame is Egress Timestamp Report */
	HIF_RX_ETS = (1U << 9)
} pfe_ct_hif_rx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_hif_rx_flags_t) == sizeof(uint16_t));

/**
 * @brief	HIF RX packet header
 */
typedef struct __attribute__((packed))
{
	/*	Punt reason flags */
	pfe_ct_punt_reasons_t punt_reasons;
	/*	Ingress physical interface ID */
	pfe_ct_phy_if_id_t i_phy_if;
	/*	Ingress logical interface ID */
	uint8_t i_log_if;
	/*	Rx frame flags */
	pfe_ct_hif_rx_flags_t flags;
	/*	Queue */
	uint8_t queue;
	/*	Reserved */
	uint8_t reserved;
	/*	RX timestamp */
	uint32_t rx_timestamp_ns;
	uint32_t rx_timestamp_s;
} pfe_ct_hif_rx_hdr_t;

ct_assert(sizeof(pfe_ct_hif_rx_hdr_t) == 16);

typedef enum __attribute__((packed))
{
	/*	No flag being set */
	HIF_TX_NO_FLAG = 0,
	HIF_TX_RESERVED0 = (1U << 0),
	HIF_TX_RESERVED1 = (1U << 1),
	/*	Generate egress timestamp */
	HIF_TX_ETS = (1U << 2),
	/*	IP checksum offload. If set then PFE will calculate and
		insert IP header checksum. */
	HIF_TX_IP_CSUM = (1U << 3),
	/*	TCP checksum offload. If set then PFE will calculate and
		insert TCP header checksum. */
	HIF_TX_TCP_CSUM = (1U << 4),
	/*	UDP checksum offload. If set then PFE will calculate and
		insert UDP header checksum. */
	HIF_TX_UDP_CSUM = (1U << 5),
	/*	Transmit Inject Flag. If not set the packet will be processed
		according to Physical Interface configuration. If set then
		e_phy_ifs is valid and packet will be transmitted via physical
		interfaces given by the list. */
	HIF_TX_INJECT = (1U << 6),
	/*	Inter-HIF communication frame */
	HIF_TX_IHC = (1U << 7)
} pfe_ct_hif_tx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_hif_tx_flags_t) == sizeof(uint8_t));

/**
 * @brief	HIF TX packet header
 */
typedef struct __attribute__((aligned))
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
	 	report. */
	uint16_t refnum;
	/*	List of egress physical interfaces to be used for injection. Bit positions
		correspond to pfe_ct_phy_if_id_t values (1U << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	HIF cookie. Arbitrary 32-bit value to be passed to classifier. Can be used
		as matching rule within logical interface. See pfe_ct_if_m_rules_t. */
	uint32_t cookie;
} pfe_ct_hif_tx_hdr_t;

ct_assert(sizeof(pfe_ct_hif_tx_hdr_t) == 16);

/*	We expect given pfe_ct_hif_tx_hdr_t size due to byte order compatibility. */
ct_assert(0 == (sizeof(pfe_ct_hif_tx_hdr_t) % sizeof(uint32_t)));

/**
 * @brief	Egress Timestamp Report
 */
typedef struct __attribute__((packed))
{
	uint8_t reserved[3];
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
	uint8_t reserved[24];
} pfe_ct_post_cls_hdr_t;

/**
 * @brief	Routing actions
 * @details	When packet is routed an action or actions can be assigned to be executed
 * 			during the routing process. This can be used to configure the router to do
 *			NAT, update TTL, or insert	VLAN header.
 */
typedef enum __attribute__((packed))
{
	RT_ACT_ADD_ETH_HDR = (1U << 0),		/*!< Construct/Update Ethernet Header */
	RT_ACT_ADD_VLAN_HDR = (1U << 1),		/*!< Construct/Update outer VLAN Header */
	RT_ACT_ADD_PPPOE_HDR = (1U << 2),	/*!< Construct/Update PPPOE Header */
	RT_ACT_DEC_TTL = (1U << 7),			/*!< Decrement TTL */
	RT_ACT_ADD_VLAN1_HDR = (1U << 11),	/*!< Construct/Update inner VLAN Header */
	RT_ACT_CHANGE_SIP_ADDR = (1U << 17),	/*!< Change Source IP Address */
	RT_ACT_CHANGE_SPORT = (1U << 18),	/*!< Change Source Port */
	RT_ACT_CHANGE_DIP_ADDR = (1U << 19),	/*!< Change Destination IP Address */
	RT_ACT_CHANGE_DPORT = (1U << 20),	/*!< Change Destination Port */
	RT_ACT_DEL_VLAN_HDR = (1U << 21),	/*!< Delete outer VLAN Header */
	RT_ACT_INVALID = (1U << 31)			/*!< Invalid value */
} pfe_ct_route_actions_t;

/*	We expect given pfe_ct_route_actions_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_route_actions_t) == sizeof(uint32_t));

/**
 * @brief	Arguments for routing actions
 */
typedef struct __attribute__((packed))
{
	/*	Source MAC address (RT_ACT_ADD_ETH_HDR) */
	uint8_t smac[6];
	/*	Destination MAC address (RT_ACT_ADD_ETH_HDR) */
	uint8_t dmac[6];
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
	union
	{
		struct
		{
			uint32_t	sip;
			uint32_t	dip;
		} v4;

		struct
		{
			uint32_t	sip[4];
			uint32_t	dip[4];
		} v6;
	};

	/*	Inner VLAN ID (RT_ACT_ADD_VLAN1_HDR) */
	uint16_t vlan1;
	/*	Reserved */
	uint16_t res;
	uint32_t sa;
} pfe_ct_route_actions_args_t;

/**
 * @brief	Routing table entry flags
 */
typedef enum __attribute__((packed))
{
	RT_FL_VALID = (1U << 0),	/*!< Entry is valid */
	RT_FL_IPV6 = (1U << 1),	/*!< If set entry is IPv6 else it is IPv4 */
	RT_FL_MAX = (1U << 31)	/* Ensure proper size */
} pfe_ct_rtable_flags_t;

/*	We expect given pfe_ct_rtable_flags_t size due to byte order compatibility. */
ct_assert(sizeof(pfe_ct_rtable_flags_t) == sizeof(uint32_t));

/**
 * @brief	Routing table entry status flags
 */
typedef enum __attribute__((packed))
{
	RT_STATUS_ACTIVE = (1U << 0)	/*!< If set, entry has been matched
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_rtable_entry_tag
{
	/*	Pointer to next entry in a hash bucket */
	PFE_PTR(struct __pfe_ct_rtable_entry_tag) next;
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
	union
	{
		struct
		{
			uint32_t	sip;
			uint32_t	dip;
		} v4;

		struct
		{
			uint32_t	sip[4];
			uint32_t	dip[4];
		} v6;
	} u;

	/*	---------- 6x8 byte boundary ---------- */

	/*	*/
	uint8_t entry_state;
	/*	Egress physical interface ID */
	pfe_ct_phy_if_id_t e_phy_if;
	/*	Information updated by the Classifier */
	pfe_rtable_entry_status_t status;
	/*	IPv6 flag */
	uint8_t flag_ipv6;
	/*	Routing actions */
	pfe_ct_route_actions_t actions;
	pfe_ct_route_actions_args_t args;
	/*	General purpose storage */
	uint32_t id5t; /* 5-tuple identifier for the IPsec */
	uint32_t dummy;
	uint32_t rt_orig;
} pfe_ct_rtable_entry_t;

/*	We expect given pfe_ct_rtable_entry_t size due to HW requirement. */
ct_assert(sizeof(pfe_ct_rtable_entry_t) == 128);

#endif /* HW_S32G_PFE_CT_H_ */
/** @} */

