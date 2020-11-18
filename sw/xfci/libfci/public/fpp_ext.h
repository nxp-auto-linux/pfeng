/* =========================================================================
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgrLibFCI
 * @{
 *
 * @file        fpp_ext.h
 * @brief       Extension of the legacy fpp.h
 * @details     All FCI commands and related elements not present within the
 *              legacy fpp.h shall be put into this file. All macro values
 *              (uint16_t) shall have the upper nibble set to b1111 to ensure
 *              no conflicts with the legacy macro values.
 * @note        Documentation is part of libfci.h.
 */

#ifndef FPP_EXT_H_
#define FPP_EXT_H_

/*	Compiler abstraction macros */
#ifndef CAL_PACKED
#define CAL_PACKED				__attribute__((packed))
#endif /* CAL_PACKED */

#ifndef CAL_PACKED_ALIGNED
#define CAL_PACKED_ALIGNED(n)	__attribute__((packed, aligned(n)))
#endif /* CAL_PACKED_ALIGNED */

#define FPP_ERR_INTERNAL_FAILURE				0xffff

/**
 * @def FPP_CMD_PHY_IF
 * @brief FCI command for working with physical interfaces.
 * @note Command is defined as extension of the legacy fpp.h.
 * @details Interfaces are needed to be known to FCI to support insertion of routes and conntracks.
 *          Command can be used to get operation mode, mac address and operation flags (enabled, promisc).
 * @details Command can be used with various `.action` values:
 *          - @c FPP_ACTION_UPDATE: Updates properties of an existing physical interface.
 *          - @c FPP_ACTION_QUERY: Gets head of list of existing physical interfaces properties.
 *          - @c FPP_ACTION_QUERY_CONT: Gets next item from list of existing physical interfaces. Shall
 * 				be called after @ref FPP_ACTION_QUERY was called. On each call it replies with properties
 *              of the next interface in the list.
 *
 * @note Precondition to use the query is to atomically lock the access with @ref FPP_CMD_IF_LOCK_SESSION.
 *
 * Command Argument Type: @ref fpp_phy_if_cmd_t
 *
 * Action FPP_ACTION_UPDATE
 * --------------------------
 * Update interface properties. Set fpp_phy_if_cmd_t.action to @ref FPP_ACTION_UPDATE and fpp_phy_if_cmd_t.name
 * to name of the desired interface to be updated. Rest of the fpp_phy_if_cmd_t members will be considered
 * to be used as the new interface properties. It is recommended to use read-modify-write approach in
 * combination with @ref FPP_ACTION_QUERY and @ref FPP_ACTION_QUERY_CONT.
 * 
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Get interface properties. Set fpp_phy_if_cmd_t.action to @ref FPP_ACTION_QUERY to get first interface from
 * the list of physical interfaces or @ref FPP_ACTION_QUERY_CONT to get subsequent entries. Response data
 * type for query commands is of type @ref fpp_phy_if_cmd_t.
 *
 * For operation modes see @ref fpp_phy_if_op_mode_t. For operation flags see @ref fpp_if_flags_t.
 * 
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Success.
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: Last entry in the query session.
 *     - @c FPP_ERR_IF_WRONG_SESSION_ID: Someone else is already working with the interfaces.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 * 
 * @hideinitializer
 */
#define FPP_CMD_PHY_IF					0xf100

/**
 * @def FPP_CMD_LOG_IF
 * @brief FCI command for working with logical interfaces
 * @note    Command is defined as extension of the legacy fpp.h.
 * @details Command can be used to update match rules of logical interface or for adding egress interfaces.
 *          It can also update operational flags (enabled, promisc, match).
 *          Following values of `.action` are supported:
 *          - @c FPP_ACTION_REGISTER: Creates a new logical interface.
 *          - @c FPP_ACTION_DEREGISTER: Destroys an existing logical interface.
 *          - @c FPP_ACTION_UPDATE: Updates properties of an existing logical interface.
 *          - @c FPP_ACTION_QUERY: Gets head of list of existing logical interfaces parameters.
 *          - @c FPP_ACTION_QUERY_CONT: Gets next item from list of existing logical interfaces. Shall
 *            be called after @ref FPP_ACTION_QUERY was called. On each call it replies with properties
 *            of the next interface.
 * 
 * Precondition to use the query is to atomic lock the access with @ref FPP_CMD_IF_LOCK_SESSION.
 *
 * Command Argument Type: @ref fpp_log_if_cmd_t
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * To create a new logical interface the @ref FPP_CMD_LOG_IF command expects following values to be set
 * in the command argument structure:
 * @code{.c}
 *   fpp_log_if_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_REGISTER,   // Register new logical interface
 *     .name = "logif1",                // Name of the new logical interface
 *     .parent_name = "emac0"           // Name of the parent physical interface
 *   };
 * @endcode
 * The interface <i>logif1</i> will be created as child of <i>emac0</i> without any configuration
 * and disabled. Names of available physical interfaces can be obtained via @ref FPP_CMD_PHY_IF +
 * @ref FPP_ACTION_QUERY + @ref FPP_ACTION_QUERY_CONT.
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure to remove a logical interface:
 * @code{.c}
 *   fpp_log_if_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_DEREGISTER,   // Destroy an existing logical interface
 *     .name = "logif1",                  // Name of the logical interface to destroy
 *   };
 * @endcode
 *
 * Action FPP_ACTION_UPDATE
 * ------------------------
 * To update logical interface properties just set fpp_log_if_cmd_t.action to @ref FPP_ACTION_UPDATE and
 * fpp_log_if_cmd_t.name to the name of logical interface which you wish to update. Rest of the
 * fpp_log_if_cmd_t structure members will be considered to be used as the new interface properties. It
 * is recommended to use read-modify-write approach in combination with @ref FPP_ACTION_QUERY and @ref
 * FPP_ACTION_QUERY_CONT.
 *
 * For match rules see (@ref fpp_if_m_rules_t). For match rules arguments see (@ref fpp_if_m_args_t).
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Update successful.
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: If corresponding logical interface doesn't exit.
 *     - @c FPP_ERR_IF_RESOURCE_ALREADY_LOCKED: Someone else is already configuring the interfaces.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Get interface properties. Set fpp_log_if_cmd_t.action to @ref FPP_ACTION_QUERY to get first interface
 * from the list of all logical interfaces or @ref FPP_ACTION_QUERY_CONT to get subsequent entries.
 * Response data type for query commands is of type fpp_log_if_cmd_t.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Success.
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: Last entry in the query session.
 *     - @c FPP_ERR_IF_WRONG_SESSION_ID: Someone else is already working with the interfaces.
 *     - @c FPP_ERR_IF_MATCH_UPDATE_FAILED: Update of match flags has failed.
 *     - @c FPP_ERR_IF_EGRESS_UPDATE_FAILED: Update of egress interfaces has failed.
 *     - @c FPP_ERR_IF_EGRESS_DOESNT_EXIST: Egress interface provided in command doesn't exist.
 *     - @c FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED: Operation flags update has failed (PROMISC/ENABLE/MATCH).
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 * 
 * @hideinitializer
 */
#define FPP_CMD_LOG_IF					0xf101

/**
 * @def FPP_ERR_IF_ENTRY_ALREADY_REGISTERED
 * @hideinitializer
 */
#define FPP_ERR_IF_ENTRY_ALREADY_REGISTERED		0xf103

/**
 * @def FPP_ERR_IF_ENTRY_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_IF_ENTRY_NOT_FOUND				0xf104

/**
 * @def FPP_ERR_IF_EGRESS_DOESNT_EXIST
 * @hideinitializer
 */
#define FPP_ERR_IF_EGRESS_DOESNT_EXIST			0xf105

/**
 * @def FPP_ERR_IF_EGRESS_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_EGRESS_UPDATE_FAILED			0xf106

/**
 * @def FPP_ERR_IF_MATCH_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_MATCH_UPDATE_FAILED			0xf107

/**
 * @def FPP_ERR_IF_OP_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_OP_UPDATE_FAILED				0xf108

/**
 * @def FPP_ERR_IF_OP_CANNOT_CREATE
 * @hideinitializer
 */
#define FPP_ERR_IF_OP_CANNOT_CREATE				0xf109

/**
 * @def FPP_ERR_IF_RESOURCE_ALREADY_LOCKED
 * @hideinitializer
 */
#define FPP_ERR_IF_RESOURCE_ALREADY_LOCKED		0xf110

/**
 * @def FPP_ERR_IF_WRONG_SESSION_ID
 * @hideinitializer
 */
#define FPP_ERR_IF_WRONG_SESSION_ID				0xf111

/**
 * @def		FPP_CMD_IF_LOCK_SESSION
 * @brief	FCI command to perform lock on interface database.
 * @details	The reason for it is guaranteed atomic operation between fci/rpc/platform.
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * Possible command return values are:
 *     -  FPP_ERR_OK: Lock successful
 *     -  FPP_ERR_IF_RESOURCE_ALREADY_LOCKED: Database was already locked by someone else
 * 
 * @hideinitializer
 */
#define FPP_CMD_IF_LOCK_SESSION					0x0015

/**
 * @def		FPP_CMD_IF_UNLOCK_SESSION
 * @brief	FCI command to perform unlock on interface database.
 * @details	The reason for it is guaranteed atomic operation between fci/rpc/platform.
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * Possible command return values are:
 *     -  FPP_ERR_OK: Lock successful
 *     -  FPP_ERR_IF_WRONG_SESSION_ID: The lock wasn't locked or was locked in different
 *     									 session and will not be unlocked.
 * @hideinitializer
 */
#define FPP_CMD_IF_UNLOCK_SESSION				0x0016

/**
 * @brief	Interface flags
 */
typedef enum CAL_PACKED
{
	FPP_IF_ENABLED = (1 << 0),		/*!< If set, interface is enabled */
	FPP_IF_PROMISC = (1 << 1),		/*!< If set, interface is promiscuous */
	FPP_IF_MATCH_OR = (1 << 3),		/*!< Result of match is logical OR of rules, else AND */
	FPP_IF_DISCARD = (1 << 4),		/*!< Discard matching frames */
	FPP_IF_MIRROR = (1 << 5)		/*!< If set mirroring is enabled */
} fpp_if_flags_t;

/**
 * @typedef fpp_phy_if_op_mode_t
 * @brief	Physical if modes
 */
typedef enum CAL_PACKED
{
	FPP_IF_OP_DISABLED = 0,			/*!< Disabled */
	FPP_IF_OP_DEFAULT = 1,			/*!< Default operational mode */
	FPP_IF_OP_BRIDGE = 2,			/*!< L2 bridge */
	FPP_IF_OP_ROUTER = 3,			/*!< L3 router */
	FPP_IF_OP_VLAN_BRIDGE = 4,		/*!< L2 bridge with VLAN */
	FPP_IF_OP_FLEXIBLE_ROUTER = 5	/*!< Flexible router */
} fpp_phy_if_op_mode_t;

/**
 * @brief	Match rules. Can be combined using bitwise OR.
 */
typedef enum CAL_PACKED
{
	FPP_IF_MATCH_TYPE_ETH = (1 << 0),		/**< Match ETH Packets */
	FPP_IF_MATCH_TYPE_VLAN = (1 << 1),		/**< Match VLAN Tagged Packets */
	FPP_IF_MATCH_TYPE_PPPOE = (1 << 2),		/**< Match PPPoE Packets */
	FPP_IF_MATCH_TYPE_ARP = (1 << 3),		/**< Match ARP Packets */
	FPP_IF_MATCH_TYPE_MCAST = (1 << 4),		/**< Match Multicast (L2) Packets */
	FPP_IF_MATCH_TYPE_IPV4 = (1 << 5),		/**< Match IPv4 Packets */
	FPP_IF_MATCH_TYPE_IPV6 = (1 << 6),		/**< Match IPv6 Packets */
	FPP_IF_MATCH_RESERVED7 = (1 << 7),		/**< Reserved */
	FPP_IF_MATCH_RESERVED8 = (1 << 8),		/**< Reserved */
	FPP_IF_MATCH_TYPE_IPX = (1 << 9),		/**< Match IPX Packets */
	FPP_IF_MATCH_TYPE_BCAST = (1 << 10),	/**< Match Broadcast (L2) Packets */
	FPP_IF_MATCH_TYPE_UDP = (1 << 11),		/**< Match UDP Packets */
	FPP_IF_MATCH_TYPE_TCP = (1 << 12),		/**< Match TCP Packets */
	FPP_IF_MATCH_TYPE_ICMP = (1 << 13),		/**< Match ICMP Packets */
	FPP_IF_MATCH_TYPE_IGMP = (1 << 14),		/**< Match IGMP Packets */
	FPP_IF_MATCH_VLAN = (1 << 15),			/**< Match VLAN ID */
	FPP_IF_MATCH_PROTO = (1 << 16),			/**< Match IP Protocol */
	FPP_IF_MATCH_SPORT = (1 << 20),			/**< Match L4 Source Port */
	FPP_IF_MATCH_DPORT = (1 << 21),			/**< Match L4 Destination Port */
	FPP_IF_MATCH_SIP6 = (1 << 22),			/**< Match Source IPv6 Address */
	FPP_IF_MATCH_DIP6 = (1 << 23),			/**< Match Destination IPv6 Address */
	FPP_IF_MATCH_SIP = (1 << 24),			/**< Match Source IPv4 Address */
	FPP_IF_MATCH_DIP = (1 << 25),			/**< Match Destination IPv4 Address */
	FPP_IF_MATCH_ETHTYPE = (1 << 26),		/**< Match EtherType */
	FPP_IF_MATCH_FP0 = (1 << 27),			/**< Match Packets Accepted by Flexible Parser 0 */
	FPP_IF_MATCH_FP1 = (1 << 28),			/**< Match Packets Accepted by Flexible Parser 1 */
	FPP_IF_MATCH_SMAC = (1 << 29),			/**< Match Source MAC Address */
	FPP_IF_MATCH_DMAC = (1 << 30),			/**< Match Destination MAC Address */
	FPP_IF_MATCH_HIF_COOKIE = (1 << 31),	/**< Match HIF header cookie value */
	/* Ensure proper size */
	FPP_IF_MATCH_MAX = (1 << 31)
} fpp_if_m_rules_t;

/**
 * @brief	Match rules arguments.
 * @details Every value corresponds to specified match rule (@ref fpp_if_m_rules_t).
 */
typedef struct CAL_PACKED_ALIGNED(4)
{
	/** VLAN ID (@ref FPP_IF_MATCH_VLAN) */
	uint16_t vlan;
	/** EtherType (@ref FPP_IF_MATCH_ETHTYPE) */
	uint16_t ethtype;
	/** L4 source port number (@ref FPP_IF_MATCH_SPORT) */
	uint16_t sport;
	/** L4 destination port number (@ref FPP_IF_MATCH_DPORT) */
	uint16_t dport;
	/* Source and destination addresses */
	struct
	{
		/**	IPv4 source and destination address (@ref FPP_IF_MATCH_SIP, @ref FPP_IF_MATCH_DIP) */
		struct
		{
			uint32_t sip;
			uint32_t dip;
		} v4;

		/**	IPv6 source and destination address (@ref FPP_IF_MATCH_SIP6, @ref FPP_IF_MATCH_DIP6) */
		struct
		{
			uint32_t sip[4];
			uint32_t dip[4];
		} v6;
	};
	/** IP protocol (@ref FPP_IF_MATCH_PROTO) */
	uint8_t proto;
	/** Source MAC Address (@ref FPP_IF_MATCH_SMAC) */
	uint8_t smac[6];
	/** Destination MAC Address (@ref FPP_IF_MATCH_DMAC) */
	uint8_t dmac[6];
	/** Flexible Parser table 0 (@ref FPP_IF_MATCH_FP0) */
	char fp_table0[16];
	/** Flexible Parser table 1 (@ref FPP_IF_MATCH_FP1) */
	char fp_table1[16];
	/** HIF header cookie (@ref FPP_IF_MATCH_HIF_COOKIE) */
	uint32_t hif_cookie;
} fpp_if_m_args_t;

/**
 * @brief	Physical interface statistics
 * @details Statistics used by physical interfaces (EMAC, HIF).
 * @note All statistics counters are in network byte order.
 */
typedef struct CAL_PACKED_ALIGNED(4)
{
	uint32_t ingress;	/*!< Number of ingress frames for the given interface  */
	uint32_t egress;	/*!< Number of egress frames for the given interface */
	uint32_t malformed;	/*!< Number of ingress frames with detected error (i.e. checksum) */
	uint32_t discarded;	/*!< Number of ingress frames which were discarded */
} fpp_phy_if_stats_t;

/**
 * @brief	Algorithm statistics
 * @details Statistics used by algorithms in class (eg. log ifs).
 * @note All statistics counters are in network byte order.
 */
typedef struct CAL_PACKED_ALIGNED(4)
{
	uint32_t processed;	/*!< Number of frames processed regardless the result */
	uint32_t accepted;	/*!< Number of frames matching the selection criteria */
	uint32_t rejected;	/*!< Number of frames not matching the selection criteria */
	uint32_t discarded;	/*!< Number of frames marked to be dropped */
} fpp_algo_stats_t;

/**
 * @brief	Interface blocking state
 */
typedef enum CAL_PACKED
{
	BS_NORMAL = 0,		/*!< Learning and forwarding enabled */
	BS_BLOCKED = 1,		/*!< Learning and forwarding disabled */
	BS_LEARN_ONLY = 2,	/*!< Learning enabled, forwarding disabled */
	BS_FORWARD_ONLY = 3	/*!< Learning disabled, forwarding enabled */
} fpp_phy_if_block_state_t;

/**
 * @brief		Data structure to be used for physical interface commands
 * @details		Usage:
 * 				- As command buffer in functions @ref fci_write, @ref fci_query or
 * 				  @ref fci_cmd, with @ref FPP_CMD_PHY_IF command.
 * 				- As reply buffer in functions @ref fci_query or @ref fci_cmd,
 * 				  with @ref FPP_CMD_PHY_IF command.
 */
typedef struct CAL_PACKED
{
	uint16_t action;			/**< Action */
	char name[IFNAMSIZ];		/**< Interface name */
	uint32_t id;				/**< Interface ID (network endian) */
	fpp_if_flags_t flags;		/**< Interface flags (network endian) */
	fpp_phy_if_op_mode_t mode;	/**< Phy if mode (network endian) */
	fpp_phy_if_block_state_t block_state;	/**< Phy if block state */
	uint8_t mac_addr[6];		/**< Phy if MAC (network endian) */
	char mirror[IFNAMSIZ];		/**< Name of interface to mirror the traffic to */
	fpp_phy_if_stats_t	stats;	/**< Physical interface statistics */
} fpp_phy_if_cmd_t;

/**
 * @brief		Data structure to be used for logical interface commands
 * @details		Usage:
 * 				- As command buffer in functions @ref fci_write, @ref fci_query or
 * 				  @ref fci_cmd, with @ref FPP_CMD_LOG_IF command.
 * 				- As reply buffer in functions @ref fci_query or @ref fci_cmd,
 * 				  with @ref FPP_CMD_LOG_IF command.
 */
typedef struct CAL_PACKED
{
	uint16_t action;			/**< Action */
	char name[IFNAMSIZ];		/**< Interface name */
	uint32_t id;				/**< Interface ID (network endian) */
	char parent_name[IFNAMSIZ];	/**< Parent physical interface name */
	uint32_t parent_id;			/**< Parent physical interface ID (network endian) */
	uint32_t egress;			/**< Egress interfaces in the form of mask (to get egress id: egress & (1 < id))
								   must be stored in network order (network endian) */
	fpp_if_flags_t flags;		/**< Interface flags from query or flags to be set (network endian) */
	fpp_if_m_rules_t match;		/**< Match rules from query or match rules to be set (network endian) */
	fpp_if_m_args_t arguments;	/**< Arguments for match rules (network endian) */
	fpp_algo_stats_t stats;		/**< Logical interface statistics */
} fpp_log_if_cmd_t;

/**
 * @def FPP_CMD_L2_BD
 * @brief VLAN-based L2 bridge domain management
 * @details Bridge domain can be used to include a set of physical interfaces and isolate them
 *          from another domains using VLAN. Command can be used with various `.action` values:
 *          - @c FPP_ACTION_REGISTER: Create a new bridge domain.
 *          - @c FPP_ACTION_DEREGISTER: Delete bridge domain.
 *          - @c FPP_ACTION_UPDATE: Update a bridge domain meaning that will rewrite domain properties except of VLAN ID.
 *          - @c FPP_ACTION_QUERY: Gets head of list of registered domains.
 *          - @c FPP_ACTION_QUERY_CONT: Get next item from list of registered domains. Shall be called after
 *               @c FPP_ACTION_QUERY was called. On each call it replies with parameters of next domain.
 *               It returns @c FPP_ERR_RT_ENTRY_NOT_FOUND when no more entries exist.
 *
 * Command Argument Type: @ref fpp_l2_bd_cmd_t
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bd_cmd_t cmd_data =
 *   {
 *     // Register new bridge domain
 *     .action = FPP_ACTION_REGISTER,
 *     // VLAN ID associated with the domain (network endian)
 *     .vlan = ...,
 *     // Action to be taken when destination MAC address (uni-cast) of a packet
 *     // matching the domain is found in the MAC table: 0 - Forward, 1 - Flood,
 *     // 2 - Punt, 3 - Discard
 *     .ucast_hit = ...,
 *     // Action to be taken when destination MAC address (uni-cast) of a packet
 *     // matching the domain is not found in the MAC table.
 *     .ucast_miss = ...,
 *     // Multicast hit action
 *     .mcast_hit = ...,
 *     // Multicast miss action
 *     .mcast_miss = ...
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain added.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument.
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_ALREADY_REGISTERED: Given domain already registered.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 *
 * Action FPP_ACTION_DEREGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bd_cmd_t cmd_data =
 *   {
 *     // Delete bridge domain
 *     .action = FPP_ACTION_DEREGISTER,
 *     // VLAN ID associated with the domain to be deleted (network endian)
 *     .vlan = ...,
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain removed.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument.
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: Given domain not found.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * Action FPP_ACTION_UPDATE
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bd_cmd_t cmd_data =
 *   {
 *     // Update bridge domain
 *     .action = FPP_ACTION_UPDATE,
 *     // VLAN ID associated with the domain to be updated (network endian)
 *     .vlan = ...,
 *     // New unicast hit action (0 - Forward, 1 - Flood, 2 - Punt, 3 - Discard)
 *     .ucast_hit = ...,
 *     // New unicast miss action
 *     .ucast_miss = ...,
 *     // New multicast hit action
 *     .mcast_hit = ...,
 *     // New multicast miss action
 *     .mcast_miss = ...,
 *     // New port list (network endian). Bitmask where every set bit represents
 *     // ID of physical interface being member of the domain. For instance bit
 *     // (1 << 3), if set, says that interface with ID=3 is member of the domain.
 *     // Only valid interface IDs are accepted by the command. If flag is set,
 *     // interface is added to the domain. If flag is not set and interface
 *     // has been previously added, it is removed. The IDs are given by the
 *     // related FCI endpoint and related networking HW. Interface IDs can be
 *     // obtained via FPP_CMD_PHY_IF.
 *     .if_list = ...,
 *     // Flags marking interfaces listed in @c if_list to be 'tagged' or
 *     // 'untagged' (network endian). If respective flag is set, corresponding
 *     // interface within the @c if_list is treated as 'untagged' meaning that
 *     // the VLAN tag will be removed. Otherwise it is configured as 'tagged'.
 *     // Note that only interfaces listed within the @c if_list are taken into
 *     // account.
 *     .untag_if_list = ...,
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain updated.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument.
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: Given domain not found.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bd_cmd_t cmd_data =
 *   {
 *     .action = ...    // Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 *   };
 * @endcode
 *
 * Response data type for queries: @ref fpp_l2_bd_cmd_t
 *
 * Response data provided (all values in network byte order):
 * @code{.c}
 *     // VLAN ID associated with domain (network endian)
 *     rsp_data.vlan;
 *     // Action to be taken when destination MAC address (uni-cast) of a packet
 *     // matching the domain is found in the MAC table: 0 - Forward, 1 - Flood,
 *     // 2 - Punt, 3 - Discard
 *     rsp_data.ucast_hit;
 *     // Action to be taken when destination MAC address (uni-cast) of a packet
 *     // matching the domain is not found in the MAC table.
 *     rsp_data.ucast_miss;
 *     // Multicast hit action.
 *     rsp_data.mcast_hit;
 *     // Multicast miss action.
 *     rsp_data.mcast_miss;
 *     // Bitmask where every set bit represents ID of physical interface being member
 *     // of the domain. For instance bit (1 << 3), if set, says that interface with ID=3
 *     // is member of the domain.
 *     rsp_data.if_list;
 *     // Similar to @c if_list but this interfaces are configured to be VLAN 'untagged'.
 *     rsp_data.untag_if_list;
 *     // See the fpp_l2_bd_flags_t.
 *     rsp_data.flags;
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Response buffer written.
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: No more entries.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_BD						0xf200

#define FPP_ERR_L2BRIDGE_DOMAIN_ALREADY_REGISTERED	0xf201
#define FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND			0xf202

/**
 * @brief	L2 bridge domain flags
 */
typedef enum CAL_PACKED
{
	FPP_L2BR_DOMAIN_DEFAULT = (1 << 0),	/*!< Domain type is default */
	FPP_L2BR_DOMAIN_FALLBACK = (1 << 1)	/*!< Domain type is fallback */
} fpp_l2_bd_flags_t;

/**
 * @brief   Data structure to be used for command buffer for L2 bridge domain control commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with commands: @ref FPP_CMD_L2_BD.
 */
typedef struct CAL_PACKED fpp_l2_bridge_domain_control_cmd
{
	/**	Action to be executed (register, unregister, query, ...) */
	uint16_t action;
	/**	VLAN ID associated with the bridge domain (network endian) */
	uint16_t vlan;
	/**	Action to be taken when destination MAC address (uni-cast) of a packet matching the domain
		is found in the MAC table (network endian): 0 - Forward, 1 - Flood, 2 - Punt, 3 - Discard */
	uint8_t ucast_hit;
	/**	Action to be taken when destination MAC address (uni-cast) of a packet matching the domain
		is not found in the MAC table */
	uint8_t ucast_miss;
	/**	Multicast hit action */
	uint8_t mcast_hit;
	/**	Multicast miss action */
	uint8_t mcast_miss;
	/**	Port list (network endian). Bitmask where every set bit represents ID of physical interface
		being member of the domain. For instance bit (1 « 3), if set, says that interface with ID=3
		is member of the domain. Only valid interface IDs are accepted by the command. If flag is set,
		interface is added to the domain. If flag is not set and interface has been previously added,
		it is removed. The IDs are given by the related FCI endpoint and related networking HW. Interface
		IDs can be obtained via FPP_CMD_PHY_IF. */
	uint32_t if_list;
	/**	Flags marking interfaces listed in @c if_list to be 'tagged' or 'untagged' (network endian).
		If respective flag is set, corresponding interface within the @c if_list is treated as 'untagged'
		meaning that the VLAN tag will be removed. Otherwise it is configured as 'tagged'. Note that
		only interfaces listed within the @c if_list are taken into account. */
	uint32_t untag_if_list;
	/**	See the @ref fpp_l2_bd_flags_t */
	fpp_l2_bd_flags_t flags;
} fpp_l2_bd_cmd_t;

/**
 * @def FPP_CMD_FP_TABLE
 * @brief Administers the Flexible Parser tables
 * @details The Flexible Parser table is an ordered set of Flexible Parser rules which
 *          are matched in the order of appearance until match occurs or end of the table
 *          is reached. The following actions can be done on the table:
 *          - @c FPP_ACTION_REGISTER: Create a new table with a given name.
 *          - @c FPP_ACTION_DEREGISTER: Destroy an existing table.
 *          - @c FPP_ACTION_USE_RULE: Add a rule into the table at specified position.
 *          - @c FPP_ACTION_UNUSE_RULE: Remove a rule from the table.
 *          - @c FPP_ACTION_QUERY: Return the first rule in the table.
 *          - @c FPP_ACTION_QUERY_CONT: Return the next rule in the table.
 *
 * The Flexible Parser starts processing the table from the 1st rule in the table. If there is no match
 * the Flexible Parser always continues with the rule following the currently processed rule.
 * The processing ends once rule match happens and the rule action is one of the FP_ACCEPT
 * or FP_REJECT and the respective value is returned.
 * REJECT is also returned after the last rule in the table was processed without any match.
 * The Flexible Parser may branch to arbitrary rule in the table if some rule matches and the
 * action is FP_NEXT_RULE. Note that loops are forbidden.
 *
 * See the FPP_CMD_FP_RULE and @ref fpp_fp_rule_props_t for the detailed description
 * of how the rules are being matched.
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_fp_table_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_REGISTER,    // Add a new table
 *      .t.table_name = "table_name",     // Unique up-to-15-character table identifier
 *   };
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_fp_table_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_DEREGISTER,  // Remove an existing table
 *      .t.table_name = "table_name",     // Identifier of the table to be destroyed
 *   };
 * @endcode
 *
 * Action FPP_ACTION_USE_RULE
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_fp_table_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_USE_RULE,    // Add existing rule into specified table
 *      .t.table_name = "table_name",     // Identifier of the table to add the rule
 *      .t.rule_name = "rule_name",       // Identifier of the rule to be added into the table
 *   };
 * @endcode
 * @note Single rule can be member of only one table.
 *
 * Action FPP_ACTION_UNUSE_RULE
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_UNUSE_RULE,  // Remove an existing table
 *      .t.rule_name = "rule_name",       // Identifier of the rule to be removed from the table
 *   };
 * @endcode
 *
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY,       // Start query of the table rules
 *      .t.table_name = "table_name",     // Identifier of the table to be queried
 *   };
 * @endcode
 *
 * Response data type for queries: @ref fpp_fp_rule_cmd_t
 * 
 * Response data provided:
 * @code{.c}
 *   rsp_data.r.name;             // Name of the rule
 *   rsp_data.r.data;             // Expected data value (network endian)
 *   rsp_data.r.mask;             // Mask to be applied on frame data (network endian)
 *   rsp_data.r.offset;           // Offset of the data in the frame (network endian)
 *   rsp_data.r.invert;           // Invert match or not
 *   rsp_data.r.match_action;     // Action to be done on match
 *   rsp_data.r.next_rule_name;   // Next rule to be examined if match_action == FP_NEXT_RULE
 * @endcode
 * @note All data is provided in the network byte order.
 *
 * Action FPP_ACTION_QUERY_CONT
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY_CONT,    // Continue query of the table rules by the next rule
 *      .t.table_name = "table_name",       // Identifier of the table to be queried
 *   };
 * @endcode
 *
 * Response data is provided in the same form as for FPP_ACTION_QUERY action.
 * 
 * @hideinitializer
 */
#define FPP_CMD_FP_TABLE                    0xf220

/**
 * @def FPP_CMD_FP_RULE
 * @brief Administers the Flexible Parser rules
 * @details Each Flexible Parser rule consists of a condition specified by @c data, @c mask and @c offset triplet and
 *          action to be performed. If 32-bit frame data at given @c offset masked by @c mask is equal to the specified
 *          @c data masked by the same @c mask then the condition is true. An invert flag may be set to invert the condition
 *          result. The rule action may be either @c accept, @c reject or @c next_rule which means to continue with a specified rule.
 *
 *          The rule administering command may be one of the following actions:
 *           - @c FPP_ACTION_REGISTER: Create a new rule.
 *           - @c FPP_ACTION_DEREGISTER: Delete an existing rule.
 *           - @c FPP_ACTION_QUERY: Return the first rule (among all existing rules).
 *           - @c FPP_ACTION_QUERY_CONT: Return the next rule.
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_fp_rule_cmd_t cmd_data =
 *   {
 *      // Creates a new rule
 *      .action = FPP_ACTION_REGISTER,
 *      // Unique up-to-15-character rule identifier
 *      .r.rule_name = "rule_name",
 *      // 32-bit data to match with the frame data at given offset (network endian)
 *      .r.data = htonl(0x08000000),
 *      // 32-bit mask to apply on the frame data and .r.data before comparison (network endian)
 *      .r.mask = htonl(0xFFFF0000),
 *      // Offset of the frame data to be compared (network endian)
 *      .r.offset = htonl(12),
 *      // Invert match or not (values 0 or 1)
 *      .r.invert = 0,
 *      // How to calculate the offset
 *      .r.match_action = FP_OFFSET_FROM_L2_HEADER,
 *      // Action to be done on match
 *      .r.offset_from = FP_ACCEPT,
 *      // Identifier of the next rule to use when match_action == FP_NEXT_RULE
 *      .r.next_rule_name = "rule_name2"
 *   };
 * @endcode
 * This example is used to match and accept all IPv4 frames (16-bit value 0x0800 at bytes 12 and 13,
 * when starting bytes counting from 0).
 * @note All values are specified in the network byte order.
 * @warning It is forbidden to create rule loops using the @a next_rule feature.
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_fp_rule_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_DEREGISTER,   // Deletes an existing rule
 *      .r.rule_name = "rule_name",        // Identifier of the rule to be deleted
 *   };
 * @endcode
 *
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_rule_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY       // Start the rules query
 *   };
 * @endcode
 *
 * Response data type for queries: @ref fpp_fp_rule_cmd_t
 * 
 * Response data provided:
 * @code{.c}
 *   rsp_data.r.name;             // Name of the rule
 *   rsp_data.r.data;             // Expected data value (network endian)
 *   rsp_data.r.mask;             // Mask to be applied on frame data (network endian)
 *   rsp_data.r.offset;           // Offset of the data in the frame (network endian)
 *   rsp_data.r.invert;           // Invert match or not
 *   rsp_data.r.match_action;     // Action to be done on match
 *   rsp_data.r.next_rule_name;   // Next rule to be examined if match_action == FP_NEXT_RULE
 * @endcode
 * @note All data is provided in the network byte order.
 *
 * Action FPP_ACTION_QUERY_CONT
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_rule_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY_CONT    // Continue with the rules query
 *   };
 * @endcode
 *
 * Response data is provided in the same form as for FPP_ACTION_QUERY action.
 * 
 * @hideinitializer
 */
#define FPP_CMD_FP_RULE                     0xf221

#define FPP_ERR_FP_RULE_NOT_FOUND			0xf222

/**
 * @def FPP_ACTION_USE_RULE
 * @brief Flexible Parser specific 'use' action for FPP_CMD_FP_TABLE.
 * @hideinitializer
 */
#define FPP_ACTION_USE_RULE 10

/**
 * @def FPP_ACTION_UNUSE_RULE
 * @brief Flexible Parser specific 'unuse' action for FPP_CMD_FP_TABLE.
 * @hideinitializer
 */
#define FPP_ACTION_UNUSE_RULE 11

/**
 * @brief Specifies the Flexible Parser result on the rule match
 */
typedef enum CAL_PACKED
{
    FP_ACCEPT,   /**< Flexible parser result on rule match is ACCEPT */
    FP_REJECT,   /**< Flexible parser result on rule match is REJECT */
    FP_NEXT_RULE /**< On rule match continue matching by the specified rule */
} fpp_fp_rule_match_action_t;

/**
 * @brief Specifies how to calculate the frame data offset
 * @details The offset may be calculated either from the L2, L3 or L4 header beginning.
 *          The L2 header beginning is also the Ethernet frame beginning because the Ethernet
 *          frame begins with the L2 header. This offset is always valid however if the L3 or
 *          L4 header is not recognized then the rule is always skipped as not-matching.
 */
typedef enum CAL_PACKED
{
    FP_OFFSET_FROM_L2_HEADER = 2, /**< Calculate offset from the L2 header (frame beginning) */
    FP_OFFSET_FROM_L3_HEADER = 3, /**< Calculate offset from the L3 header */
    FP_OFFSET_FROM_L4_HEADER = 4  /**< Calculate offset from the L4 header */
} fpp_fp_offset_from_t;

/**
 * @brief Properties of the Flexible parser rule
 * @details The rule match can be described as:
 * @code{.c}
 *  ((frame_data[offset] & mask) == (data & mask)) ? match = true : match = false;
 *  match = (invert ? !match : match);
 * @endcode
 * Value of match being equal to true causes:
 * - Flexible Parser to stop and return ACCEPT
 * - Flexible Parser to stop and return REJECT
 * - Flexible Parser to set the next rule to rule specified in next_rule_name
 */
typedef struct CAL_PACKED fpp_fp_rule_props_tag
{
	/*	Unique identifier of the rule. It is a string up to 15 characters + '\0' */
    uint8_t rule_name[16];
	/*	Expected data (network endian) to be found in the frame to match the rule. */
    uint32_t data;
	/*	Mask (network endian) to be applied on both expected data and frame data */
    uint32_t mask;
	/*	Offset (network endian) of the data in the frame (from L2, L3, or L4 header
		- see @c offset_from) */
    uint16_t offset;
	/*	Invert the match result after match is calculated */
    uint8_t invert;
	/*	Specifies a rule to continue matching if this rule matches and the @c match_action
		is @c FP_NEXT_RULE */
    uint8_t next_rule_name[16];
	/*	Specifies the Flexible Parser behavior on rule match */
    fpp_fp_rule_match_action_t match_action;
	/*	Specifies layer from which header beginning is @c offset calculated */
    fpp_fp_offset_from_t offset_from;
} fpp_fp_rule_props_t;

/**
 * @brief Arguments for the FPP_CMD_FP_RULE command
 */
typedef struct CAL_PACKED fpp_fp_rule_cmd_tag
{
    uint16_t action;        /**< Action to be done */
    fpp_fp_rule_props_t r;  /**< Parameters of the rule */
} fpp_fp_rule_cmd_t;

/**
 * @brief Arguments for the FPP_CMD_FP_TABLE command
 */
typedef struct CAL_PACKED fpp_flexible_parser_table_cmd
{
    uint16_t action;                  /**< Action to be done */
    union
    {
        struct
        {
            uint8_t table_name[16];   /**< Name of the table to be administered by the action */
            uint8_t rule_name[16];    /**< Name of the rule to be added/removed to/from the table */
            uint16_t position;        /**< Position where to add rule (network endian) */
        } t;
        fpp_fp_rule_props_t r; /**< Properties of the rule - used as query result */
    };
} fpp_fp_table_cmd_t;


/**
 * @def FPP_CMD_FP_FLEXIBLE_FILTER
 * @brief Uses flexible parser to filter out frames from further processing.
 * @details Allows registration of a Flexible Parser table (see @ref FPP_CMD_FP_TABLE) as a
 *          filter which:
 *           - @c FPP_ACTION_REGISTER: Use the specified table as a Flexible Filter (replace old table by a new one if already configured).
 *           - @c FPP_ACTION_DEREGISTER: Disable Flexible Filter, no table will be used as Flexible Filter.
 *
 * The Flexible Filter examines received frames before any other processing and discards those which have
 * REJECT result from the configured Flexible Parser.
 *
 * See the @ref FPP_CMD_FP_TABLE for flexible parser behavior description.
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_filter_cmd_t cmd_data =
 *   {
 *      // Set the specified table as Flexible Filter
 *      .action = FPP_ACTION_REGISTER,
 *      // Name of the Flexible Parser table to be used to filter the frames
 *      .table_name = "table_name"
 *   }
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_filter_cmd_t cmd_data =
 *   {
 *      // Disable the Flexible Filter
 *      .action = FPP_ACTION_DEREGISTER,
 *   }
 * @endcode
 *
 * @hideinitializer
 */
#define FPP_CMD_FP_FLEXIBLE_FILTER 0xf225

/*
* @brief Arguments for the FPP_CMD_FP_FLEXIBLE_FILTER command
*/
typedef struct CAL_PACKED fpp_flexible_filter_cmd
{
    uint16_t action;         /**< Action to be done on Flexible Filter */
    uint8_t table_name[16];  /**< Name of the Flexible Parser table to be used */
} fpp_flexible_filter_cmd_t;

/**
 * @def FPP_CMD_DATA_BUF_PUT
 * @brief FCI command to send an arbitrary data to the accelerator
 * @details Command is intended to be used to send custom data to the accelerator.
 *          Format of the command argument is given by the @ref fpp_buf_cmd_t
 *          structure which also defines the maximum payload length. Subsequent
 *          commands are not successful until the accelerator reads and
 *          acknowledges the current request.
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_buf_cmd_t cmd_data =
 *   {
 *     // Specify buffer payload
 *     .payload = ...,
 *     // Payload length in number of bytes
 *     .len = ...,
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Data written and available to the accelerator
 *     - @c FPP_ERR_AGAIN: Previous command has not been finished yet
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 */
#define FPP_CMD_DATA_BUF_PUT		0xf300

/**
 * @def FPP_CMD_DATA_BUF_AVAIL
 * @brief Event reported when accelerator wants to send a data buffer to host
 * @details Indication of this event also carries the buffer payload and payload
 *          length. Both are available via the event callback arguments (see the
 *          callback type and arguments within description of @ref fci_register_cb()).
 */
#define FPP_CMD_DATA_BUF_AVAIL		0xf301

/**
 * @def FPP_ERR_AGAIN
 * @hideinitializer
 */
#define FPP_ERR_AGAIN				0xf302

/**
 * @def FPP_CMD_ENDPOINT_SHUTDOWN
 * @brief Notify client about endpoint shutdown event. 
 */
#define FPP_CMD_ENDPOINT_SHUTDOWN	0xf303

/**
 * @brief Argument structure for the FPP_CMD_DATA_BUF_PUT command
 */
typedef struct CAL_PACKED fpp_buf_cmd_tag
{
    uint8_t payload[64];	/**< The payload area */
    uint8_t len;			/**< Payload length in number of bytes */
    uint8_t reserved1;
    uint16_t reserved2;
} fpp_buf_cmd_t;

/**
 * @def FPP_CMD_SPD
 * @brief Configures the SPD (Security Policy Database) for IPsec
 * @note The feature is available only for some Premium firmware versions and it shall not be used
 *       with firmware not supporting the IPsec to avoid undefined behavior.
 * @details The command is connected with @c fpp_spd_cmd_t type and allows complete SPD management
 *          which involves insertion of an entry at a given position (@c FPP_ACTION_REGISTER), removal
 *          of an entry at a given position (@c FPP_ACTION_DEREGISTER) and reading the database data
 *          (@c FPP_ACTION_QUERY and @c FPP_ACTION_QUERY_CONT).
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * The FPP_ACTION_REGISTER action adds an entry at a given position into the SPD belonging
 * to a given physical interface. The SPD is created if the entry is the 1st one and the position
 * is ignored in such case. Creation of the SPD enables the IPsec processing for given interface.
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_spd_cmd_t cmd_data =
 *   {
 *      // Set the new rule in SPD
 *      .action = FPP_ACTION_REGISTER,
 *      // Name of the physical interface which SPD shall be modified
 *      .name = "emac0",
 *      // Add as a 4th rule (1st rule used position 0), current 4th rule will follow the newly added rule
 *      .position = 3,
 *      // Set the traffic matching criteria
 *      .saddr = 0xC0A80101, //192.168.1.1
 *      .daddr = 0xC0A80102, //192.168.1.2
 *      .protocol = 17,      //UDP
 *      .sport = 0,          //Source port - not set, see the .flags
 *      .dport = 0,          //Destination port - not set, see the .flags
 *      // Set ports as opaque i.e. ignored, missing FPP_SPD_FLAG_IPv6 means IPv4 traffic
 *      .flags = FPP_SPD_FLAG_SPORT_OPAQUE | FPP_SPD_FLAG_DPORT_OPAQUE
 *      .spi = 1,            //SPI to match in ESP or AH header (used only for action FPP_SPD_ACTION_PROCESS_DECODE)
 *      // Set action for matching traffic
 *      .spd_action = FPP_SPD_ACTION_PROCESS_DECODE, //Do IPsec decoding
 *      .sa_id = 1,          //HSE SAD entry ID to be used to process the traffic
 *   }
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * The FPP_ACTION_DEREGISTER action removes an entry at a given position in the SPD belonging
 * to a given physical interface. The SPD is destroyed if the entry is the last one which disables
 * the IPsec support on the given interface.
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_filter_cmd_t cmd_data =
 *   {
 *      // Disable the Flexible Filter
 *      .action = FPP_ACTION_DEREGISTER,
 *      // Name of the physical interface which SPD shall be modified
 *      .name = "emac0",
 *      // Remove the 4th rule (1st rule used position 0)
 *      .position = 3,
 *   }
 * @endcode
 *
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_spd_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY       // Start the rules query
 *      // Name of the physical interface which SPD shall be queried
 *      .name = "emac0",
 *   };
 * @endcode
 *
 * Response data type for queries: @ref fpp_spd_cmd_t
 *
 * Response data provided has the same format as FPP_ACTION_REGISTER action.
 * @note All data is provided in the network byte order.
 *
 * Action FPP_ACTION_QUERY_CONT
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_spd_cmd_t cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY_CONT    // Continue with the rules query
 *      // Name of the physical interface which SPD shall be queried
 *      .name = "emac0",
 *   };
 * @endcode
 * @hideinitializer
 */
#define FPP_CMD_SPD 0xf226

/**
* @brief Sets the action to be done for frames matching the SPD entry criteria
*/
typedef enum CAL_PACKED
{
	FPP_SPD_ACTION_INVALID = 0U,	/* Undefined action - do not set this one */
	FPP_SPD_ACTION_DISCARD,			/* Discard the frame */
	FPP_SPD_ACTION_BYPASS,			/* Bypass IPsec and forward normally */
	FPP_SPD_ACTION_PROCESS_ENCODE,	/* Process IPsec */
	FPP_SPD_ACTION_PROCESS_DECODE	/* Process IPsec */
} fpp_spd_action_t;

/**
* @brief Flags values to be used in fpp_spd_cmd_t structure .flags field.
*/
typedef enum CAL_PACKED
{
	FPP_SPD_FLAG_IPv6 = (1U << 1U),			/* IPv4 if not set, IPv6 if set */
	FPP_SPD_FLAG_SPORT_OPAQUE = (1U << 2U),	/* Do not match Source PORT */
	FPP_SPD_FLAG_DPORT_OPAQUE = (1U << 3U),	/* Do not match Destination PORT */
} fpp_spd_flags_t;

/**
 * @brief Argument structure for the FPP_CMD_SPD command
 */
typedef struct CAL_PACKED
{
	uint16_t action;			/**< Action */
	char name[IFNAMSIZ];		/**< Interface name */
	fpp_spd_flags_t flags;
	uint16_t position;			/**< Rule position (0 = 1st one, X = insert before Xth rule, if X > count then add as a last one) */
	uint32_t saddr[4];			/**< Source IP address (IPv4 uses only 1st word) */
	uint32_t daddr[4];			/**< Destination IP address (IPv4 uses only 1st word) */
	uint16_t sport;				/**< Source port */
	uint16_t dport;				/**< Destination port */
	uint8_t protocol;			/**< Protocol ID: TCP, UDP */
	uint32_t sa_id;				/**< SAD entry identifier (used only for actions SPD_ACT_PROCESS_ENCODE) */
	uint32_t spi;				/**< SPI to match if action is FPP_SPD_ACTION_PROCESS_DECODE */
	fpp_spd_action_t spd_action;	/**< Action to be done on the frame */
} fpp_spd_cmd_t;

/**
 * @def FPP_CMD_QOS_QUEUE
 * @brief Management of QoS queues
 * @details Command can be used with following `.action` values:
 *          - @c FPP_ACTION_UPDATE: Update queue configuration
 *          - @c FPP_ACTION_QUERY: Get queue properties
 *
 * Command Argument Type: @ref fpp_qos_queue_cmd_t
 *
 * Action FPP_ACTION_UPDATE
 * ------------------------
 * To update queue properties just set 
 *   - `fpp_qos_queue_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_queue_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_queue_cmd_t.id` to the queue ID.
 *
 * Rest of the fpp_qos_queue_cmd_t structure members will be considered to be used as the new
 * queue properties. It is recommended to use read-modify-write approach in combination with
 * @ref FPP_ACTION_QUERY.
 * 
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Get current queue properties. Set
 *   - `fpp_qos_queue_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_queue_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_queue_cmd_t.id` to the queue ID.
 *
 * Response data type for the query command is fpp_qos_scheduler_cmd_t.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Success.
 *     - @c FPP_ERR_QUEUE_NOT_FOUND: Queue not found.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Invalid argument/value.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_QUEUE			0xf400

/**
 * @def FPP_ERR_QOS_QUEUE_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_QOS_QUEUE_NOT_FOUND	0xf401

/**
 * @brief Argument of the @ref FPP_CMD_QOS_QUEUE command.
 */
typedef struct CAL_PACKED fpp_qos_queue_cmd
{
	/**	Action */
	uint16_t action;
	/**	Interface name */
	char if_name[IFNAMSIZ];
	/**	Queue ID. IDs start with 0 and maximum value depends on the number of
		available queues within the given interface `.if_name`. See @ref egress_qos. */
	uint8_t id;
	/**	Queue mode:
		- 0 - Disabled. Queue will drop all packets.
		- 1 - Default. HW implementation-specific. Normally not used.
		- 2 - Tail drop
		- 3 - WRED */
	uint8_t mode;
	/**	Minimum threshold (network endian). Value is `.mode`-specific:
		- Disabled, Default: n/a
		- Tail drop: n/a
		- WRED: Threshold in number of packets in the queue at which the WRED lowest
				drop probability zone starts, i.e. if queue fill level is below
				this threshold the drop probability is 0%.
	*/
	uint32_t min;
	/**	Maximum threshold (network endian). Value is `.mode`-specific:
		- Disabled, Default: n/a
		- Tail drop: The queue length in number of packets. Queue length
					 is the number of packets the queue can accommodate before
					 drops will occur.
		- WRED: Threshold in number of packets in the queue at which the WRED highest
				drop probability zone ends, i.e. if queue fill level is above this
				threshold the drop probability is 100%.
	*/
	uint32_t max;
	/** WRED drop probabilities for all probability zones in [%]. The lowest probability zone
		is `.zprob[0]`. Only valid for `.mode = WRED`. Value 255 means 'invalid'. Number of zones
		per queue is implementation-specific. See the @ref egress_qos. */
	uint8_t zprob[32];
} fpp_qos_queue_cmd_t;

/**
 * @def FPP_CMD_QOS_SCHEDULER
 * @brief Management of QoS scheduler
 * @details Command can be used with following `.action` values:
 *          - @c FPP_ACTION_UPDATE: Update scheduler configuration
 *          - @c FPP_ACTION_QUERY: Get scheduler properties
 *
 * Command Argument Type: @ref fpp_qos_scheduler_cmd_t
 *
 * Action FPP_ACTION_UPDATE
 * ------------------------
 * To update scheduler properties just set 
 *   - `fpp_qos_scheduler_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_scheduler_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_scheduler_cmd_t.id` to the scheduler ID.
 *
 * Rest of the fpp_qos_scheduler_cmd_t structure members will be considered to be used as the new
 * scheduler properties. It is recommended to use read-modify-write approach in combination with
 * @ref FPP_ACTION_QUERY.
 *
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Get current scheduler properties. Set
 *   - `fpp_qos_scheduler_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_scheduler_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_scheduler_cmd_t.id` to the scheduler ID.
 *
 * Response data type for the query command is fpp_qos_scheduler_cmd_t.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Success.
 *     - @c FPP_ERR_SCHEDULER_NOT_FOUND: Scheduler not found.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Invalid argument/value.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_SCHEDULER			0xf410

/**
 * @def FPP_ERR_QOS_SCHEDULER_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_QOS_SCHEDULER_NOT_FOUND	0xf411

/**
 * @brief Argument of the @ref FPP_CMD_QOS_SCHEDULER command.
 */
typedef struct CAL_PACKED fpp_qos_scheduler_cmd
{
	/**	Action */
	uint16_t action;
	/**	Name of physical interface owning the scheduler */
	char if_name[IFNAMSIZ];
	/**	Scheduler ID. IDs start with 0 and maximum value depends on the number of
		available schedulers within the given interface `.if_name`. See @ref egress_qos. */
	uint8_t id;
	/**	Scheduler mode:
		- 0 - Scheduler disabled
		- 1 - Data rate (payload length)
		- 2 - Packet rate (number of packets) */
	uint8_t mode;
	/**	Scheduler algorithm:
			- 0 - PQ (Priority Queue). Input with the highest priority
				  is serviced first. Input 0 has the @b lowest priority.
			- 1 - DWRR (Deficit Weighted Round Robin)
			- 2 - RR (Round Robin)
			- 3 - WRR (Weighted Round Robin) */
	uint8_t algo;
	/**	Input enable bitfield (network endian). When a bit `n` is
		set it means that scheduler input `n` is enabled and connected
		to traffic source defined by `.source[n]`. Number of inputs is
		implementation-specific. See the @ref egress_qos. */
	uint32_t input_en;
	/**	Input weight (network endian). Scheduler algorithm-specific:
			- PQ, RR - n/a
			- WRR, DWRR - Weight in units given by scheduler `.mode` */
	uint32_t input_w[32];
	/**	Traffic source ID per scheduler input. Scheduler traffic sources
		are implementation-specific. See the @ref egress_qos. */
	uint8_t input_src[32];
} fpp_qos_scheduler_cmd_t;

/**
 * @def FPP_CMD_QOS_SHAPER
 * @brief Management of QoS shaper
 * @details Command can be used with following `.action` values:
 *          - @c FPP_ACTION_UPDATE: Update scheduler configuration
 *          - @c FPP_ACTION_QUERY: Get scheduler properties
 *
 * Command Argument Type: @ref fpp_qos_shaper_cmd_t
 *
 * Action FPP_ACTION_UPDATE
 * ------------------------
 * To update scheduler properties just set 
 *   - `fpp_qos_shaper_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_shaper_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_shaper_cmd_t.id` to the shaper ID.
 *
 * Rest of the fpp_qos_shaper_cmd_t structure members will be considered to be used as the new
 * shaper properties. It is recommended to use read-modify-write approach in combination with
 * @ref FPP_ACTION_QUERY.
 * 
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Get current scheduler properties. Set
 *   - `fpp_qos_shaper_cmd_t.action` to @ref FPP_ACTION_QUERY
 *   - `fpp_qos_shaper_cmd_t.if_name` to name of the physical interface and
 *   - `fpp_qos_shaper_cmd_t.id` to the shaper ID.
 *
 * Response data type for the query command is fpp_qos_shaper_cmd_t.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Success.
 *     - @c FPP_ERR_SHAPER_NOT_FOUND: Shaper not found.
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Invalid argument/value.
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_SHAPER				0xf420

/**
 * @def FPP_ERR_QOS_SHAPER_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_QOS_SHAPER_NOT_FOUND	0xf421

/**
 * @brief Argument of the @ref FPP_CMD_QOS_SHAPER command.
 */
typedef struct CAL_PACKED fpp_qos_shaper_cmd
{
	/**	Action */
	uint16_t action;
	/**	Interface name */
	char if_name[IFNAMSIZ];
	/**	Shaper ID. IDs start with 0 and maximum value depends on the number of
		available shapers within the given interface `.if_name`. See @ref egress_qos. */
	uint8_t id;
	/**	Position of the shaper */
	uint8_t position;
	/**	Shaper mode:
		- 0 - Shaper disabled
		- 1 - Data rate. The `isl` is in units of bits-per-second and
			  `max_credit` with `min_credit` are numbers of bytes.
		- 2 - Packet rate. The `isl` is in units of packets-per-second
			  and `max_credit` with `min_credit` are number of packets.*/
	uint8_t mode;
	/**	Idle slope in units per second (network endian) */
	uint32_t isl;
	/**	Max credit (network endian) */
	int32_t max_credit;
	/**	Min credit (network endian) */
	int32_t min_credit;
} fpp_qos_shaper_cmd_t;

#endif /* FPP_EXT_H_ */

/** @}*/
