/*
** $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/subsys_id.h,v $
** $Revision: 1.5.83.4 $  $Author: kcs $
** $State: Exp $  $Locker:  $
** $Date: 93/09/17 18:35:28 $
**
**
*/

/*
***************************************************************************
*  subsytem names and ID's assignement
***************************************************************************
*
* Modification History : 
* 11-02-88 : Added and re-arranged subsystem ID assignments and set numbers.
* 09-02-88 : Added Catalog set numbers for the formatter and CM.
* 08-19-88 : Added more non-configurable group names.  Also, added TRANSIT
*            canonical address defines and the structures and defines for
*            trace kinds and log classes (OCR #135) stille.
* 03-10-88 : added ROSE to the subsystemid list
* 02-03-88 : initial version
* 12-5-88  : added #ifndef KERNEL around some data structures 
* 09-01-89 : Added new NL set and reorganised and use symbolic defn when 
*            necessary
* 02-26-90 : clean up of file. sorted it out so it's easier to see what 
*            numbers are used and such.               - kathryn vandiver -
*
***************************************************************************
*/


/*-----------------------------------------------------------------------*/
/* This file defines the subsystem numbers for the network subsystems.   */
/* This includes files for OSI, GOSIP, FTAM, MMS, OpenView, ARPA, X.25   */
/* X.400, X.500 and all associated drivers. All TRANSIT card related     */
/* values will probably be removed in 9.0.                               */
/* The order of organization is as follows:                              */
/* 1. Subsystem id numbers, 0 through 255.                               */
/* 2. Trace kinds and data structure                                     */
/* 3. Log classes and data structure.                                    */
/* 4. Canonical Addresses for the TRANSIT Card. This is OSI specific.    */
/* 5. Test subsystem id numbers assigned to Canonical addresses.         */
/* 6. The TRANSIT OSI Express stack subsystem id numbers.                */
/*    These values are canonical addresses. This is why they are not in  */
/*    the 0-255 range.                                                   */
/* 7. Formatter catalog set assignments.                                 */
/*-----------------------------------------------------------------------*/


#define  NS_HOST_SS_START  0
#define  NS_HOST_SS_END    63
#define  OSI_HOST_SS_START 64
#define  OSI_HOST_SS_END   255
#define  MAX_SUBSYS        256


/* These are the subsystems for the old NS logging */
/* these were removed from ns_diag.h */
#define NS_LS_LOGGING           0
#define NS_LS_NFT               1   /* dscopy */
#define NS_LS_LOOPBACK          2   /* loopback driver 8.0 */
#define NS_LS_NI                3   /* PPN or NI under SLIP 8.0 */
#define NS_LS_IPC               4   /* IP´ protocol */
#define NS_LS_SOCKREGD          5 
#define NS_LS_TCP               6
#define NS_LS_PXP               7
#define NS_LS_UDP               8
#define NS_LS_IP                9
#define NS_LS_PROBE             10
#define NS_LS_LAN0              11    /* 802.3 driver      */
#define NS_LS_DRIVER            11    /* 802.3 driver      */
#define NS_LS_RLBD              12  
#define NS_LS_BUFS              13
#define NS_LS_CASE21            14
#define NS_LS_ROUTER21          15
#define NS_LS_NFS               16
#define NS_LS_NETISR            17
#define NS_LS_X25               18    /* X.25 logging only */
#define NS_LS_NSE               19 
#define NS_LS_STRLOG            20
#define NS_LS_TIRDWR            21
#define NS_LS_TIMOD             22
#define NS_LS_COUNT             23    /* is this necessary ? */

#define X25L2                   24    /* tracing only */
#define X25L3                   25    /* tracing only */
#define NS_TRC_FILTER           26 /* used by fmter to converge NS trc format */
#define NS_TRC_NAME             27 /* "   "    "    "  */
#define NS_TRACE_MAX            30 /* used for range checking in ns_trace_link*/

#define TOKEN                   31    /* Token Ring (802.5) driver */





/*
 * OSI_HOST_SERVICES (valid numbers are 64 - 95 )
 */
#define  FTAM_INIT              64
#define  FTAM_RESP              65
#define  MMS                    66
#define  X400                   67
#define  X500                   68
#define  NM_AGENT               69
#define  FTAM_VFS               70
#define  MMS_EB                 71
#define  FTAM_USER              72
#define  FTAM_FTP_GW            73
#define  FTP_FTAM_GW            74

/* These are the subsystems for OpenView */
#define OVA                     75
#define SNMP                    76
#define CMOT                    77
#define OVE                     78
#define OVC                     79
#define OVW                     80
#define OVD                     81
#define OVS                     82
#define OVCAPI                  83
#define OVEXTERNAL              84
#define OV_DM                   OVCAPI
#define OVWAPI                  85

/* These are subsystems for Telecom Integrating Environment */
#define TIE_DRS                 87
#define TIE_SECURITY            88
#define TIE_DW                  89

/* These are subsystems for the OPUSK OSI Stack */
#define OTS                     90
#define OTS_NETWORK             91
#define OTS_TRANSPORT           92
#define OTS_SES                 93
#define OTS_ACSE_PRES           94

#define FDDI                    95 /* FDDI driver */

/* Reserved test ID assignments */
#define  TEST_ID_1              96
#define  TEST_ID_2              97

/*  kernel subsystems that do not call the old NS log or trace routines*/
#define  DRIVER_OSI0_ID         98
#define  CIA_ID                 99
#define  ULIPC_ID               100


/* These are subsystems for X.500 */
#define  X500_DUA_SSID          101
#define  X500_DSA_SSID          102
#define  X500_SSA_SSID          103
#define  X500_DIB_SSID          104
#define  X500_API_SSID          105
#define  X500_TOOLS_SSID        106
/*#define  X500_ETC_SSID        107  future for X.500? (not used yet) */


/* These are subsystems for X.400 */
#define X4XFER                  109
#define DECODER                 110
#define ENCODER                 111
#define MTASD                   112
#define MTADI                   113
#define MTA                     114
#define RTSE                    115

/*
 * OSI management services
 */
#define  SHM               116   /* Shared memory manager             */

#define  ACSE_US           119   /* user space ACSE for GOSIP         */
#define  ROS               120   /* ROSE                              */
#define  HPS               121  
#define  CM                122   /* connection management             */
#define  EM                123   /* Event Manager                     */
#define  ULA_UTILS         124   /* upper layer applications - utils  */
#define  ICS_DAEMON        125   /* initial store configurator daemon */
#define  ICS_CONFIGURATOR  126   /* initial store configurator        */
#define  FORMATTER         127   /* formatter for all messages        */

#define  ISDN              128   /* ISDN driver */


/*-------------------------------------------------------------------------*/
/*   All defined trace kind values.                                        */
/*   These can be OR'd to produce combinations of trace kinds.             */
/*   The trace kind data structure is also listed here.                    */
/*-------------------------------------------------------------------------*/

#define HDR_IN_BIT          0x80000000 /* inbound header tracing mask  */
#define HDR_OUT_BIT         0x40000000 /* outbound header tracing mask */
#define PDU_IN_BIT          0x20000000 /* inbound PDU tracing mask     */
#define PDU_OUT_BIT         0x10000000 /* outbound PDU tracing mask    */
#define PROCEDURE_TRACE_BIT 0x08000000 /* procedure entry/exit trace   */
#define STATE_TRACE_BIT     0x04000000 /* state machine tracing mask   */
#define ERROR_TRACE_BIT     0x02000000 /* error tracing mask           */
#define LOGGING_TRACE_BIT   0x01000000 /* log call tracing mask        */
#define LOOP_BACK_BIT       0x00800000 /* for loopback                 */
#define PTOP_BIT            0x00400000 /* for point to point           */
#define GLOBAL_RESERVED     0x00300000 /* global reserved bits mask    */
#define SUBSYSTEM_BITS      0x000fffff /* subsystem specific bit mask  */

#ifndef _KERNEL
typedef struct {
        unsigned hdr_in_bit          : 1;  /* inbound header tracing     */
        unsigned hdr_out_bit         : 1;  /* outbound header tracing    */
        unsigned pdu_in_bit          : 1;  /* inbound PDU tracing        */
        unsigned pdu_out_bit         : 1;  /* outbound PDU tracing       */
        unsigned procedure_trace_bit : 1;  /* procedure entry/exit trace */
        unsigned state_trace_bit     : 1;  /* state machine tracing      */
        unsigned error_trace_bit     : 1;  /* error tracing              */
        unsigned logging_trace_bit   : 1;  /* log call tracing           */
        unsigned loop_back_bit       : 1;  /* for loopback               */
        unsigned ptop_bit            : 1;  /* for point to point         */
        unsigned global_reserved     : 2;  /* global reserved bits       */
        unsigned subsystem_bits      : 20; /* subsystem specific bits    */
} trace_kind_bits_type;
#endif



/*-------------------------------------------------------------------------*/
/* LOG CLASSES.  Please note - log classes for the old NS kernel log call  */
/* can be found in ../h/net_diag.h                                         */
/* The log class data structure is also listed here.                       */
/*-------------------------------------------------------------------------*/

#define  INFORMATIVE        0x1
#define  WARNING            0x2
#define  ERROR              0x4
#define  DISASTER           0x8
#define  NETMSG             0x10

#ifndef _KERNEL
typedef struct {
        unsigned reserved         : 27;
        unsigned netmsg_bit       : 1;
        unsigned disaster_bit     : 1;
        unsigned error_bit        : 1;
        unsigned warning_bit      : 1;
        unsigned informative_bit  : 1;
} log_class_bits_type;
#endif




/*
 * HP_OSI_EXPRESS_ENVIRONMENT
 */

/* Canonical addresses of the card for TRANSIT, still under HP_OSI_EXPRESS */

#define   NULL_CANON_ADDR       000000
#define   LLC_CANON_ADDR        023110    /*  IEEE 802 LLC             */
#define   LAN_3_CANON_ADDR      023113    /*  IEEE 802.3 MAC           */
#define   LAN_4_CANON_ADDR      023114    /*  IEEE 802.4 MAC           */
#define   LAN_5_CANON_ADDR      023115    /*  IEEE 802.5 MAC           */
#define   IP_CANON_ADDR         002401    /*  ISO 8473 CL Internet     */
#define   XP_CANON_ADDR         002035    /*  ISO 8073 Transport       */
#define   SS_CANON_ADDR         002600    /*  ISO 8327 CO Session      */
#define   PR_CANON_ADDR         002601    /*  ISO 8823 CO Presentation */
#define   AC_CANON_ADDR         002602    /*  ISO 8650 ACSE            */
#define   BB_CANON_ADDR         003007    /*  Bounce Back Module       */
#define   AFCP_OSI_CANON_ADDR   003053    /*  Avesta flow control pm   */
#define   ALCP_OSI_CANON_ADDR   003061    /*  Avesta link control pm   */
#define   SIA_CANON_ADDR        002406    /*  SI Agent                 */
#define   EG_CANON_ADDR         002407    /*  Exception Generator      */
#define   GPM_CANON_ADDR        002410    /*  Generic Protocol Module  */
#define   SII_CANON_ADDR        003040    /*  SI Initiator             */
#define   SIT_CANON_ADDR        003041    /*  SI Target                */
#define   MMGR_CANON_ADDR       060001    /*  Memory Manager           */
#define   BMGR_CANON_ADDR       060002    /*  Buffer Manager           */
#define   SUB_CANON_ADDR        060003    /*  Subtasker/Dispatcher     */
#define   TMGR_CANON_ADDR       060004    /*  Timer Manager            */
#define   PMGR_CANON_ADDR       060005    /*  Path Manager             */
#define   PIF_CANON_ADDR        060006    /*  Protocol Interface       */
#define   QMGR_CANON_ADDR       060007    /*  Queue Manager            */
#define   MISC_UTIL_CANON_ADDR  060010    /*  Misc. Utilities          */
#define   BM_CANON_ADDR         060011    /*  Backplane Message I/F    */
#define   TRC_CANON_ADDR        060012    /*  Trace Module             */
#define   LOG_CANON_ADDR        060013    /*  Log Module               */
#define   CM_CANON_ADDR         060014    /*  Protocol Mgmt. Services  */
#define   WATSON_CANON_ADDR     060015    /*  Protocol Stack Monitor   */
#define   BH_CANON_ADDR         060016    /*  Backplane Handler        */
   
#define  CARD_PATH_MGR     PMGR_CANON_ADDR
#define  CARD_BMI          BM_CANON_ADDR
#define  CARD_CMS          CM_CANON_ADDR  
#define  HP_OSI_EXPRESS_CMS CARD_CMS

#define  CARD_BB           BB_CANON_ADDR
#define  CARD_SIA          SIA_CANON_ADDR
#define  CARD_EG           EG_CANON_ADDR
#define  CARD_GPM          GPM_CANON_ADDR

/* Generic OSI entities */

/* 
 * HP_OSI_EXPRESS_STACK
 */
#define  ACSE              AC_CANON_ADDR      /* 0x582 canonical address */
#define  PRESENTATION      PR_CANON_ADDR      /* 0x581 canonical address */
#define  SESSION           SS_CANON_ADDR      /* 0x580 canonical address */
#define  TRANSPORT         XP_CANON_ADDR      /* 0x41d canonical address */
#define  NETWORK           IP_CANON_ADDR      /* 0x501 canonical address */
#define  LLC               LLC_CANON_ADDR     /* 0x2648 canonical address */ 
#define  MAC_8023          LAN_3_CANON_ADDR   /* 0x264b canonical address */
#define  MAC_8024          LAN_4_CANON_ADDR   /* 0x264c canonical address */
#define  MAC_8025          LAN_5_CANON_ADDR   /* 0x264d canonical address */


/**************************************************************************
 * osi.cat catalog set assignments
 **************************************************************************
 */

#define NL_FORMATTER_SET       1
#define NL_HPS_SET             3
#define NL_CM_SET              4
#define NL_FTAM_SET            5
#define NL_FTAM_DIAG_SET       6
#define NL_FTAM_AIF_SET        7
#define NL_EM_SET              8
#define NL_ULA_SET             9
#define NL_SHM_SET             12
#define NL_OSI0_DRIVER_SET     13  
#define NL_CIA_SET             14
#define NL_ULIPC_SET           15
#define NL_MMS_SET             16
#define NL_ACSE_SET            17

