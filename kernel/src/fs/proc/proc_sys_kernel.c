#include <fs/proc.h>
#include <boot/boot.h>
#include <drivers/smbios.h>
#include <task/ns.h>
#include <task/task.h>

#define PROC_SYS_KERNEL_CAP_LAST_CAP "40\n"
#define PROC_SYS_KERNEL_THREADS_MAX "629145\n"
#define PROC_SYS_FS_NR_OPEN "1048576\n"

static char proc_sys_kernel_boot_id[37];
static bool proc_sys_kernel_boot_id_ready = false;

static uint64_t proc_sys_kernel_boot_id_next(uint64_t *state) {
    uint64_t x = *state;

    if (x == 0)
        x = UINT64_C(0x9e3779b97f4a7c15);

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static void proc_sys_kernel_boot_id_ensure(void) {
    smbios_system_info_t system_info;
    uint8_t bytes[16];
    uint64_t state;

    if (proc_sys_kernel_boot_id_ready)
        return;

    state = nano_time() ^ (boot_get_boottime() << 32) ^
            (uint64_t)(uintptr_t)&proc_sys_kernel_boot_id_ready;

    for (size_t i = 0; i < sizeof(bytes); i += sizeof(uint64_t)) {
        uint64_t value = proc_sys_kernel_boot_id_next(&state);
        size_t chunk = MIN(sizeof(uint64_t), sizeof(bytes) - i);
        memcpy(bytes + i, &value, chunk);
    }

    if (smbios_get_system_info(&system_info) == 0) {
        for (size_t i = 0; i < sizeof(bytes); i++)
            bytes[i] ^= system_info.uuid[i];
    }

    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;

    snprintf(proc_sys_kernel_boot_id, sizeof(proc_sys_kernel_boot_id),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
             "%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5],
             bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11],
             bytes[12], bytes[13], bytes[14], bytes[15]);
    proc_sys_kernel_boot_id_ready = true;
}

static const char *proc_sys_kernel_uts_value(size_t *len_out, bool domainname) {
    task_t *task = current_task;
    task_uts_namespace_t *uts_ns =
        (task && task->nsproxy) ? task->nsproxy->uts_ns : NULL;
    const char *value;

    if (domainname) {
        value = uts_ns ? uts_ns->domainname : "";
    } else {
        value = uts_ns ? uts_ns->nodename : "aether";
    }

    if (len_out)
        *len_out = strlen(value);
    return value;
}

static size_t proc_sys_kernel_uts_read(void *addr, size_t offset, size_t size,
                                       bool domainname) {
    size_t len = 0;
    const char *content = proc_sys_kernel_uts_value(&len, domainname);

    if (offset >= len)
        return 0;

    size_t to_copy = MIN(size, len - offset);
    memcpy(addr, content + offset, to_copy);
    return to_copy;
}

static ssize_t proc_sys_kernel_uts_write(const void *addr, size_t offset,
                                         size_t size, bool domainname) {
    task_t *task = current_task;
    task_uts_namespace_t *uts_ns =
        (task && task->nsproxy) ? task->nsproxy->uts_ns : NULL;
    char *target;
    size_t max_len;
    size_t copy_len;

    (void)offset;
    if (!uts_ns || !addr)
        return -EINVAL;

    target = domainname ? uts_ns->domainname : uts_ns->nodename;
    max_len = sizeof(uts_ns->domainname) - 1;
    copy_len = MIN(size, max_len);

    while (copy_len > 0 && (((const char *)addr)[copy_len - 1] == '\n' ||
                            ((const char *)addr)[copy_len - 1] == '\0')) {
        copy_len--;
    }

    memcpy(target, addr, copy_len);
    target[copy_len] = '\0';
    return size;
}

size_t proc_sys_kernel_osrelease_stat(proc_handle_t *handle) {
    return strlen(BUILD_VERSION);
}

size_t proc_sys_kernel_osrelease_read(proc_handle_t *handle, void *addr,
                                      size_t offset, size_t size) {
    const char *content = BUILD_VERSION;
    size_t len = strlen(content);
    if (offset >= len) {
        return 0;
    }
    size_t to_copy = MIN(size, len - offset);
    memcpy(addr, content + offset, to_copy);
    return to_copy;
}

static size_t proc_sys_kernel_const_read(const char *content, void *addr,
                                         size_t offset, size_t size) {
    size_t len = strlen(content);

    if (offset >= len)
        return 0;

    size_t to_copy = MIN(size, len - offset);
    memcpy(addr, content + offset, to_copy);
    return to_copy;
}

size_t proc_sys_kernel_pid_max_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen("4194304\n");
}

size_t proc_sys_kernel_pid_max_read(proc_handle_t *handle, void *addr,
                                    size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read("4194304\n", addr, offset, size);
}

size_t proc_sys_kernel_tainted_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen("0\n");
}

size_t proc_sys_kernel_tainted_read(proc_handle_t *handle, void *addr,
                                    size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read("0\n", addr, offset, size);
}

size_t proc_sys_kernel_printk_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen("4\t4\t1\t7\n");
}

size_t proc_sys_kernel_printk_read(proc_handle_t *handle, void *addr,
                                   size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read("4\t4\t1\t7\n", addr, offset, size);
}

size_t proc_sys_kernel_cap_last_cap_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen(PROC_SYS_KERNEL_CAP_LAST_CAP);
}

size_t proc_sys_kernel_cap_last_cap_read(proc_handle_t *handle, void *addr,
                                         size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read(PROC_SYS_KERNEL_CAP_LAST_CAP, addr,
                                      offset, size);
}

size_t proc_sys_kernel_threads_max_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen(PROC_SYS_KERNEL_THREADS_MAX);
}

size_t proc_sys_kernel_threads_max_read(proc_handle_t *handle, void *addr,
                                        size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read(PROC_SYS_KERNEL_THREADS_MAX, addr, offset,
                                      size);
}

size_t proc_sys_kernel_boot_id_stat(proc_handle_t *handle) {
    (void)handle;
    proc_sys_kernel_boot_id_ensure();
    return strlen(proc_sys_kernel_boot_id);
}

size_t proc_sys_kernel_boot_id_read(proc_handle_t *handle, void *addr,
                                    size_t offset, size_t size) {
    (void)handle;
    proc_sys_kernel_boot_id_ensure();
    return proc_sys_kernel_const_read(proc_sys_kernel_boot_id, addr, offset,
                                      size);
}

size_t proc_sys_kernel_hostname_stat(proc_handle_t *handle) {
    size_t len = 0;

    (void)handle;
    (void)proc_sys_kernel_uts_value(&len, false);
    return len;
}

size_t proc_sys_kernel_hostname_read(proc_handle_t *handle, void *addr,
                                     size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_uts_read(addr, offset, size, false);
}

ssize_t proc_sys_kernel_hostname_write(proc_handle_t *handle, const void *addr,
                                       size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_uts_write(addr, offset, size, false);
}

size_t proc_sys_kernel_domainname_stat(proc_handle_t *handle) {
    size_t len = 0;

    (void)handle;
    (void)proc_sys_kernel_uts_value(&len, true);
    return len;
}

size_t proc_sys_kernel_domainname_read(proc_handle_t *handle, void *addr,
                                       size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_uts_read(addr, offset, size, true);
}

ssize_t proc_sys_kernel_domainname_write(proc_handle_t *handle,
                                         const void *addr, size_t offset,
                                         size_t size) {
    (void)handle;
    return proc_sys_kernel_uts_write(addr, offset, size, true);
}

size_t proc_sys_fs_nr_open_stat(proc_handle_t *handle) {
    (void)handle;
    return strlen(PROC_SYS_FS_NR_OPEN);
}

size_t proc_sys_fs_nr_open_read(proc_handle_t *handle, void *addr,
                                size_t offset, size_t size) {
    (void)handle;
    return proc_sys_kernel_const_read(PROC_SYS_FS_NR_OPEN, addr, offset, size);
}
