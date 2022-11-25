/* $Header: netfunc.h,v 1.7.83.4 93/09/17 18:31:04 kcs Exp $ */

#ifndef	_SYS_NETFUNC_INCLUDED
#define	_SYS_NETFUNC_INCLUDED

#define	NET_ARP			0
#define	NET_ARPIOCTL		1
#define	NET_DDPINTR		2
#define	NET_DOMAININIT		3
#define	NET_IFINIT		4
#define	NET_IFIOCTL		5
#define	NET_IPINTR		6
#define	NET_MAPINTR		7
#define	NET_MBINIT      	8
#define	NET_NETISR_DAEMON 	9
#define	NET_NIPCINIT		10
#define	NET_NSINTR		11
#define	NET_NTIMO_INIT  	12
#define	NET_NULL_INIT		13
#define	NET_NVSINPUT		14
#define	NET_NVSJOIN		15
#define	NET_PROBE		16
#define	NET_RTIOCTL		17
#define	NET_SOO_STAT		18
#define	NET_UNP_GC		19
#define	NET_X25INTR		20
#define	NET_DUXINTR		21
#define NET_PRBUNSOL		22
#define NET_ARPINIT		23
#define NET_ARPINTR		24
#define NET_RAWINTR		25
/*
** Define symbols for APPLETALK 
*/
#define NETDDP_GETATNODE	26
#define NETDDP_AARPINPUT	27
#define NETDDP_ATALK_OUTPUT	28
/*
** Define symbols for streams
*/
#define NETSTR_SCHED            29
#define NETSTR_MEM              30
#define NETSTR_WELD             31
/*
 * Define symbol for NIT
 */
#define NET_NITINTR		32
/*
 * Define symbol for ISDN
 */
#define NET_ISDNINTR		33

/*  WARNING:  Before adding new #defines, make sure that sys/netfunc.c
**  declares enough space in its netproc[] array for the new numbers
**  you are going to add.  If it doesn't, add more.
*/

#define NETCALL(function) (*netproc[function])
/* usage: NETCALL(function)(parm1, parm2, ...) */

#define	netproc_assign(index,function)	netproc[index] = function

extern	int (*netproc[])();
#endif	/* _SYS_NETFUNC_INCLUDED */
