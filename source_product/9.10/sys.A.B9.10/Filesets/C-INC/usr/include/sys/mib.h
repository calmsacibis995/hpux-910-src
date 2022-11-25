/*
 * @(#)mib.h: $Revision: 1.7.83.4 $ $Date: 93/09/17 18:29:35 $
 * $Locker:  $
 */

#ifndef	_SYS_MIB_INCLUDED	/* allow multiple includes of this file */
#define	_SYS_MIB_INCLUDED	/* without causing compilation errors   */
#ifdef _KERNEL_BUILD
#include "../h/time.h"
#else  /* ! _KERNEL_BUILD */
#include <time.h>
#endif /* _KERNEL_BUILD */
/*
 * Include file for accessing MIB objects
 */

typedef	unsigned long	counter;
typedef	unsigned long	gauge;
typedef	unsigned int	thresh;
typedef unsigned long	TimeTicks;
typedef	unsigned long	ipaddr;

/*
 * Assignment of object id uses the following scheme :
 *
 *	object id = <ID group id> + <index>
 */
#define INDX_MASK	0xFFFF		/* mask to pick up index */
#define OBJID(x,y)	((x)<<16) + (y)
/*
 *	Group ID's
 */
#define GP_sys		1
#define GP_if		2
#define GP_at		3
#define GP_ip		4
#define GP_icmp		5
#define GP_tcp		6
#define GP_udp		7
#define GP_MAX		GP_udp
/*
 * System Group
 */
#define ID_sys				OBJID(GP_sys,0)
#define ID_sysDescr			OBJID(GP_sys,1)
#define ID_sysObjectID			OBJID(GP_sys,2)
#define ID_sysUpTime			OBJID(GP_sys,3)
/*
 * Interfaces Group
 */
#define	ID_ifNumber			OBJID(GP_if,1)
#define	ID_ifTable			OBJID(GP_if,2)
#define	ID_ifEntry			OBJID(GP_if,3)
/*
 * Address Translation Group
 */
#define	ID_atNumEnt			OBJID(GP_at,1)
#define	ID_atTable			OBJID(GP_at,2)
#define	ID_atEntry			OBJID(GP_at,3)
/*
 * IP Group : Ordinary Counters
 */
#define ID_ip				OBJID(GP_ip,0)
#define ID_ipForwarding			OBJID(GP_ip,1)
#define ID_ipDefaultTTL			OBJID(GP_ip,2) 
#define ID_ipInReceives			OBJID(GP_ip,3)
#define ID_ipInHdrErrors       		OBJID(GP_ip,4)
#define ID_ipInAddrErrors      		OBJID(GP_ip,5) 
#define ID_ipForwDatagrams     		OBJID(GP_ip,6)
#define ID_ipInUnknownProtos   		OBJID(GP_ip,7) 
#define ID_ipInDiscards        		OBJID(GP_ip,8) 
#define ID_ipInDelivers        		OBJID(GP_ip,9) 
#define ID_ipOutRequests       		OBJID(GP_ip,10) 
#define ID_ipOutDiscards       		OBJID(GP_ip,11) 
#define ID_ipOutNoRoutes       		OBJID(GP_ip,12) 
#define ID_ipReasmTimeout      		OBJID(GP_ip,13) 
#define ID_ipReasmReqds      		OBJID(GP_ip,14) 
#define ID_ipReasmOKs			OBJID(GP_ip,15) 
#define ID_ipReasmFails      		OBJID(GP_ip,16)
#define ID_ipFragOKs      		OBJID(GP_ip,17)
#define ID_ipFragFails      		OBJID(GP_ip,18) 
#define ID_ipFragCreates       		OBJID(GP_ip,19) 
#define MIB_ipMAXCTR			19		/* used in ip_nm.h */
/*
 * IP Address Table
 */
#define	ID_ipAddrNumEnt			OBJID(GP_ip,1025)
#define	ID_ipAddrTable			OBJID(GP_ip,1026)
#define	ID_ipAddrEntry			OBJID(GP_ip,1027)
/*
 * IP Routing Table 
 */
#define	ID_ipRouteNumEnt		OBJID(GP_ip,1028)
#define	ID_ipRouteTable			OBJID(GP_ip,1029)
#define	ID_ipRouteEntry			OBJID(GP_ip,1030)
/*
 * IP Network To Media Table
 */
#define ID_ipNetToMediaTableNum           OBJID(GP_ip,1031)
#define ID_ipNetToMediaTable              OBJID(GP_ip,1032)
#define ID_ipNetToMediaTableEnt           OBJID(GP_ip,1033)
/*
 * ICMP Group
 */
#define ID_icmp				OBJID(GP_icmp,0)
#define ID_icmpInMsgs			OBJID(GP_icmp,1)
#define ID_icmpInErrors			OBJID(GP_icmp,2)
#define ID_icmpInDestUnreachs		OBJID(GP_icmp,3)
#define ID_icmpInTimeExcds		OBJID(GP_icmp,4)
#define ID_icmpInParmProbs		OBJID(GP_icmp,5)
#define ID_icmpInSrcQuenchs		OBJID(GP_icmp,6)
#define ID_icmpInRedirects		OBJID(GP_icmp,7)
#define ID_icmpInEchos			OBJID(GP_icmp,8)
#define ID_icmpInEchoReps		OBJID(GP_icmp,9)
#define ID_icmpInTimestamps		OBJID(GP_icmp,10)
#define ID_icmpInTimestampReps		OBJID(GP_icmp,11)
#define ID_icmpInAddrMasks		OBJID(GP_icmp,12)
#define ID_icmpInAddrMaskReps		OBJID(GP_icmp,13)
#define ID_icmpOutMsgs			OBJID(GP_icmp,14)
#define ID_icmpOutErrors		OBJID(GP_icmp,15)
#define ID_icmpOutDestUnreachs		OBJID(GP_icmp,16)
#define ID_icmpOutTimeExcds		OBJID(GP_icmp,17)
#define ID_icmpOutParmProbs		OBJID(GP_icmp,18)
#define ID_icmpOutSrcQuenchs		OBJID(GP_icmp,19)
#define ID_icmpOutRedirects		OBJID(GP_icmp,20)
#define ID_icmpOutEchos			OBJID(GP_icmp,21)
#define ID_icmpOutEchoReps		OBJID(GP_icmp,22)
#define ID_icmpOutTimestamps		OBJID(GP_icmp,23)
#define ID_icmpOutTimestampReps		OBJID(GP_icmp,24)
#define ID_icmpOutAddrMasks		OBJID(GP_icmp,25)
#define ID_icmpOutAddrMaskReps		OBJID(GP_icmp,26)
#define MIB_icmpMAXCTR			26
/*
 * TCP group
 */
#define ID_tcp				OBJID(GP_tcp,0)
#define ID_tcpRtoAlgorithm		OBJID(GP_tcp,1) 
#define ID_tcpRtoMin			OBJID(GP_tcp,2) 
#define ID_tcpRtoMax			OBJID(GP_tcp,3) 
#define ID_tcpMaxConn			OBJID(GP_tcp,4) 
#define ID_tcpActiveOpens		OBJID(GP_tcp,5) 
#define ID_tcpPassiveOpens		OBJID(GP_tcp,6) 
#define ID_tcpAttemptFails		OBJID(GP_tcp,7) 
#define ID_tcpEstabResets		OBJID(GP_tcp,8) 
#define ID_tcpCurrEstab			OBJID(GP_tcp,9) 
#define ID_tcpInSegs			OBJID(GP_tcp,10) 
#define ID_tcpOutSegs			OBJID(GP_tcp,11) 
#define ID_tcpRetransSegs		OBJID(GP_tcp,12) 
#define ID_tcpInErrs			OBJID(GP_tcp,13) 
#define ID_tcpOutRsts			OBJID(GP_tcp,14) 
#define MIB_tcpMAXCTR			14
/*
 * TCP Connection Table
 */
#define	ID_tcpConnNumEnt		OBJID(GP_tcp,1025)
#define	ID_tcpConnTable			OBJID(GP_tcp,1026)
#define	ID_tcpConnEntry			OBJID(GP_tcp,1027)
/*
 * UDP group
 */
#define ID_udp				OBJID(GP_udp,0)
#define ID_udpInDatagrams		OBJID(GP_udp,1) 
#define ID_udpNoPorts			OBJID(GP_udp,2) 
#define ID_udpInErrors			OBJID(GP_udp,3) 
#define ID_udpOutDatagrams		OBJID(GP_udp,4)
#define MIB_udpMAXCTR			4
/*
 * UDP Listener Table
 */
#define	ID_udpLsnNumEnt			OBJID(GP_udp,1025)
#define	ID_udpLsnTable			OBJID(GP_udp,1026)
#define	ID_udpLsnEntry			OBJID(GP_udp,1027)
/*
 *	ioctl argument
 */
struct nmparms {
	int	objid;		/* object identifier */
	char	*buffer;	/* user buffer */
	int	*len;		/* size of user buffer */
};
/*
 *	Structure Definitions for MIB table entries
 */
typedef struct {
	int	ifIndex;
	char	ifDescr[64];
	int	ifType;
	int	ifMtu;
	gauge	ifSpeed;
	unsigned char	ifPhysAddress[8];
	int	ifAdmin;
	int	ifOper;
	TimeTicks ifLastChange;
	counter ifInOctets;
	counter ifInUcastPkts;
	counter ifInNUcastPkts;
	counter ifInDiscards;
	counter ifInErrors;
	counter ifInUnknownProtos;
	counter ifOutOctets;
	counter ifOutUcastPkts;
	counter ifOutNUcastPkts;
	counter ifOutDiscards;
	counter ifOutErrors;
	gauge   ifOutQlen;
} mib_ifEntry;

typedef struct {
	int	IfIndex;
	unsigned char	PhysAddr[8];
	ipaddr	NetAddr;
} mib_AtEntry;

typedef struct {
	ipaddr	Addr;
	int	IfIndex;
	ipaddr	NetMask;
	ipaddr	BcastAddr;
	int	ReasmMaxSize;
} mib_ipAdEnt;

typedef struct {
        int     IfIndex;
        unsigned char   PhysAddr[8]; /* This is realy a 6 char, with the fill */
				     /*	is 8 char                             */
        ipaddr  NetAddr;
	int	Type;
} mib_ipNetToMediaEnt;

#define INTM_OTHER	1
#define INTM_INVALID	2
#define INTM_DYNAMIC	3
#define INTM_STATIC	4

typedef struct {
	ipaddr	Dest;
	int	IfIndex;
	int	Metric1;
	int	Metric2;
	int	Metric3;
	int	Metric4;
	ipaddr	NextHop;
	int	Type;
	int	Proto;
	int	Age;
	ipaddr  Mask;
} mib_ipRouteEnt;

#define NMOTHER         1
#define NMINVALID       2
#define NMDIRECT        3
#define NMREMOTE        4



typedef struct {
	int	State;
	ipaddr	LocalAddress;
	short	LocalPort;
	ipaddr	RemAddress;
	short	RemPort;	
} mib_tcpConnEnt;

typedef struct {
	ipaddr	LocalAddress;
	short	LocalPort;
} mib_udpLsnEnt;
/*
 *	Definitions for event management
 */
#define	MAXEVINFO	40
#define MAXEVNTS	32
struct	event {
	struct	timeval	time;	/* timestamp */
	int	code;		/* event code */
	int	len;		/* amt of data in info */
	char	info[MAXEVINFO]; /* event specific information */
};

#ifdef	_KERNEL
struct	evrec {
	struct	event	ev;
	struct	evrec	*evnext;
};
#define	NMEVHDRLEN	sizeof(struct timeval) + 2*sizeof (int)
#endif	/* _KERNEL */
#endif	/* _SYS_MIB_INCLUDED */
