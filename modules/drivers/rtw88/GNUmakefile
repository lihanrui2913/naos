KM_NAME := rtw88

override SRCFILES := \
	glue.c \
	rtw88/main.c \
	rtw88/mac80211.c \
	rtw88/util.c \
	rtw88/tx.c \
	rtw88/rx.c \
	rtw88/mac.c \
	rtw88/phy.c \
	rtw88/coex.c \
	rtw88/efuse.c \
	rtw88/fw.c \
	rtw88/ps.c \
	rtw88/sec.c \
	rtw88/bf.c \
	rtw88/regd.c \
	rtw88/sar.c \
	rtw88/usb.c \
	rtw88/rtw8723x.c \
	rtw88/rtw8723d.c \
	rtw88/rtw8723d_table.c \
	rtw88/rtw8723du.c \
	rtw88/rtw88xxa.c \
	rtw88/rtw8812a.c \
	rtw88/rtw8812a_table.c \
	rtw88/rtw8812au.c \
	rtw88/rtw8814a.c \
	rtw88/rtw8814a_table.c \
	rtw88/rtw8814au.c \
	rtw88/rtw8821a.c \
	rtw88/rtw8821a_table.c \
	rtw88/rtw8821au.c \
	rtw88/rtw8821c.c \
	rtw88/rtw8821c_table.c \
	rtw88/rtw8821cu.c \
	rtw88/rtw8822b.c \
	rtw88/rtw8822b_table.c \
	rtw88/rtw8822bu.c \
	rtw88/rtw8822c.c \
	rtw88/rtw8822c_table.c \
	rtw88/rtw8822cu.c

override CFLAGS += \
	-DKBUILD_MODNAME=\"rtw88\"

MODULE_LINK_LIBS := $(PROJECT_ROOT)/libgcc_$(ARCH).a
MODULE_INCLUDE_DIRS += \
	$(PROJECT_ROOT)/modules/lcompat/include \
	$(PROJECT_ROOT)/kernel/src

include $(PROJECT_ROOT)/modules/build/module.mk
