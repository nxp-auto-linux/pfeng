# =========================================================================
#  Copyright 2018-2020 NXP
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

ifndef KERNELRELEASE
include ../build_env.mak
endif

ifeq ($(TARGET_OS),LINUX)
  ARTIFACT = fci.o
else
  ARTIFACT = fci.a
endif

TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#User defined include/preprocessor flags and libraries
INCLUDES += -I$(INC_PREFIX)src \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)../common/public \
			-I$(INC_PREFIX)../oal/public \
			-I$(INC_PREFIX)../hal/public \
			-I$(INC_PREFIX)../bpool/public \
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
CCFLAGS_all += -Wall -fmessage-length=0
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
SRCS += src/fci_l2br_domains.c
SRCS += src/fci_connections.c
SRCS += src/fci_routes.c
SRCS += src/fci_interfaces.c
SRCS += src/fci_rt_db.c
SRCS += src/fci_fp_db.c
SRCS += src/fci_fp.c
SRCS += src/fci_flexible_filter.c
SRCS += src/fci_spd.c
SRCS += src/fci_qos.c
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

ccflags-y += $(INCLUDES)
ccflags-y += -Wno-date-time
ccflags-y += $(CCFLAGS_pfe)
ccflags-y += $(GLOBAL_CCFLAGS)

fci-y := $(OBJS)

obj-m += fci.o

linux:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

linux-clean:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` clean

