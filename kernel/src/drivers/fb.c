#include <drivers/fb.h>
#include <drivers/logger.h>
#include <mod/dlinker.h>
#include <fs/dev.h>
#include <arch/arch.h>
#include <fs/fs_syscall.h>
#include <fs/sys.h>
#include <boot/boot.h>

typedef struct fb_file {
    boot_framebuffer_t *framebuffer;
    struct fb_var_screeninfo var;
} fb_file_t;

static void fb_init_var(boot_framebuffer_t *framebuffer,
                        struct fb_var_screeninfo *var) {
    memset(var, 0, sizeof(*var));
    var->xres = framebuffer->width;
    var->yres = framebuffer->height;
    var->xres_virtual = framebuffer->width;
    var->yres_virtual = framebuffer->height;
    var->red = (struct fb_bitfield){.offset = framebuffer->red_mask_shift,
                                    .length = framebuffer->red_mask_size};
    var->green = (struct fb_bitfield){.offset = framebuffer->green_mask_shift,
                                      .length = framebuffer->green_mask_size};
    var->blue = (struct fb_bitfield){.offset = framebuffer->blue_mask_shift,
                                     .length = framebuffer->blue_mask_size};
    var->transp = (struct fb_bitfield){.offset = 24, .length = 8};
    var->bits_per_pixel = framebuffer->bpp;
    var->height = framebuffer->height / 4;
    var->width = framebuffer->width / 4;
}

static fb_file_t *fb_file_from_fd(fd_t *fd) {
    return (fb_file_t *)device_file_private(fd);
}

ssize_t fb_open(void *data, void *arg) {
    boot_framebuffer_t *framebuffer = data;
    fd_t *fd = arg;
    fb_file_t *file;

    if (!framebuffer || !fd)
        return -EINVAL;
    file = calloc(1, sizeof(*file));
    if (!file)
        return -ENOMEM;
    file->framebuffer = framebuffer;
    fb_init_var(framebuffer, &file->var);
    if (device_file_set_private(fd, file) < 0) {
        free(file);
        return -EBADF;
    }
    return 0;
}

ssize_t fb_close(void *data, void *arg) {
    fd_t *fd = arg;
    fb_file_t *file = fb_file_from_fd(fd);

    (void)data;
    if (file) {
        device_file_set_private(fd, NULL);
        free(file);
    }
    return 0;
}

ssize_t fb_read(void *data, void *buf, uint64_t offset, size_t len, fd_t *fd) {
    fb_file_t *file = fb_file_from_fd(fd);
    boot_framebuffer_t *framebuffer = file ? file->framebuffer : data;
    uint64_t size;

    if (!framebuffer || !buf)
        return -EINVAL;
    size = framebuffer->pitch * framebuffer->height;
    if (offset >= size)
        return 0;
    len = MIN(len, size - offset);
    memcpy(buf, (char *)framebuffer->address + offset, len);
    return (ssize_t)len;
}

ssize_t fb_write(void *data, void *buf, uint64_t offset, size_t len, fd_t *fd) {
    fb_file_t *file = fb_file_from_fd(fd);
    boot_framebuffer_t *framebuffer = file ? file->framebuffer : data;
    uint64_t size;

    if (!framebuffer || !buf)
        return -EINVAL;
    size = framebuffer->pitch * framebuffer->height;
    if (offset >= size)
        return -ENOSPC;
    len = MIN(len, size - offset);
    memcpy((char *)framebuffer->address + offset, buf, len);
    return len;
}

ssize_t fb_ioctl(void *data, int cmd, void *arg, fd_t *fd) {
    fb_file_t *file = fb_file_from_fd(fd);
    boot_framebuffer_t *framebuffer = file ? file->framebuffer : data;

    if (!framebuffer || !file)
        return -EBADF;

    cmd = cmd & 0xFFFFFFFF;

    switch (cmd) {
    case FBIOGET_FSCREENINFO: {
        struct fb_fix_screeninfo *fb_fix = arg;
        if (!fb_fix)
            return -EFAULT;
        memset(fb_fix, 0, sizeof(*fb_fix));
        memcpy(fb_fix->id, "NAOS-FBDEV", 10);
        fb_fix->smem_start = translate_address(get_current_page_dir(false),
                                               (uint64_t)framebuffer->address);
        fb_fix->smem_len = framebuffer->pitch * framebuffer->height;
        fb_fix->type = FB_TYPE_PACKED_PIXELS;
        fb_fix->type_aux = 0;
        fb_fix->visual = FB_VISUAL_TRUECOLOR;
        fb_fix->xpanstep = 0;
        fb_fix->ypanstep = 0;
        fb_fix->ywrapstep = 0;
        fb_fix->line_length = framebuffer->pitch;
        fb_fix->mmio_start = translate_address(get_current_page_dir(false),
                                               (size_t)framebuffer->address);
        fb_fix->mmio_len = framebuffer->pitch * framebuffer->height;
        fb_fix->capabilities = 0;
        return 0;
    }
    case FBIOGET_VSCREENINFO: {
        struct fb_var_screeninfo *fb_var = arg;
        if (!fb_var)
            return -EFAULT;
        *fb_var = file->var;
        return 0;
    }
    case 0x4605: // FBIOPUTCMAP, ignore so no xorg.log spam
        return 0;
    case 0x5413: {
        struct winsize *win = arg;
        if (!win)
            return -EFAULT;
        win->ws_col = framebuffer->width / 8;
        win->ws_row = framebuffer->height / 16;

        win->ws_xpixel = (uint16_t)framebuffer->width;
        win->ws_ypixel = (uint16_t)framebuffer->height;

        return 0;
    }
    case FBIOPUT_VSCREENINFO:
    case FBIOPAN_DISPLAY: {
        struct fb_var_screeninfo *requested = arg;
        if (!requested)
            return -EFAULT;
        if (requested->xoffset >= file->var.xres_virtual ||
            requested->yoffset >= file->var.yres_virtual)
            return -EINVAL;
        file->var.xoffset = requested->xoffset;
        file->var.yoffset = requested->yoffset;
        file->var.activate = requested->activate;
        return 0;
    }
    default:
        return (uint64_t)-ENOTTY;
    }
}

void *fb_map(void *data, void *addr, size_t offset, size_t len, size_t prot,
             fd_t *fd) {
    fb_file_t *file = fb_file_from_fd(fd);
    boot_framebuffer_t *framebuffer = file ? file->framebuffer : data;
    uint64_t size;

    (void)prot;
    if (!framebuffer || !file)
        return (void *)(int64_t)-EBADF;
    size = framebuffer->pitch * framebuffer->height;
    if (offset > size || len > size - offset)
        return (void *)(int64_t)-EINVAL;

    uint64_t fb_addr = translate_address(get_current_page_dir(false),
                                         (uint64_t)framebuffer->address) +
                       offset;

    map_page_range((uint64_t *)phys_to_virt(current_task->mm->page_table_addr),
                   (uint64_t)addr, (uint64_t)fb_addr, len,
                   PT_FLAG_R | PT_FLAG_W | PT_FLAG_U);

    return addr;
}

void fbdev_init() {
    boot_framebuffer_t *framebuffer = boot_get_framebuffer();

    if (!framebuffer) {
        return;
    }

    char name[16];
    sprintf(name, "fb%d", 0);
    device_install(DEV_CHAR, DEV_FB, framebuffer, name, 0, fb_open, fb_close,
                   fb_ioctl, NULL, fb_read, fb_write, fb_map);

    vfs_node_t *graphics = sysfs_ensure_dir("/sys/class/graphics");

    sprintf(name, "fb%d", 0);
    vfs_node_t *node = sysfs_child_append(graphics, name, true);

    vfs_node_t *modes = sysfs_child_append(node, "modes", false);
    char content[64];
    sprintf(content, "U:%dx%d\n", framebuffer->width, framebuffer->height);
    sysfs_write_node(modes, content, strlen(content), 0);

    vfs_node_t *device = sysfs_child_append(node, "device", true);
    vfs_node_t *subsystem =
        sysfs_child_append_symlink(device, "subsystem", "/sys/class/graphics");
    vfs_node_t *uevent = sysfs_child_append(device, "uevent", false);
    sprintf(content, "MAJOR=%d\nMINOR=%d\nDEVNAME=fb%d\nSUBSYSTEM=graphics\n",
            29, 0, 0);
    sysfs_write_node(uevent, content, strlen(content), 0);

    char *subsystem_fullpath = sysfs_node_path(subsystem);
    vfs_node_t *subsystem_link =
        sysfs_child_append_symlink(node, "subsystem", subsystem_fullpath);
    free(subsystem_fullpath);

    char *uevent_fullpath = sysfs_node_path(uevent);
    vfs_node_t *uevent_link =
        sysfs_child_append_symlink(node, "uevent", uevent_fullpath);
    free(uevent_fullpath);

    vfs_iput(graphics);
    vfs_iput(node);
    vfs_iput(modes);
    vfs_iput(device);
    vfs_iput(subsystem);
    vfs_iput(uevent);
    if (subsystem_link)
        vfs_iput(subsystem_link);
    if (uevent_link)
        vfs_iput(uevent_link);
}
