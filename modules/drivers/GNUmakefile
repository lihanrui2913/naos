all:
ifeq ($(ARCH), x86_64)
	$(Q)$(MAKE) -C e1000
endif

	$(Q)$(MAKE) -C sound

	$(Q)$(MAKE) -C nvme

	$(Q)$(MAKE) -C usb
	$(Q)$(MAKE) -C virtio

	$(Q)$(MAKE) -C hub
	$(Q)$(MAKE) -C hid
	$(Q)$(MAKE) -C msc
