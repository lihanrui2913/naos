all:
ifeq ($(ARCH), x86_64)
	$(call PRINT_STEP,MAKE,modules/drivers/net/e1000)
	$(Q)$(MAKE) -C e1000
endif

	$(call PRINT_STEP,MAKE,modules/drivers/net/virtio-net)
	$(Q)$(MAKE) -C virtio-net

ifneq ($(BUILD_LINUX_DRIVERS), 0)
	$(call PRINT_STEP,MAKE,modules/drivers/linux/net-rtw88)
	$(Q)$(MAKE) -C ../linux/net-rtw88
endif
