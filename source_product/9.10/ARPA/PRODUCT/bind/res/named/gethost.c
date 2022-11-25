/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)gethostnamadr.c	6.39 (Berkeley) 1/4/90
 *	@(#)$Header: gethost.c,v 1.31.109.1 91/11/21 11:45:32 kcs Exp $
 */

#ifdef _NAMESPACE_CLEAN
#define fprintf _fprintf
#define printf _printf
#define strcpy _strcpy
#define strcat _strcat
#define ultoa _ultoa
#define dn_expand _dn_expand
#define dn_skipname _dn_skipname
#define _getshort __getshort
#define res_search _res_search
#define res_query _res_query
#define _res_close __res_close
#define h_errno _h_errno
#define strlen _strlen
#define strncpy _strncpy
#define inet_ntoa _inet_ntoa
#define inet_addr _inet_addr
#define fopen _fopen
#define fclose _fclose
#define fgets _fgets
#define yp_match _yp_match
#define yp_first _yp_first
#define yp_next _yp_next
#define usingyellow _usingyellow
#define yellowup _yellowup
#define memcpy _memcpy
#define memcmp _memcmp
#define rewind _rewind
#define BINDup _BINDup
#define strcasecmp _strcasecmp
#ifdef	_ANSIC_CLEAN
#define	malloc _malloc
#define	free _free
#endif
/* local */
#define gethostbyname _gethostbyname
#define gethostbyaddr _gethostbyaddr
#define sethostent _sethostent
#define endhostent _endhostent
#define gethostent _gethostent
#define sethostfile _sethostfile
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#ifdef NOTDEF
#include <arpa/inet.h>
#endif /* NOTDEF */
#include <arpa/nameser.h>
#include <resolv.h>
#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif HP_NFS

#define	MAXALIASES	35
#define	MAXADDRS	35

static char *h_addr_ptrs[MAXADDRS + 1];

static struct hostent host;
static char *host_aliases[MAXALIASES];
static char hostbuf[BUFSIZ+1];
static struct in_addr host_addr;
static char HOSTDB[] = "/etc/hosts";
static FILE *hostf = NULL;
static char hostaddr[MAXADDRS];
static char *host_addrs[2];
static int stayopen = 0;

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;


static char *any();

#ifdef HP_NFS
static char domain[256];
static char *current = NULL;	/* key for current entry in YP database */
static int currentlen;
extern int usingyellow;		/* are yellow pages up? */
extern int yellowup();		/* are yellow pages up? */
#endif HP_NFS

int BINDup();                   /* is the name server up? */
static struct hostent *interpret();

int h_errno;
extern errno;

static struct hostent *
getanswer(answer, anslen, iquery)
	querybuf *answer;
	int anslen;
	int iquery;
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *eom;
	char *bp, **ap;
	int type, class, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = answer->buf + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = dn_expand((char *)answer->buf, eom,
			     cp, bp, buflen)) < 0) {
				h_errno = NO_RECOVERY;
				return ((struct hostent *) NULL);
			}
			cp += n + QFIXEDSZ;
			host.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			h_errno = HOST_NOT_FOUND;
		else
			h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
	ap = host_aliases;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
	host.h_addr_list = h_addr_ptrs;
#endif
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);
 		cp += sizeof(u_short);
		class = _getshort(cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = _getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (iquery && type == T_PTR) {
			if ((n = dn_expand((char *)answer->buf, eom,
			    cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			cp += n;
			host.h_name = bp;
			return(&host);
		}
		if (iquery || type != T_A)  {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != host.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			host.h_length = n;
			getclass = class;
			host.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				host.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += sizeof(align) - ((u_long)bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("size (%d) too big\n", n);
#endif
			break;
		}
#ifdef hpux
		memcpy(*hap++ = bp, cp, n);
#else
		bcopy(cp, *hap++ = bp, n);
#endif
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
		*hap = NULL;
#else
		host.h_addr = h_addr_ptrs[0];
#endif
		return (&host);
	} else {
		h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
}


#ifdef _NAMESPACE_CLEAN
#undef gethostbyname
#pragma _HP_SECONDARY_DEF _gethostbyname gethostbyname
#define gethostbyname _gethostbyname
#endif

struct hostent *
gethostbyname(name)
	char *name;
{
	querybuf buf;
	register char *cp;
	int n;
	struct hostent *hp;
	struct hostent *_gethtbyname();
#ifdef HP_NFS
	char *val;
	int reason, vallen;
#endif HP_NFS

        h_errno = HOST_NOT_FOUND;
        if (name == NULL || *name == '\0')
               return ((struct hostent *) NULL);
	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit(name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				/*
				 * All-numeric, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.  What if someone types
				 * 255.255.255.255?  The test below will
				 * succeed spuriously... ???
				 */
				if ((host_addr.s_addr = inet_addr(name)) == -1) {
					h_errno = HOST_NOT_FOUND;
					return((struct hostent *) NULL);
				}
				host.h_name = name;
				host.h_aliases = host_aliases;
				host_aliases[0] = NULL;
				host.h_addrtype = AF_INET;
				host.h_length = sizeof(u_long);
				h_addr_ptrs[0] = (char *)&host_addr;
				h_addr_ptrs[1] = (char *)0;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
				host.h_addr_list = h_addr_ptrs;
#else
				host.h_addr = h_addr_ptrs[0];
#endif
				return (&host);
			}
			if (!isdigit(*cp) && *cp != '.') 
				break;
		}

	if ((n = res_search(name, C_IN, T_A, buf.buf, sizeof(buf))) >= 0) {
		return (getanswer(&buf, n, 0));
	}
	else {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_search failed\n");
#endif DEBUG
		if (errno == ECONNREFUSED) {
#ifdef HP_NFS
			/* may not have called yellowup yet */
			if (domain[0] == 0)
				yellowup(1, domain);
			
			if (yellowup(0, domain)) {

				if(current)
					free(current);
				current = NULL;

				if (reason = yp_match(domain, "hosts.byname",
						name, strlen(name), &val, &vallen)) {
#ifdef DEBUG
					fprintf(stderr, "reason yp_match failed is %d\n",
						reason);
#endif DEBUG
					errno = ECONNREFUSED;
					return ((struct hostent *) NULL);
				}
				else {
					strncpy(hostbuf,val,vallen);
					hostbuf[vallen] = '\n';
					hp = interpret(hostbuf);
					free(val);
					errno = ECONNREFUSED;
					return(hp);
				}
			}
			else 
#endif HP_NFS
				{
				hp = _gethtbyname(name);
				errno = ECONNREFUSED;
				return (hp);
			}
		}
		else
			return ((struct hostent *) NULL);
	}
}


#ifdef _NAMESPACE_CLEAN
#undef gethostbyaddr
#pragma _HP_SECONDARY_DEF _gethostbyaddr gethostbyaddr
#define gethostbyaddr _gethostbyaddr
#endif

struct hostent *
gethostbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	int n;
	querybuf buf;
	register struct hostent *hp;
	char qbuf[MAXDNAME];
	struct hostent *_gethtbyaddr();
#ifdef HP_NFS
	char *adrstr, *val;
	int reason, vallen;
#endif HP_NFS
	extern char *ultoa();

	h_errno = HOST_NOT_FOUND;
        if (addr == NULL)
               return ((struct hostent *) NULL);

	if ((type != AF_INET) || (len != sizeof(struct in_addr)))
		return ((struct hostent *) NULL);

	/* Changed from a sprintf for size considerations */
	strcpy(qbuf, ultoa((unsigned)addr[3] & 0xff));
	strcat(qbuf, ".");
	strcat(qbuf, ultoa((unsigned)addr[2] & 0xff));
	strcat(qbuf, ".");
	strcat(qbuf, ultoa((unsigned)addr[1] & 0xff));
	strcat(qbuf, ".");
	strcat(qbuf, ultoa((unsigned)addr[0] & 0xff));
	strcat(qbuf, ".in-addr.arpa");

	if ((n = res_query(qbuf, C_IN, T_PTR, (char *)&buf, sizeof(buf))) >= 0){
		hp = getanswer(&buf, n, 1);
		if (hp == NULL)
			return ((struct hostent *) NULL);
		hp->h_addrtype = type;
		hp->h_length = len;
		h_addr_ptrs[0] = (char *)&host_addr;
		h_addr_ptrs[1] = (char *)0;
		host_addr = *(struct in_addr *)addr;
		return(hp);
	}
	else {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query failed\n");
#endif DEBUG
		if (errno == ECONNREFUSED) {
#ifdef HP_NFS
			/* may not have called yellowup yet */
			if (domain[0] == 0)
				yellowup(1, domain);
			
			if (yellowup(0, domain)) {

				if(current)
					free(current);
				current = NULL;

				adrstr = inet_ntoa(*(int *)addr);
				if (reason = yp_match(domain, "hosts.byaddr",
						adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
					fprintf(stderr, "reason yp_match failed is %d\n",
						reason);
#endif DEBUG
					errno = ECONNREFUSED;
					return ((struct hostent *) NULL);
				}
				else {
					strncpy(hostbuf,val,vallen);
					hostbuf[vallen] = '\n';
					hp = interpret(hostbuf);
					free(val);
					errno = ECONNREFUSED;
					return(hp);
				}
			}
			else
#endif HP_NFS
				{
				hp = _gethtbyaddr(addr, len, type);
				errno = ECONNREFUSED;
				return(hp);
		        }
		}
		else
			return ((struct hostent *) NULL);
	}
}


#ifdef _NAMESPACE_CLEAN
#undef sethostent
#pragma _HP_SECONDARY_DEF _sethostent sethostent
#define sethostent _sethostent
#endif

sethostent(stayopenflag)
int stayopenflag;
{
	if (BINDup(1)) {
		if (stayopenflag)
			_res.options |= RES_STAYOPEN | RES_USEVC;
	}
#ifdef HP_NFS
	else if (yellowup(1, domain)) {
		if (current) {
			free(current);
			current = NULL;
		}
		stayopen |= stayopenflag;
	}
#endif HP_NFS
	else
		_sethtent(stayopenflag);
}

#ifdef _NAMESPACE_CLEAN
#undef endhostent
#pragma _HP_SECONDARY_DEF _endhostent endhostent
#define endhostent _endhostent
#endif

endhostent()
{
	if (BINDup(0)) {
		_res.options &= ~(RES_STAYOPEN | RES_USEVC);
		_res_close();
	}
#ifdef HP_NFS
	else if (yellowup(0, domain)) {
		if (current && !stayopen) {
			free(current);
			current = NULL;
		}
	}
#endif HP_NFS
	else 
		_endhtent();
}


#ifdef _NAMESPACE_CLEAN
#undef gethostent
#pragma _HP_SECONDARY_DEF _gethostent gethostent
#define gethostent _gethostent
#endif

struct hostent *
gethostent()
{
	register struct hostent *hp;
	struct hostent *_gethtent();
#ifdef HP_NFS
	char *key, *val;
	int keylen, vallen, reason;
#endif HP_NFS

	if (BINDup(0))
		return ((struct hostent *) NULL);
#ifdef HP_NFS
	else if (yellowup(0, domain)) {
		if (current == NULL) {
			if (reason =  yp_first(domain, "hosts.byaddr",
					&key, &keylen, &val, &vallen)) {
#ifdef DEBUG
				fprintf(stderr, "reason yp_first failed is %d\n",
					reason);
#endif DEBUG
				return NULL;
			}
		}
		else {
			if (reason = yp_next(domain, "hosts.byaddr",
					current, currentlen,
					&key, &keylen, &val, &vallen)) {
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
		hp = interpret(val);
		free(val);
		return (hp);
	}
#endif HP_NFS
	else 
		return (_gethtent());
}


#ifdef _NAMESPACE_CLEAN
#undef sethostfile
#pragma _HP_SECONDARY_DEF _sethostfile sethostfile
#define sethostfile _sethostfile
#endif

sethostfile(name)
char *name;
{
#ifdef lint
name = name;
#endif
}



int
_sethtent(f)
	int f;
{
	if (hostf == NULL)
		hostf = fopen(HOSTDB, "r" );
	else
		rewind(hostf);
	stayopen |= f;
}

int
_endhtent()
{
	if (hostf && !stayopen) {
		(void) fclose(hostf);
		hostf = NULL;
	}
}

struct hostent *
_gethtent()
{
	register struct hostent *hp;
	char *p;

	if (hostf == NULL && (hostf = fopen(HOSTDB, "r" )) == NULL)
		return ((struct hostent *) NULL);
	for (;;) {
		if ((p = fgets(hostbuf, BUFSIZ, hostf)) == NULL)
			return ((struct hostent *) NULL);
		if ((hp = interpret(p)) != NULL)
			return (hp);
	}
}

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
_gethtbyname(name)
	char *name;
{
	register struct hostent *p;
	register char **cp;
	
	_sethtent(0);
	while (p = _gethtent()) {
		if (strcasecmp(p->h_name, name) == 0)
			break;
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (strcasecmp(*cp, name) == 0)
				goto found;
	}
found:
	_endhtent();
	return (p);
}

struct hostent *
_gethtbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	register struct hostent *p;

	_sethtent(0);
	while (p = _gethtent())
#ifdef hpux
		if (p->h_addrtype == type && !memcmp(p->h_addr, addr, len))
#else
		if (p->h_addrtype == type && !bcmp(p->h_addr, addr, len))
#endif
			break;
	_endhtent();
	return (p);
}
