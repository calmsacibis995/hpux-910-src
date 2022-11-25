/*
	These libc routines are provided from Jim Sttraton of COSL to 
	improve the performance of getpwent().  Since COSL can not provide
	a permenant fix to the performance problem (to search through a 
	passwd file of 5000 entries in size on a secure system takes 48 
	minues), the workaround COSL provides here is a local copy of 
	getpwent() along with some other related routines with which 
	sendmail can be built to avoid the unacceptable performance problem.
	To avoid the collision in names, we have renamed those routines.

			--- Theresa Chen, 09/22/92 ---

	These are versions of setpwent_(), getpwent_() and endpwent_() that
	do not read the shadow password file in order to speed up
	performance.  These should only be used when the pw_passwd,
	pw_audid and pw_audflg fields of the passwd struct are not needed.
*/

static char rcsid[] = "$Header: getpwentNS.c,v 1.1.109.3 95/02/21 16:07:55 mike Exp $";

#include <sys/param.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif /* HP_NFS */

#define LINESIZE  512	   /* used to size /etc/passwd line buffers  */

#define	  COLON		':'
#define	  COMMA		','
#define	  NEWLINE	'\n'
#define	  PLUS		'+'
#define	  MINUS		'-'
#define	  ATSIGN	'@'
#define	  EOS		'\0'

extern void rewind();
extern long strtol();
extern int strcmp();
extern int strlen();
extern FILE *fopen();
extern int fclose();
extern char *fgets();
extern char *strcpy();
extern char *strncpy();
extern char *malloc();

void setpwent_();
void endpwent_();
struct passwd *getpwent_();
static struct passwd *interpret();

static struct passwd NULLPW =
    { NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL};
static char PASSWD[]   = "/etc/passwd";

static char EMPTY[] = "";
static FILE *pwf = NULL;   /* pointer into /etc/passwd */

static char line[LINESIZE+1];

#ifdef HP_NFS
static char domain[256];
static char *yp;	/* pointer into yellow pages */
static int yplen;
static char *oldyp = NULL;
static int oldyplen;

static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();

static struct list
{
    char *name;
    struct list *nxt;
} *minuslist;			/* list of - items */
#endif /* HP_NFS */


void
setpwent_()
{
#ifdef HP_NFS
    if (getdomainname(domain, sizeof (domain)) < 0)
    {
	(void)fputs(
	    "setpwent_: getdomainname system call missing\n", stderr);
	exit(1);
    }
#endif /* HP_NFS */
    if (pwf == NULL)
	pwf = fopen(PASSWD, "r");
    else
	rewind(pwf);
#ifdef HP_NFS
    if (yp)
	free(yp);
    yp = NULL;
    freeminuslist();
#endif /* HP_NFS */
}

void
endpwent_()
{
    if (pwf != NULL)
    {
	(void)fclose(pwf);
	pwf = NULL;
    }
#ifdef HP_NFS
    if (yp)
	free(yp);
    yp = NULL;
    freeminuslist();
    endnetgrent();
#endif /* HP_NFS */
}

struct passwd *
getpwent_()
{
    struct passwd *pw;
#ifdef HP_NFS
    static struct passwd *savepw;
    char *user;
    char *mach;
    char *dom;

    if (domain[0] == 0 && getdomainname(domain, sizeof (domain)) < 0)
    {
	(void)fputs(
	    "getpwent_: getdomainname system call missing\n", stderr);
	exit(1);
    }
#endif /* HP_NFS */
    if (pwf == NULL && (pwf = fopen(PASSWD, "r")) == NULL)
	return NULL;

    for (;;)
    {
#ifdef HP_NFS
	if (yp)
	{
	    pw = interpretwithsave(yp, yplen, savepw);
	    free(yp);
	    if (pw == NULL)
		return NULL;
	    getnextfromyellow();
	    if (!onminuslist(pw))
	    {
		return pw;
	    }
	}
	else if (getnetgrent(&mach, &user, &dom))
	{
	    if (user)
	    {
		pw = getnamefromyellow(user, savepw);
		if (pw != NULL && !onminuslist(pw))
		{
		    return pw;
		}
	    }
	}
	else
	{
	    endnetgrent();
#endif /* HP_NFS */
	    if (fgets(line, LINESIZE, pwf) == NULL)
		return NULL;

	    if ((pw = interpret(line, strlen(line))) == NULL)
		return NULL;
#ifdef HP_NFS
	    switch (line[0])
	    {
	    case PLUS:
		if (strcmp(pw->pw_name, "+") == 0)
		{
		    getfirstfromyellow();
		    savepw = save(pw);
		}
		else if (line[1] == ATSIGN)
		{
		    savepw = save(pw);
		    if (innetgr(pw->pw_name + 2, (char *)NULL, "*", domain))
		    {
			/* include the whole yp database */
			getfirstfromyellow();
		    }
		    else
		    {
			setnetgrent(pw->pw_name + 2);
		    }
		}
		else
		{
		    /*
		     * else look up this entry in yellow pages
		     */
		    savepw = save(pw);
		    pw = getnamefromyellow(pw->pw_name + 1, savepw);
		    if (pw != NULL && !onminuslist(pw))
		    {
			return pw;
		    }
		}
		break;
	    case MINUS:
		if (line[1] == ATSIGN)
		{
		    if (innetgr(pw->pw_name + 2, (char *)NULL, "*", domain))
		    {
			/* everybody was subtracted */
			return NULL;
		    }
		    setnetgrent(pw->pw_name + 2);
		    while (getnetgrent(&mach, &user, &dom))
		    {
			if (user)
			{
			    addtominuslist(user);
			}
		    }
		    endnetgrent();
		}
		else
		{
		    addtominuslist(pw->pw_name + 1);
		}
		break;
	    default:
		if (!onminuslist(pw))
		{
		    return pw;
		}
		break;
	    }
	}
#else /* not HP_NFS */
	return pw;
#endif /* not HP_NFS */
    }
}

static char *
pwskip(p)
register char *p;
{
    while (*p && *p != COLON && *p != NEWLINE)
	p++;
    if (*p == NEWLINE)
	*p = EOS;
    else if (*p != EOS)
	*p++ = EOS;
    return p;
}

static struct passwd *
interpret(ptr, len)
char *ptr;
int len;
{
    register char *p;
    char *end;
    long x, strtol();
    static struct passwd passwd;
    static char line[LINESIZE + 1];
    register int ypentry;

    (void)strncpy(line, ptr, len);
    p = line;
    line[len] = NEWLINE;
    line[len + 1] = EOS;

    /*
     * Set "ypentry" if this entry references the Yellow Pages;
     * if so, null UIDs and GIDs are allowed (because they will be
     * filled in from the matching Yellow Pages entry).
     */
    ypentry = (*p == PLUS);

    passwd.pw_name = p;
    p = pwskip(p);
    if (line[0] == MINUS)
    {				/* We got a "-<entry>" */
	passwd.pw_passwd =
	passwd.pw_age =
	passwd.pw_comment =
	passwd.pw_gecos =
	passwd.pw_dir =
	passwd.pw_shell = "";
	passwd.pw_uid = passwd.pw_gid = 0;
	return &passwd;
    }

    passwd.pw_passwd = p;
    p = pwskip(p);
    if (*p == COLON && !ypentry)
	/* check for non-null uid */
	return NULL;
    x = strtol(p, &end, 10);
    p = end;
    if (*p++ != COLON && !ypentry)
	/* check for numeric value - must have stopped on the colon */
	return NULL;
    passwd.pw_uid = (x < -2 || x > MAXUID) ? (MAXUID + 1) : x;
    if (*p == COLON && !ypentry)
	/* check for non-null gid */
	return NULL;
    x = strtol(p, &end, 10);
    p = end;
    if (*p++ != COLON && !ypentry)
	/* check for numeric value - must have stopped on the colon */
	return NULL;
    passwd.pw_gid = (x < 0 || x > MAXUID) ? (MAXUID + 1) : x;
    passwd.pw_comment = EMPTY;
    passwd.pw_gecos = p;
    p = pwskip(p);
    passwd.pw_dir = p;
    p = pwskip(p);
    passwd.pw_shell = p;
    (void)pwskip(p);

    p = passwd.pw_passwd;
    while ((*p != EOS) && (*p != COMMA))
	p++;

    if (*p == COMMA)
	*p++ = EOS;

    passwd.pw_age = p;

    return &passwd;
}


#ifdef HP_NFS

static
getnextfromyellow()
{
    int reason;
    char *key;
    int keylen;

    reason = yp_next(domain, "passwd.byname", oldyp, oldyplen,
		     &key, &keylen, &yp, &yplen);
    if (reason)
    {
#ifdef DEBUG
	fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
	yp = NULL;
    }
    if (oldyp)
	free(oldyp);
    oldyp = key;
    oldyplen = keylen;
}

static
getfirstfromyellow()
{
    int reason;
    char *key;
    int keylen;

    /*
     * ensure that oldyp will not attempt to free unowned space
     * several lines hence
     */
    oldyp = NULL;

    reason = yp_first(domain, "passwd.byname",
		      &key, &keylen, &yp, &yplen);
    if (reason)
    {
#ifdef DEBUG
	fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
	yp = NULL;
    }
    if (oldyp)
	free(oldyp); /* NULL on first pass */
    oldyp = key;
    oldyplen = keylen;
}

static struct passwd *
getnamefromyellow(name, savepw)
char *name;
struct passwd *savepw;
{
    struct passwd *pw;
    int reason;
    char *val;
    int vallen;

    reason = yp_match(domain, "passwd.byname", name, strlen(name),
		      &val, &vallen);
    if (reason)
    {
#ifdef DEBUG
	fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
	return NULL;
    }
    else
    {
	pw = interpretwithsave(val, vallen, savepw);
	free(val);
	if (pw == NULL)
	    return NULL;

	return pw;
    }
}

static struct passwd *
interpretwithsave(val, len, savepw)
char *val;
struct passwd *savepw;
{
    struct passwd *pw;

    if ((pw = interpret(val, len)) == NULL)
	return NULL;
    if (savepw->pw_passwd && *savepw->pw_passwd)
	pw->pw_passwd = savepw->pw_passwd;
    if (savepw->pw_gecos && *savepw->pw_gecos)
	pw->pw_gecos = savepw->pw_gecos;
    if (savepw->pw_dir && *savepw->pw_dir)
	pw->pw_dir = savepw->pw_dir;
    if (savepw->pw_shell && *savepw->pw_shell)
	pw->pw_shell = savepw->pw_shell;
    return pw;
}

static
freeminuslist()
{
    struct list *ls, *next_ls;;

    for (next_ls = NULL, ls = minuslist; ls != NULL;)
    {
	next_ls = ls->nxt;

	free(ls->name);
	free((char *)ls);

	ls = next_ls;
    }
    minuslist = NULL;
}

static
addtominuslist(name)
char *name;
{
    struct list *ls;
    char *buf;

    ls = (struct list *)malloc(sizeof (struct list));
    buf = malloc((unsigned)strlen(name) + 1);
    (void)strcpy(buf, name);
    ls->name = buf;
    ls->nxt = minuslist;
    minuslist = ls;
}

/*
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the yellow pages
 */
static struct passwd *
save(pw)
struct passwd *pw;
{
    static int firsttime = 1;
    static struct passwd sv_pw =
    {
	NULL, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL
    };

    /* free up stuff from last call */
    if (firsttime == 0)
    {
	free(sv_pw.pw_passwd);
	free(sv_pw.pw_gecos);
	free(sv_pw.pw_dir);
	free(sv_pw.pw_shell);
    }
    else
	firsttime = 0;

    sv_pw.pw_passwd = malloc((unsigned)strlen(pw->pw_passwd) + 1);
    (void)strcpy(sv_pw.pw_passwd, pw->pw_passwd);

    sv_pw.pw_gecos = malloc((unsigned)strlen(pw->pw_gecos) + 1);
    (void)strcpy(sv_pw.pw_gecos, pw->pw_gecos);

    sv_pw.pw_dir = malloc((unsigned)strlen(pw->pw_dir) + 1);
    (void)strcpy(sv_pw.pw_dir, pw->pw_dir);

    sv_pw.pw_shell = malloc((unsigned)strlen(pw->pw_shell) + 1);
    (void)strcpy(sv_pw.pw_shell, pw->pw_shell);

    return &sv_pw;
}

static
onminuslist(pw)
struct passwd *pw;
{
    struct list *ls;
    register char *nm;

    nm = pw->pw_name;
    for (ls = minuslist; ls != NULL; ls = ls->nxt)
	if (strcmp(ls->name, nm) == 0)
	    return 1;
    return 0;
}

#endif /* HP_NFS */
