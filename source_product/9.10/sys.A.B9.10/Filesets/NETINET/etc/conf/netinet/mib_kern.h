/*
 * @(#)mib_kern.h: $Revision: 1.6.83.4 $ $Date: 93/09/17 19:04:25 $
 * $Locker:  $
 */

#ifndef	_SYS_MIB_KERN_INCLUDED	/* allow multiple includes of this file */
#define	_SYS_MIB_KERN_INCLUDED	/* without causing compilation errors   */
/*
 * 	macros for ICMP Network Management
 */
#define MIB_icmpIncrCounter(id)		MIB_icmpcounter[id & INDX_MASK]++;
/*
 * 	macros for IP Network Management
 */
#define MIB_ipIncrCounter(id)		MIB_ipcounter[id & INDX_MASK]++;
#define MIB_ipIncrRouteNumEnt		MIB_ipRouteNumEnt ++;
#define MIB_ipDecrRouteNumEnt		MIB_ipRouteNumEnt --;
/*
 * 	macros for TCP
 */
#define MIB_tcpIncrCounter(id)	MIB_tcpcounter[id & INDX_MASK]++;
#define MIB_tcpIncrConnNumEnt   MIB_tcpConnNumEnt++;
#define MIB_tcpDecrConnNumEnt   MIB_tcpConnNumEnt--;
/*
 * 	macros for UDP
 */
#define MIB_udpIncrCounter(id)	MIB_udpcounter[id & INDX_MASK]++;
#define MIB_udpIncrLsnNumEnt	MIB_udpLsnNumEnt++;
#define MIB_udpDecrLsnNumEnt	MIB_udpLsnNumEnt--;
/*
 *	IP Routing Table : routing mechanisms
 */
#define	NMOTHER		1
#define	NMLOCAL		2		/* netwk mgmt routing mechanisms */
#define	NMMGMT		3
#define	NMICMP		4
#define NMEGP		5
#define NMGGP		6
#define	NMHELLO		7
#define NMRIP		8
#define	NMIS-IS		9
#define NMES-IS		10
#define NMCISCOIGRP	11
#define NMBBNSPFIGP	12
#define NMOSPF		13
/*
 *	Network Management Event Codes
 */
#define	NMV_COLDSTART		0
#define	NMV_WARMSTART		1
#define	NMV_LINKDOWN		2
#define	NMV_LINKUP		3
#define	NMV_AUTHENFAIL		4
#define	NMV_EGPNEIGHLOSS	5
#define NMV_DUPLINKADDRS     1025

/*
 *	Interface state defines for Network Management
 *      Values assigned to ifAdminStatus and ifOperStatus
 */
#define		LINK_UP		1	
#define		LINK_DOWN       2

/*
 *	Interface type defines for Network Management
 */
#define UNKNOWN                 1
#define DDN_X25			4
#define RFC877_X25		5
#define ETHERNET_CSMACD         6
#define IEEE8023_CSMACD         7
#define IEEE8025_TOKEN_RING     9
#define APOLLO_TOKEN_RING      10
#define NM_LOOPBACK	       24

/*
 *	Link bandwidth defines for Network Management
 */
#define ETHER_BANDWIDTH         10000000
#define FDDI_BANDWIDTH         100000000
#define TOKEN_16_BANDWIDTH      16000000
#define TOKEN_4_BANDWIDTH        4000000
#define ATR_BANDWIDTH		12000000

/*
 *	Definitions for Subsystem Registration
 */
struct	nmsw {
	int	(*pr_get)();		/* subsystem get routine */
	int	(*pr_set)();		/* subsystem set routine */
	int	(*pr_create)();		/* subsystem create routine */
	int	(*pr_delete)();		/* subsystem delete routine */
}; 
/*
 *	External Declarations
 */
extern	unsigned char	ipDefaultTTL;
extern	int	MIB_ipRouteNumEnt;
extern	int	MIB_tcpConnNumEnt;
extern	int	MIB_udpLsnNumEnt;
extern	counter	MIB_ipcounter[];
extern	counter	MIB_tcpcounter[];
extern	counter	MIB_udpcounter[];
extern	counter	MIB_icmpcounter[];
extern	struct nmsw nmsw[];
extern	nmget_ip(),nmset_ip(),nmcreate_ip(),nmdelete_ip();
extern	nmget_tcp(),mib_unsupp();
extern	nmget_udp();
extern	nmget_icmp();
extern	nmget_sys();
extern	nmget_if(),nmset_if();
/*
 *	Register subsystem procedure handlers
 */
#define	NMREG(gid,pget,pset,pcreate,pdelete) {	\
	if ( gid > GP_MAX )	\
		return (-1);	\
	else {			\
		nmsw[gid].pr_get = pget;	\
		nmsw[gid].pr_set = pset;	\
		nmsw[gid].pr_create = pcreate;	\
		nmsw[gid].pr_delete = pdelete;	\
	}\
}
#endif	_SYS_MIB_KERN_INCLUDED
