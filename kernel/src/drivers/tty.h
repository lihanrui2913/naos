#include <libs/klibc.h>
#include <libs/llist.h>

#define NCCS 32
typedef struct termios {
    uint32_t c_iflag;   /* input mode flags */
    uint32_t c_oflag;   /* output mode flags */
    uint32_t c_cflag;   /* control mode flags */
    uint32_t c_lflag;   /* local mode flags */
    uint8_t c_line;     /* line discipline */
    uint8_t c_cc[NCCS]; /* control characters */
} termios;

#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

#define IGNBRK 0000001
#define BRKINT 0000002
#define IGNPAR 0000004
#define PARMRK 0000010
#define INPCK 0000020
#define ISTRIP 0000040
#define INLCR 0000100
#define IGNCR 0000200
#define ICRNL 0000400
#define IUCLC 0001000
#define IXON 0002000
#define IXANY 0004000
#define IXOFF 0010000
#define IMAXBEL 0020000
#define IUTF8 0040000

#define OPOST 0000001
#define OLCUC 0000002
#define ONLCR 0000004
#define OCRNL 0000010
#define ONOCR 0000020
#define ONLRET 0000040
#define OFILL 0000100
#define OFDEL 0000200
#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE)
#define NLDLY 0000400
#define NL0 0000000
#define NL1 0000400
#define CRDLY 0003000
#define CR0 0000000
#define CR1 0001000
#define CR2 0002000
#define CR3 0003000
#define TABDLY 0014000
#define TAB0 0000000
#define TAB1 0004000
#define TAB2 0010000
#define TAB3 0014000
#define BSDLY 0020000
#define BS0 0000000
#define BS1 0020000
#define FFDLY 0100000
#define FF0 0000000
#define FF1 0100000
#endif

#define VTDLY 0040000
#define VT0 0000000
#define VT1 0040000

#define B0 0000000
#define B50 0000001
#define B75 0000002
#define B110 0000003
#define B134 0000004
#define B150 0000005
#define B200 0000006
#define B300 0000007
#define B600 0000010
#define B1200 0000011
#define B1800 0000012
#define B2400 0000013
#define B4800 0000014
#define B9600 0000015
#define B19200 0000016
#define B38400 0000017

#define B57600 0010001
#define B115200 0010002
#define B230400 0010003
#define B460800 0010004
#define B500000 0010005
#define B576000 0010006
#define B921600 0010007
#define B1000000 0010010
#define B1152000 0010011
#define B1500000 0010012
#define B2000000 0010013
#define B2500000 0010014
#define B3000000 0010015
#define B3500000 0010016
#define B4000000 0010017

#define CSIZE 0000060
#define CS5 0000000
#define CS6 0000020
#define CS7 0000040
#define CS8 0000060
#define CSTOPB 0000100
#define CREAD 0000200
#define PARENB 0000400
#define PARODD 0001000
#define HUPCL 0002000
#define CLOCAL 0004000

#define ISIG 0000001
#define ICANON 0000002
#define ECHO 0000010
#define ECHOE 0000020
#define ECHOK 0000040
#define ECHONL 0000100
#define NOFLSH 0000200
#define TOSTOP 0000400
#define IEXTEN 0100000

#define TCOOFF 0
#define TCOON 1
#define TCIOFF 2
#define TCION 3

#define TCIFLUSH 0
#define TCOFLUSH 1
#define TCIOFLUSH 2

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#define EXTA 0000016
#define EXTB 0000017
#define CBAUD 0010017
#define CBAUDEX 0010000
#define CIBAUD 002003600000
#define CMSPAR 010000000000
#define CRTSCTS 020000000000

#define XCASE 0000004
#define ECHOCTL 0001000
#define ECHOPRT 0002000
#define ECHOKE 0004000
#define FLUSHO 0010000
#define PENDIN 0040000
#define EXTPROC 0200000

#define XTABS 0014000

struct vt_mode {
    char mode;    /* vt mode */
    char waitv;   /* if set, hang on writes if not active */
    short relsig; /* signal to raise on release req */
    short acqsig; /* signal to raise on acquisition */
    short frsig;  /* unused (set to 0) */
};
#define VT_GETMODE 0x5601 /* get mode of active vt */
#define VT_SETMODE 0x5602 /* set mode of active vt */
#define VT_AUTO 0x00      /* auto vt switching */
#define VT_PROCESS 0x01   /* process controls switching */
#define VT_ACKACQ 0x02    /* acknowledge switch */

#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSETSW 0x5403
#define TCSETSF 0x5404
#define TCGETA 0x5405
#define TCSETA 0x5406
#define TCSETAW 0x5407
#define TCSETAF 0x5408
#define TCSBRK 0x5409
#define TCXONC 0x540A
#define TCFLSH 0x540B
#define TIOCEXCL 0x540C
#define TIOCNXCL 0x540D
#define TIOCSCTTY 0x540E
#define TIOCGPGRP 0x540F
#define TIOCSPGRP 0x5410
#define TIOCOUTQ 0x5411
#define TIOCSTI 0x5412
#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define TIOCMGET 0x5415
#define TIOCMBIS 0x5416
#define TIOCMBIC 0x5417
#define TIOCMSET 0x5418
#define TIOCGSOFTCAR 0x5419
#define TIOCSSOFTCAR 0x541A
#define FIONREAD 0x541B
#define TIOCINQ FIONREAD
#define TIOCLINUX 0x541C
#define TIOCCONS 0x541D
#define TIOCGSERIAL 0x541E
#define TIOCSSERIAL 0x541F
#define TIOCPKT 0x5420
#define FIONBIO 0x5421
#define TIOCNOTTY 0x5422
#define TIOCSETD 0x5423
#define TIOCGETD 0x5424
#define TCSBRKP 0x5425  /* Needed for POSIX tcsendbreak() */
#define TIOCSBRK 0x5427 /* BSD compatibility */
#define TIOCCBRK 0x5428 /* BSD compatibility */
#define TIOCGSID 0x5429 /* Return the session ID of FD */

struct winsize {
    uint16_t ws_row;
    uint16_t ws_col;
    uint16_t ws_xpixel;
    uint16_t ws_ypixel;
};

#define KDGETMODE 0x4B3B // 获取终端模式命令
#define KDSETMODE 0x4B3A // 设置终端模式命令
#define KD_TEXT 0x00     // 文本模式
#define KD_GRAPHICS 0x01 // 图形模式

#define KDGKBMODE 0x4B44 /* gets current keyboard mode */
#define KDSKBMODE 0x4B45 /* sets current keyboard mode */
#define K_RAW 0x00       // 原始模式（未处理扫描码）
#define K_XLATE 0x01     // 转换模式（生成ASCII）
#define K_MEDIUMRAW 0x02 // 中等原始模式
#define K_UNICODE 0x03   // Unicode模式

#define VT_OPENQRY 0x5600 /* get next available vt */
#define VT_GETMODE 0x5601 /* get mode of active vt */
#define VT_SETMODE 0x5602

#define VT_GETSTATE 0x5603
#define VT_SENDSIG 0x5604

#define VT_ACTIVATE 0x5606   /* make vt active */
#define VT_WAITACTIVE 0x5607 /* wait for vt active */

struct vt_state {
    uint16_t v_active; // 活动终端号
    uint16_t v_state;  // 终端状态标志
};

#define VT_AUTO 0x00    // 自动切换模式
#define VT_PROCESS 0x01 // 进程控制模式

enum tty_device_type {
    TTY_DEVICE_SERIAL = 0, // 串口设备
    TTY_DEVICE_GRAPHI = 1, // 图形设备
};

typedef struct tty_virtual_device tty_device_t;
typedef struct tty_session tty_t;

typedef struct tty_device_ops {
    size_t (*write)(tty_device_t *device, const char *buf, size_t count);
    size_t (*read)(tty_device_t *device, char *buf, size_t count);
    void (*flush)(tty_device_t *res);
} tty_device_ops_t;

struct tty_graphics_ {
    void *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

struct tty_serial_ {
    uint16_t port;
};

typedef struct tty_virtual_device { // TTY 设备
    enum tty_device_type type;
    tty_device_ops_t ops; // 图形设备不具备 read write 操作
    void *private_data;   // 实际设备
    char name[32];

    struct llist_header node;
} tty_device_t;

typedef struct tty_session_ops {
    size_t (*write)(tty_t *device, const char *buf, size_t count);
    size_t (*read)(tty_t *device, char *buf, size_t count);
    void (*flush)(tty_t *res);
} tty_session_ops_t;

typedef struct tty_session { // 一个 TTY 会话
    void *terminal;
    struct termios termios;
    struct vt_mode current_vt_mode;
    int tty_kbmode;
    int tty_mode;
    uint64_t at_process_group_id;
    tty_session_ops_t ops;
    tty_device_t *device; // 会话所属的TTY设备
} tty_t;

extern tty_t *kernel_session;

int tty_write(void *dev, void *buf, uint64_t offset, size_t size,
              uint64_t flags);

tty_device_t *get_tty_device(const char *name);
tty_device_t *alloc_tty_device(enum tty_device_type type);
uint64_t register_tty_device(tty_device_t *device);
uint64_t delete_tty_device(tty_device_t *device);
void tty_init();
void tty_init_session();
void tty_init_session_serial();
