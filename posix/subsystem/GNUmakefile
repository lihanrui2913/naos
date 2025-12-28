NAME := subsystem

override SRCFILES := $(shell find . -name "*.[Sc]" | LC_ALL=C sort)

override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
override OBJ := $(addprefix $(PROJECT_ROOT)/obj-posix-$(ARCH)/$(NAME)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))

all: $(OBJ)
	$(CC) $(CFLAGS) -shared $(OBJ) $(PROJECT_ROOT)/libgcc_$(ARCH).a -o $(PROJECT_ROOT)/initramfs-$(ARCH)/posix-$(NAME)

# Compilation rules for *.c files.
$(PROJECT_ROOT)/obj-posix-$(ARCH)/$(NAME)/%.c.o: %.c GNUmakefile
	mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) -I$(shell pwd)/ -c $< -o $@
