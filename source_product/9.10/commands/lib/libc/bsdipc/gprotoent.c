/* @(#)$Header: gprotoent.c,v 64.10 89/02/13 09:11:49 jmc Exp $ */

#ifdef _NAMESPACE_CLEAN
#define fprintf _fprintf
#define ltoa _ltoa
#define fclose _fclose
#define fopen  _fopen
#define fgets  _fgets
#define yp_first _yp_first
#define yp_match _yp_match
#define yp_next _yp_next
#define getdomainname _getdomainname
#define usingyellow _usingyellow
#define yellowup _yellowup
#define strlen _strlen
#define strcmp _strcmp
#define strncpy _strncpy
#define atoi _atoi
#define rewind _rewind
#ifdef	_ANSIC_CLEAN
#define	malloc _malloc
#define	free _free
#endif
/* local */
#define getprotobynumber _getprotobynumber
#define getprotobyname _getprotobyname
#define setprotoent _setprotoent
#define endprotoent _endprotoent
#define getprotoent _getprotoent
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif HP_NFS

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14
#define LINEBUFSIZ	1024

static char PROTODB[] = "/etc/protocols";
static FILE *protof = NULL;
static struct protoent *interpret();
struct protoent *getprotoent();
static int stayopen = 0;
static char *any();
#ifdef HP_NFS
static char domain[256];
static char *current = NULL;	/* current entry, analogous to protof */
static int currentlen;
extern int usingyellow;		/* are yellow pages up? */
extern int yellowup();		/* are yellow pages up? */
#endif HP_NFS


#ifdef _NAMESPACE_CLEAN
#undef getprotobynumber
#pragma _HP_SECONDARY_DEF _getprotobynumber getprotobynumber
#define getprotobynumber _getprotobynumber
#endif

struct protoent *
getprotobynumber(proto)
	register int proto;
{
	register struct protoent *p;
#ifdef HP_NFS
	int reason;
	char *adrstr, *val;
	int vallen;
	extern char *ltoa();
#endif HP_NFS


	setprotoent(0);
#ifdef HP_NFS
	if (!usingyellow) {
#endif HP_NFS
		while (p = getprotoent()) {
			if (p->p_proto == proto)
				break;
		}
#ifdef HP_NFS
	}
	else {
		adrstr = ltoa(proto);
		if (reason = yp_match(domain, "protocols.bynumber",
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
	endprotoent();
	return (p);
}



#ifdef _NAMESPACE_CLEAN
#undef getprotobyname
#pragma _HP_SECONDARY_DEF _getprotobyname getprotobyname
#define getprotobyname _getprotobyname
#endif

struct protoent *
getprotobyname(name)
	register char *name;
{
	register struct protoent *p;
	register char **cp;
#ifdef HP_NFS
	int reason;
	char *val;
	int vallen;
#endif HP_NFS

	setprotoent(0);
#ifdef HP_NFS
	if (!usingyellow) {
#endif HP_NFS
		while (p = getprotoent()) {
			if (strcmp(p->p_name, name) == 0)
				break;
			for (cp = p->p_aliases; *cp != 0; cp++)
				if (strcmp(*cp, name) == 0)
					goto found;
		}
#ifdef HP_NFS
	}
	else {
		if (reason = yp_match(domain, "protocols.byname",
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
	endprotoent();
	return (p);
}



#ifdef _NAMESPACE_CLEAN
#undef setprotoent
#pragma _HP_SECONDARY_DEF _setprotoent setprotoent
#define setprotoent _setprotoent
#endif

setprotoent(f)
	int f;
{
#ifdef HP_NFS
	getdomainname(domain, sizeof(domain));
#endif HP_NFS
	if (protof == NULL)
		protof = fopen(PROTODB, "r");
	else
		rewind(protof);
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
#undef endprotoent
#pragma _HP_SECONDARY_DEF _endprotoent endprotoent
#define endprotoent _endprotoent
#endif

endprotoent()
{
#ifdef HP_NFS
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
#endif HP_NFS
	if (protof && !stayopen) {
		fclose(protof);
		protof = NULL;
	}
}



#ifdef _NAMESPACE_CLEAN
#undef getprotoent
#pragma _HP_SECONDARY_DEF _getprotoent getprotoent
#define getprotoent _getprotoent
#endif

struct protoent *
getprotoent()
{
	static char line1[LINEBUFSIZ+1];
#ifdef HP_NFS
	int reason;
	char *key, *val;
	int keylen, vallen;
	struct protoent *pp;

	yellowup(0, domain);
	if (!usingyellow) {
#endif HP_NFS
		if (protof == NULL && (protof = fopen(PROTODB, "r")) == NULL)
			return (NULL);
	        if (fgets(line1, LINEBUFSIZ, protof) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
#ifdef HP_NFS
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "protocols.bynumber",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
				reason);
#endif
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "protocols.bynumber",
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
	pp = interpret(val, vallen);
	free(val);
	return (pp);
#endif HP_NFS
}



static struct protoent *
interpret(val, len)
{
	static char *proto_aliases[MAXALIASES];
	static struct protoent proto;
	static char line[LINEBUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (getprotoent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (getprotoent());
	*cp = '\0';
	proto.p_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		return (getprotoent());
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	proto.p_proto = atoi(cp);
	q = proto.p_aliases = proto_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &proto_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&proto);
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
