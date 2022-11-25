/*
 * $Header: nm_udp.c,v 1.4.83.5 93/11/11 12:39:30 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_udp.c,v $
 * $Revision: 1.4.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:39:30 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_udp.c $Revision: 1.4.83.5 $";
#endif

#include "../h/mib.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/socket.h"
#include "../h/malloc.h"

#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"

extern  struct inpcb udb;

/* 	UDP counters */
counter MIB_udpcounter[MIB_udpMAXCTR+1]={0};
int	MIB_udpLsnNumEnt=0;
/*
 *	Get routine for UDP Group
 */
nmget_udp(id, ubuf, klen)
	int	id;		/* object identifier */
	char	*ubuf;		/* user buffer */
	int	*klen;		/* size of ubuf, in kernel */
{
	int	status=0,nbytes=0;
	int	nument=0,mallocf=0;
	char	*kbuf;

	switch (id) {

	case ID_udp :		/* get all udp counters */

		nbytes = MIB_udpMAXCTR * sizeof(counter);
		if (*klen < nbytes)
			return (EMSGSIZE);

		kbuf = (char *) &MIB_udpcounter[1];
		*klen = nbytes;
		status = NULL;
		break;

	case ID_udpLsnNumEnt :
		kbuf = (char *) &MIB_udpLsnNumEnt;
		*klen = sizeof(int);
		status = NULL;
		break;

	case ID_udpLsnTable :
		if (*klen < sizeof(mib_udpLsnEnt))
			return (EMSGSIZE);

		nument = *klen / sizeof(mib_udpLsnEnt);
		if (nument > MIB_udpLsnNumEnt)
			nument = MIB_udpLsnNumEnt;

		nbytes = nument * sizeof(mib_udpLsnEnt);
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nbytes, M_TEMP, M_WAITOK); 
		mallocf =1;
		status = nmget_udpLsnTable((mib_udpLsnEnt *) kbuf, klen, nument);
		break;

	case ID_udpLsnEntry :
		if (*klen < sizeof(mib_udpLsnEnt))
			return (EMSGSIZE);

		MALLOC( kbuf, char *, sizeof(mib_udpLsnEnt), M_TEMP, M_WAITOK);
		mallocf = 1;
		if (status = copyin(ubuf, kbuf, sizeof(mib_udpLsnEnt)))
			break;
		status = nmget_udpLsnEntry((mib_udpLsnEnt *) kbuf, klen);
		break;

	default :		/* get 1 udp counter */

		if ((id&INDX_MASK) > MIB_udpMAXCTR)
			return (EINVAL);
		if (*klen < sizeof(counter))
			return (EMSGSIZE);

		kbuf = (char *) &MIB_udpcounter[id & INDX_MASK];
		*klen = sizeof (counter);
		status = NULL;
	};
	if (status==NULL)
		status = copyout(kbuf, ubuf, *klen);
	if (mallocf)
		FREE(kbuf, M_TEMP);
	return (status);
}
/*
 *	get UDP Listener Table
 */
nmget_udpLsnTable(bufp, klen, nument)
	mib_udpLsnEnt	*bufp;		/* kernel buffer */
	int	*klen;		/* size of user buffer, in kernel */
	int	nument;		/* # entries that can fit in kbuf */
{
	int	actent=0, s=0;
	struct	inpcb	*inp;

	s = splnet();
	for (inp=udb.inp_next; (inp!=&udb) && (actent<nument); inp = inp->inp_next) {
		bufp->LocalAddress 	= inp->inp_laddr.s_addr;
		bufp->LocalPort 	= inp->inp_lport;
		bufp++;
		actent++;
	}
	splx(s);

	*klen = actent * sizeof(mib_udpLsnEnt);
	return (NULL);
}
/*
 *	Get UDP Listener Entry
 */
nmget_udpLsnEntry(bufp, klen)
	mib_udpLsnEnt	*bufp;
	int	*klen;
{
	struct	inpcb	*inp;
	int	s=0;

	if (*klen < sizeof (mib_udpLsnEnt)) 
		return (EMSGSIZE);

	s = splnet();
	for (inp=udb.inp_next; (inp!=&udb); inp = inp->inp_next) {
		if ((bufp->LocalAddress == inp->inp_laddr.s_addr) &&
		    (bufp->LocalPort == inp->inp_lport))
			break; 
	}
	splx(s);	
	if (inp==&udb) 
		return (ENOENT);
	else {
		*klen = sizeof(mib_udpLsnEnt);
		return (NULL);
	} 
}
