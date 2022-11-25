/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** Portions are:
**
** Copyright (c) 1983 Regents of the University of California.
** All rights reserved.  The Berkeley software License Agreement
** specifies the terms and conditions for redistribution.
**
** from 1.38 88/02/07 SMI; from UCB 5.14 (Berkeley) 1/23/89
**
** CONFIG.C
**
** This file contains routines which parse the configuration file,
** manage the table of servers, and set up inetd to listen on 
** the appropriate ports so that it can start the servers.
*/

/*
static char rcsid[] = "$Header: config.c,v 1.8.109.9 94/11/18 10:50:27 mike Exp $";
*/

#include "inetd.h"
#include <sys/resource.h>

#define SWAP(type, a, b) { type c = a; a = b; b = c; }

int      lines; /* Used to count lines */

/* Local functions */

static char		*_attachline(),
			*_nextline(),
			**makeargv(),
			*lstrdup();

static int		argvcmp(),
			chgentry(),
			setconfig(),
			strtonum();

static struct servtab 	*enter(),	
			*findserv(), 
			*findrpcserv(),
			*getconfigent(),
			*rpcpair();

static void	 	addserv(), 
			allocserv(),
			checkinit(), 
			_config(),
			_configrpc(),
			endconfig(),
			freeconfig(),
			purgetab(),
			setuprpc();


/*
** CONFIG
**
**    Grab each configuration file entry, add it to the server
**    table or replace an existing entry.  Purge entries which no longer
**    appear in the config file.
*/

config()
{
	register struct servtab  *cp;
	long omask;

	/* Open the configuration file */
	if (!setconfig()) 
	{
		syslog(LOG_ERR, "%s: %m", CONFIG_FILE);
		return -1;
	}

	/* Unmark the "se_checked" fields */
	checkinit();

	/* 
	  Read the configuration file and pass each 
	  entry to the appropriate "config" function.
	*/
	while (cp = getconfigent()) 
		cp->se_isrpc ? _configrpc(cp) : _config(cp);

	/* Close the configuration file */
	endconfig();

	omask = sigblock(SIGBLOCK);
	/* Purge server table ("servtab") of inactive entries */
	purgetab();
	(void) sigsetmask(omask);

	/* Setup the RPC-based services */
	setuprpc();

	return 0;
}

/*
** SETCONFIG
**
**    Open the configuration file.
*/

static int
setconfig()
{

    lines = 0;
    if (fconfig != NULL) {
	fseek(fconfig, 0L, SEEK_SET);
	return (1);
    }
    fconfig = fopen(CONFIG_FILE, "r");
    return (fconfig != NULL);
}

/*
** ENDCONFIG
**
**    Close the configuration file.
*/

static void
endconfig()
{
    if (fconfig) {
	(void) fclose(fconfig);
	fconfig = NULL;
    }
}


/*
** _NEXTLINE
**
**    Get next line of the configuration file.
*/

static char *
_nextline(ignoreblklines)
   int ignoreblklines;		/* if true ignore blank lines */
{
    char *cp;
    static char	  line[MAXLINE+1];
    FILE *filep;
	
    filep = fconfig;
    while (TRUE) {
	if ((cp = fgets(line,MAXLINE,filep)) == NULL)
	    return (char *)NULL;
	lines++;
	/* ignore blank lines unless we are attaching to previous */
	/* line in which case it should not be ignored */
	if ((*cp == '\n') && (ignoreblklines))
	    continue;		/* skip blank lines */
	if ((*cp == '#') && (ignoreblklines))
	    continue;		/* skip comment lines */
	return(line);
    }
}

/*
** _ATTACHLINE
**
**    If the line is continued (ends with a \), attach the next line
**    to it.
*/

static char *
_attachline()
{
    char *p, *cp;
    static char *saveline = NULL;
    int len, newlen;

    /*
    **	_attachline is responsible for freeing all memory allocated
    **	for reading configuration/security file lines.
    */
    if (saveline != NULL) {
	free(saveline);
	saveline = (char *)NULL;
    }

    /*
    **	get the next line ignoring blank lines
    */
    if ((p = _nextline(1)) == NULL)
	return (char *)NULL;

    /* saveline allocated here; it is freed
    **	by this instance of _attachline if about to return NULL,
    **	otherwise by the next instance of _attachline.
    */
    saveline = lstrdup(p);
    cp = saveline;
    while ((*cp != '\0') && (*cp != '\n')) {
	if (*cp == '\\') {
	    *cp = ' ';
	    cp++;
	    if (*cp == '\0')
		break;
	    if (*cp == '\n') {
		/*
		** p will point to the next (possibly blank
		** or null) line
		*/
		p = _nextline(0);
		if (p == NULL) {
		    /*
		    ** allows the last line of the
		    ** file to end with a backslash.
		    ** This break will cause us to
		    ** exit the while loop normally,
		    ** that is, with *cp == \n . 
		    */
		    break;
		}
		/*
		** len gets number of characters preceding
		** '\n' in saveline
		*/
		len = cp - saveline;
		/*
		** newlen gets number of characters in p
		** including '\n' if any
		*/
		newlen = strlen(p);
		/*
		** saveline gets enough space for:
		** the existing data without its '\n', 
		** the new line, including its '\n' if any,
		** and the '\0' at the end of the new line.
		** See comment above about freeing saveline
		*/
		saveline = (char *)realloc(saveline, len + newlen + 1);
		/*
		** establish proper relationship between
		** cp and new location of saveline.
		*/
		cp = saveline + len;
		strcpy(cp,p);
		/*
		** cp now points to the beginning of the
		** new line, which has been appended to
		** saveline.  The '\n' at which it was
		** pointing has been overwritten. Thus
		** we should not increment cp.
		*/
	    }
	    else
		syslog(LOG_WARNING, "%s: \\ found before end of line %d",
		       CONFIG_FILE, lines);
	}
	else {
	    /*
	    ** increment cp only if we have not attached a line
	    */
	    cp++;
	}
    }
    if (*cp == '\0') {
	/*
	**	reached end of line without finding newline; line too long.
	*/
	free(saveline);
	saveline = (char *)NULL;
    }
    return(saveline);
}


/*
** GETCONFIGENT
**
**    Parse a line of the configuration file and return
**    a corresponding server info table entry.  Return NULL
**    if there are no more lines in the file.
*/

static struct servtab *
getconfigent()
{
    static struct servtab serv, *sep;
    char *p, *cp; 
    char *line = (char *)NULL;
    struct rpcent *r;

#define FORMATERR "%s: line %d: %s"

    sep = &serv;
    for ( ; (line = _attachline()) != NULL; freeconfig(sep)) {
	/*
	**  The char * fields of *sep are each made to point into
	**  line, which is allocated by _attachline.  The strings must
	**  be copied before _attachline is called again, because 
	**  _attachline frees line on entry.
	*/
    	bzero((char *) sep, sizeof(struct servtab));
	sep->se_service = lstrdup(strtok(line," \t\n"));
	if (sep->se_service == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
		    lines, "Missing service");
	    continue;
	}
	cp = strtok(NULL," \n\t");
	if (cp == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE, 
		    lines, "Missing socket type");
	    continue;
	}
	if (strcmp(cp, "stream") == 0)
	    sep->se_socktype = SOCK_STREAM;
	else if (strcmp(cp, "dgram") == 0)
	    sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(cp, "rdm") == 0)
	    sep->se_socktype = SOCK_RDM;
	else if (strcmp(cp, "seqpacket") == 0)
	    sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(cp, "raw") == 0)
	    sep->se_socktype = SOCK_RAW;
	else {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
		    lines, "Unsupported socket type");
	    continue;
	}
	sep->se_proto = lstrdup(strtok(NULL," \n\t"));
	if (sep->se_proto == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
		    lines, "Missing protocol");
	    continue;
	}
	cp = strtok(NULL," \n\t");
	if (cp == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE, 
		    lines, "Missing wait/nowait");
	    continue;
	}
	sep->se_wait = (strcmp(cp,"wait") == 0);
	sep->se_endwait = 0;
	sep->se_user = lstrdup(strtok(NULL," \n\t"));
	if (sep->se_user == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE, 
		    lines, "Missing user name");
	    continue;
	}
	sep->se_server = lstrdup(strtok(NULL," \n\t"));
	if (sep->se_server == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
		    lines, "Missing server");
	    continue;
	}
	if (strcmp(sep->se_server, "internal") == 0) {
	    register struct biltin *bi;
		
	    for (bi = biltins; bi->bi_service; bi++)
		if (bi->bi_socktype == sep->se_socktype &&
		    strcmp(bi->bi_service, sep->se_service) == 0)
		    break;
	    if (bi->bi_service == 0) {
		syslog(LOG_ERR, "%s: Internal service unknown",
			servicename(sep));
		continue;
	    }
	    sep->se_bi = bi;
	    sep->se_wait = bi->bi_wait;
	    sep->se_argv = NULL;
	    sep->se_isrpc = 0;
	    badfile = FALSE;
	    break;
	} 
	else
	    sep->se_bi = NULL;
	/* 
	** If the server is an rpc server then we need to
	** check for program and version numbers.	 These
	** numbers are used by the protocol to recognize
	** with which server they want to talk to.
	*/
	sep->se_isrpc = !strcmp(sep->se_service, "rpc");
	if (sep->se_isrpc) {
	    cp = strtok(NULL," \n\t");
	    if (cp == NULL) {
		syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
			lines, "Missing program number");
		continue;
	    }
	    sep->se_rpc.prog = strtonum(cp);
	    /* if the program number is not a number error */
	    if (sep->se_rpc.prog == (unsigned)-1) {
		syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
			lines, "Invalid program number");
		continue;
	    }
	    r = getrpcbynumber(sep->se_rpc.prog);
	    if (r != (struct rpcent *)NULL)
		sep->se_rpc.name = lstrdup(r->r_name);
	    else
		sep->se_rpc.name = lstrdup(ultoa(sep->se_rpc.prog));
	    cp = strtok(NULL," \n\t");
	    if (cp == NULL) {
		syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
			lines, "Missing version number");
		continue;
	    }

	    /* if there is a range */
	    if ((p = strchr(cp,'-')) != NULL ) {
		if (p == cp) {
		    syslog(LOG_ERR, FORMATERR, 
			    CONFIG_FILE,
			    lines,
			    "Missing low number in range");
		    continue;
		}
		*p = '\0';
		p++ ;
		if (*p == '\0') {
		    syslog(LOG_ERR, FORMATERR, 
			    CONFIG_FILE,
			    lines,
			    "Missing high number in range");
		    continue;
		}
		/** get the two numbers **/
		sep->se_rpc.lowvers = strtonum(cp);
		sep->se_rpc.highvers = strtonum(p);
		if (sep->se_rpc.lowvers > sep->se_rpc.highvers) {
		    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
			    lines,
			    "Low value in range exceeds high value");
		    continue;
		}
	    }
	    else {
		/* if there is a single number */
		sep->se_rpc.lowvers = sep->se_rpc.highvers = strtonum(cp);
	    }
	}

	cp = strtok(NULL,"\n");
	if (cp == NULL) {
	    syslog(LOG_ERR, FORMATERR, CONFIG_FILE,
		    lines, "Missing argument list");
	    continue;
	}
	while (*cp == ' ' || *cp == '\t')
	    cp++;
	sep->se_argv = makeargv(cp);
	badfile = FALSE;
	break;

    }

    /*
    ** if we exit the while loop because _attachline returned NULL,
    ** no more lines in the file.
    */
    return((line != NULL) ? sep : (struct servtab *)NULL);
}

/*
** ENTER
**
**    Insert an entry into the server information table.
*/

static struct servtab *
enter(cp)
	struct servtab *cp;
{
    struct servtab *sep;
    long omask;

    sep = (struct servtab *)malloc(sizeof (*sep));
    if (sep == (struct servtab *)0) {
	syslog(LOG_ERR, "Out of memory.");
	killprocess(0);
    }
    memcpy((char *)sep, (char *)cp, sizeof *cp);
    sep->se_fd = -1;
    omask = sigblock(SIGBLOCK);
    sep->se_next = servtab;
    servtab = sep;
    sigsetmask(omask);
    return (sep);
}


/*
** FREECONFIG
**
**    Free the memory allocated for a server info table entry.
*/

static void
freeconfig(cp)
    struct servtab *cp;
{
    int i = 0;

    if (cp->se_service)
	free(cp->se_service);
    if (cp->se_proto)
	free(cp->se_proto);
    if (cp->se_user)
	free(cp->se_user);
    if (cp->se_server)
	    free(cp->se_server);
    if (cp->se_argv) {
	while (cp->se_argv[i] != NULL)
	    free(cp->se_argv[i++]);
	free(cp->se_argv);
    }
    if (cp->se_isrpc && cp->se_rpc.name)
	free(cp->se_rpc.name);
		
}


/*
**  MAKEARGV -- break up a string into words
**
**	Parameters:
**		p -- the string to break up.
**
**	Returns:
**		a char **argv (dynamically allocated)
**
**	Side Effects:
**		munges p.
*/

static char **
makeargv(p)
	register char *p;
{
	char *q;
	int i;
	char **avp;
	char *argv[MAXARGS + 1];

	/* take apart the words */
	i = 0;
	while (*p != '\0' && i < MAXARGS)
	{
		q = p;
		while (*p != '\0' && !isspace(*p))
			p++;
		while (isspace(*p))
			*p++ = '\0';
		argv[i++] = lstrdup(q);
	}
	argv[i++] = NULL;

	/* now make a copy of the argv */
	avp = (char **)malloc(sizeof *avp * i);
	if (avp == (char **)NULL) {
	    syslog(LOG_ERR, "Out of memory.");
	    killprocess(0);
	}
	memcpy((char *)avp, (char *) argv, sizeof *avp * i);

	return (avp);
}

/*
** ARGVCMP
**
**    Compare two argument vectors.  Return 0 iff the vectors
**    are equivalent.
**
*/

static int
argvcmp(a, b)
    register char **a, **b;
{
    register int result = 0;

    if (a != NULL && b != NULL) {
	while (*a && *b && !result) {
	    result = strcmp(*a, *b);
	    a++, b++;
	}
	return(*a != *b || result);
    } else
	return (a != b);
}

/*
** SETUP
**
**    Prepare to accept connections for a service.
**    Create a socket to listen for connections or datagrams, bind it to
**    the address of that service, add its socket descriptor to the
**    list of descriptors to poll, and increment the number of socket
**    descriptors and active services. 
**
**    Returns 0 if the service was successfully prepared.
**    Returns 1 if all but the bind() succeeded.     
**    Returns -1 if there was some sort of permanent error.
*/

int
setup(sep)
register struct servtab *sep;
{
	int on = 1;

	if (sep == NULL)
		return -1;

	if ((sep->se_fd = socket(AF_INET, sep->se_socktype, 0)) < 0) {
		syslog(LOG_ERR, "%s: socket: %m", servicename(sep));
		return -1;
	}

#define	turnon(fd, opt) \
	    setsockopt(fd, SOL_SOCKET, opt, (char *)&on, sizeof (on))
	if (strcmp(sep->se_proto, "tcp") == 0 && (options & SO_DEBUG) &&
	    turnon(sep->se_fd, SO_DEBUG) < 0)
	    syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
	if (turnon(sep->se_fd, SO_REUSEADDR) < 0)
	    syslog(LOG_ERR, "setsockopt (SO_REUSEADDR): %m");
#undef turnon

	if (bind(sep->se_fd, &sep->se_ctrladdr, 
			sizeof (struct sockaddr_in)) < 0) 
	{
		syslog(LOG_ERR, "%s: bind: %m", servicename(sep));
		(void) close(sep->se_fd);
		sep->se_fd = -1;
		if (!timingout) 
		{
	    		timingout = 1;
	    		alarm(RETRYTIME);
		}
		return 1;
	}

	if (sep->se_socktype == SOCK_STREAM)
		if (listen(sep->se_fd, 20) < 0) 
		{
			syslog(LOG_ERR, "%s: listen: %m", servicename(sep));
			(void) close(sep->se_fd);
			sep->se_fd = -1;
			return -1;
		}

	addserv(sep);
	return 0;
}

/*
** SETUPRPC
**
**    Prepare to accept connections for RPC services. For each service,
**    create a socket to listen for connections or datagrams, bind it
**    to an address (system selected),  add its socket descriptor to the
**    list of descriptors to poll, register it with the portmapper
**    and increment the number of socket descriptors and active services.
**
*/

static void
setuprpc()
{
    register struct servtab *sep = servtab;
    register int i;
    struct sockaddr_in addr;
    int len = sizeof(struct sockaddr_in);

    for (; sep != NULL; sep = sep->se_next)
    {
	if (!sep->se_isrpc 		/* not an RPC server */
	    || (sep->se_fd >= 0)	/* already active */
	    || (sep->se_checked <= 0)) 	/* deleted */
		continue;
	
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((sep->se_fd = socket(AF_INET, sep->se_socktype, 0)) < 0) 
	{
		syslog(LOG_ERR, "%s: socket: %m", servicename(sep));
		continue;
	}

	if (bind(sep->se_fd, &addr, sizeof (struct sockaddr_in)) < 0) 
	{
		syslog(LOG_ERR, "%s: bind: %m", servicename(sep));
		(void) close(sep->se_fd);
		sep->se_fd = -1;
		continue;
	}

	if (getsockname(sep->se_fd, &addr, &len) != 0) 
	{
		syslog(LOG_ERR, "%s: getsockname: %m", servicename(sep));
		(void) close(sep->se_fd);
		sep->se_fd = -1;
		continue;
	}

	for (i = sep->se_rpc.lowvers; i <= sep->se_rpc.highvers; i++)
	{
	    unsigned short int proto = 
	        (sep->se_socktype == SOCK_DGRAM) ? IPPROTO_UDP : IPPROTO_TCP;
	    if (!pmap_set(sep->se_rpc.prog, i, proto, ntohs(addr.sin_port)))
		syslog(LOG_ERR, "%s: Cannot register version %d",
		       servicename(sep), i);
	}
	
	if (sep->se_socktype == SOCK_STREAM)
		if (listen(sep->se_fd, 20) < 0) 
		{
			syslog(LOG_ERR, "%s: listen: %m", servicename(sep));
			(void) close(sep->se_fd);
			sep->se_fd = -1;
			continue;
		}

	if (sep->se_checked == 2) /* new server */
		syslog(LOG_INFO, "%s: Added service, server %s",
		    servicename(sep), sep->se_server);

	addserv(sep);
    }

}

/*
** UNREGISTER
**
**   Unregister an RPC service.
*/

void
unregister(sep)
register struct servtab *sep;
{
	register struct servtab *s;
	register int i = sep->se_rpc.lowvers;

	if (sep == NULL || !sep->se_isrpc)
		return;

	/* Unregister the server */
	for (; i <= sep->se_rpc.highvers; i++)
		pmap_unset(sep->se_rpc.prog, i);

	/* 
	   If this server has a pair (same prog number,
	   different protocol), it has also been
	   unregistered above (unfortunately!); delete it, 
	   so it can be set up again by setuprpc().
	*/

	if ((s = rpcpair(sep)) != NULL)
	{
	    if (s->se_rpc.highvers > sep->se_rpc.highvers)
		for (i = sep->se_rpc.highvers + 1; i <= s->se_rpc.highvers; i++)
		    pmap_unset(s->se_rpc.prog, i);
	    delserv(s);
	}
}

/*
** SERVICENAME
**
**   Return a string containing service/protocol for non RPC
**   services, rpc.rpcservice/protocol for RPC services.
*/

char *
servicename(cp)
struct servtab *cp;
{
    static char name[MAXLINE];

    if (cp == NULL)
	name[0] = '\0';
    else
    if (cp->se_isrpc)
	sprintf(name, "rpc.%s/%s", cp->se_rpc.name,
		cp->se_proto);
    else
	sprintf(name, "%s/%s", cp->se_service, cp->se_proto);
    return(name);
}

/*
** STRTONUM
**
**    Convert a string to a number.  This works only for positive
**    numbers, and returns a -1 if it doesn't work as opposed to atoi
**    which returns a zero whether the string represented a zero or
**    was not a number.
*/

static int
strtonum(string)
    char *string;
{
    int ret;

    if (!strcmp(string, "0")) 
	return (0);
    else {
	if ((ret = atoi(string)) == 0)
	    return(-1);
	else
	    return(ret);
    }
}


/*
** STRDUP
**
**    Duplicate a string into malloc'd memory.  Returns NULL
**    if given a NULL pointer.
*/

static char *
lstrdup(cp)
    char *cp;
{
    char *new;

    if (cp == NULL)
	return(NULL);
    new = (char *)malloc((unsigned)(strlen(cp) + 1));
    if (new == (char *)0) {
	syslog(LOG_ERR, "Out of memory.");
	killprocess(0);
    }
    strcpy(new, cp);
    return (new);
}

static void
checkinit()
{
	register struct servtab *sep = servtab;

	for (; sep != NULL; sep = sep->se_next)
		sep->se_checked = 0;
}

static struct servtab *
findserv(cp) 		/* find non-RPC server entry */
register struct servtab *cp;
{
	register struct servtab *sep = servtab;

	if (cp->se_isrpc)
		return NULL;

	for (; sep != NULL; sep = sep->se_next) 
	{

		if (sep->se_isrpc)
			continue;

		if (strcmp(sep->se_service, cp->se_service) == 0 &&
		    strcmp(sep->se_proto, cp->se_proto) == 0)
			break;
	}

	return sep;
}

static struct servtab *
findrpcserv(cp) /* find RPC server entry */
register struct servtab *cp;
{
	register struct servtab *sep = servtab;

	if (!cp->se_isrpc)
		return NULL;

	for (; sep != NULL; sep = sep->se_next) 
	{

		if (!sep->se_isrpc)
			continue;

		if (sep->se_rpc.prog == cp->se_rpc.prog &&
		    strcmp(sep->se_proto, cp->se_proto) == 0)
			break;
	}

	return sep;
}

static void
purgetab()
{
	register struct servtab *sep, **sepp = &servtab;

	while (sep = *sepp) 
	{
		if (sep->se_checked > 0) 
		{
			sepp = &sep->se_next;
			continue;
		}

		*sepp = sep->se_next;
		if (sep->se_fd != -1)
		{
			if (sep->se_isrpc)
				unregister(sep);
			delserv(sep);
		}

		syslog(LOG_INFO, "%s: Deleted service", servicename(sep));
		freeconfig(sep);
		free((char *)sep);
	}
}

static void
_config(cp)
register struct servtab *cp;
{
	register struct servtab *sep;
	int newserver = 0;
	long omask;
	struct servent *sp;

	sep = findserv(cp);

	if (sep != NULL)
	{
		omask = sigblock(SIGBLOCK);
		(void) chgentry(sep, cp);
		freeconfig(cp);
		sigsetmask(omask);

	}
	else
	{
		sep = enter(cp);
		newserver++;
	}

	sep->se_checked = 1;

	sp = getservbyname(sep->se_service, sep->se_proto);
	if (sp == NULL) 
	{
		syslog(LOG_ERR, "%s: Unknown service", servicename(sep));
		delserv(sep);
		return;
	}

	if (sp->s_port != sep->se_ctrladdr.sin_port) 
	{
		sep->se_ctrladdr.sin_port = sp->s_port;
		delserv(sep);
	}

	if (sep->se_fd == -1) 
	{
	    if (nservers >= maxservers) 
	    {
		syslog(LOG_ERR, "%s: Too many services (max %d)",
		       servicename(sep), maxservers);
		return;
	    }

	    if (setup(sep) >= 0 && newserver)
		syslog(LOG_INFO, "%s: Added service, server %s",
		       servicename(sep), sep->se_server);
	}
}

static struct servtab *
rpcpair(sep)
register struct servtab *sep;
{
	register struct servtab *s = servtab;
	unsigned int prog = sep->se_rpc.prog;

	for (; s != NULL; s = s->se_next)
	{
		if (s->se_isrpc)
			if (prog == s->se_rpc.prog && s != sep)
				break;
	}

	return s;
}

static void
_configrpc(cp)
register struct servtab *cp;
{
	register struct servtab *sep;
	int newserver = 0;
	int changed = 0;
	long omask;

	sep = findrpcserv(cp);

	if (sep != NULL)
	{
		omask = sigblock(SIGBLOCK);
		changed = chgentry(sep, cp);
		freeconfig(cp);
		sigsetmask(omask);

		if (!changed)
		{
			sep->se_checked = 1;
			return;
		}

	}
	else
	{
		sep = enter(cp);
		newserver++;
	}

	if (newserver)
		sep->se_checked = 2;
	else if (sep->se_wait > 1)
		sep->se_checked = 3;
	else
		sep->se_checked = 1;

	unregister(sep);
	delserv(sep);

}

static int
chgentry(sep, cp)
register struct servtab *sep;
register struct servtab *cp;
{
	register int changed = 0;

	if (sep->se_socktype != cp->se_socktype)
	{
		sep->se_socktype = cp->se_socktype;
		++changed;
	}

	/*
	 ** if changing to a no-wait from a wait service,
	 ** and we are waiting for a server now,
	 ** set se_endwait so that reapchild() will change
	 ** this service to no-wait when the server dies.
	 */

	if (sep->se_wait > 1) 
	{
		if (!cp->se_wait) 
		{
		    sep->se_endwait = 1;
		    ++changed;
		}
	}
	else
	{
		if (sep->se_wait != cp->se_wait)
		{
			sep->se_wait = cp->se_wait;
			++changed;
		}
	}

	if (strcmp(sep->se_user, cp->se_user)) {
		SWAP(char *, sep->se_user, cp->se_user);
		syslog(LOG_INFO, "%s: New user id %s", servicename(sep),
		       sep->se_user);
		++changed;
	}

	if (strcmp(sep->se_server, cp->se_server)) {
		sep->se_bi = cp->se_bi;
		SWAP(char *, sep->se_server, cp->se_server);
		syslog(LOG_INFO, "%s: New server %s", servicename(sep),
		       sep->se_server);
		++changed;
	}

	if (argvcmp(sep->se_argv, cp->se_argv)) {
		SWAP(char **, sep->se_argv, cp->se_argv);
		syslog(LOG_INFO, "%s: New arguments for server %s", 
		       servicename(sep), sep->se_server);
		++changed;
	}

	if (sep->se_isrpc) 
		if (sep->se_rpc.lowvers != cp->se_rpc.lowvers ||
	    		sep->se_rpc.highvers != cp->se_rpc.highvers)
		{
	    		sep->se_rpc.lowvers = cp->se_rpc.lowvers;
	    		sep->se_rpc.highvers = cp->se_rpc.highvers;
			++changed;
		}

	return (changed > 0);
}

/* 
   Activate service (add socket descriptor 
   to list of descriptors to poll)
*/

static void
addserv(sep)
register struct servtab *sep;
{
	if (sep == NULL || sep->se_fd < 0)
		return;

	FD_SET(sep->se_fd, &allsock);
	++nsock;

	if (sep->se_fd > maxsock)
		maxsock = sep->se_fd;

	++nservers;
	if (nservers >= maxservers) 
		allocserv();
}

/* 
   Deactivate service (close socket and remove its
   descriptor from the list of descriptors to poll) 
*/

void
delserv(sep)
register struct servtab *sep;
{
	if (sep == NULL || sep->se_fd < 0)
		return;

	FD_CLR(sep->se_fd, &allsock);
	--nsock;
	(void) close(sep->se_fd);
	sep->se_fd = -1;
	--nservers;
}

static void 
allocserv()
{

/*
** If we've reached the current limit on the number of servers,
** try to allocate some more.  Unless inetd is configured for
** hundreds or thousands of servers, this code will never be
** used.
*/
	struct rlimit rbuf;
	register int newmax;

	getrlimit(RLIMIT_NOFILE, &rbuf);
	newmax = maxservers + OTHERDESCRIPTORS + 10;  /* Try to get 10 more */
	rbuf.rlim_cur = newmax;
	setrlimit(RLIMIT_NOFILE, &rbuf);
	getrlimit(RLIMIT_NOFILE, &rbuf);
	if (rbuf.rlim_cur == newmax)
	    maxservers = newmax - OTHERDESCRIPTORS;
}

