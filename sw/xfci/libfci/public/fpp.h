/* =========================================================================
 *  Copyright (C) 2010 Mindspeed Technologies, Inc.
 *  Copyright 2014-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */


#ifndef FPP_H_
#define FPP_H_

/*	Compiler abstraction macros */
#ifndef CAL_PACKED
#define CAL_PACKED				__attribute__((packed))
#endif /* CAL_PACKED */

#ifndef CAL_PACKED_ALIGNED
#define CAL_PACKED_ALIGNED(n)	__attribute__((packed, aligned(n)))
#endif /* CAL_PACKED_ALIGNED */

#ifndef CAL_ALIGNED
#define CAL_ALIGNED(n)	__attribute__((aligned(n)))
#endif /* CAL_ALIGNED */

/**
 * @file	fpp.h
 * @brief	The legacy FCI API
 * @details This file origin is the fpp.h file from CMM sources.
 */

#include "pfe_cfg.h"

#if !defined(__KERNEL__) && !defined(PFE_CFG_TARGET_OS_AUTOSAR)
#include <stdint.h>
#else
#include "oal_types.h"
#endif

#define IFNAMSIZ	16	/*	Maximum length of interface name */

/*--------------------------------------- General ---------------------------*/
/* Errors */
#define FPP_ERR_OK                                      0
#define FPP_ERR_UNKNOWN_COMMAND                         1
#define FPP_ERR_WRONG_COMMAND_SIZE			2
#define FPP_ERR_WRONG_COMMAND_PARAM			3
#define FPP_ERR_UNKNOWN_ACTION                          4
#define FPP_ERR_SA_ENTRY_NOT_FOUND                      909

/**
 * @brief Generic 'register' action for FPP_CMD_*.
 * @hideinitializer
 */
#define FPP_ACTION_REGISTER     0U

/**
 * @brief Generic 'deregister' action for FPP_CMD_*.
 * @hideinitializer
 */
#define FPP_ACTION_DEREGISTER   1
#define FPP_ACTION_KEEP_ALIVE   2
#define FPP_ACTION_REMOVED      3

/**
 * @brief Generic 'update' action for FPP_CMD_*.
 * @hideinitializer
 */
#define FPP_ACTION_UPDATE       4

/**
 * @brief Generic 'query' action for FPP_CMD_*.
 * @hideinitializer
 */
#define FPP_ACTION_QUERY        6

/**
 * @brief Generic 'query continue' action for FPP_CMD_*.
 * @hideinitializer
 */
#define FPP_ACTION_QUERY_CONT   7
#define FPP_ACTION_QUERY_LOCAL  8
#define FPP_ACTION_TCP_FIN      9

/*----------------------------------Tunnels-----------------------------------*/
#define FPP_ERR_TNL_ALREADY_CREATED			1004	

/*------------------------------------- Sockets ------------------------------*/
#define FPP_ERR_SOCK_ALREADY_OPEN			1200
#define FPP_ERR_SOCKID_ALREADY_USED			1201
#define FPP_ERR_SOCK_ALREADY_OPENED_WITH_OTHER_ID	1202
#define FPP_ERR_TOO_MANY_SOCKET_OPEN			1203
#define FPP_ERR_SOCKID_UNKNOWN				1204
#define FPP_ERR_SOCK_ALREADY_IN_USE			1206
#define FPP_ERR_RTP_CALLID_IN_USE 			1207
#define FPP_ERR_RTP_UNKNOWN_CALL 			1208
#define FPP_ERR_WRONG_SOCKID 				1209
#define FPP_ERR_RTP_SPECIAL_PKT_LEN 			1210
#define FPP_ERR_RTP_CALL_TABLE_FULL 			1211
#define FPP_ERR_WRONG_SOCK_FAMILY 			1212
#define FPP_ERR_WRONG_SOCK_PROTO 			1213
#define FPP_ERR_WRONG_SOCK_TYPE 			1214
#define FPP_ERR_MSP_NOT_READY				1215
#define FPP_ERR_WRONG_SOCK_MODE				1216

typedef struct {
	uint16_t id;
	uint8_t type;
	uint8_t mode;
	uint32_t saddr;
	uint32_t daddr;
	uint16_t sport;
	uint16_t dport;
	uint8_t proto;
	uint8_t queue;
	uint16_t dscp;
	uint32_t route_id;
#if defined(COMCERTO_2000) || defined(LS1043)
        uint16_t secure;
        uint16_t sa_nr_rx;
        uint16_t sa_handle_rx[4];
        uint16_t sa_nr_tx;
        uint16_t sa_handle_tx[4];
	uint16_t pad;
#endif
} __attribute__((__packed__)) fpp_socket4_open_cmd_t;

typedef struct {
	uint16_t id;
	uint16_t rsvd1;
	uint32_t saddr;
	uint16_t sport;
	uint8_t rsvd2;
	uint8_t queue;
	uint16_t dscp;
	uint16_t pad;
	uint32_t route_id;
#if defined(COMCERTO_2000) || defined(LS1043)
        uint16_t secure;
        uint16_t sa_nr_rx;
        uint16_t sa_handle_rx[4];
        uint16_t sa_nr_tx;
        uint16_t sa_handle_tx[4];
	uint16_t pad2;
#endif
} __attribute__((__packed__)) fpp_socket4_update_cmd_t;

typedef struct {
	uint16_t id;
	uint16_t pad1;
} __attribute__((__packed__)) fpp_socket4_close_cmd_t;

typedef struct {
	uint16_t id;
	uint8_t type;
	uint8_t mode;
	uint32_t saddr[4];
	uint32_t daddr[4];
	uint16_t sport;
	uint16_t dport;
	uint8_t proto;
	uint8_t queue;
	uint16_t dscp;
	uint32_t route_id;
#if defined(COMCERTO_2000) || defined(LS1043)
        uint16_t secure;
        uint16_t sa_nr_rx;
        uint16_t sa_handle_rx[4];
        uint16_t sa_nr_tx;
        uint16_t sa_handle_tx[4];
	uint16_t pad;
#endif
} __attribute__((__packed__)) fpp_socket6_open_cmd_t;

typedef struct {
	uint16_t id;
	uint16_t rsvd1;
	uint32_t saddr[4];
	uint16_t sport;
	uint8_t rsvd2;
	uint8_t queue;
	uint16_t dscp;
	uint16_t pad;
	uint32_t route_id;
#if defined(COMCERTO_2000) || defined(LS1043)
        uint16_t secure;
        uint16_t sa_nr_rx;
        uint16_t sa_handle_rx[4];
        uint16_t sa_nr_tx;
        uint16_t sa_handle_tx[4];
	uint16_t pad2;
#endif
} __attribute__((__packed__)) fpp_socket6_update_cmd_t;

typedef struct {
	uint16_t id;
	uint16_t pad1;
} __attribute__((__packed__)) fpp_socket6_close_cmd_t;

/*------------------------------------- Tunnel -------------------------------*/
#define FPP_ERR_TNL_ENTRY_NOT_FOUND                     1001

/*------------------------------------- Protocols ----------------------------*/
typedef enum {
        FPP_PROTO_IPV4 = 0,
        FPP_PROTO_IPV6,
        FPP_PROTO_PPPOE,
        FPP_PROTO_MC4,
        FPP_PROTO_MC6
} fpp_proto_t;

/*------------------------------------- Multicast ----------------------------*/
#define FPP_ERR_MC_ENTRY_NOT_FOUND			700

/*------------------------------------ Conntrack -----------------------------*/
#define FPP_ERR_CT_ENTRY_ALREADY_REGISTERED		100
#define FPP_ERR_CT_ENTRY_NOT_FOUND			101

/**
 * @addtogroup  dxgrLibFCI
 * @{
 */

/**
 * @def         FPP_CMD_IPV4_CONNTRACK
 * @brief       FCI command for management of IPv4 conntracks.
 * @details     Related topics: @ref l3_router
 * @details     Related data types: @ref fpp_ct_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new IPv4 conntrack and bind it to previously created route(s).
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing IPv4 conntrack.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of IPv4 conntrack.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) IPv4 conntrack query session and get properties 
 *                   of the first IPv4 conntrack from the internal list of IPv4 conntracks.
 *              - @c FPP_ACTION_QUERY_CONT <br> 
 *                   Continue the query session and get properties of the next IPv4 conntrack
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new IPv4 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *      
 *    .saddr = ...,        // 'orig' direction: Source IP address. [NBO]
 *    .daddr = ...,        // 'orig' direction: Destination IP address. [NBO]
 *    .sport = ...,        // 'orig' direction: Source port. [NBO]
 *    .dport = ...,        // 'orig' direction: Destination port. [NBO]
 *      
 *    .saddr_reply = ...,  // 'reply' direction: Source IP address. [NBO]
 *                         // Used for NAT, otherwise equals '.daddr'.
 *      
 *    .daddr_reply = ...,  // 'reply' direction: Destination IP address.
 *                         // Used for NAT, otherwise equals '.saddr'.
 *      
 *    .sport_reply = ...,  // 'reply' direction: Source port. [NBO]
 *                         // Used for NAT, otherwise equals '.dport'.
 *      
 *    .dport_reply = ...,  // 'reply' direction: Destination port. [NBO]
 *                         // Used for NAT, otherwise equals '.sport'.
 *      
 *    .protocol = ...,     // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    .flags = ...,        // Flags. A bitset. [NBO]
 *      
 *    .route_id = ...,     // 'orig' direction: ID of an associated route. [NBO]
 *                         // See FPP_CMD_IP_ROUTE.
 *      
 *    .route_id_reply = ...,  // 'reply' direction: ID of an associated route. [NBO]
 *                            // See FPP_CMD_IP_ROUTE.
 *      
 *    .vlan = ...,         // 'orig' direction: VLAN tag. [NBO]
 *                         // If non-zero, then this VLAN tag is added to the routed packet.
 *                         // If the packet already has a VLAN tag, then its tag is replaced.
 *      
 *    .vlan_reply = ...    // 'reply' direction: VLAN tag. [NBO]
 *                         // If non-zero, then this VLAN tag is added to the routed packet.
 *                         // If the packet already has a VLAN tag, then its tag is replaced.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 * <b>orig and reply direction</b><br>
 * By default the connection is created as bi-directional. It means that two routing table entries
 * are created at once:
 *   - one for standard flow ('orig' direction), defined by `.protocol`, `.saddr`, `.daddr`, 
 *     `.sport`, and `.dport`
 *   - one for reverse flow ('reply' direction), defined by `.protocol`, `.saddr_reply`, `.daddr_reply`,
 *     `.sport_reply` and `.dport_reply`.
 * 
 * To create an uni-directional connection (only one routing table entry), set one of these flags 
 * (@b never both) when configuring a conntrack:
 * - 'orig' direction only:  `.flags |= CTCMD_FLAGS_REP_DISABLED`, and @b don't set `.route_id_reply`.
 * - 'reply' direction only: `.flags |= CTCMD_FLAGS_ORIG_DISABLED`, and @b don't set `.route_id`.
 *
 * <b>NAT and NAPT/PAT</b><br>
 * To configure NAT or NAPT/PAT connection, set 'reply' IP addresses and ports to different values
 * than 'orig' IP addresses and ports.
 * -# `.daddr_reply != .saddr`: Source address of packets in the 'orig' direction will be changed
 *    from `.saddr` to `daddr_reply`. In case of a bi-directional connection, destination address
 *    of packets in the 'reply' direction will be changed from `.daddr_reply` to `.saddr`.
 * -# `.saddr_reply != .daddr`: Destination address of packets in the 'orig' direction will be changed
 *    from `.daddr` to `.saddr_reply`. In case of a bi-directional connection, source address of
 *    packets in the 'reply' direction will be changed from `.saddr_reply` to `.daddr`.
 * -# `.dport_reply != .sport`: Source port of packets in the 'orig' direction will be changed
 *    from `.sport` to `.dport_reply. In case of a bi-directional connection, destination port of
 *    packets in the 'reply' direction will be changed from `.dport_reply` to `.sport`.
 * -# `.sport_reply != .dport`: Destination port of packets in the 'orig' direction will be changed
 *    from `.dport` to `.sport_reply`. In case of a bi-directional connection, source port of packets
 *    in the 'reply' direction will be changed from `.sport_reply` to `.dport`.
 *
 * <b>Disable port checking</b><br>
 * It is possible to leave out ports from matching process of a particular conntrack.
 * To do so, configure the conntrack's `.sport` and `.dport` to zero.
 * This allows routing based only on 3-tuple (protocol, source IP, destination IP).
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing IPv4 conntrack. 'Orig' properties are mandatory for this action.
 * 'Reply' properties are optional.
 * @code{.c}
 *  .............................................  
 *  fpp_ct_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *      
 *    // Identification of the target conntrack.
 *    .saddr = ...,        // 'orig' direction: Source IP address. [NBO]
 *    .daddr = ...,        // 'orig' direction: Destination IP address. [NBO]
 *    .sport = ...,        // 'orig' direction: Source port. [NBO]
 *    .dport = ...         // 'orig' direction: Destination port. [NBO]
 *    .protocol = ...,     // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    .saddr_reply = ...,  // 'reply' direction: Source IP address. [NBO]
 *                         // Used for NAT, otherwise equals '.daddr'.
 *      
 *    .daddr_reply = ...,  // 'reply' direction: Destination IP address.
 *                         // Used for NAT, otherwise equals '.saddr'.
 *      
 *    .sport_reply = ...,  // 'reply' direction: Source port. [NBO]
 *                         // Used for NAT, otherwise equals '.dport'.
 *      
 *    .dport_reply = ...,  // 'reply' direction: Destination port. [NBO]
 *                         // Used for NAT, otherwise equals '.sport'.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of an IPv4 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *      
 *    // Identification of the target conntrack.
 *    .saddr = ...,     // 'orig' direction: Source IP address. [NBO]
 *    .daddr = ...,     // 'orig' direction: Destination IP address. [NBO]
 *    .sport = ...,     // 'orig' direction: Source port. [NBO]
 *    .dport = ...,     // 'orig' direction: Destination port. [NBO]
 *    .protocol = ...,  // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    // Modification of the target conntrack.
 *    .flags |= ntohs(CTCMD_FLAGS_TTL_DECREMENT)  // The only modification available: 
 *                                                // set/unset TTL decrement flag.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of an IPv4 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *    
 *  fpp_ct_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u; 
 *    
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_IPV4_CONNTRACK,
 *                  sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the first IPv4 conntrack from 
 *  //  the internal list of IPv4 conntracks.
 *    
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_IPV4_CONNTRACK,
 *                  sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the next IPv4 conntrack from 
 *  //  the internal list of IPv4 conntracks.
 *  .............................................  
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_CT_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the IPv4 conntrack query session (no more IPv4 conntracks).
 *        - For other ACTIONs: Unknown (nonexistent) IPv4 conntrack was requested.
 * - @c FPP_ERR_CT_ENTRY_ALREADY_REGISTERED <br>
 *        Requested IPv4 conntrack already exists (is already registered).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property (probably nonexistent route).
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_IPV4_CONNTRACK				0x0314

/**
 * @def         FPP_CMD_IPV6_CONNTRACK
 * @brief       FCI command for management of IPv6 conntracks.
 * @details     Related topics: @ref l3_router
 * @details     Related data types: @ref fpp_ct6_cmd_t
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new IPv6 conntrack and bind it to previously created route(s).
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing IPv6 conntrack.
 *              - @c FPP_ACTION_UPDATE <br>
 *                   Modify properties of IPv6 conntrack.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) IPv6 conntrack query session and get properties 
 *                   of the first IPv6 conntrack from the internal list of IPv6 conntracks.
 *              - @c FPP_ACTION_QUERY_CONT <br> 
 *                   Continue the query session and get properties of the next IPv6 conntrack
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new IPv6 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct6_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_REGISTER,  // Action
 *      
 *    .saddr = {...},         // 'orig' direction: Source IP address. [NBO]
 *    .daddr = {...},         // 'orig' direction: Destination IP address. [NBO]
 *    .sport =  ...,          // 'orig' direction: Source port. [NBO]
 *    .dport =  ...,          // 'orig' direction: Destination port. [NBO]
 *      
 *    .saddr_reply = {...},   // 'reply' direction: Source IP address. [NBO]
 *                            // Used for NAT, otherwise equals '.daddr'.
 *      
 *    .daddr_reply = {...},   // 'reply' direction: Destination IP address.
 *                            // Used for NAT, otherwise equals '.saddr'.
 *      
 *    .sport_reply = ...,     // 'reply' direction: Source port. [NBO]
 *                            // Used for NAT, otherwise equals '.dport'.
 *      
 *    .dport_reply = ...,     // 'reply' direction: Destination port. [NBO]
 *                            // Used for NAT, otherwise equals '.sport'.
 *      
 *    .protocol = ...,        // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    .flags = ...,           // Flags. A bitset. [NBO]
 *      
 *    .route_id = ...,        // 'orig' direction: ID of an associated route. [NBO]
 *                            // See FPP_CMD_IP_ROUTE.
 *      
 *    .route_id_reply = ...,  // 'reply' direction: ID of an associated route. [NBO]
 *                            // See FPP_CMD_IP_ROUTE.
 *      
 *    .vlan = ...,         // 'orig' direction: VLAN tag. [NBO]
 *                         // If non-zero, then this VLAN tag is added to the routed packet.
 *                         // If the packet already has a VLAN tag, then its tag is replaced.
 *      
 *    .vlan_reply = ...    // 'reply' direction: VLAN tag. [NBO]
 *                         // If non-zero, then this VLAN tag is added to the routed packet.
 *                         // If the packet already has a VLAN tag, then its tag is replaced.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 * <b>orig and reply direction</b><br>
 * By default the connection is created as bi-directional. It means that two routing table entries
 * are created at once:
 *   - one for standard flow ('orig' direction), defined by `.protocol`, `.saddr`, `.daddr`, 
 *     `.sport`, and `.dport`
 *   - one for reverse flow ('reply' direction), defined by `.protocol`, `.saddr_reply`, `.daddr_reply`,
 *     `.sport_reply` and `.dport_reply`.
 * 
 * To create an uni-directional connection (only one routing table entry), set one of these flags 
 * (@b never both) when configuring a conntrack:
 * - 'orig' direction only:  `.flags |= CTCMD_FLAGS_REP_DISABLED`, and @b don't set `.route_id_reply`.
 * - 'reply' direction only: `.flags |= CTCMD_FLAGS_ORIG_DISABLED`, and @b don't set `.route_id`.
 *
 * <b>NAT and NAPT/PAT</b><br>
 * To configure NAT or NAPT/PAT connection, set 'reply' IP addresses and ports to different values
 * than 'orig' IP addresses and ports.
 * -# `.daddr_reply != .saddr`: Source address of packets in the 'orig' direction will be changed
 *    from `.saddr` to `daddr_reply`. In case of a bi-directional connection, destination address
 *    of packets in the 'reply' direction will be changed from `.daddr_reply` to `.saddr`.
 * -# `.saddr_reply != .daddr`: Destination address of packets in the 'orig' direction will be changed
 *    from `.daddr` to `.saddr_reply`. In case of a bi-directional connection, source address of
 *    packets in the 'reply' direction will be changed from `.saddr_reply` to `.daddr`.
 * -# `.dport_reply != .sport`: Source port of packets in the 'orig' direction will be changed
 *    from `.sport` to `.dport_reply. In case of a bi-directional connection, destination port of
 *    packets in the 'reply' direction will be changed from `.dport_reply` to `.sport`.
 * -# `.sport_reply != .dport`: Destination port of packets in the 'orig' direction will be changed
 *    from `.dport` to `.sport_reply`. In case of a bi-directional connection, source port of packets
 *    in the 'reply' direction will be changed from `.sport_reply` to `.dport`.
 *
 * <b>Disable port checking</b><br>
 * It is possible to leave out ports from matching process of a particular conntrack.
 * To do so, configure the conntrack's `.sport` and `.dport` to zero.
 * This allows routing based only on 3-tuple (protocol, source IP, destination IP).
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing IPv6 conntrack. 'Orig' properties are mandatory for this action.
 * 'Reply' properties are optional.
 * @code{.c}
 *  .............................................  
 *  fpp_ct6_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *      
 *    // Identification of the target conntrack.
 *    .saddr = {...},        // 'orig' direction: Source IP address. [NBO]
 *    .daddr = {...},        // 'orig' direction: Destination IP address. [NBO]
 *    .sport =  ...,         // 'orig' direction: Source port. [NBO]
 *    .dport =  ...          // 'orig' direction: Destination port. [NBO]
 *    .protocol = ...,       // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    .saddr_reply = {...},  // 'reply' direction: Source IP address. [NBO]
 *                           // Used for NAT, otherwise equals '.daddr'.
 *      
 *    .daddr_reply = {...},  // 'reply' direction: Destination IP address.
 *                           // Used for NAT, otherwise equals '.saddr'.
 *      
 *    .sport_reply = ...,    // 'reply' direction: Source port. [NBO]
 *                           // Used for NAT, otherwise equals '.dport'.
 *      
 *    .dport_reply = ...,    // 'reply' direction: Destination port. [NBO]
 *                           // Used for NAT, otherwise equals '.sport'.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_UPDATE
 * -----------------
 * Modify properties of an IPv6 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct6_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_UPDATE,  // Action
 *      
 *    // Identification of the target conntrack.
 *    .saddr = {...},   // 'orig' direction: Source IP address. [NBO]
 *    .daddr = {...},   // 'orig' direction: Destination IP address. [NBO]
 *    .sport =  ...,    // 'orig' direction: Source port. [NBO]
 *    .dport =  ...,    // 'orig' direction: Destination port. [NBO]
 *    .protocol = ...,  // IANA IP Protocol Number (protocol ID). [NBO]
 *      
 *    // Modification of the target conntrack.
 *    .flags |= ntohs(CTCMD_FLAGS_TTL_DECREMENT)  // The only modification available: 
 *                                                // set/unset TTL decrement flag.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
 *                                                 (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of an IPv6 conntrack.
 * @code{.c}
 *  .............................................  
 *  fpp_ct6_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *    
 *  fpp_ct6_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u; 
 *    
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_IPV6_CONNTRACK,
 *                  sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the first IPv6 conntrack from 
 *  //  the internal list of IPv6 conntracks.
 *    
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_IPV6_CONNTRACK,
 *                  sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the next IPv6 conntrack from 
 *  //  the internal list of IPv6 conntracks.
 *  .............................................  
 * @endcode
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_CT_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the IPv6 conntrack query session (no more IPv6 conntracks).
 *        - For other ACTIONs: Unknown (nonexistent) IPv6 conntrack was requested.
 * - @c FPP_ERR_CT_ENTRY_ALREADY_REGISTERED <br>
 *        Requested IPv6 conntrack already exists (is already registered).
 * - @c FPP_ERR_WRONG_COMMAND_PARAM <br>
 *        Unexpected value of some property (probably nonexistent route).
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_OWNER <br>
 *        The client is not FCI owner.
 * - @c FPP_ERR_FCI_OWNERSHIP_NOT_AUTHORIZED <br>
 *        The client is not authorized get FCI ownership.
 * - @c FPP_ERR_INTERNAL_FAILURE <br>
 *        Internal FCI failure.
 *
 * @hideinitializer
 */
#define FPP_CMD_IPV6_CONNTRACK          		0x0414

/**
 * @brief       Conntrack statistics.
 * @details     Related data types: @ref fpp_ct_cmd_t and @ref fpp_ct6_cmd_t
 * @note        @b All values are in a network byte order [@b NBO].
 *
 * @snippet     fpp.h  fpp_conntrack_stats_t
 */
/* [fpp_conntrack_stats_t] */
typedef struct CAL_PACKED_ALIGNED(4) {
    uint32_t hit;        /*< Number of frames that hit the conntrack */
    uint32_t hit_bytes;  /*< Sum of bytesizes of all frames that hit the conntrack */
} fpp_conntrack_stats_t;
/* [fpp_conntrack_stats_t] */

/**
 * @brief       Data structure for IPv4 conntrack.
 * @details     Related FCI commands: @ref FPP_CMD_IPV4_CONNTRACK, @ref FPP_CMD_IP_ROUTE
 * @details     See @ref l3_router for detailed explanation how to create conntracks.
 *
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp.h  fpp_ct_cmd_t
 */
/* [fpp_ct_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4) {
    uint16_t action;          /*< Action */
    uint16_t rsvd0;           /*< RESERVED (do not use) */
    
    uint32_t saddr;           /*< 'orig' direction: Source IP address. [NBO] */
    uint32_t daddr;           /*< 'orig' direction: Destination IP address. [NBO] */
    uint16_t sport;           /*< 'orig' direction: Source port. [NBO] */
    uint16_t dport;           /*< 'orig' direction: Destination port. [NBO] */
    
    uint32_t saddr_reply;     /*< 'reply' direction: Source IP address. [NBO] 
                                   Used for NAT, otherwise equals '.daddr'. */
    
    uint32_t daddr_reply;     /*< 'reply' direction: Destination IP address. [NBO]
                                   Used for NAT, otherwise equals '.saddr'. */
    
    uint16_t sport_reply;     /*< 'reply' direction: Source port. [NBO]
                                   Used for NAT, otherwise equals '.dport'. */
    
    uint16_t dport_reply;     /*< 'reply' direction: Destination port. [NBO] 
                                   Used for NAT, otherwise equals '.sport'. */
    
    uint16_t protocol;        /*< IANA IP Protocol Number (protocol ID). [NBO] */
    uint16_t flags;           /*< Flags. A bitset. [NBO]. See FPP_CMD_IPV4_CONNTRACK. */
    uint32_t fwmark;          /*< RESERVED (do not use) */
    
    uint32_t route_id;        /*< 'orig' direction: ID of an associated route. [NBO]
                                  See FPP_CMD_IP_ROUTE. */
    
    uint32_t route_id_reply;  /*< 'reply' direction: ID of an associated route. [NBO]
                                  See FPP_CMD_IP_ROUTE. */
    
    uint16_t vlan;            /*< 'orig' direction: VLAN tag. [NBO]
                                  If non-zero, then this VLAN tag is added to the routed
                                  packet. If the packet already has a VLAN tag, then its tag
                                  is replaced. */
    
    uint16_t vlan_reply;      /*< 'reply' direction: VLAN tag. [NBO]
                                  If non-zero, then this VLAN tag is added to the routed
                                  packet. If the packet already has a VLAN tag, then its tag
                                  is replaced. */

    fpp_conntrack_stats_t CAL_PACKED_ALIGNED(4) stats;        /*< 'orig'  statistics [ro] */
    fpp_conntrack_stats_t CAL_PACKED_ALIGNED(4) stats_reply;  /*< 'reply' statistics [ro] */
} fpp_ct_cmd_t;
/* [fpp_ct_cmd_t] */

typedef struct CAL_PACKED {
	uint16_t action;                       /*Action to perform*/
	uint16_t format;                       /* bit 0 : indicates if SA info are present in command */
                                                /* bit 1 : indicates if orig Route info is present in command  */
                                                /* bit 2 : indicates if repl Route info is present in command  */
	uint32_t saddr;                        /*Source IP address*/
	uint32_t daddr;                        /*Destination IP address*/
	uint16_t sport;                        /*Source Port*/
	uint16_t dport;                        /*Destination Port*/
	uint32_t saddr_reply;
	uint32_t daddr_reply;
	uint16_t sport_reply;
	uint16_t dport_reply;
	uint16_t protocol;             /*TCP, UDP ...*/
	uint16_t flags;
	uint32_t fwmark;
	uint32_t route_id;
	uint32_t route_id_reply;
        /* optional security parameters */
        uint8_t sa_dir;
        uint8_t sa_nr;
        uint16_t sa_handle[4];
        uint8_t sa_reply_dir;
        uint8_t sa_reply_nr;
        uint16_t sa_reply_handle[4];
	uint32_t tunnel_route_id;
	uint32_t tunnel_route_id_reply;
} fpp_ct_ex_cmd_t;

/**
 * @brief       Data structure for IPv6 conntrack.
 * @details     Related FCI commands: @ref FPP_CMD_IPV6_CONNTRACK, @ref FPP_CMD_IP_ROUTE
 * @details     See @ref l3_router for detailed explanation how to create conntracks.
 *
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp.h  fpp_ct6_cmd_t
 */
/* [fpp_ct6_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4) {
    uint16_t action;          /*< Action */
    uint16_t rsvd1;           /*< RESERVED (do not use) */
    
    uint32_t saddr[4];        /*< 'orig' direction: Source IP address. [NBO] */
    uint32_t daddr[4];        /*< 'orig' direction: Destination IP address. [NBO] */
    uint16_t sport;           /*< 'orig' direction: Source port. [NBO] */
    uint16_t dport;           /*< 'orig' direction: Destination port. [NBO] */
    
    uint32_t saddr_reply[4];  /*< 'reply' direction: Source IP address. [NBO] 
                                   Used for NAT, otherwise equals '.daddr'. */
    
    uint32_t daddr_reply[4];  /*< 'reply' direction: Destination IP address. [NBO]
                                   Used for NAT, otherwise equals '.saddr'. */
    
    uint16_t sport_reply;     /*< 'reply' direction: Source port. [NBO]
                                   Used for NAT, otherwise equals '.dport'. */
    
    uint16_t dport_reply;     /*< 'reply' direction: Destination port. [NBO] 
                                   Used for NAT, otherwise equals '.sport'. */
    
    uint16_t protocol;        /*< IANA IP Protocol Number (protocol ID). [NBO] */
    uint16_t flags;           /*< Flags. A bitset. [NBO. See FPP_CMD_IPV4_CONNTRACK. */
    uint32_t fwmark;          /*< RESERVED (do not use) */
    
    uint32_t route_id;        /*< 'orig' direction: ID of an associated route. [NBO]
                                  See FPP_CMD_IP_ROUTE. */
    
    uint32_t route_id_reply;  /*< 'reply' direction: ID of an associated route. [NBO]
                                  See FPP_CMD_IP_ROUTE. */
    
    uint16_t vlan;            /*< 'orig' direction: VLAN tag. [NBO]
                                  If non-zero, then this VLAN tag is added to the routed
                                  packet. If the packet already has a VLAN tag, then its tag
                                  is replaced. */
    
    uint16_t vlan_reply;      /*< 'reply' direction: VLAN tag. [NBO]
                                  If non-zero, then this VLAN tag is added to the routed
                                  packet. If the packet already has a VLAN tag, then its tag
                                  is replaced. */

    fpp_conntrack_stats_t CAL_PACKED_ALIGNED(4) stats;        /*< 'orig'  statistics [ro] */
    fpp_conntrack_stats_t CAL_PACKED_ALIGNED(4) stats_reply;  /*< 'reply' statistics [ro] */
} fpp_ct6_cmd_t;
/* [fpp_ct6_cmd_t] */

typedef struct CAL_PACKED {
	uint16_t action;                       /*Action to perform*/
	uint16_t format;                       /* indicates if SA info are present in command */
	uint32_t saddr[4];                     /*Source IP address*/
	uint32_t daddr[4];                     /*Destination IP address*/
	uint16_t sport;                        /*Source Port*/
	uint16_t dport;                        /*Destination Port*/
	uint32_t saddr_reply[4];
	uint32_t daddr_reply[4];
	uint16_t sport_reply;
	uint16_t dport_reply;
	uint16_t protocol;                     /*TCP, UDP ...*/
	uint16_t flags;
	uint32_t fwmark;
	uint32_t route_id;
	uint32_t route_id_reply;
	uint8_t sa_dir;
	uint8_t sa_nr;
	uint16_t sa_handle[4];
	uint8_t sa_reply_dir;
	uint8_t sa_reply_nr;
	uint16_t sa_reply_handle[4];
	uint32_t tunnel_route_id;
	uint32_t tunnel_route_id_reply;
} fpp_ct6_ex_cmd_t;

/** @}*/

/*--------------------------------------- IP ---------------------------------*/ 
#define FPP_ERR_RT_ENTRY_ALREADY_REGISTERED		200
#define FPP_ERR_RT_ENTRY_NOT_FOUND			201

/**
 * @addtogroup  dxgrLibFCI
 * @{
 */

/**
 * @def         FPP_CMD_IP_ROUTE
 * @brief       FCI command for management of IP routes.
 * @details     Related topics: @ref l3_router
 * @details     Related data types: @ref fpp_rt_cmd_t
 * @details     In the context of PFE, a route represents a direction where the matching 
 *              traffic shall be forwarded to. Every route specifies an egress physical interface
 *              and a MAC address of the next network node.
 * @details     Supported `.action` values:
 *              - @c FPP_ACTION_REGISTER <br>
 *                   Create a new route.
 *              - @c FPP_ACTION_DEREGISTER <br>
 *                   Remove (destroy) an existing route.
 *              - @c FPP_ACTION_QUERY <br>
 *                   Initiate (or reinitiate) a route query session and get properties 
 *                   of the first route from the internal collective list of all routes
 *                   (regardless of IP type nor conntrack affiliation).
 *              - @c FPP_ACTION_QUERY_CONT <br> 
 *                   Continue the query session and get properties of the next route
 *                   from the list. Intended to be called in a loop (to iterate through the list).
 *
 * FPP_ACTION_REGISTER
 * -------------------
 * Create a new route. For detailed info about route properties, see fpp_rt_cmd_t.
 * @code{.c}
 *  .............................................  
 *  fpp_rt_cmd_t cmd_to_fci = 
 *  {
 *    .action  = FPP_ACTION_REGISTER,  // Action
 *    .src_mac = ...,                  // Source MAC address.
 *    .dst_mac = ...,                  // Destination MAC address.
 *    .output_device = ...,            // Name of the egress physical interface.
 *    .id    = ...,                    // Route ID. [NBO]. User-defined.
 *    .flags = ...                     // Flags. [NBO]. 1 for IPv4 routes, 2 for IPv6 routes.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
 *                                           (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_DEREGISTER
 * ---------------------
 * Remove (destroy) an existing route.
 * @code{.c}
 *  .............................................  
 *  fpp_rt_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_DEREGISTER,  // Action
 *    .id     = ...                     // Route ID. [NBO]. User-defined.
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
 *                                           (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
 * FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * ------------------------------------------
 * Get properties of a route.
 * @code{.c}
 *  .............................................  
 *  fpp_rt_cmd_t cmd_to_fci = 
 *  {
 *    .action = FPP_ACTION_QUERY  // Action
 *  };
 *    
 *  fpp_rt_cmd_t reply_from_fci = {0};
 *  unsigned short reply_length = 0u; 
 *    
 *  int rtn = 0;
 *  rtn = fci_query(client, FPP_CMD_IP_ROUTE,
 *                  sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the first route from 
 *  //  the internal collective list of all routes.
 *    
 *  cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
 *  rtn = fci_query(client, FPP_CMD_IP_ROUTE,
 *                  sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
 *                  &reply_length, (unsigned short*)(&reply_from_fci));
 *    
 *  // 'reply_from_fci' now holds properties of the next route from 
 *  //  the internal collective list of all routes.
 *  .............................................  
 * @endcode 
 *
 * Command return values (for all applicable ACTIONs)
 * --------------------------------------------------
 * - @c FPP_ERR_OK <br>
 *        Success
 * - @c FPP_ERR_RT_ENTRY_NOT_FOUND
 *        - For FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT: The end of the route query session (no more routes).
 *        - For other ACTIONs: Unknown (nonexistent) route was requested.
 * - @c FPP_ERR_RT_ENTRY_ALREADY_REGISTERED <br>
 *        Requested route already exists (is already registered).
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
#define FPP_CMD_IP_ROUTE			0x0313

/**
 * @def         FPP_CMD_IPV4_RESET
 * @brief       FCI command to remove all IPv4 routes and conntracks. 
 * @details     Related topics: @ref l3_router, @ref FPP_CMD_IP_ROUTE, <br> @ref FPP_CMD_IPV4_CONNTRACK
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................  
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV4_RESET, 0, NULL); 
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
#define FPP_CMD_IPV4_RESET			0x0316
#define FPP_CMD_IP_ROUTE_CHANGE			0x0318

/**
 * @def         FPP_CMD_IPV6_RESET
 * @brief       FCI command to remove all IPv6 routes and conntracks. 
 * @details     Related topics: @ref l3_router, @ref FPP_CMD_IP_ROUTE, <br> @ref FPP_CMD_IPV6_CONNTRACK
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................  
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV6_RESET, 0, NULL); 
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
#define FPP_CMD_IPV6_RESET				0x0416

/**
 * @brief       Data structure for a route.
 * @details     Related FCI commands: @ref FPP_CMD_IP_ROUTE
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp.h  fpp_rt_cmd_t
 */
/* [fpp_rt_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4) {
    uint16_t action;        /*< Action */
    uint16_t mtu;           /*< RESERVED (do not use) */
    
    uint8_t src_mac[6];     /*< Source MAC address. When a packet is routed, this address 
                                is set as the source MAC address of the packet. If left
                                unset (all-zero), then PFE automatically uses MAC address
                                of the associated physical interface (.output_device). */
    
    uint8_t dst_mac[6];     /*< Destination MAC address. When a packet is routed, this address
                                is set as the destination MAC address of the packet. */
    
    uint16_t pad;           /*< RESERVED (do not use) */
    
    char output_device[IFNAMSIZ];   /*< Name of the egress physical interface. 
                                        When a packet is routed, it is egressed
                                        through this physical interface. */
    
    char input_device[IFNAMSIZ];            /*< RESERVED (do not use) */
    char underlying_input_device[IFNAMSIZ]; /*< RESERVED (do not use) */
    
    uint32_t id;            /*< Route ID. [NBO]. Unique route identifier. */
    uint32_t flags;         /*< Flags. [NBO]. 1 for IPv4 routes, 2 for IPv6 routes. */
    
    uint32_t dst_addr[4];   /*< RESERVED (do not use) */
} fpp_rt_cmd_t;
/* [fpp_rt_cmd_t] */

#define FPP_IP_ROUTE_6o4	(1<<0)
#define FPP_IP_ROUTE_4o6	(1<<1)

/* Structure representing the command sent to enable/disable Ipsec pre-fragmentation */
typedef struct {
	uint16_t pre_frag_en;
	uint16_t rsvd;
} __attribute__((__packed__)) fpp_ipsec_cmd_t;

/** @}*/

/* ----------------------------------- RTP ----------------------------------*/
#define FPP_ERR_RTP_STATS_MAX_ENTRIES			1230
#define FPP_ERR_RTP_STATS_STREAMID_ALREADY_USED	1231
#define FPP_ERR_RTP_STATS_STREAMID_UNKNOWN		1232
#define FPP_ERR_RTP_STATS_DUPLICATED			1233
#define FPP_ERR_RTP_STATS_WRONG_DTMF_PT			1234
#define FPP_ERR_RTP_STATS_WRONG_TYPE			1235


#define FPP_CMD_RTP_OPEN		0x0801
#define FPP_CMD_RTP_UPDATE		0x0802
#define FPP_CMD_RTP_TAKEOVER	0x0803
#define FPP_CMD_RTP_CONTROL		0x0804
#define FPP_CMD_RTP_SPECTX_PLD	0x0805
#define FPP_CMD_RTP_SPECTX_CTRL	0x0806
#define FPP_CMD_RTCP_QUERY		0x0807
#define FPP_CMD_RTP_CLOSE		0x0808

#define FPP_RTP_TAKEOVER_MODE_TSINCR_FREQ	1
#define FPP_RTP_TAKEOVER_MODE_SSRC			2

#define FPP_MAX_SPTX_STRING_SIZE	160

typedef struct {
	uint16_t	call_id;
	uint16_t	socket_a;
	uint16_t	socket_b;
	uint16_t	rsvd;
} __attribute__((__packed__)) fpp_rtp_open_cmd_t;

typedef struct {
	uint16_t	call_id;
	uint16_t	rsvd;
} __attribute__((__packed__)) fpp_rtp_close_cmd_t;

typedef struct {
	uint16_t	call_id;
	uint16_t	socket;
	uint16_t	mode;
	uint16_t	seq_number_base;
	uint32_t	ssrc;
	uint32_t	ts_base;
	uint32_t	ts_incr;
} __attribute__((__packed__)) fpp_rtp_takeover_cmd_t;

typedef struct {
	uint16_t	call_id;
	uint16_t	control_dir;
} __attribute__((__packed__)) fpp_rtp_ctrl_cmd_t;

#define FPP_RTP_SPEC_TX_START       0
#define FPP_RTP_SPEC_TX_RESPONSE    1
#define FPP_RTP_SPEC_TX_STOP        2

typedef struct {
        uint16_t	call_id;
        uint16_t	type;
} __attribute__((__packed__)) fpp_rtp_spec_tx_ctrl_cmd_t;

typedef struct {
        uint16_t	call_id;
        uint16_t	payload_id;
        uint16_t	payload_length;
        uint16_t	payload[80];
} __attribute__((__packed__)) fpp_rtp_spec_tx_payload_cmd_t;

typedef struct {
	uint16_t	socket_id;
	uint16_t       flags;
} __attribute__((__packed__)) fpp_rtcp_query_cmd_t;

typedef struct {
	uint32_t	prev_reception_period;
	uint32_t	last_reception_period;
	uint32_t	num_tx_pkts;
	uint32_t	num_rx_pkts;
	uint32_t	last_rx_seq;
	uint32_t	last_rx_timestamp;
	uint8_t	rtp_header[12];
	uint32_t	num_rx_dup;
	uint32_t	num_rx_since_rtcp;
	uint32_t	num_tx_bytes;
	uint32_t	min_jitter;
	uint32_t	max_jitter;
	uint32_t	average_jitter;
	uint32_t	num_rx_lost_pkts;
	uint32_t	min_reception_period;
	uint32_t	max_reception_period;
	uint32_t	average_reception_period;
	uint32_t	num_malformed_pkts;
	uint32_t	num_expected_pkts;
	uint32_t	num_late_pkts;
	uint16_t	sport;
	uint16_t	dport;
	uint32_t	num_cumulative_rx_lost_pkts;
	uint32_t	ssrc_overwrite_value;
} __attribute__((__packed__)) fpp_rtcp_query_res_t;

/*-------------------------------- RTP QoS Measurement-----------------------*/
#define	FPP_CMD_RTP_STATS_ENABLE		0x0810
#define	FPP_CMD_RTP_STATS_DISABLE		0x0811
#define	FPP_CMD_RTP_STATS_QUERY			0x0812
#define	FPP_CMD_RTP_STATS_DTMF_PT		0x0813

#define FPP_RTPSTATS_TYPE_IP4		0
#define FPP_RTPSTATS_TYPE_IP6		1
#define FPP_RTPSTATS_TYPE_MC4		2
#define FPP_RTPSTATS_TYPE_MC6		3
#define FPP_RTPSTATS_TYPE_RLY		4
#define FPP_RTPSTATS_TYPE_RLY6		5

typedef struct {
	uint16_t stream_id;
	uint16_t stream_type;
	uint32_t saddr[4];
	uint32_t daddr[4];
	uint16_t sport;
	uint16_t dport;
	uint16_t proto;
	uint16_t mode;
} __attribute__((__packed__)) fpp_rtp_stat_enable_cmd_t;

typedef struct {
	uint16_t stream_id;
} __attribute__((__packed__)) fpp_rtp_stat_disable_cmd_t;

typedef struct  {
	uint16_t pt; /* 2 payload types coded on 8bits */
} __attribute__((__packed__)) fpp_rtp_stat_dtmf_pt_cmd_t;


/*-------------------------------- Voice Buffer-----------------------*/
#define FPP_CMD_VOICE_BUFFER_LOAD	0x0820
#define FPP_CMD_VOICE_BUFFER_UNLOAD	0x0821
#define FPP_CMD_VOICE_BUFFER_START	0x0822
#define FPP_CMD_VOICE_BUFFER_STOP	0x0823
#define FPP_CMD_VOICE_BUFFER_RESET	0x0824

#define FPP_VOICE_BUFFER_SCATTER_MAX	48

typedef struct {
	uint16_t buffer_id;
	uint16_t payload_type;
	uint16_t frame_size;
	uint16_t entries;
	uint32_t data_len;
	uint8_t page_order[FPP_VOICE_BUFFER_SCATTER_MAX];
	uint32_t addr[FPP_VOICE_BUFFER_SCATTER_MAX];
} __attribute__((__packed__)) fpp_voice_buffer_load_cmd_t;

typedef struct {
	uint16_t buffer_id;
} __attribute__((__packed__)) fpp_voice_buffer_unload_cmd_t;

typedef struct {
	uint16_t socket_id;
	uint16_t buffer_id;
	uint16_t seq_number_base;
	uint16_t padding;
	uint32_t ssrc;
	uint32_t timestamp_base;
} __attribute__((__packed__)) fpp_voice_buffer_start_cmd_t;

typedef struct {
        uint16_t socket_id;
} __attribute__((__packed__)) fpp_voice_buffer_stop_cmd_t;
/*-------------------------------- Exceptions -------------------------------*/
#define FPP_CMD_EXPT_QUEUE_DSCP             0x0C01
#define FPP_CMD_EXPT_QUEUE_CONTROL          0x0C02
#define FPP_CMD_EXPT_QUEUE_RESET            0x0C03

#define FPP_EXPT_Q0 0
#define FPP_EXPT_Q1 1
#define FPP_EXPT_Q2 2
#define FPP_EXPT_Q3 3
#define FPP_EXPT_MAX_QUEUE	FPP_EXPT_Q3

#define FPP_EXPT_MAX_DSCP   63

typedef struct {
	uint16_t	queue;
	uint16_t	num_dscp;
	uint8_t	dscp[FPP_EXPT_MAX_DSCP];
	uint8_t	pad;
} __attribute__((__packed__)) fpp_expt_queue_dscp_cmd_t;

typedef struct {
	uint16_t	queue;
	uint16_t	pad;
} __attribute__((__packed__)) fpp_expt_queue_control_cmd_t;

/*--------------------------------------------- QM ---------------------------*/
/* 0x0200 -> 0x02FF : QM module */
#define FPP_CMD_QM_QOSENABLE		0x0201
#define FPP_CMD_QM_QOSALG		0x0202
#define FPP_CMD_QM_NHIGH		0x0203
#define FPP_CMD_QM_MAX_TXDEPTH		0x0204
#define FPP_CMD_QM_MAX_QDEPTH		0x0205
#define FPP_CMD_QM_MAX_WEIGHT		0x0206
#define FPP_CMD_QM_RATE_LIMIT		0x0207
#define FPP_CMD_QM_EXPT_RATE		0x020c
#define FPP_CMD_QM_QUERY		0x020d
#define FPP_CMD_QM_QUERY_EXPT_RATE	0x020e
                
#define FPP_CMD_QM_RESET		0x0210 
#define FPP_CMD_QM_SHAPER_CFG		0x0211 
#define FPP_CMD_QM_SCHED_CFG		0x0212 
#define FPP_CMD_QM_DSCP_MAP		0x0213 
#define FPP_CMD_QM_QUEUE_QOSENABLE    0x0214

#define FPP_CMD_QM_QUERY_PORTINFO	0x0220
#define FPP_CMD_QM_QUERY_QUEUE		0x0221
#define FPP_CMD_QM_QUERY_SHAPER		0x0222
#define FPP_CMD_QM_QUERY_SCHED		0x0223

#define FPP_MAX_DSCP        		63
#define FPP_NUM_DSCP        		64

#if defined(COMCERTO_2000) ||  defined(LS1043)
#define FPP_NUM_QUEUES 			16
#define	FPP_PORT_SHAPER_NUM		0xffff
#else
#define FPP_NUM_QUEUES 			32
#endif

#ifndef LS1043
#define FPP_NUM_SHAPERS			8
#define FPP_NUM_SCHEDULERS		8
#else
#define FPP_NUM_SHAPERS			1
#define FPP_NUM_SCHEDULERS		1
#endif

#define FPP_EXPT_TYPE_ETH   0x0
#define FPP_EXPT_TYPE_WIFI  0x1
#define FPP_EXPT_TYPE_ARP   0x2
#define FPP_EXPT_TYPE_PCAP  0x3

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t	enable;
} __attribute__((__packed__)) fpp_qm_qos_enable_cmd_t;              

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t enable_flag;
	uint32_t queue_qosenable_mask; /* Bit mask of queues on which Qos is enabled */
} __attribute__((__packed__)) fpp_qm_queue_qos_enable_cmd_t;


typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	scheduler;
} __attribute__((__packed__)) fpp_qm_qos_alg_cmd_t;
                
typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	number_high_queues;
} __attribute__((__packed__)) fpp_qm_nhigh_cmd_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	max_bytes;
} __attribute__((__packed__)) fpp_qm_max_txdepth_cmd_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	qtxdepth[FPP_NUM_QUEUES];
} __attribute__((__packed__)) fpp_qm_max_qdepth_cmd_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	qxweight[FPP_NUM_QUEUES];
} __attribute__((__packed__)) fpp_qm_max_weight_cmd_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	enable;
        uint32_t	queues;
        uint32_t	rate;
        uint32_t	bucket_size;
} __attribute__((__packed__)) fpp_qm_rate_limit_cmd_t;

typedef struct {
        uint16_t	if_type;
        uint16_t	pkts_per_msec;
} __attribute__((__packed__)) fpp_qm_expt_rate_cmd_t;

typedef struct
{
        uint16_t	action;
        uint16_t	mask;
        uint32_t	aggregate_bandwidth;
        uint32_t	bucketsize;
} __attribute__((__packed__)) fpp_qm_query_rl_t;

#ifndef COMCERTO_2000
typedef struct
{
	uint16_t action;
	uint16_t port;
	uint32_t queue_qosenable_mask;         /* bit mask of queues on which Qos is enabled */
	uint32_t max_txdepth;

	uint32_t shaper_qmask[FPP_NUM_SHAPERS];			/* mask of queues assigned to this shaper */
	uint32_t tokens_per_clock_period[FPP_NUM_SHAPERS];	/* bits worth of tokens available on every 1 msec clock period */
	uint32_t bucket_size[FPP_NUM_SHAPERS];		/* max bucket size in bytes */

	uint32_t sched_qmask[FPP_NUM_SCHEDULERS];
	uint8_t sched_alg[FPP_NUM_SCHEDULERS];				/* current scheduling algorithm */
	
	uint16_t max_qdepth[FPP_NUM_QUEUES];
} __attribute__((__packed__)) fpp_qm_query_cmd_t;
#endif

#if defined(COMCERTO_2000) || defined(LS1043)
typedef struct fpp_qm_query_portinfo_cmd
{
	uint16_t status;
#ifndef LS1043
	uint16_t port;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint32_t queue_qosenable_mask;         /* bit mask of queues on which Qos is enabled */
	uint16_t max_txdepth;			/* ignored on C2000 */
	uint8_t  ifg;
	uint8_t  unused;
} __attribute__((__packed__)) fpp_qm_query_portinfo_cmd_t;

typedef struct fpp_qm_query_queue_cmd
{
	uint16_t status;
#ifndef LS1043
	uint16_t port;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t queue_num;
	uint16_t qweight;
	uint16_t max_qdepth;
	uint16_t unused;
} __attribute__((__packed__)) fpp_qm_query_queue_cmd_t;

typedef struct fpp_qm_query_shaper_cmd
{
	uint16_t status;
#ifndef LS1043
	uint16_t port;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t shaper_num;
	uint8_t  enabled;
	uint8_t  unused;
	uint32_t qmask;
	uint32_t rate;
	uint32_t bucket_size;
} __attribute__((__packed__)) fpp_qm_query_shaper_cmd_t;

typedef struct fpp_qm_query_sched_cmd
{
	uint16_t status;
#ifndef LS1043
	uint16_t port;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t sched_num;
	uint8_t  alg;
	uint8_t  unused;
	uint32_t qmask;
} __attribute__((__packed__)) fpp_qm_query_sched_cmd_t;
#endif

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
	uint16_t	pad;
} __attribute__((__packed__)) fpp_qm_reset_cmd_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	shaper;
        uint16_t	enable;
        uint8_t	ifg;
        uint8_t	ifg_change_flag;
        uint32_t	rate;
        uint32_t	bucket_size;
        uint32_t	queues;
} __attribute__((__packed__)) fpp_qm_shaper_cfg_t;

typedef struct {
#ifndef LS1043
	uint16_t	interface;
#else
	uint8_t	interface[IFNAMSIZ];
#endif
        uint16_t	scheduler;
        uint8_t	algo;
        uint8_t	algo_change_flag;
	uint16_t	pad;
        uint32_t	queues;
} __attribute__((__packed__)) fpp_qm_scheduler_cfg_t;

typedef struct {
        uint16_t	queue;
        uint16_t	num_dscp;
        uint8_t	dscp[FPP_NUM_DSCP];
} __attribute__((__packed__)) fpp_qm_dscp_queue_mod_t;

/*---------------------------------------- RX --------------------------------*/
/*Function codes*/
/* 0x00xx : Rx module */
#define FPP_CMD_RX_CNG_ENABLE			0x0003
#define FPP_CMD_RX_CNG_DISABLE			0x0004
#define FPP_CMD_RX_CNG_SHOW			0x0005

#define FPP_CMD_RX_L2BRIDGE_ENABLE		0x0008
#define FPP_CMD_RX_L2BRIDGE_ADD			0x0009
#define FPP_CMD_RX_L2BRIDGE_REMOVE		0x000a
#define FPP_CMD_RX_L2BRIDGE_QUERY_STATUS	0x000b
#define FPP_CMD_RX_L2BRIDGE_QUERY_ENTRY		0x000c
#define FPP_CMD_RX_L2FLOW_ENTRY		0x000d
#define FPP_CMD_RX_L2BRIDGE_MODE			0x000e
#define FPP_CMD_RX_L2BRIDGE_FLOW_TIMEOUT	0x000f
#define FPP_CMD_RX_L2BRIDGE_FLOW_RESET		0x0010


#define FPP_CMD_L2BRIDGE_LRN_TIMEOUT   0x0020
#define FPP_CMD_L2BRIDGE_LRN_RESET     0x0021
#define FPP_CMD_L2BRIDGE_ENABLE_PORT   0x0022
#define FPP_CMD_L2BRIDGE_ADD_ENTRY     0x0023
#define FPP_CMD_L2BRIDGE_REMOVE_ENTRY  0x0024
#define FPP_CMD_L2BRIDGE_QUERY_ENTRY   0x0025
#define FPP_CMD_L2BRIDGE_QUERY_STATUS  0x0026
#define FPP_CMD_L2BRIDGE_MODE          0x0027

#define FPP_ERR_BRIDGE_ENTRY_NOT_FOUND  50
#define FPP_ERR_BRIDGE_ENTRY_ALREADY_EXISTS  51

#define FPP_BRIDGE_QMOD_NONE 			0
#define FPP_BRIDGE_QMOD_DSCP			1

#define FPP_L2_BRIDGE_MODE_MANUAL	0	
#define FPP_L2_BRIDGE_MODE_AUTO		1
#define FPP_L2_BRIDGE_MODE_LEARNING    2
#define FPP_L2_BRIDGE_MODE_NO_LEARNING 3

typedef struct {
	uint16_t	interface;
	uint16_t	acc_value;
	uint16_t	on_thr;
	uint16_t	off_thr;
	uint32_t	flag;
	uint32_t	val1;
	uint32_t	val2;
} __attribute__((__packed__)) fpp_rx_icc_enable_cmd_t;

typedef struct {
	uint16_t	interface;
} __attribute__((__packed__)) fpp_rx_icc_disable_cmd_t;

typedef struct {
	uint16_t	padding_in_rc_out;
	uint16_t	state;
	uint16_t	acc_value;
	uint16_t	on_thr;
	uint16_t	off_thr;
} __attribute__((__packed__)) fpp_rx_icc_show_return_cmd_t;

/* L2 Bridging Enable command */
typedef struct {
	uint16_t	interface;
	uint16_t	enable_flag;
	char  		input_name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_l2_bridge_enable_cmd_t;


/* L2 Bridging Add Entry command */
typedef struct {
	uint16_t	input_interface;
	uint16_t	input_svlan;
	uint16_t	input_cvlan;
	uint8_t	destaddr[6];
	uint8_t	srcaddr[6];
	uint16_t	ethertype;
	uint16_t	output_interface;
	uint16_t	output_svlan;
	uint16_t	output_cvlan;
	uint16_t	pkt_priority;
 	uint16_t	svlan_priority;
 	uint16_t	cvlan_priority;
	char		input_name[IFNAMSIZ];
	char		output_name[IFNAMSIZ];
	uint16_t	queue_modifier;
	uint16_t	session_id;
} __attribute__((__packed__)) fpp_l2_bridge_add_entry_cmd_t;


/* L2 Bridging Remove Entry command */
typedef struct {
	uint16_t	input_interface;
	uint16_t	input_svlan;
	uint16_t	input_cvlan;
        uint8_t	destaddr[6];
        uint8_t	srcaddr[6];
	uint16_t	ethertype;
	uint16_t	session_id;
	uint16_t	reserved;
	char  		input_name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_l2_bridge_remove_entry_cmd_t;


/* L2 Bridging Query Status response */
typedef struct {
	uint16_t ackstatus;
	uint16_t status;
	uint8_t ifname[IFNAMSIZ];
	uint32_t eof;
} __attribute__((__packed__)) fpp_l2_bridge_query_status_response_t;


/* L2 Bridging Query Entry response */
typedef struct {
	uint16_t	ackstatus;
	uint16_t	eof;
	uint16_t	input_interface;
	uint16_t	input_svlan;
	uint16_t	input_cvlan;
	uint8_t	destaddr[6];
	uint8_t	srcaddr[6];
	uint16_t	ethertype;
	uint16_t	output_interface;
	uint16_t	output_svlan;
	uint16_t	output_cvlan;
	uint16_t	pkt_priority;
	uint16_t	svlan_priority;
	uint16_t	cvlan_priority;
	char		input_name[IFNAMSIZ];
	char		output_name[IFNAMSIZ];
	uint16_t	queue_modifier;
	uint16_t	session_id;
} __attribute__((__packed__)) fpp_l2_bridge_query_entry_response_t;

/* L2 Bridging  Flow entry command */
typedef struct {
	uint16_t action;				/*Action to perform*/
	uint16_t	ethertype;			/* If VLAN Tag !=0, ethertype of next header */
	uint8_t	destaddr[6];			/* Dst MAC addr */
	uint8_t	srcaddr[6];			/* Src MAC addr */
	uint16_t	svlan_tag; 			/* S TCI */
	uint16_t	cvlan_tag; 			/* C TCI */
	uint16_t	session_id;			/* Meaningful only if ethertype PPPoE */
	uint16_t	pad1;
	char		input_name[IFNAMSIZ];		/* Input itf name */
	char		output_name[IFNAMSIZ];	/* Output itf name */
	/* L3-4 optional information*/
	uint32_t saddr[4];
	uint32_t daddr[4];
	uint16_t sport;
	uint16_t dport;
	uint8_t proto;
	uint8_t pad;
	uint16_t mark;				/* QoS Mark*/
	uint32_t timeout;			/* Entry timeout only for QUERY */
} __attribute__((__packed__)) fpp_l2_bridge_flow_entry_cmd_t;

/* L2 Bridging Control command */
typedef struct {
	uint16_t mode_timeout;		/* Either set bridge mode or set timeout for flow entries */
} __attribute__((__packed__)) fpp_l2_bridge_control_cmd_t;

/*------------------------------------- Stat ----------------------------------*/
/*Function codes*/
/* 0x00xx : Stat module */
#define FPP_CMD_STAT_ENABLE		0x0E01 
#define FPP_CMD_STAT_QUEUE		0x0E02  
#define FPP_CMD_STAT_INTERFACE_PKT	0x0E03
#define FPP_CMD_STAT_CONNECTION		0x0E04
#define FPP_CMD_STAT_PPPOE_STATUS	0x0E05
#define FPP_CMD_STAT_PPPOE_ENTRY	0x0E06
#define FPP_CMD_STAT_BRIDGE_STATUS	0x0E07
#define FPP_CMD_STAT_BRIDGE_ENTRY	0x0E08
#define FPP_CMD_STAT_IPSEC_STATUS	0x0E09
#define FPP_CMD_STAT_IPSEC_ENTRY	0x0E0A
#define FPP_CMD_STAT_VLAN_STATUS        0x0E0B
#define FPP_CMD_STAT_VLAN_ENTRY         0x0E0C
#define FPP_CMD_STAT_TUNNEL_STATUS      0x0E0D
#define FPP_CMD_STAT_TUNNEL_ENTRY       0x0E0E
#define FPP_CMD_STAT_FLOW               0x0E0F

#define FPP_CMM_STAT_RESET		0x0001
#define FPP_CMM_STAT_QUERY		0x0002
#define FPP_CMM_STAT_QUERY_RESET	0x0003

#define FPP_CMM_STAT_ENABLE		0x0001
#define FPP_CMM_STAT_DISABLE		0x0000

/* Definitions of Bit Masks for the features */
#define FPP_STAT_QUEUE_BITMASK		0x00000001
#define FPP_STAT_INTERFACE_BITMASK	0x00000002
#define FPP_STAT_PPPOE_BITMASK		0x00000008
#define FPP_STAT_BRIDGE_BITMASK		0x00000010
#define FPP_STAT_IPSEC_BITMASK		0x00000020
#define FPP_STAT_VLAN_BITMASK 		0x00000040
#define FPP_STAT_TUNNEL_BITMASK 	0x00000080
#define FPP_STAT_FLOW_BITMASK 	        0x00000100

#define FPP_STAT_UNKNOWN_CMD		0
#define FPP_STAT_ENABLE_CMD		1 
#define FPP_STAT_QUEUE_CMD		2
#define FPP_STAT_INTERFACE_PKT_CMD	3
#define FPP_STAT_CONNECTION_CMD		4
#define FPP_STAT_PPPOE_CMD		5
#define FPP_STAT_BRIDGE_CMD		6
#define FPP_STAT_IPSEC_CMD		7
#define FPP_STAT_VLAN_CMD               8
#define FPP_STAT_TUNNEL_CMD             9
#define FPP_STAT_FLOW_CMD              10

#define FPP_ERR_FLOW_ENTRY_NOT_FOUND	1600
#define FPP_ERR_INVALID_IP_FAMILY	1601

typedef struct {
	uint16_t	action; /* 1 - Enable, 0 - Disable */
	uint16_t	pad;
	uint32_t	bitmask; /* Specifies the feature to be enabled or disabled */
} __attribute__((__packed__)) fpp_stat_enable_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	interface;
	uint16_t	queue;
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_queue_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	interface;
} __attribute__((__packed__)) fpp_stat_interface_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_connection_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_pppoe_status_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_bridge_status_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_ipsec_status_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_vlan_status_cmd_t;

typedef struct {
	uint16_t	action; /* Reset, Query, Query & Reset */
	uint16_t	pad;
        char            if_name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_stat_tunnel_status_cmd_t;

typedef struct {
	uint8_t	action;
	uint8_t	pad;
	uint8_t	ip_family;
	uint8_t	Protocol;
	uint16_t	Sport;		/*Source Port*/
	uint16_t	Dport;		/*Destination Port*/
	union {
		struct {
			uint32_t	Saddr;		/*Source IPv4 address*/
			uint32_t	Daddr;		/*Destination IPv4 address*/
		} v4;
		struct {
			uint32_t	Saddr_v6[4];		/*Source IPv6 address*/
			uint32_t	Daddr_v6[4];		/*Destination IPv6 address*/
		} v6;
	} ipv;
} __attribute__((__packed__)) fpp_stat_flow_status_cmd_t;

typedef struct {
	uint16_t	ackstatus;
	uint16_t	rsvd1;
	uint32_t	peak_queue_occ;
	uint32_t	emitted_pkts;
	uint32_t	dropped_pkts;
} __attribute__((__packed__)) fpp_stat_queue_response_t;

typedef struct {
	uint16_t	ackstatus;
	uint16_t	rsvd1;
	uint32_t	total_pkts_transmitted;
	uint32_t	total_pkts_received;
	uint32_t	total_bytes_transmitted[2]; /* 64 bit counter stored as 2*32 bit counters */
	uint32_t	total_bytes_received[2]; /* 64 bit counter stored as 2*32 bit counters */
} __attribute__((__packed__)) fpp_stat_interface_pkt_response_t;

typedef struct {
	uint16_t	ackstatus;
	uint16_t	rsvd1;
	uint32_t	max_active_connections;
	uint32_t	num_active_connections;
} __attribute__((__packed__)) fpp_stat_conn_response_t;

typedef struct {
	uint16_t ackstatus;
} __attribute__((__packed__)) fpp_stat_pppoe_status_response_t;

typedef struct {
	uint16_t 	ackstatus;
	uint16_t 	eof;
	uint16_t	sessionid;
	uint16_t	interface_no; /* WAN_PORT_ID for WAN & LAN_PORT_ID for LAN */
	uint32_t	total_packets_received;
	uint32_t	total_packets_transmitted;
} __attribute__((__packed__)) fpp_stat_pppoe_entry_response_t;

typedef struct {
	uint16_t	ackstatus;
} __attribute__((__packed__)) fpp_stat_bridge_status_response_t;

typedef struct {
	uint16_t	ackstatus;
	uint16_t	eof;
	uint16_t	input_interface;
	uint16_t	input_svlan;
	uint16_t	input_cvlan;
	uint8_t	dst_mac[6];
	uint8_t	src_mac[6];
	uint16_t	ether_type;
	uint16_t	output_interface;
	uint16_t	output_svlan;
	uint16_t	output_cvlan;
	uint16_t	session_id;
	uint32_t	total_packets_transmitted;
	char        input_name[IFNAMSIZ];
	char        output_name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_stat_bridge_entry_response_t;

typedef struct {
	uint16_t	ackstatus;
	uint16_t	eof;
	uint16_t	family;
	uint16_t	proto;
	uint32_t	spi;
	uint32_t	dst_ip[4];
	uint32_t	total_pkts_processed;
	uint32_t	total_bytes_processed[2];
	uint16_t	sagd;
	uint16_t	pad;
} __attribute__((__packed__)) fpp_stat_ipsec_entry_response_t;


typedef struct {
	uint16_t ackstatus;
	uint16_t eof;
	uint16_t vlanID;
	uint16_t rsvd;
	uint32_t total_packets_received;
	uint32_t total_packets_transmitted;
	uint32_t total_bytes_received[2];
	uint32_t total_bytes_transmitted[2];
	unsigned char vlanifname[IFNAMSIZ];
	unsigned char phyifname[IFNAMSIZ];
} __attribute__((__packed__)) fpp_stat_vlan_entry_response_t;


typedef struct {
	uint16_t ackstatus;
	uint16_t eof;
	uint32_t rsvd;
	uint32_t total_packets_received;
	uint32_t total_packets_transmitted;
	uint32_t total_bytes_received[2];
	uint32_t total_bytes_transmitted[2];
	unsigned char if_name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_stat_tunnel_entry_response_t;

typedef struct {
	uint16_t	ackstatus;
	uint8_t	ip_family;
	uint8_t	Protocol;
	uint16_t	Sport;		/*Source Port*/
	uint16_t	Dport;		/*Destination Port*/
	union {
		struct {
			uint32_t	Saddr;		/*Source IPv4 address*/
			uint32_t	Daddr;		/*Destination IPv4 address*/
		} v4;
		struct {
			uint32_t	Saddr_v6[4];		/*Source IPv6 address*/
			uint32_t	Daddr_v6[4];		/*Destination IPv6 address*/
		} v6;
	} ipv;
	uint64_t	TotalPackets;
	uint64_t	TotalBytes;
} __attribute__((__packed__)) fpp_stat_flow_entry_response_t;


/*------------------------------------ Altconf --------------------------------*/
#define FPP_CMD_ALTCONF_SET		0x1001
#define FPP_CMD_ALTCONF_RESET		0x1002

/* option IDs */
#define FPP_ALTCONF_OPTION_MCTTL	0x0001  /*Multicast TTL option */
#define FPP_ALTCONF_OPTION_IPSECRL	0x0002  /*IPSEC Rate Limiting option */
#define FPP_ALTCONF_OPTION_ALL		0xFFFF
#define FPP_ALTCONF_OPTION_MAX		FPP_ALTCONF_OPTION_IPSECRL + 1 /*include the "all" option*/

#define FPP_ALTCONF_MODE_DEFAULT	0 /* same default value used for all options */
#define FPP_ALTCONF_OPTION_MAX_PARAMS	3 /* IPSEC Rate Limiting has 3 parameters. To be updated if a new option is add with more 32bits params */

/* ALL options */
#define FPP_ALTCONF_ALL_NUM_PARAMS	1
#define FPP_ALTCONF_ALL_MODE_DEFAULT	FPP_ALTCONF_MODE_DEFAULT

/* Multicast TTL Configuration definitions */
#define FPP_ALTCONF_MCTTL_MODE_DEFAULT	FPP_ALTCONF_MODE_DEFAULT
#define FPP_ALTCONF_MCTTL_MODE_IGNORE	1
#define FPP_ALTCONF_MCTTL_MODE_MAX	FPP_ALTCONF_MCTTL_MODE_IGNORE
#define FPP_ALTCONF_MCTTL_NUM_PARAMS	1  /* maximu number of u32 allowed for this option */

/* IPSEC Rate Limiting Configuration definitions */
#define FPP_ALTCONF_IPSECRL_OFF		0
#define FPP_ALTCONF_IPSECRL_ON		1
#define FPP_ALTCONF_IPSECRL_NUM_PARAMS	3  /* maximu number of u32 allowed for this option */

typedef struct {
	uint16_t	option_id;
	uint16_t	num_params;
	uint32_t	params[FPP_ALTCONF_OPTION_MAX_PARAMS];
} __attribute__((__packed__)) fpp_alt_set_cmd_t;

/*--------------------------------- NATPT ------------------------------------*/
#define FPP_CMD_NATPT_OPEN		0x1101
#define FPP_CMD_NATPT_CLOSE		0x1102
#define FPP_CMD_NATPT_QUERY		0x1103

#define FPP_NATPT_CONTROL_6to4		0x01
#define FPP_NATPT_CONTROL_4to6		0x02
#define FPP_NATPT_CONTROL_TCPFIN	0x0100

typedef struct {
	uint16_t		socket_a;
	uint16_t		socket_b;
	uint16_t		control;
	uint16_t		rsvd1;
} __attribute__((__packed__)) fpp_natpt_open_cmd_t;

typedef struct {
	uint16_t		socket_a;
	uint16_t		socket_b;
} __attribute__((__packed__)) fpp_natpt_close_cmd;

typedef struct {
	uint16_t		reserved1;
	uint16_t		socket_a;
	uint16_t		socket_b;
	uint16_t		reserved2;
} __attribute__((__packed__)) fpp_natpt_query_cmd_t;

typedef struct {
	uint16_t		retcode;
	uint16_t		socket_a;
	uint16_t		socket_b;
	uint16_t		control;
	uint64_t		stat_v6_received;
	uint64_t		stat_v6_transmitted;
	uint64_t		stat_v6_dropped;
	uint64_t		stat_v6_sent_to_ACP;
	uint64_t		stat_v4_received;
	uint64_t		stat_v4_transmitted;
	uint64_t		stat_v4_dropped;
	uint64_t		stat_v4_sent_to_ACP;
} __attribute__((__packed__)) fpp_natpt_query_response_t;

/*---------------------------------- Fast Forwarding -------------------------*/
#define FPP_CMD_IPV4_FF_CONTROL		0x0321

/* Structure representing the command sent to enable/disable fast-forward */
typedef struct {
        uint16_t enable;
        uint16_t reserved;
} __attribute__((__packed__)) fpp_ff_ctrl_cmd_t;

/*---------------------------------- VLAN ------------------------------------*/
#define FPP_ERR_VLAN_ENTRY_ALREADY_REGISTERED	600
#define FPP_ERR_VLAN_ENTRY_NOT_FOUND		601

#define FPP_CMD_VLAN_ENTRY		0x0901
#define FPP_CMD_VLAN_RESET		0x0902

/* VLAN command as understood by FPP */
typedef struct {
        uint16_t       action;
        uint16_t       vlan_id; /* Carries skip count for ACTION_QUERY */
        char 		vlan_ifname[IFNAMSIZ];
        char 		vlan_phy_ifname[IFNAMSIZ];
} __attribute__((__packed__)) fpp_vlan_cmd_t;

/*---------------------------------- MacVlan ------------------------------------*/
#define FPP_CMD_MACVLAN_ENTRY		0x1401
#define FPP_CMD_MACVLAN_RESET           0x1402

#define FPP_ERR_MACVLAN_ENTRY_ALREADY_REGISTERED       60
#define FPP_ERR_MACVLAN_ENTRY_NOT_FOUND                61


/* MacVlan command as understood by FPP */
typedef struct {
        uint16_t       action;
        unsigned char   macaddr[6]; 
        char 		macvlan_ifname[IFNAMSIZ];
        char 		macvlan_phy_ifname[IFNAMSIZ];
} __attribute__((__packed__)) fpp_macvlan_cmd_t;

/*--------------------------------- Ipsec ------------------------------------*/
/* 0x0axx : IPSec module */
#define FPP_CMD_IPSEC_SA_ADD			0x0a01
#define FPP_CMD_IPSEC_SA_DELETE			0x0a02
#define FPP_CMD_IPSEC_SA_FLUSH			0x0a03
#define FPP_CMD_IPSEC_SA_SET_KEYS		0x0a04
#define FPP_CMD_IPSEC_SA_SET_TUNNEL		0x0a05
#define FPP_CMD_IPSEC_SA_SET_NATT		0x0a06
#define FPP_CMD_IPSEC_SA_SET_STATE		0x0a07
#define FPP_CMD_IPSEC_SA_SET_LIFETIME		0x0a08
#define FPP_CMD_IPSEC_SA_NOTIFY			0x0a09 
#define FPP_CMD_IPSEC_SA_ACTION_QUERY		0x0a0a 
#define FPP_CMD_IPSEC_SA_ACTION_QUERY_CONT	0x0a0b 
#define FPP_CMD_IPSEC_FLOW_ADD			0x0a11
#define FPP_CMD_IPSEC_FLOW_REMOVE		0x0a12
#define FPP_CMD_IPSEC_FLOW_NOTIFY		0x0a13
#define FPP_CMD_IPSEC_FRAG_CFG			0x0a14
#define FPP_CMD_IPSEC_SA_TNL_ROUTE		0x0a15
#define FPP_CMD_IPSEC_SA_ACTION_SHOW		0x0a16

#define FPP_CMD_NETKEY_SA_ADD			FPP_CMD_IPSEC_SA_ADD
#define FPP_CMD_NETKEY_SA_DELETE		FPP_CMD_IPSEC_SA_DELETE
#define FPP_CMD_NETKEY_SA_FLUSH			FPP_CMD_IPSEC_SA_FLUSH
#define FPP_CMD_NETKEY_SA_SET_KEYS		FPP_CMD_IPSEC_SA_SET_KEYS
#define FPP_CMD_NETKEY_SA_SET_TUNNEL		FPP_CMD_IPSEC_SA_SET_TUNNEL
#define FPP_CMD_NETKEY_SA_SET_NATT		FPP_CMD_IPSEC_SA_SET_NATT
#define FPP_CMD_NETKEY_SA_SET_STATE		FPP_CMD_IPSEC_SA_SET_STATE
#define FPP_CMD_NETKEY_SA_SET_LIFETIME		FPP_CMD_IPSEC_SA_SET_LIFETIME
#define FPP_CMD_NETKEY_FLOW_ADD			FPP_CMD_IPSEC_FLOW_ADD
#define FPP_CMD_NETKEY_FLOW_REMOVE		FPP_CMD_IPSEC_FLOW_REMOVE
#define FPP_CMD_NETKEY_FLOW_NOTIFY		FPPCMD_IPSEC_FLOW_NOTIFY

typedef struct {
        uint16_t	action;
        uint16_t	handle; /* handle */
        /* SPI information */
        uint16_t	mtu;    /* mtu configured */
        uint16_t	rsvd1;
        uint32_t	spi;      /* spi */
        uint8_t	sa_type; /* SA TYPE Prtocol ESP/AH */
        uint8_t	family; /* Protocol Family */
        uint8_t	mode; /* Tunnel/Transport mode */
        uint8_t	replay_window; /* Replay Window */
        uint32_t	dst_ip[4];
        uint32_t	src_ip[4];

          /* Key information */
        uint8_t	key_alg;
        uint8_t	state; /* SA VALID /EXPIRED / DEAD/ DYING */
	uint16_t	flags; /* ESP AH enabled /disabled */

        uint8_t	cipher_key[32];
        uint8_t	auth_key[20];
        uint8_t	ext_auth_key[12];


        /* Tunnel Information */
        uint8_t	tunnel_proto_family;
        uint8_t	rsvd[3];
        union {
                struct {
                        uint32_t	daddr;
                        uint32_t	saddr;
                        uint8_t	tos;
                        uint8_t	protocol;
                        uint16_t	total_length;
                } ipv4;

                struct {
                        uint32_t	traffic_class_hi:4;
                        uint32_t	version:4;
                        uint32_t	flow_label_high:4;
                        uint32_t	traffic_class:4;
                        uint32_t	flow_label_lo:16;
                        uint32_t	daddr[4];
                        uint32_t	saddr[4];
                } ipv6;

        } tnl;

        uint64_t	soft_byte_limit;
        uint64_t	hard_byte_limit;
        uint64_t	soft_packet_limit;
        uint64_t	hard_packet_limit;

} __attribute__((__packed__)) fpp_sa_query_cmd_t;

/*--------------------------------------- PPPoE --------------------------------*/
#define FPP_CMD_PPPOE_ENTRY             0x0601
#define FPP_CMD_PPPOE_GET_IDLE		0x0603
#define FPP_CMD_PPPOE_RELAY_ENTRY	0x0610
#define FPP_CMD_PPPOE_RELAY_ADD		0x0611
#define FPP_CMD_PPPOE_RELAY_REMOVE	0x0612

#define FPP_ERR_PPPOE_ENTRY_ALREADY_REGISTERED	800
#define FPP_ERR_PPPOE_ENTRY_NOT_FOUND		801

/* Structure representing the command sent to add or remove a pppoe session */
typedef struct {
	uint16_t action;		 	/*Action to perform*/
	uint16_t sessionid;
	uint8_t  macaddr[6];
	char      phy_intf[IFNAMSIZ];
	char      log_intf[IFNAMSIZ];
	uint16_t mode;
} __attribute__((__packed__)) fpp_pppoe_cmd_t;

typedef struct {
	char		ppp_if[IFNAMSIZ];
	uint32_t	xmit_idle;
	uint32_t	recv_idle;
} __attribute__((__packed__)) fpp_pppoe_idle_t;

typedef struct {
        uint8_t        peermac1[6];
        uint8_t        peermac2[6];
        char            ipifname[IFNAMSIZ];
        char            opifname[IFNAMSIZ];
        uint16_t       sesID;
        uint16_t       relaysesID;
} __attribute__((__packed__)) fpp_relay_info_t;

/* Structure representing the command sent to add or remove a pppoe session */
typedef struct {
        uint16_t       action;      /*Action to perform */
        uint8_t        peermac1[6];
        uint8_t        peermac2[6];
	uint8_t        ipif_mac[6];
	uint8_t        opif_mac[6];
        char            ipifname[IFNAMSIZ];
        char            opifname[IFNAMSIZ];
        uint16_t       sesID;
        uint16_t       relaysesID;
        uint16_t       pad;
} __attribute__((__packed__)) fpp_pppoe_relay_cmd_t;

#ifdef WIFI_ENABLE
/*----------------------------------------WiFi ------------------------------*/
#define FPP_ERR_WIFI_DUPLICATE_OPERATION	2001
/* 0x2000: WiFi module */
#define FPP_CMD_WIFI_VAP_ENTRY			0x2001
#define FPP_CMD_VWD_ENABLE			0x2002
#define FPP_CMD_VWD_DISABLE			0x2003
#define FPP_CMD_WIFI_VAP_QUERY			0x2004
#define FPP_CMD_WIFI_VAP_RESET			0x2005

typedef struct fpp_wifi_vap_query_response
{
	uint16_t	vap_id;
	char		ifname[IFNAMSIZ];
	uint16_t	phy_port_id;
} __attribute__((__packed__)) fpp_wifi_vap_query_response_t;

typedef struct fpp_wifi_cmd
{
#define		FPP_VWD_VAP_ADD		0
#define		FPP_VWD_VAP_REMOVE	1
#define		FPP_VWD_VAP_UPDATE	2
#define		FPP_VWD_VAP_RESET	3
#define		FPP_VWD_VAP_CONFIGURE	4
	uint16_t	action;
	uint16_t	vap_id;
	char		ifname[IFNAMSIZ];
	char		mac_addr[6];
	uint16_t	wifi_guest_flag;
} __attribute__((__packed__)) fpp_wifi_cmd_t;

#endif

/*-------------------------------------- Tunnel -----------------------------*/
#define FPP_CMD_TUNNEL_ADD		0x0B01
#define FPP_CMD_TUNNEL_DEL		0x0B02
#define FPP_CMD_TUNNEL_UPDATE		0x0B03
#define FPP_CMD_TUNNEL_SEC		0x0B04
#define FPP_CMD_TUNNEL_QUERY		0x0B05
#define FPP_CMD_TUNNEL_QUERY_CONT	0x0B06
#define FPP_CMD_TUNNEL_4rd_ID_CONV_dport    0x0B07
#define FPP_CMD_TUNNEL_4rd_ID_CONV_psid     0x0B08

/* CMM / FPP API Command */
typedef struct {
	char            name[IFNAMSIZ];
	uint32_t       local[4];
	uint32_t       remote[4];
	char            output_device[IFNAMSIZ];
	uint8_t        mode;
	uint8_t        secure;
	uint8_t        encap_limit;
	uint8_t        hop_limit;
	uint32_t       flow_info; /* Traffic class and FlowLabel */
	uint16_t       frag_off;
	uint16_t       enabled;
	uint32_t	route_id;
	uint16_t	mtu;
	uint16_t	pad;
} __attribute__((__packed__)) fpp_tunnel_create_cmd_t;

typedef struct {
        char            name[IFNAMSIZ];
} __attribute__((__packed__)) fpp_tunnel_del_cmd_t;

typedef struct {
        char            name[IFNAMSIZ];
        uint16_t       sa_nr;
        uint16_t       sa_reply_nr;
        uint16_t       sa_handle[4];
        uint16_t       sa_reply_handle[4];
} __attribute__((__packed__)) fpp_tunnel_sec_cmd_t;

/* CMM / FPP API Command */
typedef struct {
	unsigned short	result;
	unsigned short 	unused;
        char            name[IFNAMSIZ];
        uint32_t       local[4];
        uint32_t       remote[4];
        uint8_t        mode;
        uint8_t        secure;
        uint8_t        encap_limit;
        uint8_t        hop_limit;
        uint32_t       flow_info; /* Traffic class and FlowLabel */
        uint16_t       frag_off;
        uint16_t       enabled;
        uint32_t       route_id;
        uint16_t       mtu;
        uint16_t       pad;
} __attribute__((__packed__)) fpp_tunnel_query_cmd_t;


#ifdef SAM_LEGACY

typedef struct {
          int                            port_set_id;                    /**< Port Set ID               */
          int                            port_set_id_length;             /**< Port Set ID length        */
          int                            psid_offset;                    /**< PSID offset               */
}sam_port_info_t;
typedef sam_port_info_t rt_mw_ipstack_sam_port_t;

typedef struct {
       uint8_t        name[IFNAMSIZ];
       sam_port_info_t sam_port_info;
        uint32_t       IdConvStatus:1,
                       unused:31;
} __attribute__((__packed__)) fpp_tunnel_id_conv_cmd_t;

#else

typedef struct {
	uint16_t IdConvStatus;
        uint16_t       Pad;
} __attribute__((__packed__)) fpp_tunnel_id_conv_cmd_t;
#endif

/*--------------------------------- Timeout ---------------------------------*/

/**
 * @addtogroup  dxgrLibFCI
 * @{
 */

/**
 * @def         FPP_CMD_IPV4_SET_TIMEOUT
 * @brief       FCI command for configuration of conntrack timeouts.
 * @details     Related topics: @ref l3_router
 * @details     Related data types: @ref fpp_timeout_cmd_t
 * @details     @ref FPP_CMD_IPV4_SET_TIMEOUT sets default timeout for @b both @ref FPP_CMD_IPV4_CONNTRACK 
 *              and @ref FPP_CMD_IPV6_CONNTRACK.
 * @details     This command allows for configuration of conntrack default timeout periods. 
 *              Three protocol groups are distinguished: `TCP (6)`, `UDP (17)` and `others` 
 *              (all other protocols; usually represented by 0).
 *              Timeout can be set independently for each of these groups.
 * @details     Factory-default timeout values are:
 *                - 5 days for `TCP`
 *                - 300 seconds for `UDP`
 *                - 240 seconds for `others`
 * @details     If these timeouts are updated (changed), then all newly created conntracks 
 *              are created with updated timeout values. Conntracks which were created before 
 *              the change have their timeout updated with the first received packet after the change.
 * @details     Supported `.action` values: ---
 * <br>
 * @code{.c}
 *  .............................................  
 *  fpp_timeout_cmd_t cmd_to_fci = 
 *  {
 *    .protocol = ...,       // IP Protocol Number (protocol ID). [NBO]
 *                           // The only accepted values are 6 (TCP), 17 (UDP) or 0 (others).
 *    
 *    .timeout_value1 = ...  // Timeout value in seconds. [NBO]
 *  };
 *    
 *  int rtn = 0;
 *  rtn = fci_write(client, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
 *                                                   (unsigned short*)(&cmd_to_fci));
 *  .............................................  
 * @endcode
 *
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
#define FPP_CMD_IPV4_SET_TIMEOUT	0x0319
#define FPP_CMD_IPV4_GET_TIMEOUT	0x0320
#define FPP_CMD_IPV4_FRAGTIMEOUT        0x0333
#define FPP_CMD_IPV4_SAMFRAGTIMEOUT	0x0334
#define FPP_CMD_IPV6_GET_TIMEOUT	0x0420
#define FPP_CMD_IPV6_FRAGTIMEOUT	0x0433

/**
 * @brief       Data structure for conntrack timeout setting.
 * @details     Related FCI commands: @ref FPP_CMD_IPV4_SET_TIMEOUT
 * @details     @ref FPP_CMD_IPV4_SET_TIMEOUT sets timeout for @b both @ref FPP_CMD_IPV4_CONNTRACK 
 *              and @ref FPP_CMD_IPV6_CONNTRACK.
 *
 * @note        Some values are in a network byte order [NBO].
 *
 * @snippet     fpp.h  fpp_timeout_cmd_t
 */
/* [fpp_timeout_cmd_t] */
typedef struct CAL_PACKED_ALIGNED(4) {
    uint16_t    protocol;         /*< IP Protocol Number (protocol ID). [NBO]
                                      The only accepted values are 6 (TCP), 17 (UDP) or
                                      0 (others). */
    
    uint16_t    sam_4o6_timeout;  /*< RESERVED (do not use) */
    uint32_t    timeout_value1;   /*< Timeout value in seconds. [NBO] */
    uint32_t    timeout_value2;   /*< RESERVED (do not use) */
} fpp_timeout_cmd_t;
/* [fpp_timeout_cmd_t] */

typedef struct {
	uint16_t	timeout;
	uint16_t	mode;
} __attribute__((__packed__)) fpp_frag_timeout_cmd_t;

/** @}*/

/*---------------------------------------PKTCAP---------------------------------*/
#define FPP_CMD_PKTCAP_IFSTATUS				0x0d02
#define FPP_CMD_PKTCAP_FLF                              0x0d03
#define FPP_CMD_PKTCAP_SLICE				0x0d04
#define FPP_CMD_PKTCAP_QUERY				0x0d05

#define FPP_PKTCAP_STATUS				0x1
#define FPP_PKTCAP_SLICE				0x2
#define MAX_FLF_INSTRUCTIONS                            30

typedef struct {
	uint16_t	action;
	uint8_t 	ifindex;
	uint8_t	status;
}__attribute__((__packed__)) fpp_pktcap_status_cmd_t;

typedef struct {
	uint16_t	action;
	uint8_t 	ifindex;
	uint8_t	rsvd;
	uint16_t	slice;
}__attribute__((__packed__)) fpp_pktcap_slice_cmd_t;

typedef struct {
	uint16_t	slice;
	uint16_t	status;
}__attribute__((__packed__)) fpp_pktcap_query_cmd_t;

/*----------------------------------------PKTCAP-------------------------------*/

/* Port Update command - begin */

#define FPP_CMD_PORT_UPDATE 0x0505
typedef struct {
	uint16_t port_id;
	char ifname[IFNAMSIZ];
}__attribute__((__packed__)) fpp_port_update_cmd_t;


/*---------------------------------------ICC---------------------------------*/

#define FPP_CMD_ICC_RESET				0x1500
#define FPP_CMD_ICC_THRESHOLD				0x1501
#define FPP_CMD_ICC_ADD_DELETE				0x1502
#define FPP_CMD_ICC_QUERY				0x1503

#define FPP_ERR_ICC_TOO_MANY_ENTRIES		1500
#define FPP_ERR_ICC_ENTRY_ALREADY_EXISTS	1501
#define FPP_ERR_ICC_ENTRY_NOT_FOUND		1502
#define FPP_ERR_ICC_THRESHOLD_OUT_OF_RANGE	1503
#define FPP_ERR_ICC_INVALID_MASKLEN		1504

typedef struct {
	uint16_t	reserved1;
	uint16_t	reserved2;
} __attribute__((__packed__)) fpp_icc_reset_cmd_t;

typedef struct {
	uint16_t	bmu1_threshold;
	uint16_t	bmu2_threshold;
} __attribute__((__packed__)) fpp_icc_threshold_cmd_t;

typedef struct {
	uint16_t	action;
	uint8_t	interface;
	uint8_t	table_type;
	union {
		struct {
			uint16_t type;
		} ethertype;
		struct {
			uint8_t ipproto[256 / 8];
		} protocol;
		struct {
			uint8_t dscp_value[64 / 8];
		} dscp;
		struct {
			uint32_t v4_addr;
			uint8_t v4_masklen;
		} ipaddr;
		struct {
			uint32_t v6_addr[4];
			uint8_t v6_masklen;
		} ipv6addr;
		struct {
			uint16_t sport_from;
			uint16_t sport_to;
			uint16_t dport_from;
			uint16_t dport_to;
		} port;
		struct {
			uint16_t vlan_from;
			uint16_t vlan_to;
			uint16_t prio_from;
			uint16_t prio_to;
		} vlan;
	} icc;
} __attribute__((__packed__)) fpp_icc_add_delete_cmd_t;

typedef struct {
	uint16_t	action;
	uint8_t	interface;
	uint8_t	reserved;
} __attribute__((__packed__)) fpp_icc_query_cmd_t;

typedef struct {
	uint16_t	rtncode;
	uint16_t	query_result;
	uint8_t	interface;
	uint8_t	table_type;
	union {
		struct {
			uint16_t type;
		} ethertype;
		struct {
			uint8_t ipproto[256 / 8];
		} protocol;
		struct {
			uint8_t dscp_value[64 / 8];
		} dscp;
		struct {
			uint32_t v4_addr;
			uint8_t v4_masklen;
		} ipaddr;
		struct {
			uint32_t v6_addr[4];
			uint8_t v6_masklen;
		} ipv6addr;
		struct {
			uint16_t sport_from;
			uint16_t sport_to;
			uint16_t dport_from;
			uint16_t dport_to;
		} port;
		struct {
			uint16_t vlan_from;
			uint16_t vlan_to;
			uint16_t prio_from;
			uint16_t prio_to;
		} vlan;
	} icc;
} __attribute__((__packed__)) fpp_icc_query_reply_t;

/*----------------------------------------L2TP-------------------------------*/
#define FPP_CMD_L2TP_ITF_ADD		0x1600
#define FPP_CMD_L2TP_ITF_DEL		0x1601

typedef struct {
	char ifname[IFNAMSIZ];
	uint16_t	sock_id;
	uint16_t	local_tun_id;
	uint16_t	peer_tun_id;
	uint16_t	local_ses_id;
	uint16_t	peer_ses_id;
	uint16_t	options;
}__attribute__((__packed__)) fpp_l2tp_itf_add_cmd_t;

typedef struct {
	char ifname[IFNAMSIZ];
}__attribute__((__packed__)) fpp_l2tp_itf_del_cmd_t;
#endif /* FPP_H_ */
