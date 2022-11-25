/* @(#)getgroup.c	$Revision: 1.24.109.1 $	$Date: 91/11/19 14:22:16 $  
getgroup.c	2.1 86/04/16 NFSSRC 
static  char sccsid[] = "getgroup.c 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.";
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "table.h"
#include "util.h"
#include "getgroup.h"

#define MAXGROUPLEN 1024

/*
 * Stolen mostly, from getnetgrent.c          
 *
 * my_getgroup() performs the same function as _getgroup(), but operates 
 * on /etc/netgroup directly, rather than doing nis lookups.
 * 
 * /etc/netgroup must first loaded into a hash table so the matching
 * function can look up lines quickly.
 */


/* To check for cycles in netgroups */
struct list {
	char *name;
	struct list *nxt; 
};


extern stringtable ngtable; /* stored info from /etc/netgroup */

static struct grouplist *grouplist=NULL; /* stores a list of users in a group */

static char *any();
static char *match();
static char *fill();
static void freegrouplist();
static void doit();
#ifdef NLS
extern nl_catd nlmsg_fd;
#endif NLS



static void
freegrouplist() 
{
	struct grouplist *gl;
	
	for (gl = grouplist; gl != NULL; gl = gl->gl_nxt) {
		if (gl->gl_name) FREE(gl->gl_name);
		if (gl->gl_domain) FREE(gl->gl_domain);
		if (gl->gl_machine) FREE(gl->gl_machine);
		FREE(gl);
	}
	grouplist = NULL;
}




struct grouplist *
my_getgroup(group)
	char *group;
{
	freegrouplist();
	doit(group,(struct list *) NULL);
	return grouplist;
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
			(void)fprintf(stderr,
				(catgets(nlmsg_fd,NL_SETN,2, "Cycle detected in /etc/netgroup: %s.\n")),group);
			return;	
		}
	}

/*  HPNFS
 *
 *  ALLOCA is defined in util.h.  The definition is based on alloca,
 *  a compiler and machine dependent routine which allocates a
 *  number of bytes of space in the stack frame of the caller, and
 *  returns a pointer to the allocated block.  This temporary space is
 *  automatically freed when the caller returns.  Since:
 *
 * 	1) alloca doesn't exist on HP-UX and
 *	2) doit uses little memory when allocating space for each node
 *	   of the struct list (and the total list is expected to be small)
 *
 *  simply replace any use of ALLOCA by MALLOC.  This change could be
 *  applied to getnetgrent.c, too.
 *  
 *  Dave Erickson, 1-22-87.
 *
 *  HPNFS  */

	ls = MALLOC(struct list);
	ls->name = group;
	ls->nxt = list;
	list = ls;
	
	p = val;
	while (p != NULL) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == EOS || *p =='#')
			break;
		if (*p == '(') {
			gpls = MALLOC(struct grouplist);
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
			*q = EOS;
			doit(p,list);
			*q = ' ';
		}
		p = any(p, " \t");
	}

	return;

syntax_error:
	(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3, "syntax error in /etc/netgroup\n")));
	(void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,4, "--- %s %s\n")),group,val);
	return;
}




/*
 * Fill a buffer "target" selectively from buffer "start".
 * "termchar" terminates the information in start, and preceding
 * or trailing white space is ignored.  If the buffer "start" is
 * empty, "target" is filled with "*". The location just after the 
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
	register char *r;
	int size;

	for (p = start; *p == ' ' || *p == '\t'; p++)
		;
	r = strchr(p, termchar);
	if (r == NULL) {
		return(NULL);	
	}
	if (p == r) {
/*		STRNCPY(*target, "*", 1);	*/
		*target = NULL;	
	} else {
		for (q = r-1; *q == ' ' || *q == '\t'; q--)
			;
		size = q-p+1;
		STRNCPY(*target, p, size);
	}
	return(r+1);	
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
	return (NULL);
}



/*
 * The equivalent of yp_match. Returns the match, or NULL if there is none. 
 */
static char *
match(group)
	char *group;
{
	return(lookup(ngtable,group));
}
