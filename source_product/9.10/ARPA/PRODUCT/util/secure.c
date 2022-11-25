/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** SECURE.C
**
** This file contains the routines which read the security file and
** decide whether a remote host is allowed to use a particular service.
**
*/

#ifndef lint
static char rcsid[] = "$Header: secure.c,v 1.7.109.6 92/04/10 12:43:19 bazavan Exp $";
#endif /* ~lint */

#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/nameser.h>
#include <resolv.h> 

#ifndef TRUE
#define TRUE            1
#define FALSE           0
#endif TRUE

#define NOSECURITY      0       /* No security file used */
#define SECURITY        1       /* Security file used */
#define BADSECURITY     -1	/* Problem with security file */
#define OUTOFMEMORY     -3      /* Out of memory when mallocing */

#define MAXLINE         2048
#define SECURITY_FILE   "/usr/adm/inetd.sec"
#define SIGBLOCK        (sigmask(SIGCHLD) | sigmask(SIGHUP) |\
			 sigmask(SIGALRM) | sigmask(SIGQUIT))

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif /* ~INADDR_NONE */

/*
** Information from security file
*/

struct  sec_info {
  char        *service;       /* name of service */
  int         allowed;        /* hosts in list are allowed/denied */
  char        *list;          /* list of allowed/denied hosts */
  struct      sec_info *sec_next;
};   

static struct sec_info *sec_info = NULL; 

static FILE     *fsecure;       /* security file */
static int      seclines;       /* Used to count lines in sec. file      */
static int	readsec = 0;	/* If 0 security file has not been read yet  */
static struct stat lastmod, lastread;	
				/* Last modification time when file was last */
				/* read and "latest" modification time.	     */
static char *mydomain;          /* Used when recognizing flag host names */

/*
** ISBROADCAST
**
**    Determine if the given address is a broadcast address.
**    This can be made more intelligent to pick out more kinds
**    of broadcasts.  For now, it is not very sophisticated.
*/

isbroadcast(sin)
    struct sockaddr_in *sin;
{
    if (sin->sin_addr.s_addr == 0 ||
	sin->sin_addr.s_addr == INADDR_BROADCAST)
	return (1);
    return (0);
}
	

/*
** GETREMOTENAME
**
**    Set up the remote host name and IP address for use in the
**    security check and for logging.
*/

getremotenames(from, hostflag, hp, remotehost, remoteaddr)
    struct sockaddr_in *from;
    int hostflag;
    struct hostent **hp;
    char **remotehost;
    char **remoteaddr;
{
    extern int errno;
    static char unknown[] = "unknown";
    static struct hostent *remotehp;

    /*
    ** Free memory allocated by last call.
    */
    if (*remoteaddr != NULL) {
        free(*remoteaddr);
        *remoteaddr = (char *)NULL;
    }
    if (*remotehost != NULL) {
        if (*remotehost != unknown) 
            free(*remotehost);
        *remotehost = (char *)NULL;
    }

    /* Find our local domain for flat host name matching */
    if (!(_res.options & RES_INIT))
	res_init();
    mydomain = _res.defdname;

    /* Get the IP address */
    if (*remoteaddr == NULL)
	*remoteaddr = strdup(inet_ntoa(from->sin_addr));

    /* Get the name, if desired */
    if (hostflag && *remotehost == NULL) {
	remotehp = NULL;
	if (isbroadcast(from))
	    *remotehost = strdup("broadcast");
	else {
	    errno = 0;
	    remotehp = gethostbyaddr(&from->sin_addr, sizeof(struct in_addr),
				     from->sin_family);
	    if (remotehp != NULL && errno != ECONNREFUSED) {
		/* 
		 * If name returned by gethostbyaddr is from the
		 * nameserver, attempt to verify that we haven't
		 * been fooled by someone in a remote net; look up
		 * the name and check that this address
		 * corresponds to the name.
		 */
		long oldoptions = _res.options;
		char rhname[MAXDNAME];
		register int hostok = 0;

		strncpy(rhname, remotehp->h_name, sizeof(rhname) - 1);
		rhname[sizeof(rhname) - 1] = 0;
		if (strchr(rhname, '.') != NULL)
		    _res.options &= ~(RES_DEFNAMES | RES_DNSRCH);
		remotehp = gethostbyname(rhname);
		_res.options = oldoptions;
		if (remotehp)
		    for (; remotehp->h_addr_list[0]; remotehp->h_addr_list++)
			if (!memcmp(remotehp->h_addr_list[0], 
				    (caddr_t)&from->sin_addr,
				    sizeof(from->sin_addr))) {
			    hostok++;
			    break;
			}
		if (!hostok) 
		    remotehp = NULL;
	    }

	    if (remotehp == NULL ||  
		(*remotehost = strdup(remotehp->h_name)) == NULL ||
		**remotehost == (char)0) {
		*remotehost = unknown;
	    }
	    else {
		/* strip the local domain */
		register char *p = strchr(*remotehost, '.');

		if (p && !strcasecmp(p+1, _res.defdname))
		    *p = 0;
	    }
	    *hp = remotehp;
	}
    }
}

/*
** USESECURITY
**
**	Return NOSECURITY if no SECURITY_FILE exists, SECURITY
**      if it does exist, or BADSECURITY if there is a problem
**      with it.  If malloc fails while adding a new security 
**	entry, will return OUTOFMEMORY.  If the security file 
**	has been modified, it will be reread.
**
*/
static
usesecurity()
{

    /*
    ** If the file doesn't exist, there is no security check. 
    */
    if (stat(SECURITY_FILE, &lastmod) < 0) {
	if (errno == ENOENT || errno == ENOTDIR) {
	    return (NOSECURITY);
	}
	syslog(LOG_ERR, "%s: stat: %m", SECURITY_FILE);
	return (BADSECURITY);
    }

    /* 
    ** Check if we need to reread the security file.
    */
    if (!readsec || (lastmod.st_mtime > lastread.st_mtime)) {
	fsecure = fopen(SECURITY_FILE, "r");
	if (fsecure == NULL) {
	    syslog(LOG_ERR, "%s: fopen: %m", SECURITY_FILE);
	    return (BADSECURITY);
	}
        if (readsecfile() == OUTOFMEMORY)
	    return (OUTOFMEMORY);
	fclose(fsecure);
	readsec = TRUE;
    } 
    return (SECURITY);

}


/*
** ISHOSTNAME
**
**    Returns non-zero if the given string has the possibility of being a
**    hostname (i.e. it either has a domain or it consists entirely of
**    letters, digits and dashes.)
*/

ishostname(h)
    char *h;
{
    register char *p;

    /* 
    ** The top level domain will be a letter followed by any number of
    ** letters and digits.  If it's not, the string is either an ip
    ** address or is a bogus FQDN.  The quickest test is to just ensure
    ** that the first character of the top-level domain is a letter.
    */
    if ((p = strrchr(h, '.')) != NULL)
	return (isalpha(p[1]));

    /* 
    ** If there was no . in the string, it must be a flat host name.
    ** A valid hostname consists of letters, digits, and dashes only.
    */
    while (*h && (isalpha(*h) || isdigit(*h) || *h == '-'))
	h++;
    return (!*h);
}

/*
** INETD_SECURE 
**
**     inetd_secure returns:
**         1   when the remote host is cleared to access a service
**	   0   when the remote host is denied to access a servie
**        -1   when the inetd.sec file has bad entries
**	  -3   when creating the security internal table fails,
**	       due to malloc failure
*/
int
inetd_secure(service, from)
    char	*service;
    struct	sockaddr_in *from;
{
    char *cp, *p, *addrpointer;
    int netaddr;
    struct netent *netname;
    char *host;    /* Used when recognizing flag host names */
    struct sec_info *sec;
    static char *_remotehost = NULL;  
    static char *_remoteaddr = NULL;  
    static struct hostent *_remotehp;  
    int secflag;

    secflag = usesecurity();
    /* It is up to the caller to deal with the error cases. */
    if (secflag == BADSECURITY || secflag == OUTOFMEMORY)
       return (secflag);
    /* If there is no  security file on the system, no security
       check. */
    if (secflag == NOSECURITY)
       return (1);

    /* only when we have a good security file, we continue */
    /* to find correct address to match remote host address */
    for (sec = sec_info; sec; sec = sec->sec_next)
	if (!strcmp(sec->service, service))
	    break;

    /* 
    ** if there is no entry for the service, the host is cleared.
    */
    if (sec == NULL)
	return (1); 

    /*
    ** If there is an entry, but no list, allow or deny the host. 
    */ 
    if (sec->list == NULL)
	return (sec->allowed);

    addrpointer = inet_ntoa(from->sin_addr);
    netaddr = inet_netof(from->sin_addr);
    netname = getnetbyaddr(netaddr, AF_INET);
    p = strdup(sec->list);
    cp = p;

    for (;;) {
	char *tmp;
	char ew;		/* character found after end of current word */
	char *ep;		/* pointer to character following current    */
	/* word, set to '\0' for string comparisons  */

	/*
	**  skip blanks and tabs
	*/
	while (*cp  == ' ' || *cp == '\t')
	    cp++;
	/*
        **  if we've reached the end of the string
	*/
	if (*cp == '\0')
	    break;

	/*
	**  find end of current word
	*/
	ep = cp;
	while (*ep != '\0' && *ep != ' ' && *ep != '\t')
	    ep++;
	ew = *ep;
	*ep = '\0';

	/*
	** if list member matches address or netname,
	** the remote host is allowed/denied.
	*/
	if (!strcmp(cp, addrpointer) || !strcmp(cp, netname->n_name)) {
	    free(p);
	    return (sec->allowed);
	}

	/*
	 ** Check for ranges and wild card characters 
	 */
	if (internet_parse(cp, &from->sin_addr, sec->service) > 0) {
	    free(p);
	    return (sec->allowed);
	}

	/*
	** If we haven't matched thus far, see if it's
	** worth checking a hostname match.  If it is,
	** call getremotename() to setup the host name
	** before we try the match.
	*/
	if (ishostname(cp)) {
	    getremotenames(from, TRUE, &_remotehp, &_remotehost, &_remoteaddr);
	    host = _remotehp->h_name; 
	    if (!strcasecmp(cp, host) || 
		/* flat hostname match */
		((tmp = strchr(host, '.')) != NULL &&
		 strcasecmp(++tmp, mydomain) == 0 &&
		 strncasecmp(cp, host, tmp - host - 1) == 0 &&
		 cp[tmp - host - 1] == '\0')) 
	    {
		free(p);
		return (sec->allowed);
	    }
	}

	if (ew == '\0')		/* this was the last word */
	    break;
	else			/* prepare to find the next word */
	    cp = ++ep;
    }

    free(p);

    /* 
     ** if the service was found but the host we are looking for is
     ** not in list, then if it was a list of allowed hosts the
     ** host is not allowed, and if it was a list of hosts not
     ** allowed, the host is allowed.
     */
    return (!sec->allowed);

}


/*
** INTERNET_PARSE
**
**
**    routine to take a string, which represents an entry in the
**    security file, and a pointer to an address in Internet format
**    (i.e four bytes), and return an indication of whether the
**    address matches the string, where the string can have wild cards
**    and ranges
**
**    returns a -1 in case of config error, a zero if it doesn't match
**    and a 1 if it succeeds.
*/
static
internet_parse(string, addr, service)
        char *string;
        char *service;
        unsigned char *addr;
{
	int i;
	unsigned char low, high;
	unsigned char num;
	char *cp;
	char store[MAXDNAME];
	char *list[4];

	/* 
	** If this is not an address: for example hostname or netname
	** return with no match.
	*/
	if ( strspn(string,"0123456789-*.") != strlen(string) )
	    return (0);

	/* save string before we destroy it! */
	strncpy(store, string, sizeof(store));

	cp = strtok(store, ".\0");
	for (i=0; i<4; i++) {
		list[i] = cp;
		cp = strtok(NULL,".\0");
		if (cp == NULL || *cp == '\0')
			cp = "*";
	}

	for (i=0; i<4; i++) {
	    /*
	    ** Check for wild card.  Only if it is exactly the
	    ** string "*" will it match.
	    */
	    if (strcmp(list[i], "*") == 0)
		    continue;
	    /*
	    ** if it still contains a wild card, make it an error 
	    */
	    if (strchr(list[i], '*') != NULL) {
		    syslog(LOG_ERR, 
	    "%s: Field contains other characters in addition to * for %s",
			   SECURITY_FILE, service);
		    return (-1);
	    }

	    /*
	    ** check for a string with a range of numbers.
	    */
	    if ((cp = strchr(list[i], '-')) != NULL) {
		    if (cp == list[i]) {
			    syslog(LOG_ERR, "%s: Missing low value in range for %s",
				   SECURITY_FILE, service);
			    return (-1);
		    }
		    *cp++ = NULL;
		    if (*cp == '\0') {
			    syslog(LOG_ERR, "%s: Missing high value in range for %s",
				   SECURITY_FILE, service);
			    return (-1);
		    }
		    low	 = atoi(list[i]);
		    high = atoi(cp);

		    /* 
		    ** this is a hack.  We needed to pick out the four
		    ** bytes in the in_addr structure and compare them
		    ** one at a time.  Since in_addr is not a array,
		    ** we kludged it to look like an array of chars,
		    ** which will be converted to integers by C when
		    ** it does the actual comparison. SIGH
		    */
		    if (low <= addr[i] && addr[i] <= high)
			    continue;
		    else
			    if (high < low) {
				    syslog(LOG_ERR, 
					   "%s: Low value in range exceeds high value for %s",
					   SECURITY_FILE, service);
				    return (-1);
			    }
			    else
				    return (0);
	    }

	    /* check for the number exactly */
	    num = atoi(list[i]);
	    if (num == addr[i])
		    continue;
	    else
		    return(0);
    }

    /*
    ** if made it this far, it succeeded
    */
    return(1);

}


/*
** NEXTLINE
**
**    Get next line of the configuration file.
*/
static char *
nextline(ignoreblklines)
   int ignoreblklines;		/* if true ignore blank lines */
{
    char *cp;
    static char	  line[MAXLINE+1];
    FILE *filep;
	
    filep = fsecure;
    while (TRUE) {
	if ((cp = fgets(line,MAXLINE,filep)) == NULL)
	    return (char *)NULL;
	seclines++;
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
** ATTACHLINE
**
**    If the line is continued (ends with a \), attach the next line
**    to it.
*/

static char *
attachline()
{
    char *p, *cp;
    static char *saveline = NULL;
    int len, newlen;

    seclines = 0;
    /*
    **	attachline is responsible for freeing all memory allocated
    **	for reading configuration/security file lines.
    */
    if (saveline != NULL) {
	free(saveline);
	saveline = (char *)NULL;
    }

    /*
    **	get the next line ignoring blank lines
    */
    if ((p = nextline(1)) == NULL)
	return (char *)NULL;

    /* saveline allocated here; it is freed
    **	by this instance of attachline if about to return NULL,
    **	otherwise by the next instance of attachline.
    */
    saveline = strdup(p);
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
		p = nextline(0);
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
		       SECURITY_FILE, seclines);
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
** READSECFILE
**
**     read the security file and fill the security structure with
**     information.
*/
static
readsecfile()
{
    char	*endp;		/* pointer to end of list of */
                                /* allowed/denied hosts/networks */
    char	*cp;	/* pointer used to traverse security file line */
    char	*line;	/* pointer to beginning of line from attachline */
    struct sec_info s, *sec = &s;


    if (sec_info != NULL)
	freesec_info();

    while ((line = attachline()) != NULL) {
	memset(sec, 0, sizeof(*sec));
	cp = strtok(line," \t\n");
	if (cp == NULL)
	    continue;

	/* accept a MAXNUM line for compatibility sake */
	if (!strncmp(cp, "MAXNUM", 6)) {
           syslog(LOG_WARNING, "%s: The MAXNUM limit is obsolete.",
		  SECURITY_FILE);
	   continue;
	}
	
	/*
	**	otherwise, the first word on the line is the name of a service
	*/
	sec->service = strdup(cp);
	cp = strtok(NULL," \t\n");
	/*
	**	"service" by itself means allow everybody to use service.
	*/
	if (cp == NULL) {
	    sec->allowed = TRUE;
            if (entersec(sec) == OUTOFMEMORY)
	       return(OUTOFMEMORY);
	    continue;
	}
	else if (!strcmp(cp, "allow"))
	    sec->allowed = TRUE;
	else if (!strcmp(cp, "deny"))
	    sec->allowed = FALSE;
	else {
	    syslog(LOG_WARNING, 
		   "%s: allow/deny field does not have a valid entry for %s",
		   SECURITY_FILE, sec->service);
	    continue;
	}
	/*
	**	get past the end of the "allow" or "deny" token
	*/
	while (*cp++ != '\0');

	/*
	 **	skip blanks and tabs
	 */
	while ((*cp == ' ')|| (*cp == '\t'))
	    cp++;
	if ((*cp == '\n') || (*cp == '\0')) {
	    /*
	     **  "<service> allow" means allow everybody;
	     **  "<service> deny" means allow nobody.
	     */
	    sec->list = NULL;
	}
	else {
	    endp = cp + strlen(cp) - 1;
	    if (*endp == '\n') {
		*endp = '\0';
	    }
	    sec->list = strdup(cp);
	}
        if (entersec(sec) == OUTOFMEMORY)
	    return(OUTOFMEMORY);
    }				/* while */

    if (stat(SECURITY_FILE,&lastread) < 0)
	syslog(LOG_WARNING, "%s: stat: %m", SECURITY_FILE);
    return 0;

}


/*
** ENTERSEC
**
**    Add an entry to the security list.
*/
static
entersec(cp)
	struct sec_info *cp;
{
    register struct sec_info *sec;
    int omask;
    char *strdup();


    sec = (struct sec_info *)malloc(sizeof (*sec));
    if (sec == (struct sec_info *)0) {
	syslog(LOG_ERR, "Out of memory.");
	return (OUTOFMEMORY);
    }
    *sec = *cp;

    omask = sigblock(SIGBLOCK);
    sec->sec_next = sec_info;
    sec_info = sec;
    sigsetmask(omask);
}


/*
** FREESEC_INFO
**
**    Erase the sec_info table.
*/
static
freesec_info()
{
    struct sec_info *sec;

    while ((sec = sec_info) != NULL) {
	sec_info = sec_info->sec_next;
	if (sec->service)
	    free(sec->service);
	if (sec->list)
	    free(sec->list);
	free(sec);
    }
}


