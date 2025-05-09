#
# Copyright 2018-2023,2025 NXP
#
# SPDX-License-Identifier:     BSD OR GPL-2.0
#
#

ifeq ($(KERNELDIR), )
	$(error Path to the kernel must be provided in KERNELDIR variable!)
endif

# Set up all required build options
TARGET_OS := LINUX
ARCH := arm64
PLATFORM ?= aarch64-fsl-linux
PFE_CFG_MULTI_INSTANCE_SUPPORT ?= 0
PFE_CFG_LINUX_TRACE ?= 0
PFE_CFG_HIF_USE_BD_TRIGGER ?= 1
PFE_CFG_BD_MEM ?= "pfe-bdr-pool"
PFE_CFG_SYS_MEM ?= "pfe-bmu2-pool"
PFE_CFG_RT_MEM ?= "pfe-rt-pool"
PFE_CFG_BMU2_BUF_COUNT ?= 256

# Warning for PFE_CFG_LINUX_TRACE=1:
# The compile time checker doesn't work well with -Og
# See: https://lkml.org/lkml/2018/11/2/542

#
# Unsupported config options
# * PFE_CFG_CSUM_ALL_FRAMES		: dynamicly managed in driver
# * PFE_CFG_MC_HIF, PFE_CFG_SC_HIF	: both are supported pernamently
# * PFE_CFG_IEEE1588_SUPPORT		: dynamicly managed in driver

PLAT_DEFINES = KBUILD_MODPOST_WARN=1 TARGET_OS=$(TARGET_OS) KERNELDIR=$(KERNELDIR) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CROSS_COMPILE=$(PLATFORM)-

-include ../build_env.mak

ccflags-y += -I$(PWD)/../oal/public
ccflags-y += -I$(PWD)/../oal/public/linux
ccflags-y += -I$(PWD)/../hal/public
ccflags-y += -I$(PWD)/../common/public
ccflags-y += -I$(PWD)/../fifo/public
ccflags-y += -I$(PWD)/../elf/public
ccflags-y += -I$(PWD)/../bpool/public
ccflags-y += -I$(PWD)/../pfe_platform/public
ccflags-y += -I$(PWD)/../pfe_platform/hw/$(TARGET_HW)
ccflags-y += -I$(PWD)/../pfe_hif_drv/public
ccflags-y += -I$(PWD)/../fci/public
ccflags-y += -DPFE_CFG_TARGET_OS_LINUX
ccflags-y += -D$(PFE_CFG_TARGET_ARCH_DEF)
ccflags-y += $(GLOBAL_CCFLAGS)
ifneq ($(PFE_CFG_LINUX_TRACE),0)
ccflags-y += -Og
endif
ccflags-y += -Wno-date-time
ccflags-y += -Werror

OBJ_LIBFCI_DIR = ../xfci/libfci
OBJ_DIRS = ../oal ../bpool  ../elf  ../fifo  ../pfe_platform ../common

ifneq ($(PFE_CFG_FCI_ENABLE),0)
OBJ_DIRS += ../fci
endif

pfeng-objs-libs := ../pfe_platform/pfe_platform.o ../oal/oal.o ../elf/elf.o ../fifo/fifo.o ../bpool/bpool.o ../common/common.o

ifneq ($(PFE_CFG_FCI_ENABLE),0)
pfeng-objs-libs += ../fci/fci.o
endif

pfeng-objs-core := pfeng-debugfs.o pfeng-hif.o pfeng-bman.o pfeng-netif.o pfeng-ethtool.o pfeng-hwts.o pfeng-dt.o pfeng-mdio.o pfeng-ptp.o pfeng-gpr.o

ifneq ($(PFE_CFG_PFE_MASTER),0)
pfeng-objs := $(pfeng-objs-core) $(pfeng-objs-libs) pfeng-drv.o pfeng-fw.o pfeng-phylink.o
obj-m += pfeng.o
else
pfeng-slave-objs := $(pfeng-objs-core) $(pfeng-objs-libs) pfeng-slave-drv.o
ifneq ($(PFE_CFG_PFE_SLAVE_PHY),0)
pfeng-slave-objs += pfeng-slave-phy.o
endif
obj-m += pfeng-slave.o
endif

.PHONY: all drv-build drv-clean libfci-build libfci-clean

modules:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules

libfci-build:
	cd $(OBJ_LIBFCI_DIR); $(MAKE) $(PLAT_DEFINES) linux

libfci-clean:
	cd $(OBJ_LIBFCI_DIR); $(MAKE) $(PLAT_DEFINES) linux-clean

drv-build:
	for dir in $(OBJ_DIRS); do (cd $$dir; $(MAKE) $(PLAT_DEFINES) linux || exit 1) || exit 1; done
	$(MAKE) $(PLAT_DEFINES) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

drv-clean:
	for dir in $(OBJ_DIRS); do (cd $$dir; $(MAKE) $(PLAT_DEFINES) linux-clean || exit 1) || exit 1; done
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean

clean: drv-clean libfci-clean

all: drv-build
