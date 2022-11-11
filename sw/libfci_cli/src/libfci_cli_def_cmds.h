/* =========================================================================
 *  Copyright 2020-2022 NXP
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

#ifndef CLI_DEF_CMDS_H_
#define CLI_DEF_CMDS_H_

#include <stdint.h>
#include <stdbool.h>

/* ==== DEFINITIONS ======================================================== */
/*
    For each cli command, fill the necessary info here.
    Do NOT include any *.h files. It should not be needed.

    CMD_00_NO_COMMAND is hardcoded.

    Search for keyword 'CMD_LAST' to get to the bottom of the cli command definition list.


    Description of a cli cmd definition (xx is a number from 01 to 99)
    ------------------------------------------------------------------
    CMD_xx_ENUM_NAME    CMD_MY_TEST        This enum key is automatically created and associated
                                           with the given cli command.

    CMD_xx_CLI_TXT      "my-test"          Command-line text which represents the given cli command.

    CMD_xx_CMDEXEC      my_test            Name of a global function which is called when this command is invoked.
                                           The global function itself can be defined in any source file.
                                           The global function must conform to the prototype:
                                           int my_test(const cli_cmdargs_t* p_cmdargs);

    CMD_xx_HELP         txt_help_mytest    Name of a text array which represents a help text for this cli command.
                                           The array needs to be defined in the source file 'libfci_cli_def_help.c'.
*/

#define CMD_01_ENUM_NAME    CMD_PHYIF_PRINT
#define CMD_01_CLI_TXT      "phyif-print"
#define CMD_01_CMDEXEC      cli_cmd_phyif_print
#define CMD_01_HELP         txt_help_phyif_print

#define CMD_02_ENUM_NAME    CMD_PHYIF_UPDATE
#define CMD_02_CLI_TXT      "phyif-update"
#define CMD_02_CMDEXEC      cli_cmd_phyif_update
#define CMD_02_HELP         txt_help_phyif_update

#define CMD_03_ENUM_NAME    CMD_PHYIF_MAC_PRINT
#define CMD_03_CLI_TXT      "phyif-mac-print"
#define CMD_03_CMDEXEC      cli_cmd_phyif_mac_print
#define CMD_03_HELP         txt_help_phyif_mac_print

#define CMD_04_ENUM_NAME    CMD_PHYIF_MAC_ADD
#define CMD_04_CLI_TXT      "phyif-mac-add"
#define CMD_04_CMDEXEC      cli_cmd_phyif_mac_add
#define CMD_04_HELP         txt_help_phyif_mac_add

#define CMD_05_ENUM_NAME    CMD_PHYIF_MAC_DEL
#define CMD_05_CLI_TXT      "phyif-mac-del"
#define CMD_05_CMDEXEC      cli_cmd_phyif_mac_del
#define CMD_05_HELP         txt_help_phyif_mac_del

/*      CMD_06_ENUM_NAME    reserved for future PHYIF cmds */
/*      CMD_07_ENUM_NAME    reserved for future PHYIF cmds */
/*      CMD_08_ENUM_NAME    reserved for future PHYIF cmds */
/*      CMD_09_ENUM_NAME    reserved for future PHYIF cmds */

#define CMD_10_ENUM_NAME    CMD_LOGIF_PRINT
#define CMD_10_CLI_TXT      "logif-print"
#define CMD_10_CMDEXEC      cli_cmd_logif_print
#define CMD_10_HELP         txt_help_logif_print

#define CMD_11_ENUM_NAME    CMD_LOGIF_UPDATE
#define CMD_11_CLI_TXT      "logif-update"
#define CMD_11_CMDEXEC      cli_cmd_logif_update
#define CMD_11_HELP         txt_help_logif_update

#define CMD_12_ENUM_NAME    CMD_LOGIF_ADD
#define CMD_12_CLI_TXT      "logif-add"
#define CMD_12_CMDEXEC      cli_cmd_logif_add
#define CMD_12_HELP         txt_help_logif_add

#define CMD_13_ENUM_NAME    CMD_LOGIF_DEL
#define CMD_13_CLI_TXT      "logif-del"
#define CMD_13_CMDEXEC      cli_cmd_logif_del
#define CMD_13_HELP         txt_help_logif_del

/*      CMD_14_ENUM_NAME    reserved for future LOGIF cmds */

#define CMD_15_ENUM_NAME    CMD_MIRROR_PRINT
#define CMD_15_CLI_TXT      "mirror-print"
#define CMD_15_CMDEXEC      cli_cmd_mirror_print
#define CMD_15_HELP         txt_help_mirror_print

#define CMD_16_ENUM_NAME    CMD_MIRROR_UPDATE
#define CMD_16_CLI_TXT      "mirror-update"
#define CMD_16_CMDEXEC      cli_cmd_mirror_update
#define CMD_16_HELP         txt_help_mirror_update

#define CMD_17_ENUM_NAME    CMD_MIRROR_ADD
#define CMD_17_CLI_TXT      "mirror-add"
#define CMD_17_CMDEXEC      cli_cmd_mirror_add
#define CMD_17_HELP         txt_help_mirror_add

#define CMD_18_ENUM_NAME    CMD_MIRROR_DEL
#define CMD_18_CLI_TXT      "mirror-del"
#define CMD_18_CMDEXEC      cli_cmd_mirror_del
#define CMD_18_HELP         txt_help_mirror_del

/*      CMD_19_ENUM_NAME    reserved for future MIRROR cmds */

#define CMD_20_ENUM_NAME    CMD_BD_PRINT
#define CMD_20_CLI_TXT      "bd-print"
#define CMD_20_CMDEXEC      cli_cmd_bd_print
#define CMD_20_HELP         txt_help_bd_print

#define CMD_21_ENUM_NAME    CMD_BD_UPDATE
#define CMD_21_CLI_TXT      "bd-update"
#define CMD_21_CMDEXEC      cli_cmd_bd_update
#define CMD_21_HELP         txt_help_bd_update

#define CMD_22_ENUM_NAME    CMD_BD_ADD
#define CMD_22_CLI_TXT      "bd-add"
#define CMD_22_CMDEXEC      cli_cmd_bd_add
#define CMD_22_HELP         txt_help_bd_add

#define CMD_23_ENUM_NAME    CMD_BD_DEL
#define CMD_23_CLI_TXT      "bd-del"
#define CMD_23_CMDEXEC      cli_cmd_bd_del
#define CMD_23_HELP         txt_help_bd_del

#define CMD_24_ENUM_NAME    CMD_BD_INSIF
#define CMD_24_CLI_TXT      "bd-insif"
#define CMD_24_CMDEXEC      cli_cmd_bd_insif
#define CMD_24_HELP         txt_help_bd_insif

#define CMD_25_ENUM_NAME    CMD_BD_REMIF
#define CMD_25_CLI_TXT      "bd-remif"
#define CMD_25_CMDEXEC      cli_cmd_bd_remif
#define CMD_25_HELP         txt_help_bd_remif

#define CMD_26_ENUM_NAME    CMD_BD_FLUSH
#define CMD_26_CLI_TXT      "bd-flush"
#define CMD_26_CMDEXEC      cli_cmd_bd_flush
#define CMD_26_HELP         txt_help_bd_flush

#define CMD_27_ENUM_NAME    CMD_BD_STENT_PRINT
#define CMD_27_CLI_TXT      "bd-stent-print"
#define CMD_27_CMDEXEC      cli_cmd_bd_stent_print
#define CMD_27_HELP         txt_help_bd_stent_print

#define CMD_28_ENUM_NAME    CMD_BD_STENT_UPDATE
#define CMD_28_CLI_TXT      "bd-stent-update"
#define CMD_28_CMDEXEC      cli_cmd_bd_stent_update
#define CMD_28_HELP         txt_help_bd_stent_update

#define CMD_29_ENUM_NAME    CMD_BD_STENT_ADD
#define CMD_29_CLI_TXT      "bd-stent-add"
#define CMD_29_CMDEXEC      cli_cmd_bd_stent_add
#define CMD_29_HELP         txt_help_bd_stent_add

#define CMD_30_ENUM_NAME    CMD_BD_STENT_DEL
#define CMD_30_CLI_TXT      "bd-stent-del"
#define CMD_30_CMDEXEC      cli_cmd_bd_stent_del
#define CMD_30_HELP         txt_help_bd_stent_del

/*      CMD_31_ENUM_NAME    reserved for future BD cmds */
/*      CMD_32_ENUM_NAME    reserved for future BD cmds */
/*      CMD_33_ENUM_NAME    reserved for future BD cmds */
/*      CMD_34_ENUM_NAME    reserved for future BD cmds */
/*      CMD_35_ENUM_NAME    reserved for future BD cmds */
/*      CMD_36_ENUM_NAME    reserved for future BD cmds */
/*      CMD_37_ENUM_NAME    reserved for future BD cmds */
/*      CMD_38_ENUM_NAME    reserved for future BD cmds */
/*      CMD_39_ENUM_NAME    reserved for future BD cmds */

#define CMD_40_ENUM_NAME    CMD_FPTABLE_PRINT
#define CMD_40_CLI_TXT      "fptable-print"
#define CMD_40_CMDEXEC      cli_cmd_fptable_print
#define CMD_40_HELP         txt_help_fptable_print

#define CMD_41_ENUM_NAME    CMD_FPTABLE_ADD
#define CMD_41_CLI_TXT      "fptable-add"
#define CMD_41_CMDEXEC      cli_cmd_fptable_add
#define CMD_41_HELP         txt_help_fptable_add

#define CMD_42_ENUM_NAME    CMD_FPTABLE_DEL
#define CMD_42_CLI_TXT      "fptable-del"
#define CMD_42_CMDEXEC      cli_cmd_fptable_del
#define CMD_42_HELP         txt_help_fptable_del

#define CMD_43_ENUM_NAME    CMD_FPTABLE_INSRULE
#define CMD_43_CLI_TXT      "fptable-insrule"
#define CMD_43_CMDEXEC      cli_cmd_fptable_insrule
#define CMD_43_HELP         txt_help_fptable_insrule

#define CMD_44_ENUM_NAME    CMD_FPTABLE_REMRULE
#define CMD_44_CLI_TXT      "fptable-remrule"
#define CMD_44_CMDEXEC      cli_cmd_fptable_remrule
#define CMD_44_HELP         txt_help_fptable_remrule

#define CMD_45_ENUM_NAME    CMD_FPRULE_PRINT
#define CMD_45_CLI_TXT      "fprule-print"
#define CMD_45_CMDEXEC      cli_cmd_fprule_print
#define CMD_45_HELP         txt_help_fprule_print

#define CMD_46_ENUM_NAME    CMD_FPRULE_ADD
#define CMD_46_CLI_TXT      "fprule-add"
#define CMD_46_CMDEXEC      cli_cmd_fprule_add
#define CMD_46_HELP         txt_help_fprule_add

#define CMD_47_ENUM_NAME    CMD_FPRULE_DEL
#define CMD_47_CLI_TXT      "fprule-del"
#define CMD_47_CMDEXEC      cli_cmd_fprule_del
#define CMD_47_HELP         txt_help_fprule_del

/*      CMD_48_ENUM_NAME    reserved for future FP cmds */
/*      CMD_49_ENUM_NAME    reserved for future FP cmds */

#define CMD_50_ENUM_NAME    CMD_ROUTE_PRINT
#define CMD_50_CLI_TXT      "route-print"
#define CMD_50_CMDEXEC      cli_cmd_route_print
#define CMD_50_HELP         txt_help_route_print

#define CMD_51_ENUM_NAME    CMD_ROUTE_ADD
#define CMD_51_CLI_TXT      "route-add"
#define CMD_51_CMDEXEC      cli_cmd_route_add
#define CMD_51_HELP         txt_help_route_add

#define CMD_52_ENUM_NAME    CMD_ROUTE_DEL
#define CMD_52_CLI_TXT      "route-del"
#define CMD_52_CMDEXEC      cli_cmd_route_del
#define CMD_52_HELP         txt_help_route_del

#define CMD_53_ENUM_NAME    CMD_CNTK_PRINT
#define CMD_53_CLI_TXT      "cntk-print"
#define CMD_53_CMDEXEC      cli_cmd_cntk_print
#define CMD_53_HELP         txt_help_cntk_print

#define CMD_54_ENUM_NAME    CMD_CNTK_UPDATE
#define CMD_54_CLI_TXT      "cntk-update"
#define CMD_54_CMDEXEC      cli_cmd_cntk_update
#define CMD_54_HELP         txt_help_cntk_update

#define CMD_55_ENUM_NAME    CMD_CNTK_ADD
#define CMD_55_CLI_TXT      "cntk-add"
#define CMD_55_CMDEXEC      cli_cmd_cntk_add
#define CMD_55_HELP         txt_help_cntk_add

#define CMD_56_ENUM_NAME    CMD_CNTK_DEL
#define CMD_56_CLI_TXT      "cntk-del"
#define CMD_56_CMDEXEC      cli_cmd_cntk_del
#define CMD_56_HELP         txt_help_cntk_del

#define CMD_57_ENUM_NAME    CMD_CNTK_TIMEOUT
#define CMD_57_CLI_TXT      "cntk-timeout"
#define CMD_57_CMDEXEC      cli_cmd_cntk_timeout
#define CMD_57_HELP         txt_help_cntk_timeout

#define CMD_58_ENUM_NAME    CMD_ROUTE_AND_CNTK_RESET
#define CMD_58_CLI_TXT      "route-and-cntk-reset"
#define CMD_58_CMDEXEC      cli_cmd_route_and_cntk_reset
#define CMD_58_HELP         txt_help_route_and_cntk_reset

/*      CMD_59_ENUM_NAME    reserved for future RT & CNTK cmds */

#define CMD_60_ENUM_NAME    CMD_SPD_PRINT
#define CMD_60_CLI_TXT      "spd-print"
#define CMD_60_CMDEXEC      cli_cmd_spd_print
#define CMD_60_HELP         txt_help_spd_print

#define CMD_61_ENUM_NAME    CMD_SPD_ADD
#define CMD_61_CLI_TXT      "spd-add"
#define CMD_61_CMDEXEC      cli_cmd_spd_add
#define CMD_61_HELP         txt_help_spd_add

#define CMD_62_ENUM_NAME    CMD_SPD_DEL
#define CMD_62_CLI_TXT      "spd-del"
#define CMD_62_CMDEXEC      cli_cmd_spd_del
#define CMD_62_HELP         txt_help_spd_del

/*      CMD_63_ENUM_NAME    reserved for future SPD cmds */
/*      CMD_64_ENUM_NAME    reserved for future SPD cmds */

#define CMD_65_ENUM_NAME    CMD_FWFEAT_PRINT
#define CMD_65_CLI_TXT      "fwfeat-print"
#define CMD_65_CMDEXEC      cli_cmd_fwfeat_print
#define CMD_65_HELP         txt_help_fwfeat_print

#define CMD_66_ENUM_NAME    CMD_FWFEAT_SET
#define CMD_66_CLI_TXT      "fwfeat-set"
#define CMD_66_CMDEXEC      cli_cmd_fwfeat_set
#define CMD_66_HELP         txt_help_fwfeat_set
 
#define CMD_67_ENUM_NAME    CMD_FWFEAT_EL_PRINT
#define CMD_67_CLI_TXT      "fwfeat-el-print"
#define CMD_67_CMDEXEC      cli_cmd_fwfeat_el_print
#define CMD_67_HELP         txt_help_fwfeat_el_print

#define CMD_68_ENUM_NAME    CMD_FWFEAT_EL_SET
#define CMD_68_CLI_TXT      "fwfeat-el-set"
#define CMD_68_CMDEXEC      cli_cmd_fwfeat_el_set
#define CMD_68_HELP         txt_help_fwfeat_el_set

/*      CMD_69_ENUM_NAME    reserved for future FWFEAT cmds */

#define CMD_70_ENUM_NAME    CMD_QOS_QUE_PRINT
#define CMD_70_CLI_TXT      "qos-que-print"
#define CMD_70_CMDEXEC      cli_cmd_qos_que_print
#define CMD_70_HELP         txt_help_qos_que_print

#define CMD_71_ENUM_NAME    CMD_QOS_QUE_UPDATE
#define CMD_71_CLI_TXT      "qos-que-update"
#define CMD_71_CMDEXEC      cli_cmd_qos_que_update
#define CMD_71_HELP         txt_help_qos_que_update

#define CMD_72_ENUM_NAME    CMD_QOS_SCH_PRINT
#define CMD_72_CLI_TXT      "qos-sch-print"
#define CMD_72_CMDEXEC      cli_cmd_qos_sch_print
#define CMD_72_HELP         txt_help_qos_sch_print

#define CMD_73_ENUM_NAME    CMD_QOS_SCH_UPDATE
#define CMD_73_CLI_TXT      "qos-sch-update"
#define CMD_73_CMDEXEC      cli_cmd_qos_sch_update
#define CMD_73_HELP         txt_help_qos_sch_update

#define CMD_74_ENUM_NAME    CMD_QOS_SHP_PRINT
#define CMD_74_CLI_TXT      "qos-shp-print"
#define CMD_74_CMDEXEC      cli_cmd_qos_shp_print
#define CMD_74_HELP         txt_help_qos_shp_print

#define CMD_75_ENUM_NAME    CMD_QOS_SHP_UPDATE
#define CMD_75_CLI_TXT      "qos-shp-update"
#define CMD_75_CMDEXEC      cli_cmd_qos_shp_update
#define CMD_75_HELP         txt_help_qos_shp_update

/*      CMD_76_ENUM_NAME    reserved for future QOS cmds */
/*      CMD_77_ENUM_NAME    reserved for future QOS cmds */
/*      CMD_78_ENUM_NAME    reserved for future QOS cmds */
/*      CMD_79_ENUM_NAME    reserved for future QOS cmds */

#define CMD_80_ENUM_NAME    CMD_QOS_POL_PRINT
#define CMD_80_CLI_TXT      "qos-pol-print"
#define CMD_80_CMDEXEC      cli_cmd_qos_pol_print
#define CMD_80_HELP         txt_help_qos_pol_print

#define CMD_81_ENUM_NAME    CMD_QOS_POL_SET
#define CMD_81_CLI_TXT      "qos-pol-set"
#define CMD_81_CMDEXEC      cli_cmd_qos_pol_set
#define CMD_81_HELP         txt_help_qos_pol_set

#define CMD_82_ENUM_NAME    CMD_QOS_POL_WRED_PRINT
#define CMD_82_CLI_TXT      "qos-pol-wred-print"
#define CMD_82_CMDEXEC      cli_cmd_qos_pol_wred_print
#define CMD_82_HELP         txt_help_qos_pol_wred_print

#define CMD_83_ENUM_NAME    CMD_QOS_POL_WRED_UPDATE
#define CMD_83_CLI_TXT      "qos-pol-wred-update"
#define CMD_83_CMDEXEC      cli_cmd_qos_pol_wred_update
#define CMD_83_HELP         txt_help_qos_pol_wred_update

#define CMD_84_ENUM_NAME    CMD_QOS_POL_SHP_PRINT
#define CMD_84_CLI_TXT      "qos-pol-shp-print"
#define CMD_84_CMDEXEC      cli_cmd_qos_pol_shp_print
#define CMD_84_HELP         txt_help_qos_pol_shp_print

#define CMD_85_ENUM_NAME    CMD_QOS_POL_SHP_UPDATE
#define CMD_85_CLI_TXT      "qos-pol-shp-update"
#define CMD_85_CMDEXEC      cli_cmd_qos_pol_shp_update
#define CMD_85_HELP         txt_help_qos_pol_shp_update

#define CMD_86_ENUM_NAME    CMD_QOS_POL_FLOW_PRINT
#define CMD_86_CLI_TXT      "qos-pol-flow-print"
#define CMD_86_CMDEXEC      cli_cmd_qos_pol_flow_print
#define CMD_86_HELP         txt_help_qos_pol_flow_print

#define CMD_87_ENUM_NAME    CMD_QOS_POL_FLOW_ADD
#define CMD_87_CLI_TXT      "qos-pol-flow-add"
#define CMD_87_CMDEXEC      cli_cmd_qos_pol_flow_add
#define CMD_87_HELP         txt_help_qos_pol_flow_add

#define CMD_88_ENUM_NAME    CMD_QOS_POL_FLOW_DEL
#define CMD_88_CLI_TXT      "qos-pol-flow-del"
#define CMD_88_CMDEXEC      cli_cmd_qos_pol_flow_del
#define CMD_88_HELP         txt_help_qos_pol_flow_del

/*      CMD_89_ENUM_NAME    reserved for future QOS_POL cmds */

#define CMD_90_ENUM_NAME    CMD_FCI_OWNERSHIP
#define CMD_90_CLI_TXT      "fci-ownership"
#define CMD_90_CMDEXEC      cli_cmd_fci_ownership
#define CMD_90_HELP         txt_help_fci_ownership

/*      CMD_91_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_92_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_93_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_94_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_95_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_96_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_97_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_98_ENUM_NAME    reserved for future misc OWNERSHIP cmds */
/*      CMD_99_ENUM_NAME    reserved for future misc OWNERSHIP cmds */

#define CMD_100_ENUM_NAME   CMD_DAEMON_PRINT
#define CMD_100_CLI_TXT     "daemon-print"
#define CMD_100_CMDEXEC     cli_cmd_daemon_print
#define CMD_100_HELP        txt_help_daemon_print

#define CMD_101_ENUM_NAME   CMD_DAEMON_UPDATE
#define CMD_101_CLI_TXT     "daemon-update"
#define CMD_101_CMDEXEC     cli_cmd_daemon_update
#define CMD_101_HELP        txt_help_daemon_update

#define CMD_102_ENUM_NAME   CMD_DAEMON_START
#define CMD_102_CLI_TXT     "daemon-start"
#define CMD_102_CMDEXEC     cli_cmd_daemon_start
#define CMD_102_HELP        txt_help_daemon_start

#define CMD_103_ENUM_NAME   CMD_DAEMON_STOP
#define CMD_103_CLI_TXT     "daemon-stop"
#define CMD_103_CMDEXEC     cli_cmd_daemon_stop
#define CMD_103_HELP        txt_help_daemon_stop




#define CMD_198_ENUM_NAME    CMD_DEMO_FEATURE_PRINT
#define CMD_198_CLI_TXT      "demo-feature-print"
#define CMD_198_CMDEXEC      cli_cmd_demo_feature_print
#define CMD_198_HELP         txt_help_demo_feature_print

#define CMD_199_ENUM_NAME    CMD_DEMO_FEATURE_RUN
#define CMD_199_CLI_TXT      "demo-feature-run"
#define CMD_199_CMDEXEC      cli_cmd_demo_feature_run
#define CMD_199_HELP         txt_help_demo_feature_run

/* CMD_LAST (keep this at the bottom of the cli command definition list) */

/* ==== TYPEDEFS & DATA ==================================================== */

/* cmd IDs */
typedef enum cli_cmd_tt {
    CMD_00_NO_COMMAND = 0u,

#ifdef CMD_01_ENUM_NAME
       CMD_01_ENUM_NAME,
#endif
#ifdef CMD_02_ENUM_NAME
       CMD_02_ENUM_NAME,
#endif
#ifdef CMD_03_ENUM_NAME
       CMD_03_ENUM_NAME,
#endif
#ifdef CMD_04_ENUM_NAME
       CMD_04_ENUM_NAME,
#endif
#ifdef CMD_05_ENUM_NAME
       CMD_05_ENUM_NAME,
#endif
#ifdef CMD_06_ENUM_NAME
       CMD_06_ENUM_NAME,
#endif
#ifdef CMD_07_ENUM_NAME
       CMD_07_ENUM_NAME,
#endif
#ifdef CMD_08_ENUM_NAME
       CMD_08_ENUM_NAME,
#endif
#ifdef CMD_09_ENUM_NAME
       CMD_09_ENUM_NAME,
#endif

#ifdef CMD_10_ENUM_NAME
       CMD_10_ENUM_NAME,
#endif
#ifdef CMD_11_ENUM_NAME
       CMD_11_ENUM_NAME,
#endif
#ifdef CMD_12_ENUM_NAME
       CMD_12_ENUM_NAME,
#endif
#ifdef CMD_13_ENUM_NAME
       CMD_13_ENUM_NAME,
#endif
#ifdef CMD_14_ENUM_NAME
       CMD_14_ENUM_NAME,
#endif
#ifdef CMD_15_ENUM_NAME
       CMD_15_ENUM_NAME,
#endif
#ifdef CMD_16_ENUM_NAME
       CMD_16_ENUM_NAME,
#endif
#ifdef CMD_17_ENUM_NAME
       CMD_17_ENUM_NAME,
#endif
#ifdef CMD_18_ENUM_NAME
       CMD_18_ENUM_NAME,
#endif
#ifdef CMD_19_ENUM_NAME
       CMD_19_ENUM_NAME,
#endif

#ifdef CMD_20_ENUM_NAME
       CMD_20_ENUM_NAME,
#endif
#ifdef CMD_21_ENUM_NAME
       CMD_21_ENUM_NAME,
#endif
#ifdef CMD_22_ENUM_NAME
       CMD_22_ENUM_NAME,
#endif
#ifdef CMD_23_ENUM_NAME
       CMD_23_ENUM_NAME,
#endif
#ifdef CMD_24_ENUM_NAME
       CMD_24_ENUM_NAME,
#endif
#ifdef CMD_25_ENUM_NAME
       CMD_25_ENUM_NAME,
#endif
#ifdef CMD_26_ENUM_NAME
       CMD_26_ENUM_NAME,
#endif
#ifdef CMD_27_ENUM_NAME
       CMD_27_ENUM_NAME,
#endif
#ifdef CMD_28_ENUM_NAME
       CMD_28_ENUM_NAME,
#endif
#ifdef CMD_29_ENUM_NAME
       CMD_29_ENUM_NAME,
#endif

#ifdef CMD_30_ENUM_NAME
       CMD_30_ENUM_NAME,
#endif
#ifdef CMD_31_ENUM_NAME
       CMD_31_ENUM_NAME,
#endif
#ifdef CMD_32_ENUM_NAME
       CMD_32_ENUM_NAME,
#endif
#ifdef CMD_33_ENUM_NAME
       CMD_33_ENUM_NAME,
#endif
#ifdef CMD_34_ENUM_NAME
       CMD_34_ENUM_NAME,
#endif
#ifdef CMD_35_ENUM_NAME
       CMD_35_ENUM_NAME,
#endif
#ifdef CMD_36_ENUM_NAME
       CMD_36_ENUM_NAME,
#endif
#ifdef CMD_37_ENUM_NAME
       CMD_37_ENUM_NAME,
#endif
#ifdef CMD_38_ENUM_NAME
       CMD_38_ENUM_NAME,
#endif
#ifdef CMD_39_ENUM_NAME
       CMD_39_ENUM_NAME,
#endif

#ifdef CMD_40_ENUM_NAME
       CMD_40_ENUM_NAME,
#endif
#ifdef CMD_41_ENUM_NAME
       CMD_41_ENUM_NAME,
#endif
#ifdef CMD_42_ENUM_NAME
       CMD_42_ENUM_NAME,
#endif
#ifdef CMD_43_ENUM_NAME
       CMD_43_ENUM_NAME,
#endif
#ifdef CMD_44_ENUM_NAME
       CMD_44_ENUM_NAME,
#endif
#ifdef CMD_45_ENUM_NAME
       CMD_45_ENUM_NAME,
#endif
#ifdef CMD_46_ENUM_NAME
       CMD_46_ENUM_NAME,
#endif
#ifdef CMD_47_ENUM_NAME
       CMD_47_ENUM_NAME,
#endif
#ifdef CMD_48_ENUM_NAME
       CMD_48_ENUM_NAME,
#endif
#ifdef CMD_49_ENUM_NAME
       CMD_49_ENUM_NAME,
#endif

#ifdef CMD_50_ENUM_NAME
       CMD_50_ENUM_NAME,
#endif
#ifdef CMD_51_ENUM_NAME
       CMD_51_ENUM_NAME,
#endif
#ifdef CMD_52_ENUM_NAME
       CMD_52_ENUM_NAME,
#endif
#ifdef CMD_53_ENUM_NAME
       CMD_53_ENUM_NAME,
#endif
#ifdef CMD_54_ENUM_NAME
       CMD_54_ENUM_NAME,
#endif
#ifdef CMD_55_ENUM_NAME
       CMD_55_ENUM_NAME,
#endif
#ifdef CMD_56_ENUM_NAME
       CMD_56_ENUM_NAME,
#endif
#ifdef CMD_57_ENUM_NAME
       CMD_57_ENUM_NAME,
#endif
#ifdef CMD_58_ENUM_NAME
       CMD_58_ENUM_NAME,
#endif
#ifdef CMD_59_ENUM_NAME
       CMD_59_ENUM_NAME,
#endif

#ifdef CMD_60_ENUM_NAME
       CMD_60_ENUM_NAME,
#endif
#ifdef CMD_61_ENUM_NAME
       CMD_61_ENUM_NAME,
#endif
#ifdef CMD_62_ENUM_NAME
       CMD_62_ENUM_NAME,
#endif
#ifdef CMD_63_ENUM_NAME
       CMD_63_ENUM_NAME,
#endif
#ifdef CMD_64_ENUM_NAME
       CMD_64_ENUM_NAME,
#endif
#ifdef CMD_65_ENUM_NAME
       CMD_65_ENUM_NAME,
#endif
#ifdef CMD_66_ENUM_NAME
       CMD_66_ENUM_NAME,
#endif
#ifdef CMD_67_ENUM_NAME
       CMD_67_ENUM_NAME,
#endif
#ifdef CMD_68_ENUM_NAME
       CMD_68_ENUM_NAME,
#endif
#ifdef CMD_69_ENUM_NAME
       CMD_69_ENUM_NAME,
#endif

#ifdef CMD_70_ENUM_NAME
       CMD_70_ENUM_NAME,
#endif
#ifdef CMD_71_ENUM_NAME
       CMD_71_ENUM_NAME,
#endif
#ifdef CMD_72_ENUM_NAME
       CMD_72_ENUM_NAME,
#endif
#ifdef CMD_73_ENUM_NAME
       CMD_73_ENUM_NAME,
#endif
#ifdef CMD_74_ENUM_NAME
       CMD_74_ENUM_NAME,
#endif
#ifdef CMD_75_ENUM_NAME
       CMD_75_ENUM_NAME,
#endif
#ifdef CMD_76_ENUM_NAME
       CMD_76_ENUM_NAME,
#endif
#ifdef CMD_77_ENUM_NAME
       CMD_77_ENUM_NAME,
#endif
#ifdef CMD_78_ENUM_NAME
       CMD_78_ENUM_NAME,
#endif
#ifdef CMD_79_ENUM_NAME
       CMD_79_ENUM_NAME,
#endif

#ifdef CMD_80_ENUM_NAME
       CMD_80_ENUM_NAME,
#endif
#ifdef CMD_81_ENUM_NAME
       CMD_81_ENUM_NAME,
#endif
#ifdef CMD_82_ENUM_NAME
       CMD_82_ENUM_NAME,
#endif
#ifdef CMD_83_ENUM_NAME
       CMD_83_ENUM_NAME,
#endif
#ifdef CMD_84_ENUM_NAME
       CMD_84_ENUM_NAME,
#endif
#ifdef CMD_85_ENUM_NAME
       CMD_85_ENUM_NAME,
#endif
#ifdef CMD_86_ENUM_NAME
       CMD_86_ENUM_NAME,
#endif
#ifdef CMD_87_ENUM_NAME
       CMD_87_ENUM_NAME,
#endif
#ifdef CMD_88_ENUM_NAME
       CMD_88_ENUM_NAME,
#endif
#ifdef CMD_89_ENUM_NAME
       CMD_89_ENUM_NAME,
#endif

#ifdef CMD_90_ENUM_NAME
       CMD_90_ENUM_NAME,
#endif
#ifdef CMD_91_ENUM_NAME
       CMD_91_ENUM_NAME,
#endif
#ifdef CMD_92_ENUM_NAME
       CMD_92_ENUM_NAME,
#endif
#ifdef CMD_93_ENUM_NAME
       CMD_93_ENUM_NAME,
#endif
#ifdef CMD_94_ENUM_NAME
       CMD_94_ENUM_NAME,
#endif
#ifdef CMD_95_ENUM_NAME
       CMD_95_ENUM_NAME,
#endif
#ifdef CMD_96_ENUM_NAME
       CMD_96_ENUM_NAME,
#endif
#ifdef CMD_97_ENUM_NAME
       CMD_97_ENUM_NAME,
#endif
#ifdef CMD_98_ENUM_NAME
       CMD_98_ENUM_NAME,
#endif
#ifdef CMD_99_ENUM_NAME
       CMD_99_ENUM_NAME,
#endif

#ifdef CMD_100_ENUM_NAME
       CMD_100_ENUM_NAME,
#endif
#ifdef CMD_101_ENUM_NAME
       CMD_101_ENUM_NAME,
#endif
#ifdef CMD_102_ENUM_NAME
       CMD_102_ENUM_NAME,
#endif
#ifdef CMD_103_ENUM_NAME
       CMD_103_ENUM_NAME,
#endif
#ifdef CMD_104_ENUM_NAME
       CMD_104_ENUM_NAME,
#endif
#ifdef CMD_105_ENUM_NAME
       CMD_105_ENUM_NAME,
#endif
#ifdef CMD_106_ENUM_NAME
       CMD_106_ENUM_NAME,
#endif
#ifdef CMD_107_ENUM_NAME
       CMD_107_ENUM_NAME,
#endif
#ifdef CMD_108_ENUM_NAME
       CMD_108_ENUM_NAME,
#endif
#ifdef CMD_109_ENUM_NAME
       CMD_109_ENUM_NAME,
#endif

#ifdef CMD_110_ENUM_NAME
       CMD_110_ENUM_NAME,
#endif
#ifdef CMD_111_ENUM_NAME
       CMD_111_ENUM_NAME,
#endif
#ifdef CMD_112_ENUM_NAME
       CMD_112_ENUM_NAME,
#endif
#ifdef CMD_113_ENUM_NAME
       CMD_113_ENUM_NAME,
#endif
#ifdef CMD_114_ENUM_NAME
       CMD_114_ENUM_NAME,
#endif
#ifdef CMD_115_ENUM_NAME
       CMD_115_ENUM_NAME,
#endif
#ifdef CMD_116_ENUM_NAME
       CMD_116_ENUM_NAME,
#endif
#ifdef CMD_117_ENUM_NAME
       CMD_117_ENUM_NAME,
#endif
#ifdef CMD_118_ENUM_NAME
       CMD_118_ENUM_NAME,
#endif
#ifdef CMD_119_ENUM_NAME
       CMD_119_ENUM_NAME,
#endif

#ifdef CMD_120_ENUM_NAME
       CMD_120_ENUM_NAME,
#endif
#ifdef CMD_121_ENUM_NAME
       CMD_121_ENUM_NAME,
#endif
#ifdef CMD_122_ENUM_NAME
       CMD_122_ENUM_NAME,
#endif
#ifdef CMD_123_ENUM_NAME
       CMD_123_ENUM_NAME,
#endif
#ifdef CMD_124_ENUM_NAME
       CMD_124_ENUM_NAME,
#endif
#ifdef CMD_125_ENUM_NAME
       CMD_125_ENUM_NAME,
#endif
#ifdef CMD_126_ENUM_NAME
       CMD_126_ENUM_NAME,
#endif
#ifdef CMD_127_ENUM_NAME
       CMD_127_ENUM_NAME,
#endif
#ifdef CMD_128_ENUM_NAME
       CMD_128_ENUM_NAME,
#endif
#ifdef CMD_129_ENUM_NAME
       CMD_129_ENUM_NAME,
#endif

#ifdef CMD_130_ENUM_NAME
       CMD_130_ENUM_NAME,
#endif
#ifdef CMD_131_ENUM_NAME
       CMD_131_ENUM_NAME,
#endif
#ifdef CMD_132_ENUM_NAME
       CMD_132_ENUM_NAME,
#endif
#ifdef CMD_133_ENUM_NAME
       CMD_133_ENUM_NAME,
#endif
#ifdef CMD_134_ENUM_NAME
       CMD_134_ENUM_NAME,
#endif
#ifdef CMD_135_ENUM_NAME
       CMD_135_ENUM_NAME,
#endif
#ifdef CMD_136_ENUM_NAME
       CMD_136_ENUM_NAME,
#endif
#ifdef CMD_137_ENUM_NAME
       CMD_137_ENUM_NAME,
#endif
#ifdef CMD_138_ENUM_NAME
       CMD_138_ENUM_NAME,
#endif
#ifdef CMD_139_ENUM_NAME
       CMD_139_ENUM_NAME,
#endif

#ifdef CMD_140_ENUM_NAME
       CMD_140_ENUM_NAME,
#endif
#ifdef CMD_141_ENUM_NAME
       CMD_141_ENUM_NAME,
#endif
#ifdef CMD_142_ENUM_NAME
       CMD_142_ENUM_NAME,
#endif
#ifdef CMD_143_ENUM_NAME
       CMD_143_ENUM_NAME,
#endif
#ifdef CMD_144_ENUM_NAME
       CMD_144_ENUM_NAME,
#endif
#ifdef CMD_145_ENUM_NAME
       CMD_145_ENUM_NAME,
#endif
#ifdef CMD_146_ENUM_NAME
       CMD_146_ENUM_NAME,
#endif
#ifdef CMD_147_ENUM_NAME
       CMD_147_ENUM_NAME,
#endif
#ifdef CMD_148_ENUM_NAME
       CMD_148_ENUM_NAME,
#endif
#ifdef CMD_149_ENUM_NAME
       CMD_149_ENUM_NAME,
#endif

#ifdef CMD_150_ENUM_NAME
       CMD_150_ENUM_NAME,
#endif
#ifdef CMD_151_ENUM_NAME
       CMD_151_ENUM_NAME,
#endif
#ifdef CMD_152_ENUM_NAME
       CMD_152_ENUM_NAME,
#endif
#ifdef CMD_153_ENUM_NAME
       CMD_153_ENUM_NAME,
#endif
#ifdef CMD_154_ENUM_NAME
       CMD_154_ENUM_NAME,
#endif
#ifdef CMD_155_ENUM_NAME
       CMD_155_ENUM_NAME,
#endif
#ifdef CMD_156_ENUM_NAME
       CMD_156_ENUM_NAME,
#endif
#ifdef CMD_157_ENUM_NAME
       CMD_157_ENUM_NAME,
#endif
#ifdef CMD_158_ENUM_NAME
       CMD_158_ENUM_NAME,
#endif
#ifdef CMD_159_ENUM_NAME
       CMD_159_ENUM_NAME,
#endif

#ifdef CMD_160_ENUM_NAME
       CMD_160_ENUM_NAME,
#endif
#ifdef CMD_161_ENUM_NAME
       CMD_161_ENUM_NAME,
#endif
#ifdef CMD_162_ENUM_NAME
       CMD_162_ENUM_NAME,
#endif
#ifdef CMD_163_ENUM_NAME
       CMD_163_ENUM_NAME,
#endif
#ifdef CMD_164_ENUM_NAME
       CMD_164_ENUM_NAME,
#endif
#ifdef CMD_165_ENUM_NAME
       CMD_165_ENUM_NAME,
#endif
#ifdef CMD_166_ENUM_NAME
       CMD_166_ENUM_NAME,
#endif
#ifdef CMD_167_ENUM_NAME
       CMD_167_ENUM_NAME,
#endif
#ifdef CMD_168_ENUM_NAME
       CMD_168_ENUM_NAME,
#endif
#ifdef CMD_169_ENUM_NAME
       CMD_169_ENUM_NAME,
#endif

#ifdef CMD_170_ENUM_NAME
       CMD_170_ENUM_NAME,
#endif
#ifdef CMD_171_ENUM_NAME
       CMD_171_ENUM_NAME,
#endif
#ifdef CMD_172_ENUM_NAME
       CMD_172_ENUM_NAME,
#endif
#ifdef CMD_173_ENUM_NAME
       CMD_173_ENUM_NAME,
#endif
#ifdef CMD_174_ENUM_NAME
       CMD_174_ENUM_NAME,
#endif
#ifdef CMD_175_ENUM_NAME
       CMD_175_ENUM_NAME,
#endif
#ifdef CMD_176_ENUM_NAME
       CMD_176_ENUM_NAME,
#endif
#ifdef CMD_177_ENUM_NAME
       CMD_177_ENUM_NAME,
#endif
#ifdef CMD_178_ENUM_NAME
       CMD_178_ENUM_NAME,
#endif
#ifdef CMD_179_ENUM_NAME
       CMD_179_ENUM_NAME,
#endif

#ifdef CMD_180_ENUM_NAME
       CMD_180_ENUM_NAME,
#endif
#ifdef CMD_181_ENUM_NAME
       CMD_181_ENUM_NAME,
#endif
#ifdef CMD_182_ENUM_NAME
       CMD_182_ENUM_NAME,
#endif
#ifdef CMD_183_ENUM_NAME
       CMD_183_ENUM_NAME,
#endif
#ifdef CMD_184_ENUM_NAME
       CMD_184_ENUM_NAME,
#endif
#ifdef CMD_185_ENUM_NAME
       CMD_185_ENUM_NAME,
#endif
#ifdef CMD_186_ENUM_NAME
       CMD_186_ENUM_NAME,
#endif
#ifdef CMD_187_ENUM_NAME
       CMD_187_ENUM_NAME,
#endif
#ifdef CMD_188_ENUM_NAME
       CMD_188_ENUM_NAME,
#endif
#ifdef CMD_189_ENUM_NAME
       CMD_189_ENUM_NAME,
#endif

#ifdef CMD_190_ENUM_NAME
       CMD_190_ENUM_NAME,
#endif
#ifdef CMD_191_ENUM_NAME
       CMD_191_ENUM_NAME,
#endif
#ifdef CMD_192_ENUM_NAME
       CMD_192_ENUM_NAME,
#endif
#ifdef CMD_193_ENUM_NAME
       CMD_193_ENUM_NAME,
#endif
#ifdef CMD_194_ENUM_NAME
       CMD_194_ENUM_NAME,
#endif
#ifdef CMD_195_ENUM_NAME
       CMD_195_ENUM_NAME,
#endif
#ifdef CMD_196_ENUM_NAME
       CMD_196_ENUM_NAME,
#endif
#ifdef CMD_197_ENUM_NAME
       CMD_197_ENUM_NAME,
#endif
#ifdef CMD_198_ENUM_NAME
       CMD_198_ENUM_NAME,
#endif
#ifdef CMD_199_ENUM_NAME
       CMD_199_ENUM_NAME,
#endif

    CMD_LN  /* length of the ID enum list */
} cli_cmd_t;

/* ==== PUBLIC FUNCTIONS =================================================== */

bool cli_cmd_is_valid(uint16_t value);
bool cli_cmd_is_not_valid(uint16_t value);

bool cli_cmd_is_daemon_related(uint16_t value);
bool cli_cmd_is_not_daemon_related(uint16_t value);

int cli_cmd_execute(cli_cmd_t cmd, const cli_cmdargs_t* p_cmdargs);

int cli_cmd_txt2cmd(cli_cmd_t* p_rtn_cmd, const char* p_txt);
const char* cli_cmd_cmd2txt(cli_cmd_t cmd);

/* ========================================================================= */

#endif
