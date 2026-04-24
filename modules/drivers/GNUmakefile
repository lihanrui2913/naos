all:
ifeq ($(ARCH), x86_64)
	$(Q)$(MAKE) -C e1000
endif

	$(call PRINT_STEP,MAKE,modules/drivers/sound)
	$(Q)$(MAKE) -C sound

	$(call PRINT_STEP,MAKE,modules/drivers/nvme)
	$(Q)$(MAKE) -C nvme

	$(call PRINT_STEP,MAKE,modules/drivers/usb)
	$(Q)$(MAKE) -C usb
	$(call PRINT_STEP,MAKE,modules/drivers/virtio)
	$(Q)$(MAKE) -C virtio

	$(call PRINT_STEP,MAKE,modules/drivers/hub)
	$(Q)$(MAKE) -C hub
	$(call PRINT_STEP,MAKE,modules/drivers/hid)
	$(Q)$(MAKE) -C hid
	$(call PRINT_STEP,MAKE,modules/drivers/msc)
	$(Q)$(MAKE) -C msc
