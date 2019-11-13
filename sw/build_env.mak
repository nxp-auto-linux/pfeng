# =========================================================================
#  Copyright 2018-2019 NXP
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation 
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
# KERNELDIR = /home/hop/workplace/ASK/CR_RSR/CR/src-openwrt-ls1012a_7.0.0/mykern-4.1.35
#

# ***********************
# USER configuration
# ***********************
#Build profile, possible values: release, debug, profile, coverage
export BUILD_PROFILE?=release
#Build architecture/variant string, possible values: x86, armv7le, etc...
export PLATFORM?=x86
#Target os, possible values: QNX, LINUX
export TARGET_OS?=QNX
#Target HW, possible values:  ls1012a, s32g
export TARGET_HW ?= s32g
#NULL argument checks. Enable when debugging is needed. 1 - enable, 0 - disable.
export GLOBAL_CFG_NULL_ARG_CHECK?=0
#Paranoid IRQ handing. Adds HW resource protection at potentially critical places.
export GLOBAL_CFG_PARANOID_IRQ?=0
#Multi-instance driver support (includes IHC API). 1 - enable, 0 - disable.
export GLOBAL_CFG_MULTI_INSTANCE_SUPPORT?=1
#Master/Slave variant switch
export GLOBAL_CFG_PFE_MASTER?=1
#HIF NOCPY support
export GLOBAL_CFG_HIF_NOCPY_SUPPORT?=0
#HIF NOCPY direct mode. When disabled then LMEM copy mode is used.
export GLOBAL_CFG_HIF_NOCPY_DIRECT?=0
#Force TX CSUM calculation on all frames (this forcibly overwrite all IP/TCP/UDP checksums in FW)
export GLOBAL_CFG_CSUM_ALL_FRAMES?=0
#PFE Safety periodic unmasking thread. 1 - enable(IRQ unmasked in thread), 0 - disable(IRQ unmasked in ISR).
export GLOBAL_CFG_SAFETY_WORKER?=1
#HIF sequence number check. 1 - enable, 0 - disable
export GLOBAL_CFG_HIF_SEQNUM_CHECK?=0
#IP version
export GLOBAL_CFG_IP_VERSION?=IP_VERSION_FPGA_5_0_4
#Build of rtable feature. 1 - enable, 0 - disable
export GLOBAL_CFG_RTABLE_ENABLED?=1
#Build of l2bridge feature. 1 - enable, 0 - disable
export GLOBAL_CFG_L2BRIDGE_ENABLED?=1
#Build support for FCI. 1 - enable, 0 - disable
export GLOBAL_CFG_FCI_ENABLED?=1
#Build support for hif_global_err_poller_func. 1 - enable, 0 - disable
export GLOBAL_CFG_GLOB_ERR_POLL_WORKER?=1
#Build support flexible parser and flexible router
export GLOBAL_CFG_PFE_FLEX_PARS_RTR_ENABLED?=1

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

# ***********************
# Global stuff
# ***********************
ifneq ($(GLOBAL_CFG_NULL_ARG_CHECK),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_NULL_ARG_CHECK
endif

ifneq ($(GLOBAL_CFG_PARANOID_IRQ),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_PARANOID_IRQ
endif

ifneq ($(GLOBAL_CFG_MULTI_INSTANCE_SUPPORT),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_MULTI_INSTANCE_SUPPORT
endif

ifneq ($(GLOBAL_CFG_PFE_MASTER),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_PFE_MASTER
else
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_PFE_SLAVE
endif

ifneq ($(GLOBAL_CFG_HIF_NOCPY_SUPPORT),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_HIF_NOCPY_SUPPORT
endif

ifneq ($(GLOBAL_CFG_HIF_NOCPY_DIRECT),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_HIF_NOCPY_DIRECT
endif

ifneq ($(GLOBAL_CFG_CSUM_ALL_FRAMES),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_CSUM_ALL_FRAMES
endif

ifneq ($(GLOBAL_CFG_SAFETY_WORKER),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_SAFETY_WORKER
endif

ifneq ($(GLOBAL_CFG_HIF_SEQNUM_CHECK),0)
    GLOBAL_CCFLAGS+=-DGLOBAL_CFG_HIF_SEQNUM_CHECK
endif

ifneq ($(GLOBAL_CFG_IP_VERSION),)
GLOBAL_CCFLAGS+=-DIP_VERSION_FPGA_5_0_4=100
GLOBAL_CCFLAGS+=-DIP_VERSION_NPU_7_14=101
GLOBAL_CCFLAGS+=-DGLOBAL_CFG_IP_VERSION=$(GLOBAL_CFG_IP_VERSION)
else
$(error IP version must be set)
endif

ifneq ($(GLOBAL_CFG_RTABLE_ENABLED),0)
    GLOBAL_CCFLAGS+= -DGLOBAL_CFG_RTABLE_ENABLED
endif

ifneq ($(GLOBAL_CFG_L2BRIDGE_ENABLED),0)
    GLOBAL_CCFLAGS+= -DGLOBAL_CFG_L2BRIDGE_ENABLED
endif

ifneq ($(GLOBAL_CFG_FCI_ENABLED),0)
    GLOBAL_CCFLAGS+= -DGLOBAL_CFG_FCI_ENABLED
endif

ifneq ($(GLOBAL_CFG_GLOB_ERR_POLL_WORKER),0)
    GLOBAL_CCFLAGS+= -DGLOBAL_CFG_GLOB_ERR_POLL_WORKER
endif

ifneq ($(GLOBAL_CFG_PFE_FLEX_PARS_RTR_ENABLED),0)
    GLOBAL_CCFLAGS+= -DGLOBAL_CFG_PFE_FLEX_PARS_RTR_ENABLED
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

    export BUILD_PROFILE_DEF:=BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])
    export TARGET_HW_DEF:=TARGET_HW_$(shell echo $(TARGET_HW) | tr [a-z] [A-Z])
    export CONFIG_NAME?=$(PLATFORM)-$(BUILD_PROFILE)
    export TARGET_ARCH_DEF=TARGET_ARCH_$(PLATFORM)
    export TARGET_OS_DEF=TARGET_OS_$(TARGET_OS)
    export TARGET_ENDIAN_DEF=ENDIAN_LITTLE

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
    export TARGET_ARCH_DEF=TARGET_ARCH_$(shell echo $(PLATFORM) | cut -d '-' -f 1-1)
    export TARGET_OS_DEF=TARGET_OS_$(TARGET_OS)
    export TARGET_ENDIAN_DEF=ENDIAN_LITTLE
    export BUILD_PROFILE_DEF:=BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])
    export TARGET_HW_DEF:=TARGET_HW_$(shell echo $(TARGET_HW) | tr [a-z] [A-Z])

    export OUTPUT_DIR=./

    export CC=$(PLATFORM)-gcc
    export CXX=$(PLATFORM)-g++
    export LD=$(PLATFORM)-ld
    export INC_PREFIX=$(PWD)/

endif # TARGET_OS
