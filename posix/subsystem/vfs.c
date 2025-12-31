#include "vfs.h"

struct llist_header mount_points;

void vfs_init() { llist_init_head(&mount_points); }
