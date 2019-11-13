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
 * @addtogroup  dxgrLibFCI
 * @{
 *
 * @file        fpp_ext.h
 * @brief       Extension of the legacy fpp.h
 * @details     All FCI commands and related elements not present within the
 * 				legacy fpp.h shall be put into this file. All macro values
 * 				(uint16_t) shall have the upper nibble set to b1111 to ensure
 *				no conflicts with the legacy macro values.
 * @note		Documentation is part of libfci.h.
 */

#ifndef FPP_EXT_H_
#define FPP_EXT_H_

#define FPP_ERR_INTERNAL_FAILURE				0xffff

#define FPP_CMD_PHY_INTERFACE					0xf100
#define FPP_CMD_LOG_INTERFACE					0xf101

#define FPP_ERR_IF_ENTRY_ALREADY_REGISTERED		0xf103
#define FPP_ERR_IF_ENTRY_NOT_FOUND				0xf104
#define FPP_ERR_IF_EGRESS_DOESNT_EXIST			0xf105
#define FPP_ERR_IF_EGRESS_UPDATE_FAILED			0xf106
#define FPP_ERR_IF_MATCH_UPDATE_FAILED			0xf107
#define FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED		0xf108

#define FPP_ERR_IF_RESOURCE_ALREADY_LOCKED		0xf110
#define FPP_ERR_IF_WRONG_SESSION_ID				0xf111

#define FPP_CMD_IF_LOCK_SESSION				0x0015
#define FPP_CMD_IF_UNLOCK_SESSION			0x0016

/**
 * @brief	Interface flags
 */
typedef enum __attribute__((packed))
{
	FPP_IF_ENABLED = (1 << 0),		/*!< If set, interface is enabled */
	FPP_IF_PROMISC = (1 << 1),		/*!< If set, interface is promiscuous */
	FPP_IF_MATCH_OR = (1 << 3)		/*!< Result of match is logical OR of rules, else AND */
} fpp_if_flags_t;

/**
 * @brief	Physical if modes
 */
typedef enum __attribute__((packed))
{
	FPP_IF_OP_DISABLED = 0,		/*!< Disabled */
	FPP_IF_OP_DEFAULT = 1,		/*!< Default operational mode */
	FPP_IF_OP_BRIDGE = 2,		/*!< L2 bridge */
	FPP_IF_OP_ROUTER = 3,		/*!< L3 router */
	FPP_IF_OP_VLAN_BRIDGE = 4	/*!< L2 bridge with VLAN */
} fpp_phy_if_op_mode_t;


typedef enum __attribute__((packed))
{
	FPP_IF_MATCH_TYPE_ETH = (1 << 0),		/*!< Match ETH Packets */
	FPP_IF_MATCH_TYPE_VLAN = (1 << 1),		/*!< Match VLAN Tagged Packets */
	FPP_IF_MATCH_TYPE_PPPOE = (1 << 2),		/*!< Match PPPoE Packets */
	FPP_IF_MATCH_TYPE_ARP = (1 << 3),		/*!< Match ARP Packets */
	FPP_IF_MATCH_TYPE_MCAST = (1 << 4),		/*!< Match Multicast (L2) Packets */
	FPP_IF_MATCH_TYPE_IP = (1 << 5),		/*!< Match IP Packets */
	FPP_IF_MATCH_TYPE_IPV6 = (1 << 6),		/*!< Match IPv6 Packets */
	FPP_IF_MATCH_TYPE_IPV4 = (1 << 7),		/*!< Match IPv4 Packets */
	FPP_IF_MATCH_RESERVED = (1 << 8),		/*!< Reserved */
	FPP_IF_MATCH_TYPE_IPX = (1 << 9),		/*!< Match IPX Packets */
	FPP_IF_MATCH_TYPE_BCAST = (1 << 10),	/*!< Match Broadcast (L2) Packets */
	FPP_IF_MATCH_TYPE_UDP = (1 << 11),		/*!< Match UDP Packets */
	FPP_IF_MATCH_TYPE_TCP = (1 << 12),		/*!< Match TCP Packets */
	FPP_IF_MATCH_TYPE_ICMP = (1 << 13),		/*!< Match ICMP Packets */
	FPP_IF_MATCH_TYPE_IGMP = (1 << 14),		/*!< Match IGMP Packets */
	FPP_IF_MATCH_VLAN = (1 << 15),			/*!< Match VLAN ID */
	FPP_IF_MATCH_PROTO = (1 << 16),			/*!< Match IP Protocol */
	FPP_IF_MATCH_SPORT = (1 << 20),			/*!< Match L4 Source Port */
	FPP_IF_MATCH_DPORT = (1 << 21),			/*!< Match L4 Destination Port */
	FPP_IF_MATCH_SIP6 = (1 << 22),			/*!< Match Source IPv6 Address */
	FPP_IF_MATCH_DIP6 = (1 << 23),			/*!< Match Destination IPv6 Address */
	FPP_IF_MATCH_SIP = (1 << 24),			/*!< Match Source IPv4 Address */
	FPP_IF_MATCH_DIP = (1 << 25),			/*!< Match Destination IPv4 Address */
	FPP_IF_MATCH_ETHTYPE = (1 << 26),		/*!< Match EtherType */
	FPP_IF_MATCH_FP0 = (1 << 27),			/*!< Match Packets Accepted by Flexible Parser 0 */
	FPP_IF_MATCH_FP1 = (1 << 28),			/*!< Match Packets Accepted by Flexible Parser 1 */
	FPP_IF_MATCH_SMAC = (1 << 29),			/*!< Match Source MAC Address */
	FPP_IF_MATCH_DMAC = (1 << 30),			/*!< Match Destination MAC Address */
	/* Ensure proper size */
	FPP_IF_MATCH_MAX = (1 << 30)
} fpp_if_m_rules_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/* VLAN ID (FPP_IF_MATCH_VLAN) */
	uint16_t vlan;
	/* EtherType (FPP_IF_MATCH_ETHTYPE) */
	uint16_t ethtype;
	/* L4 source port number (FPP_IF_MATCH_SPORT) */
	uint16_t sport;
	/* L4 destination port number (FPP_IF_MATCH_DPORT) */
	uint16_t dport;
	/* Source and destination addresses */
	struct
	{
		/*	IPv4 (FPP_IF_MATCH_SIP, FPP_IF_MATCH_DIP) */
		struct
		{
			uint32_t sip;
			uint32_t dip;
		} v4;

		/*	IPv6 (FPP_IF_MATCH_SIP6, FPP_IF_MATCH_DIP6) */
		struct
		{
			uint32_t sip[4];
			uint32_t dip[4];
		} v6;
	};
	/* IP protocol (FPP_IF_MATCH_PROTO) */
	uint8_t proto;
	/* Source MAC Address (FPP_IF_MATCH_SMAC) */
	uint8_t smac[6];
	/* Destination MAC Address (FPP_IF_MATCH_DMAC) */
	uint8_t dmac[6];
} fpp_if_m_args_t;


typedef struct fpp_phy_if_cmd
{
	uint16_t action;
	char name[IFNAMSIZ];							/* Interface name */
	uint32_t id;									/* Interface ID */
	fpp_if_flags_t flags;							/* Interface flags */
	fpp_phy_if_op_mode_t mode;						/* Phy if mode */
	uint8_t mac_addr[6];							/* Phy if MAC*/
} fpp_phy_if_cmd_t;

typedef struct fpp_log_if_cmd
{
	uint16_t action;
	char name[IFNAMSIZ];							/* Interface name */
	uint32_t id;									/* Interface ID */
	char parent_name[IFNAMSIZ];						/* Parent physical interface name */
	uint32_t parent_id;								/* Parent physical interface ID */
	uint32_t egress;								/* Egress interfaces in the form of mask (to get egress id: egress & (1 < id))*/
	fpp_if_flags_t flags;							/* Interface flags from query or flags to be set */
	fpp_if_m_rules_t match;							/* Match rules from query or match rules to be set */
	fpp_if_m_args_t arguments;						/* Network format! Additional arguments for match rules*/
} fpp_log_if_cmd_t;


/**
 * @def FPP_CMD_L2BRIDGE_DOMAIN
 * @brief Creates a standard, VLAN-based L2 bridge domain
 * @details Standard domain can be used to include a set of physical interfaces and isolate them
 *          from another domains using VLAN. Command can be used with various `.action` values:
 *          - @c FPP_ACTION_REGISTER: Create a new bridge domain.
 *          - @c FPP_ACTION_DEREGISTER: Delete bridge domain.
 *          - @c FPP_ACTION_UPDATE: Update a bridge domain meaning that will rewrite domain properties except of VLAN ID.
 *          - @c FPP_ACTION_QUERY: Gets head of list of registered domains.
 *          - @c FPP_ACTION_QUERY_CONT: Get next item from list of registered domains. Shall be called after
 *               @c FPP_ACTION_QUERY was called. On each call it replies with parameters of next domain.
 *               It returns @c FPP_ERR_RT_ENTRY_NOT_FOUND when no more entries exist.
 *
 * Command Argument Type: struct @ref fpp_l2_bridge_domain_control_cmd (@c fpp_l2_bridge_domain_control_cmd_t)
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_domain_control_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_REGISTER,   // Register new bridge domain
 *     .vlan = ...,                     // VLAN ID associated with the domain (network endian)
 *     .ucast_hit = ...,                // Action to be taken when destination MAC address (uni-cast) of a packet
 *                                         matching the domain is found in the MAC table: 0 - Forward, 1 - Flood, 2 - Punt, 3 - Discard
 *     .ucast_miss = ...,               // Action to be taken when destination MAC address (uni-cast) of a packet
 *                                         matching the domain is not found in the MAC table.
 *     .mcast_hit = ...,                // Multicast hit action
 *     .mcast_miss = ...                // Multicast miss action
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain added
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_ALREADY_REGISTERED: Given domain already registered
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 *
 * Action FPP_ACTION_DEREGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_domain_control_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_DEREGISTER, // Delete bridge domain
 *     .vlan = ...,                     // VLAN ID associated with the domain to be deleted (network endian)
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain removed
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: Given domain not found
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 * Action FPP_ACTION_UPDATE
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_domain_control_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_DEREGISTER, // Delete bridge domain
 *     .vlan = ...,                     // VLAN ID associated with the domain to be updated (network endian)
 *     .ucast_hit = ...,                // New unicast hit action (0 - Forward, 1 - Flood, 2 - Punt, 3 - Discard)
 *     .ucast_miss = ...,               // New unicast miss action
 *     .mcast_hit = ...,                // New multicast hit action
 *     .mcast_miss = ...,               // New multicast miss action
 *     .if_list = ...,                  // New port list. Bitmask where every set bit represents ID of physical interface
 *                                         being member of the domain. For instance bit (1 << 3), if set, says that interface
 *                                         with ID=3 is member of the domain. Only valid interface IDs are accepted by the
 *                                         command. If flag is set, interface is added to the domain. If flag is not set and
 *                                         interface has been previously added, it is removed. The IDs are given by the related
 *                                         FCI endpoint and related networking HW. For exact values please see the HW/FW
 *                                         documentation.
 *     .untag_if_list = ...,            // Flags marking interfaces listed in @c if_list to be 'tagged' or 'untagged'. If
 *                                         respective flag is set, corresponding interface within the @c if_list is treated
 *                                         as VLAN tagged. Otherwise it is configured as 'untagged'. Note that only interfaces
 *                                         listed within the @c if_list are taken into account.
 *   };
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Domain updated
 *     - @c FPP_ERR_WRONG_COMMAND_PARAM: Unexpected argument
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: Given domain not found
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_domain_control_cmd_t cmd_data =
 *   {
 *     .action = ...    // Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 *   };
 * @endcode
 *
 * Response data type for queries: struct @ref fpp_l2_bridge_domain_control_cmd (@c fpp_l2_bridge_domain_control_cmd_t)
 *
 * Response data provided (all values in network byte order):
 * @code{.c}
 *     rsp_data.vlan;          // VLAN ID associated with domain (network endian)
 *     rsp_data.ucast_hit;     // Action to be taken when destination MAC address (uni-cast) of a packet
 *                                matching the domain is found in the MAC table: 0 - Forward, 1 - Flood, 2 - Punt, 3 - Discard
 *     rsp_data.ucast_miss;    // Action to be taken when destination MAC address (uni-cast) of a packet
 *                                matching the domain is not found in the MAC table.
 *     rsp_data.mcast_hit;     // Multicast hit action.
 *     rsp_data.mcast_miss;    // Multicast miss action.
 *     rsp_data.if_list;       // Bitmask where every set bit represents ID of physical interface being member of the domain.
 *                                For instance bit (1 << 3), if set, says that interface with ID=3 is member of the domain.
 *     rsp_data.untag_if_list; // Similar to @c if_list but this interfaces are configured to be VLAN 'untagged'.
 *     rsp_data.flags;         // See the fpp_l2_bridge_domain_flags_t.
 * @endcode
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Response buffer written
 *     - @c FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND: No more entries
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 */
#define FPP_CMD_L2BRIDGE_DOMAIN						0xf200

#define FPP_ERR_L2BRIDGE_DOMAIN_ALREADY_REGISTERED	0xf201
#define FPP_ERR_L2BRIDGE_DOMAIN_NOT_FOUND			0xf202

/**
 * @brief	L2 bridge domain flags
 */
typedef enum
{
	FPP_L2BR_DOMAIN_DEFAULT = (1 << 0),	/*!< Domain type is default */
	FPP_L2BR_DOMAIN_FALLBACK = (1 << 1)	/*!< Domain type is fallback */
} fpp_l2_bridge_domain_flags_t;

/**
 * @struct  fpp_l2_bridge_domain_control_cmd
 * @brief   Data structure to be used for command buffer for L2 bridge domain control commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with commands: @ref FPP_CMD_L2BRIDGE_DOMAIN.
 */
typedef struct fpp_l2_bridge_domain_control_cmd
{
	uint16_t action;	/*	Action to be executed (register, unregister, query, ...) */
	uint16_t vlan;		/*	VLAN ID associated with the bridge domain */
	uint8_t ucast_hit;
	uint8_t ucast_miss;
	uint8_t mcast_hit;
	uint8_t mcast_miss;
	uint32_t if_list;
	uint32_t untag_if_list;
	fpp_l2_bridge_domain_flags_t flags;
} __attribute__((__packed__)) fpp_l2_bridge_domain_control_cmd_t;

/**
 * @def FPP_CMD_FP_TABLE
 * @brief Administers the Flexible Parser tables
 * @details The Flexible Parser table is an ordered set of Flexible Parser rules which
 *          are matched in the order of appearance until match occurs or end of the table
 *          is reached. The following actions can be done on the table:
 *          - @c FPP_ACTION_REGISTER creates a new table with a given name
 *          - @c FPP_ACTION_DEREGISTER destroys an existing table
 *          - @c FPP_ACTION_USE_RULE adds a rule into the table at specified position
 *          - @c FPP_ACTION_UNUSE_RULE removes a rule from the table
 *          - @c FPP_ACTION_QUERY returns the first rule in the table
 *          - @c FPP_ACTION_QUERY_CONT returns the next rule in the table
 *
 * The Flexible Parser starts processing the table from the 1st rule in the table. If there is no match
 * the Flexible Parser always continues with the rule following the currently processed rule. 
 * The processing ends once rule match happens and the rule action is one of the FP_ACCEPT
 * or FP_REJECT and the respective value is returned. 
 * REJECT is also returned after the last rule in the table was processed without any match. 
 * The Flexible Parser may branch to arbitrary rule in the table if some rule matches and the 
 * action is FP_NEXT_RULE. Note that loops are forbidden. 
 *
 * See the FPP_CMD_FP_RULE and fpp_flexible_parser_rule_props for the detailed description of how
 * the rules are being matched.
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_REGISTER,    //Add a new table
 *      .t.table_name = "table_name",     //Unique up-to-15-character table identifier
 *   };
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_DEREGISTER,  //Remove an existing table
 *      .t.table_name = "table_name",     //Identifier of the table to be destroyed
 *   };
 * @endcode
 *
 * Action FPP_ACTION_USE_RULE
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_USE_RULE,    //Add existing rule into specified table
 *      .t.table_name = "table_name",     //Identifier of the table to add the rule
 *      .t.rule_name = "rule_name",       //Identifier of the rule to be added into the table
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
 *      .action = FPP_ACTION_UNUSE_RULE,  //Remove an existing table
 *      .t.rule_name = "rule_name",       //Identifier of the rule to be removed from the table
 *   };
 * @endcode
 *
 * Action FPP_ACTION_QUERY
 * -----------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_table_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_QUERY,       //Start query of the table rules
 *      .t.table_name = "table_name",     //Identifier of the table to be queried
 *   };
 * @endcode
 *
 * Response data is provided in a form
 * @code{.c}
 *   rsp_data.r.name;             // Name of the rule
 *   rsp_data.r.data;             // Expected data value
 *   rsp_data.r.mask;             // Mask to be applied on frame data
 *   rsp_data.r.offset;           // Offset of the data in the frame
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
 *      .action = FPP_ACTION_QUERY_CONT,    //Continue query of the table rules by the next rule
 *      .t.table_name = "table_name",       //Identifier of the table to be queried
 *   };
 * @endcode
 *
 * Response data is provided in the same form as for FPP_ACTION_QUERY action.
 */
#define FPP_CMD_FP_TABLE                    0xf220

/**
 * @def FPP_CMD_FP_RULE
 * @brief Administers the Flexible Parser rules
 * @details Each Flexible Parser rule consists of a condition specified by @a data, @a mask and @a offset triplet and
 *          action to be performed. If 32-bit frame data at given @a offset masked by @a mask is equal to the specified
 *          @a data masked by the same @a mask then the condition is true. An invert flag may be set to invert the condition
 *          result. The rule action may be either @a accept, @a reject or @a next_rule which means to continue with a specified rule.
 *
 *          The rule administering command may be one of the following actions
 *           - @c FPP_ACTION_REGISTER creates a new rule
 *           - @c FPP_ACTION_DEREGISTER deletes an existing rule
 *           - @c FPP_ACTION_QUERY returns the first rule (among all existing rules)
 *           - @c FPP_ACTION_QUERY_CONT returns the next rule
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_rule_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_REGISTER,     // Creates a new rule
 *      .r.rule_name = "rule_name",        // Unique up-to-15-character rule identifier
 *      .r.data = 0x08000000,              // 32-bit data to match with the frame data at given offset
 *      .r.mask = 0xFFFF0000,              // 32-bit mask to apply on the frame data and .r.data before comparison
 *      .r.offset = 12,                    // Offset of the frame data to be compared
 *      .r.invert = 0,                     // Invert match or not (values 0 or 1)
 *      .r.match_action = FP_OFFSET_FROM_L2_HEADER, // How to calculate the offset
 *      .r.offset_from = FP_ACCEPT,        // Action to be done on match
 *      .r.next_rule_name = "rule_name2"   // Identifier of the next rule to use when match_action == FP_NEXT_RULE
 *   };
 * @endcode
 * This example is used to match and accept all IPv4 frames (16-bit value 0x0800 at bytes 12 and 13, when starting bytes counting from 0).
 * @note All values are specified in the network byte order.
 * @warning It is forbidden to create rule loops using the @a next_rule feture.
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_parser_rule_cmd cmd_data =
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
 * Response data is provided in a form
 * @code{.c}
 *   rsp_data.r.name;             // Name of the rule
 *   rsp_data.r.data;             // Expected data value
 *   rsp_data.r.mask;             // Mask to be applied on frame data
 *   rsp_data.r.offset;           // Offset of the data in the frame
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
 */
#define FPP_CMD_FP_RULE                     0xf221

#define FPP_ERR_FP_RULE_NOT_FOUND			0xf222

#define FPP_ACTION_USE_RULE 10
#define FPP_ACTION_UNUSE_RULE 11

/**
 * @brief Specifies the Flexible Parser result on the rule match
 */
typedef enum
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
typedef enum
{
    FP_OFFSET_FROM_L2_HEADER = 2, /**< Calculate offset from the L2 header (frame beginning) */
    FP_OFFSET_FROM_L3_HEADER = 3, /**< Calculate offset from the L3 header */
    FP_OFFSET_FROM_L4_HEADER = 4  /**< Calculate offset from the L4 header */
} fpp_fp_offset_from_t;

/**
 * @brief Properties of the Flexible parser rule
 * @details The rule match can be described as:
 * @code{.c}
 *  ((frame_data[offset] & mask) == (data & mask)) ? match = true : match = false
 *  match = (invert ? !match : match)
 * @nocode
 * Value of match being equal to true causes:
 * - Flexible Parser to stop and return ACCEPT
 * - Flexible Parser to stop and return REJECT
 * - Flexible Parser to set the next rule to rule specified in next_rule_name
 */
typedef struct fpp_flexible_parser_rule_props
{
    uint8_t rule_name[16]; /**< Unique identifier of the rule. It is a string up to 15 characters + '\0' */
    uint32_t data;         /**< Expected data to be found in the frame to match the rule. */
    uint32_t mask;         /**< Mask to be applied on both expected data and frame data */
    uint16_t offset;       /**< Offset of the data in the frame (from L2, L3, or L4 header - see @c offset_from) */
    uint8_t invert;        /**< Invert the match result after match is calculated */
    uint8_t next_rule_name[16];              /**< Specifies a rule to continue matching if this rule
                                                  matches and the @c match_action is @c FP_NEXT_RULE*/
    fpp_fp_rule_match_action_t match_action; /**< Specifies the Flexible Parser behavior on rule match */
    fpp_fp_offset_from_t offset_from;        /**< Specifies layer from which header beginning is @c offset calculated */
} fpp_flexible_parser_rule_props;

/**
 * @brief Arguments for the FPP_CMD_FP_RULE command
 */
typedef struct fpp_flexible_parser_rule_cmd
{
    uint16_t action;                   /**< Action to be done */
    fpp_flexible_parser_rule_props r;  /**< Parameters of the rule */

} __attribute__((__packed__)) fpp_flexible_parser_rule_cmd;

/**
 * @brief Arguments for the FPP_CMD_FP_TABLE command
 */
typedef struct fpp_flexible_parser_table_cmd
{
    uint16_t action;                  /**< Action to be done */
    union
    {
        struct
        {
            uint8_t table_name[16];   /**< Name of the table to be administered by the action */
            uint8_t rule_name[16];    /**< Name of the rule to be added/removed to/from the table */
            uint16_t position;        /**< Position where to add rule */
        } t;
        fpp_flexible_parser_rule_props r; /**< Properties of the rule - used as query result */
    };
} fpp_flexible_parser_table_cmd;


/**
 * @def FPP_FP_CMD_FLEXIBLE_FILTER
 * @brief Uses flexible parser to filter out frames from further processing.
 * @details Allows registration of a Flexible Parser table (see FPP_CMD_FP_TABLE) as a
 *          filter which
 *           - @c FPP_ACTION_REGISTER Use the specified table as a Flexible Filter (replace old table by a new one if already configured)
 *           - @c FPP_ACTION_DEREGISTER Disable Flexible Filter, no table will be used as Flexible Filter.
 *
 * The Flexible Filter examines received frames before any other processing and discards those which have
 * REJECT result from the configured Flexible Parser.
 *
 * See the FPP_CMD_FP_TABLE for flexible parser behavior description.
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_filter_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_REGISTER,  // Set the specified table as Flexible Filter
 *      .table_name = "table_name"      // Name of the Flexible Parser table to be used to filter the frames
 *   }
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_flexible_filter_cmd cmd_data =
 *   {
 *      .action = FPP_ACTION_DEREGISTER,  // Disable the Flexible Filter
 *   }
 * @endcode
 *
 */
#define FPP_FP_CMD_FLEXIBLE_FILTER 0xf225

/*
* @brief Argumenst for the FPP_FP_CMD_FLEXIBLE_FILTER command
*/
typedef struct fpp_flexible_filter_cmd
{
    uint16_t action;         /**< Action to be done on Flexible Filter */
    uint8_t table_name[16];  /**< Name of the Flexible Parser table to be used */
} fpp_flexible_filter_cmd;


#endif /* FPP_EXT_H_ */

/** @}*/

