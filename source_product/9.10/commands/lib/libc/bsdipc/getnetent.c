/* @(#)$Header: getnetent.c,v 66.2 90/11/12 11:08:20 jmc Exp $ */

#ifdef _NAMESPACE_CLEAN
#define fclose _fclose
#define fgets  _fgets
#define fopen  _fopen
#define inet_ntoa _inet_ntoa
#define inet_makeaddr _inet_makeaddr
#define inet_network _inet_network
#define usingyellow _usingyellow
#define yellowup _yellowup
#define yp_first _yp_first
#define yp_match _yp_match
#define yp_next _yp_next
#define strlen _strlen
#define strcmp _strcmp
#define strcpy _strcpy
#define strncpy _strncpy
#define strchr _strchr
#define strrchr _strrchr
#define getdomainname _getdomainname
#define rewind _rewind
#ifdef	_ANSIC_CLEAN
#define	malloc _malloc
#define	free _free
#endif
/* local */
#define getnetbyaddr _getnetbyaddr
#define setnetent _setnetent
#define endnetent _endnetent
#define getnetent _getnetent
#define getnetbyname _getnetbyname
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif
#include <netinet/in.h>

/**************************************************************/
/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14
#define LINEBUFSIZ	1024

static char NETDB[] = "/etc/networks";
static FILE *netf = NULL;
static int stayopen = 0;
static struct netent *interpret();
struct netent *getnetent();
static char *any();
#ifdef HP_NFS
static char domain[256];
static char *current = NULL;	/* current entry, analogous to netf */
static int currentlen;
extern int usingyellow;		/* are yellow pages up? */
extern int yellowup();		/* are yellow pages up? */
char *inet_ntoa();
struct in_addr inet_makeaddr();
static char *nettoa();
#endif HP_NFS



#ifdef _NAMESPACE_CLEAN
#undef getnetbyaddr
#pragma _HP_SECONDARY_DEF _getnetbyaddr getnetbyaddr
#define getnetbyaddr _getnetbyaddr
#endif

struct netent *
getnetbyaddr(net, type)
{
	register struct netent *p;
#ifdef HP_NFS
	int reason;
	char *adrstr, *val;
	int vallen;
#endif HP_NFS

	/*
	**	Why go on?
	**	How would we generalize this routine to multiple address types?
	*/
	if (type != AF_INET)
		return NULL;

	setnetent(0);
#ifdef HP_NFS
	if (!usingyellow) {
#endif HP_NFS
		while (p = getnetent()) {
			if (p->n_addrtype == type && p->n_net == net)
				break;
		}
#ifdef HP_NFS
	}
	else {
		adrstr = nettoa(net);
		if (reason = yp_match(domain, "networks.byaddr",
		    adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_match failed is %d\n",
			    reason);
#endif DEBUG
			p = NULL;
		    }
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
#endif HP_NFS
	endnetent();
	return (p);
}



#ifdef _NAMESPACE_CLEAN
#undef getnetbyname
#pragma _HP_SECONDARY_DEF _getnetbyname getnetbyname
#define getnetbyname _getnetbyname
#endif

struct netent *
getnetbyname(name)
	register char *name;
{
	register struct netent *p;
	register char **cp;
#ifdef HP_NFS
	int reason;
	char *val;
	int vallen;
#endif HP_NFS

	setnetent(0);
#ifdef HP_NFS
	if (!usingyellow) {
#endif HP_NFS
		while (p = getnetent()) {
			if (strcmp(p->n_name, name) == 0)
				break;
			for (cp = p->n_aliases; *cp != 0; cp++)
				if (strcmp(*cp, name) == 0)
					goto found;
		}
#ifdef HP_NFS
	}
	else {
		if (reason = yp_match(domain, "networks.byname",
		    name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_match failed is %d\n",
				reason);
#endif DEBUG
			p = NULL;
		    }
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
#endif HP_NFS
found:
	endnetent();
	return (p);
}



#ifdef _NAMESPACE_CLEAN
#undef setnetent
#pragma _HP_SECONDARY_DEF _setnetent setnetent
#define setnetent _setnetent
#endif

setnetent(f)
	int f;
{
#ifdef HP_NFS
	getdomainname(domain, sizeof(domain));
#endif HP_NFS
	if (netf == NULL)
		netf = fopen(NETDB, "r");
	else
		rewind(netf);
#ifdef HP_NFS
	if (current)
		free(current);
	current = NULL;
#endif HP_NFS
	stayopen |= f;
#ifdef HP_NFS
	yellowup(1, domain);	/* recompute whether yellow pages are up */
#endif HP_NFS
}



#ifdef _NAMESPACE_CLEAN
#undef endnetent
#pragma _HP_SECONDARY_DEF _endnetent endnetent
#define endnetent _endnetent
#endif

endnetent()
{
#ifdef HP_NFS
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
#endif HP_NFS
	if (netf && !stayopen) {
		fclose(netf);
		netf = NULL;
	}
}



#ifdef _NAMESPACE_CLEAN
#undef getnetent
#pragma _HP_SECONDARY_DEF _getnetent getnetent
#define getnetent _getnetent
#endif

struct netent *
getnetent()
{
	static char line1[LINEBUFSIZ+1];
#ifdef HP_NFS
	int reason;
	char *key, *val;
	int keylen, vallen;
	struct netent *np;

	yellowup(0, domain);
	if (!usingyellow) {
#endif HP_NFS
		if (netf == NULL && (netf = fopen(NETDB, "r")) == NULL)
			return (NULL);
	        if (fgets(line1, LINEBUFSIZ, netf) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
#ifdef HP_NFS
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "networks.byaddr",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
				reason);
#endif DEBUG
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "networks.byaddr",
		    current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
				reason);
#endif DEBUG
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	np = interpret(val, vallen);
	free(val);
	return (np);
#endif HP_NFS
}



static struct netent *
interpret(val, len)
{
	static char *net_aliases[MAXALIASES];
	static struct netent net;
	static char line[LINEBUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (getnetent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (getnetent());
	*cp = '\0';
	net.n_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		return (getnetent());
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	net.n_net = inet_network(cp);
	net.n_addrtype = AF_INET;
	q = net.n_aliases = net_aliases;
	if (p != NULL) 
		cp = p;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &net_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&net);
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



#ifdef HP_NFS
/*
**	nettoa()	--	convert network address to ascii
*/
static
char *
nettoa(net)
	unsigned net;
{
	static char buf[16];
	char *p, *strchr(), *strrchr();
	struct in_addr in;
	int addr;

	in = inet_makeaddr(net, INADDR_ANY);
	addr = in.s_addr;
	strcpy(buf, inet_ntoa(in));
	if (IN_CLASSA(htonl(addr))) {
		p = strchr(buf, '.');
		if (p == NULL)
			return NULL;
		stripzeros(p);
	}
	else if (IN_CLASSB(htonl(addr))) {
		p = strchr(buf, '.');
		if (p == NULL)
			return NULL;
		p = strchr(p+1, '.');
		if (p == NULL)
			return NULL;
		stripzeros(p);
	}
	else if (IN_CLASSC(htonl(addr))) {
		p = strrchr(buf, '.');
		if (p == NULL)
			return NULL;
		stripzeros(p);
	}
	return buf;
}

/*
**  Skip trailing zeros in a dotted quad starting
**  at the first dotted zero.
*/
static int
stripzeros(cp)
register char *cp;
{

	register char *p;

	while (*cp && *cp == '.') {
		p = cp;
		cp++;
		while (*cp && *cp == '0')
			cp++;
		if (!*cp || *cp == '.') {
			*p = '\0';
			break;
		} else
			while (*cp && *cp != '.')
				cp++;

	}

}
#endif HP_NFS

