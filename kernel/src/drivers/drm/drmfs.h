#pragma once

#include <fs/vfs/vfs.h>

int drmfs_create_file(const char *prefix, const struct vfs_file_operations *ops,
                      void *private_data, umode_t mode, unsigned int open_flags,
                      struct vfs_file **out_file, struct vfs_inode **out_inode);
bool drmfs_owns_file(struct vfs_file *file);
