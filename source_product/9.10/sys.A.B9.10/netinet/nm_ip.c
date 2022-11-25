/*
 * $Header: nm_ip.c,v 1.6.83.5 93/11/11 12:39:18 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_ip.c,v $
 * $Revision: 1.6.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:39:18 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_ip.c $Revision: 1.6.83.5 $";
#endif

#include "../h/mib.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/mbuf.h"
#include "../h/malloc.h"
#include "../h/socket.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/mib_kern.h"


#define NM_IPCANFORW     1
#define NM_IPCANNOTFORW  2

#define NM_DELETE 0
#define NM_CREATE 1


#define satosinp(a) ((struct sockaddr_in *)(a))

/*
 * 	Global Variables :IP counters for Network Management
 */
counter MIB_ipcounter[MIB_ipMAXCTR+1]={0};
int	MIB_ipRouteNumEnt;
int	MIB_ipAddrNumEnt;
extern	int	ipforwarding;
extern	struct  in_ifaddr *in_ifaddr; 
/*
 *	Subsystem nmget_ip() routine
 */
nmget_ip (id, ubuf, klen)
	int	id;
	char	*ubuf;
	int	*klen;
{
	int	status=0,nbytes=0;
	int	nument=0;
	int	s;
	short	mallocf=0;			/* flag if MALLOC called */
	char 	*kbuf;
	register struct in_ifaddr  *ia;

	switch (id) {

	case 	ID_ip:

		nbytes = MIB_ipMAXCTR * sizeof(counter);
		if (*klen < nbytes)
			return (EMSGSIZE);

		if (( nm_activeifs() > 1 ) && ipforwarding )

	 		MIB_ipcounter[ID_ipForwarding & INDX_MASK] = 
						NM_IPCANFORW ;
		else
	 		MIB_ipcounter[ID_ipForwarding & INDX_MASK] = 
						NM_IPCANNOTFORW ;

		MIB_ipcounter[ID_ipDefaultTTL & INDX_MASK] = (counter) ipDefaultTTL;
		kbuf = (char *) &MIB_ipcounter[1];
		*klen = nbytes;
		status = NULL;
		break;

	case	ID_ipAddrNumEnt :

		if (*klen < sizeof(int))
			return(EMSGSIZE);

		s = splnet();
		for (ia = in_ifaddr; ia; ia=ia->ia_next)
			nument ++;
		splx(s);
		kbuf = (char *) &nument;
		*klen	= sizeof (nument);
		status = NULL;
		break;

	case	ID_ipAddrTable :
		nument = *klen / sizeof (mib_ipAdEnt);
		if (nument <1)
			return (EMSGSIZE);

		nbytes = nument * sizeof(mib_ipAdEnt);
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nbytes, M_TEMP, M_WAITOK); 
		mallocf = 1;
		status = nmget_ipAddrTable((mib_ipAdEnt *) kbuf, nument, klen);
		break;

	case	ID_ipAddrEntry :
		if (*klen < sizeof(mib_ipAdEnt))
			return (EMSGSIZE);

		MALLOC( kbuf, char *, sizeof(mib_ipAdEnt), M_TEMP, M_WAITOK);
		mallocf = 1;
		if (status = copyin (ubuf, kbuf, sizeof(mib_ipAdEnt)))
			break;
		status = nmget_ipAddrEntry((mib_ipAdEnt *) kbuf, klen);
		break;

	case	ID_ipRouteNumEnt :
		
		if (*klen < sizeof(int))
			return (EMSGSIZE);

		kbuf = (char *) &MIB_ipRouteNumEnt;
		*klen = sizeof (int);
		status = NULL;
		break;

	case	ID_ipRouteTable :
		
		nument	= *klen / sizeof(mib_ipRouteEnt);
		if (nument <1)
			return (EMSGSIZE);

		if (nument > MIB_ipRouteNumEnt)
			nument=MIB_ipRouteNumEnt;
		nbytes = nument * sizeof(mib_ipRouteEnt);

		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nbytes, M_TEMP, M_WAITOK); 
		mallocf = 1;
		status = nmget_ipRouteTable((mib_ipRouteEnt *) kbuf, nument, klen);
		break;

	case	ID_ipRouteEntry :
		if( *klen < sizeof(mib_ipRouteEnt))
			return(EMSGSIZE);

		MALLOC ( kbuf, char *, sizeof(mib_ipRouteEnt), M_TEMP, M_WAITOK);
		mallocf = 1;
		if (status = copyin (ubuf, kbuf, sizeof(mib_ipRouteEnt)))
			break;
		status = nmget_ipRouteEntry((mib_ipRouteEnt *) kbuf, klen);
		break;

        case    ID_ipNetToMediaTableNum:        /* return number of entries */

                if ( *klen < sizeof(int))
                        return (EMSGSIZE);


   		nument = nmget_ipNetToMediaNum();
                kbuf = (char *) &nument;
                *klen = sizeof (int);
                status = NULL;
                break;


	case 	ID_ipNetToMediaTableEnt:	     /* return specific entry */

                if ( *klen < sizeof(mib_ipNetToMediaEnt))
                        return (EMSGSIZE);

                MALLOC ( kbuf, char *, sizeof(mib_ipNetToMediaEnt), 
					M_TEMP, M_WAITOK);
                mallocf = 1;
                if (status = copyin (ubuf, kbuf, sizeof(mib_ipNetToMediaEnt)))
                        break;


                status = nmget_ipNetToMediaEntry((mib_ipNetToMediaEnt*)kbuf,
                               klen);
                break;
 
	
	case 	ID_ipNetToMediaTable:        /* return all completed entries */

                if ((nument = *klen/sizeof(mib_ipNetToMediaEnt)) < 1)
                        return (EMSGSIZE);

                nbytes = nument * sizeof(mib_ipNetToMediaEnt);
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nbytes, M_TEMP, M_WAITOK); 
                mallocf = 1;

		status = nmget_ipNetToMediaTable((mib_ipNetToMediaEnt*)kbuf,
				nument,klen);
                break;


	default	:	/* get individual IP counter */
		if ((id & INDX_MASK) > MIB_ipMAXCTR) 
			return(EINVAL);

		if (*klen < sizeof(counter))
			return (EMSGSIZE);

		switch (id) {
		case ID_ipForwarding :
	                if (( nm_activeifs() > 1 ) && ipforwarding )
	                        MIB_ipcounter[ID_ipForwarding & INDX_MASK] =
                                                	NM_IPCANFORW ;
                	else
                        	MIB_ipcounter[ID_ipForwarding & INDX_MASK] =
                                                	NM_IPCANNOTFORW ;

			kbuf = (char *) &MIB_ipcounter[ID_ipForwarding & 
								INDX_MASK];
			break;

		case ID_ipDefaultTTL:
			/*   Store in integer array because ipDefaultTTL was
			     declared as u_char and we are going to copyout
			     4 bytes.	T. Ngo
			*/
			MIB_ipcounter[ID_ipDefaultTTL & INDX_MASK] = (counter) ipDefaultTTL;
			kbuf = (char *) &MIB_ipcounter[id&INDX_MASK];
			break;

		default : 
			kbuf = (char *) &MIB_ipcounter[id&INDX_MASK];
		}
		*klen = sizeof (counter);
		status = NULL;
	}
	if (status==NULL)
		status = copyout (kbuf, ubuf, *klen);
	if (mallocf)
		FREE(kbuf, M_TEMP);
	return (status);
}

/*
 *	Get entire IP Address Table
 */
nmget_ipAddrTable(kbuf, nument, klen) 
	mib_ipAdEnt	*kbuf;		/* kernel buffer */
	int	nument;		/* # entries that can fit in kbuf */
	int	*klen;		/* size of user buffer */
{
	register struct ifnet		*ifp;
	register struct in_ifaddr	*ia;
	int	actent=0;
	int	s;

	s = splnet();
	for (ifp = ifnet; ifp && (actent<nument); ifp = ifp->if_next) {

		/* Find struct in_ifaddr for interface	*/
		for (ia = in_ifaddr; ia; ia=ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;
		if (ia) {
			kbuf->Addr 	= IA_SIN(ia)->sin_addr.s_addr;
			kbuf->NetMask 	= ia->ia_subnetmask;
			kbuf->BcastAddr = ia->ia_netbroadcast.s_addr;
			kbuf->IfIndex 	= ifp->if_index;
			kbuf->ReasmMaxSize = IP_MAXPACKET;
			kbuf ++;
			actent ++;
		}
	}
	splx(s);
	*klen = actent * sizeof(mib_ipAdEnt);
	return (NULL);
}

/*
 *	Get specific IP Address Entry
 */
nmget_ipAddrEntry(kbuf, klen) 
	mib_ipAdEnt	*kbuf;		/* user space */
	int	*klen;
{
	register struct ifnet		*ifp;
	register struct in_ifaddr	*ia;
	int	s;

	s = splnet();
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {

		/* Find struct in_ifaddr for interface	*/
		for (ia = in_ifaddr; ia; ia=ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;

		/* Check for matching IP address	*/
		if ((ia) && (IA_SIN(ia)->sin_addr.s_addr == kbuf->Addr))
			break;
	}
	splx(s);
	if (ifp==0)
		return (ENOENT);
	else {
		kbuf->Addr 	= IA_SIN(ia)->sin_addr.s_addr;
		kbuf->NetMask 	= ia->ia_subnetmask;
		kbuf->BcastAddr = ia->ia_netbroadcast.s_addr;
		kbuf->IfIndex 	= ifp->if_index;
		kbuf->ReasmMaxSize = IP_MAXPACKET;
		*klen = sizeof(mib_ipAdEnt);
		return (NULL);
	}
}

/*
 *	get entire IP Routing Table
 */
nmget_ipRouteTable(kbuf, nument, klen)
	mib_ipRouteEnt	*kbuf;	/* kernel buffer */
	int	nument;		/* #entries that can fit in kbuf */
	int	*klen;		/* size of user buffer */
{
	int	 actent=0;
	int	 s, doinghost;
	struct	 rtentry	**table;
	register struct	rtentry	*rt;
	register struct in_ifaddr	*ia;
	int	 i;

	doinghost =1;
	table = rthost;

	s = splnet();
again:	
	for (i=0; i< RTHASHSIZ; i++) {
	    for (rt=table[i]; rt && (actent<nument); rt=rt->rt_next) {
		if (rt->rt_dst.sa_family != AF_INET)
				continue;

		kbuf->Dest	= satosinp(&(rt->rt_dst))->sin_addr.s_addr;
		kbuf->IfIndex	= rt->rt_ifp->if_index;
		kbuf->Metric1	= -1;
		kbuf->Metric2	= -1;
		kbuf->Metric3	= -1;
		kbuf->Metric4	= -1;
		kbuf->NextHop	= satosinp(&(rt->rt_gateway))->sin_addr.s_addr;
		kbuf->Proto	= rt->rt_proto;
		kbuf->Age	= time.tv_sec - rt->rt_upd;

		/*  Figure out ipRouteType	*/

		if ((rt->rt_flags & RTF_UP) == 0 ||
		    (rt->rt_ifp->if_flags & IFF_UP) == 0)
			kbuf->Type = NMINVALID;
		else {
			if (rt->rt_flags & RTF_HOST)
				kbuf->Type = NMDIRECT;
			else
				kbuf->Type = NMREMOTE;
			if (rt->rt_flags & RTF_GATEWAY)
				kbuf->Metric1 = 1;
		}

		/*  Figure out ipRouteMask	*/

		for (ia = in_ifaddr; ia; ia=ia->ia_next)
			if (ia->ia_ifp == rt->rt_ifp)
				break;
		if (ia)
			kbuf->Mask = ia->ia_subnetmask;
		else    kbuf->Mask = (ipaddr) 0;

		if (kbuf->Dest == (ipaddr) 0)
			kbuf->Mask = (ipaddr) 0;
		kbuf++;
		actent++;
	    }
	}

	if (doinghost) {
		doinghost = 0;
		table = rtnet;
		goto again;
	}

	splx(s);
	*klen = actent * sizeof(mib_ipRouteEnt);
	return (NULL);
}

/*
 *	Find routing entry with perfect matching destination
 *
 *	NOTE : The destination must match every byte because of the following
 *	scenario.  Assume the routing table contains a default route going
 *	through gateway 15.13.104.1.  If nm_findrt() returns a route that 
 *	satisfies af_netmatch function, and the user does a SET to change 
 *	the route for destination 15.13.104.123 to go through gateway to 
 *	15.13.95.2.  This will change the default route, not just for the 
 *	destination 15.13.104.123.  This is NOT what we want.
 */
nm_findrt(dst,rtentry)
	struct	sockaddr dst;
	struct	rtentry	**rtentry;
{
	short	af;
	int	s, doinghost;
	struct	rtentry	*rt;
	u_long	hash;
	struct	rtentry	**table;
	struct	afhash	h;

	af = dst.sa_family;
	(*afswitch[af].af_hash)(&dst,&h);

	hash	= h.afh_hosthash;
	doinghost =1; table = rthost;

	s = splnet();
again:	
	for (rt = table[RTHASHMOD(hash)]; rt; rt = rt->rt_next) {
		if (rt->rt_dst.sa_family != af)
			continue;
		if (rt->rt_hash != hash)
			continue;
		if (bcmp((caddr_t)&rt->rt_dst, (caddr_t)&dst,
				sizeof(dst))) continue;
		splx(s);
		*rtentry = rt;
		return(NULL);
	}
	if (doinghost) {
		doinghost = 0; table = rtnet;
		hash = h.afh_nethash; 
		goto again;
		}
	splx(s);
	*rtentry = NULL;
	return (ENOENT);		/* Not found */
}

/*
 *	Get specific entry in IP Routing Table
 */
nmget_ipRouteEntry(kbuf, klen)
	mib_ipRouteEnt	*kbuf;
	int	*klen;
{
	struct	rtentry	*rt=NULL;
	struct	rtentry	**table;
	register struct in_ifaddr	*ia;
	static 	struct sockaddr dst={AF_INET};

	dst.sa_family = AF_INET;
	satosinp(&dst)->sin_addr.s_addr = kbuf->Dest;

	nm_findrt(dst, &rt);
	if (rt==NULL)
		return (ENOENT);

	kbuf->IfIndex	= rt->rt_ifp->if_index;
	kbuf->Metric1	= -1;
	kbuf->Metric2	= -1;
	kbuf->Metric3	= -1;
	kbuf->Metric4	= -1;
	kbuf->NextHop	= satosinp(&(rt->rt_gateway))->sin_addr.s_addr;
	kbuf->Proto	= rt->rt_proto;
	kbuf->Age	= time.tv_sec - rt->rt_upd;

	/*  Figure out ipRouteType	*/

	if ((rt->rt_flags & RTF_UP) == 0 ||
			(rt->rt_ifp->if_flags & IFF_UP) == 0)
		kbuf->Type = NMINVALID;
	else {
		if (rt->rt_flags & RTF_HOST)
			kbuf->Type = NMDIRECT;
		else 
			kbuf->Type = NMREMOTE;
		if (rt->rt_flags & RTF_GATEWAY)
			kbuf->Metric1 = 1;
	}

	/*  Figure out ipRouteMask	*/

	for (ia = in_ifaddr; ia; ia=ia->ia_next)
		if (ia->ia_ifp == rt->rt_ifp)
			break;
	if (ia)
		kbuf->Mask = ia->ia_subnetmask;
	else    kbuf->Mask = (ipaddr) 0;

	if (kbuf->Dest == (ipaddr) 0)
		kbuf->Mask = (ipaddr) 0;

	*klen = sizeof(mib_ipRouteEnt);
	return (NULL);
}

/*
 *	Subsystem set() routine
 */
nmset_ip (id, ubuf, klen)
	int	id;		/* object identifier */
	char	*ubuf;		/* user buffer 	*/
	int	*klen;		/* length in kernel space */
{
	int	status=0;
	int	ipttl=0;
	mib_ipRouteEnt	kbuf;
	struct	rtentry	*rt=NULL;
	static	struct	rtentry	entry;
	short	flags;
	int     ipForw_value;

	extern	int	rtrequest();

	switch (id) {

	case 	ID_ipDefaultTTL :

		if (*klen < sizeof(int))
			return (EMSGSIZE);

		if (status = copyin(ubuf, &ipttl, sizeof(int)))
			return(status);

		if ( ipttl < 1 || ipttl > 255)
			return (EINVAL);

		ipDefaultTTL = ipttl;
		status = NULL;
		break;

	case    ID_ipForwarding :

                if (*klen < sizeof(int))
                        return (EMSGSIZE);

                if (status = copyin(ubuf, &ipForw_value, sizeof(int)))
                        return(status);

		if (ipForw_value == NM_IPCANNOTFORW ){
			ipforwarding = 0;
			}
		else {
                	if ( nm_activeifs() > 1 ) 
				ipforwarding = 1;
			else
				return(EINVAL);
		}
		break;


	case 	ID_ipRouteEntry :
	
		if( *klen < sizeof(mib_ipRouteEnt))
			return(EMSGSIZE);

		if (status = copyin(ubuf, &kbuf, sizeof(mib_ipRouteEnt)))
			return(status);

		entry.rt_dst.sa_family = AF_INET;
		satosinp(&(entry.rt_dst))->sin_addr.s_addr =  kbuf.Dest;

		/*  
		The NextHop supplied by user represents the new gateway.  
		We need the gateway in the existing route entry before 
		we can call rtrequest(SIOCDELRT).  So we first find
		the existing route entry.  
		*/
		nm_findrt(entry.rt_dst, &rt);
		if (rt==NULL)
			return (ENOENT);

		flags = rt->rt_flags;
		if (status = rtrequest(SIOCDELRT, rt))
			return (status);
		if (kbuf.Type == NMINVALID)
			break;

		entry.rt_gateway.sa_family = AF_INET;
		satosinp(&(entry.rt_gateway))->sin_addr.s_addr = kbuf.NextHop;
		entry.rt_proto	= NMMGMT;
	        entry.rt_flags  = RTF_UP;
       		if (kbuf.Type == NMDIRECT)
                	entry.rt_flags  |= RTF_HOST;
        	if (kbuf.Metric1 > 0 )
               		entry.rt_flags  |= RTF_GATEWAY;

		status = rtrequest(SIOCADDRT, &entry);
		break;

	default :
		return (EOPNOTSUPP);
	}
	return (status);
}

nmcreate_ip(id, ubuf, klen)
	int	id;
	char	*ubuf;
	int	*klen;
{
	int	status=0;

	/* currently only supported for ipRouteTable and ipNetToMediaTable */

	switch(id) {
	
	case ID_ipRouteTable:

		status =  nmcreate_ipRoute((mib_ipRouteEnt *)ubuf, klen);
		break;

	case ID_ipNetToMediaTable:

		status=nmcrOrdel_ipNetToMedia( NM_CREATE , 
			(mib_ipNetToMediaEnt *)ubuf, klen);
		break;

	default :
		return(EINVAL);
	
	}
	return (status);
}

nmdelete_ip(id, ubuf, klen)
	int	id;
	char	*ubuf;
	int	*klen;
{
	int	status=0;


	switch(id) {
	
	case ID_ipRouteTable:

		status =  nmdelete_ipRoute((mib_ipRouteEnt *)ubuf, klen);
		break;

	case ID_ipNetToMediaTable:

		status= nmcrOrdel_ipNetToMedia( NM_DELETE , 
			(mib_ipNetToMediaEnt *)ubuf, klen);
		break;

	default :
		return(EINVAL);
	
	}
	return (status);
}


int	
nmcreate_ipRoute ( ubuf, klen)
        mib_ipRouteEnt    *ubuf;          /* user buffer  */
        int      	  *klen;          /* length in kernel space */

{
        int     status=0;
        mib_ipRouteEnt  kbuf;
        struct  rtentry entry;

        extern  int     rtrequest();

	if( *klen < sizeof(mib_ipRouteEnt))
		return(EMSGSIZE);

	if (status = copyin(ubuf, &kbuf, sizeof(mib_ipRouteEnt)))
		return(status);

	if (kbuf.Type == NMINVALID)
		return;

	bzero(&entry, sizeof(struct rtentry));
	entry.rt_dst.sa_family = AF_INET;
	entry.rt_gateway.sa_family = AF_INET;
	satosinp(&(entry.rt_dst))->sin_addr.s_addr =  kbuf.Dest;
	satosinp(&(entry.rt_gateway))->sin_addr.s_addr = kbuf.NextHop;
	entry.rt_proto	= NMMGMT;

        entry.rt_flags  = RTF_UP;
        if (kbuf.Type == NMDIRECT)
                entry.rt_flags  |= RTF_HOST;
        if (kbuf.Metric1 > 0 )
                entry.rt_flags  |= RTF_GATEWAY;

	status = rtrequest(SIOCADDRT, &entry);
	return(status);
}

int	
nmcrOrdel_ipNetToMedia (cmd, ubuf, klen)
	int			 cmd;		 /* delete or create */
        mib_ipNetToMediaEnt      *ubuf;          /* user buffer  */
        int     		 *klen;          /* length in kernel space */

{
        int     status=0;
        mib_ipNetToMediaEnt  kbuf;
        static  struct  arpreq req;
        struct  arpreq *ar=&req;
	int arpreq_cmd;
	struct ifaddr *ifa;
	struct sockaddr_in *sin;
        
	extern  int     arpioctl();



        if( *klen < sizeof(mib_ipNetToMediaEnt))
                return(EMSGSIZE);

        if (status = copyin(ubuf, &kbuf, sizeof(mib_ipNetToMediaEnt)))
                return(status);

	req.arp_pa.sa_family  = AF_INET;
	req.arp_ha.sa_family  = AF_UNSPEC;
	bcopy ((caddr_t)kbuf.PhysAddr, (caddr_t)req.arp_ha.sa_data, 
		sizeof(kbuf.PhysAddr));
	sin = (struct sockaddr_in *)&ar->arp_pa;
	bcopy ((caddr_t)&kbuf.NetAddr, (caddr_t)&sin->sin_addr.s_addr, 
		sizeof(kbuf.NetAddr));
	req.arp_flags = ATF_COM;

        if ((ifa = ifa_ifwithnet(&ar->arp_pa)) == NULL ||
		(ifa->ifa_ifp->if_index != kbuf.IfIndex )) 
                        return (ENETUNREACH);

	if (kbuf.Type == INTM_INVALID)
		arpreq_cmd = SIOCDARP;
	else
		arpreq_cmd = cmd == NM_DELETE ? SIOCDARP : SIOCSARP ;

	status = arpioctl(arpreq_cmd, ar);
	return(status);

}

int	
nmdelete_ipRoute ( ubuf, klen)
        mib_ipRouteEnt    *ubuf;          /* user buffer  */
        int     	  *klen;          /* length in kernel space */

{


        int     status=0;
        mib_ipRouteEnt  kbuf;
        struct  rtentry entry;

        extern  int     rtrequest();

	
	if( *klen < sizeof(mib_ipRouteEnt))
		return(EMSGSIZE);

	if (status = copyin(ubuf, &kbuf, sizeof(mib_ipRouteEnt)))
		return(status);

	bzero(&entry, sizeof(struct rtentry));
	entry.rt_dst.sa_family = AF_INET;
	entry.rt_gateway.sa_family = AF_INET;
	satosinp(&(entry.rt_dst))->sin_addr.s_addr =  kbuf.Dest;
	satosinp(&(entry.rt_gateway))->sin_addr.s_addr = kbuf.NextHop;
	entry.rt_flags  = RTF_UP;
	if (kbuf.Type == NMDIRECT)
		entry.rt_flags 	|= RTF_HOST;
	if (kbuf.Metric1 > 0 )
		entry.rt_flags 	|= RTF_GATEWAY;
		

	if (status = rtrequest(SIOCDELRT, &entry))
		return (status);

}

int
nm_activeifs()
/* This routine counts the number of active interfaces (those which are UP)
   excluding loopback                                                      */

{
        struct in_ifaddr *ia;
        int ifcount = 0;

        for (ia = in_ifaddr; ia; ia = ia->ia_next) {
                if ( (ia->ia_ifa.ifa_ifp->if_flags & IFF_UP) &&
                     (bcmp(ia->ia_ifa.ifa_ifp->if_name, "lo",2 ) ) )
                        ifcount++;
        }
        return (ifcount);
}


