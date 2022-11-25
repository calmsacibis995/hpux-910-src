/*
 * @(#)if_atr.h: $Revision: 1.3.83.4 $ $Date: 93/09/17 19:01:54 $
 * $Locker:  $
 */

/* $Header: if_atr.h,v 1.3.83.4 93/09/17 19:01:54 kcs Exp $ */

/*
 * On Apollo Token Ring (ATR), ARPA packets are encapsulated within Domain
 * Distributed Services (DDS) packets.  Other protocols are encapsulated
 * within an ATR frame which includes both a hardware and a control header.
 * On the target machine, ARPA packets are distinguished from other DDS
 * traffic through the use of the destination * socket.
 *
 */

/*
 * The packet type fields in the frame and control headers are made up of the
 * following bit combinations.
 */
#define PKTTYPF_NON_AEGIS       0x01
#define PKTTYPF_SW_DIAG         0x02
#define PKTTYPF_USER            0x04
#define PKTTYPF_PAGING          0x08
#define PKTTYPF_PLEASE          0x10
#define PKTTYPF_THANK_YOU       0x20
#define PKTTYPF_HW_DIAG         0x40
#define PKTTYPF_BRDCST          0x80

/*
 * Commonly used flag combinations in the packet type fields.
 */
#define ATRF_BCAST              (PKTTYPF_BRDCST)
#define ATRF_USR                (PKTTYPF_NON_AEGIS)
#define ATRF_ARPA               (PKTTYPF_USER|PKTTYPF_PLEASE)
#define ATRF_SW_DIAG            (PKTTYPF_SW_DIAG|PKTTYPF_PLEASE)

/*
 * Early ACK bits included in the hardware header.
 */
#define PKTEAKF_ACKPAR          0x02
#define PKTEAKF_LSTN            0x08

/*
 * Apollo IDP header versions.
 */
#define PKT_OLD_VERSION         1
#define PKT_INTERIM_VERSION     2
#define PKT_INTERNET_VERSION    3

/*
 * Apollo protocol types.
 */
#define PKT_IDP_PROTOCOL        1
#define PKT_PEP_PROTOCOL        2
#define PKT_IDP_PEP_TYPE        4
#define PKT_IDP_APOLLO_TYPE     0xbe
#define PKT_PEP_APOLLO_TYPE     0x8031
#define PKT_PEP_HPIPC_TYPE      41

/*
 * Node IDs pre-defined by the OS.
 */
#define PKT_WHO_NODE_ID         2
#define PKT_FAILURE_NODE_ID     1
#define PKT_FAILURE_NODE_ID2    3
#define PKT_FAILURE_NODE_ID3    4
#define PKT_NODE_ID_MAX         ((u_long)0x000fffff)

/*
 * Apollo socket types used in the DDS header.  Note that the TCP/IP socket
 * is the same as user_sock2.  Also note that sockets in the range PKT_SOCK_USERMIN
 * to PKT_SOCK_USERMAX are reserved for client application use.
 */
#define PKT_BIT_BUCKET_SOCK             0xffff
#define PKT_PAGE_SOCK                   1
#define PKT_FILE_SOCK                   2
#define PKT_NETMAN_SOCK                 3
#define PKT_INFO_SOCK                   4
#define PKT_WHO_SOCK                    5
#define PKT_FILE_OVER_SOCK              6
#define PKT_SW_DIAG_SOCK                7
#define PKT_RIP_SOCK                    8
#define PKT_MBX_SOCK                    9
#define PKT_NAME_SOCK                   10
#define PKT_USER_SOCK1                  10
#define PKT_TCPIP_SOCK                  11
#define PKT_USER_SOCK2                  11
#define PKT_LB_SOCK                     12
#define PKT_PING_SOCK                   13
#define PKT_KDBG_SOCK                   14
#define PKT_DPCI_SOCK                   15
#define PKT_SOCK_USERMIN                16
#define PKT_SOCK_USERMAX                32

/*
 * The ATR frame includes a header with a header data area and a data
 * area.  The following constants are used in determining if a separate
 * data area is needed in an outgoing frame.
 */
#define ATR_HDRDATA_MAX                 512
#define ATR_DATA_MAX                    1024
#define ATR_HDR_MAGIC                   6

/*
 * Apollo Token Ring frame header and Domain Distributed Services packet
 * headers.  Note that many of these structures are word-aligned.
 */

/*
 * Frame and control headers.  The control header is shared with DDS
 * on other network types such as Ethernet.
 */
typedef struct pkt_hdwr_hdr {
        u_long          to_id;          /* Destination node ID */
        u_char          pkttyp;         /* Includes the PKTTYPF_ flags */
        u_char          clk_low1;       /* Used in record-keeping */
        u_char          clk_low0;       /* Field added to maintain alignment */
        u_char          early_ack;      /* Part of the ATR protocol */
        u_long          from_id;        /* Source node ID */
} pkt_hdwr_hdr_t;                       /* ATR frame header */
#define clock_low       clk_low1        /* clk_low has been broken in two for alignment */

typedef struct pkt_cntl_hdr {
        u_char          version;        /* Old, interim, or internet (unimplemented) */
        u_char          chksum;         /* Packet checksum */
        u_char          pkttyp;         /* Includes the PKTTYPF_ flags */
        u_char          qdepth;         /* Number of buffers available */
        u_short hdr_lnth;       /* Length of header */
        u_short hdr_data_lnth;  /* Length of data with header */
        u_short data_lnth;      /* Separate data length */
        u_short trans_id;       /* Client or OS transaction ID */
} pkt_cntl_hdr_t;                       /* Packet control header */

/*
 * Old version DDS header which cannot be routed.  Still used by boot
 * services on the PROM.
 */
typedef struct pkt_old_route {
        u_short route[2];       /* Destination */
} pkt_old_route_t;                      /* Destination info */

typedef struct pkt_old_addr {
        u_char          lnth;           /* Length */
        u_char          curr;           /* Element of old_route.route to use */
        u_short src_sock;       /* Source socket */
        u_short src_node;       /* Source node */
        pkt_old_route_t old_route;      /* Destination */
} pkt_old_addr_t;                       /* Old-style packet address */

typedef struct pkt_old_dds_hdr {
        pkt_hdwr_hdr_t  hdwr;
        pkt_cntl_hdr_t  cntl;
        pkt_old_addr_t  adr;
} pkt_old_dds_hdr_t;                    /* Old-style ATR and DDS header for IP */

/*
 * Interim version DDS header.  The style currently in most frequent use.
 * Note the use of the word_alignment pragma.  ATR headers tend to be
 * word-aligned, not longword-aligned.
 */

#ifdef __hp9000s800
#pragma HP_ALIGN DOMAIN_WORD
#endif /* __hp9000s800  */

typedef struct pkt_new_route {
        u_short org_sock;       /* Source socket */
        u_long          org_node;       /* Source node ID */
        u_short route[2];       /* Socket routing information */
} pkt_new_route_t;                      /* Packet routing info */

typedef struct pkt_addr {
        u_char          lnth;           /* Length */
        u_char          curr;           /* Element of new_route to use */
        u_short src_sock;       /* Source socket */
        u_short src_node;       /* Source node */
        pkt_new_route_t new_route;      /* Destination and routing information */
} pkt_addr_t;                           /* Packet address */

typedef struct pkt_idp_host_addr {
        u_short high16;         /* Unused */
        u_long          host_id;        /* IDP entity */
} pkt_idp_host_addr_t;                  /* IDP host address */

typedef struct pkt_idp_name {
        union {
            u_long              network;/* IDP network number */
            struct {
                u_short         net_hi;
                u_short         net_lo;
            } net;
        } n_u;
        pkt_idp_host_addr_t     host;   /* IDP address */
        u_short         socket; /* IDP socket number */
} pkt_idp_name_t;                       /* IDP name */

typedef struct pkt_idp_header {
        u_short checksum;       /* IDP checksum */
        u_short length;         /* IDP length */
        u_char          transport_control;
        u_char          packet_type;    /* IDP packet type */
        pkt_idp_name_t  dest_name;      /* Destination */
        pkt_idp_name_t  src_name;       /* Source */
} pkt_idp_header_t;                     /* IDP packet header */

#define PKT_IDP_NOCHECKSUM      (0xffff)/* No IDP checksum used */

typedef struct pkt_dds_hdr {
        pkt_hdwr_hdr_t          hdwr;
        pkt_cntl_hdr_t          cntl;
        pkt_addr_t              adr;
        pkt_idp_header_t        idp;
} pkt_dds_hdr_t;                        /* Complete ATR and DDS header */

#ifdef __hp9000s800
#pragma HP_ALIGN POP
#endif /* __hp9000s800  */

/*
 * Headers used by some upper layer services.
 */

#ifdef __hp9000s800
#pragma HP_ALIGN DOMAIN_WORD
#endif /* __hp9000s800  */

typedef struct pkt_pep_hdr {
        u_long          transaction_id;
        u_short packet_type;
} pkt_pep_hdr_t;                        /* PEP packet header */


typedef struct pkt_uid {
        u_long          high;
        u_long          low;
} pkt_uid_t;                            /* IPC UID definition */

typedef struct pkt_ipc_hdr {
        pkt_uid_t       to_uid;
        pkt_uid_t       from_uid;
} pkt_ipc_hdr_t;                        /* IPC packet header */

/*
 * The header used by clients other than Domain Distributed Services.
 */
typedef u_long pkt_usr_type_t;          /* De-multiplexing type in non-DDS packets */

typedef struct pkt_usr_hdr {
        pkt_hdwr_hdr_t  hdwr;
        pkt_cntl_hdr_t  cntl;
        pkt_usr_type_t  type;
} pkt_usr_hdr_t;                        /* ATR header for non-DDS users */

/*
 * The generic Apollo Token Ring header.
 */
typedef struct atr_header {
        pkt_hdwr_hdr_t          hdwr;
        pkt_cntl_hdr_t          cntl;
        pkt_addr_t              adr;
        pkt_idp_header_t        idp;
        pkt_pep_hdr_t           pep;
        pkt_ipc_hdr_t           ipc;
} atr_header_t;                         /* Common portion of ATR header */

#ifdef __hp9000s800
#pragma HP_ALIGN POP
#endif /* __hp9000s800  */

/*
 * TCP/IP definitions for Apollo Token Ring networks.
 */
typedef u_long dr_addr_t;               /* An Apollo DDS network ID or node ID. */

/*
 * IP or ARP header pre-pended to IP packet for de-multiplexing packets destined
 * for the TCP/IP socket at the target machine.  Matches that defined for Domain OS
 * TCP/IP.
 */
typedef struct dr_header {
        u_short dr_length;      /* Packet length + sizeof(dr_header_t) */
        u_short dr_type;        /* Packet type, i.e. IP or ARP */
        dr_addr_t       dr_dest;        /* Destination node ID */
        dr_addr_t       dr_src;         /* Source node ID */
} dr_header_t;

/*
 * Domain OS TCP/IP packet types.
 */
#define DR_IPTYPE       1               /* Protocol type for IP on ATR */
#define DR_ARPTYPE      2               /* Protocol type for ARP on ATR */
#define DR_DDSTYPE      3               /* DDS -- does not exist in Domain OS TCP/IP */

#define ATRTYPE_PUP     ARPTYPE_PUP     /* PUP protocol */
#define ATRTYPE_IP      ARPTYPE_IP      /* IP protocol */
#define ATRTYPE_ARP     ARPTYPE_ARP     /* Addr. resolution protocol */

#define ATRMTU  (1280-sizeof(dr_header_t))      /* IP MTU interoperable with Domain OS IP */
#define ATRMAX  4096                            /* ATR MTU between DN10000s -- not used by IP */
#define ATRMIN  0                               /* No minimum packet size */

/*
 * Definitions for Apollo Token Ring without a counterpart in the header
 * files for other network types.
 */
#define ATRBROADCAST    (0L)            /* ATR broadcast address */
#define IFT_ATR         (IFT_OTHER)     /* XXX.  No official type has as yet been designated */
#define ARPHRD_ATR      (2)             /* XXX.  No official type has as yet been designated */

/*
 * Apollo Token Ring Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 */
struct  atr_arp {
        struct          arphdr ea_hdr;  /* fixed-size header */
        dr_addr_t       arp_sha;        /* sender hardware address */
        struct in_addr  arp_spa;        /* sender protocol address */
        dr_addr_t       arp_tha;        /* target hardware address */
        struct in_addr  arp_tpa;        /* target protocol address */
};

/* #ifdef       not_yet */
/*
 * Definitions for the data area in a DDS packet.
 */

#ifdef __hp9000s800
#pragma HP_ALIGN DOMAIN_WORD
#endif /* __hp9000s800  */

typedef short volx_t;
typedef short proc1_id_t;
typedef long thread_t;                  /* XXX remove or redefine */
typedef unsigned int clockh_t;          /* high 32 bits of clock */

typedef union aegis_clock {
    struct {
        union {
            clockh_t    high;
            struct {
                u_short hi_hi;
                u_short hi_lo;
            } hi;
        } h_u;
        u_short    low;
    } c1;
    struct {
        u_short    high16;
        union {
            u_long      low32;
            struct {
                u_short lo_hi;
                u_short lo_lo;
            } lo;
        } l_u;
    } c2;
} aegis_clock_t;

#define NETWORK_HDR_SIZE                512

typedef enum {                  /* kinds of network hdwr */
        network_ring,                   /* Domain ring */
        network_nil,                    /* Dummy network (no hardware) */
        network_user,                   /* User-space driver interface */
        network_t1,                     /* IIC hardware */
        network_ether,                  /* Ethernet driver */
        network_ring_8025,              /* IEEE 802.5 Token Ring */
        network_chat                    /* Channel Controller-AT (CHAT) */
} network_hdwr;

#define ASKNODE_VERSION                 3
#define ASKNODE_VERS_NET_PORT_WITH_UID  4
#define ASKNODE_WHO_MAX                 2000
#define ASKNODE_REM_WHO_MAX             100
#define ASKNODE_WHO_PATTERN             0xb1ff
#define ASKNODE_INFOMAX                 (NETWORK_HDR_SIZE-8)
#define ASKNODE_UIDLIST_MAX             57
#define ASKNODE_UIDLIST2_MAX            ((ASKNODE_INFOMAX-8)/sizeof(pkt_uid_t))
#define ASKNODE_OLD_VOLX_PMAX           8
#define ASKNODE_THREADLIST_MAX          ((ASKNODE_INFOMAX-8)/sizeof(thread_t))

#define NETWORK_BAD_RQST                0x0011000D  /* status returned for unsupported asknode requests */

/* pascal-ish booleans needed for Aegis compatibility */
#define false   ((u_char) 0x00)
#define true    ((u_char) 0xFF)

/* asknode kinds */
#define ask_who                 0
#define who_r                   1
#define ask_time                2
#define time_r                  3
#define ask_node_root           4
#define node_root_r             5
#define ask_network_stats       6
#define network_stats_r         7
#define ask_cal                 8
#define cal_r                   9
#define ask_volx                10
#define volx_r                  11
#define ask_diskless            12
#define diskless_r              13
#define ask_failure_report      14
#define failure_report_r        15
#define ask_sm                  16
#define sm_r                    17
#define ask_proc2list           18
#define proc2list_sr            19
#define ask_proc2info           20
#define proc2info_sr            21
#define ask_proc2fault_pgrp     22
#define proc2fault_pgrp_sr      23
#define ask_net_root            24
#define net_root_r              25
#define ask_bldt                26
#define bldt_r                  27
#define ask_sysuids             28
#define sysuids_r               29
#define ask_clear_net           30
#define ask_ring_info           31
#define ask_ring_info_r         32
#define ask_proc1info           33
#define proc1info_sr            34
#define ask_upids               35
#define upids_sr                36
#define ask_log                 37
#define log_r                   38
#define ask_config              39
#define config_r                40
#define ask_perf_info           41
#define perf_info_r             42
#define ask_proc2pid            43
#define pid_r                   44
#define ask_rem_who             45
#define rem_who_r               46
#define ask_fail_msg            47
#define fail_msg_r              48
#define ask_logfile             49
#define logfile_r               50
#define ask_proc1list           51
#define proc1list_sr            52
#define ask_proc2fault_opt      53
#define proc2fault_opt_sr       54
#define ask_network_number      55
#define network_number_r        56
#define ask_net_ports           57
#define net_ports_r             58
#define ask_port_desc           59
#define port_desc_r             60
#define ask_port_stat           61
#define port_stat_r             62
#define ask_route_stat          63
#define route_stat_r            64
#define ask_rip                 65
#define rip_r                   66
#define ask_route_bklog         67
#define route_bklog_r           68
#define ask_who_notopo          69
#define who_notopo_r            70
#define ask_boot_device         71
#define boot_device_r           72
#define ask_load_avg            73
#define load_avg_r              74
#define ask_va_info             75
#define va_info_r               76
#define ask_os_rev_info         77
#define os_rev_info_r           78
#define ask_zombie_list         79
#define zombie_list_sr          80
#define ask_disk_stats          81
#define disk_stats_r            82
#define ask_logfile2            83
#define logfile2_r              84
#define ask_disp_list           85
#define disp_list_r             86
#define ask_proc2list2          87
#define proc2list2_sr           88
#define ask_zombie_list2        89
#define zombie_list2_sr         90
#define ask_port_stat2          91
#define port_stat2_r            92
#define ask_vm_info             93
#define vm_info_r               94
#define ask_volinfo             95
#define volinfo_r               96
#define ask_delay_stats         97
#define delay_stats_t           98
#define ask_threadlist          99
#define threadlist_r            100
#define ask_threadinfo          101
#define threadinfo_r            102

typedef struct {
        short           ring_cmd;       /* ringlog command */
        short           netl_cmd;       /* netlog command */
        u_long          node;           /* to node for netlog */
        short           sock;           /* to sock for netlog */
        long            kind;           /* kinds to log */
} ask_log_t;

typedef struct {
        u_long          start_node;     /* node initiating rem_who */
        u_long          rem_node;       /* where to send reply */
        u_long          rem_net;
        short           first;          /* true only for first node rem_who was sent to */
        short           die_cnt;
        long            prop_delay;     /* units of 4 usec */
} ask_rem_who_t;

typedef struct {
        network_hdwr    dev_type;
        long            dev_num;
} ask_net_port_t;

#define ask_p2_fault            0       /* Options values for ask_proc2fault_opt */
#define ask_p2_enq              1
#define ask_p2_pgroup           2
#define ask_p2_pgroup_enq       3

typedef struct asknode_rqst {
        short                   version;
        short                   kind;
        union {
                struct {
                        u_long  node;           /* for asknode_$who request */
                        short   diecnt;
                } ar1;
                volx_t          volx;           /* for asknode_$info volx request */
                short           unit;           /* for asknode_$info sm request */
                struct {
                        pkt_uid_t procuid;      /* for asknode_$info proc2 request */
                        long    procdat;
                        long    procopt;        /* this field only for proc2fault_opt */
                } ar2;
                long            fail_type;      /* for ask_failure_report */
                proc1_id_t      pid;            /* for asknode_$info proc1 request */
                ask_log_t       ask_log_stuff;  /* for asknode_$log request */
                long            lword;          /* just for copying for rmt request */
                                                /* also limit on ask_rem_who node list */
                ask_rem_who_t   rem_who_stuff;  /* for ask_rem_who requests */
                struct {
                        u_short logfile_maxlen;
                        u_short logfile2_start;
                } ar3;
                ask_net_port_t  port_id;        /* ask_port_desc, ask_port_stat */
                struct {
                        u_short ctlr_num;
                        u_short disk_unit;
                } ar4;
                long            next_rqst;      /* ask_proc2list2, ask_zombie_list2: where to start */
                struct {
                        long    start_point;
                        u_short upid;
                } ar5;
                thread_t        thread_id;      /* ask_threadinfo : thread_id  */
        } ar_u;
} asknode_rqst_t;

typedef union asknode_rqst_data {
        struct {
                u_long  node;                   /* for asknode_$who request */
                short   diecnt;
        } ar1;
        volx_t          volx;                   /* for asknode_$info volx request */
        short           unit;                   /* for asknode_$info sm request */
        struct {
                pkt_uid_t procuid;              /* for asknode_$info proc2 request */
                long    procdat;
                long    procopt;                /* this field only for proc2fault_opt */
        } ar2;
        long            fail_type;              /* for ask_failure_report */
        proc1_id_t      pid;                    /* for asknode_$info proc1 request */
        ask_log_t       ask_log_stuff;          /* for asknode_$log request */
        long            lword;                  /* just for copying for rmt request */
                                                /* also limit on ask_rem_who node list */
        ask_rem_who_t   rem_who_stuff;          /* for ask_rem_who requests */
        struct {
                u_short logfile_maxlen; /* for ask_logfile request */
                u_short logfile2_start;
        } ar3;
        ask_net_port_t  port_id;                /* ask_port_desc, ask_port_stat */
        struct {
                u_short ctlr_num;
                u_short disk_unit;
        } ar4;
        u_long          next_rqst;              /* ask_proc2list2, ask_zombie_list2: where to start */
        struct {
                long    start_point;            /* for ask_threadlist */
                u_short upid;
        } ar5;
        thread_t        thread_id;              /* ask_threadinfo : thread_id  */
} asknode_rqst_data_t;

#define ASK_RTSTAT_VERSION      3               /* Version ID for ask_route_stat_t */

typedef struct {                        /* Routing stats */
        short           version;
        short           routing;
        u_short         std_routing;
        clockh_t        start_time;
        short           q_slots;
        short           q_oflo;
        long            misroute;
        long            too_far;
        long            pkts_routed;
        long            dlen_err;
        long            std_misroute;           /* stats for std XNS routing */
        long            std_too_far;
        long            std_pkts_routed;
        long            std_dlen_err;
} ask_route_stat_t;

typedef struct {                        /* Per-port routing stats */
        short           version;
        clockh_t        port_open;
        long            rcv_cnt;
        long            send_cnt;
        short           devstat_len;            /* Number of bytes in devstat */
        short           devstat_offset;         /* addr(devstat) - addr(version) */
        pkt_uid_t       port_type_uid;          /* type UID of the network driver */
        long            dat_len;                /* Number of bytes of data allowed in a packet. */
                                                /* Can always send at least 256 bytes of header */
                                                /* data, too. */
        char            devstat[100];           /* Note: If the network port is a ring, devstat */
                                                /* looks like ring_$data_t */
} ask_port_stat_t;

typedef enum {                                  /* Class of service at a network port */
        rtport_notopen,                         /* No service. Port is not open. */
        rtport_noservice,                       /* Port is open, but does not offer any service. */
        rtport_aegis,                           /* Normal Aegis requests only. */
        rtport_route,                           /* Full routing service. */
        rtport_xnsroute,                        /* Standard XNS routing service only. */
        rtport_allroute                         /* Domain and standard XNS routing service. */
} route_service_t;

typedef struct route_port {                     /* Network port descriptor */
        u_long          net_num;                /* Network this port faces */
        route_service_t port_service;           /* Class of service at this port */
        network_hdwr    port_dev;
        union {                                 /* Type of network device */
                struct {                        /* Null network */
                        short   id_val;         /* -- has an internal identifier */
                        short   reserved[22];   /* -- space-holder for future additions */
                } r1;
                                                /* Ring network */
                short   ring_num;               /* -- ring board number */

                struct {                        /* User-written driver */
                        short   id_num;         /* -- assigned user port number */
                        u_char  sock_slots;     /* -- number of packet slots in the buffer queue */
                        u_char  xxx_pad;
                } r2;
                                                /* Interphase T1 driver */
                short   t1_num;                 /* -- Board number */

                                                /* Ethernet controller */
                short   eth_num;                /* -- Board number */
        } r_un;
} route_port_t;

typedef struct {                        /* Describes routing towards some target network. */
        long            target_net;             /* Network number of target net. */
        u_long          next_hop;               /* Next hop towards that network. */
        clockh_t        exp_time;               /* Time at which this entry 'expires' from current state. */
        short           n_hops;                 /* Number of hops to get to that net. */
        long            lkup_cnt;               /* Number of lookups that found this entry. */
        short           entry_st;               /* Describes trustworthiness of the routing entry */
        route_port_t    next_port;              /* Next network port towards the target. */
} ask_rip_t;

typedef enum {
        winchester,
        floppy,
        ring_xmit,
        ring_rcv,
        storage_module,
        ctape,
        ethernet,
        display,
        ring_8025,
        chat,
        fddi,
        cdrom
} ctype_t;

typedef struct {
        ctype_t         ctlrtype;               /* Controller type -- could be network */
        short           ctlrno;                 /* Controller number */
        short           unitno;                 /* Unit number */
        short           lvolno;                 /* Logical volume number */
} ask_boot_device_t;

typedef struct {
        long            avg1;                   /* one minute average */
        long            avg5;                   /* five minute average */
        long            avg15;                  /* fifteen minute average */
} ask_load_avg_t;

typedef struct {
        short           stype;                  /* smd_$display_type_t value */
        pkt_uid_t       stype_uid;              /* display type UID */
        short           ttl_x;                  /* Total (visible+hidden) width */
        short           ttl_y;                  /* Total (visible+hidden) height */
        short           vis_x;                  /* Visible width */
        short           vis_y;                  /* Visible height */
} ask_disp_t;

typedef char asknode_txt_t[100];

typedef enum {
        vol_bad,
        vol_log,
        vol_phys
} ask_volinfo_kind_t;

typedef struct {
        short                   vi_ctyp;
        short                   vi_cnum;
        short                   vi_unit;
        ask_volinfo_kind_t      vi_lkind;
        union {
/*              disk_cnt_t      vi_stat; */

                struct {
                        pkt_uid_t vi_ed_uid;
                        int     vi_n_free;
                        int     vi_n_blk;
                } av2;
        } av1;
} ask_volinfo_t;

typedef struct {                        /*  response to ask_delay_stats */
        long    page_Rdelays;                   /* (paging stats) Number of delayed requests */
        long    page_Rtime;                     /* Total delay time of delayed requests (clockh units) */
        long    page_Sdelays;                   /* Number of delayed services */
        long    page_Stime;                     /* Total delay time of delayed services (clockh units */
        long    file_Rdelays;                   /* (rem_file stats) */
        long    file_Rtime;
        long    file_Sdelays;
        long    file_Stime;
        long    stat_Rdelays;                   /* (asknode stats) */
        long    stat_Rtime;
        long    stat_Sdelays;
        long    stat_Stime;
} ask_delay_t;

typedef struct {
        union {
                u_long  node_l;
                struct {
                        u_short node_hi;
                        u_short node_lo;
                } node_s;
        } node;
        short   who_pattern;            /*special unlikely bit_pattern*/
        short   who_diecnt;
} ask_who_reply_t;


typedef struct asknode_reply {
        short                   version;
        short                   kind;
        u_long                  status;
#define ASKNODE_REPLY_HDRSIZE   8   /* size of 1st 3 fields */
        union {

/*              cal_timezone_rec_t cal_timezone_rec;    / * ask_cal */

                struct {
                        u_long  partner;                /* ask_diskless */
                        u_char  diskless;
                } ar1;

/*              network_stats_rec_t net_stats_rec;      / * ask_network_stats */

                pkt_uid_t       uid;                    /* ask_node_root */

/*              sm_cnt_t        sm_cnt[4];              / * ask_sm */

                struct {
                        long    btime;                  /* ask_time */
                        long    ctime;
                } ar2;

                struct {
                        pkt_uid_t ed_uid;               /* ask_volx */
                        long    n_free;
                        long    n_blk;
                } ar3;

                ask_who_reply_t ar4;            /* ask_who */

                struct {
                        short           uidlist_cnt;    /* ask_proc2list */
                        pkt_uid_t       uidlist[ASKNODE_UIDLIST_MAX];
                } ar5;

                char procinfo[ASKNODE_INFOMAX];         /* ask_proc[1/2]info */

                struct {
                        short   txt_len;                /* ask bldt */
                        asknode_txt_t txt;
                } ar6;

/*              network_psrv_ringinfo_t ring_diag_info; / * ask_ring_info */

/*              proc2_info_t proc1info;                 / * ask_proc1info */

/*              proc2_info_t proc2info;                 / * ask_proc2info */

                struct {
                        short   upid;                   /* ask_upids */
                        short   uppid;
                        short   upgid;
                } ar7;

                struct {
                        short   config_valid_cnt;       /* cnt of valid flds in config */
                                                        /*e.g. 1 says Mach_id is OK,  2 says MACH_ID & AUX_INFO are OK*/
                        short   config_mach_id;         /* 1 Machine ID -- from prom_$machine_id */
/*                      aux_info_t config_aux_info;     / * 2 aux_info -- from prom_$machine_id */
                        short   config_disp_type;       /* 3 Display type */
                        short   config_peb_kind;        /* 4 kind of peb */
                        u_short config_memory_size;     /* 5 # of pages of physical memory */
                        u_char  config_diskless;        /* 6 is this node diskless? */
                        u_long  config_os_mom;          /* 7 node ID of node booted from (me if not diskless) */
                        short   config_win0_id;         /* 8 winchester "ID" for win disk 0 */
                        long    config_revision;        /* 9 if this is a TERN, the REV#, else 0 */
                        short   config_gpu_type;        /*10 GPU type (0:absent 1:chameleon etc.)*/
                        short   config_disk_types;      /*11 disk types */
                        short   config_nets;            /*12 network types */
                        short   config_pbus;            /*13 pbu types */
                        short   config_tapes;           /*14 tape types */
                        struct {                        /*15 list of mounted wins and smds */
                                short   device_count;   /* number of valid entries in following */
                                short   controller_type[ASKNODE_OLD_VOLX_PMAX];
                                short   disk_type[ASKNODE_OLD_VOLX_PMAX];
                        } config_devices;
                        long    config_mem_size32;      /*16 same as #5, but supports larger physical address space */
                        u_short config_num_cpus;        /* 17 number of cpus in system */
                } ar8;

/*              network_perf_info_t perf_info; */

                proc1_id_t      pid;                    /* ask_proc2pid */

/*              network_failure_rec_t fail_msg;         / * ask_fail_msg */

                struct {
                        u_short logfile_len;            /* ask_logfile -- length of data */
                        short logfile_data[2];          /* start of data (up to a page) */
                } ar9;

#ifdef  not_yet
                struct {
                        short           proc1list_cnt;  /* ask_proc1list -- number of items */
                        proc1_list_t    proc1list;      /* [pid, type] list */
                } ar10;
#endif  /* not_yet */

                u_long  network;                        /* network number */

                route_port_t    port_desc;              /* ask_port_desc */

                ask_route_stat_t route_stat;            /* ask_route_stat */

                ask_port_stat_t port_stat;              /* ask_port_stat */

                ask_rip_t       rip_item;               /* ask_rip */

                struct {
                        short   thru_q_size;            /* size of thru-traffic queue */
                        long    thru_q_oflo;            /* overflows at the thru-traffic queue */
                        long    thru_q_bklog[2];        /* backlog measurements at the thru-traffic queue */
                } ar11;

                struct {
                        short   n_ports;                /* ask_net_ports */
                        ask_net_port_t port_list[2];
                } ar12;

                ask_boot_device_t bootdev;              /* ask_load_avg */

                ask_load_avg_t  loadav;                 /* ask_load_avg */

                struct {                                /* ask_va_info */
                        long    private_addr_size;
                        long    private_text_size;
                        long    previous_rss;
                        long    current_rss;
                        long    maximum_rss;
                } ar13;

/* XXX          os_rev_t        os_rev_info;            /* ask_os_rev_info -> return revision info */

#ifdef  not_yet
                struct {
                        win_cnt_t disk_stats_rec;       /* disk statistics */
                        u_char  exists;                 /* disk existence flag */
                } ar14;
#endif  /* not_yet */

                struct {
                        short   n_disp;                 /* number of displays (may be 0) */
                        short   disp_desc_len;          /* length of a disp_inf element (may change in time) */
                        ask_disp_t disp_inf[2];         /* info about each display */
                } ar15;
                struct {
                        short   uidlist_cnt2;           /* ask_proc2list2, ask_zombie_list2 */
                        u_char  uidlist_more2;
                        long    uidlist_next_rqst2;
                        pkt_uid_t uidlist2[ASKNODE_UIDLIST2_MAX];
                } ar16;
                struct {                                /* ask_vm_info */
                        long    vm_cnt;                 /* count of valid fields returned (not including this one!) */
                        long    vm_page_size;           /* page size in bytes */
                        long    vm_seg_size;            /* segment size in bytes */
                } ar17;

                ask_volinfo_t   volinfo;                /* ask_vol_info */
                ask_delay_t     delay_info;             /* ask_delay_stats */
                struct {                                /* ask_threadlist */
                        short   nthreads;
                        u_char  more;
                        long    next;
                        thread_t thread_list[ASKNODE_THREADLIST_MAX];
                } ar18;
/*              thread_info_t   thread_info;            / * ask_threadinfo */
                long            timeout;                /* used if status = network_$long_timeout */
        } ar_u;
} asknode_reply_t;

typedef u_long asknode_node_list_t[ASKNODE_WHO_MAX];
/* typedef      char asknode_tzname_t[4]; */

#ifdef __hp9000s800
#pragma HP_ALIGN POP
#endif /* __hp9000s800  */

/*
 * Just enough DDS RIP stuff to let us learn our NET ID.
 */
#ifdef __hp9000s800
#pragma HP_ALIGN DOMAIN_WORD
#endif /* __hp9000s800  */

typedef struct rip_pkt_item {
    u_long  target_net;        /* Network to route to */
    u_short hops_to_target;    /* Distance to target net */
} rip_pkt_item_t;

#define RIP_MAX_PER_PKT   90
#define RIP_INFO_REQUEST   1
#define RIP_INFO_RESPONSE  2

typedef struct rip_pkt {
    u_short        rip_op;                     /* rip_info_request or rip_info_response */
    rip_pkt_item_t  rip_data[RIP_MAX_PER_PKT];  /* ROUTING INFO */
} rip_pkt_t;
#ifdef __hp9000s800
#pragma HP_ALIGN POP
#endif /* __hp9000s800  */


#ifdef  _KERNEL
#define ac_anaddr       ac_hwaddr       /* ATR name for hardware address */
#define at_anaddr       at_hwaddr       /* as for arpcom structure */

extern u_char atrbroadcastaddr[];

#define ATRBUFLEN       18              /* Min size of buffer for output of address */

/*
 * On the Ethernet, a sockaddr structure is used to pass a complete
 * frame header between low level routines and the driver.  On the
 * ATR interface, the dr_header_t structure is used similarly.
 */
typedef struct sockaddr_atr {
        u_short         sa_family;      /* address family */
        u_char          sa_len;         /* total length */
        u_short sa_align;       /* Bring to a longword boundary */
        dr_header_t     sa_header;      /* Complete DR header */
} sockaddr_atr_t;

/*
 * Function prototypes for hp/if_atrsubr.c and hp/if_atr.c .
 */
#if     (defined(__STDC__) || defined(__GNUC__)) && !defined(_NO_PROTO)
# define        P(s) s
#else
# define P(s) ()
#endif

/* hp/if_atrsubr.c */
#ifdef OSFATR
int     atr_output P((struct ifnet *, struct mbuf *, struct sockaddr *,
                        struct rtentry *));
void    atr_input P((struct ifnet *, struct atr_header *, struct mbuf *));
char    *atr_sprintf P((char *, u_char *, u_char *));
/* if_atr.c */
int     atr_arprequest P((struct arpcom *, struct in_addr *, u_char *));
int     atr_arpresolve P((struct arpcom *, struct mbuf *, struct in_addr *,
                                u_char *, int *));
int     atr_arpinput P((struct arpcom *, struct mbuf *));
int     atr_arpoutput P((struct ifnet *, struct mbuf *, u_char *, u_short));
int     atr_arpioctl P((struct arpcom *, int, char *));
#endif

#undef  P

/*
 * Debugging information for all ATR interfaces.
 */
typedef struct atr_data {
        u_short version;
        u_short pad0;
        u_long          xmit_call;
        u_long          xmitcnt;
        u_short xmit_nack;
        u_short xmit_wack;
        u_short xmit_overrun;
        u_short xmit_ackpar;
        u_short xmit_bus;
        u_short xmit_nortn;
        u_short xmit_modem;
        u_short xmit_error;
        u_short xmit_timout;
        u_short pad1;
        u_long          rcvcnt;
        u_short rcveor;
        u_short rcvcrc;
        u_short rcvtimout;
        u_short rcvbus;
        u_short rcvxerr;
        u_short rcv_modem_err;
        u_short rcv_pkterr;
        u_short rcvovr;
        u_short rcvackparerr;
        u_short rcvhdr_chksum;
        u_char          broken;
        u_char          pad2;
        u_char          delay_in;
        u_char          pad3;
        u_char          forced_last_time;
        u_char          pad4;
        u_char          forcing_now;
        u_char          pad5;
} atr_data_t;

typedef struct atr_swdiag_data {
        u_short version;
        u_short pad0;
        u_long          rcvcnt;
        u_short rcveor;
        u_short rcvcrc;
        u_short rcvtimout;
        u_short rcvbus;
        u_short rcvxerr;
        u_short rcv_modem_err;
        u_short rcv_pkterr;
        u_short rcvovr;
        u_short rcvackparerr;
        u_short rcvhdr_chksum;
        u_long          from_node;
} atr_swdiag_data_t;

typedef struct atr_stat_blk {
        u_char          tmask;                  /* tmask for the 2 boards */
        atr_data_t      data[2];                /* Data  for the 2 boards */
        atr_swdiag_data_t swdiag_data;          /* SW diagnostic stats */
        u_long          swdiag_rcvcnt;          /* SW diagnostic total receives */
        u_long          swdiag_goodrcv_cnt;     /* SW diagnostic good receives */
        u_long          swdiag_nodeid;          /* SW diagnostic node ID */
        u_short paging_overflow;        /* Counts overflows at paging server socket */
        u_short xmit_esb;               /* Counts esb errors on transmit */
        u_short xmit_biphase;           /* Counts biphase errors on transmit */
        u_short rcv_esb;                /* Counts esb errors on receive */
        u_short rcv_biphase;            /* Counts biphase errors on receive */
        u_short xmit_waited;            /* Counts times busy wait loop in ring_$sendp expired  */
        u_short clobbered_hdr;          /* Counts times header was clobbered on transmit */
        u_short send_null_cnt;          /* Counts times ring_$send_null was called */
        u_long          rcv_int_cnt;            /* Number of times ring rcv interrupt happened */
        u_short busy_on_rcv_int;        /* Number of times hardware was busy on interrupt */
        u_short abort_cnt;              /* Number of pkts thrown away */
        u_short wakeup_cnt;             /* Counts times slow path used on network rcv */
        u_short bad_data_cnt;           /* Counts incorrect header/data lengths on rcv */
} atr_stat_blk_t;
#endif  /* _KERNEL */

/* size of the header that's always supposed to be there */
#define BASICHDRSIZE    (sizeof(pkt_hdwr_hdr_t) + \
                         sizeof(pkt_cntl_hdr_t) + \
                         sizeof(pkt_addr_t)     + \
                         sizeof(pkt_idp_header_t))
