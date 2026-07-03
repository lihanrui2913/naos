#include <drivers/serial.h>
#include <init/callbacks.h>

bool serial_initialized = false;

static serial_driver_t *active_serial_driver = NULL;
static spinlock_t serial_write_lock = SPIN_INIT;
#define SERIAL_READABLE_NOTIFIER_LIMIT 16
static serial_readable_notifier_t
    serial_readable_notifiers[SERIAL_READABLE_NOTIFIER_LIMIT];
static void *serial_readable_notifier_opaque[SERIAL_READABLE_NOTIFIER_LIMIT];
static size_t serial_readable_notifier_count = 0;
static bool serial_sched_update_registered = false;
static bool serial_readable_latched = false;

static int serial_sched_update_notify(void) {
    bool readable = serial_can_read();

    if (!readable) {
        serial_readable_latched = false;
        return 0;
    }

    if (serial_readable_latched)
        return 0;

    serial_readable_latched = true;
    for (size_t i = 0; i < serial_readable_notifier_count; i++) {
        if (serial_readable_notifiers[i])
            serial_readable_notifiers[i](serial_readable_notifier_opaque[i]);
    }

    return 0;
}

int serial_register_driver(serial_driver_t *driver) {
    if (!driver || !driver->write)
        return -1;

    active_serial_driver = driver;
    serial_initialized = true;
    serial_readable_latched = false;
    if (!serial_sched_update_registered) {
        regist_on_sched_update_callback(serial_sched_update_notify);
        serial_sched_update_registered = true;
    }
    return 0;
}

serial_driver_t *serial_get_driver() { return active_serial_driver; }

bool serial_can_read() {
    if (!serial_initialized || !active_serial_driver)
        return false;

    if (active_serial_driver->can_read)
        return active_serial_driver->can_read(active_serial_driver);

    return active_serial_driver->read != NULL;
}

bool serial_read(char *ch) {
    if (!ch || !serial_initialized || !active_serial_driver ||
        !active_serial_driver->read)
        return false;

    return active_serial_driver->read(active_serial_driver, ch);
}

char read_serial() {
    char ch = 0;
    (void)serial_read(&ch);
    return ch;
}

void write_serial(char ch) {
    if (!serial_initialized || !active_serial_driver ||
        !active_serial_driver->write) {
        return;
    }

    active_serial_driver->write(active_serial_driver, ch);
}

void serial_printk(const char *buf, int len) {
    if (!serial_initialized || !buf || len <= 0)
        return;

    spin_lock(&serial_write_lock);

    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n')
            write_serial('\r');
        write_serial(buf[i]);
    }

    spin_unlock(&serial_write_lock);
}

int serial_register_readable_notifier(serial_readable_notifier_t fn,
                                      void *opaque) {
    if (!fn || serial_readable_notifier_count >= SERIAL_READABLE_NOTIFIER_LIMIT)
        return -1;

    serial_readable_notifiers[serial_readable_notifier_count] = fn;
    serial_readable_notifier_opaque[serial_readable_notifier_count] = opaque;
    serial_readable_notifier_count++;
    return 0;
}
