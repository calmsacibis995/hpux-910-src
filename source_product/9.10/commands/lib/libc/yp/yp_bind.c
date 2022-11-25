/*	@(#)yp_bind.c	$Revision: 12.7.1.2 $	$Date: 94/10/05 14:51:31 $  */
/*
yp_bind.c	2.2 86/04/24 NFSSRC
static  char sccsid[] = "yp_bind.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define bind 				_bind
#define catgets 			_catgets
#define clntudp_bufcreate 		_clntudp_bufcreate
#define close 				_close
#define connect 			_connect
#define dup				_dup
#define dup2				_dup2
#define fcntl 				_fcntl
#define fprintf 			_fprintf
#define getdomainname 			_getdomainname
#define geteuid 			_geteuid
#define getpid 				_getpid
#define getsockname 			_getsockname
#define memset 				_memset
#define setsockopt 			_setsockopt
#define sleep 				_sleep
#define socket 				_socket
#define strcmp 				_strcmp
#define strcpy 				_strcpy
#define strlen 				_strlen
#define xdr_pmap 			_xdr_pmap
#define xdr_u_long 			_xdr_u_long
#define xdr_void 			_xdr_void
#define xdr_ypbind_resp 		_xdr_ypbind_resp
#define xdr_ypdomain_wrap_string 	_xdr_ypdomain_wrap_string
#define yp_bind 			_yp_bind	/* In this file */
#define yp_get_default_domain 		_yp_get_default_domain	/*In this file*/
#define yp_unbind 			_yp_unbind	/* In this file */

#ifdef _ANSIC_CLEAN
#define free 			_free
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAMESPACE_CLEAN */

#define NL_SETN 22	/* set number */
#include <nl_types.h>
static nl_catd nlmsg_fd;

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>
#include <fcntl.h>
#include <string.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _ypbind_rmutex;
extern REC_MUTEX _ypgetdom_rmutex;
#endif
extern int errno;
extern int sleep();
extern char *malloc();
extern char *strcpy();

enum bind_status {
	BS_BAGIT,
	BS_RETRY,
	BS_OK
};

bool check_pmap_up();
bool check_binder_up();
enum bind_status talk2_pmap();
enum bind_status talk2_binder();
void talk2_server();
enum bind_status get_binder_port();
bool check_binding();
void newborn();
struct dom_binding *load_dom_binding();

/*
 * Time parameters when talking to the ypbind and pmap processes
 */

#define YPSLEEPTIME 5			/* Time to sleep between tries */
unsigned int _ypsleeptime = YPSLEEPTIME;

#define YPBIND_TIMEOUT 30		/* Total seconds for timeout */
#define YPBIND_INTER_TRY 30		/* Seconds between tries */

static struct timeval bind_intertry = {
	YPBIND_INTER_TRY,		/* Seconds */
	0				/* Microseconds */
	};
static struct timeval bind_timeout = {
	YPBIND_TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

/*
 * Time parameters when talking to the ypserv process
 */

#ifdef  DEBUG
#define YPTIMEOUT 120			/* Total seconds for timeout */
#define YPINTER_TRY 60			/* Seconds between tries */
#else
#define YPTIMEOUT 20			/* Total seconds for timeout */
#define YPINTER_TRY 5			/* Seconds between tries */
#endif

#define MAX_TRIES_FOR_NEW_YP 2		/* Number of times we'll try to get
					 *   the current version of NIS
					 *   server before we try the other */
static struct timeval ypserv_intertry = {
	YPINTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval _ypserv_timeout = {
	YPTIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

static struct in_addr my_addr;		/* Local internet addr */
static struct dom_binding *bound_domains; /* List of bound domains */
static char default_domain[YPMAXDOMAIN+1];
/*
 * binder_port holds what we believe to be the local UDP port for ypbind.  It 
 * is set only by talk2_pmap.  It is cleared (set to 0) by:
 *	1. talk2_pmap: always upon entry.
 *	2. check_binder_up if:
 *		- It can't create a socket to speak to the binder.
 *		- If it fails to bind to the port.
 *	3. talk2_binder if there are RPC errors when trying to use the port.
 *
 * HPNFS binder_tcp_port contains the local TCP port for ypbind.  It is
 * 	 used only by check_binder_up and set only by talk2_pmap and 
 *	 check_binder_up.
 */
static unsigned long binder_port;	/* Initialize to "no port" */
static unsigned long binder_tcp_port;	/* Initialize to "no port" */

/*
 * Attempts to locate a network information service server which serves a passed domain.  If
 * one is found, an entry is created on the static list of domain-server pairs
 * pointed to by cell bound_domains, a udp path to the server is created and
 * the function returns 0.  Otherwise, the function returns a defined errorcode
 * YPERR_xxxx.
 */
#ifdef _THREAD_SAFE
/*
 * The routine assumes that the _ypbind_rmutex lock has already
 * been acquired.
 */
#endif
int
_yp_dobind(domain, binding)
	char *domain;
	struct dom_binding **binding;	/* if result == 0, ptr to dom_binding */
{
	struct dom_binding *pdomb;	/* Ptr to new domain binding */
	struct sockaddr_in ypbinder;	/* To talk with ypbinder */
	char *pdomain;			/* For xdr interface */
	struct ypbind_resp ypbind_resp; /* Response from local ypbinder */
	int vers;			/* ypbind program version number */
	int tries;			/* Number of times we've tried with
					 *  the current protocol */
	int status;
	enum bind_status loopctl;
	bool bound;
	bool oldport = FALSE;
	int domlen;			/*  Length of the domain string  */

	if (domain == NULL)
		return (YPERR_BADARGS);

	domlen = strlen(domain);
	if ((domlen == 0) || (domlen > YPMAXDOMAIN))
		return (YPERR_BADARGS);

	newborn();

	if (check_binding(domain, binding) )
		return (0);		/* We are bound */


	/*
	 * Use loopback address. (changed for BSD4.3 - dlr)
	 */
	my_addr.s_addr = htonl(INADDR_LOOPBACK);

	pdomain = domain;

	/*
	 * Try to get the binder's port, using the current program version.
	 * The version may be changed to the old version, deep in the bowels
	 * of talk2_binder.
	 */
	for (bound = FALSE, vers = YPBINDVERS; !bound; ) {

		if (binder_port) {
			oldport = TRUE;
		} else {
			oldport = FALSE;

			/*
			 * Get the binder's port.  We'll loop as long as
			 * get_binder_port returns BS_RETRY.
			 */
			for (loopctl = BS_RETRY; loopctl != BS_OK; ) {

	 			switch (loopctl =
				    get_binder_port(vers, &status) ) {
				case BS_BAGIT:
					return (status);
				case BS_OK:
					break;
				}
			}
		}

		/*
		 * See whether ypbind is up.  If no, bag it if it's a
		 * resource error, or if we are using a port we just got
		 * from the port mapper.  Otherwise loop around to try to
		 * get a valid port.
		 */
		if (!check_binder_up(&ypbinder, &status, vers)) {

			if (status == YPERR_RESRC) {
				return (status);
			}

			if (!oldport && status == YPERR_YPBIND) {
				return (status);
			}

			continue;
		}

		/*
		 * At this point, we think we know how to talk to the
		 * binder, and the binder is apparently alive.  Until we
		 * succeed in binding the domain, or we know we can't ever
		 * bind the domain, we will try forever.  This loops when
		 * talk2_binder returns BS_RETRY, and terminates when
		 * talk2_binder returns BS_BAGIT, or BS_OK.  If binder_port
		 * gets cleared, we will not execute this loop again, but
		 * will go to the top of the enclosing loop to try to get
		 * the binder's port again.  It is never the case that both
		 * talk2_binder returns  BS_OK and that it clears the
		 * binder_port.
		 */
		for (loopctl = BS_RETRY, tries = 1;
		    binder_port && (loopctl != BS_OK); tries++) {

			switch (loopctl = talk2_binder(&ypbinder, &vers,
			    tries, &pdomain, &ypbind_resp, &status) ) {
			case BS_BAGIT:
				return (status);
			case BS_OK:
				bound = TRUE;
			}
		}
	}

	if ( (pdomb = load_dom_binding(&ypbind_resp, vers, domain, &status) ) ==
	    (struct dom_binding *) NULL) {
		return (status);
	}

	if (vers == YPBINDOLDVERS) {
		talk2_server(pdomb);
	}

	*binding = pdomb;			/* Return ptr to the binding
						 *   entry */
	return (0);				/* This is the go path */
}

/*
 * This is a "wrapper" function for _yp_dobind for vanilla user-level
 * functions which neither know nor care about struct dom_bindings.
 */

#ifdef _NAMESPACE_CLEAN
#undef yp_bind
#pragma _HP_SECONDARY_DEF _yp_bind yp_bind
#define yp_bind _yp_bind
#endif

int
yp_bind(domain)
	char *domain;
{

	struct dom_binding *binding;
#ifdef _THREAD_SAFE
	int retval;

	_rec_mutex_lock(&_ypbind_rmutex);
	retval = _yp_dobind(domain, &binding);
	_rec_mutex_unlock(&_ypbind_rmutex);
	return (retval);
#else
	return (_yp_dobind(domain, &binding) );
#endif
}

/*
 * Attempts to find a dom_binding in the list at bound_domains having the
 * domain name field equal to the passed domain name, and removes it if found.
 * The domain-server binding will not exist after the call to this function.
 * All resources associated with the binding will be freed.
 */

#ifdef _NAMESPACE_CLEAN
#undef yp_unbind
#pragma _HP_SECONDARY_DEF _yp_unbind yp_unbind
#define yp_unbind _yp_unbind
#endif

void
yp_unbind (domain)
	char *domain;
{
	struct dom_binding *pdomb;
	struct dom_binding *ptrail = 0;
	int domlen;			/*  Length of the domain string  */
	struct sockaddr_in local_name;	/* Used with getsockname */
	int local_name_len = sizeof (struct sockaddr_in);

	if (domain == NULL)
		return;

	domlen = strlen(domain);
	if ((domlen == 0) || (domlen > YPMAXDOMAIN))
		return;

	/* HPNFS To satisfy lint so that it does not complain about a   */
	/* a variable being used before being set and so that we avoid  */
	/* any possible problems with the s800 optimizer, ptrail is     */
	/* initialized to bound_domains at the top of the loop.  Ptrail */
	/* will never be used anyway if it is the first time through    */
	/* the loop.							*/

#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_ypbind_rmutex);
#endif
	for (pdomb = bound_domains, ptrail = bound_domains; pdomb != NULL;
	    ptrail = pdomb, pdomb = pdomb->dom_pnext) {

		if (strcmp(domain, pdomb->dom_domain) == 0) {

			/*
			 * The act of destroying the rpc client closes
			 * the associated socket.  However, the fd
			 * representing what we believe to be a YP socket may
			 * really belong to someone else due to closes
			 * followed by new opens.
			 */

			local_name_len = sizeof (local_name);
			if ((getsockname(pdomb->dom_socket,
			    (struct sockaddr *) &local_name,
			    &local_name_len) != 0)  ||
			    (local_name.sin_family != AF_INET) ||
			    (local_name.sin_port != pdomb->dom_local_port)) {

				int tmp, tobesaved;

				tmp = dup(tobesaved = pdomb->dom_socket);
				clnt_destroy(pdomb->dom_client);
				tobesaved = dup2(tmp, tobesaved);
				(void) close(tmp);

			} else {
				clnt_destroy(pdomb->dom_client);
				(void) close(pdomb->dom_socket);
			}

			if (pdomb == bound_domains) {
				bound_domains = pdomb->dom_pnext;
			} else {
				ptrail->dom_pnext = pdomb->dom_pnext;
			}

			free((char *) pdomb);
			break;
		}
	}
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_ypbind_rmutex);
#endif
}

/*
 * This is a wrapper for the system call getdomainname which returns a
 * ypclnt.h error code in the failure case.  It also checks to see that
 * the domain name is non-null, knowing that the null string is going to
 * get rejected elsewhere in the nis client package.
 */

#ifdef _NAMESPACE_CLEAN
#undef yp_get_default_domain
#pragma _HP_SECONDARY_DEF _yp_get_default_domain yp_get_default_domain
#define yp_get_default_domain _yp_get_default_domain
#endif

int
yp_get_default_domain (domain)
	char **domain;
{
	if (domain == NULL) {
		return (YPERR_BADARGS);
	}

#ifdef _THREAD_SAFE
	/*
	 * We are assuming that the default domain does not change, otherwise
	 * this routine would need a new interface because we are returning
	 * a pointer to static data.
	 */

	_rec_mutex_lock(&_ypgetdom_rmutex);
#endif
	if (getdomainname(default_domain, YPMAXDOMAIN+1) == 0) {
		if (strlen(default_domain) > 0) {
			*domain = default_domain;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_ypgetdom_rmutex);
#endif
			return (0);
		} else {
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_ypgetdom_rmutex);
#endif
			return (YPERR_NODOM);
		}

	} else {
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_ypgetdom_rmutex);
#endif
		return (YPERR_YPERR);
	}
}

/*
 * This checks to see if this is a new process incarnation which has
 * inherited bindings from a parent, and unbinds the world if so.
 */
static void
newborn()
{
	static long int mypid = 0;	/* Cached to detect forks */
	long int testpid;

	if ((testpid = getpid() ) != mypid) {
		mypid = testpid;

		while (bound_domains) {
			yp_unbind(bound_domains->dom_domain);
		}
	}
}

/*
 * This checks that the socket for a domain which has already been bound
 * hasn't been closed or changed under us.  If it has, unbind the domain
 * without closing the socket, which may be in use by some higher level
 * code.  This returns TRUE and points the binding parameter at the found
 * dom_binding if the binding is found and the socket looks OK, and FALSE
 * otherwise.
 */

static bool
check_binding(domain, binding)
        char *domain;
        struct dom_binding **binding;
{
        struct dom_binding *pdomb;
        struct sockaddr_in local_name;
        int local_name_len = sizeof(struct sockaddr_in);

        for (pdomb = bound_domains; pdomb != NULL; pdomb = pdomb->dom_pnext) {

                if (strcmp(domain, pdomb->dom_domain) == 0) {

                        local_name_len = sizeof(local_name);
                        if ((getsockname(pdomb->dom_socket,
                            (struct sockaddr *) &local_name,
                            &local_name_len) != 0)  ||
                            (local_name.sin_family != AF_INET) ||
                            (local_name.sin_port != pdomb->dom_local_port) ) {
                                yp_unbind(domain);
                                break;
                        } else {
                                *binding = pdomb;
                                return (TRUE);
                        }
                }
        }

        return (FALSE);
}

/*
 * This checks whether the portmapper is up.  If the connect fails, the
 * portmapper is dead.  As a side effect, the pmapper sockaddr_in is
 * initialized.  The connection is not used for anything else, so it is
 * immediately closed.
 * NOTE: This routine is very similar to the routine with the same name 
 *	 in the file pmap_clnt.c.  Any changes made to this routine should 
 *	 be made to the one in pmap_clnt.c.
 */
static bool
check_pmap_up(pmapper, err)
	struct sockaddr_in *pmapper;
	int *err;
{
	int sokt;
	struct linger linger;
	int len = sizeof(struct linger);

	pmapper->sin_addr = my_addr;
	pmapper->sin_family = AF_INET;
	pmapper->sin_port = PMAPPORT;
	memset(pmapper->sin_zero, 0, 8);
	sokt =  socket(AF_INET, SOCK_STREAM, 0);

	if (sokt == -1) {
		*err = YPERR_RESRC;
		return (FALSE);
	} else {
		/*	HPNFS	jad	87.07.07
		**	added SO_LINGER(0) to force a close of the socket
		**	without a TIME_WAIT state; this function gets called
		**	a lot, and can leave *many* TIME_WAIT states ...
		*/
		linger.l_onoff = 1;
		linger.l_linger = 0;
		(void) setsockopt(sokt,SOL_SOCKET,SO_LINGER,&linger,len);
	}

	if (connect(sokt, (struct sockaddr *) pmapper,
	    sizeof(struct sockaddr_in)) < 0) {
		(void) close(sokt);
		*err = YPERR_PMAP;
		return (FALSE);
	}

	(void) close(sokt);
	return (TRUE);
}

/*
 * This checks whether ypbind is up.  If the bind succeeds, ypbind is dead,
 * because the binder port returned from portmap should already be in use.
 * There are two side effects.  The ypbind sockaddr_in is initialized. If
 * the function returns FALSE, the global binder_port will be set to 0.
 */
static bool
check_binder_up(ypbinder, err, vers)
	struct sockaddr_in *ypbinder;
	int *err;
	int vers;
{
	int sokt;
	int status;
	int tcp_sokt, len = sizeof(struct linger);
	struct sockaddr_in pmapper;
	struct sockaddr_in tcp_ypbinder;
	struct linger linger;

	if (binder_port == 0) {
		return (FALSE);
	}

	ypbinder->sin_addr = my_addr;
	ypbinder->sin_family = AF_INET;
	ypbinder->sin_port = htons(binder_port);
	memset(ypbinder->sin_zero, 0, 8);
	sokt =  socket(AF_INET, SOCK_DGRAM, 0); /* Throw-away socket */

	if (sokt == -1) {
		binder_port = 0;
		*err = YPERR_RESRC;
		return (FALSE);
	}

	errno = 0;
	status = bind(sokt, (struct sockaddr *) ypbinder,
	    sizeof(struct sockaddr_in));
	(void) close(sokt);

	if (status == -1) {
		if (errno == EADDRINUSE) {
			/*
			 * since we cannot grab the port, ypbind must be
			 * up and healthy.
			 * HPNFS This happens when you execute this
			 * 	 check as superuser, since ypbind is started
			 * 	 as superuser and now binds to a reserved
			 *	 port (due to bindresvport() call in 
			 *	 clntudp_bufcreate()).
			 */
			return (TRUE);
		}
#ifdef B1_NET
		/*
		 * B1 network does not check for users id = 0
		 */
		if ((errno == EACCES) &&
		    (binder_port < IPPORT_RESERVED))  {
/*}*/
#else
		if ((errno == EACCES) &&
		    (binder_port < IPPORT_RESERVED) &&
		    (geteuid() != 0)) {
#endif B1_NET
			/*
			 * HPNFS Now that ypbind can bind to a reserved port
			 *	 since it is normally started by the superuser 
			 *	 and calls bindresvport(), if we are not the 
			 *	 superuser and we attempt to bind to the 
			 *	 this port we will get EACCESS.
			 *	 To make sure that ypbind is really up
			 *	 in this case we will attempt to connect to
			 *	 it (and for that we need to obtain the TCP 
			 *	 port for ypbind from portmap)
			 */
			/* Is portmap up?  */
			if (check_pmap_up(&pmapper, err)) 
			{
				/* Get TCP port for ypbind from portmap */
				talk2_pmap(&pmapper, vers, err, 
					   &binder_tcp_port, IPPROTO_TCP);

				tcp_ypbinder.sin_addr = my_addr;
				tcp_ypbinder.sin_family = AF_INET;
				tcp_ypbinder.sin_port= htons(binder_tcp_port);
				memset(tcp_ypbinder.sin_zero, 0, 8);

				tcp_sokt =  socket(AF_INET, SOCK_STREAM, 0);
				if (tcp_sokt == -1) {
					*err = YPERR_RESRC;
					binder_port = 0;
					binder_tcp_port = 0;
					return (FALSE);
				} else {
					linger.l_onoff = 1;
					linger.l_linger = 0;
					(void) setsockopt(tcp_sokt,SOL_SOCKET,
							SO_LINGER,&linger,len);
				}
				/* If connect fails we cannot contact ypbind */
				/* Fall through to return at end of routine */
				if (connect(tcp_sokt, 
				            (struct sockaddr *) &tcp_ypbinder, 
					     sizeof(struct sockaddr_in)) < 0) {
					(void) close(tcp_sokt);
					binder_tcp_port = 0;
				} else {
					(void) close(tcp_sokt);
					return (TRUE);
				}
			}
			else 
			{
				binder_port = 0;
				binder_tcp_port = 0;
				*err = YPERR_PMAP;
				return(FALSE);
			}
		}
	}
	binder_port = 0;
	*err = YPERR_YPBIND;
	return (FALSE); 
}

/*
 * This asks the portmapper for addressing info for ypbind speaking a passed
 * program version number.  If it gets that info, the port number is stashed
 * in binder_port, but binder_port will be set to 0 when talk2_pmap returns
 * anything except BS_OK.  If the RPC call to the portmapper failed, the
 * current process will be put to sleep for _ypsleeptime seconds before
 * this function returns.
 * HPNFS We have added to this routine the ability to return a TCP or UDP
 * 	 port for ypbind.  Since this routine is static this routine and the 
 *	 change are not visible outside this file.  These changes were added
 *	 to provide a complete fix for when ypbind is bound to a reserved 
 *	 port and is killed with a SIGKILL and cannot clean up with the 
 *	 portmapper.  This is an NFS 3.2 change.
 */
static enum bind_status
talk2_pmap(pmapper, vers, err, bind_port, prot)
	struct sockaddr_in *pmapper;
	int vers;
	int *err;
	unsigned long *bind_port;
	unsigned long prot;	
{
	int sokt;
	CLIENT *client;
	struct pmap portmap;
	enum clnt_stat clnt_stat;

	*bind_port = 0;
	portmap.pm_prog = YPBINDPROG;
	portmap.pm_vers = vers;
	portmap.pm_prot = prot;
	portmap.pm_port = 0;		/* Don't care */

	sokt = RPC_ANYSOCK;

	if ((client = clntudp_bufcreate(pmapper, PMAPPROG, PMAPVERS,
	    bind_intertry, &sokt, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))  == NULL) {
		*err = YPERR_RPC;
		return (BS_BAGIT);
	}

	clnt_stat = (enum clnt_stat) clnt_call(client, PMAPPROC_GETPORT,
	    xdr_pmap, &portmap, xdr_u_long, bind_port, bind_timeout);
	clnt_destroy(client);
	(void) close(sokt);

	if (clnt_stat  == RPC_SUCCESS) {

		if (*bind_port != 0) {
			return (BS_OK);
		} else {
			*err = YPERR_YPBIND;
			return (BS_BAGIT);
		}

	} else {
		(void) sleep(_ypsleeptime);
		*err = YPERR_RPC;
		return (BS_RETRY);
	}
}

/*
 * This talks to the local ypbind process, and asks for a binding for the
 * passed domain.  As a side effect, if a version mismatch is detected, the
 * ypbind program version number may be changed to the old version.  In the
 * success case, the ypbind response will be returned as it was loaded by
 * ypbind - that is, containing a valid binding.  If the RPC call to ypbind
 * failed, the current process will be put to sleep for _ypsleeptime seconds
 * before this function returns.
 */
static enum bind_status
talk2_binder(ypbinder, vers, tries, ppdomain, ypbind_resp, err)
	struct sockaddr_in *ypbinder;
	int *vers;
	int tries;
	char **ppdomain;
	struct ypbind_resp *ypbind_resp;
	int *err;
{
	int sokt;
	CLIENT *client;
	enum clnt_stat clnt_stat;

	sokt = RPC_ANYSOCK;

	/* Try for the other version is we can't find this one.  This
	   will ensure we never fail to find a NIS+ server if that is
	   all we have.
	 */
	if ((tries > 0) && ((tries % MAX_TRIES_FOR_NEW_YP) == 0))
		*vers = (*vers == YPBINDVERS) ? YPBINDOLDVERS : YPBINDVERS;

	if ((client = clntudp_bufcreate(ypbinder, YPBINDPROG, *vers,
	    bind_intertry, &sokt, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))  == NULL) {
		*err = YPERR_RPC;
		return (BS_BAGIT);
	}

	clnt_stat = (enum clnt_stat)clnt_call(client, YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, ppdomain, xdr_ypbind_resp,
	    ypbind_resp, bind_timeout);
	clnt_destroy(client);
	(void) close(sokt);

	if (clnt_stat == RPC_SUCCESS) {

		if (ypbind_resp->ypbind_status == YPBIND_SUCC_VAL) {
			/* Binding successfully returned from ypbind */
			return (BS_OK);
		} else {
			/* If this is not the last time to try for this
			   version of NIS server, then sleep */
			if ((tries % MAX_TRIES_FOR_NEW_YP) !=
				(MAX_TRIES_FOR_NEW_YP-1)) {
				(void) sleep(_ypsleeptime);
			}

			*err = YPERR_DOMAIN;
			return (BS_RETRY);
		}

	} else {

		if (clnt_stat == RPC_PROGVERSMISMATCH) {

			if (*vers == YPBINDOLDVERS) {
				*err = YPERR_YPBIND;
				return (BS_BAGIT);
			} else {
				*vers = YPBINDOLDVERS;
			}
		} else {
			(void) sleep(_ypsleeptime);
			binder_port = 0;
		}

		*err = YPERR_RPC;
		return (BS_RETRY);
	}
}

/*
 * This handles all the conversation with the portmapper to find the port
 * ypbind is listening on.  If binder_port is already non-zero, this returns
 * BS_OK immediately without changing anything.
 */
static enum bind_status
get_binder_port(vers, err)
	int vers;			/* !ypbind! program version number */
	int *err;
{
	struct sockaddr_in pmapper;

	if (binder_port) {
		return (BS_OK);
	}

	if (!check_pmap_up(&pmapper, err) ) {
		return (BS_BAGIT);
	}

	return (talk2_pmap(&pmapper, vers, err, &binder_port, IPPROTO_UDP));
}

/*
 * This allocates some memory for a domain binding, initialize it, and
 * returns a pointer to it.  Based on the program version we ended up
 * talking to ypbind with, fill out an opvector of appropriate protocol
 * modules.
 */
static struct dom_binding *
load_dom_binding(ypbind_resp, vers, domain, err)
	struct ypbind_resp *ypbind_resp;
	int vers;
	char *domain;
	int *err;
{
	struct dom_binding *pdomb;
	struct sockaddr_in dummy;	/* To get a port bound to socket */
	struct sockaddr_in local_name;
	int local_name_len = sizeof(struct sockaddr_in);

	nlmsg_fd = _nfs_nls_catopen();

	pdomb = (struct dom_binding *) NULL;

	if ((pdomb = (struct dom_binding *) malloc(sizeof(struct dom_binding)))
		== NULL) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "yp_bind:  load_dom_binding malloc failure\n")));
		*err = YPERR_RESRC;
		return (struct dom_binding *) (NULL);
	}

	pdomb->dom_server_addr.sin_addr =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;
	pdomb->dom_server_addr.sin_family = AF_INET;
	pdomb->dom_server_addr.sin_port =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_port;
	memset(pdomb->dom_server_addr.sin_zero, 0, 8);
	pdomb->dom_server_port =
	    ypbind_resp->ypbind_respbody.ypbind_bindinfo.ypbind_binding_port;
	pdomb->dom_socket = RPC_ANYSOCK;
	pdomb->dom_vers = (vers == YPBINDOLDVERS) ? YPOLDVERS : YPVERS;

	/*
	 * Open up a udp path to the server, which will remain active globally.
	 */
	if ((pdomb->dom_client = clntudp_bufcreate(&(pdomb->dom_server_addr),
	    YPPROG, ((vers == YPBINDVERS) ? YPVERS : YPOLDVERS) ,
	    ypserv_intertry, &(pdomb->dom_socket), RPCSMALLMSGSIZE, YPMSGSZ))
	    == NULL) {
		free((char *) pdomb);
		*err = YPERR_RPC;
		return (struct dom_binding *) (NULL);
	}
	/*
	 * Bind the socket to a bogus address so a port gets allocated for
	 * the socket, but so that sendto will still work.
	 */
	(void) fcntl(pdomb->dom_socket, F_SETFD, 1);
	dummy.sin_family = AF_INET;
	dummy.sin_addr.s_addr = 0;
	dummy.sin_port = 0;
	memset(dummy.sin_zero, 0, 8);

	/* 
	** HPNFS It is not important what bind returns since it might
	** 	 already have been bound by clntudp_bufcreate(),
	**	 which can do a bindresvport().	
	*/

	bind (pdomb->dom_socket, (struct sockaddr *) &dummy,sizeof(dummy));

	/*
	 * Remember the bound port number
	 */
	if (getsockname(pdomb->dom_socket, (struct sockaddr *) &local_name,
	    &local_name_len) == 0) {
		pdomb->dom_local_port = local_name.sin_port;
	} else {
		free((char *) pdomb);
		*err = YPERR_YPERR;
		return (struct dom_binding *) (NULL);
	}

	(void) strcpy(pdomb->dom_domain, domain);/* Remember the domain name */
	pdomb->dom_pnext = bound_domains;	/* Link this to the list as */
	bound_domains = pdomb;			/* ... the head entry */
	return (pdomb);
}

/*
 * This checks to see if a ypserv which we know speaks v1 NIS program number
 * also speaks v2 version.  This makes the assumption that the nis service at
 * the node is supplied by a single process, and that RPC will deliver a
 * message for a different program version number than that which the server
 * regestered.
 */
static void
talk2_server(pdomb)
	struct dom_binding *pdomb;
{
	int sokt;
	CLIENT *client;
	enum clnt_stat clnt_stat;

	sokt = RPC_ANYSOCK;

	if ((client = clntudp_bufcreate(&(pdomb->dom_server_addr),
	    YPPROG, YPVERS, ypserv_intertry, &sokt, RPCSMALLMSGSIZE, YPMSGSZ))
	    == NULL) {
		return;
	}
	clnt_stat = (enum clnt_stat) clnt_call(client, YPBINDPROC_NULL,
	    xdr_void, 0, xdr_void, 0, _ypserv_timeout);

	if (clnt_stat == RPC_SUCCESS) {
		clnt_destroy(pdomb->dom_client);
		(void) close(pdomb->dom_socket);
		pdomb->dom_client = client;
		pdomb->dom_socket = sokt;
		pdomb->dom_vers = YPVERS;
	} else {
		clnt_destroy(client);
		(void) close(sokt);
	}
}




