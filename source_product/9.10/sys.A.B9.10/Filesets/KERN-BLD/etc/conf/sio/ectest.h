/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/ectest.h,v $
 * $Revision: 1.15.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:37:13 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 

/* BEGIN_IMS   ectest.h
******************************************************************************
****								
****		               ectest.h                 
****								
******************************************************************************
* Description           Header file for user program uxectest and
*                       pseudo-device driver ectest. Together these form
*                       a test module for Uxether, the Ethernet LAN driver
*                       for the M840.
*
* To Do List            Make potentially large fiels double when floating
*                       point works.
*
*                       Blow away next field in node_info struct.      
*
* Notes
*
* Modification History  Original 8/9/85. Kim Taylor.
*
******************************************************************************
* END_IMS  ectest.h       */

/* some odds and ends */

/* commands for IOCTL call */
#define ECTEST_INIT  _IO('k',0)
#define ECTEST_SEND  _IOW('k',1,struct uxec_testrq)
#define ECTEST_RECV  _IOR('k',2,struct node_info)

#define RANDOM    -1  /* randomly generate packet lengths */

#define P_BEGIN    0  /* packet types */
#define P_DATA     1
#define P_END      2

#define MAXNODES   3  /* maximum usuable nodes, processes */
#define MAXPROCS   3

#define E_BUF      (struct mbuf *)NULL
#define KERNEL_SPACE 0

#define TST_DATA1  0x55555555  /* for data validation */
#define TST_DATA2  0xaaaaaaaa

#define BADHEADER  0x00000001  /* error codes */
#define BADTYPE    0x00000002
#define BADLENGTH  0x00000004
#define BADDATA    0x00000008

#define INVALID_SRC -1  /* return code from ecoutput() */

#define ECTEST_TYPE    0x8003  /* extended sap used in ectest pkts */
#define LANTEST_TYPE    0x810  /* type used in ectest pkts */

#define headsize sizeof(struct test_header)
#define  makeline(x,y)  { for (i=0; i<COLS; i++ ) waddch(x,y); }


/*
 * struct uxec_testrq: store information used for a particular test of
 *                     the 802 or ethernet driver: destination, packet length,
 *                     interarrival time, number of packets, node id.
 */

struct uxec_testrq {

   int             n_packets;      /* number of packets to send */
   int             node_id;        /* id for this test */
   int             proc_id;        /* process id (not real process id) */

   struct in_addr  srcaddr;        /* ip address of source lan card */
   struct sockaddr destaddr;       /* physical ethernet address */
   int             pack_length;    /* RANDOM = -1 */
   int             arrival_time;   /* mean interarrival time */
   int             check_data;     /* 1 if data correctness */
                                   /* check requested */  
   int		   is_802_test;	   /* flag for 802 test */
   short	   dxsap;	   /* 802 dest extended SAP */
   short	   sxsap;	   /* 802 source extended SAP */
   char		   dsap;	   /* 802 dest SAP */
   char		   ssap;	   /* 802 source SAP */
   char		   ctrl;	   /* 802 ctrl field (UI, TEST, or XID) */
   } ;



/*
 * struct test_header: diagnostic information to be stored in the test packet
 *                     header: node id, proc id, type, sequence number, 
 *                     packet length.
 */

struct test_header {
   int node_no;
   int proc_no;
   int pack_type;
   union val {
      int seq_no;    /* assumed 1 for first packet  (P_BEGIN type) */
      int do_check;  /* need this first packet only */
   } val;
   int pack_length;
} ;

/* 
 * struct node_info  -- structure holding both source and destination
 *                      statistics for a node/process pair. A two-dimensional
 *                      array of (MAXNODES,MAXPROCS) structures is maintained
 *                      by ectest, and is read by a user program by a read
 *                      call to /dev/ectest.
 *
 */

struct node_info {
   int node_id;
   int proc_id;

/* source data */
   int packs_sent;
   int bytes_sent;
   long total_delay; /* in ticks */

/* destination data */
   int  badpacks;      /* number of errored packets */
   int  lostpacks;     /* number of lost packets */
   int  whicherr;      /* bit fields indicate which errors have occurred */

   char status;        /* U: unused, R: running, F: finished since */
                       /* last readm D: completely done */
   long numbytes;      /* total bytes read */
   long lastbytes;     /* numbytes at time of last read */
   int numpacks;       /* total packets received */

   struct timeval start;     /* time first packet received */
   struct timeval laststop;  /* time of last read */
   struct timeval stop;      /* time finished, or of current read */

   int do_check;             /* perform data accuracy check */
   struct node_info *next;   /* no longer used: if you delete it, remake */
                             /* both user and kernel programs!           */
                             /* (I am out of time!)                      */

};

struct macct    *ectest_macct;
