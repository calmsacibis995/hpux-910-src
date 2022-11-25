
/*
 * $Header: nipc_misc.c,v 1.4.83.5 93/10/27 17:23:09 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_misc.c,v $
 * $Revision: 1.4.83.5 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 17:23:09 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_misc.c $Revision: 1.4.83.5 $";
#endif

/* 
 * Misc. Netipc utilities.
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../netinet/in.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_err.h"


/* Search the Netipc domains and find a nipc_protosw */
mi_findproto(domain, proto, protocol, type)
int			domain;
struct nipc_protosw	**proto;
int			protocol;
int			type;
{
	extern struct nipc_domain	*nipc_domain_list;
	struct nipc_domain	*dptr;
	struct nipc_protosw	*pptr;
	int			error;

	/* FIND THE CORRECT DOMAIN */
	for (dptr = nipc_domain_list; dptr; dptr= dptr->nd_next_domain) 
	    if ((dptr->nd_domain & NIPC_DOMAIN_MASK) == domain) 
		break;
	if (dptr == 0)	
	 	return(EPROTONOSUPPORT);

	
	/* FIND PROTOCOL/TYPE PAIR */
	error = EPROTONOSUPPORT;
	for (pptr=dptr->nd_protosw; pptr < dptr->nd_protoswNPROTOSW; pptr++) {	
		/* IF PROTOCOL IS FOUND */ 
		if (pptr->np_protocol == protocol) {
			error = EPROTOTYPE;
			if (pptr->np_type == type) {
				/* TYPE MATCH, DONE */
				*proto = pptr;
				return(0);
			}
			/* else look for another instance of the protocol */ 
		}
		/* else not a protocol match */
	}
	return(error);
}
 
/*
 *	FIND HIGHEST ORDINAL DESCRIPTOR IN THE MAPS
 */

mi_gethighbit(depth, readmap, writemap, exceptionmap)
	int 	depth;
	fd_mask	readmap[], writemap[], exceptionmap[];
{
	 int	i;
	int		highbit=0;
	unsigned        target;
	fd_mask 	merge[howmany(FD_SETSIZE,NFDBITS)];

	/* figure depth of map arrays */
	depth--;	/* array is indexed 0-(depth -1); adjust depth here */

	/* merge the bit maps */
	for (i=0; i<= depth; i++)
		merge[i] = (readmap[i] | writemap[i] | exceptionmap[i]);

	/* find which highest int which holds a set file descriptor */
	while (merge[depth] == 0 && depth > 0)
		depth--;

	/* FIND HIGHEST SET BIT IN THAT INT */
	target = merge[depth];

	if((target >> 16) > 0) {
		target = target >> 16;
		highbit += 16;
	}
	if((target >> 8) > 0) {
		target = target >> 8;
		highbit += 8;
	}
	if((target >> 4) > 0) {
		target = target >> 4;
		highbit += 4;
	}
	if((target >> 2) > 0) {
		target = target >> 2;
		highbit += 2;
	}
	if((target >> 1) > 0) {
		target = target >> 1;
		highbit += 1;
	}
	
	/* return the number of the highest bit set */
	return(depth * NFDBITS + highbit);
}


 
mi_getoptions( user_opt, kopt)
	caddr_t		user_opt;
	struct k_opt	*kopt;
{
	struct opthead		ohead;		/* user option header 	*/
	struct optentry		oentry;		/* user option entry	*/
	int			i;		/* loop variable	*/
	unsigned		size;		/* size of data		*/
	char 			*addr;		/* kernel addr for data	*/
			

	/* INITIALIZE OPTIONS AS UNDEFINED */
	kopt->ko_flags.i = 0;	
		
	/* USER MAY SPECIFY NO OPTIONS */
	if (user_opt == (caddr_t)0)
		return(0);

	/* COPYIN() HEADER OF USER OPTION  */
	if (copyin(user_opt,(caddr_t)&ohead, sizeof(struct opthead))) 
		return(EFAULT);

	/* VERIFY HEADER SYNTAX */
	if (ohead.oh_count < 0	||
	    ohead.oh_length < ohead.oh_count*sizeof(struct optentry)) {
		return(E_OPTSYNTAX);
	}

	/*  FOR EACH ENTRY:  COPY IN ENTRY, VALIDATE IT, COPYIN DATA */
	for (i = 0; i < ohead.oh_count; i++) {

		/* COPY IN THE STRUCTURE DESCRIBING THE OPTION */
 		if (copyin( user_opt + sizeof(struct opthead) +
				(sizeof(struct optentry) * i), 
				(caddr_t)&oentry,	
				sizeof(struct optentry) ))
			return(EFAULT);

		/* SKIP EMPTY ENTRIES */
		if (oentry.oe_kind == 0)
			continue;

		/* SANITY CHECKS ON LENGTH AND OFFSET */
		if (	(oentry.oe_length < 0) || 
			(oentry.oe_offset < 0) ||
			((oentry.oe_offset + oentry.oe_length)  >
			    ((u_short)sizeof(struct opthead)
				+(u_short)ohead.oh_length)))
			return(E_OPTSYNTAX);
		
 
		/* GET SIZE AND DEST. ADDR OF OPTION DATA */
		switch (oentry.oe_kind) {
		case NSO_DATA_OFFSET:
			if (KOPT_DEFINED(kopt, KO_DATA_OFFSET))
				return(E_DUPOPTION);
			KOPT_DEFINE(kopt, KO_DATA_OFFSET);
			size = (unsigned) sizeof(kopt->ko_data_offset);
			addr = (caddr_t)&kopt->ko_data_offset;
			break;
		case NSO_MAX_CONN_REQ_BACK:
			if (KOPT_DEFINED(kopt, KO_MAX_CONN_REQ_BACK))
				return(E_DUPOPTION);
			KOPT_DEFINE(kopt, KO_MAX_CONN_REQ_BACK);
			size = (unsigned) sizeof(kopt->ko_max_conn_req_back);
			addr = (caddr_t)&kopt->ko_max_conn_req_back;
			break;
		case NSO_MAX_RECV_SIZE:
			if (KOPT_DEFINED(kopt, KO_MAX_RECV_SIZE))
				return(E_DUPOPTION);
			KOPT_DEFINE(kopt, KO_MAX_RECV_SIZE);
			size = (unsigned) sizeof(kopt->ko_max_recv_size);
			addr = (caddr_t)&kopt->ko_max_recv_size;
			break;
		case NSO_MAX_SEND_SIZE:
			if (KOPT_DEFINED(kopt, KO_MAX_SEND_SIZE))
				return(E_DUPOPTION);
			KOPT_DEFINE(kopt, KO_MAX_SEND_SIZE);
			size = (unsigned) sizeof(kopt->ko_max_send_size);
			addr = (caddr_t)&kopt->ko_max_send_size;
			break;
		case NSO_PROTOCOL_ADDRESS:
			if (KOPT_DEFINED(kopt, KO_PROTOCOL_ADDRESS))
				return(E_DUPOPTION);
			KOPT_DEFINE(kopt, KO_PROTOCOL_ADDRESS);
			size = oentry.oe_length;
			if (size > NIPC_MAX_PROTOADDR)
				return(E_OPTSYNTAX);
			addr = (caddr_t) kopt->ko_protoaddr.data;
			kopt->ko_protoaddr.len = oentry.oe_length;
			kopt->ko_protocol_address = &kopt->ko_protoaddr;
			break;
		case NSO_MIN_BURST_IN:
		case NSO_MIN_BURST_OUT:
			/* ignore these options, but don't return an error */
			continue;
		default:
			return(E_OPTOPTION);
		}

		/* CHECK OPTION DATA LENGTH AGAINST EXPECTED LENGTH */
		if (oentry.oe_length != size)
			return(E_OPTSYNTAX);
		
		/* COPY OPTION DATA INTO KERNEL STRUCTURE */
		if (copyin(user_opt + oentry.oe_offset, addr, size))
			return(EFAULT); 
	};	/* end for */		

	return(0);
}			/***** getoptions *****/

 
/* copy a user specified port into a sockaddr_in */
mi_pxpaddr(iaddr, m)
struct nipc_protoaddr	*iaddr;
struct mbuf		*m;
{
	struct sockaddr_in	*sin;

	/* SET MBUF LENGTH */
	m->m_len = sizeof(struct sockaddr);

	/* POINT TO DATA */
	sin = mtod(m, struct sockaddr_in *);

	/* SET SOCKADDR_IN FIELDS */
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;

	/* copy the port into the sockaddr_in */
	if (iaddr != 0) {
		/* if address supplied then do extra check */
		if (iaddr->len != NIPC_PXP_ADDRLEN)
			return(E_ADDROPT);
		/* bcopy(iaddr->data, sin->sin_port, NIPC_PXP_ADDRLEN); */
		sin->sin_port = iaddr->data[0];
	}
	else
		sin->sin_port = 0;

	return(0);
}

 
/* copy a user specified port into a sockaddr_in */
mi_tcpaddr(iaddr, m)
struct nipc_protoaddr	*iaddr;
struct mbuf		*m;
{
	struct sockaddr_in	*sin;

	/* SET MBUF LENGTH */
	m->m_len = sizeof(struct sockaddr);

	/* POINT TO DATA */
	sin = mtod(m, struct sockaddr_in *);

	/* SET SOCKADDR_IN FIELDS */
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;

	/* copy the port into the sockaddr_in */
	if (iaddr != 0) {
		/* if address supplied then do extra check */
		if (iaddr->len != NIPC_TCP_ADDRLEN)
			return(E_ADDROPT);
		/* bcopy(iaddr->data, sin->sin_port, NIPC_TCP_ADDRLEN); */
		sin->sin_port = iaddr->data[0];
	}
	else
		sin->sin_port = 0;

	return(0);
}

 
mi_timerpopped(p)
caddr_t p;
{
	wakeup( p );
}
