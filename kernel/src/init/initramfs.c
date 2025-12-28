#include <init/initramfs.h>
#include <boot/boot.h>

uint32_t parse_hex(const char *c, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        uint32_t d;
        if (*c >= 'a' && *c <= 'f') {
            d = *c++ - 'a' + 10;
        } else if (*c >= 'A' && *c <= 'F') {
            d = *c++ - 'A' + 10;
        } else if (*c >= '0' && *c <= '9') {
            d = *c++ - '0';
        } else {
            ASSERT(!"Unexpected character in CPIO header");
        }
        v = (v << 4) | d;
    }
    return v;
}

struct header {
    char magic[6];
    char inode[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char numLinks[8];
    char mtime[8];
    char fileSize[8];
    char devMajor[8];
    char devMinor[8];
    char rdevMajor[8];
    char rdevMinor[8];
    char nameSize[8];
    char check[8];
};

const uint32_t type_mask = 0170000;
const uint32_t regular_type = 0100000;
const uint32_t directory_type = 0040000;

void *initramfs_data = NULL;
size_t initramfs_size = 0;

void initramfs_init() {
    boot_module_t *modules[128];
    size_t count;
    boot_get_modules(modules, &count);
    if (count == 0)
        return;
    initramfs_data = modules[0]->data;
    initramfs_size = modules[0]->size;
}

initramfs_handle_t *initramfs_lookup(const char *path) {
    if (!initramfs_data || !initramfs_size)
        return NULL;
    void *p = initramfs_data;
    uint64_t limit = initramfs_size;
    while (true) {
        struct header h;
        memcpy(&h, p, sizeof(struct header));

        uint32_t magic = parse_hex(h.magic, 6);
        ASSERT(magic == 0x070701 || magic == 0x070702);

        uint32_t mode = parse_hex(h.mode, 8);
        uint32_t name_size = parse_hex(h.nameSize, 8);
        uint32_t file_size = parse_hex(h.fileSize, 8);
        void *data = p + ((sizeof(struct header) + name_size + 3) & ~3);

        char name[name_size];
        memset(name, 0, name_size);
        memcpy(name, p + sizeof(struct header), name_size - 1);
        if (!strcmp(name, "TRAILER!!!"))
            break;
        if (!strcmp(name, "."))
            goto next;

        if (!strcmp(name, path)) {
            initramfs_handle_t *handle = malloc(sizeof(initramfs_handle_t));
            handle->data = data;
            return handle;
        }

    next:
        p = data + ((file_size + 3) & ~3);
    }

    return NULL;
}

void initramfs_read(initramfs_handle_t *handle, void *buf, size_t offset,
                    size_t len) {
    memcpy(buf, handle->data + offset, len);
}
