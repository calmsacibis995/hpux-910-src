/*	@(#)getrpcent.c	$Revision: 12.5 $	$Date: 90/12/10 17:22:34 $  */
/* 
static  char sccsid[] = "getrpcent.c 1.1 86/02/03  Copyr 1984 Sun Micro";
getrpcent.c	2.1 86/04/11 NFSSRC 
*/

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define atoi 			_atoi
#define catgets 		_catgets
#define endrpcent 		_endrpcent	/* In this file */
#define exit 			___exit
#define fclose 			_fclose
#define fgets 			_fgets
#define fopen 			_fopen
#define fprintf 		_fprintf
#define getdomainname 		_getdomainname
#define getrpcbyname 		_getrpcbyname	/* In this file */
#define getrpcbynumber 		_getrpcbynumber	/* In this file */
#define getrpcent 		_getrpcent	/* In this file */
#define rewind 			_rewind
#define setrpcent 		_setrpcent	/* In this file */
#define sprintf 		_sprintf
#define strcmp 			_strcmp
#define strlen 			_strlen
#define strncpy 		_strncpy
#define usingyellow 		_usingyellow
#define yellowup 		_yellowup
#define yp_first 		_yp_first
#define yp_match 		_yp_match
#define yp_next 		_yp_next

#ifdef _ANSIC_CLEAN
#define free 			_free
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#define NL_SETN 1	/* set number */
#include <nl_types.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpcsvc/ypclnt.h>

/*
 * BUFSIZ changed to 8K in stdio.h, but we don't need that much, and this
 * may help prevent commands from getting unneccessarily large.  1024 is
 * probably still bigger than necessary, but it's backward compatible.
 */

#undef BUFSIZ
#define BUFSIZ	1024

/*
 * Internet version.
 */

#define	MAXALIASES	35
#define	MAXADDRSIZE	14

static char domain[256];
static int stayopen;
static char *current = NULL;	/* current entry, analogous to hostf */
static int currentlen;
static struct rpcent *interpret();
static char *any();
static char RPCDB[] = "/etc/rpc";
static FILE *rpcf = NULL;
extern int usingyellow;		/* is network information service up? */
extern int yellowup();		/* is network information service up? */
static nl_catd nlmsg_fd;

#ifdef _NAMESPACE_CLEAN
#undef getrpcbynumber
#pragma _HP_SECONDARY_DEF _getrpcbynumber getrpcbynumber
#define getrpcbynumber _getrpcbynumber
#endif

struct rpcent *
getrpcbynumber(number)
	register int number;
{
	register struct rpcent *p;
	int reason;
	char adrstr[10], *val;
	int vallen;

	setrpcent(0);
	if (!usingyellow) {
		while (p = getrpcent()) {
			if (p->r_number == number)
				break;
		}
	}
	else {
		sprintf(adrstr, "%d", number);
		if (reason = yp_match(domain, "rpc.bynumber",
		    adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_match failed is %d\n",
			    reason);
#endif
			p = NULL;
		    }
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
	endrpcent();
	return (p);
}

#ifdef _NAMESPACE_CLEAN
#undef getrpcbyname
#pragma _HP_SECONDARY_DEF _getrpcbyname getrpcbyname
#define getrpcbyname _getrpcbyname
#endif

struct rpcent *
getrpcbyname(name)
	char *name;
{
	register struct rpcent *rpc = NULL;
	char *val;
	int reason, vallen;
	char **rp;

	setrpcent(0);
	if (usingyellow) {
		if (reason = yp_match(domain, "rpc.byna",
		    name, strlen(name), &val, &vallen)) {/* error */
#ifdef DEBUG
			fprintf(stderr, "reason yp_match failed is %d\n",
			    reason);
#endif
			rpc = NULL; /* continue sequential search */
		} else { /* no error, found key/value pair; return it */
			rpc = interpret(val, vallen);
			free(val);
			return (rpc);
		}
	}

	while(rpc = getrpcent()) {
		if (strcmp(rpc->r_name, name) == 0)
			return (rpc);
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0)
				return (rpc);
		}
	}
	
	endrpcent();
	return (NULL);
}

#ifdef _NAMESPACE_CLEAN
#undef setrpcent
#pragma _HP_SECONDARY_DEF _setrpcent setrpcent
#define setrpcent _setrpcent
#endif

setrpcent(f)
	int f;
{
	nlmsg_fd = _nfs_nls_catopen();
	if (getdomainname(domain, sizeof(domain)) < 0) {
		fprintf(stderr, 
		    (catgets(nlmsg_fd,NL_SETN,1, "setrpcent: getdomainname system call missing\n")));
		exit(1);
	}
	if (rpcf == NULL)
		rpcf = fopen(RPCDB, "r");
	else
		rewind(rpcf);
	if (current)
		free(current);
	current = NULL;
	stayopen |= f;
	yellowup(1, domain);	/* recompute whether network information service are up */
}

#ifdef _NAMESPACE_CLEAN
#undef endrpcent
#pragma _HP_SECONDARY_DEF _endrpcent endrpcent
#define endrpcent _endrpcent
#endif

endrpcent()
{
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
	if (rpcf && !stayopen) {
		fclose(rpcf);
		rpcf = NULL;
	}
}

#ifdef _NAMESPACE_CLEAN
#undef getrpcent
#pragma _HP_SECONDARY_DEF _getrpcent getrpcent
#define getrpcent _getrpcent
#endif

struct rpcent *
getrpcent()
{
	struct rpcent *hp;
	int reason;
	char *key, *val;
	int keylen, vallen;
	char line1[BUFSIZ+1];

	yellowup(0, domain);
	if (!usingyellow) {
		if (rpcf == NULL && (rpcf = fopen(RPCDB, "r")) == NULL)
			return (NULL);
	        if (fgets(line1, BUFSIZ, rpcf) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "rpc.bynumber",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
			    reason);
#endif
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "rpc.bynumber",
		    current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
			    reason);
#endif
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	hp = interpret(val, vallen);
	free(val);
	return (hp);
}

static struct rpcent *
interpret(val, len)
{
	static char *rpc_aliases[MAXALIASES];
	static struct rpcent rpc;
	static char line[BUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (getrpcent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (getrpcent());
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		return (getrpcent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	rpc.r_name = line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	rpc.r_number = atoi(cp);
	q = rpc.r_aliases = rpc_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &rpc_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&rpc);
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
