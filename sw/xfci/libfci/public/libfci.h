/* =========================================================================
 *  Copyright (C) 2007 Mindspeed Technologies, Inc.
 *  Copyright 2015-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup    dxgrLibFCI LibFCI
 * @brief       This is the Fast Control Interface available to host applications to
 *              communicate with the networking engine.
 * @details     The FCI is intended to provide a generic configuration and monitoring interface
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
 *              -# User calls @ref fci_cmd() to send a command with arguments to the endpoint.
 *              -# Endpoint receives the command and performs requested actions.
 *              -# Endpoint generates response and sends it back to the client.
 *              -# Client receives the response and informs the caller.
 *              -# User calls @ref fci_close() to finalize the @ref FCI_CLIENT instance.
 *
 * @if INCLUDE_ASYNC_DESC
 *              Usage scenario example - asynchronous message processing:
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
 * @section a_and_d Acronyms and Definitions
 *              - <b>Physical Interface:</b> Interface physically able to send and receive data
 *                (EMAC, HIF). Physical interfaces are pre-defined and can't be added or removed in
 *                runtime. Every physical interface has associated a @b default logical interface and
 *                set of properties like classification algorithm.
 *              - <b>Logical Interface:</b> Extension of physical interface defined by set of rules
 *                which describes Ethernet traffic. Intended to be used to dispatch traffic being
 *                received via particular physical interfaces using 1:N association i.e. traffic
 *                received by a physical interface can be classified and distributed to N logical
 *                interfaces. These can be either connected to SW stack running in host system or
 *                just used to distribute traffic to an arbitrary physical interface(s). Logical
 *                interfaces are dynamic objects and can be created and destroyed in runtime.
 *              - <b>Classification Algorithm:</b> Way how ingress traffic is being processed by
 *                the PFE firmware.
 *              - <b>Route:</b> Routes are representing direction where matching traffic shall be
 *                forwarded to. Every route specifies egress physical interface and MAC address
 *                of next network node.
 *              - <b>Conntrack:</b> "Tracked connection", a data structure containing information
 *                about a connection. In context of this document it always refers
 *                to an IP connection (TCP, UDP, other). Term is equal to 'routing table entry'.
 *                Conntracks contain reference to routes which shall be used in case when a packet
 *                is matching the conntrack properties.
 *
 * @section lfs Functions Summary
 *              - @ref fci_open() <br>
 *                <i>Connect to endpoint and create client instance.</i>
 *              - @ref fci_close() <br>
 *                <i>Close connection and destroy the client instance.</i>
 *              - @ref fci_write() <br>
 *                <i>Execute FCI command without data response.</i>
 *              - @ref fci_cmd() <br>
 *                <i>Execute FCI command with data response.</i>
 *              - @ref fci_query() <br>
 *                <i>Alternative to @ref fci_cmd().</i>
 *              - @ref fci_catch() <br>
 *                <i>Poll for and process received asynchronous messages.</i>
 *              - @ref fci_register_cb() <br>
 *                <i>Register callback to be called in case of received message.</i>
 *
 * @section fci_cs Commands Summary
 *              - @ref FPP_CMD_PHY_IF <br>
 *                <i>Management of physical interfaces.</i>
 *              - @ref FPP_CMD_LOG_IF <br>
 *                <i>Management of logical interfaces.</i>
 *              - @ref FPP_CMD_IF_LOCK_SESSION <br>
 *                <i>Get exclusive access to interfaces.</i>
 *              - @ref FPP_CMD_IF_UNLOCK_SESSION <br>
 *                <i>Cancel exclusive access to interfaces.</i>
 *              - @ref FPP_CMD_L2_BD <br>
 *                <i>L2 bridge domains management.</i>
 *              - @ref FPP_CMD_FP_TABLE <br>
 *                <i>Administration of @ref flex_parser tables.</i>
 *              - @ref FPP_CMD_FP_RULE <br>
 *                <i>Administration of @ref flex_parser rules.</i>
 *              - @ref FPP_CMD_FP_FLEXIBLE_FILTER <br>
 *                <i>Utilization of @ref flex_parser to filter out (drop) frames.</i>
 *              - @ref FPP_CMD_IPV4_RESET <br>
 *                <i>Reset IPv4 (routes, conntracks, ...).</i>
 *              - @ref FPP_CMD_IPV6_RESET <br>
 *                <i>Reset IPv6 (routes, conntracks, ...).</i>
 *              - @ref FPP_CMD_IP_ROUTE <br>
 *                <i>Management of IP routes.</i>
 *              - @ref FPP_CMD_IPV4_CONNTRACK <br>
 *                <i>Management of IPv4 connections.</i>
 *              - @ref FPP_CMD_IPV6_CONNTRACK <br>
 *                <i>Management of IPv6 connections.</i>
 *              - @ref FPP_CMD_IPV4_SET_TIMEOUT <br>
 *                <i>Configuration of connection timeouts.</i>
 *              - @ref FPP_CMD_DATA_BUF_PUT <br>
 *                <i>Send arbitrary data to the accelerator.</i>
 *              - @ref FPP_CMD_SPD <br>
 *                <i>Configure the IPsec offload.</i>
 *              - @ref FPP_CMD_QOS_QUEUE <br>
 *                <i>Management of @ref egress_qos queues.</i>
 *              - @ref FPP_CMD_QOS_SCHEDULER <br>
 *                <i>Management of @ref egress_qos schedulers.</i>
 *              - @ref FPP_CMD_QOS_SHAPER <br>
 *                <i>Management of @ref egress_qos shapers.</i>
 *
 * @section cbks Events summary
 * @if FCI_EVENTS_IMPLEMENTED
 *              - @ref FPP_CMD_IPV4_CONNTRACK_CHANGE <br>
 *                <i>Endpoint reports event related to IPv4 connection.</i>
 *              - @ref FPP_CMD_IPV6_CONNTRACK_CHANGE <br>
 *                <i>Endpoint reports event related to IPv6 connection.</i>
 * @endif
 *              - @ref FPP_CMD_DATA_BUF_AVAIL <br>
 *                <i>Network accelerator sends a data buffer to host.</i>
 *
 * @section if_mgmt Interface Management
 *              Physical Interface
 *              ------------------
 *              Physical interfaces are static objects and are defined at startup.
 *              LibFCI client can get a list of currently available physical interfaces using query
 *              option of the @ref FPP_CMD_PHY_IF command. Every physical interface contains
 *              a list of logical interfaces. Without any configuration all physical interfaces are
 *              in default operation mode. It means that all ingress traffic is processed using only
 *              associated @b default logical interface. Default logical interface is always the tail
 *              of the list of associated logical interfaces. When new logical interface is associated,
 *              it is placed at head position of the list so the default one remains on tail. User
 *              can change the used classification algorithm via update option of the
 *              @ref FPP_CMD_PHY_IF command.
 *
 *              Here are supported operations related to physical interfaces:
 *
 *              To @b list available physical interfaces:
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Read first interface via @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_QUERY.
 *              -# Read next interface(s) via @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_QUERY_CONT.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              To @b modify a physical interface (read-modify-write):
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Read interface properties via @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_QUERY +
 *                 @ref FPP_ACTION_QUERY_CONT.
 *              -# Modify desired properties.
 *              -# Write modifications using @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              Logical Interface
 *              -----------------
 *              Logical interfaces specify traffic endpoints. They are connected to respective
 *              physical interfaces and contain information about which traffic can they accept
 *              and how the accepted traffic shall be processed (where, resp. to which @b physical
 *              interface(s) the matching traffic shall be forwarded). For example, there can be two
 *              logical interfaces associated with an EMAC1, one accepting traffic with VLAN ID = 10
 *              and the second one accepting all remaining traffic. First one can be configured to
 *              forward the matching traffic to EMAC1 and the second one to drop the rest.
 *
 *              Logical interfaces can be created and destroyed in runtime using actions related
 *              to the @ref FPP_CMD_LOG_IF command. Note that first created logical interface
 *              on a physical interface becomes the default one (tail). All subsequent logical
 *              interfaces are added at head position of list of interfaces.
 *
 *              \image latex flexible_router.eps "Configuration Example" width=7cm
 *              The example shows scenario when physical interface EMAC1 is configured in
 *              @ref FPP_IF_OP_FLEXIBLE_ROUTER operation mode:
 *              -# Packet is received via EMAC1 port of the PFE.
 *              -# Classifier walks through list of Logical Interfaces associated with the ingress
 *                 Physical Interface. Every Logical Interface contains a set of classification rules
 *                 (see @ref fpp_if_m_rules_t) the classification process is using to match the ingress
 *                 packet. Note that the list is searched from head to tail, where tail is the default
 *                 logical interface.
 *              -# Information about matching Logical Interface and Physical Interface is passed to
 *                 the Routing and Forwarding Algorithm. The algorithm reads the Logical Interface
 *                 and retrieves forwarding properties.
 *              -# The matching Logical Interface is configured to forward the packet to EMAC2 and
 *                 HIF so the forwarding algorithm ensures that. Optionally, here the packet can be
 *                 modified according to interface setup (VLAN insertion, source MAC address
 *                 replacement, ...).
 *              -# Packet is physically transmitted via dedicated interfaces. Packet replica sent
 *                 to HIF carries metadata describing the matching Logical and Physical interface
 *                 so the host driver can easily dispatch the traffic.
 *
 *              Here are supported operations related to logical interfaces:
 *
 *              To @b create new logical interface:
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Create logical interface via @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_REGISTER.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              To @b list available logical interfaces:
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Read first interface via @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_QUERY.
 *              -# Read next interface(s) via @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_QUERY_CONT.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              To @b modify an interface (read-modify-write):
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Read interface properties via @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_QUERY +
 *                 @ref FPP_ACTION_QUERY_CONT.
 *              -# Modify desired properties.
 *              -# Write modifications using @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_UPDATE.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *              To @b remove logical interface:
 *              -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *              -# Remove logical interface via @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_DEREGISTER.
 *              -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 * @section feature Features
 * @subsection l3_router IPv4/IPv6 Router (TCP/UDP)
 *              Introduction
 *              ------------
 *              The IPv4/IPv6 Forwarder is a dedicated feature to offload the host CPU from tasks
 *              related to forwarding of specific IP traffic between physical interfaces. Normally,
 *              the ingress IP traffic is passed to the host CPU running TCP/IP stack which is responsible
 *              for routing of the packets. Once the stack identifies that a packet does not belong to any
 *              of local IP endpoints it performs lookup in routing table to determine how to process such
 *              traffic. If the routing table contains entry associated with the packet (5-tuple search)
 *              the stack modifies and forwards the packet to another interface to reach its intended
 *              destination node. The PFE can be configured to identify flows which do not need to enter
 *              the host CPU using its internal routing table, and to ensure that the right packets are
 *              forwarded to the right destination interfaces.
 *
 *              Configuration
 *              -------------
 *              The FCI contains mechanisms to setup particular Physical Interfaces to start classifying
 *              packets using Router classification algorithm as well as to manage PFE routing tables.
 *              The router configuration then consists of following steps:
 *              -# Optionally use @ref FPP_CMD_IPV4_RESET or @ref FPP_CMD_IPV6_RESET to initialize the
 *                 router. All previous configuration changes will be discarded.
 *              -# Create one or more routes (@ref FPP_CMD_IP_ROUTE + @ref FPP_ACTION_REGISTER). Once
 *                 created, every route has an unique identifier. Creating route on an physical
 *                 interface causes switch of operation mode of that interface to @ref FPP_IF_OP_ROUTER.
 *              -# Create one or more IPv4 routing table entries (@ref FPP_CMD_IPV4_CONNTRACK +
 *                 @ref FPP_ACTION_REGISTER).
 *              -# Create one or more IPv6 routing table entries (@ref FPP_CMD_IPV6_CONNTRACK +
 *                 @ref FPP_ACTION_REGISTER).
 *              -# Set desired physical interface(s) to router mode @ref FPP_IF_OP_ROUTER using
 *                 @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE. This selects interfaces which
 *                 will use routing algorithm to classify ingress traffic.
 *              -# Enable physical interface(s) by setting the @ref FPP_IF_ENABLED flag via the
 *                 @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *              -# Optionally change MAC address(es) via @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *
 *              From this point the traffic matching created conntracks is processed according to
 *              conntrack properties (e.g. NAT) and fast-forwarded to configured physical interfaces.
 *              Conntracks are subject of aging. When no traffic has been seen for specified time
 *              period (see @ref FPP_CMD_IPV4_SET_TIMEOUT) the conntracks are removed.
 *
 *              Routes and conntracks can be listed using query commands:
 *              - @ref FPP_CMD_IP_ROUTE + @ref FPP_ACTION_QUERY + @ref FPP_ACTION_QUERY_CONT.
 *              - @ref FPP_CMD_IPV4_CONNTRACK + @ref FPP_ACTION_QUERY + @ref FPP_ACTION_QUERY_CONT.
 *              - @ref FPP_CMD_IPV6_CONNTRACK + @ref FPP_ACTION_QUERY + @ref FPP_ACTION_QUERY_CONT.
 *
 *              When conntrack or route are no more required, they can be deleted via corresponding
 *              command:
 *              - @ref FPP_CMD_IP_ROUTE + @ref FPP_ACTION_DEREGISTER,
 *              - @ref FPP_CMD_IPV4_CONNTRACK + @ref FPP_ACTION_DEREGISTER, and
 *              - @ref FPP_CMD_IPV6_CONNTRACK + @ref FPP_ACTION_DEREGISTER.
 *
 *              Deleting route causes deleting all associated conntracks. When the latest route on
 *              an interface is deleted, the interface is put to default operation mode
 *              @ref FPP_IF_OP_DEFAULT.
 *
 * @subsection l2_bridge L2 Bridge (Switch)
 *              Introduction
 *              ------------
 *              The L2 Bridge functionality covers forwarding of packets based on MAC addresses. It
 *              provides possibility to move bridging-related tasks from host CPU to the PFE and thus
 *              offloads the host-based networking stack. The L2 Bridge feature represents a network
 *              switch device implementing following functionality:
 *              - <b>MAC table and address learning:</b>
 *                The L2 bridging functionality is based on determining to which interface an ingress
 *                packet shall be forwarded. For this purpose a network switch device implements so
 *                called bridging table (MAC table) which is searched to get target interface for each
 *                packet entering the switch. If received source MAC address does not match any MAC
 *                table entry then a new entry, containing the Physical Interface which the packet has
 *                been received on, is added - learned. Destination MAC address of an ingress packet
 *                is then used to search the table to determine the target interface.
 *              - <b>Aging:</b>
 *                Each MAC table entry gets default timeout value once learned. In time this timeout is
 *                being decreased until zero is reached. Entries with zero timeout value are automatically
 *                removed from the table. The timeout value is re-set each time the corresponding table
 *                entry is used to process a packet.
 *              - <b>Port migration:</b>
 *                When a MAC address is seen on one interface of the switch and an entry has been created,
 *                it is automatically updated when the MAC address is seen on another interface.
 *              - <b>VLAN Awareness:</b>
 *                The bridge implements VLAN table. This table is used to implement VLAN-based policies
 *                like Ingress and Egress port membership. Feature includes configurable VLAN tagging
 *                and un-tagging functionality per bridge interface (Physical Interface). The bridge
 *                utilizes PFE HW accelerators to perform MAC and VLAN table lookup thus this operation
 *                is highly optimized. Host CPU SW is only responsible for correct bridge configuration
 *                using the dedicated API.
 *
 *              L2 Bridge VLAN Awareness and Domains
 *              --------------------------
 *              The VLAN awareness is based on entities called Bridge Domains (BD) which are visible to
 *              both classifier firmware, and the driver, and are used to abstract particular VLANs.
 *              Every BD contains configurable set of properties:
 *              - Associated VLAN ID.
 *              - Set of Physical Interfaces which are members of the domain.
 *              - Information about which of the member interfaces are ’tagged’ or ’untagged’.
 *              - Instruction how to process matching uni-cast packets (forward, flood, discard, ...).
 *              - Instruction how to process matching multi-cast packets.
 *
 *              The L2 Bridge then consists of multiple BD types:
 *              - <b>The Default BD:</b>
 *                Default domain is used by the classification process when a packet has been received
 *                with default VLAN ID. This can happen either if the packet does not contain VLAN tag
 *                or the VLAN tag is equal to the default VLAN configured within the bridge.
 *              - <b>The Fall-back BD:</b>
 *                This domain is used when packet with an unknown VLAN ID (does not match any standard
 *                or default domain) is received in @ref FPP_IF_OP_VLAN_BRIDGE mode. It is also used
 *                as representation of simple L2 bridge when VLAN awareness is disabled (in case of
 *                the @ref FPP_IF_OP_BRIDGE mode).
 *              - <b>Set of particular Standard BDs:</b>
 *                Standard domain. Specifies what to do when packet with VLAN ID matching the Standard
 *                BD is received.
 *
 *              Configuration
 *              -------------
 *              Here are steps needed to configure VLAN-aware switch:
 *              -# Optionally get list of available physical interfaces and their IDs. See the
 *                 @ref if_mgmt.
  *             -# Create a bridge domain (VLAN domain) (@ref FPP_CMD_L2_BD +
 *                 @ref FPP_ACTION_REGISTER).
  *             -# Configure domain hit/miss actions (@ref FPP_CMD_L2_BD + @ref FPP_ACTION_UPDATE)
 *                 to let the bridge know how to process matching traffic.
 *              -# Add physical interfaces as members of that domain (@ref FPP_CMD_L2_BD +
 *                 @ref FPP_ACTION_UPDATE). Adding interface to a bridge domain causes switch of its
 *                 operation mode to @ref FPP_IF_OP_VLAN_BRIDGE and enabling promiscuous mode on MAC
 *                 level.
 *              -# Set physical interface(s) to VLAN bridge mode @ref FPP_IF_OP_VLAN_BRIDGE using
 *                 @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *              -# Set promiscuous mode and enable physical interface(s) by setting the
 *                 @ref FPP_IF_ENABLED and @ref FPP_IF_PROMISC flags via the @ref FPP_CMD_PHY_IF +
 *                 @ref FPP_ACTION_UPDATE.
 *
 *              For simple, non-VLAN aware switch do:
 *              -# Optionally get list of available physical interfaces and their IDs. See the
 *                 @ref if_mgmt.
  *             -# Add physical interfaces as members of fall-back BD (@ref FPP_CMD_L2_BD +
 *                 @ref FPP_ACTION_UPDATE). The fall-back BD is identified by VLAN 0 and exists
 *                 automatically.
 *              -# Configure domain hit/miss actions (@ref FPP_CMD_L2_BD + @ref FPP_ACTION_UPDATE)
 *                 to let the bridge know how to process matching traffic.
 *              -# Set physical interface(s) to simple bridge mode @ref FPP_IF_OP_BRIDGE using
 *                 @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *              -# Set promiscuous mode and enable physical interface(s) by setting the
 *                 @ref FPP_IF_ENABLED and @ref FPP_IF_PROMISC flags via the @ref FPP_CMD_PHY_IF +
 *                 @ref FPP_ACTION_UPDATE.
 *
 *              Once interfaces are in bridge domain, all ingress traffic is processed according
 *              to bridge domain setup. Unknown source MAC addresses are being learned and after
 *              specified time period without traffic are being aged.
 *
 *              An interface can be added to or removed from BD at any time via
 *              @ref FPP_CMD_L2_BD + @ref FPP_ACTION_UPDATE. When interface is removed
 *              from all bridge domains (is not associated with any BD), its operation mode is
 *              automatically switched to @ref FPP_IF_OP_DEFAULT and MAC promiscuous mode is disabled.
 *
 *              List of available bridge domains with their properties can be retrieved using
 *              @ref FPP_CMD_L2_BD + @ref FPP_ACTION_QUERY + @ref FPP_ACTION_QUERY_CONT.
 *
 * @subsection l2l3_bridge L2L3 Bridge
 *             Introduction
 *             ------------
 *             The L2L3 Bridge is an extension of the available L2 bridge and IPv4/IPv6 Router algorithms.
 *             It requires both algorithms to be configured and (at least one) static entry with local
 *             MAC address flag being set, which denotes that the MAC address belongs to the IP Router.
 *
 *             Whenever a frame arrives it is checked against the local MAC addresses and it is passed
 *             to the IP Router algorithm when the frame destination address equals to one of local MAC
 *             addresses. Otherwise, it is passed to the L2 bridge.
 *
 *             Note that static entry forward list is ignored when the frame is passed to the IP router.
 *
 *             Configuration
 *             -------------
 *             To run the L2L3 Bridge mode
 *             -# Configure L2 Bridge and IP Router algorithms as described in respective sections.
 *             -# Create at least one static entry with local address flag being set.
 *             -# Set physical interface(s) to L2L3 Bridge mode using
 *                 @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE.
 *
 * @subsection flex_parser Flexible Parser
 *             Introduction
 *             ------------
 *             The Flexible Parser is PFE firmware-based feature allowing user to extend standard
 *             ingress packet classification process by set of customizable classification rules.
 *             According to the rules the Flexible Parser can mark frames as ACCEPTED or REJECTED.
 *             The rules are configurable by user and exist in form of tables. Every classification
 *             table entry consist of following fields:
 *             - 32-bit Data field to be compared with value from the ingress frame.
 *             - 32-bit Mask field (active bits are ’1’) specifying which bits of the data field will
 *                be used to perform the comparison.
 *             - 16-bit Configuration field specifying rule properties including the offset to the frame
 *               data which shall be compared.
 *
 *             The number of entries within the table is configurable by user. The table is processed
 *             sequentially starting from entry index 0 until the last one is reached or classification
 *             is terminated by a rule configuration. When none of rules has decided that the packet
 *             shall be accepted or rejected the default result is REJECT.
 *
 *             Example
 *             -------
 *             This is example of how Flexible Parser table can be configured. Every row contains single
 *             rule and processing starts with rule 0. ACCEPT/REJECT means that the classification is
 *             terminated with given result, CONTINUE means that next rule (sequentially) will be
 *             evaluated. CONTINUE with N says that next rule to be evaluated is N. Evaluation of the
 *             latest rule not resulting in ACCEPT or REJECT results in REJECT.
 *
 *             Rule|Flags                         |Mask |Next|Condition
 *             ----|------------------------------|-----|----|-----------------------------------------
 *             0   |FP_FL_INVERT<br>FP_FL_REJECT  |!= 0 |n/a |if ((PacketData&Mask) != (RuleData&Mask))<br> then REJECT<br> else CONTINUE
 *             1   |FP_FL_ACCEPT                  |!= 0 |n/a |if ((PacketData&Mask) == (RuleData&Mask))<br> then ACCEPT<br> else CONTINUE
 *             2   | -                            |!= 0 |4   |if ((PacketData&Mask) == (RuleData&Mask))<br> then CONTINUE with 4<br> else CONTINUE
 *             3   |FP_FL_REJECT                  |= 0  |n/a |REJECT
 *             4   |FP_FL_INVERT                  |!= 0 |6   |if ((PacketData&Mask) != (RuleData&Mask))<br> then CONTINUE with 6<br> else CONTINUE
 *             5   |FP_FL_ACCEPT                  |= 0  |n/a |ACCEPT
 *             6   |FP_FL_INVERT<br>FP_FL_ACCEPT  |!= 0 |n/a |if ((PacketData&Mask) != (RuleData&Mask))<br> then ACCEPT<br> else CONTINUE
 *             7   |FP_FL_REJECT                  |= 0  |n/a |REJECT
 *
 *             Configuration
 *             -------------
 *             -# Create a Flexible Parser table using @ref FPP_CMD_FP_TABLE + @ref FPP_ACTION_REGISTER.
 *             -# Create one or multiple rules with @ref FPP_CMD_FP_RULE + @ref FPP_ACTION_REGISTER.
 *             -# Assing rules to tables via @ref FPP_CMD_FP_TABLE + @ref FPP_ACTION_USE_RULE.
 *                Rules can be removed from table with @ref FPP_ACTION_UNUSE_RULE.
 *
 *             Created table can be used for instance as argument of @ref flex_router. When not needed
 *             the table can be deleted with @ref FPP_CMD_FP_TABLE + @ref FPP_ACTION_DEREGISTER
 *             and particular rules with @ref FPP_CMD_FP_RULE + @ref FPP_ACTION_DEREGISTER. This
 *             cleanup should be always considered since tables and rules are stored in limited PFE
 *             internal memory.
 *
 *             Flexible parser classification introduces performance penalty which is proportional
 *             to number of rules and complexity of the table.
 *
 * @subsection flex_router Flexible Router
 *             Introduction
 *             ------------
 *             Flexible router specifies behavior when ingress packets are classified and routed
 *             according to custom rules different from standard L2 Bridge (Switch) or IPv4/IPv6
 *             Router processing. Feature allows definition of packet distribution rules using physical
 *             and logical interfaces. The classification hierarchy is given by ingress physical
 *             interface containing a configurable set of logical interfaces. Every time a packet is
 *             received via the respective physical interface, which is configured to use the Flexible
 *             Router classification, a walk through the list of associated logical interfaces is
 *             performed. Every logical interface is used to match the packet using interface-specific
 *             rules (@ref fpp_if_m_rules_t). In case of match the matching packet is processed
 *             according to the interface configuration (e.g. forwarded via specific physical
 *             interface(s), dropped, sent to host, ...). In case when more rules are specified, the
 *             logical interface can be configured to apply logical AND or OR to get the match result.
 *             Please see the example within @ref if_mgmt.
 *
 *             Configuration
 *             -------------
 *             -# Lock interface database with @ref FPP_CMD_IF_LOCK_SESSION.
 *             -# Use @ref FPP_CMD_PHY_IF + @ref FPP_ACTION_UPDATE to set a physical interface(s)
 *                to @ref FPP_IF_OP_FLEXIBLE_ROUTER operation mode.
 *             -# Use @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_REGISTER to create new logical
 *                interface(s) if needed.
 *             -# Optionally, if @ref flex_parser is desired to be used as a classification rule,
 *                create table(s) according to @ref flex_parser description.
 *             -# Configure existing logical interface(s) (set match rules and arguments) via
 *                @ref FPP_CMD_LOG_IF + @ref FPP_ACTION_UPDATE.
 *             -# Unlock interface database with @ref FPP_CMD_IF_UNLOCK_SESSION.
 *
 *             Note that Flexible Router can be used to implement certain form of @ref l3_router as
 *             well as @ref l2_bridge. Such usage is of course not recommended since both mentioned
 *             features exist as fully optimized implementation and usage of Flexible Router this way
 *             would pointlessly affect forwarding performance.
 *
 * @subsection ipsec_offload IPsec Offload
 *             Introduction
 *             ------------
 *             The IPsec offload feature is a premium one and requires a special premium firmware version
 *             to be available for use. It allows the chosen IP frames to be transparently encoded by the IPsec and
 *             IPsec frames to be transparently decoded without the CPU intervention using just the PFE and HSE engines.
 *
 *             The SPD database needs to be established on an interface which contains entries describing frame
 *             match criteria together with the SA ID reference to the SA established within the HSE describing
 *             the IPsec processing criteria. Frames matching the criteria are then processed by the HSE according
 *             to the chosen SA and returned for the classification via physical interface of UTIL PE. Normal
 *             classification follows the IPsec processing thus the decrypted packets can be e.g. routed.
 *
 *             Configuration
 *             -------------
 *             -# Use (repeatedly) the @ref FPP_CMD_SPD command with FPP_ACTION_REGISTER action to set the SPD entries
 *             -# Optionally the @ref FPP_CMD_SPD command with FPP_ACTION_DEREGISTER action can be used to delete SPD entries
 *
 *             The HSE also requires the configuration via interfaces of the HSE firmware which is out of the scope of this
 *             document. The SAs referenced within the SPD entries must exist prior creation of the respective SPD entry.
 *
 * @subsection egress_qos Egress QoS
 *             Introduction
 *             ------------
 *             The egress QoS allows user to prioritize, aggregate and shape traffic intended to
 *             leave the accelerator via physical interface. Each physical interface contains dedicated
 *             QoS block with specific number of schedulers, shapers and queues.
 *             @if S32G2
 *                Following applies for the S32G2/PFE:
 *                - Number of queues: 8
 *                - Maximum queue depth: 255
 *                - Probability zones per queue: 8
 *                - Number of schedulers: 2
 *                - Number of scheduler inputs: 8
 *                - Allowed data sources which can be connected to the scheduler inputs:
 *
 *                  Source|Description
 *                  ------|----------------------
 *                  0 - 7 | Queue 0 - 7
 *                  8     | Output of Scheduler 0
 *                  255   | Invalid
 *
 *                - Number of shapers: 4
 *                - Allowed shaper positions:
 *
 *                  Position  |Description
 *                  ----------|------------------------------------------
 *                  0         | Output of Scheduler 1 (QoS master output)
 *                  1 - 8     | Input 0 - 7 of Scheduler 1
 *                  9 - 16    | Input 0 - 7 of Scheduler 0
 *                  255       | Invalid, Shaper disconnected
 *
 *                  Note that only shapers connected to a common scheduler inputs are aware
 *                  of each other and do share the 'conflicting transmission' signal.
 *
 *                Configuration
 *                -------------
 *                By default, the egress QoS topology looks like this:
 *                @verbatim
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
 *                meaning that all queues are connected to Scheduler 1 and the scheduler discipline
 *                is set to Round Robin. Rate mode is set to Data Rate (bps). Queues are in Tail Drop
 *                Mode.
 *
 *                To <b> list queue </b> properties:
 *                -# Read queue properties via @ref FPP_CMD_QOS_QUEUE + @ref FPP_ACTION_QUERY.
 *
 *                To <b> list scheduler </b> properties:
 *                -# Read scheduler properties via @ref FPP_CMD_QOS_SCHEDULER + @ref FPP_ACTION_QUERY.
 *
 *                To <b> list shaper </b> properties:
 *                -# Read shaper properties via @ref FPP_CMD_QOS_SHAPER + @ref FPP_ACTION_QUERY.
 *
 *                To <b> modify queue </b> properties:
 *                -# Read scheduler properties via @ref FPP_CMD_QOS_QUEUE + @ref FPP_ACTION_QUERY.
 *                -# Modify desired properties.
 *                -# Write modifications using @ref FPP_CMD_QOS_QUEUE + @ref FPP_ACTION_UPDATE.
 *
 *                To <b> modify scheduler </b> properties (read-modify-write):
 *                -# Read scheduler properties via @ref FPP_CMD_QOS_SCHEDULER + @ref FPP_ACTION_QUERY.
 *                -# Modify desired properties.
 *                -# Write modifications using @ref FPP_CMD_QOS_SCHEDULER + @ref FPP_ACTION_UPDATE.
 *
 *                To <b> modify shaper </b> properties (read-modify-write):
 *                -# Read shaper properties via @ref FPP_CMD_QOS_SHAPER + @ref FPP_ACTION_QUERY.
 *                -# Modify desired properties.
 *                -# Write modifications using @ref FPP_CMD_QOS_SCHEDULER + @ref FPP_ACTION_UPDATE.
 *
 *                To <b> change QoS topology </b> to following example form:
 *                @verbatim
                           SCH0
                           (WRR)
                        +--------+
                  Q0--->| 0      |           SCH1
                  Q1--->| 1      |           (PQ)
                  Q2--->| 2      |        +--------+
                  Q3--->| 3      +------->| 0      |
                  Q4--->| 4      |        | 1      |
                        | ...    |        | 2      |
                        | 7      |        | 3      +--->
                        +--------+        | ...    |
                                    Q6--->| 6      |
                                    Q7--->| 7      |
                                          +--------+
                  @endverbatim
 *                -# Please see the @ref FPP_CMD_QOS_SCHEDULER for full C example
 *                (@ref fpp_cmd_qos_scheduler.c).
 *
 *                To <b> add traffic shapers </b>:
 *                @verbatim
                           SCH0
                           (WRR)
                        +--------+
                  Q0--->| 0      |               SCH1
                  Q1--->| 1      |               (PQ)
                  Q2--->| 2      |            +--------+
                  Q3--->| 3      +--->SHP0--->| 0      |
                  Q4--->| 4      |            | 1      |
                        | ...    |            | 2      |
                        | 7      |            | 3      +--->SHP2--->
                        +--------+            | ...    |
                                 Q6---SHP1--->| 6      |
                                 Q7---------->| 7      |
                                              +--------+
                  @endverbatim
 *                -# Please see the @ref FPP_CMD_QOS_SHAPER for full C example
 *                (@ref fpp_cmd_qos_shaper.c).
 *             @else
 *                Device is unknown...
 *             @endif
 *
 */

/**
 * @example fpp_cmd_phy_if.c
 * @example fpp_cmd_log_if.c
 * @example fpp_cmd_ip_route.c
 * @example fpp_cmd_ipv4_conntrack.c
 * @example fpp_cmd_ipv6_conntrack.c
 * @example fpp_cmd_qos_queue.c
 * @example fpp_cmd_qos_scheduler.c
 * @example fpp_cmd_qos_shaper.c
 */

/**
 * @addtogroup  dxgrLibFCI
 * @{
 *
 * @file        libfci.h
 * @brief       Generic LibFCI header file
 * @details     This file contains generic API and API description
 */

#ifndef _LIBFCI_H
#define _LIBFCI_H

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */


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
 * @warning     It is not recommended to enable this feature.
 *
 * @hideinitializer
 */
#define FCI_CFG_FORCE_LEGACY_API FALSE

#if FALSE == FCI_CFG_FORCE_LEGACY_API
    /**
	 * @if FCI_EVENTS_IMPLEMENTED
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
	 * @endif
     */
    #define FPP_CMD_IPV4_CONNTRACK_CHANGE   0x0315u
    /**
	 * @if FCI_EVENTS_IMPLEMENTED
     * @def         FPP_CMD_IPV6_CONNTRACK_CHANGE
     * @brief       Callback event value for IPv6 conntracks
     * @details     One of the values the callback registered by @ref fci_register_cb can get in @c fcode
     *              argument.
     *
     *              This value indicates IPv6 conntrack event. The payload argument shall be cast to
     *              @ref fpp_ct6_ex_cmd type. Otherwise the event is same as
     *              @ref FPP_CMD_IPV4_CONNTRACK_CHANGE.
     * @hideinitializer
	 * @endif
     */
    #define FPP_CMD_IPV6_CONNTRACK_CHANGE   0x0415u
#endif /* FCI_CFG_FORCE_LEGACY_API */

/**
 * @def CTCMD_FLAGS_ORIG_DISABLED
 * @brief Disable connection originator
 * @hideinitializer
 */
#define CTCMD_FLAGS_ORIG_DISABLED           (1U << 0)

/**
 * @def         CTCMD_FLAGS_REP_DISABLED
 * @brief       Disable connection replier
 * @details     Used to create uni-directional connections (see @ref FPP_CMD_IPV4_CONNTRACK,
 *              @ref FPP_CMD_IPV4_CONNTRACK)
 * @hideinitializer
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
 * @param[in]   this_client The FCI client instance
 * @param[in]   fcode Command to be executed. Available commands are listed in @ref fci_cs.
 * @param[in]   cmd_len Length of the command arguments structure in bytes
 * @param[in]   pcmd Pointer to structure holding command arguments.
 * @param[out]  rsplen Pointer to memory where length of the data response will be provided
 * @param[out]  rsp_data Pointer to memory where the data response shall be written.
 * @retval      <0 Failed to execute the command.
 * @retval      >=0 Command was executed with given return value (@c FPP_ERR_OK for success).
 */
int fci_query(FCI_CLIENT *this_client, unsigned short fcode, unsigned short cmd_len, unsigned short *pcmd, unsigned short *rsplen, unsigned short *rsp_data);

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
int fci_fd(FCI_CLIENT *this_client);

/** @} */
#endif /* _LIBFCI_H */

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
 * Command Argument Type: @ref fpp_l2_bridge_control_cmd_t
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_control_cmd_t cmd_data =
 *   {
 *     // Timeout: Either FPP_L2_BRIDGE_MODE_LEARNING or FPP_L2_BRIDGE_MODE_NO_LEARNING
 *     .mode_timeout = ...
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_control_cmd_t
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
 * Command Argument Type: @ref fpp_l2_bridge_enable_cmd_t
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
 * @struct  fpp_l2_bridge_enable_cmd_t
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
 * Command Argument Type: @ref fpp_l2_bridge_add_entry_cmd_t
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_add_entry_cmd_t cmd_data =
 *   {
 *     // Destination MAC address to match
 *     .destaddr = {..., ..., ..., ..., ..., ...},
 *     // Interface (external port) to send out matching frames
 *     .output_name = ...
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_add_entry_cmd_t
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
 * Command Argument Type: @ref fpp_l2_bridge_remove_entry_cmd_t
 *
 * Items to be set in command argument structure:
 * @code{.c}
 *   fpp_l2_bridge_remove_entry_cmd_t cmd_data =
 *   {
 *     // Destination MAC address to match - identifies the entry
 *     .destaddr = {..., ..., ..., ..., ..., ...},
 *   };
 * @endcode
 *
 * @hideinitializer
 */

/*
 * @struct  fpp_l2_bridge_remove_entry_cmd_t
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
 * Command Argument Type: @ref fpp_l2_bridge_control_cmd_t
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
