/* @(#) $Revision: 70.2 $ */

/*
 * Routines and data common to all the line printer functions.
 */
#include "lp.h"

#define bzero(a,b)	memset(a,0,b)
#define bcopy(a,b,c)	memcpy(b,a,c)


/*
 * Create a connection to the remote printer server.
 * Most of this code comes from rcmd.c.
 */
getport(rhost)
	char *rhost;
{
	char	work[BUFSIZ];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s, lport = IPPORT_RESERVED - 1;
	long	timo = 1;
	int err;

	/*
	 * Get the host address and port number to connect to.
	 */
	if (rhost == NULL)
		fatal("no remote host to connect to",1);
	hp = gethostbyname(rhost);
	if (hp == NULL){
		sprintf(work,"unknown host %s", rhost);
		fatal(work,1);
	}
	sp = getservbyname("printer", "tcp");
	if (sp == NULL)
		fatal("printer/tcp: unknown service",1);
	bzero((char *)&sin, sizeof(sin));
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = sp->s_port;

	/*
	 * Try connecting to the server.
	 */
retry:
	s = rresvport(&lport);
	if (s < 0)
		return(-1);
	if (connect(s, (caddr_t)&sin, sizeof(sin), 0) < 0) {
		err = errno;
		(void) close(s);
		errno = err;
		if (errno == EADDRINUSE) {
			lport--;
			goto retry;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		return(-1);
	}
	return(s);
}

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return(-1);
	for (; *alport > IPPORT_RESERVED/2; (*alport)--) {
		sin.sin_port = htons((u_short) *alport);
		if (bind(s, (caddr_t)&sin, sizeof(sin), 0) >= 0)
			return(s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL)
			break;
	}
	(void) close(s);
	return(-1);
}


/* This procedure will query the system for the hostname of the rootserver
   of a given client.  If the given client is either a standalone system or
   the rootserver of the cluster, then host will simply be copied to rootname.  
   Otherwize, the clusterconf file will be searched for the rootserver entry.
   The rootserver identity will then be copied to rootname.           */

int getrootname(host, rootname)
char *host;
char *rootname;
{
    int cnodecount;
    int i;
    char *phost;
    struct cct_entry *clusterentry;
    char shortname[SP_MAXHOSTNAMELEN];

	if (!rootname)		/* rootname must be allocated prior to call */
	    return(-1);
	rootname[0] = '\0';
        phost=host;

	if ((cnodecount = cnodes((cnode_t *)NULL)) == -1 )
	    return(-1); 	/* error */

	if (cnodecount == 0) 	/* standalone system */
	{
	    strncpy(rootname, host, SP_MAXHOSTNAMELEN);
	    return(0);
	}

        for (i=0; *phost != NULL && *phost != '.' && i < SP_MAXHOSTNAMELEN; i++) {
               shortname[i]= *phost;
               *phost++;
        }
        shortname[i]='\0';
	if ((clusterentry = getccnam(shortname)) == NULL)
		return(-1);

	if (clusterentry->cnode_type == 'r') /* host is rootserver */
	{
	    	strncpy(rootname, host, SP_MAXHOSTNAMELEN);
		return(0);
	}

	  	/* host is a client, search for rootserver */
	setccent();	/* rewind clusterconf file */
	while (clusterentry = getccent())
	{
	    if (clusterentry->cnode_type == 'r')
	    {
		strncpy(rootname, clusterentry->cnode_name,
		         SP_MAXHOSTNAMELEN);
		endccent();
		return(0);
	    }
	}
	endccent();
	return(-1);

} /* getrootname */
