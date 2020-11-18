# =========================================================================
#  Copyright 2018-2020 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
# =========================================================================

#all makefiles in the environment require "all" as default target
.DEFAULT_GOAL:= all

# ***********************
# QNX required definitions
# ***********************
# QNX_BASE = <path to QNX SDK home>

#
# ***********************
# Linux required definitions
# ***********************
# TARGET_OS = LINUX
# PLATFORM = aarch64-fsl-linux ...
# ARCH = arm64
# KERNELDIR = /home/hop/workplace/ASK/CR_RSR/CR/src-openwrt-7.0.0/mykern-4.1.35
#

# ***********************
# USER configuration
# ***********************
#Build profile, possible values: release, debug, profile, coverage
export BUILD_PROFILE?=release
#Build architecture/variant string, possible values: x86, armv7le, etc...
export PLATFORM?=aarch64le
#Target os, possible values: QNX, LINUX
export TARGET_OS?=QNX
#Target HW, possible values: s32g
export TARGET_HW ?= s32g
#NULL argument checks. Enable when debugging is needed. 1 - enable, 0 - disable.
export PFE_CFG_NULL_ARG_CHECK?=0
#Paranoid IRQ handing. Adds HW resource protection at potentially critical places.
export PFE_CFG_PARANOID_IRQ?=0
#Code for debugging. Adds program parts useful for debugging but reduces performance.
export PFE_CFG_DEBUG?=0
#HIF driver mode. 0 - SC, 1 - MC
export PFE_CFG_HIF_DRV_MODE?=0
#Host interfaces to be used by particular driver interfaces (see pfe_ct_phy_if_id_t)
export PFE_CFG_PFE0_IF=6
export PFE_CFG_PFE1_IF=7
export PFE_CFG_PFE2_IF=8
#Multi-instance driver support (includes IHC API). 1 - enable, 0 - disable.
export PFE_CFG_MULTI_INSTANCE_SUPPORT?=0
#Master/Slave variant switch
export PFE_CFG_PFE_MASTER?=1
#Host interface identifying the 'master' driver location (see pfe_ct_phy_if_id_t)
export PFE_CFG_MASTER_IF?=6
#Main local host interface to be used for data communication (see pfe_ct_phy_if_id_t)
export PFE_CFG_LOCAL_IF?=$(PFE_CFG_PFE0_IF)
#HIF NOCPY support
export PFE_CFG_HIF_NOCPY_SUPPORT?=0
#HIF NOCPY direct mode. When disabled then LMEM copy mode is used.
export PFE_CFG_HIF_NOCPY_DIRECT?=0
#Force TX CSUM calculation on all frames (this forcibly overwrite all IP/TCP/UDP checksums in FW)
export PFE_CFG_CSUM_ALL_FRAMES?=0
#HIF sequence number check. 1 - enable, 0 - disable
export PFE_CFG_HIF_SEQNUM_CHECK?=0
#IP version
export PFE_CFG_IP_VERSION?=PFE_CFG_IP_VERSION_NPU_7_14a
#Build of rtable feature. 1 - enable, 0 - disable
export PFE_CFG_RTABLE_ENABLE?=1
#Build of l2bridge feature. 1 - enable, 0 - disable
export PFE_CFG_L2BRIDGE_ENABLE?=1
#Build support for FCI. 1 - enable, 0 - disable
export PFE_CFG_FCI_ENABLE?=1
#Build support for thread detecting PFE errors in polling mode. 1 - enable, 0 - disable
export PFE_CFG_GLOB_ERR_POLL_WORKER?=1
#Build support for flexible parser and flexible router
export PFE_CFG_FLEX_PARSER_AND_FILTER?=1
#Enable Interface Database worker thread. 1 - enable, 0 - disable
export PFE_CFG_IF_DB_WORKER?=0
#Enable IEEE1588 timestamping support
export PFE_CFG_IEEE1588_SUPPORT?=0
#Input clock frequency for the IEEE1588 timestamping unit (EMAC)
export PFE_CFG_IEEE1588_I_CLK_HZ=0
#Output clock to be used to drive the IEEE1588 system time counter
export PFE_CFG_IEEE1588_EMAC0_O_CLK_HZ=0
export PFE_CFG_IEEE1588_EMAC1_O_CLK_HZ=0
export PFE_CFG_IEEE1588_EMAC2_O_CLK_HZ=0
#PFE system buffers location
export PFE_CFG_SYS_MEM="pfe_ddr"
#Buffer descriptors location
export PFE_CFG_BD_MEM="pfe_ddr"
#RX buffers location
export PFE_CFG_RX_MEM="pfe_ddr"
#Routing table location
export PFE_CFG_RT_MEM="pfe_ddr"
#Enable firmware-based priority control for HIF traffic
export PFE_CFG_HIF_PRIO_CTRL=1

ifeq ($(PFE_CFG_HIF_DRV_MODE),0)
  #Use multi-client HIF driver. Required when multiple logical interfaces need to
  #send/receive packets using the same HIF channel.
  export PFE_CFG_MC_HIF?=0
  export PFE_CFG_SC_HIF?=1
else
  #Use single-client HIF driver. Beneficial when more HIF channels are available and
  #every logical interface can send/receive packets using dedicated HIF channel.
  export PFE_CFG_MC_HIF?=1
  export PFE_CFG_SC_HIF?=0
endif

#Enable or disable HIF traffic routing. When enabled, traffic sent from host via
#HIF will be routed according to HIF physical interface setup. When disabled, the
#traffic will be directly injected to specified list of interfaces.
export PFE_CFG_ROUTE_HIF_TRAFFIC?=0

ifneq ($(PFE_CFG_MC_HIF),0)
  ifneq ($(PFE_CFG_SC_HIF),0)
    $(error Impossible configuration)
  endif
endif

ifneq ($(PFE_CFG_SC_HIF),0)
  ifneq ($(PFE_CFG_MC_HIF),0)
    $(error Impossible configuration)
  endif
endif

ifeq ($(PFE_CFG_PFE_MASTER),0)
  ifeq ($(PFE_CFG_MULTI_INSTANCE_SUPPORT),0)
    $(warning Slave driver must have multi-instance support enabled)
    PFE_CFG_MULTI_INSTANCE_SUPPORT=1
  endif
  export PFE_CFG_FCI_ENABLE=0
endif

#Include HIF TX FIFO fix. This is SW workaround for HIF stall issue.
ifeq ($(PFE_CFG_IP_VERSION),PFE_CFG_IP_VERSION_NPU_7_14)
export PFE_CFG_HIF_TX_FIFO_FIX=1
else
export PFE_CFG_HIF_TX_FIFO_FIX=0
endif
#Set default verbosity level for sysfs. Valid values are from 1 to 10.
export PFE_CFG_VERBOSITY_LEVEL?=4

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

# ***********************
# Global stuff
# ***********************
ifneq ($(PFE_CFG_VERBOSITY_LEVEL),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_VERBOSITY_LEVEL=$(PFE_CFG_VERBOSITY_LEVEL)
endif

ifneq ($(PFE_CFG_NULL_ARG_CHECK),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_NULL_ARG_CHECK
endif

ifneq ($(PFE_CFG_PARANOID_IRQ),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_PARANOID_IRQ
endif

ifneq ($(PFE_CFG_DEBUG),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_DEBUG
endif

ifneq ($(PFE_CFG_MULTI_INSTANCE_SUPPORT),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_MULTI_INSTANCE_SUPPORT
endif

ifneq ($(PFE_CFG_PFE_MASTER),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_PFE_MASTER
else
    GLOBAL_CCFLAGS+=-DPFE_CFG_PFE_SLAVE
endif

GLOBAL_CCFLAGS+=-DPFE_CFG_MASTER_IF=$(PFE_CFG_MASTER_IF)
GLOBAL_CCFLAGS+=-DPFE_CFG_LOCAL_IF=$(PFE_CFG_LOCAL_IF)
GLOBAL_CCFLAGS+=-DPFE_CFG_PFE0_IF=$(PFE_CFG_PFE0_IF)
GLOBAL_CCFLAGS+=-DPFE_CFG_PFE1_IF=$(PFE_CFG_PFE1_IF)
GLOBAL_CCFLAGS+=-DPFE_CFG_PFE2_IF=$(PFE_CFG_PFE2_IF)

ifneq ($(PFE_CFG_HIF_NOCPY_SUPPORT),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_HIF_NOCPY_SUPPORT
endif

ifneq ($(PFE_CFG_HIF_NOCPY_DIRECT),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_HIF_NOCPY_DIRECT
endif

ifneq ($(PFE_CFG_CSUM_ALL_FRAMES),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_CSUM_ALL_FRAMES
endif

ifneq ($(PFE_CFG_HIF_SEQNUM_CHECK),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_HIF_SEQNUM_CHECK
endif

ifneq ($(PFE_CFG_IP_VERSION),)
GLOBAL_CCFLAGS+=-DPFE_CFG_IP_VERSION=$(PFE_CFG_IP_VERSION)
else
$(error IP version must be set)
endif

ifneq ($(PFE_CFG_RTABLE_ENABLE),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_RTABLE_ENABLE
endif

ifneq ($(PFE_CFG_L2BRIDGE_ENABLE),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_L2BRIDGE_ENABLE
endif

ifneq ($(PFE_CFG_FCI_ENABLE),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_FCI_ENABLE
endif

ifneq ($(PFE_CFG_GLOB_ERR_POLL_WORKER),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_GLOB_ERR_POLL_WORKER
endif

ifneq ($(PFE_CFG_FLEX_PARSER_AND_FILTER),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_FLEX_PARSER_AND_FILTER
endif

ifneq ($(PFE_CFG_IF_DB_WORKER),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_IF_DB_WORKER
endif

ifneq ($(PFE_CFG_MC_HIF),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_MC_HIF
endif

ifneq ($(PFE_CFG_SC_HIF),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_SC_HIF
endif

ifneq ($(PFE_CFG_ROUTE_HIF_TRAFFIC),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_ROUTE_HIF_TRAFFIC
endif

ifneq ($(PFE_CFG_HIF_TX_FIFO_FIX),0)
    GLOBAL_CCFLAGS+= -DPFE_CFG_HIF_TX_FIFO_FIX
endif

ifneq ($(PFE_CFG_IEEE1588_SUPPORT),0)
  ifeq ($(PFE_CFG_IEEE1588_I_CLK_HZ),0)
    $(error When IEEE1588 support is enabled the PFE_CFG_IEEE1588_I_CLK_HZ shall not be zero)
  endif
    GLOBAL_CCFLAGS+=-DPFE_CFG_IEEE1588_SUPPORT
    GLOBAL_CCFLAGS+=-DPFE_CFG_IEEE1588_I_CLK_HZ=$(PFE_CFG_IEEE1588_I_CLK_HZ)
    GLOBAL_CCFLAGS+=-DPFE_CFG_IEEE1588_EMAC0_O_CLK_HZ=$(PFE_CFG_IEEE1588_EMAC0_O_CLK_HZ)
    GLOBAL_CCFLAGS+=-DPFE_CFG_IEEE1588_EMAC1_O_CLK_HZ=$(PFE_CFG_IEEE1588_EMAC1_O_CLK_HZ)
    GLOBAL_CCFLAGS+=-DPFE_CFG_IEEE1588_EMAC2_O_CLK_HZ=$(PFE_CFG_IEEE1588_EMAC2_O_CLK_HZ)
endif

ifneq ($(PFE_CFG_SYS_MEM),0)
  # Pass string literal
  ifeq ($(TARGET_OS),LINUX)
    GLOBAL_CCFLAGS+=-DPFE_CFG_SYS_MEM='\"$(PFE_CFG_SYS_MEM)\"'
  else
    GLOBAL_CCFLAGS+=-DPFE_CFG_SYS_MEM='$(PFE_CFG_SYS_MEM)'
  endif
endif

ifneq ($(PFE_CFG_BD_MEM),0)
  # Pass string literal
  ifeq ($(TARGET_OS),LINUX)
    GLOBAL_CCFLAGS+=-DPFE_CFG_BD_MEM='\"$(PFE_CFG_BD_MEM)\"'
  else
    GLOBAL_CCFLAGS+=-DPFE_CFG_BD_MEM='$(PFE_CFG_BD_MEM)'
  endif
endif

ifneq ($(PFE_CFG_RX_MEM),0)
  # Pass string literal
  ifeq ($(TARGET_OS),LINUX)
    GLOBAL_CCFLAGS+=-DPFE_CFG_RX_MEM='\"$(PFE_CFG_RX_MEM)\"'
  else
    GLOBAL_CCFLAGS+=-DPFE_CFG_RX_MEM='$(PFE_CFG_RX_MEM)'
  endif
endif

ifneq ($(PFE_CFG_RT_MEM),0)
  # Pass string literal
  ifeq ($(TARGET_OS),LINUX)
    GLOBAL_CCFLAGS+=-DPFE_CFG_RT_MEM='\"$(PFE_CFG_RT_MEM)\"'
  else
    GLOBAL_CCFLAGS+=-DPFE_CFG_RT_MEM='$(PFE_CFG_RT_MEM)'
  endif
endif

ifneq ($(PFE_CFG_HIF_PRIO_CTRL),0)
    GLOBAL_CCFLAGS+=-DPFE_CFG_HIF_PRIO_CTRL
endif

# This variable will be propagated to every Makefile in the project
export GLOBAL_CCFLAGS;

# ***********************
# QNX environment
# ***********************
ENV_NOT_SET:=

ifeq ($(TARGET_OS),QNX)

    ifeq ($(QNX_HOST), )
        ENV_NOT_SET:=1
    endif

    ifeq ($(QNX_TARGET), )
        ENV_NOT_SET:=1
    endif

    ifeq ($(ENV_NOT_SET),1)
        ifeq ($(QNX_BASE), )
            $(error Path to QNX SDP7 must be provided in QNX_BASE variable!)
        endif

        export QNX_TARGET:=$(QNX_BASE)/target/qnx7
        export QNX_CONFIGURATION:=$(USERPROFILE)/.qnx
        export MAKEFLAGS:=-I$(QNX_TARGET)/usr/include
        export TMPDIR:=$(TMP)

        ifeq ($(OS),Windows_NT)
            export QNX_HOST:=$(QNX_BASE)/host/win64/x86_64
            export QNX_HOST_CYG:=$(shell cygpath -u $(QNX_BASE))/host/win64/x86_64
            export PATH:=$(QNX_HOST_CYG)/usr/bin:$(PATH)
        else
            export QNX_HOST:=$(QNX_BASE)/host/linux/x86_64
            export PATH:=$(QNX_HOST)/usr/bin:$(PATH)
        endif
    endif

    export PFE_CFG_BUILD_PROFILE_DEF:=PFE_CFG_BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])
    export CONFIG_NAME?=$(PLATFORM)-$(BUILD_PROFILE)
    export PFE_CFG_TARGET_ARCH_DEF=PFE_CFG_TARGET_ARCH_$(PLATFORM)
    export PFE_CFG_TARGET_OS_DEF=PFE_CFG_TARGET_OS_$(TARGET_OS)

    export OUTPUT_DIR=build/$(CONFIG_NAME)

    export CC=qcc -Vgcc_nto$(PLATFORM)
    export CXX=qcc -lang-c++ -Vgcc_nto$(PLATFORM)
    export LD=$(CC)
    export INC_PREFIX=

endif # TARGET_OS

# ***********************
# Linux environment
# ***********************

ifeq ($(TARGET_OS),LINUX)

    export CONFIG_NAME?=$(PLATFORM)-$(BUILD_PROFILE)
    export PFE_CFG_TARGET_ARCH_DEF=PFE_CFG_TARGET_ARCH_$(shell echo $(PLATFORM) | cut -d '-' -f 1-1)
    export PFE_CFG_TARGET_OS_DEF=PFE_CFG_TARGET_OS_$(TARGET_OS)
    export PFE_CFG_BUILD_PROFILE_DEF:=PFE_CFG_BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])

    export OUTPUT_DIR=./

    export CC=$(PLATFORM)-gcc
    export CXX=$(PLATFORM)-g++
    export LD=$(PLATFORM)-ld
    export INC_PREFIX=$(PWD)/

endif # TARGET_OS
