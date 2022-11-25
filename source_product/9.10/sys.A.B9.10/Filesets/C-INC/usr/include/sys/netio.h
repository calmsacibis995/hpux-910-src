/* $Header: netio.h,v 1.18.83.3 93/09/17 19:47:13 kcs Exp $ */

#ifndef _SYS_NETIO_INCLUDED
#define _SYS_NETIO_INCLUDED

/* netio.h: Constants and structures used for network ioctl() calls */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
     extern char *net_aton(char *, char *, int);
     extern char *net_ntoa(char *, char *, int);
#else /* not _PROTOTYPES */
     extern char *net_aton();
     extern char *net_ntoa();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Structures and constants for NETCTRL and NETSTAT ioctl() 'arg' parameter */
    
struct fis {
	int reqtype;
	int vtype;
	union {
		float  f;
		int    i;
	        unsigned char s[100];
	} value;
};

/*  Possible arguments for ioctl() 'request' parameter */

#define NETCTRL     _IOW ('N',1,struct fis)       /* control requests    */
#define NETSTAT     _IOWR('N',2,struct fis)       /* statistics requests */

#define FLOATTYPE    -1         /* 'vtype' values */
#define INTEGERTYPE  -2

/* a non-negative value for vtype indicates 's' contains a string of */
/* length 'vtype';  vtype does not account for the trailing null in  */
/* 'C' language strings                                              */


/* NETCTRL-only (write-only) values for fis.reqtype  */
#ifdef __hp9000s300
#define RESET_INTERFACE		    0	 /* no parameters are needed	 */
#define ADD_MULTICAST		    2	 /* 's' contains the address	 */
#define DELETE_MULTICAST	    3	 /* 's' contains the address	 */
#define ENABLE_BROADCAST	   13	 /* receive broadcast packets	 */
#define DISABLE_BROADCAST	   14	 /* do not receive broadcast	 */
#define LOG_TYPE_FIELD		   51	 /* 'i' contains 'type' field	 */
#define LOG_DEST_ADDR		   52	 /* 's' contains dest. address	 */
#define LOG_READ_TIMEOUT	   53	 /* 'i' contains milliseconds	 */
#define LOG_READ_CACHE		   54	 /* 'i' contains packet count	 */
#define LOG_DSAP		   55	 /* 'i' contains the dsap value	 */
#define LOG_SSAP		   56	 /* 'i' contains the ssap value	 */
#define LOG_SNAP_TYPE             803    /* 's' contains the snap value  */
#define LOG_CONTROL		   57	 /* 'i' contains control value	 */
#define ENABLE_PROMISCUOUS         63    /* not supported on Series 300  */
#define DISABLE_PROMISCUOUS        64    /* not supported on Series 300  */
#define CHANGE_NODE_RAM_ADDR	   66 	 /* change station RAM address	 */
#define LOG_RAW_802MAC             67    /* no parameters are needed     */
#define LOG_RIF_ADDR               68    /* 's' contains 802.5 RI value  */
#define SET_TRN_FUNC_ADDR_MASK     69    /* 's' contains 802.5 FA mask   */
#define CLEAR_TRN_FUNC_ADDR_MASK   70    /* 's' contains 802.5 FA mask   */
#define RESET_STATISTICS	  100	 /* no parameters are needed	 */
#endif /* __hp9000s300 */

#ifdef __hp9000s800 
#define DO_EXTERNAL_LOOPBACK       49    /* no parameters are needed     */
#define RESET_STATISTICS           50    /* no parameters are needed     */
#define RESET_INTERFACE            51    /* no parameters are needed     */
#define ADD_MULTICAST              52    /* 's' contains the address     */
#define DELETE_MULTICAST           53    /* 's' contains the address     */
#define ENABLE_BROADCAST           54    /* receive broadcast packets    */
#define DISABLE_BROADCAST          55    /* do not receive broadcast     */
#define LOG_DSAP                   56    /* 'i' contains the dsap value  */
#define LOG_SSAP                   57    /* 'i' contains the ssap value  */
#define LOG_SNAP_TYPE             803    /* 's' contains the snap value  */
#define LOG_CONTROL                58    /* 'i' contains control value   */
#define LOG_DEST_ADDR              59    /* 's' contains dest. address   */
#define LOG_TYPE_FIELD             60    /* 'i' contains 'type' field    */
#define LOG_READ_TIMEOUT           61    /* 'i' contains milliseconds    */
#define LOG_READ_CACHE             62    /* 'i' contains packet count    */
#define ENABLE_PROMISCUOUS         63    /* receive promiscuous packets  */
#define DISABLE_PROMISCUOUS        64    /* do not receive promiscuous   */
#define CHANGE_NODE_RAM_ADDR	   66 	 /* change station RAM address	 */ 
#define LOG_RAW_802MAC             67    /* no parameters are needed     */
#define LOG_RIF_ADDR               68    /* 's' contains 802.5 RI value  */
#define SET_TRN_FUNC_ADDR_MASK     69    /* 's' contains 802.5 FA mask   */
#define CLEAR_TRN_FUNC_ADDR_MASK   70    /* 's' contains 802.5 FA mask   */
#endif /* __hp9000s800 */

#define LLA_SIGNAL_MASK		   65	 /* set SIGIO mask for connection */

/********************************************************************
 * the following defines are used to configure SIGIO for a LLA file;
 * Each define is a bit mapping for a incoming event:
 *   0x1 = packet received
 *   0x2 = input queue overflow
 *********************************************************************/
#define LLA_NO_SIGNAL	0                /* 'i' contains signal mask    */
#define LLA_PKT_RECV	0x1
#define LLA_Q_OVERFLOW  0x2

/* For the following statistics values: NETSTAT returns the value in */
/* an integertype, 'i'; NETCTRL resets the statistic to zero */
#ifdef __hp9000s300
#define RX_FRAME_COUNT		  101
#define TX_FRAME_COUNT		  102
#define UNDEL_RX_FRAMES		  103  
#define UNTRANS_FRAMES		  104 
#define RX_BAD_CRC_FRAMES	  105 
#define COLLISIONS		  106 
#define DEFERRED		  107 
#define ONE_COLLISION		  108 
#define MORE_COLLISIONS		  109 
#define EXCESS_RETRIES		  110 
#define LATE_COLLISIONS		  111
#define CARRIER_LOST		  112 
#define NO_HEARTBEAT		  113 
#define ALIGNMENT_ERRORS	  114 
#define MISSED_FRAMES		  115 
#define BAD_CONTROL_FIELD	  116 
#define UNKNOWN_PROTOCOL	  117 
#define	LAN_RESTART		  118  /* DUX only */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define COLLISIONS                  0 
#define TX_FRAME_COUNT              1
#define RX_FRAME_COUNT              2
#define UNTRANS_FRAMES              3 
#define UNDEL_RX_FRAMES             4  
#define NO_TX_SPACE                 5 
#define LITTLE_RX_SPACE             6 
#define DEFERRED                    7 
#define ONE_COLLISION               8 
#define MORE_COLLISIONS             9 
#define EXCESS_RETRIES             10 
#define LATE_COLLISIONS            11
#define CARRIER_LOST               12 
#define TDR                        13 
#define RX_BAD_CRC_FRAMES          15 
#define ALIGNMENT_ERRORS           16 
#define ILLEGAL_FRAME_SIZE         17 
#define MISSED_FRAMES              18 
#define NO_HEARTBEAT               19 

#define BAD_CONTROL_FIELD          21 
#define UNKNOWN_PROTOCOL           22 
#define RX_XID                     23 
#define RX_TEST                    24 
#define RX_SPECIAL_DROPPED         25 
#define RX_ERRORS                  26 
#define	LAN_RESTART		   27  /* DUX only */

/* Token ring specific information */
#define TRN_LINE_ERR               28
#define TRN_BURST_ERR              29
#define TRN_ARI_FCI_ERR            30
#define TRN_LOST_ERR               31
#define TRN_CONGESTION_ERR         32
#define TRN_COPY_ERR               33
#define TRN_TOKEN_ERR              34
#define TRN_BUS_ERR                35
#define TRN_PARITY_ERR             36
#define TRN_RING_STATUS            37
#define TRN_ADAPTER_CHECK          38
#define TRN_COMMAND_REJ            39
#endif /* __hp9000s800 */    


/* NETSTAT-only (read-only) values for fis.reqtype  */

#ifdef __hp9000s300
#define     LOCAL_ADDRESS	1	 /* vtype > 0; working address	 */
#define	    DEVICE_STATUS	5	 /* vtype = INTEGERTYPE		 */
#define	    MULTICAST_ADDRESSES 6	 /* vtype = INTEGERTYPE		 */
#define	    FRAME_HEADER	50	 /* vtype>= 0			 */
#define     PERMANENT_ADDRESS   88       /* vtype > 0; NOVRAM on IO card */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define     DEVICE_STATUS       80       /* vtype = INTEGERTYPE          */
#define     MULTICAST_ADDRESSES 81       /* vtype = INTEGERTYPE          */
#define     FRAME_HEADER        82       /* vtype>= 0                    */
#define     ARQ_STATUS          84
#define     STATUS_BUFFER       85
#define     MULTICAST_ADDR_LIST 86
#define     LOCAL_ADDRESS       87       /* vtype > 0; working address   */
#define     PERMANENT_ADDRESS   88       /* vtype > 0; NOVRAM on IO card */
#define     STATUS_BUFFER_CLEAR 89
#define     PEEK_ADDRESS        90
#define     IODC                91
#define     PEEK_DATA           92
#define     READ_REGISTER       93
#define     RESET_HT            94
#define     RESET_HI            95
#define     SELFTEST            96
#define     LAN_LOOPBACK        97
#define     PATCH               98
#define     PATCH_END           99
#define     FLASH_INIT          100
#define     LINK_SPEED          101      /* vtype = INTEGERTYPE          */
#define     MTU_BYTE_SIZE       102      /* vtype = INTEGERTYPE          */
#define     IF_UNIT_NUMBER      103      /* vtype = INTEGERTYPE          */
#define     TRN_FUNC_ADDR       104      /* vtype = INTEGERTYPE          */
#endif /* __hp9000s800 */


/* values returned by DEVICE_STATUS */

#define     INACTIVE            0
#define     INITIALIZING        1
#define     ACTIVE              2
#define     FAILED              3


/* arg.value.i values for LOG_CONTROL */

#define     UI_CONTROL          3        /* unnumbered information; default */
#define     XID_CONTROL         0xBF     /* IEEE802 XID frame               */
#define     TEST_CONTROL        0xF3     /* IEEE802 TEST frame              */


/* device major numbers */

#ifdef __hp9000s300
#define	    DIO_IEEE802		18
#define	    DIO_ETHERNET	19  /* ETHERNET is a XEROX corp. trademark  */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define     LANUNIT_ETHERNET    49  /* ETHERNET is a XEROX corp. trademark  */
#define     LANUNIT_IEEE802     50
#define     CIO_ETHERNET        49  /* ETHERNET is a XEROX corp. trademark  */
#define     CIO_IEEE802         50
#endif /* __hp9000s800 */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* ifndef _SYS_NETIO_INCLUDED */
