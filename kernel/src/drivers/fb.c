#include <drivers/kernel_logger.h>
#include <arch/arch.h>
#include <boot/boot.h>

boot_framebuffer_t *framebuffer = NULL;

boot_framebuffer_t *get_current_fb() { return framebuffer; }

void fbdev_init() { framebuffer = boot_get_framebuffer(); }
