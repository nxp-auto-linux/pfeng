/* =========================================================================
 *  Copyright 2018-2023 NXP
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

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

/*	IP address conversion */

#define FCI_CONNECTIONS_CFG_MAX_STR_LEN		128U

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static void fci_connections_ipv4_cmd_to_5t(const fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv4_cmd_to_5t_rep(const fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv6_cmd_to_5t(const fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple);
static void fci_connections_ipv6_cmd_to_5t_rep(const fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple);
static pfe_rtable_entry_t *fci_connections_create_entry(const fci_rt_db_entry_t *route,
								const pfe_5_tuple_t *tuple, const pfe_5_tuple_t *tuple_rep);
static errno_t fci_connections_ipv4_cmd_to_entry(const fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv4_cmd_to_rep_entry(const fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv6_cmd_to_entry(const fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_ipv6_cmd_to_rep_entry(const fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface);
static errno_t fci_connections_entry_to_ipv4_cmd(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *rep_entry, fpp_ct_cmd_t *ct_cmd);
static errno_t fci_connections_entry_to_ipv6_cmd(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *rep_entry, fpp_ct6_cmd_t *ct6_cmd);
static void fci_connections_ipv4_cbk(pfe_rtable_entry_t *entry, pfe_rtable_cbk_event_t event);
static void fci_connections_ipv6_cbk(pfe_rtable_entry_t *entry, pfe_rtable_cbk_event_t event);
static errno_t fci_connections_ipvx_ct_cmd(bool_t ipv6, const fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len);
#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#ifdef NXP_LOG_ENABLED
static char_t * fci_connections_ipv4_cmd_to_str(fpp_ct_cmd_t *ct_cmd);
static char_t * fci_connections_ipv6_cmd_to_str(fpp_ct6_cmd_t *ct6_cmd);
static char_t * fci_connections_build_str(bool_t ipv6, uint8_t *sip, uint8_t *dip, uint16_t *sport, uint16_t *dport,
									uint8_t *sip_out, uint8_t *dip_out, uint16_t *sport_out, uint16_t *dport_out, uint8_t *proto);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#define ETH_43_PFE_START_SEC_VAR_CLEARED_8
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: fci_connections_build_str */
static char_t fci_connections_build_str_buf[FCI_CONNECTIONS_CFG_MAX_STR_LEN];

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

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
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSIP: %s --> %s\n", sip_str, sip_out_str);
	}
	else
	{
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSIP: %s\n", sip_str);
	}

	if (0 != memcmp(dip, dip_out, ip_addr_len))
	{
		/*	DIP need to be changed to DIP_OUT */
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDIP: %s --> %s\n", dip_str, dip_out_str);
	}
	else
	{
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDIP: %s\n", dip_str);
	}

	if (*sport != *sport_out)
	{
		/*	SPORT need to be changed to DPORT_REPLY */
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSPORT: %d --> %d\n", oal_ntohs(*sport), oal_ntohs(*sport_out));
	}
	else
	{
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tSPORT: %d\n", oal_ntohs(*sport));
	}

	if (*dport != *dport_out)
	{
		/*	DPORT need to be changed to SPORT_REPLY */
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDPORT: %d --> %d\n", oal_ntohs(*dport), oal_ntohs(*dport_out));
	}
	else
	{
		len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len,
							"\t\tDPORT: %d\n", oal_ntohs(*dport));
	}

	/*	Last line. Shall not contain EOL character. */
	len += snprintf(fci_connections_build_str_buf + len, FCI_CONNECTIONS_CFG_MAX_STR_LEN - len, "\t\tPROTO: %d", *proto);

	return fci_connections_build_str_buf;
}
#endif /* NXP_LOG_ENABLED */
#endif /* PFE_CFG_VERBOSITY_LEVEL */

/**
 * @brief		Convert CT command (IPv4) to 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv4_cmd_to_5t(const fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

		(void)memcpy(&tuple->src_ip.v4, &ct_cmd->saddr, 4);
		(void)memcpy(&tuple->dst_ip.v4, &ct_cmd->daddr, 4);
		tuple->src_ip.is_ipv4 = TRUE;
		tuple->dst_ip.is_ipv4 = TRUE;
		tuple->sport = oal_ntohs(ct_cmd->sport);
		tuple->dport = oal_ntohs(ct_cmd->dport);
		tuple->proto = (uint8_t)oal_ntohs(ct_cmd->protocol);
	}
}

/**
 * @brief		Convert CT command (IPv4) to reply 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv4_cmd_to_5t_rep(const fpp_ct_cmd_t *ct_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

		(void)memcpy(&tuple->src_ip.v4, &ct_cmd->saddr_reply, 4);
		(void)memcpy(&tuple->dst_ip.v4, &ct_cmd->daddr_reply, 4);
		tuple->src_ip.is_ipv4 = TRUE;
		tuple->dst_ip.is_ipv4 = TRUE;
		tuple->sport = oal_ntohs(ct_cmd->sport_reply);
		tuple->dport = oal_ntohs(ct_cmd->dport_reply);
		tuple->proto = (uint8_t)oal_ntohs(ct_cmd->protocol);
	}
}

/**
 * @brief		Convert CT command (IPv6) to 5 tuple representation
 * @param[in]	ct6_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv6_cmd_to_5t(const fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

		(void)memcpy(&tuple->src_ip.v6, &ct6_cmd->saddr[0], 16);
		(void)memcpy(&tuple->dst_ip.v6, &ct6_cmd->daddr[0], 16);
		tuple->src_ip.is_ipv4 = FALSE;
		tuple->dst_ip.is_ipv4 = FALSE;
		tuple->sport = oal_ntohs(ct6_cmd->sport);
		tuple->dport = oal_ntohs(ct6_cmd->dport);
		tuple->proto = (uint8_t)oal_ntohs(ct6_cmd->protocol);
	}
}

/**
 * @brief		Convert CT command (IPv6) to reply 5 tuple representation
 * @param[in]	ct_cmd The command
 * @param[out]	tuple Pointer to location where output shall be written
 */
static void fci_connections_ipv6_cmd_to_5t_rep(const fpp_ct6_cmd_t *ct6_cmd, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

		(void)memcpy(&tuple->src_ip.v6, &ct6_cmd->saddr_reply[0], 16);
		(void)memcpy(&tuple->dst_ip.v6, &ct6_cmd->daddr_reply[0], 16);
		tuple->src_ip.is_ipv4 = FALSE;
		tuple->dst_ip.is_ipv4 = FALSE;
		tuple->sport = oal_ntohs(ct6_cmd->sport_reply);
		tuple->dport = oal_ntohs(ct6_cmd->dport_reply);
		tuple->proto = (uint8_t)oal_ntohs(ct6_cmd->protocol);
	}
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
static pfe_rtable_entry_t *fci_connections_create_entry(const fci_rt_db_entry_t *route,
															const pfe_5_tuple_t *tuple, const pfe_5_tuple_t *tuple_rep)
{
	pfe_rtable_entry_t *new_entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == route) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		new_entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Create new entry */
		new_entry = pfe_rtable_entry_create();
		if (NULL == new_entry)
		{
			NXP_LOG_ERROR("Couldn't create routing table entry\n");
		}
		else
		{
			/*	Set 5-tuple */
			if (EOK != pfe_rtable_entry_set_5t(new_entry, tuple))
			{
				NXP_LOG_WARNING("Can't set 5 tuple\n");
				pfe_rtable_entry_free(NULL, new_entry);
				new_entry = NULL;
			}
			else
			{
				/*	Set properties */
				(void)pfe_rtable_entry_set_dstif(new_entry, route->iface);
				pfe_rtable_entry_set_timeout(new_entry, fci_connections_get_default_timeout(tuple->proto));
				/*	Set route ID (network endian) */
				pfe_rtable_entry_set_route_id(new_entry, route->id);
				/*	Set ttl decrement by default */
				pfe_rtable_entry_set_ttl_decrement(new_entry);

				/*	Change MAC addresses */
				pfe_rtable_entry_set_out_mac_addrs(new_entry, route->src_mac, route->dst_mac);

				/*	Check if SRC IP NAT is requested */
				if (0 != memcmp(&tuple->src_ip, &tuple_rep->dst_ip, sizeof(pfe_ip_addr_t)))
				{
					/*	SADDR need to be changed to DADDR_REPLY */
					if (EOK != pfe_rtable_entry_set_out_sip(new_entry, &tuple_rep->dst_ip))
					{
						NXP_LOG_WARNING("Couldn't set output SIP\n");
						pfe_rtable_entry_free(NULL, new_entry);
						new_entry = NULL;
					}
				}

				if (NULL != new_entry)
				{
					/*	Check if DST IP NAT is requested */
					if (0 != memcmp(&tuple->dst_ip, &tuple_rep->src_ip, sizeof(pfe_ip_addr_t)))
					{
						/*	DADDR need to be changed to SADDR_REPLY */
						if (EOK != pfe_rtable_entry_set_out_dip(new_entry, &tuple_rep->src_ip))
						{
							NXP_LOG_WARNING("Couldn't set output DIP\n");
							pfe_rtable_entry_free(NULL, new_entry);
							new_entry = NULL;
						}
					}

					if (NULL != new_entry)
					{
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
				}
			}
		}
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
static errno_t fci_connections_ipv4_cmd_to_entry(const fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Check if original direction is enabled */
		if (0U != (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
		{
			/*	Original direction is disabled */
			*entry = NULL;
			*iface = NULL;
			ret = EOK;
		}
		else
		{
			/*	Get route */
			route = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_BY_ID, (const void *)&ct_cmd->route_id);
			if (NULL == route)
			{
				NXP_LOG_WARNING("No such route (0x%x)\n", (uint_t)ct_cmd->route_id);
				ret = EINVAL;
			}
			else
			{
				/*	Get 5 tuples */
				fci_connections_ipv4_cmd_to_5t(ct_cmd, tuple);
				fci_connections_ipv4_cmd_to_5t_rep(ct_cmd, tuple_rep);

				/*	Create new entry for flow given by the 'tuple' */
				*entry = fci_connections_create_entry(route, tuple, tuple_rep);
				if (NULL == *entry)
				{
					NXP_LOG_WARNING("Couldn't create routing rule\n");
					ret = EINVAL;
				}
				else
				{
					/*	Set callback */
					pfe_rtable_entry_set_callback(*entry, fci_connections_ipv4_cbk, NULL);

					/*	Set vlan (if applicable) */
					if (0U != ct_cmd->vlan)
					{
						pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct_cmd->vlan), TRUE);
					}

					/*	Return interface */
					*iface = route->iface;
					ret = EOK;
				}
			}
		}
	}

	return ret;
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
static errno_t fci_connections_ipv4_cmd_to_rep_entry(const fpp_ct_cmd_t *ct_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct_cmd) || (NULL == entry) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Check if reply direction is enabled */
		if (0U != (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_REP_DISABLED))
		{
			/*	Reply direction is disabled */
			*entry = NULL;
			*iface = NULL;
			ret = EOK;
		}
		else
		{
			/*	Get route */
			route = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_BY_ID, (const void *)&ct_cmd->route_id_reply);
			if (NULL == route)
			{
				NXP_LOG_WARNING("No such route (0x%x)\n", (uint_t)oal_ntohl(ct_cmd->route_id_reply));
				ret = EINVAL;
			}
			else
			{
				/*	Get 5 tuples. Reply entries are created using 'reply' values of CT commands. */
				fci_connections_ipv4_cmd_to_5t(ct_cmd, tuple_rep);
				fci_connections_ipv4_cmd_to_5t_rep(ct_cmd, tuple);

				/*	Create new entry for flow given by the 'tuple' */
				*entry = fci_connections_create_entry(route, tuple, tuple_rep);
				if (NULL == *entry)
				{
					NXP_LOG_WARNING("Couldn't create 'reply' routing rule\n");
					ret = EINVAL;
				}
				else
				{
					/*	Set callback only if this is a lone reply entry (does not have paired orig entry) */
					if (0U != (oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
					{
						pfe_rtable_entry_set_callback(*entry, fci_connections_ipv4_cbk, NULL);
					}

					/*	Set vlan (if applicable) */
					if (0U != ct_cmd->vlan_reply)
					{
						pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct_cmd->vlan_reply), TRUE);
					}

					/*	Return interface */
					*iface = route->iface;
					ret = EOK;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Convert CT command (IPv6) to a new routing table entry
 * @param[in]	ct6_cmd The command
 * @param[out]	entry Pointer to memory where pointer to the new entry shall be written
 * @param[out]	iface Pointer where target interface instance shall be written
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_ipv6_cmd_to_entry(const fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == entry) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Check if original direction is enabled */
		if (0U != (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
		{
			/*	Original direction is disabled */
			*entry = NULL;
			ret = EOK;
		}
		else
		{
			/*	Get route */
			route = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_BY_ID, (const void *)&ct6_cmd->route_id);
			if (NULL == route)
			{
				NXP_LOG_WARNING("No such route (0x%x)\n", (uint_t)ct6_cmd->route_id);
				ret = EINVAL;
			}
			else
			{
				/*	Get 5 tuples */
				fci_connections_ipv6_cmd_to_5t(ct6_cmd, tuple);
				fci_connections_ipv6_cmd_to_5t_rep(ct6_cmd, tuple_rep);

				/*	Create new entry for flow given by the 'tuple' */
				*entry = fci_connections_create_entry(route, tuple, tuple_rep);
				if (NULL == *entry)
				{
					NXP_LOG_WARNING("Couldn't create routing rule\n");
					ret =  EINVAL;
				}
				else
				{
					/*	Set callback */
					pfe_rtable_entry_set_callback(*entry, fci_connections_ipv6_cbk, NULL);

					/*	Set vlan (if applicable) */
					if (0U != ct6_cmd->vlan)
					{
						pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct6_cmd->vlan), TRUE);
					}

					/*	Return interface */
					*iface = route->iface;
					ret = EOK;
				}
			}
		}
	}
	return ret;
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
static errno_t fci_connections_ipv6_cmd_to_rep_entry(const fpp_ct6_cmd_t *ct6_cmd, pfe_rtable_entry_t **entry, pfe_phy_if_t **iface)
{
	fci_t *fci_context = (fci_t *)&__context;
	fci_rt_db_entry_t *route;
	pfe_5_tuple_t tuple_buf, tuple_rep_buf;
	pfe_5_tuple_t *tuple = &tuple_buf, *tuple_rep = &tuple_rep_buf;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == ct6_cmd) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Check if reply direction is enabled */
		if (0U != (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_REP_DISABLED))
		{
			/*	Reply direction is disabled */
			*entry = NULL;
			*iface = NULL;
			ret = EOK;
		}
		else
		{
			/*	Get route */
			route = fci_rt_db_get_first(&fci_context->route_db, RT_DB_CRIT_BY_ID, (const void *)&ct6_cmd->route_id_reply);
			if (NULL == route)
			{
				NXP_LOG_WARNING("No such route (0x%x)\n", (uint_t)oal_ntohl(ct6_cmd->route_id_reply));
				ret = EINVAL;
			}
			else
			{
				/*	Get 5 tuples. Reply entries are created using 'reply' values of CT commands. */
				fci_connections_ipv6_cmd_to_5t(ct6_cmd, tuple_rep);
				fci_connections_ipv6_cmd_to_5t_rep(ct6_cmd, tuple);

				/*	Create new entry for flow given by the 'tuple' */
				*entry = fci_connections_create_entry(route, tuple, tuple_rep);
				if (NULL == *entry)
				{
					NXP_LOG_WARNING("Couldn't create 'reply' routing rule\n");
					ret = EINVAL;
				}
				else
				{
					/*	Set callback only if this is a lone reply entry (does not have paired orig entry) */
					if (0U != (oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_ORIG_DISABLED))
					{
						pfe_rtable_entry_set_callback(*entry, fci_connections_ipv6_cbk, NULL);
					}

					/*	Set vlan (if applicable) */
					if (0U != ct6_cmd->vlan_reply)
					{
						pfe_rtable_entry_set_out_vlan(*entry, oal_ntohs(ct6_cmd->vlan_reply), TRUE);
					}

					/*	Return interface */
					*iface = route->iface;
					ret = EOK;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Convert data of routing table entry into CT command data (IPv4).
 * @details		Handles reply direction as well (if reply dir is present).
 * @param[in]	entry		Pointer to routing table entry.
 * @param[in]	rep_entry	Pointer to routing table entry in reply direction. Can be NULL.
 * @param[out]	ct_cmd		Pointer to the command struct whch shall be filled.
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_entry_to_ipv4_cmd(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *rep_entry, fpp_ct_cmd_t *ct_cmd)
{
	errno_t ret = EINVAL;
	fci_t *fci_context = (fci_t *)&__context;
	pfe_ip_addr_t sip, dip;
	uint32_t route_id = 0U;
	pfe_5_tuple_t tuple;
	pfe_ct_route_actions_t actions;
	uint16_t vlan;
	pfe_ct_conntrack_stats_t stats = {0U};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ct_cmd)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Prepare statistics data */
		ret = pfe_rtable_get_stats(fci_context->rtable, &stats, pfe_rtable_entry_get_stats_index(entry));
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Failed to get routing entry statistics: %d", ret);
		}

		/*	Build reply structure */
		pfe_rtable_entry_get_sip(entry, &sip);
		pfe_rtable_entry_get_dip(entry, &dip);
		(void)pfe_rtable_entry_get_route_id(entry, &route_id);
		vlan = pfe_rtable_entry_get_out_vlan(entry);

		/*	Fill basic info */
		(void)memcpy(&ct_cmd->saddr, &sip.v4, 4);
		(void)memcpy(&ct_cmd->daddr, &dip.v4, 4);
		ct_cmd->sport = oal_htons(pfe_rtable_entry_get_sport(entry));
		ct_cmd->dport = oal_htons(pfe_rtable_entry_get_dport(entry));
		ct_cmd->vlan = oal_htons(vlan);
		(void)memcpy(&ct_cmd->saddr_reply, &ct_cmd->daddr, 4);
		(void)memcpy(&ct_cmd->daddr_reply, &ct_cmd->saddr, 4);
		ct_cmd->sport_reply = ct_cmd->dport;
		ct_cmd->dport_reply = ct_cmd->sport;
		ct_cmd->protocol = oal_ntohs(pfe_rtable_entry_get_proto(entry));
		ct_cmd->flags = 0U;
		ct_cmd->route_id = route_id;
		ct_cmd->stats.hit = oal_htonl(stats.hit);
		ct_cmd->stats.hit_bytes = oal_htonl(stats.hit_bytes);

		/*	Check if reply direction exists */
		if (NULL == rep_entry)
		{
			/*	This means that entry in 'reply' direction has not been requested
				so the appropriate flag shall be set to indicate that. */
			ct_cmd->flags |= oal_htons(CTCMD_FLAGS_REP_DISABLED);
		}
		else
		{
			/*	Prepare reply direction statistics data */
			ret = pfe_rtable_get_stats(fci_context->rtable, &stats, pfe_rtable_entry_get_stats_index(rep_entry));
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Failed to get reply routing entry statistics: %d", ret);
			}

			/*	Prepare reply direction vlan data */
			vlan = pfe_rtable_entry_get_out_vlan(rep_entry);
			ct_cmd->vlan_reply = oal_htons(vlan);

			/*	Prepare reply direction statistics data */
			ct_cmd->stats_reply.hit = oal_htonl(stats.hit);
			ct_cmd->stats_reply.hit_bytes = oal_htonl(stats.hit_bytes);

			/*	Prepare reply direction route id */
			pfe_rtable_entry_get_route_id(rep_entry, &route_id);
			ct_cmd->route_id_reply = route_id;
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

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_DEC_TTL))
		{
			ct_cmd->flags |= oal_htons(CTCMD_FLAGS_TTL_DECREMENT);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_SIP_ADDR))
		{
			(void)memcpy(&ct_cmd->daddr_reply, &tuple.src_ip.v4, 4);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_DIP_ADDR))
		{
			(void)memcpy(&ct_cmd->saddr_reply, &tuple.dst_ip.v4, 4);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_SPORT))
		{
			ct_cmd->dport_reply = oal_htons(tuple.sport);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_DPORT))
		{
			ct_cmd->sport_reply = oal_htons(tuple.dport);
		}
	}

	return ret;
}

/**
 * @brief		Convert data of routing table entry into CT command data (IPv6).
 * @details		Handles reply direction as well (if reply dir is present).
 * @param[in]	entry		Pointer to routing table entry.
 * @param[in]	rep_entry	Pointer to routing table entry in reply direction. Can be NULL.
 * @param[out]	ct6_cmd		Pointer to the command struct whch shall be filled.
 * @return		EOK if success, error code otherwise
 */
static errno_t fci_connections_entry_to_ipv6_cmd(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *rep_entry, fpp_ct6_cmd_t *ct6_cmd)
{
	errno_t ret = EINVAL;
	fci_t *fci_context = (fci_t *)&__context;
	pfe_ip_addr_t sip, dip;
	uint32_t route_id = 0U;
	pfe_5_tuple_t tuple;
	pfe_ct_route_actions_t actions;
	uint16_t vlan;
	pfe_ct_conntrack_stats_t stats = {0U};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ct6_cmd)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Prepare statistics data */
		ret = pfe_rtable_get_stats(fci_context->rtable, &stats, pfe_rtable_entry_get_stats_index(entry));
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Failed to get routing entry statistics: %d", ret);
		}

		/*	Build reply structure */
		pfe_rtable_entry_get_sip(entry, &sip);
		pfe_rtable_entry_get_dip(entry, &dip);
		(void)pfe_rtable_entry_get_route_id(entry, &route_id);
		vlan = pfe_rtable_entry_get_out_vlan(entry);

		/*	Fill basic info */
		(void)memcpy(ct6_cmd->saddr, &sip.v6, 16);
		(void)memcpy(ct6_cmd->daddr, &dip.v6, 16);
		ct6_cmd->sport = oal_htons(pfe_rtable_entry_get_sport(entry));
		ct6_cmd->dport = oal_htons(pfe_rtable_entry_get_dport(entry));
		ct6_cmd->vlan = oal_htons(vlan);
		(void)memcpy(ct6_cmd->saddr_reply, ct6_cmd->daddr, 16);
		(void)memcpy(ct6_cmd->daddr_reply, ct6_cmd->saddr, 16);
		ct6_cmd->sport_reply = ct6_cmd->dport;
		ct6_cmd->dport_reply = ct6_cmd->sport;
		ct6_cmd->protocol = oal_ntohs(pfe_rtable_entry_get_proto(entry));
		ct6_cmd->flags = 0U;
		ct6_cmd->route_id = route_id;
		ct6_cmd->stats.hit = oal_htonl(stats.hit);
		ct6_cmd->stats.hit_bytes = oal_htonl(stats.hit_bytes);

		/*	Check if reply direction exists */
		if (NULL == rep_entry)
		{
			/*	This means that entry in 'reply' direction has not been requested
				so the appropriate flag shall be set to indicate that. */
			ct6_cmd->flags |= oal_htons(CTCMD_FLAGS_REP_DISABLED);
		}
		else
		{
			/*	Prepare reply direction statistics data */
			ret = pfe_rtable_get_stats(fci_context->rtable, &stats, pfe_rtable_entry_get_stats_index(rep_entry));
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Failed to get reply routing entry statistics: %d", ret);
			}

			/*	Prepare reply direction vlan data */
			vlan = pfe_rtable_entry_get_out_vlan(rep_entry);
			ct6_cmd->vlan_reply = oal_htons(vlan);

			/*	Prepare reply direction statistics data */
			ct6_cmd->stats_reply.hit = oal_htonl(stats.hit);
			ct6_cmd->stats_reply.hit_bytes = oal_htonl(stats.hit_bytes);

			/*	Prepare reply direction route id */
			pfe_rtable_entry_get_route_id(rep_entry, &route_id);
			ct6_cmd->route_id_reply = route_id;
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

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_DEC_TTL))
		{
			ct6_cmd->flags |= oal_htons(CTCMD_FLAGS_TTL_DECREMENT);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_SIP_ADDR))
		{
			(void)memcpy(ct6_cmd->daddr_reply, &tuple.src_ip.v6, 16);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_DIP_ADDR))
		{
			(void)memcpy(ct6_cmd->saddr_reply, &tuple.dst_ip.v6, 16);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_SPORT))
		{
			ct6_cmd->dport_reply = oal_htons(tuple.sport);
		}

		if (0U != ((uint32_t)actions & (uint32_t)RT_ACT_CHANGE_DPORT))
		{
			ct6_cmd->sport_reply = oal_htons(tuple.dport);
		}
	}

	return ret;
}

/**
 * @brief		Callback for routing table entries (IPv4).
 * @details		Function prototype of this function must conform to
 *				routing table callback prototype. See [pfe_rtable.c].
 * @param[in]	entry	Pointer to affected routing table entry.
 * @param[out]	event	What happened with the entry.
 *
 * @warning		This callback is called from routing table mutex locked context.
 *				Do NOT call functions that lock routing table mutex in this callback.
 *				It would cause a deadlock.
 */
static void fci_connections_ipv4_cbk(pfe_rtable_entry_t *entry, pfe_rtable_cbk_event_t event)
{
	errno_t ret = -1;
	fci_msg_t msg = {0};
	fpp_ct_cmd_t *msg_payload = (fpp_ct_cmd_t *)msg.msg_cmd.payload;
	fci_core_client_t *client = NULL;
	pfe_rtable_entry_t *rep_entry = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	FCI-created routing entries use refptr to store FCI client reference */
		client = (fci_core_client_t *)pfe_rtable_entry_get_refptr(entry);
		if (NULL == client)
		{
			NXP_LOG_DEBUG("NULL refptr. This routing entry was created by a NULL FCI client.\n");
			return;
		}

		/*	prepare message general data */
		msg.type = FCI_MSG_CMD;
		msg.msg_cmd.code = FPP_CMD_IPV4_CONNTRACK_CHANGE;
		msg.msg_cmd.length = sizeof(fpp_ct_cmd_t);
		if (RTABLE_ENTRY_TIMEOUT == event)
		{
			msg_payload->action = FPP_ACTION_REMOVED;
		}
		else
		{
			NXP_LOG_WARNING("Routing entry event %d not mapped to any FCI event action\n.", event);
			return;
		}

		/*	prepare message payload data */
		rep_entry = pfe_rtable_entry_get_child(NULL, entry);  /* NULL is OK. It is assumed the rtable mutex is already locked. */
		ret = fci_connections_entry_to_ipv4_cmd(entry, rep_entry, msg_payload);
		pfe_rtable_entry_free(NULL, rep_entry);  /* NULL is OK. It is assumed the rtable mutex is already locked. */
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Can't convert entry to FCI cmd: %d\n", ret);
			return;
		}

		/*	send unsolicited FCI event message */
		ret = fci_core_client_send(client, &msg, NULL);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Could not notify FCI client.\n");
		}
	}
}

/**
 * @brief		Callback for routing table entries (IPv6).
 * @details		Function prototype of this function must conform to
 *				routing table callback prototype. See [pfe_rtable.c].
 * @param[in]	entry	Pointer to affected routing table entry.
 * @param[out]	event	What happened with the entry.
 *
 * @warning		This callback is called from routing table mutex locked context.
 *				Do NOT call functions that lock routing table mutex in this callback.
 *				It would cause a deadlock.
 */
static void fci_connections_ipv6_cbk(pfe_rtable_entry_t *entry, pfe_rtable_cbk_event_t event)
{
	errno_t ret = -1;
	fci_msg_t msg = {0};
	fpp_ct6_cmd_t *msg_payload = (fpp_ct6_cmd_t *)msg.msg_cmd.payload;
	fci_core_client_t *client = NULL;
	pfe_rtable_entry_t *rep_entry = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	FCI-created routing entries use refptr to store FCI client reference */
		client = (fci_core_client_t *)pfe_rtable_entry_get_refptr(entry);
		if (NULL == client)
		{
			NXP_LOG_DEBUG("NULL refptr. This routing entry was created by a NULL FCI client.\n");
			return;
		}

		/*	prepare message general data */
		msg.type = FCI_MSG_CMD;
		msg.msg_cmd.code = FPP_CMD_IPV6_CONNTRACK_CHANGE;
		msg.msg_cmd.length = sizeof(fpp_ct6_cmd_t);
		if (RTABLE_ENTRY_TIMEOUT == event)
		{
			msg_payload->action = FPP_ACTION_REMOVED;
		}
		else
		{
			NXP_LOG_WARNING("Routing entry event %d not mapped to any FCI event action\n.", event);
			return;
		}

		/*	prepare message payload data */
		rep_entry = pfe_rtable_entry_get_child(NULL, entry);  /* NULL is OK. It is assumed the rtable mutex is already locked. */
		ret = fci_connections_entry_to_ipv6_cmd(entry, rep_entry, msg_payload);
		pfe_rtable_entry_free(NULL, rep_entry);  /* NULL is OK. It is assumed the rtable mutex is already locked. */
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Can't convert entry to FCI cmd: %d\n", ret);
			return;
		}

		/*	send unsolicited FCI event message */
		ret = fci_core_client_send(client, &msg, NULL);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("Could not notify FCI client.\n");
		}
	}
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
static errno_t fci_connections_ipvx_ct_cmd(bool_t ipv6, const fci_msg_t *msg, uint16_t *fci_ret, void *reply_buf, uint32_t *reply_len)
{
	const fci_t *fci_context = (fci_t *)&__context;
	fpp_ct_cmd_t *ct_cmd, *ct_reply;
	fpp_ct6_cmd_t *ct6_cmd, *ct6_reply;
	errno_t ret = EOK;
	pfe_rtable_entry_t *entry = NULL, *rep_entry = NULL;
	pfe_5_tuple_t tuple;
	pfe_phy_if_t *phy_if = NULL, *phy_if_reply = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((*reply_len < sizeof(fpp_ct_cmd_t)) || (*reply_len < sizeof(fpp_ct6_cmd_t)))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_ct_cmd_t or fpp_ct6_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			if (sizeof(fpp_ct_cmd_t) > sizeof(fpp_ct6_cmd_t))
			{
				(void)memset(reply_buf, 0, sizeof(fpp_ct_cmd_t));
			}
			else
			{
				(void)memset(reply_buf, 0, sizeof(fpp_ct6_cmd_t));
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
						NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Couldn't convert command to valid entry\n");
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
						NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Couldn't convert command to valid entry (reply direction)\n");
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
						/*	Remember the issuing FCI client and the associated reply entry */
						pfe_rtable_entry_set_refptr(entry, msg->client);
						pfe_rtable_entry_set_child(entry, rep_entry);

						ret = pfe_rtable_add_entry(fci_context->rtable, entry);
						if (EEXIST == ret)
						{
							NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Entry already added\n");
							*fci_ret = FPP_ERR_RT_ENTRY_ALREADY_REGISTERED;
							if (EOK != pfe_rtable_del_entry(fci_context->rtable, entry))
							{
								NXP_LOG_WARNING("Can't remove route entry\n");
							}

							pfe_rtable_entry_free(fci_context->rtable, entry);
							entry = NULL;

							if (NULL != rep_entry)
							{
								if (EOK != pfe_rtable_del_entry(fci_context->rtable, rep_entry))
								{
									NXP_LOG_WARNING("Can't remove route entry\n");
								}

								pfe_rtable_entry_free(fci_context->rtable, rep_entry);
								rep_entry = NULL;
							}
						}
						else if (EOK != ret)
						{
							NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Can't add entry: %d\n", ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							if (EOK != pfe_rtable_del_entry(fci_context->rtable, entry))
							{
								NXP_LOG_WARNING("Can't remove route entry\n");
							}

							pfe_rtable_entry_free(fci_context->rtable, entry);
							entry = NULL;

							if (NULL != rep_entry)
							{
								if (EOK != pfe_rtable_del_entry(fci_context->rtable, rep_entry))
								{
									NXP_LOG_WARNING("Can't remove route entry\n");
								}

								pfe_rtable_entry_free(fci_context->rtable, rep_entry);
								rep_entry = NULL;
							}
						}
						else
						{
							NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry added\n");
							*fci_ret = FPP_ERR_OK;
						}
					}

					/*	Add entry also for reply direction if requested */
					if (NULL != rep_entry)
					{
						/*	Remember the issuing FCI client */
						pfe_rtable_entry_set_refptr(rep_entry, msg->client);

						ret = pfe_rtable_add_entry(fci_context->rtable, rep_entry);
						if (EEXIST == ret)
						{
							NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Reply entry already added\n");
						}
						else if (EOK != ret)
						{
							NXP_LOG_WARNING("FPP_CMD_IPVx_CONNTRACK: Can't add reply entry: %d\n", ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							if (NULL != entry)
							{
								if (EOK != pfe_rtable_del_entry(fci_context->rtable, entry))
								{
									NXP_LOG_WARNING("Can't remove route entry\n");
								}

								pfe_rtable_entry_free(fci_context->rtable, entry);
								entry = NULL;
							}

							if (EOK != pfe_rtable_del_entry(fci_context->rtable, rep_entry))
							{
								NXP_LOG_WARNING("Can't remove route entry\n");
							}

							pfe_rtable_entry_free(fci_context->rtable, rep_entry);
							rep_entry = NULL;
						}
						else
						{
							NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry added (reply direction)\n");
							*fci_ret = FPP_ERR_OK;
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

					entry = pfe_rtable_get_first(fci_context->rtable, RTABLE_CRIT_BY_5_TUPLE, (void *)&tuple);

					/*	Delete the entries from table */
					if (NULL != entry)
					{
						/*	Get associated entry */
						rep_entry = pfe_rtable_entry_get_child(fci_context->rtable, entry);

						ret = pfe_rtable_del_entry(fci_context->rtable, entry);
						if (EOK != ret)
						{
							/*	Notify rtable module we are done working with this rtable entry */
							pfe_rtable_entry_free(fci_context->rtable, entry);
							entry = NULL;

							NXP_LOG_WARNING("Can't remove route entry: %d\n", ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break;
						}
						else
						{
							/*	Release all entry-related resources */
							NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry removed\n");
							pfe_rtable_entry_free(fci_context->rtable, entry);
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
						ret = pfe_rtable_del_entry(fci_context->rtable, rep_entry);
						if (EOK != ret)
						{
							/*	Notify rtable module we are done working with this rtable entry */
							pfe_rtable_entry_free(fci_context->rtable, rep_entry);
							rep_entry = NULL;

							NXP_LOG_WARNING("Can't remove reply route entry: %d\n", ret);
							*fci_ret = FPP_ERR_WRONG_COMMAND_PARAM;
							break;
						}
						else
						{
							/*	Release all entry-related resources */
							NXP_LOG_DEBUG("FPP_CMD_IPVx_CONNTRACK: Entry removed (reply direction)\n");
							pfe_rtable_entry_free(fci_context->rtable, rep_entry);
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

					/*	Get entry by 5-tuple */
					if (TRUE == ipv6)
					{
						fci_connections_ipv6_cmd_to_5t(ct6_cmd, &tuple);
					}
					else
					{
						fci_connections_ipv4_cmd_to_5t(ct_cmd, &tuple);
					}

					entry = pfe_rtable_get_first(fci_context->rtable, RTABLE_CRIT_BY_5_TUPLE, (void *)&tuple);

					if (NULL != entry)
					{

						if (TRUE == ipv6)
						{
							if ((oal_ntohs(ct6_cmd->flags) & CTCMD_FLAGS_TTL_DECREMENT) != 0U)
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
							if ((oal_ntohs(ct_cmd->flags) & CTCMD_FLAGS_TTL_DECREMENT) != 0U)
							{
								pfe_rtable_entry_set_ttl_decrement(entry);
							}
							else
							{
								pfe_rtable_entry_remove_ttl_decrement(entry);
							}
						}

						/*	Notify rtable module we are done working with this rtable entry */
						pfe_rtable_entry_free(fci_context->rtable, entry);
						entry = NULL;

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

					entry = pfe_rtable_get_first(fci_context->rtable, crit, NULL);
					if (NULL == entry)
					{
						ret = EOK;
						*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
						break;
					}
					fallthrough;
				}

				case FPP_ACTION_QUERY_CONT:
				{
					if (NULL == entry)
					{
						entry = pfe_rtable_get_next(fci_context->rtable);
						if (NULL == entry)
						{
							ret = EOK;
							*fci_ret = FPP_ERR_CT_ENTRY_NOT_FOUND;
							break;
						}
					}

					/* Get partner of the entry. */
					rep_entry = pfe_rtable_entry_get_child(fci_context->rtable, entry);

					ct6_reply = (fpp_ct6_cmd_t *)(reply_buf);
					ct_reply = (fpp_ct_cmd_t *)(reply_buf);

					/*	Set the reply length + reply data */
					if (TRUE == ipv6)
					{
						*reply_len = sizeof(fpp_ct6_cmd_t);
						ret = fci_connections_entry_to_ipv6_cmd(entry, rep_entry, ct6_reply);
					}
					else
					{
						*reply_len = sizeof(fpp_ct_cmd_t);
						ret = fci_connections_entry_to_ipv4_cmd(entry, rep_entry, ct_reply);
					}

					/*	Notify rtable module we are done working with this rtable entry */
					pfe_rtable_entry_free(fci_context->rtable, entry);
					pfe_rtable_entry_free(fci_context->rtable, rep_entry);
					entry = NULL;
					rep_entry = NULL;

					*fci_ret = FPP_ERR_OK;
					ret = EOK;

					break;
				}

				default:
				{
					NXP_LOG_WARNING("Connection Command: Unknown action received: 0x%x\n", ct_cmd->action);
					*fci_ret = FPP_ERR_UNKNOWN_ACTION;
					break;
				}
			}
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
errno_t fci_connections_ipv4_ct_cmd(const fci_msg_t *msg, uint16_t *fci_ret, fpp_ct_cmd_t *reply_buf, uint32_t *reply_len)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = fci_connections_ipvx_ct_cmd(FALSE, msg, fci_ret, (void *)reply_buf, reply_len);
	}
	return ret;
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
errno_t fci_connections_ipv6_ct_cmd(const fci_msg_t *msg, uint16_t *fci_ret, fpp_ct6_cmd_t *reply_buf, uint32_t *reply_len)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = fci_connections_ipvx_ct_cmd(TRUE, msg, fci_ret, (void *)reply_buf, reply_len);
	}
	return ret;
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
	const fci_t *fci_context = (fci_t *)&__context;
	fpp_timeout_cmd_t *timeout_cmd;
	pfe_rtable_entry_t *entry = NULL;
	uint8_t proto;
	uint32_t timeout;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (*reply_len < sizeof(fpp_timeout_cmd_t))
		{
			NXP_LOG_WARNING("Buffer length does not match expected value (fpp_timeout_cmd_t)\n");
			ret = EINVAL;
		}
		else
		{
			/*	No data written to reply buffer (yet) */
			*reply_len = 0U;
			/*	Initialize the reply buffer */
			(void)memset(reply_buf, 0, sizeof(fpp_timeout_cmd_t));

			timeout_cmd = (fpp_timeout_cmd_t *)(msg->msg_cmd.payload);

			/*	Update FCI-wide defaults applicable for new connections */
			if (EOK != fci_connections_set_default_timeout((uint8_t)oal_ntohs(timeout_cmd->protocol), oal_ntohl(timeout_cmd->timeout_value1)))
			{
				NXP_LOG_WARNING("Can't set default timeout\n");
			}
			else
			{
				NXP_LOG_DEBUG("Default timeout for protocol %u set to %u seconds\n", (uint_t)oal_ntohs(timeout_cmd->protocol), (uint_t)oal_ntohl(timeout_cmd->timeout_value1));
			}

			/*	Update existing connections */
			entry = pfe_rtable_get_first(fci_context->rtable, RTABLE_CRIT_ALL, NULL);
			while (NULL != entry)
			{
				proto = pfe_rtable_entry_get_proto(entry);
				timeout = fci_connections_get_default_timeout(proto);
				pfe_rtable_entry_set_timeout(entry, timeout);

				/*	Notify rtable module we are done working with this rtable entry */
				pfe_rtable_entry_free(fci_context->rtable, entry);

				entry = pfe_rtable_get_next(fci_context->rtable);
			}

			*fci_ret = FPP_ERR_OK;
			ret = EOK;
		}
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
	const fci_t *fci_context = (fci_t *)&__context;
	pfe_rtable_entry_t *entry = NULL;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_ERROR("Context not initialized\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		NXP_LOG_DEBUG("Removing all connections\n");

		entry = pfe_rtable_get_first(fci_context->rtable, RTABLE_CRIT_ALL, NULL);
		while (NULL != entry)
		{
			ret = pfe_rtable_del_entry(fci_context->rtable, entry);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("Couldn't properly drop a connection: %d\n", ret);
			}

			/*	Release the entry */
			pfe_rtable_entry_free(fci_context->rtable, entry);

			entry = pfe_rtable_get_next(fci_context->rtable);
		}
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
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (ip_proto)
		{
			case 6U:
			{
				fci_context->default_timeouts.timeout_tcp = timeout;
				break;
			}

			case 17U:
			{
				fci_context->default_timeouts.timeout_udp = timeout;
				break;
			}

			default:
			{
				fci_context->default_timeouts.timeout_other = timeout;
				break;
			}
		}
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get default timeout value for connections
 * @param[in] 	ip_proto IP protocol number
 * @return 		Timeout value in seconds for entries matching given protocol
 */
uint32_t fci_connections_get_default_timeout(uint8_t ip_proto)
{
	const fci_t *fci_context = (fci_t *)&__context;
	uint32_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (ip_proto)
		{
			case 6U:
			{
				ret = fci_context->default_timeouts.timeout_tcp;
				break;
			}

			case 17U:
			{
				ret = fci_context->default_timeouts.timeout_udp;
				break;
			}

			default:
			{
				ret = fci_context->default_timeouts.timeout_other;
				break;
			}
		}
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */

/** @}*/
