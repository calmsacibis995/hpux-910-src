/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/net.h,v $
 * $Revision: 1.20.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:27:42 $
 *
 */
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/



#ifdef NETWORK
/* Include needed I/O files */
#include "../io/an_ionet.h"

/* This file is included after sys/mount.h, which has its own defintion
 * of MAVAIL.  analyze doesn't use that one.  Undef it so it doesn't clash
 * with the one in sys/mbuf.h.
 */
#undef MAVAIL

#ifdef BUILDFROMH

#include <h/ns_ipc.h>
#include <h/mbuf.h>
#include <h/domain.h>
#include <h/protosw.h>
#include <h/socket.h>
#include <h/socketvar.h>
#include <h/ns_ipcvar.h>

#else

#include <sys/ns_ipc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ns_ipcvar.h>

#endif

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_var.h>
#include <netinet/pxp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/if_ether.h>
#include <net/raw8023.h>
#include <net/lan_802.h>
#include <netinet/arp.h>
#include <net/probe.h>
#include <net/probe_4.h>
#include <netinet/probe_2.h>
#include <sio/lanc.h>
#include <sio/lan0.h>
#include <sio/lan1.h>

/* DTYPE_SOCKET is from sys/file.h when KERNEL is defined.  This value MUST
 * track the one in sys/file.h.
 */
#define DTYPE_SOCKET 2

/* These are from sys/ns_ipc.h when KERNEL is defined.  These values MUST
 * track the ones in sys/ns_ipc.h.
 */
#define NS_VC		6
#define NS_DEST		7

/* NOTE: The following are user configurable parameters which determine
 * how many networking structures analyze can read.  These figures are 
 * much more than generous for the current Indigo kernels.
 */

#define MAX_DOMAINS	10
#define MAX_INETSW	10*MAX_DOMAINS
#define MAX_IFNETS	10
#define MAX_XSAPS	20
#define MAX_IPQS	100
#define MAX_IPASFRAGS   512
#define MAX_SOCKETS	512
#define MAX_SOCKBUFS	MAX_SOCKETS*2+100   /* 100 is fudge for live kernel */
#define MAX_NUM_LAN0    10		    /* used to decide num_lan0 bad  */

#define DEFAULT_NUM_LAN0 1		    /* used if num_lan0 is bad	    */



struct	nlookup_entry	{
    char    *nl_name;	    /* name of this data structure	    */
    short   nl_type;	    /* type of this structure's contents    */
    short   nl_xindx;	    /* this structures index into the nlist */
    int     nl_defaultv;    /* default address to use for displaying*/
    int     (*nl_verify)(); /* proc to call to verify this structure*/
    void    (*nl_dump)();   /* proc to call to dump this structure  */
};



struct	netdata	{
    uint    nd_v;	/* virtual kernel address of symbol	*/
    uint    nd_p;	/* actual core file address of symbol	*/
    char    *nd_l;	/* ptr to in memory copy of data struct	*/
    uint    nd_num;	/* number of elements in array at *nd_l */
    uint    nd_mnum;	/* max num of elements in array at *nd_l*/
    uint    nd_size;	/* maximum size of array at *nd_l	*/
    uint    nd_sizeof;	/* size of data struct			*/
    uint    *nd_vlist;	/* array of vaddrs of elements or null  */
    uint    *nd_llist;	/* array of laddrs of elements or null  */
};

#define VA(xindx, cast)   (cast)(netdata[xindx].nd_v)
#define PA(xindx, cast)   (cast)(netdata[xindx].nd_p)
#define LA(xindx, cast)   (cast)(netdata[xindx].nd_l)
#define NDNUM(xindx)	    (netdata[xindx].nd_num)
#define NDMNUM(xindx)	    (netdata[xindx].nd_mnum)
#define NDSIZE(xindx)	    (netdata[xindx].nd_size)
#define NDSIZEOF(xindx)	    (netdata[xindx].nd_sizeof)
#define NDVLIST(xindx)	    (netdata[xindx].nd_vlist)
#define NDLLIST(xindx)	    (netdata[xindx].nd_llist)


/*** The following macros reference the net_nl lookup table
 *** that is defined within netget.c ***********************/
#define	X_FIRSTNET      0

#define	X_MBUTL         0
#define	X_MBMAP         1
#define	X_MBSTAT        2
#define	X_NMBCLUSTER	3
#define	X_MFREE         4
#define	X_MCLFREE       5
#define	X_mBMAP         6
#define	X_MBUF_MEMORY	7
#define	X_MASTAT        8
#define	X_MCLREFCNT     9
#define X_INETSW        10
#define X_DOMAINS       11
#define X_IFNET		12
#define X_LANIFT	13
#define X_NUM_LAN0	14
#define X_XSAP_HEAD	15
#define X_XSAP_LIST	16
#define X_SAP_ACTIVE	17
#define X_IPQMAXLEN	18
#define X_IPSTAT	19
#define X_LAN0IFT	20		/*RJ*/
#define X_IPQ		21
#define X_ICMPSTAT	22
#define X_TCP_CB	23
#define X_TCPSTAT	24
#define X_PXP_CB	25
#define X_PXPSTAT	26
#define X_UDP_CB	27
#define X_UDPSTAT	28
#define X_SBSTAT	29
#define X_PROBESTAT	30
#define X_PR_NTAB	31
#define X_PR_VNATAB	32
#define X_LAN1IFT	33	/* RJ */
#define X_NUM_LAN1	34	/* RJ */

#define X_LASTNLIST	34

/* entries beyond this point have no corresponding symbol in the
 * nlist array, and use this value only as an index into nlookup.
 */

#define X_SOCKET	35
#define X_SOCKBUF	36
#define X_INPCB		37
#define X_MSGMTBL	38
#define X_IPASFRAG	39

#define X_LASTNET       39




/* define NT #defines based on mbuf defines */
#define MBTONT(x)	(MT_LASTONE+1+x)

#define NT_MBUF	    	MBTONT(0)
#define NT_CLUSTER	MBTONT(1)
#define NT_NONE  	MBTONT(2)
#define NT_ANY   	MBTONT(3)
#define NT_IFNET  	MBTONT(4)
#define NT_LANIFT	MBTONT(5)
#define NT_SAPINPUT	MBTONT(6)
#define NT_XSAPLINK	MBTONT(7)
#define NT_PRNTAB	MBTONT(8)
#define NT_PRVNATAB	MBTONT(9)
#define NT_ROUTE	MBTONT(10)
#define NT_RTENTRY	MBTONT(11)
#define NT_IPINTRQ	MBTONT(12)
#define NT_IPQ		MBTONT(13)
#define NT_IPASFRAG	MBTONT(14)
#define NT_INPCB	MBTONT(15)
#define NT_TCPCB	MBTONT(16)
#define NT_MSGMTBL	MBTONT(17)
#define NT_PXPCB	MBTONT(18)
#define NT_RAWCB	MBTONT(19)
#define NT_RAW8023CB	MBTONT(20)
#define NT_DOMAIN	MBTONT(21)
#define NT_PROTOSW	MBTONT(22)
#define NT_SOCKET	MBTONT(23)
#define NT_SOCKBUF	MBTONT(24)
#define NT_NAMERECORD	MBTONT(25)
#define NT_RFAUSER	MBTONT(26)
#define NT_RFAINODE	MBTONT(27)
#define NT_RFAPROCMEM	MBTONT(28)
#define NT_NETUNAM	MBTONT(29)
#define NT_PROBEINTRQ   MBTONT(30)

#define NT_LASTONE  	MBTONT(30)



/* netflags -- this structure contains the mask and corresponding name for 
 * networking flags. 
 */

struct netflags {
    int     val;
    char    *name;
};

struct netopt {
    int     val;
    char    *name;
};

extern struct netopt netopts[];

#define NETO_MDUMP	0
#define NETO_MFOLLOW	1
#define NETO_INUSE	2



/* Misc macros */

#define NVALIDV(v)     (vton(v) != -1)		    /* return non-zero if v 
						       is a valid networking 
						       virtual address  */
#define NTVALIDV(n,v)  (ntvton(n,v) != -1)
#define vmbtocl(v)     (mtocl(vmtod(v)&~NETCLOFSET))/* virtual mbtocl()	*/



/* Error handling */

struct neterr {
    int	    ntyp;
    uint    vaddr;
};

extern struct neterr neterr;

/* Error type setting and clearing macros */
#define NESAV		struct neterr _neterr; _neterr = neterr
#define NERES		neterr = _neterr
#define NECLR		NESET(-1, -1)
#define NECLRNT		NESETNT(-1)
#define NECLRVA		NESETVA(-1)
#define NESET(nt, va)   { NESETNT(nt); NESETVA(va); }
#define NESETNT(nt)	neterr.ntyp = (nt)
#define NESETVA(va)	neterr.vaddr = ((int)va)

#define NE_FATAL    0
#define NE_ERROR    1
#define NE_WARNING  2
#define NE_MSG	    3

#define NFATAL(msg, val)    net_error(NE_FATAL, msg, val)
#define NERROR(msg, val)    net_error(NE_ERROR, msg, val)
#define NERROR2(msg, v1, v2)    net_error(NE_ERROR, msg, v1, v2)
#define NWARN(msg, val)	    net_error(NE_WARNING, msg, val)
#define NWARN2(msg, v1, v2)    net_error(NE_WARNING, msg, v1, v2)
#define NMSG(msg, val)	    net_error(NE_MSG, msg, val)
#define NMSG2(msg, v1, v2)    net_error(NE_MSG, msg, v1, v2)
#define NMSG3(msg, v1, v2, v3)    net_error(NE_MSG, msg, v1, v2, v3)
    


/* Debugging macros */

#ifdef DEBUG
#define	    NDBG(f)	{printf f; printf ("\n");} 
#else  DEBUG
#define	    NDBG(f)	/* do nothing */
#endif DEBUG


#define TRASH_VAL	-3	/* 0 and -3 are good candidates */



/* Networking data structures */
extern int     nmbclusters;
extern struct  mbuf
	        *mfree, 
		*mclfree, 
		*mbuf_memory;
extern  struct	mastat	mastat;
extern  struct	mbstat	mbstat;
extern  struct	sbstat	sbstat[];
extern  struct	probestat probestat;
extern  struct  domain  *domains;
extern  struct  ifnet   *ifnet;
extern  char    mclrefcnt[NMBCLUSTERS];
extern  struct  lan_ift *lanift;
extern  struct  lan0_ift *lan0ift;	/* rj */
extern  struct  lan1_ift_t *lan1ift;	/* rj */
extern  int     num_lan0;
extern  int     num_lan1;
extern  struct  xsap_link *xsap_head;
extern  struct  xsap_link *xsap_list_head;
extern  struct  sap_input sap_active[NSAPS];
extern  int	ipqmaxlen;
extern  struct	ipstat	ipstat;
extern  struct  ifqueue ipintrq;
extern  struct  ifqueue probeintrq;
extern  struct	ipq *ipq;
extern  struct	icmpstat icmpstat;
extern  struct	inpcb tcp_cb;		/* head of queue of active tcpcb's */
extern  struct	tcpstat tcpstat;	/* tcp statistics */
extern  struct	inpcb pxp_cb;		/* head of queue of active pxpcb's */
extern  struct	pxpstat pxpstat;	/* pxp statistics */
extern  struct	inpcb udp_cb;		/* head of queue of active udpcb's */
extern  struct	udpstat udpstat;	/* udp statistics */


/* analyze data structures */
extern int    nflg;
extern int    socket_conn;
extern int    nmmtcp;
extern struct nlist net_nl[];
extern struct netdata netdata[];
extern struct nlookup_entry nlookup[];
extern char   mclmap[NMBCLUSTERS];
extern char   mclfreemap[NMBCLUSTERS];



/* analyze routines */
extern uint vtol();
extern char *net_malloc();
extern char *net_getstr();
extern char *netdata_ntypname();
extern int  netverify_nop(),
	    netverify_ifnet(),
	    netverify_ifqueue(),
	    netverify_inpcb(),
	    netverify_ipq(),
	    netverify_ipasfrag(),
	    netverify_lanift(),
	    netverify_mbuf(),
	    netverify_prntab(),
	    netverify_prvnatab(),
	    netverify_socket(),
	    netverify_sockbuf(),
	    netverify_sapinput(),
	    netverify_tcpcb(),
	    netverify_xsaplink();
extern void n_dump_nop(),
	    netdump_cluster(),
	    netdump_domain(),
	    netdump_ifnet(),
	    netdump_ifqueue(),
	    netdump_inpcb(),
	    netdump_ipq(),
	    netdump_ipasfrag(),
	    netdump_lanift(),
	    netdump_lan0ift(),
	    netdump_lan1ift(),
	    netdump_macct(), 
	    netdump_mbuf(),
	    netdump_msgmtbl(),
	    netdump_pcb(),
	    netdump_prntab(),
	    netdump_protosw(), 
	    netdump_prvnatab(),
	    netdump_pxpcb(),
	    netdump_route(), 
	    netdump_rtentry(), 
	    netdump_sapinput(),
	    netdump_sockaddr(),
	    netdump_sockbuf(),
	    netdump_socket(),
	    netdump_tcpcb(),
	    netdump_xsaplink();
	    

#ifdef	NS_MBUF_QA
extern	ns_mbuf_qa_on;
#endif	NS_MBUF_QA

#else  NETWORK

/* These are defined so that references to them in the grammar and
 * in display.c will be resolved when networking is configured out.
 */

#define mtocl(x)	0
#define dtom(x)		0
#define dumpnet()	\
			fprintf(outf, "networking not supported\n");
#define cltom(x)	0
#define vmbtocl(x)	0

#endif NETWORK

