/* =========================================================================
 *  Copyright (C) 2007 Mindspeed Technologies, Inc.
 *  Copyright 2015-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2023,2025 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup    dxgrLibFCI  LibFCI
 * @brief       This is Fast Control Interface available for host applications to
 *              communicate with the networking engine.
 * @details     The FCI is intended to provide a generic configuration and monitoring interface
 *              for the networking acceleration HW. Provided API shall remain the same within all
 *              HW/OS-specific implementations to keep dependent applications portable across
 *              various systems.
 *
 *              The LibFCI is not directly touching the HW. Instead, it only passes commands to
 *              a dedicated software component (OS/HW-specific endpoint) and receives return values.
 *              The endpoint is then responsible for HW configuration. This approach supports
 *              a kernel-user space deployment where the user space contains only API and the logic is
 *              implemented in kernel.
 *
 *              Implementation uses appropriate transport mechanism to pass data between
 *              LibFCI user and the endpoint. For reference: in Linux a netlink socket is used;
 *              in QNX a message is used.
 *
 * @section     how_to_use  How to use the FCI API
 * @subsection  fciuse_cmd  Sending FCI commands
 *              -# Call @ref fci_open() to get an @ref FCI_CLIENT instance, using @ref FCI_GROUP_NONE
 *                 as a multicast group mask. This opens a connection to an FCI endpoint.
 *              -# Call @ref fci_write() or @ref fci_query() to send a command to the endpoint. <br> See @ref fci_cs.
 *                 - Endpoint receives the command and executes requested actions.
 *                 - Endpoint generates a response and sends it back to the client.
 *              -# [optional] Repeat the previous step to send all requested FCI commands.
 *              -# Call @ref fci_close() to finalize the @ref FCI_CLIENT instance.
 * @subsection  fci_owner  FCI ownership in Master-Slave setup
 *              The FCI ownership applies only to PFE driver Master-Slave setup. This feature ensures that at any given time
 *              there is only one driver instance which can successfully issue FCI commands.
 *              Configuration of FCI Ownership is stored in the Master instance. The configuration specifies
 *              which driver instances are allowed to acquire FCI ownership.
 *
 *              There can be only one FCI owner at a time. Only the FCI commands issued by the FCI
 *              owner can be executed successfully. FCI commands issued by a driver instance which
 *              does not have FCI ownership are never executed and return an error code instead.
 *              Driver instance can acquire or release the FCI ownership via the following means:
 *                 - <b>Manually</b>, by requesting the FCI ownership via dedicated FCI commands
 *              @ref FPP_CMD_FCI_OWNERSHIP_LOCK and @ref FPP_CMD_FCI_OWNERSHIP_UNLOCK.
 *              If the ownership is acquired manually, it has to be manually released as well
 *              (responsibility of the requesting driver).
 *                 - <b>Automatically</b>, by acquiring floating FCI ownership if it is not held at the moment by
 *              other client. The floating FCI ownership is granted temporarily for each issued FCI
 *              command. Once the execution of the respective FCI command is finished, the FCI
 *              ownership is automatically released, so other sender can take FCI ownership.
 *
 * @if INCLUDE_ASYNC_DESC
 * @subsection  fciuse_msg  Asynchronous message processing
 *              -# User calls @ref fci_open() to get the @ref FCI_CLIENT instance. It is
 *                 important to set @ref FCI_GROUP_CATCH bit in multicast group mask.
 *              -# User calls @ref fci_register_cb() to register custom function for handling
 *                 asynchronous messages from PFE.
 *              -# User calls @ref fci_catch() function.
 *              -# For each received message, the @ref fci_catch() calls previously registered
 *                 callback.
 *              -# When the callback returns @ref FCI_CB_CONTINUE, the @ref fci_catch() function
 *                 waits for another message.
 *              -# When the callback returns @ref FCI_CB_STOP or when error occurred,
 *                 function @ref fci_catch() returns.
 *              -# User calls fci_close() to finalize the @ref FCI_CLIENT instance.
 * @endif
 *
 * @section     a_and_d  Acronyms and Definitions
 *              - <b>PFE:</b> <br>
 *                Packet Forwarding Engine. A dedicated HW component (networking accelerator)
 *                which is configured by this FCI API.
 *              - <b>NBO:</b> <br>
 *                Network Byte Order. When working with values or properties which are stored in [NBO],
 *                consider using appropriate endianess conversion functions.
 *              - <b>L2/L3/L4:</b> <br>
 *                Layers of the OSI model.
 *              - <b>Physical Interface:</b> <br>
 *                See @ref mgmt_phyif.
 *              - <b>Logical Interface:</b> <br>
 *                See @ref mgmt_logif.
 *              - <b>Classification Algorithm:</b> <br>
 *                Method how ingress traffic is processed by the PFE firmware.
 *              - <b>Route:</b> <br> @anchor ref__route
 *                In the context of PFE, a route represents a direction where the matching
 *                traffic shall be forwarded to. Every route specifies an egress physical interface
 *                and a MAC address of the next network node.
 *              - <b>Conntrack:</b> <br> @anchor ref__conntrack
 *                "Tracked connection", a data structure with information about a connection.
 *                In the context of PFE, it always refers to an IP connection (TCP, UDP, other).
 *                The term is equal to a 'routing table entry'. Each conntrack is linked with some @b route.
 *                The route is used to forward traffic that matches the conntrack's properties.
 *              - <b>RSPAN:</b> <br>
 *                Remote Switch port Analyzer. A way of monitoring traffic via traffic mirroring between ports.
 *                In the context of PFE, this refers to traffic mirroring between physical interfaces.
 *                See chapter @ref mgmt_phyif and its subchapter @link ref__mirroring_rules_mgmt Mirroring rules management @endlink.
 *
 * @section     lfs  Functions Summary
 *              - @ref fci_open() <br>
 *                <i>Connect to endpoint and create a client instance.</i>
 *              - @ref fci_close() <br>
 *                <i>Close a connection to endpoint and destroy the client instance.</i>
 *              - @ref fci_write() <br>
 *                <i>Execute FCI command without data response.</i>
 *              - @ref fci_cmd() <br>
 *                <i>Execute FCI command with data response.</i>
 *              - @ref fci_query() <br>
 *                <i>Alternative to @ref fci_cmd().</i>
 *              - @ref fci_catch() <br>
 *                <i>Poll for and process received asynchronous messages.</i>
 *              - @ref fci_register_cb() <br>
 *                <i>Register a callback to be called in case of a received message.</i>
 *
 * @section     fci_cs  Commands Summary
 *              - @ref FPP_CMD_PHY_IF <br>
 *                <i>Management of physical interfaces.</i>
 *              - @ref FPP_CMD_LOG_IF <br>
 *                <i>Management of logical interfaces.</i>
 *              - @ref FPP_CMD_IF_LOCK_SESSION <br>
 *                <i>Get exclusive access to interface database.</i>
 *              - @ref FPP_CMD_IF_UNLOCK_SESSION <br>
 *                <i>Cancel exclusive access to interface database.</i>
 *              - @ref FPP_CMD_IF_MAC <br>
 *                <i>Management of interface MAC addresses.</i>
 *              - @ref FPP_CMD_MIRROR <br>
 *                <i>Management of interface mirroring rules.</i>
 *              - @ref FPP_CMD_L2_BD <br>
 *                <i>Management of L2 bridge domains.</i>
 *              - @ref FPP_CMD_L2_STATIC_ENT <br>
 *                <i>Management of L2 static entries.</i>
 *              - @ref FPP_CMD_L2_FLUSH_LEARNED <br>
 *                <i>Remove all dynamically learned MAC table entries.</i>
 *              - @ref FPP_CMD_L2_FLUSH_STATIC <br>
 *                <i>Remove all static MAC table entries.</i>
 *              - @ref FPP_CMD_L2_FLUSH_ALL <br>
 *                <i>Remove all MAC table entries.</i>
 *              - @ref FPP_CMD_FP_TABLE <br>
 *                <i>Management of @ref flex_parser tables.</i>
 *              - @ref FPP_CMD_FP_RULE <br>
 *                <i>Management of @ref flex_parser rules.</i>
 *              - @ref FPP_CMD_IPV4_RESET <br>
 *                <i>Remove all IPv4 routes and conntracks.</i>
 *              - @ref FPP_CMD_IPV6_RESET <br>
 *                <i>Remove all IPv6 routes and conntracks.</i>
 *              - @ref FPP_CMD_IP_ROUTE <br>
 *                <i>Management of IP routes.</i>
 *              - @ref FPP_CMD_IPV4_CONNTRACK <br>
 *                <i>Management of IPv4 conntracks.</i>
 *              - @ref FPP_CMD_IPV6_CONNTRACK <br>
 *                <i>Management of IPv6 conntracks.</i>
 *              - @ref FPP_CMD_IPV4_SET_TIMEOUT <br>
 *                <i>Configuration of conntrack timeouts.</i>
 *              - @ref FPP_CMD_DATA_BUF_PUT <br>
 *                <i>Send arbitrary data to the accelerator.</i>
 *              - @ref FPP_CMD_SPD <br>
 *                <i>Management of the IPsec offload.</i>
 *              - @ref FPP_CMD_QOS_QUEUE <br>
 *                <i>Management of @ref egress_qos queues.</i>
 *              - @ref FPP_CMD_QOS_SCHEDULER <br>
 *                <i>Management of @ref egress_qos schedulers.</i>
 *              - @ref FPP_CMD_QOS_SHAPER <br>
 *                <i>Management of @ref egress_qos shapers.</i>
 *              - @ref FPP_CMD_QOS_POLICER <br>
 *                <i>@ref ingress_qos policer enable/disable.</i>
 *              - @ref FPP_CMD_QOS_POLICER_FLOW <br>
 *                <i>Management of @ref ingress_qos packet flows.</i>
 *              - @ref FPP_CMD_QOS_POLICER_WRED <br>
 *                <i>Management of @ref ingress_qos WRED queues.</i>
 *              - @ref FPP_CMD_QOS_POLICER_SHP <br>
 *                <i>Management of @ref ingress_qos shapers.</i>
 *              - @ref FPP_CMD_FCI_OWNERSHIP_LOCK <br>
 *                <i>Management of @ref fci_owner.</i>
 *              - @ref FPP_CMD_FCI_OWNERSHIP_UNLOCK <br>
 *                <i>Management of @ref fci_owner.</i>
 *
 * @section     cbks  Events summary
 * @if FCI_EVENTS_IMPLEMENTED
 *              - @ref FPP_CMD_IPV4_CONNTRACK_CHANGE <br>
 *                <i>Endpoint reports events related to IPv4 conntracks.</i>
 *              - @ref FPP_CMD_IPV6_CONNTRACK_CHANGE <br>
 *                <i>Endpoint reports events related to IPv6 conntracks.</i>
 * @endif
 *              - @ref FPP_CMD_DATA_BUF_AVAIL <br>
 *                <i>Network accelerator sends a data buffer to a host.</i>
 *
 * @section     if_mgmt  Interface Management
 * @subsection  mgmt_phyif  Physical Interface
 *              Physical interfaces are static objects (defined at startup), which represent hardware
 *              interfaces of PFE. They are used by PFE for ingress/egress of network traffic.
 *
 *              Physical interfaces have several configurable properties. See @ref FPP_CMD_PHY_IF
 *              and @ref fpp_phy_if_cmd_t. Among all these properties, a `.mode` property
 *              is especially important. Mode of a physical interface specifies which classification
 *              algorithm shall be applied on ingress traffic of the interface.
 *
 *              Every physical interface can have a list of logical interfaces.
 *              By default, all physical interfaces are in a default mode (@ref FPP_IF_OP_DEFAULT).
 *              In the default mode, ingress traffic of a given physical interface is processed using
 *              only the associated @b default @ref mgmt_logif.
 *
 *              <br>
 *              Supported FCI operations related to physical interfaces:
 *
 *              To @b list available physical interfaces:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Read out properties of physical interface(s).
 *                 <br> (@ref FPP_CMD_PHY_IF + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b modify properties of a physical interface (read-modify-write):
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Read out properties of the target physical interface.
 *                 <br> (@ref FPP_CMD_PHY_IF + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Locally modify the properties. See fpp_phy_if_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_PHY_IF + FPP_ACTION_UPDATE)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              <br><br>
 *              Hardcoded physical interface names and physical interface IDs:
 *              | name     | ID | comment                                                |
 *              | -------- | -- | ------------------------------------------------------ |
 *              | emac0    | 0  | Representation of real physical ports connected to PFE.
 *              | emac1    | 1  | ^
 *              | emac2    | 2  | ^
 *              | ---      | -- | ---reserved---
 *              | util     | 5  | Special internal port for communication with the util firmware. <br> (fully functional only with the PREMIUM firmware)
 *              | hif0     | 6  | Host Interfaces. Used for traffic forwarding between PFE and a host.
 *              | hif1     | 7  | ^
 *              | hif2     | 8  | ^
 *              | hif3     | 9  | ^
 *
 *              MAC address management
 *              ----------------------
 *              @b Emac physical interfaces can have multiple MAC addresses. This can be used
 *              for MAC address filtering - emac physical interfaces can be configured to accept traffic
 *              intended for several different recipients (several different destination MAC addresses).
 *
 *              To @b add a new MAC address to emac physical interface:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Add a new MAC address to emac physical interface.
 *                 <br> (@ref FPP_CMD_IF_MAC + FPP_ACTION_REGISTER)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b remove a MAC address from emac physical interface:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Remove the MAC address from emac physical interface.
 *                 <br> (@ref FPP_CMD_IF_MAC + FPP_ACTION_DEREGISTER)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b list MAC addresses of emac physical interface:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Read out MAC address(es) of the target emac physical interface.
 *                 <br> (@ref FPP_CMD_IF_MAC + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              Mirroring rules management @anchor ref__mirroring_rules_mgmt
 *              --------------------------
 *              Physical interfaces can be configured to mirror their ingress or egress traffic.
 *              Configuration data for mirroring are managed as separate entities - mirroring rules.
 *
 *              To @b create a new mirroring rule:
 *              <br> (@ref FPP_CMD_MIRROR + FPP_ACTION_REGISTER)
 *
 *              To @b assign a mirroring rule to a physical interface:
 *              <br> Write name of the desired mirror rule in `.rx_mirrors[i]` or `.tx_mirrors[i]` property
 *                   of the physical interface. Use steps described in @ref mgmt_phyif, section @b modify.
 *
 *              To @b update a mirroring rule:
 *              <br> (@ref FPP_CMD_MIRROR + FPP_ACTION_UPDATE)
 *
 *              To @b list available mirroring rules:
 *              <br> (@ref FPP_CMD_MIRROR + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              Examples
 *              --------
 *              @ref demo_feature_physical_interface.c
 *
 * @subsection  mgmt_logif  Logical Interface
 *              Logical interfaces are dynamic objects (definable at runtime) which represent traffic endpoints.
 *              They are associated with their respective parent physical interfaces. Logical interfaces
 *              can be used for the following purposes:
 *              - To forward traffic from PFE to a host.
 *              - To forward traffic or its replicas between physical interfaces (1:N distribution).
 *              - To serve as classification & forwarding rules for @ref flex_router.
 *
 *              Logical interfaces have several configurable properties. See @ref FPP_CMD_LOG_IF
 *              and @ref fpp_log_if_cmd_t.
 *
 *              Logical interfaces can be created and destroyed at runtime. Every @e physical interface
 *              can have a list of associated @e logical interfaces. The very first logical interface
 *              in the list (tail position) is considered the @b default logical interface of the given
 *              physical interface. New logical interfaces are always added to the top of the list (head position),
 *              creating a sequence which is ordered from the head (the newest one) back to the tail (the default one).
 *              This forms a classification sequence, which is important if the parent physical interface
 *              operates in the Flexible Router mode.
 *
 *              Similar to physical interfaces, the logical interfaces can be set to a @b promiscuous mode.
 *              For logical interfaces, a promiscuous mode means a logical interface will accept all
 *              ingress traffic it is asked to classify, regardless of the interface's active match rules.
 *
 *              <br>
 *              Supported operations related to logical interfaces:
 *
 *              To @b create a new logical interface in PFE:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Create a new logical interface.
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_REGISTER)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b remove a logical interface from PFE:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Remove the logical interface.
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_DEREGISTER)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b list available logical interfaces:
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Read out properties of logical interface(s).
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 *              To @b modify properties of a logical interface (read-modify-write):
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Read out properties of the target logical interface.
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Locally modify the properties. See fpp_log_if_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_UPDATE)
 *              -# Unlock the interface database.
 *                 <br> (@ref FPP_CMD_IF_UNLOCK_SESSION)
 *
 * @section     features  Features
 * @subsection  l3_router  IPv4/IPv6 Router (TCP/UDP)
 *              Introduction
 *              ------------
 *              IPv4/IPv6 Router is a dedicated feature to offload a host from tasks
 *              related to forwarding of specific IP packets between physical interfaces.
 *              Without the offload, IP packets are passed to the host's TCP/IP stack and
 *              the host is responsible for routing of packets. That is "slow path" routing.
 *              PFE can be configured to provide "fast path" routing, identifying IP packets
 *              which can be forwarded directly by PFE (using its internal routing table)
 *              without host intervention.
 *
 *              Configuration
 *              -------------
 *              -# [optional] Reset the Router.
 *                 <br> This clears all existing IPv4/IPv6 routes and conntracks in PFE.
 *                 <br> (@ref FPP_CMD_IPV4_RESET)
 *                 <br> (@ref FPP_CMD_IPV6_RESET)
 *              -# Create one or more IPv4/IPv6 routes.
 *                 <br> (@ref FPP_CMD_IP_ROUTE + FPP_ACTION_REGISTER)
 *              -# Create one or more IPv4/IPv6 conntracks.
 *                 <br> (@ref FPP_CMD_IPV4_CONNTRACK + FPP_ACTION_REGISTER)
 *                 <br> (@ref FPP_CMD_IPV6_CONNTRACK + FPP_ACTION_REGISTER)
 *              -# Configure the physical interfaces which shall classify their ingress traffic
 *                 by the Router classification algorithm. Use steps described in
 *                 @ref mgmt_phyif (section @b modify) and do the following for each desired physical interface:
 *                 - Set mode of the interface to @ref FPP_IF_OP_ROUTER.
 *                 - Enable the interface by setting the flag @ref FPP_IF_ENABLED.
 *
 *              Once the Router is operational, all ingress IP packets of the Router-configured physical
 *              interfaces are matched against existing conntracks using a 5-tuple match (protocol, source IP,
 *              destination IP, source port, destination port). If a packet matches some existing conntrack,
 *              it is processed and modified according to conntrack's properties (destination MAC, NAT, PAT, etc.)
 *              and then gets fast-forwarded to an egress physical interface as specified by the conntrack's route.
 *
 *              Additional operations
 *              ---------------------
 *              Conntracks are subjected to aging. If no matching packets are detected on a conntrack
 *              for a specified time period, the conntrack is automatically removed from PFE.
 *              To @b set the @b timeout period, use the following command (shared for both
 *              IPv4 and IPv6 conntracks):
 *              <br> (@ref FPP_CMD_IPV4_SET_TIMEOUT)
 *
 *              <br>
 *              To @b remove a route or a conntrack:
 *              - (@ref FPP_CMD_IP_ROUTE + FPP_ACTION_DEREGISTER)
 *              - (@ref FPP_CMD_IPV4_CONNTRACK + FPP_ACTION_DEREGISTER)
 *              - (@ref FPP_CMD_IPV6_CONNTRACK + FPP_ACTION_DEREGISTER)
 *
 *              <small>
 *              Note: <br>
 *              Removing a route which is used by some conntracks causes the associated connntracks
 *              to be removed as well.
 *              </small> <br>
 *
 *              To @b list available routes or conntracks:
 *              - (@ref FPP_CMD_IP_ROUTE + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              - (@ref FPP_CMD_IPV4_CONNTRACK + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              - (@ref FPP_CMD_IPV6_CONNTRACK + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              By default, PFE conntracks decrement TTL of processed IP packets. This behavior can be
 *              set/unset for individual conntracks by their flag @ref CTCMD_FLAGS_TTL_DECREMENT.
 *              To @b modify an already existing conntrack:
 *              - (@ref FPP_CMD_IPV4_CONNTRACK + FPP_ACTION_UPDATE)
 *              - (@ref FPP_CMD_IPV6_CONNTRACK + FPP_ACTION_UPDATE)
 *
 *              Examples
 *              --------
 *              @ref demo_feature_router_simple.c, @ref demo_feature_router_nat.c
 *
 * @subsection  l2_bridge  L2 Bridge (Switch)
 *              Introduction
 *              ------------
 *              L2 Bridge is a dedicated feature to offload a host from tasks related
 *              to MAC address-based forwarding of Ethernet frames. PFE can be configured
 *              to act as a network switch, implementing the following functionality:
 *              - <b>MAC table:</b>
 *                L2 Bridge uses its own MAC table to keep track of encountered MAC addresses.
 *                Each MAC table entry consists of a MAC address and a physical interface
 *                which should be used to reach the given MAC address. MAC table entries can
 *                be dynamic (learned) or static.
 *              - <b>MAC address learning:</b>
 *                L2 Bridge is capable of automatically adding (learning) new MAC table entries from
 *                ingress frames with new (not yet encountered) source MAC addresses.
 *              - <b>Aging:</b>
 *                MAC table entries are subjected to aging. If a MAC table entry is not used for
 *                a certain (hardcoded) time period, it is automatically removed from the MAC table.
 *                Static entries are not affected by aging.
 *              - <b>Static entries:</b>
 *                It is possible to manually add static (non-aging) entries to the MAC table.
 *                Static entries can be used as a part of L2 Bridge forward-only configuration
 *                (with MAC learning disabled). With such a setup, only a predetermined
 *                traffic (matching the static entries) will be forwarded.
 *              - <b>Blocking states of physical interfaces:</b>
 *                Each physical interface which is configured to be a part of the L2 Bridge can
 *                be finetuned to allow/deny MAC learning or frame forwarding of its ingress traffic.
 *                See @ref fpp_phy_if_block_state_t.
 *              - <b>Port migration:</b>
 *                If there is already a learned MAC table entry (a MAC address + a target physical interface)
 *                and the MAC address is detected on another interface, then the entry is automatically
 *                updated (new target physical interface is set).
 *              - <b>VLAN Awareness:</b>
 *                The L2 Bridge uses its own VLAN table to support VLAN-based policies
 *                like Ingress or Egress port membership. It also supports configuration of bridge
 *                domain ports (represented by physical interfaces) to provide VLAN tagging and
 *                untagging services, effectively allowing creation of access / trunk ports.
 *
 *              The L2 Bridge utilizes PFE HW accelerators to perform highly optimized MAC and VLAN
 *              table lookups. Host is responsible only for the initial bridge configuration via
 *              the FCI API.
 *
 *              L2 Bridge VLAN Awareness and Domains
 *              ------------------------------------
 *              The VLAN awareness is based on entities called Bridge Domains (BD), which are visible to
 *              both the classifier firmware and the driver. BDs are used to abstract particular VLANs.
 *              Every BD has a configurable set of properties (see @ref fpp_l2_bd_cmd_t):
 *              - Associated VLAN ID.
 *              - Set of physical interfaces which represent ports of the BD.
 *              - Information about which ports are tagged or untagged.
 *                - Tagged port adds a VLAN tag to egressed frames if they are not VLAN tagged, or keeps
 *                  the tag of the frames intact if they are already VLAN tagged.
 *                - Untagged port removes the VLAN tag from egressed frames if the frames are VLAN tagged.
 *              - Instruction how to process matching uni-cast frames.
 *              - Instruction how to process matching multi-cast frames.
 *
 *              The L2 Bridge recognizes several BD types:
 *              - <b>Default BD:</b> @anchor ref__default_bd <br>
 *                Factory default VLAN ID of this bridge domain is @b 1.
 *                This domain is used to process ingress frames which either have a VLAN tag equal
 *                to the Default BD's VLAN ID, or don't have a VLAN tag at all (untagged Ethernet frames).
 *              - <b>Fall-back BD:</b> <br>
 *                This domain is used to process ingress frames which have an unknown VLAN tag.
 *                Unknown VLAN tag means that the VLAN tag does not match any existing standard BD nor the default BD.
 *              - <b>Standard BD:</b> <br>
 *                Standard user-defined bridge domains. Used by a VLAN-aware Bridge. These BDs
 *                process ingress frames which have a VLAN tag that matches the BD's VLAN ID.
 *
 *              Configuration
 *              -------------
 *              -# Create a bridge domain (VLAN domain).
 *                 <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_REGISTER)
 *              -# Configure hit/miss actions of the bridge domain.
 *                 <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_UPDATE)
 *              -# Configure which physical interfaces are considered members (ports) of the bridge domain.
 *                 Also specify which ports are VLAN tagged and which ports are not.
 *                 <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_UPDATE)
 *              -# Repeat previous steps to create all required bridge domains (VLAN domains).
 *                 Physical interfaces can be members of multiple bridge domains.
 *              -# Configure the physical interfaces which shall classify their ingress traffic
 *                 by the VLAN-aware Bridge classification algorithm. Use steps described in
 *                 @ref mgmt_phyif (section @b modify) and do the following for each desired physical interface:
 *                 - Set mode of the interface to @ref FPP_IF_OP_VLAN_BRIDGE.
 *                 - Enable the promiscuous mode by setting the flag @ref FPP_IF_PROMISC.
 *                 - Enable the interface by setting the flag @ref FPP_IF_ENABLED.
 *
 *              Once the L2 Bridge is operational, ingress Ethernet frames of the Bridge-configured
 *              physical interfaces are processed according to setup of bridge domains. VLAN tag of
 *              every ingress frame is inspected and the frame is then processed by an appropriate bridge domain.
 *
 *              Additional operations
 *              ---------------------
 *              To @b remove a bridge domain:
 *              <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_DEREGISTER)
 *
 *              <small>
 *              Note: <br>
 *              Default BD and Fall-back BD cannot be removed.
 *              </small> <br>
 *
 *              To @b list available bridge domains:
 *              <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              To @b modify properties of a bridge domain (read-modify-write):
 *              -# Read properties of the target bridge domain.
 *                 <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Locally modify the properties. See fpp_l2_bd_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_L2_BD + FPP_ACTION_UPDATE)
 *
 *              Operations related to MAC table static entries
 *              ----------------------------------------------
 *              To @b create a new static entry:
 *              <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_REGISTER)
 *
 *              To @b remove a static entry:
 *              <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_DEREGISTER)
 *
 *              To @b list available static entries:
 *              <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              To @b modify properties of a static entry (read-modify-write):
 *              -# Read properties of the target static entry.
 *                 <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              -# Locally modify the properties. See fpp_l2_static_ent_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_UPDATE)
 *
 *              To @b flush all static entries in PFE:
 *              <br> (@ref FPP_CMD_L2_FLUSH_STATIC)
 *
 *              Examples
 *              --------
 *              @ref demo_feature_L2_bridge_vlan.c
 *
 * @subsection  l2l3_bridge  L2L3 Bridge
 *              Introduction
 *              ------------
 *              L2L3 Bridge is an extension of the L2 Bridge and IP Router features.
 *              It allows both features to be simultaneously available on a physical interface.
 *              Traffic with specific destination MAC addresses is passed to the IP Router.
 *              The rest is handled by the L2 Bridge.
 *
 *              Configuration
 *              -------------
 *              -# Configure @ref l3_router.
 *              -# Configure @ref l2_bridge.
 *              -# Create at least one MAC table static entry with the 'local' flag. Note that
 *                 if a static entry is configured as local, then its egress list is ignored.
 *                 Also note that 'local' static entries must have a correct VLAN (and MAC address)
 *                 in order to properly match the ingress traffic.
 *                 <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_REGISTER)
 *                 <br> (@ref FPP_CMD_L2_STATIC_ENT + FPP_ACTION_UPDATE)
 *              -# Configure the physical interfaces which shall classify their ingress traffic
 *                 by the L2L3 Bridge classification algorithm. Use steps described in
 *                 @ref mgmt_phyif (section @b modify) and do the following for each desired physical interface:
 *                 - Set mode of the interface to @ref FPP_IF_OP_L2L3_VLAN_BRIDGE.
 *                 - Enable the promiscuous mode by setting the flag @ref FPP_IF_PROMISC.
 *                 - Enable the interface by setting the flag @ref FPP_IF_ENABLED.
 *
 *              Once the L2L3 Bridge is operational, it checks the ingress traffic of
 *              L2L3 Bridge-configured physical interfaces against 'local' static entries
 *              in the L2 Bridge MAC table. If traffic's destination MAC matches a MAC address
 *              of some 'local' static entry, then the traffic is passed to the IP Router.
 *              Otherwise the traffic is passed to the L2 Bridge.
 *
 *              Examples
 *              --------
 *              @ref demo_feature_L2L3_bridge_vlan.c
 *
 * @subsection  flex_parser  Flexible Parser
 *              Introduction
 *              ------------
 *              Flexible Parser is a PFE firmware-based feature which can classify ingress traffic
 *              according to a set of custom classification rules. The feature is intended to be used
 *              as an extension of other PFE features/classification algorithms. Flexible Parser consists
 *              of the following elements:
 *              - <b>FP rule:</b>
 *                A classification rule. See @ref FPP_CMD_FP_RULE.
 *                FP rules inspect content of Ethernet frames. Based on the inspection result
 *                (whether the condition of a rule is satisfied or not), a next step of the Flexible Parser
 *                classification process is taken.
 *              - <b>FP table:</b> @anchor ref__fp_table
 *                An ordered set of FP rules. See @ref FPP_CMD_FP_TABLE. These tables can be assigned
 *                as extensions of other PFE features/classification algorithms. Namely, they can be used
 *                as an argument for:
 *                - Flexible Filter of a physical interface. See @ref fpp_phy_if_cmd_t (`.ftable`).
 *                  Flexible Filter acts as a traffic filter, pre-emptively discarding ingress traffic
 *                  which is rejected by the associated FP table. Accepted traffic is then processed
 *                  according to mode of the physical interface.
 *                - @ref FPP_IF_MATCH_FP0 / @ref FPP_IF_MATCH_FP1 match rules of a logical interface.
 *                  See @ref flex_router.
 *
 *              Flexible Parser classification introduces a performance penalty which is proportional to a count
 *              of rules and complexity of a used table. Always consider whether the use of this feature is
 *              really necessary. If it is necessary, then try to use FP tables with as few rules as possible.
 *
 *              Configuration
 *              -------------
 *              -# Create one or multiple FP rules.
 *                 <br> (@ref FPP_CMD_FP_RULE + FPP_ACTION_REGISTER)
 *              -# Create one or multiple FP tables.
 *                 <br> (@ref FPP_CMD_FP_TABLE + FPP_ACTION_REGISTER)
 *              -# Assign rules to tables. Each rule can be assigned only to one table.
 *                 <br> (@ref FPP_CMD_FP_TABLE + FPP_ACTION_USE_RULE)
 *              -# [optional] If required, an FP rule can be removed from an FP table.
 *                 The rule can be then assigned to a different table.
 *                 <br> (@ref FPP_CMD_FP_TABLE + FPP_ACTION_UNUSE_RULE)
 *              -# Use FP tables wherever they are required. See @link ref__fp_table FP table @endlink.
 *
 *              @b WARNING: <br>
 *              Do not modify FP tables which are already in use! Always first remove the FP table
 *              from use, then modify it (add/delete/rearrange rules), then put it back to its use.
 *              Failure to adhere to this warning will result in an undefined behavior of Flexible Parser.
 *              <br>
 *
 *              Once an FP table is configured and put to use, it will start classifying the ingress traffic
 *              in whatever role it was assigned to (see @link ref__fp_table FP table @endlink).
 *              Classification always starts from the very first rule of the table (index 0). Normally,
 *              rules of the table are evaluated sequentially till the traffic is either accepted, rejected,
 *              or the end of the table is reached. If the end of the table is reached and the traffic is
 *              still not accepted nor rejected, then Flexible Parser automatically rejects it.
 *
 *              Based on the action of an FP rule, it is possible to make a jump from the currently
 *              evaluated rule to any other rule in the same table. This can be used in some complex scenarios.
 *
 *              @b WARNING: <br>
 *              It is prohibited to use jumps to create loops. Failure to adhere to this warning
 *              will result in an undefined behavior of Flexible Parser.
 *
 *              Additional operations
 *              ---------------------
 *              It is advised to always remove rules and tables which are not needed, because these
 *              unused objects would needlessly occupy limited internal memory of PFE. To @b remove
 *              an FP rule or an FP table:
 *              - (@ref FPP_CMD_FP_RULE + FPP_ACTION_DEREGISTER)
 *              - (@ref FPP_CMD_FP_TABLE + FPP_ACTION_DEREGISTER)
 *
 *              To @b list FP rules or FP tables:
 *              - (@ref FPP_CMD_FP_RULE + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *              - (@ref FPP_CMD_FP_TABLE + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              FP table example
 *              ----------------
 *              This is an example of how a Flexible Parser table can look like.
 *              - Every row is one FP rule.
 *              - The classification process starts from the rule 0.
 *              - ACCEPT/REJECT means the classification is terminated with the given result.
 *              - CONTINUE means that the next rule in a sequence (next row) shall be evaluated.
 *              - NEXT_RULE <name> means that the next rule to evaluate shall be the rule <name>.
 *              - FrameData is an inspected value from an ingress Ethernet frame.
 *                Each rule can inspect a different value from the frame.
 *                See @ref FPP_CMD_FP_RULE and @ref fpp_fp_rule_props_t, fields `.offset` and `.offset_from`.
 *              - RuleData is a template value inside the FP rule. It is compared with the inspected value
 *                from the ingress Ethernet frame.
 *              - Mask is a bitmask specifying which bits of the RuleData and FrameData shall be compared
 *                (the rest of the bits is ignored).
 *
 *               i | Rule   | Flags                          | Mask | Condition of the rule + actions
 *              ---|--------|--------------------------------|------|------------------------------------------
 *               0 | MyR_01 | @b FP_INVERT <br> FP_REJECT    | != 0 | if ((FrameData & Mask) @b != (RuleData & Mask)) <br> then REJECT <br> else CONTINUE
 *               1 | MyR_02 |    FP_ACCEPT                   | != 0 | if ((FrameData & Mask)==(RuleData & Mask)) <br> then ACCEPT <br> else CONTINUE
 *               2 | MyR_03 |    FP_NEXT_RULE                | != 0 | if ((FrameData & Mask)==(RuleData & Mask)) <br> then NEXT_RULE MyR_11 <br> else CONTINUE
 *               3 | MyR_0r |    FP_REJECT                   | == 0 | REJECT
 *               4 | MyR_11 | @b FP_INVERT <br> FP_NEXT_RULE | != 0 | if ((FrameData & Mask) @b != (RuleData & Mask)) <br> then NEXT_RULE MyR_21 <br> else CONTINUE
 *               5 | MyR_1a |    FP_ACCEPT                   | == 0 | ACCEPT
 *               6 | MyR_21 | @b FP_INVERT <br> FP_ACCEPT    | != 0 | if ((FrameData & Mask) @b != (RuleData & Mask)) <br> then ACCEPT <br> else CONTINUE
 *               7 | MyR_2r |    FP_REJECT                   | == 0 | REJECT
 *
 *              Examples
 *              --------
 *              @ref demo_feature_flexible_filter.c
 *
 * @subsection  flex_router  Flexible Router
 *              Introduction
 *              ------------
 *              Flexible Router is a PFE firmware-based feature which uses logical interfaces
 *              (and their match rules) to classify ingress traffic. Replicas of the accepted traffic
 *              can be forwarded to one or multiple physical interfaces.
 *
 *              Flexible Router classification introduces a performance penalty which is proportional to a count
 *              of used logical interfaces (and their match rules). Always consider whether the use of this feature
 *              is really necessary. If it is necessary, then try to use as few logical interfaces as possible.
 *
 *              Configuration
 *              -------------
 *              -# Lock the interface database.
 *                 <br> (@ref FPP_CMD_IF_LOCK_SESSION)
 *              -# Create one or multiple logical interfaces. See @ref mgmt_logif for more info.
 *                 For Flexible Router purposes, pay attention to the order of logical interfaces.
 *                 <br> (@ref FPP_CMD_LOG_IF + FPP_ACTION_REGISTER)
 *              -# Configure the logical interfaces. Use steps described in
 *                 @ref mgmt_logif (section @b modify) and do the following for each desired logical interface:
 *                 - [optional] Set interface properties such as egress, match rules and match rule arguments.
 *                 - [optional] If multiple match rules are used, then set or clear the flag
 *                   @ref FPP_IF_MATCH_OR in order to specify a logical relation between the rules.
 *                 - Enable the interface by setting the flag @ref FPP_IF_ENABLED.
 *              -# Configure the physical interfaces which shall classify their ingress traffic
 *                 by the Flexible Router classification algorithm. Use steps described in
 *                 @ref mgmt_phyif (section @b modify) and do the following for each desired physical interface:
 *                 - Set mode of the interface to @ref FPP_IF_OP_FLEXIBLE_ROUTER.
 *                 - Enable the interface by setting the flag @ref FPP_IF_ENABLED.
 *              -# Unlock the interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              Once the Flexible Router is operational, it classifies the ingress traffic of
 *              Flexible Router-configured physical interfaces. The process is based on the
 *              classification sequence of logical interfaces (see @ref mgmt_logif). Classifier walks
 *              through the sequence from the head position back to tail, matching the ingress
 *              traffic against match rules of logical interfaces which are in the sequence.
 *              If a match is found (traffic conforms with match rules of the given logical interface),
 *              then the traffic is processed according to the interface's configuration (forwarded,
 *              dropped, sent to a host, etc.).
 *
 *              Configuration example
 *              ---------------------
 *              This example shows a scenario where emac1 physical interface is configured in
 *              the @ref FPP_IF_OP_FLEXIBLE_ROUTER mode. Goal is to classify ingress traffic
 *              on emac1 interface. If the traffic matches classification criteria,
 *              a replica of the traffic is egressed through both emac2 and hif0 interfaces.
 *              <br>
 *              @image latex flexible_router.eps "Configuration Example" width=7cm
 *              -# Traffic is ingressed (received) through emac1 port of PFE.
 *              -# Classifier walks through the list of logical interfaces associated with the emac1
 *                 physical interface.
 *              -# If some logical interface accepts the traffic, then information about the matching
 *                 logical interface (and its parent physical interface) is passed to the Routing and
 *                 Forwarding Algorithm. Algorithm reads the logical interface and retrieves forwarding properties.
 *              -# Traffic is forwarded by the Routing and Forwarding Algorithm based on the provided information.
 *                 In this example, the logical interface specified that a replica of the traffic shall be
 *                 forwarded to both emac2 and hif0 interfaces.
 *              -# Traffic is transmitted via physical interfaces.
 *
 *              Examples
 *              --------
 *              @ref demo_feature_flexible_router.c
 *
 * @subsection  ipsec_offload IPsec Offload
 *              Introduction
 *              ------------
 *              The IPsec offload feature is a premium one and requires a special premium firmware version
 *              to be available for use. It allows the chosen IP frames to be transparently encoded by the IPsec and
 *              IPsec frames to be transparently decoded without the CPU intervention using just the PFE and HSE engines.
 *
 *              @b WARNING: <br>
 *              The IPsec offload feature is available only for some Premium versions of PFE firmware.
 *              The feature should @b not be used with a firmware which does not support it.
 *              Failure to adhere to this warning will result in an undefined behavior of PFE.
 *              <br>
 *
 *              The SPD database needs to be established on an interface which contains entries describing frame
 *              match criteria together with the SA ID reference to the SA established within the HSE describing
 *              the IPsec processing criteria. Frames matching the criteria are then processed by the HSE according
 *              to the chosen SA and returned for the classification via physical interface of UTIL PE. Normal
 *              classification follows the IPsec processing thus the decrypted packets can be e.g. routed.
 *
 *              <br>Supported operations related to the IPsec offload:
 *
 *              To @b create a new SPD entry in the SPD table of a physical interface:
 *              <br> (@ref FPP_CMD_SPD + FPP_ACTION_REGISTER)
 *
 *              To @b remove an SPD entry from the SPD table of a physical interface:
 *              <br> (@ref FPP_CMD_SPD + FPP_ACTION_DEREGISTER)
 *
 *              To @b list existing SPD entries from the SPD table of a physical interface:
 *              <br> (@ref FPP_CMD_SPD + FPP_ACTION_QUERY and FPP_ACTION_QUERY_CONT)
 *
 *              The HSE also requires the configuration via interfaces of the HSE firmware which is out of the scope of this
 *              document. The SAs referenced within the SPD entries must exist prior creation of the respective SPD entry.
 *
 *              Examples
 *              --------
 *              @ref demo_feature_spd.c
 *
 * @subsection  egress_qos Egress QoS
 *              Introduction
 *              ------------
 *              The Egress QoS allows user to prioritize, aggregate and shape traffic intended to
 *              leave the accelerator through some @link mgmt_phyif physical interface @endlink.
 *              Egress QoS is implemented as follows:
 *              - Each @b emac physical interface has its own QoS block.
 *              - All @b hif physical interfaces share one common QoS block.
 *
 *              Every QoS block has a platform-specific number of queues, schedulers and shapers.
 *
 *              @if S32G2
 *                The following applies for each @b S32G2/PFE QoS block:
 *                - @b Queues: @anchor ref__queues
 *                     - Number of queues: 8
 *                     - Size of a @link ref__queue_slot_pools queue slot pool @endlink for each @b emac : 255
 *                     - Size of a queue slot pool for each @b hif : 32
 *                     - Probability zones per queue: 8
 *                       <small><br><br>
 *                       Queues of @b hif interfaces:
 *                       Every hif interface has only @b 2 queues, indexed as follows:
 *                         - [0] : low priority queue (L)
 *                         - [1] : high priority queue (H)
 *
 *                       Use only these indexes if hif queues are configured via FCI commands.
 *                       </small>
 *                - @b Schedulers:
 *                     - Number of schedulers: 2
 *                     - Number of scheduler inputs: 8
 *                     - Traffic sources which can be connected to scheduler inputs:
 *                       (see @link fpp_qos_scheduler_cmd_t @endlink.input_src)
 *                         Source|Description
 *                         ------|----------------------
 *                         0 - 7 | Queue 0 - 7
 *                         8     | Output of Scheduler 0
 *                         255   | Invalid (nothing connected)
 *
 *                - @b Shapers:
 *                     - Number of shapers: 4
 *                     - Shaper positions:
 *                       (see @link fpp_qos_shaper_cmd_t @endlink.position)
 *                         Position  |Description
 *                         ----------|------------------------------------------
 *                         0         | Output of Scheduler 1 (QoS master output)
 *                         1 - 8     | Input 0 - 7 of Scheduler 1
 *                         9 - 16    | Input 0 - 7 of Scheduler 0
 *                         255       | Invalid (shaper disconnected)
 *                     Note that only shapers connected to common scheduler inputs are aware
 *                     of each other and do share the 'conflicting transmission' signal.
 *              @endif
 *
 *              Queue slot pools @anchor ref__queue_slot_pools
 *              ----------------
 *              Every QoS block has it own pool of queue slots. These slots can be assigned to particular queues.
 *              Length of a queue is equal to number of assigned slots. It is possible to configure queue lengths via FCI API.
 *              Setting a queue length (@link fpp_qos_queue_cmd_t @endlink.max) means assigning given number of slots to the given queue.
 *              Sum of all queue lengths of the particular physical interface cannot be bigger than size of its queue slot pool.
 *
 *              See section @link ref__queues Queues @endlink for info about queue slot pool sizes for various physical interfaces.
 *              <small><br><br>
 *              Examples of queue slots distribution:
 *              - Asymmetrical queue slots distribution for @b emac0.
 *                Only 241 slots of the emac0 are used. Emac0 still has 14 free queue slots which could be used to
 *                lenghten some emac0 queues, if needed.
 *                  - queue 0 : 150 slots
 *                  - queues 1 .. 7 : 13 slots per each queue
 *              - Symmetrical queue slots distribution for @b hif0.
 *                All 32 queue slots of the hif0 are used. There are no free queue slots left on hif0.
 *                  - queue 0 : 16 slots
 *                  - queue 1 : 16 slots
 *              </small>
 *
 *              Traffic queueing algorithm
 *              --------------------------
 *              The following pseudocode explains traffic queueing algorithm of PFE:
 *              @code{.c}
 *              .............................................
 *              get_queue_for_packet(pkt)
 *              {
 *                queue = 0;
 *
 *                if (pkt.hasVlanTag)
 *                {
 *                  queue = pkt.VlanHdr.PCP;
 *                }
 *                else
 *                {
 *                  if (pkt.isIPv4)
 *                  {
 *                    queue = (pkt.IPv4Hdr.DSCP) / 8;
 *                  }
 *                  if (pkt.isIPv6)
 *                  {
 *                    queue = (pkt.IPv6Hdr.TrafficClass.DS) / 8;
 *                  }
 *                }
 *
 *                return queue;
 *              }
 *              .............................................
 *              @endcode
 *
 *              <small>
 *              @b Note:
 *              Hif interfaces have only two queues. Their queueing algorithm is similar to the
 *              aforementioned pseudocode, but is modified to produce only two results:
 *                - 0 : traffic belongs to the hif's low priority queue.
 *                - 1 : traffic belongs to the hif's high priority queue.
 *
 *              </small>
 *
 *              Configuration
 *              -------------
 *              By default, the egress QoS topology looks like this:
 *              @verbatim
                         SCH1
                         (RR)
                      +--------+
                Q0--->| 0      |
                Q1--->| 1      |
                Q2--->| 2      |
                Q3--->| 3      +--->
                Q4--->| 4      |
                ...   | ...    |
                Q7--->| 7      |
                      +--------+
                @endverbatim
 *
 *              All queues are connected to Scheduler 1 and the scheduler discipline
 *              is set to Round Robin. Rate mode is set to Data Rate (bps). Queues are
 *              in Tail Drop mode.
 *
 *              To <b> list QoS queue </b> properties:
 *              -# Read QoS queue properties.
 *                 <br> (@ref FPP_CMD_QOS_QUEUE + FPP_ACTION_QUERY)
 *
 *              To <b> list QoS scheduler </b> properties:
 *              -# Read QoS scheduler properties.
 *                 <br> (@ref FPP_CMD_QOS_SCHEDULER + FPP_ACTION_QUERY)
 *
 *              To <b> list QoS shaper </b> properties:
 *              -# Read QoS shaper properties.
 *                 <br> (@ref FPP_CMD_QOS_SHAPER + FPP_ACTION_QUERY)
 *
 *              To <b> modify QoS queue </b> properties (read-modify-write):
 *              -# Read QoS queue properties.
 *                 <br> (@ref FPP_CMD_QOS_QUEUE + FPP_ACTION_QUERY)
 *              -# Locally modify the properties. See fpp_qos_queue_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_QOS_QUEUE + FPP_ACTION_UPDATE)
 *
 *              To <b> modify QoS scheduler </b> properties (read-modify-write):
 *              -# Read QoS scheduler properties.
 *                 <br> (@ref FPP_CMD_QOS_SCHEDULER + FPP_ACTION_QUERY)
 *              -# Locally modify the properties. See fpp_qos_scheduler_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_QOS_SCHEDULER + FPP_ACTION_UPDATE)
 *
 *              To <b> modify QoS shaper </b> properties (read-modify-write):
 *              -# Read QoS shaper properties.
 *                 <br> (@ref FPP_CMD_QOS_SHAPER + FPP_ACTION_QUERY)
 *              -# Locally modify the properties. See fpp_qos_shaper_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_QOS_SHAPER + FPP_ACTION_UPDATE)
 *
 *              Examples
 *              --------
 *              @ref demo_feature_qos.c
 *
 * @subsection  ingress_qos Ingress QoS
 *              Introduction
 *              ------------
 *              The Ingress QoS allows user to prioritize, aggregate and shape traffic as
 *              it comes into the accelerator through an @b emac @link mgmt_phyif physical interface @endlink,
 *              before it is further processed by the accelerator.
 *
 *              Each @b emac physical interface has its own Ingress QoS Policer block.
 *              Every Policer block has its dedicated flow classification table, WRED queues and Ingress QoS shapers.
 *              Exact size of flow classification table and exact numbers/limits of WRED queues and
 *              Ingress QoS shapers are platform-specific.
 *
 *              @if S32G2
 *                The following applies for each @b S32G2/PFE Ingress QoS block ("policer"):
 *                - @b Flow classification table:
 *                     - Maximum number of flows: 64
 *
 *                - @b WRED queues:
 *                     - Number of queues: 3 (DMEM, LMEM, RXF)
 *                     - Maximum queue depth: 8192 for DMEM ; 512 for LMEM and RXF
 *                     - Probability zones per queue: 4
 *
 *                - @b Ingress @b QoS @b shapers:
 *                     - Number of shapers: 2
 *              @endif
 *
 *              Configuration
 *              -------------
 *              By default, the Ingress QoS block ("policer") is organized as follows:
 *              @verbatim
                              policer
         +-------------------------------------------------+
         |                                                 |
         |  +------------+   +--------+   +-------------+  |
Ingress  |  |            |   |  WRED  |   | Ingress QoS |  |   Further PFE
traffic--+->| flow table +-->| queues +-->|   shapers   +--+-->processing
         |  |            |   |        |   |             |  |
         |  +-----+------+   +---+----+   +-------------+  |
         |        |              |                         |
         +--------+--------------+-------------------------+
                  |              |
                 possible packet drop
                @endverbatim
 *
 *              - <tt>policer</tt>  (@ref FPP_CMD_QOS_POLICER)
 *                    - The Ingress QoS block ("policer") itself.
 *                    - The whole block can be enabled/disabled. If the block is disabled,
 *                      it is bypassed and does not affect performance.
 *              - <tt>flow table</tt> which contains flows  (@ref FPP_CMD_QOS_POLICER_FLOW)
 *                    - Flow classification table. Contains user-defined flows.
 *                    - Each flow represents a certain criteria, such as traffic type to match
 *                      (VLAN, ARP, IPv4, etc.) or some data within the traffic to match
 *                      (match VLAN ID, match IP address, etc).
 *                    - Ingressing traffic is compared with flows and their criteria.
 *                      If traffic matches some flow, then (based on flow action), the traffic gets
 *                      either dropped or marked as Managed or Reserved.
 *                      Traffic which does not match any flow from the table is marked as Unmanaged.
 *              - <tt>WRED queues</tt>  (@ref FPP_CMD_QOS_POLICER_WRED)
 *                    - Ingress QoS WRED queues. These queues (by HW design) always use WRED algorithm.
 *                    - Individual queues can be disabled. If all queues are disabled,
 *                      then the WRED queueing module is bypassed.
 *                    - Traffic is queued (or possibly dropped) based on the momentary queue fill and
 *                      also based on the marking of the traffic (Unmanaged/Managed/Reserved).
 *                      See description of @ref fpp_iqos_wred_thr_t enum members.
 *              - <tt>Ingress QoS shapers</tt>  (@ref FPP_CMD_QOS_POLICER_SHP)
 *                    - Ingress QoS shapers. These shapers can be used to shape ingress traffic
 *                      to ensure optimal data flow.
 *                    - Individual shapers can be disabled. If all shapers are disabled,
 *                      then the Ingress QoS shaper module is bypassed.
 *                    - Shapers can be assigned to shape one of several predefined traffic types.
 *                      See description of @ref fpp_iqos_shp_type_t enum members.
 *
 *              To @b get Ingress QoS @b policer status:
 *              -# (@ref FPP_CMD_QOS_POLICER + FPP_ACTION_QUERY)
 *
 *              To @b list Ingress QoS @b flow properties:
 *              -# (@ref FPP_CMD_QOS_POLICER_FLOW + FPP_ACTION_QUERY, FPP_ACTION_QUERY_CONT)
 *
 *              To @b list Ingress QoS @b WRED queue properties:
 *              -# (@ref FPP_CMD_QOS_POLICER_WRED + FPP_ACTION_QUERY)
 *
 *              To @b list Ingress QoS @b shaper properties:
 *              -# (@ref FPP_CMD_QOS_POLICER_SHP + FPP_ACTION_QUERY)
 *
 *              <br>
 *              To @b enable/disable Ingress QoS @b policer:
 *              -# (@ref FPP_CMD_QOS_POLICER + FPP_ACTION_UPDATE)
 *
 *              To @b add Ingress QoS @b flow to flow classification table:
 *              -# (@ref FPP_CMD_QOS_POLICER_FLOW + FPP_ACTION_REGISTER)
 *
 *              To @b remove Ingress QoS @b flow from flow classification table:
 *              -# (@ref FPP_CMD_QOS_POLICER_FLOW + FPP_ACTION_DEREGISTER)
 *
 *              To @b modify Ingress QoS @b WRED queue properties (read-modify-write):
 *              -# Read Ingress QoS WRED queue properties.
 *                 <br> (@ref FPP_CMD_QOS_POLICER_WRED + FPP_ACTION_QUERY)
 *              -# Locally modify the properties. See fpp_qos_policer_wred_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_QOS_POLICER_WRED + FPP_ACTION_UPDATE)
 *
 *              To @b modify Ingress QoS @b shaper properties (read-modify-write):
 *              -# Read Ingress QoS shaper properties.
 *                 <br> (@ref FPP_CMD_QOS_POLICER_SHP + FPP_ACTION_QUERY)
 *              -# Locally modify the properties. See fpp_qos_policer_shp_cmd_t.
 *              -# Write the modified properties back to PFE.
 *                 <br> (@ref FPP_CMD_QOS_POLICER_SHP + FPP_ACTION_UPDATE)
 *
 *              Examples
 *              --------
 *              @ref demo_feature_qos_policer.c
 */

/**
 * @example demo_feature_physical_interface.c
 * @example demo_feature_L2_bridge_vlan.c
 * @example demo_feature_router_simple.c
 * @example demo_feature_router_nat.c
 * @example demo_feature_L2L3_bridge_vlan.c
 * @example demo_feature_flexible_filter.c
 * @example demo_feature_flexible_router.c
 * @example demo_feature_spd.c
 * @example demo_feature_qos.c
 * @example demo_feature_qos_policer.c
 *
 * @example demo_common.c
 * @example demo_phy_if.c
 * @example demo_log_if.c
 * @example demo_if_mac.c
 * @example demo_mirror.c
 * @example demo_l2_bd.c
 * @example demo_fp.c
 * @example demo_rt_ct.c
 * @example demo_spd.c
 * @example demo_qos.c
 * @example demo_qos_pol.c
 * @example demo_fwfeat.c
 * @example demo_fci_owner.c
 */

/**
 * @addtogroup  dxgrLibFCI
 * @{
 *
 * @file        libfci.h
 * @brief       Generic LibFCI header file
 * @details     This file contains generic API and API description
 */

#ifndef LIBFCI_H
#define LIBFCI_H

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */


/**
 * @def         CTCMD_FLAGS_ORIG_DISABLED
 * @brief       Disable connection originator.
 * @details      <!-- empty, but is needed by Doxygen generator -->
 * @hideinitializer
 */
#define CTCMD_FLAGS_ORIG_DISABLED           ((uint16_t)1U << 0)

/**
 * @def         CTCMD_FLAGS_REP_DISABLED
 * @brief       Disable connection replier.
 * @details     Used to create uni-directional connections (see @ref FPP_CMD_IPV4_CONNTRACK,
 *              @ref FPP_CMD_IPV4_CONNTRACK)
 * @hideinitializer
 */
#define CTCMD_FLAGS_REP_DISABLED            ((uint16_t)1U << 1)

/**
 * @def         CTCMD_FLAGS_TTL_DECREMENT
 * @brief       Enable TTL decrement
 * @details     Used to decrement TTL field when the pkt is routed
 * @hideinitializer
 */
#define CTCMD_FLAGS_TTL_DECREMENT            ((uint16_t)1U << 2)

/**
 * @def         FPP_CMD_IPV4_CONNTRACK_CHANGE
 *
 * @brief       FCI event: driver reports status change of some IPv4 conntrack.
 *
 * @details     Related data types: fpp_ct_cmd_t
 */
#define FPP_CMD_IPV4_CONNTRACK_CHANGE   0x0315u

/**
 * @def         FPP_CMD_IPV6_CONNTRACK_CHANGE
 *
 * @brief       FCI event: driver reports status change of some IPv6 conntrack.
 *
 * @details     Related data types: fpp_ct6_cmd_t
 */
#define FPP_CMD_IPV6_CONNTRACK_CHANGE   0x0415u

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
typedef enum
{
    FCI_GROUP_NONE  = 0x00000000, /**< Default MCAST group value, no group, for sending FCI commands */
    FCI_GROUP_CATCH = 0x00000001  /**< MCAST group for catching events */
} fci_mcast_groups_t;

/**
 * @typedef     fci_client_type_t
 * @brief       List of supported FCI client types
 * @details     FCI client can specify using this type to which FCI endpoint shall be connected.
 */
typedef enum
{
    FCI_CLIENT_DEFAULT = 0, /**< Default type (equivalent of legacy FCILIB_FF_TYPE macro) */
	FCILIB_FF_TYPE = 0 /* Due to compatibility purposes */
} fci_client_type_t;

/**
 * @typedef     fci_cb_retval_t
 * @brief       The FCI callback return values
 * @details     These return values shall be used in FCI callback (see @ref fci_register_cb).
 *              It tells @ref fci_catch function whether it should return or continue.
 */
typedef enum
{
    FCI_CB_STOP = 0, /**< Stop waiting for events and exit @ref fci_catch function */
    FCI_CB_CONTINUE  /**< Continue waiting for next events */
} fci_cb_retval_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief       Creates new FCI client and opens a connection to FCI endpoint
 * @details     Binds the FCI client with FCI endpoint. This enables sending/receiving data
 *              to/from the endpoint. Refer to the remaining API for possible communication
 *              options.
 * @param[in]   client_type Client type. Default value is FCI_CLIENT_DEFAULT. See @ref fci_client_type_t.
 * @param[in]   group A 32-bit multicast group mask. Each bit represents single multicast address.
 *                    FCI instance will listen to specified multicast addresses as well it will
 *                    send data to all specified multicast groups. See @ref fci_mcast_groups_t.
 * @return      The FCI client instance or NULL if failed
 */
FCI_CLIENT * fci_open(fci_client_type_t client_type, fci_mcast_groups_t group);

/**
 * @brief       Disconnects from FCI endpoint and destroys FCI client instance
 * @details     Terminate the FCI client and release all allocated resources.
 * @param[in]   client The FCI client instance
 * @return      0 if success, error code otherwise
 */
int fci_close(FCI_CLIENT *client);

/**
 * @brief       Catch and process all FCI messages delivered to the FCI client
 * @details     Function is intended to be called in its own thread. It waits for message/event
 *              reception. If there is an event callback associated with the FCI client, assigned
 *              by function @ref fci_register_cb(), then, when message is received, the callback is
 *              called to process the data. As long as there is no error and the callback returns
 *              @ref FCI_CB_CONTINUE, @ref fci_catch() continues waiting for another message. Otherwise it
 *              returns.
 *
 * @note        This is a blocking function.
 *
 * @note        Multicast group FCI_GROUP_CATCH shall be used when opening the client for catching
 *              messages
 *
 * @see         fci_register_cb()
 * @param[in]   client The FCI client instance
 * @return      0 if success, error code otherwise
 */
int fci_catch(FCI_CLIENT *client);

/**
 * @brief       Run an FCI command with optional data response
 * @details     This routine can be used when one need to perform any command either with or
 *              without data response. If the command responded with some data structure the
 *              structure is written into the rep_buf. The length of the returned data structure
 *              (number of bytes) is written into rep_len.
 *
 * @internal
 * @note        This shall be a blocking call.
 * @endinternal
 *
 * @note        The @c rep_buf buffer must be aligned to 4.
 *
 * @param[in]   client The FCI client instance
 * @param[in]   fcode Command to be executed. Available commands are listed in @ref fci_cs.
 * @param[in]   cmd_buf Pointer to structure holding command arguments.
 * @param[in]   cmd_len Length of the command arguments structure in bytes.
 * @param[out]  rep_buf Pointer to memory where the data response shall be written. Can be NULL.
 * @param[in,out]   rep_len Pointer to variable where number of response bytes shall be written.
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Command was executed with given return value (@c FPP_ERR_OK for success).
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
 * @note        If either @c rsp_data or @c rsplen is NULL pointer, the response data is discarded.
 *
 * @param[in]   client The FCI client instance
 * @param[in]   fcode Command to be executed. Available commands are listed in @ref fci_cs.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[in]   cmd_buf Pointer to structure holding command arguments.
 * @param[out]  rep_len Pointer to memory where length of the data response will be provided
 * @param[out]  rep_buf Pointer to memory where the data response shall be written.
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Command was executed with given return value (@c FPP_ERR_OK for success).
 */
int fci_query(FCI_CLIENT *client, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf, unsigned short *rep_len, unsigned short *rep_buf);
/**
 * @brief       Run an FCI command
 * @details     Similar as the fci_query() but without data response. The endpoint receiving the
 *              command is still responsible for generating response but the response is not
 *              delivered to the caller.
 *
 * @internal
 * @note        This shall be a blocking call.
 * @endinternal
 *
 * @param[in]   client The FCI client instance
 * @param[in]   fcode Command to be executed. Available commands are listed in @ref fci_cs.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[in]   cmd_buf Pointer to structure holding command arguments
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Command was executed with given return value (@c FPP_ERR_OK for success).
 */
int fci_write(FCI_CLIENT *client, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf);

/**
 * @brief       Register event callback function
 * @details     FCI endpoint can send various asynchronous messages to the FCI client. In such case,
 *              a callback registered via this function is executed if @ref fci_catch() is running.
 * @param[in]   client The FCI client instance
 * @param[in]   event_cb The callback function to be executed. When called then @c fcode specifies event
 *                       code (available events are listed in @ref cbks), @c payload is pointer to event
 *                       payload and the @c len is number of bytes in the payload buffer.
 * @return      0 if success, error code otherwise
 * @note        In order to continue receiving messages, the callback function shall always return
 *              @ref FCI_CB_CONTINUE. Any other value will cause the @ref fci_catch to return.
 */
int fci_register_cb(FCI_CLIENT *client, fci_cb_retval_t (*event_cb)(unsigned short fcode, unsigned short len, unsigned short *payload));

/**
 * @brief       Obsolete function, shall not be used
 */
int fci_fd(FCI_CLIENT *client);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* LIBFCI_H */
