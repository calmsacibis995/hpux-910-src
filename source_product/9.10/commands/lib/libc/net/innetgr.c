/*	@(#)innetgr.c	$Revision: 12.4 $	$Date: 90/08/30 11:48:27 $  */
/*
static char sccsid[] = "innetgr.c 1.1 86/02/03 Copyr 1985 Sun Micro";
innetgr.c	2.1 86/04/11 NFSSRC 
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

#define catgets 		_catgets
#define catopen 		_catopen
#define exit 			___exit
#define fprintf 		_fprintf
#define getdomainname 		_getdomainname
#define getenv 			_getenv
#define innetgr 		_innetgr	/* In this file */
#define nl_init 		_nl_init
#define strchr 			_strchr
#define strcmp 			_strcmp
#define strlen 			_strlen
#define strncmp 		_strncmp
#define strncpy 		_strncpy
#define uselocal 		_uselocal
#define usingyellow 		_usingyellow
#define yellowup 		_yellowup
#define yp_match 		_yp_match

#ifdef _ANSIC_CLEAN
#define free 			_free
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#define NL_SETN 3	/* set number */
#include <nl_types.h>

#include <stdio.h>
#include <ctype.h>
#include <rpcsvc/ypclnt.h>
#include <string.h>
#include <arpa/trace.h>

/* 
 * innetgr: test whether I'm in /etc/netgroup
 * 
 */

extern int usingyellow;		/* is network information service up? */
extern int yellowup();		/* is network information service up? */
extern char *malloc();

static char *any();
static char *name, *machine, *domain;
static char thisdomain[256];
static char *list[200];		/* can nest recursively this deep */
static char **listp;		/* pointer into list */
static nl_catd nlmsg_fd;


#ifdef _NAMESPACE_CLEAN
#undef innetgr
#pragma _HP_SECONDARY_DEF _innetgr innetgr
#define innetgr _innetgr
#endif

innetgr(grp, mach, nm, dom)
	char *grp;
	char *mach;
	char *nm;
	char *dom;
{
	int res;
	TRACE("innetgr SOP");

	nlmsg_fd = _nfs_nls_catopen();

	if (getdomainname(thisdomain, sizeof(thisdomain)) < 0) {
		(void) fprintf(stderr, 
		    (catgets(nlmsg_fd,NL_SETN,1, "innetgr: getdomainname system call missing\r\n")));
	    exit(1);
	}
	listp = list;
	machine = mach;
	name = nm;
	domain = dom;
	yellowup(0, thisdomain);
	if (domain && usingyellow) {
		if (name && !machine) {
			if (lookup("netgroup.byuser",grp,name,domain,&res)) {
				return(res);
			}
		} else if (machine && !name) {
			if (lookup("netgroup.byhost",grp,machine,domain,&res)) {
				return(res);
			}
		}
	}
	TRACE("innetgr: about to call return(doit)");
	return doit(grp);
}
	

	

/* 
 * calls itself recursively
 */
static
doit(group)
	char *group;
{
	char *key, *val;
	int vallen,keylen;
	char *r, *string;
	int match;
	register char *p, *q;
	register char **lp;
	int err;
	
	TRACE("doit: SOP");

	nlmsg_fd = _nfs_nls_catopen();
	
	*listp++ = group;
	if (listp > list + sizeof(list)) {
		(void) fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "innetgr: recursive overflow\r\n")));
		listp--;
		TRACE("doit: EOP, 1st return 0");
		return (0);
	}
	key = group;
	keylen = strlen(group);
	TRACE("Calling yellowup");
	yellowup(0, thisdomain);
	TRACE2("After yellow: usingyellow: %d", usingyellow);
	if (usingyellow)
	{
	    err = yp_match(thisdomain, "netgroup", key, keylen, &val, &vallen);
	    if (err) {
#ifdef DEBUG
		if (err == YPERR_KEY)
			(void) fprintf(stderr,
			    "innetgr: no such netgroup as %s\n", group);
		else
			(void) fprintf(stderr, "innetgr: yp_match, %s\n",yperr_string(err));
#endif /* DEBUG */
			listp--;
			TRACE("doit: EOP, 2nd return 0");
			return(0);
	    }
	}
	else
	{
	    TRACE2("doit: uselocal with key: %s", key);
	    if (uselocal(key, &val, &vallen) == 0)
	    {
		listp--;
		TRACE("doit: EOP, 3rd return 0");
		return(0);
	    }
	}
	/* 
	 * check for recursive loops
	 */
	TRACE("doit: Before for loop");
	for (lp = list; lp < listp-1; lp++)
		if (strcmp(*lp, group) == 0) {
			(void) fprintf(stderr,
			    (catgets(nlmsg_fd,NL_SETN,3, "innetgr: netgroup %s called recursively\r\n")),
			    group);
			listp--;
			TRACE("doit: EOP, 4th return 0");
			return(0);
		}
	
	p = val;
	TRACE2("doit: p is %s before p[vallen] = 0", p);
	p[vallen] = 0;
	TRACE2("doit: p is %s", p);
	while (p != NULL) {
		match = 0;
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 0 || *p == '#')
			break;
		if (*p == '(') {
			p++;
			while (*p == ' ' || *p == '\t')
				p++;
			r = q = strchr(p, ',');
			if (q == NULL) {
				(void) fprintf(stderr,
				    (catgets(nlmsg_fd,NL_SETN,4, "innetgr: syntax error in /etc/netgroup\r\n")));
				listp--;
				TRACE("doit: EOP, 5th return 0");
				return(0);
			}
			if (p == q || machine == NULL)
				match++;
			else {
				while (*(q-1) == ' ' || *(q-1) == '\t')
					q--;
				string = malloc(q-p+1);
				if (string == NULL)
					return(0);
				strncpy(string, p, q-p);
				string[q-p] = '\0';
				if (strcmp(machine, string) == 0)
					match++;
				free(string);
			}
			p = r+1;

			while (*p == ' ' || *p == '\t')
				p++;
			r = q = strchr(p, ',');
			if (q == NULL) {
				(void) fprintf(stderr,
				    (catgets(nlmsg_fd,NL_SETN,5, "innetgr: syntax error in /etc/netgroup\r\n")));
				listp--;
				TRACE("doit: EOP, 6th return 0");
				return(0);
			}
			if (p == q || name == NULL)
				match++;
			else {
				while (*(q-1) == ' ' || *(q-1) == '\t')
					q--;
				string = malloc(q-p+1);
				if (string == NULL)
					return(0);
				strncpy(string, p, q-p);
				string[q-p] = '\0';
				if (strcmp(name, string) == 0)
					match++;
				free(string);
			}
			p = r+1;

			while (*p == ' ' || *p == '\t')
				p++;
			r = q = strchr(p, ')');
			if (q == NULL) {
				(void) fprintf(stderr,
				    (catgets(nlmsg_fd,NL_SETN,6, "innetgr: syntax error in /etc/netgroup\r\n")));
				listp--;
				TRACE("doit: EOP, 7th return 0");
				return(0);
			}
			if (p == q || domain == NULL)
				match++;
			else {
				while (*(q-1) == ' ' || *(q-1) == '\t')
					q--;
				string = malloc(q-p+1);
				if (string == NULL)
					return(0);
				strncpy(string, p, q-p);
				string[q-p] = '\0';
				if (strcmp(domain, string) == 0)
					match++;
				free(string);
			}
			p = r+1;
			if (match == 3) {
				free(val);
				listp--;
				TRACE("doit: EOP, 1st 1");
				return 1;
			}
		}
		else {
			q = any(p, " \t\n#");
			if (q && *q == '#')
				break;
			if (q)
				*q = 0;
			if (doit(p)) {
				free(val);
				listp--;
				TRACE("doit: EOP: 2nd 1");
				return 1;
			}
			if (q)
				*q = ' ';
		}
		p = any(p, " \t");
	}
	free(val);
	listp--;
	TRACE("doit: EOP: 8th return 0");
	return 0;
}

/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	TRACE("any: SOP");
	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	TRACE("any: EOP");
	return ((char *)0);
}

/*
 * return 1 if "what" is in the comma-separated, newline-terminated "list"
 */
static
inlist(what,list)
	char *what;
	char *list;
{
#	define TERMINATOR(c)    (c == ',' || c == '\n')

	register char *p;
	int len;
         
	TRACE("inlist: SOP");
	len = strlen(what);     
	p = list;
	do {             
		if (strncmp(what,p,len) == 0 && TERMINATOR(p[len])) {
			return(1);
		}
		while (!TERMINATOR(*p)) {
			p++;
		}
		p++;
	} while (*p);
	TRACE("inlist: EOP");
	return(0);
}




/*
 * Lookup a host or user name in a nis map.  Set result to 1 if group in the 
 * lookup-up list of groups. Return 1 if the map was found.
 */
static
lookup(map,group,name,domain,res)
	char *map;
	char *group;
	char *name;
	char *domain;
	int *res;
{
	int err;
	char *val;
	int vallen;
	char key[256];
	char *wild = "*";
	int i;

	TRACE("lookup: SOP");
	for (i = 0; i < 4; i++) {
		switch (i) {
		case 0: makekey(key,name,domain); break;
		case 1: makekey(key,wild,domain); break;	
		case 2: makekey(key,name,wild); break;
		case 3: makekey(key,wild,wild); break;	
		}
		err  = yp_match(thisdomain,map,key,strlen(key),&val,&vallen); 
		if (!err) {
			*res = inlist(group,val);
			free(val);
			if (*res) {
				TRACE("lookup: 1st return 1");
				return(1);
			}
		} else {
#ifdef DEBUG
			(void) fprintf(stderr,
				"yp_match(%s,%s) failed: %s.\n",map,key,yperr_string(err));
#endif /* DEBUG */
			if (err != YPERR_KEY)  {
				TRACE("lookup: return 0");
				return(0);
			}
		}
	}
	*res = 0;
	TRACE("lookup: 2nd return 1");
	return(1);
}



/*
 * Generate a key for a netgroup.byXXXX nis map
 */
static
makekey(key,name,domain)
	register char *key;
	register char *name;
	register char *domain;
{
	TRACE("makekey: SOP");
	while (*key++ = *name++)
		;
	*(key-1) = '.';
	while (*key++ = *domain++)
		;
	TRACE("makekey: EOP");
}	
