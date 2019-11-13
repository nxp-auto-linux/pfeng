/* =========================================================================
 *  Copyright (C) 2007 Mindspeed Technologies, Inc.
 *  Copyright 2015-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2018 NXP
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
 * @defgroup    dxgrLibFCI LibFCI
 * @brief       This is the Fast Control Interface available to host applications to
 *              communicate with the networking engine.
 * @details
 *              The FCI is intended to provide a generic configuration and monitoring interface
 *              to networking acceleration HW. Provided API shall remain the same within all
 *              HW/OS-specific implementations to keep dependent applications portable across
 *              various systems.
 *              
 *              The LibFCI is not directly touching the HW. Instead, it only passes commands to
 *              dedicated software component (OS/HW-specific endpoint) and receives return values.
 *              The endpoint is then responsible for HW configuration. This approach supports
 *              kernel-user space deployment where user space contains only API and the logic is
 *              implemented in kernel.
 *              
 *              Implementation uses appropriate transport mechanism to pass data between
 *              LibFCI user and the endpoint. For reference, in Linux netlink socket will be used,
 *              in QNX it will be a message.
 *
 *              Usage scenario example - FCI command execution:
 *              -# User calls @ref fci_open() to get the @ref FCI_CLIENT instance, use FCI_GROUP_NONE
 *                 as multicast group mask.
 *              -# User calls @ref fci_cmd() to send a command with arguments to the endpoint
 *              -# Endpoint receives the command and performs requested actions
 *              -# Endpoint generates response and sends it back to the client
 *              -# Client receives the response and informs the caller
 *              -# User calls @ref fci_close() to finalize the @ref FCI_CLIENT instance
 *              
 *              Usage scenario example - asynchronous message processing:
 *              -# User calls @ref fci_open() to get the @ref FCI_CLIENT instance. It is 
 *                 important to set @ref FCI_GROUP_CATCH bit in multicast group mask.
 *              -# User calls @ref fci_register_cb() to register custom function for handling 
 *                 asynchronous messages from firmware
 *              -# User calls @ref fci_catch() function
 *              -# For each received message, the @ref fci_catch() calls previously registered
 *                 callback
 *              -# When the callback returns @ref FCI_CB_CONTINUE, the @ref fci_catch() function
 *                 waits for another message
 *              -# When the callback returns @ref FCI_CB_STOP or when error ocured,
 *                 function @ref fci_catch() returns.
 *              -# User calls fci_close() to finalize the @ref FCI_CLIENT instance
 *
 *              Acronyms and Definitions
 *              ========================
 *              - route: Data structure which specifies where outgoing traffic will be sent.
 *                It contains following information: egress interface, destination MAC address
 *                and destination IP address.
 *              - conntrack: "tracked connection", a data structure containing information
 *                about a connection. In context of this document it always refers
 *                to an IP connection (TCP, UDP, other).
 */
/**
 * @addtogroup  dxgrLibFCI
 * @{
 * 
 * @file        libfci.h
 * @brief       Generic LibFCI header file
 * @details     This file contains generic API and API description
 *
 */

#ifndef _LIBFCI_H
#define _LIBFCI_H

/* TODO put to config file: */
/**
 * @def FCI_CFG_FORCE_LEGACY_API
 * @brief       Changes the LibFCI API so it is more compatible with legacy implementation
 * @details     LibFCI API was modified to avoid some inconvenient properties. Here are the points
 *              the legacy API differs in:
 *              -# With legacy API, argument @c rsp_data of function @c fci_query shall be provided shifted by two
 *                 bytes this way:
 *                 @code{.c}
 *                   reply_struct_t rsp_data;
 *                   retval = fci_query(this_client, fcode, cmd_len, &pcmd, &rsplen, (unsigned short *)(&rsp_data) + 1u);
 *                 @endcode
 *                 Where @c reply_struct_t is the structure type depending on command being called.
 *              -# In legacy API, macros @ref FPP_CMD_IPV4_CONNTRACK_CHANGE and @ref FPP_CMD_IPV6_CONNTRACK_CHANGE are
 *                 defined in application files. In current API they are defined here in @ref libfci.h.
 *
 * @hideinitializer
 */
#define FCI_CFG_FORCE_LEGACY_API FALSE

#if FALSE == FCI_CFG_FORCE_LEGACY_API
    /**
     * @def         FPP_CMD_IPV4_CONNTRACK_CHANGE
     * @brief       Callback event value for IPv4 conntracks
     * @details     One of the values the callback registered by @ref fci_register_cb can get in @c fcode
     *              argument.
     *
     *              This value indicates IPv4 conntrack event. The payload argument shall be cast to
     *              @ref fpp_ct_ex_cmd type. Then all addresses, all ports and protocol shall be used to
     *              identify connection while the @c action item indicates type of event:
     *              - @ref FPP_ACTION_KEEP_ALIVE: conntrack entry is still active
     *              - @ref FPP_ACTION_REMOVED: conntrack entry was removed
     *              - @ref FPP_ACTION_TCP_FIN: TCP FIN or TCP RST packet was received, conntrack was removed
     * @hideinitializer
     */
    #define FPP_CMD_IPV4_CONNTRACK_CHANGE   0x0315u
    /**
     * @def         FPP_CMD_IPV6_CONNTRACK_CHANGE
     * @brief       Callback event value for IPv6 conntracks
     * @details     One of the values the callback registered by @ref fci_register_cb can get in @c fcode
     *              argument.
     *
     *              This value indicates IPv6 conntrack event. The payload argument shall be cast to
     *              @ref fpp_ct6_ex_cmd type. Otherwise the event is same as
     *              @ref FPP_CMD_IPV4_CONNTRACK_CHANGE.
     * @hideinitializer
     */
    #define FPP_CMD_IPV6_CONNTRACK_CHANGE   0x0415u
#endif /* FCI_CFG_FORCE_LEGACY_API */

/**
 * @def CTCMD_FLAGS_ORIG_DISABLED
 * @brief Disable connection originator
 */
#define CTCMD_FLAGS_ORIG_DISABLED           (1U << 0)

/**
 * @def         CTCMD_FLAGS_REP_DISABLED
 * @brief       Disable connection replier
 * @details     Used to create uni-directional connections (see @ref FPP_CMD_IPV4_CONNTRACK,
 *              @ref FPP_CMD_IPV4_CONNTRACK)
 */
#define CTCMD_FLAGS_REP_DISABLED            (1U << 1)

/**
 * @struct      FCI_CLIENT
 * @brief       The FCI client representation type
 * @details     This is the FCI instance representation. It is used by the rest of the API to
 *              communicate with associated endpoint. The endpoint can be a standalone
 *              application/driver taking care of HW configuration tasks and shall be able to
 *              interpret commands sent via the LibFCI API.
 * 
 * @internal
 * @note        The __fci_client_tag structure has to be provided by the OS/HW specific
 *              implementation.
 * @endinternal
 */
typedef struct __fci_client_tag FCI_CLIENT;

/**
 * @typedef     fci_mcast_groups_t
 * @brief       List of supported multicast groups
 * @details     An FCI client instance can be member of a multicast group. It means it can send and
 *              receive multicast messages to/from another group members (another FCI instances or FCI
 *              endpoints). This can be in most cases used by FCI endpoint to notify all associated
 *              FCI instances about some event has occurred.
 * @note        Each group is intended to be represented by a single bit flag (max 32-bit, so it is
 *              possible to have max 32 multicast groups). Then, groups can be combined using
 *              bitwise OR operation.
 */
typedef enum fci_mcast_groups
{
    FCI_GROUP_NONE  = 0x00000000, /**< Default MCAST group value, no group, for sending FCI commands */
    FCI_GROUP_CATCH = 0x00000001  /**< MCAST group for catching events */
} fci_mcast_groups_t;

/**
 * @typedef     fci_client_type_t
 * @brief       List of supported FCI client types
 * @details     FCI client can specify using this types to which FCI endpoint shall be connected.
 */
typedef enum fci_client_type
{
    FCI_CLIENT_DEFAULT = 0, /**< Default type (equivalent of legacy FCILIB_FF_TYPE macro) */
	FCILIB_FF_TYPE = 0 /* Due to compatibility purposes */
} fci_client_type_t;

/**
 * @typedef     fci_cb_retval_t
 * @brief       The FCI callback return values
 * @details     This return values shall be used in FCI callback (see @ref fci_register_cb).
 *              It tells @ref fci_catch function whether it should return or continue.
 */
typedef enum fci_cb_retval
{
    FCI_CB_STOP = 0, /**< Stop waiting for events and exit @ref fci_catch function */
    FCI_CB_CONTINUE  /**< Continue waiting for next events */
} fci_cb_retval_t;

/**
 * @brief       Creates new FCI client and opens a connection to FCI endpoint
 * @details     Binds the FCI client with FCI endpoint. This enables sending/receiving data
 *              to/from the endpoint. Refer to the remaining API for possible communication
 *              options.
 * @param[in]   type Client type. Default value is FCI_CLIENT_DEFAULT. See @ref fci_client_type_t.
 * @param[in]   group A 32-bit multicast group mask. Each bit represents single multicast address.
 *                    FCI instance will listen to specified multicast addresses as well it will
 *                    send data to all specified multicast groups. See @ref fci_mcast_groups_t.
 * @return      The FCI client instance or NULL if failed
 */
FCI_CLIENT * fci_open(fci_client_type_t type, fci_mcast_groups_t group);

/**
 * @brief       Disconnects from FCI endpoint and destroys FCI client instance
 * @details     Terminate the FCI client and release all allocated resources.
 * @param[in]   client The FCI client instance
 * @return      0 if success, error code otherwise
 */
int fci_close(FCI_CLIENT *client);

/**
 * @brief       Catch and process all FCI messages delivered to the FCI client
 * @details     Function is intended to be called in its own thread. It waits for message reception.
 *              If there is an event callback associated with the FCI client, assigned by function
 *              @ref fci_register_cb, then, when message is received, the callback is called to
 *              process the data. As long as there is no error and the callback returns
 *              @ref FCI_CB_CONTINUE, fci_catch continues waiting for another message. Otherwise it
 *              returns.
 *
 * @note        This is a blocking function.
 *
 * @note        Multicast group FCI_GROUP_CATCH shall be used when opening the client for catching
 *              messages
 * @internal
 * @note        Better solution would be not to call this function from the application but rather
 *              implement an internal thread-based mechanism to listen for messages and to process
 *              the messages internally. The callback would then be executed in this thread's
 *              context.
 * @endinternal
 *              
 * @see         fci_register_cb()
 * @param[in]   client The FCI client instance
 * @retval      0 Success
 * @retval      Error code. See the 'errno' for more details
 */
int fci_catch(FCI_CLIENT *client);

/**
 * @brief       Run an FCI command with optional data response
 * @details     This routine can be used when one need to perform any command either with or
 *              without data response. Anyway, the routine always returns data into response
 *              buffer because the return value of the command executed on the endpoint is
 *              always written in first two bytes of the response buffer.
 *
 *              There are two possible situations:
 *              - The command responded with some data structure: the structure is written into
 *                the rep_buf and first two bytes are overwritten by the return value. The
 *                length of the data structure is written into rep_len.
 *              - The command did not respond with data structure: only the two bytes containing the
 *                return value are written into the rep_buf. Value 2 is written into the rep_len.
 *
 * @internal
 * @note        This shall be a blocking call.
 * @endinternal
 *
 * @note        The rep_buf buffer must be aligned to 4 and the length of the rep_buf must be FCI_MAX_PAYLOAD.
 *
 * @note        The differences among @ref fci_query, @ref fci_write and @ref fci_cmd functions are:
 *                - fci_cmd: return value says only whether command was executed. The return value
 *                  of the command (provided at first two bytes of rep_buf) shall be checked by user.
 *                  There are restrictions for alignment and length of rep_buf.
 *                - fci_query: return value reflects both successfull execution and called command's status.
 *                  Return value is present in rsp_data anyway, but there is no need to check it there.
 *                - fci_write: return value reflects both successfull execution and called command's status.
 *                  Reply buffer is not provided.
 *
 * @param[in]   client The FCI client instance
 * @param[in]   fcode Command to be executed. The command codes are defined in 'fpp.h' as FPP_CMD_*
 *              macros.
 * @param[in]   cmd_buf Pointer to structure holding command arguments. The structures used for command
 *              arguments are defined in 'fpp.h'.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[out]  rep_buf Pointer to memory where the data response shall be written
 * @param[in,out]   rep_len Pointer to variable which defines length of the buffer when the
 *                  function is called and length of the response when the function returns. In bytes.
 * @retval      <0 Failed to execute the command.
 * @retval      0 Command was executed. The rep_buf (first 2 bytes) need to be checked. 
 */
int fci_cmd(FCI_CLIENT *client, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len);

/**
 * @brief       Run an FCI command with data response
 * @details     This routine can be used when one need to perform a command which is resulting
 *              in a data response. It is suitable for various 'query' commands like reading of
 *              whole tables or structured entries from the endpoint.
 *
 * @internal
 * @note        This shall be a blocking call.
 * @endinternal
 *
 * @note        If either rsp_data or rsplen is NULL pointer, the response data is discarded.
 *
 * @param[in]   this_client The FCI client instance
 * @param[in]   fcode Command to be executed. The command codes are defined in 'fpp.h' as FPP_CMD_*
 *              macros.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[in]   pcmd Pointer to structure holding command arguments. The structures used for command
 *              arguments are defined in 'fpp.h'.
 * @param[out]  rsplen Pointer to memory where length of the data response will be provided
 * @param[out]  rsp_data Pointer to memory where the data response shall be written.
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Return code of the command.
 */
int fci_query(FCI_CLIENT *this_client, unsigned short fcode, unsigned short cmd_len, unsigned short *pcmd, unsigned short *rsplen, unsigned short *rsp_data);

/**
 * @brief       Run an FCI command
 * @details     Similar as the fci_query() but without data response. The endpoint receiving the
 *              command is still responsible for generating response but the response is not
 *              delivered to the caller. Only the initial 2 bytes are
 *              propagated via return value of this function.
 *
 * @internal
 * @note        This shall be a blocking call.
 * @endinternal
 *
 * @param[in]   client The FCI client instance
 * @param[in]   fcode Command to be executed. The command codes are defined in 'fpp.h' as FPP_CMD_*
 *              macros.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[in]   cmd_buf Pointer to structure holding command arguments. The structures used for command
 *              arguments are defined in 'fpp.h'.
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Return code of the command.
 */
int fci_write(FCI_CLIENT *client, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf);

/**
 * @brief       Register event callback function
 * @details     Once FCI endpoint (or another client in the same multicast group) sends message to the
 *              FCI client, this callback is called. The callback will work only if function @ref fci_catch
 *              is running.
 * @param[in]   client The FCI client instance, use the same instance as when calling @ref fci_catch
 *              function
 * @param[in]   event_cb The callback function to be executed
 * @return      0 if success, error code otherwise
 * @note        In order to continue receiving messages, the callback function shall always return
 *              @ref FCI_CB_CONTINUE. Any other value will cause the @ref fci_catch to return.
 * @note        Here is the list of defined messages. Expected @c fcode value is either
 *              @ref FPP_CMD_IPV4_CONNTRACK_CHANGE or @ref FPP_CMD_IPV6_CONNTRACK_CHANGE.
 */
int fci_register_cb(FCI_CLIENT *client, fci_cb_retval_t (*event_cb)(unsigned short fcode, unsigned short len, unsigned short *payload));

/**
 * @brief       Obsolete function, shall not be used
 */
int fci_fd(FCI_CLIENT *this_client);

/** @} */
#endif /* _LIBFCI_H */

/* Documentation of the FPP.h file: */
/**
 * @addtogroup  dxgrLibFCI
 * @{
 * 
 * @file        fpp.h
 * @brief       FCI API
 * @details     This file defines Fast Control Interface (FCI) API
 *
*/

/**
 * @def FPP_CMD_IPV4_RESET
 * @brief Specifies FCI command that clears all IPv4 routes (see @ref FPP_CMD_IP_ROUTE)
 *        and conntracks (see @ref FPP_CMD_IPV4_CONNTRACK)
 * @details This command uses no arguments.
 *
 * Command Argument Type: none (cmd_buf = NULL; cmd_len = 0;)
 * 
 * @hideinitializer
 */

/**
 * @def FPP_CMD_IPV6_RESET
 * @brief Specifies FCI command that clears all IPv6 routes (see @ref FPP_CMD_IP_ROUTE)
 *        and conntracks (see @ref FPP_CMD_IPV6_CONNTRACK)
 * @details This command uses no arguments.
 *
 * Command Argument Type: none (cmd_buf = NULL; cmd_len = 0;)
 *
 * @hideinitializer
 */

/**
 * @def FPP_CMD_IP_ROUTE
 * @brief Specifies FCI command for working with routes
 * @details Binds an IP address and a physical address to an interface.
 *          This command can be used with various values of `.action`:
 *          - @c FPP_ACTION_REGISTER: Defines a new route.
 *          - @c FPP_ACTION_DEREGISTER: Deletes previously defined route.
 *          - @c FPP_ACTION_QUERY: Gets parameters of existing routes. It creates a snapshot of all active
 *            route entries and replies with first of them.
 *          - @c FPP_ACTION_QUERY_CONT: Shall be called periodically after @c FPP_ACTION_QUERY was called. On each
 *            call it replies with parameters of next route. It returns @c FPP_ERR_RT_ENTRY_NOT_FOUND when no more
 *            entries exist.
 * 
 * Command Argument Type: struct @ref fpp_rt_cmd (@c fpp_rt_cmd_t)
 * 
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_rt_cmd_t cmd_data = 
 *   {
 *     .action = FPP_ACTION_REGISTER, // Register new route
 *     .mtu = ...,                    // MTU must be specified
 *     .dst_mac = ...,                // Destination MAC address (network endian)
 *     .output_device = ...,          // Name of egress interface (the name from operating system, e.g. eth0)
 *     .id = ...                      // Chosen number will be used as unique route identifier
 *     .flags = ...,                  // 1 for IPv4 addressing, 2 for IPv6
 *     .dst_addr = ...,               // Destination IPv4 or IPv6 address (network endian)
 *   };
 * @endcode
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_rt_cmd_t cmd_data = 
 *   {
 *     .action = FPP_ACTION_DEREGISTER, // Deregister a route
 *     .id = ...                        // Unique route identifier
 *   };
 * @endcode
 * 
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_rt_cmd_t cmd_data = 
 *   {
 *     .action = ...     // Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 *   };
 * @endcode
 * 
 * Response data type for queries: struct @ref fpp_rt_cmd (@c fpp_rt_cmd_t)
 *
 * Response data provided:
 * @code{.c}
 *     rsp_data.mtu;            // MTU
 *     rsp_data.dst_mac;        // Destination MAC address
 *     rsp_data.output_device;  // Output device name
 *     rsp_data.id;             // Route ID
 *     rsp_data.dst_addr;       // Destination IP address
 * @endcode
 * 
 * @hideinitializer
 */

/**
 * @struct      fpp_rt_cmd
 * @brief       Data structure to be used for command buffer for route commands
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IP_ROUTE command.
 *              - for reply buffer in functions @ref fci_query or @ref fci_cmd,
 *                with @ref FPP_CMD_IP_ROUTE command.
 */

/**
 * @def		FPP_CMD_IF_LOCK_SESSION
 * @brief	FCI command to perform lock on interface database.
 * @details	The reason for it is guaranteed atomic operation between fci/rpc/platform.
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Lock successful
 *     - @c FPP_ERR_IF_RESOURCE_ALREADY_LOCKED: Database was already locked by someone else
 */

/**
 * @def		FPP_CMD_IF_UNLOCK_SESSION
 * @brief	FCI command to perform unlock on interface database.
 * @details	The reason for it is guaranteed atomic operation between fci/rpc/platform.
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Lock successful
 *     - @c FPP_ERR_IF_WRONG_SESSION_ID: The lock wasn't locked or was locked in different
 *     									 session and will nor be unlocked.
 */

/**
 * @def		FPP_CMD_PHY_INTERFACE
 * @brief	FCI command for working with physical interfaces
 * @details	Interfaces are needed to be known to FCI to support insertion of routes and conntracks
 * 			insertion. Command can be used to get operation mode, mac address
 * 			and operation flags (enabled, promisc).
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * 			Following values of `.action` are supported:
 * 			- @c FPP_ACTION_QUERY: Get first interface from list of registered interfaces.
 * 			- @c FPP_ACTION_QUERY_CONT: Get next interface from list of registered interfaces.
 * 				Intended to be called after @c FPP_ACTION_QUERY until @ FPP_ERR_IF_ENTRY_NOT_FOUND
 * 				is returned meaning there is no more interfaces in the list.
 *
 * Precondition to use the query is to atomicly lock the access with FPP_CMD_IF_LOCK_SESSION.
 *
 * Command Argument Type: struct fpp_phy_if_cmd (@c fpp_phy_if_cmd)
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 *	@code{.c}
 * 		fpp_phy_if_cmd cmd_data =
 * 		{
 * 			.action = ...	// Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 * 		};
 *	@endcode
 *
 * Response data type for queries: struct @ref fpp_phy_if_cmd (@c fpp_phy_if_cmd)
 *
 * Response data provided:
 *	@code{.c}
 * 		rsp_data.name;		// Name of the interface
 * 		rsp_data.mac_addr;	// MAC address
 * 		rsp_data.id         // Interface ID
 * 		rsp_data.mode       // Operation mode
 * 		rsp_data.flags      // Operation flags
 *	@endcode
 *
 *	For operation modes see (@c fpp_phy_if_op_mode_t)
 *	For operation flags see (@c fpp_if_flags_t)
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Update successful
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: Last entry in the query session
 *     - @c FPP_ERR_IF_WRONG_SESSION_ID: Someone else is already working with the interfaces
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 * @hideinitializer
 */

/**
 * @def		FPP_CMD_LOG_INTERFACE
 * @brief	FCI command for working with logical interfaces
 * @details	Command can be used to update match rules of logical interface or for adding egress interfaces.
 * 			It can also update operational flags (enabled, promisc, match). Query with this command
 * 			will list matchrules+arguments, operational flags, egress interfaces.
 *
 * @note	Command is defined as extension of the legacy fpp.h.
 *
 * 			Following values of `.action` are supported:
 * 			- @c FPP_ACTION_UPDATE: Will update the logical interfaces with all provided information.
 * 			- @c FPP_ACTION_QUERY: Get first interface from list of registered interfaces.
 * 			- @c FPP_ACTION_QUERY_CONT: Get next interface from list of registered interfaces.
 * 				Intended to be called after @c FPP_ACTION_QUERY until @ FPP_ERR_IF_ENTRY_NOT_FOUND
 * 				is returned meaning there is no more interfaces in the list.
 *
 * Precondition to use the query is to atomicly lock the access with FPP_CMD_IF_LOCK_SESSION.
 *
 * Command Argument Type: struct fpp_log_if_cmd (@c fpp_log_if_cmd)
 *
 * FPP_ACTION_UPDATE
 * ----------------
 * * Items to be set in command argument structure:
 *	@code{.c}
 * 		fpp_log_if_cmd cmd_data =
 * 		{
 * 			.action = ...		// FPP_ACTION_UPDATE
 * 			.name = ...			// Required: Name of the interface
 * 			.egress = ...		// Optional: New egress interface mask (to add id to egress: egress = old_egress | (1U << id))
 *			.flags = ...		// Optional: New flags
 *			.match = ...		// Optional: Match rules
  *			.arguments = ...	// Optional: Arguments corresponding to required match rules
 * 		};
 *	@endcode
 *
 *	For operation flags see (@c fpp_if_m_rules_t).
 *	For match arguments see (@c fpp_if_m_args_t).
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Update successful
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: If corresponding logical interface doesn't exit
 *     - @c FPP_ERR_IF_RESOURCE_ALREADY_LOCKED: Someone else is already configuring the interfaces
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 *	@code{.c}
 * 		fpp_log_if_cmd cmd_data =
 * 		{
 * 			.action = ...	// Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 * 		};
 *	@endcode
 *
 * Response data type for queries: struct @ref fpp_log_if_cmd (@c fpp_log_if_cmd)
 *
 * Response data provided:
 *	@code{.c}
 * 		rsp_data.name;			// Name of the interface
 * 		rsp_data.id				// Interface ID
 * 		rsp_data.parrent_name;	// Name of the parent interface
 * 		rsp_data.parrent_id		// Interface ID of parent physical interface
 * 		rsp_data.egress			// Egress interfaces in a mask form (to extract id: id == egress & (1 < id))
 * 		rsp_data.flags			// Operation flags
 * 		rsp_data.match			// Mach flags (if match flag is set than corresponding argument can be extracted from .arguments)
 * 		rsp_data.arguments		// Mach arguments
 *	@endcode
 *
 *	For operation flags see (@c fpp_if_m_rules_t).
 *	For match args see (@c fpp_if_m_args_t).
 *
 * Possible command return values are:
 *     - @c FPP_ERR_OK: Update successful
 *     - @c FPP_ERR_IF_ENTRY_NOT_FOUND: Last entry in the query session
 *     - @c FPP_ERR_IF_WRONG_SESSION_ID: Someone else is already working with the interfaces
 *     - @c FPP_ERR_IF_MATCH_UPDATE_FAILED: Update of match flags has failed
 *     - @c FPP_ERR_IF_EGRESS_UPDATE_FAILED: Update of egress interfaces has failed
 *     - @c FPP_ERR_IF_EGRESS_DOESNT_EXIST: Egress interface provided in command doesn't exist
 *     - @c FPP_ERR_IF_OP_FLAGS_UPDATE_FAILED: Operation flags update has failed (PROMISC/ENABLE/MATCH)
 *     - @c FPP_ERR_INTERNAL_FAILURE: Internal FCI failure
 *
 * @hideinitializer
 */

/**
 * @struct		fpp_if_cmd
 * @brief		Data structure to be used for interface commands
 * @details		Usage:
 * 				- As command buffer in functions @ref fci_write, @ref fci_query or
 * 				  @ref fci_cmd, with @ref FPP_CMD_INTERFACE command.
 * 				- As reply buffer in functions @ref fci_query or @ref fci_cmd,
 * 				  with @ref FPP_CMD_INTERFACE command.
 */

/**
 * @def FPP_CMD_IPV4_CONNTRACK
 * @brief Specifies FCI command for working with tracked connections
 * @details This command can be used with various values of `.action`:
 *          - @c FPP_ACTION_REGISTER: Defines a connection and binds it to previously created route(s).
 *          - @c FPP_ACTION_DEREGISTER: Deletes previously defined connection.
 *          - @c FPP_ACTION_QUERY: Gets parameters of existing connection. It creates a snapshot of all active
 *            conntrack entries and replies with first of them.
 *          - @c FPP_ACTION_QUERY_CONT: Shall be called periodically after @c FPP_ACTION_QUERY was called. On each
 *            call it replies with parameters of next connection. It returns @c FPP_ERR_CT_ENTRY_NOT_FOUND when no more
 *            entries exist.
 * 
 * Command Argument Type: struct @ref fpp_ct_cmd (@c fpp_ct_cmd_t)
 * 
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct_cmd_t cmd_data =  
 *   {
 *     .action = FPP_ACTION_REGISTER, // Register new conntrack
 *     .saddr = ...,                  // Source IPv4 address (network endian)
 *     .daddr = ...,                  // Destination IPv4 address (network endian)
 *     .sport = ...,                  // Source port (network endian)
 *     .dport = ...,                  // Destination port (network endian)
 *     .saddr_reply = ...,            // Reply source IPv4 address (network endian). Used for NAT, otherwise equals .daddr
 *     .daddr_reply = ...,            // Reply destination IPv4 address (network endian). Used for NAT, otherwise equals .saddr
 *     .sport_reply = ...,            // Reply source port (network endian). Used for NAT, otherwise equals .dport
 *     .dport_reply = ...,            // Reply destination port (network endian). Used for NAT, otherwise equals .sport
 *     .protocol = ...,               // IP protocol ID (17=UDP, 6=TCP, ...)
 *     .flags = ...,                  // Bidirectional/Single direction
 *     .route_id = ...,               // ID of route previously created with .FPP_CMD_IP_ROUTE command
 *     .route_id_reply = ...          // ID of reply route previously created with .FPP_CMD_IP_ROUTE command
 *   };
 * @endcode
 * Original direction is towards interface defined through @c route_id.
 * Reply direction is towards interface defined through @c route_id_reply.
 * 
 * By default the connection is bidirectional. To create single directional connection, either:
 * - set `.flags |= CTCMD_FLAGS_REP_DISABLED` and don't set @c route_id_reply, or
 * - set `.flags |= CTCMD_FLAGS_ORIG_DISABLED` and don't set @c route_id.
 *
 * To configure NATed connection, set reply addresses and/or ports different than original 
 * addresses and ports. To achieve NAPT (also called PAT), use @c daddr_reply and @c dport_reply.
 * -# `daddr_reply != saddr`: Source address of packets in original direction will be changed
 *    from @c saddr to @c daddr_reply. Destination address of packets in reply direction will be
 *    changed from @c daddr_reply to @c saddr. 
 * -# `dport_reply != sport`: Source port of packets in original direction will be changed
 *    from @c sport to @c dport_reply. Destination port of packets in reply direction will be
 *    changed from @c dport_reply to @c sport. 
 * 
 * To achieve port forwarding or DMZ, use  @c saddr_reply and @c sport_reply:
 * -# `saddr_reply != daddr`: Destination address of packets in original direction will
 *    be changed from @c daddr to @c saddr_reply. Source address of packets in reply direction will be
 *    changed from @c saddr_reply to @c daddr. 
 * -# `sport_reply != dport`: Destination port of packets in original direction will be changed
 *    from @c dport to @c sport_reply. Source port of packets in reply direction will be
 *    changed from @c sport_reply to @c dport. 
 * 
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct_cmd_t cmd_data =  
 *   {
 *     .action = FPP_ACTION_DEREGISTER, // Deregister previously created conntrack
 *     .saddr = ...,                    // Source IPv4 address (network endian)
 *     .daddr = ...,                    // Destination IPv4 address (network endian)
 *     .sport = ...,                    // Source port (network endian)
 *     .dport = ...,                    // Destination port (network endian)
 *     .saddr_reply = ...,              // Reply source IPv4 address (network endian)
 *     .daddr_reply = ...,              // Reply destination IPv4 address (network endian)
 *     .sport_reply = ...,              // Reply source port (network endian)
 *     .dport_reply = ...,              // Reply destination port (network endian)
 *     .protocol = ...,                 // IP protocol ID
 *   };
 * @endcode
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct_cmd_t cmd_data =  
 *   {
 *     .action = ...     // Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 *   };
 * @endcode
 *
 * Response data type for queries: struct @ref fpp_ct_ex_cmd (@c fpp_ct_ex_cmd_t)
 *
 * Response data provided:
 * @code{.c}
 *     rsp_data.saddr;         // Source IPv4 address (network endian)
 *     rsp_data.daddr;         // Destination IPv4 address (network endian)
 *     rsp_data.sport;         // Source port (network endian)
 *     rsp_data.dport;         // Destination port (network endian)
 *     rsp_data.saddr_reply;   // Reply source IPv4 address (network endian), matches daddr
 *     rsp_data.daddr_reply;   // Reply destination IPv4 address (network endian), matches saddr
 *     rsp_data.sport_reply;   // Reply source port (network endian), matches dport
 *     rsp_data.dport_reply;   // Reply destination port (network endian), matches sport
 *     rsp_data.protocol;      // IP protocol ID (17=UDP, 6=TCP, ...)
 * @endcode
 *
 * @hideinitializer
 */

/**
 * @struct      fpp_ct_cmd
 * @brief       Data structure used in various functions for conntrack management
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IPV4_CONNTRACK command. Alternatively
 *                structure @ref fpp_ct_ex_cmd can be used here.
 */
/**
 * @struct      fpp_ct_ex_cmd
 * @brief       Data structure used in various functions for conntrack management
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IPV4_CONNTRACK command. Alternatively
 *                structure @ref fpp_ct_cmd can be used here.
 *              - for reply buffer in functions @ref fci_query or @ref fci_cmd,
 *                with @ref FPP_CMD_IPV4_CONNTRACK command.
 *              - for payload buffer in FCI callback (see @ref fci_register_cb),
 *                with @ref FPP_CMD_IPV4_CONNTRACK_CHANGE event.
 */

/**
 * @def FPP_CMD_IPV4_SET_TIMEOUT
 * @brief Specifies FCI command for setting timeouts of conntracks
 * @details This command sets timeout for conntracks based on protocol. Three kinds
 * of protocols are distinguished: TCP, UDP and others. For each of them timeout can
 * be set independently. For UDP it is possible to set different value for bidirectional
 * and single-directional connection. Default timeout value is 5 days for TCP, 30 s for
 * UDP and 240 s for others.
 *
 * Newly created connections are being created with new timeout values already set. 
 * Previously created connections have their timeout updated with first received packet.
 * 
 * Command Argument Type: struct @ref fpp_timeout_cmd (@c fpp_timeout_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_timeout_cmd_t cmd_data =  
 *   {
 *     .protocol;        // IP protocol to be affected. Either 17 for UDP, 6 for TCP or 0 for others.
 *     .sam_4o6_timeout; // Use 0 for normal connections, 1 for 4over6 PI tunnel connections.
 *     .timeout_value1;  // Timeout value in seconds.
 *     .timeout_value2;  // Optional timeout value which is valid only for UDP connections.
 *                       // If the value is set (non zero), then it affects unidirectional UDP
 *                       // connections only.
 *   };
 * @endcode
 * @hideinitializer
 */

/**
 * @struct      fpp_timeout_cmd
 * @brief       Data structure to be used for command buffer for timeout settings
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IPV4_SET_TIMEOUT command.
 */

/**
 * @def FPP_CMD_IPV6_CONNTRACK
 * @brief Specifies FCI command for working with tracked connections
 * @details This command can be used with various values of `.action`:
 *          - @c FPP_ACTION_REGISTER: Defines a connection and binds it to previously created route(s).
 *          - @c FPP_ACTION_DEREGISTER: Deletes previously defined connection.
 *          - @c FPP_ACTION_QUERY: Gets parameters of existing connection. It creates a snapshot of all active
 *            conntrack entries and replies with first of them.
 *          - @c FPP_ACTION_QUERY_CONT: Shall be called periodically after @c FPP_ACTION_QUERY was called. On each
 *            call it replies with parameters of next connection. It returns @c FPP_ERR_CT_ENTRY_NOT_FOUND when no more
 *            entries exist.
 *
 * Command Argument Type: struct @ref fpp_ct6_cmd (@c fpp_ct6_cmd_t)
 *
 * Action FPP_ACTION_REGISTER
 * --------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct6_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_REGISTER, // Register new conntrack
 *     .saddr[0..3] = ...,            // Source IPv6 address, (network endian)
 *     .daddr[0..3] = ...,            // Destination IPv6 address, (network endian)
 *     .sport = ...,                  // Source port (network endian)
 *     .dport = ...,                  // Destination port (network endian)
 *     .saddr_reply[0..3] = ...,      // Reply source IPv6 address (network endian). Used for NAT, otherwise equals .daddr[0..3]
 *     .daddr_reply[0..3] = ...,      // Reply destination IPv6 address (network endian). Used for NAT, otherwise equals .saddr[0..3]
 *     .sport_reply = ...,            // Reply source port (network endian). Used for NAT, otherwise equals .dport
 *     .dport_reply = ...,            // Reply destination port (network endian). Used for NAT, otherwise equals .sport
 *     .protocol = ...,               // IP protocol ID (17=UDP, 6=TCP, ...)
 *     .flags = ...,                  // Bidirectional/Single direction
 *     .route_id = ...,               // ID of route previously created with .FPP_CMD_IP_ROUTE command
 *     .route_id_reply = ...          // ID of reply route previously created with .FPP_CMD_IP_ROUTE command
 *   };
 * @endcode
 * Original direction is towards interface defined through @c route_id.
 * Reply direction is towards interface defined through @c route_id_reply.
 *
 * By default the connection is bidirectional. To create single directional connection, either:
 * - set `.flags |= CTCMD_FLAGS_REP_DISABLED` and don't set @c route_id_reply, or
 * - set `.flags |= CTCMD_FLAGS_ORIG_DISABLED` and don't set @c route_id.
 *
 * To configure NATed connection, set reply addresses and/or ports different than original
 * addresses and ports. To achieve NAPT (also called PAT), use @c daddr_reply and @c dport_reply.
 * -# `daddr_reply != saddr`: Source address of packets in original direction will be changed
 *    from @c saddr to @c daddr_reply. Destination address of packets in reply direction will be
 *    changed from @c daddr_reply to @c saddr.
 * -# `dport_reply != sport`: Source port of packets in original direction will be changed
 *    from @c sport to @c dport_reply. Destination port of packets in reply direction will be
 *    changed from @c dport_reply to @c sport.
 *
 * To achieve port forwarding or DMZ, use  @c saddr_reply and @c sport_reply:
 * -# `saddr_reply != daddr`: Destination address of packets in original direction will
 *    be changed from @c daddr to @c saddr_reply. Source address of packets in reply direction will be
 *    changed from @c saddr_reply to @c daddr.
 * -# `sport_reply != dport`: Destination port of packets in original direction will be changed
 *    from @c dport to @c sport_reply. Source port of packets in reply direction will be
 *    changed from @c sport_reply to @c dport.
 *
 * Action FPP_ACTION_DEREGISTER
 * ----------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct6_cmd_t cmd_data =
 *   {
 *     .action = FPP_ACTION_DEREGISTER, // Deregister previously created conntrack
 *     .saddr[0..3] = ...,              // Source IPv6 address, (network endian)
 *     .daddr[0..3] = ...,              // Destination IPv6 address, (network endian)
 *     .sport = ...,                    // Source port (network endian)
 *     .dport = ...,                    // Destination port (network endian)
 *     .saddr_reply[0..3] = ...,        // Reply source IPv6 address (network endian)
 *     .daddr_reply[0..3] = ...,        // Reply destination IPv6 address (network endian)
 *     .sport_reply = ...,              // Reply source port (network endian)
 *     .dport_reply = ...,              // Reply destination port (network endian)
 *     .protocol = ...,                 // IP protocol ID
 *   };
 * @endcode
 *
 * Action FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT
 * -------------------------------------------------
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_ct6_cmd_t cmd_data =
 *   {
 *     .action = ...     // Either FPP_ACTION_QUERY or FPP_ACTION_QUERY_CONT
 *   };
 * @endcode
 *
 * Response data type for queries: struct @ref fpp_ct6_ex_cmd (@c fpp_ct6_ex_cmd_t)
 *
 * Response data provided (all values in network byte order):
 * @code{.c}
 *     rsp_data.saddr;         // Source IPv6 address (network endian)
 *     rsp_data.daddr;         // Destination IPv6 address (network endian)
 *     rsp_data.sport;         // Source port (network endian)
 *     rsp_data.dport;         // Destination port (network endian)
 *     rsp_data.saddr_reply;   // Reply source IPv6 address (network endian), matches daddr
 *     rsp_data.daddr_reply;   // Reply destination IPv6 address (network endian), matches saddr
 *     rsp_data.sport_reply;   // Reply source port (network endian), matches dport
 *     rsp_data.dport_reply;   // Reply destination port (network endian), matches sport
 *     rsp_data.protocol;      // IP protocol ID (17=UDP, 6=TCP, ...)
 * @endcode
 *
 * @hideinitializer
 */

/**
 * @struct      fpp_ct6_cmd
 * @brief       Data structure used in various functions for conntrack management
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IPV6_CONNTRACK command. Alternatively
 *                structure @ref fpp_ct6_ex_cmd can be used here.
 */
/**
 * @struct      fpp_ct6_ex_cmd
 * @brief       Data structure used in various functions for conntrack management
 * @details     It can be used:
 *              - for command buffer in functions @ref fci_write, @ref fci_query or
 *                @ref fci_cmd, with @ref FPP_CMD_IPV6_CONNTRACK command. Alternatively
 *                structure @ref fpp_ct6_cmd can be used here.
 *              - for reply buffer in functions @ref fci_query or @ref fci_cmd,
 *                with @ref FPP_CMD_IPV6_CONNTRACK command.
 *              - for payload buffer in FCI callback (see @ref fci_register_cb),
 *                with @ref FPP_CMD_IPV6_CONNTRACK_CHANGE event.
 */

/*
 * @def FPP_CMD_L2BRIDGE_MODE
 * @brief Specifies FCI command that enables or disables automatic L2 bridge learning
 * @details In both modes, the physical addresses can be associated with ports with @ref FPP_CMD_L2BRIDGE_ADD_ENTRY
 *          command. That creates static entry which is valid until it is removed with
 *          @ref FPP_CMD_L2BRIDGE_REMOVE_ENTRY command. In learning mode the physical addresses are also associated
 *          to ports automatically (learned). This way dynamic entries are created. Their lifetime can be specified
 *          by @ref FPP_CMD_L2BRIDGE_LRN_TIMEOUT command. All dynamic entries can be deleted at once with
 *          @ref FPP_CMD_L2BRIDGE_LRN_RESET command.
 *          
 *          The disable command causes that no new entries are learned after this command
 *          and that the learned entries do not expire (the aging timer stops ticking).
 *          The enable command resumes the learning and aging again. The L2 bridge is 
 *          configured to start with learning and aging process enabled. 
 *          
 *          This command may be used to learn entries during certain period and to prevent
 *          further learning for security reasons.
 * 
 * Command Argument Type: struct @ref fpp_l2_bridge_control_cmd (@c fpp_l2_bridge_control_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_control_cmd_t cmd_data =  
 *   {
 *     .mode_timeout = ...   // Either FPP_L2_BRIDGE_MODE_LEARNING or FPP_L2_BRIDGE_MODE_NO_LEARNING
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_control_cmd
 * @brief   Data structure to be used for command buffer for L2 bridge control commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with commands: @ref FPP_CMD_L2BRIDGE_MODE and @ref FPP_CMD_L2BRIDGE_LRN_TIMEOUT.
 */

/*
 * @def FPP_CMD_L2BRIDGE_ENABLE_PORT
 * @brief Specifies FCI command that adds or removes a port to or from the L2 Bridge
 * @details The enable action is selected by setting @c enable_flag to 1 and it causes that
 *          the specified interface becomes part of the bridge. The physical interface acts as
 *          bridge external port and all frames arriving at this interface are subject to the
 *          fast forwarding according to the L2 Bridge rules. Frames sent out by the original
 *          interface are also subject to the L2 Bridge fast forwarding. The interface becomes
 *          L2 Bridge internal port and the physical interface becomes external port and frames
 *          are fast forwarded between them or any other L2 Bridge port.
 *
 *          The disable action, selected by setting @c enable_flag to 0, causes the interface
 *          to return to the normal operation.
 *
 *          The interface is selected by setting the @c input_name to the name of the interface.
 * 
 * Command Argument Type: struct @ref fpp_l2_bridge_enable_cmd (@c fpp_l2_bridge_enable_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_enable_cmd_t cmd_data =  
 *   {
 *       .input_name = ...,   // Interface to add/remove
 *       .enable_flag = ...   // Enable flag, either 1 or 0
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_enable_cmd
 * @brief   Data structure to be used for command buffer for L2 bridge port disabling/enabling commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with @ref FPP_CMD_L2BRIDGE_ENABLE_PORT command.
 */

/*
 * @def FPP_CMD_L2BRIDGE_ADD_ENTRY
 * @brief Specifies FCI command that adds a static entry into the L2 bridge
 * @details Allows adding a fast forwarding rule into L2 bridge without involving Learning process.
 *          Such rule is also not affected by aging. The matching destination address @c destaddr
 *          and the outbound interface name @c output_name must be provided in the command data.
 *          
 *          The command data carry the MAC address to be matched and name of the interface which physical
 *          interface shall be used to send out the frames. It is not possible to send frames to internal port.
 * 
 * Command Argument Type: struct @ref fpp_l2_bridge_add_entry_cmd (@c fpp_l2_bridge_add_entry_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_add_entry_cmd_t cmd_data =  
 *   {
 *     .destaddr = {..., ..., ..., ..., ..., ...}, // Destination MAC address to match
 *     .output_name = ...                          // Interface (external port) to send out matching frames
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_add_entry_cmd
 * @brief   Data structure to be used for command buffer for L2 bridge control commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with @ref FPP_CMD_L2BRIDGE_ADD_ENTRY command.
 */

/*
 * @def FPP_CMD_L2BRIDGE_REMOVE_ENTRY
 * @brief Specifies FCI command that removes static entry from the L2 bridge
 * @details This is a reverse operation to the FPP_CMD_L2BRIDGE_ADD_ENTRY which removes the previously
 *          added static entry. Just the MAC address is needed to identify the entry.
 * 
 * Command Argument Type: struct @ref fpp_l2_bridge_remove_entry_cmd (@c fpp_l2_bridge_remove_entry_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_remove_entry_cmd_t cmd_data =  
 *   {
 *     .destaddr = {..., ..., ..., ..., ..., ...}, // Destination MAC address to match - identifies the entry
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_remove_entry_cmd
 * @brief   Data structure to be used for command buffer for L2 bridge control commands
 * @details It can be used:
 *          - for command buffer in functions @ref fci_write or @ref fci_cmd,
 *            with @ref FPP_CMD_L2BRIDGE_REMOVE_ENTRY command.
 */

/*
 * @def FPP_CMD_L2BRIDGE_LRN_TIMEOUT
 * @brief Specifies FCI command that sets the life time for the learned L2 Bridge entries
 * @details Each learned entry starts with a configured life time which is decremented
 *          each second unless the entry is used to fast forward frame. The entry is deleted
 *          when the life time reaches value 0. The initial value of the life time is configured
 *          by this command. The value is expressed in seconds and the allowed range is 1 to 65535.
 *          The default value is 300 seconds (5 minutes). 
 * @note The entries learned before the timeout change are not updated unless they are used for
 *       fast forward - in each seconds the following two options can happen to the existing entry
 *       - entry life time is decremented
 *       - entry life time is set to the configured value (the latest value applies)
 * 
 * Command Argument Type: struct @ref fpp_l2_bridge_control_cmd (@c fpp_l2_bridge_control_cmd_t)
 * 
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_control_cmd_t cmd_data =  
 *   {
 *     .mode_timeout = ...,   // Timeout in seconds
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @def FPP_CMD_L2BRIDGE_LRN_RESET
 * @brief Specifies an FCI command that removes all learned entries from the L2 bridge
 * @details All learned entries are removed from the bridge immediately. Static entries added by the 
 *          FPP_CMD_L2BRIDGE_ADD_ENTRY are not affected and their removal is possible only by command
 *          FPP_CMD_L2BRIDGE_REMOVE_ENTRY. The command does not use any data. 
 * 
 * Command Argument Type: none (cmd_buf = NULL; cmd_len = 0;)
 *
 * @hideinitializer
 */

/*
 * @def FPP_CMD_L2BRIDGE_QUERY_ENTRY
 * To Be Documented, not implemented yet
 * 
 * @hideinitializer
 */

/*
 * @def FPP_CMD_L2BRIDGE_QUERY_STATUS
 * To Be Documented, not implemented yet
 *
 * @hideinitializer
 */

/** @}*/
