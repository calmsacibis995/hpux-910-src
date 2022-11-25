/* @(#)rpc.lockd:	$Revision: 1.20.109.2 $	$Date: 95/01/19 17:00:18 $
*/
/* (#)udp.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)udp.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)udp.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

/*
 * this file consists of routines to support call_udp();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

/* ************************************************************** */
/* NOTE: prot_alloc.c, prot_libr.c, prot_lock.c, prot_main.c,     */
/* prot_msg.c, prot_pklm.c, prot_pnlm.c, prot_priv.c, prot_proc.c,*/
/* sm_monitor.c, svc_udp.c, tcp.c, udp.c, AND pmap.c share        */
/* a single message catalog (lockd.cat).  The last three files    */
/* pmap.c, tcp.c, udp.c have messages BOTH in lockd.cat AND       */
/* statd.cat.  For that reason we have allocated message ranges   */
/* for each one of the files.  If you need more than 10 messages  */
/* in this file check the  message numbers used by the other files*/
/* listed above in the NLS catalogs.                              */
/* ************************************************************** */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include "prot_lock.h"
#include "cache.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#define MAX_HASHSIZE 100

#include "prot_sec.h"


char *malloc();
char *xmalloc();
static CLIENT *client;
static int link_sock = RPC_ANYSOCK;
static int oldprognum, oldversnum, valid;
static char *oldhost;
extern int debug;
extern int HASH_SIZE;
extern char *progname;

struct cache *table[MAX_HASHSIZE];
int cache_len = sizeof(struct cache);

/*
 * copy_hp() copies a hostent structure and free_hp() frees the memory 
 * malloc'd for that structure.  These routines are necessary because 
 * gethostbyname() returns data in a static data area.  We only copy the
 * addresses because that's all we need at the moment.
 */

#define MAXADDRS	35		/* Same as in gethostent.c */

struct hostent *
copy_hp( hp )
struct hostent *hp;
{
	struct hostent *newhp;	/* New host ptr */
	struct in_addr *hostaddr;
	int count;

	if ( (newhp = (struct hostent *)xmalloc(sizeof *newhp)) == NULL )
		return (NULL);
	*newhp = *hp;
	newhp->h_addr_list = (char **) xmalloc( sizeof(char *) * MAXADDRS );
	if ( newhp->h_addr_list == NULL ) {
		xfree(&newhp);
		return (NULL);
	}
	hostaddr = (struct in_addr *)xmalloc( sizeof(struct in_addr)*MAXADDRS );
	if ( hostaddr == NULL ) {
		xfree(&newhp->h_addr_list);
		xfree(&newhp);
		return(NULL);
	}
	for (count=0; hp->h_addr_list[count] != NULL ; count++) {
		hostaddr[count] = *(struct in_addr *)hp->h_addr_list[count];
		newhp->h_addr_list[count] = (char *) &hostaddr[count];
	}
	newhp->h_addr_list[count] = NULL;
	return (newhp);
}

free_hp( hp )
struct hostent *hp;
{
	xfree(&hp->h_addr_list[0]);
	xfree(&hp->h_addr_list);
	xfree(&hp);
	return;
}
		
	

hash(name)
char *name;
{
	int len;
	int i, c;

	c = 0;
	len = strlen(name);
	for(i = 0; i< len; i++) {
		c = c +(int) name[i];
	}
	c = c %HASH_SIZE;
	/* tjs 12/93 we may not always have ASCII character */
	c = abs(c);
	return(c);
}

/*
 * find_hash returns the cached entry;
 * it returns NULL if not found;
 */
struct cache *
find_hash(host, prognum, versnum)
char *host;
int prognum, versnum;
{
	struct cache *cp;

	cp = table[hash(host)];
	while( cp != NULL) {
		if(strcmp(cp->host, host) == 0 &&
		 cp->prognum == prognum && cp->versnum == versnum) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

/*
 * find_hash_xid() -- same as find_hash, except based on xid.
 */
struct cache *
find_hash_xid( xid )
int xid;
{
        struct cache *cp;
	int i;

	for ( i=0 ; i<MAX_HASHSIZE; i++) {
	    cp = table[i];
	    while( cp != NULL) {
                if( cp->xid == xid ) {
                        /*found */
                        return(cp);
                }
                cp = cp->nxt;
	    }
        }
        return(NULL);
}

/*
 * init_hash() -- added to initialize the new cache fields used with the
 * support for asynchronous portmap calls.  Main weirdity here is that it
 * needs to copy the hostent information to a local copy because the stuff
 * returned by gethostent() is in a static buffer.  This is a separate
 * function from add_hash() so that it can be called in case of retransmissions
 * were we already have a hash entry and just want to start over.
 */

enum clnt_stat
init_hash( cp, host )
struct cache *cp;
char *host;
{
	struct hostent *hp;

	cp->server_addr.sin_port = (u_short) -1;

	cp->psendtime.tv_sec = 0;	/* never have sent before */
	cp->psendtime.tv_usec = 0;
	cp->addrtime.tv_sec = 0;	/* Never had valid address */
	cp->addrtime.tv_usec = 0;
	cp->paddrtime.tv_sec = 0;
	cp->paddrtime.tv_usec = 0;	
	cp->firsttime = TRUE;		/* first attempt  is special */
	cp->port_state = PORT_INVALID;		/* Don't have port # yet */

	cp->h_index = 0;
	
	if ( (hp = gethostbyname(host)) == NULL) {
		return ( RPC_UNKNOWNHOST );
	};
	if ( cp->hp )
		free_hp(cp->hp);
	if ( (cp->hp = copy_hp(hp)) == NULL ) {
	    logmsg((catgets(nlmsg_fd,NL_SETN,1020,"ERROR -- out of memory")));
	    return(RPC_SYSTEMERROR);
	}
	if ( cp->client ) {
	    clnt_destroy( cp->client );
	    cp->client = NULL;
	}
	return (RPC_SUCCESS);
}

struct cache *
add_hash(host, prognum, versnum)
char *host;
int prognum, versnum;
{
	struct cache *cp;
	int h;
	enum clnt_stat clnt_stat;

	if((cp = (struct cache *) xmalloc(cache_len)) == NULL ) {
		return(NULL);	/* malloc error */
	}
	if((cp->host = xmalloc(strlen(host)+1)) == NULL ) {
		xfree(&cp);
		return(NULL);	/* malloc error */
	}
	strcpy(cp->host, host);
	cp->prognum = prognum;
	cp->versnum = versnum;
	cp->hp = NULL;
	cp->client = NULL;
	h = hash(host);
	cp->nxt = table[h];
	table[h] = cp;
	if(debug > 2)
	     logmsg((catgets(nlmsg_fd,NL_SETN,1021, "add hash entry (%1$x), %2$s, %3$x, %4$x\n")), cp, host, prognum, versnum);
	return(cp);
}

void
delete_hash(host) 
char *host;
{
	struct cache *cp;
	struct cache *cp_prev = NULL;
	struct cache *next;
	int h;

	/* if there is more than one entry with same host name;
	 * delete has to be recurrsively called
         */
	h = hash(host);
	next = table[h];
	while((cp = next) != NULL) {
		next = cp->nxt;
		if(strcmp(cp->host, host) == 0) {
			if(cp_prev == NULL) {
				table[h] = cp->nxt;
			}
			else {
				cp_prev->nxt = cp->nxt;
			}
			if(debug > 2)
	     		     logmsg((catgets(nlmsg_fd,NL_SETN,1022, "delete hash entry (%1$d), %2$s, %3$d, %4$d\n")), cp, host, cp->prognum, cp->versnum);
			if (cp->client != NULL ) {
			   clnt_destroy(cp->client);
			}
			if ( cp->hp != NULL ) {
			   free_hp(cp->hp);
			}
			xfree(&cp->host);
			xfree(&cp);
		}
		else {
			cp_prev = cp;
		}
	}
}

/*
 * call_udp() was rewritten to allow for asynchronous portmapper requests.
 * This allows us to service requests at the same time we are trying to talk
 * to a down system's portmapper.  The key here is the new routine getport()
 * in pmap.c.  Also, we don't re-get the port and client structure if we 
 * just got them.  This avoids regetting information if we just got it for
 * some other request.  The other side-effect of this is that we don't have
 * two places where we are doing the same chunk of code, as was evident in
 * the structure of this function before.
 */

call_udp(host, prognum, versnum, procnum, inproc, in, outproc, out, valid_in, t)
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
	enum clnt_stat clnt_stat;

	clnt_stat = call_remote(IPPROTO_UDP, host, prognum, versnum, procnum,
			 	inproc, in, outproc, out, valid_in, t);
	if ( debug )
		logmsg((catgets(nlmsg_fd,NL_SETN,1023,"call_udp[%1$s, %2$d, %3$d, %4$d] returns %5$d")), host, prognum, versnum, procnum, clnt_stat);

	return ((int)clnt_stat);
}

call_remote(proto, host, prog, vers, proc, inproc, in, outproc, out, valid_in,t)
	int proto, prog,vers,proc;
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout, now;
	struct cache *cp;
	int socket = RPC_ANYSOCK;

	if((cp = find_hash(host, prog, vers)) == NULL) {
		if((cp = add_hash(host, prog, vers)) == NULL) {
			logmsg((catgets(nlmsg_fd,NL_SETN,1024, "Cannot send due to out of cache\n")));
			return(-1);
		}
		if ( (clnt_stat = init_hash( cp, host)) != RPC_SUCCESS )  {
			delete_hash(host);
			return(clnt_stat);
		}
		if(debug)
		     logmsg((catgets(nlmsg_fd,NL_SETN,1025, "(%1$x):[%2$s, %3$d, %4$d] is a new connection\n")), cp, host, prog, vers);

	}
	else if ( (cp->port_state == PORT_VALID_TIMEOUT) && valid_in == 0 ) {
		/*
		 * We have a timeout and we know the port number.  See if
		 * we have had the port number long enough to really think its
		 * bad.  If so, then start over with portmapper, etc.
		 */
		(void) gettimeofday( &now, 0 );
		if ( (now.tv_sec - cp->addrtime.tv_sec) > MINWAITTIME ) {
		    if (debug)
			logmsg((catgets(nlmsg_fd,NL_SETN,1026, "(%1$x):[%2$s, %3$d, %4$d] is a timed out connection\n")), cp, host, prog, vers);

		    if ( (clnt_stat = init_hash( cp, host)) != RPC_SUCCESS )  {
			delete_hash(host);
			return(clnt_stat);
		    }
		}
	}
	/*
	 * Now have a valid hash entry, check on the state of the getting
	 * the port number.  If port == 0, then we successful contacted the
	 * portmapper, but the lockd wasn't registered.  Otherwise,
	 * we are in the process of trying to contact the portmapper, and
	 * want to try again.  Otherwise, port should be a valid port number
	 * for the remote lockd.  Note, if getport returns RPC_SUCCESS, then
	 * sin_port was set as a side-effect in getport().
	 */
	if ( cp->port_state && (cp->server_addr.sin_port == 0) ) {
		delete_hash(host);
		return ((int) RPC_PROGNOTREGISTERED);
	}
	if ( !cp->port_state ) {
		clnt_stat = getport( cp, prog, vers, proto, t);
		if ( clnt_stat != RPC_SUCCESS ) {
			if ( clnt_stat != RPC_TIMEDOUT )
				delete_hash(host); 	/* Start over */
			return ( (int) clnt_stat);
		}
	}
	/*
	 * At this point we know we have a valid port #.  However, cp->client
	 * may be NULL if this is the first time we are actually trying to
 	 * talk to the server.  If so, create the client structure.  Note that
	 * portmap will not be contacted at this point because we already have
	 * a valid port number.
	 */
	if ( cp->client == NULL ) {
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		if (debug)
		    logmsg((catgets(nlmsg_fd,NL_SETN,1027,"clnt_create(%1$s): addr = %2$x, port = %3$d")),
		      host, cp->server_addr.sin_addr, cp->server_addr.sin_port);
		if ( proto == IPPROTO_UDP ) {
		    if ( (cp->client = clntudp_create( &cp->server_addr, 
				prog, vers, timeout, &link_sock,
				RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL) {
				    delete_hash(host);
				    return ((int) rpc_createerr.cf_stat);
		    }
		}
		else {
		    if ( (cp->client = clnttcp_create( &cp->server_addr, prog,
		     vers, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL){
				delete_hash(host);
				return ((int) rpc_createerr.cf_stat);
		    }
                }
	}
	tottimeout.tv_sec = t;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(cp->client, proc, inproc, in,
	    outproc, out, tottimeout);
	/*
	 * If timeout = 0 the clnt_call will return RPC_TIMEDOUT without
 	 * ever waiting.  We map this to RPC_SUCCESS so that RPC_TIMEDOUT
	 * can be used to mean we are waiting for a response from portmap.
	 */
	if ( t == 0 && clnt_stat == RPC_TIMEDOUT )
		clnt_stat = RPC_SUCCESS;

	cp->port_state = PORT_VALID_TIMEOUT;

	if (clnt_stat == RPC_SUCCESS && proto == IPPROTO_TCP )
		delete_hash ( host);

	return ((int) clnt_stat);
}

initialize_link_sock()
{
	init_socket( &link_sock );
}

init_socket( sock )
int *sock;
{

	int dontblock = 1;

	*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (*sock < 0) {
		log_perror((catgets(nlmsg_fd,NL_SETN,1028, "socket() call failed")));
		semclose();
		exit(1);
	}
	/* 
	 * Attempt to bind to prov port 
	 * If the caller is a non-su, then the bindresvport just
	 * fails quietly 
	 */

	/*
	 * In the case of a secure system, the SEC_REMOTE privilege will
	 * be added to the effective privilege so the call to
	 * bindresvport() will work.  It just doesn't make any sense
	 * to have any user who isn't a member of the "network" protected
	 * subsystem to execute this command. 
	 */
	ENABLEPRIV(SEC_REMOTE);

	(void)bindresvport(*sock, (struct sockaddr_in *)0);

	DISABLEPRIV(SEC_REMOTE);

	/*
	 * Avoid fd's 0,1, and 2, as these may cause conflicts with
	 * calling programs. Only need to check high end (2) because
	 * we checked the low end (<0) above.
	 */
	if( *sock <= 2 ) {
		int newfd = fcntl(*sock, F_DUPFD, 3);

		if ( newfd < 0 ) { 
			log_perror((catgets(nlmsg_fd,NL_SETN,1029, "Failure in fcntl() to create new socket")));
			semclose();
			exit(1);
		}
		(void) close (*sock);
		*sock = newfd;
	}
	/* the sockets rpc controls are non-blocking */
	(void)ioctl(*sock, FIONBIO, &dontblock);
	if (debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,1030, "socket descriptor for link_sock = %d\n")), *sock);
	return;
}
