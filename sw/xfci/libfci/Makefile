# =========================================================================
#  Copyright 2018-2022 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
# =========================================================================

include ../../build_env.mak

ARTIFACT = libfci.a
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Build profile. Possible values: release, debug, profile, coverage
BUILD_PROFILE ?= release

#Build string for architecture/variant.
CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)

TARGET_OS ?= QNX

#Specific configuration per TARGET_OS.
ifeq ($(TARGET_OS),LINUX)
	CC ?= $(PLATFORM)-gcc
	CXX ?= $(PLATFORM)-g++
	INC_PREFIX = 
	
	#Expand GLOBAL_CCFLAGS to get rid of single quotes around string parameters of GLOBAL_CCFLAGS.
	#Those single quotes are utilized in build process of the Linux PFE driver, but they cause warnings in this makefile.
	GLOBAL_CCFLAGS := $(shell echo $(GLOBAL_CCFLAGS))
else
	PLATFORM ?= aarch64le
	PFE_CFG_TARGET_ARCH_DEF = PFE_CFG_TARGET_ARCH_$(PLATFORM)
	PFE_CFG_TARGET_OS_DEF = PFE_CFG_TARGET_OS_$(TARGET_OS)
	PFE_CFG_BUILD_PROFILE_DEF := PFE_CFG_BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])

	CC = qcc -Vgcc_nto$(PLATFORM)
	CXX = qcc -lang-c++ -Vgcc_nto$(PLATFORM)
	LD = $(CC) -static -a
	INC_PREFIX =
endif

#User defined include/preprocessor flags and libraries
INCLUDES += -I$(INC_PREFIX)src \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)../../common/public \
			-I$(INC_PREFIX)../../hal/public \
			-I$(INC_PREFIX)../../oal/public \
			-I$(INC_PREFIX)../../fci/public \
			-I$(INC_PREFIX)../../pfe_platform/public

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
CCFLAGS_all += -Wall -fmessage-length=0 -fvisibility=hidden
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
CCFLAGS_all += $(CCFLAGS_pfe)
CCFLAGS_all += -D$(PFE_CFG_TARGET_ARCH_DEF) -D$(PFE_CFG_TARGET_OS_DEF) -D$(PFE_CFG_BUILD_PROFILE_DEF)
#Include YOCTO sysroot
ifeq ($(TARGET_OS),LINUX)
ifdef PKG_CONFIG_SYSROOT_DIR
CCFLAGS_all += --sysroot=$(PKG_CONFIG_SYSROOT_DIR)
endif
endif
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
ifeq ($(TARGET_OS),LINUX)
	SRCS = src/libfci_linux.c
endif
ifeq ($(TARGET_OS),QNX)
	SRCS = src/libfci_qnx.c
endif


#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Rules section for default compilation and linking
all: $(TARGET)

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
ifeq ($(TARGET_OS),LINUX)
	$(AR) rcs  $(TARGET) $(OBJS)
else
	$(LD) $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)
endif

.PHONY clean:
clean: $(addsuffix .clean,$(DEPENDENCIES))
	rm -fr $(OUTPUT_DIR)

#Deps cleaning rule
%.clean:
	make --no-print-directory -C $(subst build/$(PLATFORM)-$(BUILD_PROFILE),,$(dir $@)) clean

rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)

#
# Linux build

ccflags-y += $(INCLUDES)
ccflags-y += $(CCFLAGS_pfe)

linux:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) M=`pwd`

linux-clean: clean

