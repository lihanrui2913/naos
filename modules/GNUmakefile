MAKEFLAGS += -rR --no-print-directory

include ../build/common-env.mk
include build/common-flags.mk

all:
	$(call PRINT_STEP,MKDIR,$(PROJECT_ROOT)/modules-$(ARCH))
	$(Q)mkdir -p $(PROJECT_ROOT)/modules-$(ARCH)

ifneq ($(BUILD_LINUX_DRIVERS), 0)
	$(call PRINT_STEP,MAKE,modules/lcompat)
	$(Q)$(MAKE) -C lcompat
endif

	$(call PRINT_STEP,MAKE,modules/drivers)
	$(Q)$(MAKE) -C drivers
	$(call PRINT_STEP,MAKE,modules/fs)
	$(Q)$(MAKE) -C fs
	$(call PRINT_STEP,MAKE,modules/net)
	$(Q)$(MAKE) -C net
