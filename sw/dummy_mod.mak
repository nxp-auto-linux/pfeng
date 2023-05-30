#
# Copyright 2023 NXP
#
# SPDX-License-Identifier: GPL-2.0
#

ccflags-y += $(INCLUDES)
ccflags-y += $(CCFLAGS_pfe)
ccflags-y += $(CFLAGS_MODULE)
ccflags-y += $(GLOBAL_CCFLAGS)
ccflags-y += -Werror

linux::
	$(MAKE) CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` GLOBAL_CCFLAGS="$(GLOBAL_CCFLAGS)" modules

linux-clean:
	$(MAKE) CROSS_COMPILE=$(PLATFORM)-  ARCH=$(ARCH) -C $(KERNELDIR) M=`pwd` clean


MODULE_NAME = $(subst .a,,$(subst .o,,$(ARTIFACT)))
ifneq (,$(findstring .a,$(ARTIFACT)))
    MODULE_NAME         := _dummy
    obj-m               += $(MODULE_NAME).o
    $(MODULE_NAME)-objs := lib.a ../common/src/dummy_main.o
    lib-y               := $(OBJS)
    LIB_OBJECT          := $(subst .a,.o,$(ARTIFACT))

linux::
	@echo "  mv lib.a -> $(LIB_OBJECT)"
	@cp lib.a $(LIB_OBJECT)
	@cp .lib.a.cmd .$(LIB_OBJECT).cmd
else
    obj-m               += $(MODULE_NAME).o
    $(MODULE_NAME)-objs := $(OBJS)
endif
