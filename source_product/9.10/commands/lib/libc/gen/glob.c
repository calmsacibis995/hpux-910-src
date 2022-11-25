/* @(#) $Revision: 70.4 $ */
/**************************************************************************
 *	glob,globfree - pathname matching
 *
 *	Author: Edgar Circenis (UDL)
 *
 *	int glob(pattern,flags,errfunc,pglob)
 *	char	*pattern;
 *	int	flags;
 *	int	(*errfunc)();
 *	glob_t	*pglob;
 *
 *	void globfree(pglob)
 *	glob_t	*pglob;
 *	
 *	The glob(3C) function is a pathname generator.  The argument
 *	pattern is a pointer to a pathname pattern to be expanded.
 *	
 *	The glob() function stores the number of matched pathnames
 *	in pglob->gl_pathc and a pointer to a sorted, NULL-terminated
 *	list of pathnames in pglob->gl_pathv.
 *	
 *	It is the caller's responsibility to allocate space for the
 *	structure pointed to by pglob.  The glob() function
 *	allocates other space as needed, including the memory
 *	pointed to by gl_pathv.  The globfree() function frees any
 *	space associated with pglob from a previous call to glob().
 *	
 *	The value of flags is the bitwise inclusive OR of
 *	the following constants defined in <glob.h>:
 *	
 *	GLOB_ERR  Causes glob() to return when it first encounters a
 *	   directory that it cannot open or read.
 *	   Ordinarily, glob() continues to find matches.
 *	
 *	GLOB_MARK Each pathname that matches pattern and is a
 *	   directory has a `/' appended.
 *	
 *	GLOB_NOSORT Ordinarily, glob() sorts the matching pathnames
 *	   according to the currently active collation sequence as
 *	   defined by LC_COLLATE. When this flag is used, the order
 *	   of pathnames returned is unspecified.
 *	
 *	GLOB_NOCHECK If pattern does not match any pathname, then
 *	   glob() returns a list consisting of only pattern,
 *	   and the number of matched pathnames is 1.
 *	
 *	GLOB_DOOFFS Make use of pglob->gl_offs.  If this flag is set,
 *	   pglob->gl_offs is used to specify how many NULL pointers to
 *	   add to the beginning of pglob->gl_pathv.  In other words,
 *	   pglob->gl_pathv shall point to pglob->gl_offs NULL pointers,
 *	   followed by pglob->gl_pathc pathname pointers, followed by
 *	   a NULL pointer.
 *	
 *	GLOB_APPEND Append pathnames generated to the ones from a
 *	   previous call to glob(). (see manpage)
 *	
 *      GLOB_NOESCAPE If this flag is set, then a \ matches a \ in
 *         the filename, else \c matches the same character c.
 *
 *	If, during the search, a directory is encountered that
 *	cannot be opened or read and errfunc is not NULL, glob()
 *	calls (*errfunc)() with two arguments:
 *		- A pointer to the path that failed.
 *		- The value of errno from the failure.
 *
 *	If errfunc is called and returns non-zero, or if the
 *	GLOB_ERR flag is set in flags, glob() stops the scan and
 *	returns GLOB_ABORTED after setting gl_pathcand gl_pathvin
 *	pglob to reflect the paths already scanned.  If GLOB_ERR is
 *	not set and either errfunc is NULL or (*errfunc)() returns
 *	zero, the error is ignored.
 *	
 *	If glob() terminates due to an error, it returns one of the
 *	following constants (defined in <glob.h>), otherwise, it
 *	returns zero.
 *	
 *	GLOB_NOSPACE An attempt to allocate memory failed.
 *	
 *	GLOB_ABORTED   The scan was stopped because GLOB_ERR was set
 *		     or (*errfunc)() returned non-zero.
 *	
 *      GLOB_NOMATCH Did not find a single match and GLOB_NOCHECK
 *                   was not set.
 *
 *	In any case, the argument pglob->gl_pathc returns the number
 *	of matched pathnames and the argument pglob->gl_pathv
 *	contains a pointer to a null-terminated list of matched and
 *	sorted pathnames.  However, if pglob->gl_pathc is zero, the
 * 	value of pglob->gl_pathv is undefined.
 *
 *	IMPLEMENTATION NOTES
 *
 *	glob() uses a breadth-first search algorithm to traverse the
 *	directory structure specified by pattern.  A rewrite for depth-
 *	first search may be forthcoming to decrease memory usage, time
 *	used, and allow the efficient use of chdir().
 *
 *	glob() uses a singly linked list to store the pathnames that it
 *	finds.  When the glob_t record is filled in, this list is traversed
 *	and the gl_pathv[] structure is filled.  The gl_mem field in the
 *	glob_t record is a pointer to the first node in the linked list.
 *
 *	glob() should not be reverse engineered into any of the shell
 *	programs (ksh in particular) because the shells have their own
 *	expansion rules (i.e. ~- and ~+ in ksh) that conflict with the 
 *	POSIX 1003.2 definition for glob().  Of course, some amount of
 *	preprocessing of shell input could put a pattern into glob()-
 *	recognizable format.
 *
 *	Even though the manpage says that gl_pathv is undefined if gl_pathc
 *	is zero, glob() returns a NULL terminated list containing only a
 *	single NULL pointer, unless an error occurs, in which case, gl_pathv
 *	may indeed be undefined.  This only happens in rare instances when
 *	GLOB_NOSPACE is returned.
 *
 *	06/06/89 - ec
 *	CDF support was initially implemented in the half-baked way that
 *	the shells implement it (i.e. only patterns of form name+ would
 *	match a CDF).  Now, full CDF compatibility has been added.  Any
 *	pattern will match a CDF as long as the trailing '+' is explicitly
 *	present in the pattern.  The CDF '+' will NOT be matched by '*', '?',
 *	or in a bracket expression (the same rules for matching '.' in a
 *	hidden file.
 *
 *	06/08/89 - ec
 *	Patterns ending with slash are assumed to be requests for
 *	directories only.  Output consists of directory paths with a
 *	slash appended to each.
 *
 **************************************************************************/
#ifdef _NAMESPACE_CLEAN
#define glob _glob
#define globfree _globfree
#define closedir _closedir
#define opendir _opendir
#define readdir _readdir
#define fflush __fflush
#define printf _printf
#define nl_strcmp _nl_strcmp
#define strcat _strcat
#define strchr _strchr
#define strcpy _strcpy
#define strlen _strlen
#define strncat _strncat
#define strncpy _strncpy
#define getenv _getenv
#define getpwnam _getpwnam
#define getpwuid _getpwuid
#define getuid _getuid
#define stat _stat
#define qsort _qsort
#define fnmatch _fnmatch
#endif /* _NAMESPACE_CLEAN */


#include <glob.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <regex.h>
#ifdef NLS
#include <nl_ctype.h>
#endif /* NLS */

#define _XPG4
#include <fnmatch.h>

#define GLOB_REGERR	999		/* RE construction error */

struct list {
	struct list	*next;
	char		*s;		/* the component itself */
	char		dummy;		/* string storage */
};


static char		s[MAXPATHLEN];		/* string storage */
static char		path[MAXPATHLEN];	/* string storage */
static char		*mypat;
static int		count;		/* number of matches in do_match */
static struct list	*list;
static struct list	*node;
static struct list	*nextnode;
static struct stat	buf;
static int		slash;		/* for detecting trailing slash */
static int		glob_err;
static char		*tokstr;
static char		*mystrtok();
#if defined(DUX) || defined(DISKLESS)
static int		iscdf=0;
#endif /* DUX || DISKLESS */


#define Malloc malloc
#define Realloc realloc
#define Free free

#define CLEANUP_CURRENT_NODES()                  \
		for ( ;node;node=nextnode) {     \
			nextnode=node->next;     \
			Free(node);              \
		}

#define CLEANUP_NEW_NODES()                                \
		for (node=newlist;node;node=newnode) {     \
 		        newnode=node->next;                \
			Free(node);                        \
		}

#define INIT_NEW_NODE() {                                  \
		newnode->next=newlist;                     \
		newnode->s=(char *)&(newnode->dummy);      \
		strcpy(newnode->s,s);                      \
		newlist=newnode;                           \
				  }

static
cleanup(bad,where)	/* free temporary storage and set error value */
int bad;
int where;	/* debugging aid */
{
	if (mypat) {
		Free(mypat);
		mypat=NULL;
	}
	if (bad) {
		for (node=list;node;node=nextnode) {
			nextnode=node->next;
			Free(node);
		}
		list=NULL;
	}
	glob_err=bad;
#ifdef WHERE
	printf("cleanup %d at %d\n",bad,where);
	fflush(stdout);
#endif
}

static int
isplaintext(s, flags)
char *s;
int flags;
{
   char c;

   /*
    * This routine is called to weed out components of a pattern that
    * have special characters.  The [?* are definitely special characters;
    * the \ was added for simplicity, but it really should not be here.
    * For more details, see the comments in do_match().
    */
   while ((c = *s++) != NULL)
      if ((c == '[') || (c == '?') || (c == '*') ||
          ((c == '\\') && ((flags&GLOB_NOESCAPE) == 0))) return(0);

   return(1);   /* yes, it is plain text == no special characters found */
}


#if defined(DUX) || defined(DISKLESS)
/* check to see if a matches b+ */
/* return 1 if match, else 0    */
static int
cdfcmp(a,b)
char *a;
char *b;
{
	int i=0;

	while (1) {
		if (*(b+i)) {
			if (*(a+i) && *(a+i)==*(b+i)) i++;
			else return(0);
		} else break;
	}
	if (*(a+i)=='+' && *(a+i+1)=='\0') return(1);
	else return(0);
}
#endif /* DUX || DISKLESS */


/*	Match pattern with directory entry.	*/
/*	matches initial "." explicitly and also	*/
/*	checks for CDFs.  Returns 0 for success	*/

static int
match(pattern,string,flags)
char *pattern;
char *string;
int   flags;
{
	int noescape;

#ifdef DEBUG
	printf("matching <%s> to <%s>\n",pattern,string);
#endif
	if (*string == '.' && *pattern != '.')    /* explicit match for "." */
		return(1);
#if defined(DUX) || defined(DISKLESS)
	{
	int i,j;
	char name[MAXNAMLEN+1];

	iscdf=0;
	noescape = (flags&GLOB_NOESCAPE) ? FNM_NOESCAPE : 0;
	j=strlen(path);
	strcat(path,"/");
	strcat(path,string);
	i=stat(path,&buf);	/* does file exist? */
	if (pattern[strlen(pattern)-1]=='+') {	/* possible match for cdf? */
		strcat(path,"+");
		if (stat(path,&buf)==0) {	/* and file is cdf? */
			path[j]='\0';
			strcpy(name,string);
			strcat(name,"+");
			if (fnmatch(pattern,name,FNM_PATHNAME|noescape)==0) {
				iscdf=1;
				return(0);
			}
		}
	}
	path[j]='\0';
	if (i == -1)
		return(1);	/* stat failed; don't do normal match */
	}
#endif /* DUX || DISKLESS */
        /* do standard match */
	return(fnmatch(pattern,string,FNM_PATHNAME|noescape));
}


static struct list *
do_match(oldlist,pat,flags,errfunc)  /* recursively match pathname elements */
struct list	*oldlist;	/* list of paths found so far */
char		*pat;		/* pattern for next level of path */
int		flags;		/* glob flags */
int		(*errfunc)();	/* glob errfunc() */
{
	struct list	*newlist=NULL,*newnode,**last;
	DIR		*dirp;
	struct dirent	*dirent;
        int              saveerrno;

	if (pat==NULL) {
		if (slash) {	/* delete anything that isn't a directory */
			last = &oldlist;
			for (node=oldlist;node;node=nextnode) {
				nextnode=node->next;
				if (stat(node->s,&buf)==0 && S_ISDIR(buf.st_mode))
					last = &(node->next);
				else {
					count--;
					Free(node);
					*last=nextnode;
				}
			}
		}
		return(oldlist);
	}
	count=0;
	for (node=oldlist;node;node=nextnode) {
		nextnode=node->next;
		if (node->s[0]=='\0') /* avoid leading "./" for relative path */
			strcpy(path,".");
		else
			strcpy(path,node->s);

		/* Open and read directory, constructing list of all
		   entries that match pat */

		if ((dirp=opendir(path))!=NULL) {
			while((dirent=readdir(dirp))!=NULL) {
				if (match(pat,dirent->d_name,flags)==0) {
					strcpy(s,node->s);
					if (s[0]!='\0' && s[strlen(s)-1]!='/')
						strcat(s,"/");
					strcat(s,dirent->d_name);
#if defined(DUX) || defined(DISKLESS)
					if (iscdf) strcat(s,"+");
#endif /* DUX || DISKLESS */
                                        if ((flags&GLOB_MARK) &&
                                            (stat(s,&buf)==0) &&
                                            (S_ISDIR(buf.st_mode)))
                                           strcat(s,"/");
#ifdef DEBUG
					printf("adding %s to newlist\n",s);
#endif
					newnode=(struct list *)Malloc(sizeof(struct list)+strlen(s));
					if (!newnode) {
						cleanup(GLOB_NOSPACE,1);
						CLEANUP_CURRENT_NODES();
						CLEANUP_NEW_NODES();
						return(NULL);
					}
                                        INIT_NEW_NODE();
					count++;
				}
			}    /* end of while loop */
			closedir(dirp);
	        } else {     /* unable to opendir this path */

			/*
			 * At this point, we were unable to open the directory.
			 * This could be for several reasons, including that
			 * the directory is not readable.
			 *
			 * Previously, we simply gave up on this directory;
			 * we either ignored it, or considered it an error.
			 * If the directory is okay, but simply unreadble,
			 * this is incorrect behavior.  That is why we added
			 * the code below.
			 *
			 * The code below tries to be smart regarding
			 * unreadable directories.  We build a pathname to
			 * the next level by appending the pattern and then
			 * doing a stat().  This algorithm works well when the
			 * pattern is exactly the name of an underlying file
			 * or directory.  Unfortunately, IT DOES NOT WORK
			 * FOR ALL CASES!
			 *
			 * The issue is that patterns with special characters
			 * should not be further processed, because the only
			 * way to do the proper expansion is by opening/reading
			 * the directory - which we tried to do above.  We try
			 * to "weed out" those patterns by calling the new
			 * routine isplaintext().  This works well for the
			 * special characters [*? but does not do the right
			 * thing for the special character \ which is normally
			 * used to quote the next character.
			 *
			 * THE BUG:  isplaintext() considers the \ a special
                         *	     character just like [*? even though it
			 *           really is not.  (It is not the same,
			 *           because we can process the \ without
			 *           reading the directory.)
                         *
			 * This bug causes glob() to ignore files that live
			 * in an unreadable directory, when the pattern has
			 * at least one \ in it.  The fix may involve
			 * processing the \ before building the pathaname
			 * that we pass to stat().  We are not doing this
			 * right now because it is late in the 9.0 cycle and
			 * we may introduce other bugs with the change.
			 *
			 * --jcm 6/4/92
			 */
		        saveerrno = errno;
			strcpy(s, node->s);
			if (s[0]!='\0' && s[strlen(s)-1]!='/') strcat(s,"/");
			strcat(s, pat);
                        if (isplaintext(pat,flags) && (stat(s, &buf)==0)) {
                           if (S_ISDIR(buf.st_mode)) strcat(s,"/");
#ifdef DEBUG
			   printf("adding %s to newlist\n",s);
#endif
			   newnode=(struct list *)Malloc(sizeof(struct list)+strlen(s));
 			   if (!newnode) {
				cleanup(GLOB_NOSPACE,1);
				CLEANUP_CURRENT_NODES();
				CLEANUP_NEW_NODES();
				return(NULL);
			   }
                           INIT_NEW_NODE();
			   count++;
		        } else if (saveerrno != ENOTDIR) {
                           if ((errfunc && (*errfunc)(path, saveerrno)) || flags&GLOB_ERR) {
                                CLEANUP_CURRENT_NODES();
			        glob_err=GLOB_ABORTED;
			        return(newlist);
			   } /* 
			      * if ((errfunc == NULL) || (it returned != 0))
			      *    just ignore it.
			      */
		        }  /* if (saveerrno == ENOTDIR) just ignore it. */
  	        }
		Free(node);
	}   /*  end of outer for loop  */
/* we use mystrtok because a "/" needs to be matched explicitly with a "/"
   in the pathname.  Thus, a pattern can be thought of as many sub-
   patterns, each at a different level in the directory heirarchy.	*/
	if (newlist)
		return(do_match(newlist,mystrtok(NULL),flags,errfunc));
	else
		return(NULL);
}


static int
scmp(a,b)	/* international string compare for qsort */
char **a,**b;
{
	return(nl_strcmp(*a,*b));
}

static char *mystrtok(s)
char *s;
{
	char *p,*p2,*dptr,delim;
	int done;

	if (s) tokstr=s;
	p=tokstr;
	if (p == 0) return(NULL);	/* return if no tokens */
	while (*p=='/') {		/* skip leading slashes */
		tokstr++;
		p++;
	}
	if (*p==0) return(tokstr=0);
	while (1) {
		if (*p=='[') {
			p++;
			delim=0;
			done=0;
			while (!done) {
				switch(*p) {
					case '[':	/* check for constructs like [[:alpha:]] */
						delim=0;
						switch(*++p) {
							case ':':
							case '.':
							case '=':
								delim = *p;
								dptr=p;
						}
						break;
					case ']':	/* check for non-terminating "]" */
						if ((p-tokstr>2 && *(p-1)!=delim) || (p-tokstr==2 && *(p-1)!='!')) {
							p++;
							done++;
							break;
						}
						if (*(p-1)==delim && (p-1)!=dptr)
							delim=0;
						p++;
						break;
					case '\\':	/* skip over escaped characters */
						p++;
						if (*p) p++;
						break;
					case '\0':	/* no matching ']' */
						p2=tokstr;
						tokstr=p;
						return(p2);
					default:
#ifdef NLS16
						if (FIRSTof2(*p) && SECof2(*(p+1)))
							p++;
#endif /* NLS16 */
						p++;
						break;
				}
			}
		} else if (*p=='/') {
			*p='\0';
			p2=tokstr;
			tokstr= ++p;
			return(p2);
		} else if (*p=='\0') {	/* no more tokens */
			p2=tokstr;
			tokstr=0;
			return(p2);
		} else {
#ifdef NLS16
			if (FIRSTof2(*p) && SECof2(*(p+1)))
				p++;
#endif /* NLS16 */
			p++;
		}
	}
}

#ifdef _NAMESPACE_CLEAN
#undef glob
#pragma _HP_SECONDARY_DEF _glob glob
#define glob _glob
#endif /* _NAMESPACE_CLEAN */

int
glob(pattern,flags,errfunc,pglob)
char	*pattern;
int	flags;
int	(*errfunc)();
glob_t	*pglob;
{
	glob_t		*pglob2;
	char		**pathv;
	int		i,j;
	int		offset;
	char		*p,*p2;

/* This is a front end to glob() for handling GLOB_APPEND nonsense.
   Basically, we do a glob() of the new pattern with the bits for
   GLOB_DOOFFS and GLOB_APPEND cleared.  If everything works, we
   tack the new data onto the end of the old pglob record and return. */

	if (flags&GLOB_APPEND) {
		if (!(pglob2=(glob_t *)Malloc(sizeof(glob_t))))
			return(GLOB_NOSPACE);
		pglob2->gl_offs=0;
		if ((i=glob(pattern,flags&(~(GLOB_DOOFFS|GLOB_APPEND)),errfunc,pglob2))!=0) {
			Free(pglob2);
			return(i);
		}
		offset=(flags&GLOB_DOOFFS)?pglob->gl_offs:0;
		offset += pglob->gl_pathc;
		if(!(pathv=(char **)Malloc((offset+pglob2->gl_pathc+1)*sizeof(int)))) {
			globfree(pglob2);
			Free(pglob2);
			return(GLOB_NOSPACE);
		}
		for (i=0;i<offset;i++)
			pathv[i]=pglob->gl_pathv[i];
		for (j=0;j<pglob2->gl_pathc;j++)
			pathv[i++]=pglob2->gl_pathv[j];
		pathv[i]=NULL;
		Free(pglob2->gl_pathv);
		if (pglob->gl_pathc==0) {
			pglob->gl_mem=pglob2->gl_mem;
		} else {
			for (node=(struct list *)pglob->gl_mem;node->next;node=node->next);
			node->next=(struct list *)pglob2->gl_mem;
		}
		pglob->gl_pathc += pglob2->gl_pathc;
		Free(pglob->gl_pathv);
		pglob->gl_pathv = pathv;
		Free(pglob2);
		return(0);
	}

	/* This is where the _REAL_ glob() code starts */

	if (mypat=(char *)malloc(strlen(pattern)+1))
		strcpy(mypat,pattern);
	else
		goto NULL_RETURN;
	list=NULL;
	glob_err=count=slash=0;

	/* If trailing slash appears in pattern set GLOB_MARK	*/
	/* internally and set flag so that non-directories can	*/
	/* be weeded out later.					*/

	if (pattern[0] && pattern[strlen(pattern)-1]=='/') {
		slash++;
		flags|=GLOB_MARK;
	}

	/* If string is empty, don't bother looking */

	if (strlen(mypat)==0)
		goto NULL_RETURN;

	/**** Determine root directory for search ****/

	if (mypat[0]=='/')
		strcpy(s,"/");

/* we don't want a "./" appended to every relative pathname, so we trick
   the do_match routine into thinking that an empty string means "."	*/

	else
		s[0]='\0';

	/* create an initial list containing only the name for the root dir */

	list=(struct list *)Malloc(sizeof(struct list)+strlen(s));
	if (list == NULL) {
		cleanup(GLOB_NOSPACE,8);
		goto NULL_RETURN;
	}
	list->next=NULL;
	list->s=(char *)&(list->dummy);
	strcpy(list->s,s);

	/**** Do file name generation ****/

	count=1;
	list=do_match(list,mystrtok(mypat),flags,errfunc);

NULL_RETURN:	/* starting here, you can return pattern */
	if (count==0) {
           if (flags&GLOB_NOCHECK) {
		/* no files were found and GLOB_NOCHECK */
		/* was set, so return original pattern. */
		count=1;
		list=(struct list *)Malloc(sizeof(struct list)+strlen(pattern));
		if (list == NULL) {
			cleanup(GLOB_NOSPACE,9);
			goto NULL_RETURN2;
		}
		list->next=NULL;
		list->s=(char *)&(list->dummy);
		strcpy(list->s,pattern);
           } else if (glob_err == 0) glob_err = GLOB_NOMATCH;
	}
	
	/**** Generate pglob record ****/

	cleanup(glob_err,10);	/* free any remaining storage */

NULL_RETURN2:	/* starting here, you return an empty record */
	pglob->gl_pathc=count;
	offset=(flags&GLOB_DOOFFS) ? pglob->gl_offs:0;
	pglob->gl_pathv=(char **)Malloc((count+offset+1)*sizeof(int));
	if (pglob->gl_pathv == NULL) {
		cleanup(GLOB_NOSPACE,11);
		pglob->gl_pathc=0;
		goto NULL_RETURN3;
	}
	for (i=0;i<offset;i++)
		pglob->gl_pathv[i]=NULL;
	for (node=list;node;node=node->next)
		pglob->gl_pathv[i++]=node->s;
	pglob->gl_pathv[i]=NULL;
	if (!(flags&GLOB_NOSORT))	/* sort pathname list if requested */
		qsort(&(pglob->gl_pathv[offset]),count,sizeof(pglob->gl_pathv[offset]),scmp);
NULL_RETURN3:	/* last chance if all mallocs fail */
	pglob->gl_mem=(char *)list;
	if (glob_err==GLOB_REGERR) glob_err=0;
	return(glob_err);
	/* There!  It's done, now someone had better use it! */
}

#ifdef _NAMESPACE_CLEAN
#undef globfree
#pragma _HP_SECONDARY_DEF _globfree globfree
#define globfree _globfree
#endif /* _NAMESPACE_CLEAN */

void
globfree(pglob)	/* free memory used by pglob */
glob_t	*pglob;
{
	for (node=(struct list*)pglob->gl_mem;node;node=nextnode) {
		nextnode=node->next;
		Free(node);
	}
	Free(pglob->gl_pathv);	/* free string pointers */
}
