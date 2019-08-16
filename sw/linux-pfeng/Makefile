#
# Copyright 2018-2019 NXP
#
# SPDX-License-Identifier:     BSD OR GPL-2.0
#
#

KERNELDIR ?= /usr/src/linux-headers-`uname -r`
TARGET_OS := LINUX
GLOBAL_CFG_SAFETY_WORKER = 0
GLOBAL_CFG_MULTI_INSTANCE_SUPPORT = 0
GLOBAL_CFG_RUN_ON_VDK ?= 0
ifneq ($(GLOBAL_CFG_RUN_ON_VDK),0)
PLATFORM ?= aarch64-fsl-linux
ARCH ?= arm64
else
PLATFORM ?= x86_64-linux-gnu
ARCH ?= x86
endif

PLAT_DEFINES = TARGET_OS=$(TARGET_OS) KERNELDIR=$(KERNELDIR) PLATFORM=$(PLATFORM) ARCH=$(ARCH) CROSS_COMPILE=$(PLATFORM)-

-include ../build_env.mak

ccflags-y += -I$(PWD)/../oal/public
ccflags-y += -I$(PWD)/../oal/public/linux
ccflags-y += -I$(PWD)/../hal/public
ccflags-y += -I$(PWD)/../common/public
ccflags-y += -I$(PWD)/../elf/public
ccflags-y += -I$(PWD)/../bpool/public
ccflags-y += -I$(PWD)/../pfe_platform/public
ccflags-y += -I$(PWD)/../pfe_hif_drv/public
ccflags-y += -DTARGET_OS=LINUX -DTARGET_OS_LINUX
ifneq ($(GLOBAL_CFG_RUN_ON_VDK),0)
ccflags-y += -DTARGET_ARCH_aarch64
else
ccflags-y += -DTARGET_ARCH_x86_64
endif
ccflags-y += -DTARGET_HW=$(TARGET_HW) -DTARGET_HW_S32G
ccflags-y += $(GLOBAL_CCFLAGS)
ccflags-y += -g
ccflags-y += -Wno-date-time

OBJ_DIRS = ../oal ../bpool  ../elf  ../fifo  ../pfe_hif_drv  ../pfe_platform ../common
pfeng-objs := ../pfe_platform/pfe_platform.o ../oal/oal.o ../elf/elf.o ../fifo/fifo.o ../pfe_hif_drv/pfe_hif_drv.o ../bpool/bpool.o ../common/blalloc.o
verif-objs := ../pfe_platform/pfe_platform.o ../oal/oal.o ../elf/elf.o ../fifo/fifo.o ../pfe_hif_drv/pfe_hif_drv.o ../bpool/bpool.o

pfeng-objs += pfeng-ethtool.o pfeng-fw.o pfeng-platform.o pfeng-main.o pfeng-debugfs.o pfeng-sysfs.o
ifneq ($(GLOBAL_CFG_RUN_ON_VDK),0)
pfeng-objs += pfeng-hw-vdk.o
else
pfeng-objs += pfeng-hw-pci.o
endif

verif-objs += pfe-verif.o

obj-m += pfeng.o
#obj-m += verif.o

.PHONY: all

modules:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules

all:
	for dir in $(OBJ_DIRS); do (cd $$dir; $(MAKE) $(PLAT_DEFINES) linux || exit 1) || exit 1; done
	make CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

drv-clean:
	for dir in $(OBJ_DIRS); do (cd $$dir; $(MAKE) $(PLAT_DEFINES) linux-clean || exit 1) || exit 1; done
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
