/*
 * $Header: nm_if.c,v 1.5.83.5 93/11/11 12:25:37 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/nm_if.c,v $
 * $Revision: 1.5.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:25:37 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_if.c $Revision: 1.5.83.5 $";
#endif

#include "../h/mib.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/malloc.h"
#include "../h/socket.h"

#include "../net/if.h"
#include "../netinet/mib_kern.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

extern	int   MIB_ifNumber;
/*
 *	Get routine for Interfaces Group
 */
nmget_if (id, ubuf, klen)
	int  id; 		/* object identifier */
	char *ubuf;             /* user buffer in which to store data 	*/
	int  *klen ;		/* size of user buffer , in kernel	*/

{
	int 	status=0, nument=0;
	int	ifindex=0;
	int	flag=0;		/* set if MALLOC is called */
	char 	*kbuf;		

	switch (id) {

	case ID_ifNumber :

   		if (*klen < sizeof (counter)) 
			return (EMSGSIZE);

		kbuf = (char *) &MIB_ifNumber;
   		*klen = sizeof (int);
   		status = NULL;
	   	break;

	case ID_ifEntry :
		
		if (*klen < sizeof(mib_ifEntry))
			return (EMSGSIZE);

		if (status = copyin (ubuf, &ifindex, sizeof(ifindex)))
			return (status);
		if (ifindex > MIB_ifNumber)
			return (EINVAL);

		MALLOC( kbuf, char *, sizeof(mib_ifEntry), M_TEMP, M_WAITOK);
		flag = 1;

		((mib_ifEntry *) kbuf)->ifIndex = ifindex;
		status = nmget_ifEntry((mib_ifEntry *) kbuf, klen);
		break;

	case ID_ifTable :

		if (*klen < sizeof(mib_ifEntry))
			return (EMSGSIZE);

		nument = *klen/sizeof(mib_ifEntry);
		if (nument > MIB_ifNumber)
			nument = MIB_ifNumber;

		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nument*sizeof(mib_ifEntry),
					M_TEMP, M_WAITOK); 
		flag =1;
		status = nmget_ifTable((mib_ifEntry *) kbuf, nument, klen);
		break;

	default :
		status = EINVAL;
      	}

	if (status==NULL)
		status = copyout(kbuf, ubuf, *klen);
	if (flag) 
		FREE(kbuf, M_TEMP);
	return (status);
}
/*
 *  	Get a specific ifEntry 
 */
nmget_ifEntry (kbuf, klen)
	mib_ifEntry	*kbuf;		/* kernel buffer */
	int		*klen;		/* size of returned data */
{
	int	index=0, status=0;
	int	s;
	struct ifnet	*ifp=ifnet;

	index = kbuf->ifIndex;

	/*	find ifnet with matching index	*/
	s = splnet();
	for (ifp=ifnet; ifp ; ifp=ifp->if_next)
		if(ifp->if_index == index) 
			break;
	if (!ifp) {
		splx(s);
		return(ENOENT);
	}

	/*	call driver  specific routine	*/
	if (!(ifp->if_control)) {
		splx(s);
		return (EOPNOTSUPP);
	}

   	status = (*(ifp->if_control)) (ifp,IFC_IFNMGET, kbuf);
	if (status==NULL)
		*klen = sizeof(mib_ifEntry);
	else	
		*klen = 0;

	splx(s);
   	return (status);
}
/*
 *	Get entire ifTable
 */
nmget_ifTable (kbuf, nument, klen)
	mib_ifEntry 	*kbuf;		/* kernel buffer */
	int		nument;		/* # entries that can fit in kbuf */
	int		*klen;		/* size of buffer */
{
	int		actent=0;		/* # of entries */
	int		status=0;
	int		s;
	struct ifnet	*ifp;
	mib_ifEntry	*save;

	s = splnet();
	for (ifp=ifnet; ifp && (actent<nument); ifp=ifp->if_next) {
	    	if (ifp->if_control) {
			status = (*(ifp->if_control)) (ifp, IFC_IFNMGET, kbuf);
			if (status==NULL) {
				actent++;
				kbuf ++;
			}
		}
	}
	splx(s);
	*klen = actent * sizeof(mib_ifEntry);
	return (NULL);
}
/*
 *	Set routine for Interfaces Group
 */
nmset_if (id, ubuf, klen)
	int  id; 		/* object identifier */
	char *ubuf;             /* user buffer */
	int  *klen ;		/* size of ubuf, in kernel */
{
	int	status=0,ifindex=0;
	int	flag=0;		/* flag if MALLOC called */
	int	s;
	struct ifnet	*ifp;
	mib_ifEntry	*kbuf;

	switch(id) {

	case ID_ifEntry :
		
		if (*klen < sizeof(mib_ifEntry))
			return (EMSGSIZE);

		if (status = copyin (ubuf, &ifindex, sizeof(ifindex)))
			return (status);

		if (ifindex > MIB_ifNumber)
			return (EINVAL);

		/*	find ifnet with matching index	*/
		s = splnet();
		for (ifp=ifnet; ifp ; ifp=ifp->if_next)
			if(ifp->if_index == ifindex) 
				break;
		if (ifp == NULL) {
			splx(s);
			return(ENOENT);
		}

		/*	call driver  specific routine	*/
		if (ifp->if_control == NULL) {
			splx(s);
			return (EOPNOTSUPP);
		}

		MALLOC( kbuf, mib_ifEntry *, sizeof(mib_ifEntry), M_TEMP, M_WAITOK);
		flag = 1;
		if (status = copyin (ubuf, kbuf, sizeof(mib_ifEntry))) {
			splx(s);
			break;
		}
   		status = (*(ifp->if_control)) (ifp,IFC_IFNMSET, kbuf);
		splx(s);
		break;

	default	:
		return (EINVAL);
	}
	if (flag)
		FREE (kbuf, M_TEMP);
	return (status);
}

extern struct timeval sysInit;
extern struct timeval ms_gettimeofday();

if_mibevent(ifp, event_id, mif)
	struct ifnet *ifp;
	int event_id;
	mib_ifEntry *mif;
{
	extern          nmevenq();
	struct evrec    *ev;
	struct timeval	diff;
        struct timeval  time;
	int    status   = 0;
	int    s;

        s = splimp();

	if (event_id == NMV_LINKUP && !(ifp->if_flags & IFF_UP)) 
		mif->ifOper = LINK_UP;
        else if (event_id == NMV_LINKDOWN && ifp->if_flags & IFF_UP)
		mif->ifOper = LINK_DOWN;
        else	{
        	splx (s);
		return (0);
	}
        splx (s);

        time = ms_gettimeofday();
	diff.tv_sec = time.tv_sec - sysInit.tv_sec;
	diff.tv_usec = time.tv_usec - sysInit.tv_usec;

	if (diff.tv_usec < 0) {
		diff.tv_sec --;
		diff.tv_usec += 1000000;
        }

	mif->ifLastChange = (TimeTicks) 100*diff.tv_sec + (diff.tv_usec)/10000;

	MALLOC(ev, struct evrec *, sizeof (struct evrec), M_TEMP, M_NOWAIT);

	if (!ev)
		return(ENOMEM);

	ev->ev.code = event_id;

	(*(int *)(ev->ev.info)) = mif->ifIndex;
	ev->ev.len = MAXEVINFO; 
        nmevenq(ev);
	return(status);
}
