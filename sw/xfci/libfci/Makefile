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

TARGET_OS ?= QNX

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= release

#Compiler definitions
include ../../build_env.mak

ifeq ($(TARGET_OS),LINUX)
  OUTPUT_DIR = build/$(CONFIG_NAME)
  CC = $(PLATFORM)-gcc
  CXX =$(PLATFORM)-g++
  LD =ar rsc
  INC_PREFIX = 
else
  #Build architecture/variant string, possible values: x86, armv7le, etc...
  PLATFORM ?= aarch64le
  CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
  TARGET_ARCH_DEF = TARGET_ARCH_$(PLATFORM)
  TARGET_OS_DEF = TARGET_OS_$(TARGET_OS)
  TARGET_ENDIAN_DEF = ENDIAN_LITTLE
  BUILD_PROFILE_DEF := BUILD_PROFILE_$(shell echo $(BUILD_PROFILE) | tr [a-z] [A-Z])

  OUTPUT_DIR = build/$(CONFIG_NAME)
  CC = qcc -Vgcc_nto$(PLATFORM)
  CXX = qcc -lang-c++ -Vgcc_nto$(PLATFORM)
  LD = $(CC) -static -a
  INC_PREFIX =
endif

ARTIFACT = libfci.a
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#User defined include/preprocessor flags and libraries
INCLUDES += -I$(INC_PREFIX)src \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)../../common/public \
			-I$(INC_PREFIX)../../hal/public \
			-I$(INC_PREFIX)../../fci/public \
			-I$(INC_PREFIX)../../pfe_platform/public

#LIBS += -L/path/to/my/lib/$(PLATFORM)/usr/lib -lmylib
		
#Dependencies
DEPENDENCIES = $(subst -L,,$(subst $(EMPTY) $(EMPTY)-l:,/,$(LIBS)))

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
CCFLAGS_all += $(CCFLAGS_pfe)
CCFLAGS_all += -D$(TARGET_ARCH_DEF) -D$(TARGET_OS_DEF) -D$(TARGET_ENDIAN_DEF) -D$(BUILD_PROFILE_DEF)
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
	$(LD) $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

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
ccflags-y += -DTARGET_OS_LINUX

bpool-y := src/common.o
#bpool-y := $(OBJS)

obj-m += libcommon.o

ARCH := arm64
KERNELDIR := /home/hop/workplace/ASK/CR_RSR/CR/src-openwrt-ls1012a_7.0.0/mykern-4.1.35

linux:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` modules
	#make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` O=`pwd`"/build/"$(PLATFORM) $@

linux-clean:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` clean

