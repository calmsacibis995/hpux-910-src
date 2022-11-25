
static char rcsid[] = "$Header: hosttable.c,v 1.5.109.2 91/11/21 14:01:41 kcs Exp $";

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <rpcsvc/ypclnt.h>

#define	MAXALIASES	35
#define	MAXADDRS	35

static struct hostent host;
static char *host_aliases[MAXALIASES];
static char hostbuf[BUFSIZ+1];
static char HOSTDB[] = "/etc/hosts";
static FILE *hostf = NULL;
static char hostaddr[MAXADDRS];
static char *host_addrs[2];
static int stayopen = 0;
static char *any();

static char domain[256];
static char *current = NULL;	/* key for current entry in YP database */
static int currentlen;
extern int usingyellow;		/* are yellow pages up? */
extern int yellowup();		/* are yellow pages up? */

extern errno;

struct hostent *interpret();

static struct hostent *
interpret(p)
char *p;
{
	register char *cp, **q;

	if (*p == '#')
		return ((struct hostent *) NULL);
	cp = any(p, "#\n");
	if (cp == NULL)
		return ((struct hostent *) NULL);
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		return ((struct hostent *) NULL);
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
	host.h_addr_list = host_addrs;
#endif
	host.h_addr = hostaddr;
	*((u_long *)host.h_addr) = inet_addr(p);
	host.h_length = sizeof (u_long);
	host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

struct hostent *
yp_gethtbyname(name)
char *name;
{
	char *val;
	int reason, vallen;
	struct hostent *hp;

	/* may not have called yellowup yet */
	if (domain[0] == 0)
		yellowup(1, domain);
			
	if (reason = yp_match(domain, "hosts.byname",
				name, strlen(name), &val, &vallen)) {
		return ((struct hostent *) NULL);
	} else {
		strncpy(hostbuf,val,vallen);
		hostbuf[vallen] = '\n';
		hp = interpret(hostbuf);
		free(val);
		return(hp);
	}
}

struct hostent *
yp_gethtbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	char *adrstr, *val;
	int reason, vallen;
	struct hostent *hp;

	/* may not have called yellowup yet */
	if (domain[0] == 0)
		yellowup(1, domain);
			
	adrstr = inet_ntoa(*(int *)addr);
	if (reason = yp_match(domain, "hosts.byaddr",
				adrstr, strlen(adrstr), &val, &vallen)) {
			return ((struct hostent *) NULL);
	} else {
		strncpy(hostbuf,val,vallen);
		hostbuf[vallen] = '\n';
		hp = interpret(hostbuf);
		free(val);
		return(hp);
	}
}

