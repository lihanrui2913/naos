#pragma once

#include <libs/klibc.h>

typedef struct initramfs_handle {
    void *data;
} initramfs_handle_t;

void initramfs_init();
initramfs_handle_t *initramfs_lookup(const char *path);
void initramfs_read(initramfs_handle_t *handle, void *buf, size_t offset,
                    size_t len);
