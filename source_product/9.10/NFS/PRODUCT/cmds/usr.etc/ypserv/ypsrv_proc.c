/*	@(#)ypsrv_proc.c	$Revision: 1.27.109.3 $	$Date: 94/12/16 09:07:41 $  
*/
/* (#)ypserv_proc.c	2.1 86/04/16 NFSSRC */
/*static char sccsid[] = "(#)ypserv_proc.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: ypsrv_proc.o $Revision: 1.27.109.3 $ 94/06/01 PHNE_5081";
#endif

/*
 * This contains Network Information Service server code which supplies the set of
 * functions requested using rpc.   The top level functions in this module
 * are those which have symbols of the form YPPROC_xxxx defined in
 * yp_prot.h, and symbols of the form YPOLDPROC_xxxx defined in ypsym.h.
 * The latter exist to provide compatibility to the old version of the yp
 * protocol/server, and may emulate the behaviour of the previous software
 * by invoking some other program.
 * 
 * This module also contains functions which are used by (and only by) the
 * top-level functions here.
 *  
 */

#ifndef YP_CACHE
#undef YP_UPDATE
#endif

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else /* NLS */
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif /* NLS */

#include "ypsym.h"
#ifdef	TRACEON
# define LIBTRACE
#endif
#include <arpa/trace.h>

#ifdef YP_CACHE
#include "yp_cache.h"
extern entrylst * search_ypcachemap ();
extern entrylst * add_to_ypcachemap();
extern entrylst * add_n_prune_list();
extern int cache_size;  /* value set by user */
#ifdef YP_UPDATE
extern bool_t uflag;	/* tells if YP_UPDATE is being used */
extern bool mustpull;	/* xfr map regardless when YP_UPDATE is in effect */
#endif /* YP_UPDATE */
#endif /* YP_CACHE */

#ifdef DBINTERDOMAIN
#include <sys/wait.h>
#include <netdb.h>
#include <ctype.h>
#endif /* DBINTERDOMAIN */

extern char *environ;
static char ypxfr_proc[] = YPXFR_PROC;
static char yppush_proc[] = YPPUSH_PROC;
struct yppriv_sym {
	char *sym;
	unsigned len;
};
static struct yppriv_sym filter_set[] = {
	{ORDER_KEY, ORDER_KEY_LENGTH},
	{MASTER_KEY, MASTER_KEY_LENGTH},
	{INPUT_FILE, INPUT_FILE_LENGTH},
#ifdef DBINTERDOMAIN
        {DNS_FALLBACK_KEY, DNS_FALLBACK_KEY_LENGTH},
#endif /* DBINTERDOMAIN */
	{NULL, 0}
};
void ypfilter();
bool isypsym();
bool xdrypserv_ypall();

extern char *inet_ntoa();
extern bool logging;
extern int logmsg();

#ifdef YPSERV_DEBUG
extern	struct debug_struct *debug_infoptr;
extern	void debug_get_req_args();
extern	void debug_getval();
extern	void debug_getkeyval();
#endif /* YPSERV_DEBUG */

#ifdef NLS
static nl_catd nlmsg_fd;
static nl_catd nlmsg_fd1;
static int first_time = 1;

static nl_catd
nls_front()
{
        if (first_time)
        {
                nl_init(getenv("LANG"));
                nlmsg_fd1 = catopen("ypsrv_proc",0);
                first_time = 0;
        }
        return(nlmsg_fd1);
}
#endif /* NLS */
#define FORK_ERR if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,1, "%s() fork failure"), fun)
#define EXEC_ERR if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,2, "%s() execl failure"), fun)
#define RESPOND_ERR if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,3, "%s() can't respond to rpc request"), fun)
#define FREE_ERR if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,4, "%s() can't free args"), fun)


/*
 * This determines whether or not a passed domain is served by this server,
 * and returns a boolean.  Used by both old and new protocol versions.
 */
void
ypdomain(rqstp, transp, always_respond)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	bool always_respond;
{
	/* char domain_name[YPMAXDOMAIN + 1]; */
	/* char *pdomain_name = domain_name; */
	char *pdomain_name = (char *)0;
	bool isserved;
	char *fun = "ypdomain";
	int	err;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	TRACE("ypdomain:  SOP");
	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		TRACE("ypdomain:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(pdomain_name);
#endif /* YPSERV_DEBUG */

	TRACE2("ypdomain:  calling ypcheck_domain, domain = \"%s\"",
		pdomain_name);
	isserved = (bool) ypcheck_domain(pdomain_name);

	if (isserved || always_respond) {
		if ((err = svc_sendreply(transp, xdr_bool, &isserved)) == 0) {
			TRACE("ypdomain:  svc_sendreply error to isserved");
			RESPOND_ERR;
		}
		if (!isserved  &&  logging) {
			TRACE("ypdomain:  not served, but always respond");
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,5,
				"Domain %s not supported"), pdomain_name);
		}

	} else {
		/*
		 * This case is the one in which the domain is not
		 * supported, and in which we are not to respond in the
		 * unsupported case.  We are going to make an error happen
		 * to allow the portmapper to end his wait without the
		 * normal udp timeout period.  The assumption here is that
		 * the only process in the world which is using the function
		 * in its no-answer-if-nack form is the portmapper, which is
		 * doing the krock for pseudo-broadcast.  If some poor fool
		 * calls this function as a single-cast message, the nack
		 * case will look like an incomprehensible error.  Sigh...
		 * (The traditional Unix disclaimer)
		 */

		TRACE("ypdomain:  not served, forcing svcerr_decode");
		svcerr_decode(transp);
#ifdef YPSERV_DEBUG
		/* moved under YPSERV_DEBUG to avoid writing millions of logmsgs */
		 if (logging)
			(void) logmsg(catgets(nlmsg_fd,NL_SETN,6,
			"Domain %s not supported (broadcast)"), pdomain_name);
#endif /* YPSERV_DEBUG */
	}
#ifdef YPSERV_DEBUG
	debug_infoptr->result		= isserved;
	debug_infoptr->resp_status	= err;
#endif /* YPSERV_DEBUG */

	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name)) {
		TRACE("ypdomain:  svc_freeargs err");
	  	FREE_ERR ;
	}
	TRACE("ypdomain:  EOP");
}

#ifdef DBINTERDOMAIN

/*
 * Inter-domain name support
 */
static char yp_interdomain[] = DNS_FALLBACK_KEY;
static int  yp_interdomain_sz = DNS_FALLBACK_KEY_LENGTH;


/*
 * When we don't find a match in our local domain, this function is invoked
 * to try the match using the inter-domain binding. Returns one of the
 * following three values to deftermine if we forked or not, and if so, if we
 * are in the child or the parent process.  The child blocks while it queries
 * the name server trying to answer the request, while the parent immediately
 * returns to avoid any deadlock problems.
 */
# define NEITHER_PROC 0
# define CHILD_PROC 1
# define PARENT_PROC 2

#define NQTIME 10		/* minutes */
#define PQTIME 30		/* minutes */
struct child {
	long            pid;	/* opaque */
	datum           key;
	datum           val;
	char           *map;
	struct timeval  enqtime;
	int             h_errno;
	SVCXPRT        *xprt;
	struct sockaddr_in caller;
	unsigned long   xid;
	struct child   *next_child;
};

static struct child *children = NULL;
extern          debuginterdomain;
extern          dbinterdomain;
#define IFDBI if (dbinterdomain)
dezombie()
{
	int             pid;
	union wait      wait_status;
	int 		hold_errno;	/*save for svc run*/

	TRACE("dezombie:  SOP");
	hold_errno = errno;

	while (TRUE) {
		pid = wait3(&wait_status, WNOHANG, NULL);

		if (pid == 0) {
			break;
		} else if (pid == -1) {
			break;
		}
	}
	errno = hold_errno;
	signal(SIGCHLD, dezombie);
	TRACE("dezombie:  EOP");
}

struct child   *
child_bykey(map, keydat)
	char           *map;
	datum           keydat;
{
	struct child   *chl;
	struct child   *prev;
	struct timeval  now;
	struct timezone tzp;
	int             secs;
	
	TRACE("child_bykey:  SOP");
	if (keydat.dptr == NULL) {
	   	TRACE("child_bykey:  EOP - keydat.dptr == NULL");
		return (NULL);
	}
	if (keydat.dsize <= 0) {
	   	TRACE("child_bykey:  EOP - keydat.dsize <= 0");
		return (NULL);
	}
	if (map == NULL) {
	   	TRACE("child_bykey:  EOP - map == NULL");
		return (NULL);
	}
	gettimeofday(&now, &tzp);

	for (prev = children, chl = children; chl;) {
		/* check for expiration */
		if (chl->h_errno == TRY_AGAIN)
			secs = NQTIME * 60;
		else
			secs = PQTIME * 60;
		if ((chl->pid == 0) && (chl->enqtime.tv_sec + secs) < now.tv_sec) {
			IFDBI           printf("bykey:stale child flushed %x\n", chl);
			/*deleteing the first element is tricky*/
			if (chl == children) {
				children = children->next_child;
				freechild(chl);
				prev = children;
				chl = children;
				continue;
			} else {
			/*deleteing  a middle element*/
				prev->next_child = chl->next_child;
				freechild(chl);
				chl = prev->next_child;
				continue;
			}
		} else if (chl->map)
			if (0 == strcmp(map, chl->map))
				if (chl->key.dptr) {
					if (keydat.dptr[keydat.dsize - 1] == 0)
						keydat.dsize--;	/* supress trailing null */
					if ((chl->key.dsize == keydat.dsize))
						if (0 == strncasecmp(chl->key.dptr, keydat.dptr, keydat.dsize)) {
							/* move to beginning */
							if (chl != children) {
								prev->next_child = chl->next_child;
								chl->next_child = children;
								children = chl;
							}
							TRACE("child_bykey:  EOP - found child");
							return (chl);
						}
				}
		prev = chl;
		chl = chl->next_child;
	}
	TRACE("child_bykey:  EOP -  Didn't find child");
	return (NULL);
}

struct child   *
newchild(map, keydat)
	char           *map;
	datum           keydat;
{
	char           *strdup();
	struct child   *chl;
	struct timezone tzp;
	
	TRACE("newchild:  SOP");
	chl = (struct child *) calloc(1, sizeof(struct child));
	if (chl == NULL) {
	   	TRACE("newchild:  EOP - chl == NULL");
		return (NULL);
	}
	IFDBI           printf("child  enqed\n");
	chl->map = (char *) strdup(map);
	if (chl->map == NULL) {
		free(chl);
		TRACE("newchild:  EOP - chl->map == NULL");
		return (NULL);
	}
	chl->key.dptr = malloc(keydat.dsize + 1);
	if (chl->key.dptr == NULL) {
		free(chl->map);
		free(chl);
		TRACE("newchild:  EOP - chl->key.dptr == NULL");
		return (NULL);
	}
	if (keydat.dptr != NULL)
		if (keydat.dptr[keydat.dsize - 1] == 0)
			keydat.dsize = keydat.dsize - 1;	/* delete trailing null
								 * case */
	chl->key.dsize = keydat.dsize;
	chl->val.dptr = 0;
	memcpy(chl->key.dptr, keydat.dptr, keydat.dsize);
	gettimeofday(&(chl->enqtime), &tzp);
	chl->next_child = children;
	children = chl;
	TRACE("newchild:  EOP - successes");
	return (chl);
}

struct child   *
deqchild(x)
	struct child   *x;
{
	struct child   *chl;
	struct child   *prev;
	
	TRACE("deqchild:  SOP");
	if (x == children) {
		children = children->next_child;
		x->next_child == NULL;
		TRACE("deqchild:  EOP - removed first entry");
		return (x);
	}
	for (chl = children, prev = children; chl; chl = chl->next_child) {
		if (chl == x) {
			/* deq it */
			prev->next_child = chl->next_child;
			chl->next_child == NULL;
			TRACE("deqchild:  EOP - removed an entry");
			return (chl);
		}
		prev = chl;
	}
	TRACE("deqchild:  EOP - bad");
	return (NULL);		/* bad */
}

freechild(x)
	struct child   *x;
{
   	TRACE("freechild:  SOP");
	if (x == NULL) {
	   	TRACE("freechild:  EOP - NULL");
		return (-1);
	}
	if (x->map)
		free(x->map);
	if (x->key.dptr)
		free(x->key.dptr);
	if (x->val.dptr)
		free(x->val.dptr);
	free(x);
	TRACE("freechild:  EOP - successful");
	return (0);
}

static int      my_done();

int
yp_matchdns(map, keydat, valdatp, statusp, rqstp, transp)
	char           *map;	/* map name */
	datum           keydat;	/* key to match (e.g. host name) */
	datum          *valdatp;/* returned value if found */
	unsigned       *statusp;/* returns the status */
	struct svc_req *rqstp;
	SVCXPRT        *transp;
{
	int             h;
	datum           idkey, idval;
	int             pid;
	int             byname, byaddr;
	struct child   *chl;
	struct child    chld;
	struct timeval  now;
	struct timezone tzp;
	int             try_again;
	int             mask;
	int             i;

	TRACE("yp_matchdns:  SOP");
	try_again = 0;
	/*
	 * Skip the domain resolution if: 1. it is not turned on 2. map other
	 * than hosts.byXXX 3. a null string (usingypmap() likes to send
	 * these) 4. a single control character (usingypmap() again)
	 */
	byname = strcmp(map, "hosts.byname") == 0;
	byaddr = strcmp(map, "hosts.byaddr") == 0;
	if ((!byname && !byaddr) ||
	    keydat.dsize == 0 || keydat.dptr[0] == '\0' ||
          !isascii(keydat.dptr[0]) || !isgraph(keydat.dptr[0])) {
		*statusp = YP_NOKEY;
		TRACE("yp_matchdns:  EOP - no hosts map");
		return (NEITHER_PROC);
	}

	idkey.dptr = yp_interdomain;
	idkey.dsize = yp_interdomain_sz;
	idval = fetch(idkey);

	if (!debuginterdomain && idval.dptr == NULL) {
		*statusp = YP_NOKEY;
		TRACE("yp_matchdns:  EOP - no support for DNS");
		return (NEITHER_PROC);
	}
	chl = child_bykey(map, keydat);
	if (chl) {
		gettimeofday(&now, &tzp);
		if (chl->h_errno == TRY_AGAIN)
			try_again = 1;
		else if (chl->pid) {
			IFDBI           printf("dropped resolver %d active\n", chl->pid);

			/*update xid*/	
			if (transp) {
				chl->xprt = transp;
				chl->caller = (* (svc_getcaller(transp)));
				chl->xid = svcudp_getxid(transp);
				IFDBI  printf("xid now %d\n", chl->xid);
			}
			TRACE("yp_matchdns:  EOP - resolver active");
			return (PARENT_PROC);	/* drop */
		}
		switch (chl->h_errno) {
		case NO_RECOVERY:
#ifndef NO_DATA
#define NO_DATA NO_ADDRESS
#endif
		case NO_DATA:
		case HOST_NOT_FOUND:
			IFDBI printf("cache NO_KEY\n");
			*statusp = YP_NOKEY;
			TRACE("yp_matchdns:  EOP - No match from DNS");
			return (NEITHER_PROC);

		case TRY_AGAIN:
			IFDBI printf("try_again\n");
			try_again = 1;
			break;
		case 0:
			IFDBI printf("cache ok\n");
			if (chl->val.dptr) {
				*valdatp = chl->val;
				*statusp = YP_TRUE;
				TRACE("yp_matchdns: EOP - ???????");
				return (NEITHER_PROC);
			}
			break;

		default:
			freechild(deqchild(chl));
			chl = NULL;
			break;
		}
	}
	/* have a trier activated -- tell them to try again */
	if (try_again) {
		if (chl->pid) {
			*statusp = YP_NOMORE;	/* try_again overloaded */
			TRACE("yp_matchdns:  EOP - Stop trying");
			return (NEITHER_PROC);
		}
	}
	if (chl) {
		gettimeofday(&(chl->enqtime), &tzp);
	} else
		chl = newchild(map, keydat);

	if (chl == NULL) {
		perror("newchild failed");
		*statusp = YP_YPERR;
		TRACE("yp_matchdns:  EOP - newchild failed");
		return (NEITHER_PROC);
	}
	keydat.dptr[keydat.dsize] = 0;
	IFDBI nres_enabledebug();
	if (byname)
		h = nres_gethostbyname(keydat.dptr, my_done, chl);
	else {
		long            addr;
		addr = inet_addr(keydat.dptr);
		h = nres_gethostbyaddr(&addr, sizeof(addr), AF_INET, my_done, chl);
	}
	if (h == 0) {		/* definite immediate reject */
		IFDBI           printf("imediate reject\n");
		freechild(deqchild(chl));
		*statusp = YP_NOKEY;
		TRACE("yp_matchdns:  EOP - no address for host");
		return (NEITHER_PROC);
	} else if (h == -1) {
		perror("nres failed\n");
		*statusp = YP_YPERR;
		TRACE("yp_matchdns:  EOP - nres failed");
		return (NEITHER_PROC);
	} else {
		chl->pid = h;
		/* should stash transport so my_done can answer */
		if (try_again) {
			*statusp = YP_NOMORE;	/* try_again overloaded */
			TRACE("yp_matchdns:  EOP - stop trying");
			return (NEITHER_PROC);

		}
		chl->xprt = transp;
		if (transp) {
			chl->caller = (* (svc_getcaller(transp)));
			chl->xid = svcudp_getxid(transp);
		}
		TRACE("yp_matchdns:  EOP - successful????");
		return (PARENT_PROC);
	}
}

static int
my_done(n, h, chl, errcode)
	char           *n;	/* opaque */
	struct hostent *h;
	struct child   *chl;
	int errcode;
{
	static char     buf[1024];
	char           *endbuf;
	datum           valdatp;
	int             i;
	SVCXPRT        *transp;
	struct sockaddr_in caller_hold;
	unsigned long   xid_hold;
	struct ypresp_val resp;
	struct timezone tzp;

	TRACE("my_done:  SOP");
	IFDBI printf("my_done %d\n",chl->pid);
	gettimeofday(&(chl->enqtime), &tzp);
	chl->pid = 0;

	if (h == NULL) {
		chl->h_errno = errcode; 
		if (chl->h_errno == TRY_AGAIN)
			resp.status = YP_NOMORE;
		else
			resp.status = YP_NOKEY;
		valdatp.dptr = NULL;
		valdatp.dsize = 0;
	} else {
		chl->h_errno = 0;
		endbuf = buf;
		for (i = 0; h->h_addr_list[i]; i++) {
			sprintf(endbuf, "%s\t%s\n", inet_ntoa(
				   *(struct in_addr *) (h->h_addr_list[i])),
				h->h_name);
			endbuf = &endbuf[strlen(endbuf)];
			if ((&buf[sizeof(buf)] - endbuf) < 300)
				break;
		}
		valdatp.dptr = buf;
		valdatp.dsize = strlen(buf);
		chl->val.dsize = valdatp.dsize;
		chl->val.dptr = malloc(valdatp.dsize);
		if (chl->val.dptr == NULL) {
			perror("my_done");
			freechild(deqchild(chl));
			TRACE("my_done:  lookup failed");
			return (-1);
		}
		memcpy(chl->val.dptr, valdatp.dptr, valdatp.dsize);
		resp.status = YP_TRUE;
	}
	/* try to answer here */

	transp = chl->xprt;
	if (transp) {
		caller_hold = *(svc_getcaller(transp));
		xid_hold = svcudp_setxid(transp, chl->xid);
		*(svc_getcaller(transp)) = chl->caller;
		resp.valdat = valdatp;
		if (!svc_sendreply(transp, xdr_ypresp_val, &resp)) {
		}
		*(svc_getcaller(transp)) = caller_hold;
		svcudp_setxid(transp, xid_hold);
	}
	TRACE("my_done:  EOP - successful");
	return (0);
}

#endif /* DBINTERDOMAIN */

/*
 * The following procedures are used only in the new protocol.
 */

/*
 * This implements the nis "match" function.
 */
void
ypmatch(rqstp, transp) 
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
#ifdef YP_CACHE
	int err = 0;
	struct	entrylst *e = (struct entrylst *) 0;
#endif /* YP_CACHE */

	struct ypreq_key req;
	struct ypresp_val resp;
	char *fun = "ypmatch";

#ifdef DBINTERDOMAIN
	int  didfork = NEITHER_PROC;
#endif /* DBINTERDOMAIN */

	TRACE("ypmatch:  SOP");
	req.domain = req.map = NULL;
	req.keydat.dptr = NULL;
	resp.valdat.dptr = NULL;
	resp.valdat.dsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	if (!svc_getargs(transp, xdr_ypreq_key, &req) ) {
		TRACE("ypmatch:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}
#ifdef YP_CACHE
	if (cache_size > 0)
	    e = search_ypcachemap (req.domain, req.map, req.keydat);

	if (e != 0) { /* entry in cache, copy it over */
		resp.valdat.dptr  = e->key_val.valdat.dptr;
		resp.valdat.dsize = e->key_val.valdat.dsize;
		resp.status	  = YP_TRUE;
	} else {
#endif /* YP_CACHE */

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	   TRACE("ypmatch:  calling ypset_current_map");
	   if (ypset_current_map(req.map, req.domain, &resp.status) &&
               yp_map_access(rqstp, transp, &resp.status)) {

			resp.valdat = fetch(req.keydat);

			if (resp.valdat.dptr != NULL) {
			    resp.status = YP_TRUE;
#ifdef YP_CACHE
			    if (cache_size > 0) {

				/* add_n_prune_list (as a NON_STICKY entry). */
			    	e = add_n_prune_list (
						req.domain, req.map,
						req.keydat, resp.valdat,
						NON_STICKY); /* not sticky */

			    	if (e == (entrylst *)0) {

	 				logmsg(catgets(nlmsg_fd,NL_SETN,7,"out of memory, or bad map or domain. caching failed"));

			        }
			    }
#endif /* YP_CACHE */
			} else {
#ifdef DBINTERDOMAIN
				/*
				 * Try to do an inter-domain binding on the same name
				 * if this is for hosts.byname map.
				 */
				didfork = yp_matchdns(req.map, req.keydat, &resp.valdat, &resp.status, rqstp, transp);
#else
				resp.status = YP_NOKEY;
#endif /* DBINTERDOMAIN */
			}
	}
#ifdef YP_CACHE
	}
#endif /* YP_CACHE */

#ifdef YPSERV_DEBUG
	(void) debug_getval(&resp);
#endif /* YPSERV_DEBUG */

#ifdef DBINTERDOMAIN
	if (didfork != PARENT_PROC)
		if (!svc_sendreply(transp, xdr_ypresp_val, &resp)) {
		   	TRACE("ypmatch: svc_sendreply error");
			RESPOND_ERR;
		}
#else
	if (!svc_sendreply(transp, xdr_ypresp_val, &resp) ) {
		TRACE("ypmatch:  svc_sendreply error");
		RESPOND_ERR;
	}
#endif /* DBINTERDOMAIN */

	if (!svc_freeargs(transp, xdr_ypreq_key, &req) ) {
		TRACE("ypmatch:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("ypmatch:  EOP");
}

/*
 * This implements the nis "get first" function.
 */
void
ypfirst(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_key_val resp;
	char *fun = "ypfirst";

	TRACE("ypfirst:  SOP");
	req.domain = req.map = NULL;
	resp.keydat.dptr = resp.valdat.dptr = NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypfirst:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	TRACE("ypfirst:  calling ypset_current_map");
	if (ypset_current_map(req.map, req.domain, &resp.status) &&
             yp_map_access(rqstp, transp, &resp.status)) {
		TRACE("ypfirst:  calling ypfilter");
		ypfilter(NULL, &resp.keydat, &resp.valdat, &resp.status);
	}

#ifdef YPSERV_DEBUG
	(void) debug_getkeyval(&resp);
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_ypresp_key_val, &resp) ) {
		TRACE("ypfirst:  svc_sendreply error");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypfirst:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("ypfirst:  EOP");
}

/*
 * This implements the nis "get next" function.
 */
void
ypnext(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_key req;
	struct ypresp_key_val resp;
	char *fun = "ypnext";

	TRACE("ypnext:  SOP");
	req.domain = req.map = req.keydat.dptr = NULL;
	req.keydat.dsize = 0;
	resp.keydat.dptr = resp.valdat.dptr = NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, xdr_ypreq_key, &req) ) {
		TRACE("ypnext:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */
	TRACE("ypnext:  calling ypset_current_map");
	if (ypset_current_map(req.map, req.domain, &resp.status) &&
           yp_map_access(rqstp, transp, &resp.status)) {
		TRACE("ypnext:  calling ypfilter");
		ypfilter(&req.keydat, &resp.keydat, &resp.valdat, &resp.status);

	}

#ifdef YPSERV_DEBUG
	(void) debug_getkeyval(&resp);
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_ypresp_key_val, &resp) ) {
		TRACE("ypnext:  svc_sendreply error");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_key, &req) ) {
		TRACE("ypnext:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("ypnext:  EOP");
}

/*
 * This implements the  "transfer map" function.  It takes the domain and
 * map names and the callback information provided by the requester (yppush
 * on some node), and execs a ypxfr process to do the actual transfer.  
 */
/*  HPNFS
 *
 *  It's worthy of mentioning that the variables in this routine labeled
 *  "*proto" are misleading. They should read something like "program",
 *  because they contain a program number, not a protocol. See yppush and
 *  ypxfr code to verify this. - Dave Erickson.
 *
 *  HPNFS
 */
void
ypxfr(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_xfr req;
	char transid[10];
	char proto[15];
	char port[10];
	struct sockaddr_in *caller;
	char *ipaddr;
	int pid;
	char *fun = "ypxfr";

#ifdef YP_UPDATE
	int map_type, map_state, entries;
#endif /* YP_UPDATE */

	TRACE("ypxfr:  SOP");
	req.ypxfr_domain = req.ypxfr_map = req.ypxfr_owner = NULL;
	req.ypxfr_ordernum = 0;
		
#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	if (!svc_getargs(transp, xdr_ypreq_xfr, &req) ) {
		TRACE("ypxfr:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	(void) sprintf(transid, "%d", req.transid);
	(void) sprintf(proto, "%d", req.proto);
	(void) sprintf(port, "%d", req.port);
	caller = svc_getcaller(transp);
	ipaddr = inet_ntoa(caller->sin_addr);

#ifdef YP_UPDATE
	/*
	 * Here we determine whether to refuse the xfer or to accept it. In
	 * order to effect the 'update' without making changes to the protocol,
	 * we have to base our determination of whether or not to xfer a map
	 * on the value of the transaction id, because it is the only piece
	 * that is not interpreted by the servers. Usually it is between 0 and
	 * 999999(microseconds field of time), but could be different. But with
	 * a 'new' yppush, we make sure that the id is set to this range when
	 * running w/o the -u option & is greater when running yppush with the
	 * -u option. THIS CAN BE GUARENTEED iff THE MASTER IS AN HP MACHINE
	 * RUNNING 'new' YP. THIS IS KEY TO THE WHOLE OPERATION. Here, when
	 * the uflag is not on, we will go ahead & xfer the map as usual,
	 * regardless of the value of the tid. ypmake runs yppush with the -u
	 * option if ypmake is run with the UFLAG=1. 
	 *
	 * So, if (uflag) and (transid > YPMAX_TRANSID) and (there is a AUTOXFER
	 * type map in our cache) and the total count of entries in the map is
	 * less than cache_size then we will ignore the transfer request.
	 * Otherwise we will transfer the map. A map is typed AUTO_XFER only if
	 * we have received an 'update_entry' request for that map. - PRABHA.
	 */

	map_type = get_map_type	(req.ypxfr_domain, req.ypxfr_map);
	map_state= get_map_state(req.ypxfr_domain, req.ypxfr_map);
	entries  = get_entries	(req.ypxfr_domain, req.ypxfr_map);

	if	((uflag) &&				/* uflag ok */
		 (! mustpull) &&		/* no mustpull option (-p) */
		 (req.transid >= YPMAX_TRANSID)  &&	/* id within range */
		 (map_type == AUTOXFER_TYPE_MAP) &&	/* id within range */
		 (entries > 0) &&			/* we have a cache map*/
		 (entries < cache_size) &&		/* that isn't too big */
		 (map_state == STEADY_STATE)) { /* & one thats not BAD/BOOTING*/
			logmsg(catgets(nlmsg_fd,NL_SETN,8,"request to ypxfr %s/%s map declined."),req.ypxfr_domain, req.ypxfr_map);
			goto skip_xfer;
	} else {
		/*
		 * If map_state is not STEADY we will accept a transfer. That is
		 * because we have to assume that our map is NOT current when we
		 * came up (or is BAD). To do so we will reset the map, change
		 * it's state to STEADY & then transfer the map. We will assume
		 * that the xfer will succeed. Essentially, a map changes state
		 * to STEADY upon a ypxfr since booting.
		 */
#endif /*YP_UPDATE */

		TRACE("ypxfr:  about to vfork");
		pid = vfork();

		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {

			TRACE("ypxfr:  child about to execl");
			if (execl(ypxfr_proc, "ypxfr", "-d", req.ypxfr_domain, 
		    	"-C", transid, proto, ipaddr, port, req.ypxfr_map, NULL)) {
				TRACE("ypxfr:  child execl error");
				EXEC_ERR;
			}

			TRACE("ypxfr:  child exiting");
			_exit(1);
		}
#ifdef YP_CACHE
		reset_ypcachemap (req.ypxfr_domain, req.ypxfr_map);
#ifdef YP_UPDATE
	}
skip_xfer:
#endif /* YP_UPDATE */
#endif /* YP_CACHE */
	if (!svc_sendreply(transp, xdr_void, 0) ) {
		TRACE("ypxfr:  svc_sendreply error");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_xfr, &req) ) {
		TRACE("ypxfr:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("ypxfr:  EOP");
}

/*
 * This implements the "get all" function.
 */
void
ypall(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
        struct ypresp_val resp; /* not returned to the caller */
	int pid;
	char *fun = "ypall";

	TRACE("ypall:  SOP");
	req.domain = req.map = NULL;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypall:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	TRACE("ypall:  about to fork");
	pid = fork();
	
	if (pid) {
		
		if (pid == -1) {
			FORK_ERR;
		}

		if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
			TRACE("ypall:  svc_freeargs error by parent");
			FREE_ERR;
		}

		TRACE("ypall:  parent EOP");
		return;
	}


	/*
	 * This is the child process.  The work gets done by xdrypserv_ypall
	 * we must clear the "current map" first so that we do not
	 * share a seek pointer with the parent server.
	 */

	/*  We do not want the child, if killed, to unregister ypserv from
	 *  portmap, so each of the signals is handled by the default method.
	 */

	TRACE("ypall:  child setting default signals");
	(void) signal(SIGHUP,  SIG_DFL);
	(void) signal(SIGINT,  SIG_DFL);
	(void) signal(SIGIOT,  SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	TRACE("ypall:  child calling ypclr_current_map");
	ypclr_current_map();

	if (ypset_current_map(req.map, req.domain, &resp.status) &&
	    !yp_map_access(rqstp, transp, &resp.status)) {
			    req.map[0] = '-';
	  }
	if (!svc_sendreply(transp, xdrypserv_ypall, &req) ) {
		TRACE("ypall:  svc_sendreply error by child");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypall:  svc_freeargs error by child");
		FREE_ERR;
	}

	TRACE("ypall:  child EOP");
	exit(0);
}

/*
 * This implements the "get master name" function.
 */
void
ypmaster(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	char *nullstring = "";
	char *fun = "ypmaster";

	TRACE("ypmaster:  SOP");
	req.domain = req.map = NULL;
	resp.master = nullstring;
	resp.status  = YP_TRUE;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypmaster:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	 (void) debug_get_req_args(&req);
#ifdef DEBUG
	 fprintf(stderr,"debg.v1: master mapname = %s\n",debug_infoptr->mapname);
#endif /* DEBUG */
#endif /* YPSERV_DEBUG */

	TRACE("ypmaster:  calling ypset_current_map");
	if (ypset_current_map(req.map, req.domain, &resp.status)) {
		if (!ypget_map_master(req.map, req.domain, &resp.master) ) {
			TRACE("ypmaster:  bad database?");
			resp.status = YP_BADDB;
		}
	}

#ifdef YPSERV_DEBUG
	{ int len;
	debug_infoptr->resp_status = resp.status;
	len = strlen(resp.master);
	debug_infoptr->vallen = len;
        len = (len < YPMAXVAL) ? len : YPMAXVAL-1;
	memcpy(debug_infoptr->val, resp.master, len);
	}
#ifdef DEBUG
	 fprintf(stderr,"debg.v1: master: %s, status = %d\n",resp.master, resp.status);
#endif /* DEBUG */
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_ypresp_master, &resp) ) {
		TRACE("ypmaster:  svc_sendreply error");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypmaster:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("ypmaster:  EOP");
}

/*
 * This implements the "get order number" function.
 */
void
yporder(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_order resp;
	char *fun = "yporder";

	TRACE("yporder:  SOP");
	req.domain = req.map = NULL;
	resp.status  = YP_TRUE;
	resp.ordernum  = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	resp.ordernum = 0;

	TRACE("yporder:  calling ypset_current_map");
	if (ypset_current_map(req.map, req.domain, &resp.status)) {

		if (!ypget_map_order(req.map, req.domain, &resp.ordernum) ) {
			TRACE("yporder:  bad database?");
			resp.status = YP_BADDB;
		}
	}

#ifdef YPSERV_DEBUG
	debug_infoptr->resp_status = resp.status;
	debug_infoptr->result = resp.ordernum;
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_ypresp_order, &resp) ) {
		TRACE("yporder:  svc_sendreply error");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("yporder:  svc_freeargs error");
		FREE_ERR;
	}
	TRACE("yporder:  EOP");
}

void
ypmaplist(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	/* char domain_name[YPMAXDOMAIN + 1]; */
	/* char *pdomain_name = domain_name; */
	char *pdomain_name = (char *)0;
	char *fun = "ypmaplist";
	struct ypresp_maplist maplist;
	struct ypmaplist *tmp;

	TRACE("ypmaplist:  SOP");
	maplist.list = (struct ypmaplist *) NULL;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		TRACE("ypmaplist:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(pdomain_name);
#ifdef DEBUG
	fprintf(stderr,"debg.v2: maplist domainname = %s\n",debug_infoptr->domainname);
#endif /* DEBUG */
#endif /* YPSERV_DEBUG */

	TRACE2("ypmaplist:  calling yplist_maps, domain_name = \"%s\"",
		pdomain_name);
	maplist.status = yplist_maps(pdomain_name, &maplist.list);

#ifdef YPSERV_DEBUG
	debug_infoptr->resp_status = maplist.status;

#ifdef DEBUG
	fprintf(stderr,"debg.v2: maplist status = %d\n",debug_infoptr->resp_status);
#endif /* DEBUG */

#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_ypresp_maplist, &maplist) ) {
		TRACE("ypmaplist:  svc_sendreply error");
		RESPOND_ERR;
	}

	while (maplist.list) {
#ifdef DEBUG
		fprintf(stderr,"debg.v2: mapname: %s\n",maplist.list->ypml_name);
#endif /* DEBUG */
		tmp = maplist.list->ypml_next;
		/* (void) mem_free(maplist.list, sizeof(struct ypmaplist)); */
		(void) free (maplist.list);
		maplist.list = tmp;
	}
	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name)) {
		TRACE("ypmaplist:  svc_freeargs err");
	  	FREE_ERR ;
	}
	TRACE("ypmaplist:  EOP");
}

#ifdef YP_UPDATE
yp_update_mapentry(rqstp, transp)
        struct svc_req *rqstp;
        SVCXPRT *transp;
{
        int err = 0;
        bool is_served = FALSE;
        update_entry ue;
        entrylst                *ep, *p;
        struct ypreq_nokey      *dm;
	char	*fun = "yp_update_mapentry";
#ifdef NLS
        nlmsg_fd = nls_front();
#endif /* NLS */

	/* just return if uflag is not on. We do cache sticky
	 * entries when uflag is on, even if cache_size == 0 */

	if (uflag == 0)
		goto retn;

        /* initialize ue */
        ue.dm = (struct ypreq_nokey *)0;
        ue.ep = (entrylst *)0;

        if (!svc_getargs(transp, xdr_update_entry, &ue)) {
                TRACE("yp_update_map:  svc_getargs error");
                svcerr_decode(transp);
                return;
        }

        /* ue now contains the passwd message to be cache'd */
        dm = ue.dm;
        ep = ue.ep;

	if ((dm) && (ep)) { /* if NULL, bad msg */
           if ((p = search_ypcachemap(dm->domain, dm->map, ep->key_val.keydat))
		  		== NULL) { /* entry not in cache */
		/* is this a domain we serve ? quit if not */
		is_served = (bool) ypcheck_domain(dm->domain);
		if (!is_served) { /* domain not served */ 
			err = 1;
			goto done;
		}
	   } else { /* 'search_ypcachemap' sets the cache_ptrs. */
		/* detach the entry unless it is a duplicate */
		if (p->seq_id != ep->seq_id) {
			detachentry(0, p);
			free (p);
		} else {
			goto done;
		}
	   }
#ifdef DEBUG
	   /* NOTE: no NLSing of DEBUG messages */
	   (void) logmsg("new update: domain = %s, map =%s, val = %s\n",
			  dm->domain, dm->map, (ep->key_val).valdat.dptr);
#endif /* DEBUG */

           /* add to cache as a sticky entry. */
           p = add_to_ypcachemap(dm->domain, dm->map, ep->key_val.keydat, ep->key_val.valdat, STICKY);

	   if (p == (entrylst *)0) {

	   	/* set_map_state(dm->domain, dm->map, BADMAP); */

		logmsg(catgets(nlmsg_fd,NL_SETN,7,"out of memory ! caching failed"));

	    	(void)set_map_type(dm->domain, dm->map, AUTOXFER_TYPE_MAP);
	   }
        } else
		err = 1;
done:
        if (!svc_freeargs(transp, xdr_update_entry, &ue)) {
                TRACE("yp_update_mapentry:  svc_freeargs error");
                FREE_ERR ;
        }
retn:
        if (!svc_sendreply(transp, xdr_int, &err)) {
                TRACE("yp_update_mapentry:  svc_sendreply err");
                RESPOND_ERR;
        }
}
#endif /* YP_UPDATE */
/*
 * The following procedures are used only to support the old protocol.
 */

/*
 * This implements the nis "match" function.
 */
void
ypoldmatch(rqstp, transp) 
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;
	char *fun = "ypoldmatch";

#if DBINTERDOMAIN
	int	didfork = NEITHER_PROC;
#endif /* DBINTERDOMAIN */

	TRACE("ypoldmatch:  SOP");
	req.ypmatch_req_domain = req.ypmatch_req_map = NULL;
	req.ypmatch_req_keyptr = NULL;
	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	if (req.yp_reqtype != YPMATCH_REQTYPE) {
		resp.ypmatch_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypmatch_req_map,
	    req.ypmatch_req_domain, &resp.ypmatch_resp_status) &&
             yp_map_access(rqstp, transp, &resp.ypmatch_resp_status)) {

		resp.ypmatch_resp_valdat = fetch(req.ypmatch_req_keydat);

		if (resp.ypmatch_resp_valptr != NULL) {
			resp.ypmatch_resp_status = YP_TRUE;
		} else {
#ifdef DBINTERDOMAIN
			didfork = yp_matchdns(req.ypmatch_req_map,
					      req.ypmatch_req_keydat,
					      &resp.ypmatch_resp_valdat,
				    &resp.ypmatch_resp_status, rqstp, NULL);
#else
			resp.ypmatch_resp_status = YP_NOKEY;
#endif /* DBINTERDOMAIN */
		}
	  }

	resp.yp_resptype = YPMATCH_RESPTYPE;

#ifdef YPSERV_DEBUG
	(void) debug_getval(&(resp.yp_respbody.yp_resp_valtype));
#endif /* YPSERV_DEBUG */

#ifdef DBINTERDOMAIN
	
	if (didfork != PARENT_PROC)
		if (!svc_sendreply(transp, _xdr_ypresponse, &resp)) {
		   	TRACE("ypoldmatch:  svc_sendreply error");
			RESPOND_ERR;
		}
#else
	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}
#endif /* DBINTERDOMAIN */

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("ypoldmatch:  svc_freeargs err");
		FREE_ERR;
	}
	TRACE("ypoldmatch:  EOP");
}

/*
 * This implements the nis "get first" function.
 */
void
ypoldfirst(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;	
	char *fun = "ypoldfirst";

	TRACE("ypoldfirst:  SOP");
	req.ypfirst_req_domain = req.ypfirst_req_map = NULL;
	resp.ypfirst_resp_keyptr = resp.ypfirst_resp_valptr = NULL;
	resp.ypfirst_resp_keysize = resp.ypfirst_resp_valsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	if (req.yp_reqtype != YPFIRST_REQTYPE) {
		resp.ypfirst_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypfirst_req_map,
	    req.ypfirst_req_domain, &resp.ypfirst_resp_status) &&
            yp_map_access(rqstp, transp, &resp.ypmatch_resp_status)) {

		resp.ypfirst_resp_keydat = firstkey();

		if (resp.ypfirst_resp_keyptr != NULL) {
			resp.ypfirst_resp_valdat =
			    fetch(resp.ypfirst_resp_keydat);

			if (resp.ypfirst_resp_valptr != NULL) {
				resp.ypfirst_resp_status = YP_TRUE;
			} else {
				resp.ypfirst_resp_status = YP_BADDB;
			}

		} else {
			resp.ypfirst_resp_status = YP_NOKEY;
		}
	}

	resp.yp_resptype = YPFIRST_RESPTYPE;

#ifdef YPSERV_DEBUG
	(void) debug_getkeyval(&(resp.yp_respbody.yp_resp_key_valtype));
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("ypoldfirst:  svc_freeargs err");
		FREE_ERR;
	}
	TRACE("ypoldfirst:  EOP");
}

/*
 * This implements the nis "get next" function.
 */
void
ypoldnext(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;
	char *fun = "ypoldnext";

	TRACE("ypoldnext:  SOP");
	req.ypnext_req_domain = req.ypnext_req_map = NULL;
	req.ypnext_req_keyptr = NULL;
	resp.ypnext_resp_keyptr = resp.ypnext_resp_valptr = NULL;
	resp.ypnext_resp_keysize = resp.ypnext_resp_valsize = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	if (req.yp_reqtype != YPNEXT_REQTYPE) {
		resp.ypnext_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypnext_req_map,
	    req.ypnext_req_domain, &resp.ypnext_resp_status) &&
            yp_map_access(rqstp, transp, &resp.ypmatch_resp_status)) {
		resp.ypnext_resp_keydat = nextkey(req.ypnext_req_keydat);

		if (resp.ypnext_resp_keyptr != NULL) {
			resp.ypnext_resp_valdat =
			    fetch(resp.ypnext_resp_keydat);

			if (resp.ypnext_resp_valptr != NULL) {
				resp.ypnext_resp_status = YP_TRUE;
			} else {
				resp.ypnext_resp_status = YP_BADDB;
			}

		} else {
			resp.ypnext_resp_status = YP_NOMORE;
		}

	}

	resp.yp_resptype = YPNEXT_RESPTYPE;

#ifdef YPSERV_DEBUG
	(void) debug_getkeyval(&(resp.yp_respbody.yp_resp_key_valtype));
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("ypoldnext:  svc_freeargs err");
		FREE_ERR;
	}

	TRACE("ypoldnext:  EOP");
}

/*
 * This retrieves the order number and master peer name from the map.
 * The conditions for the various message fields are:
 * 	domain is filled in iff the domain exists.
 *	map is filled in iff the map exists.
 * 	order number is filled in iff it's in the map.
 * 	owner is filled in iff the master peer is in the map.
 */
void
ypoldpoll(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	struct ypresponse resp;
	char *map = "";
	char *domain = "";
	char *owner = "";
	unsigned error;
	char *fun = "ypoldpoll";

	TRACE("ypoldpoll:  SOP");
	req.yppoll_req_domain = req.yppoll_req_map = NULL;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	resp.yppoll_resp_ordernum = 0;

	if (req.yp_reqtype == YPPOLL_REQTYPE) {
		if (strcmp(req.yppoll_req_domain,"yp_private")==0 ||
		    strcmp(req.yppoll_req_map,"ypdomains")==0 ||
		    strcmp(req.yppoll_req_map,"ypmaps")==0 ) {
		  /*
		   * backward compatibility for 2.0 NIS servers
		   */
			domain = req.yppoll_req_domain;
			map = req.yppoll_req_map;
			resp.yppoll_resp_ordernum = 0;		    
		}

		else if (ypset_current_map(req.yppoll_req_map,
		    req.yppoll_req_domain, &error)) {
			domain = req.yppoll_req_domain;
			map = req.yppoll_req_map;
			(void) ypget_map_order(map, domain,
			    &resp.yppoll_resp_ordernum);
			(void) ypget_map_master(map, domain,
				    &owner);
		} else {

			switch (error) {

			case YP_BADDB:
				map = req.yppoll_req_map;
				/* Fall through to set the domain, too. */

			case YP_NOMAP:
				domain = req.yppoll_req_domain;
				break;
			}
		}
	}

	resp.yp_resptype = YPPOLL_RESPTYPE;
	resp.yppoll_resp_domain = domain;
	resp.yppoll_resp_map = map;
	resp.yppoll_resp_owner = owner;

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("ypoldpoll:  svc_freeargs err");
		FREE_ERR;
	}
	TRACE("ypoldpoll:  EOP");
}

/*
 * yppush
 */
void
yppush(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	int pid;
	char *fun = "yppush";

	TRACE("yppush:  SOP");
	req.yppush_req_domain = req.yppush_req_map = NULL;
#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		TRACE("yppush:  svc_getargs err");
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	TRACE("yppush:  about to vfork");
	pid = vfork();

	if (pid == -1) {
		FORK_ERR;
	} else if (pid == 0) {

		ypclr_current_map();

		TRACE("yppush:  child about to execl");
		if (execl(yppush_proc, "yppush", "-d", req.yppush_req_domain, 
		    req.yppush_req_map, NULL) ) {
			TRACE("yppush:  child execl error");
			EXEC_ERR;
		}

		TRACE("yppush:  child exiting");
		_exit(1);
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		TRACE("yppush:  svc_sendreply err");
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("yppush:  svc_freeargs err");
		FREE_ERR;
	}
	TRACE("yppush:  EOP");
}

/*
 * This clears the current map, vforks, and execs the ypxfr process to get
 * the map referred to in the request.
 */
void
ypget(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	int pid;
	char *fun = "ypget";

	TRACE("ypget:  SOP");
	req.ypget_req_domain = req.ypget_req_map = req.ypget_req_owner = NULL;
	req.ypget_req_ordernum = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}

	if (req.yp_reqtype == YPGET_REQTYPE) {

		pid = vfork();

		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {

			ypclr_current_map();

			if (execl(ypxfr_proc, "ypxfr", "-d",
			    req.ypget_req_domain, "-h", req.ypget_req_owner,
			    req.ypget_req_map, NULL) ) {
				EXEC_ERR;
			}

			_exit(1);
		}
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("ypget:  svc_freeargs err");
		RESPOND_ERR;
	}

	TRACE("ypget:  EOP");
}

/*
 * This clears the current map, vforks, and execs the ypxfr process to get
 * the map referred to in the request.
 */
void
yppull(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	int pid;
	char *fun = "yppull";

	TRACE("yppull:  SOP");
	req.yppull_req_domain = req.yppull_req_map = NULL;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

#ifdef YPSERV_DEBUG
	(void) debug_get_req_args(&req);
#endif /* YPSERV_DEBUG */

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}

	if (req.yp_reqtype == YPPULL_REQTYPE) {

		pid = vfork();

		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {

			ypclr_current_map();

			if (execl(ypxfr_proc, "ypxfr", "-d",
			    req.yppull_req_domain, req.yppull_req_map, NULL) ) {
				EXEC_ERR;
			}

			_exit(1);
		}
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		TRACE("yppull:  svc_freeargs err");
		FREE_ERR;
	}

	TRACE("yppull:  EOP");
}

/*
 * Ancillary functions used by the top-level functions within this module
 */

/*
 * This returns TRUE if a given key is a yp-private symbol, otherwise FALSE
 */
static bool
isypsym(key)
	datum *key;
{
	struct yppriv_sym *psym;

	TRACE("isypsym:  SOP");
#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (key->dptr == NULL) {
		return (FALSE);
	}

	for (psym = filter_set; psym->sym; psym++) {

		if (psym->len == key->dsize &&
		    !memcmp(psym->sym, key->dptr, key->dsize) ) {
			TRACE("isypsym:  EOP - true");
			return (TRUE);
		}
	}

	TRACE("isypsym:  EOP - false");
	return (FALSE);
}

/*
 * This provides private-symbol filtration for the enumeration functions.
 */
static void
ypfilter(inkey, outkey, val, status)
	datum *inkey;
	datum *outkey;
	datum *val;
	int *status;
{
	datum k;

	TRACE("ypfilter:  SOP");
#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	if (inkey) {

		if (isypsym(inkey) ) {
			*status = YP_BADARGS;
			TRACE("ypfilter:  EOP - status = YP_BADARGS");
			return;
		}

		k = nextkey(*inkey);
	} else {
		k = firstkey();
	}

	while (k.dptr && isypsym(&k)) {
		k = nextkey(k);
	}

	if (k.dptr == NULL) {
		*status = YP_NOMORE;
		TRACE("ypfilter:  EOP - status = YP_NOMORE");
		return;
	}

	*outkey = k;
	*val = fetch(k);

	if (val->dptr != NULL) {
		*status = YP_TRUE;
	} else {
		*status = YP_BADDB;
	}
	TRACE2("ypfilter:  EOP - status = %d", *status);
}

/*
 * Serializes a stream of struct ypresp_key_val's.  This is used
 * only by the ypserv side of the transaction.
 */
static bool
xdrypserv_ypall(xdrs, req)
	XDR * xdrs;
	struct ypreq_nokey *req;
{
	bool more = TRUE;
	struct ypresp_key_val resp;

	TRACE("xdrypserv_ypall:  SOP");
	resp.keydat.dptr = resp.valdat.dptr = (char *) NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;
#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	TRACE("xdrypserv_ypall:  calling ypset_current_map");
	if (ypset_current_map(req->map, req->domain, &resp.status)) {
		TRACE("xdrypserv_ypall:  calling ypfilter 1");
		ypfilter(NULL, &resp.keydat, &resp.valdat, &resp.status);

		while (resp.status == YP_TRUE) {
			if (!xdr_bool(xdrs, &more) ) {
				TRACE("xdrypserv_ypall:  EOP - false1");
				return (FALSE);
			}

			if (!xdr_ypresp_key_val(xdrs, &resp) ) {
				TRACE("xdrypserv_ypall:  EOP - false2");
				return (FALSE);
			}

			TRACE("xdrypserv_ypall:  calling ypfilter 2");
			ypfilter(&resp.keydat, &resp.keydat, &resp.valdat,
			    &resp.status);
		}

	}

	if (!xdr_bool(xdrs, &more) ) {
		TRACE("xdrypserv_ypall:  EOP - false3");
		return (FALSE);
	}

	if (!xdr_ypresp_key_val(xdrs, &resp) ) {
		TRACE("xdrypserv_ypall:  EOP - false4");
		return (FALSE);
	}

	more = FALSE;

	if (!xdr_bool(xdrs, &more) ) {
		TRACE("xdrypserv_ypall:  EOP - false5");
		return (FALSE);
	}

	TRACE("xdrypserv_ypall:  EOP - true");
	return (TRUE);
}

#ifdef DBINTERDOMAIN

char *strdup(s1)
char *s1;
{
    char *s2;
    extern char *malloc(), *strcpy();

    s2 = malloc(strlen(s1)+1);
    if (s2 != NULL)
        s2 = strcpy(s2, s1);
    return(s2);
}

#endif /* DBINTERDOMAIN */
