/* =========================================================================
 *  Copyright 2020-2021 NXP
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
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_qos.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_qos.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static int qos_que_print(const fpp_qos_queue_cmd_t* p_que)
{
    assert(NULL != p_que);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*squeue %"PRIu8":\n", indent, "", demo_qos_que_ld_get_id(p_que));
    
    indent += 4;
    
    printf("%-*sinterface: %s\n"
           "%-*sque-mode:  %"PRIu8" (%s)\n"
           "%-*sthld-min:  %"PRIu32"\n"
           "%-*sthld-max:  %"PRIu32"\n",
           indent, "", demo_qos_que_ld_get_if_name(p_que),
           indent, "", demo_qos_que_ld_get_mode(p_que), cli_value2txt_que_mode(demo_qos_que_ld_get_mode(p_que)),
           indent, "", demo_qos_que_ld_get_min(p_que),
           indent, "", demo_qos_que_ld_get_max(p_que));
    
    {
        printf("%-*szprob:     ", indent, "");
        const char* p_txt_delim = "";  /* no delim in front of the first item */
        for (uint8_t i = 0u; (ZPROBS_LN > i); (++i))
        {
            printf("%s[%"PRIu8"]<%"PRIu8">", p_txt_delim, i, demo_qos_que_ld_get_zprob_by_id(p_que, i));
            p_txt_delim = ",";
        }
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

static int qos_sch_print(const fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(NULL != p_sch);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*sscheduler %"PRIu8":\n", indent, "", demo_qos_sch_ld_get_id(p_sch));
    
    indent += 4;
    
    const uint8_t sch_mode = demo_qos_sch_ld_get_mode(p_sch);
    const uint8_t sch_algo = demo_qos_sch_ld_get_algo(p_sch);
    printf("%-*sinterface: %s\n"
           "%-*ssch-mode:  %"PRIu8" (%s)\n"
           "%-*ssch-algo:  %"PRIu8" (%s)\n",
           indent, "", demo_qos_sch_ld_get_if_name(p_sch),
           indent, "", (sch_mode), cli_value2txt_sch_mode(sch_mode),
           indent, "", (sch_algo), cli_value2txt_sch_algo(sch_algo));
    
    {
        printf("%-*ssch-in:    ", indent, "");
        const char* p_txt_delim = "";  /* no delim in front of the first item */
        for (uint8_t i = 0u; (SCH_INS_LN > i); (++i))
        {
            const char* p_txt = cli_value2txt_sch_in(demo_qos_sch_ld_get_input_src(p_sch, i));
            if (demo_qos_sch_ld_is_input_enabled(p_sch, i))
            {
                printf("%s[%"PRIu8"]<%s:%"PRIu32">", p_txt_delim, i, p_txt, demo_qos_sch_ld_get_input_weight(p_sch, i));
            }
            else
            {
                printf("%s[%"PRIu8"]<%s>", p_txt_delim, i, p_txt);
            }
            p_txt_delim = ",";
        }
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

static int qos_shp_print(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(NULL != p_shp);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*sshaper %"PRIu8":\n", indent, "", demo_qos_shp_ld_get_id(p_shp));
    
    indent += 4;
    
    printf("%-*sinterface:  %s\n"
           "%-*sshp-mode:   %"PRIu8" (%s)\n"
           "%-*sshp-pos:    %"PRIu8" (%s)\n"
           "%-*sisl:        %"PRIu32"\n"
           "%-*scredit-min: %"PRId32"\n"
           "%-*scredit-max: %"PRId32"\n",
           indent, "", demo_qos_shp_ld_get_if_name(p_shp),
           indent, "", demo_qos_shp_ld_get_mode(p_shp), cli_value2txt_shp_mode(demo_qos_shp_ld_get_mode(p_shp)),
           indent, "", demo_qos_shp_ld_get_position(p_shp), cli_value2txt_shp_pos(demo_qos_shp_ld_get_position(p_shp)),
           indent, "", demo_qos_shp_ld_get_isl(p_shp),
           indent, "", demo_qos_shp_ld_get_min_credit(p_shp),
           indent, "", demo_qos_shp_ld_get_max_credit(p_shp));
    
    return (FPP_ERR_OK); 
}

/* ==== PUBLIC FUNCTIONS : QoS queue ======================================= */

int cli_cmd_qos_que_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_queue_cmd_t que = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print a single QoS queue */
            rtn = demo_qos_que_get_by_id(cli_p_cl, &que, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_que_print(&que);
            }
        }
        else
        {
            /* print all QoS queues of the given interface */
            rtn = demo_qos_que_print_by_phyif(cli_p_cl, qos_que_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_que_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_queue_cmd_t que = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_QUE,       NULL, (p_cmdargs->que_sch_shp.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_que_get_by_id(cli_p_cl, &que, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp_mode.is_valid)
        {
            demo_qos_que_ld_set_mode(&que, (p_cmdargs->que_sch_shp_mode.value));
        }
        if (p_cmdargs->thmin.is_valid)
        {
            demo_qos_que_ld_set_min(&que, (p_cmdargs->thmin.value));
        }
        if (p_cmdargs->thmax.is_valid)
        {
            demo_qos_que_ld_set_max(&que, (p_cmdargs->thmax.value));
        }
    }
    
    /* modify local data - zprob elements */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->zprob.is_valid))
    {
        for (uint8_t i = 0u; (ZPROBS_LN > i); (++i))
        {
            const uint8_t cmdarg_value = (p_cmdargs->zprob.arr[i]);
            if (cli_que_zprob_is_not_keep(cmdarg_value))
            {
                demo_qos_que_ld_set_zprob(&que, i, cmdarg_value);
            }
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_que_update(cli_p_cl, &que);
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : QoS scheduler =================================== */

int cli_cmd_qos_sch_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_scheduler_cmd_t sch = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print a single QoS scheduler */
            rtn = demo_qos_sch_get_by_id(cli_p_cl, &sch, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_sch_print(&sch);
            }
        }
        else
        {
            /* print all QoS schedulers of the given interface */
            rtn = demo_qos_sch_print_by_phyif(cli_p_cl, qos_sch_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_sch_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_scheduler_cmd_t sch = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_SCH,       NULL, (p_cmdargs->que_sch_shp.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_sch_get_by_id(cli_p_cl, &sch, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp_mode.is_valid)
        {
            demo_qos_sch_ld_set_mode(&sch, (p_cmdargs->que_sch_shp_mode.value));
        }
        if (p_cmdargs->sch_algo.is_valid)
        {
            demo_qos_sch_ld_set_algo(&sch, (p_cmdargs->sch_algo.value));
        }
    }
    
    /* modify local data - scheduler inputs */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->sch_in.is_valid))
    {
        for (uint8_t i = 0u; (SCH_INS_LN > i); (++i))
        {
            const uint8_t cmdarg_src = (p_cmdargs->sch_in.arr_src[i]);
            const uint32_t cmdarg_w  = (p_cmdargs->sch_in.arr_w[i]);
            if (cli_que_zprob_is_not_keep(cmdarg_src))
            {
                const bool enable = cli_sch_in_is_not_dis(cmdarg_src);
                demo_qos_sch_ld_set_input(&sch, i, enable, cmdarg_src, cmdarg_w);
            }
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_sch_update(cli_p_cl, &sch);
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : QoS shaper ====================================== */

int cli_cmd_qos_shp_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_shaper_cmd_t shp = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print a single QoS shaper */
            rtn = demo_qos_shp_get_by_id(cli_p_cl, &shp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_shp_print(&shp);
            }
        }
        else
        {
            /* print all QoS schedulers of the given interface */
            rtn = demo_qos_shp_print_by_phyif(cli_p_cl, qos_shp_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_shp_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_shaper_cmd_t shp = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_SHP,       NULL, (p_cmdargs->que_sch_shp.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_shp_get_by_id(cli_p_cl, &shp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp_mode.is_valid)
        {
            demo_qos_shp_ld_set_mode(&shp, (p_cmdargs->que_sch_shp_mode.value));
        }
        if (p_cmdargs->shp_pos.is_valid)
        {
            demo_qos_shp_ld_set_position(&shp, (p_cmdargs->shp_pos.value));
        }
        if (p_cmdargs->isl.is_valid)
        {
            demo_qos_shp_ld_set_isl(&shp, (p_cmdargs->isl.value));
        }
        if (p_cmdargs->crmin.is_valid)
        {
            demo_qos_shp_ld_set_min_credit(&shp, (p_cmdargs->crmin.value));
        }
        if (p_cmdargs->crmax.is_valid)
        {
            demo_qos_shp_ld_set_max_credit(&shp, (p_cmdargs->crmax.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_shp_update(cli_p_cl, &shp);
    }
    
    return (rtn);
}

/* ========================================================================= */
