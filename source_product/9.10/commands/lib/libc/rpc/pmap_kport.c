/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rpc/pmap_kport.c,v $
 * $Revision: 12.0 $	$Author: nfsmgr $
 * $State: Exp $   	$Locker:  $
 * $Date: 89/09/25 16:08:45 $
 */

/*
 * pmap_kgetport.c
 * Kernel interface to pmap rpc service.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 * Copyright (C) 1988, Hewlett Packard Company
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/pmap_prot.h"

#include "../h/time.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in_pcb.h"

#include "../h/ns_diag.h"

#define retries 4
static struct timeval tottimeout = { 1, 0 };


/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 *
 * The 'address' argument is used to locate the portmapper, then
 * modified to contain the port number, if one was found.  If no
 * port number was found, 'address'->sin_port returns unchanged.
 *
 * Returns:	 0  if port number successfully found for 'program'
 *		-1  (<0) if 'program' was not registered
 *		 1  (>0) if there was an error contacting the portmapper
 */
int
pmap_kgetport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	u_short port = 0;
	register CLIENT *client;
	struct pmap parms;
	int error = 0;
	struct sockaddr_in tmpaddr;
	register enum clnt_stat status;

	/* copy 'address' so that it doesn't get trashed */
	tmpaddr = *address;

	tmpaddr.sin_port = htons(PMAPPORT);
	client = clntkudp_create(&tmpaddr, PMAPPROG, PMAPVERS,retries,u.u_cred);

	if (client != (CLIENT *)NULL) {
		/*
		 * On HP-UX we interrupt at the RPC level.
		 */
		clntkudp_setint(client);
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		if ((status = CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap,
                    &parms, xdr_u_short, &port, tottimeout)) != RPC_SUCCESS){
			if ( status == RPC_INTR )
				error = 2 ;
			else
				error = 1;    /* error contacting portmapper */
		} else if (port == 0) {
			error = -1;	/* program not registered */
		} else {
			address->sin_port = htons(port);  /* save the port # */
		}
		clntkudp_freecred(client);
		AUTH_DESTROY(client->cl_auth);
		CLNT_DESTROY(client);
	}
	else {
		int interrupted;
		extern int lbolt;
		/*
		 * Must be some system error, probably out of mbufs.  To
		 * prevent a "spin-loop", we sleep a second to give the system
		 * time to recover, handling interrupts of course!
		 */
		interrupted = sleep( (caddr_t)&lbolt, PZERO+1 | PCATCH);
		if ( interrupted )
			error = 2;	/* flag the interrupt */
	}

	return (error);
}

/*
 * getport_loop -- kernel interface to pmap_kgetport()
 *
 * Talks to the portmapper using the sockaddr_in supplied by 'address',
 * to lookup the specified 'program'.
 *
 * Modifies 'address'->sin_port by rewriting the port number, if one
 * was found.  If a port number was not found (ie, return value != 0),
 * then 'address'->sin_port is left unchanged.
 *
 * If the portmapper does not respond, prints console message (once).
 * Retries forever, unless a signal is received.
 *
 * Returns:	 0  the port number was successfully put into 'address'
 *		-1  (<0) the requested process is not registered.
 *		 1  (>0) the portmapper did not respond
 *		 2  (>0) the portmapper did not respond and a signal occurred.
 */
getport_loop(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_long protocol;
{
	register int pe = 0;
	register int i = 0;

	/* sit in a tight loop until the portmapper responds */
	while ((i = pmap_kgetport(address, program, version, protocol)) > 0) {

		/* test to see if a signal has come in */
		if ( i == 2 ) {
		    NS_LOG(LE_NFS_LM_PORTMAP_GIVE_UP,NS_LC_WARNING,NS_LS_NFS,0);
		    goto out;		/* got a signal */
		}
		/* 
		 * We got no response, so check to see if the portmap is even
		 * up and running by seeing if it's UDP port is bound.
		 */
		if ( !check_port_bound((u_short)PMAPPORT) ) {
		    NS_LOG(LE_NFS_LM_PORTMAP_NOT_UP, NS_LC_ERROR, NS_LS_NFS, 0);
		    goto out;	/* NOTE: i should be 1 here */
		}
		/* print this message only once */
		if (pe++ == 0) {
		    NS_LOG( LE_NFS_LM_PORTMAP_TRYING,NS_LC_WARNING,NS_LS_NFS,0);
		}
	}				/* go try the portmapper again */

	/* got a response...print message if there was a delay */
	if (pe != 0) {
		NS_LOG(LE_NFS_LM_PORTMAP_OK, NS_LC_WARNING, NS_LS_NFS, 0 );
	}
out:
	return(i);	/* may return <0 if program not registered */
}

/*
 * check_port_bound() -- front end to in_pcblookup() to check whether
 * a given UDP port is bound or not.  Returns 1 if bound, 0 if not.
 */

extern struct inpcb udp_cb;

int
check_port_bound( port )
u_short port;
{
    struct in_addr inaddr_any;	 /* Wildcard address */

    inaddr_any.s_addr = INADDR_ANY;
    if ( in_pcblookup(&udp_cb,inaddr_any,0,inaddr_any,port,INPLOOKUP_WILDCARD) )
	return(1);

    return (0); /* port wasn't bound */
}
