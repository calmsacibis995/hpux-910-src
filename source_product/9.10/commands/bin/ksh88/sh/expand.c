/* HPUX_ID: @(#) $Revision: 70.1 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/*
 *	File name expansion
 *
 *	David Korn
 *	AT&T Bell Laboratories
 *
 */

#include	"sh_config.h"
#ifdef KSHELL
#   include	"defs.h"
#else
#   include	<sys/stat.h>
#   include	<setjmp.h>
#   ifdef _unistd_
#	include	<unistd.h>
#   endif /* _unistd_ */
#endif /* KSHELL */
/* now for the directory reading routines */
#ifdef FS_3D
#   undef _ndir_
#   define _dirent_ 1
#endif /* FS_3D */
#ifdef _ndir_
#   undef	direct
#   define direct dirent
#   include	<ndir.h>
#else
#   undef	dirent
#   ifndef FS_3D
#	define dirent direct
#   endif /* FS_3D */
#   ifdef _dirent_
#	include	<dirent.h>
#   else
#	include	<sys/dir.h>
#	ifndef rewinddir	/* old system V */
#           define OLDSYS5 1
#	    define NDENTS	32
	    typedef struct 
	    {
		int		fd;
		struct direct	*next;
		struct direct	*last;
		struct direct	entries[NDENTS];
		char		extra;
		ino_t		save;
	    } DIR;
	    DIR *opendir();
	    struct direct *readdir();
#  	    define closedir(dir)	close(dir->fd)
#	endif /* rewinddir */
#   endif /* _dirent_ */
#endif


#ifdef KSHELL
#   define check_signal()	(sh.trapnote&SIGSET)
#    define argbegin	argnxt.cp
    extern char	*strrchr();
    int		path_expand();
    void	rm_files();
    int		f_complete();
    static	char	*sufstr;
    static	int	suflen;
#else
#   define check_signal()	(0)
#   define round(x,y)		(((int)(x)+(y)-1)&~((y)-1))
#   define stak_end(x)		(*(globptr()->last = (x))=0)
#   define sh_access		access
#   define suflen		0
    struct argnod
    {
	struct argnod	*argbegin;
	struct argnod	*argchn;
	char		argval[1];
    };
    static char		*sh_copy();
    static char		*stak_begin();
    static int		test_type();
#endif /* KSHELL */


/*
 * This routine builds a list of files that match a given pathname
 * Uses external routine strmatch() to match each component
 * A leading . must match explicitly
 *
 */

struct glob
{
	int		argn;
	char		**argv;
	int		flags;
	struct argnod	*rescan;
	struct argnod	*match;	
	DIR		*dirf;
#ifndef KSHELL
	char		*memlast;
	char		*last;
	struct argnod	*resume;
	jmp_buf		jmpbuf;
	char		begin[1];
#endif
};


#define GLOB_RESCAN 1
#define	argstart(ap)	((ap)->argbegin)
#define globptr()	((struct glob*)membase)

static struct glob	 *membase;

static void		addmatch();
static void		glob_dir();
extern int		strmatch();


int path_expand(pattern)
char *pattern;
{
	register struct argnod *ap;
	register struct glob *gp;
#ifdef KSHELL
	struct glob globdata;
	membase = &globdata;
#endif /* KSHELL */
	gp = globptr();
	ap = (struct argnod*)stak_alloc(strlen(pattern)+sizeof(struct argnod)+suflen);
	gp->rescan =  ap;
	gp->argn = 0;
#ifdef KSHELL
	gp->match = st.gchain;
#else
	gp->match = 0;
#endif /* KSHELL */
	ap->argbegin = ap->argval;
	ap->argchn = 0;
#ifdef KSHELL
	/*
	 *  we stak_alloced() ap->argval so it is our
	 *  growing string at the end of memory. So it 
	 *  doesn't matter if we copy past the end of it.
	 */
	pattern = sh_copy(pattern,ap->argval);
	if(suflen)
		sh_copy(sufstr,pattern);
#else
	sh_copy(pattern,ap->argval);
#endif /* KSHELL */
	suflen = 0;
	do
	{
		gp->rescan = ap->argchn;
		glob_dir(ap);
	}
	while(ap = gp->rescan);
#ifdef KSHELL
	st.gchain = gp->match;
#endif /* KSHELL */
	return(gp->argn);
}

static void glob_dir(ap)
struct argnod *ap;
{
	register char	*rescan;
	register char	*prefix;
	register char	*pat;
	DIR 		*dirf;
	char		quote = 0;
	char		savequote = 0;
	char		meta = 0;
	char		bracket = 0;
#if defined(DUX) || defined(DISKLESS)
	char		cdf_plus = 0;	/* flag a '+' in pattern */
	char		*cdf_plusp;	/* points to '+' in pattern */
#endif /* DUX */
	char		first;
	char		*dirname;
	struct dirent	*dirp;
	if(check_signal())
		return;
	pat = rescan = argstart(ap);
	prefix = dirname = ap->argval;
	first = (rescan == prefix);
	/* check for special chars */
	while(1) switch(*rescan++)
	{
		case 0:
			/*
			 *  End of the pattern
			 */
			rescan = 0;
			if(meta)
				goto process;
			if(first)
				return;
			if(quote)
				sh_trim(argstart(ap));
			if(sh_access(prefix,F_OK)==0)
				addmatch((char*)0,prefix,(char*)0);
			return;

		case '/':
			if(meta)
				goto process;
			pat = rescan;
			bracket = 0;
			savequote = quote;
			break;

		case '[':
			/*
			 *  A '[' is a meta character only if it is
			 *  followed by a ']', other wise it is a 
			 *  normal character.  We set bracket to 1
			 *  here and if we ever run across a ']', then
			 *  meta gets set to 1 below iff we have seen
			 *  the '['.
			 */
			bracket = 1;
			break;

		case ']':
			/*
			 *  should bracket be reset to 0 after this?
			 *  for the case of multiple '['?
			 */
			meta |= bracket;
			break;

#if defined(DUX) || defined(DISKLESS)
		case '+':
			/*
			 *  Check to see if the plus is a literal, or a possible CDF.
			 *  Only a '+' or '+*' at the end of a path component can be 
			 *  a CDF
			 *  
			 *  (cdf_plus == 1) <==>  a) there is a plus in the pattern
			 *		          b) the '+' is a possible CDF '+'
			 *                           (at end of path component or
			 *			     followed only by stars)
			 *  We want cdf_plusp to point to the '+' in the
			 *  pattern so that we don't have to search for it
			 *  later (we will need to look in the directory for
			 *  a filename w/o the '+').
			 *
			 *  When we are done, rescan should point to the next
			 *  character to match:
			 *  If pattern is:          *rescan should be:
			 *     ...file+/		'/'
			 *     ...file+*<slash>		'/'
			 *     ...file+***<slash>	'/'
			 *     ...file+c		'c'
			 *     ...file+*c		'*'
			 *     ...file+***c		<the last *>
			 *     ...fele+**c/		<the last *>
			 */

			
		   	switch(*rescan) {
			   case 0:     /* FALL THROUGH */
			   case '/':
			        /*
				 *  The simple cases:
				 *  ...file+  and  ...file+/
				 */
		                /*  save position of the '+' */
		   	        cdf_plusp=rescan;
			        cdf_plusp--;
			        cdf_plus=1;
				break;
		           case '*': 
			        /*  
				 *  The cases where '+' is followed by
				 *  one or more '*'s
				 */

				meta=1;
		                /*  save position of the '+' */
		   	        cdf_plusp=rescan;
			        cdf_plusp--;

			        /*
				 *  make rescan point to the first character
				 *  that is not a '*' after the '+'.  Multiple
				 *  stars are the same as one star.
				 */
				while (*rescan == '*')
				   rescan++;

				if (*rescan == 0 || *rescan == '/')
				        /*
					 *  We found something like:
					 *      ...file+**  or ...file+***<slash>...
					 *  so it is a good CDF '+'
					 */
					cdf_plus=1;
				else
				   /*
				    *  We found something like:
				    *     ...file+***x or ...file+***c/...
				    *
				    *  This is NOT a possible CDF '+' so 
				    *  reset rescan to point to character after
				    *  the '+'.  Since this is not a CDF '+', we
				    *  can throw away the value of cdf_plus. 
				    *  Above, cdf_plusp was set to point to the '+'
				    */
				   rescan = ++cdf_plusp;
				break;
			} /* end of switch */
			/*
			 *  If *rescan is not a '\0', '/', or '*', then
			 *  treat the '+' as an ordinary character and 
			 *  continue processing.
			 */
			break;
#endif /* DUX */
		case '*':
		case '?':
		case '(':
			meta=1;
			break;

		case '\\':
			quote = 1;
			rescan++;
	}
process:
	if(pat == prefix)
	{
		dirname = ".";
		prefix = 0;
	}
	else
	{
		if(pat==prefix+1)
			dirname = "/";
		*(pat-1) = 0;
		if(savequote)
			sh_trim(argstart(ap));
	}
	if(dirf=opendir(dirname))
	{
		/* check for rescan */
		if(rescan)
			*(rescan-1) = 0;
		while(dirp = readdir(dirf))
		{
#if  (! defined(DUX)  && ! defined(DISKLESS))

			if(dirp->d_ino==0 || (*dirp->d_name=='.' && *pat!='.'))
				continue;
			if(strmatch(dirp->d_name, pat))
				addmatch(prefix,dirp->d_name,rescan);

#else

			char fullname[PATH_MAX], *base_name, *end;
			struct stat dummy;

			/*
			 *  skip over "." and ".." in the directory
			 */
			if(dirp->d_ino==0 || (*dirp->d_name=='.' && *pat!='.'))
				continue;

			/*
			 *  Set up fullname to be the entire path of
			 *  the file.
			 *
			 *  Set base_name to point to the basename of this
			 *  path (sh_copy returns a pointer to the last 
			 *  character in the string that was copied).
			 *  end will point to the trailing null at end
			 *  of fullname (where we want to put a '+' to 
			 *  stat for CDFs).
			 */
			base_name = sh_copy(dirname, fullname);
			*base_name++ = '/';
			end = sh_copy(dirp->d_name, base_name);

			if(strmatch(dirp->d_name, pat))
			{
				/*
				 *  The name in the directory matches the
				 *  pattern.  Check to see if this is a
				 *  CDF for which we have no context.  If
				 *  it is, then do not add it to our list,
				 *  otherwise add the name to the list of 
				 *  matches.
				 *
				 *  If stat("fullname",&dummy) fails, but
				 *  stat("fullname+", &dummy) succeedes, then
				 *  "fullname" is a cdf for which we have no 
				 *  context.
				 */
				if (stat(fullname, &dummy) != 0)
				{
					*end++ = '+'; *end = '\0';
					if((stat(fullname, &dummy) == 0)  &&
					   (dummy.st_mode & S_ISUID)      &&
					   ((dummy.st_mode & S_IFMT)==S_IFDIR))
						continue;
					/*
					 * The stat of "fullname" failed, so as
					 * it is a symlink to nowhere or a 
					 * directory with no execute permission.
					 * Take out trailing '+'.
					 */
					 *--end = '\0';
				}
				/* we add the name *with* any '+' */
				addmatch(prefix,base_name,rescan);
			}
			else if (cdf_plus)
			{
				/*
				 *  dirp->d_name did not match the pattern.
				 *  The pattern ends in "+" or "+*".  strmatch
				 *  does not know about CDFs so we check here
				 *  to see if dirp->d_name is a CDF and matches
				 *  the "+" in the pattern.
				 */

				struct stat dummy;

				/*
				 *  Take out the "+" in the pattern and see
				 *  if the name matches the pattern.
				 */
				*cdf_plusp = '\0';
				if(strmatch(dirp->d_name, pat))
				{
					/*
					 *  The name matches the pattern w/o
					 *  the plus.  See if the file is a 
					 *  CDF, and if so, add it to our list
					 */
					*end++ = '+'; *end = '\0';
					if((stat(fullname, &dummy) == 0)  &&
					   (dummy.st_mode &S_ISUID) &&
					   (dummy.st_mode & S_IFMT)==S_IFDIR)
						addmatch(prefix,base_name,rescan);
				}
				/* restore the pattern */
				*cdf_plusp = '+';
			}
#endif  /* DUX || DISKLESS */
		}
		closedir(dirf);
	}
	return;
}

static  void addmatch(dir,pat,rescan)
char *dir, *pat, *rescan;
{
	register struct argnod *ap = (struct argnod*)stak_begin();
	register struct glob *gp = globptr();
	register char *cp = ap->argval;
#ifdef KSHELL
	ap->argflag = A_RAW;
#endif /* KSHELL */
	if(dir)
	{
		/*
		 *  If we have a directory prefix (e.g., "./")
		 *  we prepend it here to set up a full pathname
		 */
		cp = sh_copy(dir,cp);
		*cp++ = '/';
	}
	/* append filename to pathname (if any) */
	cp = sh_copy(pat,cp);
	if(rescan)
	{
		if(test_type(ap->argval,S_IFMT,S_IFDIR)==0)
			return;
		*cp++ = '/';
		ap->argbegin = cp;
		cp = sh_copy(rescan,cp);
		ap->argchn = gp->rescan;
		gp->rescan = ap;
	}
	else
	{
#ifdef KSHELL
		if(is_option(MARKDIR) && test_type(ap->argval,S_IFMT,S_IFDIR))
			*cp++ = '/';
#endif /* KSHELL */
		ap->argchn = gp->match;
		gp->match = ap;
		gp->argn++;
	}
	stak_end(cp);
}


#ifdef KSHELL

/*
 * remove tmp files
 * template of the form /tmp/sh$$.???
 */

void	rm_files(template)
register char *template;
{
	register char *cp;
	struct argnod  *schain;
	cp = strrchr(template,'.');
	*(cp+1) = 0;
	f_complete(template,"*");
	schain = st.gchain;
	while(schain)
	{
		unlink(schain->argval);
		schain = schain->argchn;
	}
}

/*
 * file name completion
 * generate the list of files found by adding an suffix to end of name
 * The number of matches is returned
 */

f_complete(name,suffix)
char *name;
register char *suffix;
{
	st.gchain =  0;
	sufstr = suffix;
	suflen = strlen(suffix);
	return(path_expand(name));
}

#else

static char *sh_copy(sp,dp)
register char *sp;
register char *dp;
{
	register char *memlast = globptr()->memlast;
	while(dp < memlast)
	{
		if((*dp = *sp++)==0)
			return(dp);
		dp++;
	}
	longjmp(globptr()->jmpbuf);
}

static char * stak_begin()
{
	register struct argnod *ap;
	register struct glob *gp = globptr();
	ap = (struct argnod*)(round(gp->last,sizeof(struct argnod*)));
	gp->last = ap->argval;
	return((char*)ap);
}

/*
 * Return true if the mode bits of file <f> corresponding to <mask> have
 * the value equal to <field>.  If <f> is null, then the previous stat
 * buffer is used.
 */

static test_type(f,mask,field)
char *f;
int field;
{
	static struct stat statb;
	if(f && stat(f,&statb)<0)
		return(0);
	return((statb.st_mode&mask)==field);
}

/*
 * remove backslashes
 */

static void sh_trim(sp)
register char *sp;
{
	register char *dp = sp;
	register int c;
	while(1)
	{
		if((c= *sp++) == '\\')
			c = *sp++;
		*dp++ = c;
		if(c==0)
			break;
	}
}
#endif /* KSHELL */

#ifdef OLDSYS5

static DIR dirbuff;

DIR *opendir(name)
char *name;
{
	register int fd;
	struct stat statb;
	if((fd = open(name,0)) < 0)
		return(0);
	if(fstat(fd,&statb) < 0 || (statb.st_mode&S_IFMT)!= S_IFDIR)
	{
		close(fd);
		return(0);
	}
	dirbuff.fd = fd;
	dirbuff.next = dirbuff.last = dirbuff.entries + NDENTS;
	return(&dirbuff);
}

struct direct *readdir(dir)
register DIR *dir;
{
	register int n;
	struct direct *dp;
	if(dir->next >= dir->last)
	{
		n = read(dir->fd,(char*)dir->entries,NDENTS*sizeof(struct direct));
		n /= sizeof(struct direct);
		if(n <=0)
			return(0);
		dir->next = dir->entries;
		dir->last = dir->entries + n;
	}
	else
		dir->next->d_ino =  dir->save;
	dp = (struct direct*)dir->next++;
	dir->save = dir->next->d_ino;
	dir->next->d_ino = 0;
	return(dp);
}
#endif /* OLDSYS5 */

#if HPBRACE
int expbrace(todo)
struct argnod *todo;
/*@
	assume todo!=0;
	return count satisfying count>=1;
@*/
{
	register char *cp;
	register int brace;
	register struct argnod *ap;
	struct argnod *top = 0;
	struct argnod *apin;
	char *pat, *rescan;
	char *sp;
	char comma;
	int count = 0;
	todo->argchn = 0;
again:
	apin = ap = todo;
	todo = ap->argchn;
	cp = ap->argval;
	comma = brace = 0;
	/* first search for {...,...} */
	while(1) switch(*cp++)
	{
		case '{':
			if(brace++==0)
				pat = cp;
			break;
		case '}':
			if(--brace>0)
				break;
			if(brace==0 && comma)
				goto endloop1;
			comma = brace = 0;
			break;
		case ',':
			if(brace==1)
				comma = 1;
			break;
		case '\\':
			cp++;
			break;
		case 0:
			/* insert on stack */
			ap->argchn = top;
			top = ap;
			if(todo)
				goto again;
			for(; ap; ap=apin)
			{
				apin = ap->argchn;
				if((brace = path_expand(ap->argval)))
					count += brace;
				else
				{
					ap->argchn = st.gchain;
					st.gchain = ap;
					count++;
				}
				st.gchain->argflag |= A_MAKE;
			}
			return(count);
	}
endloop1:
	rescan = cp;
	cp = pat-1;
	*cp = 0;
	while(1)
	{
		brace = 0;
		/* generate each pattern and but on the todo list */
		while(1) switch(*++cp)
		{
			case '\\':
				cp++;
				break;
			case '{':
				brace++;
				break;
			case ',':
				if(brace==0)
					goto endloop2;
				break;
			case '}':
				if(--brace<0)
					goto endloop2;
		}
	endloop2:
		/* check for match of '{' */
		if(*cp != '}')
			ap = (struct argnod*)stak_begin();
		else
			ap = apin;
		*cp = 0;
		sp = sh_copy(apin->argval,ap->argval);
		sp = sh_copy(pat,sp);
		sp = sh_copy(rescan,sp);
		ap->argchn = todo;
		todo = ap;
		if(ap == apin)
			break;
		stak_end(sp);
		pat = cp+1;
	}
	goto again;
}
#endif /* HPBRACE */
