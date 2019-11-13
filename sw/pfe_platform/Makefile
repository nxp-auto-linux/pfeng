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

ifndef KERNELRELEASE
include ../build_env.mak
endif

ifeq ($(TARGET_OS),LINUX)
  ARTIFACT = pfe_platform.o
else
  ARTIFACT = pfe_platform.a
endif

TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#User defined include/preprocessor flags and libraries

INCLUDES += -I$(INC_PREFIX).. \
			-I$(INC_PREFIX)public \
			-I$(INC_PREFIX)hw/$(TARGET_HW) \
			-I$(INC_PREFIX)../oal/public \
			-I$(INC_PREFIX)../oal/public/$(shell echo $(TARGET_OS) | tr [A-Z] [a-z]) \
			-I$(INC_PREFIX)../hal/public \
			-I$(INC_PREFIX)../common/public \
			-I$(INC_PREFIX)../elf/public \
			-I$(INC_PREFIX)../fifo/public \
			-I$(INC_PREFIX)../bpool/public \
			-I$(INC_PREFIX)../pfe_hif_drv/public \
			-I$(INC_PREFIX)../fci/public

#Dependencies
DEPENDENCIES = $(subst -L,,$(subst $(EMPTY) $(EMPTY)-l:,/,$(LIBS)))

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

CCFLAGS_pfe +=	-DGCC_TOOLCHAIN \
				-DPFE_FW_SBL=1 \
				-DPFE_FW_FULL=2\
				-DPFE_CFG_FIRMWARE_VARIANT=PFE_FW_FULL
CCFLAGS_pfe += -D$(TARGET_ARCH_DEF) -D$(TARGET_OS_DEF) -D$(TARGET_ENDIAN_DEF) -D$(TARGET_HW_DEF) -D$(BUILD_PROFILE_DEF)

CCFLAGS_pfe +=-DPFE_CT_H_MD5=$(word 1, $(shell md5sum $(INC_PREFIX)public/pfe_ct.h))
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
ifeq ($(GLOBAL_CFG_PFE_MASTER),0)
SRCS =	src/pfe_class.c \
		src/pfe_hif_chnl.c \
		src/pfe_hif_ring.c \
		src/pfe_hif.c \
		src/pfe_idex.c \
		src/pfe_log_if_slave.c \
		src/pfe_if_db.c \
		src/pfe_pe.c \
		src/pfe_phy_if_slave.c \
		src/pfe_fp.c \
		src/pfe_flexible_filter.c
else
SRCS =	src/pfe_bmu.c \
		src/pfe_class.c \
		src/pfe_emac.c \
		src/pfe_gpi.c \
		src/pfe_hif_chnl.c \
		src/pfe_hif_nocpy.c \
		src/pfe_hif_ring.c \
		src/pfe_hif.c \
		src/pfe_idex.c \
		src/pfe_l2br.c \
		src/pfe_l2br_table.c \
		src/pfe_log_if.c \
		src/pfe_if_db.c \
		src/pfe_pe.c \
		src/pfe_phy_if.c \
		src/pfe_rtable.c \
		src/pfe_safety.c \
		src/pfe_tmu.c \
		src/pfe_util.c \
		src/pfe_fp.c \
		src/pfe_flexible_filter.c		
endif
		
SRCS += hw/$(TARGET_HW)/pfe_bmu_csr.c \
		hw/$(TARGET_HW)/pfe_emac_csr.c \
		hw/$(TARGET_HW)/pfe_hif_csr.c \
		hw/$(TARGET_HW)/pfe_util_csr.c \
		hw/$(TARGET_HW)/pfe_class_csr.c \
		hw/$(TARGET_HW)/pfe_gpi_csr.c \
		hw/$(TARGET_HW)/pfe_hif_nocpy_csr.c \
		hw/$(TARGET_HW)/pfe_tmu_csr.c \
		hw/$(TARGET_HW)/pfe_safety_csr.c
ifeq ($(GLOBAL_CFG_PFE_MASTER),0)
  SRCS += hw/$(TARGET_HW)/pfe_platform_slave.c
else
  SRCS += hw/$(TARGET_HW)/pfe_platform_master.c
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

ccflags-y += $(INCLUDES)
ccflags-y += -Wno-date-time
ccflags-y += $(CCFLAGS_pfe)
ccflags-y += $(GLOBAL_CCFLAGS)

pfe_platform-y := src/pfe_bmu.o \
	src/pfe_class.o \
	src/pfe_flexible_filter.o \
	src/pfe_fp.o \
	src/pfe_emac.o \
	src/pfe_gpi.o \
	src/pfe_hif.o \
	src/pfe_hif_chnl.o \
	src/pfe_hif_nocpy.o \
	src/pfe_hif_ring.o \
	src/pfe_pe.o \
	src/pfe_tmu.o \
	src/pfe_util.o \
	src/pfe_phy_if.o \
	src/pfe_log_if.o \
	src/pfe_if_db.o \
	src/pfe_safety.o \
	hw/$(TARGET_HW)/pfe_bmu_csr.o \
	hw/$(TARGET_HW)/pfe_emac_csr.o \
	hw/$(TARGET_HW)/pfe_hif_csr.o \
	hw/$(TARGET_HW)/pfe_util_csr.o \
	hw/$(TARGET_HW)/pfe_class_csr.o \
	hw/$(TARGET_HW)/pfe_gpi_csr.o \
	hw/$(TARGET_HW)/pfe_hif_nocpy_csr.o \
	hw/$(TARGET_HW)/pfe_tmu_csr.o \
	hw/$(TARGET_HW)/pfe_safety_csr.o
	
ifeq ($(GLOBAL_CFG_PFE_MASTER),0)
pfe_platform-y += hw/$(TARGET_HW)/pfe_platform_slave.o
else
pfe_platform-y +=hw/$(TARGET_HW)/pfe_platform_master.o
endif

ifeq ($(GLOBAL_CFG_RTABLE_ENABLED),1)
    pfe_platform-y += src/pfe_rtable.o
endif
ifeq ($(GLOBAL_CFG_L2BRIDGE_ENABLED),1)
    pfe_platform-y += src/pfe_l2br.o \
        src/pfe_l2br_table.o
endif
     
obj-m += pfe_platform.o

linux:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

linux-clean:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` clean

