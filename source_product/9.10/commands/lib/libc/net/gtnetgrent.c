/*   @(#)gtnetgrent.c	$Revision: 70.2 $	$Date: 93/11/02 12:41:20 $  */
/*
static char sccsid[] = "getnetgrent.c 1.1 86/02/03 Copyr 1985 Sun Micro";
getnetgrent.c	2.1 86/04/11 NFSSRC 
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
#define endnetgrent 		_endnetgrent
#define exit 			___exit
#define fclose 			_fclose
#define fgets 			_fgets
#define fopen 			_fopen
#define fprintf 		_fprintf
#define getdomainname 		_getdomainname
#define getnetgrent 		_getnetgrent
#define rewind 			_rewind
#define setnetgrent 		_setnetgrent
#define stat 			_stat
#define strchr 			_strchr
#define strcmp 			_strcmp
#define strcpy 			_strcpy
#define strlen 			_strlen
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

#define NL_SETN 2	/* set number */
#include <nl_types.h>

#include <stdio.h>
#include <ctype.h>
#include <rpcsvc/ypclnt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * BUFSIZ changed to 8K in stdio.h, but we don't need that much, and this
 * may help prevent commands from getting unneccessarily large.  1024 is
 * probably still bigger than necessary, but it's backward compatible.
 */
#undef BUFSIZ
#define BUFSIZ	1024
#define iseol(c)        (c == '\0' || c == '\n' || c == '#')

#define MAXGROUPLEN 1024
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

/* 
 * access members of a netgroup
 */

struct grouplist {		/* also used by pwlib */
	char *gl_machine;
	char *gl_name;
	char *gl_domain;
	struct grouplist *gl_nxt;
};

/*	HPNFS
**	removed all references to "alloca()" routine -- part of Berkeley
**	not present on HP-UX.  See later comments for code change req'd.
*/
extern char *malloc();
extern char *strcpy(),*strncpy(), *strtok(), *strchr();

static char *any();

static struct list {			/* list of names to check for loops */
	char *name;
	struct list *nxt;
};


static struct grouplist *grouplist, *grlist;

static FILE *netgrf = NULL;
static char NETGRDB[] = "/etc/netgroup";
extern int usingyellow;		/* is network information service up? */
extern int yellowup();		/* is network information service up? */

static void getgroup();
static void doit();
static char *fill();
static char *match();
static char domain[256];

static nl_catd nlmsg_fd;


#ifdef _NAMESPACE_CLEAN
#undef setnetgrent
#pragma _HP_SECONDARY_DEF _setnetgrent setnetgrent
#define setnetgrent _setnetgrent
#endif

setnetgrent(grp)
	char *grp;
{
	static char oldgrp[256];
	
	if (strcmp(oldgrp, grp) == 0)
		grlist = grouplist;
	else {
		if (grouplist != NULL)
			endnetgrent();
		getgroup(grp);
		grlist = grouplist;
		(void) strcpy(oldgrp, grp);
		yellowup(1, domain);
	}
}

#ifdef _NAMESPACE_CLEAN
#undef endnetgrent
#pragma _HP_SECONDARY_DEF _endnetgrent endnetgrent
#define endnetgrent _endnetgrent
#endif

endnetgrent() {
	struct grouplist *gl;
	
	for (gl = grouplist; gl != NULL; gl = gl->gl_nxt) {
		if (gl->gl_name)
			free(gl->gl_name);
		if (gl->gl_domain)
			free(gl->gl_domain);
		if (gl->gl_machine)
			free(gl->gl_machine);
		free((char *) gl);
	}
	grouplist = NULL;
	grlist = NULL;
	if (netgrf && !usingyellow)
	{
		fclose(netgrf);
		netgrf = NULL;
	}
}

#ifdef _NAMESPACE_CLEAN
#undef getnetgrent
#pragma _HP_SECONDARY_DEF _getnetgrent getnetgrent
#define getnetgrent _getnetgrent
#endif

getnetgrent(machinep, namep, domainp)
	char **machinep, **namep, **domainp;
{
	if (grlist) {
		if (machinep == NULL)
			machinep = (char **) malloc(1);
		*machinep = grlist->gl_machine;
		if (namep == NULL)
			namep = (char **) malloc(1);
		*namep = grlist->gl_name;
		if (domainp == NULL)
			domainp = (char **) malloc(1);
		*domainp = grlist->gl_domain;
		grlist = grlist->gl_nxt;
		return (1);
	}
	else
		return (0);
}



static void
getgroup(grp)
	char *grp;
{
	nlmsg_fd = _nfs_nls_catopen();

	if (getdomainname(domain, sizeof(domain)) < 0) {
		(void) fprintf(stderr, 
		    (catgets(nlmsg_fd,NL_SETN,1, "getnetgrent: getdomainname system call missing\n")));
	    exit(1);
	}
	doit(grp,(struct list *) NULL);
}
		

/*
 * recursive function to find the members of netgroup "group". "list" is
 * the path followed through the netgroups so far, to check for cycles.
 */
static void
doit(group,list)
    char *group;
    struct list *list;
{
    register char *p, *q;
    register struct list *ls;
    char *val;
    struct grouplist *gpls;
 
    nlmsg_fd = _nfs_nls_catopen();

    /*
     * check for non-existing groups
     */
    if ((val = match(group)) == NULL) {
        return;
    }
 
 
    /*
     * check for cycles
     */
    for (ls = list; ls != NULL; ls = ls->nxt) {
        if (strcmp(ls->name, group) == 0) {
            (void) fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,2, "getnetgrent: Cycle detected in /etc/netgroup: %s.\n")),group);
            return;
        }
    }
 
 
    /*	HPNFS
    **	removed reference to "alloca()" routine -- Berkeley library call
    **	not present on HP-UX.  replaced by "malloc()" with explicit
    **	"free()" call before ALL returns!
    */
    ls = (struct list *) malloc(sizeof(struct list));
    ls->name = group;
    ls->nxt = list;
    list = ls;
    
    p = val;
    while (p != NULL) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == 0 || *p =='#')
            break;
        if (*p == '(') {
            gpls = (struct grouplist *) malloc(sizeof(struct grouplist));
            p++;
 
            if (!(p = fill(p,&gpls->gl_machine,',')))  {
                goto syntax_error;
            }
            if (!(p = fill(p,&gpls->gl_name,','))) {
                goto syntax_error;
            }
            if (!(p = fill(p,&gpls->gl_domain,')'))) {
                goto syntax_error;
            }
            gpls->gl_nxt = grouplist;
            grouplist = gpls;
        } else {
            q = any(p, " \t\n#");
            if (q && *q == '#')
                break;
	    if (q != NULL)
            	*q = 0;
            doit(p,list);
	    if (q != NULL)
            	*q = ' ';
        }
        p = any(p, " \t");
    }
    /*	HPNFS
    **	free goes with previous malloc() for struct ls;
    **	added because alloca() grabs stack space and frees it
    **	upon return from function -- this emulates alloca() ...
    */
    free(ls);
    return;
 
syntax_error:
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3, "getnetgrent: syntax error in /etc/netgroup\n")));
    (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,4, "--- %s\n")),val);
    /*	HPNFS
    **	free goes with previous malloc() for struct ls;
    **	added because alloca() grabs stack space and frees it
    **	upon return from function -- this emulates alloca() ...
    */
    free(ls);
    return;
}



/*
 * Fill a buffer "target" selectively from buffer "start".
 * "termchar" terminates the information in start, and preceding
 * or trailing white space is ignored. The location just after the
 * terminating character is returned.  
 */
static char *
fill(start,target,termchar)
    char *start;
    char **target;
    char termchar;
{
    register char *p;
    register char *q;
    char *r;
	unsigned size;
	
 
    for (p = start; *p == ' ' || *p == '\t'; p++)
        ;
    r = strchr(p, termchar);
    if (r == NULL) {
        return(NULL);
    }
    if (p == r) {
		*target = NULL;	
    } else {
        for (q = r-1; *q == ' ' || *q == '\t'; q--)
            ;
		size = q - p + 1;
		*target = malloc(size+1);
		(void) strncpy(*target,p,(int) size);
		(*target)[size] = 0;
	}
    return(r+1);
}



static char *
match(group)
	char *group;
{
	char *val;
	int vallen;
	int err;

	yellowup(0, domain);
	if (usingyellow)
	{
	    err = yp_match(domain,"netgroup",group,strlen(group),&val,&vallen);
	    if (err) {
#ifdef DEBUG
		(void) fprintf(stderr,"yp_match(netgroup,%s) failed: %s\n",group
				,yperr_string(err));
#endif /* DEBUG */
		/* HPNFS If the NIS error was because there is a map but the   */
		/* key doesn't match anything on it then return NULL,         */
		/* otherwise, check the local /etc/netgroup.  If the local    */
		/* file does not contain the group return NULL.		      */
		return(NULL);
	     }
	}
	else
	{
	    if (uselocal(group, &val, &vallen) == 0)
		return(NULL);
	}
	return(val);
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

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

/* HPNFS This routine parses through the local /etc/netgroup file until */
/* it finds the group it is looking for as the first entry in the line. */
/* It returns a 1 and sets val to be the rest of the line for that      */
/* netgroup if it finds the netgroup, otherwise it returns a 0.		*/

#ifdef _NAMESPACE_CLEAN
#undef uselocal
#pragma _HP_SECONDARY_DEF _uselocal uselocal
#define uselocal _uselocal
#endif

uselocal(key, val, vallen)
	char *key, **val;
	int *vallen;
{
	char *p, *cp, *cp1, *ngroup;
	char lline[BUFSIZ+1];
	static struct stat netgrpstat;
	struct stat newstat;

	if (stat(NETGRDB, &newstat) < 0)
		return(0);
	if ((newstat.st_ino != netgrpstat.st_ino) ||
	    (newstat.st_mtime != netgrpstat.st_mtime)) {
		if (netgrf != NULL)
			fclose(netgrf);
		if ((netgrf = fopen(NETGRDB, "r")) == NULL)
			return(0);
		netgrpstat = newstat;
	} else {
		if (netgrf != NULL)
			rewind(netgrf);
		else	/* is NULL -- the file is not open */
			if ((netgrf = fopen(NETGRDB, "r")) == NULL)
				return(0);
	}

  while (TRUE)
   {
   do {
    if ((fgets(lline, BUFSIZ, netgrf)) == NULL)
    		return(0);
     } while (iseol(lline[0]));
	p = lline;
     for(;;) {
	while (!iseol(*p))
	{
	    p++;
	    }
	if (*p == '\n' && *(p - 1) == '\\') {
		*(p - 1) = ' ';
		if (!fgets(p, BUFSIZ, netgrf)) {
		break;
		}
		} else {
		  *p = 0;
		  break;
		  }
	}
	p = lline;
	ngroup = p;
	p = any(p, " \t");
	if (p == NULL)
	    continue;
	*p++ = '\0';

		if (strcmp(ngroup, key) == 0) 
		{
			*val = malloc(strlen(p)+1);
			strcpy(*val,p);
			*vallen = strlen(*val);
			return(1);
		}
	}
}
