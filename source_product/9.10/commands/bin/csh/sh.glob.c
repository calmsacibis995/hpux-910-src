/* @(#) $Revision: 72.2 $ */   
/**************************************************************************
 * C Shell
 **************************************************************************/

#include <sys/param.h>
#include <unistd.h>
#include <fnmatch.h>

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 9	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif
#include "sh.h"

#ifdef NLS
extern	int	_nl_space_alt;
#define	ALT_SP	(_nl_space_alt & TRIM)
#endif

int	globcnt;

char	*globchars =	"`{[*?";

CHAR	*gpath, *gpathp, *lastgpathp;
int	globbed;
bool	noglob;
bool	nonomatch;
CHAR	*entp;
CHAR	**sortbas;
extern char	*_uae_index;

/*  Called by:
	doexec ()
	doforeach ()
	echo ()
	doeval ()
	globone ()
	set1 ()
*/
/**********************************************************************/
CHAR **
glob(v)
	register CHAR **v;
/**********************************************************************/
{
	CHAR agpath[MAXPATHLEN];
	CHAR *agargv[GAVSIZ];
	static globfirstime=1;

/*  Initialize these to point to agpath, and initialize it to NULL.
    globcnt is a global.
*/

#ifdef TRACE_DEBUG
  printf ("glob (1): pid: %d, gargc: %d, globcnt: %d\n", getpid (), gargc, 
	  globcnt);
#endif

	gpath = agpath; gpathp = gpath; *gpathp = 0;
	lastgpathp = &gpath[sizeof agpath / sizeof (CHAR) - 2];
	ginit(agargv); globcnt = 0;

#ifdef TRACE_DEBUG
  printf ("glob (2): pid: %d, gargc: %d, globcnt: %d\n", getpid (), gargc, 
	  globcnt);
#endif

/*  Look for CH_noglob in shvar list.  If found then noglob = 1, otherwise 0.
*/
	noglob = (bool) (adrof(CH_noglob) != NULL);

/*  Look for CH_nonomatch in shvar list.  If found then nonomatch = 1, 
    otherwise 0.
*/
	/* casts to type 'bool' added to shut compiler up (twk) */

	nonomatch = (bool) (adrof(CH_nonomatch) != NULL);
	globcnt = (bool) noglob | nonomatch;

#ifdef TRACE_DEBUG
  printf ("glob (3): pid: %d, gargc: %d, globcnt: %d\n", getpid (), gargc, 
	  globcnt);
#endif

/*  Loop through all strings.
*/
	while (*v)
		collect(*v++);

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values:
*/
  {
    CHAR **traceGargv;

    traceGargv = gargv;

    printf ("glob (4): pid: %d, globcnt: %d, gflag: %o, gargc: %d\n", 
	     getpid (), globcnt, gflag, gargc);

    while (*traceGargv)
      {
        printf ("\tgargv: %s\n", to_char (*traceGargv));
        traceGargv ++;
      }
  }
#endif

	if (globcnt == 0 && (gflag&1)) {
		if (globfirstime)
		    globfirstime=0;
		else
		    blkfree(gargv); 
		gargv = 0;
		return (0);
	} else

/*  This routine calloc's new space and copies the gargv strings into it.
    Note that gargv was set to point to the local CHAR * array agargv[]
    by ginit.
*/
		return (gargv = copyblk(gargv));
}

/*  Called by:
	Dfix2 ()
	glob ()
*/
/**********************************************************************/
ginit(agargv)
	CHAR **agargv;
/**********************************************************************/
{

/*  gargv, gargc, and gnleft are global.
*/
	agargv[0] = 0; gargv = agargv; sortbas = agargv; gargc = 0;
	gnleft = NCARGS - 4;
}

/*  Called by:
	glob ()
*/
/**********************************************************************/
collect(as)
	register CHAR *as;
/**********************************************************************/
{
	register int i;

/*  If there is a backqote in the string, find the string between the backquotes
    and execute it.
*/
	if (Any('`', as)) {
#ifdef GDEBUG
		printf("doing backp of %s\n", to_char(as));
#endif
		(void) dobackp(as, 0);
#ifdef GDEBUG
		printf("backp done, acollect'ing\n");
#endif
		for (i = 0; i < pargc; i++)

/*  Loop through all the arguments in pargc.
*/
			if (noglob)

/*  Copy pargv into gargv.
*/
				Gcat(pargv[i], nullstr);
			else
				acollect(pargv[i]);

		if (pargv)

/*  Free all strings in pargv.
*/
			blkfree(pargv), pargv = 0;
#ifdef GDEBUG
		printf("acollect done\n");
#endif
	} else if (noglob || eq(as, "{") || eq(as, "{}")) {

/*  Copy as into gargv.
*/
		Gcat(as, nullstr);
		sort();
	} else
		acollect(as);
}

/*  Called by:
	collect ()
*/
/**********************************************************************/
acollect(as)
	register CHAR *as;
/**********************************************************************/
{
	register int ogargc = gargc;

	gpathp = gpath; *gpathp = 0; globbed = 0;
	expand(as);

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;

    traceGargv = gargv;

    printf ("acollect (1): pid: %d, gargc: %d, ogargc: %d\n", getpid (), gargc,
	    ogargc);

    while (*traceGargv)
      {
        printf ("\tgargv: %s\n", to_char (*traceGargv));
        traceGargv ++;
      }
  }
#endif

	if (gargc == ogargc) {
		if (nonomatch) {

/*  Copy as into gargv.
*/
			Gcat(as, nullstr);

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;

    traceGargv = gargv;
    printf ("acollect (2): pid: %d, gargc=ogargc, nonomatch\n", getpid ());
    while (*traceGargv)
      {
        printf ("\tArgv: %s\n", to_char (*traceGargv));
        traceGargv ++;
      }
  }
#endif
			sort();
		}
	} else
		sort();

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;

    traceGargv = gargv;

    printf ("acollect (3): pid: %d, Just before exit:\n", getpid ());
    while (*traceGargv)
      {
        printf ("\tArgv: %s\n", to_char (*traceGargv));
        traceGargv ++;
      }
  }
#endif
}
               /* MAXCH increased from 1024 --> NCARGS; */

#define MAXCH NCARGS

/*  Called by:
	collect ()
	acollect ()
	execbrc ()
*/
/**********************************************************************/
sort()
/**********************************************************************/
{
	register CHAR **p1, **p2, *c;
	CHAR **Gvp = &gargv[gargc];
	char tmp[MAXCH*2+1];

	p1 = sortbas;
	while (p1 < Gvp-1) {
		p2 = p1;
		while (++p2 < Gvp) {
			strcpy(tmp, to_char(*p1));
			if (strcoll(tmp, to_char(*p2)) > 0)
				c = *p1, *p1 = *p2, *p2 = c;
		}
		p1++;
	}
	sortbas = Gvp;
}
/*  Called by:
	acollect ()
	execbrc ()
	amatch ()
*/
/**********************************************************************/
expand(as)
	CHAR *as;
/**********************************************************************/
{
	register CHAR *cs;
	register CHAR *sgpathp, *oldcs;
	struct stat stb;

	sgpathp = gpathp;
	cs = as;
	if (*cs == '~' && gpathp == gpath) {

/*  Add ~ to gpathp.
*/
		addpath('~');

/*  Add charcters to gpathp if they are letters, digits, or -.
*/
		for (cs++; letter(*cs) || digit(*cs) || *cs == '-';)
			addpath(*cs++);

/*  If the character is NULL or / then the string was ~ or ~/ so
    gpathp = ~  and gpath = ~
	      ^             ^
	      |             |
*/
		if (!*cs || *cs == '/') {

/*  The string was ~xxx or ~xxx/.
*/
			if (gpathp != gpath + 1) {

/*  Make the string \0xxx.
*/
				*gpathp = 0;

/*  Copied the directory, so now we have \0/users/...
*/
				if (gethdir(gpath + 1))
					error((catgets(nlmsg_fd,NL_SETN,1, 
					"Unknown user: %s")), to_char(gpath+1));

/*  Copy the path, so now we have /users/...
*/
				Strcpy(gpath, gpath + 1);
			} else

/*  We had ~ or ~/; replace gpath with the home directory name.
*/
				Strcpy(gpath, value(CH_home));

/*  Find the NULL at the end of the path name.
*/
			gpathp = strend(gpath);
		}
	}

/*  As long as the character is not one of the glob characters (`{[*?)
    if the end of the string, add to gargv if no globbing is in effect.
    Otherwise stat the directory and then add it to gargv if it exists.
*/
	while (!any(*cs, globchars)) {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
  printf ("expand (1): pid: %d, cs: %s\n", getpid (), to_char(cs));
*/
#endif 

		if (*cs == 0) {
			if (!globbed)

/*  Store gpath in gargv.
*/
				Gcat(gpath, nullstr);
			else if (stat(to_char(gpath), &stb) >= 0) {
				Gcat(gpath, nullstr);
				globcnt++;
			}
			goto endit;
		}

/*  Add the character to gpathp.
*/
		addpath(*cs++);
	}

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
  printf ("expand (2): pid: %d, cs: %s\n", getpid (), to_char(cs));
  printf ("\tas: %s\n", to_char(as));
*/
#endif 

	oldcs = cs;

/*  Scan backwards to the / or the beginning of the string.  Then pick up
    the / and null out the rest of the path.
*/
	while (cs > as && *cs != '/')
		cs--, gpathp--;
	if (*cs == '/')
		cs++, gpathp++;
	*gpathp = 0;
	if (*oldcs == '{') {
		(void) execbrc(cs, NOSTR);
		return;
	}

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("expand (3): pid: %d, Calling matchdir with cs: %s\n", getpid (), 
          to_char(cs));
#endif 

	matchdir(cs);
endit:

/*  Reset gpathp to point to the beginning of the string.
*/
	gpathp = sgpathp;
	*gpathp = 0;
}

/*  Called by:
	expand ()
*/
/**********************************************************************/
matchdir(pattern)
	CHAR *pattern;
/**********************************************************************/
{
	register struct direct *dp;
	DIR *dirp;
	register CHAR *dir_name;
	register CHAR tmpd_name[MAXCH];
#if defined(DISKLESS) || defined(DUX)
	char path[MAXPATHLEN+1];
	char *entry;
	int len;
	int hidden = (int) adrof(CH_hidden);
#endif defined(DISKLESS) || defined(DUX)

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("matchdir (1): pid: %d, Pattern: %s\n", getpid (), to_char(pattern));
#endif 

	if (gpath[0] == '\0')
		dir_name = CH_dot;
	else
		dir_name = gpath;

#if defined(DISKLESS) || defined(DUX)
	if (hidden == 0)
	{
	    (void) strcpy(path, to_char(dir_name));

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
  printf ("matchdir (2): pid: %d, Path: %s\n", getpid (), path);
*/
#endif 

	    (void) strcat(path, "/");

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("matchdir (3): pid: %d, Path: %s\n", getpid (), path);
#endif 

	    len = strlen(path);
	    entry = path+len;
	}
#endif defined(DISKLESS) || defined(DUX)

	/* Use directory(3C) routines */

	if ((dirp = opendir(to_char(dir_name))) == (DIR *)0) {

		/* Handle error conditions in the same way as the    */
		/* original Berkeley csh. If we can't find the error */
		/* we will have to assume opendir failed to allocate */
		/* the memory it needs, so we set errno to ENOMEM.   */

		struct stat stb;
		int dirf;

		if ((dirf = open(to_char(dir_name), 0)) < 0) {
			if (globbed)
				return;
			goto patherr;
		}

		if (fstat(dirf,&stb) < 0) {
			(void) close(dirf);
			goto patherr;
		}
		(void) close(dirf);

		if (!isdir(stb))
			errno = ENOTDIR;
		else
			errno = ENOMEM;

		goto patherr;
	}

	while ((dp = readdir(dirp)) != (struct direct *)0) {
		Strcpy(tmpd_name, to_short(dp->d_name));

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("matchdir (4): pid: %d,\n", getpid ());
  printf ("\tCalling match with pattern: %s, ", to_char (pattern));
  printf ("file name: %s\n", to_char(tmpd_name));
#endif 

/*  Call match () with each name in the directory, and the original pattern.
*/
		if (match(tmpd_name, pattern)) {
#if defined(DISKLESS) || defined(DUX)
			if (hidden == 0) {      /* If "hidden" isn't set. */
			    struct stat stb;

/*  If a match occurred and hidden isn't set, entry was set to point to the
    character just after the last / in the full path name to the file (stored
    in path).  Copy the file name that matched into path, beginning at this
    position.  Then stat the file.  If this is a CDF and there is no context
    for the machine, then the stat will fail.  Otherwise it should succeed.
    If the stat fails, try appending a + and stat'ing it as a CDF.

    It looks to me like if the stat as a CDF works, the while loop going
    through the directory is continued, without the path getting copied to
    gpath, which is the thing eventually used.  If the while loop isn't
    continued, then the file name from the while loop directory read is
    copied onto gpath.  So if the file was a CDF that couldn't be stat'ed
    we forget about it.  It seems useless to check to see whether or not the 
    file is a CDF.
*/
			    /* Append entry name to directory. */

			    (void) strcpy(entry, dp->d_name);

			    /* If we can't stat the normal file, */
			    /*  check to see if it is a CDF by   */
			    /*  stating it with an appended '+'. */

			    if (stat(path, &stb) < 0) {
				len = strlen(path);


#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("matchdir (5): pid: %d, Can't stat file; trying with +\n", getpid ());
#endif 

				path[len] = '+';
				path[len+1] = '\0';

/*  We get to this point if the stat on the file fails.  This can occur if there
    is no context in the CDF for the local machine or if the file got removed.

    If the stat of the CDF fails, the original file name gets copied to gargv
    and we get a 'file not found' error.  If the stat of the CDF works, the 
    whole path gets ignored and a 'no match' error occurs.
*/
				if ((stat (path, &stb) >= 0) &&
				    ((stb.st_mode & S_IFMT) == S_IFDIR) &&
				    (stb.st_mode & S_ISUID))

				    /* If we can stat it with an appended   */
				    /* '+' and the file is a directory with */
				    /* setuid, then it's a CDF.             */

				    continue;
			    } /* if stat NOT OK */
                         
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
                          else
			    {
                              printf ("matchdir (6): pid: %d, ", getpid ());
			      printf ("Stat OK file: %s\n", path );
			    }
#endif 
			} /* if hidden == 0 */
#endif defined(DISKLESS) || defined(DUX)

/*  Gpath gets the file name without a '+'.
*/
			Gcat(gpath, to_short(dp->d_name));
			globcnt++;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;
    traceGargv = gargv;
    printf ("matchdir (7): pid: %d, gargc: %d\n", getpid (), gargc);
    while (*traceGargv)  
      {
	printf ("\tGargv: %s\n", to_char (*traceGargv));
	traceGargv ++;
      }
  }
#endif

		} /* if there is a string match */
	} /* while loop going through the directory */
	closedir(dirp);
	return;

patherr:
	Perror(to_char(gpath));
}

/*  Called by:
	expand ()
	amatch ()
*/
/**********************************************************************/
execbrc(p, s)
	CHAR *p, *s;
/**********************************************************************/
{
	CHAR restbuf[WRDSIZ + 2];
	register CHAR *pe, *pm, *pl;
	int brclev = 0;
	int level;
	CHAR *lm, savec, *sgpathp;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values:
*/
  printf ("execbrc (1): pid: %d\n", getpid ());
  printf ("\tpattern: %s\n", to_char (p));
  printf ("\tfile name: %s\n", to_char (s));
  printf ("\trestbuf: %s\n", to_char (restbuf));
#endif

	for (lm = restbuf; *p != '{'; *lm++ = *p++)
		continue;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values:
*/
  printf ("execbrc (2): pid: %d, After getting to the {, p: %s\n", getpid (), 
          to_char (p));
  printf ("\tlm: %s\n", to_char (lm));
#endif

	for (pe = ++p; *pe; pe++)
	switch (*pe) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev == 0)
			goto pend;
		brclev--;
		continue;

	case '[':
		level = 1;
		for (pe++; *pe; pe++) {
			if (*pe == ']')
				level--;
				if (level == 0)
					break;
			else if (*pe == '[')
				level++;
		}
		if (!*pe)
			error((catgets(nlmsg_fd,NL_SETN,2, "Missing ]")));
		continue;
	}
pend:
	if (brclev || !*pe)
		error((catgets(nlmsg_fd,NL_SETN,3, "Missing }")));
	for (pl = pm = p; pm <= pe; pm++)
	switch (*pm & (QUOTE|TRIM)) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev) {
			brclev--;
			continue;
		}
		goto doit;

	case ','|QUOTE:
	case ',':
		if (brclev)
			continue;
doit:
		savec = *pm;
		*pm = 0;
		Strcpy(lm, pl);
		Strcat(restbuf, pe + 1);
		*pm = savec;

/*  If this is the first call from expand, s is in fact NULL.  This means
    that the first glob character seen was a {.
*/

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("execbrc (3): pid: %d, Original string: %s\n", getpid (),
	  to_char (s));
  printf ("\trestbuf: %s (s=NULL - call expand, else call amatch)\n", 
	  to_char (restbuf));
#endif
		if (s == 0) {
			sgpathp = gpathp;
			expand(restbuf);
			gpathp = sgpathp;
			*gpathp = 0;
		} else if (amatch(s, restbuf))
			return (1);
		sort();
		pl = pm + 1;
		continue;

	case '[':
		for (pm++; *pm; pm++) {
			if (*pm == ']')
				level--;
				if (level == 0)
					break;
			else if (*pm == '[')
				level++;
		}
		if (!*pm)
			error((catgets(nlmsg_fd,NL_SETN,4, "Missing ]")));
		continue;
	}
	return (0);
}

/*  Called by:
	matchdir ()
*/
/**********************************************************************/
match(s, p)
	CHAR *s, *p;
/**********************************************************************/
{
	register int c;
	register CHAR *sentp;
	CHAR sglobbed = globbed;

	if (*s == '.' && *p != '.')
		return (0);
	sentp = entp;
	entp = s;

/*  Set up entp to be the full string if it is needed in amatch and then
    call amatch with the string and the pattern.
*/
	c = amatch(s, p);
	entp = sentp;
	globbed = sglobbed;
	return (c);
}

unsigned char lastp[1024];

/*  Called by:
	execbrc ()
	match ()
	amatch ()
*/
/**********************************************************************/
amatch(s, p)
	register CHAR *s, *p;
/**********************************************************************/
{
	register int scc;
	CHAR *sgpathp;
	struct stat stb;
	int c, cc;
	CHAR *bstart, save;
	int level = 0;
	int plusFlag;

/*  Added to save memory for freeing later in the case of '*'.  The string
    that will be saved is part of a pathname.  Hence, it cannot have a length
    greater than MAXPATHLEN.
*/
	CHAR sSaver1 [MAXPATHLEN + 1];
	CHAR sSaver2 [MAXPATHLEN + 1];

/*  Added for [..] CDF expansion.
*/
    char bracketS [MAXPATHLEN + 1];
    char bracketPlusPath [MAXPATHLEN + 1];
    struct stat bracketPlusStat;

/*  Added for + CDF expansion.
*/
    char plusPath [MAXPATHLEN + 1];
    struct stat plusStat;
    char *pathPtr;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (1): pid: %d\n", getpid ());
  printf ("\tfile: %s, ", to_char (s));
  printf ("pattern: %s\n", to_char(p));
  printf ("next character in pattern: %c\n", *p);
#endif

/*  For CDFs, one last time through this routine is made when the string
    is NULL, but there might be a + in the pattern provided the last thing
    in the pattern was a *.  Check to see if p is a glob character (`{[*?) or
    a +, and it the file name string is NULL.  If not, return a "no match".
*/

  if (!*s && (*p != '+') && (*p != '{') && (*p != '[') && (*p != '*')
      && (*p != '?') && (*p != '`'))
    {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (2): pid: %d\n", getpid ());
  printf ("\tNULL string and pattern was not a glob character or +\n");
#endif
      return 0;
    }

	globbed = 1;
	for (;;) {

/*  scc is a single character in the string which is to be matched.
*/
		scc = *s++ & TRIM;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (3): pid: %d, string character (scc): %c\n", getpid (), scc);
  printf ("\ts: %s\n", to_char (s));
#endif 

		switch (c = *p++) {

		case '{':
			return (execbrc(p - 1, s - 1));

		case '[':

			/* find the matching ']', bstart points to	*/
			/* the beginning of the "[...]" string.		*/
			s--;
			bstart = (p-1);

/*  Initialize the plusFlag to FALSE.  If a '+' is found in the brackets, the
    flag will be set to TRUE.  This flag will then be used to decide if the
    file name should be tested as a CDF or not.  The flag must be re-initialized
    to FALSE each time an open bracket is seen.
*/
                        plusFlag = 0;

			while (cc = *p++) {
				if (cc == ']') 
				  {
					if (level == 0) 
					  {
						save = *p;
						*p = '\0';
						break;
					  } 
					else
						level--;
				  } 
				else if (cc == '[')
					level++;
				else if (cc == '+')
				  plusFlag = 1;
			}
			if (cc == 0)
			  error((catgets(nlmsg_fd,NL_SETN,5, "Missing ]")));
			if (strcmp(lastp, to_char(bstart))) {	/* different */
				strcpy(lastp, to_char(bstart));
			} 

#ifdef FNMATCH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (4): pid: %d, Calling fnmatch with lastp: %s, s: %s, f: %d\n",
          getpid (), lastp, to_char (s), _FNM_UAE);

#ifdef FNMATCH_DEBUG_2
  {
    char *debugS;

    printf ("\tlastp (bstart = *CHAR) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho %ho %ho %ho\n", *bstart, *(bstart + 1), 
	    *(bstart + 2), *(bstart + 3), *(bstart + 4), *(bstart + 5),
	    *(bstart + 6));
    
    printf ("\ts (*CHAR) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho %ho %ho %ho\n", *s, *(s + 1), *(s + 2), 
	    *(s + 3), *(s + 4), *(s + 5), *(s + 6));
    
    printf ("\tlastp (*char) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho %ho %ho %ho\n", (*lastp & 0377), 
	    (*(lastp + 1) & 0377), (*(lastp + 2) & 0377), 
	    (*(lastp + 3) & 0377), (*(lastp + 4) & 0377),
	    (*(lastp + 5) & 0377), (*(lastp + 6) & 0377));

    debugS = to_char (s);
    printf ("\tto_char(s) (*char) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho %ho %ho %ho\n", (*debugS & 0377), 
	    (*(debugS + 1) & 0377), (*(debugS + 2) & 0377), 
	    (*(debugS + 3) & 0377), (*(debugS + 4) & 0377),
	    (*(debugS + 5) & 0377), (*(debugS + 6) & 0377));
  }
#endif
#endif
			switch (fnmatch(lastp,to_char(s),_FNM_UAE)) {
				case 0:		/* match */
#ifdef FNMATCH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (4.5): pid: %d, fnmatch returned 0, save: %c\n", getpid (),
	  save);
  printf ("\t_uae_index: %c\n", *_uae_index);
#endif
					*p=save;
					s = to_short(_uae_index);
#ifdef FNMATCH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (4.6): pid: %d, s: %c\n", getpid (), *s);
#endif
					break;

/*  If no match, and a '+' was in the bracket expression, try with a + on the 
    file name.  Since there can be many characters in no particular order in 
    the brackets, we can't easily check to see if the last character in the 
    pattern is a +.  So the flag that was set as the bracket characters were
    searched for a closing bracket is used.  If this flag is not set, then a
    '+' was not seen before the closing bracket and there is no point in trying
    a match against the file name as a CDF.  If there was a plus in the bracket
    expression, the match is re-tried with the file name as a CDF.  If the match
    works withe the file name as a CDF, then try to stat the directory and
    see if it is a CDF.  If so, check the character in the pattern directly
    following the closing bracket ([...]).  It must be 0 (no more characters
    in the pattern) or a /.  If either of these cases is true, add the file
    name onto gpath and then onto gargv.  If there are more characters in
    the pattern following the /, call expand with that new pattern.  If gargv
    has been augmented, return 0 so that matchdir won't do the same thing.
    If any of these conditions is not true, set the pattern to the first
    character after the closing bracket and return 0.
*/
				case 1:		/* no match */
#ifdef FNMATCH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (4.75): pid: %d, fnmatch returned 1.\n", getpid ());
#endif
				  if (!plusFlag)
				    {
				      *p = save;
				      return 0;
				    }
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (5): pid: %d, Ready to call fnmatch with a +.\n", getpid ());
#endif

/*  Copy the file name into another variable and append a +.  Copy the full
    pathname to this file and append a + onto it too.  The first variable will
    be used for pattern matching and the second during a stat to see if the
    file is a CDF provided the pattern matches.
*/
    strcpy (bracketS, to_char (s));
    strcat (bracketS, "+");

    strcpy (bracketPlusPath, to_char (gpath));
    strcat (bracketPlusPath, to_char (entp));
    strcat (bracketPlusPath, "+/");
    
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (6): pid: %d\n", getpid ());
  printf ("\tCalling fnmatch with %s and %s\n", lastp, bracketS);
  printf ("\tbracketPlusPath: %s\n", bracketPlusPath);
#endif

    switch (fnmatch (lastp, bracketS, _FNM_UAE))
      {

/*  Pattern matched, so see if the file is a CDF, and if the next character
    in the pattern is 0 or a /.
*/
	case 0:

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (7): pid: %d, Got a match with a +.\n", getpid ());
  printf ("\tIndex returned from fnmatch: %u: %c\n", (unsigned) _uae_index,
          *_uae_index);
#endif
		if ((stat (bracketPlusPath, &bracketPlusStat) == 0)
		     && ((bracketPlusStat.st_mode & S_IFMT) == S_IFDIR)
		     && (bracketPlusStat.st_mode & S_ISUID))
                  {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (8): pid: %d, Directory was a CDF, *p:%c, save: %s\n", 
          getpid (), *p, to_char (save));
#endif

/*  Restore the pattern string to the first character after the closing
    bracket.  
*/
		    *p = save;

/*  If the file is a CDF, the pattern must have no more characters or a /
    or a * as its next character.  If it is a *, these will be eaten up.
    The character after all *'s must be a / or no character.  This allows
    things like +**.../ and +**...  Note that +*...x should not work since
    the + in this case could not denote a CDF.
*/
		    if (!*p || (*p == '/') || (*p == '*'))
		      {
			while (*p == '*')
			  p ++;

			if (*p && (*p != '/'))
			  return 0;
			  
			s = entp;
			sgpathp = gpathp;

/*  fnmatch returns a pointer to the first character in the string that
    matches the pattern.  In the case of + and a CDF, this can only occur
    if there are no more characters in the pattern or if the next character
    is a /.  In either case, the entire string must have matched.  So we
    need to copy the entire file name into gpath.
*/
			while (*s)
			  addpath (*s++);

			addpath ('+');

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (9): pid: %d, Gpath: %s.\n", getpid (), to_char (gpath));
#endif

/*  If the next character in the pattern is 0, or the one after the / is 0,
    this path needs to be put together in gpath and then appended to gargv.
    If the pattern does have a / as the next character, this needs to be
    added to gpath before it is added to gargv.
*/
			if ((*p == 0) || (*(p + 1) == 0))
			  {
			    if (*p == '/')
			      addpath ('/');

			    Gcat (gpath, nullstr);
			    globcnt ++;
			  }

/*  We get here if the next character in the pattern was a / and the one after
    that was NOT 0.  In this case, gpath needs to have a / added onto it and
    the expansion continued through a call to expand.
*/
			else
			  {
			    p++;
			    addpath ('/');
			    expand (p);
			  }

		        gpathp = sgpathp;
		        *gpathp = 0;

/*  In either case, return 0 so that matchdir doesn't add the file name to 
    gargv again.
*/
		        return 0;
		      }
                    
/*  Next character in the pattern wasn't NULL or a /.  Since the pattern was
    restored to the character after the closing bracket before this test, all
    that needs to happen is to return a 0.
*/
		    else
		      return 0;
                  }
	
/*  Matching pattern wasn't a CDF.  Here the pattern needs to be restored to
    be the character after the closing bracket before 0 is returned.
*/
		else
		  {
		    *p = save;
		    return 0;
		  }

/*  No match with a + on the end of the file name, so restore the pattern
    to the character after the closing bracket and return 0.
*/
	case 1:

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (10): pid: %d, No match with a +.\n", getpid ());
#endif
		*p = save;
		return 0;
         
        default:
		error ((catgets (nlmsg_fd, NL_SETN, 14, "Invalid pattern")));
      }  /* End of the switch statement testing matching a + on the file name */

/*  Back to the first switch statement for the file name without a +.
*/
				default:	/* error in pattern */
#ifdef FNMATCH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("amatch (10.5): pid: %d, fnmatch returned something else.\n", 
	   getpid ());
#endif
					error((catgets(nlmsg_fd,NL_SETN,14, 
					       "Invalid pattern")));

			}  /* End of first switch for [..] */

			continue;  

		case '*':

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (11): pid: %d, Found a *\n", getpid ());
#endif 

/*  If there are no more characters in the pattern then there is a match.
*/
			if (!*p)
				return (1);
			if (*p == '/') {
				p++;
				goto slash;
			}

#ifdef TRACE_DEBUG
  printf ("amatch (11.1): pid: %d,before loop: s: %s\n", getpid (), 
	  to_char (s));
#endif 


/*  If there are more characters in the pattern, go back to the previous
    character in the string, and try matching it with the pattern.  If there
    is a match then the call to amatch will continue matching the rest of the
    string till it indicates an entire string match or a character doesn't
    match.  If the whole string matches, then return a match.  If something
    didn't match, go to the next character in the string and try again.
    Keep this up until there are no more characters in the string.  Note that
    the result is that the return from the amatch call will be propagated up
    if it is 1.  
    
    Before the additions to understand CDFs, if there was a match that 
    augmented gargv but did return 0, the rest of the string would still be 
    tested.  Seems wasteful, but this is the way the code worked if a / was 
    seen.  
    
    This has been changed since the addition of CDF support since if the string
    is NULL we are calling amatch once more, and it may have matched on the
    previous call.  To test this, the counter of items matched is tested
    against its previous value.  This variable globcnt gets incremented each
    time a string is added to gargv (each call to Gcat has an accompanying
    increment of globcnt).  If the value is the same and amatch returned 0
    then the loop is continued.  But if the value is different and a 0 was
    returned then a match did occur so this loop can be terminated with a 
    return.

    In response to a bug, this loop has been changed so that the value of 
    the string 's' is saved before the call to amatch, and restored after it
    if no match occurred.  It turned out that in the 7.0 version, the string
    's' was actually incremented somehow during the recursive call to 
    'amatch'.  The result was that one or more characters of the string were
    skipped on the next iteration of the loop.  For example, in matching
    '[a-z]*[a-z]' with 'abc', the 'a' matched the first '[a-z]', then 'bc' was
    used in the recursive all with the pattern being '[a-z]'.  When this
    failed, the value of 's' after the call was 'c', so the next time through
    this loop there was nothing in the string 's'.  The fix is to save the 
    value of 's' before the call, then restore it after the call.

    The saving/restoration is done using 2 buffers.  First, the value of 's'
    is copied into the first buffer.  It is then copied into the second, and
    's' is set to point to the first.  After the call, 's' is restored to the
    second buffer, then for the next iteration its new value is copied into
    the first buffer.  Two buffers ended up being used because the 'Strcpy'
    routine seemed to have problems copying from inside one buffer to itself.
    The buffers are MAXPATHLEN + 1 in length.  This should be enough since
    this routine is called for globbing on a single path component.
*/
			for (s--; *s; s++)
                           {
                             int oldGlobcnt;

                             oldGlobcnt = globcnt;
			     Strcpy (sSaver1, s);
			     Strcpy (sSaver2, sSaver1);
			     s = sSaver1;

#ifdef TRACE_DEBUG
  printf ("amatch (11.25): %d, calling amatch with string: %s\n", getpid (), 
	  to_char (s));
  printf ("\tpattern: %s\n", to_char (p));
  printf ("\tsSaver1: %s\n", to_char (sSaver1));
  printf ("\tsSaver2: %s\n", to_char (sSaver2));
  printf ("\tvalue of s: %d, sSaver1: %d, sSaver2: %d\n", s, sSaver1, sSaver2);
#endif

				if (amatch(s, p))
				  {

#ifdef TRACE_DEBUG
  printf ("amatch (11.5): %d, amatch returned 1\n", getpid ());
#endif
					return (1);
				  }
				
				else if (globcnt > oldGlobcnt)
				  return 0;

				else
				  {
#ifdef TRACE_DEBUG
  printf ("amatch (11.75): %d, amatch returned 0, s: %s", getpid (),
	  to_char (s));
  printf (", p: %s", to_char (p));
  printf (", sSaver1: %s\n", to_char (sSaver1));
  printf (", sSaver2: %s\n", to_char (sSaver2));
  printf ("\tvalue of s: %d, sSaver1: %d, sSaver2: %d\n", s, sSaver1, sSaver2);
#endif

				    s = sSaver2;
#ifdef TRACE_DEBUG
  printf ("amatch (11.9): %d, restored s: %s\n", getpid (), to_char (s));
  printf ("\tvalue of s: %d, sSaver1: %d, sSaver2: %d\n", s, sSaver1, sSaver2);
#endif
				  }
                           }

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (12): pid: %d, Fell out of * loop; s: %s\n", getpid (),
          to_char (s));
  printf ("\tp: %s\n", to_char (p));
#endif

/*  Even though we fell out, try one more time with a NULL string in case there
    is a + in the pattern which might match a CDF.  This is needed for strings
    like ...*{+}, where the pattern at this point is {+} but the string is 
    NULL.  Another call to amatch will cause a call to execbrc which will then
    call amatch (NULL, +) which will check for a CDF.

    In all cases, return what amatch returns.  Note that we only get to here
    if the string doesn't match in the previous for loop.
*/
  return (amatch (s, p));

		case 0:

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (12.5): pid: %d, NULL pattern, string: %c\n", getpid (), scc);
#endif 
			return (scc == 0);

		default:

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (13): pid: %d, default: pattern character (c):%c\n", 
	  getpid (), c);
#endif 

			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

/*  This is where strings like ...{+} will come after being expanded to ...+
    by execbrc.  This code tests whether the file name has a + in it, or if
    it is NULL, whether it is a CDF.
*/
		case '+':
			if ((c &TRIM) != scc)
			  {
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (14): pid: %d, + in pattern, not in string: %s\n", getpid (),
          to_char (s));
#endif 

/*  More characters in the file name, but no match with a +, so no match
    period.
*/
                            if (scc)
			      {
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (15): pid: %d, scc not NULL: %c\n", getpid (), scc);
#endif 
                                return 0;
			      }

/*  No more characters in the file name, check for a CDF.
*/
			    else 
			      {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (16): pid: %d, No more characters in string\n", getpid ());
  printf ("\tgpath: %s, ", to_char (gpath));
  printf ("entp: %s\n", to_char (entp));
  printf ("p: %s\n", to_char (p));
#endif 

/*  If the file is a CDF, the pattern must have no more characters or a /
    or a * as its next character.  If it is a *, these will be eaten up.
    The character after all *'s must be a / or no character.  This allows
    things like +**.../ and +**...  Note that +*...x should not work since
    the + in this case could not denote a CDF.
*/
			        if (!*p || (*p == '/') || (*p == '*'))
				  {
			            while (*p == '*')
			              p ++;
            
			            if (*p && (*p != '/'))
			              return 0;

                                    strcpy (plusPath, to_char (gpath));
			            strcat (plusPath, to_char (entp));
                                    pathPtr = plusPath + strlen (plusPath);
			            strcpy (pathPtr, "+/");


#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (17): pid: %d, Path to stat: %s\n", getpid (), plusPath);
#endif 
				    if ((stat (plusPath, &plusStat) == 0) 
				       && ((plusStat.st_mode & S_IFMT) == S_IFDIR)
				       && (plusStat.st_mode & S_ISUID))
				      {

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace execution.
*/
  printf ("amatch (18): pid: %d, Directory is a CDF\n", getpid ());
#endif 

/*  CDF matched, so add the file name plus + to gpath.  If there are no
    more characters in the pattern, add gpath onto gargv.  Otherwise,
    there is a / next in the pattern (we checked for NULL or / in the 
    pattern after the + above).  So add the / to gpath and do the same
    check for more characters in the pattern.  If there aren't any, add
    gpath onto gargv.  If there are, then call expand with the new pattern.
    In any case, when we fall out of this condition, return a 0 to match,
    which will be returned to matchdir so that it doesn't re-add gpath to 
    gargv.
*/
                                        s = entp;
				        sgpathp = gpathp;
				        while (*s)
					  addpath (*s ++);
                                      
				        addpath ('+');

				        if ((*p == 0) || (*(p + 1) == 0))
					  {
					    if (*p == '/')
					      addpath ('/');

					    Gcat (gpath, nullstr);
					    globcnt ++;
#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;
    traceGargv = gargv;
    printf ("amatch (19): pid: %d, gargc: %d\n", getpid (), gargc);
    while (*traceGargv)  
      {
	printf ("\tGargv: %s\n", to_char (*traceGargv));
	traceGargv ++;
      }
  }
#endif
					  }

/*  We get here if the next character in the pattern was a / and the one after
    that was NOT 0.  In this case, gpath needs to have a / added onto it and
    the expansion continued through a call to expand.
*/
				        else
					  {
					    p++;
					    addpath ('/');
					    expand (p);
					  }
					  
					gpathp = sgpathp;
					*gpathp = 0;

/*  In either case, return 0 so that matchdir doesn't add the file name to 
    gargv again.
*/
				        return 0;
				      }
				    
/*  The stat failed to match a CDF.
*/
				    else
				      return 0;
				  }
				
/*  The next character in the pattern wasn't NULL or a /.
*/
				else
				  return 0;
			      }  /*  End of no more characters in file name */
			  }  /* End of character in file name was not a + */

/*  Here the character in the file name was a +, so the string and pattern
    match so far; just continue.
*/
			continue;

		case '/':

/*  If there are more characters in the string, this doesn't match.
*/
			if (scc)
				return (0);

/*  Otherwise, restore the initial string (saved in match) and add this on to
    gpath.  Then add a '/'.  Next stat the result.  If it can be stat'ed, and
    is a directory, then this directory should be searched.  If there are no
    more characters in the pattern, then the search is done.  Otherwise, call
    expand all over again, starting from the new directory.
*/

slash:
			s = entp;
			sgpathp = gpathp;
			while (*s)
				addpath(*s++);
			addpath('/');
			if (stat(to_char(gpath), &stb) == 0 && isdir(stb))
				if (*p == 0) {
					Gcat(gpath, nullstr);
					globcnt++;

#ifdef TRACE_DEBUG
/*  Debug:  Used to trace values.
*/
  {
    CHAR **traceGargv;
    traceGargv = gargv;
    printf ("amatch (20): pid: %d, gargc: %d\n", getpid (), gargc);
    while (*traceGargv)  
      {
	printf ("\tGargv: %s\n", to_char (*traceGargv));
	traceGargv ++;
      }
  }
#endif
				} else
					expand(p);
			gpathp = sgpathp;
			*gpathp = 0;
			return (0);
		}
	}
}

/*  Called by:
	exp2c ()
	search ()
	Gmatch ()
	madrof ()
*/
/**********************************************************************/
Gmatch(s, p)
	register CHAR *s, *p;
/**********************************************************************/
{
	register int scc;
	int c, cc;
	CHAR *bstart, save;
	int level = 0;

	for (;;) {
		scc = *s++ & TRIM;
		switch (c = *p++) {

		case '[':

			/* find the matching ']', bstart points to	*/
			/* the beginning of the "[...]" string.		*/
			s--;
			bstart = (p-1);
			while (cc = *p++) {
				if (cc == ']') {
					if (level == 0) {
						save = *p;
						*p = '\0';
						break;
					} else
						level--;
				} else if (cc == '[')
					level++;
			}
			if (cc == 0)
				error((catgets(nlmsg_fd,NL_SETN,5, "Missing ]")));
			if (strcmp(lastp, to_char(bstart))) {	/* different */
				strcpy(lastp, to_char(bstart));
			}

#ifdef FNMATCH_DEBUG
  printf ("Gmatch (1): pid: %d, Calling fnmatch with lastp: %s, s: %s, f:%d\n",
	   getpid (), lastp, to_char (s), _FNM_UAE);

#ifdef FNMATCH_DEBUG_2
  {
    char *debugS;

    printf ("\tlastp (bstart = *CHAR) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho\n", *bstart, *(bstart + 1), *(bstart + 2), 
		*(bstart + 3));
    
    printf ("\ts (*CHAR) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho\n", *s, *(s + 1), *(s + 2), *(s + 3));
    
    printf ("\tlastp (*char) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho\n", (*lastp & 0377), (*(lastp + 1) & 0377), 
	    (*(lastp + 2) & 0377), (*(lastp + 3) & 0377));

    debugS = to_char (s);
    printf ("\tto_char(s) (*char) as unsigned shorts:\n");
    printf ("\t\t%ho %ho %ho %ho\n", (*debugS & 0377), (*(debugS + 1) & 0377), 
	    (*(debugS + 2) & 0377), (*(debugS + 3) & 0377));
  }
#endif
#endif
			switch (fnmatch(lastp,to_char(s),_FNM_UAE)) {
				case 0:		/* match */

#ifdef FNMATCH_DEBUG
  printf ("Gmatch (2): pid: %d, fnmatch returned 0\n", getpid ());
#endif
					*p=save;
					s = to_short(_uae_index);
					break;
				case 1:		/* no match */

#ifdef FNMATCH_DEBUG
  printf ("Gmatch (3): pid: %d, fnmatch returned 1\n", getpid ());
#endif
					*p=save;
					return(0);
				default:	/* error in pattern */

#ifdef FNMATCH_DEBUG
  printf ("Gmatch (4): pid: %d, fnmatch returned something else\n", getpid ());
#endif
					error((catgets(nlmsg_fd,NL_SETN,14, "Invalid pattern")));
			}
			continue;

		case '*':
			if (!*p)
				return (1);
			for (s--; *s; s++)
				if (Gmatch(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		}
	}
}

/*  Called by:
	Dword ()
	collect ()
	acollect ()
	expand ()
	matchdir ()
	amatch ()
*/
/*  Purpose:  Store the concatenation of two string in gargv.
*/
/**********************************************************************/
Gcat(s1, s2)
	register CHAR *s1, *s2;
/**********************************************************************/
{
/*  Note that gargc is incremented before use each time through this routine.
*/
	gnleft -= Strlen(s1) + Strlen(s2) + 1;
	if (gnleft <= 0 || ++gargc >= GAVSIZ)
		error((catgets(nlmsg_fd,NL_SETN,7, "Arguments too long")));
	gargv[gargc] = 0;

/*  Concatenate s2 onto s1 and strore in gargv [gargc].
*/
	gargv[gargc - 1] = Strspl(s1, s2);
}

/*  Called by:
	expand ()
	amatch ()
*/
/*  Purpose:  Add a character to gpathp and keep it NULL terminated.
*/
/**********************************************************************/
addpath(c)
	CHAR c;
/**********************************************************************/
{

#ifdef ADDPATH_DEBUG
/*  Debug:  Used to trace values.
*/
  printf ("addpath (1): pid: %d, Character: %c\n", getpid (), c);
  printf ("\tgpathp: %hu, lastgpathp: %hu\n", gpathp, lastgpathp);
#endif

	if (gpathp >= lastgpathp)
		error((catgets(nlmsg_fd,NL_SETN,8, "Pathname too long")));
	*gpathp++ = c;
	*gpathp = 0;
}

/*  Called by:
	Dfix ()
	heredoc ()
	doexec ()
	doforeach ()
	echo ()
	doeval ()
	globone ()
	set1 ()
*/
/*  Purpose:  Check each string in string array for ~, {, {}.  If ~, then
	      modify gflag.  If NOT { or {} then run function on each
	      character in the string.
*/
/**********************************************************************/
rscan(t, f)
	register CHAR **t;
	int (*f)();
/**********************************************************************/
{
	register CHAR *p, c;

/*  Check each string in t for ~, {, {} and skip the character if it is a { or
    {}.  Set gflag if the character is a ~.
*/
	while (p = *t++) {
		if (f == tglob)
			if (*p == '~')
				gflag |= 2;
			else if (eq(p, "{") || eq(p, "{}"))
				continue;

/*  For each character in the string, do the function on that character.
*/
		while (c = *p++)
			(*f)(c);
	}
}

/*  Called by:
	heredoc ()
	doexec ()
	echo ()
	setenv ()
	doeval ()
	globone ()
	setq ()
*/
/**********************************************************************/
scan(t, f)
	register CHAR **t;
	int (*f)();
/**********************************************************************/
{
	register CHAR *p, c;

	while (p = *t++)
		while (c = *p)
			*p++ = (*f)(c);
}

/*  Called by:
	setenv ()
    This is the same as scan, except that only one string is checked,
    rather that an array of them.
*/
/**********************************************************************/
scan1(t, f)
	register CHAR *t;
	int (*f)();
/**********************************************************************/
{
	register CHAR *p = t, c;

	while (c = *p)
		*p++ = (*f)(c);
}

/*  Called by:
	set1 () via rscan ()
	doexec () via rscan ()
	doforeach () via rscan ()
	echo () via rscan ()
	doeval () via rscan ()
	globone () via rscan ()
*/
/*  Purpose:  Modify gflag if the character is a glob character.
*/
/**********************************************************************/
tglob(c)
	register CHAR c;
/**********************************************************************/
{
/*  globchars is a global (`[{*?).  If c is { then OR gflag with 2.
    Otherwise OR gflag with 1.
*/
	if (any(c, globchars))
		gflag |= c == '{' ? 2 : 1;
	return (c);
}

/*  Called by:
	heredoc () via scan ()
	doexec () via scan ()
	echo () via scan ()
	setenv () via scan ()
	doeval () via scan ()
	globone () via scan ()
	setq () via scan ()
*/
/**********************************************************************/
trim(c)
	CHAR c;
/**********************************************************************/
{

	return (c & TRIM);
}

/*  Called by:
	dosource ()
	dfollow ()
	doexec ()
	exp6 ()
	dogoto ()
	doswitch ()
	dosetenv ()
	pkill ()
	doio ()
	asx ()
*/
/**********************************************************************/
CHAR *
globone(str)
	register CHAR *str;
/**********************************************************************/
{
	CHAR *gv[2];
	register CHAR **gvp;
	register CHAR *cp;

	gv[0] = str;
	gv[1] = 0;
	gflag = 0;

/*  Both rscan and tglob set gflag if globbing characters are seen.
*/
	rscan(gv, tglob);
	if (gflag) {

/*  Glob returns a pointer to gargv after it has been copied to calloc'd
    space.
*/
		gvp = glob(gv);
		if (gvp == 0) {
			setname(to_char(str));
			bferr((catgets(nlmsg_fd,NL_SETN,9, "No match")));
		}
/*  Set up a pointer to the first match, and increment the match array pointer.
    If *gvp is not NULL, then there was more than one match.
*/
		cp = *gvp++;
		if (cp == 0)
			cp = nullstr;

/*  If there was more than one match, generate an error message.
*/
		else if (*gvp) {
			setname(to_char(str));
			bferr((catgets(nlmsg_fd,NL_SETN,10, "Ambiguous")));
		} else
			cp = strip(cp);
/*
		if (cp == 0 || *gvp) {
			setname(str);
			bferr(cp ? "Ambiguous" : "No output");
		}
*/

/*  Free the memory calloc'd in glob for the array of matches.

    It sure looks like whatever cp was pointing to is freed by this routine!
*/
		xfree((CHAR *)gargv); gargv = 0;
	} 

/*  If no globbing took place, calloc space for the string and save it.
*/
      else 
	{
		scan(gv, trim);
		cp = savestr(gv[0]);
	}

      return (cp);
}

/*
 * Command substitute cp.  If literal, then this is
 * a substitution from a << redirection, and so we should
 * not crunch blanks and tabs, separating words only at newlines.
 */
/*  Called by:
	collect ()
	heredoc ()
*/
/**********************************************************************/
CHAR **
dobackp(cp, literal)
	CHAR *cp;
	bool literal;
/**********************************************************************/
{
/*  apargv, pargv, pargcp, pargs, pargc, pnleft are all globals.
*/
	register CHAR *lp, *rp;
	CHAR *ep;
	CHAR word[WRDSIZ];
	CHAR *apargv[GAVSIZ + 2];

	if (pargv) {
		abort();
		blkfree(pargv);
	}
	pargv = apargv;
	pargv[0] = NOSTR;
	pargcp = pargs = word;
	pargc = 0;
	pnleft = WRDSIZ - 4;
	for (;;) {
		for (lp = cp; *lp != '`'; lp++) {
			if (*lp == 0) {
				if (pargcp != pargs)
					pword();
#ifdef GDEBUG
				printf("leaving dobackp\n");
#endif
/*  Copy pointers to malloc space and return a ponter to it.
*/
				return (pargv = copyblk(pargv));
			}
/*  Copy a character onto pargcp.
*/
			psave(*lp);
		}
/*  Move to the first character after the `.
*/
		lp++;
		for (rp = lp; *rp && *rp != '`'; rp++)

/*  If the character is a backslash, move to the next character.
*/
			if (*rp == '\\') {
				rp++;

/*  If the end of the string is found, then this is an error.
*/
				if (!*rp)
					goto oops;
			}
		if (!*rp)
oops:
			error((catgets(nlmsg_fd,NL_SETN,11, "Unmatched `")));

/*  Copy the string into the malloc area.  (ep is the string between the
    backquotes.)  Replace the second backquote with NULL
*/
		ep = savestr(lp);
		ep[rp - lp] = 0;

/*  Execute the command in backquotes.
*/
		backeval(ep, literal);
#ifdef GDEBUG
		printf("back from backeval\n");
#endif

/*  Move the string past the second backquote.
*/
		cp = rp + 1;
	}
}

#ifndef NONLS
CHAR CH_fakecom0[] = {'`',' ','.','.','.',' ','`',0};
#else
#define CH_fakecom0	"` ... `"
#endif

/*  Called by:
	dobackp ()
*/
/**********************************************************************/
backeval(cp, literal)
	CHAR *cp;
	bool literal;
/**********************************************************************/
{
	int pvec[2];
	int quoted = (literal || (cp[0] & QUOTE)) ? QUOTE : 0;
	unsigned char ibuf[WRDSIZ];	/* NLS: changed to unsigned char */
	register int icnt = 0, c;
	register unsigned char *ip;	/* NLS: changed to unsigned char */
	bool hadnl = 0;
	CHAR *fakecom[2];
	struct command faket;

#ifdef DEBUG_BACKEVAL
  {
    CHAR *debugPtr;

    debugPtr = cp;
    printf ("backeval (1): %d, string: %s\n\t", getpid (), to_char (cp));
    
    while (*debugPtr != 0)
      printf ("\t%lo\n", *debugPtr ++);
  }
#endif

	faket.t_dtyp = TCOM;
	faket.t_dflg = 0;
	faket.t_dlef = 0;
	faket.t_drit = 0;
	faket.t_dspr = 0;
	faket.t_dcom = fakecom;
	fakecom[0] = CH_fakecom0;
	fakecom[1] = 0;
	/*
	 * We do the psave job to temporarily change the current job
	 * so that the following fork is considered a separate job.
	 * This is so that when backquotes are used in a
	 * builtin function that calls glob the "current job" is not corrupted.
	 * We only need one level of pushed jobs as long as we are sure to
	 * fork here.
	 */
	psavejob();
	/*
	 * It would be nicer if we could integrate this redirection more
	 * with the routines in sh.sem.c by doing a fake execute on a builtin
	 * function that was piped out.
	 */
	mypipe(pvec);
	if (pfork(&faket, -1) == 0) {
		struct wordent paraml;
		struct command *t;

		close(pvec[0]);
		(void) dmove(pvec[1], 1);
		(void) dmove(SHDIAG, 2);

		initdesc();

/*  Free strings in pargv in case we have nested backquotes */
                if (pargv)
                        blkfree(pargv), pargv = 0;

		arginp = cp;

/*  This loop unquoted all the characters in the backquote evaluation, including
    any quoted newlines.  Then when lex called word, which called getC which
    called readc on arginp, a single newline was seen.  This was passed back
    up to word, which quits as soon as it sees a newline.  
    
		while (*cp)
			*cp++ &= TRIM;

    This behavior was changed by making this loop only trim the character if it
    isn't a quoted newline.  In addition, the logic was changed so that the 
    TRIMming just occurs on the character, then the pointer is advanced for 
    both cases (TRIM and not TRIM).
*/
/*
    A further fix to the above:
    By the time we get to this point, the \ has been stripped off,
    so all we have is the quoted newline character.  When we
    call lex(), the quoted newline character is not recognized
    as anything special and gets treated as any "normal"
    character would.  Thus, it never gets removed from the input stream.
    We replace the quoted newline with a blank since that is the
    documented behavior.
*/
		while (*cp)
		  {
		     if (*cp != ('\n' | QUOTE))
		       *cp &= TRIM;
		     else
		       *cp = ' ';
		     
		     cp ++;
		  }

		(void) lex(&paraml);
		if (err)
			error(err);
		alias(&paraml);
		t = syntax(paraml.next, &paraml, 0);
		if (err)
			error(err);
		if (t)
			t->t_dflg |= FPAR;
		execute(t, -1);
		exitstat();
	}
	xfree(cp);
	close(pvec[1]);
	sighold(SIGCLD); /* hold SIGCLD until we have read everything */
			 /* from the childs pipe.                     */
	do {
		int cnt = 0;

		for (;;) {
			if (icnt == 0) {
				ip = ibuf;
				icnt = read(pvec[0], ip, WRDSIZ);
				if (icnt <= 0) {
					c = -1;
					break;
				}
			}
			if (hadnl)
				break;
			--icnt;
			c = (*ip++ & 0377);
#ifdef NLS16
			/* have to get a whole character */
			if (FIRSTof2(c) && SECof2(*ip)) { 
				--icnt;
				c = ((c << 8) | (*ip++ & 0377)) & TRIM;
			}
#endif
			if (c == 0)
				break;
			if (c == '\n') {
				/*
				 * Continue around the loop one
				 * more time, so that we can eat
				 * the last newline without terminating
				 * this word.
				 */
				hadnl = 1;
				continue;
			}
#ifndef NLS
			if (!quoted && (c == ' ' || c == '\t'))
#else
			if (!quoted && (c == ' ' || c == '\t' || c == ALT_SP))
#endif
				break;
			cnt++;
			psave(c | quoted);
		}
		/*
		 * Unless at end-of-file, we will form a new word
		 * here if there were characters in the word, or in
		 * any case when we take text literally.  If
		 * we didn't make empty words here when literal was
		 * set then we would lose blank lines.
		 */
		if (c != -1 && (cnt || literal))
			pword();
		hadnl = 0;
	} while (c >= 0);
	sigrelse(SIGCLD);
#ifdef GDEBUG
	printf("done in backeval, pvec: %d %d\n", pvec[0], pvec[1]);
	printf("also c = %c <%o>\n", c, c);
#endif
	close(pvec[0]);
	pwait();
	prestjob();
}

/*  Called by:
	dobackp ()
	backeval ()
	pword ()
*/
/*  Purpose:  Add a character to the end of pargcp.
*/
/**********************************************************************/
psave(c)
	CHAR c;
/**********************************************************************/
{
	if (--pnleft <= 0)
		error((catgets(nlmsg_fd,NL_SETN,12, "Word too long")));
	*pargcp++ = c;
}

/*  Called by:
	dobackp ()
	backeval ()
*/
/*  Purpose:  Copy pargs string to pargv array and set partcp to pargs 
	      (copied args to include this argument).
*/
/**********************************************************************/
pword()
/**********************************************************************/
{
/*  Terminate pargcp with a NULL.
*/
	psave(0);
	if (pargc == GAVSIZ)
		error((catgets(nlmsg_fd,NL_SETN,13, "Too many words from ``")));

/*  Copy the string into the malloc area.
*/
	pargv[pargc++] = savestr(pargs);
	pargv[pargc] = NOSTR;
#ifdef GDEBUG
	printf("got word %s\n", to_char(pargv[pargc-1]));
#endif
	pargcp = pargs;
	pnleft = WRDSIZ - 4;
}
