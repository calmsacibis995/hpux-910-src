/*	@(#)yp_master.c	$Revision: 12.1 $	$Date: 90/03/21 11:01:17 $  */
/*
yp_master.c	2.1 86/04/14 NFSSRC
static  char sccsid[] = "yp_master.c 1.1 86/02/03 Copyr 1985 Sun Micro";
*/

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define sleep 			_sleep
#define strcmp 			_strcmp
#define strcpy 			_strcpy
#define strlen 			_strlen
#define xdr_ypreq_nokey 	_xdr_ypreq_nokey
#define xdr_ypresp_master 	_xdr_ypresp_master
#define yp_master 		_yp_master		/* In this file */
#define yp_unbind 		_yp_unbind
#define ypprot_err 		_ypprot_err

#ifdef _ANSIC_CLEAN
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */

#endif /* _NAMESPACE_CLEAN */

#define NULL 0
#include <time.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

int v1domaster();
int v2domaster();

extern struct timeval _ypserv_timeout;
extern int _yp_dobind();
extern unsigned int _ypsleeptime;
extern char *malloc();

/*
 * This checks parameters, and implements the outer "until binding success"
 * loop.
 */

#ifdef _NAMESPACE_CLEAN
#undef yp_master
#pragma _HP_SECONDARY_DEF _yp_master yp_master
#define yp_master _yp_master
#endif

int
yp_master (domain, map, master)
	char *domain;
	char *map;
	char **master;
{
	int domlen;
	int maplen;
	int reason;
	struct dom_binding *pdomb;
	int (*dofun)();

	if ( (map == NULL) || (domain == NULL) ) {
		return (YPERR_BADARGS);
	}
	
	domlen = strlen(domain);
	maplen = strlen(map);
	
	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (master == NULL) ) {
		return (YPERR_BADARGS);
	}

	for (;;) {
		
		if (reason = _yp_dobind(domain, &pdomb) ) {
			return (reason);
		}

		dofun = (pdomb->dom_vers == YPVERS) ? v2domaster : v1domaster;

		reason = (*dofun)(domain, map, pdomb, _ypserv_timeout,
		    master);

		if (reason == YPERR_RPC) {
			yp_unbind(domain);
			(void) sleep(_ypsleeptime);
		} else {
			break;
		}
	}
	
	return (reason);
}

/*
 * This talks v2 to ypserv
 */
static int
v2domaster (domain, map, pdomb, timeout, master)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **master;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	unsigned int retval = 0;

	req.domain = domain;
	req.map = map;
	resp.master = NULL;

	/*
	 * Do the get_master request.  If the rpc call failed, return with
	 * status from this point.  
	 */
	
	if(clnt_call(pdomb->dom_client, YPPROC_MASTER, xdr_ypreq_nokey, &req,
	    xdr_ypresp_master, &resp, timeout) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	/* See if the request succeeded */
	
	if (resp.status != YP_TRUE) {
		retval = ypprot_err((unsigned) resp.status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval && ((*master = malloc(
	    (unsigned) strlen(resp.master) + 1)) == NULL)) {
		retval = YPERR_RESRC;

	}

	if (!retval) {
		strcpy(*master, resp.master);
	}
	
	CLNT_FREERES(pdomb->dom_client, xdr_ypresp_master, &resp);
	return (retval);
}

/*
 * This talks v1 to ypserv
 */
static int
v1domaster(domain, map, pdomb, timeout, master)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **master;
{
	struct yprequest req;
	struct ypresponse resp;
	enum clnt_stat clnt_stat;
	unsigned int retval = 0;
	
	req.yp_reqtype = YPPOLL_REQTYPE;
	req.yppoll_req_domain = domain;
	req.yppoll_req_map = map;
	resp.yppoll_resp_domain = resp.yppoll_resp_map =
	    resp.yppoll_resp_owner = (char *) NULL;
	    
	/*
	 * Simulate a v2 "get master" request by doing a v1 "poll map"
	 * request, interpreting the response into current ypclnt.h
	 * return values, or returning the map master.
	 */
	if( (clnt_stat = (enum clnt_stat) clnt_call(pdomb->dom_client,
	    YPOLDPROC_POLL, _xdr_yprequest, &req, _xdr_ypresponse,
	    &resp, timeout) ) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	if (resp.yp_resptype != YPPOLL_RESPTYPE) {
		return (YPERR_YPERR);
	}

	if (!strcmp(resp.yppoll_resp_domain, domain) ) {

		if (!strcmp(resp.yppoll_resp_map, map) ) {
			
			if (! strcmp(resp.yppoll_resp_owner, "") ) {
				retval = YPERR_BADDB;
			}
				
		} else {
			retval = YPERR_MAP;
		}
		
	} else {
		retval = YPERR_DOMAIN;
	}

	if (!retval && ((*master = malloc(
	    (unsigned) strlen(resp.yppoll_resp_owner) + 1)) == NULL)) {
		retval = YPERR_RESRC;
	}

	if (!retval) {
		strcpy(*master, resp.yppoll_resp_owner);
	}
	
	CLNT_FREERES(pdomb->dom_client, _xdr_ypresponse, &resp);
	return (retval);
}
