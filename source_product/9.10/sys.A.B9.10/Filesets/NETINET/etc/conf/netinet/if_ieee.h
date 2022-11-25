/* $Header: if_ieee.h,v 1.5.83.4 93/09/17 19:02:19 kcs Exp $ */

#ifndef	_SYS_IF_IEEE_INCLUDED
#define	_SYS_IF_IEEE_INCLUDED
/*
 * header field size constants; needed because of non-uniform padding of
 * data structures by various C compilers (we need exactly 17 bytes for
 * the size of the ieee8023_hdr and 24 for ieee8023xsap_hdr; sizeof() can
 * return 18 and 26, respectively).
 */
#define IEEE8023_HLEN        17     /* header length for ieee8023 */
#define MIN_IEEE8022_HLEN     3     /* minimum length for 802.2 header     */
#define SNAP_802_2_HLEN       8     /* minimum length for 802.2 snap header*/
#define IEEE_RAW8023_HLEN    14     /* header length for raw ieee8023 */
#define IEEE_8025_MAC_HLEN   14     /* header length for raw mac ieee8025 */
#define IEEE8023XSAP_HLEN    24     /* header length for extended ieee8023 */
#define IEEE8023XSAP_LEN     10   /* this+data=length(sum of fields after len)*/
#define SNAP8023_HLEN        22     /* header length for snap8023 */
#define SNAP8023XT_HLEN      26     /* header length for extended snap8023 */
#define IEEE8024_HLEN        16     /* header length for ieee8024 */
#define IEEE8024XSAP_HLEN    23     /* header length for extended ieee8024 */
#define SNAP8024_HLEN        21     /* header length for snap8024 */
#define SNAP8024XT_HLEN      25     /* header length for extended snap8024 */

/*
 * The following are 802.5 header size constants.  Note that these values
 * are for both full source routing (sr) and non-source route fields.        
 * Note representation is for Type I only - Type II make CTL(2) 
 *
 * SNAP8025    AC(1)+FC(1)+DA(6)+SA(6)+DSAP(1)+SSAP(1)+CTL(1)+ORDID(3)+ETYPE(2)
 * SNAP8025+SR AC(1)+FC(1)+DA(6)+SA(6)+RIF(18)+DSAP(1)+SSAP(1)+CTL(1)+ORDID(3)
 *              +ETYPE(2)
 * IEEE8025    AC(1)+FC(1)+DA(6)+SA(6)+DSAP(1)+SSAP(1)+CTL(1)
 * IEEE8025+SR AC(1)+FC(1)+DA(6)+SA(6)+RIF(18)+DSAP(1)+SSAP(1)+CTL(1)
 */
#define SNAP8025_HLEN        22     /* header length for snap8025 */
#define SNAP8025_SR_HLEN     40     /* header length for snap8025 + sr */
#define IEEE8025_HLEN        17     /* header length for ieee8025 */
#define IEEE8025_SR_HLEN     35     /* header length for ieee8025 + sr */
#define IEEE8025_RII         0x80   /* rif bit for IBM source routing */
#define IEEE8025_RIF_DIR     0x80   /* rif direction bit for IBM srouting */

struct ieee8023_hdr {                   /* 802.3 packet header */
        u_char  destaddr[6];            /* 802.3 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.3 Source Address (48 bits) */
        u_short length;                 /* Packet length in bytes */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
};

struct ieee8023xsap_hdr {               /* 802.3 extended SAPs header */
        u_char  destaddr[6];            /* 802.3 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.3 Source Address (48 bits) */
        u_short length;                 /* Packet length in bytes */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_short dxsap;                  /* extended dsap */
        u_short sxsap;                  /* extended ssap */
};

struct snap8023_hdr {                   /* SNAP header */
        u_char  destaddr[6];            /* 802.3 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.3 Source Address (48 bits) */
        u_short length;                 /* Packet length in bytes */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_short type;                   /* Type value */
};

struct snap8023xt_hdr {                 /* SNAP extended SAP header */
        u_char  destaddr[6];            /* 802.3 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.3 Source Address (48 bits) */
        u_short length;                 /* Packet length in bytes */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_short type;                   /* Value is always ETHERTYPE_HP */
        u_short dcanon_addr;            /* destination canonical address */
        u_short scanon_addr;            /* source canonical address */
};

struct ieee8024_hdr {                   /* 802.4 packet header */
	u_char	frame_ctrl;		/* frame control field */
        u_char  destaddr[6];            /* 802.4 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.4 Source Address (48 bits) */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
};

struct ieee8024xsap_hdr {               /* 802.4 extended SAPs header */
	u_char	frame_ctrl;		/* frame control field */
        u_char  destaddr[6];            /* 802.4 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.4 Source Address (48 bits) */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_char  dxsap[2];               /* extended dsap */
        u_char  sxsap[2];               /* extended ssap */
};

struct snap8024_hdr {                   /* SNAP header */
	u_char	frame_ctrl;		/* frame control field */
        u_char  destaddr[6];            /* 802.4 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.4 Source Address (48 bits) */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_char  type[2];                /* Type value */
};

struct snap8024xt_hdr {                 /* SNAP extended SAP header */
	u_char	frame_ctrl;		/* frame control field */
        u_char  destaddr[6];            /* 802.4 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.4 Source Address (48 bits) */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  hdr_fill[3];            /* Padding for alignment */
        u_char  type[2];                /* Value is always ETHERTYPE_HP */
        u_char  dcanon_addr[2];         /* destination canonical address */
        u_char  scanon_addr[2];         /* source canonical address */
};

struct snapfddi_hdr_info {		/* SNAP header over FF FDDI */
	u_char  destaddr[6];            /* destination address (48 bits) */
	u_short type;                   /* ether type iff manf == 0 */
};

struct ieee8025_sr_hdr {                /* 802.5 packet header */
        u_char  access_ctl;             /* access control field */
        u_char  frame_ctl;              /* frame control field */
        u_char  destaddr[6];            /* 802.5 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.5 Source Address (48 bits) */
        u_char  rif[18];                /* 802.5 Source Routing info */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
};

struct snap8025_sr_hdr {                /* SNAP header */
        u_char  access_ctl;             /* access control field */
        u_char  frame_ctl;              /* frame control field */
        u_char  destaddr[6];            /* 802.5 Destination Address(48 bits) */
        u_char  sourceaddr[6];          /* 802.5 Source Address (48 bits) */
        u_char  rif[18];                /* 802.5 Source Routing info */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  orgid[3];               /* org id */
        u_short type;                   /* Type value */
};

struct snap8022_hdr {                   /* SNAP header */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
        u_char  orgid[3];               /* org id */
        u_short type;                   /* Type value */
};

struct ieee8022_hdr {                   /* IEEE 8022 header */
        u_char  dsap;                   /* dsap; canonical address */
        u_char  ssap;                   /* ssap; canonical address */
        u_char  ctrl;                   /* ctrl */
};



#define SNAP8023_MTU	1492  /* max snap8023 packet size (without header) */
#define IEEE8023_MTU	1497  /* max ieee8023 packet size (without header) */
#define IEEE8023_MIN	43    /* minimum packet size (without header) */
#define SNAP8024_MTU	8166  /* max snap8024 packet size (without header) */
#define IEEE8024_MTU	8171  /* max ieee8024 packet size (without header) */

#define SNAP8025_4_MTU   4170  /* max snap8025 pckt size 4M (no hdr) 4k + 64*/
#define IEEE8025_4_MTU   4170  /* max ieee8025 pckt size 4M (no hdr) 4k + 64*/
#define SNAP8025_16_MTU  4170  /* max snap8025 pckt size 16M (no hdr) 4k + 64*/
#define IEEE8025_16_MTU  4170  /* max ieee8025 pckt size 16M (no hdr) 4k + 64*/

#define IEEE8024_FC	0x62  /* default frame control field for 802.4 */

#define IEEE8025_AC	0x00  /* default access control field for 802.5 */
#define IEEE8025_FC	0x40  /* default frame control field for 802.5 */

/* IEEE 802.2 header ctrl field defines */
#define	IEEECTRL_DEF 	0x03		/* UI format */
#define	IEEECTRL_XID	0xBF		/* Exchange Identification */
#define IEEECTRL_TEST	0xF3		/* Test Packet */

/*
 * defines for current well-known saps and Canonical Addresses 
 */
#define MIN_ETHER_TYPE	0x600		/* Value below this is assumed to */
                                        /* be length field of IEEE 802 pkt.*/
#define	IEEESAP_IP	0x06		/* IP SAP */
#define IEEESAP_SNAP	0xAA		/* SNAP SAP */
#define	IEEESAP_OSI	0xFE		/* OSI SAP */
#define IEEESAP_HP	0xFC		/* HP DSN Address Expansion SAP */
#define IEEESAP_NM	0xF8		/* HP Network Management SAP */
#define IEEEXSAP_PROBE	0x0503		/* canoniccal XSAP for Probe */
#define IEEEXSAP_DUX	0x164f          /* canonical XSAP for DUX */

#endif	/* _SYS_IF_IEEE_INCLUDED */
