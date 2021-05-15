# =========================================================================
#  Copyright 2018-2021 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
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
			-I$(INC_PREFIX)../fci/public \
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

CCFLAGS_pfe += -DGCC_TOOLCHAIN
CCFLAGS_pfe += -D$(PFE_CFG_TARGET_ARCH_DEF) -D$(PFE_CFG_TARGET_OS_DEF) -D$(PFE_CFG_BUILD_PROFILE_DEF)
PFE_CT_H_START_OFFSET=$(word 1, $(shell cat -n $(INC_PREFIX)public/pfe_ct.h | sed -n '/=== \*/,$$p'))
PFE_CT_H_MD5 = $(word 1, $(shell cat $(INC_PREFIX)public/pfe_ct.h | sed -n '$(PFE_CT_H_START_OFFSET),$$p' | tr -d '\040\011\012\015' | md5sum))

CCFLAGS_pfe +=-DPFE_CFG_PFE_CT_H_MD5=$(PFE_CT_H_MD5)
$(info pfe_ct.h MD5 sum is $(PFE_CT_H_MD5))

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
ifeq ($(PFE_CFG_PFE_MASTER),0)
SRCS =	src/pfe_class.c \
		src/pfe_hif_chnl.c \
		src/pfe_hif_ring.c \
		src/pfe_hif.c \
		src/pfe_idex.c \
		src/pfe_log_if_slave.c \
		src/pfe_if_db.c \
		src/pfe_pe.c \
		src/pfe_phy_if_slave.c \
		src/pfe_mac_db.c
else
SRCS =	src/pfe_bmu.c \
		src/pfe_class.c \
		src/pfe_emac.c \
		src/pfe_gpi.c \
		src/pfe_hif_chnl.c \
		src/pfe_hif_nocpy.c \
		src/pfe_hif_ring.c \
		src/pfe_hif.c \
		src/pfe_l2br.c \
		src/pfe_l2br_table.c \
		src/pfe_log_if.c \
		src/pfe_if_db.c \
		src/pfe_pe.c \
		src/pfe_phy_if.c \
		src/pfe_rtable.c \
		src/pfe_safety.c \
		src/pfe_wdt.c \
		src/pfe_tmu.c \
		src/pfe_util.c \
		src/pfe_fp.c \
		src/pfe_flexible_filter.c \
		src/pfe_fw_feature.c \
		src/pfe_spd.c \
		src/pfe_spd_acc.c \
		src/pfe_mac_db.c

ifeq ($(PFE_CFG_MULTI_INSTANCE_SUPPORT),1)
SRCS += src/pfe_idex.c
endif
endif

SRCS += hw/$(TARGET_HW)/pfe_bmu_csr.c \
		hw/$(TARGET_HW)/pfe_emac_csr.c \
		hw/$(TARGET_HW)/pfe_hif_csr.c \
		hw/$(TARGET_HW)/pfe_util_csr.c \
		hw/$(TARGET_HW)/pfe_class_csr.c \
		hw/$(TARGET_HW)/pfe_gpi_csr.c \
		hw/$(TARGET_HW)/pfe_hif_nocpy_csr.c \
		hw/$(TARGET_HW)/pfe_tmu_csr.c \
		hw/$(TARGET_HW)/pfe_safety_csr.c \
		hw/$(TARGET_HW)/pfe_wdt_csr.c
ifeq ($(PFE_CFG_PFE_MASTER),0)
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
ifneq ($(PFE_CFG_LINUX_TRACE),0)
ccflags-y += -Og
endif
ccflags-y += $(CCFLAGS_pfe)
ccflags-y += $(GLOBAL_CCFLAGS)
ccflags-y += -Werror

ifeq ($(PFE_CFG_PFE_MASTER),0)
pfe_platform-y := src/pfe_hif_chnl.o \
		src/pfe_hif_ring.o \
		src/pfe_hif.o \
		src/pfe_log_if_slave.o \
		src/pfe_if_db.o \
		src/pfe_pe.o \
		src/pfe_phy_if_slave.o \
		src/pfe_mac_db.o
else
pfe_platform-y := src/pfe_bmu.o \
	src/pfe_class.o \
	src/pfe_flexible_filter.o \
	src/pfe_fw_feature.o \
	src/pfe_fp.o \
	src/pfe_emac.o \
	src/pfe_gpi.o \
	src/pfe_hif.o \
	src/pfe_hif_chnl.o \
	src/pfe_hif_ring.o \
	src/pfe_pe.o \
	src/pfe_tmu.o \
	src/pfe_util.o \
	src/pfe_phy_if.o \
	src/pfe_log_if.o \
	src/pfe_if_db.o \
	src/pfe_safety.o \
	src/pfe_spd.o \
	src/pfe_spd_acc.o \
	src/pfe_mac_db.o

ifeq ($(PFE_CFG_RTABLE_ENABLE),1)
    pfe_platform-y += src/pfe_rtable.o
endif
ifeq ($(PFE_CFG_L2BRIDGE_ENABLE),1)
    pfe_platform-y += src/pfe_l2br.o \
        src/pfe_l2br_table.o
endif
endif



pfe_platform-y += src/pfe_bmu.o \
	hw/$(TARGET_HW)/pfe_bmu_csr.o \
	hw/$(TARGET_HW)/pfe_emac_csr.o \
	hw/$(TARGET_HW)/pfe_hif_csr.o \
	hw/$(TARGET_HW)/pfe_util_csr.o \
	hw/$(TARGET_HW)/pfe_class_csr.o \
	hw/$(TARGET_HW)/pfe_gpi_csr.o \
	hw/$(TARGET_HW)/pfe_tmu_csr.o \
	hw/$(TARGET_HW)/pfe_safety_csr.o

ifneq ($(PFE_CFG_IP_VERSION),PFE_CFG_IP_VERSION_FPGA_5_0_4)
pfe_platform-y += hw/$(TARGET_HW)/pfe_wdt_csr.o
pfe_platform-y += src/pfe_wdt.o
endif

ifeq ($(PFE_CFG_PFE_MASTER),0)
pfe_platform-y += hw/$(TARGET_HW)/pfe_platform_slave.o
else
pfe_platform-y +=hw/$(TARGET_HW)/pfe_platform_master.o
endif

ifneq ($(PFE_CFG_MULTI_INSTANCE_SUPPORT),0)
pfe_platform-y += src/pfe_idex.o
endif

obj-m += pfe_platform.o

linux:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

linux-clean:
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` clean

