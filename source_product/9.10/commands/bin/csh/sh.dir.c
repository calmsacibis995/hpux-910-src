/* @(#) $Revision: 66.1 $ */      
/********************************************************
  C Shell - directory management
 ********************************************************/

#include <sys/param.h>

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 4	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include "sh.h"
#include "sh.dir.h"

struct	directory *dfind();
CHAR	*dfollow();
#if defined(DISKLESS) || defined(DUX)
CHAR	*dcanon();
CHAR	*find_dotdot();
#endif defined(DISKLESS) || defined(DUX)
struct	directory dhead;		/* "head" of loop */
int	printd;				/* force name to be printed */

#ifndef NONLS
CHAR 	fakev0[] = {'d','i','r','s','\0'};
static  CHAR 	*fakev[] = { fakev0, NOSTR };
#else
static	char	*fakev[] = { "dirs", 0 };
#endif

/*
 * dinit - initialize current working directory
 */
/**********************************************************************/
dinit(hp)
	CHAR *hp;
/**********************************************************************/
{
	register CHAR *cp;
	register struct directory *dp;
	CHAR path[MAXPATHLEN];


	if (loginsh && hp)
		cp = hp;
	else
		cp = getwd(path);
	dp = (struct directory *)calloc(sizeof (struct directory), 1);
	dp->di_name = savestr(cp);
	dp->di_count = 0;
	dhead.di_next = dhead.di_prev = dp;
	dp->di_next = dp->di_prev = &dhead;
	printd = 0;
	dnewcwd(dp);
}

/*
 * dodirs - list all directories in directory loop
 */
/**********************************************************************/
dodirs(v)
	CHAR **v;
/**********************************************************************/
{
	register struct directory *dp;
	bool lflag;
	CHAR *hp = value(CH_home);

#ifdef DEBUG_DIRS
  {
    CHAR **debugV;

    debugV = v;
    printf ("dodirs (1): %d\n", getpid ());
    while (*debugV)
      {
	printf ("\tdebugV: %s\n", to_char (*debugV));
	debugV ++;
      }
  }
#endif

	if (*hp == '\0')
		hp = NOSTR;
	if (*++v != NOSTR)
		if (eq(*v, "-l") && *++v == NOSTR)
		  lflag = 1;
		else
		  error((catgets(nlmsg_fd,NL_SETN,1, "Usage: dirs [ -l ]")));
	else
		lflag = 0;
	dp = dcwd;
	do {
		if (dp == &dhead)
			continue;
		if (!lflag && hp != NOSTR) {
			dtildepr(hp, dp->di_name);
		} else
			printf("%s", to_char(dp->di_name));
		printf(" ");
	} while ((dp = dp->di_prev) != dcwd);
	printf("\n");
}

/**********************************************************************/
dtildepr(home, dir)
	register CHAR *home, *dir;
/**********************************************************************/
{
	char end = *(dir+Strlen(home));
	if (!eq(home, "/") && prefix(home, dir) && (end=='/' || end=='\0'))
		printf("~%s", to_char(dir + Strlen(home)));
	else
		printf("%s", to_char(dir));
}

/*
 * dochngd - implement chdir command.
 */
/**********************************************************************/
dochngd(v)
	CHAR **v;
/**********************************************************************/
{
	register CHAR *cp;
	register struct directory *dp;

#ifdef DEBUG_DIRS
  {
    CHAR **debugV;

    debugV = v;
    printf ("dochngd (1): %d\n", getpid ());
    while (*debugV)
      {
	printf ("\tdebugV: %s\n", to_char (*debugV));
	debugV ++;
      }
  }
#endif

	printd = 0;
	if (*++v == NOSTR) {
		if ((cp = value(CH_home)) == NOSTR || *cp == 0)
			bferr((catgets(nlmsg_fd,NL_SETN,2, "No home directory")));
		if (chdir(to_char(cp)) < 0)
			bferr((catgets(nlmsg_fd,NL_SETN,3, "Can't change to home directory")));
		cp = savestr(cp);
	} else if ((dp = dfind(*v)) != 0) {
		printd = 1;
		if (chdir(to_char(dp->di_name)) < 0)
			Perror(to_char(dp->di_name));
		dcwd->di_prev->di_next = dcwd->di_next;
		dcwd->di_next->di_prev = dcwd->di_prev;
		goto flushcwd;
	} else
		cp = dfollow(*v);
	if (cp == 0)
	{
		Perror(to_char(*v));
		return(0);
	}
	dp = (struct directory *)calloc(sizeof (struct directory), 1);
	dp->di_name = cp;
	dp->di_count = 0;
	dp->di_next = dcwd->di_next;
	dp->di_prev = dcwd->di_prev;
	dp->di_prev->di_next = dp;
	dp->di_next->di_prev = dp;

#ifdef DEBUG_DIRS
  printf ("dochngd (2): %d\n", getpid());
  printf ("\tname: %s", to_char (dp -> di_name));
  printf (", next name: %s", to_char (dp -> di_next -> di_name));
  printf (", prev name: %s\n", to_char (dp -> di_prev -> di_name));
#endif

flushcwd:
	dfree(dcwd);
	dnewcwd(dp);
}

/*
 * dfollow - change to arg directory; 
 * if arg exists but is not accessible then returns 0
 * if arg does not exist then fall back on cdpath. --hn
 */

#ifndef NONLS
CHAR CH_dotdot_slash[] = {'.','.','/',0};
CHAR CH_dot_slash[] = {'.','/',0};
#else
#define CH_dotdot_slash 	"../"
#define CH_dot_slash		"./"
#endif

/**********************************************************************/
CHAR *
dfollow(cp)
	register CHAR *cp;
/**********************************************************************/
{
	register CHAR **cdp;
	struct varent *c;
	
	cp = globone(cp);
	if (chdir(to_char(cp)) == 0)
		goto gotcha;
	if (errno != 2)   /* arg exits but is not accessible */
		return(0);
	if (cp[0] != '/' && !prefix(CH_dot_slash, cp) && !prefix(CH_dotdot_slash, cp)
	    && (c = adrof(CH_cdpath))) {
		for (cdp = c->vec; *cdp; cdp++) {
			CHAR buf[MAXPATHLEN];

			Strcpy(buf, *cdp);
			Strcat(buf, CH_slash);
			Strcat(buf, cp);
			if (chdir(to_char(buf)) >= 0) {
				printd = 1;
				xfree(cp);
				cp = savestr(buf);
				goto gotcha;
			}
		}
	}
	if (adrof(cp)) {
		CHAR *dp = value(cp);

		if (dp[0] == '/' || dp[0] == '.')
			if (chdir(to_char(dp)) >= 0) {
				xfree(cp);
				cp = savestr(dp);
				printd = 1;
				goto gotcha;
			}
	}
	xfree(cp); 
	Perror(to_char(cp));

gotcha:
	if (*cp != '/') {
		CHAR *dp = calloc( ((unsigned)(Strlen(cp) + Strlen(dcwd->di_name) + 2) * sizeof(CHAR)), 1);
		Strcpy(dp, dcwd->di_name);
		Strcat(dp, CH_slash);
		Strcat(dp, cp);
		xfree(cp);
		cp = dp;

#ifdef DEBUG_DIRS
  printf ("dfollow (1): %d, dir name: %s\n", getpid (), to_char (cp));
#endif

	}
#if defined(DISKLESS) || defined(DUX)
	return (dcanon(cp));
#else not defined(DISKLESS) || defined(DUX)
	dcanon(cp);
	return (cp);
#endif not defined(DISKLESS) || defined(DUX)
}

/*
 * dopushd - push new directory onto directory stack.
 *	with no arguments exchange top and second.
 *	with numeric argument (+n) bring it to top.
 */
/**********************************************************************/
dopushd(v)
	CHAR **v;
/**********************************************************************/
{
	register struct directory *dp;

	printd = 1;
	if (*++v == NOSTR) {
		if ((dp = dcwd->di_prev) == &dhead)
			dp = dhead.di_prev;
		if (dp == dcwd)
			bferr((catgets(nlmsg_fd,NL_SETN,4, "No other directory")));
		if (chdir(to_char(dp->di_name)) < 0) {
			dp->di_prev->di_next = dp->di_next;
			dp->di_next->di_prev = dp->di_prev;
			dodirs(fakev);
			error((catgets(nlmsg_fd,NL_SETN,8, "Directory %s doesn't exist, removed from stack")),to_char(dp->di_name));
		/*	Perror(to_char(dp->di_name));	*/
		}
		dp->di_prev->di_next = dp->di_next;
		dp->di_next->di_prev = dp->di_prev;
		dp->di_next = dcwd->di_next;
		dp->di_prev = dcwd;
		dcwd->di_next->di_prev = dp;
		dcwd->di_next = dp;
	} else if (dp = dfind(*v)) {
		if (chdir(to_char(dp->di_name)) < 0) {
			dp->di_prev->di_next = dp->di_next;
			dp->di_next->di_prev = dp->di_prev;
			dodirs(fakev);
			error((catgets(nlmsg_fd,NL_SETN,8, "Directory %s doesn't exist, removed from stack")),to_char(dp->di_name));
		/*	Perror(to_char(dp->di_name));	*/
		}
	} else {
		register CHAR *cp;

		cp = dfollow(*v);
		if (cp == 0)
		{
			Perror(to_char(*v));
			return(0);
		}
		else
		{
			dp = (struct directory *)calloc(sizeof (struct directory), 1);
			dp->di_name = cp;
			dp->di_count = 0;
			dp->di_prev = dcwd;
			dp->di_next = dcwd->di_next;
			dcwd->di_next = dp;
			dp->di_next->di_prev = dp;
		}
	}
	dnewcwd(dp);
}

/*
 * dfind - find a directory if specified by numeric (+n) argument
 */
/**********************************************************************/
struct directory *
dfind(cp)
	register CHAR *cp;
/**********************************************************************/
{
	register struct directory *dp;
	register int i;
	register CHAR *ep;

	if (*cp++ != '+')
		return (0);
	for (ep = cp; digit(*ep); ep++)
		continue;
	if (*ep)
		return (0);
	i = getn(cp);
	if (i <= 0)
		return (0);
	for (dp = dcwd; i != 0; i--) {
		if ((dp = dp->di_prev) == &dhead)
			dp = dp->di_prev;
		if (dp == dcwd)
			bferr((catgets(nlmsg_fd,NL_SETN,5, "Directory stack not that deep")));
	}
	return (dp);
}

/*
 * dopopd - pop a directory out of the directory stack
 *	with a numeric argument just discard it.
 */
/**********************************************************************/
dopopd(v)
	CHAR **v;
/**********************************************************************/
{
	register struct directory *dp, *p;

	printd = 1;
	if (*++v == NOSTR)
		dp = dcwd;
	else if ((dp = dfind(*v)) == 0)
		bferr((catgets(nlmsg_fd,NL_SETN,6, "Bad directory")));
	if (dp->di_prev == &dhead && dp->di_next == &dhead)
		bferr((catgets(nlmsg_fd,NL_SETN,7, "Directory stack empty")));
	if (dp == dcwd) {
		if ((p = dp->di_prev) == &dhead)
			p = dhead.di_prev;
		if (chdir(to_char(p->di_name)) < 0) {
			p->di_prev->di_next = p->di_next;
			p->di_next->di_prev = p->di_prev;
			dodirs(fakev);
			error((catgets(nlmsg_fd,NL_SETN,8, "Directory %s doesn't exist, removed from stack")),to_char(p->di_name));
		/*	Perror(to_char(p->di_name));	*/
		}
	}
	dp->di_prev->di_next = dp->di_next;
	dp->di_next->di_prev = dp->di_prev;
	if (dp == dcwd)
		dnewcwd(p);
	else
		dodirs(fakev);
	dfree(dp);
}

/*
 * dfree - free the directory (or keep it if it still has ref count)
 */
/**********************************************************************/
dfree(dp)
	register struct directory *dp;
/**********************************************************************/
{

	if (dp->di_count != 0)
		dp->di_next = dp->di_prev = 0;
	else
		xfree(dp->di_name), xfree((CHAR *)dp);
}

/*
 * dcanon - canonicalize the pathname, removing excess ./ and ../ etc.
 *	we are of course assuming that the file system is standardly
 *	constructed (always have ..'s, directories have links)
 */
#if defined(DISKLESS) || defined(DUX)
CHAR *
#endif defined(DISKLESS) || defined(DUX)
/**********************************************************************/
dcanon(cp)
	CHAR *cp;
/**********************************************************************/
{
	register CHAR *p, *sp;
#if defined(DISKLESS) || defined(DUX)
	CHAR *Strncpy();
	CHAR root[MAXPATHLEN], *replace;
	register CHAR *plusp;
	int cp_len = Strlen(cp), len_diff;
#endif defined(DISKLESS) || defined(DUX)
	register bool slash;

	if (*cp != '/')
		abort();
	for (p = cp; *p; ) {		/* for each component */
		sp = p;			/* save slash address */
		while(*++p == '/')	/* flush extra slashes */
			;
		if (p != ++sp)
			Strcpy(sp, p);
		p = sp;			/* save start of component */
		slash = 0;
		while(*++p)		/* find next slash or end of path */
			if (*p == '/') {
				slash = 1;
				*p = 0;
				break;
			}
		if (*sp == '\0')	/* if component is null */
			if (--sp == cp)	/* if path is one CHAR (i.e. /) */
				break;
			else
				*sp = '\0';
#if defined(DISKLESS) || defined(DUX)
		else if (eq(sp, ".+")) {
			if (slash) {
				Strcpy(sp, ++p);
				p = --sp;
			} else if (--sp != cp)
				*sp = '\0';
		}
#endif defined(DISKLESS) || defined(DUX)
		else if (eq(sp, ".")) {
			if (slash) {
				Strcpy(sp, ++p);
				p = --sp;
			} else if (--sp != cp)
				*sp = '\0';
#if defined(DISKLESS) || defined(DUX)
		} else if (eq(sp, "..+")) {
		    if ((sp - 1) != cp)
		    {
			for (plusp = sp-2; (plusp != cp) && (*plusp != '/'); plusp--);

			if (plusp == cp)
			    Strcpy(root, CH_slash);
			else
			    *(Strncpy(root, cp, plusp-cp)+(plusp-cp)) = '\0';

			replace = find_dotdot(root, cp);

			len_diff = Strlen(replace) - Strlen(plusp);

			if (len_diff > 0){
			    CHAR *dp;
			    cp_len += (len_diff + 2);
			    dp = calloc((unsigned)(cp_len * sizeof(CHAR)), 1);

			    if (plusp != cp)
				Strcpy(dp, root);

			    if (replace != NULL) {
				Strcat(dp, CH_slash);
				Strcat(dp, replace);
			    }

			    if (slash) {
				Strcat(dp, CH_slash);
				Strcat(dp, ++p);
			    }

			    p = dp + (p - cp);
			    sp = dp + (sp - cp);

			    xfree(cp);
			    cp = dp;
			}
			else {
			    if (plusp == cp)
				*(plusp+1) = '\0';
			    else
				*plusp = '\0';

			    if (replace != NULL) {
				Strcat(plusp, CH_slash);
				Strcat(plusp, replace);
			    }

			    if (slash) {
				    Strcat(plusp, CH_slash);
				    Strcat(plusp, ++p);
			    }

			    p = plusp;
			}
		    }
		    else
		    {
			if (slash) {
			    Strcpy(sp, ++p);
			    p = sp - 1;
			}
			else
			    *sp = '\0';
		    }
#endif defined(DISKLESS) || defined(DUX)
		} else if (eq(sp, "..")) {
			if (--sp != cp)
				while (*--sp != '/')
					;
#if defined(DISKLESS) || defined(DUX)
			if ((sp > cp) && (*(sp-1) == '+')) {
			    plusp = sp;
			    do
				for (plusp--;
				    (plusp > cp) && (*plusp != '/'); plusp--);
			    while ((plusp > cp) && (*(plusp-1) == '+'));

			    if (plusp == cp)
				Strcpy(root, CH_slash);
			    else
				*(Strncpy(root, cp, plusp-cp)+(plusp-cp)) = '\0';
			    replace = find_dotdot(root, cp);

			    len_diff = Strlen(replace) - Strlen(plusp);

			    if (len_diff > 0){
				CHAR *dp;

				cp_len += (len_diff + 2);

				dp = calloc((unsigned)(cp_len * sizeof(CHAR)), 1);

				Strcpy(dp, root);

				if (replace != NULL) {
				    Strcat(dp, CH_slash);
				    Strcat(dp, replace);
				}

				if (slash) {
				    Strcat(dp, CH_slash);
				    Strcat(dp, ++p);
				}

				p = dp + (p - cp);
				sp = dp + (sp - cp);

				xfree(cp);
				cp = dp;
			    } else {
				if (plusp == cp+1)
				    *plusp-- = '\0';
				else {
				    if (plusp == cp && replace == NULL)
					*(plusp+1) = '\0';
				    else
					*plusp = '\0';
				}

				if (replace != NULL) {
				    Strcat(plusp, CH_slash);
				    Strcat(plusp, replace);
				}

				if (slash) {
					Strcat(plusp, CH_slash);
					Strcat(plusp, ++p);
				}

				p = plusp;
			    }
			} else
#endif defined(DISKLESS) || defined(DUX)
			if (slash) {
				Strcpy(++sp, ++p);
				p = --sp;
			} else if (cp == sp)
				*++sp = '\0';
			else
				*sp = '\0';
		} else if (slash)
			*p = '/';
	}

#ifdef DEBUG_DIRS
  printf ("dcanon (1): %d, dir name: %s\n", getpid (), to_char (cp));
#endif

	return(cp);
}

/*
 * dnewcwd - make a new directory in the loop the current one
 */
/**********************************************************************/
dnewcwd(dp)
	register struct directory *dp;
/**********************************************************************/
{

	dcwd = dp;
	set(CH_cwd, savestr(dcwd->di_name));
	if (printd)
		dodirs(fakev);
}


#if defined(DISKLESS) || defined(DUX)

/* Listed here, because find_dotdot()  returns pointers to these. */
CHAR buf[2][MAXPATHLEN+1];

#ifndef NONLS
CHAR CH_dotdot[] = {'.','.',0};
CHAR CH_dotdotplus[] = {'.','.','+',0};
CHAR CH_plus[] = {'+',0};
CHAR CH_plusslash[] = {'+','/',0};
#else
#define CH_dotdot	".."
#define CH_dotdotplus	"..+"
#define CH_plus		"+"
#define CH_plusslash	"+/"
#endif

/**********************************************************************/
CHAR *
find_dotdot(rootpath, startpath)
CHAR *rootpath, *startpath;
/**********************************************************************/
/* Find_dotdot() - This routine is used to resolve abiguity in a path.
 *  It is used when doing a cd .. or ..+ that may involves CDF's.  The
 *  "rootpath" passed in should be an unambiguos path (already resolved
 *  using this routine if necessary).  The "startpath" should include
 *  the rootpath as a prefix.  ie: startpath =
 *  <rootpath>/<some_other_path_components>.  The some_other_path_component
 *  will be resolved by this routine.  That resolution may be longer or
 *  shorter that the original component.  The returned strings will only
 *  be that component and won't include any part of the rootpath.  The
 *  algorithm used is basically a limited "pwd".  This routine does
 *  stats and open/read/closedir's.  It uses one file descriptor.
 */
{
    CHAR *Strchr();
    struct direct *dir;
    DIR *dirp;
    CHAR dot[MAXPATHLEN+1], dotdot[MAXPATHLEN+1];
    CHAR *ddplus, *ptr0, *ptr1, *tmp;
    struct stat statroot, statdot, statdotdot;
    int hidden, phidden;
    int i;

    buf[0][0] = '\0';
    buf[1][0] = '\0';
    ptr0 = buf[0];
    ptr1 = buf[1];

    hidden = phidden = 0;

				/* Count the /'s in the path. */
    for (i = 1, tmp = startpath; (tmp = Strchr(tmp+1,'/')) != NULL; i++);

				/* Stat the destination. */
    if (stat(to_char(rootpath), &statroot) < 0)
	return(NULL);

				/* Create the working path and stat it. */
    (void) Strcpy(dotdot, startpath);

    if (stat(to_char(dotdot), &statdotdot) < 0)
	return(NULL);

				/* If the startpath ends in a CDF, remember. */
    if (((statdotdot.st_mode & S_IFMT) == S_IFDIR) &&
	(statdotdot.st_mode & S_ISUID))
	hidden = 1;

				/* Quick check, is the rootpath == startpath? */
    if ((statdotdot.st_dev == statroot.st_dev)
    	    && (statdotdot.st_ino == statroot.st_ino)
	    && (statdotdot.st_cnode == statroot.st_cnode))
	return(NULL);

    for (; i > 0; i--)		/* Go until we would be at "/". */
    {
				/* Save where we are at. */
	(void) Strcpy(dot, dotdot);

	if (stat(to_char(dot), &statdot) < 0)
	    return(NULL);

	ddplus = dotdot + Strlen(dotdot);

	if (*(ddplus-1) != '/')
	    *ddplus++ = '/';

				/* Find the next highest directory. */
	Strcpy(ddplus, CH_dotdotplus);
	
				/* Try "..+" first, ... */
	if ((dirp = opendir(to_char(dotdot))) == NULL)
	{			/* Well, that didn't work, so try "..". */
	    Strcpy(ddplus, CH_dotdot);

	    if ((dirp = opendir(to_char(dotdot))) == NULL)
		return(NULL);	/* Oops, how did we get to here. */
	}

				/* Stat the new parent. */
	if (stat(to_char(dotdot), &statdotdot) < 0)
	    return(NULL);

		/* Check whether the parent directory is hidden or not. */
	if (((statdotdot.st_mode & S_IFMT) == S_IFDIR) &&
	    (statdotdot.st_mode & S_ISUID))
	    phidden = 1;
	else
	    phidden = 0;

	if (statroot.st_dev == statdotdot.st_dev)
	{	/* We are getting close, the rootpath and current path are on
		   the same device.  Just look for matching inode numbers. */
	    do
		if ((dir = readdir(dirp)) == NULL)
		    return(NULL);
	    while (dir->d_ino != statdot.st_ino);

	    if ((statroot.st_ino == statdotdot.st_ino) && 
		    (statroot.st_cnode == statdotdot.st_cnode))
	    {	/* We found the root path, return the path difference. */
		Strcpy(ptr1, to_short(dir->d_name));
		Strcat(ptr1, ptr0);
		if (hidden)
		    Strcat(ptr1, CH_plus);
		return(ptr1);
	    }
	}
	else do
	{				/* Look for inode = inode, etc ... */
	    if ((dir = readdir(dirp)) == NULL)
		return(NULL);
	    stat(dir->d_name, &statdotdot);
	}
	while ((statdotdot.st_ino != statdot.st_ino)
		&& (statdotdot.st_dev != statdot.st_dev)
		&& (statdotdot.st_cnode != statdot.st_cnode));

	closedir(dirp);

		/* Prepend this name to the front of the previos components. */
	if (phidden == 1)
	    Strcpy(ptr1, CH_plusslash);
	else
	    Strcpy(ptr1, CH_slash);

	Strcat(ptr1, to_short(dir->d_name));

	Strcat(ptr1, ptr0);

	tmp = ptr0;
	ptr0 = ptr1;
	ptr1 = tmp;
    }

    /* Didn't find the rootpath after we went back to '/' from startpath. */
    return(NULL);
}
#endif defined(DISKLESS) || defined(DUX)

