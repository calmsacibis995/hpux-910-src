/* @(#) $Revision: 72.1 $ */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define getpwnam _getpwnam
#define setpwent _setpwent
#define endpwent _endpwent
#define getpwuid _getpwuid
#define fgetpwent _fgetpwent
#define getpwent _getpwent
#define fgets _fgets
#define fputs _fputs
#define ltoa _ltoa
#define strlen _strlen
#define getdomainname _getdomainname
#define exit ___exit
#define fopen _fopen
#define fclose _fclose
#define rewind _rewind
#define strcmp _strcmp
#define strncpy _strncpy
#define strtol _strtol
#define strcpy _strcpy
#ifdef SHADOWPWD
#   define getspwnam _getspwnam
#   define setspwent _setspwent
#   define getspwent _getspwent
#   define endspwent _endspwent
#   define getspwuid _getspwuid
#   define getspwaid _getspwaid
#   define fgetspwent _fgetspwent
#endif
#define strncmp _strncmp
#define endnetgrent _endnetgrent
#define getnetgrent _getnetgrent
#define innetgr _innetgr
#define setnetgrent _setnetgrent
#define yp_first _yp_first
#define yp_match _yp_match
#define yp_next _yp_next

#ifdef DEBUG
#define fprintf _fprintf
#endif
#ifdef _ANSIC_CLEAN
#   define free _free
#   define malloc _malloc
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

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
extern int strncmp();
extern int strlen();
extern FILE *fopen();
extern int fclose();
extern char *fgets();
extern char *strcpy();
extern char *strncpy();
extern char *malloc();

#ifdef SHADOWPWD
#include <errno.h>

/*
 * Routines, variables and definitions for support of the "shadow"
 * password file.
 */
#define MAXAID	 2147483647
void setspwent();
void endspwent();
struct s_passwd *getspwent();
static void __setspwent();
static void __endspwent();
static struct passwd *__check_secure();
static struct s_passwd *__fgetspwent();
static struct s_passwd *__getspwent();
static struct s_passwd *__getspwnam();

static struct s_passwd s_passwd;
static struct s_passwd tmp_s_passwd;
static struct s_passwd NULLSPW = {NULL, NULL, 0, 0};
static char SPASSWD[]  = "/.secure/etc/passwd";
static char STAR[]     = "*";
static FILE *spwf = NULL;   /* pointer into /.secure/etc/passwd */
static FILE *t_spwf = NULL;
static char s_line[LINESIZE+1];
static char tmp_s_line[LINESIZE+1];
#endif

void setpwent();
void endpwent();
struct passwd *getpwent();
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
static struct passwd *getuidfromyellow();

static struct list
{
    char *name;
    struct list *nxt;
} *minuslist;			/* list of - items */
#endif /* HP_NFS */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getpwnam
#pragma _HP_SECONDARY_DEF _getpwnam getpwnam
#define getpwnam _getpwnam
#endif

struct passwd *
getpwnam(name)
register char *name;
{
    struct passwd *pw;

    setpwent();
    if (!pwf)
	return NULL;

#ifdef HP_NFS
    while (fgets(line, LINESIZE, pwf) != NULL)
    {
	if ((pw = interpret(line, strlen(line))) == NULL)
	    continue;
	if (matchname(line, &pw, name))
	{
	    endpwent();
#ifdef SHADOWPWD
	    return __check_secure(pw);
#else
	    return pw;
#endif
	}
    }
    endpwent();
    return NULL;
#else /* not HP_NFS */
    while ((pw = getpwent()) && strcmp(name, pw->pw_name))
	continue;
    endpwent();
    return pw;
#endif /* not HP_NFS */
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getpwuid
#pragma _HP_SECONDARY_DEF _getpwuid getpwuid
#define getpwuid _getpwuid
#endif

struct passwd *
getpwuid(uid)
register uid_t uid;
{
    struct passwd *pw;

    setpwent();
    if (!pwf)
	return NULL;
#ifdef HP_NFS
    uid = ((uid == UID_NOBODY) ? -2 : uid);

    while (fgets(line, LINESIZE, pwf) != NULL)
    {
	if ((pw = interpret(line, strlen(line))) == NULL)
	    continue;
	if (matchuid(line, &pw, uid))
	{
	    endpwent();
#ifdef SHADOWPWD
	    return __check_secure(pw);
#else
	    return pw;
#endif
	}
    }
    endpwent();
    return NULL;
#else /* not HP_NFS */
    while ((pw = getpwent()) && pw->pw_uid != uid)
	continue;
    endpwent();
    return pw;
#endif /* not HP_NFS */
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setpwent
#pragma _HP_SECONDARY_DEF _setpwent setpwent
#define setpwent _setpwent
#endif

void
setpwent()
{
#ifdef HP_NFS
    if (getdomainname(domain, sizeof (domain)) < 0)
    {
	(void)fputs(
	    "setpwent: getdomainname system call missing\n", stderr);
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

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef endpwent
#pragma _HP_SECONDARY_DEF _endpwent endpwent
#define endpwent _endpwent
#endif

void
endpwent()
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

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fgetpwent
#pragma _HP_SECONDARY_DEF _fgetpwent fgetpwent
#define fgetpwent _fgetpwent
#endif

struct passwd *
fgetpwent(f)
FILE *f;
{
#ifdef SHADOWPWD
    struct passwd *pw;

    if (fgets(line, LINESIZE, f) == NULL)
	return NULL;
    pw = interpret(line, strlen(line));
    if (pw != NULL)
	return __check_secure(pw);
    else
	return pw;
#else
    if (fgets(line, LINESIZE, f) == NULL)
	return NULL;
    return interpret(line, strlen(line));
#endif /* SHADOWPWD */
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

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getpwent
#pragma _HP_SECONDARY_DEF _getpwent getpwent
#define getpwent _getpwent
#endif

struct passwd *
getpwent()
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
	    "getpwent: getdomainname system call missing\n", stderr);
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
#ifdef SHADOWPWD
		return __check_secure(pw);
#else
		return pw;
#endif
	    }
	}
	else if (getnetgrent(&mach, &user, &dom))
	{
	    if (user)
	    {
		pw = getnamefromyellow(user, savepw);
		if (pw != NULL && !onminuslist(pw))
		{
#ifdef SHADOWPWD
		    return __check_secure(pw);
#else
		    return pw;
#endif
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
#ifdef SHADOWPWD
			return __check_secure(pw);
#else
			return pw;
#endif
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
#ifdef SHADOWPWD
		    return __check_secure(pw);
#else
		    return pw;
#endif
		}
		break;
	    }
	}
#else /* not HP_NFS */
#ifdef SHADOWPWD
	return __check_secure(pw);
#else
	return pw;
#endif
#endif /* not HP_NFS */
    }
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
matchname(line1, pwp, name)
char line1[];
struct passwd **pwp;
char *name;
{
    struct passwd *savepw;
    struct passwd *pw = *pwp;

    switch (line1[0])
    {
    case PLUS:
	if (strcmp(pw->pw_name, "+") == 0)
	{
	    savepw = save(pw);
	    pw = getnamefromyellow(name, savepw);
	    if (pw)
	    {
		*pwp = pw;
		return 1;
	    }
	    else
		return 0;
	}
	if (line1[1] == ATSIGN)
	{
	    if (innetgr(pw->pw_name + 2, (char *)NULL, name, domain))
	    {
		savepw = save(pw);
		pw = getnamefromyellow(name, savepw);
		if (pw)
		{
		    *pwp = pw;
		    return 1;
		}
	    }
	    return 0;
	}
	if (strcmp(pw->pw_name + 1, name) == 0)
	{
	    savepw = save(pw);
	    pw = getnamefromyellow(pw->pw_name + 1, savepw);
	    if (pw)
	    {
		*pwp = pw;
		return 1;
	    }
	    else
		return 0;
	}
	break;
    case MINUS:
	if (line1[1] == ATSIGN)
	{
	    if (innetgr(pw->pw_name + 2, (char *)NULL, name, domain))
	    {
		*pwp = NULL;
		return 0;
	    }
	}
	else if (strcmp(pw->pw_name + 1, name) == 0)
	{
	    *pwp = NULL;
	    return 0;
	}
	break;
    default:
	if (strcmp(pw->pw_name, name) == 0)
	    return 1;
    }
    return 0;
}

static
matchuid(line1, pwp, uid)
char line1[];
struct passwd **pwp;
uid_t uid;
{
    struct passwd *savepw;
    struct passwd *pw = *pwp;
    char *user, *mach, *dom;
    char group[256];

    switch (line1[0])
    {
    case PLUS:
	if (strcmp(pw->pw_name, "+") == 0)
	{
	    savepw = save(pw);
	    pw = getuidfromyellow(uid, savepw);
	    if (pw && !onminuslist(pw))
	    {
		*pwp = pw;
		return 1;
	    }
	    else
	    {
		return 0;
	    }
	}
	if (line1[1] == ATSIGN)
	{
	    (void)strcpy(group, pw->pw_name + 2);
	    savepw = save(pw);
	    pw = getuidfromyellow(uid, savepw);
	    if (pw &&
		innetgr(group, (char *)NULL, pw->pw_name, domain) &&
		!onminuslist(pw))
	    {
		*pwp = pw;
		return 1;
	    }
	    else
	    {
		return 0;
	    }
	}
	savepw = save(pw);
	pw = getnamefromyellow(pw->pw_name + 1, savepw);
	if (pw && pw->pw_uid == uid && !onminuslist(pw))
	{
	    *pwp = pw;
	    return 1;
	}
	else
	    return 0;
	break;
    case MINUS:
	if (line[1] == ATSIGN)
	{
	    if (innetgr(pw->pw_name + 2, (char *)NULL, "*", domain))
	    {
		/* everybody was subtracted */
		return 0;
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
	if ((pw->pw_uid == uid) && !onminuslist(pw))
	    return 1;
    }
    return 0;
}

static
uidof(name)
char *name;
{
    struct passwd *pw;

    pw = getnamefromyellow(name, &NULLPW);
    if (pw)
	return pw->pw_uid;
    else
	return MAXUID + 1;
}

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
getuidfromyellow(uid, savepw)
uid_t uid;
struct passwd *savepw;
{
    extern char *ltoa();

    struct passwd *pw;
    int reason;
    char *val;
    int vallen;
    char *uidstr;

    uidstr = ltoa(uid);
    reason = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr),
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

#ifdef SHADOWPWD
/*************** BEGIN SECURE PASSWD CODE ******************/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getspwnam
#pragma _HP_SECONDARY_DEF _getspwnam getspwnam
#define getspwnam _getspwnam
#endif

struct s_passwd *
getspwnam(name)
register char *name;
{
    struct s_passwd *spw;

    setspwent();
    if (!spwf)
	return NULL;

    while ((spw = getspwent()) && strcmp(name, spw->pw_name) != 0)
	continue;
    endspwent();
    return spw;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getspwuid
#pragma _HP_SECONDARY_DEF _getspwuid getspwuid
#define getspwuid _getspwuid
#endif

struct s_passwd *
getspwuid(uid)
register uid_t uid;
{
    struct passwd *pw;
    struct s_passwd spw;

    if ((pw = getpwuid(uid)) == NULL)
	return NULL;

    return getspwnam(pw->pw_name);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getspwaid
#pragma _HP_SECONDARY_DEF _getspwaid getspwaid
#define getspwaid _getspwaid
#endif

struct s_passwd *
getspwaid(aid)
register aid_t aid;
{
    struct s_passwd *spw;

    setspwent();
    if (!spwf)
	return NULL;

    while ((spw = getspwent()) && spw->pw_audid != aid)
	continue;
    endspwent();
    return spw;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setspwent
#pragma _HP_SECONDARY_DEF _setspwent setspwent
#define setspwent _setspwent
#endif

void
setspwent()
{
    if (spwf == NULL)
	spwf = fopen(SPASSWD, "r");
    else
	rewind(spwf);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef endspwent
#pragma _HP_SECONDARY_DEF _endspwent endspwent
#define endspwent _endspwent
#endif

void
endspwent()
{
    if (spwf != NULL)
    {
	fclose(spwf);
	spwf = NULL;
    }
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fgetspwent
#pragma _HP_SECONDARY_DEF _fgetspwent fgetspwent
#define fgetspwent _fgetspwent
#endif

struct s_passwd *
fgetspwent(f)
FILE *f;
{
    register char *p;
    char *end;
    long x, strtol();

    if ((p = fgets(s_line, LINESIZE, f)) == NULL)
	return NULL;

    s_passwd.pw_name = p;
    p = pwskip(p);
    s_passwd.pw_passwd = p;
    p = pwskip(p);
    if (p == NULL || *p == ':') /* check for non-null aid */
	return NULL;
    x = strtol(p, &end, 10);
    if (*end == NULL || *end != ':') /* check for numeric value */
	return NULL;
    p = pwskip(p);
    s_passwd.pw_audid = (x < 0 || x > MAXAID) ? (MAXAID + 1) : x;
    if (p == NULL || *p == ':')
	/* check for non-null audit flag */
	return NULL;
    x = strtol(p, &end, 10);
    s_passwd.pw_audflg = (x < 0 || x > 1) ? 1 : x;
				/* if audflg not zero */
    /* or 1 then set to 1 */
    p = s_passwd.pw_passwd;
    while (*p && *p != ',')
	p++;
    if (*p)
	*p++ = '\0';
    s_passwd.pw_age = p;
    return &s_passwd;
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getspwent
#pragma _HP_SECONDARY_DEF _getspwent getspwent
#define getspwent _getspwent
#endif

struct s_passwd *
getspwent()
{
    extern struct s_passwd *fgetspwent();

    if (spwf == NULL)
    {
	if ((spwf = fopen(SPASSWD, "r")) == NULL)
	    return NULL;
    }
    return fgetspwent(spwf);
}

/*
 * The following routines use a separate buffer and file pointers
 * to the secure passwd file to return information when called by
 * getpwent
 */

static struct s_passwd *
__getspwent()
{
    if (t_spwf == NULL)
    {
	if ((t_spwf = fopen(SPASSWD, "r")) == NULL)
	    return NULL;
    }
    return __fgetspwent(t_spwf);
}

static struct s_passwd *
__fgetspwent(f)
FILE *f;
{
    register char *p;
    char *end;
    long x, strtol();

    if ((p = fgets(tmp_s_line, LINESIZE, f)) == NULL)
	return NULL;

    tmp_s_passwd.pw_name = p;
    p = pwskip(p);
    tmp_s_passwd.pw_passwd = p;
    p = pwskip(p);
    if (p == NULL || *p == ':') /* check for non-null aid */
	return NULL;
    x = strtol(p, &end, 10);
    if (*end == NULL || *end != ':') /* check for numeric value */
	return NULL;
    p = pwskip(p);
    tmp_s_passwd.pw_audid = (x < 0 || x > MAXAID) ? (MAXAID + 1) : x;
    if (p == NULL || *p == ':') /* check for non-null audit flag */
	return NULL;
    x = strtol(p, &end, 10);
    tmp_s_passwd.pw_audflg = (x < 0 || x > 1) ? 1 : x;
				/* if audflg not zero */
    /* or 1 then set to 1 */
    p = tmp_s_passwd.pw_passwd;
    while (*p && *p != ',')
	p++;
    if (*p)
	*p++ = '\0';
    tmp_s_passwd.pw_age = p;
    return &tmp_s_passwd;
}

static struct s_passwd *
__getspwnam(name)
	register char *name;
{
    struct s_passwd *spw;

    __setspwent();
    if (!t_spwf)
	return NULL;

    while ((spw = __getspwent()) && strcmp(name, spw->pw_name) != 0)
	continue;
    __endspwent();
    return spw;
}

static void
__setspwent()
{
    if (t_spwf == NULL)
	t_spwf = fopen(SPASSWD, "r");
    else
	rewind(t_spwf);
}

static void
__endspwent()
{
    if (t_spwf != NULL)
    {
	fclose(t_spwf);
	t_spwf = NULL;
    }
}

static struct passwd *
__check_secure(pw)
struct passwd *pw;
{
    static int secure_exists = -1; /*
				    * 1 if SPASSWD exists
				    * 0 if SPASSWD does not exist
				    * -1 if first call to __check_secure
				    */
    struct s_passwd *spw;

    if (pw == NULL)		   /* Return if called with invalid arg */
	return NULL;

    pw->pw_audid = pw->pw_audflg = -1;

    if (secure_exists == 0)
	return pw;

    if (secure_exists == 1)
    {
	if ((spw = __getspwnam(pw->pw_name)) != NULL)
	{
	    pw->pw_passwd = spw->pw_passwd;
	    pw->pw_audflg = spw->pw_audflg;
	    pw->pw_audid = spw->pw_audid;
	    pw->pw_age = spw->pw_age;
	}
	return pw;
    }

    if (secure_exists == -1)
    {
	__setspwent();
	if (t_spwf == NULL)    /* non-NULL if able to open the file */
	    secure_exists = 0;
	else
	{
	    secure_exists = 1;
	    if ((spw = __getspwnam(pw->pw_name)) != NULL)
	    {
	        pw->pw_passwd = spw->pw_passwd;
	        pw->pw_audflg = spw->pw_audflg;
	        pw->pw_audid = spw->pw_audid;
	        pw->pw_age = spw->pw_age;
	    }
	}
	return pw;
    }
}
#endif /* SHADOWPWD */
