# =========================================================================
#  Copyright 2018-2024 NXP
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

ifndef __KERNEL__
include ../build_env.mak
endif

ARTIFACT = libfci_cli
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Build profile. Possible values: release, debug, profile, coverage
BUILD_PROFILE ?= release

#Build string for architecture/variant.
CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)

TARGET_OS ?= QNX

#This is for cleaner YOCTO recipe (cross-compiling from yocto needs explicit path to the target sysroot).
SYSROOT_DIR ?= ""
ifneq ($(SYSROOT_DIR), "")
	CC += --sysroot=$(SYSROOT_DIR)
endif

#Specific configuration per TARGET_OS.
ifeq ($(TARGET_OS),LINUX)
	LD = $(CC)
	LIBS += -L../xfci/libfci/build/$(PLATFORM)-$(BUILD_PROFILE) -l:libfci.a
	LIBS_all += -lpthread
	
	#Expand GLOBAL_CCFLAGS to get rid of single quotes around string parameters of GLOBAL_CCFLAGS.
	#Those single quotes are utilized in build process of the Linux PFE driver, but they cause warnings in this makefile.
	GLOBAL_CCFLAGS := $(shell echo $(GLOBAL_CCFLAGS))
	
	CLI_TARGET_OS = "LNX"
	CLI_DRV_VERSION = $(shell cat -n ../linux-pfeng/pfeng.h | grep 'PFENG_DRIVER_VERSION\s*\K.*' -Po)
	CLI_DRV_COMMIT_HASH = "M4_DRIVER_COMMIT_HASH"
else
#This branch by defaut means QNX.
	LIBS += -L../xfci/libfci/build/$(PLATFORM)-$(BUILD_PROFILE) -l:libfci.a
	LIBS_all += -lsocket
	
	CLI_TARGET_OS = "QNX"
	CLI_DRV_VERSION = "$(RELEASE_VERSION)"
	CLI_DRV_COMMIT_HASH = "$(DRIVER_COMMIT_HASH)"
endif

#User defined include/preprocessor flags and libraries
INCLUDES += -I../xfci/libfci/public \
			-I../fci/public \
			-I../common/public \
			-I./src/libfci_demo \
			-I./src/daemon

#Dependencies
DEPENDENCIES = $(subst -L,,$(subst $(EMPTY) $(EMPTY)-l:,/,$(LIBS)))

#Compiler flags for build profiles
CCFLAGS_release += -O3
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
CCFLAGS_all += -D$(PFE_CFG_TARGET_ARCH_DEF) -D$(PFE_CFG_TARGET_OS_DEF) -D$(PFE_CFG_BUILD_PROFILE_DEF)
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Driver versioning info - MD5 hash of pfe_ct.h, drv version, target OS.
PFE_CT_H_START_OFFSET=$(word 1, $(shell cat -n ../pfe_platform/public/pfe_ct.h | sed -n '/=== \*/,$$p'))
PFE_CT_H_MD5 = $(word 1, $(shell cat ../pfe_platform/public/pfe_ct.h | sed -n '$(PFE_CT_H_START_OFFSET),$$p' | tr -d '\040\011\012\015' | md5sum))
CCFLAGS += -DPFE_CT_H_MD5=\"$(PFE_CT_H_MD5)\"
CCFLAGS += -DCLI_TARGET_OS=\"$(CLI_TARGET_OS)\"
ifneq ($(CLI_DRV_VERSION), "")
	CCFLAGS += -DCLI_DRV_VERSION=\"$(CLI_DRV_VERSION)\"
endif
ifneq ($(CLI_DRV_COMMIT_HASH), "")
	CCFLAGS += -DCLI_DRV_COMMIT_HASH=\"$(CLI_DRV_COMMIT_HASH)\"
endif

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS = $(call rwildcard, src, c)

#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Deps building rule
.PHONY: $(DEPENDENCIES)
$(DEPENDENCIES):
	@make --no-print-directory -C $(subst build/$(PLATFORM)-$(BUILD_PROFILE),,$(dir $@))

#Compiling rule
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $(GLOBAL_CCFLAGS) $<

#Linking rule
EMPTY =
$(TARGET):$(OBJS) $(subst -L,,$(subst $(EMPTY) $(EMPTY)-l:,/,$(LIBS)))
	$(LD) -o $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)
	cp $(TARGET) $(ARTIFACT) 

#Rules section for default compilation and linking
all: $(TARGET)

.PHONY clean:
clean: $(addsuffix .clean,$(DEPENDENCIES))
	rm -fr $(OUTPUT_DIR)

#Deps cleaning rule
%.clean:
	make --no-print-directory -C $(subst build/$(PLATFORM)-$(BUILD_PROFILE),,$(dir $@)) clean


rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)
