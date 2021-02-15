#
# Copyright 2018-2021 NXP
#
# SPDX-License-Identifier: GPL-2.0
#
#

KERNELDIR ?= /usr/src/linux-headers-`uname -r`
TARGET_OS := LINUX
PFE_CFG_MULTI_INSTANCE_SUPPORT ?= 0
PLATFORM ?= aarch64-fsl-linux
ARCH ?= arm64
PFE_CFG_IP_VERSION ?= PFE_CFG_IP_VERSION_NPU_7_14a
PFE_CFG_LINUX_TRACE ?= 0
PFE_CFG_MC_HIF=0
PFE_CFG_SC_HIF=1

# Unsupported config options
# * PFE_CFG_CSUM_ALL_FRAMES	: dynamically managable in driver
# * PFE_CFG_MC_HIF		: only one client per channel
# * PFE_CFG_IEEE1588_SUPPORT : dynamicly managed in driver

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

OBJ_LIBFCI_DIR = ../xfci/libfci
OBJ_DIRS = ../oal ../bpool  ../elf  ../fifo  ../pfe_hif_drv  ../pfe_platform ../common

ifneq ($(PFE_CFG_FCI_ENABLE),0)
OBJ_DIRS += ../fci
endif

pfeng-objs := ../pfe_platform/pfe_platform.o ../oal/oal.o ../elf/elf.o ../fifo/fifo.o ../pfe_hif_drv/pfe_hif_drv.o ../bpool/bpool.o ../common/blalloc.o
pfeng-slave-objs := ../pfe_platform/pfe_platform.o ../oal/oal.o ../elf/elf.o ../fifo/fifo.o ../pfe_hif_drv/pfe_hif_drv.o ../bpool/bpool.o ../common/blalloc.o

ifneq ($(PFE_CFG_FCI_ENABLE),0)
pfeng-objs += ../fci/fci.o
pfeng-slave-objs += ../fci/fci.o
endif

ifneq ($(PFE_CFG_PFE_MASTER),0)
pfeng-objs += pfeng-drv.o pfeng-hw-s32g.o pfeng-fw.o pfeng-debugfs.o pfeng-napi.o pfeng-ethtool.o pfeng-mdio.o pfeng-phylink.o pfeng-hif.o pfeng-ptp.o pfeng-hwts.o
obj-m += pfeng.o
else
pfeng-slave-objs += pfeng-drv.o pfeng-hw-s32g.o pfeng-fw.o pfeng-debugfs.o pfeng-napi.o pfeng-ethtool.o pfeng-hif.o pfeng-hwts.o
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
	make $(PLAT_DEFINES) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

drv-clean:
	for dir in $(OBJ_DIRS); do (cd $$dir; $(MAKE) $(PLAT_DEFINES) linux-clean || exit 1) || exit 1; done
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean

clean: drv-clean libfci-clean

all: drv-build
