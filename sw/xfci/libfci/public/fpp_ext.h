/* =========================================================================
 *  Copyright 2018-2024 NXP
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

#define FPP_ERR_ENTRY_NOT_FOUND					0xf104
#define FPP_ERR_INTERNAL_FAILURE				0xffff

/* Size limit for the strings specifying mirror name. */
#define MIRROR_NAME_SIZE 16

/**
 * @def         FPP_CMD_PHY_IF
 * @brief       FCI command for management of physical interfaces.
 * @details     Related topics: @ref mgmt_phyif
 * @details     Related data types: @ref fpp_phy_if_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of a physical interface.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a physical interface query session and get properties
 *                   of the first physical interface from the internal list of physical interfaces.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next physical interface
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * @note
 * All operations with physical interfaces require exclusive lock of the interface database.
 * See @ref FPP_CMD_IF_LOCK_SESSION.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of a physical interface. It is recommended to use the read-modify-write
 * approach (see @ref mgmt_phyif). Some properties cannot be modified (see fpp_phy_if_cmd_t).
 * @code{.c}
 *  .............................................
 *  fpp_phy_if_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *    .name   = "...",              // Interface name (see chapter Physical Interface)
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_phy_if_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_PHY_IF, sizeof(fpp_phy_if_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_phy_if_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_phy_if_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_PHY_IF,
 *                  sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first physical interface from
 *  //  the internal list of physical interfaces.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_PHY_IF,
 *                  sizeof(fpp_phy_if_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next physical interface from
 *  //  the internal list of physical interfaces.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the physical interface query session (no more interfaces).
 *        - For other ACTIONs: Unknown (nonexistent) physical interface was requested.
 * - @c FPP_ERR_IF_WRONG_SESSION_ID <br>
 *        Some other client has the interface database locked for exclusive access.
 * - @c FPP_ERR_MIRROR_NOT_FOUND <br>
 *        Unknown (nonexistent) mirroring rule in the `.rx_mirrors` or `.tx_mirrors` property.
 * - @c FPP_ERR_FW_FEATURE_NOT_AVAILABLE <br>
 *        Attempted to modify properties which are not available (not enabled in FW).
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_PHY_IF					0xf100

/**
 * @def         FPP_CMD_LOG_IF
 * @brief       FCI command for management of logical interfaces.
 * @details     Related topics: @ref mgmt_logif
 * @details     Related data types: @ref fpp_log_if_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new logical interface.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing logical interface.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of a logical interface.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a logical interface query session and get properties
 *                   of the first logical interface from the internal collective list of all
 *                   logical interfaces (regardless of physical interface affiliation).
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next logical interface
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * @note
 * All operations with logical interfaces require exclusive lock of the interface database.
 * See @ref FPP_CMD_IF_LOCK_SESSION.
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new logical interface. The newly created interface is by default disabled and
 * without any configuration. For configuration, see the following FPP_ACTION_UPDATE.
 * @code{.c}
 *  .............................................
 *  fpp_log_if_cmd_t cmd_to_fci =
 *  {
 *    .action      = FPP_ACTION_REGISTER,  // Action
 *    .name        = "...",                // Interface name (user-defined)
 *    .parent_name = "..."                 // Parent physical interface name
 *                                         // (see chapter Physical Interface)
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 * @warning Do not create multiple logical interfaces with the same name.
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing logical interface.
 * @code{.c}
 *  .............................................
 *  fpp_log_if_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .name   = "..."                   // Name of an existing logical interface.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of a logical interface. It is recommended to use the read-modify-write
 * approach (see @ref mgmt_logif). Some properties cannot be modified (see fpp_log_if_cmd_t).
 * @code{.c}
 *  .............................................
 *  fpp_log_if_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *    .name   = "...",              // Name of an existing logical interface.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_log_if_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a logical interface.
 * @code{.c}
 *  .............................................
 *  fpp_log_if_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_log_if_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_LOG_IF,
 *                  sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first logical interface from
 *  //  the internal collective list of all logical interfaces.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_LOG_IF,
 *                  sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next logical interface from
 *  //  the internal collective list of all logical interfaces.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the logical interface query session (no more interfaces).
 *        - For other ACTIONs: Unknown (nonexistent) logical interface was requested.
 * - @c FPP_ERR_IF_ENTRY_ALREADY_REGISTERED <br>
 *        Requested logical interface already exists (is already registered).
 * - @c FPP_ERR_IF_WRONG_SESSION_ID <br>
 *        Some other client has the interface database locked for exclusive access.
 * - @c FPP_ERR_IF_RESOURCE_ALREADY_LOCKED <br>
 *        Same as FPP_ERR_IF_WRONG_SESSION_ID.
 * - @c FPP_ERR_IF_MATCH_UPDATE_FAILED <br>
 *        Update of match flags has failed.
 * - @c FPP_ERR_IF_EGRESS_UPDATE_FAILED <br>
 *        Update of the `.egress` bitset has failed.
 * - @c FPP_ERR_IF_EGRESS_DOESNT_EXIST <br>
 *        Invalid (nonexistent) egress physical interface in the `.egress` bitset.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_LOG_IF					0xf101

/**
 * @def         FPP_ERR_IF_ENTRY_ALREADY_REGISTERED
 * @hideinitializer
 */
#define FPP_ERR_IF_ENTRY_ALREADY_REGISTERED		0xf103

/**
 * @def         FPP_ERR_IF_ENTRY_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_IF_ENTRY_NOT_FOUND				0xf104

/**
 * @def         FPP_ERR_IF_EGRESS_DOESNT_EXIST
 * @hideinitializer
 */
#define FPP_ERR_IF_EGRESS_DOESNT_EXIST			0xf105

/**
 * @def         FPP_ERR_IF_EGRESS_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_EGRESS_UPDATE_FAILED			0xf106

/**
 * @def         FPP_ERR_IF_MATCH_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_MATCH_UPDATE_FAILED			0xf107

/**
 * @def         FPP_ERR_IF_OP_UPDATE_FAILED
 * @hideinitializer
 */
#define FPP_ERR_IF_OP_UPDATE_FAILED				0xf108

/**
 * @def         FPP_ERR_IF_OP_CANNOT_CREATE
 * @hideinitializer
 */
#define FPP_ERR_IF_OP_CANNOT_CREATE				0xf109

/**
 * @def         FPP_ERR_IF_NOT_SUPPORTED
 * @hideinitializer
 */
#define FPP_ERR_IF_NOT_SUPPORTED				0xf117

/**
 * @def         FPP_ERR_IF_RESOURCE_ALREADY_LOCKED
 * @hideinitializer
 */
#define FPP_ERR_IF_RESOURCE_ALREADY_LOCKED		0xf110

/**
 * @def         FPP_ERR_IF_WRONG_SESSION_ID
 * @hideinitializer
 */
#define FPP_ERR_IF_WRONG_SESSION_ID				0xf111

/**
 * @def         FPP_CMD_IF_LOCK_SESSION
 * @brief       FCI command to get exclusive access to interface database.
 * @details     Related topics: @ref mgmt_phyif, @ref mgmt_logif, @ref flex_router
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_RESOURCE_ALREADY_LOCKED <br>
 *        Some other client has the interface database locked for exclusive access.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_IF_LOCK_SESSION					0x0015

/**
 * @def         FPP_CMD_IF_UNLOCK_SESSION
 * @brief       FCI command to cancel exclusive access to interface database.
 * @details     Related topics: @ref mgmt_phyif, @ref mgmt_logif, @ref flex_router
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_WRONG_SESSION_ID <br>
 *        Either the database is not locked, or it is currently locked by some other client.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_IF_UNLOCK_SESSION				0x0016

/**
 * @brief       Interface flags
 * @details     Related data types: @ref fpp_phy_if_cmd_t, @ref fpp_log_if_cmd_t
 * @details     Some of these flags are applicable only for physical interfaces [phyif],
 *              some are applicable only for logical interfaces [logif] and some are applicable
 *              for both [phyif,logif].
 */
typedef enum CAL_PACKED
{
    FPP_IF_ENABLED = (1 << 0),          /**< [phyif,logif] <br>
                                             If set, the interface is enabled. */
    FPP_IF_PROMISC = (1 << 1),          /**< [phyif,logif] <br>
                                             If set, the interface is configured as promiscuous.
                                             - promiscuous phyif: all ingress traffic is accepted, regardless of destination MAC.
                                             - promiscuous logif: all inspected traffic is accepted, regardless of active match rules. */
    FPP_IF_FF_ALL_TCP = (1 << 2),       /**< [phyif] <br>
                                             Special handling of ingress TCP SYN|FIN|RST packets in routing process.
                                             Applicable only when the interface uses mode which involves traffic 
                                             routing (e.g. FPP_IF_OP_ROUTER).
                                             If set, ingress TCP SYN|FIN|RST packets which match some conntrack are
                                             fast-forwarded as usual (according to the matching conntrack). This is default behavior.
                                             If @not set, ingress TCP SYN|FIN|RST packets which match some conntrack are @not 
                                             fast-forwarded. Instead, they are passed to the default logical interface. */
    FPP_IF_MATCH_OR = (1 << 3),         /**< [logif] <br>
                                             If multiple match rules are active and this flag is set,
                                             then the final result of a match process is a logical OR of the rules.
                                             If this flag is @b not set, then the final result is a logical AND of the rules. */
    FPP_IF_DISCARD = (1 << 4),          /**< [logif] <br>
                                             If set, discard matching frames. */
    FPP_IF_MIRROR = (1 << 5),           /* DEPRECATED. Do not use.
                                             [phyif] <br>
                                             If set, mirroring is enabled. */
    FPP_IF_LOADBALANCE = (1 << 6),      /* DEPRECATED. Do not use.
                                             [phyif] <br>
                                             If set, the interface is a part of a loadbalance bucket. */
    FPP_IF_VLAN_CONF_CHECK = (1 << 7),  /**< [phyif] <br>
                                             If set, the interface enforces a strict VLAN conformance check. */
    FPP_IF_PTP_CONF_CHECK = (1 << 8),   /**< [phyif] <br>
                                             If set, the interface enforces a strict PTP conformance check. */
    FPP_IF_PTP_PROMISC = (1 << 9),      /**< [phyif] <br>
                                             If set, then PTP traffic is accepted even if the FPP_IF_VLAN_CONF_CHECK is set. */
    FPP_IF_ALLOW_Q_IN_Q = (1 << 11),    /**< [phyif] <br>
                                             If set, the interface accepts QinQ-tagged traffic. */
    FPP_IF_DISCARD_TTL = (1 << 12),     /**< [phyif] <br>
                                             If set, then packets with TTL<2 are automatically discarded.
                                             If @b not set, then packets with TTL<2 are passed to the default logical interface. */
    FPP_IF_MAX = (int)(1U << 31U)
} fpp_if_flags_t;

/**
 * @brief       Physical interface operation mode.
 * @details     Related data types: @ref fpp_phy_if_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_IF_OP_DEFAULT = 0,          /**< Default operation mode */
    FPP_IF_OP_VLAN_BRIDGE = 1,      /**< @ref l2_bridge, VLAN aware version */
    FPP_IF_OP_ROUTER = 2,           /**< @ref l3_router */
    FPP_IF_OP_FLEXIBLE_ROUTER = 3,  /**< @ref flex_router */
    FPP_IF_OP_L2L3_VLAN_BRIDGE = 4, /**< @ref l2l3_bridge, VLAN-aware version */
} fpp_phy_if_op_mode_t;

/**
 * @brief       Match rules.
 * @details     Related data types: @ref fpp_log_if_cmd_t, @ref fpp_if_m_args_t
 * @note        L2/L3/L4 are layers of the OSI model.
 */
typedef enum CAL_PACKED
{
    FPP_IF_MATCH_TYPE_ETH = (1 << 0),     /**< Match ETH packets */
    FPP_IF_MATCH_TYPE_VLAN = (1 << 1),    /**< Match VLAN tagged packets */
    FPP_IF_MATCH_TYPE_PPPOE = (1 << 2),   /**< Match PPPoE packets */
    FPP_IF_MATCH_TYPE_ARP = (1 << 3),     /**< Match ARP packets */
    FPP_IF_MATCH_TYPE_MCAST = (1 << 4),   /**< Match multicast (L2) packets */
    FPP_IF_MATCH_TYPE_IPV4 = (1 << 5),    /**< Match IPv4 packets */
    FPP_IF_MATCH_TYPE_IPV6 = (1 << 6),    /**< Match IPv6 packets */
    FPP_IF_MATCH_RESERVED7 = (1 << 7),    /**< Reserved */
    FPP_IF_MATCH_RESERVED8 = (1 << 8),    /**< Reserved */
    FPP_IF_MATCH_TYPE_IPX = (1 << 9),     /**< Match IPX packets */
    FPP_IF_MATCH_TYPE_BCAST = (1 << 10),  /**< Match L2 broadcast packets */
    FPP_IF_MATCH_TYPE_UDP = (1 << 11),    /**< Match UDP packets */
    FPP_IF_MATCH_TYPE_TCP = (1 << 12),    /**< Match TCP packets */
    FPP_IF_MATCH_TYPE_ICMP = (1 << 13),   /**< Match ICMP packets */
    FPP_IF_MATCH_TYPE_IGMP = (1 << 14),   /**< Match IGMP packets */
    FPP_IF_MATCH_VLAN = (1 << 15),        /**< Match VLAN ID (see fpp_if_m_args_t) */
    FPP_IF_MATCH_PROTO = (1 << 16),       /**< Match IP Protocol Number (protocol ID) See fpp_if_m_args_t. */
    FPP_IF_MATCH_SPORT = (1 << 20),       /**< Match L4 source port (see fpp_if_m_args_t) */
    FPP_IF_MATCH_DPORT = (1 << 21),       /**< Match L4 destination port (see fpp_if_m_args_t) */
    FPP_IF_MATCH_SIP6 = (1 << 22),        /**< Match source IPv6 address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_DIP6 = (1 << 23),        /**< Match destination IPv6 address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_SIP = (1 << 24),         /**< Match source IPv4 address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_DIP = (1 << 25),         /**< Match destination IPv4 address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_ETHTYPE = (1 << 26),     /**< Match EtherType (see fpp_if_m_args_t) */
    FPP_IF_MATCH_FP0 = (1 << 27),         /**< Match Ethernet frames accepted by Flexible Parser 0 (see fpp_if_m_args_t) */
    FPP_IF_MATCH_FP1 = (1 << 28),         /**< Match Ethernet frames accepted by Flexible Parser 1 (see fpp_if_m_args_t) */
    FPP_IF_MATCH_SMAC = (1 << 29),        /**< Match source MAC address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_DMAC = (1 << 30),        /**< Match destination MAC address (see fpp_if_m_args_t) */
    FPP_IF_MATCH_HIF_COOKIE = (int)(1U << 31U),  /**< Match HIF header cookie. HIF header cookie is a part of internal overhead data.
                                                      It is attached to traffic data by a host's PFE driver. */

    /* Ensure proper size */
    FPP_IF_MATCH_MAX = (int)(1U << 31U)
} fpp_if_m_rules_t;

/**
 * @brief       Match rules arguments.
 * @details     Related data types: @ref fpp_log_if_cmd_t, @ref fpp_if_m_rules_t
 * @details     Each value is an argument for some match rule.
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_if_m_args_t
 */
/* [fpp_if_m_args_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t vlan;     /*< VLAN ID. [NBO]. See FPP_IF_MATCH_VLAN. */
    uint16_t ethtype;  /*< EtherType. [NBO]. See FPP_IF_MATCH_ETHTYPE. */
    uint16_t sport;    /*< L4 source port. [NBO]. See FPP_IF_MATCH_SPORT. */
    uint16_t dport;    /*< L4 destination port [NBO]. See FPP_IF_MATCH_DPORT. */

    /* Source and destination IP addresses */
    struct
    {
        struct
        {
            uint32_t sip;     /*< IPv4 source address. [NBO]. See FPP_IF_MATCH_SIP. */
            uint32_t dip;     /*< IPv4 destination address. [NBO]. See FPP_IF_MATCH_DIP. */
        } v4;

        struct
        {
            uint32_t sip[4];  /*< IPv6 source address. [NBO]. See FPP_IF_MATCH_SIP6. */
            uint32_t dip[4];  /*< IPv6 destination address. [NBO]. See FPP_IF_MATCH_DIP6. */
        } v6;
    } ipv;

    uint8_t proto;        /*< IP Protocol Number (protocol ID). See FPP_IF_MATCH_PROTO. */
    uint8_t smac[6];      /*< Source MAC Address. See FPP_IF_MATCH_SMAC. */
    uint8_t dmac[6];      /*< Destination MAC Address. See FPP_IF_MATCH_DMAC. */
    char fp_table0[16];   /*< Flexible Parser table 0 (name). See FPP_IF_MATCH_FP0. */
    char fp_table1[16];   /*< Flexible Parser table 1 (name). See FPP_IF_MATCH_FP1. */
    uint32_t hif_cookie;  /*< HIF header cookie. [NBO]. See FPP_IF_MATCH_HIF_COOKIE. */
} fpp_if_m_args_t;
/* [fpp_if_m_args_t] */

/**
 * @brief       Physical interface statistics.
 * @details     Related data types: @ref fpp_phy_if_cmd_t
 * @note        @b All values are in a network byte order [@b NBO].
 *
 * @snippet     fpp_ext.h  fpp_phy_if_stats_t
 */
/* [fpp_phy_if_stats_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint32_t ingress;    /*< Count of ingress frames for the given interface. */
    uint32_t egress;     /*< Count of egress frames for the given interface. */
    uint32_t malformed;  /*< Count of ingress frames with detected error (e.g. checksum). */
    uint32_t discarded;  /*< Count of ingress frames which were discarded. */
} fpp_phy_if_stats_t;
/* [fpp_phy_if_stats_t] */

/**
 * @brief       Logical interface statistics.
 * @details     Related data types: @ref fpp_log_if_cmd_t
 * @note        @b All values are in a network byte order [@b NBO].
 *
 * @snippet     fpp_ext.h  fpp_algo_stats_t
 */
/* [fpp_algo_stats_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint32_t processed;  /*< Count of frames processed (regardless of the result). */
    uint32_t accepted;   /*< Count of frames matching the selection criteria. */
    uint32_t rejected;   /*< Count of frames not matching the selection criteria. */
    uint32_t discarded;  /*< Count of frames marked to be dropped. */
} fpp_algo_stats_t;
/* [fpp_algo_stats_t] */

/**
 * @brief       Physical interface blocking state.
 * @details     Related data types: @ref fpp_phy_if_cmd_t
 * @details     Used when a physical interface is configured in a Bridge-like mode.
 *              See @ref l2_bridge and @ref l2l3_bridge. Affects the following Bridge-related
 *              capabilities of a physical interface:
 *              - Learning of MAC addresses from the interface's ingress traffic.
 *              - Forwarding of the interface's ingress traffic.
 */
typedef enum CAL_PACKED
{
    BS_NORMAL = 0,       /**< Learning and forwarding enabled. */
    BS_BLOCKED = 1,      /**< Learning and forwarding disabled. */
    BS_LEARN_ONLY = 2,   /**< Learning enabled, forwarding disabled. */
    BS_FORWARD_ONLY = 3  /**< Learning disabled, forwarding enabled. <br>
                              Traffic is forwarded only if its both source and destination MAC addresses
                              are known to the bridge. */
} fpp_phy_if_block_state_t;

/* Number of mirrors which can be configured per rx/tx on a physical interface.
   The value is equal to the number supported by the firmware. */
#define FPP_MIRRORS_CNT 2U

/**
 * @brief       Data structure for a physical interface.
 * @details     Related FCI commands: @ref FPP_CMD_PHY_IF
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_phy_if_cmd_t
 */
/* [fpp_phy_if_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;            /*< Action */
    char name[IFNAMSIZ];        /*< Interface name. [ro] */
    uint32_t id;                /*< Interface ID. [NBO,ro] */
    fpp_if_flags_t flags;       /*< Interface flags. [NBO]. A bitset. */
    fpp_phy_if_op_mode_t mode;  /*< Interface mode. */
    fpp_phy_if_block_state_t block_state;  /*< Interface blocking state. */
    fpp_phy_if_stats_t stats;   /*< Physical interface statistics. [ro] */

    /* Names of associated mirroring rules for ingress traffic. See FPP_CMD_MIRROR.
       Empty string at given position == position is disabled. */
    char rx_mirrors[FPP_MIRRORS_CNT][MIRROR_NAME_SIZE];

    /* Names of associated mirroring rules for egress traffic. See FPP_CMD_MIRROR.
       Empty string at given position == position is disabled. */
    char tx_mirrors[FPP_MIRRORS_CNT][MIRROR_NAME_SIZE];

    char ftable[16];    /*< Name of a Flexible Parser table which shall be used
                            as a Flexible Filter of this physical interface.
                            Empty string == Flexible filter is disabled.
                            See Flexible Parser for more info. */

    char ptp_mgmt_if[IFNAMSIZ];  /*< Name of a physical interface which serves as
                                     a PTP management interface.
                                     Empty string == disabled */

} fpp_phy_if_cmd_t;
/* [fpp_phy_if_cmd_t] */

/**
 * @brief       Data structure for a logical interface.
 * @details     Related FCI commands: @ref FPP_CMD_LOG_IF
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_log_if_cmd_t
 */
/* [fpp_log_if_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;            /*< Action */
    uint8_t res[2];             /*< RESERVED (do not use) */
    char name[IFNAMSIZ];        /*< Interface name. [ro] */
    uint32_t id;                /*< Interface ID. [NBO,ro] */
    char parent_name[IFNAMSIZ]; /*< Parent physical interface name. [ro] */
    uint32_t parent_id;         /*< Parent physical interface ID. [NBO,ro] */

    uint32_t egress;            /*< Egress physical interfaces. [NBO]. A bitset.
                                    Each physical interface is represented by a bitflag.
                                    Conversion between a physical interface ID and a corr-
                                    esponding bitflag is (1uL << "physical interface ID"). */

    fpp_if_flags_t flags;       /*< Interface flags. [NBO]. A bitset. */
    fpp_if_m_rules_t match;     /*< Match rules. [NBO]. A bitset. */
    fpp_if_m_args_t CAL_PACKED_ALIGNED(4) arguments;  /*< Match rules arguments. */
    fpp_algo_stats_t CAL_PACKED_ALIGNED(4) stats;     /*< Logical interface statistics [ro] */
} fpp_log_if_cmd_t;
/* [fpp_log_if_cmd_t] */

/**
 * @def         FPP_CMD_IF_MAC
 * @brief       FCI command for management of interface MAC addresses.
 * @details     Related topics: @ref mgmt_phyif
 * @details     Related data types: @ref fpp_if_mac_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Add a new MAC address to an interface.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove an existing MAC address from an interface.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a MAC address query session and get
 *                   the first MAC address of the requested interface.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get the next MAC address
 *                   of the requested interface. Intended to be called in a loop
 *                   (to iterate through the list).
 *
 * @note
 * All operations with interface MAC addresses require exclusive lock of the interface database.
 * See @ref FPP_CMD_IF_LOCK_SESSION.
 *
 * @note
 * MAC address management is available only for @b emac physical interfaces.
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Add a new MAC address to emac physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_if_mac_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *    .name   = "...",                // Physical interface name
 *    .mac    = {...}                 // Physical interface MAC
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IF_MAC, sizeof(fpp_if_mac_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove an existing MAC address from emac physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_if_mac_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .name   = "...",                  // Physical interface name
 *    .mac    = {...}                   // Physical interface MAC
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IF_MAC, sizeof(fpp_if_mac_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get MAC addresses of a requested emac physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_if_mac_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *    .name   = "...",            // Physical interface name
 *  };
 *
 *  fpp_if_mac_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_IF_MAC,
 *                  sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds the first MAC address of the requested physical interface.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_IF_MAC,
 *                  sizeof(fpp_if_mac_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds the next MAC address of the requested physical interface.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_MAC_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the MAC address query session (no more MAC addresses).
 *        - For other ACTIONs: Unknown (nonexistent) MAC address was requested.
 * - @c FPP_ERR_IF_MAC_ALREADY_REGISTERED <br>
 *        Requested MAC address already exists (is already registered).
 * - @c FPP_ERR_IF_ENTRY_NOT_FOUND <br>
 *        Unknown (nonexistent) physical interface was requested.
 * - @c FPP_ERR_IF_NOT_SUPPORTED <br>
 *        Requested physical interface does not support MAC address management.
 * - @c FPP_ERR_IF_WRONG_SESSION_ID <br>
 *        Some other client has the interface database locked for exclusive access.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_IF_MAC							0xf120

#define FPP_ERR_IF_MAC_ALREADY_REGISTERED		0xf121
#define FPP_ERR_IF_MAC_NOT_FOUND				0xf122

/**
 * @brief       Data structure for interface MAC address.
 * @details     Related FCI commands: @ref FPP_CMD_IF_MAC
 *
 * @snippet     fpp_ext.h  fpp_if_mac_cmd_t
 */
/* [fpp_if_mac_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t action;      /*< Action */
    char name[IFNAMSIZ];  /*< Physical interface name. */
    uint8_t mac[6];       /*< Physical interface MAC. */
} fpp_if_mac_cmd_t;
/* [fpp_if_mac_cmd_t] */

/**
 * @def         FPP_CMD_MIRROR
 * @brief       FCI command for management of interface mirroring rules.
 * @details     Related topics: @ref if_mgmt
 * @details     Related data types: @ref fpp_mirror_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new mirroring rule.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing mirroring rule.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of a mirroring rule.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a mirroring rule query session and get properties
 *                   of the first mirroring rule from the internal list of mirroring rules.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next mirroring rule
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new mirroring rule. When creating a new mirroring rule, it is also possible to
 * simultaneously set its properties (using the same rules which apply to @ref FPP_ACTION_UPDATE).
 * @code{.c}
 *  .............................................
 *  fpp_mirror_cmd_t cmd_to_fci =
 *  {
 *    .action        = FPP_ACTION_REGISTER, // Action
 *    .name          = "...",               // Name of the mirroring rule.
 *    .egress_phy_if = "...",               // Name of the physical interface where to mirror.
 *
 *    // optional
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing mirroring rule.
 * @code{.c}
 *  .............................................
 *  fpp_mirror_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .name   = "...",                  // Name of the mirroring rule.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of a mirroring rule. It is recommended to use the read-modify-write
 * approach. Some properties cannot be modified (see fpp_mirror_cmd_t).
 * @code{.c}
 *  .............................................
 *  fpp_mirror_cmd_t cmd_to_fci =
 *  {
 *    .action        = FPP_ACTION_REGISTER, // Action
 *    .name          = "...",               // Name of the mirroring rule.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_mirror_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_MIRROR, sizeof(fpp_mirror_cmd_t),
 *                                         (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a mirroring rule.
 * @code{.c}
 *  .............................................
 *  fpp_mirror_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_mirror_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_MIRROR,
 *                  sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first mirroring rule from
 *  //  the internal list of mirroring rules.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_MIRROR,
 *                  sizeof(fpp_mirror_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next mirroring rule from
 *  //  the internal list of mirroring rules.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_MIRROR_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the mirroring rule query session (no more mirroring rules).
 *        - For other ACTIONs: Unknown (nonexistent) mirroring rule was requested.
 * - @c FPP_ERR_MIRROR_ALREADY_REGISTERED <br>
 *        Requested mirroring rule already exists (is already registered).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_IF_ENTRY_NOT_FOUND <br>
 *        Unknown (nonexistent) physical interface in the `.egress_phy_if` property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_MIRROR						0xf130

#define FPP_ERR_MIRROR_ALREADY_REGISTERED	0xf131
#define FPP_ERR_MIRROR_NOT_FOUND			0xf132
#define FPP_ERR_MIRROR_CURRENTLY_UTILIZED	0xf133

/**
 * @brief       Mirroring rule modification actions.
 * @details     Related data types: @ref fpp_mirror_cmd_t, @ref fpp_modify_args_t
 */
typedef enum CAL_PACKED
{
    MODIFY_ACT_NONE = 0U,                   /**< No action to be done. */
    MODIFY_ACT_ADD_VLAN_HDR = (1U << 1),    /**< Construct/Update outer VLAN Header. */

    /* Ensure proper size */
    MODIFY_ACT_INVALID = (int)(1U << 31)
} fpp_modify_actions_t;

/**
 * @brief       Arguments for mirroring rule modification actions.
 * @details     Related data types: @ref fpp_mirror_cmd_t, @ref fpp_modify_actions_t
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_modify_args_t
 */
/* [fpp_modify_args_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t vlan;  /*< VLAN ID to be used by MODIFY_ACT_ADD_VLAN_HDR. [NBO] */
} fpp_modify_args_t;
/* [fpp_modify_args_t] */

/**
 * @brief       Data structure for interface mirroring rule.
 * @details     Related FCI commands: @ref FPP_CMD_MIRROR
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_mirror_cmd_t
 */
/* [fpp_mirror_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;               /*< Action */
    char name[MIRROR_NAME_SIZE];   /*< Name of the mirroring rule. [ro] */
    char egress_phy_if[IFNAMSIZ];  /*< Name of the physical interface where to mirror. */

    char filter_table_name[16];	   /*< Name of a Flexible Parser table that can be used
                                       to filter which frames to mirror.
                                       Empty string == disabled (no filtering).
                                       See Flexible Parser for more info. */

    fpp_modify_actions_t m_actions;  /*< Modifications to be done on mirrored frame. [NBO] */
    fpp_modify_args_t    m_args;     /*< Configuration values (arguments) for m_actions. */
} fpp_mirror_cmd_t;
/* [fpp_mirror_cmd_t] */

/**
 * @def         FPP_CMD_L2_BD
 * @brief       FCI command for management of L2 bridge domains.
 * @details     Related topics: @ref l2_bridge, @ref l2l3_bridge
 * @details     Related data types: @ref fpp_l2_bd_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new bridge domain.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing bridge domain.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of a bridge domain.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a bridge domain query session and get properties
 *                   of the first bridge domain from the internal list of bridge domains.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next bridge domain
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new bridge domain. When creating a new bridge domain, it is also possible to
 * simultaneously set its properties (using the same rules which apply to @ref FPP_ACTION_UPDATE).
 * @code{.c}
 *  .............................................
 *  fpp_l2_bd_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *    .vlan   = ...,                  // VLAN ID of a new bridge domain. [NBO] (user-defined)
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_l2_bd_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t),
 *                                        (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing bridge domain.
 * @code{.c}
 *  .............................................
 *  fpp_l2_bd_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .vlan   = ...,                    // VLAN ID of an existing bridge domain. [NBO]
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t),
 *                                        (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of a logical interface. It is recommended to use the read-modify-write
 * approach. Some properties cannot be modified (see fpp_l2_bd_cmd_t).
 * @code{.c}
 *  .............................................
 *  fpp_l2_bd_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *    .vlan   = ...,                // VLAN ID of an existing bridge domain. [NBO]
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_l2_bd_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_BD, sizeof(fpp_l2_bd_cmd_t),
 *                                        (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a bridge domain.
 * @code{.c}
 *  .............................................
 *  fpp_l2_bd_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_l2_bd_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_L2_BD,
 *                  sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first bridge domain from
 *  //  the internal list of bridge domains.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_L2_BD,
 *                  sizeof(fpp_l2_bd_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next bridge domain from
 *  //  the internal list of bridge domains.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_L2_BD_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the bridge domain query session (no more bridge domains).
 *        - For other ACTIONs: Unknown (nonexistent) bridge domain was requested.
 * - @c FPP_ERR_L2_BD_ALREADY_REGISTERED <br>
 *        Requested bridge domain already exists (is already registered).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_BD						0xf200

#define FPP_ERR_L2_BD_ALREADY_REGISTERED	0xf201
#define FPP_ERR_L2_BD_NOT_FOUND				0xf202

/**
 * @brief       L2 bridge domain flags
 * @details     Related data types: @ref fpp_l2_bd_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_L2_BD_DEFAULT = (1 << 0),   /*!< Domain type is default */
    FPP_L2_BD_FALLBACK = (1 << 1)   /*!< Domain type is fallback */
} fpp_l2_bd_flags_t;

/**
 * @brief       Domain statistics.
 * @details     Related data types: @ref fpp_l2_bd_cmd_t
 * @note        @b All values are in a network byte order [@b NBO].
 *
 * @snippet     fpp_ext.h  fpp_l2_bd_stats_t
 */
/* [fpp_l2_bd_stats_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint32_t ingress;       /*< Count of ingress frames for the given domain. */
    uint32_t egress;        /*< Count of egress frames for the given domain. */
    uint32_t ingress_bytes; /*< Count of ingress bytes. */
    uint32_t egress_bytes;  /*< Count of egress bytes. */
} fpp_l2_bd_stats_t;
/* [fpp_l2_bd_stats_t] */

/**
 * @brief       Data structure for L2 bridge domain.
 * @details     Related FCI commands: @ref FPP_CMD_L2_BD
 * @details     Bridge domain actions (what to do with a frame):
 *              | value | meaning |
 *              |-------|---------|
 *              | 0     | Forward |
 *              | 1     | Flood   |
 *              | 2     | Punt    |
 *              | 3     | Discard |
 *
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_l2_bd_cmd_t
 */
/* [fpp_l2_bd_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;    /*< Action */
    uint16_t vlan;      /*< Bridge domain VLAN ID. [NBO,ro] */

    uint8_t ucast_hit;  /*< Bridge domain action when the destination MAC of an inspected
                            frame is an unicast MAC and it matches some entry in the
                            Bridge MAC table. */

    uint8_t ucast_miss; /*< Bridge domain action when the destination MAC of an inspected
                            frame is an unicast MAC and it does NOT match any entry in the
                            Bridge MAC table. */

    uint8_t mcast_hit;  /*< Similar to ucast_hit, but for frames which have a multicast
                            destination MAC address. */

    uint8_t mcast_miss; /*< Similar to ucast_miss, but for frames which have a multicast
                            destination MAC address. */

    uint32_t if_list;   /*< Bridge domain ports. [NBO]. A bitset.
                            Ports are represented by physical interface bitflags.
                            If a bitflag of some physical interface is set here, the interface
                            is then considered a port of the given bridge domain.
                            Conversion between a physical interface ID and a corresponding
                            bitflag is (1uL << "physical interface ID"). */

    uint32_t untag_if_list; /*< A bitset [NBO], denoting which bridge domain ports from
                                '.if_list' are considered untagged (their egress frames
                                have the VLAN tag removed).
                                Ports which are present in both the '.if_list' bitset and
                                this bitset are considered untagged.
                                Ports which are present only in the '.if_list' bitset are
                                considered tagged. */

    fpp_l2_bd_flags_t flags;  /*< Bridge domain flags [NBO,ro] */

    fpp_l2_bd_stats_t CAL_PACKED_ALIGNED(4) stats; /*< Domain traffic statistics. [ro] */
} fpp_l2_bd_cmd_t;
/* [fpp_l2_bd_cmd_t] */

/**
 * @def         FPP_CMD_L2_STATIC_ENT
 * @brief       FCI command for management of L2 static entries.
 * @details     Related topics: @ref l2_bridge, @ref l2l3_bridge
 * @details     Related data types: @ref fpp_l2_static_ent_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new static entry.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing static entry.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of a static entry.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) static entry query session and get properties
 *                   of the first static entry from the internal collective list of all
 *                   L2 static entries (regardless of bridge domain affiliation).
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next static entry
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * @note
 * When using this command, it is recommended to disable dynamic learning of MAC addresses on all
 * physical interfaces which are configured to be a part of @ref l2_bridge or @ref l2l3_bridge.
 * See @ref FPP_CMD_PHY_IF and @ref fpp_phy_if_block_state_t.
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new L2 static entry.
 * @code{.c}
 *  .............................................
 *  fpp_l2_static_ent_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *    .vlan   = ...,                  // VLAN ID of an associated bridge domain. [NBO]
 *    .mac    = ...,                  // Static entry MAC address.
 *    .forward_list = ...             // Egress physical interfaces. [NBO]
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
 *                                                (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing L2 static entry.
 * @code{.c}
 *  .............................................
 *  fpp_l2_static_ent_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .vlan   = ...,                    // VLAN ID of an associated bridge domain. [NBO]
 *    .mac    = ...                     // Static entry MAC address.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
 *                                                (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of L2 static entry. It is recommended to use the read-modify-write
 * approach. Some properties cannot be modified (see fpp_l2_static_ent_cmd_t).
 * @code{.c}
 *  .............................................
 *  fpp_l2_static_ent_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *    .vlan   = ...,                // VLAN ID of an associated bridge domain. [NBO]
 *    .mac    = ...,                // Static entry MAC address.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_l2_static_ent_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_STATIC_ENT, sizeof(fpp_l2_static_ent_cmd_t),
 *                                                (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of L2 static entry.
 * @code{.c}
 *  .............................................
 *  fpp_l2_static_ent_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_l2_static_ent_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_L2_STATIC_ENT,
 *                  sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first static entry from
 *  //  the internal collective list of all static entries.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_L2_STATIC_ENT,
 *                  sizeof(fpp_l2_static_ent_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next static entry from
 *  //  the internal collective list of all static entries.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_L2_STATIC_EN_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the L2 static entry query session (no more L2 static entries).
 *        - For other ACTIONs: Unknown (nonexistent) L2 static entry was requested.
 * - @c FPP_ERR_L2_STATIC_ENT_ALREADY_REGISTERED <br>
 *        Requested L2 static entry already exists (is already registered).
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_STATIC_ENT						0xf340

#define FPP_ERR_L2_STATIC_ENT_ALREADY_REGISTERED	0xf341
#define FPP_ERR_L2_STATIC_EN_NOT_FOUND				0xf342

/**
 * @brief       Data structure for L2 static entry.
 * @details     Related FCI commands: @ref FPP_CMD_L2_STATIC_ENT
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_l2_static_ent_cmd_t
 */
/* [fpp_l2_static_ent_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;        /*< Action */

    uint16_t vlan;          /*< VLAN ID of an associated bridge domain. [NBO,ro]
                                VLAN-aware static entries are applied only on frames
                                which have a matching VLAN tag.
                                For non-VLAN aware static entries, use VLAN ID of
                                the Default BD (Default Bridge Domain). */

    uint8_t mac[6];         /*< Static entry MAC address. [ro] */

    uint32_t forward_list;  /*< Egress physical interfaces. [NBO]. A bitset.
                                Frames with matching destination MAC address (and VLAN tag)
                                are forwarded through all physical interfaces which are a part
                                of this bitset. Physical interfaces are represented by
                                bitflags. Conversion between a physical interface ID and
                                a corresponding bitflag is (1uL << "physical interface ID").*/

    uint8_t local;          /*< Local MAC address. (0 == false, 1 == true)
                                A part of L2L3 Bridge feature. If true, then the forward list
                                of such a static entry is ignored and frames with
                                a corresponding destination MAC address are passed to
                                the IP router algorithm. See chapter about L2L3 Bridge. */

    uint8_t dst_discard;    /*< Frames with matching destination MAC address (and VLAN tag)
                                shall be discarded. (0 == disabled, 1 == enabled) */

    uint8_t src_discard;    /*< Frames with matching source MAC address (and VLAN tag)
                                shall be discarded. (0 == disabled, 1 == enabled) */
} fpp_l2_static_ent_cmd_t;
/* [fpp_l2_static_ent_cmd_t] */

/**
 * @def         FPP_CMD_L2_FLUSH_LEARNED
 * @brief       FCI command to remove all dynamically learned MAC table entries.
 * @details     Related topics: @ref l2_bridge, @ref l2l3_bridge
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_FLUSH_LEARNED, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_FLUSH_LEARNED					0xf380

/**
 * @def         FPP_CMD_L2_FLUSH_STATIC
 * @brief       FCI command to remove all static MAC table entries.
 * @details     Related topics: @ref l2_bridge, @ref l2l3_bridge, @ref FPP_CMD_L2_STATIC_ENT
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_FLUSH_STATIC, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_FLUSH_STATIC						0xf390

/**
 * @def         FPP_CMD_L2_FLUSH_ALL
 * @brief       FCI command to remove all MAC table entries (clear the whole MAC table).
 * @details     Related topics: @ref l2_bridge, @ref l2l3_bridge
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_L2_FLUSH_ALL, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_L2_FLUSH_ALL						0xf3a0

/**
 * @def         FPP_CMD_FP_TABLE
 * @brief       FCI command for management of Flexible Parser tables.
 * @details     Related topics: @ref flex_parser
 * @details     Related data types: @ref fpp_fp_table_cmd_t, @ref fpp_fp_rule_props_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new FP table.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing FP table.
 *              - @c FPP_ACTION_USE_RULE <br>
 *                   Insert an FP rule into an FP table at the specified position.
 *              - @c FPP_ACTION_UNUSE_RULE <br>
 *                   Remove an FP rule from an FP table.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) an FP table query session and get properties
 *                   of the first FP @b rule from the requested FP table.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next FP @b rule
 *                   from the requested FP table. Intended to be called in a loop
 *                   (to iterate through the requested FP table).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new FP table.
 * @code{.c}
 *  .............................................
 *  fpp_fp_table_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_REGISTER,    // Action
 *    .table_info.t.table_name = "..."  // Name of a new FP table.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t),
 *                                                  (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing FP table.
 * @code{.c}
 *  .............................................
 *  fpp_fp_table_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .table_info.t.table_name = "..."  // Name of an existing FP table.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t),
 *                                                  (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 * @note FP table cannot be destroyed if it is in use by some PFE feature.
 *       First remove the table from use, then destroy it.
 *
 * FPP_ACTION_USE_RULE
 * -------------------
 * Insert an FP rule at the specified position in an FP table.
 * - If there are already some rules in the table, they are shifted accordingly to make room
 *   for the newly inserted rule.
 * - If the desired position is greater than the count of all rules in the table, the newly
 *   inserted rule is placed as the last rule of the table.
 *
 * @code{.c}
 *  .............................................
 *  fpp_fp_table_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_USE_RULE,     // Action
 *    .table_info.t.table_name = "...",  // Name of an existing FP table.
 *    .table_info.t.rule_name  = "...",  // Name of an existing FP rule.
 *    .table_info.t.position   = ...     // Desired position of the rule in the table.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t),
 *                                                  (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 * @note Each FP rule can be assigned only to one FP table (cannot be simultaneously a member of multiple FP tables).
 *
 * FPP_ACTION_UNUSE_RULE
 * ---------------------
 * Remove an FP rule from an FP table.
 * @code{.c}
 *  .............................................
 *  fpp_fp_table_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UNUSE_RULE,   // Action
 *    .table_info.t.table_name = "...",  // Name of an existing FP table.
 *    .table_info.t.rule_name  = "...",  // Name of an FP rule which is a member of the table.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_TABLE, sizeof(fpp_fp_table_cmd_t),
 *                                                  (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of an FP @b rule from the requested FP table.
 * Query result (properties of the @b rule) is stored in the member `.table_info.r`.
 * @code{.c}
 *  .............................................
 *  fpp_fp_table_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY         // Action
 *    .table_info.t.table_name = "...",  // Name of an existing FP table.
 *  };
 *
 *  fpp_fp_table_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_FP_TABLE,
 *                  sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci.table_info.r' now holds properties of the first FP rule from
 *  //  the requested FP table.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_FP_TABLE,
 *                  sizeof(fpp_fp_table_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci.table_info.r' now holds properties of the next FP rule from
 *  //  the requested FP table.
 *  .............................................
 * @endcode
 * @note There is currently no way to read a list of existing FP tables from PFE.
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c ENOENT @c (-2)
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the FP table query session (no more FP @b rules in the requested table).
 *        - For other ACTIONs: Unknown (nonexistent) FP table was requested.
 * - @c EEXIST @c (-17) <br>
 *        Requested FP table already exists (is already registered).
 * - @c EACCES @c (-13) <br>
 *        Requested FP table cannot be destroyed (is probably in use by some PFE feature).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_FP_TABLE						0xf220

/**
 * @def         FPP_CMD_FP_RULE
 * @brief       FCI command for management of Flexible Parser rules.
 * @details     Related topics: @ref flex_parser
 * @details     Related data types: @ref fpp_fp_rule_cmd_t, @ref fpp_fp_rule_props_t
 * @details     Each FP rule consists of a condition specified by the following properties:
 *              `.data`, `.mask` and `.offset` + `.offset_from`. FP rule then works as follows:
 *              32-bit data value from the inspected Ethernet frame (at given @c offset_from +
 *              @c offset position, masked by the @c mask) is compared with the @c data value
 *              (masked by the same @c mask). If the values are equal, then condition of the FP rule
 *              is true. An invert flag may be set to invert the condition result.
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new FP rule.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing FP rule.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) an FP rule query session and get properties
 *                   of the first FP rule from the internal collective list of all
 *                   FP rules (regardless of FP table affiliation).
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next FP rule
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new FP rule. For detailed info about FP rule properties, see fpp_fp_rule_cmd_t.
 * @code{.c}
 *  .............................................
 *  fpp_fp_rule_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *    .r.rule_name = "...",           // Rule name. A string of up to 15 characters + '\0'.
 *    .r.data      = ...,             // Expected data. [NBO]
 *    .r.mask      = ...,             // Bitmask. [NBO]
 *    .r.offset    = ...,             // Offset (in bytes). [NBO]
 *    .r.invert    = ...,             // Invert the match result.
 *
 *    .r.next_rule_name = "...",      // Name of the FP rule to jump to if '.match_action' ==
 *                                    // FP_NEXT_RULE. Set all-zero if unused.
 *
 *    .r.match_action = ...,          // Action to do if the inspected frame matches
 *                                    // the FP rule criteria.
 *
 *    .r.offset_from = ...            // Header for offset calculation.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t),
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing FP rule.
 * @code{.c}
 *  .............................................
 *  fpp_fp_rule_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .r.rule_name = "...",             // Name of an existing FP rule.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FP_RULE, sizeof(fpp_fp_rule_cmd_t),
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 * @note FP rule cannot be destroyed if it is a member of some FP table.
 *       First remove the rule from the table, then destroy the rule.
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of an FP rule. Query result is stored in the member `.r`.
 * @code{.c}
 *  .............................................
 *  fpp_fp_rule_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_fp_rule_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_FP_RULE,
 *                  sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci.r' now holds properties of the first FP rule from
 *  //  the internal collective list of all FP rules.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_FP_RULE,
 *                  sizeof(fpp_fp_rule_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci.r' now holds properties of the next FP rule from
 *  //  the internal collective list of all FP rules.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c ENOENT @c (-2)
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the FP rule query session (no more FP rules).
 *        - For other ACTIONs: Unknown (nonexistent) FP rule was requested.
 * - @c EEXIST @c (-17) <br>
 *        Requested FP rule already exists (is already registered).
 * - @c EACCES @c (-13) <br>
 *        Requested FP rule cannot be destroyed (is probably a member of some FP table).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_FP_RULE						0xf221

#define FPP_ERR_FP_RULE_NOT_FOUND			0xf222

/**
 * @def     FPP_ACTION_USE_RULE
 * @brief   Flexible Parser specific 'use' action for FPP_CMD_FP_TABLE.
 * @hideinitializer
 */
#define FPP_ACTION_USE_RULE 10

/**
 * @def     FPP_ACTION_UNUSE_RULE
 * @brief   Flexible Parser specific 'unuse' action for FPP_CMD_FP_TABLE.
 * @hideinitializer
 */
#define FPP_ACTION_UNUSE_RULE 11

/**
 * @brief       Action to do with an inspected Ethernet frame if the frame matches FP rule criteria.
 * @details     Related data types: @ref fpp_fp_rule_props_t
 * @details     Exact meaning of FP_ACCEPT and FP_REJECT (what happens with the inspected frame)
 *              depends on the context in which the parent FP table is used. See @ref flex_parser.
 *              Generally (without any further logic inversions), FP_ACCEPT means the frame is
 *              accepted and processed by PFE, while FP_REJECT means the frame is discarded.
 */
typedef enum CAL_PACKED
{
    FP_ACCEPT,    /**< Flexible Parser accepts the frame. */
    FP_REJECT,    /**< Flexible Parser rejects the frame. */
    FP_NEXT_RULE  /**< Flexible Parser continues with the matching process, but jumps to
                       a specific FP rule in the FP table. */
} fpp_fp_rule_match_action_t;

/**
 * @brief       Header for offset calculation.
 * @details     Related data types: @ref fpp_fp_rule_props_t <br>
 * @details     Offset can be calculated either from the L2, L3 or L4 header beginning.
 *              The L2 header is also the beginning of an Ethernet frame.
 * @details     L2 header is always a valid header for offset calculation. Other headers may be missing
 *              in some Ethernet frames. If an FP rule expects L3/L4 header (for offset calculation)
 *              but the given header is missing in the inspected Ethernet frame, then the result
 *              of the matching process is "frame does not match FP rule criteria".
 */
typedef enum CAL_PACKED
{
    FP_OFFSET_FROM_L2_HEADER = 2,  /**< Calculate offset from the L2 header (frame beginning). */
    FP_OFFSET_FROM_L3_HEADER = 3,  /**< Calculate offset from the L3 header. */
    FP_OFFSET_FROM_L4_HEADER = 4   /**< Calculate offset from the L4 header. */
} fpp_fp_offset_from_t;

/**
 * @brief       Properties of an FP rule (Flexible Parser rule).
 * @details     Related data types: @ref fpp_fp_table_cmd_t, @ref fpp_fp_rule_cmd_t
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_fp_rule_props_t
 */
/* [fpp_fp_rule_props_t] */
typedef struct CAL_PACKED
{
    uint8_t rule_name[16];  /*< Rule name. A string of up to 15 characters + '\0'. */

    uint32_t data;          /*< Expected data. [NBO]. This value is expected to be found
                                at the specified offset in the inspected Ethernet frame. */

    uint32_t mask;          /*< Bitmask [NBO], selecting which bits of a 32bit value shall
                                be used for data comparison. This bitmask is applied on both
                                '.data' value and the inspected value for the frame. */

    uint16_t offset;        /*< Offset (in bytes) of the inspected value in the frame. [NBO]
                                This offset is calculated from the '.offset_from' header. */

    uint8_t invert;         /*< Invert the match result before match action is selected. */

    uint8_t next_rule_name[16];  /*< Name of the FP rule to jump to if '.match_action' ==
                                     FP_NEXT_RULE. Set all-zero if unused. This next rule must
                                     be in the same FP table (cannot jump across tables). */

    fpp_fp_rule_match_action_t match_action;  /*< Action to do if the inspected frame
                                                  matches the FP rule criteria. */

    fpp_fp_offset_from_t offset_from;  /*< Header for offset calculation. */
} fpp_fp_rule_props_t;
/* [fpp_fp_rule_props_t] */

/**
 * @brief       Data structure for an FP rule.
 * @details     Related FCI commands: @ref FPP_CMD_FP_RULE
 *
 * @snippet     fpp_ext.h  fpp_fp_rule_cmd_t
 */
/* [fpp_fp_rule_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t action;        /*< Action */
    fpp_fp_rule_props_t r;  /*< Properties of the rule. */
} fpp_fp_rule_cmd_t;
/* [fpp_fp_rule_cmd_t] */

/**
 * @brief       Data structure for an FP table.
 * @details     Related FCI commands: @ref FPP_CMD_FP_TABLE
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_fp_table_cmd_t
 */
/* [fpp_fp_table_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t action;                 /*< Action */
    union
    {
        struct
        {
            uint8_t table_name[16];  /*< Name of the FP table to be administered. */
            uint8_t rule_name[16];   /*< Name of the FP rule to be added/removed. */
            uint16_t position;       /*< Position in the table where to add the rule. [NBO] */
        } t;
        fpp_fp_rule_props_t r;       /*< Query result - properties of a rule from the table */
    } table_info;
} fpp_fp_table_cmd_t;
/* [fpp_fp_table_cmd_t] */

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
 *     - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER: The client is not FCI owner
 *     - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED: The client is not authorized get FCI ownership
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
typedef struct CAL_PACKED
{
    uint8_t payload[64];	/**< The payload area */
    uint8_t len;			/**< Payload length in number of bytes */
    uint8_t reserved1;
    uint16_t reserved2;
} fpp_buf_cmd_t;

/**
 * @def         FPP_CMD_SPD
 * @brief       FCI command for management of the IPsec offload (SPD entries).
 * @details     Related topics: @ref ipsec_offload
 * @details     Related data types: @ref fpp_spd_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new SPD entry.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing SPD entry.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) an SPD entry query session and get properties
 *                   of the first SPD entry from the SPD database of a target physical interface.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next SPD entry
 *                   from the SPD database of the target physical interface.
 *                   Intended to be called in a loop (to iterate through the database).
 *
 *              @b WARNING: <br>
 *              The IPsec offload feature is available only for some Premium versions of PFE firmware.
 *              The feature should @b not be used with a firmware which does not support it.
 *              Failure to adhere to this warning will result in an undefined behavior of PFE.
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new SPD entry in the SPD database of a target physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_spd_cmd_t cmd_to_fci =
 *  {
 *    .action   = FPP_ACTION_REGISTER,  // Action
 *
 *    .name     = "...",  // Physical interface name (see chapter Physical Interface).
 *    .flags    =  ...,   // SPD entry flags. A bitset.
 *    .position =  ...,   // Entry position. [NBO]
 *    .saddr    = {...},  // Source IP address. [NBO]
 *    .daddr    = {...},  // Destination IP address. [NBO]
 *
 *    .sport    =  ...,   // Source port. [NBO]
 *                        // Optional (does not have to be set). See '.flags'.
 *
 *    .dport    =  ...,   // Destination port. [NBO]
 *                        // Optional (does not have to be set). See '.flags'.
 *
 *    .protocol =  ...,   // IANA IP Protocol Number (protocol ID).
 *
 *    .sa_id    =  ...,   // SAD entry identifier for HSE. [NBO]
 *                        // Used only when '.spd_action' == SPD_ACT_PROCESS_ENCODE).
 *
 *    .spi      =  ...    // SPI to match in the ingress traffic. [NBO]
 *                        // Used only when '.spd_action' == SPD_ACT_PROCESS_DECODE).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(cliet, FPP_CMD_SPD, sizeof(fpp_spd_cmd_t),
 *                                     (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing SPD entry.
 * @code{.c}
 *  .............................................
 *  fpp_spd_cmd_t cmd_to_fci =
 *  {
 *    .action   = FPP_ACTION_DEREGISTER,  // Action
 *    .name     = "...",  // Physical interface name (see chapter Physical Interface).
 *    .position =  ...,   // Entry position. [NBO]
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(cliet, FPP_CMD_SPD, sizeof(fpp_spd_cmd_t),
 *                                     (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of an SPD entry.
 * @code{.c}
 *  .............................................
 *  fpp_spd_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *    .name   = "...",  // Physical interface name (see chapter Physical Interface).
 *  };
 *
 *  fpp_spd_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_SPD,
 *                  sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first SPD entry from
 *  //  the SPD database of the target physical interface..
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_SPD,
 *                  sizeof(fpp_spd_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next SPD entry from
 *  //  the SPD database of the target physical interface.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_IF_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the SPD entry query session (no more SPD entries).
 *        - For other ACTIONs: Unknown (nonexistent) SPD entry was requested.
 * - @c FPP_ERR_FW_FEATURE_NOT_AVAILABLE <br>
 *        The feature is not available (not enabled in FW).
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_SPD 0xf226

/**
 * @brief       Action to be done for frames matching the SPD entry criteria.
 * @details     Related data types: @ref fpp_spd_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_SPD_ACTION_INVALID = 0U,    /**< RESERVED (do not use) */
    FPP_SPD_ACTION_DISCARD,         /**< Discard the frame. */
    FPP_SPD_ACTION_BYPASS,          /**< Bypass IPsec and forward normally. */
    FPP_SPD_ACTION_PROCESS_ENCODE,  /**< Send to HSE for encoding. */
    FPP_SPD_ACTION_PROCESS_DECODE   /**< Send to HSE for decoding. */
} fpp_spd_action_t;

/**
 * @brief       Flags for SPD entry.
 * @details     Related data types: @ref fpp_spd_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_SPD_FLAG_IPv6 = (1U << 1U),         /**< IPv4 if this flag @b not set. IPv6 if set. */
    FPP_SPD_FLAG_SPORT_OPAQUE = (1U << 2U), /**< Do @b not match @c fpp_spd_cmd_t.sport. */
    FPP_SPD_FLAG_DPORT_OPAQUE = (1U << 3U), /**< Do @b not match @c fpp_spd_cmd_t.dport. */
} fpp_spd_flags_t;

/**
 * @brief       Data structure for an SPD entry.
 * @details     Related FCI commands: @ref FPP_CMD_SPD
 * @note        Some values are in a network byte order [NBO].
 * @note        HSE is a Hardware Security Engine, a separate HW accelerator.
 *              Its configuration is outside the scope of this document.
 *
 * @snippet     fpp_ext.h  fpp_spd_cmd_t
 */
/* [fpp_spd_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;        /*< Action */
    char name[IFNAMSIZ];    /*< Physical interface name. */
    fpp_spd_flags_t flags;  /*< SPD entry flags. A bitset. */

    uint16_t position;      /*< Entry position. [NBO]
                                0 : insert as the first entry of the SPD table.
                                N : insert as the Nth entry of the SPD table, starting from 0.
                                Entries are inserted (not overwritten). Already existing
                                entries are shifted to make room for the newly inserted one.
                                If (N > current count of SPD entries) then the new entry
                                gets inserted as the last entry of the SPD table. */

    uint32_t saddr[4];      /*< Source IP address. [NBO]
                                IPv4 uses only element [0]. Address type is set in '.flags' */

    uint32_t daddr[4];      /*< Destination IP address. [NBO]
                                IPv4 uses only element [0]. Address type is set in '.flags' */

    uint16_t sport;         /*< Source port. [NBO]
                                Optional (does not have to be set). See '.flags' */

    uint16_t dport;         /*< Destination port. [NBO]
                                Optional (does not have to be set). See '.flags' */

    uint8_t protocol;       /*< IANA IP Protocol Number (protocol ID). */

    uint32_t sa_id;         /*< SAD entry identifier for HSE. [NBO]
                                Used only when '.spd_action' == SPD_ACT_PROCESS_ENCODE).
                                Corresponding SAD entry must exist in HSE. */

    uint32_t spi;           /*< SPI to match in the ingress traffic. [NBO]
                                Used only when '.spd_action' == SPD_ACT_PROCESS_DECODE). */

    fpp_spd_action_t spd_action;  /*< Action to be done on the frame. */
} fpp_spd_cmd_t;
/* [fpp_spd_cmd_t] */

/**
 * @def         FPP_CMD_QOS_QUEUE
 * @brief       FCI command for management of Egress QoS queues.
 * @details     Related topics: @ref egress_qos
 * @details     Related data types: @ref fpp_qos_queue_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of Egress QoS queue.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get properties of a target Egress QoS queue.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of an Egress QoS queue.
 * @code{.c}
 *  .............................................
 *  fpp_qos_queue_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,  // Action
 *    .if_name = "...",              // Physical interface name.
 *    .id      =  ...,               // Queue ID.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_qos_queue_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_QUEUE, sizeof(fpp_qos_queue_cmd_t),
 *                                            (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get properties of a target Egress QoS queue.
 * @code{.c}
 *  .............................................
 *  fpp_qos_queue_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY  // Action
 *    .if_name = "...",            // Physical interface name.
 *    .id      =  ...              // Queue ID.
 *  };
 *
 *  fpp_qos_queue_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_QUEUE,
 *                  sizeof(fpp_qos_queue_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the target Egress QoS queue.
 *  .............................................
 * @endcode
 *
  * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_QOS_QUEUE_NOT_FOUND <br>
 *        Unknown (nonexistent) Egress QoS queue was requested.
 * - @c FPP_ERR_QOS_QUEUE_SUM_OF_LENGTHS_EXCEEDED <br>
 *        Sum of all Egress QoS queue lengths for a given physical interface would exceed limits of the interface.
 *        First shorten some other queues of the interface, then lengthen the queue of interest.
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_IF_NOT_SUPPORTED <br>
 *        Requested interface does not support Egress QoS queue management.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
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
 * @def FPP_ERR_QOS_SCHEDULER_NOT_FOUND
 * @hideinitializer
 */
#define FPP_ERR_QOS_QUEUE_SUM_OF_LENGTHS_EXCEEDED	0xf402

/**
 * @brief       Data structure for QoS queue.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_QUEUE
 * @details     Related topics: @ref egress_qos
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_queue_cmd_t
 */
/* [fpp_qos_queue_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;         /*< Action */
    char if_name[IFNAMSIZ];  /*< Physical interface name. [ro] */

    uint8_t id;         /*< Queue ID. [ro]
                            minimal ID == 0
                            maximal ID is implementation defined. See Egress QoS. */

    uint8_t mode;       /*< Queue mode:
                            0 == Disabled. Queue will drop all packets.
                            1 == Default. HW implementation-specific. Normally not used.
                            2 == Tail drop
                            3 == WRED */

    uint32_t min;       /*< Minimum threshold. [NBO]. Value is `.mode`-specific:
                            - Disabled, Default: n/a
                            - Tail drop: n/a
                            - WRED: Threshold in number of packets in the queue at which
                                    the WRED lowest drop probability zone starts.
                                    While the queue fill level is below this threshold,
                                    the drop probability is 0%. */

    uint32_t max;       /*< Maximum threshold. [NBO]. Value is `.mode`-specific:
                            - Disabled, Default: n/a
                            - Tail drop: The queue length in number of packets. Queue length
                                         is the number of packets the queue can accommodate
                                         before drops will occur.
                            - WRED: Threshold in number of packets in the queue at which
                                    the WRED highest drop probability zone ends.
                                    While the queue fill level is above this threshold,
                                    the drop probability is 100%. */

    uint8_t zprob[32];  /*< WRED drop probabilities for all probability zones in [%].
                            The lowest probability zone is `.zprob[0]`. Only valid for
                            `.mode = WRED`. Value 255 means 'invalid'. Number of zones
                            per queue is implementation-specific. See Egress QoS. */
} fpp_qos_queue_cmd_t;
/* [fpp_qos_queue_cmd_t] */

/**
 * @def         FPP_CMD_QOS_SCHEDULER
 * @brief       FCI command for management of Egress QoS schedulers.
 * @details     Related topics: @ref egress_qos
 * @details     Related data types: @ref fpp_qos_scheduler_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of Egress QoS scheduler.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get properties of a target Egress QoS scheduler.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of an Egress QoS scheduler.
 * @code{.c}
 *  .............................................
 *  fpp_qos_scheduler_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,  // Action
 *    .if_name = "...",              // Physical interface name.
 *    .id      =  ...,               // Scheduler ID.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_qos_scheduler_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_SCHEDULER, sizeof(fpp_qos_scheduler_cmd_t),
 *                                                (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get properties of a target Egress QoS scheduler.
 * @code{.c}
 *  .............................................
 *  fpp_qos_scheduler_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY  // Action
 *    .if_name = "...",            // Physical interface name.
 *    .id      =  ...              // Scheduler ID.
 *  };
 *
 *  fpp_qos_scheduler_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_SCHEDULER,
 *                  sizeof(fpp_qos_scheduler_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the target Egress QoS scheduler.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_QOS_SCHEDULER_NOT_FOUND <br>
 *        Unknown (nonexistent) Egress QoS scheduler was requested.
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_IF_NOT_SUPPORTED <br>
 *        Requested interface does not support Egress QoS scheduler management.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
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
 * @brief       Data structure for QoS scheduler.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_SCHEDULER
 * @details     Related topics: @ref egress_qos
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_scheduler_cmd_t
 */
/* [fpp_qos_scheduler_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;        /*< Action */
    char if_name[IFNAMSIZ]; /*< Physial interface name. [ro] */

    uint8_t id;             /*< Scheduler ID. [ro]
                                minimal ID == 0
                                maximal ID is implementation defined. See Egress QoS. */

    uint8_t mode;           /*< Scheduler mode:
                                0 == Scheduler disabled
                                1 == Data rate (payload length)
                                2 == Packet rate (number of packets) */

    uint8_t algo;           /*< Scheduler algorithm:
                                0 == PQ (Priority Queue). Input with the highest priority
                                     is serviced first. Input 0 has the @b lowest priority.
                                1 == DWRR (Deficit Weighted Round Robin).
                                2 == RR (Round Robin).
                                3 == WRR (Weighted Round Robin). */

    uint32_t input_en;      /*< Input enable bitfield. [NBO]
                                When a bit `n` is set it means that scheduler input `n`
                                is enabled and connected to traffic source defined by
                                `.source[n]`. Number of inputs is implementation-specific.
                                See Egress QoS. */

    uint32_t input_w[32];   /*< Input weight. [NBO]. Scheduler algorithm-specific:
                                - PQ, RR - n/a
                                - WRR, DWRR - Weight in units given by `.mode` */

    uint8_t input_src[32];  /*< Traffic source for each scheduler input. Traffic sources
                                are implementation-specific. See Egress QoS. */
} fpp_qos_scheduler_cmd_t;
/* [fpp_qos_scheduler_cmd_t] */

/**
 * @def         FPP_CMD_QOS_SHAPER
 * @brief       FCI command for management of Egress QoS shapers.
 * @details     Related topics: @ref egress_qos
 * @details     Related data types: @ref fpp_qos_shaper_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of Egress QoS shaper.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get properties of a target Egress QoS shaper.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of an Egress QoS shaper.
 * @code{.c}
 *  .............................................
 *  fpp_qos_shaper_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,  // Action
 *    .if_name = "...",              // Physical interface name.
 *    .id      =  ...,               // Shaper ID.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_qos_shaper_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_SHAPER, sizeof(fpp_qos_shaper_cmd_t),
 *                                             (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get properties of a target Egress QoS shaper.
 * @code{.c}
 *  .............................................
 *  fpp_qos_shaper_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY  // Action
 *    .if_name = "...",            // Physical interface name.
 *    .id      =  ...,             // Shaper ID.
 *  };
 *
 *  fpp_qos_shaper_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_SHAPER,
 *                  sizeof(fpp_qos_shaper_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the target Egress QoS shaper.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_QOS_SHAPER_NOT_FOUND <br>
 *        Unknown (nonexistent) Egress QoS shaper was requested.
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property.
 * - @c FPP_ERR_IF_NOT_SUPPORTED <br>
 *        Requested interface does not support Egress QoS shaper management.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
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
 * @brief       Data structure for QoS shaper.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_SHAPER
 * @details     Related topics: @ref egress_qos
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_shaper_cmd_t
 */
/* [fpp_qos_shaper_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;        /*< Action */
    char if_name[IFNAMSIZ]; /*< Physial interface name. [ro] */

    uint8_t id;             /*< Shaper ID. [ro]
                                minimal ID == 0
                                maximal ID is implementation defined. See Egress QoS. */

    uint8_t position;       /*< Position of the shaper.
                                Positions are implementation defined. See Egress QoS. */

    uint32_t isl;           /*< Idle slope in units per second (see `.mode`). [NBO] */
    int32_t max_credit;     /*< Max credit. [NBO] */
    int32_t min_credit;     /*< Min credit. [NBO] */

    uint8_t mode;           /*< Shaper mode:
                                0 == Shaper disabled
                                1 == Data rate.
                                     `.isl` is in bits-per-second.
                                     `.max_credit` and `.min_credit` are in number of bytes.
                                2 == Packet rate.
                                     `isl` is in packets-per-second.
                                     `.max_credit` and `.min_credit` are in number of packets.
                                */
} fpp_qos_shaper_cmd_t;
/* [fpp_qos_shaper_cmd_t] */

/**
 * @def         FPP_CMD_QOS_POLICER
 * @brief       FCI command for Ingress QoS policer enable/disable.
 * @details     Related topics: @ref ingress_qos, @ref FPP_CMD_QOS_POLICER_FLOW, @ref FPP_CMD_QOS_POLICER_WRED, @ref FPP_CMD_QOS_POLICER_SHP
 * @details     Related data types: @ref fpp_qos_policer_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Enable/disable Ingress QoS policer of a physical interface.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get status of a target Ingress QoS policer.
 *
 * @note
 * Management of Ingress QoS policer is available only for @b emac physical interfaces.
 *
 * @note
 * Effects of enable/disable:
 * - If an Ingress QoS policer gets disabled, its associated flow table, WRED module and shaper module are disabled as well.
 * - If an Ingress QoS policer gets enabled, it starts with default configuration
 *   (clear flow table, default WRED configuration, default shaper configuration).
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Enable/disable Ingress QoS policer of an @b emac physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,
 *    .if_name = "...",    // Physical interface name ('emac' interfaces only).
 *    .enable  =  ...      // 0 == disabled ; 1 == enabled
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_POLICER, sizeof(fpp_qos_policer_cmd_t),
 *                                              (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get status (enabled/disabled) of an Ingress QoS policer.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY,
 *    .if_name = "...",    // Physical interface name ('emac' interfaces only).
 *  };
 *
 *  fpp_qos_policer_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_POLICER,
 *                  sizeof(fpp_qos_policer_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds the '.enable' field set accordingly.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Wrong physical interface provided (i.e. non-'emac'), or unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_POLICER 0xf430

/**
 * @brief       Data structure for Ingress QoS policer enable/disable.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_POLICER
 * @details     Related topics: @ref ingress_qos
 * @note        Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_policer_cmd_t
 */
/* [fpp_qos_policer_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;
    char if_name[IFNAMSIZ]; /*< Physical interface name ('emac' interfaces only). [ro] */
    uint8_t enable;         /*< Enable/disable switch of the Ingress QoS Policer HW module.
                                0 == disabled, 1 == enabled. */
} fpp_qos_policer_cmd_t;
/* [fpp_qos_policer_cmd_t] */

/**
 * @def         FPP_CMD_QOS_POLICER_FLOW
 * @brief       FCI command for management of Ingress QoS packet flows.
 * @details     Related topics: @ref ingress_qos
 * @details     Related data types: @ref fpp_qos_policer_flow_cmd_t, @ref fpp_iqos_flow_spec_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Add a flow to an Ingress QoS flow classification table.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove a flow from an Ingress QoS flow classification table.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a flow query session and get properties
 *                   of the first flow from an Ingress QoS flow clasification table.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next
 *                   flow from the table. Intended to be called in a loop
 *                   (to iterate through the table).
 *
 * @note
 * - Management of Ingress QoS packet flows is available only for @b emac physical interfaces.
 * - Management of Ingress QoS packet flows is possible only if the associated @ref FPP_CMD_QOS_POLICER is enabled.
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Add a packet flow to an Ingress QoS flow classification table. Specify flow parameters and
 * the action to be done for packets which conform to the given flow.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_flow_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_REGISTER,
 *    .if_name = "...",  // Physical interface name ('emac' interfaces only).
 *    .id      =  ...,   // Position in the classification table. 0xFF == automatic placement.
 *
 *    .flow    = {...}   // Flow specification structure.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_POLICER_FLOW, sizeof(fpp_qos_policer_flow_cmd_t),
 *                                                   (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove a flow from an Ingress QoS flow classification table.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_flow_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_DEREGISTER,
 *    .if_name = "...",  // Physical interface name ('emac' interfaces only).
 *    .id      =  ...,   // Position in the classification table.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_POLICER_FLOW, sizeof(fpp_qos_policer_flow_cmd_t),
 *                                                   (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of the Ingress QoS flow.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_flow_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY,
 *    .if_name = "...",  // Physical interface name ('emac' interfaces only).
 *    .id      =  ...,   // Entry position in the table, from 0 to "table size - 1".
 *  };
 *
 *  fpp_qos_policer_flow_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_POLICER_FLOW,
 *                  sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds the content of the first available (i.e. active) flow
 *  //  from the Ingress QoS flow classification table of the target physical interface.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_QOS_POLICER_FLOW,
 *                  sizeof(fpp_qos_policer_flow_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next available flow from the table.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_QOS_POLICER_FLOW_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the Ingress QoS flow query session (no more flows).
 *        - For other ACTIONs: Unknown (nonexistent) Ingress QoS flow was requested.
 * - @c FPP_ERR_QOS_POLICER_FLOW_TABLE_FULL <br>
 *        Attempting to register flow with `.id >= FPP_IQOS_FLOW_TABLE_SIZE` or flow table full.
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Wrong physical interface provided (i.e. non-'emac'), or unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_POLICER_FLOW            0xf440

#define FPP_ERR_QOS_POLICER_FLOW_TABLE_FULL 0xf441
#define FPP_ERR_QOS_POLICER_FLOW_NOT_FOUND  0xf442

/**
 * @brief       Argumentless flow types (match flags).
 * @details     Related data types: @ref fpp_iqos_flow_spec_t
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_FLOW_TYPE_ETH = (1 << 0),     /**< Match ETH packets. */
    FPP_IQOS_FLOW_TYPE_PPPOE = (1 << 1),   /**< Match PPPoE packets. */
    FPP_IQOS_FLOW_TYPE_ARP = (1 << 2),     /**< Match ARP packets. */
    FPP_IQOS_FLOW_TYPE_IPV4 = (1 << 3),    /**< Match IPv4 packets. */
    FPP_IQOS_FLOW_TYPE_IPV6 = (1 << 4),    /**< Match IPv6 packets. */
    FPP_IQOS_FLOW_TYPE_IPX = (1 << 5),     /**< Match IPX packets. */
    FPP_IQOS_FLOW_TYPE_MCAST = (1 << 6),   /**< Match L2 multicast packets. */
    FPP_IQOS_FLOW_TYPE_BCAST = (1 << 7),   /**< Match L2 broadcast pakcets. */
    FPP_IQOS_FLOW_TYPE_VLAN = (1 << 8),    /**< Match VLAN tagged packets. */

    FPP_IQOS_FLOW_TYPE_MAX = FPP_IQOS_FLOW_TYPE_VLAN,
    /* Ensure proper size */
    FPP_IQOS_FLOW_TYPE_RESERVED = (uint16_t)(1U << 15U)
} fpp_iqos_flow_type_t;

/**
 * @brief       Argumentful flow types (match flags).
 * @details     Related data types: @ref fpp_iqos_flow_spec_t, @ref fpp_iqos_flow_args_t
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_ARG_VLAN = (1 << 0),     /**< Match bitmasked VLAN value. */
    FPP_IQOS_ARG_TOS = (1 << 1),      /**< Match bitmasked TOS value. */
    FPP_IQOS_ARG_L4PROTO = (1 << 2),  /**< Match bitmasked L4 protocol value. */
    FPP_IQOS_ARG_SIP = (1 << 3),      /**< Match prefixed source IPv4/IPv6 address. */
    FPP_IQOS_ARG_DIP = (1 << 4),      /**< Match prefixed destination IPv4/IPv6 address. */
    FPP_IQOS_ARG_SPORT = (1 << 5),    /**< Match L4 source port range. */
    FPP_IQOS_ARG_DPORT = (1 << 6),    /**< Match L4 destination port range. */

    FPP_IQOS_ARG_MAX = FPP_IQOS_ARG_DPORT,
    /* Ensure proper size */
    FPP_IQOS_ARG_RESERVED = (uint16_t)(1U << 15U)
} fpp_iqos_flow_arg_type_t;

/**
 * @brief       @ref FPP_CMD_QOS_POLICER_FLOW, @ref fpp_iqos_flow_args_t : <br>
 *              Bitmask for comparison of the whole VLAN ID (all bits compared).
*/
#define FPP_IQOS_VLAN_ID_MASK 0xFFF

/**
 * @brief       @ref FPP_CMD_QOS_POLICER_FLOW, @ref fpp_iqos_flow_args_t : <br>
 *              Bitmask for comparison of the whole TOS/TCLASS field (all bits compared).
*/
#define FPP_IQOS_TOS_MASK     0xFF

/**
 * @brief       @ref FPP_CMD_QOS_POLICER_FLOW, @ref fpp_iqos_flow_args_t : <br>
 *              Bitmask for comparison of the whole L4 protocol field (all bits compared).
*/
#define FPP_IQOS_L4PROTO_MASK 0xFF

/**
 * @brief       @ref FPP_CMD_QOS_POLICER_FLOW, @ref fpp_iqos_flow_args_t : <br>
 *              Network prefix for comparison of the whole IP address (all bits compared).
*/
#define FPP_IQOS_SDIP_MASK    0x3F

/**
 * @brief       Arguments for argumentful flow types.
 * @details     Related data types: @ref fpp_iqos_flow_spec_t, @ref fpp_iqos_flow_arg_type_t
 *
 * @details
 * Bitmasking
 * ----------
 * Explanation: <br>
 *   In the comparison process for argumentful flow types, bitmasking works as follows: <br>
 *   `if ((PacketData & Mask) == (ArgData & Mask)), then packet matches the flow.`
 *   - `PacketData` is the inspected value from an ingress packet.
 *   - `ArgData` is the argument value of an argumentful flow type.
 *   - `Mask` is the bitmask of an argumentful flow type. <br>
 *      For IP addresses, the network prefix (e.g. /24) is internally converted into
 *      a valid subnet mask (/24 == 0xFFFFFFF0).
 *
 * Example: <br>
 *   If `.l4proto_m = 0x07` , then only the lowest 3 bits of the TOS field would be compared.
 *   That means any protocol value with matching lowest 3 bits would be accepted.
 *
 * Hint: <br>
 * Use the provided bitmask symbols (see descriptions of struct fields) to compare whole values (all bits).
 * Do not use custom bitmasks, unless some specific scenario requires such refinement.
 *
 * Description
 * -----------
 * @note
 * Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_iqos_flow_args_t
 */
/* [fpp_iqos_flow_args_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t vlan;       /*< FPP_IQOS_ARG_VLAN: VLAN ID (max 4095). [NBO] */

    uint16_t vlan_m;     /*< FPP_IQOS_ARG_VLAN: VLAN ID comparison bitmask (12b). [NBO]
                             Use FPP_IQOS_VLAN_ID_MASK to compare whole value (all bits). */

    uint8_t tos;         /*< FPP_IQOS_ARG_TOS: TOS field for IPv4, TCLASS for IPv6. */

    uint8_t tos_m;       /*< FPP_IQOS_ARG_TOS: TOS comparison bitmask.
                             Use FPP_IQOS_TOS_MASK to compare whole value (all bits). */

    uint8_t l4proto;     /*< FPP_IQOS_ARG_L4PROTO: L4 protocol field for IPv4 and IPv6. */

    uint8_t l4proto_m;   /*< FPP_IQOS_ARG_L4PROTO: L4 protocol comparison bitmask.
                             Use FPP_IQOS_L4PROTO_MASK to compare whole value (all bits). */

    uint32_t sip;        /*< FPP_IQOS_ARG_SIP: Source IP address for IPv4/IPv6. [NBO] */
    uint32_t dip;        /*< FPP_IQOS_ARG_DIP: Destination IP address for IPv4/IPv6. [NBO] */

    uint8_t sip_m;       /*< FPP_IQOS_ARG_SIP: Source IP address - network prefix.
                             Use FPP_IQOS_SDIP_MASK to compare whole address (all bits). */

    uint8_t dip_m;       /*< FPP_IQOS_ARG_DIP: Destination IP address - network prefix.
                             Use FPP_IQOS_SDIP_MASK to compare whole address (all bits). */

    uint16_t sport_max;  /*< FPP_IQOS_ARG_SPORT: Max L4 source port. [NBO] */
    uint16_t sport_min;  /*< FPP_IQOS_ARG_SPORT: Min L4 source port. [NBO] */
    uint16_t dport_max;  /*< FPP_IQOS_ARG_DPORT: Max L4 destination port. [NBO] */
    uint16_t dport_min;  /*< FPP_IQOS_ARG_DPORT: Min L4 destination port. [NBO] */
} fpp_iqos_flow_args_t;
/* [fpp_iqos_flow_args_t] */

/**
 * @brief       Action to be done for matching packets.
 * @details     Related data types: @ref fpp_iqos_flow_spec_t
 * @note        See @ref ingress_qos for explanation of Ingress QoS traffic classification.
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_FLOW_MANAGED = 0,  /**< Classify the matching packet as Managed traffic. Default action. */
    FPP_IQOS_FLOW_DROP,         /**< Drop the packet. */
    FPP_IQOS_FLOW_RESERVED,     /**< Classify the matching packet as Reserved traffic. */

    FPP_IQOS_FLOW_COUNT         /* must be last */
} fpp_iqos_flow_action_t;

/**
 * @brief       Specification of Ingress QoS packet flow.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_POLICER_FLOW
 * @details     Related data types: @ref fpp_qos_policer_flow_cmd_t
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp_ext.h  fpp_iqos_flow_spec_t
 */
/* [fpp_iqos_flow_spec_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    fpp_iqos_flow_type_t type_mask;          /*< Argumentless flow types to match. [NBO]
                                                 A bitset mask. */

    fpp_iqos_flow_arg_type_t arg_type_mask;  /*< Argumentful flow types to match. [NBO]
                                                 A bitset mask. */

    fpp_iqos_flow_args_t CAL_PACKED_ALIGNED(4) args; /*< Arguments for argumentful flow types.
                                                         Related to 'arg_type_mask'. */

    fpp_iqos_flow_action_t action;           /*< Action to be done for matching packets. */
} fpp_iqos_flow_spec_t;
/* [fpp_iqos_flow_spec_t] */

/**
 * @brief       Data structure for Ingress QoS packet flow.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_POLICER_FLOW
 * @details     Related topics: @ref ingress_qos
 *
 * @snippet     fpp_ext.h  fpp_qos_policer_flow_cmd_t
 */
/* [fpp_qos_policer_flow_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;
    char if_name[IFNAMSIZ]; /*< Physical interface name ('emac' interfaces only). */

    uint8_t id;             /*< Position in the classification table.
                                minimal ID == 0
                                maximal ID is implementation defined. See Ingress QoS.
                                For FPP_ACTION_REGISTER, value 0xFF means "don't care".
                                If 0xFF is set as registration id, driver will automatically
                                choose the first available free position. */

    fpp_iqos_flow_spec_t CAL_PACKED_ALIGNED(4) flow;  /*< Flow specification. */
} fpp_qos_policer_flow_cmd_t;
/* [fpp_qos_policer_flow_cmd_t] */

/**
 * @def         FPP_CMD_QOS_POLICER_WRED
 * @brief       FCI command for management of Ingress QoS WRED queues.
 * @details     Related topics: @ref ingress_qos
 * @details     Related data types: @ref fpp_qos_policer_wred_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of Ingress QoS WRED queue.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get properties of a target Ingress QoS WRED queue.
 *
 * @note
 * - Management of Ingress QoS WRED queues is available only for @b emac physical interfaces.
 * - Management of Ingress QoS WRED queues is possible only if the associated @ref FPP_CMD_QOS_POLICER is enabled.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Update Ingress QoS WRED queue of a target physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_wred_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,
 *    .if_name = "...",      // Physical interface name ('emac' interfaces only).
 *    .queue   =  ...,       // Target Ingress QoS WRED queue (DMEM, LMEM, RXF).
 *    .enable  =  ...,       // Enable/disable switch (0 == disabled, 1 == enabled).
 *
 *    .thr[]   = {...},      // Min/max/full WRED thresholds.
 *                           // 0xFFFF == let HW keep its currently configured thld value.
 *
 *    .zprob[] = {...}       // WRED drop probabilities for all zones in 1/16 increments.
 *                           // 0xFF == let HW keep its currently configured zone value.
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_POLICER_WRED, sizeof(fpp_qos_policer_wred_cmd_t),
 *                                                   (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get properties of a target Ingress QoS WRED queue.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_wred_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY,
 *    .if_name = "...",      // Physical interface name ('emac' interfaces only).
 *    .queue   =  ...,       // Target Ingress QoS WRED queue (DMEM, LMEM, RXF).
 *  };
 *
 *  fpp_qos_policer_wred_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_POLICER_WRED,
 *                  sizeof(fpp_qos_policer_wred_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the target Ingress QoS WRED queue.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Wrong physical interface provided (i.e. non-'emac'), or unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_POLICER_WRED            0xf450

/**
 * @brief       Supported target queues of Ingress QoS WRED.
 * @details     Related data types: @ref fpp_qos_policer_wred_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_Q_DMEM = 0,/**< Queue which is in DMEM (Data Memory). Standard storage. */
    FPP_IQOS_Q_LMEM,    /**< Queue which is in LMEM (Local Memory). Faster execution but smaller capacity. */
    FPP_IQOS_Q_RXF,     /**< Queue which is in FIFO of the associated physical interface. */

    FPP_IQOS_Q_COUNT    /* must be last */
} fpp_iqos_queue_t;

/**
 * @brief       Supported probability zones of Ingress QoS WRED queue.
 * @details     Related data types: @link fpp_qos_policer_wred_cmd_t @endlink`.zprob[]`
 * @note        This enum represents valid array indexes.
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_WRED_ZONE1 = 0,   /**< WRED probability zone 1 (lowest). */
    FPP_IQOS_WRED_ZONE2,       /**< WRED probability zone 2. */
    FPP_IQOS_WRED_ZONE3,       /**< WRED probability zone 3. */
    FPP_IQOS_WRED_ZONE4,       /**< WRED probability zone 4 (highest). */

    FPP_IQOS_WRED_ZONES_COUNT  /* must be last */
} fpp_iqos_wred_zone_t;

/**
 * @brief       Thresholds of Ingress QoS WRED queue.
 * @details     Related data types: @link fpp_qos_policer_wred_cmd_t @endlink`.thr[]`
 * @note        - This enum represents valid array indexes.
 * @note        - See @ref ingress_qos for explanation of Ingress QoS traffic classification. <br>
 *                (Unmanaged/Managed/Reserved)
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_WRED_MIN_THR = 0,  /**< WRED queue min threshold. <br>
                                     If queue fill below `.thr[FPP_IQOS_WRED_MIN_THR]`, the following applies:
                                     - Drop Unmanaged traffic by probability zones.
                                     - Keep Managed and Reserved traffic. */

    FPP_IQOS_WRED_MAX_THR,      /**< WRED queue max threshold. <br>
                                     If queue fill over `.thr[FPP_IQOS_WRED_MIN_THR]` but below `.thr[FPP_IQOS_WRED_MAX_THR]`, the following applies:
                                     - Drop all Unmanaged and Managed traffic.
                                     - Keep Reserved traffic. */

    FPP_IQOS_WRED_FULL_THR,     /**< WRED queue full threshold. <br>
                                     If queue fill over `.thr[FPP_IQOS_WRED_FULL_THR]`, then drop all traffic. */

    FPP_IQOS_WRED_THR_COUNT     /* must be last */
} fpp_iqos_wred_thr_t;

/**
 * @brief       Data structure for Ingress QoS WRED queue.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_POLICER_WRED
 * @details     Related topics: @ref ingress_qos
 * @details     Related data types: @ref fpp_iqos_wred_thr_t, @ref fpp_iqos_wred_zone_t
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_policer_wred_cmd_t
 */
/* [fpp_qos_policer_wred_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;
    char if_name[IFNAMSIZ];  /*< Physical interface name ('emac' interfaces only). [ro] */
    fpp_iqos_queue_t queue;  /*< Target Ingress QoS WRED queue. [ro] */
    uint8_t enable;          /*< Enable/disable switch of the target WRED queue HW module.
                                 0 == disabled, 1 == enabled. */

    uint16_t thr[FPP_IQOS_WRED_THR_COUNT]; /*< WRED queue thresholds. [NBO]
                                               See 'fpp_iqos_wred_thr_t'.
                                               Unit is "number of packets".
                                               Min value == 0
                                               Max value is implementation defined. See
                                               Ingress QoS chapter for implementation details.
                                               Value 0xFFFF == HW keeps its currently
                                                               configured value. */

    uint8_t zprob[FPP_IQOS_WRED_ZONES_COUNT]; /*< WRED drop probabilities for all probability
                                                  zones. See 'fpp_iqos_wred_zone_t'.
                                                  One unit (1) represents probability of 1/16.
                                                  Min value == 0   ( 0/16 = 0%)
                                                  Max value == 15  (15/16 = 93,75%)
                                                  Value 255 == HW keeps its currently
                                                               configured value. */
} fpp_qos_policer_wred_cmd_t;
/* [fpp_qos_policer_wred_cmd_t] */

/**
 * @def         FPP_CMD_QOS_POLICER_SHP
 * @brief       FCI command for management of Ingress QoS shapers.
 * @details     Related topics: @ref ingress_qos
 * @details     Related data types: @ref fpp_qos_policer_shp_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of Ingress QoS shaper.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Get properties of a target Ingress QoS shaper.
 *
 * @note
 * - Management of Ingress QoS shapers is available only for @b emac physical interfaces.
 * - Management of Ingress QoS shapers is possible only if the associated @ref FPP_CMD_QOS_POLICER is enabled.
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Configure Ingress QoS credit based shaper (IEEE 802.1Q) of a target physical interface.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_shp_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_UPDATE,
 *    .if_name = "...",  // Physical interface name ('emac' interfaces only).
 *    .id      =  ...,   // ID of the target Ingress QoS shaper.
 *
 *    ... = ...  // Properties (data fields) to be updated, and their new (modified) values.
 *               // Some properties cannot be modified (see fpp_qos_policer_shp_cmd_t).
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_QOS_POLICER_SHP, sizeof(fpp_qos_policer_shp_cmd_t),
 *                                                  (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY
 * ----------------
 * Get properties of a target Ingress QoS shaper.
 * @code{.c}
 *  .............................................
 *  fpp_qos_policer_shp_cmd_t cmd_to_fci =
 *  {
 *    .action  = FPP_ACTION_QUERY,
 *    .if_name = "...",  // Physical interface name ('emac' interfaces only).
 *    .id      =  ...    // ID of the target Ingress QoS shaper.
 *  };
 *
 *  fpp_qos_policer_shp_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_QOS_POLICER_SHP,
 *                  sizeof(fpp_qos_policer_shp_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the target Ingress QoS shaper.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Wrong physical interface provided (i.e. non-'emac'), or unexpected value of some property.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_QOS_POLICER_SHP             0xf460

/**
 * @brief       Types of Ingress QoS shaper.
 * @details     Related data types: @ref fpp_qos_policer_shp_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_SHP_PORT_LEVEL = 0,    /**< Port level data rate shaper. */
    FPP_IQOS_SHP_BCAST,             /**< Shaper for broadcast packets. */
    FPP_IQOS_SHP_MCAST,             /**< Shaper for multicast packets. */

    FPP_IQOS_SHP_TYPE_COUNT         /* must be last */
} fpp_iqos_shp_type_t;

/**
 * @brief       Modes of Ingress QoS shaper.
 * @details     Related data types: @ref fpp_qos_policer_shp_cmd_t
 */
typedef enum CAL_PACKED
{
    FPP_IQOS_SHP_BPS = 0,          /**< `.isl` is in bits-per-second. <br>
                                        `.max_credit` and `.min_credit` are in number of bytes. */

    FPP_IQOS_SHP_PPS,              /**< `.isl` is in packets-per-second. <br>
                                        `.max_credit` and `.min_credit` are in number of packets. */

    FPP_IQOS_SHP_RATE_MODE_COUNT   /* must be last */
} fpp_iqos_shp_rate_mode_t;

/**
 * @brief       Data structure for Ingress QoS shaper.
 * @details     Related FCI commands: @ref FPP_CMD_QOS_POLICER_SHP
 * @details     Related topics: @ref ingress_qos
 * @note        - Some values are in a network byte order [NBO].
 * @note        - Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_qos_policer_shp_cmd_t
 */
/* [fpp_qos_policer_shp_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;
    char if_name[IFNAMSIZ];    /*< Physical interface name ('emac' interfaces only). [ro] */

    uint8_t id;                /*< ID of the target Ingress QoS shaper. [ro]
                                   Min ID == 0
                                   Max ID is implementation defined. See Ingress QoS. */

    uint8_t enable;            /*< Enable/disable switch of the target Ingress QoS shaper 
                                   HW module. 0 == disabled, 1 == enabled. */

    fpp_iqos_shp_type_t type;  /*< Shaper type. Port level, bcast or mcast. */

    fpp_iqos_shp_rate_mode_t mode;  /*< Shaper mode. Bits or packets. */
    uint32_t isl;                   /*< Idle slope. Units are '.mode' dependent. [NBO] */
    int32_t max_credit;             /*< Max credit. Units are '.mode' dependent. [NBO] */
    int32_t min_credit;             /*< Min credit. Units are '.mode' dependent. [NBO]
                                        Must be negative. */
} fpp_qos_policer_shp_cmd_t;
/* [fpp_qos_policer_shp_cmd_t] */

/**
 * @def         FPP_CMD_FW_FEATURE
 * @brief       FCI command for management of configurable FW features.
 * @details     Related topics: ---
 * @details     Related data types: @ref fpp_fw_features_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Enable/disable a FW feature.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a FW feature query session and get properties
 *                   of the first FW feature from the internal list of FW features.
 *              - @c FPP_ACTION_QUERY_CONT <br>
 *                   Continue the query session and get properties of the next FW feature
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Enable/disable a FW feature.
 * @code{.c}
 *  .............................................
 *  fpp_fw_features_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *    .val    = ...                 // 0 == disabled ; 1 == enabled
 *  };
 *
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FW_FEATURE, sizeof(fpp_fw_features_cmd_t),
 *                                             (unsigned short*)(&cmd_to_fci));
 *  .............................................
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a FW feature.
 * @code{.c}
 *  .............................................
 *  fpp_fw_features_cmd_t cmd_to_fci =
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *
 *  fpp_fw_features_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u;
 *
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_FW_FEATURE,
 *                  sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the first FW feature from
 *  //  the internal list of FW features.
 *
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_FW_FEATURE,
 *                  sizeof(fpp_fw_features_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *
 *  // 'reply_from_fci' now holds properties of the next FW feature from
 *  //  the internal list of FW features.
 *  .............................................
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c ENOENT @c (-2)
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the FW feature query session (no more FW features).
 *        - For other ACTIONs: Unknown (nonexistent) FW feature was requested.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_FW_FEATURE					0xf250
#define FPP_ERR_FW_FEATURE_NOT_FOUND		0xf251
#define FPP_ERR_FW_FEATURE_NOT_AVAILABLE	0xf252
#define FPP_FEATURE_NAME_SIZE 32
#define FPP_FEATURE_DESC_SIZE 128

/**
 * @brief       Feature flags
 * @details     Flags combinations:
 *              - @c FEAT_PRESENT is missing: <br>
 *                     The feature is not available.
 *              - @c FEAT_PRESENT is set, but @c FEAT_RUNTIME is missing: <br>
 *                     The feature is always enabled (cannot be disabled).
 *              - @c FEAT_PRESENT is set and @c FEAT_RUNTIME is set: <br>
 *                     The feature can be enabled/disable at runtime.
 *                     Enable state must be read out of DMEM.
 */
typedef enum CAL_PACKED
{
    FEAT_NONE = 0U,
    FEAT_PRESENT = (1U << 0U),      /**< Feature not available if this not set. */
    FEAT_RUNTIME = (1U << 1U)       /**< Feature can be enabled/disabled at runtime. */
} fpp_fw_feature_flags_t;

/**
 * @brief       Data structure for FW feature setting.
 * @details     Related FCI commands: @ref FPP_CMD_FW_FEATURE
 * @note        Some values cannot be modified by FPP_ACTION_UPDATE [ro].
 *
 * @snippet     fpp_ext.h  fpp_fw_features_cmd_t
 */
/* [fpp_fw_features_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t action;                       /*< Action */
    char name[FPP_FEATURE_NAME_SIZE + 1];  /*< Feature name. [ro] */
    char desc[FPP_FEATURE_DESC_SIZE + 1];  /*< Feature description. [ro] */

    uint8_t val;        /*< Feature current state.
                            0 == disabled ; 1 == enabled */

    fpp_fw_feature_flags_t flags;  /*< Feature configuration variant. [ro] */
    uint8_t def_val;               /*< Factory default value of the '.val' property. [ro] */
    uint8_t reserved;              /*< RESERVED (do not use) */
} fpp_fw_features_cmd_t;
/* [fpp_fw_features_cmd_t] */

#define FPP_CMD_FW_FEATURE_ELEMENT				0xf260
#define FPP_ERR_FW_FEATURE_ELEMENT_NOT_FOUND		0xf261
#define FPP_ERR_FW_FEATURE_ELEMENT_READ_ONLY		0xf262

enum CAL_PACKED
{
    FW_FEATURE_ELEMENT_DEFAUT = 0U,
    FW_FEATURE_ELEMENT_CONFIG = 1U,
    FW_FEATURE_ELEMENT_STATS = 2U
};

/* [fpp_fw_feature_element_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4)
{
    uint16_t action;                                 /*< Action */
    char fw_feature_name[FPP_FEATURE_NAME_SIZE + 1]; /*< Name of a fw feature (see fpp_fw_features_cmd_t) */
    char element_name[FPP_FEATURE_NAME_SIZE + 1];    /*< Name of the fw feature's target element */
    uint8_t group; /*< Element group
                                             0 == ANY ; No group specified. Special value, intended only for FPP_ACTION_QUERY.
                                                        FPP_ACTION_QUERY command with EMPTY element_name[] and with this group starts
                                                        a QUERY/QUERY_CONT process that will successively report all elements of 
                                                        the parent fw feature (regardless of their element group).
                                             1 == CFG  ;Configuration group. Command with this group can target only some configuration element.
                                                        FPP_ACTION_QUERY command with EMPTY element_name[] and with this group starts
                                                        a QUERY/QUERY_CONT process that will successively report all configuration elements of 
                                                        the parent fw feature.
                                             2 == STATS ; Statistics group. Command with this group can target only some statistics element.
                                                        FPP_ACTION_QUERY command with EMPTY element_name[] and with this group starts
                                                        a QUERY/QUERY_CONT process that will successively report all statistics elements of 
                                                        the parent fw feature. */

    uint8_t unit_size;     /*< Byte size of element's data unit.
                               Data unit exact size (and underlying data type) is feature and element specific.
                               See appropriate documentation of fw feature elements. */
    uint8_t index;         /*< Index into element's data (as laid out in PFE firmware) which correspond with the first data unit in the .payload */
    uint8_t count;         /*< Count of consecutive data units in the .payload */
    uint8_t payload[128];  /*< Data (composed of one or more data units). */
} fpp_fw_features_element_cmd_t;
/* [fpp_fw_feature_element_cmd_t] */

/**
 * @def         FPP_CMD_FCI_OWNERSHIP_LOCK
 * @brief       FCI command to get FCI ownership.
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FCI_OWNERSHIP_LOCK, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_FCI_OWNERSHIP_ALREADY_LOCKED <br>
 *        The FCI ownership is already held by other client.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_FCI_OWNERSHIP_LOCK  0xf500

/**
 * @def         FPP_CMD_FCI_OWNERSHIP_UNLOCK
 * @brief       FCI command to release FCI ownership.
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_FCI_OWNERSHIP_UNLOCK, 0, NULL);
 *  .............................................
 * @endcode
 *
 * Command return values
 * ---------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_FCI_OWNERSHIP_UNLOCK  0xf501

/**
 * @def         FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED
 * @hideinitializer
 */
#define FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED  0xf502

/**
 * @def         FPP_ERR_FCI_OWNERSHIP_ALREADY_LOCKED
 * @hideinitializer
 */
#define FPP_ERR_FCI_OWNERSHIP_ALREADY_LOCKED  0xf503

/**
 * @def         FPP_ERR_FCI_OWNERSHIP_NOT_OWNER
 * @hideinitializer
 */
#define FPP_ERR_FCI_OWNERSHIP_NOT_OWNER 0xf504

/**
 * @def         FPP_ERR_FCI_OWNERSHIP_NOT_ENABLED
 * @hideinitializer
 */
#define FPP_ERR_FCI_OWNERSHIP_NOT_ENABLED 0xf505

/**
 * @def FPP_CMD_HEALTH_MONITOR_EVENT
 * @brief FCI event that notifies client about some Health Monitor event from driver.
 */
#define FPP_CMD_HEALTH_MONITOR_EVENT    0xf660
#define FPP_HEALTH_MONITOR_DESC_SIZE    64

/**
 * @brief       Data structure for Health Monitor event.
 * @details     For details about events (exact meaning of ID, type and src), see Health Monitor documentation.
 */
/* [fpp_health_monitor_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(2)
{
    uint16_t action;
    uint16_t id;     /*< Event ID as reported by Health Monitor. */
    uint8_t type;    /*< 0 == information ; 1 == warning ; 2 == error */
    uint8_t src;     /*< Source (which part of PFE asserted the event) */
    char desc[FPP_HEALTH_MONITOR_DESC_SIZE];  /*< Event description */
} fpp_health_monitor_cmd_t;
/* [fpp_health_monitor_cmd_t] */


#endif /* FPP_EXT_H_ */

/** @}*/
