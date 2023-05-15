# =========================================================================
#  Copyright 2018-2023 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
# =========================================================================

ifndef KERNELRELEASE
include ../build_env.mak
endif

ARTIFACT = oal.a

TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#User defined include/preprocessor flags and libraries

INCLUDES += -I$(INC_PREFIX).. \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)../hal/public \
			-I$(INC_PREFIX)../fifo/public \
			-I$(INC_PREFIX)../common/public \
			-I$(INC_PREFIX)../pfe_platform/public

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
ifeq ($(TARGET_OS),LINUX)
SRCS += src/oal_mbox_linux.c
SRCS += src/oal_irq_linux.c
SRCS += src/oal_mm_linux.c
SRCS += src/oal_thread_linux.c
SRCS += src/oal_time_linux.c
SRCS += src/oal_util_linux.c
SRCS += src/oal_util_net_linux.c
SRCS += src/oal_job_linux.c
endif

ifeq ($(TARGET_OS),QNX)
SRCS += src/oal_mbox_qnx.c
SRCS += src/oal_irq_qnx.c
SRCS += src/oal_mm_qnx.c
SRCS += src/oal_thread_qnx.c
SRCS += src/oal_time_qnx.c
SRCS += src/oal_util_qnx.c
SRCS += src/oal_util_net_qnx.c
SRCS += src/oal_job_qnx.c
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

include $(M)/../dummy_mod.mak
