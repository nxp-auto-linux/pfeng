/* =========================================================================
 *  Copyright 2020-2023 NXP
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

#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "../libfci_cli_common.h"
#include "../libfci_cli_def_optarg_keywords.h"


/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "demo_common.h"
#include "demo_rt_ct.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

#define TXT_STRINGIFY(SYMBOL)  #SYMBOL

static int fciev_snprintf(char**const pp_rtn_dst, int* p_rtn_dst_ln, const char* p_txt_fmt, ...);

static int fciev_print_ip_route(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload);
static int fciev_print_health_monitor_event(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload);
static int fciev_print_ipv4_conntrack_change(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload);
static int fciev_print_ipv6_conntrack_change(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload);

/* ==== PRIVATE FUNCTIONS : aux ============================================ */

/* print correct decoded data based on FCI event ID ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_payload_decoded(char**const pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short fcode, const unsigned short len, const unsigned short* payload)
{
    int rtn = -1;
    
    /* header */
    rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "payload_decoded = \n{\n");
    
    /* decoder selection */
    if (0 == rtn)
    {
        switch (fcode)
        {
            case FPP_CMD_ENDPOINT_SHUTDOWN:
                ;  /* no payload ; empty */
            break;
            
            case FPP_CMD_IP_ROUTE:
                rtn = fciev_print_ip_route(pp_rtn_dst, p_rtn_dst_ln, len, payload);
            break;
            
            case FPP_CMD_HEALTH_MONITOR_EVENT:
                rtn = fciev_print_health_monitor_event(pp_rtn_dst, p_rtn_dst_ln, len, payload);
            break;
            
            case FPP_CMD_IPV4_CONNTRACK_CHANGE:
                rtn = fciev_print_ipv4_conntrack_change(pp_rtn_dst, p_rtn_dst_ln, len, payload);
            break;
            
            case FPP_CMD_IPV6_CONNTRACK_CHANGE:
                rtn = fciev_print_ipv6_conntrack_change(pp_rtn_dst, p_rtn_dst_ln, len, payload);
            break;
            
            default:
                rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "  libfci_cli version " CLI_VERSION_STRING " cannot decode payload of this FCI event \n");
            break;
        }
    }
    
    /* footer */
    if (0 == rtn)
    {
        rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "}\n");
    }
    
    return (rtn);
}

/* conversion table for FCI event IDs ; event IDs are defined in FCI API header files */
static const char* fciev_fcode2txt(unsigned short fcode)
{
    char* p_txt = NULL; 
    switch (fcode)
    {
        case FPP_CMD_ENDPOINT_SHUTDOWN:
            p_txt = TXT_STRINGIFY(FPP_CMD_ENDPOINT_SHUTDOWN);
        break;
        
        case FPP_CMD_IP_ROUTE:
            p_txt = TXT_STRINGIFY(FPP_CMD_IP_ROUTE);
        break;
        
        case FPP_CMD_HEALTH_MONITOR_EVENT:
            p_txt = TXT_STRINGIFY(FPP_CMD_HEALTH_MONITOR_EVENT);
        break;
        
        case FPP_CMD_IPV4_CONNTRACK_CHANGE:
            p_txt = TXT_STRINGIFY(FPP_CMD_IPV4_CONNTRACK_CHANGE);
        break;
        
        case FPP_CMD_IPV6_CONNTRACK_CHANGE:
            p_txt = TXT_STRINGIFY(FPP_CMD_IPV6_CONNTRACK_CHANGE);
        break;
        
        default:
            p_txt = "---";
        break;
    }
    
    return (p_txt);
}

/* conversion table for FCI actions ; action IDs are defined in FCI API header files */
static const char* fciev_action2txt(uint16_t action)
{
    char* p_txt = NULL; 
    switch (action)
    {
        case FPP_ACTION_REGISTER:
            p_txt = TXT_STRINGIFY(FPP_ACTION_REGISTER);
        break;
        
        case FPP_ACTION_DEREGISTER:
            p_txt = TXT_STRINGIFY(FPP_ACTION_DEREGISTER);
        break;
        
        case FPP_ACTION_KEEP_ALIVE:
            p_txt = TXT_STRINGIFY(FPP_ACTION_KEEP_ALIVE);
        break;
        
        case FPP_ACTION_REMOVED:
            p_txt = TXT_STRINGIFY(FPP_ACTION_REMOVED);
        break;
        
        default:
            p_txt = "---";
        break;
    }
    
    return (p_txt);
}

/* print text and manipulate values of dst ptr and dst remaining free space */
static int fciev_snprintf(char**const pp_rtn_dst, int* p_rtn_dst_ln, const char* p_txt_fmt, ...)
{
    assert((NULL != pp_rtn_dst) && (NULL != *pp_rtn_dst));
    assert((NULL != p_rtn_dst_ln) && (0 != *p_rtn_dst_ln));
    
    int rtn = -1;
    int chars_written = 0;
    
    va_list args;
    va_start(args, p_txt_fmt);
    chars_written = vsnprintf(*pp_rtn_dst, *p_rtn_dst_ln, p_txt_fmt, args);
    va_end(args);
    
    if ((0 > chars_written) || (*p_rtn_dst_ln <= chars_written))
    {
        rtn = -1;
    }
    else
    {
        rtn = 0;
        *pp_rtn_dst += chars_written;
        *p_rtn_dst_ln -= chars_written;
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : printers ======================================= */

/* print header of the FCI event txt representation */
static int fciev_print_header(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short fcode, const unsigned short len)
{
    char txt_time[64] = {0};
    const time_t t = time(NULL);
    
    strftime(txt_time, sizeof(txt_time), "%c", localtime(&t));
    
    return fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, 
                "\n==== FCI_EVENT_beg =====================\n"
                "timestamp   = %-10lu (%s)\n"
                "fcode       = 0x%04X     (%s)\n"
                "len         = %hu\n",
                t, txt_time,
                fcode, fciev_fcode2txt(fcode),
                len
           );
}

/* print raw payload data ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_payload_raw(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload)
{
    int rtn = -1;
    
    const unsigned char* p = (const unsigned char*)(payload);
    
    /* header and first payload byte */
    rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "payload_raw = \n{\n  |%02X|", p[0]);
    
    /* payload bytes */
    if (0 == rtn)
    {
        for (uint16_t i = 1u; (i < len); (i++))
        {
            /* new line if applicable */
            if ((0 == rtn) && (0 == (i % 16u)))
            {
                rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "\n  |");
            }
            
            /* payload bytes */
            if (0 == rtn)
            {
                rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "%02X|", p[i]);
            }
        }
    }
    
    /* footer */
    if (0 == rtn)
    {
        rtn = fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln, "\n}\n");
    }
    
    return (rtn);
}

/* print decoded FPP_CMD_IP_ROUTE ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_ip_route(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload)
{
    UNUSED(len);  /* just to suppress gcc warning */
    
    const fpp_rt_cmd_t* p_rt = (fpp_rt_cmd_t*)(payload);
    
    return fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln,
                "  action = %"PRIu16" (%s)\n"
                "  id     = %"PRIu32"\n",
                p_rt->action, fciev_action2txt(p_rt->action),
                demo_rt_ld_get_route_id(p_rt)
           );
}

/* print decoded FPP_CMD_HEALTH_MONITOR_EVENT ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_health_monitor_event(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload)
{
    UNUSED(len);  /* just to suppress gcc warning */
    
    const fpp_health_monitor_cmd_t* p_hm = (fpp_health_monitor_cmd_t*)(payload);
    
    return fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln,
                "  id   = %-5"PRIu16"\n"
                "  type = %-5"PRIu8" (%s)\n"
                "  src  = %-5"PRIu8" (%s)\n"
                "  desc = %s\n",
                ntohs(p_hm->id),
                p_hm->type, cli_value2txt_hm_type(p_hm->type),
                p_hm->src, cli_value2txt_hm_src(p_hm->src),
                p_hm->desc
           );
}

/* print decoded FPP_CMD_IPV4_CONNTRACK_CHANGE ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_ipv4_conntrack_change(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload)
{
    UNUSED(len);  /* just to suppress gcc warning */
    
    const fpp_ct_cmd_t* p_ct = (fpp_ct_cmd_t*)(payload);
    
    const uint32_t s   = demo_ct_ld_get_saddr(p_ct);
    const uint8_t* p_s = (const uint8_t*)(&s);
    
    const uint32_t d   = demo_ct_ld_get_daddr(p_ct);
    const uint8_t* p_d = (const uint8_t*)(&d);
    
    const uint32_t sr   = demo_ct_ld_get_saddr_reply(p_ct);
    const uint8_t* p_sr = (const uint8_t*)(&sr);
    
    const uint32_t dr   = demo_ct_ld_get_daddr_reply(p_ct);
    const uint8_t* p_dr = (const uint8_t*)(&dr);
    
    return fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln,
                "  action                = %"PRIu16" (%s)\n"
                "  saddr                 = %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\n"
                "  daddr                 = %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\n"
                "  sport                 = %"PRIu16"\n"
                "  dport                 = %"PRIu16"\n"
                "  saddr_reply           = %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\n"
                "  daddr_reply           = %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"\n"
                "  sport_reply           = %"PRIu16"\n"
                "  dport_reply           = %"PRIu16"\n"
                "  protocol              = %"PRIu16"\n"
                "  flags                 = 0x%04"PRIx16"\n"
                "  route_id              = %"PRIu32"\n"
                "  route_id_reply        = %"PRIu32"\n"
                "  vlan                  = %"PRIu16"\n"
                "  vlan_reply            = %"PRIu16"\n"
                "  stats.hit             = %"PRIu32"\n"
                "  stats.hit_bytes       = %"PRIu32"\n"
                "  stats_reply.hit       = %"PRIu32"\n"
                "  stats_reply.hit_bytes = %"PRIu32"\n",
                p_ct->action, fciev_action2txt(p_ct->action),
                p_s[3],p_s[2],p_s[1],p_s[0],
                p_d[3],p_d[2],p_d[1],p_d[0],
                demo_ct_ld_get_sport(p_ct),
                demo_ct_ld_get_dport(p_ct),
                p_sr[3],p_sr[2],p_sr[1],p_sr[0],
                p_dr[3],p_dr[2],p_dr[1],p_dr[0],
                demo_ct_ld_get_sport_reply(p_ct),
                demo_ct_ld_get_dport_reply(p_ct),
                demo_ct_ld_get_protocol(p_ct),
                demo_ct_ld_get_flags(p_ct),
                demo_ct_ld_get_route_id(p_ct),
                demo_ct_ld_get_route_id_reply(p_ct),
                demo_ct_ld_get_vlan(p_ct),
                demo_ct_ld_get_vlan_reply(p_ct),
                demo_ct_ld_get_stt_hit(p_ct),
                demo_ct_ld_get_stt_hit_bytes(p_ct),
                demo_ct_ld_get_stt_reply_hit(p_ct),
                demo_ct_ld_get_stt_reply_hit_bytes(p_ct)
           );
}

/* print decoded FPP_CMD_IPV6_CONNTRACK_CHANGE ; NOTE: non-zero len and non-NULL payload are assumed */
static int fciev_print_ipv6_conntrack_change(char** pp_rtn_dst, int* p_rtn_dst_ln, const unsigned short len, const unsigned short* payload)
{
    UNUSED(len);  /* just to suppress gcc warning */
    
    const fpp_ct6_cmd_t* p_ct6 = (fpp_ct6_cmd_t*)(payload);
    
    const uint32_t* p_s32 = demo_ct6_ld_get_saddr(p_ct6);
    const uint8_t*  p_s   = (const uint8_t*)(p_s32);
    
    const uint32_t* p_d32 = demo_ct6_ld_get_daddr(p_ct6);
    const uint8_t*  p_d   = (const uint8_t*)(p_d32);
    
    const uint32_t* p_sr32 = demo_ct6_ld_get_saddr_reply(p_ct6);
    const uint8_t*  p_sr   = (const uint8_t*)(p_sr32);
    
    const uint32_t* p_dr32 = demo_ct6_ld_get_daddr_reply(p_ct6);
    const uint8_t*  p_dr   = (const uint8_t*)(p_dr32);
    
    return fciev_snprintf(pp_rtn_dst, p_rtn_dst_ln,
                "  action                = %"PRIu16" (%s)\n"
                "  saddr                 = %02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8"\n"
                "  daddr                 = %02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8"\n"
                "  sport                 = %"PRIu16"\n"
                "  dport                 = %"PRIu16"\n"
                "  saddr_reply           = %02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8"\n"
                "  daddr_reply           = %02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8":%02"PRIx8"%02"PRIx8"\n"
                "  sport_reply           = %"PRIu16"\n"
                "  dport_reply           = %"PRIu16"\n"
                "  protocol              = %"PRIu16"\n"
                "  flags                 = 0x%04"PRIx16"\n"
                "  route_id              = %"PRIu32"\n"
                "  route_id_reply        = %"PRIu32"\n"
                "  vlan                  = %"PRIu16"\n"
                "  vlan_reply            = %"PRIu16"\n"
                "  stats.hit             = %"PRIu32"\n"
                "  stats.hit_bytes       = %"PRIu32"\n"
                "  stats_reply.hit       = %"PRIu32"\n"
                "  stats_reply.hit_bytes = %"PRIu32"\n",
                p_ct6->action, fciev_action2txt(p_ct6->action),
                p_s[3],p_s[2],p_s[1],p_s[0], p_s[7],p_s[6],p_s[5],p_s[4], p_s[11],p_s[10],p_s[9],p_s[8], p_s[15],p_s[14],p_s[13],p_s[12],
                p_d[3],p_d[2],p_d[1],p_d[0], p_d[7],p_d[6],p_d[5],p_d[4], p_d[11],p_d[10],p_d[9],p_d[8], p_d[15],p_d[14],p_d[13],p_d[12],
                demo_ct6_ld_get_sport(p_ct6),
                demo_ct6_ld_get_dport(p_ct6),
                p_sr[3],p_sr[2],p_sr[1],p_sr[0], p_sr[7],p_sr[6],p_sr[5],p_sr[4], p_sr[11],p_sr[10],p_sr[9],p_sr[8], p_sr[15],p_sr[14],p_sr[13],p_sr[12],
                p_dr[3],p_dr[2],p_dr[1],p_dr[0], p_dr[7],p_dr[6],p_dr[5],p_dr[4], p_dr[11],p_dr[10],p_dr[9],p_dr[8], p_dr[15],p_dr[14],p_dr[13],p_dr[12],
                demo_ct6_ld_get_sport_reply(p_ct6),
                demo_ct6_ld_get_dport_reply(p_ct6),
                demo_ct6_ld_get_protocol(p_ct6),
                demo_ct6_ld_get_flags(p_ct6),
                demo_ct6_ld_get_route_id(p_ct6),
                demo_ct6_ld_get_route_id_reply(p_ct6),
                demo_ct6_ld_get_vlan(p_ct6),
                demo_ct6_ld_get_vlan_reply(p_ct6),
                demo_ct6_ld_get_stt_hit(p_ct6),
                demo_ct6_ld_get_stt_hit_bytes(p_ct6),
                demo_ct6_ld_get_stt_reply_hit(p_ct6),
                demo_ct6_ld_get_stt_reply_hit_bytes(p_ct6)
           );
}

/* ==== PUBLIC FUNCTIONS =================================================== */ 

/* print txt representation of FCI event */
int daemon_fciev2txt_print(char* p_dst, int dst_ln, const unsigned short fcode, const unsigned short len, const unsigned short* payload)
{
    assert(NULL != p_dst);
    assert(0 != dst_ln);
    /* payload is allowed to be NULL (arrives from 3rd party, so everything is possible) */
    
    int rtn = -1;
    
    /* header */
    rtn = fciev_print_header(&p_dst, &dst_ln, fcode, len);
    
    /* payload (if applicable) */
    if ((0 != len) && (NULL != payload))
    {
        /* print raw payload data */
        if (0 == rtn)
        {
            rtn = fciev_print_payload_raw(&p_dst, &dst_ln, len, payload);
        }
        
        /* print decoded payload data */
        if (0 == rtn)
        {
            rtn = fciev_print_payload_decoded(&p_dst, &dst_ln, fcode, len, payload);
        }
    }
    
    /* footer */
    if (0 == rtn)
    {
        rtn = fciev_snprintf(&p_dst, &dst_ln, "==== FCI_EVENT_end =====================\n");
    }
    
    return (rtn);
}

/* ========================================================================= */
