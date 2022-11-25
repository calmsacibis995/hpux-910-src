/* $Revision: 72.1 $ */     
/* $Header: getgrent.c,v 72.1 92/12/04 11:30:26 ssa Exp $ */

#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#define getgrgid_r _getgrgid_r
#define setgrent_r _setgrent_r
#define endgrent_r _endgrent_r
#define getgrnam_r _getgrnam_r
#define fgetgrent_r _fgetgrent_r
#define getgrent_r _getgrent_r
#else
#define getgrgid _getgrgid
#define setgrent _setgrent
#define endgrent _endgrent
#define getgrnam _getgrnam
#define fgetgrent _fgetgrent
#define getgrent _getgrent
#endif
#define getdomainname _getdomainname
#define fputs _fputs
#ifdef _REENTRANT_FUNCTIONS
#define ltoa_r _ltoa_r
#else
#define ltoa _ltoa
#endif
#define exit ___exit
#define fgets _fgets
#define fopen _fopen
#define fclose _fclose
#define rewind _rewind
#define strcmp _strcmp
#define strlen _strlen
#define strncpy _strncpy
#define strtol _strtol
#define strcpy _strcpy
#define yp_first _yp_first
#define yp_match _yp_match
#define yp_next _yp_next
#ifdef DEBUG
#   define fprintf _fprintf
#endif
#       ifdef   _ANSIC_CLEAN
#define free _free
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <stdio.h>
#include <grp.h>
#include <limits.h>
#ifdef _REENTRANT_FUNCTIONS
#include <errno.h>
#include <values.h>
#include "rec_mutex.h"

extern REC_MUTEX _group_rmutex;
#endif

#ifdef _REENTRANT_FUNCTIONS
/* align an address onto a valid char * value */
#define	ALIGN(a)	(char *)(((unsigned)(a))&~(sizeof(char *) - 1))
#endif

#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif HP_NFS

#define	COLON	':'
#define	COMMA	','
#define	NEWLINE	'\n'
#define	PLUS	'+'
#define	MINUS	'-'
#define	EOS	'\0'

#define	MAXGRP	(LINE_MAX-50)/9
#ifndef _REENTRANT_FUNCTIONS
#define	MAXINT 0x7fffffff;
#endif
#define MAX_LINE LINE_MAX       /* Maximum length of line in /etc/group */

extern int fclose();
extern char *fgets();
extern FILE *fopen();
extern void rewind();
extern long strtol();
extern size_t strlen();
extern int strcmp();
extern char *strcpy();
extern char *strncpy();
extern void *malloc();
extern void free();
extern void exit();
extern char *ltoa();

#ifndef _REENTRANT_FUNCTIONS
void setgrent(), endgrent();
struct group *getgrent();

static struct group *interpret();

static char GROUP[] = "/etc/group";
static FILE *grf = NULL;
static char *gr_mem[MAXGRP];

#ifdef HP_NFS
static char domain[256];
static char *yp;		/* pointer into yellow pages */
static int yplen;
static char *oldyp = NULL;	
static int oldyplen;

static struct list {
    char *name;
    struct list *nxt;
} *minuslist;			/* list of - items */

static struct group *save();
static struct group *getnamefromyellow();
static struct group *getgidfromyellow();
#endif HP_NFS

#else

static char GROUP[] = "/etc/group";

static char domain[256];

void setgrent_r(), endgrent_r();
static struct group *interpret_r();
static struct group *save();
static struct group *getnamefromyellow_r();
static struct group *getgidfromyellow_r();
static struct group *restruct_group();
static struct _gr_state *findgrs();
static struct _gr_state *allocgrs();

struct list
{
    char *name;
    struct list *nxt;
};			/* list of - items */

/* these are created on a per FILE object basis */
struct _gr_state
{
    FILE *grf;
    char *d_yp;		/* pointer into yellow pages */
    int d_yplen;
    char *d_oldyp;
    int d_oldyplen;
    struct list *d_minuslist;
    struct group savegp;
    struct _gr_state *next;
};

static struct _gr_state *grsp;
static struct _gr_state *grslist = NULL;

#define yp			grsp->d_yp
#define yplen			grsp->d_yplen
#define oldyp			grsp->d_oldyp
#define oldyplen		grsp->d_oldyplen
#define minuslist		grsp->d_minuslist

#define setgrent()		setgrent_r(&grf)
#define endgrent()		endgrent_r(&grf)
#define matchname(a,b,c)        matchname_r(a, b, c, buffer, buflen)
#define matchgid(a,b,c)         matchgid_r(a, b, c, buffer, buflen)
#define getnamefromyellow(a,b)  getnamefromyellow_r(a,b,group,buffer,buflen)
#define getgidfromyellow(a,b)   getgidfromyellow_r(a,b,group,buffer,buflen)
#define interpret(a, b)		interpret_r(a, b, group, buffer, buflen)

#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef getgrgid_r
#pragma _HP_SECONDARY_DEF _getgrgid_r getgrgid_r
#define getgrgid_r _getgrgid_r
#else
#undef getgrgid
#pragma _HP_SECONDARY_DEF _getgrgid getgrgid
#define getgrgid _getgrgid
#endif
#endif

#ifdef _REENTRANT_FUNCTIONS
int
getgrgid_r(gid, group, buffer, buflen)
register gid_t gid;
struct group *group;
char *buffer;
int buflen;
#else
struct group *
getgrgid(gid)
register gid_t gid;
#endif
{
	struct group *gp;
#ifdef _REENTRANT_FUNCTIONS
	FILE *grf = NULL;

	if ((group == NULL) || (buffer == NULL) || (buflen < 1)) {
		errno = EINVAL;
		return(-1); 
	}

	_rec_mutex_lock(&_group_rmutex);
#endif

	setgrent();
	if (!grf)
	{
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_unlock(&_group_rmutex);
		return(-1);
#else
		return NULL;
#endif
	}

#ifdef HP_NFS
	{
	    char line[MAX_LINE+1];

	    while (fgets(line, MAX_LINE, grf) != NULL) {
		    if ((gp = interpret(line, (int)strlen(line))) == NULL)
			    continue;
		    if (matchgid(&gp, gid, line)) {
			    endgrent();
#ifdef _REENTRANT_FUNCTIONS
			    _rec_mutex_unlock(&_group_rmutex);
			    return ((gp) ? 0 : -1);
#else
			    return gp;
#endif
		    }
	    }
	}
	endgrent();
#ifdef _REENTRANT_FUNCTIONS
	_rec_mutex_unlock(&_group_rmutex);
	return(-1);
#else
	return NULL;
#endif
#else not HP_NFS
	while((gp = getgrent()) && gp->gr_gid != gid)
		;
	endgrent();
	return(gp);
#endif not HP_NFS
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef getgrnam_r
#pragma _HP_SECONDARY_DEF _getgrnam_r getgrnam_r
#define getgrnam_r _getgrnam_r
#else
#undef getgrnam
#pragma _HP_SECONDARY_DEF _getgrnam getgrnam
#define getgrnam _getgrnam
#endif
#endif

#ifdef _REENTRANT_FUNCTIONS
int
getgrnam_r(name, group, buffer, buflen)
register char *name;
struct group *group;
char *buffer;
int buflen;
#else
struct group *
getgrnam(name)
register char *name;
#endif
{
	struct group *gp;
#ifdef _REENTRANT_FUNCTIONS
	FILE *grf = NULL;

	if ((group == NULL) || (buffer == NULL) || (buflen < 1)) {
		errno = EINVAL;
		return(-1); 
	}
#endif
	if(name == NULL || *name == '\0')
#ifdef _REENTRANT_FUNCTIONS
		return(-1);
#else
		return(NULL);
#endif

#ifdef _REENTRANT_FUNCTIONS
	_rec_mutex_lock(&_group_rmutex);
#endif
	setgrent();
	if (!grf)
	{
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_lock(&_group_rmutex);
		return(-1);
#else
		return NULL;
#endif
	}
#ifdef HP_NFS
	{
	    char line[MAX_LINE+1];

	    while (fgets(line, MAX_LINE, grf) != NULL) {
		    if ((gp = interpret(line, (int)strlen(line))) == NULL)
			    continue;
		    if (matchname(&gp, name, line)) {
			    endgrent();
#ifdef _REENTRANT_FUNCTIONS
			    _rec_mutex_unlock(&_group_rmutex);
			    return ((gp) ? 0 : -1);
#else
			    return gp;
#endif
		    }
	    }
	}
	endgrent();
#ifdef _REENTRANT_FUNCTIONS
	_rec_mutex_unlock(&_group_rmutex);
	return(-1);
#else
	return NULL;
#endif
#else not HP_NFS
	while((gp = getgrent()) && strcmp(gp->gr_name, name))
		;
	endgrent();
	return(gp);
#endif not HP_NFS
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef setgrent_r
#pragma _HP_SECONDARY_DEF _setgrent_r setgrent_r
#define setgrent_r _setgrent_r
#else
#undef setgrent
#pragma _HP_SECONDARY_DEF _setgrent setgrent
#define setgrent _setgrent
#endif
#endif

void
#ifdef _REENTRANT_FUNCTIONS
setgrent_r(grfp)
FILE **grfp;
#else
setgrent()
#endif
{
#ifdef _REENTRANT_FUNCTIONS
	if (grfp == NULL) return;
        _rec_mutex_lock(&_group_rmutex);
#endif
#ifdef HP_NFS
	if (getdomainname(domain, sizeof(domain)) < 0) {
		(void)fputs("setgrent: getdomainname system call missing\n",
			    stderr);
		exit(1);
	}
#endif HP_NFS
#ifdef _REENTRANT_FUNCTIONS
	if(*grfp == NULL)
	{
        	if ((*grfp = fopen(GROUP, "r")) == NULL) {
        		_rec_mutex_unlock(&_group_rmutex);
        		return;
        	}
        	if ((grsp = allocgrs(*grfp)) == NULL) {
        		fclose(*grfp);
        		*grfp = NULL;
        		_rec_mutex_unlock(&_group_rmutex);
        		return;
        	}
	}
	else /* find the gr state struct associated with this FILE object */
	{
        	if ((grsp = findgrs(*grfp)) == NULL) {
            		_rec_mutex_unlock(&_group_rmutex);
            		return;
		}
		rewind(*grfp);

		if (yp) {
			free(yp);
			yp = NULL;
		}
		if (oldyp) {
			free(oldyp);
			oldyp = NULL;
		}
		freeminuslist();
	}
	_rec_mutex_unlock(&_group_rmutex);
#else
	if(grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);
#ifdef HP_NFS
	if (yp) {
		free((void *)yp);
		yp = NULL;
	}
	freeminuslist();
#endif HP_NFS
#endif
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef endgrent_r
#pragma _HP_SECONDARY_DEF _endgrent_r endgrent_r
#define endgrent_r _endgrent_r
#else
#undef endgrent
#pragma _HP_SECONDARY_DEF _endgrent endgrent
#define endgrent _endgrent
#endif
#endif

void
#ifdef _REENTRANT_FUNCTIONS
endgrent_r(grfp)
FILE **grfp;
#else
endgrent()
#endif
{
#ifdef _REENTRANT_FUNCTIONS
	if (grfp == NULL) return;

        _rec_mutex_lock(&_group_rmutex);
	if(*grfp != NULL) {
	        if ((grsp = findgrs(*grfp)) == NULL) {
       			_rec_mutex_unlock(&_group_rmutex);
        		return;
        	}

		(void) fclose(*grfp);
		*grfp = NULL;
		if (yp) {
			free(yp);
			yp = NULL;
		}
		if (oldyp) {
			free(oldyp);
			oldyp = NULL;
		}
		freeminuslist();

		deallocgrs(grsp);
	}
	_rec_mutex_unlock(&_group_rmutex);
#else
	if(grf != NULL) {
		(void) fclose(grf);
		grf = NULL;
	}
#ifdef HP_NFS
	if (yp) {
		free((void *)yp);
		yp = NULL;
	}
	if (oldyp) {
		free((void *)oldyp);
		oldyp = NULL;
	}
	freeminuslist();
#endif HP_NFS
#endif
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef fgetgrent_r
#pragma _HP_SECONDARY_DEF _fgetgrent_r fgetgrent_r
#define fgetgrent_r _fgetgrent_r
#else
#undef fgetgrent
#pragma _HP_SECONDARY_DEF _fgetgrent fgetgrent
#define fgetgrent _fgetgrent
#endif
#endif

#ifdef _REENTRANT_FUNCTIONS
int
fgetgrent_r(f, group, buffer, buflen)
	FILE *f;
        struct group *group;
	char *buffer;
	int buflen;
#else
struct group *
fgetgrent(f)
	FILE *f;
#endif
{
	char line[MAX_LINE+1];
#ifdef _REENTRANT_FUNCTIONS
	int retval;

	if ((group == NULL) || (buffer == NULL) || (buflen < 1)) {
		errno = EINVAL;
		return(-1); 
	}
#endif
	if(fgets(line, MAX_LINE, f) == NULL)
#ifdef _REENTRANT_FUNCTIONS
		return(-1);
	return ((interpret(line, (int)strlen(line))) ? 0 : -1);
#else
		return(NULL);
	return (interpret(line, (int)strlen(line)));
#endif
}

static char *
grskip(p,c)
	register char *p;
	register c;
{
	while(*p != EOS && *p != c && *p != NEWLINE)
		++p;
	if (*p == NEWLINE)
		*p = EOS;
	else if (*p != EOS)
		*p++ = EOS;
	return(p);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#ifdef _REENTRANT_FUNCTIONS
#undef getgrent_r
#pragma _HP_SECONDARY_DEF _getgrent_r getgrent_r
#define getgrent_r _getgrent_r
#else
#undef getgrent
#pragma _HP_SECONDARY_DEF _getgrent getgrent
#define getgrent _getgrent
#endif
#endif

#ifdef _REENTRANT_FUNCTIONS
int
getgrent_r(group, buffer, buflen, grfp)
struct group *group;
char *buffer;
int buflen;
FILE **grfp;
#else
struct group *
getgrent()
#endif
{
	struct group *gp;
#ifdef HP_NFS
#ifdef _REENTRANT_FUNCTIONS
	FILE *grf = NULL;
	struct group *savegp;
	char line2[MAX_LINE + 1];
	struct group tempgp;
#else
	static struct group *savegp;
#endif
	char line[MAX_LINE+1];
	int reason;
	char *key;
	int keylen;

#ifdef _REENTRANT_FUNCTIONS
	if ((group == NULL) || (buffer == NULL) || (buflen < 1) || (grfp == NULL)) {
		errno = EINVAL;
		return(-1); 
	}

	_rec_mutex_lock(&_group_rmutex);
#endif
	if (domain[0] == 0 && getdomainname(domain, sizeof(domain)) < 0) {
		(void)fputs("getgrent: getdomainname system call missing\n",
			    stderr);
		exit(1);
	}
#endif HP_NFS

#ifdef _REENTRANT_FUNCTIONS
	if(*grfp == NULL)
	{
        	if ((*grfp = fopen(GROUP, "r")) == NULL) {
        		_rec_mutex_unlock(&_group_rmutex);
        		return -1;
        	}
        	if ((grsp = allocgrs(*grfp)) == NULL) {
        		fclose(*grfp);
        		*grfp = NULL;
        		_rec_mutex_unlock(&_group_rmutex);
        		return -1;
        	}
	}
	else /* find the gr state struct associated with this FILE object */
	{
        	if ((grsp = findgrs(*grfp)) == NULL) {
            		_rec_mutex_unlock(&_group_rmutex);
            		return -1;
		}
	}
	grf = *grfp;
	savegp = &grsp->savegp;
#else
	if(grf == NULL && (grf = fopen(GROUP, "r")) == NULL)
		return(NULL);
#endif
#ifdef HP_NFS
  again:
	if (yp) {
#ifdef _REENTRANT_FUNCTIONS
		gp = interpret_r(yp, yplen, &tempgp, line2, MAX_LINE + 1);
#else
		gp = interpret(yp, yplen);
#endif

		free((void *)yp);
		yp = NULL;

		if (gp == NULL)
		{
#ifdef _REENTRANT_FUNCTIONS
			_rec_mutex_unlock(&_group_rmutex);
			return(-1);
#else
			return(NULL);
#endif
		}

		if (savegp->gr_passwd && *savegp->gr_passwd)
			gp->gr_passwd =  savegp->gr_passwd;
		if (savegp->gr_mem && *savegp->gr_mem)
			gp->gr_mem = savegp->gr_mem;

		if (reason = yp_next(domain, "group.byname",
		    oldyp, oldyplen, &key, &keylen, &yp, &yplen)) {
		        /*
			 * Either there are no more entries, or there is
			 * a legitimate error (the original SUN code does
			 * not seem to care to differentiate the two).
			 */
#ifdef DEBUG
			(void)fprintf(stderr,
				      "reason yp_next failed is %d\n", reason);
#endif
			if (yp) {
				free((void *)yp);
				yp = NULL;
			}
			if (oldyp) {
				free((void *)oldyp);
				oldyp = NULL;
			}
		}
		else {
			if (oldyp)
				free((void *)oldyp);
			oldyp = key;
			oldyplen = keylen;
		}

		if (onminuslist(gp))
			goto again;
		else
			{
#ifdef _REENTRANT_FUNCTIONS
			_rec_mutex_unlock(&_group_rmutex);
			if (restruct_group(gp, group, buffer, buflen) == NULL)
				return -1;
			else
				return 0;
#else
			return gp;
#endif
			}
	}
	else if (fgets(line, MAX_LINE, grf) == NULL)
	{
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_unlock(&_group_rmutex);
		return(-1);
#else
		return(NULL);
#endif
	}

	if ((gp = interpret(line, (int)strlen(line))) == NULL)
	{
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_unlock(&_group_rmutex);
		return(-1);
#else
		return(NULL);
#endif
	}
	switch(line[0]) {
	case PLUS:
		if (strcmp(gp->gr_name, "+") == 0) {
			if (oldyp) {
				free((void *)oldyp);
				oldyp = NULL;
			}
			if (reason = yp_first(domain, "group.byname",
					      &key, &keylen, &yp, &yplen)) {
#ifdef DEBUG
				(void)fprintf(stderr,
					      "reason yp_first failed is %d\n",
					      reason);
#endif
				if (yp) {
					free((void *)yp);
					yp = NULL;
				}
			}
			else {
				oldyp = key;
				oldyplen = keylen;
			}

			savegp = save(gp);
			goto again;
		}
		/* 
		 * else look up this entry in yellow pages
		 */
		savegp = save(gp);
		gp = getnamefromyellow(gp->gr_name+1, savegp);
		if (gp == NULL || onminuslist(gp))
			goto again;
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_unlock(&_group_rmutex);
		return 0;
#else
		return gp;
#endif
	case MINUS:
		addtominuslist(gp->gr_name+1);
		goto again;
	default:
		if (onminuslist(gp))
			goto again;
#ifdef _REENTRANT_FUNCTIONS
		_rec_mutex_unlock(&_group_rmutex);
		return 0;
#else
		return gp;
#endif
	}
#else not HP_NFS
	return (fgetgrent(grf));
#endif not HP_NFS
}

static struct group *
#ifdef _REENTRANT_FUNCTIONS
interpret_r(ptr, len, group, line, linesize)
char *ptr;
int len;
struct group *group;
char *line;
int linesize;
#else
interpret(ptr, len)
char *ptr;
int len;
#endif
{
	register char *p, **q;
	char *end;
	long x;
#ifdef _REENTRANT_FUNCTIONS
	char **gr_mem, **gr_mem_end;
#else
	static struct group gp;
	static char line[MAX_LINE+1];
#endif
	register int ypentry;

#ifdef _REENTRANT_FUNCTIONS
#define gp	(*group)

	if (linesize <= len + 1)
		return(NULL);
#endif
	(void)strncpy(line, ptr, (size_t)len);
	p = line;
	line[len] = NEWLINE;
	line[len+1] = 0;

#ifdef HP_NFS
	/*
 	 * Set "ypentry" if this entry references the Yellow Pages;
	 * if so, null GIDs are allowed (because they will be filled in
	 * from the matching Yellow Pages entry).
	 */
	ypentry = (*p == PLUS);
#else not HP_NFS
	ypentry = 0;		/* No NFS, no YP therefore set this flase. */
#endif HP_NFS

#ifdef _REENTRANT_FUNCTIONS
	gr_mem = (char **)ALIGN(p + strlen(p) + sizeof(char *) - 1);
	gr_mem_end = (char **)ALIGN(p + linesize) - sizeof(char *);
#endif
	gp.gr_name = p;
	p = grskip(p,COLON);
	gp.gr_passwd = p;
	p = grskip(p,COLON);
	if (*p == COLON && !ypentry)
		/* check for non-null gid */
		return (NULL);
	x = strtol(p, &end, 10);	
	p = end;
	if (*p++ != COLON && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	gp.gr_gid = x;
	gp.gr_mem = gr_mem;
	(void) grskip(p,NEWLINE);
	q = gr_mem;
	while(*p){
#ifdef _REENTRANT_FUNCTIONS
		if (q < gr_mem_end)
#else
		if (q < &gr_mem[MAXGRP-1])
#endif
			*q++ = p;
		p = grskip(p,COMMA);
	}
	*q = NULL;
#ifdef _REENTRANT_FUNCTIONS
#undef	gp

	return(group);
#else
	return(&gp);
#endif
}

#ifdef HP_NFS

static
freeminuslist() {
	struct list *ls, *next_ls;
	
	for ( next_ls = NULL, ls = minuslist; ls != NULL; )
	{
	    next_ls = ls->nxt;

	    free((void *)(ls->name));
	    free((void *)ls);

	    ls = next_ls;
	}
	minuslist = NULL;
}


static
onminuslist(gp)
	struct group *gp;
{
	struct list *ls;
	register char *nm;
	
	nm = gp->gr_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt)
		if (strcmp(ls->name, nm) == 0)
			return 1;
	return 0;
}


static struct group *
#ifdef _REENTRANT_FUNCTIONS
getnamefromyellow_r(name, savegp, group, buffer, buflen)
	char *name;
	struct group *savegp;
	struct group *group;
	char *buffer;
	int buflen;
#else
getnamefromyellow(name, savegp)
	char *name;
	struct group *savegp;
#endif
{
	struct group *gp;
	int reason;
	char *val;
	int vallen;
#ifdef _REENTRANT_FUNCTIONS
	char line[MAX_LINE + 1];
	struct group tempgp;
#endif
	
	if (reason = yp_match(domain, "group.byname",
	    name, (int)strlen(name), &val, &vallen)) {
#ifdef DEBUG
	        (void)fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	}
	else {
#ifdef _REENTRANT_FUNCTIONS
		gp = interpret_r(val, vallen, &tempgp, line, MAX_LINE + 1);
#else
		gp = interpret(val, vallen);
#endif
		free((void *)val);
		if (gp == NULL)
			return NULL;
		if (savegp->gr_passwd && *savegp->gr_passwd)
			gp->gr_passwd =  savegp->gr_passwd;
		if (savegp->gr_mem && *savegp->gr_mem)
			gp->gr_mem = savegp->gr_mem;
#ifdef _REENTRANT_FUNCTIONS
		return restruct_group(gp, group, buffer, buflen);
#else
		return gp;
#endif
	}
}

static
addtominuslist(name)
	char *name;
{
	struct list *ls;
	char *buf;
	
	ls = (struct list *)malloc(sizeof(struct list));
	buf = (char *)malloc(strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gr_mem fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the yellow pages
 */
static struct group *
save(gp)
	struct group *gp;
{
#ifdef _REENTRANT_FUNCTIONS
#define sv_grp	(grsp->savegp)
#else
	static int firsttime = 1;
	static char *save_gr_mem[MAXGRP];	
	static struct group sv_grp = {NULL, NULL, 0, save_gr_mem};
#endif
	char **p1, **p2;
	
	/* 
	 * free up stuff from last time around
	 */
#ifndef _REENTRANT_FUNCTIONS
	if (firsttime == 0)
	{
#endif
		for (p1 = sv_grp.gr_mem;
#ifdef _REENTRANT_FUNCTIONS
			(p1 < &sv_grp.gr_mem[MAXGRP-1]) && (*p1 != NULL); p1++)
#else
			(p1 < &save_gr_mem[MAXGRP-1]) && (*p1 != NULL); p1++)
#endif
		{
			free((void *)*p1);
			*p1 = NULL;
		}

		if (sv_grp.gr_passwd != NULL)
		{
			free((void *)(sv_grp.gr_passwd));
			sv_grp.gr_passwd = NULL;
		}
#ifndef _REENTRANT_FUNCTIONS
	}
	else firsttime = 0;
#endif

	sv_grp.gr_passwd = (char *)malloc(strlen(gp->gr_passwd) + 1);
	(void) strcpy(sv_grp.gr_passwd, gp->gr_passwd);

#ifdef _REENTRANT_FUNCTIONS
	for (p1 = gp->gr_mem, p2 = sv_grp.gr_mem;
		(*p1 != NULL) && (p2 < &sv_grp.gr_mem[MAXGRP-1]); p1++, p2++)
#else
	for (p1 = gp->gr_mem, p2 = save_gr_mem;
		(*p1 != NULL) && (p2 < &save_gr_mem[MAXGRP-1]); p1++, p2++)
#endif
	{
		*p2 = (char *)malloc(strlen(*p1) + 1);
		(void) strcpy(*p2, *p1);
	}

	*p2 = 0;

	return (&sv_grp);
#ifdef _REENTRANT_FUNCTIONS
#undef sv_grp
#endif
}

static
#ifdef _REENTRANT_FUNCTIONS
matchname_r(gpp, name, line, buffer, buflen)
	struct group **gpp;
	char *name;
	char *line;
	char *buffer;
	int buflen;
#else
matchname(gpp, name, line)
	struct group **gpp;
	char *name;
	char *line;
#endif
{
	struct group *savegp;
	struct group *gp = *gpp;
#ifdef _REENTRANT_FUNCTIONS
	struct group *group = *gpp;
#endif

	switch(line[0]) {
		case PLUS:
			if (strcmp(gp->gr_name, "+") == 0) {
				savegp = save(gp);
				gp = getnamefromyellow(name, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			if (strcmp(gp->gr_name+1, name) == 0) {
				savegp = save(gp);
				gp = getnamefromyellow(gp->gr_name+1, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			break;
		case MINUS:
			if (strcmp(gp->gr_name+1, name) == 0) {
				*gpp = NULL;
				return 1;
			}
			break;
		default:
			if (strcmp(gp->gr_name, name) == 0)
				return 1;
	}
	return 0;
}

static
#ifdef _REENTRANT_FUNCTIONS
matchgid_r(gpp, gid, line, buffer, buflen)
	struct group **gpp;
	char *line;
	char *buffer;
	int buflen;
#else
matchgid(gpp, gid, line)
	struct group **gpp;
	gid_t gid;
	char *line;
#endif
{
	struct group *savegp;
	struct group *gp = *gpp;
#ifdef _REENTRANT_FUNCTIONS
	struct group *group = *gpp;
#endif

	switch(line[0]) {
		case PLUS:
			if (strcmp(gp->gr_name, "+") == 0) {
				savegp = save(gp);
				gp = getgidfromyellow(gid, savegp);
				if (gp) {
					*gpp = gp;
					return 1;
				}
				else
					return 0;
			}
			savegp = save(gp);
			gp = getnamefromyellow(gp->gr_name+1, savegp);
			if (gp && gp->gr_gid == gid) {
				*gpp = gp;
				return 1;
			}
			else
				return 0;
			/*NOTREACHED*/
			break;
		case MINUS:
			if (gid == gidof(gp->gr_name+1)) {
				*gpp = NULL;
				return 1;
			}
			break;
		default:
			if (gp->gr_gid == gid)
				return 1;
	}
	return 0;
}

static
gidof(name)
	char *name;
{
	static struct group NULLGP = {NULL, NULL, 0, NULL};
	struct group *gp;
#ifdef _REENTRANT_FUNCTIONS
	char line[MAX_LINE + 1];
	struct group tempgp;
	
	gp = getnamefromyellow_r(name, &NULLGP, &tempgp, line, MAX_LINE + 1);
#else
	gp = getnamefromyellow(name, &NULLGP);
#endif
	if (gp)
		return gp->gr_gid;
	else
		return MAXINT;
}

static struct group *
#ifdef _REENTRANT_FUNCTIONS
getgidfromyellow_r(gid, savegp, group, buffer, buflen)
	int gid;
	struct group *savegp;
	struct group *group;
	char *buffer;
	int buflen;
#else
getgidfromyellow(gid, savegp)
	gid_t gid;
	struct group *savegp;
#endif
{
	struct group *gp;
	int reason;
	char *val;
	int vallen;
#ifdef _REENTRANT_FUNCTIONS
#define LTOA_MAX	(BITS(long)/3 + 2)
	char gidstr[LTOA_MAX];
	int gidstrlen;
	char line[MAX_LINE + 1];
	struct group tempgp;
	
	gidstrlen = ltoa_r(gid, gidstr, LTOA_MAX);
#else
	char *gidstr;
	
	gidstr = ltoa((long)gid);
#endif
	if (reason = yp_match(domain, "group.bygid",
#ifdef _REENTRANT_FUNCTIONS
	    gidstr, gidstrlen, &val, &vallen)) {
#else
	    gidstr, (int)strlen(gidstr), &val, &vallen)) {
#endif
#ifdef DEBUG
	        (void)fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	}
	else {
#ifdef _REENTRANT_FUNCTIONS
		gp = interpret_r(val, vallen, &tempgp, line, MAX_LINE + 1);
#else
		gp = interpret(val, vallen);
#endif
		free((void *)val);
		if (gp == NULL)
			return NULL;
		if (savegp->gr_passwd && *savegp->gr_passwd)
			gp->gr_passwd =  savegp->gr_passwd;
		if (savegp->gr_mem && *savegp->gr_mem)
			gp->gr_mem = savegp->gr_mem;
#ifdef _REENTRANT_FUNCTIONS
		return restruct_group(gp, group, buffer, buflen);
#else
		return gp;
#endif
	}
}

#ifdef _REENTRANT_FUNCTIONS
/*
 * Copy information from static buffer to user-supplied buffer
 */
static struct group *
restruct_group(gp, user_gp, buffer, buflen)
struct group *gp;
struct group *user_gp;
char *buffer;
int buflen;
{
	register char *p = buffer;
	int i;
	int len;
	int members = 0;
	char **p1, **p2;
	char *temp;

	user_gp->gr_gid = gp->gr_gid;

	user_gp->gr_name = p;
	if ((len = _copy_string(p, gp->gr_name, buflen)) == -1)
		return NULL;
	buflen -= len + 1;
	p += len + 1;

	user_gp->gr_passwd = p;
	if ((len = _copy_string(p, gp->gr_passwd, buflen)) == -1)
		return NULL;
	buflen -= len + 1;
	p += len + 1;

	for (p1 = gp->gr_mem; *p1 != NULL; p1++) members++;

	temp = p;
	user_gp->gr_mem = (char **) ALIGN(p + sizeof(char *) - 1);
	p = (char *) user_gp->gr_mem + (members + 1) * sizeof(char *);
	len = temp - p;
	if ((buflen -= len) <= 0)
		return NULL;

	for (i = 0; i < members; i++) {
		user_gp->gr_mem[i] = p;
		if ((len = _copy_string(p, gp->gr_mem[i], buflen)) == -1)
			return NULL;
		buflen -= len + 1;
		p += len + 1;
	}

	user_gp->gr_mem[members] = NULL;

	return user_gp;
}

/*
 * Find a free _gr_state struc, or allocate a new one.
 * NOTE: the following routines assume the group mutex is locked.
 */
static struct _gr_state *
allocgrs(fp)
FILE *fp;
{
	char **p1;
	struct _gr_state *gs;

	for (gs = grslist; gs; gs = gs->next)
		if (gs->grf == NULL) break;
	
	if (gs)		/* found a free one */
		gs->grf = fp;
	else {			/* allocate some space and initialize */
		if ((gs = malloc(sizeof(struct _gr_state))) == NULL)
			return NULL;

		if ((gs->savegp.gr_mem = malloc(sizeof(char *) * MAXGRP)) == NULL) {
			free(gs);
			return NULL;
		}
		for (p1 = gs->savegp.gr_mem;
			p1 < &gs->savegp.gr_mem[MAXGRP]; p1++)
			*p1 = NULL;

		gs->grf = fp;
		gs->d_yp = NULL;
		gs->d_oldyp = NULL;
		gs->d_minuslist = NULL;
		gs->savegp.gr_passwd = NULL;
		gs->next = grslist;
		grslist = gs;
	}
	
	return gs;
}

/*
 * Release any memory allocated for savegp and mark the _gr_state
 * structure as free.
 */
static int
deallocgrs(gs)
struct _gr_state *gs;
{
	char **p1;

	gs->grf = NULL;
	if (gs->savegp.gr_passwd) {
		free(gs->savegp.gr_passwd);
		gs->savegp.gr_passwd = NULL;
	}
	for (p1 = gs->savegp.gr_mem;
		(p1 < &gs->savegp.gr_mem[MAXGRP-1]) && (*p1 != NULL); p1++)
	{
		free(*p1);
		*p1 = NULL;
	}
}

/*
 * Find the _gr_state structure that corresponds to the given FILE
 * object.
 */
static struct _gr_state *
findgrs(fp)
FILE *fp;
{
	struct _gr_state *gs;

	for (gs = grslist; gs; gs = gs->next)
		if (gs->grf == fp) return gs;
	return NULL;
}
#endif
#endif /* HP_NFS */
