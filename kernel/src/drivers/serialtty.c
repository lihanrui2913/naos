#include <drivers/tty.h>
#include <drivers/serialtty.h>
#include <mm/mm.h>

void terminal_flush_serial(tty_t *session) {
    // flanterm_flush(session->terminal);
    return;
}

extern bool serial_initialized;

spinlock_t terminal_write_serial_lock = SPIN_INIT;

size_t terminal_write_serial(tty_t *device, const char *buf, size_t count) {
    spin_lock(&terminal_write_serial_lock);
    serial_printk(buf, count);
    spin_unlock(&terminal_write_serial_lock);
    return count;
}

uint64_t create_session_terminal_serial(tty_t *session) {
    if (session->device == NULL)
        return -ENODEV;
    if (session->device->type != TTY_DEVICE_SERIAL)
        return -EINVAL;

    session->terminal = NULL;
    memset(&session->termios, 0, sizeof(termios));
    session->termios.c_iflag = BRKINT | ICRNL | INPCK | ISTRIP | IXON;
    session->termios.c_oflag = OPOST;
    session->termios.c_cflag = CS8 | CREAD | CLOCAL;
    session->termios.c_lflag = ECHO | ICANON | IEXTEN | ISIG;
    session->termios.c_line = 0;
    session->termios.c_cc[VINTR] = 3; // Ctrl-C
    session->termios.c_cc[VQUIT] =
        28; // Ctrl-session->termios.c_cc[VERASE] = 127; // DEL
    session->termios.c_cc[VKILL] = 21;    // Ctrl-U
    session->termios.c_cc[VEOF] = 4;      // Ctrl-D
    session->termios.c_cc[VTIME] = 0;     // No timer
    session->termios.c_cc[VMIN] = 1;      // Return each byte
    session->termios.c_cc[VSTART] = 17;   // Ctrl-Q
    session->termios.c_cc[VSTOP] = 19;    // Ctrl-S
    session->termios.c_cc[VSUSP] = 26;    // Ctrl-Z
    session->termios.c_cc[VREPRINT] = 18; // Ctrl-R
    session->termios.c_cc[VDISCARD] = 15; // Ctrl-O
    session->termios.c_cc[VWERASE] = 23;  // Ctrl-W
    session->termios.c_cc[VLNEXT] = 22;   // Ctrl-V
    // Initialize other control characters to 0
    for (int i = 16; i < NCCS; i++) {
        session->termios.c_cc[i] = 0;
    }

    session->tty_mode = KD_TEXT;
    session->tty_kbmode = K_XLATE;
    // session->terminal = fl_context;
    session->ops.flush = terminal_flush_serial;
    session->ops.read = NULL;
    session->ops.write = terminal_write_serial;

    return EOK;
}
