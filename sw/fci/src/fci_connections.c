/* =========================================================================
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_connections.c
 * @brief		Connection management functions.
 * @details		All IP connections related functionality provided by the FCI should be
 * 				implemented within this file.
 *
 * 				Uni- and bi-directional connections are supported. Uni-directional creates
 * 				routing table entry in original direction only. Bi-directional adds also
 * 				the opposite direction so adding a bi-directional entry results in addition
 * 				of two routing table entries.
 *
 *				Packet modifications are applied according to routing rules:
 *				- Source MAC address of forwarded packet is changed to MAC address associated with
 *				  egress interface (fci_if_db_entry_t).
 *				- Destination MAC address is changed to the one provided by route (fci_rt_db_entry_t).
 *				- Source/Destination IP and Source/Destination ports are changed according to user's
 *				  request (fpp_ct_cmd_t/fpp_ct6_cmd_t).
 *
 */
#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"

#include "oal_util_net.h"

#ifdef PFE_CFG_FCI_ENABLE

/*	IP address conversion */

#define FCI_CONNECTIONS_CFG_MAX_STR_LEN		128U

static void fci_connections_ipv4_cmd_to_5t(fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv4_cmd_to_5t_rep(fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv6_cmd_to_5t(fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv6_cmd_to_5t_rep(fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple);
static pfe_rtable_entry_t *fci_connections_create_entry(fci_rt_db_entry_t *route,
								pfe_5_tuple_t *tuple, pfe_5_tuple_t *tuple_rep);
static errno_t fci_connections_ipv4_cmd_to_entry(fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv4_cmd_to_rep_entry(fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv6_cmd_to_entry(fpp_ct6_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv6_cmd_to_rep_entry(fpp_ct6_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipvx_ct_cmd(bool_t ipv6, fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len);
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
static char_t * fci_connections_ipv4_cmd_to_str(fpp_ct_cmd_t *ct_cmd);
static char_t * fci_connections_ipv6_cmd_to_str(fpp_ct6_cmd_t *ct6_cmd);
static char_t * fci_connections_entry_to_str(pfe_rtable_entry_t *entry);
static char_t * fci_connections_build_str(bool_t ipv6, uint8_t *sip, uint8_t *dip, uint16_t *sport, uint16_t *dport,
									uint8_t *sip_out, uint8_t *dip_out, uint16_t *sport_out, uint16_t *dport_out, uint8_t *proto);
#endif /* NXP_LOG_ENABLED */					
#endif /* PFE_CFG_VERBOSITY_LEVEL */
static pfe_phy_if_t *fci_connections_rentry_to_if(pfe_rtable_entry_t *entry);

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
/**
 * @brief		Convert CT (IPv4) command to string representation
 * @param[in]	ct_cmd The command
 * @return		Pointer to memory where the output string is located
 */
static char_t * fci_connections_ipv4_cmd_to_str(fpp_ct_cmd_t *ct_cmd)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ct_cmd))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return fci_connections_build_str(FALSE,
										(uint8_t *)&ct_cmd->saddr,
										(uint8_t *)&ct_cmd->daddr,
										&ct_cmd->sport,
										&ct_cmd->dport,
										(uint8_t *)&ct_cmd->daddr_reply,
										(uint8_t *)&ct_cmd->saddr_reply,
										&ct_cmd->dport_reply,
										&ct_cmd->sport_reply,
										(uint8_t *)&ct_cmd->protocol);
}
#endif /* NXP_LOG_ENABLED */
#endif /* PFE_CFG_VERBOSITY_LEVEL */

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
/**
 * @brief		Convert CT (IPv6) command to string representation
 * @param[in]	ct_cmd The command
 * @return		Pointer to memory where the output string is located
 */
static char_t * fci_connections_ipv6_cmd_to_str(fpp_ct6_cmd_t *ct6_cmd)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == ct6_cmd))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return fci_connections_build_str(TRUE,
											(uint8_t *)&ct6_cmd->saddr,
											(uint8_t *)&ct6_cmd->daddr,
											&ct6_cmd->sport,
											&ct6_cmd->dport,
											(uint8_t *)&ct6_cmd->daddr_reply,
											(uint8_t *)&ct6_cmd->saddr_reply,
											&ct6_cmd->dport_reply,
											&ct6_cmd->sport_reply,
											(uint8_t *)&ct6_cmd->protocol);
}
#endif /* NXP_LOG_ENABLED */
#endif /* PFE_CFG_VERBOSITY_LEVEL */

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
/**
 * @brief		Build string from given values
 * @param[in]	ipv6 TRUE if IPv6 values, FALSE for IPv4
 * @param[in]	sip SIP
 * @param[in]	dip DIP
 * @param[in]	sport SPORT
 * @param[in]	dport DPORT
 * @param[in]	sip_out Output SIP
 * @param[in]	dip_out Output DIP
 * @param[in]	sport_out Output SPORT
 * @param[in]	dport_out Output DPORT
 * @param[in]	proto Protocol
 * @return		Pointer to buffer with the output string
 */
static char_t * fci_connections_build_str(bool_t ipv6, uint8_t *sip, uint8_t *dip, uint16_t *sport, uint16_t *dport,
									uint8_t *sip_out, uint8_t *dip_out, uint16_t *sport_out, uint16_t *dport_out, uint8_t *proto)
{
	static char_t buf[FCI_CONNECTIONS_CFG_MAX_STR_LEN];
	uint32_t ipv_flag = (TRUE == ipv6) ? AF_INET6 : AF_INET;
	uint8_t ip_addr_len = (TRUE == ipv6) ? 16U : 4U;
	uint32_t len = 0U;
	char_t sip_str[32+7+1]; /* 1111:1111:1111:1111:1111:1111:1111:1111 */
	char_t sip_out_str[32+7+1]; /* 1111:1111:1111:1111:1111:1111:1111:1111 */
	char_t dip_str[32+7+1]; /* 1111:1111:1111:1111:1111:1111:1111:1111 */
	char_t dip_out_str[32+7+1]; /* 1111:1111:1111:1111:1111:1111:1111:1111 */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == sip) || (NULL == dip) || (NULL == sport) || (NULL == dport)
			|| (NULL == sip_out) || (NULL == dip_out) || (NULL == sport_out) || (NULL == dport_out)
				|| (NULL == proto)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	oal_util_net_inet_ntop(ipv_flag, sip, sip_str, sizeof(sip_str));
	oal_util_net_inet_ntop(ipv_flag, dip, dip_str, sizeof(dip_str));
	oal_util_net_inet_ntop(ipv_flag, sip_out, sip_out_str, sizeof(sip_out_str));
	oal_util_net_inet_ntop(ipv_flag, dip_out, dip_out_str, sizeof(dip_out_str));

	if (0 != memcmp(sip, sip_out, ip_addr_len))
	{
		/*	SIP need to be changed to SIP_OUT */
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSIP: %s --> %s\n", sip_str, sip_out_str);
	}
	else
	{
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSIP: %s\n", sip_str);
	}

	if (0 != memcmp(dip, dip_out, ip_addr_len))
	{
		/*	DIP need to be changed to DIP_OUT */
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDIP: %s --> %s\n", dip_str, dip_out_str);
	}
	else
	{
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDIP: %s\n", dip_str);
	}

	if (*sport != *sport_out)
	{
		/*	SPORT need to be changed to DPORT_REPLY */
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSPORT: %d --> %d\n", oal_ntohs(*sport), oal_ntohs(*sport_out));
	}
	else
	{
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSPORT: %d\n", oal_ntohs(*sport));
	}

	if (*dport != *dport_out)
	{
		/*	DPORT need to be changed to SPORT_REPLY */
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDPORT: %d --> %d\n", oal_ntohs(*dport), oal_ntohs(*dport_out));
	}
	else
	{
		len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDPORT: %d\n", oal_ntohs(*dport));
	}

	/*	Last line. Shall not contain EOL character. */
	len += snprintf(buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len, "\t\tPROTO: %d", *proto);

	return buf;
}
#endif /* NXP_LOG_ENABLED */
#endif /* PFE_CFG_VERBOSITY_LEVEL */

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
/**
 * @brief		Convert routing table entry to a string representation
 * @param[in]	entry The entry
 * @return		Pointer to memory where the output string is located
 */
static char_t * fci_connections_entry_to_str(pfe_rtable_entry_t *entry)
{
	static char_t *err_str = "Entry-to-string conversion failed";
	pfe_5_tuple_t tuple;
	pfe_5_tuple_t tuple_out;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_rtable_entry_to_5t(entry, &tuple))
	{
		return err_str;
	}

	if (EOK != pfe_rtable_entry_to_5t_out(entry, &tuple_out))
	{
		return err_str;
	}

	tuple.sport = oal_htons(tuple.sport);
	tuple.dport = oal_htons(tuple.dport);
	tuple_out.sport = oal_htons(tuple_out.sport);
	tuple_out.dport = oal_htons(tuple_out.dport);

	if (tuple.src_ip.is_ipv4)
	{
		return fci_connections_build_str(FALSE,
											(uint8_t *)&tuple.src_ip.v4,
											(uint8_t *)&tuple.dst_ip.v4,
											&tuple.sport,
											&tuple.dport,
											(uint8_t *)&tuple_out.src_ip.v4,
											(uint8_t *)&tuple_out.dst_ip.v4,
											&tuple_out.sport,
											&tuple_out.dport,
											(uint8_t *)&tuple.proto);
	}
	else
	{
		return fci_connections_build_str(TRUE,
											(uint8_t *)&tuple.src_ip.v6,
											(uint8_t *)&tuple.dst_ip.v6,
											&tuple.sport,
											&tuple.dport,
											(uint8_t *)&tuple_out.src_ip.v6,
											(uint8_t *)&tuple_out.dst_ip.v6,
											&tuple_out.sport,
											&tuple_out.dport,
											(uint8_t *)&tuple.proto);
	}
}
#endif /* NXP_LOG_ENABLED */
#endif /* PFE_CFG_VERBOSITY_LEVEL */

/**
 * @brief		Convert CT command (IPv4) to 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv4_cmd_to_5t(fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	memset(tuple, 0, sizeof(pfe_5_tuple_t));

	memcpy(&tuple->src_ip.v4, &ct_cmd->saddr, 4);
	memcpy(&tuple->dst_ip.v4, &ct_cmd->daddr, 4);
	tuple->src_ip.is_ipv4 = TRUE;
	tuple->dst_ip.is_ipv4 = TRUE;
	tuple->sport = oal_ntohs(ct_cmd->sport);
	tuple->dport = oal_ntohs(ct_cmd->dport);
	tuple->proto = (uint8_t)oal_ntohs(ct_cmd->protocol);
}

/**
 * @brief		Convert CT command (IPv4) to reply 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv4_cmd_to_5t_rep(fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	memset(tuple, 0, sizeof(pfe_5_tuple_t));

	memcpy(&tuple->src_ip.v4, &ct_cmd->saddr_reply, 4);
	memcpy(&tuple->dst_ip.v4, &ct_cmd->daddr_reply, 4);
	tuple->src_ip.is_ipv4 = TRUE;
	tuple->dst_ip.is_ipv4 = TRUE;
	tuple->sport = oal_ntohs(ct_cmd->sport_reply);
	tuple->dport = oal_ntohs(ct_cmd->dport_reply);
	tuple->proto = (uint8_t)oal_ntohs(ct_cmd->protocol);
}

/**
 * @brief		Convert CT command (IPv6) to 5 tuple representation
 * @param[in]	ct6_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv6_cmd_to_5t(fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	memset(tuple, 0, sizeof(pfe_5_tuple_t));

	memcpy(&tuple->src_ip.v6, &ct6_cmd->saddr[0], 16);
	memcpy(&tuple->dst_ip.v6, &ct6_cmd->daddr[0], 16);
	tuple->src_ip.is_ipv4 = FALSE;
	tuple->dst_ip.is_ipv4 = FALSE;
	tuple->sport = oal_ntohs(ct6_cmd->sport);
	tuple->dport = oal_ntohs(ct6_cmd->dport);
	tuple->proto = (uint8_t)oal_ntohs(ct6_cmd->protocol);
}

/**
 * @brief		Convert CT command (IPv6) to reply 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv6_cmd_to_5t_rep(fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	memset(tuple, 0, sizeof(pfe_5_tuple_t));

	memcpy(&tuple->src_ip.v6, &ct6_cmd->saddr_reply[0], 16);
	memcpy(&tuple->dst_ip.v6, &ct6_cmd->daddr_reply[0], 16);
	tuple->src_ip.is_ipv4 = FALSE;
	tuple->dst_ip.is_ipv4 = FALSE;
	tuple->sport = oal_ntohs(ct6_cmd->sport_reply);
	tuple->dport = oal_ntohs(ct6_cmd->dport_reply);
	tuple->proto = (uint8_t)oal_ntohs(ct6_cmd->protocol);
}

/**
 * @brief		Create routing table entry from given inputs
 * @details		Function creates new routing table entry and adjusts its properties according
 * 				to given input values. The setup includes NAT configuration using differences
 * 				between 'tuple' and 'tuple_rep' values. NAT then corresponds with given FCI
 * 				commands (see documentation of FPP_CMD_IPV4_CONNTRACK and FPP_CMD_IPV6_CONNTRACK).
 * @param[in]	route Route to be used to create the entry. MAC address from route is used as
 *					  destination MAC address of forwarded packets.
 * @param[in]	tuple Original flow direction
 * @param[in]	tuple_rep Reply flow direction
 * @return		The routing table entry instance to be inserted into routing table or NULL if failed
 */
static pfe_rtable_entry_t *fci_connections_create_entry(fci_rt_db_entry_t *route,
															pfe_5_tuple_t *tuple, pfe_5_tuple_t *tuple_rep)
{
	pfe_rtable_entry_t *new_entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == route) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Create new entry */
	new_entry = pfe_rtable_entry_create();
	if (NULL == new_entry)
	{
		NXP_LOG_ERROR("Couldn't create routing table entry\n");
		return NULL;
	}

	/*	Set 5-tuple */
	if (EOK != pfe_rtable_entry_set_5t(new_entry, tuple))
	{
		NXP_LOG_ERROR("Can't set 5 tuple\n");
		pfe_rtable_entry_free(new_entry);
		return NULL;
	}

	/*	Set properties */
	pfe_rtable_entry_set_dstif(new_entry, route->iface);
	pfe_rtable_entry_set_timeout(new_entry, fci_connections_get_default_timeout(tuple->proto));
	/*	Set route ID (network endian) */
	pfe_rtable_entry_set_route_id(new_entry, route->id);
	/*	Set ttl decrement by default */
	pfe_rtable_entry_set_ttl_decrement(new_entry);

	/*	Change MAC addresses */
	pfe_rtable_entry_set_out_mac_addrs(new_entry, route->src_mac, route->dst_mac);

	if (NULL != tuple_rep)
	{
		/*	Check if SRC IP NAT is requested */
		if (0 != memcmp(&tuple->src_ip, &tuple_rep->dst_ip, sizeof(pfe_ip_addr_t)))
		{
			/*	SADDR need to be changed to DADDR_REPLY */
			if (EOK != pfe_rtable_entry_set_out_sip(new_entry, &tuple_rep->dst_ip))
			{
				NXP_LOG_ERROR("Couldn't set output SIP\n");
				pfe_rtable_entry_free(new_entry);
				return NULL;
			}
		}

		/*	Check if DST IP NAT is requested */
		if (0 != memcmp(&tuple->dst_ip, &tuple_rep->src_ip, sizeof(pfe_ip_addr_t)))
		{
			/*	DADDR need to be changed to SADDR_REPLY */
			if (EOK != pfe_rtable_entry_set_out_dip(new_entry, &tuple_rep->src_ip))
			{
				NXP_LOG_ERROR("Couldn't set output DIP\n");
				pfe_rtable_entry_free(new_entry);
				return NULL;
			}
		}

		/*	Check if SRC PORT translation is requested */
		if (tuple->sport != tuple_rep->dport)
		{
			/*	SPORT need to be changed to DPORT_REPLY */
			pfe_rtable_entry_set_out_sport(new_entry, tuple_rep->dport);
		}

		/*	Check if DST PORT translation is requested */
		if (tuple->dport != tuple_rep->sport)
		{
			/*	DPORT need to be changed to SPORT_REPLY */
			pfe_rtable_entry_set_out_dport(new_entry, tuple_rep->sport);
		}
	}
	else
	{
		/*	No reply direction defined = no NAT */
		;
	}

	return new_entry;
}

/**
 * @brief		Convert CT command (IPv4) to a new routing table entry
 * @param[in]	ct_cmd The command
 * @param[out]	entry Pointer to memory where pointer to the new entry shall be written
 * @param[out]	iface Pointer where target interface instance shall be written
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_ipv4_cmd_to_entry(fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check if original direction is enabled */
	if (0 != (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
	{
		/*	Original direction is disabled */
		*entry = NULL;
		*iface = NULL;
		return EOK;
	}

	/*	Get route */
	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_ID, (void *)&ct_cmd->route_id);
	if (NULL == route)
	{
		NXP_LOG_ERROR("No such route (0x%x)\n", (uint_t)ct_cmd->route_id);
		return EINVAL;
	}

	/*	Get 5 tuples */
	fci_connections_ipv4_cmd_to_5t(ct_cmd, tuple);
	fci_connections_ipv4_cmd_to_5t_rep(ct_cmd, tuple_rep);

	/*	Create new entry for flow given by the 'tuple' */
	*entry = fci_connections_create_entry(route, tuple, tuple_rep);
	if (NULL == *entry)
	{
		NXP_LOG_ERROR("Couldn't create routing rule\n");
		return EINVAL;
	}

	if (0U != ct_cmd->vlan)
	{
		pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct_cmd->vlan));
	}

	/*	Return interface */
	*iface = route->iface;

	return EOK;
}

/**
 * @brief		Convert CT command (IPv4) to a new routing table entry (reply direction)
 * @param[in]	ct_cmd The command
 * @param[out]	entry Pointer to memory where pointer to the new entry shall be written. NULL indicates
 *				that the reply direction is not requested.
 * @param[out]	iface Pointer where target interface instance shall be written. NULL indicates that the reply
 * 				direction is not requested.
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_ipv4_cmd_to_rep_entry(fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == entry) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check if reply direction is enabled */
	if (0 != (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_REP_DISABLED))
	{
		/*	Reply direction is disabled */
		*entry = NULL;
		*iface = NULL;
		return EOK;
	}

	/*	Get route */
	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_ID, (void *)&ct_cmd->route_id_reply);
	if (NULL == route)
	{
		NXP_LOG_ERROR("No such route (0x%x)\n", (uint_t)oal_ntohl(ct_cmd->route_id_reply));
		return EINVAL;
	}

	/*	Get 5 tuples. Reply entries are created using 'reply' values of CT commands. */
	fci_connections_ipv4_cmd_to_5t(ct_cmd, tuple_rep);
	fci_connections_ipv4_cmd_to_5t_rep(ct_cmd, tuple);

	/*	Create new entry for flow given by the 'tuple' */
	*entry = fci_connections_create_entry(route, tuple, tuple_rep);
	if (NULL == *entry)
	{
		NXP_LOG_ERROR("Couldn't create 'reply' routing rule\n");
		return EINVAL;
	}

	if (0U != ct_cmd->vlan_reply)
	{
		pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct_cmd->vlan_reply));
	}

	/*	Return interface */
	*iface = route->iface;

	return EOK;
}

/**
 * @brief		Convert CT command (IPv6) to a new routing table entry
 * @param[in]	ct6_cmd The command
 * @param[out]	entry Pointer to memory where pointer to the new entry shall be written
 * @param[out]	iface Pointer where target interface instance shall be written
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_ipv6_cmd_to_entry(fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == entry) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check if original direction is enabled */
	if (0 != (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
	{
		/*	Original direction is disabled */
		*entry = NULL;
		return EOK;
	}

	/*	Get route */
	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_ID, (void *)&ct6_cmd->route_id);
	if (NULL == route)
	{
		NXP_LOG_ERROR("No such route (0x%x)\n", (uint_t)ct6_cmd->route_id);
		return EINVAL;
	}

	/*	Get 5 tuples */
	fci_connections_ipv6_cmd_to_5t(ct6_cmd, tuple);
	fci_connections_ipv6_cmd_to_5t_rep(ct6_cmd, tuple_rep);

	/*	Create new entry for flow given by the 'tuple' */
	*entry = fci_connections_create_entry(route, tuple, tuple_rep);
	if (NULL == *entry)
	{
		NXP_LOG_ERROR("Couldn't create routing rule\n");
		return EINVAL;
	}

	if (0U != ct6_cmd->vlan)
	{
		pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct6_cmd->vlan));
	}

	/*	Return interface */
	*iface = route->iface;

	return EOK;
}

/**
 * @brief		Convert CT command (IPv6) to a new routing table entry (reply direction)
 * @param[in]	ct_cmd The command
 * @param[out]	entry Pointer to memory where pointer to the new entry shall be written. NULL indicates
 *				that the reply direction is not requested.
 * @param[out]	iface Pointer where target interface instance shall be written. NULL indicates that the
 * 				reply direction is not requested.
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_ipv6_cmd_to_rep_entry(fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check if reply direction is enabled */
	if (0 != (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_REP_DISABLED))
	{
		/*	Reply direction is disabled */
		*entry = NULL;
		*iface = NULL;
		return EOK;
	}

	/*	Get route */
	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_ID, (void *)&ct6_cmd->route_id_reply);
	if (NULL == route)
	{
		NXP_LOG_ERROR("No such route (0x%x)\n", (uint_t)oal_ntohl(ct6_cmd->route_id_reply));
		return EINVAL;
	}

	/*	Get 5 tuples. Reply entries are created using 'reply' values of CT commands. */
	fci_connections_ipv6_cmd_to_5t(ct6_cmd, tuple_rep);
	fci_connections_ipv6_cmd_to_5t_rep(ct6_cmd, tuple);

	/*	Create new entry for flow given by the 'tuple' */
	*entry = fci_connections_create_entry(route, tuple, tuple_rep);
	if (NULL == *entry)
	{
		NXP_LOG_ERROR("Couldn't create 'reply' routing rule\n");
		return EINVAL;
	}

	if (0U != ct6_cmd->vlan_reply)
	{
		pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct6_cmd->vlan_reply));
	}

	/*	Return interface */
	*iface = route->iface;

	return EOK;
}

/**
 * @brief		Get egress interface associated with routing table entry
 * @param[in]	entry Routing table entry
 * @return		The interface instance or NULL if failed
 */
static pfe_phy_if_t *fci_connections_rentry_to_if(pfe_rtable_entry_t *entry)
{
	fci_t *context = (fci_t *)&__context;
	uint32_t route_id;
	errno_t ret;
	fci_rt_db_entry_t *route;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get route ID */
	ret = pfe_rtable_entry_get_route_id(entry, &route_id);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Can't get route id: %d\n", ret);
		return NULL;
	}

	/*	Get 'reply' route */
	route = fci_rt_db_get_first(&context->route_db, RT_DB_CRIT_BY_ID, (void *)&route_id);
	if (NULL == route)
	{
		NXP_LOG_ERROR("No such route: %d\n", (int_t)oal_ntohl(route_id));
		return NULL;
	}

	return route->iface;
}

/**
 * @brief			Process FPP_CMD_IPV4_CONNTRACK/FPP_CMD_IPV6_CONNTRACK commands
 * @param[in]		ipv6 If TRUE then message carries FPP_CMD_IPV6_CONNTRACK. Else it contains FPP_CMD_IPV4_CONNTRACK.
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_ct_cmd_t/fpp_ct6_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with route DB protected against concurrent accesses.
 */
static errno_t fci_connections_ipvx_ct_cmd(bool_t ipv6, fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len)
{
	fci_t *context = (fci_t *)&__context;
	fpp_ct_cmd_t *ct_cmd, *ct_reply;
	fpp_ct6_cmd_t *ct6_cmd, *ct6_reply;
	errno_t ret = EOK;
	pfe_ip_addr_t sip, dip;
	uint32_t route_id;
	pfe_rtable_entry_t *entry = NULL, *rep_entry = NULL;
	pfe_5_tuple_t tuple;
	pfe_ct_route_actions_t actions;
	pfe_phy_if_t *phy_if = NULL, *phy_if_reply = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((*reply_len < sizeof(fpp_ct_cmd_t)) || (*reply_len < sizeof(fpp_ct6_cmd_t)))
	{
		NXP_LOG_DEBUG("Buffer length does not match expected value (fpp_ct_cmd_t or fpp_ct6_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	if (sizeof(fpp_ct_cmd_t) > sizeof(fpp_ct6_cmd_t))
	{
		memset(reply_buf, 0, sizeof(fpp_ct_cmd_t));
	}
	else
	{
		memset(reply_buf, 0, sizeof(fpp_ct6_cmd_t));
	}

	ct_cmd = (fpp_ct_cmd_t *)(msg->msg_cmd.payload);
	ct6_cmd = (fpp_ct6_cmd_t *)(msg->msg_cmd.payload);

	switch (ct_cmd->action)
	{
		case FPP_ACTION_REGISTER:
		{
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
			if (TRUE == ipv6)
			{
				NXP_LOG_DEBUG("Attempt to register IPv6 connection:\n%s\n", fci_connections_ipv6_cmd_to_str(ct6_cmd));
			}
			else
			{
				NXP_LOG_DEBUG("Attempt to register IPv4 connection:\n%s\n", fci_connections_ipv4_cmd_to_str(ct_cmd));
			}
#endif /* PFE_CFG_VERBOSITY_LEVEL */

			/*	Get new routing table entry in forward direction */
			if (TRUE == ipv6)
			{
				ret = fci_connections_ipv6_cmd_to_entry(ct6_cmd, &entry, &phy_if);
			}
			else
			{
				ret = fci_connections_ipv4_cmd_to_entry(ct_cmd, &entry, &phy_if);
			}

			if (EINVAL == ret)
			{
				NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Couldn't convert command to valid entry\n");
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}
			else if (EOK != ret)
			{
				break;
			}
			else
			{
                /* Empty else is required by MISRA */
				;
			}

			/*	Get new routing table entry in reply direction */
			if (TRUE == ipv6)
			{
				ret = fci_connections_ipv6_cmd_to_rep_entry(ct6_cmd, &rep_entry, &phy_if_reply);
			}
			else
			{
				ret = fci_connections_ipv4_cmd_to_rep_entry(ct_cmd, &rep_entry, &phy_if_reply);
			}

			if (EINVAL == ret)
			{
				NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Couldn't convert command to valid entry (reply direction)\n");
				*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
				ret = EOK;
				break;
			}
			else if (EOK != ret)
			{
				break;
			}
			else
			{
                /* Empty else is required by MISRA */
				;
			}

			/*	Add entry into the routing table */
			if (NULL != entry)
			{
				/*	Remember that there is an associated entry */
				pfe_rtable_entry_set_child(entry, rep_entry);
				pfe_rtable_entry_set_refptr(entry, msg->client);

				ret = pfe_rtable_add_entry(context->rtable, entry);
				if (EEXIST == ret)
				{
					NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Entry already added\n");
					*fci_ret = FPP_ERR_RT_ENTRY_ALREADY_REGISTERED;
					goto free_and_fail;
				}
				else if (EOK != ret)
				{
					NXP_LOG_ERROR("FPP_CMD_IPVx_CONNTRACK: Can't add entry: %d\n", ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					goto free_and_fail;
				}
				else
				{
					NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry added\n");
					*fci_ret = FPP_ERR_OK;
				}

				/*	Enable 'route' interface */
				ret = fci_enable_if(phy_if);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Could not enable interface (%s): %d\n", pfe_phy_if_get_name(phy_if), ret);
					goto free_and_fail;
				}
			}

			/*	Add entry also for reply direction if requested */
			if (NULL != rep_entry)
			{
				ret = pfe_rtable_add_entry(context->rtable, rep_entry);
				if (EEXIST == ret)
				{
					NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Reply entry already added\n");
				}
				else if (EOK != ret)
				{
					NXP_LOG_ERROR("FPP_CMD_IPVx_CONNTRACK: Can't add reply entry: %d\n", ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					goto free_and_fail;
				}
				else
				{
					NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry added (reply direction)\n");
					*fci_ret = FPP_ERR_OK;
				}

				/*	Enable 'reply route' interface */
				ret = fci_enable_if(phy_if_reply);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Could not enable interface (%s): %d\n", pfe_phy_if_get_name(phy_if_reply), ret);
					goto free_and_fail;
				}
			}

			break;

free_and_fail:
			/*	The 'ret' and '*fci_ret' values are already set */

			if (NULL != entry)
			{
				if (EOK != pfe_rtable_del_entry(context->rtable, entry))
				{
					NXP_LOG_ERROR("Can't remove route entry\n");
				}

				pfe_rtable_entry_free(entry);
				entry = NULL;
			}

			if (NULL != phy_if)
			{
				if (EOK != fci_disable_if(phy_if))
				{
					NXP_LOG_DEBUG("Could not disable interface (%s)\n", pfe_phy_if_get_name(phy_if));
				}
			}

			if (NULL != rep_entry)
			{
				if (EOK != pfe_rtable_del_entry(context->rtable, rep_entry))
				{
					NXP_LOG_ERROR("Can't remove route entry\n");
				}

				pfe_rtable_entry_free(rep_entry);
				entry = NULL;
			}

			if (NULL != phy_if_reply)
			{
				if (EOK != fci_disable_if(phy_if_reply))
				{
					NXP_LOG_DEBUG("Could not disable interface (%s)\n", pfe_phy_if_get_name(phy_if_reply));
				}
			}

			break;
		}

		case FPP_ACTION_DEREGISTER:
		{
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
			if (TRUE == ipv6)
			{
				NXP_LOG_DEBUG("Attempt to unregister IPv6 connection:\n%s\n", fci_connections_ipv6_cmd_to_str(ct6_cmd));
			}
			else
			{
				NXP_LOG_DEBUG("Attempt to unregister IPv4 connection:\n%s\n", fci_connections_ipv4_cmd_to_str(ct_cmd));
			}
#endif /* PFE_CFG_VERBOSITY_LEVEL */

			/*	Get entry by 5-tuple */
			if (TRUE == ipv6)
			{
				fci_connections_ipv6_cmd_to_5t(ct6_cmd, &tuple);
			}
			else
			{
				fci_connections_ipv4_cmd_to_5t(ct_cmd, &tuple);
			}

			entry = pfe_rtable_get_first(context->rtable, RTABLE_CRIT_BY_5_TUPLE, (void *)&tuple);

			/*	Delete the entries from table */
			if (NULL != entry)
			{
				/*	Get associated entry */
				rep_entry = pfe_rtable_entry_get_child(entry);

				ret = pfe_rtable_del_entry(context->rtable, entry);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't remove route entry: %d\n", ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}
				else
				{
					phy_if = fci_connections_rentry_to_if(entry);
					if (NULL == phy_if)
					{
						NXP_LOG_ERROR("Interface is NULL\n");
						*fci_ret = FPP_ERR_OK;
						ret = ENOENT;
						break;
					}

					/*	Disable interface */
					ret = fci_disable_if(phy_if);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not disable interface (%s): %d\n", pfe_phy_if_get_name(phy_if), ret);
						*fci_ret = FPP_ERR_OK;
						break;
					}

					/*	Release all entry-related resources */
					NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry removed\n");
					pfe_rtable_entry_free(entry);
					entry = NULL;
					*fci_ret = FPP_ERR_OK;
				}
			}
			else
			{
				NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry not found\n");
				*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
				break;
			}

			/*	Delete also the reply direction */
			if (NULL != rep_entry)
			{
				phy_if_reply = fci_connections_rentry_to_if(rep_entry);
				if (NULL == phy_if_reply)
				{
					NXP_LOG_ERROR("Reply interface is NULL\n");
					*fci_ret = FPP_ERR_OK;
					ret = ENOENT;
					break;
				}

				/*	Disable 'reply' interface */
				ret = fci_disable_if(phy_if_reply);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not disable interface (%s): %d\n", pfe_phy_if_get_name(phy_if_reply), ret);
					*fci_ret = FPP_ERR_OK;
					break;
				}

				ret = pfe_rtable_del_entry(context->rtable, rep_entry);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't remove reply route entry: %d\n", ret);
					*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
					break;
				}
				else
				{
					/*	Release all entry-related resources */
					NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry removed (reply direction)\n");
					pfe_rtable_entry_free(rep_entry);
					rep_entry = NULL;
					*fci_ret = FPP_ERR_OK;
				}
			}
			else
			{
				/*	TODO: Reply entry not found. Should be there or not? */
				;
			}

			break;
		}

		case FPP_ACTION_UPDATE:
		{	
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
			if (TRUE == ipv6)
			{
				NXP_LOG_DEBUG("Attempt to update IPv6 connection:\n%s\n", fci_connections_ipv6_cmd_to_str(ct6_cmd));
			}
			else
			{
				NXP_LOG_DEBUG("Attempt to update IPv4 connection:\n%s\n", fci_connections_ipv4_cmd_to_str(ct_cmd));
			}
#endif /* PFE_CFG_VERBOSITY_LEVEL */

			NXP_LOG_INFO("UPDATED conntrack, only TTL decrement flag will be updated\n");

			/*      Get entry by 5-tuple */
			if (TRUE == ipv6)
 			{
				fci_connections_ipv6_cmd_to_5t(ct6_cmd, &tuple);
			}
			else
			{
				fci_connections_ipv4_cmd_to_5t(ct_cmd, &tuple);
			}

			entry = pfe_rtable_get_first(context->rtable, RTABLE_CRIT_BY_5_TUPLE, (void *)&tuple);

			if (NULL != entry)
			{

				if (TRUE == ipv6)
				{
					if (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_TTL_DECREMENT)
					{
						pfe_rtable_entry_set_ttl_decrement(entry);
					}
					else
					{
						pfe_rtable_entry_remove_ttl_decrement(entry);
					}
				}
				else
				{
					if (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_TTL_DECREMENT)
					{
						pfe_rtable_entry_set_ttl_decrement(entry);
					}
					else
					{
						pfe_rtable_entry_remove_ttl_decrement(entry);
					}
				}

				*fci_ret = FPP_ERR_OK;
				ret = EOK;

			}
			else
			{
				NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry not found\n");
				*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
				ret = EEXIST;
			}


			break;
		}

		case FPP_ACTION_QUERY:
		{
			pfe_rtable_get_criterion_t crit = (TRUE == ipv6) ? RTABLE_CRIT_ALL_IPV6 : RTABLE_CRIT_ALL_IPV4;

			entry = pfe_rtable_get_first(context->rtable, crit, NULL);
			if (NULL == entry)
			{
				ret = EOK;
				*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
				break;
			}
		}
		/* FALLTHRU */

		case FPP_ACTION_QUERY_CONT:
		{
			if (NULL == entry)
			{
				entry = pfe_rtable_get_next(context->rtable);
				if (NULL == entry)
				{
					ret = EOK;
					*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
					break;
				}
			}

			ct6_reply = (fpp_ct6_cmd_t *)(reply_buf);
			ct_reply = (fpp_ct_cmd_t *)(reply_buf);

			/*	Set the reply length */
			if (TRUE == ipv6)
			{
				*reply_len = sizeof(fpp_ct6_cmd_t);
			}
			else
			{
				*reply_len = sizeof(fpp_ct_cmd_t);
			}

			/*	Build reply structure */
			pfe_rtable_entry_get_sip(entry, &sip);
			pfe_rtable_entry_get_dip(entry, &dip);
			pfe_rtable_entry_get_route_id(entry, &route_id);

			if (TRUE == ipv6)
			{
				memcpy(ct6_reply->saddr, &sip.v6, 16);
				memcpy(ct6_reply->daddr, &dip.v6, 16);
				ct6_reply->sport = oal_htons(pfe_rtable_entry_get_sport(entry));
				ct6_reply->dport = oal_htons(pfe_rtable_entry_get_dport(entry));
				ct6_reply->vlan = oal_htons(pfe_rtable_entry_get_out_vlan(entry));
				memcpy(ct6_reply->saddr_reply, ct6_reply->daddr, 16);
				memcpy(ct6_reply->daddr_reply, ct6_reply->saddr, 16);
				ct6_reply->sport_reply = ct6_reply->dport;
				ct6_reply->dport_reply = ct6_reply->sport;
				ct6_reply->protocol = oal_ntohs(pfe_rtable_entry_get_proto(entry));
				ct6_reply->flags = 0U;
				ct6_reply->route_id = route_id;
			}
			else
			{
				memcpy(&ct_reply->saddr, &sip.v4, 4);
				memcpy(&ct_reply->daddr, &dip.v4, 4);
				ct_reply->sport = oal_htons(pfe_rtable_entry_get_sport(entry));
				ct_reply->dport = oal_htons(pfe_rtable_entry_get_dport(entry));
				ct_reply->vlan = oal_htons(pfe_rtable_entry_get_out_vlan(entry));
				memcpy(&ct_reply->saddr_reply, &ct_reply->daddr, 4);
				memcpy(&ct_reply->daddr_reply, &ct_reply->saddr, 4);
				ct_reply->sport_reply = ct_reply->dport;
				ct_reply->dport_reply = ct_reply->sport;
				ct_reply->protocol = oal_ntohs(pfe_rtable_entry_get_proto(entry));
				ct_reply->flags = 0U;
				ct_reply->route_id = route_id;
			}

			/*	Check if reply direction does exist */
			rep_entry =  pfe_rtable_entry_get_child(entry);
			if (NULL == rep_entry)
			{
				/*	This means that entry in 'reply' direction has not been requested
				 	so the appropriate flag shall be set to indicate that. */
				if (TRUE == ipv6)
				{
					ct6_reply->flags |= oal_htons(CTCMD_FLAGS_REP_DISABLED);
				}
				else
				{
					ct_reply->flags |= oal_htons(CTCMD_FLAGS_REP_DISABLED);
				}
			}
			else
			{
				/*	Associated entry for reply direction does exist */
				if (TRUE == ipv6)
				{
					ct6_reply->vlan_reply = oal_htons(pfe_rtable_entry_get_out_vlan(rep_entry));
				}
				else
				{
					ct_reply->vlan_reply = oal_htons(pfe_rtable_entry_get_out_vlan(rep_entry));
				}
			}

			/*
				Check if some modifications (NAT) are enabled. If so, update the
			 	'reply' direction values as defined by the FCI API. Note that
				modification are enabled when entry is being added. See
				FPP_ACTION_REGISTER and fci_connections_create_entry().
			*/
			actions = pfe_rtable_entry_get_action_flags(entry);
			if (EOK != pfe_rtable_entry_to_5t_out(entry, &tuple))
			{
				NXP_LOG_ERROR("Couldn't get output tuple\n");
			}

			if (0 != (actions & RT_ACT_DEC_TTL))
			{
				if (TRUE == ipv6)
				{
					ct6_reply->flags |= oal_htons(CTCMD_FLAGS_TTL_DECREMENT);
				}
				else
				{
					ct_reply->flags |= oal_htons(CTCMD_FLAGS_TTL_DECREMENT);
				}
			}

			if (0 != (actions & RT_ACT_CHANGE_SIP_ADDR))
			{
				if (TRUE == ipv6)
				{
					memcpy(ct6_reply->daddr_reply, &tuple.src_ip.v6, 16);
				}
				else
				{
					memcpy(&ct_reply->daddr_reply, &tuple.src_ip.v4, 4);
				}
			}

			if (0 != (actions & RT_ACT_CHANGE_DIP_ADDR))
			{
				if (TRUE == ipv6)
				{
					memcpy(ct6_reply->saddr_reply, &tuple.dst_ip.v6, 16);
				}
				else
				{
					memcpy(&ct_reply->saddr_reply, &tuple.dst_ip.v4, 4);
				}
			}

			if (0 != (actions & RT_ACT_CHANGE_SPORT))
			{
				if (TRUE == ipv6)
				{
					ct6_reply->dport_reply = oal_htons(tuple.sport);
				}
				else
				{
					ct_reply->dport_reply = oal_htons(tuple.sport);
				}
			}

			if (0 != (actions & RT_ACT_CHANGE_DPORT))
			{
				if (TRUE == ipv6)
				{
					ct6_reply->sport_reply = oal_htons(tuple.dport);
				}
				else
				{
					ct_reply->sport_reply = oal_htons(tuple.dport);
				}
			}

			*fci_ret = FPP_ERR_OK;
			ret = EOK;

			break;
		}

		default:
		{
			NXP_LOG_ERROR("Connection Command: Unknown action received: 0x%x\n", ct_cmd->action);
			*fci_ret = FPP_ERR_UNKNOWN_ACTION;
			break;
		}
	}
	
	return ret;
}

/**
 * @brief			Process FPP_CMD_IPV4_CONNTRACK command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_ct_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with route DB protected against concurrent accesses.
 * @note			Input values passed via fpp_ct_cmd_t are in __NETWORK__ endian format.
 */
errno_t fci_connections_ipv4_ct_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_ct_cmd_t *reply_buf, uint32_t *reply_len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return fci_connections_ipvx_ct_cmd(FALSE, msg, fci_ret, (void *)reply_buf, reply_len);
}

/**
 * @brief			Process FPP_CMD_IPV6_CONNTRACK command
 * @param[in]		msg FCI message containing the command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_ct6_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with route DB protected against concurrent accesses.
 * @note			Input values passed via fpp_ct_cmd_t are in __NETWORK__ endian format.
 */
errno_t fci_connections_ipv6_ct_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_ct6_cmd_t *reply_buf, uint32_t *reply_len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return fci_connections_ipvx_ct_cmd(TRUE, msg, fci_ret, (void *)reply_buf, reply_len);
}

/**
 * @brief			Process FPP_CMD_IPV4_SET_TIMEOUT commands
 * @param[in]		msg FCI message containing the FPP_CMD_IPV4_SET_TIMEOUT command
 * @param[out]		fci_ret FCI command return value
 * @param[out]		reply_buf Pointer to a buffer where function will construct command reply (fpp_ct_cmd_t)
 * @param[in,out]	reply_len Maximum reply buffer size on input, real reply size on output (in bytes)
 * @return			EOK if success, error code otherwise
 * @note			Function is only called within the FCI worker thread context.
 * @note			Must run with route DB protected against concurrent accesses.
 * @note			Since command and the function name refers to IPv4, all connections including IPv6 are
 * 					being updated. This is because of the legacy implementation and missing the dedicated
 * 					FPP_CMD_IPV6_SET_TIMEOUT command.
 */
errno_t fci_connections_ipv4_timeout_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_timeout_cmd_t *reply_buf, uint32_t *reply_len)
{
	fci_t *context = (fci_t *)&__context;
	fpp_timeout_cmd_t *timeout_cmd;
	pfe_rtable_entry_t *entry = NULL;
	uint8_t proto;
	uint32_t timeout;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (*reply_len < sizeof(fpp_timeout_cmd_t))
	{
		NXP_LOG_ERROR("Buffer length does not match expected value (fpp_timeout_cmd_t)\n");
		return EINVAL;
	}
	else
	{
		/*	No data written to reply buffer (yet) */
		*reply_len = 0U;
	}

	/*	Initialize the reply buffer */
	memset(reply_buf, 0, sizeof(fpp_timeout_cmd_t));

	timeout_cmd = (fpp_timeout_cmd_t *)(msg->msg_cmd.payload);

	/*	Update FCI-wide defaults applicable for new connections */
	if (EOK != fci_connections_set_default_timeout(timeout_cmd->protocol, timeout_cmd->timeout_value1))
	{
		NXP_LOG_WARNING("Can't set default timeout\n");
	}
	else
	{
		NXP_LOG_DEBUG("Default timeout for protocol %u set to %u seconds\n", (uint_t)timeout_cmd->protocol, (uint_t)timeout_cmd->timeout_value1);
	}

	/*	Update existing connections */
	entry = pfe_rtable_get_first(context->rtable, RTABLE_CRIT_ALL, NULL);
	while (NULL != entry)
	{
		proto = pfe_rtable_entry_get_proto(entry);
		timeout = fci_connections_get_default_timeout(proto);
		pfe_rtable_entry_set_timeout(entry, timeout);
		entry = pfe_rtable_get_next(context->rtable);
	}

	*fci_ret = FPP_ERR_OK;

	return EOK;
}

/**
 * @brief		Remove a single connection, inform clients
 * @param[in]	entry The routing table entry to be removed
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
errno_t fci_connections_drop_one(pfe_rtable_entry_t *entry)
{
	fci_t *context = (fci_t *)&__context;
	fpp_ct_cmd_t *ct_cmd = NULL;
	fpp_ct6_cmd_t *ct6_cmd = NULL;
	fci_msg_t msg;
	fci_core_client_t *client;
	pfe_5_tuple_t tuple;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}

    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_rtable_entry_to_5t(entry, &tuple);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Can't convert entry to 5 tuple: %d\n", ret);
		return ret;
	}

	memset(&msg, 0, sizeof(fci_msg_t));
	msg.type = FCI_MSG_CMD;

	if (TRUE == tuple.src_ip.is_ipv4)
	{
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
		/*	IPv4 */
		NXP_LOG_DEBUG("Removing IPv4 connection:\n%s\n", fci_connections_entry_to_str(entry));
#endif /* PFE_CFG_VERBOSITY_LEVEL */
		
		client = (fci_core_client_t *)pfe_rtable_entry_get_refptr(entry);
		if (NULL != client)
		{
			msg.msg_cmd.code = FPP_CMD_IPV4_CONNTRACK;
			ct_cmd = (fpp_ct_cmd_t *)msg.msg_cmd.payload;
			ct_cmd->action = FPP_ACTION_REMOVED;

			memcpy(&ct_cmd->saddr, &tuple.src_ip.v4, 4);
			memcpy(&ct_cmd->daddr, &tuple.dst_ip.v4, 4);
			ct_cmd->sport = oal_htons(tuple.sport);
			ct_cmd->dport = oal_htons(tuple.dport);
			ct_cmd->protocol = oal_htons(tuple.proto);

			if (EOK != fci_core_client_send(client, &msg, NULL))
			{
				NXP_LOG_ERROR("Could not notify FCI client\n");
			}
		}
		else
		{
			; /*	No client ID, notification not required */
		}
	}
	else
	{
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
		/*	IPv6 */
		NXP_LOG_DEBUG("Removing IPv6 connection:\n%s\n", fci_connections_entry_to_str(entry));
#endif /* PFE_CFG_VERBOSITY_LEVEL */

		client = (fci_core_client_t *)pfe_rtable_entry_get_refptr(entry);
		if (NULL != client)
		{
			msg.msg_cmd.code = FPP_CMD_IPV6_CONNTRACK;
			ct6_cmd = (fpp_ct6_cmd_t *)msg.msg_cmd.payload;
			ct6_cmd->action = FPP_ACTION_REMOVED;

			memcpy(&ct6_cmd->saddr[0], &tuple.src_ip.v6, 16);
			memcpy(&ct6_cmd->daddr[0], &tuple.dst_ip.v6, 16);
			ct6_cmd->sport = oal_htons(tuple.sport);
			ct6_cmd->dport = oal_htons(tuple.dport);
			ct6_cmd->protocol = oal_htons(tuple.proto);

			if (EOK != fci_core_client_send(client, &msg, NULL))
			{
				NXP_LOG_ERROR("Could not notify FCI client\n");
			}
		}
		else
		{
			; /*	No client ID, notification not required */
		}
	}

	/*	Remove entry from the routing table */
	ret = pfe_rtable_del_entry(context->rtable, entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Fatal: Can't remove rtable entry = memory leak\n");
	}
	else
	{
		/*	Release the entry */
		pfe_rtable_entry_free(entry);
	}

	return ret;
}

/**
 * @brief		Remove all connections, inform clients, resolve dependencies
 * @note		Function is only called within the FCI worker thread context.
 * @note		Must run with route DB protected against concurrent accesses.
 */
void fci_connections_drop_all(void)
{
	fci_t *context = (fci_t *)&__context;
	pfe_rtable_entry_t *entry = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	NXP_LOG_DEBUG("Removing all connections\n");

	entry = pfe_rtable_get_first(context->rtable, RTABLE_CRIT_ALL, NULL);
	while (NULL != entry)
	{
		ret = fci_connections_drop_one(entry);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Couldn't properly drop a connection: %d\n", ret);
		}

		entry = pfe_rtable_get_next(context->rtable);
	}
}

/**
 * @brief		Update default timeout value for connections
 * @param		ip_proto IP protocol number for which the timeout shall be set
 * @param		timeout The timeout value in seconds
 * @return		EOK if success, error code otherwise
 */
errno_t fci_connections_set_default_timeout(uint8_t ip_proto, uint32_t timeout)
{
	fci_t *context = (fci_t *)&__context;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return EPERM;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (ip_proto)
	{
		case 6U:
		{
			context->default_timeouts.timeout_tcp = timeout;
			break;
		}

		case 17U:
		{
			context->default_timeouts.timeout_udp = timeout;
			break;
		}

		default:
		{
			context->default_timeouts.timeout_other = timeout;
			break;
		}
	}

	return EOK;
}

/**
 * @brief		Get default timeout value for connections
 * @param[in] 	ip_proto IP protocol number
 * @return 		Timeout value in seconds for entries matching given protocol
 */
uint32_t fci_connections_get_default_timeout(uint8_t ip_proto)
{
	fci_t *context = (fci_t *)&__context;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (ip_proto)
	{
		case 6U:
		{
			return context->default_timeouts.timeout_tcp;
		}

		case 17U:
		{
			return context->default_timeouts.timeout_udp;
		}

		default:
		{
			return context->default_timeouts.timeout_other;
		}
	}
}

#endif /* PFE_CFG_FCI_ENABLE */
/** @}*/
