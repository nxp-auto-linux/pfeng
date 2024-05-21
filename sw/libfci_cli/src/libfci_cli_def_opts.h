/* =========================================================================
 *  Copyright 2020-2024 NXP
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

#ifndef CLI_DEF_OPTS_H_
#define CLI_DEF_OPTS_H_

/* ==== DEFINITIONS : INCOMPATIBILITY GROUPS =============================== */

/*
    There are mutually incompatible cli opts (e.g.: --enable/--disable).
    Such incompatible opts can be viewed as "group members" of an incompatibility group.
    Within the group, only one "group member" opt can be legally detected and parsed within the apps's cli input.

    If one "group member" opt was already detected/parsed and later another "group member" opt from the same group
    is detected during the given cli input parsing session, then an error shoud be raised (incompatible opts detected).

    Handling of incompatibility groups is implemented locally in the parser function.
    To allow easier group management, IDs of incompatibility groups are declared here (and not in the parse fnc).
    Each incompatibility group is represented by:
      --> an ID key in the enum
      --> a bitflag macro (to be used in CLI OPT definitions)

    Bitflag macros are used in cli opt definitions (see below).
    Bitflag macros are expected to be at least 32bit wide.
*/
typedef enum cli_opt_incompat_grp_tt {
    OPT_GRP_NONE = 0uL,

    OPT_GRP_IP4IP6_ID,
    OPT_GRP_ENDIS_ID,
    OPT_GRP_NOREPLY_NOORIG_ID,
    OPT_GRP_ARN_ID,
    OPT_GRP_STATDYN_ID,
    OPT_GRP_LOCKUNLOCK_ID,

    OPT_GRP_LN
} cli_opt_incompat_grp_t;
#define OPT_GRP_IP4IP6          (1uL << OPT_GRP_IP4IP6_ID)
#define OPT_GRP_ENDIS           (1uL << OPT_GRP_ENDIS_ID)
#define OPT_GRP_NOREPLY_NOORIG  (1uL << OPT_GRP_NOREPLY_NOORIG_ID)
#define OPT_GRP_ARN             (1uL << OPT_GRP_ARN_ID)
#define OPT_GRP_STATDYN         (1uL << OPT_GRP_STATDYN_ID)
#define OPT_GRP_LOCKUNLOCK      (1uL << OPT_GRP_LOCKUNLOCK_ID)

/* ==== DEFINITIONS : CLI OPTS ============================================= */
/*
    For each cli option, fill the necessary info here.
    Do NOT include any *.h files here. It should not be needed.

    Opts have to be parsed via getopt_long() fnc.
    Opts usually (not always) correspond to some member of the 'cmdargs_t' superstructure.

    OPT_00_NO_OPTION is hardcoded.

    Search for keyword 'OPT_LAST' to get to the bottom of the cli option definition list.


    Description of a cli opt definition (xx is a number from 01 to 99)
    ------------------------------------------------------------------
    OPT_xx_ENUM_NAME        OPT_MY_TEST         This enum key is automatically created and associated
                                                with the given cli opt.

    OPT_xx_OPT_PARSE        opt_parse_my_test   Name of a static function which is invoked when this cli opt
                                                (and optionally its argument) is to be parsed from the input txt vector.
                                                The function is expected to be in the 'cli_parser.c' file.
                                                The function is expected to conform to the following prototype:
                                                static int opt_parse_my_test(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg);

    OPT_xx_HAS_ARG          y                   Does this cli opt expect an argument?
                                                  y : yes
                                                  n : no

    OPT_xx_INCOMPAT_GRPS    OPT_GRP_NONE        Incompatibility groups this cli opt belongs to.
                                                Example:
                                                  OPT_GRP_NONE
                                                  OPT_GRP_IP4IP6
                                                 (OPT_GRP_IP4IP6 | OPT_GRP_ENDIS)

    OPT_xx_CLI_SHORT_CODE   m                   Command-line text which represents a given cli opt.
                                                This is a "short opt" representation;
                                                a single letter/digit with one leading dash (-m).
                                                See documentation of a getopt() fnc family.
                                                If the given cli opt does not have a shortopt variant,
                                                then a generic OPT_AUTO_CODE should be used.
                                                IMPORTANT: If the given cli opt uses a shortopt variant, then
                                                           the used letter/digit must NOT be enclosed
                                                           in any quotation marks.
                                                           Reason: Based on user experience, it is beneficial to create
                                                                   a single-letter longopt version as well (--m).
                                                                   To do so automatically via C preprocessor,
                                                                   the letter/digit must NOT be enclosed in any quotation marks.

    OPT_xx_CLI_LONG_TXT_A   "my-test"           Command-line text which represents a given cli opt.
                                                This is a "long opt"; a text with two leading dashes (--my-test).
                                                See documentation of a getopt() fnc family.
                                                Up to 4 different longopt texts for the given cli opt are supported,
                                                labled by suffixes _A,_B,_C,_D.

    OPT_xx_TXT_HELP         "-m|--m|--my-test"  A help text, documenting all text representations of the given cli opt.
                                                NOTE: This text is created manually, but is expected to contain all
                                                      command-line text representations of the given cli opt.
                                                      PAY ATTENTION AND FILL PROPERLY!
                                                      Failure to do so results in incorrect help texts and
                                                      (very probably) some tedious support E-mail conversations.

    ...............     TXT_HELP__MY_TEST \     Named (not numbered) help text symbol.
    OPT_xx_TXT_HELP                             Tied with the corresponding numbered help text symbol.
                                                Intended to be used in "def_help.c" source, to prevent
                                                from shifting help texts if the given opt gets renumbered.
                                                The backslash is a line-joiner (must be the last char of the line).
*/
#define OPT_AUTO_CODE  (1000)


#define OPT_01_ENUM_NAME         OPT_IP4
#define OPT_01_OPT_PARSE         opt_parse_ip4
#define OPT_01_HAS_ARG           n
#define OPT_01_INCOMPAT_GRPS     OPT_GRP_IP4IP6
#define OPT_01_CLI_SHORT_CODE    4
#define OPT_01_CLI_LONG_TXT_A    "ip4"
#define OPT_01_TXT_HELP          "-4|--4|--ip4"
#define                          TXT_HELP__IP4 \
        OPT_01_TXT_HELP


#define OPT_02_ENUM_NAME         OPT_IP6
#define OPT_02_OPT_PARSE         opt_parse_ip6
#define OPT_02_HAS_ARG           n
#define OPT_02_INCOMPAT_GRPS     OPT_GRP_IP4IP6
#define OPT_02_CLI_SHORT_CODE    6
#define OPT_02_CLI_LONG_TXT_A    "ip6"
#define OPT_02_TXT_HELP          "-6|--6|--ip6"
#define                          TXT_HELP__IP6 \
        OPT_02_TXT_HELP


#define OPT_03_ENUM_NAME         OPT_ALL
#define OPT_03_OPT_PARSE         opt_parse_all
#define OPT_03_HAS_ARG           n
#define OPT_03_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_03_CLI_SHORT_CODE    a
#define OPT_03_CLI_LONG_TXT_A    "all"
#define OPT_03_TXT_HELP          "-a|--a|--all"
#define                          TXT_HELP__ALL \
        OPT_03_TXT_HELP


#define OPT_04_ENUM_NAME         OPT_HELP
#define OPT_04_OPT_PARSE         opt_parse_help
#define OPT_04_HAS_ARG           n
#define OPT_04_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_04_CLI_SHORT_CODE    h
#define OPT_04_CLI_LONG_TXT_A    "help"
#define OPT_04_TXT_HELP          "-h|--h|--help"
#define                          TXT_HELP__HELP \
        OPT_04_TXT_HELP


#define OPT_05_ENUM_NAME         OPT_VERBOSE
#define OPT_05_OPT_PARSE         opt_parse_verbose
#define OPT_05_HAS_ARG           n
#define OPT_05_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_05_CLI_SHORT_CODE    v
#define OPT_05_CLI_LONG_TXT_A    "verbose"
#define OPT_05_TXT_HELP          "-v|--v|--verbose"
#define                          TXT_HELP__VERBOSE \
        OPT_05_TXT_HELP


#define OPT_06_ENUM_NAME         OPT_VERSION
#define OPT_06_OPT_PARSE         opt_parse_version
#define OPT_06_HAS_ARG           n
#define OPT_06_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_06_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_06_CLI_LONG_TXT_A    "version"
#define OPT_06_TXT_HELP          "--version"
#define                          TXT_HELP__VERSION \
        OPT_06_TXT_HELP


#define OPT_07_ENUM_NAME         OPT_INTERFACE
#define OPT_07_OPT_PARSE         opt_parse_interface
#define OPT_07_HAS_ARG           y
#define OPT_07_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_07_CLI_SHORT_CODE    i
#define OPT_07_CLI_LONG_TXT_A    "interface"
#define OPT_07_TXT_HELP          "-i|--i|--interface"
#define                          TXT_HELP__INTERFACE \
        OPT_07_TXT_HELP


#define OPT_08_ENUM_NAME         OPT_PARENT
#define OPT_08_OPT_PARSE         opt_parse_parent
#define OPT_08_HAS_ARG           y
#define OPT_08_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_08_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_08_CLI_LONG_TXT_A    "parent"
#define OPT_08_TXT_HELP          "--parent"
#define                          TXT_HELP__PARENT \
        OPT_08_TXT_HELP


#define OPT_09_ENUM_NAME         OPT_MIRROR
#define OPT_09_OPT_PARSE         opt_parse_mirror
#define OPT_09_HAS_ARG           y
#define OPT_09_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_09_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_09_CLI_LONG_TXT_A    "mirr"
#define OPT_09_CLI_LONG_TXT_B    "mirror"
#define OPT_09_TXT_HELP          "--mirr|--mirror"
#define                          TXT_HELP__MIRROR \
        OPT_09_TXT_HELP


#define OPT_10_ENUM_NAME         OPT_MODE
#define OPT_10_OPT_PARSE         opt_parse_mode
#define OPT_10_HAS_ARG           y
#define OPT_10_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_10_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_10_CLI_LONG_TXT_A    "mode"
#define OPT_10_TXT_HELP          "--mode"
#define                          TXT_HELP__MODE \
        OPT_10_TXT_HELP


#define OPT_11_ENUM_NAME         OPT_BLOCK_STATE
#define OPT_11_OPT_PARSE         opt_parse_block_state
#define OPT_11_HAS_ARG           y
#define OPT_11_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_11_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_11_CLI_LONG_TXT_A    "bs"
#define OPT_11_CLI_LONG_TXT_B    "block-state"
#define OPT_11_TXT_HELP          "--bs|--block-state"
#define                          TXT_HELP__BLOCK_STATE \
        OPT_11_TXT_HELP


#define OPT_12_ENUM_NAME         OPT_ENABLE
#define OPT_12_OPT_PARSE         opt_parse_enable
#define OPT_12_HAS_ARG           n
#define OPT_12_INCOMPAT_GRPS     OPT_GRP_ENDIS
#define OPT_12_CLI_SHORT_CODE    E
#define OPT_12_CLI_LONG_TXT_A    "enable"
#define OPT_12_TXT_HELP          "-E|--E|--enable"
#define                          TXT_HELP__ENABLE \
        OPT_12_TXT_HELP


#define OPT_13_ENUM_NAME         OPT_DISABLE
#define OPT_13_OPT_PARSE         opt_parse_disable
#define OPT_13_HAS_ARG           n
#define OPT_13_INCOMPAT_GRPS     OPT_GRP_ENDIS
#define OPT_13_CLI_SHORT_CODE    D
#define OPT_13_CLI_LONG_TXT_A    "disable"
#define OPT_13_TXT_HELP          "-D|--D|--disable"
#define                          TXT_HELP__DISABLE \
        OPT_13_TXT_HELP


#define OPT_14_ENUM_NAME         OPT_PROMISC
#define OPT_14_OPT_PARSE         opt_parse_promisc
#define OPT_14_HAS_ARG           y
#define OPT_14_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_14_CLI_SHORT_CODE    P
#define OPT_14_CLI_LONG_TXT_A    "promisc"
#define OPT_14_TXT_HELP          "-P|--P|--promisc"
#define                          TXT_HELP__PROMISC \
        OPT_14_TXT_HELP


/*      OPT_15_ENUM_NAME         reserved for future use */


#define OPT_16_ENUM_NAME         OPT_MATCH_MODE
#define OPT_16_OPT_PARSE         opt_parse_match_mode
#define OPT_16_HAS_ARG           y
#define OPT_16_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_16_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_16_CLI_LONG_TXT_A    "match-mode"
#define OPT_16_TXT_HELP          "--match-mode"
#define                          TXT_HELP__MATCH_MODE \
        OPT_16_TXT_HELP


#define OPT_17_ENUM_NAME         OPT_DISCARD_ON_MATCH
#define OPT_17_OPT_PARSE         opt_parse_discard_on_match
#define OPT_17_HAS_ARG           y
#define OPT_17_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_17_CLI_SHORT_CODE    X
#define OPT_17_CLI_LONG_TXT_A    "discard-on-match"
#define OPT_17_TXT_HELP          "-X|--X|--discard-on-match"
#define                          TXT_HELP__DISCARD_ON_MATCH \
        OPT_17_TXT_HELP


#define OPT_18_ENUM_NAME         OPT_EGRESS
#define OPT_18_OPT_PARSE         opt_parse_egress
#define OPT_18_HAS_ARG           y
#define OPT_18_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_18_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_18_CLI_LONG_TXT_A    "egress"
#define OPT_18_TXT_HELP          "--egress"
#define                          TXT_HELP__EGRESS \
        OPT_18_TXT_HELP


#define OPT_19_ENUM_NAME         OPT_MATCH_RULES
#define OPT_19_OPT_PARSE         opt_parse_match_rules
#define OPT_19_HAS_ARG           y
#define OPT_19_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_19_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_19_CLI_LONG_TXT_A    "mr"
#define OPT_19_CLI_LONG_TXT_B    "match-rules"
#define OPT_19_TXT_HELP          "--mr|--match-rules"
#define                          TXT_HELP__MATCH_RULES \
        OPT_19_TXT_HELP


#define OPT_20_ENUM_NAME         OPT_VLAN
#define OPT_20_OPT_PARSE         opt_parse_vlan
#define OPT_20_HAS_ARG           y
#define OPT_20_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_20_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_20_CLI_LONG_TXT_A    "vlan"
#define OPT_20_TXT_HELP          "--vlan"
#define                          TXT_HELP__VLAN \
        OPT_20_TXT_HELP


#define OPT_21_ENUM_NAME         OPT_PROTOCOL
#define OPT_21_OPT_PARSE         opt_parse_protocol
#define OPT_21_HAS_ARG           y
#define OPT_21_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_21_CLI_SHORT_CODE    p
#define OPT_21_CLI_LONG_TXT_A    "proto"
#define OPT_21_CLI_LONG_TXT_B    "protocol"
#define OPT_21_TXT_HELP          "-p|--p|--proto|--protocol"
#define                          TXT_HELP__PROTOCOL \
        OPT_21_TXT_HELP


#define OPT_22_ENUM_NAME         OPT_ETHTYPE
#define OPT_22_OPT_PARSE         opt_parse_ethtype
#define OPT_22_HAS_ARG           y
#define OPT_22_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_22_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_22_CLI_LONG_TXT_A    "et"
#define OPT_22_CLI_LONG_TXT_B    "ether-type"
#define OPT_22_TXT_HELP          "--et|--ether-type"
#define                          TXT_HELP__ETHTYPE \
        OPT_22_TXT_HELP


#define OPT_23_ENUM_NAME         OPT_MAC
#define OPT_23_OPT_PARSE         opt_parse_mac
#define OPT_23_HAS_ARG           y
#define OPT_23_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_23_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_23_CLI_LONG_TXT_A    "mac"
#define OPT_23_CLI_LONG_TXT_B    "mac"
#define OPT_23_TXT_HELP          "--mac"
#define                          TXT_HELP__MAC \
        OPT_23_TXT_HELP


#define OPT_24_ENUM_NAME         OPT_SMAC
#define OPT_24_OPT_PARSE         opt_parse_smac
#define OPT_24_HAS_ARG           y
#define OPT_24_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_24_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_24_CLI_LONG_TXT_A    "smac"
#define OPT_24_CLI_LONG_TXT_B    "src-mac"
#define OPT_24_TXT_HELP          "--smac|--src-mac"
#define                          TXT_HELP__SMAC \
        OPT_24_TXT_HELP


#define OPT_25_ENUM_NAME         OPT_DMAC
#define OPT_25_OPT_PARSE         opt_parse_dmac
#define OPT_25_HAS_ARG           y
#define OPT_25_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_25_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_25_CLI_LONG_TXT_A    "dmac"
#define OPT_25_CLI_LONG_TXT_B    "dst-mac"
#define OPT_25_TXT_HELP          "--dmac|--dst-mac"
#define                          TXT_HELP__DMAC \
        OPT_25_TXT_HELP


#define OPT_26_ENUM_NAME         OPT_SIP
#define OPT_26_OPT_PARSE         opt_parse_sip
#define OPT_26_HAS_ARG           y
#define OPT_26_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_26_CLI_SHORT_CODE    s
#define OPT_26_CLI_LONG_TXT_A    "sip"
#define OPT_26_CLI_LONG_TXT_B    "src"
#define OPT_26_TXT_HELP          "-s|--s|--sip|--src"
#define                          TXT_HELP__SIP \
        OPT_26_TXT_HELP


#define OPT_27_ENUM_NAME         OPT_DIP
#define OPT_27_OPT_PARSE         opt_parse_dip
#define OPT_27_HAS_ARG           y
#define OPT_27_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_27_CLI_SHORT_CODE    d
#define OPT_27_CLI_LONG_TXT_A    "dip"
#define OPT_27_CLI_LONG_TXT_B    "dst"
#define OPT_27_TXT_HELP          "-d|--d|--dip|--dst"
#define                          TXT_HELP__DIP \
        OPT_27_TXT_HELP


#define OPT_28_ENUM_NAME         OPT_R_SIP
#define OPT_28_OPT_PARSE         opt_parse_r_sip
#define OPT_28_HAS_ARG           y
#define OPT_28_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_28_CLI_SHORT_CODE    r
#define OPT_28_CLI_LONG_TXT_A    "r-sip"
#define OPT_28_CLI_LONG_TXT_B    "r-src"
#define OPT_28_TXT_HELP          "-r|--r|--r-sip|--r-src"
#define                          TXT_HELP__R_SIP \
        OPT_28_TXT_HELP


#define OPT_29_ENUM_NAME         OPT_R_DIP
#define OPT_29_OPT_PARSE         opt_parse_r_dip
#define OPT_29_HAS_ARG           y
#define OPT_29_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_29_CLI_SHORT_CODE    q
#define OPT_29_CLI_LONG_TXT_A    "r-dip"
#define OPT_29_CLI_LONG_TXT_B    "r-dst"
#define OPT_29_TXT_HELP          "-q|--q|--r-dip|--r-dst"
#define                          TXT_HELP__R_DIP \
        OPT_29_TXT_HELP


#define OPT_30_ENUM_NAME         OPT_SIP6
#define OPT_30_OPT_PARSE         opt_parse_sip6
#define OPT_30_HAS_ARG           y
#define OPT_30_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_30_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_30_CLI_LONG_TXT_A    "s6"
#define OPT_30_CLI_LONG_TXT_B    "sip6"
#define OPT_30_CLI_LONG_TXT_C    "src6"
#define OPT_30_TXT_HELP          "--s6|--sip6|--src6"
#define                          TXT_HELP__SIP6 \
        OPT_30_TXT_HELP


#define OPT_31_ENUM_NAME         OPT_DIP6
#define OPT_31_OPT_PARSE         opt_parse_dip6
#define OPT_31_HAS_ARG           y
#define OPT_31_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_31_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_31_CLI_LONG_TXT_A    "d6"
#define OPT_31_CLI_LONG_TXT_B    "dip6"
#define OPT_31_CLI_LONG_TXT_C    "dst6"
#define OPT_31_TXT_HELP          "--d6|--dip6|--dst6"
#define                          TXT_HELP__DIP6 \
        OPT_31_TXT_HELP


#define OPT_32_ENUM_NAME         OPT_SPORT
#define OPT_32_OPT_PARSE         opt_parse_sport
#define OPT_32_HAS_ARG           y
#define OPT_32_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_32_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_32_CLI_LONG_TXT_A    "sport"
#define OPT_32_CLI_LONG_TXT_B    "src-port"
#define OPT_32_TXT_HELP          "--sport|--src-port"
#define                          TXT_HELP__SPORT \
        OPT_32_TXT_HELP


#define OPT_33_ENUM_NAME         OPT_DPORT
#define OPT_33_OPT_PARSE         opt_parse_dport
#define OPT_33_HAS_ARG           y
#define OPT_33_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_33_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_33_CLI_LONG_TXT_A    "dport"
#define OPT_33_CLI_LONG_TXT_B    "dst-port"
#define OPT_33_TXT_HELP          "--dport|--dst-port"
#define                          TXT_HELP__DPORT \
        OPT_33_TXT_HELP


#define OPT_34_ENUM_NAME         OPT_R_SPORT
#define OPT_34_OPT_PARSE         opt_parse_r_sport
#define OPT_34_HAS_ARG           y
#define OPT_34_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_34_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_34_CLI_LONG_TXT_A    "r-sport"
#define OPT_34_CLI_LONG_TXT_B    "r-src-port"
#define OPT_34_TXT_HELP          "--r-sport|--r-src-port"
#define                          TXT_HELP__R_SPORT \
        OPT_34_TXT_HELP


#define OPT_35_ENUM_NAME         OPT_R_DPORT
#define OPT_35_OPT_PARSE         opt_parse_r_dport
#define OPT_35_HAS_ARG           y
#define OPT_35_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_35_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_35_CLI_LONG_TXT_A    "r-dport"
#define OPT_35_CLI_LONG_TXT_B    "r-dst-port"
#define OPT_35_TXT_HELP          "--r-dport|--r-dst-port"
#define                          TXT_HELP__R_DPORT \
        OPT_35_TXT_HELP


#define OPT_36_ENUM_NAME         OPT_HIF_COOKIE
#define OPT_36_OPT_PARSE         opt_parse_hif_cookie
#define OPT_36_HAS_ARG           y
#define OPT_36_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_36_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_36_CLI_LONG_TXT_A    "hifc"
#define OPT_36_CLI_LONG_TXT_B    "hif-cookie"
#define OPT_36_TXT_HELP          "--hifc|--hif-cookie"
#define                          TXT_HELP__HIF_COOKIE \
        OPT_36_TXT_HELP


#define OPT_37_ENUM_NAME         OPT_TIMEOUT
#define OPT_37_OPT_PARSE         opt_parse_timeout
#define OPT_37_HAS_ARG           y
#define OPT_37_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_37_CLI_SHORT_CODE    w
#define OPT_37_CLI_LONG_TXT_A    "timeout"
#define OPT_37_TXT_HELP          "-w|--w|--timeout"
#define                          TXT_HELP__TIMEOUT \
        OPT_37_TXT_HELP


#define OPT_38_ENUM_NAME         OPT_TIMEOUT2
#define OPT_38_OPT_PARSE         opt_parse_timeout2
#define OPT_38_HAS_ARG           y
#define OPT_38_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_38_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_38_CLI_LONG_TXT_A    "w2"
#define OPT_38_CLI_LONG_TXT_B    "timeout2"
#define OPT_38_TXT_HELP          "--w2|--timeout2"
#define                          TXT_HELP__TIMEOUT2 \
        OPT_38_TXT_HELP


#define OPT_39_ENUM_NAME         OPT_UCAST_HIT
#define OPT_39_OPT_PARSE         opt_parse_ucast_hit
#define OPT_39_HAS_ARG           y
#define OPT_39_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_39_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_39_CLI_LONG_TXT_A    "uh"
#define OPT_39_CLI_LONG_TXT_B    "ucast-hit"
#define OPT_39_TXT_HELP          "--uh|--ucast-hit"
#define                          TXT_HELP__UCAST_HIT \
        OPT_39_TXT_HELP


#define OPT_40_ENUM_NAME         OPT_UCAST_MISS
#define OPT_40_OPT_PARSE         opt_parse_ucast_miss
#define OPT_40_HAS_ARG           y
#define OPT_40_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_40_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_40_CLI_LONG_TXT_A    "um"
#define OPT_40_CLI_LONG_TXT_B    "ucast-miss"
#define OPT_40_TXT_HELP          "--um|--ucast-miss"
#define                          TXT_HELP__UCAST_MISS \
        OPT_40_TXT_HELP


#define OPT_41_ENUM_NAME         OPT_MCAST_HIT
#define OPT_41_OPT_PARSE         opt_parse_mcast_hit
#define OPT_41_HAS_ARG           y
#define OPT_41_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_41_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_41_CLI_LONG_TXT_A    "mh"
#define OPT_41_CLI_LONG_TXT_B    "mcast-hit"
#define OPT_41_TXT_HELP          "--mh|--mcast-hit"
#define                          TXT_HELP__MCAST_HIT \
        OPT_41_TXT_HELP


#define OPT_42_ENUM_NAME         OPT_MCAST_MISS
#define OPT_42_OPT_PARSE         opt_parse_mcast_miss
#define OPT_42_HAS_ARG           y
#define OPT_42_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_42_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_42_CLI_LONG_TXT_A    "mm"
#define OPT_42_CLI_LONG_TXT_B    "mcast-miss"
#define OPT_42_TXT_HELP          "--mm|--mcast-miss"
#define                          TXT_HELP__MCAST_MISS \
        OPT_42_TXT_HELP


#define OPT_43_ENUM_NAME         OPT_TAG
#define OPT_43_OPT_PARSE         opt_parse_tag
#define OPT_43_HAS_ARG           y
#define OPT_43_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_43_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_43_CLI_LONG_TXT_A    "tag"
#define OPT_43_TXT_HELP          "--tag"
#define                          TXT_HELP__TAG \
        OPT_43_TXT_HELP


#define OPT_44_ENUM_NAME         OPT_DEFAULT
#define OPT_44_OPT_PARSE         opt_parse_default
#define OPT_44_HAS_ARG           n
#define OPT_44_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_44_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_44_CLI_LONG_TXT_A    "def"
#define OPT_44_CLI_LONG_TXT_B    "default"
#define OPT_44_TXT_HELP          "--def|--default"
#define                          TXT_HELP__DEFAULT \
        OPT_44_TXT_HELP


#define OPT_45_ENUM_NAME         OPT_FALLBACK
#define OPT_45_OPT_PARSE         opt_parse_fallback
#define OPT_45_HAS_ARG           n
#define OPT_45_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_45_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_45_CLI_LONG_TXT_A    "fbk"
#define OPT_45_CLI_LONG_TXT_B    "fallback"
#define OPT_45_TXT_HELP          "--fbk|--fallback"
#define                          TXT_HELP__FALLBACK \
        OPT_45_TXT_HELP


#define OPT_46_ENUM_NAME         OPT_4o6
#define OPT_46_OPT_PARSE         opt_parse_4o6
#define OPT_46_HAS_ARG           n
#define OPT_46_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_46_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_46_CLI_LONG_TXT_A    "4o6"
#define OPT_46_TXT_HELP          "--4o6"
#define                          TXT_HELP__4o6 \
        OPT_46_TXT_HELP


#define OPT_47_ENUM_NAME         OPT_NO_REPLY
#define OPT_47_OPT_PARSE         opt_parse_no_reply
#define OPT_47_HAS_ARG           n
#define OPT_47_INCOMPAT_GRPS     OPT_GRP_NOREPLY_NOORIG
#define OPT_47_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_47_CLI_LONG_TXT_A    "no-reply"
#define OPT_47_TXT_HELP          "--no-reply"
#define                          TXT_HELP__NO_REPLY \
        OPT_47_TXT_HELP


#define OPT_48_ENUM_NAME         OPT_NO_ORIG
#define OPT_48_OPT_PARSE         opt_parse_no_orig
#define OPT_48_HAS_ARG           n
#define OPT_48_INCOMPAT_GRPS     OPT_GRP_NOREPLY_NOORIG
#define OPT_48_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_48_CLI_LONG_TXT_A    "no-orig"
#define OPT_48_TXT_HELP          "--no-orig"
#define                          TXT_HELP__NO_ORIG \
        OPT_48_TXT_HELP


#define OPT_49_ENUM_NAME         OPT_ROUTE
#define OPT_49_OPT_PARSE         opt_parse_route
#define OPT_49_HAS_ARG           y
#define OPT_49_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_49_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_49_CLI_LONG_TXT_A    "rt"
#define OPT_49_CLI_LONG_TXT_B    "route"
#define OPT_49_TXT_HELP          "--rt|--route"
#define                          TXT_HELP__ROUTE \
        OPT_49_TXT_HELP


#define OPT_50_ENUM_NAME         OPT_R_ROUTE
#define OPT_50_OPT_PARSE         opt_parse_r_route
#define OPT_50_HAS_ARG           y
#define OPT_50_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_50_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_50_CLI_LONG_TXT_A    "r-rt"
#define OPT_50_CLI_LONG_TXT_B    "r-route"
#define OPT_50_TXT_HELP          "--r-rt|--r-route"
#define                          TXT_HELP__R_ROUTE \
        OPT_50_TXT_HELP


#define OPT_51_ENUM_NAME         OPT_RX_MIRROR0
#define OPT_51_OPT_PARSE         opt_parse_rx_mirror0
#define OPT_51_HAS_ARG           y
#define OPT_51_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_51_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_51_CLI_LONG_TXT_A    "rxmirr0"
#define OPT_51_CLI_LONG_TXT_B    "rx-mirror0"
#define OPT_51_TXT_HELP          "--rxmirr0|--rx-mirror0"
#define                          TXT_HELP__RX_MIRROR0 \
        OPT_51_TXT_HELP


#define OPT_52_ENUM_NAME         OPT_RX_MIRROR1
#define OPT_52_OPT_PARSE         opt_parse_rx_mirror1
#define OPT_52_HAS_ARG           y
#define OPT_52_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_52_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_52_CLI_LONG_TXT_A    "rxmirr1"
#define OPT_52_CLI_LONG_TXT_B    "rx-mirror1"
#define OPT_52_TXT_HELP          "--rxmirr1|--rx-mirror1"
#define                          TXT_HELP__RX_MIRROR1 \
        OPT_52_TXT_HELP


#define OPT_53_ENUM_NAME         OPT_TX_MIRROR0
#define OPT_53_OPT_PARSE         opt_parse_tx_mirror0
#define OPT_53_HAS_ARG           y
#define OPT_53_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_53_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_53_CLI_LONG_TXT_A    "txmirr0"
#define OPT_53_CLI_LONG_TXT_B    "tx-mirror0"
#define OPT_53_TXT_HELP          "--txmirr0|--tx-mirror0"
#define                          TXT_HELP__TX_MIRROR0 \
        OPT_53_TXT_HELP


#define OPT_54_ENUM_NAME         OPT_TX_MIRROR1
#define OPT_54_OPT_PARSE         opt_parse_tx_mirror1
#define OPT_54_HAS_ARG           y
#define OPT_54_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_54_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_54_CLI_LONG_TXT_A    "txmirr1"
#define OPT_54_CLI_LONG_TXT_B    "tx-mirror1"
#define OPT_54_TXT_HELP          "--txmirr1|--tx-mirror1"
#define                          TXT_HELP__TX_MIRROR1 \
        OPT_54_TXT_HELP


#define OPT_55_ENUM_NAME         OPT_TABLE
#define OPT_55_OPT_PARSE         opt_parse_table
#define OPT_55_HAS_ARG           y
#define OPT_55_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_55_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_55_CLI_LONG_TXT_A    "tb"
#define OPT_55_CLI_LONG_TXT_B    "table"
#define OPT_55_TXT_HELP          "--tb|--table"
#define                          TXT_HELP__TABLE \
        OPT_55_TXT_HELP


#define OPT_56_ENUM_NAME         OPT_TABLE0
#define OPT_56_OPT_PARSE         opt_parse_table0
#define OPT_56_HAS_ARG           y
#define OPT_56_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_56_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_56_CLI_LONG_TXT_A    "tb0"
#define OPT_56_CLI_LONG_TXT_B    "table0"
#define OPT_56_TXT_HELP          "--tb0|--table0"
#define                          TXT_HELP__TABLE0 \
        OPT_56_TXT_HELP


#define OPT_57_ENUM_NAME         OPT_TABLE1
#define OPT_57_OPT_PARSE         opt_parse_table1
#define OPT_57_HAS_ARG           y
#define OPT_57_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_57_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_57_CLI_LONG_TXT_A    "tb1"
#define OPT_57_CLI_LONG_TXT_B    "table1"
#define OPT_57_TXT_HELP          "--tb1|--table1"
#define                          TXT_HELP__TABLE1 \
        OPT_57_TXT_HELP


#define OPT_58_ENUM_NAME         OPT_RULE
#define OPT_58_OPT_PARSE         opt_parse_rule
#define OPT_58_HAS_ARG           y
#define OPT_58_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_58_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_58_CLI_LONG_TXT_A    "rl"
#define OPT_58_CLI_LONG_TXT_B    "rule"
#define OPT_58_TXT_HELP          "--rl|--rule"
#define                          TXT_HELP__RULE \
        OPT_58_TXT_HELP


#define OPT_59_ENUM_NAME         OPT_NEXT_RULE
#define OPT_59_OPT_PARSE         opt_parse_next_rule
#define OPT_59_HAS_ARG           y
#define OPT_59_INCOMPAT_GRPS     OPT_GRP_ARN
#define OPT_59_CLI_SHORT_CODE    N
#define OPT_59_CLI_LONG_TXT_A    "next-rule"
#define OPT_59_TXT_HELP          "-N|--N|--next-rule"
#define                          TXT_HELP__NEXT_RULE \
        OPT_59_TXT_HELP


#define OPT_60_ENUM_NAME         OPT_DATA
#define OPT_60_OPT_PARSE         opt_parse_data
#define OPT_60_HAS_ARG           y
#define OPT_60_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_60_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_60_CLI_LONG_TXT_A    "data"
#define OPT_60_TXT_HELP          "--data"
#define                          TXT_HELP__DATA \
        OPT_60_TXT_HELP


#define OPT_61_ENUM_NAME         OPT_MASK
#define OPT_61_OPT_PARSE         opt_parse_mask
#define OPT_61_HAS_ARG           y
#define OPT_61_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_61_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_61_CLI_LONG_TXT_A    "mask"
#define OPT_61_TXT_HELP          "--mask"
#define                          TXT_HELP__MASK \
        OPT_61_TXT_HELP


#define OPT_62_ENUM_NAME         OPT_LAYER
#define OPT_62_OPT_PARSE         opt_parse_layer
#define OPT_62_HAS_ARG           y
#define OPT_62_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_62_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_62_CLI_LONG_TXT_A    "layer"
#define OPT_62_TXT_HELP          "--layer"
#define                          TXT_HELP__LAYER \
        OPT_62_TXT_HELP


#define OPT_63_ENUM_NAME         OPT_OFFSET
#define OPT_63_OPT_PARSE         opt_parse_offset
#define OPT_63_HAS_ARG           y
#define OPT_63_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_63_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_63_CLI_LONG_TXT_A    "ofs"
#define OPT_63_CLI_LONG_TXT_B    "offset"
#define OPT_63_TXT_HELP          "--ofs|--offset"
#define                          TXT_HELP__OFFSET \
        OPT_63_TXT_HELP


#define OPT_64_ENUM_NAME         OPT_INVERT
#define OPT_64_OPT_PARSE         opt_parse_invert
#define OPT_64_HAS_ARG           n
#define OPT_64_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_64_CLI_SHORT_CODE    I
#define OPT_64_CLI_LONG_TXT_A    "invert"
#define OPT_64_TXT_HELP          "-I|--I|--invert"
#define                          TXT_HELP__INVERT \
        OPT_64_TXT_HELP


#define OPT_65_ENUM_NAME         OPT_ACCEPT
#define OPT_65_OPT_PARSE         opt_parse_accept
#define OPT_65_HAS_ARG           n
#define OPT_65_INCOMPAT_GRPS     OPT_GRP_ARN
#define OPT_65_CLI_SHORT_CODE    A
#define OPT_65_CLI_LONG_TXT_A    "accept"
#define OPT_65_TXT_HELP          "-A|--A|--accept"
#define                          TXT_HELP__ACCEPT \
        OPT_65_TXT_HELP


#define OPT_66_ENUM_NAME         OPT_REJECT
#define OPT_66_OPT_PARSE         opt_parse_reject
#define OPT_66_HAS_ARG           n
#define OPT_66_INCOMPAT_GRPS     OPT_GRP_ARN
#define OPT_66_CLI_SHORT_CODE    R
#define OPT_66_CLI_LONG_TXT_A    "reject"
#define OPT_66_TXT_HELP          "-R|--R|--reject"
#define                          TXT_HELP__REJECT \
        OPT_66_TXT_HELP


#define OPT_67_ENUM_NAME         OPT_POSITION
#define OPT_67_OPT_PARSE         opt_parse_position
#define OPT_67_HAS_ARG           y
#define OPT_67_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_67_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_67_CLI_LONG_TXT_A    "pos"
#define OPT_67_CLI_LONG_TXT_B    "position"
#define OPT_67_TXT_HELP          "--pos|--position"
#define                          TXT_HELP__POSITION \
        OPT_67_TXT_HELP


#define OPT_68_ENUM_NAME         OPT_COUNT
#define OPT_68_OPT_PARSE         opt_parse_count
#define OPT_68_HAS_ARG           y
#define OPT_68_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_68_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_68_CLI_LONG_TXT_A    "count"
#define OPT_68_TXT_HELP          "--count"
#define                          TXT_HELP__COUNT \
        OPT_68_TXT_HELP


#define OPT_69_ENUM_NAME         OPT_SAD
#define OPT_69_OPT_PARSE         opt_parse_sad
#define OPT_69_HAS_ARG           y
#define OPT_69_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_69_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_69_CLI_LONG_TXT_A    "sad"
#define OPT_69_TXT_HELP          "--sad"
#define                          TXT_HELP__SAD \
        OPT_69_TXT_HELP


#define OPT_70_ENUM_NAME         OPT_SPD_ACTION
#define OPT_70_OPT_PARSE         opt_parse_spd_action
#define OPT_70_HAS_ARG           y
#define OPT_70_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_70_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_70_CLI_LONG_TXT_A    "spdact"
#define OPT_70_CLI_LONG_TXT_B    "spd-action"
#define OPT_70_TXT_HELP          "--spdact|--spd-action"
#define                          TXT_HELP__SPD_ACTION \
        OPT_70_TXT_HELP


#define OPT_71_ENUM_NAME         OPT_SPI
#define OPT_71_OPT_PARSE         opt_parse_spi
#define OPT_71_HAS_ARG           y
#define OPT_71_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_71_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_71_CLI_LONG_TXT_A    "spi"
#define OPT_71_TXT_HELP          "--spi"
#define                          TXT_HELP__SPI \
        OPT_71_TXT_HELP


#define OPT_72_ENUM_NAME         OPT_FLEXIBLE_FILTER
#define OPT_72_OPT_PARSE         opt_parse_flexible_filter
#define OPT_72_HAS_ARG           y
#define OPT_72_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_72_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_72_CLI_LONG_TXT_A    "ff"
#define OPT_72_CLI_LONG_TXT_B    "flexible-filter"
#define OPT_72_TXT_HELP          "--ff|--flexible-filter"
#define                          TXT_HELP__FLEXIBLE_FILTER \
        OPT_72_TXT_HELP


#define OPT_73_ENUM_NAME         OPT_VLAN_CONF
#define OPT_73_OPT_PARSE         opt_parse_vlan_conf
#define OPT_73_HAS_ARG           y
#define OPT_73_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_73_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_73_CLI_LONG_TXT_A    "vlan-conf"
#define OPT_73_TXT_HELP          "--vlan-conf"
#define                          TXT_HELP__VLAN_CONF \
        OPT_73_TXT_HELP


#define OPT_74_ENUM_NAME         OPT_PTP_CONF
#define OPT_74_OPT_PARSE         opt_parse_ptp_conf
#define OPT_74_HAS_ARG           y
#define OPT_74_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_74_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_74_CLI_LONG_TXT_A    "ptp-conf"
#define OPT_74_TXT_HELP          "--ptp-conf"
#define                          TXT_HELP__PTP_CONF \
        OPT_74_TXT_HELP


#define OPT_75_ENUM_NAME         OPT_PTP_PROMISC
#define OPT_75_OPT_PARSE         opt_parse_ptp_promisc
#define OPT_75_HAS_ARG           y
#define OPT_75_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_75_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_75_CLI_LONG_TXT_A    "ptp-promisc"
#define OPT_75_TXT_HELP          "--ptp-promisc"
#define                          TXT_HELP__PTP_PROMISC \
        OPT_75_TXT_HELP

/*      OPT_76_ENUM_NAME         formerly OPT_LOOPBACK ; currently reserved for future use */

#define OPT_77_ENUM_NAME         OPT_QINQ
#define OPT_77_OPT_PARSE         opt_parse_qinq
#define OPT_77_HAS_ARG           y
#define OPT_77_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_77_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_77_CLI_LONG_TXT_A    "qinq"
#define OPT_77_CLI_LONG_TXT_B    "q-in-q"
#define OPT_77_TXT_HELP          "--qinq|--q-in-q"
#define                          TXT_HELP__QINQ \
        OPT_77_TXT_HELP


#define OPT_78_ENUM_NAME         OPT_LOCAL
#define OPT_78_OPT_PARSE         opt_parse_local
#define OPT_78_HAS_ARG           y
#define OPT_78_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_78_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_78_CLI_LONG_TXT_A    "local"
#define OPT_78_TXT_HELP          "--local"
#define                          TXT_HELP__LOCAL \
        OPT_78_TXT_HELP


#define OPT_79_ENUM_NAME         OPT_DISCARD_ON_MATCH_SRC
#define OPT_79_OPT_PARSE         opt_parse_discard_on_match_src
#define OPT_79_HAS_ARG           y
#define OPT_79_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_79_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_79_CLI_LONG_TXT_A    "X-src"
#define OPT_79_CLI_LONG_TXT_B    "discard-on-match-src"
#define OPT_79_TXT_HELP          "--X-src|--discard-on-match-src"
#define                          TXT_HELP__DISCARD_ON_MATCH_SRC \
        OPT_79_TXT_HELP


#define OPT_80_ENUM_NAME         OPT_DISCARD_ON_MATCH_DST
#define OPT_80_OPT_PARSE         opt_parse_discard_on_match_dst
#define OPT_80_HAS_ARG           y
#define OPT_80_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_80_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_80_CLI_LONG_TXT_A    "X-dst"
#define OPT_80_CLI_LONG_TXT_B    "discard-on-match-dst"
#define OPT_80_TXT_HELP          "--X-dst|--discard-on-match-dst"
#define                          TXT_HELP__DISCARD_ON_MATCH_DST \
        OPT_80_TXT_HELP


#define OPT_81_ENUM_NAME         OPT_FEATURE
#define OPT_81_OPT_PARSE         opt_parse_feature
#define OPT_81_HAS_ARG           y
#define OPT_81_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_81_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_81_CLI_LONG_TXT_A    "feat"
#define OPT_81_CLI_LONG_TXT_B    "feature"
#define OPT_81_TXT_HELP          "--feat|--feature"
#define                          TXT_HELP__FEATURE \
        OPT_81_TXT_HELP


#define OPT_82_ENUM_NAME         OPT_STATIC
#define OPT_82_OPT_PARSE         opt_parse_static
#define OPT_82_HAS_ARG           n
#define OPT_82_INCOMPAT_GRPS     OPT_GRP_STATDYN
#define OPT_82_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_82_CLI_LONG_TXT_A    "stent"
#define OPT_82_CLI_LONG_TXT_B    "static"
#define OPT_82_TXT_HELP          "--stent|--static"
#define                          TXT_HELP__STATIC \
        OPT_82_TXT_HELP


#define OPT_83_ENUM_NAME         OPT_DYNAMIC
#define OPT_83_OPT_PARSE         opt_parse_dynamic
#define OPT_83_HAS_ARG           n
#define OPT_83_INCOMPAT_GRPS     OPT_GRP_STATDYN
#define OPT_83_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_83_CLI_LONG_TXT_A    "dent"
#define OPT_83_CLI_LONG_TXT_B    "dynamic"
#define OPT_83_TXT_HELP          "--dent|--dynamic"
#define                          TXT_HELP__DYNAMIC \
        OPT_83_TXT_HELP


#define OPT_84_ENUM_NAME         OPT_QUE
#define OPT_84_OPT_PARSE         opt_parse_que
#define OPT_84_HAS_ARG           y
#define OPT_84_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_84_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_84_CLI_LONG_TXT_A    "que"
#define OPT_84_TXT_HELP          "--que"
#define                          TXT_HELP__QUE \
        OPT_84_TXT_HELP


#define OPT_85_ENUM_NAME         OPT_SCH
#define OPT_85_OPT_PARSE         opt_parse_sch
#define OPT_85_HAS_ARG           y
#define OPT_85_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_85_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_85_CLI_LONG_TXT_A    "sch"
#define OPT_85_TXT_HELP          "--sch"
#define                          TXT_HELP__SCH \
        OPT_85_TXT_HELP


#define OPT_86_ENUM_NAME         OPT_SHP
#define OPT_86_OPT_PARSE         opt_parse_shp
#define OPT_86_HAS_ARG           y
#define OPT_86_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_86_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_86_CLI_LONG_TXT_A    "shp"
#define OPT_86_TXT_HELP          "--shp"
#define                          TXT_HELP__SHP \
        OPT_86_TXT_HELP


#define OPT_87_ENUM_NAME         OPT_QUE_MODE
#define OPT_87_OPT_PARSE         opt_parse_que_mode
#define OPT_87_HAS_ARG           y
#define OPT_87_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_87_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_87_CLI_LONG_TXT_A    "que-mode"
#define OPT_87_CLI_LONG_TXT_B    "qdisc"
#define OPT_87_TXT_HELP          "--que-mode|--qdisc"
#define                          TXT_HELP__QUE_MODE \
        OPT_87_TXT_HELP


#define OPT_88_ENUM_NAME         OPT_SCH_MODE
#define OPT_88_OPT_PARSE         opt_parse_sch_mode
#define OPT_88_HAS_ARG           y
#define OPT_88_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_88_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_88_CLI_LONG_TXT_A    "sch-mode"
#define OPT_88_TXT_HELP          "--sch-mode"
#define                          TXT_HELP__SCH_MODE \
        OPT_88_TXT_HELP


#define OPT_89_ENUM_NAME         OPT_SHP_MODE
#define OPT_89_OPT_PARSE         opt_parse_shp_mode
#define OPT_89_HAS_ARG           y
#define OPT_89_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_89_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_89_CLI_LONG_TXT_A    "shp-mode"
#define OPT_89_TXT_HELP          "--shp-mode"
#define                          TXT_HELP__SHP_MODE \
        OPT_89_TXT_HELP


#define OPT_90_ENUM_NAME         OPT_THMIN
#define OPT_90_OPT_PARSE         opt_parse_thmin
#define OPT_90_HAS_ARG           y
#define OPT_90_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_90_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_90_CLI_LONG_TXT_A    "thmin"
#define OPT_90_CLI_LONG_TXT_B    "thld-min"
#define OPT_90_TXT_HELP          "--thmin|--thld-min"
#define                          TXT_HELP__THMIN \
        OPT_90_TXT_HELP


#define OPT_91_ENUM_NAME         OPT_THMAX
#define OPT_91_OPT_PARSE         opt_parse_thmax
#define OPT_91_HAS_ARG           y
#define OPT_91_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_91_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_91_CLI_LONG_TXT_A    "thmax"
#define OPT_91_CLI_LONG_TXT_B    "thld-max"
#define OPT_91_TXT_HELP          "--thmax|--thld-max"
#define                          TXT_HELP__THMAX \
        OPT_91_TXT_HELP


#define OPT_92_ENUM_NAME         OPT_THFULL
#define OPT_92_OPT_PARSE         opt_parse_thfull
#define OPT_92_HAS_ARG           y
#define OPT_92_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_92_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_92_CLI_LONG_TXT_A    "thfull"
#define OPT_92_CLI_LONG_TXT_B    "thld-full"
#define OPT_92_TXT_HELP          "--thfull|--thld-full"
#define                          TXT_HELP__THFULL \
        OPT_92_TXT_HELP


#define OPT_93_ENUM_NAME         OPT_ZPROB
#define OPT_93_OPT_PARSE         opt_parse_zprob
#define OPT_93_HAS_ARG           y
#define OPT_93_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_93_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_93_CLI_LONG_TXT_A    "zprob"
#define OPT_93_TXT_HELP          "--zprob"
#define                          TXT_HELP__ZPROB \
        OPT_93_TXT_HELP


#define OPT_94_ENUM_NAME         OPT_SCH_ALGO
#define OPT_94_OPT_PARSE         opt_parse_sch_algo
#define OPT_94_HAS_ARG           y
#define OPT_94_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_94_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_94_CLI_LONG_TXT_A    "sch-algo"
#define OPT_94_TXT_HELP          "--sch-algo"
#define                          TXT_HELP__SCH_ALGO \
        OPT_94_TXT_HELP


#define OPT_95_ENUM_NAME         OPT_SCH_IN
#define OPT_95_OPT_PARSE         opt_parse_sch_in
#define OPT_95_HAS_ARG           y
#define OPT_95_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_95_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_95_CLI_LONG_TXT_A    "sch-in"
#define OPT_95_TXT_HELP          "--sch-in"
#define                          TXT_HELP__SCH_IN \
        OPT_95_TXT_HELP


#define OPT_96_ENUM_NAME         OPT_SHP_POS
#define OPT_96_OPT_PARSE         opt_parse_shp_pos
#define OPT_96_HAS_ARG           y
#define OPT_96_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_96_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_96_CLI_LONG_TXT_A    "shp-pos"
#define OPT_96_TXT_HELP          "--shp-pos"
#define                          TXT_HELP__SHP_POS \
        OPT_96_TXT_HELP


#define OPT_97_ENUM_NAME         OPT_ISL
#define OPT_97_OPT_PARSE         opt_parse_isl
#define OPT_97_HAS_ARG           y
#define OPT_97_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_97_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_97_CLI_LONG_TXT_A    "isl"
#define OPT_97_TXT_HELP          "--isl"
#define                          TXT_HELP__ISL \
        OPT_97_TXT_HELP


#define OPT_98_ENUM_NAME         OPT_CRMIN
#define OPT_98_OPT_PARSE         opt_parse_crmin
#define OPT_98_HAS_ARG           y
#define OPT_98_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_98_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_98_CLI_LONG_TXT_A    "crmin"
#define OPT_98_CLI_LONG_TXT_B    "credit-min"
#define OPT_98_TXT_HELP          "--crmin|--credit-min"
#define                          TXT_HELP__CRMIN \
        OPT_98_TXT_HELP


#define OPT_99_ENUM_NAME         OPT_CRMAX
#define OPT_99_OPT_PARSE         opt_parse_crmax
#define OPT_99_HAS_ARG           y
#define OPT_99_INCOMPAT_GRPS     OPT_GRP_NONE
#define OPT_99_CLI_SHORT_CODE    OPT_AUTO_CODE
#define OPT_99_CLI_LONG_TXT_A    "crmax"
#define OPT_99_CLI_LONG_TXT_B    "credit-max"
#define OPT_99_TXT_HELP          "--crmax|--credit-max"
#define                          TXT_HELP__CRMAX \
        OPT_99_TXT_HELP


#define OPT_100_ENUM_NAME        OPT_R_VLAN
#define OPT_100_OPT_PARSE        opt_parse_r_vlan
#define OPT_100_HAS_ARG          y
#define OPT_100_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_100_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_100_CLI_LONG_TXT_A   "r-vlan"
#define OPT_100_TXT_HELP         "--r-vlan"
#define                          TXT_HELP__R_VLAN \
        OPT_100_TXT_HELP


#define OPT_101_ENUM_NAME        OPT_TTL_DECR
#define OPT_101_OPT_PARSE        opt_parse_ttl_decr
#define OPT_101_HAS_ARG          y
#define OPT_101_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_101_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_101_CLI_LONG_TXT_A   "ttl-decr"
#define OPT_101_CLI_LONG_TXT_B   "decr-ttl"
#define OPT_101_TXT_HELP         "--ttl-decr | --decr-ttl"
#define                          TXT_HELP__TTL_DECR \
        OPT_101_TXT_HELP


#define OPT_102_ENUM_NAME        OPT_DISCARD_IF_TTL_BELOW_2
#define OPT_102_OPT_PARSE        opt_parse_discard_if_ttl_below_2
#define OPT_102_HAS_ARG          y
#define OPT_102_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_102_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_102_CLI_LONG_TXT_A   "X-ttl"
#define OPT_102_CLI_LONG_TXT_B   "discard-if-ttl-below-2"
#define OPT_102_TXT_HELP         "--X-ttl | --discard-if-ttl-below-2"
#define                          TXT_HELP__DISCARD_IF_TTL_BELOW_2 \
        OPT_102_TXT_HELP


#define OPT_103_ENUM_NAME        OPT_MODIFY_ACTIONS
#define OPT_103_OPT_PARSE        opt_parse_modify_actions
#define OPT_103_HAS_ARG          y
#define OPT_103_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_103_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_103_CLI_LONG_TXT_A   "modify-actions"
#define OPT_103_TXT_HELP         "--modify-actions"
#define                          TXT_HELP__MODIFY_ACTIONS \
        OPT_103_TXT_HELP


#define OPT_104_ENUM_NAME        OPT_WRED_QUE
#define OPT_104_OPT_PARSE        opt_parse_wred_que
#define OPT_104_HAS_ARG          y
#define OPT_104_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_104_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_104_CLI_LONG_TXT_A   "wred-que"
#define OPT_104_TXT_HELP         "--wred-que"
#define                          TXT_HELP__WRED_QUE \
        OPT_104_TXT_HELP


#define OPT_105_ENUM_NAME        OPT_SHP_TYPE
#define OPT_105_OPT_PARSE        opt_parse_shp_type
#define OPT_105_HAS_ARG          y
#define OPT_105_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_105_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_105_CLI_LONG_TXT_A   "shp-type"
#define OPT_105_TXT_HELP         "--shp-type"
#define                          TXT_HELP__SHP_TYPE \
        OPT_105_TXT_HELP


#define OPT_106_ENUM_NAME        OPT_FLOW_ACTION
#define OPT_106_OPT_PARSE        opt_parse_flow_action
#define OPT_106_HAS_ARG          y
#define OPT_106_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_106_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_106_CLI_LONG_TXT_A   "flowact"
#define OPT_106_CLI_LONG_TXT_B   "flow-action"
#define OPT_106_TXT_HELP         "--flowact|--flow-action"
#define                          TXT_HELP__FLOW_ACTION \
        OPT_106_TXT_HELP


#define OPT_107_ENUM_NAME        OPT_FLOW_TYPES
#define OPT_107_OPT_PARSE        opt_parse_flow_types
#define OPT_107_HAS_ARG          y
#define OPT_107_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_107_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_107_CLI_LONG_TXT_A   "ft"
#define OPT_107_CLI_LONG_TXT_B   "flow-types"
#define OPT_107_TXT_HELP         "--ft|--flow-types"
#define                          TXT_HELP__FLOW_TYPES \
        OPT_107_TXT_HELP


#define OPT_108_ENUM_NAME        OPT_TOS
#define OPT_108_OPT_PARSE        opt_parse_tos
#define OPT_108_HAS_ARG          y
#define OPT_108_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_108_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_108_CLI_LONG_TXT_A   "tos"
#define OPT_108_CLI_LONG_TXT_B   "tclass"
#define OPT_108_TXT_HELP         "--tos|--tclass"
#define                          TXT_HELP__TOS \
        OPT_108_TXT_HELP


#define OPT_109_ENUM_NAME        OPT_SPORT_MIN
#define OPT_109_OPT_PARSE        opt_parse_sport_min
#define OPT_109_HAS_ARG          y
#define OPT_109_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_109_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_109_CLI_LONG_TXT_A   "sport-min"
#define OPT_109_CLI_LONG_TXT_B   "src-port-min"
#define OPT_109_TXT_HELP         "--sport-min|--src-port-min"
#define                          TXT_HELP__SPORT_MIN \
        OPT_109_TXT_HELP


#define OPT_110_ENUM_NAME        OPT_SPORT_MAX
#define OPT_110_OPT_PARSE        opt_parse_sport_max
#define OPT_110_HAS_ARG          y
#define OPT_110_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_110_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_110_CLI_LONG_TXT_A   "sport-max"
#define OPT_110_CLI_LONG_TXT_B   "src-port-max"
#define OPT_110_TXT_HELP         "--sport-max|--src-port-max"
#define                          TXT_HELP__SPORT_MAX \
        OPT_110_TXT_HELP


#define OPT_111_ENUM_NAME        OPT_DPORT_MIN
#define OPT_111_OPT_PARSE        opt_parse_dport_min
#define OPT_111_HAS_ARG          y
#define OPT_111_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_111_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_111_CLI_LONG_TXT_A   "dport-min"
#define OPT_111_CLI_LONG_TXT_B   "dst-port-min"
#define OPT_111_TXT_HELP         "--dport-min|--dst-port-min"
#define                          TXT_HELP__DPORT_MIN \
        OPT_111_TXT_HELP


#define OPT_112_ENUM_NAME        OPT_DPORT_MAX
#define OPT_112_OPT_PARSE        opt_parse_dport_max
#define OPT_112_HAS_ARG          y
#define OPT_112_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_112_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_112_CLI_LONG_TXT_A   "dport-max"
#define OPT_112_CLI_LONG_TXT_B   "dst-port-max"
#define OPT_112_TXT_HELP         "--dport-max|--dst-port-max"
#define                          TXT_HELP__DPORT_MAX \
        OPT_112_TXT_HELP


#define OPT_113_ENUM_NAME        OPT_VLAN_MASK
#define OPT_113_OPT_PARSE        opt_parse_vlan_mask
#define OPT_113_HAS_ARG          y
#define OPT_113_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_113_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_113_CLI_LONG_TXT_A   "vlan-mask"
#define OPT_113_TXT_HELP         "--vlan-mask"
#define                          TXT_HELP__VLAN_MASK \
        OPT_113_TXT_HELP


#define OPT_114_ENUM_NAME        OPT_TOS_MASK
#define OPT_114_OPT_PARSE        opt_parse_tos_mask
#define OPT_114_HAS_ARG          y
#define OPT_114_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_114_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_114_CLI_LONG_TXT_A   "tos-mask"
#define OPT_114_CLI_LONG_TXT_B   "tclass-mask"
#define OPT_114_TXT_HELP         "--tos-mask|--tclass-mask"
#define                          TXT_HELP__TOS_MASK \
        OPT_114_TXT_HELP


#define OPT_115_ENUM_NAME        OPT_PROTOCOL_MASK
#define OPT_115_OPT_PARSE        opt_parse_protocol_mask
#define OPT_115_HAS_ARG          y
#define OPT_115_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_115_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_115_CLI_LONG_TXT_A   "p-mask"
#define OPT_115_CLI_LONG_TXT_B   "proto-mask"
#define OPT_115_CLI_LONG_TXT_C   "protocol-mask"
#define OPT_115_TXT_HELP         "--p-mask|--proto-mask|--protocol-mask"
#define                          TXT_HELP__PROTOCOL_MASK \
        OPT_115_TXT_HELP


#define OPT_116_ENUM_NAME        OPT_SIP_PFX
#define OPT_116_OPT_PARSE        opt_parse_sip_pfx
#define OPT_116_HAS_ARG          y
#define OPT_116_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_116_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_116_CLI_LONG_TXT_A   "s-pfx"
#define OPT_116_CLI_LONG_TXT_B   "sip-pfx"
#define OPT_116_CLI_LONG_TXT_C   "src-pfx"
#define OPT_116_TXT_HELP         "--s-pfx|--sip-pfx|--src-pfx"
#define                          TXT_HELP__SIP_PFX \
        OPT_116_TXT_HELP


#define OPT_117_ENUM_NAME        OPT_DIP_PFX
#define OPT_117_OPT_PARSE        opt_parse_dip_pfx
#define OPT_117_HAS_ARG          y
#define OPT_117_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_117_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_117_CLI_LONG_TXT_A   "d-pfx"
#define OPT_117_CLI_LONG_TXT_B   "dip-pfx"
#define OPT_117_CLI_LONG_TXT_C   "dst-pfx"
#define OPT_117_TXT_HELP         "--d-pfx|--dip-pfx|--dst-pfx"
#define                          TXT_HELP__DIP_PFX \
        OPT_117_TXT_HELP


#define OPT_118_ENUM_NAME        OPT_PTP_MGMT_IF
#define OPT_118_HAS_ARG          y
#define OPT_118_OPT_PARSE        opt_parse_ptp_mgmt_if
#define OPT_118_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_118_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_118_CLI_LONG_TXT_A   "ptp-mgmt-if"
#define OPT_118_TXT_HELP         "--ptp-mgmt-if"
#define                          TXT_HELP__PTP_MGMT_IF \
        OPT_118_TXT_HELP


#define OPT_119_ENUM_NAME        OPT_LOCK
#define OPT_119_OPT_PARSE        opt_parse_lock
#define OPT_119_HAS_ARG          n
#define OPT_119_INCOMPAT_GRPS    OPT_GRP_LOCKUNLOCK
#define OPT_119_CLI_SHORT_CODE   L
#define OPT_119_CLI_LONG_TXT_A   "lock"
#define OPT_119_TXT_HELP         "-L|--L|--lock"
#define                          TXT_HELP__LOCK \
        OPT_119_TXT_HELP


#define OPT_120_ENUM_NAME        OPT_UNLOCK
#define OPT_120_OPT_PARSE        opt_parse_unlock
#define OPT_120_HAS_ARG          n
#define OPT_120_INCOMPAT_GRPS    OPT_GRP_LOCKUNLOCK
#define OPT_120_CLI_SHORT_CODE   U
#define OPT_120_CLI_LONG_TXT_A   "unlock"
#define OPT_120_TXT_HELP         "-U|--U|--unlock"
#define                          TXT_HELP__UNLOCK \
        OPT_120_TXT_HELP


#define OPT_121_ENUM_NAME        OPT_PRINT_TO_TERMINAL
#define OPT_121_HAS_ARG          y
#define OPT_121_OPT_PARSE        opt_print_to_terminal
#define OPT_121_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_121_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_121_CLI_LONG_TXT_A   "prn2term"
#define OPT_121_CLI_LONG_TXT_B   "print-to-terminal"
#define OPT_121_TXT_HELP         "--prn2term|--print-to-terminal"
#define                          TXT_HELP__PRINT_TO_TERMINAL \
        OPT_121_TXT_HELP


#define OPT_122_ENUM_NAME        OPT_PRINT_TO_LOGFILE
#define OPT_122_HAS_ARG          y
#define OPT_122_OPT_PARSE        opt_print_to_logfile
#define OPT_122_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_122_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_122_CLI_LONG_TXT_A   "prn2log"
#define OPT_122_CLI_LONG_TXT_B   "print-to-logfile"
#define OPT_122_TXT_HELP         "--prn2log|--print-to-logfile"
#define                          TXT_HELP__PRINT_TO_LOGFILE \
        OPT_122_TXT_HELP


#define OPT_123_ENUM_NAME        OPT_DBG_TO_TERMINAL
#define OPT_123_HAS_ARG          y
#define OPT_123_OPT_PARSE        opt_dbg_to_terminal
#define OPT_123_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_123_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_123_CLI_LONG_TXT_A   "dbg2term"
#define OPT_123_CLI_LONG_TXT_B   "dbg-to-terminal"
#define OPT_123_CLI_LONG_TXT_C   "debug-to-terminal"
#define OPT_123_TXT_HELP         "--dbg2term|--dbg-to-terminal|--debug-to-terminal"
#define                          TXT_HELP__DBG_TO_TERMINAL \
        OPT_123_TXT_HELP


#define OPT_124_ENUM_NAME        OPT_DBG_TO_DBGFILE
#define OPT_124_HAS_ARG          y
#define OPT_124_OPT_PARSE        opt_dbg_to_dbgfile
#define OPT_124_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_124_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_124_CLI_LONG_TXT_A   "dbg2dbg"
#define OPT_124_CLI_LONG_TXT_B   "dbg-to-dbgfile"
#define OPT_124_CLI_LONG_TXT_C   "debug-to-dbgfile"
#define OPT_124_TXT_HELP         "--dbg2dbg|--dbg-to-dbgfile|--debug-to-dbgfile"
#define                          TXT_HELP__DBG_TO_DBGFILE \
        OPT_124_TXT_HELP


#define OPT_125_ENUM_NAME        OPT_ELEMENT
#define OPT_125_OPT_PARSE        opt_parse_element
#define OPT_125_HAS_ARG          y
#define OPT_125_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_125_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_125_CLI_LONG_TXT_A   "el"
#define OPT_125_CLI_LONG_TXT_B   "element"
#define OPT_125_TXT_HELP         "--el|--element"
#define                          TXT_HELP__ELEMENT \
        OPT_125_TXT_HELP


#define OPT_126_ENUM_NAME        OPT_ELEMENT_GROUP
#define OPT_126_OPT_PARSE        opt_parse_element_group
#define OPT_126_HAS_ARG          y
#define OPT_126_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_126_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_126_CLI_LONG_TXT_A   "elgrp"
#define OPT_126_CLI_LONG_TXT_B   "el-group"
#define OPT_126_TXT_HELP         "--elgrp|--el-group"
#define                          TXT_HELP__ELEMENT_GROUP \
        OPT_126_TXT_HELP


#define OPT_127_ENUM_NAME        OPT_UNIT_SIZE
#define OPT_127_OPT_PARSE        opt_parse_unit_size
#define OPT_127_HAS_ARG          y
#define OPT_127_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_127_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_127_CLI_LONG_TXT_A   "us"
#define OPT_127_CLI_LONG_TXT_B   "unit_size"
#define OPT_127_TXT_HELP         "--us|--unit-size"
#define                          TXT_HELP__UNIT_SIZE \
        OPT_127_TXT_HELP


#define OPT_128_ENUM_NAME        OPT_PAYLOAD
#define OPT_128_OPT_PARSE        opt_parse_payload
#define OPT_128_HAS_ARG          y
#define OPT_128_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_128_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_128_CLI_LONG_TXT_A   "payload"
#define OPT_128_TXT_HELP         "--payload"
#define                          TXT_HELP__PAYLOAD \
        OPT_128_TXT_HELP


#define OPT_129_ENUM_NAME        OPT_FF_ALL_TCP
#define OPT_129_OPT_PARSE        opt_parse_ff_all_tcp
#define OPT_129_HAS_ARG          y
#define OPT_129_INCOMPAT_GRPS    OPT_GRP_NONE
#define OPT_129_CLI_SHORT_CODE   OPT_AUTO_CODE
#define OPT_129_CLI_LONG_TXT_A   "ff-all-tcp"
#define OPT_129_CLI_LONG_TXT_B   "fast-forward-all-tcp"
#define OPT_129_TXT_HELP         "--ff-all-tcp|--fast-forward-all-tcp"
#define                          TXT_HELP__FF_ALL_TCP \
        OPT_129_TXT_HELP




/* OPT_LAST (keep this at the bottom of the cli option definition list) */

/* ==== TYPEDEFS & DATA ==================================================== */

/* symbols for automatically generated content */
#define OPT_MC_STR_AUX(ARG)          #ARG
#define OPT_MC_STR(ARG)              OPT_MC_STR_AUX(ARG)
#define OPT_MC_CCAT_AUX(ARG1, ARG2)  ARG1 ## ARG2
#define OPT_MC_CCAT(ARG1, ARG2)      OPT_MC_CCAT_AUX(ARG1, ARG2)
#define OPT_CHR_0  '0'
#define OPT_CHR_1  '1'
#define OPT_CHR_2  '2'
#define OPT_CHR_3  '3'
#define OPT_CHR_4  '4'
#define OPT_CHR_5  '5'
#define OPT_CHR_6  '6'
#define OPT_CHR_7  '7'
#define OPT_CHR_8  '8'
#define OPT_CHR_9  '9'
#define OPT_CHR_a  'a'
#define OPT_CHR_b  'b'
#define OPT_CHR_c  'c'
#define OPT_CHR_d  'd'
#define OPT_CHR_e  'e'
#define OPT_CHR_f  'f'
#define OPT_CHR_g  'g'
#define OPT_CHR_h  'h'
#define OPT_CHR_i  'i'
#define OPT_CHR_j  'j'
#define OPT_CHR_k  'k'
#define OPT_CHR_l  'l'
#define OPT_CHR_m  'm'
#define OPT_CHR_n  'n'
#define OPT_CHR_o  'o'
#define OPT_CHR_p  'p'
#define OPT_CHR_q  'q'
#define OPT_CHR_r  'r'
#define OPT_CHR_s  's'
#define OPT_CHR_t  't'
#define OPT_CHR_u  'u'
#define OPT_CHR_v  'v'
#define OPT_CHR_w  'w'
#define OPT_CHR_x  'x'
#define OPT_CHR_y  'y'
#define OPT_CHR_z  'z'
#define OPT_CHR_A  'A'
#define OPT_CHR_B  'B'
#define OPT_CHR_C  'C'
#define OPT_CHR_D  'D'
#define OPT_CHR_E  'E'
#define OPT_CHR_F  'F'
#define OPT_CHR_G  'G'
#define OPT_CHR_H  'H'
#define OPT_CHR_I  'I'
#define OPT_CHR_J  'J'
#define OPT_CHR_K  'K'
#define OPT_CHR_L  'L'
#define OPT_CHR_M  'M'
#define OPT_CHR_N  'N'
#define OPT_CHR_O  'O'
#define OPT_CHR_P  'P'
#define OPT_CHR_Q  'Q'
#define OPT_CHR_R  'R'
#define OPT_CHR_S  'S'
#define OPT_CHR_T  'T'
#define OPT_CHR_U  'U'
#define OPT_CHR_V  'V'
#define OPT_CHR_W  'W'
#define OPT_CHR_X  'X'
#define OPT_CHR_Y  'Y'
#define OPT_CHR_Z  'Z'


#ifdef OPT_01_ENUM_NAME
  #define OPT_01_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_01_HAS_ARG)
  #if (OPT_01_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_01_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_01_CLI_SHORT_CODE)
    #define OPT_01_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_01_CLI_SHORT_CODE)
  #else
    #define OPT_01_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_02_ENUM_NAME
  #define OPT_02_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_02_HAS_ARG)
  #if (OPT_02_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_02_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_02_CLI_SHORT_CODE)
    #define OPT_02_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_02_CLI_SHORT_CODE)
  #else
    #define OPT_02_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_03_ENUM_NAME
  #define OPT_03_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_03_HAS_ARG)
  #if (OPT_03_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_03_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_03_CLI_SHORT_CODE)
    #define OPT_03_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_03_CLI_SHORT_CODE)
  #else
    #define OPT_03_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_04_ENUM_NAME
  #define OPT_04_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_04_HAS_ARG)
  #if (OPT_04_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_04_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_04_CLI_SHORT_CODE)
    #define OPT_04_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_04_CLI_SHORT_CODE)
  #else
    #define OPT_04_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_05_ENUM_NAME
  #define OPT_05_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_05_HAS_ARG)
  #if (OPT_05_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_05_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_05_CLI_SHORT_CODE)
    #define OPT_05_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_05_CLI_SHORT_CODE)
  #else
    #define OPT_05_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_06_ENUM_NAME
  #define OPT_06_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_06_HAS_ARG)
  #if (OPT_06_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_06_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_06_CLI_SHORT_CODE)
    #define OPT_06_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_06_CLI_SHORT_CODE)
  #else
    #define OPT_06_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_07_ENUM_NAME
  #define OPT_07_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_07_HAS_ARG)
  #if (OPT_07_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_07_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_07_CLI_SHORT_CODE)
    #define OPT_07_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_07_CLI_SHORT_CODE)
  #else
    #define OPT_07_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_08_ENUM_NAME
  #define OPT_08_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_08_HAS_ARG)
  #if (OPT_08_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_08_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_08_CLI_SHORT_CODE)
    #define OPT_08_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_08_CLI_SHORT_CODE)
  #else
    #define OPT_08_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_09_ENUM_NAME
  #define OPT_09_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_09_HAS_ARG)
  #if (OPT_09_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_09_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_09_CLI_SHORT_CODE)
    #define OPT_09_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_09_CLI_SHORT_CODE)
  #else
    #define OPT_09_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_10_ENUM_NAME
  #define OPT_10_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_10_HAS_ARG)
  #if (OPT_10_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_10_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_10_CLI_SHORT_CODE)
    #define OPT_10_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_10_CLI_SHORT_CODE)
  #else
    #define OPT_10_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_11_ENUM_NAME
  #define OPT_11_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_11_HAS_ARG)
  #if (OPT_11_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_11_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_11_CLI_SHORT_CODE)
    #define OPT_11_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_11_CLI_SHORT_CODE)
  #else
    #define OPT_11_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_12_ENUM_NAME
  #define OPT_12_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_12_HAS_ARG)
  #if (OPT_12_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_12_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_12_CLI_SHORT_CODE)
    #define OPT_12_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_12_CLI_SHORT_CODE)
  #else
    #define OPT_12_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_13_ENUM_NAME
  #define OPT_13_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_13_HAS_ARG)
  #if (OPT_13_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_13_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_13_CLI_SHORT_CODE)
    #define OPT_13_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_13_CLI_SHORT_CODE)
  #else
    #define OPT_13_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_14_ENUM_NAME
  #define OPT_14_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_14_HAS_ARG)
  #if (OPT_14_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_14_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_14_CLI_SHORT_CODE)
    #define OPT_14_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_14_CLI_SHORT_CODE)
  #else
    #define OPT_14_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_15_ENUM_NAME
  #define OPT_15_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_15_HAS_ARG)
  #if (OPT_15_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_15_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_15_CLI_SHORT_CODE)
    #define OPT_15_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_15_CLI_SHORT_CODE)
  #else
    #define OPT_15_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_16_ENUM_NAME
  #define OPT_16_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_16_HAS_ARG)
  #if (OPT_16_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_16_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_16_CLI_SHORT_CODE)
    #define OPT_16_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_16_CLI_SHORT_CODE)
  #else
    #define OPT_16_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_17_ENUM_NAME
  #define OPT_17_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_17_HAS_ARG)
  #if (OPT_17_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_17_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_17_CLI_SHORT_CODE)
    #define OPT_17_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_17_CLI_SHORT_CODE)
  #else
    #define OPT_17_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_18_ENUM_NAME
  #define OPT_18_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_18_HAS_ARG)
  #if (OPT_18_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_18_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_18_CLI_SHORT_CODE)
    #define OPT_18_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_18_CLI_SHORT_CODE)
  #else
    #define OPT_18_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_19_ENUM_NAME
  #define OPT_19_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_19_HAS_ARG)
  #if (OPT_19_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_19_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_19_CLI_SHORT_CODE)
    #define OPT_19_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_19_CLI_SHORT_CODE)
  #else
    #define OPT_19_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_20_ENUM_NAME
  #define OPT_20_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_20_HAS_ARG)
  #if (OPT_20_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_20_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_20_CLI_SHORT_CODE)
    #define OPT_20_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_20_CLI_SHORT_CODE)
  #else
    #define OPT_20_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_21_ENUM_NAME
  #define OPT_21_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_21_HAS_ARG)
  #if (OPT_21_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_21_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_21_CLI_SHORT_CODE)
    #define OPT_21_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_21_CLI_SHORT_CODE)
  #else
    #define OPT_21_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_22_ENUM_NAME
  #define OPT_22_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_22_HAS_ARG)
  #if (OPT_22_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_22_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_22_CLI_SHORT_CODE)
    #define OPT_22_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_22_CLI_SHORT_CODE)
  #else
    #define OPT_22_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_23_ENUM_NAME
  #define OPT_23_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_23_HAS_ARG)
  #if (OPT_23_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_23_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_23_CLI_SHORT_CODE)
    #define OPT_23_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_23_CLI_SHORT_CODE)
  #else
    #define OPT_23_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_24_ENUM_NAME
  #define OPT_24_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_24_HAS_ARG)
  #if (OPT_24_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_24_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_24_CLI_SHORT_CODE)
    #define OPT_24_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_24_CLI_SHORT_CODE)
  #else
    #define OPT_24_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_25_ENUM_NAME
  #define OPT_25_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_25_HAS_ARG)
  #if (OPT_25_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_25_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_25_CLI_SHORT_CODE)
    #define OPT_25_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_25_CLI_SHORT_CODE)
  #else
    #define OPT_25_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_26_ENUM_NAME
  #define OPT_26_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_26_HAS_ARG)
  #if (OPT_26_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_26_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_26_CLI_SHORT_CODE)
    #define OPT_26_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_26_CLI_SHORT_CODE)
  #else
    #define OPT_26_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_27_ENUM_NAME
  #define OPT_27_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_27_HAS_ARG)
  #if (OPT_27_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_27_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_27_CLI_SHORT_CODE)
    #define OPT_27_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_27_CLI_SHORT_CODE)
  #else
    #define OPT_27_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_28_ENUM_NAME
  #define OPT_28_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_28_HAS_ARG)
  #if (OPT_28_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_28_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_28_CLI_SHORT_CODE)
    #define OPT_28_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_28_CLI_SHORT_CODE)
  #else
    #define OPT_28_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_29_ENUM_NAME
  #define OPT_29_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_29_HAS_ARG)
  #if (OPT_29_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_29_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_29_CLI_SHORT_CODE)
    #define OPT_29_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_29_CLI_SHORT_CODE)
  #else
    #define OPT_29_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_30_ENUM_NAME
  #define OPT_30_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_30_HAS_ARG)
  #if (OPT_30_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_30_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_30_CLI_SHORT_CODE)
    #define OPT_30_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_30_CLI_SHORT_CODE)
  #else
    #define OPT_30_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_31_ENUM_NAME
  #define OPT_31_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_31_HAS_ARG)
  #if (OPT_31_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_31_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_31_CLI_SHORT_CODE)
    #define OPT_31_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_31_CLI_SHORT_CODE)
  #else
    #define OPT_31_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_32_ENUM_NAME
  #define OPT_32_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_32_HAS_ARG)
  #if (OPT_32_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_32_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_32_CLI_SHORT_CODE)
    #define OPT_32_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_32_CLI_SHORT_CODE)
  #else
    #define OPT_32_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_33_ENUM_NAME
  #define OPT_33_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_33_HAS_ARG)
  #if (OPT_33_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_33_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_33_CLI_SHORT_CODE)
    #define OPT_33_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_33_CLI_SHORT_CODE)
  #else
    #define OPT_33_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_34_ENUM_NAME
  #define OPT_34_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_34_HAS_ARG)
  #if (OPT_34_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_34_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_34_CLI_SHORT_CODE)
    #define OPT_34_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_34_CLI_SHORT_CODE)
  #else
    #define OPT_34_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_35_ENUM_NAME
  #define OPT_35_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_35_HAS_ARG)
  #if (OPT_35_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_35_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_35_CLI_SHORT_CODE)
    #define OPT_35_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_35_CLI_SHORT_CODE)
  #else
    #define OPT_35_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_36_ENUM_NAME
  #define OPT_36_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_36_HAS_ARG)
  #if (OPT_36_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_36_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_36_CLI_SHORT_CODE)
    #define OPT_36_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_36_CLI_SHORT_CODE)
  #else
    #define OPT_36_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_37_ENUM_NAME
  #define OPT_37_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_37_HAS_ARG)
  #if (OPT_37_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_37_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_37_CLI_SHORT_CODE)
    #define OPT_37_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_37_CLI_SHORT_CODE)
  #else
    #define OPT_37_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_38_ENUM_NAME
  #define OPT_38_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_38_HAS_ARG)
  #if (OPT_38_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_38_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_38_CLI_SHORT_CODE)
    #define OPT_38_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_38_CLI_SHORT_CODE)
  #else
    #define OPT_38_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_39_ENUM_NAME
  #define OPT_39_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_39_HAS_ARG)
  #if (OPT_39_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_39_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_39_CLI_SHORT_CODE)
    #define OPT_39_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_39_CLI_SHORT_CODE)
  #else
    #define OPT_39_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_40_ENUM_NAME
  #define OPT_40_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_40_HAS_ARG)
  #if (OPT_40_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_40_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_40_CLI_SHORT_CODE)
    #define OPT_40_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_40_CLI_SHORT_CODE)
  #else
    #define OPT_40_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_41_ENUM_NAME
  #define OPT_41_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_41_HAS_ARG)
  #if (OPT_41_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_41_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_41_CLI_SHORT_CODE)
    #define OPT_41_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_41_CLI_SHORT_CODE)
  #else
    #define OPT_41_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_42_ENUM_NAME
  #define OPT_42_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_42_HAS_ARG)
  #if (OPT_42_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_42_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_42_CLI_SHORT_CODE)
    #define OPT_42_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_42_CLI_SHORT_CODE)
  #else
    #define OPT_42_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_43_ENUM_NAME
  #define OPT_43_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_43_HAS_ARG)
  #if (OPT_43_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_43_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_43_CLI_SHORT_CODE)
    #define OPT_43_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_43_CLI_SHORT_CODE)
  #else
    #define OPT_43_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_44_ENUM_NAME
  #define OPT_44_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_44_HAS_ARG)
  #if (OPT_44_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_44_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_44_CLI_SHORT_CODE)
    #define OPT_44_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_44_CLI_SHORT_CODE)
  #else
    #define OPT_44_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_45_ENUM_NAME
  #define OPT_45_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_45_HAS_ARG)
  #if (OPT_45_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_45_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_45_CLI_SHORT_CODE)
    #define OPT_45_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_45_CLI_SHORT_CODE)
  #else
    #define OPT_45_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_46_ENUM_NAME
  #define OPT_46_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_46_HAS_ARG)
  #if (OPT_46_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_46_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_46_CLI_SHORT_CODE)
    #define OPT_46_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_46_CLI_SHORT_CODE)
  #else
    #define OPT_46_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_47_ENUM_NAME
  #define OPT_47_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_47_HAS_ARG)
  #if (OPT_47_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_47_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_47_CLI_SHORT_CODE)
    #define OPT_47_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_47_CLI_SHORT_CODE)
  #else
    #define OPT_47_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_48_ENUM_NAME
  #define OPT_48_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_48_HAS_ARG)
  #if (OPT_48_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_48_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_48_CLI_SHORT_CODE)
    #define OPT_48_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_48_CLI_SHORT_CODE)
  #else
    #define OPT_48_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_49_ENUM_NAME
  #define OPT_49_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_49_HAS_ARG)
  #if (OPT_49_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_49_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_49_CLI_SHORT_CODE)
    #define OPT_49_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_49_CLI_SHORT_CODE)
  #else
    #define OPT_49_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_50_ENUM_NAME
  #define OPT_50_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_50_HAS_ARG)
  #if (OPT_50_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_50_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_50_CLI_SHORT_CODE)
    #define OPT_50_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_50_CLI_SHORT_CODE)
  #else
    #define OPT_50_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_51_ENUM_NAME
  #define OPT_51_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_51_HAS_ARG)
  #if (OPT_51_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_51_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_51_CLI_SHORT_CODE)
    #define OPT_51_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_51_CLI_SHORT_CODE)
  #else
    #define OPT_51_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_52_ENUM_NAME
  #define OPT_52_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_52_HAS_ARG)
  #if (OPT_52_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_52_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_52_CLI_SHORT_CODE)
    #define OPT_52_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_52_CLI_SHORT_CODE)
  #else
    #define OPT_52_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_53_ENUM_NAME
  #define OPT_53_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_53_HAS_ARG)
  #if (OPT_53_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_53_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_53_CLI_SHORT_CODE)
    #define OPT_53_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_53_CLI_SHORT_CODE)
  #else
    #define OPT_53_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_54_ENUM_NAME
  #define OPT_54_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_54_HAS_ARG)
  #if (OPT_54_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_54_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_54_CLI_SHORT_CODE)
    #define OPT_54_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_54_CLI_SHORT_CODE)
  #else
    #define OPT_54_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_55_ENUM_NAME
  #define OPT_55_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_55_HAS_ARG)
  #if (OPT_55_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_55_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_55_CLI_SHORT_CODE)
    #define OPT_55_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_55_CLI_SHORT_CODE)
  #else
    #define OPT_55_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_56_ENUM_NAME
  #define OPT_56_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_56_HAS_ARG)
  #if (OPT_56_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_56_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_56_CLI_SHORT_CODE)
    #define OPT_56_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_56_CLI_SHORT_CODE)
  #else
    #define OPT_56_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_57_ENUM_NAME
  #define OPT_57_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_57_HAS_ARG)
  #if (OPT_57_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_57_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_57_CLI_SHORT_CODE)
    #define OPT_57_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_57_CLI_SHORT_CODE)
  #else
    #define OPT_57_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_58_ENUM_NAME
  #define OPT_58_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_58_HAS_ARG)
  #if (OPT_58_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_58_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_58_CLI_SHORT_CODE)
    #define OPT_58_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_58_CLI_SHORT_CODE)
  #else
    #define OPT_58_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_59_ENUM_NAME
  #define OPT_59_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_59_HAS_ARG)
  #if (OPT_59_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_59_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_59_CLI_SHORT_CODE)
    #define OPT_59_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_59_CLI_SHORT_CODE)
  #else
    #define OPT_59_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_60_ENUM_NAME
  #define OPT_60_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_60_HAS_ARG)
  #if (OPT_60_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_60_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_60_CLI_SHORT_CODE)
    #define OPT_60_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_60_CLI_SHORT_CODE)
  #else
    #define OPT_60_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_61_ENUM_NAME
  #define OPT_61_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_61_HAS_ARG)
  #if (OPT_61_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_61_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_61_CLI_SHORT_CODE)
    #define OPT_61_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_61_CLI_SHORT_CODE)
  #else
    #define OPT_61_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_62_ENUM_NAME
  #define OPT_62_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_62_HAS_ARG)
  #if (OPT_62_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_62_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_62_CLI_SHORT_CODE)
    #define OPT_62_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_62_CLI_SHORT_CODE)
  #else
    #define OPT_62_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_63_ENUM_NAME
  #define OPT_63_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_63_HAS_ARG)
  #if (OPT_63_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_63_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_63_CLI_SHORT_CODE)
    #define OPT_63_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_63_CLI_SHORT_CODE)
  #else
    #define OPT_63_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_64_ENUM_NAME
  #define OPT_64_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_64_HAS_ARG)
  #if (OPT_64_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_64_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_64_CLI_SHORT_CODE)
    #define OPT_64_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_64_CLI_SHORT_CODE)
  #else
    #define OPT_64_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_65_ENUM_NAME
  #define OPT_65_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_65_HAS_ARG)
  #if (OPT_65_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_65_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_65_CLI_SHORT_CODE)
    #define OPT_65_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_65_CLI_SHORT_CODE)
  #else
    #define OPT_65_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_66_ENUM_NAME
  #define OPT_66_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_66_HAS_ARG)
  #if (OPT_66_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_66_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_66_CLI_SHORT_CODE)
    #define OPT_66_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_66_CLI_SHORT_CODE)
  #else
    #define OPT_66_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_67_ENUM_NAME
  #define OPT_67_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_67_HAS_ARG)
  #if (OPT_67_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_67_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_67_CLI_SHORT_CODE)
    #define OPT_67_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_67_CLI_SHORT_CODE)
  #else
    #define OPT_67_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_68_ENUM_NAME
  #define OPT_68_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_68_HAS_ARG)
  #if (OPT_68_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_68_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_68_CLI_SHORT_CODE)
    #define OPT_68_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_68_CLI_SHORT_CODE)
  #else
    #define OPT_68_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_69_ENUM_NAME
  #define OPT_69_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_69_HAS_ARG)
  #if (OPT_69_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_69_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_69_CLI_SHORT_CODE)
    #define OPT_69_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_69_CLI_SHORT_CODE)
  #else
    #define OPT_69_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_70_ENUM_NAME
  #define OPT_70_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_70_HAS_ARG)
  #if (OPT_70_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_70_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_70_CLI_SHORT_CODE)
    #define OPT_70_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_70_CLI_SHORT_CODE)
  #else
    #define OPT_70_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_71_ENUM_NAME
  #define OPT_71_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_71_HAS_ARG)
  #if (OPT_71_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_71_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_71_CLI_SHORT_CODE)
    #define OPT_71_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_71_CLI_SHORT_CODE)
  #else
    #define OPT_71_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_72_ENUM_NAME
  #define OPT_72_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_72_HAS_ARG)
  #if (OPT_72_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_72_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_72_CLI_SHORT_CODE)
    #define OPT_72_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_72_CLI_SHORT_CODE)
  #else
    #define OPT_72_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_73_ENUM_NAME
  #define OPT_73_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_73_HAS_ARG)
  #if (OPT_73_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_73_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_73_CLI_SHORT_CODE)
    #define OPT_73_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_73_CLI_SHORT_CODE)
  #else
    #define OPT_73_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_74_ENUM_NAME
  #define OPT_74_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_74_HAS_ARG)
  #if (OPT_74_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_74_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_74_CLI_SHORT_CODE)
    #define OPT_74_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_74_CLI_SHORT_CODE)
  #else
    #define OPT_74_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_75_ENUM_NAME
  #define OPT_75_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_75_HAS_ARG)
  #if (OPT_75_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_75_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_75_CLI_SHORT_CODE)
    #define OPT_75_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_75_CLI_SHORT_CODE)
  #else
    #define OPT_75_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_76_ENUM_NAME
  #define OPT_76_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_76_HAS_ARG)
  #if (OPT_76_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_76_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_76_CLI_SHORT_CODE)
    #define OPT_76_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_76_CLI_SHORT_CODE)
  #else
    #define OPT_76_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_77_ENUM_NAME
  #define OPT_77_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_77_HAS_ARG)
  #if (OPT_77_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_77_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_77_CLI_SHORT_CODE)
    #define OPT_77_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_77_CLI_SHORT_CODE)
  #else
    #define OPT_77_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_78_ENUM_NAME
  #define OPT_78_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_78_HAS_ARG)
  #if (OPT_78_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_78_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_78_CLI_SHORT_CODE)
    #define OPT_78_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_78_CLI_SHORT_CODE)
  #else
    #define OPT_78_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_79_ENUM_NAME
  #define OPT_79_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_79_HAS_ARG)
  #if (OPT_79_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_79_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_79_CLI_SHORT_CODE)
    #define OPT_79_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_79_CLI_SHORT_CODE)
  #else
    #define OPT_79_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_80_ENUM_NAME
  #define OPT_80_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_80_HAS_ARG)
  #if (OPT_80_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_80_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_80_CLI_SHORT_CODE)
    #define OPT_80_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_80_CLI_SHORT_CODE)
  #else
    #define OPT_80_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_81_ENUM_NAME
  #define OPT_81_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_81_HAS_ARG)
  #if (OPT_81_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_81_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_81_CLI_SHORT_CODE)
    #define OPT_81_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_81_CLI_SHORT_CODE)
  #else
    #define OPT_81_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_82_ENUM_NAME
  #define OPT_82_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_82_HAS_ARG)
  #if (OPT_82_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_82_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_82_CLI_SHORT_CODE)
    #define OPT_82_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_82_CLI_SHORT_CODE)
  #else
    #define OPT_82_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_83_ENUM_NAME
  #define OPT_83_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_83_HAS_ARG)
  #if (OPT_83_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_83_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_83_CLI_SHORT_CODE)
    #define OPT_83_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_83_CLI_SHORT_CODE)
  #else
    #define OPT_83_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_84_ENUM_NAME
  #define OPT_84_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_84_HAS_ARG)
  #if (OPT_84_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_84_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_84_CLI_SHORT_CODE)
    #define OPT_84_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_84_CLI_SHORT_CODE)
  #else
    #define OPT_84_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_85_ENUM_NAME
  #define OPT_85_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_85_HAS_ARG)
  #if (OPT_85_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_85_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_85_CLI_SHORT_CODE)
    #define OPT_85_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_85_CLI_SHORT_CODE)
  #else
    #define OPT_85_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_86_ENUM_NAME
  #define OPT_86_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_86_HAS_ARG)
  #if (OPT_86_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_86_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_86_CLI_SHORT_CODE)
    #define OPT_86_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_86_CLI_SHORT_CODE)
  #else
    #define OPT_86_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_87_ENUM_NAME
  #define OPT_87_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_87_HAS_ARG)
  #if (OPT_87_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_87_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_87_CLI_SHORT_CODE)
    #define OPT_87_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_87_CLI_SHORT_CODE)
  #else
    #define OPT_87_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_88_ENUM_NAME
  #define OPT_88_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_88_HAS_ARG)
  #if (OPT_88_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_88_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_88_CLI_SHORT_CODE)
    #define OPT_88_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_88_CLI_SHORT_CODE)
  #else
    #define OPT_88_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_89_ENUM_NAME
  #define OPT_89_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_89_HAS_ARG)
  #if (OPT_89_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_89_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_89_CLI_SHORT_CODE)
    #define OPT_89_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_89_CLI_SHORT_CODE)
  #else
    #define OPT_89_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_90_ENUM_NAME
  #define OPT_90_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_90_HAS_ARG)
  #if (OPT_90_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_90_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_90_CLI_SHORT_CODE)
    #define OPT_90_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_90_CLI_SHORT_CODE)
  #else
    #define OPT_90_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_91_ENUM_NAME
  #define OPT_91_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_91_HAS_ARG)
  #if (OPT_91_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_91_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_91_CLI_SHORT_CODE)
    #define OPT_91_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_91_CLI_SHORT_CODE)
  #else
    #define OPT_91_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_92_ENUM_NAME
  #define OPT_92_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_92_HAS_ARG)
  #if (OPT_92_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_92_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_92_CLI_SHORT_CODE)
    #define OPT_92_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_92_CLI_SHORT_CODE)
  #else
    #define OPT_92_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_93_ENUM_NAME
  #define OPT_93_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_93_HAS_ARG)
  #if (OPT_93_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_93_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_93_CLI_SHORT_CODE)
    #define OPT_93_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_93_CLI_SHORT_CODE)
  #else
    #define OPT_93_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_94_ENUM_NAME
  #define OPT_94_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_94_HAS_ARG)
  #if (OPT_94_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_94_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_94_CLI_SHORT_CODE)
    #define OPT_94_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_94_CLI_SHORT_CODE)
  #else
    #define OPT_94_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_95_ENUM_NAME
  #define OPT_95_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_95_HAS_ARG)
  #if (OPT_95_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_95_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_95_CLI_SHORT_CODE)
    #define OPT_95_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_95_CLI_SHORT_CODE)
  #else
    #define OPT_95_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_96_ENUM_NAME
  #define OPT_96_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_96_HAS_ARG)
  #if (OPT_96_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_96_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_96_CLI_SHORT_CODE)
    #define OPT_96_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_96_CLI_SHORT_CODE)
  #else
    #define OPT_96_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_97_ENUM_NAME
  #define OPT_97_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_97_HAS_ARG)
  #if (OPT_97_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_97_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_97_CLI_SHORT_CODE)
    #define OPT_97_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_97_CLI_SHORT_CODE)
  #else
    #define OPT_97_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_98_ENUM_NAME
  #define OPT_98_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_98_HAS_ARG)
  #if (OPT_98_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_98_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_98_CLI_SHORT_CODE)
    #define OPT_98_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_98_CLI_SHORT_CODE)
  #else
    #define OPT_98_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_99_ENUM_NAME
  #define OPT_99_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_99_HAS_ARG)
  #if (OPT_99_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_99_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_99_CLI_SHORT_CODE)
    #define OPT_99_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_99_CLI_SHORT_CODE)
  #else
    #define OPT_99_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_100_ENUM_NAME
  #define OPT_100_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_100_HAS_ARG)
  #if (OPT_100_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_100_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_100_CLI_SHORT_CODE)
    #define OPT_100_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_100_CLI_SHORT_CODE)
  #else
    #define OPT_100_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_101_ENUM_NAME
  #define OPT_101_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_101_HAS_ARG)
  #if (OPT_101_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_101_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_101_CLI_SHORT_CODE)
    #define OPT_101_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_101_CLI_SHORT_CODE)
  #else
    #define OPT_101_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_102_ENUM_NAME
  #define OPT_102_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_102_HAS_ARG)
  #if (OPT_102_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_102_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_102_CLI_SHORT_CODE)
    #define OPT_102_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_102_CLI_SHORT_CODE)
  #else
    #define OPT_102_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_103_ENUM_NAME
  #define OPT_103_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_103_HAS_ARG)
  #if (OPT_103_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_103_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_103_CLI_SHORT_CODE)
    #define OPT_103_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_103_CLI_SHORT_CODE)
  #else
    #define OPT_103_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_104_ENUM_NAME
  #define OPT_104_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_104_HAS_ARG)
  #if (OPT_104_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_104_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_104_CLI_SHORT_CODE)
    #define OPT_104_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_104_CLI_SHORT_CODE)
  #else
    #define OPT_104_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_105_ENUM_NAME
  #define OPT_105_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_105_HAS_ARG)
  #if (OPT_105_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_105_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_105_CLI_SHORT_CODE)
    #define OPT_105_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_105_CLI_SHORT_CODE)
  #else
    #define OPT_105_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_106_ENUM_NAME
  #define OPT_106_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_106_HAS_ARG)
  #if (OPT_106_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_106_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_106_CLI_SHORT_CODE)
    #define OPT_106_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_106_CLI_SHORT_CODE)
  #else
    #define OPT_106_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_107_ENUM_NAME
  #define OPT_107_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_107_HAS_ARG)
  #if (OPT_107_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_107_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_107_CLI_SHORT_CODE)
    #define OPT_107_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_107_CLI_SHORT_CODE)
  #else
    #define OPT_107_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_108_ENUM_NAME
  #define OPT_108_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_108_HAS_ARG)
  #if (OPT_108_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_108_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_108_CLI_SHORT_CODE)
    #define OPT_108_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_108_CLI_SHORT_CODE)
  #else
    #define OPT_108_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_109_ENUM_NAME
  #define OPT_109_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_109_HAS_ARG)
  #if (OPT_109_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_109_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_109_CLI_SHORT_CODE)
    #define OPT_109_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_109_CLI_SHORT_CODE)
  #else
    #define OPT_109_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_110_ENUM_NAME
  #define OPT_110_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_110_HAS_ARG)
  #if (OPT_110_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_110_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_110_CLI_SHORT_CODE)
    #define OPT_110_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_110_CLI_SHORT_CODE)
  #else
    #define OPT_110_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_111_ENUM_NAME
  #define OPT_111_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_111_HAS_ARG)
  #if (OPT_111_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_111_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_111_CLI_SHORT_CODE)
    #define OPT_111_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_111_CLI_SHORT_CODE)
  #else
    #define OPT_111_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_112_ENUM_NAME
  #define OPT_112_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_112_HAS_ARG)
  #if (OPT_112_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_112_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_112_CLI_SHORT_CODE)
    #define OPT_112_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_112_CLI_SHORT_CODE)
  #else
    #define OPT_112_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_113_ENUM_NAME
  #define OPT_113_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_113_HAS_ARG)
  #if (OPT_113_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_113_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_113_CLI_SHORT_CODE)
    #define OPT_113_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_113_CLI_SHORT_CODE)
  #else
    #define OPT_113_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_114_ENUM_NAME
  #define OPT_114_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_114_HAS_ARG)
  #if (OPT_114_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_114_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_114_CLI_SHORT_CODE)
    #define OPT_114_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_114_CLI_SHORT_CODE)
  #else
    #define OPT_114_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_115_ENUM_NAME
  #define OPT_115_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_115_HAS_ARG)
  #if (OPT_115_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_115_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_115_CLI_SHORT_CODE)
    #define OPT_115_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_115_CLI_SHORT_CODE)
  #else
    #define OPT_115_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_116_ENUM_NAME
  #define OPT_116_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_116_HAS_ARG)
  #if (OPT_116_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_116_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_116_CLI_SHORT_CODE)
    #define OPT_116_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_116_CLI_SHORT_CODE)
  #else
    #define OPT_116_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_117_ENUM_NAME
  #define OPT_117_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_117_HAS_ARG)
  #if (OPT_117_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_117_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_117_CLI_SHORT_CODE)
    #define OPT_117_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_117_CLI_SHORT_CODE)
  #else
    #define OPT_117_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_118_ENUM_NAME
  #define OPT_118_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_118_HAS_ARG)
  #if (OPT_118_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_118_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_118_CLI_SHORT_CODE)
    #define OPT_118_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_118_CLI_SHORT_CODE)
  #else
    #define OPT_118_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_119_ENUM_NAME
  #define OPT_119_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_119_HAS_ARG)
  #if (OPT_119_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_119_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_119_CLI_SHORT_CODE)
    #define OPT_119_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_119_CLI_SHORT_CODE)
  #else
    #define OPT_119_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_120_ENUM_NAME
  #define OPT_120_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_120_HAS_ARG)
  #if (OPT_120_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_120_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_120_CLI_SHORT_CODE)
    #define OPT_120_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_120_CLI_SHORT_CODE)
  #else
    #define OPT_120_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_121_ENUM_NAME
  #define OPT_121_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_121_HAS_ARG)
  #if (OPT_121_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_121_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_121_CLI_SHORT_CODE)
    #define OPT_121_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_121_CLI_SHORT_CODE)
  #else
    #define OPT_121_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_122_ENUM_NAME
  #define OPT_122_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_122_HAS_ARG)
  #if (OPT_122_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_122_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_122_CLI_SHORT_CODE)
    #define OPT_122_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_122_CLI_SHORT_CODE)
  #else
    #define OPT_122_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_123_ENUM_NAME
  #define OPT_123_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_123_HAS_ARG)
  #if (OPT_123_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_123_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_123_CLI_SHORT_CODE)
    #define OPT_123_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_123_CLI_SHORT_CODE)
  #else
    #define OPT_123_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_124_ENUM_NAME
  #define OPT_124_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_124_HAS_ARG)
  #if (OPT_124_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_124_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_124_CLI_SHORT_CODE)
    #define OPT_124_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_124_CLI_SHORT_CODE)
  #else
    #define OPT_124_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_125_ENUM_NAME
  #define OPT_125_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_125_HAS_ARG)
  #if (OPT_125_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_125_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_125_CLI_SHORT_CODE)
    #define OPT_125_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_125_CLI_SHORT_CODE)
  #else
    #define OPT_125_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_126_ENUM_NAME
  #define OPT_126_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_126_HAS_ARG)
  #if (OPT_126_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_126_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_126_CLI_SHORT_CODE)
    #define OPT_126_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_126_CLI_SHORT_CODE)
  #else
    #define OPT_126_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_127_ENUM_NAME
  #define OPT_127_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_127_HAS_ARG)
  #if (OPT_127_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_127_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_127_CLI_SHORT_CODE)
    #define OPT_127_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_127_CLI_SHORT_CODE)
  #else
    #define OPT_127_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_128_ENUM_NAME
  #define OPT_128_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_128_HAS_ARG)
  #if (OPT_128_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_128_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_128_CLI_SHORT_CODE)
    #define OPT_128_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_128_CLI_SHORT_CODE)
  #else
    #define OPT_128_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_129_ENUM_NAME
  #define OPT_129_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_129_HAS_ARG)
  #if (OPT_129_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_129_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_129_CLI_SHORT_CODE)
    #define OPT_129_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_129_CLI_SHORT_CODE)
  #else
    #define OPT_129_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_130_ENUM_NAME
  #define OPT_130_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_130_HAS_ARG)
  #if (OPT_130_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_130_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_130_CLI_SHORT_CODE)
    #define OPT_130_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_130_CLI_SHORT_CODE)
  #else
    #define OPT_130_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_131_ENUM_NAME
  #define OPT_131_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_131_HAS_ARG)
  #if (OPT_131_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_131_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_131_CLI_SHORT_CODE)
    #define OPT_131_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_131_CLI_SHORT_CODE)
  #else
    #define OPT_131_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_132_ENUM_NAME
  #define OPT_132_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_132_HAS_ARG)
  #if (OPT_132_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_132_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_132_CLI_SHORT_CODE)
    #define OPT_132_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_132_CLI_SHORT_CODE)
  #else
    #define OPT_132_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_133_ENUM_NAME
  #define OPT_133_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_133_HAS_ARG)
  #if (OPT_133_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_133_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_133_CLI_SHORT_CODE)
    #define OPT_133_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_133_CLI_SHORT_CODE)
  #else
    #define OPT_133_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_134_ENUM_NAME
  #define OPT_134_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_134_HAS_ARG)
  #if (OPT_134_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_134_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_134_CLI_SHORT_CODE)
    #define OPT_134_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_134_CLI_SHORT_CODE)
  #else
    #define OPT_134_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_135_ENUM_NAME
  #define OPT_135_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_135_HAS_ARG)
  #if (OPT_135_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_135_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_135_CLI_SHORT_CODE)
    #define OPT_135_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_135_CLI_SHORT_CODE)
  #else
    #define OPT_135_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_136_ENUM_NAME
  #define OPT_136_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_136_HAS_ARG)
  #if (OPT_136_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_136_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_136_CLI_SHORT_CODE)
    #define OPT_136_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_136_CLI_SHORT_CODE)
  #else
    #define OPT_136_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_137_ENUM_NAME
  #define OPT_137_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_137_HAS_ARG)
  #if (OPT_137_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_137_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_137_CLI_SHORT_CODE)
    #define OPT_137_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_137_CLI_SHORT_CODE)
  #else
    #define OPT_137_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_138_ENUM_NAME
  #define OPT_138_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_138_HAS_ARG)
  #if (OPT_138_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_138_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_138_CLI_SHORT_CODE)
    #define OPT_138_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_138_CLI_SHORT_CODE)
  #else
    #define OPT_138_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_139_ENUM_NAME
  #define OPT_139_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_139_HAS_ARG)
  #if (OPT_139_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_139_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_139_CLI_SHORT_CODE)
    #define OPT_139_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_139_CLI_SHORT_CODE)
  #else
    #define OPT_139_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_140_ENUM_NAME
  #define OPT_140_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_140_HAS_ARG)
  #if (OPT_140_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_140_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_140_CLI_SHORT_CODE)
    #define OPT_140_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_140_CLI_SHORT_CODE)
  #else
    #define OPT_140_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_141_ENUM_NAME
  #define OPT_141_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_141_HAS_ARG)
  #if (OPT_141_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_141_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_141_CLI_SHORT_CODE)
    #define OPT_141_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_141_CLI_SHORT_CODE)
  #else
    #define OPT_141_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_142_ENUM_NAME
  #define OPT_142_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_142_HAS_ARG)
  #if (OPT_142_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_142_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_142_CLI_SHORT_CODE)
    #define OPT_142_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_142_CLI_SHORT_CODE)
  #else
    #define OPT_142_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_143_ENUM_NAME
  #define OPT_143_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_143_HAS_ARG)
  #if (OPT_143_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_143_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_143_CLI_SHORT_CODE)
    #define OPT_143_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_143_CLI_SHORT_CODE)
  #else
    #define OPT_143_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_144_ENUM_NAME
  #define OPT_144_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_144_HAS_ARG)
  #if (OPT_144_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_144_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_144_CLI_SHORT_CODE)
    #define OPT_144_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_144_CLI_SHORT_CODE)
  #else
    #define OPT_144_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_145_ENUM_NAME
  #define OPT_145_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_145_HAS_ARG)
  #if (OPT_145_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_145_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_145_CLI_SHORT_CODE)
    #define OPT_145_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_145_CLI_SHORT_CODE)
  #else
    #define OPT_145_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_146_ENUM_NAME
  #define OPT_146_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_146_HAS_ARG)
  #if (OPT_146_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_146_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_146_CLI_SHORT_CODE)
    #define OPT_146_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_146_CLI_SHORT_CODE)
  #else
    #define OPT_146_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_147_ENUM_NAME
  #define OPT_147_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_147_HAS_ARG)
  #if (OPT_147_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_147_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_147_CLI_SHORT_CODE)
    #define OPT_147_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_147_CLI_SHORT_CODE)
  #else
    #define OPT_147_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_148_ENUM_NAME
  #define OPT_148_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_148_HAS_ARG)
  #if (OPT_148_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_148_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_148_CLI_SHORT_CODE)
    #define OPT_148_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_148_CLI_SHORT_CODE)
  #else
    #define OPT_148_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_149_ENUM_NAME
  #define OPT_149_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_149_HAS_ARG)
  #if (OPT_149_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_149_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_149_CLI_SHORT_CODE)
    #define OPT_149_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_149_CLI_SHORT_CODE)
  #else
    #define OPT_149_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_150_ENUM_NAME
  #define OPT_150_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_150_HAS_ARG)
  #if (OPT_150_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_150_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_150_CLI_SHORT_CODE)
    #define OPT_150_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_150_CLI_SHORT_CODE)
  #else
    #define OPT_150_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_151_ENUM_NAME
  #define OPT_151_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_151_HAS_ARG)
  #if (OPT_151_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_151_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_151_CLI_SHORT_CODE)
    #define OPT_151_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_151_CLI_SHORT_CODE)
  #else
    #define OPT_151_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_152_ENUM_NAME
  #define OPT_152_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_152_HAS_ARG)
  #if (OPT_152_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_152_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_152_CLI_SHORT_CODE)
    #define OPT_152_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_152_CLI_SHORT_CODE)
  #else
    #define OPT_152_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_153_ENUM_NAME
  #define OPT_153_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_153_HAS_ARG)
  #if (OPT_153_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_153_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_153_CLI_SHORT_CODE)
    #define OPT_153_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_153_CLI_SHORT_CODE)
  #else
    #define OPT_153_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_154_ENUM_NAME
  #define OPT_154_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_154_HAS_ARG)
  #if (OPT_154_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_154_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_154_CLI_SHORT_CODE)
    #define OPT_154_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_154_CLI_SHORT_CODE)
  #else
    #define OPT_154_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_155_ENUM_NAME
  #define OPT_155_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_155_HAS_ARG)
  #if (OPT_155_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_155_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_155_CLI_SHORT_CODE)
    #define OPT_155_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_155_CLI_SHORT_CODE)
  #else
    #define OPT_155_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_156_ENUM_NAME
  #define OPT_156_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_156_HAS_ARG)
  #if (OPT_156_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_156_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_156_CLI_SHORT_CODE)
    #define OPT_156_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_156_CLI_SHORT_CODE)
  #else
    #define OPT_156_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_157_ENUM_NAME
  #define OPT_157_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_157_HAS_ARG)
  #if (OPT_157_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_157_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_157_CLI_SHORT_CODE)
    #define OPT_157_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_157_CLI_SHORT_CODE)
  #else
    #define OPT_157_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_158_ENUM_NAME
  #define OPT_158_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_158_HAS_ARG)
  #if (OPT_158_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_158_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_158_CLI_SHORT_CODE)
    #define OPT_158_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_158_CLI_SHORT_CODE)
  #else
    #define OPT_158_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_159_ENUM_NAME
  #define OPT_159_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_159_HAS_ARG)
  #if (OPT_159_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_159_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_159_CLI_SHORT_CODE)
    #define OPT_159_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_159_CLI_SHORT_CODE)
  #else
    #define OPT_159_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_160_ENUM_NAME
  #define OPT_160_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_160_HAS_ARG)
  #if (OPT_160_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_160_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_160_CLI_SHORT_CODE)
    #define OPT_160_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_160_CLI_SHORT_CODE)
  #else
    #define OPT_160_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_161_ENUM_NAME
  #define OPT_161_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_161_HAS_ARG)
  #if (OPT_161_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_161_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_161_CLI_SHORT_CODE)
    #define OPT_161_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_161_CLI_SHORT_CODE)
  #else
    #define OPT_161_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_162_ENUM_NAME
  #define OPT_162_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_162_HAS_ARG)
  #if (OPT_162_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_162_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_162_CLI_SHORT_CODE)
    #define OPT_162_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_162_CLI_SHORT_CODE)
  #else
    #define OPT_162_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_163_ENUM_NAME
  #define OPT_163_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_163_HAS_ARG)
  #if (OPT_163_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_163_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_163_CLI_SHORT_CODE)
    #define OPT_163_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_163_CLI_SHORT_CODE)
  #else
    #define OPT_163_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_164_ENUM_NAME
  #define OPT_164_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_164_HAS_ARG)
  #if (OPT_164_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_164_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_164_CLI_SHORT_CODE)
    #define OPT_164_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_164_CLI_SHORT_CODE)
  #else
    #define OPT_164_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_165_ENUM_NAME
  #define OPT_165_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_165_HAS_ARG)
  #if (OPT_165_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_165_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_165_CLI_SHORT_CODE)
    #define OPT_165_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_165_CLI_SHORT_CODE)
  #else
    #define OPT_165_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_166_ENUM_NAME
  #define OPT_166_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_166_HAS_ARG)
  #if (OPT_166_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_166_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_166_CLI_SHORT_CODE)
    #define OPT_166_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_166_CLI_SHORT_CODE)
  #else
    #define OPT_166_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_167_ENUM_NAME
  #define OPT_167_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_167_HAS_ARG)
  #if (OPT_167_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_167_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_167_CLI_SHORT_CODE)
    #define OPT_167_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_167_CLI_SHORT_CODE)
  #else
    #define OPT_167_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_168_ENUM_NAME
  #define OPT_168_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_168_HAS_ARG)
  #if (OPT_168_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_168_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_168_CLI_SHORT_CODE)
    #define OPT_168_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_168_CLI_SHORT_CODE)
  #else
    #define OPT_168_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_169_ENUM_NAME
  #define OPT_169_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_169_HAS_ARG)
  #if (OPT_169_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_169_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_169_CLI_SHORT_CODE)
    #define OPT_169_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_169_CLI_SHORT_CODE)
  #else
    #define OPT_169_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_170_ENUM_NAME
  #define OPT_170_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_170_HAS_ARG)
  #if (OPT_170_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_170_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_170_CLI_SHORT_CODE)
    #define OPT_170_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_170_CLI_SHORT_CODE)
  #else
    #define OPT_170_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_171_ENUM_NAME
  #define OPT_171_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_171_HAS_ARG)
  #if (OPT_171_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_171_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_171_CLI_SHORT_CODE)
    #define OPT_171_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_171_CLI_SHORT_CODE)
  #else
    #define OPT_171_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_172_ENUM_NAME
  #define OPT_172_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_172_HAS_ARG)
  #if (OPT_172_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_172_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_172_CLI_SHORT_CODE)
    #define OPT_172_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_172_CLI_SHORT_CODE)
  #else
    #define OPT_172_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_173_ENUM_NAME
  #define OPT_173_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_173_HAS_ARG)
  #if (OPT_173_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_173_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_173_CLI_SHORT_CODE)
    #define OPT_173_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_173_CLI_SHORT_CODE)
  #else
    #define OPT_173_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_174_ENUM_NAME
  #define OPT_174_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_174_HAS_ARG)
  #if (OPT_174_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_174_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_174_CLI_SHORT_CODE)
    #define OPT_174_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_174_CLI_SHORT_CODE)
  #else
    #define OPT_174_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_175_ENUM_NAME
  #define OPT_175_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_175_HAS_ARG)
  #if (OPT_175_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_175_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_175_CLI_SHORT_CODE)
    #define OPT_175_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_175_CLI_SHORT_CODE)
  #else
    #define OPT_175_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_176_ENUM_NAME
  #define OPT_176_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_176_HAS_ARG)
  #if (OPT_176_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_176_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_176_CLI_SHORT_CODE)
    #define OPT_176_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_176_CLI_SHORT_CODE)
  #else
    #define OPT_176_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_177_ENUM_NAME
  #define OPT_177_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_177_HAS_ARG)
  #if (OPT_177_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_177_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_177_CLI_SHORT_CODE)
    #define OPT_177_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_177_CLI_SHORT_CODE)
  #else
    #define OPT_177_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_178_ENUM_NAME
  #define OPT_178_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_178_HAS_ARG)
  #if (OPT_178_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_178_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_178_CLI_SHORT_CODE)
    #define OPT_178_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_178_CLI_SHORT_CODE)
  #else
    #define OPT_178_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_179_ENUM_NAME
  #define OPT_179_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_179_HAS_ARG)
  #if (OPT_179_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_179_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_179_CLI_SHORT_CODE)
    #define OPT_179_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_179_CLI_SHORT_CODE)
  #else
    #define OPT_179_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_180_ENUM_NAME
  #define OPT_180_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_180_HAS_ARG)
  #if (OPT_180_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_180_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_180_CLI_SHORT_CODE)
    #define OPT_180_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_180_CLI_SHORT_CODE)
  #else
    #define OPT_180_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_181_ENUM_NAME
  #define OPT_181_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_181_HAS_ARG)
  #if (OPT_181_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_181_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_181_CLI_SHORT_CODE)
    #define OPT_181_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_181_CLI_SHORT_CODE)
  #else
    #define OPT_181_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_182_ENUM_NAME
  #define OPT_182_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_182_HAS_ARG)
  #if (OPT_182_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_182_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_182_CLI_SHORT_CODE)
    #define OPT_182_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_182_CLI_SHORT_CODE)
  #else
    #define OPT_182_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_183_ENUM_NAME
  #define OPT_183_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_183_HAS_ARG)
  #if (OPT_183_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_183_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_183_CLI_SHORT_CODE)
    #define OPT_183_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_183_CLI_SHORT_CODE)
  #else
    #define OPT_183_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_184_ENUM_NAME
  #define OPT_184_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_184_HAS_ARG)
  #if (OPT_184_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_184_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_184_CLI_SHORT_CODE)
    #define OPT_184_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_184_CLI_SHORT_CODE)
  #else
    #define OPT_184_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_185_ENUM_NAME
  #define OPT_185_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_185_HAS_ARG)
  #if (OPT_185_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_185_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_185_CLI_SHORT_CODE)
    #define OPT_185_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_185_CLI_SHORT_CODE)
  #else
    #define OPT_185_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_186_ENUM_NAME
  #define OPT_186_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_186_HAS_ARG)
  #if (OPT_186_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_186_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_186_CLI_SHORT_CODE)
    #define OPT_186_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_186_CLI_SHORT_CODE)
  #else
    #define OPT_186_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_187_ENUM_NAME
  #define OPT_187_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_187_HAS_ARG)
  #if (OPT_187_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_187_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_187_CLI_SHORT_CODE)
    #define OPT_187_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_187_CLI_SHORT_CODE)
  #else
    #define OPT_187_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_188_ENUM_NAME
  #define OPT_188_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_188_HAS_ARG)
  #if (OPT_188_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_188_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_188_CLI_SHORT_CODE)
    #define OPT_188_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_188_CLI_SHORT_CODE)
  #else
    #define OPT_188_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_189_ENUM_NAME
  #define OPT_189_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_189_HAS_ARG)
  #if (OPT_189_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_189_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_189_CLI_SHORT_CODE)
    #define OPT_189_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_189_CLI_SHORT_CODE)
  #else
    #define OPT_189_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif

#ifdef OPT_190_ENUM_NAME
  #define OPT_190_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_190_HAS_ARG)
  #if (OPT_190_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_190_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_190_CLI_SHORT_CODE)
    #define OPT_190_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_190_CLI_SHORT_CODE)
  #else
    #define OPT_190_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_191_ENUM_NAME
  #define OPT_191_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_191_HAS_ARG)
  #if (OPT_191_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_191_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_191_CLI_SHORT_CODE)
    #define OPT_191_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_191_CLI_SHORT_CODE)
  #else
    #define OPT_191_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_192_ENUM_NAME
  #define OPT_192_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_192_HAS_ARG)
  #if (OPT_192_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_192_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_192_CLI_SHORT_CODE)
    #define OPT_192_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_192_CLI_SHORT_CODE)
  #else
    #define OPT_192_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_193_ENUM_NAME
  #define OPT_193_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_193_HAS_ARG)
  #if (OPT_193_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_193_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_193_CLI_SHORT_CODE)
    #define OPT_193_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_193_CLI_SHORT_CODE)
  #else
    #define OPT_193_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_194_ENUM_NAME
  #define OPT_194_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_194_HAS_ARG)
  #if (OPT_194_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_194_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_194_CLI_SHORT_CODE)
    #define OPT_194_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_194_CLI_SHORT_CODE)
  #else
    #define OPT_194_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_195_ENUM_NAME
  #define OPT_195_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_195_HAS_ARG)
  #if (OPT_195_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_195_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_195_CLI_SHORT_CODE)
    #define OPT_195_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_195_CLI_SHORT_CODE)
  #else
    #define OPT_195_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_196_ENUM_NAME
  #define OPT_196_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_196_HAS_ARG)
  #if (OPT_196_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_196_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_196_CLI_SHORT_CODE)
    #define OPT_196_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_196_CLI_SHORT_CODE)
  #else
    #define OPT_196_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_197_ENUM_NAME
  #define OPT_197_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_197_HAS_ARG)
  #if (OPT_197_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_197_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_197_CLI_SHORT_CODE)
    #define OPT_197_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_197_CLI_SHORT_CODE)
  #else
    #define OPT_197_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_198_ENUM_NAME
  #define OPT_198_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_198_HAS_ARG)
  #if (OPT_198_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_198_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_198_CLI_SHORT_CODE)
    #define OPT_198_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_198_CLI_SHORT_CODE)
  #else
    #define OPT_198_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif
#ifdef OPT_199_ENUM_NAME
  #define OPT_199_HAS_ARG_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_199_HAS_ARG)
  #if (OPT_199_CLI_SHORT_CODE != OPT_AUTO_CODE)
    #define OPT_199_CLI_SHORT_CODE_CHR  OPT_MC_CCAT(OPT_CHR_, OPT_199_CLI_SHORT_CODE)
    #define OPT_199_CLI_SHORT_CODE_TXT  OPT_MC_STR(OPT_199_CLI_SHORT_CODE)
  #else
    #define OPT_199_CLI_SHORT_CODE_CHR  OPT_AUTO_CODE
  #endif
#endif


/* opt IDs */
/*
    WARNING: This enum is NOT A CONSECUTIVE LIST.
             There are irregularities in numbering.
             Do NOT use this enum as array indices!
*/
typedef enum cli_opt_tt {
    OPT_00_NO_OPTION = 0,
    OPT_NONE = 0,

#ifdef OPT_01_ENUM_NAME
  #if (OPT_01_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_01_ENUM_NAME = 1001,
  #else
       OPT_01_ENUM_NAME = OPT_01_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_02_ENUM_NAME
  #if (OPT_02_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_02_ENUM_NAME = 1002,
  #else
       OPT_02_ENUM_NAME = OPT_02_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_03_ENUM_NAME
  #if (OPT_03_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_03_ENUM_NAME = 1003,
  #else
       OPT_03_ENUM_NAME = OPT_03_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_04_ENUM_NAME
  #if (OPT_04_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_04_ENUM_NAME = 1004,
  #else
       OPT_04_ENUM_NAME = OPT_04_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_05_ENUM_NAME
  #if (OPT_05_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_05_ENUM_NAME = 1005,
  #else
       OPT_05_ENUM_NAME = OPT_05_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_06_ENUM_NAME
  #if (OPT_06_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_06_ENUM_NAME = 1006,
  #else
       OPT_06_ENUM_NAME = OPT_06_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_07_ENUM_NAME
  #if (OPT_07_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_07_ENUM_NAME = 1007,
  #else
       OPT_07_ENUM_NAME = OPT_07_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_08_ENUM_NAME
  #if (OPT_08_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_08_ENUM_NAME = 1008,
  #else
       OPT_08_ENUM_NAME = OPT_08_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_09_ENUM_NAME
  #if (OPT_09_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_09_ENUM_NAME = 1009,
  #else
       OPT_09_ENUM_NAME = OPT_09_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_10_ENUM_NAME
  #if (OPT_10_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_10_ENUM_NAME = 1010,
  #else
       OPT_10_ENUM_NAME = OPT_10_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_11_ENUM_NAME
  #if (OPT_11_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_11_ENUM_NAME = 1011,
  #else
       OPT_11_ENUM_NAME = OPT_11_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_12_ENUM_NAME
  #if (OPT_12_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_12_ENUM_NAME = 1012,
  #else
       OPT_12_ENUM_NAME = OPT_12_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_13_ENUM_NAME
  #if (OPT_13_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_13_ENUM_NAME = 1013,
  #else
       OPT_13_ENUM_NAME = OPT_13_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_14_ENUM_NAME
  #if (OPT_14_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_14_ENUM_NAME = 1014,
  #else
       OPT_14_ENUM_NAME = OPT_14_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_15_ENUM_NAME
  #if (OPT_15_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_15_ENUM_NAME = 1015,
  #else
       OPT_15_ENUM_NAME = OPT_15_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_16_ENUM_NAME
  #if (OPT_16_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_16_ENUM_NAME = 1016,
  #else
       OPT_16_ENUM_NAME = OPT_16_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_17_ENUM_NAME
  #if (OPT_17_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_17_ENUM_NAME = 1017,
  #else
       OPT_17_ENUM_NAME = OPT_17_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_18_ENUM_NAME
  #if (OPT_18_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_18_ENUM_NAME = 1018,
  #else
       OPT_18_ENUM_NAME = OPT_18_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_19_ENUM_NAME
  #if (OPT_19_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_19_ENUM_NAME = 1019,
  #else
       OPT_19_ENUM_NAME = OPT_19_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_20_ENUM_NAME
  #if (OPT_20_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_20_ENUM_NAME = 1020,
  #else
       OPT_20_ENUM_NAME = OPT_20_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_21_ENUM_NAME
  #if (OPT_21_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_21_ENUM_NAME = 1021,
  #else
       OPT_21_ENUM_NAME = OPT_21_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_22_ENUM_NAME
  #if (OPT_22_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_22_ENUM_NAME = 1022,
  #else
       OPT_22_ENUM_NAME = OPT_22_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_23_ENUM_NAME
  #if (OPT_23_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_23_ENUM_NAME = 1023,
  #else
       OPT_23_ENUM_NAME = OPT_23_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_24_ENUM_NAME
  #if (OPT_24_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_24_ENUM_NAME = 1024,
  #else
       OPT_24_ENUM_NAME = OPT_24_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_25_ENUM_NAME
  #if (OPT_25_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_25_ENUM_NAME = 1025,
  #else
       OPT_25_ENUM_NAME = OPT_25_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_26_ENUM_NAME
  #if (OPT_26_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_26_ENUM_NAME = 1026,
  #else
       OPT_26_ENUM_NAME = OPT_26_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_27_ENUM_NAME
  #if (OPT_27_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_27_ENUM_NAME = 1027,
  #else
       OPT_27_ENUM_NAME = OPT_27_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_28_ENUM_NAME
  #if (OPT_28_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_28_ENUM_NAME = 1028,
  #else
       OPT_28_ENUM_NAME = OPT_28_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_29_ENUM_NAME
  #if (OPT_29_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_29_ENUM_NAME = 1029,
  #else
       OPT_29_ENUM_NAME = OPT_29_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_30_ENUM_NAME
  #if (OPT_30_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_30_ENUM_NAME = 1030,
  #else
       OPT_30_ENUM_NAME = OPT_30_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_31_ENUM_NAME
  #if (OPT_31_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_31_ENUM_NAME = 1031,
  #else
       OPT_31_ENUM_NAME = OPT_31_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_32_ENUM_NAME
  #if (OPT_32_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_32_ENUM_NAME = 1032,
  #else
       OPT_32_ENUM_NAME = OPT_32_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_33_ENUM_NAME
  #if (OPT_33_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_33_ENUM_NAME = 1033,
  #else
       OPT_33_ENUM_NAME = OPT_33_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_34_ENUM_NAME
  #if (OPT_34_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_34_ENUM_NAME = 1034,
  #else
       OPT_34_ENUM_NAME = OPT_34_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_35_ENUM_NAME
  #if (OPT_35_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_35_ENUM_NAME = 1035,
  #else
       OPT_35_ENUM_NAME = OPT_35_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_36_ENUM_NAME
  #if (OPT_36_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_36_ENUM_NAME = 1036,
  #else
       OPT_36_ENUM_NAME = OPT_36_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_37_ENUM_NAME
  #if (OPT_37_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_37_ENUM_NAME = 1037,
  #else
       OPT_37_ENUM_NAME = OPT_37_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_38_ENUM_NAME
  #if (OPT_38_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_38_ENUM_NAME = 1038,
  #else
       OPT_38_ENUM_NAME = OPT_38_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_39_ENUM_NAME
  #if (OPT_39_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_39_ENUM_NAME = 1039,
  #else
       OPT_39_ENUM_NAME = OPT_39_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_40_ENUM_NAME
  #if (OPT_40_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_40_ENUM_NAME = 1040,
  #else
       OPT_40_ENUM_NAME = OPT_40_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_41_ENUM_NAME
  #if (OPT_41_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_41_ENUM_NAME = 1041,
  #else
       OPT_41_ENUM_NAME = OPT_41_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_42_ENUM_NAME
  #if (OPT_42_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_42_ENUM_NAME = 1042,
  #else
       OPT_42_ENUM_NAME = OPT_42_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_43_ENUM_NAME
  #if (OPT_43_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_43_ENUM_NAME = 1043,
  #else
       OPT_43_ENUM_NAME = OPT_43_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_44_ENUM_NAME
  #if (OPT_44_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_44_ENUM_NAME = 1044,
  #else
       OPT_44_ENUM_NAME = OPT_44_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_45_ENUM_NAME
  #if (OPT_45_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_45_ENUM_NAME = 1045,
  #else
       OPT_45_ENUM_NAME = OPT_45_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_46_ENUM_NAME
  #if (OPT_46_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_46_ENUM_NAME = 1046,
  #else
       OPT_46_ENUM_NAME = OPT_46_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_47_ENUM_NAME
  #if (OPT_47_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_47_ENUM_NAME = 1047,
  #else
       OPT_47_ENUM_NAME = OPT_47_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_48_ENUM_NAME
  #if (OPT_48_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_48_ENUM_NAME = 1048,
  #else
       OPT_48_ENUM_NAME = OPT_48_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_49_ENUM_NAME
  #if (OPT_49_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_49_ENUM_NAME = 1049,
  #else
       OPT_49_ENUM_NAME = OPT_49_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_50_ENUM_NAME
  #if (OPT_50_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_50_ENUM_NAME = 1050,
  #else
       OPT_50_ENUM_NAME = OPT_50_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_51_ENUM_NAME
  #if (OPT_51_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_51_ENUM_NAME = 1051,
  #else
       OPT_51_ENUM_NAME = OPT_51_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_52_ENUM_NAME
  #if (OPT_52_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_52_ENUM_NAME = 1052,
  #else
       OPT_52_ENUM_NAME = OPT_52_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_53_ENUM_NAME
  #if (OPT_53_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_53_ENUM_NAME = 1053,
  #else
       OPT_53_ENUM_NAME = OPT_53_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_54_ENUM_NAME
  #if (OPT_54_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_54_ENUM_NAME = 1054,
  #else
       OPT_54_ENUM_NAME = OPT_54_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_55_ENUM_NAME
  #if (OPT_55_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_55_ENUM_NAME = 1055,
  #else
       OPT_55_ENUM_NAME = OPT_55_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_56_ENUM_NAME
  #if (OPT_56_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_56_ENUM_NAME = 1056,
  #else
       OPT_56_ENUM_NAME = OPT_56_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_57_ENUM_NAME
  #if (OPT_57_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_57_ENUM_NAME = 1057,
  #else
       OPT_57_ENUM_NAME = OPT_57_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_58_ENUM_NAME
  #if (OPT_58_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_58_ENUM_NAME = 1058,
  #else
       OPT_58_ENUM_NAME = OPT_58_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_59_ENUM_NAME
  #if (OPT_59_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_59_ENUM_NAME = 1059,
  #else
       OPT_59_ENUM_NAME = OPT_59_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_60_ENUM_NAME
  #if (OPT_60_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_60_ENUM_NAME = 1060,
  #else
       OPT_60_ENUM_NAME = OPT_60_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_61_ENUM_NAME
  #if (OPT_61_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_61_ENUM_NAME = 1061,
  #else
       OPT_61_ENUM_NAME = OPT_61_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_62_ENUM_NAME
  #if (OPT_62_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_62_ENUM_NAME = 1062,
  #else
       OPT_62_ENUM_NAME = OPT_62_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_63_ENUM_NAME
  #if (OPT_63_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_63_ENUM_NAME = 1063,
  #else
       OPT_63_ENUM_NAME = OPT_63_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_64_ENUM_NAME
  #if (OPT_64_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_64_ENUM_NAME = 1064,
  #else
       OPT_64_ENUM_NAME = OPT_64_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_65_ENUM_NAME
  #if (OPT_65_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_65_ENUM_NAME = 1065,
  #else
       OPT_65_ENUM_NAME = OPT_65_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_66_ENUM_NAME
  #if (OPT_66_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_66_ENUM_NAME = 1066,
  #else
       OPT_66_ENUM_NAME = OPT_66_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_67_ENUM_NAME
  #if (OPT_67_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_67_ENUM_NAME = 1067,
  #else
       OPT_67_ENUM_NAME = OPT_67_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_68_ENUM_NAME
  #if (OPT_68_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_68_ENUM_NAME = 1068,
  #else
       OPT_68_ENUM_NAME = OPT_68_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_69_ENUM_NAME
  #if (OPT_69_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_69_ENUM_NAME = 1069,
  #else
       OPT_69_ENUM_NAME = OPT_69_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_70_ENUM_NAME
  #if (OPT_70_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_70_ENUM_NAME = 1070,
  #else
       OPT_70_ENUM_NAME = OPT_70_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_71_ENUM_NAME
  #if (OPT_71_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_71_ENUM_NAME = 1071,
  #else
       OPT_71_ENUM_NAME = OPT_71_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_72_ENUM_NAME
  #if (OPT_72_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_72_ENUM_NAME = 1072,
  #else
       OPT_72_ENUM_NAME = OPT_72_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_73_ENUM_NAME
  #if (OPT_73_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_73_ENUM_NAME = 1073,
  #else
       OPT_73_ENUM_NAME = OPT_73_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_74_ENUM_NAME
  #if (OPT_74_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_74_ENUM_NAME = 1074,
  #else
       OPT_74_ENUM_NAME = OPT_74_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_75_ENUM_NAME
  #if (OPT_75_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_75_ENUM_NAME = 1075,
  #else
       OPT_75_ENUM_NAME = OPT_75_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_76_ENUM_NAME
  #if (OPT_76_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_76_ENUM_NAME = 1076,
  #else
       OPT_76_ENUM_NAME = OPT_76_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_77_ENUM_NAME
  #if (OPT_77_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_77_ENUM_NAME = 1077,
  #else
       OPT_77_ENUM_NAME = OPT_77_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_78_ENUM_NAME
  #if (OPT_78_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_78_ENUM_NAME = 1078,
  #else
       OPT_78_ENUM_NAME = OPT_78_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_79_ENUM_NAME
  #if (OPT_79_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_79_ENUM_NAME = 1079,
  #else
       OPT_79_ENUM_NAME = OPT_79_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_80_ENUM_NAME
  #if (OPT_80_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_80_ENUM_NAME = 1080,
  #else
       OPT_80_ENUM_NAME = OPT_80_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_81_ENUM_NAME
  #if (OPT_81_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_81_ENUM_NAME = 1081,
  #else
       OPT_81_ENUM_NAME = OPT_81_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_82_ENUM_NAME
  #if (OPT_82_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_82_ENUM_NAME = 1082,
  #else
       OPT_82_ENUM_NAME = OPT_82_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_83_ENUM_NAME
  #if (OPT_83_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_83_ENUM_NAME = 1083,
  #else
       OPT_83_ENUM_NAME = OPT_83_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_84_ENUM_NAME
  #if (OPT_84_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_84_ENUM_NAME = 1084,
  #else
       OPT_84_ENUM_NAME = OPT_84_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_85_ENUM_NAME
  #if (OPT_85_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_85_ENUM_NAME = 1085,
  #else
       OPT_85_ENUM_NAME = OPT_85_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_86_ENUM_NAME
  #if (OPT_86_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_86_ENUM_NAME = 1086,
  #else
       OPT_86_ENUM_NAME = OPT_86_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_87_ENUM_NAME
  #if (OPT_87_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_87_ENUM_NAME = 1087,
  #else
       OPT_87_ENUM_NAME = OPT_87_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_88_ENUM_NAME
  #if (OPT_88_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_88_ENUM_NAME = 1088,
  #else
       OPT_88_ENUM_NAME = OPT_88_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_89_ENUM_NAME
  #if (OPT_89_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_89_ENUM_NAME = 1089,
  #else
       OPT_89_ENUM_NAME = OPT_89_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_90_ENUM_NAME
  #if (OPT_90_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_90_ENUM_NAME = 1090,
  #else
       OPT_90_ENUM_NAME = OPT_90_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_91_ENUM_NAME
  #if (OPT_91_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_91_ENUM_NAME = 1091,
  #else
       OPT_91_ENUM_NAME = OPT_91_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_92_ENUM_NAME
  #if (OPT_92_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_92_ENUM_NAME = 1092,
  #else
       OPT_92_ENUM_NAME = OPT_92_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_93_ENUM_NAME
  #if (OPT_93_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_93_ENUM_NAME = 1093,
  #else
       OPT_93_ENUM_NAME = OPT_93_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_94_ENUM_NAME
  #if (OPT_94_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_94_ENUM_NAME = 1094,
  #else
       OPT_94_ENUM_NAME = OPT_94_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_95_ENUM_NAME
  #if (OPT_95_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_95_ENUM_NAME = 1095,
  #else
       OPT_95_ENUM_NAME = OPT_95_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_96_ENUM_NAME
  #if (OPT_96_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_96_ENUM_NAME = 1096,
  #else
       OPT_96_ENUM_NAME = OPT_96_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_97_ENUM_NAME
  #if (OPT_97_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_97_ENUM_NAME = 1097,
  #else
       OPT_97_ENUM_NAME = OPT_97_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_98_ENUM_NAME
  #if (OPT_98_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_98_ENUM_NAME = 1098,
  #else
       OPT_98_ENUM_NAME = OPT_98_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_99_ENUM_NAME
  #if (OPT_99_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_99_ENUM_NAME = 1099,
  #else
       OPT_99_ENUM_NAME = OPT_99_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_100_ENUM_NAME
  #if (OPT_100_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_100_ENUM_NAME = 1100,
  #else
       OPT_100_ENUM_NAME = OPT_100_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_101_ENUM_NAME
  #if (OPT_101_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_101_ENUM_NAME = 1101,
  #else
       OPT_101_ENUM_NAME = OPT_101_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_102_ENUM_NAME
  #if (OPT_102_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_102_ENUM_NAME = 1102,
  #else
       OPT_102_ENUM_NAME = OPT_102_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_103_ENUM_NAME
  #if (OPT_103_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_103_ENUM_NAME = 1103,
  #else
       OPT_103_ENUM_NAME = OPT_103_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_104_ENUM_NAME
  #if (OPT_104_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_104_ENUM_NAME = 1104,
  #else
       OPT_104_ENUM_NAME = OPT_104_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_105_ENUM_NAME
  #if (OPT_105_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_105_ENUM_NAME = 1105,
  #else
       OPT_105_ENUM_NAME = OPT_105_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_106_ENUM_NAME
  #if (OPT_106_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_106_ENUM_NAME = 1106,
  #else
       OPT_106_ENUM_NAME = OPT_106_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_107_ENUM_NAME
  #if (OPT_107_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_107_ENUM_NAME = 1107,
  #else
       OPT_107_ENUM_NAME = OPT_107_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_108_ENUM_NAME
  #if (OPT_108_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_108_ENUM_NAME = 1108,
  #else
       OPT_108_ENUM_NAME = OPT_108_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_109_ENUM_NAME
  #if (OPT_109_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_109_ENUM_NAME = 1109,
  #else
       OPT_109_ENUM_NAME = OPT_109_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_110_ENUM_NAME
  #if (OPT_110_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_110_ENUM_NAME = 1110,
  #else
       OPT_110_ENUM_NAME = OPT_110_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_111_ENUM_NAME
  #if (OPT_111_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_111_ENUM_NAME = 1111,
  #else
       OPT_111_ENUM_NAME = OPT_111_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_112_ENUM_NAME
  #if (OPT_112_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_112_ENUM_NAME = 1112,
  #else
       OPT_112_ENUM_NAME = OPT_112_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_113_ENUM_NAME
  #if (OPT_113_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_113_ENUM_NAME = 1113,
  #else
       OPT_113_ENUM_NAME = OPT_113_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_114_ENUM_NAME
  #if (OPT_114_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_114_ENUM_NAME = 1114,
  #else
       OPT_114_ENUM_NAME = OPT_114_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_115_ENUM_NAME
  #if (OPT_115_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_115_ENUM_NAME = 1115,
  #else
       OPT_115_ENUM_NAME = OPT_115_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_116_ENUM_NAME
  #if (OPT_116_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_116_ENUM_NAME = 1116,
  #else
       OPT_116_ENUM_NAME = OPT_116_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_117_ENUM_NAME
  #if (OPT_117_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_117_ENUM_NAME = 1117,
  #else
       OPT_117_ENUM_NAME = OPT_117_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_118_ENUM_NAME
  #if (OPT_118_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_118_ENUM_NAME = 1118,
  #else
       OPT_118_ENUM_NAME = OPT_118_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_119_ENUM_NAME
  #if (OPT_119_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_119_ENUM_NAME = 1119,
  #else
       OPT_119_ENUM_NAME = OPT_119_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_120_ENUM_NAME
  #if (OPT_120_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_120_ENUM_NAME = 1120,
  #else
       OPT_120_ENUM_NAME = OPT_120_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_121_ENUM_NAME
  #if (OPT_121_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_121_ENUM_NAME = 1121,
  #else
       OPT_121_ENUM_NAME = OPT_121_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_122_ENUM_NAME
  #if (OPT_122_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_122_ENUM_NAME = 1122,
  #else
       OPT_122_ENUM_NAME = OPT_122_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_123_ENUM_NAME
  #if (OPT_123_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_123_ENUM_NAME = 1123,
  #else
       OPT_123_ENUM_NAME = OPT_123_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_124_ENUM_NAME
  #if (OPT_124_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_124_ENUM_NAME = 1124,
  #else
       OPT_124_ENUM_NAME = OPT_124_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_125_ENUM_NAME
  #if (OPT_125_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_125_ENUM_NAME = 1125,
  #else
       OPT_125_ENUM_NAME = OPT_125_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_126_ENUM_NAME
  #if (OPT_126_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_126_ENUM_NAME = 1126,
  #else
       OPT_126_ENUM_NAME = OPT_126_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_127_ENUM_NAME
  #if (OPT_127_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_127_ENUM_NAME = 1127,
  #else
       OPT_127_ENUM_NAME = OPT_127_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_128_ENUM_NAME
  #if (OPT_128_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_128_ENUM_NAME = 1128,
  #else
       OPT_128_ENUM_NAME = OPT_128_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_129_ENUM_NAME
  #if (OPT_129_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_129_ENUM_NAME = 1129,
  #else
       OPT_129_ENUM_NAME = OPT_129_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_130_ENUM_NAME
  #if (OPT_130_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_130_ENUM_NAME = 1130,
  #else
       OPT_130_ENUM_NAME = OPT_130_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_131_ENUM_NAME
  #if (OPT_131_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_131_ENUM_NAME = 1131,
  #else
       OPT_131_ENUM_NAME = OPT_131_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_132_ENUM_NAME
  #if (OPT_132_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_132_ENUM_NAME = 1132,
  #else
       OPT_132_ENUM_NAME = OPT_132_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_133_ENUM_NAME
  #if (OPT_133_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_133_ENUM_NAME = 1133,
  #else
       OPT_133_ENUM_NAME = OPT_133_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_134_ENUM_NAME
  #if (OPT_134_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_134_ENUM_NAME = 1134,
  #else
       OPT_134_ENUM_NAME = OPT_134_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_135_ENUM_NAME
  #if (OPT_135_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_135_ENUM_NAME = 1135,
  #else
       OPT_135_ENUM_NAME = OPT_135_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_136_ENUM_NAME
  #if (OPT_136_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_136_ENUM_NAME = 1136,
  #else
       OPT_136_ENUM_NAME = OPT_136_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_137_ENUM_NAME
  #if (OPT_137_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_137_ENUM_NAME = 1137,
  #else
       OPT_137_ENUM_NAME = OPT_137_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_138_ENUM_NAME
  #if (OPT_138_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_138_ENUM_NAME = 1138,
  #else
       OPT_138_ENUM_NAME = OPT_138_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_139_ENUM_NAME
  #if (OPT_139_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_139_ENUM_NAME = 1139,
  #else
       OPT_139_ENUM_NAME = OPT_139_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_140_ENUM_NAME
  #if (OPT_140_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_140_ENUM_NAME = 1140,
  #else
       OPT_140_ENUM_NAME = OPT_140_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_141_ENUM_NAME
  #if (OPT_141_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_141_ENUM_NAME = 1141,
  #else
       OPT_141_ENUM_NAME = OPT_141_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_142_ENUM_NAME
  #if (OPT_142_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_142_ENUM_NAME = 1142,
  #else
       OPT_142_ENUM_NAME = OPT_142_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_143_ENUM_NAME
  #if (OPT_143_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_143_ENUM_NAME = 1143,
  #else
       OPT_143_ENUM_NAME = OPT_143_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_144_ENUM_NAME
  #if (OPT_144_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_144_ENUM_NAME = 1144,
  #else
       OPT_144_ENUM_NAME = OPT_144_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_145_ENUM_NAME
  #if (OPT_145_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_145_ENUM_NAME = 1145,
  #else
       OPT_145_ENUM_NAME = OPT_145_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_146_ENUM_NAME
  #if (OPT_146_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_146_ENUM_NAME = 1146,
  #else
       OPT_146_ENUM_NAME = OPT_146_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_147_ENUM_NAME
  #if (OPT_147_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_147_ENUM_NAME = 1147,
  #else
       OPT_147_ENUM_NAME = OPT_147_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_148_ENUM_NAME
  #if (OPT_148_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_148_ENUM_NAME = 1148,
  #else
       OPT_148_ENUM_NAME = OPT_148_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_149_ENUM_NAME
  #if (OPT_149_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_149_ENUM_NAME = 1149,
  #else
       OPT_149_ENUM_NAME = OPT_149_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_150_ENUM_NAME
  #if (OPT_150_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_150_ENUM_NAME = 1150,
  #else
       OPT_150_ENUM_NAME = OPT_150_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_151_ENUM_NAME
  #if (OPT_151_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_151_ENUM_NAME = 1151,
  #else
       OPT_151_ENUM_NAME = OPT_151_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_152_ENUM_NAME
  #if (OPT_152_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_152_ENUM_NAME = 1152,
  #else
       OPT_152_ENUM_NAME = OPT_152_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_153_ENUM_NAME
  #if (OPT_153_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_153_ENUM_NAME = 1153,
  #else
       OPT_153_ENUM_NAME = OPT_153_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_154_ENUM_NAME
  #if (OPT_154_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_154_ENUM_NAME = 1154,
  #else
       OPT_154_ENUM_NAME = OPT_154_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_155_ENUM_NAME
  #if (OPT_155_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_155_ENUM_NAME = 1155,
  #else
       OPT_155_ENUM_NAME = OPT_155_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_156_ENUM_NAME
  #if (OPT_156_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_156_ENUM_NAME = 1156,
  #else
       OPT_156_ENUM_NAME = OPT_156_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_157_ENUM_NAME
  #if (OPT_157_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_157_ENUM_NAME = 1157,
  #else
       OPT_157_ENUM_NAME = OPT_157_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_158_ENUM_NAME
  #if (OPT_158_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_158_ENUM_NAME = 1158,
  #else
       OPT_158_ENUM_NAME = OPT_158_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_159_ENUM_NAME
  #if (OPT_159_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_159_ENUM_NAME = 1159,
  #else
       OPT_159_ENUM_NAME = OPT_159_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_160_ENUM_NAME
  #if (OPT_160_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_160_ENUM_NAME = 1160,
  #else
       OPT_160_ENUM_NAME = OPT_160_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_161_ENUM_NAME
  #if (OPT_161_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_161_ENUM_NAME = 1161,
  #else
       OPT_161_ENUM_NAME = OPT_161_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_162_ENUM_NAME
  #if (OPT_162_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_162_ENUM_NAME = 1162,
  #else
       OPT_162_ENUM_NAME = OPT_162_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_163_ENUM_NAME
  #if (OPT_163_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_163_ENUM_NAME = 1163,
  #else
       OPT_163_ENUM_NAME = OPT_163_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_164_ENUM_NAME
  #if (OPT_164_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_164_ENUM_NAME = 1164,
  #else
       OPT_164_ENUM_NAME = OPT_164_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_165_ENUM_NAME
  #if (OPT_165_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_165_ENUM_NAME = 1165,
  #else
       OPT_165_ENUM_NAME = OPT_165_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_166_ENUM_NAME
  #if (OPT_166_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_166_ENUM_NAME = 1166,
  #else
       OPT_166_ENUM_NAME = OPT_166_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_167_ENUM_NAME
  #if (OPT_167_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_167_ENUM_NAME = 1167,
  #else
       OPT_167_ENUM_NAME = OPT_167_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_168_ENUM_NAME
  #if (OPT_168_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_168_ENUM_NAME = 1168,
  #else
       OPT_168_ENUM_NAME = OPT_168_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_169_ENUM_NAME
  #if (OPT_169_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_169_ENUM_NAME = 1169,
  #else
       OPT_169_ENUM_NAME = OPT_169_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_170_ENUM_NAME
  #if (OPT_170_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_170_ENUM_NAME = 1170,
  #else
       OPT_170_ENUM_NAME = OPT_170_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_171_ENUM_NAME
  #if (OPT_171_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_171_ENUM_NAME = 1171,
  #else
       OPT_171_ENUM_NAME = OPT_171_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_172_ENUM_NAME
  #if (OPT_172_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_172_ENUM_NAME = 1172,
  #else
       OPT_172_ENUM_NAME = OPT_172_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_173_ENUM_NAME
  #if (OPT_173_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_173_ENUM_NAME = 1173,
  #else
       OPT_173_ENUM_NAME = OPT_173_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_174_ENUM_NAME
  #if (OPT_174_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_174_ENUM_NAME = 1174,
  #else
       OPT_174_ENUM_NAME = OPT_174_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_175_ENUM_NAME
  #if (OPT_175_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_175_ENUM_NAME = 1175,
  #else
       OPT_175_ENUM_NAME = OPT_175_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_176_ENUM_NAME
  #if (OPT_176_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_176_ENUM_NAME = 1176,
  #else
       OPT_176_ENUM_NAME = OPT_176_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_177_ENUM_NAME
  #if (OPT_177_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_177_ENUM_NAME = 1177,
  #else
       OPT_177_ENUM_NAME = OPT_177_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_178_ENUM_NAME
  #if (OPT_178_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_178_ENUM_NAME = 1178,
  #else
       OPT_178_ENUM_NAME = OPT_178_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_179_ENUM_NAME
  #if (OPT_179_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_179_ENUM_NAME = 1179,
  #else
       OPT_179_ENUM_NAME = OPT_179_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_180_ENUM_NAME
  #if (OPT_180_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_180_ENUM_NAME = 1180,
  #else
       OPT_180_ENUM_NAME = OPT_180_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_181_ENUM_NAME
  #if (OPT_181_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_181_ENUM_NAME = 1181,
  #else
       OPT_181_ENUM_NAME = OPT_181_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_182_ENUM_NAME
  #if (OPT_182_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_182_ENUM_NAME = 1182,
  #else
       OPT_182_ENUM_NAME = OPT_182_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_183_ENUM_NAME
  #if (OPT_183_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_183_ENUM_NAME = 1183,
  #else
       OPT_183_ENUM_NAME = OPT_183_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_184_ENUM_NAME
  #if (OPT_184_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_184_ENUM_NAME = 1184,
  #else
       OPT_184_ENUM_NAME = OPT_184_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_185_ENUM_NAME
  #if (OPT_185_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_185_ENUM_NAME = 1185,
  #else
       OPT_185_ENUM_NAME = OPT_185_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_186_ENUM_NAME
  #if (OPT_186_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_186_ENUM_NAME = 1186,
  #else
       OPT_186_ENUM_NAME = OPT_186_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_187_ENUM_NAME
  #if (OPT_187_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_187_ENUM_NAME = 1187,
  #else
       OPT_187_ENUM_NAME = OPT_187_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_188_ENUM_NAME
  #if (OPT_188_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_188_ENUM_NAME = 1188,
  #else
       OPT_188_ENUM_NAME = OPT_188_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_189_ENUM_NAME
  #if (OPT_189_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_189_ENUM_NAME = 1189,
  #else
       OPT_189_ENUM_NAME = OPT_189_CLI_SHORT_CODE_CHR,
  #endif
#endif

#ifdef OPT_190_ENUM_NAME
  #if (OPT_190_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_190_ENUM_NAME = 1190,
  #else
       OPT_190_ENUM_NAME = OPT_190_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_191_ENUM_NAME
  #if (OPT_191_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_191_ENUM_NAME = 1191,
  #else
       OPT_191_ENUM_NAME = OPT_191_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_192_ENUM_NAME
  #if (OPT_192_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_192_ENUM_NAME = 1192,
  #else
       OPT_192_ENUM_NAME = OPT_192_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_193_ENUM_NAME
  #if (OPT_193_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_193_ENUM_NAME = 1193,
  #else
       OPT_193_ENUM_NAME = OPT_193_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_194_ENUM_NAME
  #if (OPT_194_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_194_ENUM_NAME = 1194,
  #else
       OPT_194_ENUM_NAME = OPT_194_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_195_ENUM_NAME
  #if (OPT_195_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_195_ENUM_NAME = 1195,
  #else
       OPT_195_ENUM_NAME = OPT_195_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_196_ENUM_NAME
  #if (OPT_196_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_196_ENUM_NAME = 1196,
  #else
       OPT_196_ENUM_NAME = OPT_196_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_197_ENUM_NAME
  #if (OPT_197_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_197_ENUM_NAME = 1197,
  #else
       OPT_197_ENUM_NAME = OPT_197_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_198_ENUM_NAME
  #if (OPT_198_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_198_ENUM_NAME = 1198,
  #else
       OPT_198_ENUM_NAME = OPT_198_CLI_SHORT_CODE_CHR,
  #endif
#endif
#ifdef OPT_199_ENUM_NAME
  #if (OPT_199_CLI_SHORT_CODE_CHR == OPT_AUTO_CODE)
       OPT_199_ENUM_NAME = 1199,
  #else
       OPT_199_ENUM_NAME = OPT_199_CLI_SHORT_CODE_CHR,
  #endif
#endif





    /* NOTE: no 'OPT_LN' here, because this enum IS NOT A CONSECUTIVE LIST */

} cli_opt_t;

/* ==== TYPEDEFS & DATA : MANDOPT ========================================== */
/*
    This feature is meant to be used within cli cmd callbacks, to provide a unified method for
    checking if user provided all those cli opts which are considered mandatory for the given cli cmd.

    How to use this feature in cmd callbacks:
        [1] Locally define an array of mandopt_t elements (a map of mandatory cli opts and associated conditions).
        [2] Pass it to the mandopt_check() fnc.

    Example 1 - each mandopt element is tied with one cli opt
        const mandopt_t mandopts[] =
        {
            {OPT_INTERFACE, NULL, (p_args.if_name.is_valid)},
            {OPT_PARENT   , NULL, (p_args.if_name_parent.is_valid)},
        };
        rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));

    Example 2 - some mandopt element is tied with multiple cli opts
        const mandopt_optbuf_t multiple_opts = {{OPT_ACCEPT, OPT_REJECT, OPT_NEXT_RULE}};
        const mandopt_t mandopts[] =
        {
            {OPT_RULE, NULL,            (p_args.ruleA0_name.is_valid)},
            {OPT_NONE, &multiple_opts, ((p_args.accept.is_valid) ||
                                        (p_args.reject.is_valid) ||
                                        (p_args.ruleB0_name.is_valid))},
        };
        rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
*/

#define MANDOPT_OPTS_LN  (4u)
typedef struct mandopt_buf_tt {
    cli_opt_t opts[MANDOPT_OPTS_LN];
} mandopt_optbuf_t;

typedef struct mandopt_tt {
    cli_opt_t opt;
    const mandopt_optbuf_t* p_mandopt_optbuf;
    bool is_valid;
} mandopt_t;

#define MANDOPTS_CALC_LN(MANDOPTS)  (uint8_t)(sizeof(MANDOPTS)/sizeof(mandopt_t))

/* ==== PUBLIC FUNCTIONS =================================================== */

const struct option* cli_get_longopts(void);
const char* cli_get_txt_shortopts(void);

const char* cli_opt_get_txt_help(cli_opt_t opt);
uint32_t cli_opt_get_incompat_grps(cli_opt_t opt);

void cli_mandopt_print(const char* p_txt_indent, const char* p_txt_delim);
void cli_mandopt_clear(void);
int cli_mandopt_check(const mandopt_t* p_mandopts, const uint8_t mandopts_ln);
void cli_mandopt_getinternal(mandopt_optbuf_t* p_rtn_optbuf);    /* only for daemon ; do not use casually */
void cli_mandopt_setinternal(mandopt_optbuf_t *const p_optbuf);  /* only for daemon ; do not use casually */

/* ========================================================================= */

#endif
