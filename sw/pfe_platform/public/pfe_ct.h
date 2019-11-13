/* =========================================================================
 *  Copyright 2018-2019 NXP
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

#if defined(TARGET_OS_QNX) || defined(TARGET_OS_LINUX) || defined(TARGET_OS_AUTOSAR)
	/* ALL supported drivers should go here */
	#include "oal_types.h"
#else
	/* Firmware way */
	#include "fp_types.h"
#endif

#if defined(TARGET_OS_QNX) || defined(TARGET_OS_LINUX) || defined(TARGET_OS_AUTOSAR)
	/* ALL supported drivers should go here */
	#define PFE_PTR(type)	uint32_t
#else
	/* Firmware way */
	#define PFE_PTR(type)	type *
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
	/*	Synthetic interfaces */
	PFE_PHY_IF_ID_HIF0 = 5,
	PFE_PHY_IF_ID_HIF1 = 6,
	PFE_PHY_IF_ID_HIF2 = 7,
	PFE_PHY_IF_ID_HIF3 = 8,
	/*	Internals */
	PFE_PHY_IF_ID_MAX = PFE_PHY_IF_ID_HIF3,
	PFE_PHY_IF_ID_INVALID
} pfe_ct_phy_if_id_t;

/*	We expect given pfe_ct_phy_if_id_t size due to byte order compatibility. In case this
 	assert fails the respective code must be reviewed and every usage of the type must
 	be treated by byte order swap where necessary (typically any place in host software
	where the values are communicated between firmware and the host). */
_ct_assert(sizeof(pfe_ct_phy_if_id_t) == sizeof(uint8_t));

/**
 * @brief	Interface matching rules
 * @details	This flags can be used to define matching rules for every logical interface. Every
 * 			packet received via physical interface is classified to get associated logical
 * 			interface. The classification can be done on single, or combination of rules.
 */
typedef enum __attribute__((packed))
{
	/* HW Accelerated Rules */
	IF_MATCH_TYPE_ETH = (1 << 0),		/*!< Match ETH Packets */
	IF_MATCH_TYPE_VLAN = (1 << 1),		/*!< Match VLAN Tagged Packets */
	IF_MATCH_TYPE_PPPOE = (1 << 2),		/*!< Match PPPoE Packets */
	IF_MATCH_TYPE_ARP = (1 << 3),		/*!< Match ARP Packets */
	IF_MATCH_TYPE_MCAST = (1 << 4),		/*!< Match Multicast (L2) Packets */
	IF_MATCH_TYPE_IP = (1 << 5),		/*!< Match IP Packets */
	IF_MATCH_TYPE_IPV6 = (1 << 6),		/*!< Match IPv6 Packets */
	IF_MATCH_TYPE_IPV4 = (1 << 7),		/*!< Match IPv4 Packets */
	IF_MATCH_RESERVED = (1 << 8),		/*!< Reserved */
	IF_MATCH_TYPE_IPX = (1 << 9),		/*!< Match IPX Packets */
	IF_MATCH_TYPE_BCAST = (1 << 10),	/*!< Match Broadcast (L2) Packets */
	IF_MATCH_TYPE_UDP = (1 << 11),		/*!< Match UDP Packets */
	IF_MATCH_TYPE_TCP = (1 << 12),		/*!< Match TCP Packets */
	IF_MATCH_TYPE_ICMP = (1 << 13),		/*!< Match ICMP Packets */
	IF_MATCH_TYPE_IGMP = (1 << 14),		/*!< Match IGMP Packets */
	IF_MATCH_VLAN = (1 << 15),			/*!< Match VLAN ID */
	IF_MATCH_PROTO = (1 << 16),			/*!< Match IP Protocol */
	IF_MATCH_SPORT = (1 << 20),			/*!< Match L4 Source Port */
	IF_MATCH_DPORT = (1 << 21),			/*!< Match L4 Destination Port */
	/* Pure SW */
	IF_MATCH_SIP6 = (1 << 22),			/*!< Match Source IPv6 Address */
	IF_MATCH_DIP6 = (1 << 23),			/*!< Match Destination IPv6 Address */
	IF_MATCH_SIP = (1 << 24),			/*!< Match Source IPv4 Address */
	IF_MATCH_DIP = (1 << 25),			/*!< Match Destination IPv4 Address */
	IF_MATCH_ETHTYPE = (1 << 26),		/*!< Match EtherType */
	IF_MATCH_FP0 = (1 << 27),			/*!< Match Packets Accepted by Flexible Parser 0 */
	IF_MATCH_FP1 = (1 << 28),			/*!< Match Packets Accepted by Flexible Parser 1 */
	IF_MATCH_SMAC = (1 << 29),			/*!< Match Source MAC Address */
	IF_MATCH_DMAC = (1 << 30),			/*!< Match Destination MAC Address */
	/* Ensure proper size */
	IF_MATCH_MAX = (1 << 30)
} pfe_ct_if_m_rules_t;

/*	We expect given pfe_ct_if_m_rules_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_if_m_rules_t) == sizeof(uint32_t));

typedef enum __attribute__((packed))
{
	/* Invert match result */
	FP_FL_INVERT = (1 << 0),
	/* Reject packet in case of match */
	FP_FL_REJECT = (1 << 1),
	/* Accept packet in case of match */
	FP_FL_ACCEPT = (1 << 2),
	/* Data offset is relative from start of L3 header */
	FP_FL_L3_OFFSET = (1 << 3),
	/* Data offset is relative from start of L4 header */
	FP_FL_L4_OFFSET = (1 << 4),
	/* Just to ensure correct size */
	FP_FL_MAX = (1 << 7)
} pfe_ct_fp_flags_t;

_ct_assert(sizeof(pfe_ct_fp_flags_t) == sizeof(uint8_t));

/**
 * @brief The Flexible Parser rule
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_fp_rule_tag
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

_ct_assert(sizeof(pfe_ct_fp_rule_t) == 12U);

/**
 * @brief The Flexible Parser table
 */
typedef struct __attribute((packed, aligned(4))) __pfe_ct_fp_table_tag
{
	/* Number of rules in the table */
	uint8_t count;
	/* Reserved (to keep "rules" aligned ) */
	uint8_t reserved8;
	uint16_t reserved16;
	/* Pointer to the array of "count" rules */
	PFE_PTR (pfe_ct_fp_rule_t) rules;
} pfe_ct_fp_table_t;

_ct_assert(sizeof(pfe_ct_fp_table_t) == 8U);

/**
 * @brief	Interface matching rules arguments
 * @details	Argument values needed by particular rules given by pfe_ct_if_m_rules_t.
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_if_m_args_t
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
		/*	IPv4 (IF_MATCH_SIP, IF_MATCH_DIP) */
		struct
		{
			uint32_t sip;
			uint32_t dip;
		} v4;

		/*	IPv6 (IF_MATCH_SIP6, IF_MATCH_DIP6) */
		struct
		{
			uint32_t sip[4];
			uint32_t dip[4];
		} v6;
	};
#if 0 /*Waiting for implementation of Flexible parser (AAVB-1899 will add these)*/
	/* Flexible Parser 0 table (IF_MATCH_FP0) */
	PFE_PTR(pfe_ct_fp_table_t) fp0_table;
	/* Flexible Parser 1 table (IF_MATCH_FP1) */
	PFE_PTR(pfe_ct_fp_table_t) fp1_table;
#endif
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_class_algo_stats_tag
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_phy_if_stats_tag
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_pe_stats_tag
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
	IF_OP_VLAN_BRIDGE = 4	/*!< L2 bridge with VLAN */
} pfe_ct_if_op_mode_t;

/*	We expect given pfe_ct_if_op_mode_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_if_op_mode_t) == sizeof(uint8_t));

typedef enum __attribute__((packed))
{
	IF_FL_ENABLED = (1 << 0),		/*!< If set, interface is enabled */
	IF_FL_PROMISC = (1 << 1),		/*!< If set, interface is promiscuous */
	IF_FL_FF_ALL_TCP = (1 << 2),	/*!< Enable fast-forwarding of ingress TCP SYN|FIN|RST packets */
	IF_FL_MATCH_OR = (1 << 3)		/*!< Result of match is logical OR of rules, else AND */
} pfe_ct_if_flags_t;

/*	We expect given pfe_ct_if_flags_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_if_flags_t) == sizeof(uint8_t));

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
_ct_assert(sizeof(pfe_ct_if_aft_t) == sizeof(uint8_t));

/**
 * @brief	Interface blocking state
 */
typedef enum __attribute__((packed))
{
	IF_BS_FORWARDING = 0,	/*!< Learning and forwarding enabled */
	IF_BS_BLOCKED = 1,		/*!< Learning and forwarding disabled */
	IF_BS_LEARN_ONLY = 2,	/*!< Learning enabled, forwarding disabled */
} pfe_ct_block_state_t;

/*	We expect given pfe_ct_block_state_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_block_state_t) == sizeof(uint8_t));

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
		to pfe_ct_phy_if_id_t values (1 << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	Gathered statistics */
	pfe_ct_class_algo_stats_t __attribute__((aligned(4))) class_stats; /* Must be aligned at 4 bytes */
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
} pfe_ct_log_if_t;

/**
 * @brief	The physical interface structure as seen by classifier/firmware
 * @details	This structure is shared between firmware and the driver. It represents
 * 			the interface as it is stored in the DMEM.
 * @warning	Do not modify this structure unless synchronization with firmware is ensured.
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_phy_if_tag
{
	/*	Pointer to head of list of logical interfaces (DMEM) */
	PFE_PTR(pfe_ct_log_if_t) log_ifs;
	/*	Pointer to default logical interface (DMEM) */
	PFE_PTR(pfe_ct_log_if_t) def_log_if;
	/*	Gathered statistics */
	pfe_ct_phy_if_stats_t __attribute__((aligned(4))) phy_stats; /* Must be aligned to 4 bytes */
	/*	Physical port number */
	pfe_ct_phy_if_id_t id;
	/*	Operational mode */
	pfe_ct_if_op_mode_t mode;
	/*	Flags */
	pfe_ct_if_flags_t flags;
	/*	Block state */
	pfe_ct_block_state_t block_state;
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

#if (   defined(__GNUC__) \
		&& \
		( \
			(((__GNUC__ == 5) && (__GNUC_MINOR__ == 4) && (__GNUC_PATCHLEVEL__ == 0)) \
			&& (defined(TARGET_ARCH_x86) || defined(TARGET_ARCH_x86_64))) \
			|| defined(TARGET_ARCH_TESTS) \
			|| \
			(((__GNUC__ == 6) && (__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ == 1)) \
			&& defined(TARGET_ARCH_aarch64)) \
		) \
	) \
	|| \
	(	defined(__ghs__) \
		&& ((__GHS_VERSION_NUMBER == 201814) || (__GHS_VERSION_NUMBER == 201914)) \
		&& defined(__LITTLE_ENDIAN__) \
    ) \
	|| \
    (   defined(__DCC__) \
        && (__VERSION_NUMBER__ == 7020) \
        && defined(__ORDER_LITTLE_ENDIAN__) \
	)


/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef struct __attribute__((packed)) __pfe_ct_mac_table_result_tag
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
typedef struct __attribute__((packed)) __pfe_ct_vlan_table_result_tag
{
	union
	{
		struct
		{
			/*	[19:0]  Forward list (1 << pfe_ct_phy_if_id_t) */
			uint64_t forward_list : 20;
			/*	[39:20] Untag list (1 << pfe_ct_phy_if_id_t) */
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
#elif (defined(__GNUC__)) \
	&& (((__GNUC__ == 6) && (__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ == 0)) && defined(TARGET_ARCH_MIPS_M14k))

/**
 * @brief	MAC table lookup result (31-bit)
 */
typedef struct __attribute__((packed)) __pfe_ct_mac_table_result_tag
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
typedef struct __attribute__((packed)) __pfe_ct_vlan_table_result_tag
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
			/*	[39:20] Untag list (1 << pfe_ct_phy_if_id_t) */
			uint64_t untag_list : 20;  /* List of ports to remove VLAN tag */
			/*	[19:0]  Forward list (1 << pfe_ct_phy_if_id_t) */
			uint64_t forward_list : 20;
		};

		uint64_t val;
	};
} pfe_ct_vlan_table_result_t;
#else
	#error Please specify your compiler behavior.
#endif /* Compiler Behavior */

_ct_assert(sizeof(pfe_ct_mac_table_result_t) == sizeof(uint32_t));
_ct_assert(sizeof(pfe_ct_vlan_table_result_t) == sizeof(uint64_t));

/**
 * @brief Bridge domain entry
 */
typedef pfe_ct_vlan_table_result_t pfe_ct_bd_entry_t;

/*	Date string type */
typedef uint8_t pfe_date_str_t[16];

/*	We expect given pfe_date_str_t size. */
_ct_assert(sizeof(pfe_date_str_t) > sizeof(__DATE__));

/**
 * @brief Time string type
 */
typedef uint8_t pfe_time_str_t[16];

/*	We expect given pfe_time_str_t size. */
_ct_assert(sizeof(pfe_time_str_t) > sizeof(__TIME__));

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
typedef struct __attribute__((packed)) __pfe_ct_version_tag
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
	pfe_date_str_t date;
	pfe_time_str_t time;
	/*	Version control ID (e.g. GIT commit) */
	pfe_vctrl_str_t vctrl;
	/*  This header version */
	pfe_cthdr_str_t cthdr;
} pfe_ct_version_t;

/**
	@brief Miscellaneous control commands between host and PE
*/
typedef struct __attribute__ (( packed, aligned (4) )) __pfe_ct_pe_misc_control_tag
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_classify_stats_tag
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_error_tag
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
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_error_record_tag
{
	/* Next position to write: (write_index & (FP_ERROR_RECORD_SIZE - 1)) */
	uint32_t write_index;
	/* Stored errors - pointers point to section .errors which is not
	    part of any memory, just the elf-file */
	PFE_PTR(const pfe_ct_error_t) errors[FP_ERROR_RECORD_SIZE];
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
	PFE_FW_STATE_EXCEPTION          /* FW is stopped after an exception */
} pfe_ct_pe_sw_state_t;

/**
 * @brief Monitoring of the firmware state (watchdog)
 * @details FW updates the variable with the current state and increments counter
 *          with each state transition. The driver monitors the variable.
 * @note Written only by FW and read by Driver.
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_pe_sw_state_monitor_tag
{
	uint32_t counter;               /* Incremented with each state change */
	pfe_ct_pe_sw_state_t state;     /* Reflect the current FW state */
	uint8_t reserved[3];            /* To make size multiple of 4 bytes */
} pfe_ct_pe_sw_state_monitor_t;

_ct_assert(sizeof(pfe_ct_pe_sw_state_monitor_t) == 8);


/**
 * @brief Storage for measured time intervals used during firmware performance monitoring
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_ct_measure_tag
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
_ct_assert(sizeof(pfe_ct_flexible_filter_t) == sizeof(uint32_t));

/**
 * @brief PE memory map representation type shared between host and PFE
 */
typedef struct __attribute__((packed, aligned(4))) __pfe_ct_pe_mmap_tag
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

} pfe_ct_pe_mmap_t;

typedef enum __attribute__((packed))
{
	/*	Punt by snooping feature */
	PUNT_SNOOP = (1 << 0),
	/*	Ensure proper size */
	PUNT_MAX = (1 << 15)
} pfe_ct_punt_reasons_t;

/*	We expect given pfe_ct_punt_reasons_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_punt_reasons_t) == sizeof(uint16_t));

typedef enum __attribute__((packed))
{
	/*	IP checksum valid */
	HIF_RX_IPV4_CSUM = (1 << 0),
	/*	TCP of IPv4 checksum valid */
	HIF_RX_TCPV4_CSUM = (1 << 1),
	/*	TCP of IPv6 checksum valid */
	HIF_RX_TCPV6_CSUM = (1 << 2),
	/*	UDP of IPv4 checksum valid */
	HIF_RX_UDPV4_CSUM = (1 << 3),
	/*	UDP of IPv6 checksum valid */
	HIF_RX_UDPV6_CSUM = (1 << 4),
	/*	PTP packet */
	HIF_RX_PTP = (1 << 5),
	/*	Punt flag. If set then punt reason is valid. */
	HIF_RX_PUNT = (1 << 6),
	/*	Timestamp flag. When set, timestamp is valid. */
	HIF_RX_TS = (1 << 7),
	/*	Inter - HIF communication frame */
	HIF_RX_IHC = (1 << 8)
} pfe_ct_hif_rx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_hif_rx_flags_t) == sizeof(uint16_t));

/**
 * @brief	HIF RX packet header
 */
typedef struct __attribute__((packed)) __pfe_ct_hif_rx_hdr_tag
{
	/*	Punt reason flags */
	pfe_ct_punt_reasons_t punt_reasons;
	/*	Ingress physical interface ID */
	pfe_ct_phy_if_id_t i_phy_if;
	/*	Ingress logical interface ID */
	uint8_t i_log_if;
	/*	Rx frame flags */
	pfe_ct_hif_rx_flags_t flags;
	/*	Reserved */
	uint8_t reserved[2];
	/*	RX timestamp */
	uint32_t rx_timestamp_ns;
	uint32_t rx_timestamp_s;
} pfe_ct_hif_rx_hdr_t;

_ct_assert(sizeof(pfe_ct_hif_rx_hdr_t) == 16);

typedef enum __attribute__((packed))
{
	/*	IP checksum offload. If set then PFE will calculate and
		insert IP header checksum. */
	HIF_TX_IP_CSUM = (1 << 0),
	/*	TCP checksum offload. If set then PFE will calculate and
		insert TCP header checksum. */
	HIF_TX_TCP_CSUM = (1 << 1),
	/*	UDP checksum offload. If set then PFE will calculate and
		insert UDP header checksum. */
	HIF_TX_UDP_CSUM = (1 << 2),
	/*	Transmit Inject Flag. If not set the packet will be processed
		according to Physical Interface configuration. If set then
		e_phy_ifs is valid and packet will be transmitted via physical
		interfaces given by the list. */
	HIF_TX_INJECT = (1 << 3),
	/*	Inter-HIF communication frame */
	HIF_TX_IHC = (1 << 4)
} pfe_ct_hif_tx_flags_t;

/*	We expect given pfe_ct_hif_rx_flags_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_hif_tx_flags_t) == sizeof(uint8_t));

/**
 * @brief	HIF TX packet header
 */
typedef struct __attribute__((aligned)) __pfe_ct_hif_tx_hdr_t
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
	/*	List of egress physical interfaces to be used for injection. Bit positions
		correspond to pfe_ct_phy_if_id_t values (1 << pfe_ct_phy_if_id_t). */
	uint32_t e_phy_ifs;
	/*	Reserved */
	uint32_t reserved2[2];
} pfe_ct_hif_tx_hdr_t;

_ct_assert(sizeof(pfe_ct_hif_tx_hdr_t) == 16);

/*	We expect given pfe_ct_hif_tx_hdr_t size due to byte order compatibility. */
_ct_assert(0 == (sizeof(pfe_ct_hif_tx_hdr_t) % sizeof(uint32_t)));

/**
 * @brief	Post-classification header
 */
typedef struct __attribute__((packed)) __pfe_ct_post_cls_hdr_tag
{
	uint32_t start_data_off	: 8;	/* Data start offset */
	uint32_t start_buf_off	: 10;	/* Buffer start offset */
	uint32_t pkt_length		: 14;	/* Total packet length */
	uint8_t act_phy;				/* Action block 7:4 and phyno 3:0 */
	uint8_t queueno;				/* Destination TMU queue number */
	uint16_t src_mac_msb;			/* Source MAC address [47:32] if replacement is requested */
	uint32_t src_mac_lsb;			/* Source MAC address [31:0] if replacement is requested */
	uint32_t vlanid;				/* VLAN ID if replacement/insertion is requested */
} pfe_ct_post_cls_hdr_t;

/**
 * @brief	Routing actions
 * @details	When packet is routed an action or actions can be assigned to be executed
 * 			during the routing process. This can be used to configure the router to do
 *			NAT, update TTL, or insert	VLAN header.
 */
typedef enum __attribute__((packed))
{
	RT_ACT_ADD_ETH_HDR = (1 << 0),		/*!< Construct/Update Ethernet Header */
	RT_ACT_ADD_VLAN_HDR = (1 << 1),		/*!< Construct/Update outer VLAN Header */
	RT_ACT_ADD_PPPOE_HDR = (1 << 2),	/*!< Construct/Update PPPOE Header */
	RT_ACT_DEC_TTL = (1 << 7),			/*!< Decrement TTL */
	RT_ACT_ADD_VLAN1_HDR = (1 << 11),	/*!< Construct/Update inner VLAN Header */
	RT_ACT_CHANGE_SIP_ADDR = (1 << 17),	/*!< Change Source IP Address */
	RT_ACT_CHANGE_SPORT = (1 << 18),	/*!< Change Source Port */
	RT_ACT_CHANGE_DIP_ADDR = (1 << 19),	/*!< Change Destination IP Address */
	RT_ACT_CHANGE_DPORT = (1 << 20),	/*!< Change Destination Port */
	RT_ACT_DEL_VLAN_HDR = (1 << 21),	/*!< Delete outer VLAN Header */
	RT_ACT_INVALID = (1 << 31)			/*!< Invalid value */
} pfe_ct_route_actions_t;

/*	We expect given pfe_ct_route_actions_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_route_actions_t) == sizeof(uint32_t));

/**
 * @brief	Arguments for routing actions
 */
typedef struct __attribute__((packed)) __pfe_ct_route_actions_args_tag
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
	RT_FL_VALID = (1 << 0),	/*!< Entry is valid */
	RT_FL_IPV6 = (1 << 1),	/*!< If set entry is IPv6 else it is IPv4 */
	RT_FL_MAX = (1 << 31)	/* Ensure proper size */
} pfe_ct_rtable_flags_t;

/*	We expect given pfe_ct_rtable_flags_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_ct_rtable_flags_t) == sizeof(uint32_t));

/**
 * @brief	Routing table entry status flags
 */
typedef enum __attribute__((packed))
{
	RT_STATUS_ACTIVE = (1 << 0)	/*!< If set, entry has been matched
								     by routing table lookup algorithm */
} pfe_rtable_entry_status_t;

/*	We expect given pfe_rtable_entry_status_t size due to byte order compatibility. */
_ct_assert(sizeof(pfe_rtable_entry_status_t) == sizeof(uint8_t));

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
	uint32_t dummy[2];
	uint32_t rt_orig;
} pfe_ct_rtable_entry_t;

/*	We expect given pfe_ct_rtable_entry_t size due to HW requirement. */
_ct_assert(sizeof(pfe_ct_rtable_entry_t) == 128);

#endif /* HW_S32G_PFE_CT_H_ */
/** @} */

