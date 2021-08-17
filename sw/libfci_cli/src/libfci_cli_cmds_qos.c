/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
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

#include "libfci_interface/fci_qos.h"

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
    
    printf("%-*squeue %"PRIu8":\n", indent, "", (p_que->id));
    
    indent += 4;
    
    printf("%-*sinterface: %s\n"
           "%-*sque-mode:  %"PRIu8" (%s)\n"
           "%-*sthld-min:  %"PRIu32"\n"
           "%-*sthld-max:  %"PRIu32"\n",
           indent, "", (p_que->if_name),
           indent, "", (p_que->mode), cli_value2txt_que_mode(p_que->mode),
           indent, "", (p_que->min),
           indent, "", (p_que->max));
    
    {
        printf("%-*szprob:     ", indent, "");
        const char* p_txt_delim = "";  /* no delim in front of the first item */
        for (uint8_t i = 0u; (ZPROBS_LN > i); (++i))
        {
            printf("%s[%"PRIu8"]<%"PRIu8">", p_txt_delim, i, p_que->zprob[i]);
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
    
    printf("%-*sscheduler %"PRIu8":\n", indent, "", (p_sch->id));
    
    indent += 4;
    
    printf("%-*sinterface: %s\n"
           "%-*ssch-mode:  %"PRIu8" (%s)\n"
           "%-*ssch-algo:  %"PRIu8" (%s)\n",
           indent, "", (p_sch->if_name),
           indent, "", (p_sch->mode), cli_value2txt_sch_mode(p_sch->mode),
           indent, "", (p_sch->algo), cli_value2txt_sch_algo(p_sch->algo));
    
    {
        printf("%-*ssch-in:    ", indent, "");
        const char* p_txt_delim = "";  /* no delim in front of the first item */
        for (uint8_t i = 0u; (SCH_INS_LN > i); (++i))
        {
            const char* p_txt = cli_value2txt_sch_in(p_sch->input_src[i]);
            if (fci_qos_sch_ld_is_input_enabled(p_sch, i))
            {
                printf("%s[%"PRIu8"]<%s:%"PRIu32">", p_txt_delim, i, p_txt, (p_sch->input_w[i]));
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
    
    printf("%-*sshaper %"PRIu8":\n", indent, "", (p_shp->id));
    
    indent += 4;
    
    printf("%-*sinterface:  %s\n"
           "%-*sshp-mode:   %"PRIu8" (%s)\n"
           "%-*sshp-pos:    %"PRIu8" (%s)\n"
           "%-*sisl:        %"PRIu32"\n"
           "%-*scredit-min: %"PRId32"\n"
           "%-*scredit-max: %"PRId32"\n",
           indent, "", (p_shp->if_name),
           indent, "", (p_shp->mode), cli_value2txt_shp_mode(p_shp->mode),
           indent, "", (p_shp->position), cli_value2txt_shp_pos(p_shp->position),
           indent, "", (p_shp->isl),
           indent, "", (p_shp->min_credit),
           indent, "", (p_shp->max_credit));
    
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
    const mandopt_t mandopts[] = {{OPT_INTERFACE,  NULL,  (p_cmdargs->if_name.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print single QoS queue */
            rtn = fci_qos_que_get_by_id(cli_p_cl, &que, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_que_print(&que);
            }
        }
        else
        {
            /* print all QoS queues of the given interface */
            rtn = fci_qos_que_print_by_phyif(cli_p_cl, qos_que_print, (p_cmdargs->if_name.txt));
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
        rtn = fci_qos_que_get_by_id(cli_p_cl, &que, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->que_sch_shp_mode.is_valid))
        {
            rtn = fci_qos_que_ld_set_mode(&que, (p_cmdargs->que_sch_shp_mode.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->thmin.is_valid))
        {
            rtn = fci_qos_que_ld_set_min(&que, (p_cmdargs->thmin.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->thmax.is_valid))
        {
            rtn = fci_qos_que_ld_set_max(&que, (p_cmdargs->thmax.value));
        }
    }
    
    /* modify local data - zprob elements */
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->zprob.is_valid)))
    {
        for (uint8_t i = 0u; (ZPROBS_LN > i); (++i))
        {
            const uint8_t cmdarg_value = (p_cmdargs->zprob.arr[i]);
            if (cli_que_zprob_is_not_keep(cmdarg_value))
            {
                fci_qos_que_ld_set_zprob(&que, i, cmdarg_value);
            }
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_que_update(cli_p_cl, &que);
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
    const mandopt_t mandopts[] = {{OPT_INTERFACE,  NULL,  (p_cmdargs->if_name.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print single QoS scheduler */
            rtn = fci_qos_sch_get_by_id(cli_p_cl, &sch, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_sch_print(&sch);
            }
        }
        else
        {
            /* print all QoS schedulers of the given interface */
            rtn = fci_qos_sch_print_by_phyif(cli_p_cl, qos_sch_print, (p_cmdargs->if_name.txt));
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
        rtn = fci_qos_sch_get_by_id(cli_p_cl, &sch, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->que_sch_shp_mode.is_valid))
        {
            rtn = fci_qos_sch_ld_set_mode(&sch, (p_cmdargs->que_sch_shp_mode.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->sch_algo.is_valid))
        {
            rtn = fci_qos_sch_ld_set_algo(&sch, (p_cmdargs->sch_algo.value));
        }
    }
    
    /* modify local data - scheduler inputs */
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->sch_in.is_valid)))
    {
        for (uint8_t i = 0u; (SCH_INS_LN > i); (++i))
        {
            const uint8_t cmdarg_src = (p_cmdargs->sch_in.arr_src[i]);
            const uint32_t cmdarg_w  = (p_cmdargs->sch_in.arr_w[i]);
            if (cli_que_zprob_is_not_keep(cmdarg_src))
            {
                const bool enable = cli_sch_in_is_not_dis(cmdarg_src);
                fci_qos_sch_ld_set_input(&sch, i, enable, cmdarg_src, cmdarg_w);
            }
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_sch_update(cli_p_cl, &sch);
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
    const mandopt_t mandopts[] = {{OPT_INTERFACE,  NULL,  (p_cmdargs->if_name.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print single QoS shaper */
            rtn = fci_qos_shp_get_by_id(cli_p_cl, &shp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_shp_print(&shp);
            }
        }
        else
        {
            /* print all QoS schedulers of the given interface */
            rtn = fci_qos_shp_print_by_phyif(cli_p_cl, qos_shp_print, (p_cmdargs->if_name.txt));
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
        rtn = fci_qos_shp_get_by_id(cli_p_cl, &shp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->que_sch_shp_mode.is_valid))
        {
            rtn = fci_qos_shp_ld_set_mode(&shp, (p_cmdargs->que_sch_shp_mode.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->shp_pos.is_valid))
        {
            rtn = fci_qos_shp_ld_set_position(&shp, (p_cmdargs->shp_pos.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->isl.is_valid))
        {
            rtn = fci_qos_shp_ld_set_isl(&shp, (p_cmdargs->isl.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->crmin.is_valid))
        {
            rtn = fci_qos_shp_ld_set_min_credit(&shp, (p_cmdargs->crmin.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->crmax.is_valid))
        {
            rtn = fci_qos_shp_ld_set_max_credit(&shp, (p_cmdargs->crmax.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_shp_update(cli_p_cl, &shp);
    }
    
    return (rtn);
}

/* ========================================================================= */
