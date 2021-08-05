# =========================================================================
#  Copyright 2018-2021 NXP
#
#  SPDX-License-Identifier: GPL-2.0
#
# =========================================================================

include build_env.mak

# ***********************
# Rules
# ***********************

PFE_DRIVER:=devnp-pfe-2
PFE_RELATIVES:=

# If profiling is required then a special NXP-internal-only library should be built
ifeq ($(BUILD_PROFILE),profile)
	PFE_RELATIVES+=profiling
endif

.PHONY all:
all: $(PFE_DRIVER).qp
$(PFE_DRIVER).qp: $(addsuffix .qp,$(PFE_RELATIVES))

.PHONY clean:
clean: $(PFE_DRIVER).qp_clean $(addsuffix .qp_clean,$(PFE_RELATIVES))

.ONESHELL:
%.qp: $(basename %.qp)
	cd $(basename $@)
	$(MAKE)

%.qp_clean: $(basename %.qp_clean)
	cd $(basename $@)
	$(MAKE) clean

# Return configured compiler version
.PHONY: compiler_version
compiler_version:
	@echo "$(COMPILER_NAME), $(COMPILER_VERSION)"
