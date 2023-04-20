# =========================================================================
#  Copyright 2018-2023 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
# =========================================================================

ifndef KERNELRELEASE
include ../build_env.mak
endif

ARTIFACT = fci.a

TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#User defined include/preprocessor flags and libraries
INCLUDES += -I$(INC_PREFIX)src \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)../common/public \
			-I$(INC_PREFIX)../oal/public \
			-I$(INC_PREFIX)../hal/public \
			-I$(INC_PREFIX)../bpool/public \
			-I$(INC_PREFIX)../pfe_platform/public \
			-I$(INC_PREFIX)../pfe_hif_drv/public \
			-I$(INC_PREFIX)../pfe_platform/public \
			-I$(INC_PREFIX)../xfci/libfci/public

#Dependencies
DEPENDENCIES = $(subst -L,,$(subst $(EMPTY) $(EMPTY)-l:,/,$(LIBS)))

#Compiler flags for build profiles
CCFLAGS_release += -O3
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

CCFLAGS_pfe += -D$(PFE_CFG_TARGET_ARCH_DEF) -D$(PFE_CFG_TARGET_OS_DEF) -D$(PFE_CFG_BUILD_PROFILE_DEF)

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0 -fvisibility=hidden
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
CCFLAGS_all += $(CCFLAGS_pfe)
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS += src/fci.c
SRCS += src/fci_hm.c
ifneq ($(PFE_CFG_PFE_MASTER),0)
SRCS += src/fci_l2br_domains.c
SRCS += src/fci_l2br.c
SRCS += src/fci_connections.c
SRCS += src/fci_routes.c
SRCS += src/fci_interfaces.c
SRCS += src/fci_rt_db.c
SRCS += src/fci_fp_db.c
SRCS += src/fci_fp.c
SRCS += src/fci_fw_features.c
SRCS += src/fci_spd.c
SRCS += src/fci_qos.c
SRCS += src/fci_mirror.c
endif
ifneq ($(PFE_CFG_MULTI_INSTANCE_SUPPORT),0)
SRCS += src/fci_owner.c
endif
ifeq ($(TARGET_OS),QNX)
SRCS += src/fci_core_qnx.c
endif
ifeq ($(TARGET_OS),LINUX)
SRCS += src/fci_core_linux.c
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
	$(LD) -static -a $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

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

ifdef KERNELRELEASE
# signal kernel mode for compilation to distinkt kernel/user mode difference
GLOBAL_CCFLAGS+=-D__KERNEL__
endif

ccflags-y += -Wno-date-time

include $(M)/../dummy_mod.mak
