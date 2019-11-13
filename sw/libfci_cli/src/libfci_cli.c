/* =========================================================================
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#if defined(TARGET_OS_LINUX)
#ifndef EOK
#define EOK 0
#endif
typedef int errno_t;
#endif

#define MAX_IP_ADDR_STR_LEN (32+7+1)

typedef enum
{
	CMD_IPV4_RESET,
	CMD_IPV6_RESET,
	CMD_IP_ROUTE_ADD,
	CMD_IP_ROUTE_DEL,
	CMD_IP_ROUTE_PRINT_ALL,
	CMD_IPV4_ADD_CONNTRACK,
	CMD_IPV4_DEL_CONNTRACK,
	CMD_IPV6_ADD_CONNTRACK,
	CMD_IPV6_DEL_CONNTRACK,
	CMD_CONNTRACK_PRINT_ALL,
	CMD_L2BRIDGE_DOMAIN_ADD,
	CMD_L2BRIDGE_DOMAIN_DEL,
	CMD_L2BRIDGE_DOMAIN_ADD_IF,
	CMD_L2BRIDGE_DOMAIN_DEL_IF,
	CMD_L2BRIDGE_DOMAIN_SET_ACT,
	CMD_L2BRIDGE_DOMAIN_PRINT_ALL,
    CMD_FP_ADD_RULE,
    CMD_FP_ADD_TABLE,
    CMD_FP_DEL_RULE,
    CMD_FP_DEL_TABLE,
    CMD_FP_SET_TABLE_RULE,
    CMD_FP_UNSET_TABLE_RULE,
    CMD_FP_PRINT_TABLE,
    CMD_FP_PRINT_RULES,
	CMD_FP_FLEXIBLE_PARSER,
	CMD_IF_PRINT_ALL,
	CMD_LOG_IF_UPDATE,
	CMD_INVALID
} command_t;

typedef union fci_cmd_buf
{
	fpp_rt_cmd_t rt_cmd;
	fpp_ct_cmd_t ct_cmd;
	fpp_ct6_cmd_t ct6_cmd;
	fpp_l2_bridge_enable_cmd_t br_add_port;
	fpp_l2_bridge_add_entry_cmd_t br_add_entry;
	fpp_l2_bridge_remove_entry_cmd_t br_remove_entry;
	fpp_l2_bridge_control_cmd_t br_mode_timeout;
	fpp_l2_bridge_domain_control_cmd_t br_domain_ctrl;
	fpp_phy_if_cmd_t phy_if_cmd;
	fpp_log_if_cmd_t log_if_cmd;
	struct
	{
		bool add; /* 1-Add, 0-Del */
		uint16_t vlan; /* Vlan in network endian */
		bool tag; /* 1-tagged, 0-untagged */
		char ifname[IFNAMSIZ]; /* Name of the interface */
	} br_domain_if_ctrl;
   fpp_flexible_parser_rule_cmd fp_rule_cmd;
   fpp_flexible_parser_table_cmd fp_table_cmd;
   fpp_flexible_filter_cmd fp_filter_cmd;
} fci_cmd_t;


/* Number to flags MAP */
static char *flags[32U] =		{"ETH", "VLAN", "PPPOE","ARP", "MCAST", "IP",
								"IPV6",	"IPV4", "","IPX", "BCAST", "UDP",
								"TCP","ICMP","IGMP", "VLAN_ID", "IP_PROTO", "",
								"",	"","SPORT","DPORT","SIP6","DIP6",
								"SIP","DIP","ETHER_TYPE","FP0","FP1","SMAC",
								"DMAC",""};


/**
 * @brief		Build string from given values
 * @param[in]	ipv6 true if IPv6 values, FALSE for IPv4
 * @param[in]	sip SIP
 * @param[in]	dip DIP
 * @param[in]	sport SPORT
 * @param[in]	dport DPORT
 * @param[in]	sip_out Output SIP
 * @param[in]	dip_out Output DIP
 * @param[in]	sport_out Output SPORT
 * @param[in]	dport_out Output DPORT
 * @param[in]	proto Protocol
 * @param[in]	route_id Route ID
 * @return		Pointer to buffer with the output string
 */
static void print_conntrack(bool ipv6, uint8_t *sip, uint8_t *dip, uint16_t *sport, uint16_t *dport,
								uint8_t *sip_out, uint8_t *dip_out, uint16_t *sport_out, uint16_t *dport_out, uint8_t *proto)
{
	uint32_t ipv_flag = (true == ipv6) ? AF_INET6 : AF_INET;
	uint8_t ip_addr_len = (true == ipv6) ? 16U : 4U;
	char sip_str[MAX_IP_ADDR_STR_LEN];
	char sip_out_str[MAX_IP_ADDR_STR_LEN];
	char dip_str[MAX_IP_ADDR_STR_LEN];
	char dip_out_str[MAX_IP_ADDR_STR_LEN];

	inet_ntop(ipv_flag, sip, sip_str, sizeof(sip_str));
	inet_ntop(ipv_flag, dip, dip_str, sizeof(dip_str));
	inet_ntop(ipv_flag, sip_out, sip_out_str, sizeof(sip_out_str));
	inet_ntop(ipv_flag, dip_out, dip_out_str, sizeof(dip_out_str));

	if (ipv6)
	{
		printf("IPv6 Connection\n");
	}
	else
	{
		printf("IPv4 Connection\n");
	}

	if (0 != memcmp(sip, sip_out, ip_addr_len))
	{
		/*	SIP need to be changed to SIP_OUT */
		printf("SIP: %s --> %s\n", sip_str, sip_out_str);
	}
	else
	{
		printf("SIP: %s\n", sip_str);
	}

	if (0 != memcmp(dip, dip_out, ip_addr_len))
	{
		/*	DIP need to be changed to DIP_OUT */
		printf("DIP: %s --> %s\n", dip_str, dip_out_str);
	}
	else
	{
		printf("DIP: %s\n", dip_str);
	}

	if (*sport != *sport_out)
	{
		/*	SPORT need to be changed to DPORT_REPLY */
		printf("SPORT: %d --> %d\n", ntohs(*sport), ntohs(*sport_out));
	}
	else
	{
		printf("SPORT: %d\n", ntohs(*sport));
	}

	if (*dport != *dport_out)
	{
		/*	DPORT need to be changed to SPORT_REPLY */
		printf("DPORT: %d --> %d\n", ntohs(*dport), ntohs(*dport_out));
	}
	else
	{
		printf("DPORT: %d\n", ntohs(*dport));
	}

	/*	Last line. Shall not contain EOL character. */
	printf("PROTO: %d\n\n", *proto);
}

static void print_route(bool ipv6, uint32_t *id, uint8_t *mac_addr, uint8_t *ip_addr, char *ifname)
{
	uint32_t ipv_flag = (true == ipv6) ? AF_INET6 : AF_INET;
	char ip_str[MAX_IP_ADDR_STR_LEN];

	inet_ntop(ipv_flag, ip_addr, ip_str, sizeof(ip_str));


	printf("ID: %08d MAC: %02x:%02x:%02x:%02x:%02x:%02x IP: %s\tIF: %s\n",
			*id,
			mac_addr[0],
			mac_addr[1],
			mac_addr[2],
			mac_addr[3],
			mac_addr[4],
			mac_addr[5],
			ip_str,
			ifname);
}

/**
 * @brief		Parse input arguments and generate FCI command
 * @param[in]	argc Count
 * @param[in]	argv Values
 * @param[out]	fci_cmd_buf Pointer to memory where constructed FCI command shall be written
 * @return		Command ID or CMD_INVALID if failed
 */
command_t arg_parser(int argc, char *argv[], fci_cmd_t *fci_cmd)
{
	struct cmdline_args
	{
		uint8_t src_mac[6];
		bool src_mac_valid;
		uint8_t dst_mac[6];
		bool dst_mac_valid;
		uint8_t sip[16];
		bool sip_valid;
		bool sip_is_ipv6;
		uint8_t dip[16];
		bool dip_valid;
		bool dip_is_ipv6;
		uint16_t sport;
		bool sport_valid;
		uint16_t dport;
		bool dport_valid;
		uint8_t sip_out[16];
		bool sip_out_valid;
		uint8_t dip_out[16];
		bool dip_out_valid;
		uint16_t sport_out;
		bool sport_out_valid;
		uint16_t dport_out;
		bool dport_out_valid;
		uint8_t proto;
		bool proto_valid;
		char ifname[IFNAMSIZ+1];
		bool ifname_valid;
		uint32_t route_id;
		bool route_id_valid;
		uint16_t vlan;
		bool vlan_valid;
		bool tag_flag;
		bool tag_flag_valid;
		uint8_t ucast_hit, ucast_miss, mcast_hit, mcast_miss;
		bool uhit_valid, umiss_valid, mhit_valid, mmiss_valid;
        bool invert, reject, accept, nr;
        char rule_name[16];
        char table_name[16];
        char next_rule[16];
        uint32_t data, mask;
        uint16_t offset;
        uint8_t layer;
        uint16_t position;
        bool disable;
        uint32_t match_rules;
		bool match_rules_valid;
		uint32_t egress;
		bool egress_valid;
		bool enable;
		bool enable_valid;
		bool promisc;
		bool promisc_valid;
		bool match;
		bool match_valid;
		uint16_t eth;
		bool eth_valid;
	} args;

	int i;
	command_t icmd = CMD_INVALID;

	const char help[] = "Usage:\n"
					"\tlibfci_cli --reset\n"
					"\tlibfci_cli --reset6\n"
					"\tlibfci_cli --add_route --mac 00:13:3B:9c:08:c5 --dip 192.168.1.83 --i emac0 --routeid 6 \n"
					"\tlibfci_cli --del_route --routeid 6 \n"
					"\tlibfci_cli --add_conntrack --sip 192.168.1.83 --dip 192.168.2.20 --proto 6 --sport 10 --dport 20 --routeid 6 \n"
					"\tlibfci_cli --add_conntrack --sip s1.s1.s1.s1 --sip_out s2.s2.s2.s2 --dip d1.d1.d1.d1 --dip_out d2.d2.d2.d2 \n"
					"                                 --proto 6 --sport 10 --sport_out 11 --dport 20 --dport_out 21 --routeid 7 \n"
					"\tlibfci_cli --del_conntrack --sip 192.168.1.83 --dip 192.168.2.20 --proto 6 --sport 20 --dport 20 \n"
					"\tlibfci_cli --add_conntrack6 --sip dead:: --dip beef:: --proto 6 --sport 10 --dport 20 --routeid 8 \n"
					"\tlibfci_cli --del_conntrack6 --sip dead:: --dip beef:: --proto 6 --sport 10 --dport 20 \n"
					"\tlibfci_cli --print_routes\n"
					"\tlibfci_cli --print_conntracks\n"
					"\tlibfci_cli --bd_add --vlan 123 --ucast_hit 0 --ucast_miss 1 --mcast_hit 2 --mcast_miss 3\n"
					"\tlibfci_cli --bd_del --vlan 123\n"
					"\tlibfci_cli --bd_set_act --vlan 123 --ucast_hit 0 --ucast_miss 1 --mcast_hit 2 --mcast_miss 3\n"
					"\tlibfci_cli --bd_add_if --vlan 123 --i pfe0 --tag 1\n"
					"\tlibfci_cli --bd_del_if --vlan 123 --i pfe0\n"
					"\tlibfci_cli --print_bds\n"
					"\tlibfci_cli --print_ifs\n"
					"\tlibfci_cli --if_update --i pfe0 --rule VLAN,ARP --vlan 123 --egress 5,6,7 --enable on --promisc off --match and\n"
					"\tlibfci_cli --fp_add_rule --ruleid <rulename> --data 0x08000000 --mask 0xFFFF0000 --layer 2 --offset 14 --invert --accept\n"
                    "\t\t--accept or --reject or --next_rule <rulename> are mutually exclusive, one of them is required\n"
                    "\tlibfci_cli --fp_add_table --tableid <tablename>\n"
                    "\tlibfci_cli --fp_set_table_rule --tableid <tablename> --ruleid <rulename> --position 256\n"
                    "\tlibfci_cli --fp_print_table --tableid <tablename>\n"
                    "\tlibfci_cli --fp_print_rules\n"
                    "\tlibfci_cli --fp_flexible_filter --tableid <tablename>\n\t\t--disable is mutually exclusive with --tableid <tablename>\n"
                    "\tlibfci_cli --fp_delete_rule --ruleid <rulename>\n"
                    "\tlibfci_cli --fp_delete_table --tableid <rulename>\n"
                    "\tlibfci_cli --fp_unset_table_rule --ruleid <rulename>\n"

	;

	const struct option long_opts[] =
	{
		{"reset", no_argument, (int *)&icmd, CMD_IPV4_RESET},
		{"reset6", no_argument, (int *)&icmd, CMD_IPV6_RESET},
		{"add_route", no_argument, (int *)&icmd, CMD_IP_ROUTE_ADD},
		{"del_route", no_argument, (int *)&icmd, CMD_IP_ROUTE_DEL},
		{"print_routes", no_argument, (int *)&icmd, CMD_IP_ROUTE_PRINT_ALL},
		{"add_conntrack", no_argument, (int *)&icmd, CMD_IPV4_ADD_CONNTRACK},
		{"del_conntrack", no_argument, (int *)&icmd, CMD_IPV4_DEL_CONNTRACK},
		{"add_conntrack6", no_argument, (int *)&icmd, CMD_IPV6_ADD_CONNTRACK},
		{"del_conntrack6", no_argument, (int *)&icmd, CMD_IPV6_DEL_CONNTRACK},
		{"print_conntracks", no_argument, (int *)&icmd, CMD_CONNTRACK_PRINT_ALL},
		{"bd_add", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_ADD},
		{"bd_del", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_DEL},
		{"bd_set_act", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_SET_ACT},
		{"bd_add_if", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_ADD_IF},
		{"bd_del_if", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_DEL_IF},
		{"print_bds", no_argument, (int *)&icmd, CMD_L2BRIDGE_DOMAIN_PRINT_ALL},
        {"fp_add_rule", no_argument, (int *)&icmd, CMD_FP_ADD_RULE},
        {"fp_add_table", no_argument, (int *)&icmd, CMD_FP_ADD_TABLE},
        {"fp_set_table_rule", no_argument, (int *)&icmd, CMD_FP_SET_TABLE_RULE},
        {"fp_unset_table_rule", no_argument, (int *)&icmd, CMD_FP_UNSET_TABLE_RULE},
        {"fp_print_table", no_argument, (int *)&icmd, CMD_FP_PRINT_TABLE},
        {"fp_print_rules", no_argument, (int *)&icmd, CMD_FP_PRINT_RULES},
        {"fp_flexible_filter", no_argument, (int *)&icmd, CMD_FP_FLEXIBLE_PARSER},
        {"fp_delete_rule", no_argument, (int *)&icmd, CMD_FP_DEL_RULE},
        {"fp_delete_table", no_argument, (int *)&icmd, CMD_FP_DEL_TABLE},
		{"print_ifs", no_argument, (int *)&icmd, CMD_IF_PRINT_ALL},
		{"if_update", no_argument, (int *)&icmd, CMD_LOG_IF_UPDATE},

		{"mac", required_argument, 0, 'm'},
		{"dmac", required_argument, 0, 'm'},
		{"smac", required_argument, 0, 'u'},
		{"sip", required_argument, 0, 'a'},
		{"srcip", required_argument, 0, 'a'},
		{"dip", required_argument, 0, 'b'},
		{"dstip", required_argument, 0, 'b'},
		{"sip_out", required_argument, 0, 'c'},
		{"srcip_out", required_argument, 0, 'c'},
		{"dip_out", required_argument, 0, 'd'},
		{"dstip_out", required_argument, 0, 'd'},
		{"sport", required_argument, 0, 'e'},
		{"srcport", required_argument, 0, 'e'},
		{"dport", required_argument, 0, 'f'},
		{"dstport", required_argument, 0, 'f'},
		{"sport_out", required_argument, 0, 'g'},
		{"srcport_out", required_argument, 0, 'g'},
		{"dport_out", required_argument, 0, 'h'},
		{"dstport_out", required_argument, 0, 'h'},
		{"vlan", required_argument, 0, 'j'},
		{"ucast_hit", required_argument, 0, 'k'},
		{"ucast_miss", required_argument, 0, 'l'},
		{"mcast_hit", required_argument, 0, 'o'},
		{"mcast_miss", required_argument, 0, 'p'},
		{"tag", required_argument, 0, 'q'},
		{"proto", required_argument, 0, 'z'},
		{"routeid", required_argument, 0, 'r'},
		{"rule", required_argument, 0, 'x'},
		{"egress", required_argument, 0, 'y'},
		{"enable", required_argument, 0, 't'},
		{"promisc", required_argument, 0, 'n'},
		{"match", required_argument, 0, 's'},
		{"eth", required_argument, 0, 'w'},
		{"i", required_argument, 0, 'i'},
		{"rule", required_argument, 0, 'x'},
		{"egress", required_argument, 0, 'y'},
		{"enable", required_argument, 0, 't'},
		{"promisc", required_argument, 0, 'n'},
		{"match", required_argument, 0, 's'},
		{"eth", required_argument, 0, 'w'},
        {"data", required_argument,0 ,'D'},
        {"mask", required_argument,0 ,'M'},
        {"offset", required_argument, 0 ,'O'},
        {"accept", no_argument, 0 ,'A'},
        {"reject", no_argument, 0 ,'R'},
        {"invert", no_argument, 0 ,'I'},
        {"next_rule", required_argument,0 ,'N'},
        {"tableid", required_argument,0 ,'T'},
        {"ruleid", required_argument,0 ,'U'},
        {"layer", required_argument,0 ,'L'},
        {"disable", no_argument,0 ,'E'},
        {"position", required_argument,0 ,'P'},
		{0, 0, 0, 0}
	};

	int iopt;
	int vopt;
	char *cpart;

	if (argc < 2)
	{
		printf(help);
		return CMD_INVALID;
	}

	memset(&args, 0, sizeof(struct cmdline_args));

	while ((vopt = getopt_long(argc, argv, "m:a:b:c:d:e:f:g:h:z:r:i:", long_opts, &iopt)) != -1)
	{
		switch (vopt)
		{
			case 'm':
			{
				/*	MAC address  */
				if (optarg)
				{
					cpart = strtok( optarg, ":");
					for (i=0; cpart != NULL && i < 6; i++)
					{
						args.dst_mac[i] = strtol(cpart, NULL, 16);
						cpart = strtok(NULL, ":");
					}

					if (i < 6)
					{
						printf("Invalid MAC address\n");
					}
					else
					{
						args.dst_mac_valid = true;
					}
				}

				break;
			}

			case 'u':
			{
				/*	src MAC address  */
				if (optarg)
				{
					cpart = strtok( optarg, ":");
					for (i=0; cpart != NULL && i < 6; i++)
					{
						args.src_mac[i] = strtol(cpart, NULL, 16);
						cpart = strtok(NULL, ":");
					}

					if (i < 6)
					{
						printf("Invalid MAC address\n");
					}
					else
					{
						args.src_mac_valid = true;
					}
				}

				break;
			}

			case 'a':
			{
				/*	SIP */
				if (1 == inet_pton(AF_INET, optarg, args.sip))
				{
					/*	IPv4 */
					args.sip_valid = true;
					args.sip_is_ipv6 = false;
				}
				else if (1 == inet_pton(AF_INET6, optarg, args.sip))
				{
					/*	IPv6 */
					args.sip_valid = true;
					args.sip_is_ipv6 = true;
				}
				else
				{
					printf("Invalid SIP address: %s\n", optarg);
				}

				break;
			}

			case 'b':
			{
				/*	DIP */
				if (1 == inet_pton(AF_INET, optarg, args.dip))
				{
					/*	IPv4 */
					args.dip_valid = true;
					args.dip_is_ipv6 = false;
				}
				else if (1 == inet_pton(AF_INET6, optarg, args.dip))
				{
					/*	IPv6 */
					args.dip_valid = true;
					args.dip_is_ipv6 = true;
				}
				else
				{
					printf("Invalid DIP address: %s\n", optarg);
				}

				break;
			}

			case 'c':
			{
				/*	SIP_OUT */
				if (1 == inet_pton(AF_INET, optarg, args.sip_out))
				{
					/*	IPv4 */
					args.sip_out_valid = true;
				}
				else if (1 == inet_pton(AF_INET6, optarg, args.sip_out))
				{
					/*	IPv6 */
					args.sip_out_valid = true;
				}
				else
				{
					printf("Invalid SIP_OUT address: %s\n", optarg);
				}

				break;
			}

			case 'd':
			{
				/*	DIP_OUT */
				if (1 == inet_pton(AF_INET, optarg, args.dip_out))
				{
					/*	IPv4 */
					args.dip_out_valid = true;
				}
				else if (1 == inet_pton(AF_INET6, optarg, args.dip_out))
				{
					/*	IPv6 */
					args.dip_out_valid = true;
				}
				else
				{
					printf("Invalid DIP_OUT address: %s\n", optarg);
				}

				break;
			}

			case 'w':
			{
				/*	ETH_type */
				args.eth = htons(strtol(optarg, NULL, 10));
				args.eth_valid = true;
				break;
			}

			case 'e':
			{
				/*	SPORT */
				args.sport = htons(strtol(optarg, NULL, 10));
				args.sport_valid = true;
				break;
			}

			case 'f':
			{
				/*	DPORT */
				args.dport = htons(strtol(optarg, NULL, 10));
				args.dport_valid = true;
				break;
			}

			case 'g':
			{
				/*	SPORT_OUT */
				args.sport_out = htons(strtol(optarg, NULL, 10));
				args.sport_out_valid = true;
				break;
			}

			case 'h':
			{
				/*	DPORT_OUT */
				args.dport_out = htons(strtol(optarg, NULL, 10));
				args.dport_out_valid = true;
				break;
			}

			case 'j':
			{
				/*	VLAN */
				args.vlan = htons(strtol(optarg, NULL, 10));
				args.vlan_valid = true;
				break;
			}

			case 'k':
			{
				/*	UCAST HIT */
				args.ucast_hit = strtol(optarg, NULL, 10);
				args.uhit_valid = true;
				break;
			}

			case 'l':
			{
				/*	UCAST MISS */
				args.ucast_miss = strtol(optarg, NULL, 10);
				args.umiss_valid = true;
				break;
			}

			case 'o':
			{
				/*	MCAST HIT */
				args.mcast_hit = strtol(optarg, NULL, 10);
				args.mhit_valid = true;
				break;
			}

			case 'p':
			{
				/*	MCAST MISS */
				args.mcast_miss = strtol(optarg, NULL, 10);
				args.mmiss_valid = true;
				break;
			}

			case 'q':
			{
				/*	TAG FLAG */
				args.tag_flag = (0 != strtol(optarg, NULL, 10));
				args.tag_flag_valid = true;
				break;
			}

			case 'z':
			{
				/*	PROTO */
				args.proto = (uint8_t)strtol(optarg, NULL, 10);
				args.proto_valid = true;
				break;
			}

			case 'x':
			{
				/*	Match flags  */
				if (optarg)
				{
					args.match_rules = 0U;
					args.match_rules_valid = true;

					cpart = strtok( optarg, ",");
					while(cpart != NULL)
					{
						if(0 == strcmp("", cpart))
						{
							args.match_rules_valid = false;
							break;
						}

						for (i=0; i < 8 * sizeof(fpp_if_m_rules_t); i++)
						{
							if(0 == strcmp(flags[i], cpart))
							{
								args.match_rules |= (1U << i);
							}
						}
						cpart = strtok(NULL, ",");
					}

				}

				break;
			}

			case 'y':
			{
				/*	Match flags  */
				if (optarg)
				{
					args.egress = 0U;
					args.egress_valid = true;

					cpart = strtok( optarg, ",");
					while(cpart != NULL)
					{
						if(32 <= atoi(cpart) || 0 > atoi(cpart))
						{
							args.egress_valid = false;
							break;
						}
						args.egress |= (1U << atoi(cpart));
						cpart = strtok(NULL, ",");
					}

				}

				break;
			}

			case 't':
			{
				if (optarg)
				{
					args.enable_valid = true;
					if(0 == strcmp(optarg, "on"))
					{
						args.enable = true;
					}
					else if(0 == strcmp(optarg, "off"))
					{
						args.enable = false;
					}
					else
					{
						args.enable_valid = false;
					}

				}
				break;
			}

			case 'n':
			{
				if (optarg)
				{
					args.promisc_valid = true;
					if(0 == strcmp(optarg, "on"))
					{
						args.promisc = true;
					}
					else if(0 == strcmp(optarg, "off"))
					{
						args.promisc = false;
					}
					else
					{
						args.promisc_valid = false;
					}

				}
				break;
			}

			case 's':
			{
				if (optarg)
				{
					args.match_valid = true;
					if(0 == strcmp(optarg, "or"))
					{
						args.match = true;
					}
					else if(0 == strcmp(optarg, "and"))
					{
						args.match = false;
					}
					else
					{
						args.match_valid = false;
					}

				}
				break;
			}

			case 'i':
			{
				/*	IFNAME */
				strncpy(args.ifname, optarg, IFNAMSIZ);
				args.ifname_valid = true;
				break;
			}

			case 'r':
			{
				/*	ROUTE ID */
				args.route_id = strtol(optarg, NULL, 10);
				args.route_id_valid = true;
				break;
			}


            case 'D':
            {
                /* 32bit data */
                args.data = htonl(strtoul(optarg, NULL, 0));
                break;
            }
            case 'M':
            {
                /* 32bit mask */
                args.mask = htonl(strtoul(optarg, NULL, 0));
                break;
            }
            case 'O':
            {
                /* 16bit offset */
                args.offset = strtoul(optarg, NULL, 0);
                if(args.offset > 65535)
                {
                    printf("Offset truncated to 0x%x\n", args.offset & 0xFFFFU);
                }
                args.offset = htons(args.offset);
                break;
            }
            case 'N':
            {
                /* Next rule - string */
                if((true == args.accept)||(true == args.reject))
                {
                    printf("--accept --reject --next_rule <rulename> are mutally exclusive\n");
                    return CMD_INVALID;
                }
                args.nr = true;
                strncpy(args.next_rule, optarg, 15U);
                break;
            }
            case 'A':
            {
                /* Accept */
                if((true == args.nr)||(true == args.reject))
                {
                    printf("--accept --reject --next_rule <rulename> are mutally exclusive\n");
                    return CMD_INVALID;
                }
                args.accept = true;
                break;
            }
            case 'R':
            {
                /* Reject */
                if((true == args.accept)||(true == args.nr))
                {
                    printf("--accept --reject --next_rule <rulename> are mutally exclusive\n");
                    return CMD_INVALID;
                }
                args.reject = true;
                break;
            }
            case 'I':
            {
                /* Invert */
                args.invert = true;
                break;
            }
            case 'E':
            {
                args.disable = true;
                break;
            }
            case 'T':
            {
                strncpy(args.table_name, optarg, 15U); /* Copy at most 15 characters leaving the last one \0 */
                if(strlen(optarg) > 15)
                {
                    printf("Warning: truncated %s to %s\n", optarg, args.rule_name);
                }

                break;
            }
            case 'U':
            {
                strncpy(args.rule_name, optarg, 15U); /* Copy at most 15 characters leaving the last one \0 */
                if(strlen(optarg) > 15)
                {
                    printf("Warning: truncated %s to %s\n", optarg, args.rule_name);
                }
                break;
            }
            case 'L':
            {
                args.layer = strtoul(optarg, NULL, 0);
                break;
            }
            case 'P':
            {
                args.position = strtoul(optarg, NULL, 0);
                break;
            }
			case '?':
			{
				/* Error */
				return CMD_INVALID;
				break;
			}

			case 0:
			{
				break;
			}

			default:
			{
				printf(help);
				return CMD_INVALID;
				break;
			}
		}
	}

	switch (icmd)
	{
		case CMD_IP_ROUTE_ADD:
		{
			if (args.dst_mac_valid && args.dip_valid && args.route_id_valid)
			{
				fci_cmd->rt_cmd.action = FPP_ACTION_REGISTER;
				strncpy(fci_cmd->rt_cmd.output_device, args.ifname, IFNAMSIZ-1);
				memcpy(&fci_cmd->rt_cmd.dst_mac, &args.dst_mac, 6);
				memcpy(&fci_cmd->rt_cmd.dst_addr, &args.dip, 16);
				fci_cmd->rt_cmd.id = args.route_id;

				if (args.dip_is_ipv6)
				{
					fci_cmd->rt_cmd.flags = 2U;	/* TODO: This is weird (see FCI doc). Some macro should be used instead. */
				}
				else
				{
					fci_cmd->rt_cmd.flags = 1U; /* TODO: This is weird (see FCI doc). Some macro should be used instead. */
				}
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_IP_ROUTE_DEL:
		{
			if (args.route_id_valid)
			{
				fci_cmd->rt_cmd.action = FPP_ACTION_DEREGISTER;
				fci_cmd->rt_cmd.id = args.route_id;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_IP_ROUTE_PRINT_ALL:
		{
			/*	Get and print all routes */
			break;
		}

		case CMD_IPV6_ADD_CONNTRACK:
		{
			if (args.sip_out_valid || args.dip_out_valid
					|| args.sport_out_valid || args.dport_out_valid)
			{
				printf("IPv6: IP address or L4 port modifications not supported (yet)\n");
				return CMD_INVALID;
			}

			if (args.route_id_valid && args.proto_valid && args.sip_valid
					&& args.dip_valid && args.sport_valid && args.dport_valid)
			{
				fci_cmd->ct6_cmd.action = FPP_ACTION_REGISTER;
				fci_cmd->ct6_cmd.route_id = args.route_id;
				fci_cmd->ct6_cmd.protocol = args.proto;
				memcpy(fci_cmd->ct6_cmd.saddr, args.sip, 16);
				memcpy(fci_cmd->ct6_cmd.daddr, args.dip, 16);
				fci_cmd->ct6_cmd.sport = args.sport;
				fci_cmd->ct6_cmd.dport = args.dport;
				memcpy(fci_cmd->ct6_cmd.saddr_reply, fci_cmd->ct6_cmd.daddr, 16);
				memcpy(fci_cmd->ct6_cmd.daddr_reply, fci_cmd->ct6_cmd.saddr, 16);
				fci_cmd->ct6_cmd.dport_reply = fci_cmd->ct6_cmd.sport;
				fci_cmd->ct6_cmd.sport_reply = fci_cmd->ct6_cmd.dport;

				/*	This is command to create UNI-DIRECTIONAL flow */
				fci_cmd->ct6_cmd.flags = CTCMD_FLAGS_REP_DISABLED;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_IPV4_ADD_CONNTRACK:
		{
			if (args.route_id_valid && args.proto_valid && args.sip_valid
					&& args.dip_valid && args.sport_valid && args.dport_valid)
			{
				fci_cmd->ct_cmd.action = FPP_ACTION_REGISTER;
				fci_cmd->ct_cmd.route_id = args.route_id;
				fci_cmd->ct_cmd.protocol = args.proto;
				memcpy(&fci_cmd->ct_cmd.saddr, args.sip, 4);
				memcpy(&fci_cmd->ct_cmd.daddr, args.dip, 4);
				fci_cmd->ct_cmd.sport = args.sport;
				fci_cmd->ct_cmd.dport = args.dport;

				if (args.sip_out_valid)
				{
					/*	SIP will be changed to --sip_out argument */
					memcpy(&fci_cmd->ct_cmd.daddr_reply, args.sip_out, 4);
				}
				else
				{
					/*	Do not change SIP */
					memcpy(&fci_cmd->ct_cmd.daddr_reply, &fci_cmd->ct_cmd.saddr, 4);
				}

				if (args.dip_out_valid)
				{
					/*	DIP will be changed to --dip_out argument */
					memcpy(&fci_cmd->ct_cmd.saddr_reply, args.dip_out, 4);
				}
				else
				{
					/*	Do not change DIP */
					memcpy(&fci_cmd->ct_cmd.saddr_reply, &fci_cmd->ct_cmd.daddr, 4);
				}

				if (args.sport_out_valid)
				{
					/*	SPORT will be changed to --sport_out argument */
					fci_cmd->ct_cmd.dport_reply = args.sport_out;
				}
				else
				{
					/*	Do not change SPORT */
					fci_cmd->ct_cmd.dport_reply = fci_cmd->ct_cmd.sport;
				}

				if (args.dport_out_valid)
				{
					/*	DPORT will be changed to --dport_out argument */
					fci_cmd->ct_cmd.sport_reply = args.dport_out;
				}
				else
				{
					/*	Do not change DPORT */
					fci_cmd->ct_cmd.sport_reply = fci_cmd->ct_cmd.dport;
				}

				/*	This is command to create UNI-DIRECTIONAL flow */
				fci_cmd->ct_cmd.flags = CTCMD_FLAGS_REP_DISABLED;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_IPV6_DEL_CONNTRACK:
		{
			if (args.proto_valid && args.sip_valid && args.dip_valid
					&& args.sport_valid && args.dport_valid)
			{
				fci_cmd->ct6_cmd.action = FPP_ACTION_DEREGISTER;
				fci_cmd->ct6_cmd.route_id = 0xffffffffU; /* Invalid, not needed, will be ignored */
				fci_cmd->ct6_cmd.protocol = args.proto;
				memcpy(fci_cmd->ct6_cmd.saddr, args.sip, 16);
				memcpy(fci_cmd->ct6_cmd.daddr, args.dip, 16);
				fci_cmd->ct6_cmd.sport = args.sport;
				fci_cmd->ct6_cmd.dport = args.dport;
				memcpy(fci_cmd->ct6_cmd.daddr_reply, fci_cmd->ct6_cmd.saddr, 16);
				memcpy(fci_cmd->ct6_cmd.saddr_reply, fci_cmd->ct6_cmd.daddr, 16);
				fci_cmd->ct6_cmd.dport_reply = fci_cmd->ct6_cmd.sport;
				fci_cmd->ct6_cmd.sport_reply = fci_cmd->ct6_cmd.dport;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_IPV4_DEL_CONNTRACK:
		{
			if (args.proto_valid && args.sip_valid && args.dip_valid
					&& args.sport_valid && args.dport_valid)
			{
				fci_cmd->ct_cmd.action = FPP_ACTION_DEREGISTER;
				fci_cmd->ct_cmd.route_id = 0xffffffffU; /* Invalid, not needed, will be ignored */
				fci_cmd->ct_cmd.protocol = args.proto;
				memcpy(&fci_cmd->ct_cmd.saddr, args.sip, 4);
				memcpy(&fci_cmd->ct_cmd.daddr, args.dip, 4);
				fci_cmd->ct_cmd.sport = args.sport;
				fci_cmd->ct_cmd.dport = args.dport;
				memcpy(&fci_cmd->ct_cmd.daddr_reply, &fci_cmd->ct_cmd.saddr, 4);
				memcpy(&fci_cmd->ct_cmd.saddr_reply, &fci_cmd->ct_cmd.daddr, 4);
				fci_cmd->ct_cmd.dport_reply = fci_cmd->ct_cmd.sport;
				fci_cmd->ct_cmd.sport_reply = fci_cmd->ct_cmd.dport;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_CONNTRACK_PRINT_ALL:
		{
			/*	Print all connections */
			break;
		}

		case CMD_L2BRIDGE_DOMAIN_ADD:
		{
			if (args.vlan_valid && args.uhit_valid && args.umiss_valid
					&& args.mhit_valid && args.mmiss_valid)
			{
				fci_cmd->br_domain_ctrl.action = FPP_ACTION_REGISTER;
				fci_cmd->br_domain_ctrl.vlan = args.vlan;
				fci_cmd->br_domain_ctrl.ucast_hit = args.ucast_hit;
				fci_cmd->br_domain_ctrl.ucast_miss = args.ucast_miss;
				fci_cmd->br_domain_ctrl.mcast_hit = args.mcast_hit;
				fci_cmd->br_domain_ctrl.mcast_miss = args.mcast_miss;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_DEL:
		{
			if (args.vlan_valid)
			{
				fci_cmd->br_domain_ctrl.action = FPP_ACTION_DEREGISTER;
				fci_cmd->br_domain_ctrl.vlan = args.vlan;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_SET_ACT:
		{
			if (args.vlan_valid && args.uhit_valid && args.umiss_valid
					&& args.mhit_valid && args.mmiss_valid)
			{
				fci_cmd->br_domain_ctrl.action = FPP_ACTION_UPDATE;
				fci_cmd->br_domain_ctrl.vlan = args.vlan;
				fci_cmd->br_domain_ctrl.ucast_hit = args.ucast_hit;
				fci_cmd->br_domain_ctrl.ucast_miss = args.ucast_miss;
				fci_cmd->br_domain_ctrl.mcast_hit = args.mcast_hit;
				fci_cmd->br_domain_ctrl.mcast_miss = args.mcast_miss;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_ADD_IF:
		{
			if (args.ifname_valid && args.tag_flag_valid && args.vlan_valid)
			{
				fci_cmd->br_domain_if_ctrl.add = true;
				strcpy(fci_cmd->br_domain_if_ctrl.ifname, args.ifname);
				fci_cmd->br_domain_if_ctrl.tag = args.tag_flag;
				fci_cmd->br_domain_if_ctrl.vlan = args.vlan;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_DEL_IF:
		{
			if (args.ifname_valid && args.vlan_valid)
			{
				fci_cmd->br_domain_if_ctrl.add = false;
				strcpy(fci_cmd->br_domain_if_ctrl.ifname, args.ifname);
				fci_cmd->br_domain_if_ctrl.vlan = args.vlan;
				/*	This will ensure that interface will be removed from untag list */
				fci_cmd->br_domain_if_ctrl.tag = true;
			}
			else
			{
				printf("Missing argument\n");
				return CMD_INVALID;
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_PRINT_ALL:
		{
			/*	Print all bridge domains */
			break;
		}

        case CMD_FP_ADD_RULE:
        {
            fci_cmd->fp_rule_cmd.action = FPP_ACTION_REGISTER;
            strncpy((char *)fci_cmd->fp_rule_cmd.r.rule_name,args.rule_name,16U);
            fci_cmd->fp_rule_cmd.r.data = args.data;
            fci_cmd->fp_rule_cmd.r.mask = args.mask;
            fci_cmd->fp_rule_cmd.r.offset = args.offset;
            fci_cmd->fp_rule_cmd.r.invert = args.invert;
            strncpy((char *)fci_cmd->fp_rule_cmd.r.next_rule_name, args.next_rule, 16U);
            if(true == args.accept)
            {
                fci_cmd->fp_rule_cmd.r.match_action = FP_ACCEPT;

            }
            else if(true == args.reject)
            {
                fci_cmd->fp_rule_cmd.r.match_action = FP_REJECT;
            }
            else if(true == args.nr)
            {
                fci_cmd->fp_rule_cmd.r.match_action = FP_NEXT_RULE;
            }
            else
            {
                /* Error */
                printf("--accept, --reject, or --next_rule <rulename> is required\n");
                return CMD_INVALID;
            }

            if(args.layer == 2)
            {
                fci_cmd->fp_rule_cmd.r.offset_from = FP_OFFSET_FROM_L2_HEADER;
            }
            else if(args.layer == 3)
            {
                fci_cmd->fp_rule_cmd.r.offset_from = FP_OFFSET_FROM_L3_HEADER;
            }
            else if(args.layer == 4)
            {
                fci_cmd->fp_rule_cmd.r.offset_from = FP_OFFSET_FROM_L4_HEADER;
            }
            else
            {
                printf("--layer must have value 2, 3, or 4\n");
                return CMD_INVALID;
            }
            break;
        }
        case CMD_FP_DEL_RULE:
        {
            fci_cmd->fp_rule_cmd.action = FPP_ACTION_DEREGISTER;
            strncpy((char *)fci_cmd->fp_rule_cmd.r.rule_name,args.rule_name,16U);
            break;
        }
        case CMD_FP_ADD_TABLE:
        {
            fci_cmd->fp_table_cmd.action = FPP_ACTION_REGISTER;
            strncpy((char *)fci_cmd->fp_table_cmd.t.table_name,args.table_name, 16U);
            break;
        }
        case CMD_FP_DEL_TABLE:
        {
            fci_cmd->fp_table_cmd.action = FPP_ACTION_DEREGISTER;
            strncpy((char *)fci_cmd->fp_table_cmd.t.table_name,args.table_name, 16U);
            break;
        }
        case CMD_FP_SET_TABLE_RULE:
        {
            fci_cmd->fp_table_cmd.action = FPP_ACTION_USE_RULE;
            fci_cmd->fp_table_cmd.t.position = args.position;
            strncpy((char *)fci_cmd->fp_table_cmd.t.table_name, args.table_name, 16U);
            strncpy((char *)fci_cmd->fp_table_cmd.t.rule_name, args.rule_name, 16U);
            break;
        }
        case CMD_FP_UNSET_TABLE_RULE:
        {
            /* Table name is not needed - rules keep reference to table they are in */
            strncpy((char *)fci_cmd->fp_table_cmd.t.rule_name, args.rule_name, 16U);
            fci_cmd->fp_table_cmd.action = FPP_ACTION_UNUSE_RULE;
            break;
        }
        case CMD_FP_PRINT_TABLE:
        {
            fci_cmd->fp_table_cmd.action = FPP_ACTION_QUERY;
            strncpy((char *)fci_cmd->fp_table_cmd.t.table_name, args.table_name, 16U);
            break;
        }
        case CMD_FP_PRINT_RULES:
        {
            fci_cmd->fp_rule_cmd.action = FPP_ACTION_QUERY;
            break;
        }
        case CMD_FP_FLEXIBLE_PARSER:
        {
            if(true == args.disable)
            {
                fci_cmd->fp_filter_cmd.action = FPP_ACTION_DEREGISTER;
            }
            else
            {
                fci_cmd->fp_filter_cmd.action = FPP_ACTION_REGISTER;
                strncpy((char *)fci_cmd->fp_filter_cmd.table_name, args.table_name, 16U);
            }
            break;
        }

		case CMD_IF_PRINT_ALL:
		{
			break;
		}

		case CMD_LOG_IF_UPDATE:
		{
			fci_cmd->log_if_cmd.flags = 0U;


			if(args.vlan_valid && (0U != (args.match_rules & FPP_IF_MATCH_VLAN )))
			{
				fci_cmd->log_if_cmd.arguments.vlan = args.vlan;

			}
			else if(args.vlan_valid || (0U != (args.match_rules & FPP_IF_MATCH_VLAN )))
			{
				printf("Missing VLAN_ID argument\n");
				return CMD_INVALID;
			}

			if((!args.sip_is_ipv6 && args.sip_valid) && (0U != (args.match_rules & FPP_IF_MATCH_SIP )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.v4.sip, &args.sip, sizeof(fci_cmd->log_if_cmd.arguments.v4.sip));

			}
			else if((!args.sip_is_ipv6 && args.sip_valid) || (0U != (args.match_rules & FPP_IF_MATCH_SIP )))
			{
				printf("Missing SIP argument\n");
				return CMD_INVALID;
			}

			if((!args.dip_is_ipv6 && args.dip_valid) && (0U != (args.match_rules & FPP_IF_MATCH_DIP )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.v4.dip, &args.dip, sizeof(fci_cmd->log_if_cmd.arguments.v4.dip));

			}
			else if((!args.dip_is_ipv6 && args.dip_valid) || (0U != (args.match_rules & FPP_IF_MATCH_DIP )))
			{
				printf("Missing DIP argument\n");
				return CMD_INVALID;
			}


			if(args.sip_is_ipv6 && args.sip_valid && (0U != (args.match_rules & FPP_IF_MATCH_SIP6 )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.v6.sip, &args.sip, sizeof(fci_cmd->log_if_cmd.arguments.v6.sip));

			}
			else if((args.sip_is_ipv6 && args.sip_valid) || (0U != (args.match_rules & FPP_IF_MATCH_SIP6 )))
			{
				printf("Missing SIP6 argument\n");
				return CMD_INVALID;
			}

			if(args.dip_is_ipv6 && args.dip_valid && (0U != (args.match_rules & FPP_IF_MATCH_DIP6 )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.v6.dip, &args.dip, sizeof(fci_cmd->log_if_cmd.arguments.v6.dip));

			}
			else if((args.dip_is_ipv6 && args.dip_valid) || (0U != (args.match_rules & FPP_IF_MATCH_DIP6 )))
			{
				printf("Missing DIP6 argument\n");
				return CMD_INVALID;
			}

			if(args.sport_valid && (0U != (args.match_rules & FPP_IF_MATCH_SPORT )))
			{
				fci_cmd->log_if_cmd.arguments.sport = args.sport;

			}
			else if(args.sport_valid || (0U != (args.match_rules & FPP_IF_MATCH_SPORT )))
			{
				printf("Missing SPORT argument\n");
				return CMD_INVALID;
			}

			if(args.dport_valid && (0U != (args.match_rules & FPP_IF_MATCH_DPORT )))
			{
				fci_cmd->log_if_cmd.arguments.dport = args.dport;

			}
			else if(args.dport_valid || (0U != (args.match_rules & FPP_IF_MATCH_DPORT )))
			{
				printf("Missing DPORT argument\n");
				return CMD_INVALID;
			}

			if(args.src_mac_valid && (0U != (args.match_rules & FPP_IF_MATCH_SMAC )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.smac, &args.src_mac, sizeof(fci_cmd->log_if_cmd.arguments.smac));
			}
			else if(args.src_mac_valid || (0U != (args.match_rules & FPP_IF_MATCH_SMAC )))
			{
				printf("Missing SMAC argument\n");
				return CMD_INVALID;
			}

			if(args.dst_mac_valid && (0U != (args.match_rules & FPP_IF_MATCH_DMAC )))
			{
				memcpy(&fci_cmd->log_if_cmd.arguments.dmac, &args.dst_mac, sizeof(fci_cmd->log_if_cmd.arguments.dmac));

			}
			else if(args.dst_mac_valid || (0U != (args.match_rules & FPP_IF_MATCH_DMAC )))
			{
				printf("Missing DMAC argument\n");
				return CMD_INVALID;
			}

			if(args.proto_valid && (0U != (args.match_rules & FPP_IF_MATCH_PROTO )))
			{
				fci_cmd->log_if_cmd.arguments.proto = args.proto;

			}
			else if(args.proto_valid || (0U != (args.match_rules & FPP_IF_MATCH_PROTO )))
			{
				printf("Missing PROTO argument\n");
				return CMD_INVALID;
			}

			if(args.eth_valid && (0U != (args.match_rules & FPP_IF_MATCH_ETHTYPE )))
			{
				fci_cmd->log_if_cmd.arguments.ethtype = args.eth;

			}
			else if(args.eth_valid || (0U != (args.match_rules & FPP_IF_MATCH_ETHTYPE )))
			{
				printf("Missing ETHER_TYPE argument\n");
				return CMD_INVALID;
			}



			if(args.ifname_valid)
			{
				strcpy(fci_cmd->log_if_cmd.name, args.ifname);
			}
			else
			{
				printf("Missing if name argument missing\n");
				return CMD_INVALID;
			}



			if(args.egress_valid)
			{
				fci_cmd->log_if_cmd.egress = args.egress;
			}

			if(args.enable_valid)
			{
				if(args.enable)
				{
					fci_cmd->log_if_cmd.flags |= FPP_IF_ENABLED;
				}
			}

			if(args.promisc_valid)
			{
				if(args.promisc)
				{
					fci_cmd->log_if_cmd.flags |= FPP_IF_PROMISC;
				}
			}

			if(args.match_valid)
			{
				if(args.match)
				{
					fci_cmd->log_if_cmd.flags |= FPP_IF_MATCH_OR;
				}
			}

			fci_cmd->log_if_cmd.match = args.match_rules;
			break;
		}

		default:
		{
			/* No extra action needed */
			break;
		}
	}

	return icmd;
}

/**
 * @brief		Query bridge domain by VLAN ID
 * @param		vlan
 * @param		bd_cmd_reply Here the query command result will be written
 * @return		EOK Success
 */
static errno_t query_bridge_domain(FCI_CLIENT *fcicl, uint16_t vlan, fpp_l2_bridge_domain_control_cmd_t *bd_cmd_reply)
{
	fpp_l2_bridge_domain_control_cmd_t bd_cmd;
	unsigned short reply_len;
	bool match = false;
	int err;

	/*	Query given domain */
	bd_cmd.action = FPP_ACTION_QUERY;
	reply_len = sizeof(fpp_l2_bridge_domain_control_cmd_t);
	err = fci_query(fcicl, FPP_CMD_L2BRIDGE_DOMAIN,
					sizeof(fpp_l2_bridge_domain_control_cmd_t),
					(unsigned short *)&bd_cmd, &reply_len,
					(unsigned short *)(bd_cmd_reply));

	while (FPP_ERR_OK == err)
	{
		if (bd_cmd_reply->vlan == vlan)
		{
			/*	Got the domain (in bd_reply_buf) */
			match = true;
			break;
		}

		bd_cmd.action = FPP_ACTION_QUERY_CONT;
		err = fci_query(fcicl, FPP_CMD_L2BRIDGE_DOMAIN,
						sizeof(fpp_l2_bridge_domain_control_cmd_t),
						(unsigned short *)&bd_cmd, &reply_len,
						(unsigned short *)(bd_cmd_reply));
	}

	if (false == match)
	{
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief		Query interface by name
 * @param		name
 * @param		if_cmd_reply Here the query command result will be written
 * @return		EOK Success
 */
static errno_t query_interface(FCI_CLIENT *fcicl, char *name, fpp_phy_if_cmd_t *if_cmd_reply)
{
	fpp_phy_if_cmd_t if_cmd;
	unsigned short reply_len;
	fpp_phy_if_cmd_t if_buff;
	bool match = false;
	int err;

	/* Lock interfaces for exclusive acces from FCI */
	err = fci_write(fcicl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);

	if(FPP_ERR_OK != err)
	{
		/* In case locking error return */
		return err;
	}

	/*	Convert interface name to physical interface ID */
	if_cmd.action = FPP_ACTION_QUERY;
	reply_len = sizeof(fpp_phy_if_cmd_t);
	err = fci_query(fcicl, FPP_CMD_PHY_INTERFACE,
					sizeof(fpp_phy_if_cmd_t),
					(unsigned short *)&if_cmd, &reply_len,
					(unsigned short *)(if_cmd_reply));

	while (FPP_ERR_OK == err)
	{
		if (0 == strcmp(if_cmd_reply->name, name))
		{
			/*	Got the interface */
			memcpy(&if_buff, if_cmd_reply, sizeof(fpp_phy_if_cmd_t));
			match = true;
			break;
		}

		if_cmd.action = FPP_ACTION_QUERY_CONT;
		reply_len = sizeof(fpp_phy_if_cmd_t);
		err = fci_query(fcicl, FPP_CMD_PHY_INTERFACE,
						sizeof(fpp_phy_if_cmd_t),
						(unsigned short *)&if_cmd, &reply_len,
						(unsigned short *)(if_cmd_reply));
	}

	/* Unlock interfaces */
	err = fci_write(fcicl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);

	memcpy(if_cmd_reply, &if_buff, sizeof(fpp_phy_if_cmd_t));

	if (false == match)
	{
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief		Query logical interfaces by parrent name
 * @param		fcicl instance for recursive call
 * @param		name parrent name of the interface
 * @param		callback to be called (fcicl instance, cmd,)
 * @return		EOK Success
 */
static errno_t query_log_interface(FCI_CLIENT *fcicl, char *name, int (*callback)(FCI_CLIENT *,void*))
{
	fpp_log_if_cmd_t if_cmd;
	fpp_log_if_cmd_t if_cmd_reply;
	unsigned short reply_len;
	int err, ret = ENOENT;

	/*	Convert interface name to physical interface ID */
	if_cmd.action = FPP_ACTION_QUERY;
	reply_len = sizeof(fpp_log_if_cmd_t);
	err = fci_query(fcicl, FPP_CMD_LOG_INTERFACE,
					sizeof(fpp_log_if_cmd_t),
					(unsigned short *)&if_cmd, &reply_len,
					(unsigned short *)(&if_cmd_reply));

	while (FPP_ERR_OK == err)
	{
		if (NULL != callback)
		{
			if(0U == strcmp(name, if_cmd_reply.parent_name))
			{
				/* Perform callback */
				ret = callback(fcicl, (void*)&if_cmd_reply);
			}
		}

		if_cmd.action = FPP_ACTION_QUERY_CONT;
		reply_len = sizeof(fpp_log_if_cmd_t);
		err = fci_query(fcicl, FPP_CMD_LOG_INTERFACE,
						sizeof(fpp_log_if_cmd_t),
						(unsigned short *)&if_cmd, &reply_len,
						(unsigned short *)(&if_cmd_reply));
	}

	return ret;
}

static void arg_info_to_string(fpp_if_m_args_t *args, fpp_if_m_rules_t rule, char *buff, uint32_t size)
{
	char loc_buff[MAX_IP_ADDR_STR_LEN];
	switch(rule)
	{
		case FPP_IF_MATCH_VLAN:
		{
			snprintf(buff,size,"(%u)", ntohs(args->vlan));
			break;
		}
		case FPP_IF_MATCH_SIP:
		{
			inet_ntop(AF_INET, &args->v4.sip, loc_buff, sizeof(loc_buff));
			snprintf(buff,size,"(%s)", loc_buff);
			break;
		}
		case FPP_IF_MATCH_DIP:
		{
			inet_ntop(AF_INET, &args->v4.dip, loc_buff, sizeof(loc_buff));
			snprintf(buff,size,"(%s)", loc_buff);
			break;
		}
		case FPP_IF_MATCH_SIP6:
		{
			inet_ntop(AF_INET6, &args->v6.sip, loc_buff, sizeof(loc_buff));
			snprintf(buff,size,"(%s)", loc_buff);
			break;
		}
		case FPP_IF_MATCH_DIP6:
		{
			inet_ntop(AF_INET6, &args->v6.dip, loc_buff, sizeof(loc_buff));
			snprintf(buff,size,"(%s)", loc_buff);
			break;
		}
		case FPP_IF_MATCH_SMAC:
		{
			snprintf(buff,size,"(%02x:%02x:%02x:%02x:%02x:%02x)", args->smac[0U], args->smac[1U], args->smac[2U], args->smac[3U], args->smac[4U], args->smac[5U]);
			break;
		}
		case FPP_IF_MATCH_DMAC:
		{
			snprintf(buff,size,"(%02x:%02x:%02x:%02x:%02x:%02x)", args->dmac[0U], args->dmac[1U], args->dmac[2U], args->dmac[3U], args->dmac[4U], args->dmac[5U]);
			break;
		}
		case FPP_IF_MATCH_SPORT:
		{
			snprintf(buff,size,"(%u)", ntohs(args->sport));
			break;
		}
		case FPP_IF_MATCH_DPORT:
		{
			snprintf(buff,size,"(%u)", ntohs(args->dport));
			break;
		}
		case FPP_IF_MATCH_ETHTYPE:
		{
			snprintf(buff,size,"(%u)", ntohs(args->ethtype));
			break;
		}
		default:
		{
			strcpy(buff,"");
			break;
		}
	}
}

static char * if_flag_parser(fpp_if_flags_t flags, fpp_if_flags_t flag)
{
	switch(flag)
	{
	case FPP_IF_ENABLED:
	{
		if(FPP_IF_ENABLED == (flags & FPP_IF_ENABLED))
		{
			return "ENABLED";
		}
		else
		{
			return "DISABLED";
		}
		break;
	}

	case FPP_IF_PROMISC:
	{
		if(FPP_IF_PROMISC == (flags & FPP_IF_PROMISC))
		{
			return "PROMISC_ON";
		}
		else
		{
			return "PROMISC_OFF";
		}
		break;
	}

	case FPP_IF_MATCH_OR:
	{
		if(FPP_IF_MATCH_OR == (flags & FPP_IF_MATCH_OR))
		{
			return "OR_MATCH";
		}
		else
		{
			return "AND_MATCH";
		}
	}
	default:
		/* Empty string in case of error*/
		return "INVALID";
	}
}


static int log_if_reply_parse(FCI_CLIENT *fcicl, void *log_if_reply)
{
	static char buffer[512];
	fpp_log_if_cmd_t *if_cmd_reply = (fpp_log_if_cmd_t*)log_if_reply;
	uint32_t ii= 0U;

	printf(	"\n\t%u:%s:\t<%s, %s, %s>\n\tEgress phy id: ", if_cmd_reply->id, if_cmd_reply->name,
			if_flag_parser(if_cmd_reply->flags, FPP_IF_ENABLED),
			if_flag_parser(if_cmd_reply->flags, FPP_IF_PROMISC),
			if_flag_parser(if_cmd_reply->flags, FPP_IF_MATCH_OR));

	for (ii=0; ii < 8*sizeof(if_cmd_reply->egress); ii++)
	{
		if (if_cmd_reply->egress & (1 << ii))
		{
			printf("%d ", ii);
		}
	}

	printf("\n\t%s: ", "Match rules");
	if(0U == if_cmd_reply->match)
	{
		printf("%s", "No rules found");
	}
	else
	{
		for (ii=0; ii < 8*sizeof(if_cmd_reply->match); ii++)
		{
			if (if_cmd_reply->match & (1 << ii))
			{
				arg_info_to_string(&if_cmd_reply->arguments, if_cmd_reply->match & (1 << ii), buffer, sizeof(buffer));
				printf("%s%s ", flags[ii], buffer);
			}
		}
	}
	return EOK;
}

static int phy_if_reply_parse(FCI_CLIENT *fcicl, fpp_phy_if_cmd_t * phy_if_reply)
{
	static char *mode_lut[] = {"OP_DISABLED", "OP_DEFAULT", "OP_BRIDGE", "OP_ROUTER", "OP_VLANBRIDGE","OP_INVALID"};
	uint16_t mode = 5U;
	fpp_phy_if_cmd_t *if_cmd_reply = phy_if_reply;

	/* Check that index is correct */
	if(sizeof(mode_lut)/sizeof(mode_lut[0U]) > if_cmd_reply->mode)
	{
		mode = if_cmd_reply->mode;
	}
	else
	{
		mode = 5U; /* Invalid mode */
	}

	printf("%u:%-5s: <%s, %s, %s>", if_cmd_reply->id, if_cmd_reply->name,
			if_flag_parser(if_cmd_reply->flags, FPP_IF_ENABLED),
			if_flag_parser(if_cmd_reply->flags, FPP_IF_PROMISC),
			mode_lut[mode]);
	/* Start query of log ifs*/
	query_log_interface(fcicl, if_cmd_reply->name, &log_if_reply_parse);
	/* Add new line after each phy_if query is completed */
	puts("");

	return EOK;
}



int main(int argc, char *argv[])
{
	FCI_CLIENT *fcicl;
	int err;
	command_t icmd = CMD_INVALID;
	fci_cmd_t fci_cmd_buf;
	int ipv4_cnt, ipv6_cnt;

	memset(&fci_cmd_buf, 0, sizeof(fci_cmd_t));

	icmd = arg_parser(argc, argv, &fci_cmd_buf);

	if (CMD_INVALID == icmd)
	{
		printf("%s: Wrong argument\n", argv[0]);
		return EXIT_FAILURE;
	}

	/*	Open the FCI */
	fcicl = fci_open(FCILIB_FF_TYPE, 0);
	if (NULL == fcicl)
	{
		printf("fci_open() failed\n");
		return EXIT_FAILURE;
	}

	switch (icmd)
	{
		case CMD_IPV4_RESET:
		{
			/*	Write the command */
			err = fci_write(fcicl, FPP_CMD_IPV4_RESET, 0, NULL);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_IPV4_RESET: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_IPV4_RESET: Command sent\n");
			}

			break;
		}
		case CMD_IPV6_RESET:
		{
			/*	Write the command */
			err = fci_write(fcicl, FPP_CMD_IPV6_RESET, 0, NULL);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_IPV6_RESET: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_IPV6_RESET: Command sent\n");
			}

			break;
		}

		case CMD_IP_ROUTE_ADD:
		case CMD_IP_ROUTE_DEL:
		{
			/*	Write the command */
			err = fci_write(fcicl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_IP_ROUTE: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_IP_ROUTE: Command sent\n");
			}

			break;
		}

		case CMD_IP_ROUTE_PRINT_ALL:
		{
			fpp_rt_cmd_t rt_reply_buf;
			unsigned short reply_len;

			ipv4_cnt = 0;
			ipv6_cnt = 0;

			/*	Get and print all routes */
			fci_cmd_buf.rt_cmd.action = FPP_ACTION_QUERY;
			reply_len = sizeof(fpp_rt_cmd_t);
			err = fci_query(fcicl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&rt_reply_buf));
			while (FPP_ERR_OK == err)
			{
				if (0U != (rt_reply_buf.flags & 0x2U))
				{
					print_route(true, &rt_reply_buf.id, rt_reply_buf.dst_mac, (uint8_t *)rt_reply_buf.dst_addr, rt_reply_buf.output_device);
					ipv6_cnt++;
				}
				else if (0 != (rt_reply_buf.flags & 0x1U))
				{
					print_route(false, &rt_reply_buf.id, rt_reply_buf.dst_mac, (uint8_t *)rt_reply_buf.dst_addr, rt_reply_buf.output_device);
					ipv4_cnt++;
				}
				else
				{
					printf("INVALID ROUTE FLAGS\n");
				}

				fci_cmd_buf.rt_cmd.action = FPP_ACTION_QUERY_CONT;
				reply_len = sizeof(fpp_rt_cmd_t);
				err = fci_query(fcicl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&rt_reply_buf));
			}

			if (0 == ipv4_cnt)
			{
				printf("No IPv4 routes\n");
			}
			else
			{
				printf("Found %d IPv4 routes\n", ipv4_cnt);
			}

			if (0 == ipv6_cnt)
			{
				printf("No IPv6 routes\n\n");
			}
			else
			{
				printf("Found %d IPv6 routes\n\n", ipv6_cnt);
			}

			break;
		}

		case CMD_IPV4_ADD_CONNTRACK:
		case CMD_IPV4_DEL_CONNTRACK:
		{
			err = fci_write(fcicl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_IPV4_CONNTRACK: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_IPV4_CONNTRACK: Command sent\n");
			}

			break;
		}

		case CMD_IPV6_ADD_CONNTRACK:
		case CMD_IPV6_DEL_CONNTRACK:
		{
			err = fci_write(fcicl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_IPV6_CONNTRACK: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_IPV6_CONNTRACK: Command sent\n");
			}

			break;
		}

		case CMD_CONNTRACK_PRINT_ALL:
		{
			/*	Print all connections */
			fpp_ct_cmd_t ct_reply_buf;
			fpp_ct6_cmd_t ct6_reply_buf;
			unsigned short reply_len;

			ipv4_cnt = 0;
			ipv6_cnt = 0;

			/*	Get and print all IPv4 connections */
			fci_cmd_buf.ct_cmd.action = FPP_ACTION_QUERY;
			reply_len = sizeof(fpp_ct_cmd_t);
			err = fci_query(fcicl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&ct_reply_buf));
			while (FPP_ERR_OK == err)
			{
				print_conntrack(false, /* IPv4 */
								 (uint8_t *)&ct_reply_buf.saddr,
								 (uint8_t *)&ct_reply_buf.daddr,
								 &ct_reply_buf.sport,
								 &ct_reply_buf.dport,
								 (uint8_t *)&ct_reply_buf.daddr_reply,
								 (uint8_t *)&ct_reply_buf.saddr_reply,
								 &ct_reply_buf.dport_reply,
								 &ct_reply_buf.sport_reply,
								 (uint8_t *)&ct_reply_buf.protocol);

				ipv4_cnt++;
				fci_cmd_buf.ct_cmd.action = FPP_ACTION_QUERY_CONT;
				reply_len = sizeof(fpp_ct_cmd_t);
				err = fci_query(fcicl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&ct_reply_buf));
			}

			/*	Get and print all IPv6 connections */
			fci_cmd_buf.ct6_cmd.action = FPP_ACTION_QUERY;
			reply_len = sizeof(fpp_ct6_cmd_t);
			err = fci_query(fcicl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&ct6_reply_buf));
			while (FPP_ERR_OK == err)
			{
				print_conntrack(true, /* IPv6 */
								 (uint8_t *)ct6_reply_buf.saddr,
								 (uint8_t *)ct6_reply_buf.daddr,
								 &ct6_reply_buf.sport,
								 &ct6_reply_buf.dport,
								 (uint8_t *)ct6_reply_buf.daddr_reply,
								 (uint8_t *)ct6_reply_buf.saddr_reply,
								 &ct6_reply_buf.dport_reply,
								 &ct6_reply_buf.sport_reply,
								 (uint8_t *)&ct6_reply_buf.protocol);

				ipv6_cnt++;
				fci_cmd_buf.ct6_cmd.action = FPP_ACTION_QUERY_CONT;
				reply_len = sizeof(fpp_ct6_cmd_t);
				err = fci_query(fcicl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&ct6_reply_buf));
			}

			if (0 == ipv4_cnt)
			{
				printf("No IPv4 conntracks\n");
			}
			else
			{
				printf("Found %d IPv4 conntracks\n", ipv4_cnt);
			}

			if (0 == ipv6_cnt)
			{
				printf("No IPv6 conntracks\n\n");
			}
			else
			{
				printf("Found %d IPv6 conntracks\n\n", ipv6_cnt);
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_ADD:
		case CMD_L2BRIDGE_DOMAIN_DEL:
		{
			err = fci_write(fcicl, FPP_CMD_L2BRIDGE_DOMAIN, sizeof(fpp_l2_bridge_domain_control_cmd_t), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: Command sent\n");
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_ADD_IF:
		case CMD_L2BRIDGE_DOMAIN_DEL_IF:
		{
			fpp_l2_bridge_domain_control_cmd_t bd_reply_buf;
			fpp_phy_if_cmd_t if_reply_buf;
			char ifname[IFNAMSIZ];

			/*	Save arguments */
			strcpy(ifname, fci_cmd_buf.br_domain_if_ctrl.ifname);

			/*	Get domain (bd_reply_buf) */
			if (EOK != query_bridge_domain(fcicl, fci_cmd_buf.br_domain_if_ctrl.vlan, &bd_reply_buf))
			{
				printf("No such domain (%d)\n", ntohs(fci_cmd_buf.br_domain_if_ctrl.vlan));
				return EXIT_FAILURE;
			}

			/*	Get interface (if_reply_buf) */
			if (EOK != query_interface(fcicl, ifname, &if_reply_buf))
			{
				printf("No such interface (%s)\n", ifname);
				return EXIT_FAILURE;
			}

			/*	Prepare and issue the FPP_ACTION_UPDATE command */
			bd_reply_buf.action = FPP_ACTION_UPDATE;

			if (fci_cmd_buf.br_domain_if_ctrl.add)
			{
				/*	ADD */
				bd_reply_buf.if_list |= htonl((1 << ntohl(if_reply_buf.id)));
			}
			else
			{
				/*	DEL */
				bd_reply_buf.if_list &= htonl(~(1 << ntohl(if_reply_buf.id)));
			}

			if (fci_cmd_buf.br_domain_if_ctrl.tag)
			{
				/*	ADD/DEL */
				bd_reply_buf.untag_if_list &= htonl(~(1 << ntohl(if_reply_buf.id)));
			}
			else
			{
				/*	ADD */
				bd_reply_buf.untag_if_list |= htonl((1 << ntohl(if_reply_buf.id)));
			}

			err = fci_write(fcicl, FPP_CMD_L2BRIDGE_DOMAIN, sizeof(fpp_l2_bridge_domain_control_cmd_t), (unsigned short *)&bd_reply_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: Command sent\n");
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_SET_ACT:
		{
			fpp_l2_bridge_domain_control_cmd_t bd_reply_buf;

			/*	Get domain (bd_reply_buf) */
			if (EOK != query_bridge_domain(fcicl, fci_cmd_buf.br_domain_ctrl.vlan, &bd_reply_buf))
			{
				printf("No such domain (%d)\n", ntohs(fci_cmd_buf.br_domain_if_ctrl.vlan));
				return EXIT_FAILURE;
			}

			bd_reply_buf.action = fci_cmd_buf.br_domain_ctrl.action; /* FPP_ACTION_UPDATE by the way */
			bd_reply_buf.ucast_hit = fci_cmd_buf.br_domain_ctrl.ucast_hit;
			bd_reply_buf.ucast_miss = fci_cmd_buf.br_domain_ctrl.ucast_miss;
			bd_reply_buf.mcast_hit = fci_cmd_buf.br_domain_ctrl.mcast_hit;
			bd_reply_buf.mcast_miss = fci_cmd_buf.br_domain_ctrl.mcast_miss;

			err = fci_write(fcicl, FPP_CMD_L2BRIDGE_DOMAIN, sizeof(fpp_l2_bridge_domain_control_cmd_t), (unsigned short *)&bd_reply_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_L2BRIDGE_DOMAIN: Command sent\n");
			}

			break;
		}

		case CMD_L2BRIDGE_DOMAIN_PRINT_ALL:
		{
			fpp_l2_bridge_domain_control_cmd_t bd_reply_buf;
			unsigned short reply_len;
			int cnt = 0, ii;

			fci_cmd_buf.br_domain_ctrl.action = FPP_ACTION_QUERY;
			reply_len = sizeof(fpp_l2_bridge_domain_control_cmd_t);
			err = fci_query(fcicl, FPP_CMD_L2BRIDGE_DOMAIN, sizeof(fpp_l2_bridge_domain_control_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&bd_reply_buf));
			while (FPP_ERR_OK == err)
			{
				printf("Domain: %03d ", ntohs(bd_reply_buf.vlan));

				if (ntohl(bd_reply_buf.flags) & FPP_L2BR_DOMAIN_DEFAULT)
				{
					printf("[default]");
				}

				if (ntohl(bd_reply_buf.flags) & FPP_L2BR_DOMAIN_FALLBACK)
				{
					printf("[fallback]");
				}

				printf("\n\tUnicast Action  : %d/%d (hit/miss)", bd_reply_buf.ucast_hit, bd_reply_buf.ucast_miss);
				printf("\n\tMulticast Action: %d/%d (hit/miss)", bd_reply_buf.mcast_hit, bd_reply_buf.mcast_miss);

				printf("\n\tTagged ports    : ");
				for (ii=0; ii<8*sizeof(bd_reply_buf.if_list); ii++)
				{
					if (ntohl(bd_reply_buf.if_list) & (1 << ii))
					{
						printf("%d ", ii);
					}
				}

				printf("\n\tUn-Tagged ports : ");
				for (ii=0; ii<8*sizeof(bd_reply_buf.untag_if_list); ii++)
				{
					if (ntohl(bd_reply_buf.if_list) & (1 << ii))
					{
						printf("%d ", ii);
					}
				}

				printf("\n");

				fci_cmd_buf.br_domain_ctrl.action = FPP_ACTION_QUERY_CONT;
				err = fci_query(fcicl, FPP_CMD_L2BRIDGE_DOMAIN, sizeof(fpp_l2_bridge_domain_control_cmd_t), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&bd_reply_buf));
				cnt++;
			}

			if (0 == cnt)
			{
				printf("No L2 Bridge domains\n\n");
			}
			else
			{
				printf("Found %d L2 Bridge domains\n\n", cnt);
			}


			break;
		}
        case CMD_FP_DEL_RULE:
        case CMD_FP_ADD_RULE:
        {
			err = fci_write(fcicl, FPP_CMD_FP_RULE, sizeof(fpp_flexible_parser_rule_cmd), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_FP_RULE: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_FP_RULE: Command sent\n");
			}
            break;
        }
        case CMD_FP_DEL_TABLE:
        case CMD_FP_ADD_TABLE:
        {
			err = fci_write(fcicl, FPP_CMD_FP_TABLE, sizeof(fpp_flexible_parser_table_cmd), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_FP_RULE: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_FP_RULE: Command sent\n");
			}
            break;
        }
        case CMD_FP_UNSET_TABLE_RULE:
        case CMD_FP_SET_TABLE_RULE:
        {
			err = fci_write(fcicl, FPP_CMD_FP_TABLE, sizeof(fpp_flexible_parser_table_cmd), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_CMD_FP_RULE: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_CMD_FP_RULE: Command sent\n");
			}
            break;
        }
        case CMD_FP_PRINT_TABLE:
        {
            fpp_flexible_parser_table_cmd reply_buf;
            unsigned short reply_len;
            unsigned int i = 0U;

			err = fci_query(fcicl, FPP_CMD_FP_TABLE, sizeof(fpp_flexible_parser_table_cmd), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&reply_buf));
            printf("\n");
            while(FPP_ERR_OK == err)
            {
                /* Rule and it's position in the table */
                printf("%u: %s = {", i++, reply_buf.r.rule_name);
                if(true == reply_buf.r.invert)
                {
                    printf("!");
                }
                /* Conditions */
                printf("(0x%x & 0x%x == ", ntohl(reply_buf.r.data), ntohl(reply_buf.r.mask));
                if(FP_OFFSET_FROM_L4_HEADER == reply_buf.r.offset_from)
                {
                    printf("frame[L4 header + %u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                if(FP_OFFSET_FROM_L3_HEADER == reply_buf.r.offset_from)
                {
                    printf("frame[L3 header + %u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                else
                {
                    printf("frame[%u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                /* Consequences */
                if(FP_ACCEPT == reply_buf.r.match_action)
                {
                    printf("? ACCEPT : use next rule");
                }
                else if(FP_REJECT == reply_buf.r.match_action)
                {
                    printf("? REJECT : use next rule");
                }
                else
                {
                    printf("? use rule %s : use next rule", reply_buf.r.next_rule_name);
                }
                printf("}\n");

                fci_cmd_buf.fp_rule_cmd.action = FPP_ACTION_QUERY_CONT;
                err = fci_query(fcicl, FPP_CMD_FP_TABLE, sizeof(fpp_flexible_parser_table_cmd), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&reply_buf));
            }

            break;
        }
        case CMD_FP_PRINT_RULES:
        {
            fpp_flexible_parser_rule_cmd reply_buf;
            unsigned short reply_len;

			err = fci_query(fcicl, FPP_CMD_FP_RULE, sizeof(fpp_flexible_parser_rule_cmd), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&reply_buf));
            printf("\n");
            while(FPP_ERR_OK == err)
            {
                /* Rule */
                printf("%s = {", reply_buf.r.rule_name);
                if(true == reply_buf.r.invert)
                {
                    printf("!");
                }
                /* Conditions */
                printf("(0x%x & 0x%x == ", ntohl(reply_buf.r.data), ntohl(reply_buf.r.mask));
                if(FP_OFFSET_FROM_L4_HEADER == reply_buf.r.offset_from)
                {
                    printf("frame[L4 header + %u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                if(FP_OFFSET_FROM_L3_HEADER == reply_buf.r.offset_from)
                {
                    printf("frame[L3 header + %u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                else
                {
                    printf("frame[%u] & 0x%08x)", ntohs(reply_buf.r.offset), ntohl(reply_buf.r.mask));
                }
                /* Consequences */
                if(FP_ACCEPT == reply_buf.r.match_action)
                {
                    printf("? ACCEPT : use next rule");
                }
                else if(FP_REJECT == reply_buf.r.match_action)
                {
                    printf("? REJECT : use next rule");
                }
                else
                {
                    printf("? use rule %s : use next rule", reply_buf.r.next_rule_name);
                }
                printf("}\n");

                fci_cmd_buf.fp_rule_cmd.action = FPP_ACTION_QUERY_CONT;
                err = fci_query(fcicl, FPP_CMD_FP_RULE, sizeof(fpp_flexible_parser_table_cmd), (unsigned short *)&fci_cmd_buf, &reply_len, (unsigned short *)(&reply_buf));
            };
            break;
        }
        case CMD_FP_FLEXIBLE_PARSER:
        {
            err = fci_write(fcicl, FPP_FP_CMD_FLEXIBLE_FILTER, sizeof(fpp_flexible_filter_cmd), (unsigned short *)&fci_cmd_buf);
			if (err != FPP_ERR_OK)
			{
				printf("FPP_FP_CMD_FLEXIBLE_FILTER: fci_write() failed: %d\n", err);
			}
			else
			{
				printf("FPP_FP_CMD_FLEXIBLE_FILTER: Command sent\n");
			}
            break;
        }

		case CMD_LOG_IF_UPDATE:
		{
			/* Lock interfaces for exclusive acces from FCI */
			err = fci_write(fcicl, FPP_CMD_IF_LOCK_SESSION, 0U, NULL);

			if(FPP_ERR_OK == err)
			{
				fci_cmd_buf.log_if_cmd.action =  FPP_ACTION_UPDATE;
				err = fci_write(fcicl, FPP_CMD_LOG_INTERFACE, sizeof(fpp_log_if_cmd_t), (unsigned short *)&fci_cmd_buf);

				/* Unlock interface */
				err = fci_write(fcicl, FPP_CMD_IF_UNLOCK_SESSION, 0U, NULL);
			}
			else
			{
				puts("Was not possible to lock the interfacses");
			}
			break;
		}

		case CMD_IF_PRINT_ALL:
		{
			fpp_phy_if_cmd_t if_cmd_reply;
			unsigned short reply_len;

			/* Lock interfaces for exclusive acces from FCI */
			err = fci_write(fcicl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);

			/* Query the interface and parse the results */
			fci_cmd_buf.phy_if_cmd.action = FPP_ACTION_QUERY;
			reply_len = sizeof(fci_cmd_buf.phy_if_cmd);
			err = fci_query(fcicl, FPP_CMD_PHY_INTERFACE,
							sizeof(fci_cmd_buf.phy_if_cmd),
							(unsigned short *)&fci_cmd_buf.phy_if_cmd, &reply_len,
							(unsigned short *)(&if_cmd_reply));

			while (FPP_ERR_OK == err)
			{
				err = phy_if_reply_parse(fcicl, &if_cmd_reply);

				fci_cmd_buf.phy_if_cmd.action = FPP_ACTION_QUERY_CONT;
				err = fci_query(fcicl, FPP_CMD_PHY_INTERFACE,
								sizeof(fci_cmd_buf.phy_if_cmd),
								(unsigned short *)&fci_cmd_buf.phy_if_cmd, &reply_len,
								(unsigned short *)(&if_cmd_reply));
			}

			/* Unlock interface */
			err = fci_write(fcicl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);

			break;
		}

		default:
		{
			printf("Unknown command: %d\n", icmd);
			break;
		}
	}

	/*	Close the FCI */
	if (fci_close(fcicl) < 0)
	{
		printf("fci_close() failed\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
