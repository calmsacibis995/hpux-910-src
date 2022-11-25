/*
 * $Header: nm_tcp.c,v 1.3.83.5 93/11/11 12:39:25 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_tcp.c,v $
 * $Revision: 1.3.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 12:39:25 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_tcp.c $Revision: 1.3.83.5 $";
#endif

#include "../h/mib.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/socket.h"
#include "../h/malloc.h"

#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"

/* 	TCP counters 		*/
counter MIB_tcpcounter[MIB_tcpMAXCTR+1]={0};
int	MIB_tcpConnNumEnt=0;
/*
 *	Map 4.3 BSD TCP states to RFC 1066 TCP states
 */
/***  	Without SYN_HOLD	
int	tcpStateXlate[]={1,2,3,4,5,8,6,10,9,7,11};
***/

/***  	With SYN_HOLD	***/
int	tcpStateXlate[]={1,2,3,2,4,5,8,6,10,9,7,11};
/*
 *	Subsystem nmget_tcp routine
 */
nmget_tcp(id, ubuf, klen)
	int	id;			/* object identifier */
	char	*ubuf;			/* user buffer	*/
	int	*klen;			/* size of ubuf, in kernel */
{
	int	status=0,nbytes=0;
	int	nument=0,estab=0;
	int	mallocf=0;			/* set if MALLOC called */
	char	*kbuf;

	switch (id) {

	case    ID_tcp :
	
		nbytes = MIB_tcpMAXCTR * sizeof(counter);
		if (*klen < nbytes)
			return (EMSGSIZE);

		MIB_tcpcounter[ID_tcpCurrEstab & INDX_MASK] = nmget_tcpCurrEstab();
		kbuf = (char *) &MIB_tcpcounter[1];
		*klen = nbytes;
		status = NULL;
		break;

	case	ID_tcpConnNumEnt :

		kbuf = (char *) &MIB_tcpConnNumEnt;
		*klen = sizeof(int);
		status = NULL;
		break;

	case	ID_tcpConnTable :

		if (*klen < sizeof(mib_tcpConnEnt))
			return (EMSGSIZE);

		nument = *klen / sizeof(mib_tcpConnEnt);
		if (nument > MIB_tcpConnNumEnt)
			nument = MIB_tcpConnNumEnt;

		nbytes = nument * sizeof(mib_tcpConnEnt);
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		kbuf = (char *) kmalloc(nbytes, M_TEMP, M_WAITOK); 
		mallocf =1;
		status = nmget_tcpConnTable((mib_tcpConnEnt *) kbuf, klen, nument);
		break;


	case	ID_tcpConnEntry :
		
		if (*klen < sizeof(mib_tcpConnEnt))
			return (EMSGSIZE);

		MALLOC( kbuf, char *, sizeof(mib_tcpConnEnt), M_TEMP, M_WAITOK);
		mallocf = 1;
		if (status = copyin(ubuf, kbuf, sizeof(mib_tcpConnEnt)))
			break;
		status = nmget_tcpConnEntry((mib_tcpConnEnt *) kbuf, klen);
		break;


	default :		/* get 1 tcp counter */

		if ((id&INDX_MASK) > MIB_tcpMAXCTR)
			return (EINVAL);

		if (*klen < sizeof(counter))
			return (EMSGSIZE);

		if (id==ID_tcpCurrEstab) {
			estab = nmget_tcpCurrEstab();
			kbuf = (char *) &estab;
		}
		else
			kbuf = (char *) &MIB_tcpcounter[id & INDX_MASK];
		*klen = sizeof (counter);
		status = NULL;
	}
	if (status == NULL)
		status = copyout(kbuf, ubuf, *klen);
	if (mallocf)
		FREE(kbuf, M_TEMP);
	return(status);
}
/*
 *	Count Number of TCP connections in ESTABLISHED or CLOSE_WAIT state
 */
nmget_tcpCurrEstab()
{
	register struct	inpcb	*ptr;
	register struct	tcpcb	*tcpptr;
	int	 estab=0;
	int	 s;

	s = splnet();
	for (ptr=tcb.inp_next; (ptr!= &tcb); ptr=ptr->inp_next) {
		tcpptr = (struct tcpcb *) ptr->inp_ppcb;
		if ((tcpptr->t_state == TCPS_ESTABLISHED) ||
		    (tcpptr->t_state == TCPS_CLOSE_WAIT ))
			estab ++;
	}
	splx(s);
	return (estab);
}
/*
 *	get entire TCP Connection Table
 */
nmget_tcpConnTable(bufp, klen, nument)
	mib_tcpConnEnt	*bufp;		/* kernel buffer */
	int	*klen;		/* size of user buffer, in kernel */
	int	nument;		/* # entries that can fit in kbuf */
{
	int	actent=0,s=0;
	struct	inpcb	*ptr;
	struct	tcpcb	*tcpptr;

	s = splnet();
	for (ptr=tcb.inp_next; (ptr!= &tcb) && (actent<nument); 
				ptr=ptr->inp_next) {
		tcpptr = (struct tcpcb *) ptr->inp_ppcb;
		bufp->State 		= tcpStateXlate[tcpptr->t_state];
		bufp->LocalAddress 	= ptr->inp_laddr.s_addr;
		bufp->RemAddress 	= ptr->inp_faddr.s_addr;
		bufp->LocalPort 	= ptr->inp_lport;
		bufp->RemPort 		= ptr->inp_fport;
		bufp++;
		actent++;
	}
	splx(s);

	*klen = actent * sizeof(mib_tcpConnEnt);
	return (NULL);
}
/*
 *	Get TCP Connection Entry
 */
nmget_tcpConnEntry(bufp, klen)
	mib_tcpConnEnt	*bufp;
	int	*klen;
{
	register struct	inpcb	*ptr;
	register struct	tcpcb	*tcpptr;
	int	s;

	if (*klen < sizeof (mib_tcpConnEnt)) 
		return (EMSGSIZE);

	s = splnet();
	for (ptr=tcb.inp_next; (ptr!= &tcb); ptr=ptr->inp_next) {
		tcpptr = (struct tcpcb *) ptr->inp_ppcb;
		if ((bufp->LocalAddress == ptr->inp_laddr.s_addr) &&
		    (bufp->RemAddress   == ptr->inp_faddr.s_addr) &&
		    (bufp->LocalPort == ptr->inp_lport) &&
		    (bufp->RemPort   == ptr->inp_fport)) 
			goto found;
		else	
			continue;
	}
	splx(s);	/* Not Found */
	return (ENOENT);

found:
	splx(s);
	bufp->State 	= tcpStateXlate[tcpptr->t_state];
	*klen = sizeof(mib_tcpConnEnt);
	return (NULL);
}
