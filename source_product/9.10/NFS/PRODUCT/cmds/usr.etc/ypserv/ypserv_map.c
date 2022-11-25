/*	@(#)ypserv_map.c	$Revision: 1.26.109.5 $	$Date: 94/12/16 09:07:38 $  
*/
/* ypserv_map.c	2.1 86/04/16 NFSSRC */
/*static char sccsid[] = "ypserv_map.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: ypserv_map.o $Revision: 1.26.109.5 $ 94/06/01 PHNE_5081";
#endif

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else /* NLS */
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif /* NLS */

#include "ypsym.h"
#include <ctype.h>

#ifdef YP_CACHE
#include "yp_cache.h"
#endif /* YP_CACHE */

#ifdef	TRACEON
# define LIBTRACE
#endif
#include <arpa/trace.h>

char current_map[MAXPATHLEN + 1];
extern bool logging;
extern int logmsg();
extern get_secure_nets();
extern check_secure_net();
static char map_owner[MAX_MASTER_NAME + 1];

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
                nlmsg_fd1 = catopen("ypserv_map",0);
                first_time = 0;
        }
        return(nlmsg_fd1);
}
#endif /* NLS */

/*
 * This performs an existence check on the dbm data base files <name>.pag and
 * <name>.dir.  pname is a ptr to the filename.  This should be an absolute
 * path.
 * Returns TRUE if the map exists and is accessable; else FALSE.
 *
 * Note:  The file name should be a "base" form, without a file "extension" of
 * .dir or .pag appended.  See ypmkfilename for a function which will generate
 * the name correctly.  Errors in the stat call will be reported at this level,
 * however, the non-existence of a file is not considered an error, and so will
 * not be reported.
 */
bool
ypcheck_map_existence(pname)
	char *pname;
{
	char dbfile[MAXPATHLEN + 1];
	struct stat filestat;
	int len;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	TRACE2("ypcheck_map_existence:  SOP, pname = \"%s\"", pname);
	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + 5) > (MAXPATHLEN + 1) ) {
		TRACE("ypcheck_map_existence:  bad mapname");
		return(FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, ".dir");

	TRACE2("ypcheck_map_existence:  stating pname = \"%s\"", pname);
	if (stat(dbfile, &filestat) != -1) {
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, ".pag");

		if (stat(dbfile, &filestat) != -1) {
			TRACE("ypcheck_map_existence:  EOP - pname found");
			return(TRUE);
		} else {

			if (errno != ENOENT) {
				if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,1, "Stat error on map file %s"), dbfile);
			}

			TRACE("ypcheck_map_existence:  EOP - pname not found");
			return(FALSE);
		}

	} else {

		if (errno != ENOENT) {
			if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,2, "Stat error on map file %s"), dbfile);
		}


		TRACE("ypcheck_map_existence:  EOP - pname not found");
		return(FALSE);
	}
}

/*
 * The retrieves the order number of a named map from the order number datum
 * in the map data base.  
 */
bool
ypget_map_order(map, domain, order)
	char *map;
	char *domain;
	unsigned *order;
{
	datum key;
	datum val;
	char toconvert[MAX_ASCII_ORDER_NUMBER_LENGTH + 1];
	unsigned error = 0;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	TRACE3("ypget_map_order:  SOP - map = \"%s\", domain = \"%s\"",
		map, domain);
	TRACE("ypget_map_order:  calling ypset_current_map");
	if (ypset_current_map(map, domain, &error) ) {
		key.dptr = order_key;
		key.dsize = ORDER_KEY_LENGTH;
		val = fetch(key);

		if (val.dptr != (char *) NULL) {

			if (val.dsize > MAX_ASCII_ORDER_NUMBER_LENGTH) {
				return(FALSE);
			}

			/*
			 * This is getting recopied here because val.dptr
			 * points to static memory owned by the dbm package,
			 * and we have no idea whether numeric characters
			 * follow the order number characters, nor whether
			 * the mess is null-terminated at all.
			 */

			TRACE("ypget_map_order:  doing memcpy");
			memcpy(toconvert, val.dptr, val.dsize);
			toconvert[val.dsize] = '\0';
			*order = (unsigned long) atol(toconvert);
			TRACE("ypget_map_order:  EOP - true");
			return(TRUE);
		} else {
			TRACE("ypget_map_order:  EOP - false1");
			return(FALSE);
		}
		    
	} else {
		TRACE("ypget_map_order:  EOP - false2");
		return(FALSE);
	}
}

/*
 * The retrieves the master server name of a named map from the master datum
 * in the map data base.  
 */
bool
ypget_map_master(map, domain, owner)
	char *map;
	char *domain;
	char **owner;
{
	datum key;
	datum val;
	unsigned error;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	TRACE3("ypget_map_master:  SOP - map = \"%s\", domain = \"%s\"",
		map, domain);
	TRACE("ypget_map_master:  calling ypset_current_map");
	if (ypset_current_map(map, domain, &error) ) {
		key.dptr = master_key;
		key.dsize = MASTER_KEY_LENGTH;
		val = fetch(key);

		if (val.dptr != (char *) NULL) {
			if (val.dsize > MAX_MASTER_NAME) {
				return(FALSE);
			}

			/*
			 * This is getting recopied here because val.dptr
			 * points to static memory owned by the dbm package.
			 */
			TRACE("ypget_map_master:  doing memcpy");
			memcpy(map_owner, val.dptr, val.dsize);
			map_owner[val.dsize] = '\0';
			*owner = map_owner;
			TRACE("ypget_map_master:  EOP - true");
			return(TRUE);
		} else {
			TRACE("ypget_map_master:  EOP - false1");
			return(FALSE);
		}
		    
	} else {
		TRACE("ypget_map_master:  EOP - false2");
		return(FALSE);
	}
}

/*
 * This makes a map into the current map, and calls dbminit on that map so
 * that any successive dbm operation is performed upon that map.  Returns an
 * YP_xxxx error code in error if FALSE.  
 */
bool
ypset_current_map(map, domain, error)
	char *map;
	char *domain;
	unsigned *error;
{
	char mapname[MAXPATHLEN + 1];
	int lenm, lend;

#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */
	TRACE3("ypset_current_map:  SOP - map = \"%s\", domain = \"%s\"",
		map, domain);
	if (!map || ((lenm = strlen(map)) == 0) || (lenm > YPMAXMAP) ||
	    !domain || ((lend = strlen(domain)) == 0) || (lend > YPMAXDOMAIN)) {
		*error = YP_BADARGS;
		TRACE("ypset_current_map:  EOP - bad args");
		return(FALSE);
	}

	TRACE("ypset_current_map:  calling ypmkfilename");
	switch (ypmkfilename(domain, map, mapname)) {
		case PATH_TOO_LONG:		/*  A rare (if ever) event!  */
			if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,3, "The map pathname, \"%-30s...\", is too long:\r\nit should not exceed %d characters"),
					ypdbpath, MAXPATHLEN);
			*error = YP_BADARGS;
			TRACE("ypset_current_map:  EOP - PATH_TOO_LONG");
			return (FALSE);
		case MAPNAME_TOO_LONG:
			*error = YP_NOMAP;
			TRACE("ypset_current_map:  EOP - MAPNAME_TOO_LONG");
			return (FALSE);
		case MAPNAME_OK:		/*  The filename is OK.  */
			TRACE("ypset_current_map:  EOP - MAPNAME_OK");
	}

	if (strcmp(mapname, current_map) == 0) {
		TRACE("ypset_current_map:  mapname already current map");
		return(TRUE);
	}

	/* close any dbm maps that are open and set current_map to null */
	ypclr_current_map();

	if (dbminit(mapname) >= 0) {
		TRACE("ypset_current_map:  EOP - mapname set as current map");
		(void) strcpy(current_map, mapname);
		return(TRUE);
	}
	TRACE("ypset_current_map:  dbminit of mapname failed");

	/* close any dbm maps that are open and set current_map to null */
	ypclr_current_map();
	
	if (ypcheck_domain(domain)) {

		if (ypcheck_map_existence(mapname)) {
			*error = YP_BADDB;
		} else {
			*error = YP_NOMAP;
		}
		
	} else {
		*error = YP_NODOM;
	}

	TRACE2("ypset_current_map:  EOP - error = %d", *error);
	return(FALSE);
}
bool
 yp_map_access(rqstp, transp, error)
       struct svc_req *rqstp;
       SVCXPRT *transp;
       unsigned *error;
{
       char *ypname;
       struct sockaddr_in *caller;

       TRACE("yp_map_access:  SOP");

       caller = svc_getcaller(transp);
       ypname = "ypserv";
       if (!(check_secure_net(caller,ypname))) {
               *error = YP_NOMAP;
               TRACE("yp_map_access:  EOP - caller not secure, no access");
               return(FALSE);
       }

       return (TRUE);
}
/*
 * This checks to see if there is a current map, and, if there is, does a
 * dbmclose on it and sets the current map name to null.  
 */
void
ypclr_current_map()

{

	TRACE("ypclr_current_map:  SOP");
	if (current_map[0] != '\0') {
		(void) dbmclose(current_map);
		current_map[0] = '\0';
	}
	TRACE("ypclr_current_map:  EOP");
}

#ifdef YP_CACHE
ypclear_map(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	struct ypreq_nokey req;
	char	*domainname;
	char	*mapname;
        char	*fun = "ypclear_map";


#ifdef NLS
	nlmsg_fd = nls_front();
#endif /* NLS */

	TRACE("ypclear_map:  SOP");

        req.domain = req.map = NULL;

	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypclear_map:  svc_getargs error");
		svcerr_decode(transp);
		return;
	}

	domainname = req.domain;
	mapname    = req.map;

	/*
	 * We used to remove tha map here. But that may invalidate some entries
	 * that came in (when YP_UPDATE is in effect) while the map is being
	 * transferred. So now we reset the map in the ypxfr entry in
	 * ypsrv_proc.c. Now that the xfer is complete set the map in cache to
	 * STEADY STATE.
	 */

        (void) set_map_state (domainname, mapname, STEADY_STATE);
	/*
	 * (void) remove_ypcachemap (domainname, mapname);
	 *
	 */

	if (strcmp(mapname, current_map) == 0) {
		(void) ypclr_current_map();
	}

#ifdef DEBUG
	/* Note: no NLSing of DEBUG messages. */
	if (logging) (void) logmsg("%s cache invalidated",mapname);
#endif /* DEBUG */

	if (!svc_sendreply(transp, xdr_void, 0)) {
		TRACE("ypclear_map:  YPPROC_CLEARMAP svc_sendreply failed");
		if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,5, "YPPROC_CLEARMAP can't reply to rpc call"));
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		TRACE("ypmatch:  svc_freeargs error");
		if (logging) (void) logmsg(catgets(nlmsg_fd,NL_SETN,4, "%s() can't free args"), fun);
	}

        TRACE("ypclear_map:  EOP");
}
#endif /* YP_CACHE */
