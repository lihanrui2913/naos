#pragma once

#include <libs/klibc.h>
#include <net/socket.h>

/* Netlink protocol family */
#ifndef AF_INET
#define AF_INET 2
#endif
#define AF_NETLINK 16
#define PF_NETLINK AF_NETLINK

/* Netlink protocols */
#define NETLINK_ROUTE 0
#define NETLINK_UNUSED 1
#define NETLINK_USERSOCK 2
#define NETLINK_FIREWALL 3
#define NETLINK_SOCK_DIAG 4
#define NETLINK_NFLOG 5
#define NETLINK_XFRM 6
#define NETLINK_SELINUX 7
#define NETLINK_ISCSI 8
#define NETLINK_AUDIT 9
#define NETLINK_FIB_LOOKUP 10
#define NETLINK_CONNECTOR 11
#define NETLINK_NETFILTER 12
#define NETLINK_IP6_FW 13
#define NETLINK_DNRTMSG 14
#define NETLINK_KOBJECT_UEVENT 15
#define NETLINK_GENERIC 16
#define NETLINK_SCSITRANSPORT 18
#define NETLINK_ECRYPTFS 19
#define NETLINK_RDMA 20
#define NETLINK_CRYPTO 21
#define NETLINK_SMC 22

#define NETLINK_ADD_MEMBERSHIP 1
#define NETLINK_DROP_MEMBERSHIP 2

#define NETLINK_BUFFER_SIZE 32768
#define NETLINK_BUFFER_INITIAL_SIZE NETLINK_BUFFER_SIZE
#define NETLINK_BUFFER_MAX_SIZE (NETLINK_BUFFER_SIZE * 32)

struct nlmsghdr {
    uint32_t nlmsg_len;
    uint16_t nlmsg_type;
    uint16_t nlmsg_flags;
    uint32_t nlmsg_seq;
    uint32_t nlmsg_pid;
};

struct nlmsgerr {
    int error;
    struct nlmsghdr msg;
};

struct rtattr {
    uint16_t rta_len;
    uint16_t rta_type;
};

struct ifinfomsg {
    uint8_t ifi_family;
    uint8_t __ifi_pad;
    uint16_t ifi_type;
    int32_t ifi_index;
    uint32_t ifi_flags;
    uint32_t ifi_change;
};

struct genlmsghdr {
    uint8_t cmd;
    uint8_t version;
    uint16_t reserved;
};

#define NLM_F_REQUEST 0x01
#define NLM_F_MULTI 0x02
#define NLM_F_ACK 0x04
#define NLM_F_ROOT 0x100
#define NLM_F_MATCH 0x200
#define NLM_F_DUMP (NLM_F_ROOT | NLM_F_MATCH)

#define NLMSG_NOOP 0x1
#define NLMSG_ERROR 0x2
#define NLMSG_DONE 0x3
#define NLMSG_MIN_TYPE 0x10

#define NLMSG_ALIGNTO 4U
#define NLMSG_ALIGN(len) (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1))
#define NLMSG_HDRLEN ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len) + NLMSG_HDRLEN)
#define NLMSG_SPACE(len) NLMSG_ALIGN(NLMSG_LENGTH(len))
#define NLMSG_DATA(nlh) ((void *)(((char *)(nlh)) + NLMSG_HDRLEN))
#define NLMSG_NEXT(nlh, len)                                                   \
    ((len) -= NLMSG_ALIGN((nlh)->nlmsg_len),                                   \
     (struct nlmsghdr *)(((char *)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#define NLMSG_OK(nlh, len)                                                     \
    ((len) >= (int)sizeof(struct nlmsghdr) &&                                  \
     (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) &&                            \
     (nlh)->nlmsg_len <= (uint32_t)(len))
#define NLMSG_PAYLOAD(nlh, len) ((nlh)->nlmsg_len - NLMSG_SPACE((len)))

#define GENL_NAMSIZ 16
#define GENL_HDRLEN NLMSG_ALIGN(sizeof(struct genlmsghdr))
#define GENL_ID_CTRL NLMSG_MIN_TYPE
#define GENL_START_ALLOC (NLMSG_MIN_TYPE + 3)

#define RTA_ALIGNTO 4U
#define RTA_ALIGN(len) (((len) + RTA_ALIGNTO - 1) & ~(RTA_ALIGNTO - 1))
#define RTA_LENGTH(len) (RTA_ALIGN(sizeof(struct rtattr)) + (len))
#define RTA_SPACE(len) RTA_ALIGN(RTA_LENGTH(len))
#define RTA_DATA(rta) ((void *)(((char *)(rta)) + RTA_LENGTH(0)))
#define RTA_TYPE_MASK 0x3FFFU
#define RTA_OK(rta, len)                                                       \
    ((len) >= (int)sizeof(struct rtattr) &&                                    \
     (rta)->rta_len >= sizeof(struct rtattr) &&                                \
     (rta)->rta_len <= (uint16_t)(len))
#define RTA_NEXT(rta, attrlen)                                                 \
    ((attrlen) -= RTA_ALIGN((rta)->rta_len),                                   \
     (struct rtattr *)(((char *)(rta)) + RTA_ALIGN((rta)->rta_len)))

#define RTM_NEWLINK 16
#define RTM_DELLINK 17
#define RTM_GETLINK 18
#define RTM_SETLINK 19
#define RTM_NEWADDR 20
#define RTM_DELADDR 21
#define RTM_GETADDR 22
#define RTM_NEWROUTE 24
#define RTM_DELROUTE 25
#define RTM_GETROUTE 26
#define RTM_NEWRULE 32
#define RTM_DELRULE 33
#define RTM_GETRULE 34

struct ifaddrmsg {
    uint8_t ifa_family;
    uint8_t ifa_prefixlen;
    uint8_t ifa_flags;
    uint8_t ifa_scope;
    uint32_t ifa_index;
};

struct rtmsg {
    uint8_t rtm_family;
    uint8_t rtm_dst_len;
    uint8_t rtm_src_len;
    uint8_t rtm_tos;
    uint8_t rtm_table;
    uint8_t rtm_protocol;
    uint8_t rtm_scope;
    uint8_t rtm_type;
    uint32_t rtm_flags;
};

struct fib_rule_hdr {
    uint8_t family;
    uint8_t dst_len;
    uint8_t src_len;
    uint8_t tos;
    uint8_t table;
    uint8_t res1;
    uint8_t res2;
    uint8_t action;
    uint32_t flags;
};

#define IFLA_UNSPEC 0
#define IFLA_ADDRESS 1
#define IFLA_IFNAME 3
#define IFLA_MTU 4
#define IFLA_WIRELESS 11
#define IFLA_TXQLEN 13
#define IFLA_OPERSTATE 16
#define IFLA_LINKMODE 17

#define RTNLGRP_LINK 1
#define RTNLGRP_IPV4_IFADDR 5
#define RTNLGRP_IPV4_ROUTE 7
#define RTNLGRP_IPV4_RULE 8

#define IFA_UNSPEC 0
#define IFA_ADDRESS 1
#define IFA_LOCAL 2
#define IFA_LABEL 3

#define RTA_UNSPEC 0
#define RTA_DST 1
#define RTA_OIF 4
#define RTA_GATEWAY 5
#define RTA_PREFSRC 7
#define RTA_TABLE 15

#define FRA_UNSPEC 0
#define FRA_PRIORITY 6
#define FRA_TABLE 15

#define RT_TABLE_UNSPEC 0
#define RT_TABLE_DEFAULT 253
#define RT_TABLE_MAIN 254
#define RT_TABLE_LOCAL 255

#define RTPROT_UNSPEC 0
#define RTPROT_KERNEL 2
#define RTPROT_STATIC 4

#define RT_SCOPE_UNIVERSE 0
#define RT_SCOPE_LINK 253
#define RT_SCOPE_HOST 254

#define RTN_UNSPEC 0
#define RTN_UNICAST 1
#define RTN_LOCAL 2

#define FR_ACT_UNSPEC 0
#define FR_ACT_TO_TBL 1

#define CTRL_CMD_UNSPEC 0
#define CTRL_CMD_NEWFAMILY 1
#define CTRL_CMD_DELFAMILY 2
#define CTRL_CMD_GETFAMILY 3

#define CTRL_ATTR_UNSPEC 0
#define CTRL_ATTR_FAMILY_ID 1
#define CTRL_ATTR_FAMILY_NAME 2
#define CTRL_ATTR_VERSION 3
#define CTRL_ATTR_HDRSIZE 4
#define CTRL_ATTR_MAXATTR 5
#define CTRL_ATTR_MCAST_GROUPS 7

#define CTRL_ATTR_MCAST_GRP_UNSPEC 0
#define CTRL_ATTR_MCAST_GRP_NAME 1
#define CTRL_ATTR_MCAST_GRP_ID 2

#define NAOS_GENL_ID_NL80211 GENL_START_ALLOC
#define NL80211_GENL_NAME "nl80211"
#define NL80211_GENL_VERSION 1
#define NL80211_MCGRP_SCAN_NAME "scan"
#define NL80211_MCGRP_SCAN_ID 1
#define NL80211_MCGRP_MLME_NAME "mlme"
#define NL80211_MCGRP_MLME_ID 2

#define NL80211_CMD_UNSPEC 0
#define NL80211_CMD_GET_WIPHY 1
#define NL80211_CMD_SET_WIPHY 2
#define NL80211_CMD_NEW_WIPHY 3
#define NL80211_CMD_GET_INTERFACE 5
#define NL80211_CMD_SET_INTERFACE 6
#define NL80211_CMD_NEW_INTERFACE 7
#define NL80211_CMD_SET_KEY 10
#define NL80211_CMD_NEW_KEY 11
#define NL80211_CMD_DEL_KEY 12
#define NL80211_CMD_GET_STATION 17
#define NL80211_CMD_SET_BSS 25
#define NL80211_CMD_GET_SCAN 32
#define NL80211_CMD_TRIGGER_SCAN 33
#define NL80211_CMD_NEW_SCAN_RESULTS 34
#define NL80211_CMD_SCAN_ABORTED 35
#define NL80211_CMD_AUTHENTICATE 37
#define NL80211_CMD_ASSOCIATE 38
#define NL80211_CMD_DEAUTHENTICATE 39
#define NL80211_CMD_DISASSOCIATE 40
#define NL80211_CMD_CONNECT 46
#define NL80211_CMD_DISCONNECT 48
#define NL80211_CMD_REGISTER_FRAME 58
#define NL80211_CMD_FRAME 59
#define NL80211_CMD_SET_POWER_SAVE 61
#define NL80211_CMD_GET_POWER_SAVE 62
#define NL80211_CMD_GET_WOWLAN 73
#define NL80211_CMD_SET_WOWLAN 74
#define NL80211_CMD_GET_PROTOCOL_FEATURES 95

#define NL80211_ATTR_UNSPEC 0
#define NL80211_ATTR_WIPHY 1
#define NL80211_ATTR_WIPHY_NAME 2
#define NL80211_ATTR_IFINDEX 3
#define NL80211_ATTR_IFNAME 4
#define NL80211_ATTR_IFTYPE 5
#define NL80211_ATTR_MAC 6
#define NL80211_ATTR_KEY_DATA 7
#define NL80211_ATTR_KEY_IDX 8
#define NL80211_ATTR_KEY_CIPHER 9
#define NL80211_ATTR_KEY_SEQ 10
#define NL80211_ATTR_KEY_DEFAULT 11
#define NL80211_ATTR_STA_INFO 21
#define NL80211_ATTR_WIPHY_BANDS 22
#define NL80211_ATTR_SUPPORTED_IFTYPES 32
#define NL80211_ATTR_WIPHY_FREQ 38
#define NL80211_ATTR_IE 42
#define NL80211_ATTR_MAX_NUM_SCAN_SSIDS 43
#define NL80211_ATTR_SCAN_SSIDS 45
#define NL80211_ATTR_GENERATION 46
#define NL80211_ATTR_BSS 47
#define NL80211_ATTR_SUPPORTED_COMMANDS 50
#define NL80211_ATTR_FRAME 51
#define NL80211_ATTR_SSID 52
#define NL80211_ATTR_AUTH_TYPE 53
#define NL80211_ATTR_REASON_CODE 54
#define NL80211_ATTR_KEY_TYPE 55
#define NL80211_ATTR_CIPHER_SUITES 57
#define NL80211_ATTR_PRIVACY 70
#define NL80211_ATTR_DISCONNECTED_BY_AP 71
#define NL80211_ATTR_STATUS_CODE 72
#define NL80211_ATTR_REQ_IE 77
#define NL80211_ATTR_RESP_IE 78
#define NL80211_ATTR_DURATION 87
#define NL80211_ATTR_COOKIE 88
#define NL80211_ATTR_FRAME_MATCH 91
#define NL80211_ATTR_ACK 92
#define NL80211_ATTR_PS_STATE 93
#define NL80211_ATTR_FRAME_TYPE 101
#define NL80211_ATTR_WOWLAN_TRIGGERS 117
#define NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED 118
#define NL80211_ATTR_PROTOCOL_FEATURES 172
#define NL80211_ATTR_SOCKET_OWNER 204
#define NL80211_ATTR_BSSID 245

#define NL80211_BSS_BSSID 1
#define NL80211_BSS_FREQUENCY 2
#define NL80211_BSS_TSF 3
#define NL80211_BSS_BEACON_INTERVAL 4
#define NL80211_BSS_CAPABILITY 5
#define NL80211_BSS_INFORMATION_ELEMENTS 6
#define NL80211_BSS_SIGNAL_MBM 7
#define NL80211_BSS_STATUS 9
#define NL80211_BSS_SEEN_MS_AGO 10
#define NL80211_BSS_BEACON_IES 11

#define NL80211_BSS_STATUS_AUTHENTICATED 0
#define NL80211_BSS_STATUS_ASSOCIATED 1

#define NL80211_AUTHTYPE_OPEN_SYSTEM 0
#define NL80211_AUTHTYPE_AUTOMATIC 8

#define NL80211_PS_DISABLED 0

/* Band and frequency attributes */
#define NL80211_BAND_2GHZ 0
#define NL80211_BAND_5GHZ 1

#define NL80211_BAND_ATTR_FREQS 1
#define NL80211_BAND_ATTR_RATES 2

#define NL80211_FREQUENCY_ATTR_FREQ 1
#define NL80211_FREQUENCY_ATTR_DISABLED 2

/* Cipher suite values */
#define WLAN_CIPHER_SUITE_WEP40 0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP 0x000FAC02
#define WLAN_CIPHER_SUITE_CCMP 0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104 0x000FAC05

#define ARPHRD_ETHER 1

#define IFF_UP 0x1
#define IFF_BROADCAST 0x2
#define IFF_RUNNING 0x40
#define IFF_MULTICAST 0x1000
#define IFF_LOWER_UP 0x10000

#define IF_OPER_UNKNOWN 0
#define IF_OPER_NOTPRESENT 1
#define IF_OPER_DOWN 2
#define IF_OPER_LOWERLAYERDOWN 3
#define IF_OPER_TESTING 4
#define IF_OPER_DORMANT 5
#define IF_OPER_UP 6

struct sockaddr_nl {
    uint16_t nl_family;
    unsigned short nl_pad;
    uint32_t nl_pid;
    uint32_t nl_groups;
};

// Opaque netlink buffer structure
struct netlink_buffer;
struct netdev;

// Netlink packet header for storing sender information in buffer
struct netlink_packet_hdr {
    uint32_t nl_pid;    // Sender's port ID
    uint32_t nl_groups; // Multicast groups mask
    uint32_t length;    // Message data length (excluding header)
};

// Netlink socket structure
struct netlink_sock {
    int domain;
    int type;
    int protocol;
    uint32_t portid;
    uint32_t groups;
    bool passcred;
    vfs_node_t *node;
    struct sockaddr_nl *bind_addr;
    struct netlink_buffer *buffer; // skb-backed receive queue
    struct sock_fprog *filter;
    spinlock_t lock;
};

// Netlink socket buffer management
struct netlink_buffer {
    skb_queue_t queue;
    size_t used_bytes;
    size_t size;
    size_t max_size;
    spinlock_t lock;
};

struct nla_policy {
    uint16_t type;
    uint16_t len;
};

// Netlink socket operations
extern socket_op_t netlink_ops;

int netlink_socket(int domain, int type, int protocol);
int netlink_socket_pair(int type, int protocol, int *sv);

size_t netlink_buffer_write_packet(struct netlink_sock *sock, const char *data,
                                   size_t len, uint32_t nl_pid,
                                   uint32_t nl_groups);
void netlink_broadcast_to_group(const char *buf, size_t len,
                                uint32_t sender_pid, uint32_t target_groups,
                                int protocol, uint32_t seqnum,
                                const char *devpath);

void netlink_kernel_uevent_send(const char *buf, int len);
void netlink_publish_netdev_event(struct netdev *dev, uint32_t events);
void netlink_publish_scan_event(struct netdev *dev, bool aborted);
void netlink_publish_connect_result(struct netdev *dev, uint32_t request_portid,
                                    uint16_t status_code);
void netlink_publish_disconnect_event(struct netdev *dev,
                                      uint32_t request_portid,
                                      uint16_t reason_code, bool by_ap);

void netlink_init();
