/* @(#) $Revision: 70.4 $ */

/***************************************************************************
****************************************************************************

	search.c

    This file contains all the functions to expand a pathname (which may or
    may not be a directory; if it is not, the expansion is a degenerate
    case).  These functions do a depth-first traversal of a "tree" (which
    may be an acyclic graph in reality).  As it encounters nodes in
    this graph, it calls add_flistnode to add the node to the 'flist'.
    These nodes are only added to the flist if they have been modified
    since the last (appropriate) session, if such a session exists.
    If there is no such session, or if it is a level 0 dump, the beginning
    of time is assumed, so all nodes are added to the flist.

****************************************************************************
***************************************************************************/
 
#include "head.h"

#ifdef NLS
#include <nl_ctype.h>
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

char *mydirname();
int qsstrcmp();
void free();
void qsort();
FLISTNODE *add_flistnode();
DIR *opendir();
struct dirent *readdir();

#define NUMCHILDREN 32

typedef struct {
    char *name;
    int diridx;
    ino_t ino;
    dev_t dev;
    FLISTNODE *flistptr;
} DIRENTRY;

extern char	*startdir;

extern ETAB	*dir_excldtab,
		*file_excldtab;
extern int	dir_excld,
		file_excld,
		nflag,
		Hflag;

extern int already_in_flist;
extern dtabentry *current_dirptr;
extern dtabentry *add_dentry(), *add_pdentry();
extern FLISTNODE *current_flistptr;
extern char *new_malloc2();


static char path[MAXPATHLEN];
char lostpath[MAXPATHLEN];

/***************************************************************************
    This is the entry point for the depth-first tree traversal.  It handles
    the case where the path specified is not a directory, and also sets
    up the call to expanddir, which recursively descends into the tree.
***************************************************************************/
search(ptr, weird)
char *ptr;
int weird;
{
    struct stat statbuf;
    FLISTNODE *saveflistptr;

    (void) strcpy(path, ptr);

#ifdef DEBUG
fprintf(stderr, "search: entered with ptr = %s\n", ptr);
fprintf(stderr, "search: entered with Hflag = %d\n", Hflag);
#endif

#if defined(DUX) || defined(DISKLESS)
    /*
     * Check for hidden directories if -H was specified.
     * (AND . or .. is not the starting directory.)
     */

    if ( Hflag ) {

	strcat(path,"+");

	if ( (statcall(path,&statbuf) != -1)          &&
	     ((statbuf.st_mode & S_IFMT) == S_IFDIR)  &&
	     (statbuf.st_mode & S_ISUID) )
	    (void) findpath(path);
	else
	    path[strlen(path)-1] = '\0';
    }
#endif /* DUX || DISKLESS */

    if(statcall(path, &statbuf) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,501, "fbackup(1501): could not stat %s in function search\n")), path);
	return;
    }
#ifdef DEBUG
fprintf(stderr, "search: ********************* just BEFORE add_flistnode\n");
#endif
    (void)add_pdentry(path);
    current_flistptr = add_flistnode(path, &statbuf);
/*
 * Do not search trees which have already been searched
 */
    if (already_in_flist && !weird)
	return;
#ifdef DEBUG
fprintf(stderr, "search: ********************* just AFTER add_flistnode\n");
#endif
    if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
	saveflistptr = current_flistptr;	/* save current pointer */
#ifdef DEBUG
fprintf(stderr, "search: *********************directory ptr = %s\n", ptr);
#endif



    if ((!nflag) && (statbuf.st_fstype == MOUNT_NFS)) {
msg((catgets(nlmsg_fd,NL_SETN,516, "fbackup(1516): %s not backed up - 'n' option (NFS) not specified \n")),path);

    }
	else

	if (chdir(path) == -1)
	    msg((catgets(nlmsg_fd,NL_SETN,502, "fbackup(1502): could not chdir to %s in function search\n")), path);
	else
	    (void) expanddir(statbuf.st_ino, statbuf.st_dev);
	if (chdir(startdir) == -1)
	    msg((catgets(nlmsg_fd,NL_SETN,503, "fbackup(1503): could not chdir to starting directory %s\n")), startdir);
	current_flistptr = saveflistptr;	/* restore current pointer */
    }
#ifdef DEBUG
    else {
      fprintf(stderr, "search: *********************FAILED directory TEST ptr = %s\n", ptr);
    }
#endif
}


/*
 * This is the structure used to manage a linked list of malloc chunks.
 * Each call to expanddir() declares a local variable mbuf which points
 * to its chunk.  If more chunks are needed then another mbuf is allocated
 * from the end of the current chunk and becomes the head of the linked
 * list pointed to by mbufs.
 * The chunks should be free'd when the routine returns.
 */
typedef struct m_buf {
  struct m_buf *next;
  char *mbuf;
  char *buf;
  size_t left;
} m_buf;

m_buf *mbufs = (m_buf *)NULL;

#define OK    0
#define LOST  1

/***************************************************************************
    This function normally operates in a reasonably straght-forward manner;
    that is, unless a weird case occurs.  In all cases, expanddir is called
    with the global value of path set to a directory.  Expanddir then opens
    the directory and reads all its entries (except . and ..) into a table.
    If there are too many entries to fit in the table, a new table of double
    the previous size is allocated, the old contents are copied into the
    new table, and the reading of directories into the new table continues.
    After this, this table is sorted into alphabetical order.
    Assuming the "normal" case, we sequence through the  sorted table, and
    a call to add_flistnode is made for each (directory or non-directory)
    entry.  The "weird" cases are documented below.

***************************************************************************/
int
expanddir(this_ino, this_dev)
ino_t this_ino;
dev_t this_dev;
{
    char *cp, *cp2, prevc1, prevc2, *lostch;
    struct stat statbuf;
    struct dirent *dp;
    DIR	*dirp;
    DIRENTRY *array, *tmp;
    int i, j, found, arraysize, siblink, dotdot_ok, diridx;
    int nfiles = 0, dirnum = 0;
#if defined(DUX) || defined(DISKLESS)
    int hidden = 0;                 /* =1 if CDF */
    char wayback[MAXPATHLEN];       /* Holds path back to parent dir */
    struct stat before, after;      /* Used to compute wayback */
    int namelen;
#endif /* DUX || DISKLESS */
    dtabentry *direntry;
    FLISTNODE *saveflistptr;
    m_buf mbuf;

#if defined(DEBUG) || defined(DEBUG_T)
msg("fbackup: just entered expanddir with <%s>\n", path);
#endif /* DEBUG */

    mbuf.next = mbufs;
    mbuf.buf = mbuf.mbuf = NULL;
    mbuf.left = 0;
    mbufs = &mbuf;
    arraysize = NUMCHILDREN;
    array = (DIRENTRY *)new_malloc2(arraysize * sizeof(DIRENTRY));

    if ((dirp = opendir(".")) == NULL) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,504, "fbackup(1504): could not open directory `.' (%s)\n")), path);

      new_free2(&mbuf);
      return(LOST);
    }
/*
 * Set current directory pointer for use in add_flistnode() for all files
 * being added.
 */
    direntry = add_dentry(path);

    while ((dp = readdir(dirp)) != NULL) {
	cp = dp->d_name;
	if ( (*cp == '.') && ((*(cp+1) == '\0') ||
			    ((*(cp+1) == '.') && (*(cp+2) == '\0'))))
	       continue;

	/* a CDF with the current context deleted may still have directory
	   entries for other contexts; If it does not exist in current context
	   and if Hflag is not set then skip this entry.
	 */
	if ( lstat(cp, &statbuf) == -1 && !Hflag )
		continue;

	if (nfiles == arraysize) {
	    arraysize += arraysize ;
	    tmp = (DIRENTRY*)new_malloc2(arraysize * sizeof(DIRENTRY));
    	    if (tmp == (DIRENTRY*)NULL)
		errorcleanx(catgets(nlmsg_fd,NL_SETN,505, "fbackup(1505): out of virtual memory\n"));
	    for(i = 0; i < nfiles; i++) {
/* retain the name pointer */
		tmp[i].name = array[i].name;
	    }
	    array = tmp;
	}
# if defined(DUX) || defined(DISKLESS)
/* BUGFIX need the extra char for adding the "+" later on */
	array[nfiles].name = (char*)new_malloc2(strlen(dp->d_name) + 2);
# else
	array[nfiles].name = (char*)new_malloc2(strlen(dp->d_name) + 1);
# endif /*DUX || DISKLESS*/
	if(array[nfiles].name == (char *)NULL)
	    errorcleanx(catgets(nlmsg_fd,NL_SETN,512, "fbackup(1512): out of virtual memory\n"));
	(void) strcpy(array[nfiles].name, dp->d_name);
#ifdef DEBUG_FILE
msg("fbackup: found %s in <%s>\n", dp->d_name, path);
#endif /* DEBUG */
	nfiles++;
    }
    closedir(dirp);


    qsort((char *)array, (unsigned)nfiles, sizeof(DIRENTRY), qsstrcmp);

    cp = path;
    prevc1 = prevc2 = '\0';
    while (*cp) {
	prevc2 = prevc1;
	prevc1 = *cp;
	ADVANCE(cp);
    } 
#if defined(DUX) || defined(DISKLESS)
    if ( prevc1 == '/' && prevc2 != '+' ) {
#else
    if ( prevc1 == '/' ) {
#endif /* defined(DUX) || defined(DISKLESS) */
	cp2 = cp;
    } else {
	*cp = '/';
	cp2 = cp +1;
    }


    for (i=0; i<nfiles; i++) {
#if defined(DUX) || defined(DISKLESS)
	/*
	 * Check for hidden directories if -H was specified.
	 * (AND . or .. is not the starting directory.)
	 */

	if ( Hflag ) {

	    namelen = strlen(array[i].name);
	    array[i].name[namelen] = '+';
	    array[i].name[namelen + 1] = '\0';

	    if ( (statcall(array[i].name,&statbuf) != -1)   &&
		 ((statbuf.st_mode & S_IFMT) == S_IFDIR)    &&
		 (statbuf.st_mode & S_ISUID) ) {
		namelen++;
		hidden = 1;
	    } else {
		array[i].name[namelen] = '\0';
		hidden = 0;
	    }
	    (void) strcpy(cp2,array[i].name);
#ifdef DEBUG
fprintf(stderr, "fbackup: pre findpath: <%s>\n", path);
#endif DEBUG
	    (void) findpath(path);
#ifdef DEBUG
fprintf(stderr, "fbackup: post findpath: <%s>\n", path);
#endif DEBUG
	    if ( strncmp(cp2,array[i].name,namelen) != 0 ) {
		int pathlen = strlen(path);

		/* We must readjust "cp2" because findpath() changed
		 * the "path".
		 *
		 * To readjust we'll set "cp2" to the last possible
		 * location in "path" where we could find the file name
		 * (array[i].name) and then search backwards until we
		 * find it. If we never find it, something is very wrong.
		 */

		cp2 = path + pathlen - namelen;
		for (; cp2 > path; cp2--)
		    if ( strncmp(cp2,array[i].name,namelen) == 0 )
			break;
		if ( cp2 == path )
		    msg((catgets(nlmsg_fd,NL_SETN,508, "fbackup(1508): unable to readjust \"cp2\" in expanddir().\n")));
	    }
	} else
#endif /* DUX || DISKLESS */
	    (void) strcpy(cp2, array[i].name);

	if(statcall(array[i].name, &statbuf) == -1) {
	    msg((catgets(nlmsg_fd,NL_SETN,506, "fbackup(1506): could not stat file %s\n")), array[i].name);
	} else {
	    found = FALSE;
	    if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {

							/* not a directory */

				    /* Check the non-directory exclude table */
		for (j=0; j<file_excld; j++) {
		    if ((statbuf.st_ino == file_excldtab[j].inode) &&
				(statbuf.st_dev == file_excldtab[j].device)) {
			found = TRUE;
			break;
		    }
		}
		if (!found) {	  /* (selectively) add this file to the flist */
/*
 * Set the current flist pointer for later comparison in add_flistnode()
 */
		    if (saveflistptr = add_flistnode(path, &statbuf))
			current_flistptr = saveflistptr;
		}
	    } else {					/* a directory */

					/* Check the directory exclude table */
		for (j=0; j<dir_excld; j++) {
		    if ((statbuf.st_ino == dir_excldtab[j].inode) &&
				(statbuf.st_dev == dir_excldtab[j].device)) {
			found = TRUE;
			break;
		    }
		}

		if (!found) {
					    /* not in the exclude table */
		    siblink = FALSE;
		    dotdot_ok = TRUE;
				    /* try to add this directory to the flist */
		    array[i].flistptr = add_flistnode(path, &statbuf);
		    if (array[i].flistptr != (FLISTNODE*)NULL) {
/*
 * Set the current flist pointer for later comparison in add_flistnode()
 * Also save the current flist pointer to be restored after returning
 * from the directory.
 */
			saveflistptr = current_flistptr = array[i].flistptr;
			array[i].ino = statbuf.st_ino;	     /* it was added */
			array[i].dev = statbuf.st_dev;
			array[dirnum].diridx = i;

			/* Check for 'sibling links' (siblinks).  This is when
			   there are > 1 link from inode A to inode Z with
			   different names, perhaps link1 and link2 (as opposed
			   to the more typical case where there would be a link
			   from inode A to inode Z and one from inode B to
			   inode Z.
			   This loop checks this current child diirectory (#i)
			   against all previous child directories in the 
			   current working directory.  If any siblinks are
			   found, the flag siblink is set.
			*/

			for (j=0; j<dirnum; j++) {
			    diridx = array[j].diridx;
			    if ((array[diridx].ino == statbuf.st_ino) &&
					(array[diridx].dev == statbuf.st_dev)) {
				siblink = TRUE;
				add_siblink(array[i].flistptr,
							array[diridx].flistptr);
				break;
			    }
			}
			dirnum++;

					    /* if ./.. is what it 'should' be */
			dotdot_ok = dotdotisparent(array[i].name, this_ino,
							this_dev, &statbuf);
		    }



     if ((!nflag) && (statbuf.st_fstype == MOUNT_NFS)){
msg((catgets(nlmsg_fd,NL_SETN,517, "fbackup(1517): %s not backed up - 'n' option (NFS) not specified \n")),path);

	}

		else 

		{

		    /* If it's a siblink, we don't want to expand it again.
		       If its .. isn't ok, it's already been added to the
		       weird list (so it will be handled later), so we don't
		       want to expand it either.
		    */

		    if (!siblink && dotdot_ok) {
#if defined(DUX) || defined(DISKLESS)
			/*
			 * If hidden directories are not explicitly  being
			 * searched,  remember  where  we are before doing
			 * the chdir().
			 *
			 * This  is   necessary   because   our   starting
			 * directory may have been a CDF even though we're
			 * not searching CDFs, and  after  traversing  all
			 * the  sub-directories,  a  simple ".." would not
			 * bring  us  back  to   our   original   starting
			 * directory.
			 */

			if ( ! Hflag )
			    statcall(".", &before);
#endif /* defined(DUX) || defined(DISKLESS) */

			if (chdir(array[i].name) == -1) {
			    msg((catgets(nlmsg_fd,NL_SETN,507, "fbackup(1507): could not chdir to %s\n")), array[i].name);
			} else {
#if defined(DUX) || defined(DISKLESS)
			    /*
			     * Figure out how to get back  to  where  we  just
			     * were.
			     *
			     * If "fname" is an absolute  pathname,  then  our
			     * starting   directory   is  different  from  our
			     * searching  directory,  and  we  return  to  our
			     * "Home" (i.e. starting) directory.
			     *
			     * Otherwise, if we are explicitly searching CDFs,
			     * we  first try to see if "..+" will get us back.
			     * If "..+" doesn't exist, use "..".
			     *
			     * If we are not explicitly searching  CDFs,  then
			     * we  use  the  "before/after"  stat(2)  info  to
			     * figure out the way back to our original (before
			     * chdir(2)) directory.
			     */

			    strcpy(wayback, "..+");
			    if ( Hflag ) {
			      if ( statcall(wayback, &after) == -1 ) {
				wayback[strlen(wayback)-1] = '\0';
			      }
			    } else { /* ( ! Hflag ) */
			      while (1) {
				if ( statcall(wayback, &after) == -1 ) {
				  wayback[strlen(wayback)-1] = '\0';
				  break;
				}
				else if ((before.st_ino == after.st_ino) &&
					 (before.st_dev == after.st_dev))
				  break;
				else strcat(wayback, "/..+");
			      }
			    }
#endif /* defined(DUX) || defined(DISKLESS) */

			    /* If the (sub)tree being expanded is removed while
			       we've descended into it, we suddenly don't know
			       where we are any more.  When this happens,
			       expanddir returns the value LOST, and the
			       previous invocation attempts to 'find' where we
			       are, and then continue.
			    */

			    /* Save the current flist pointer before
			     * the expanddir().
			     */ 
			    saveflistptr = current_flistptr;

			    if (expanddir(statbuf.st_ino, statbuf.st_dev)==OK) {
/*
 * Restore current directory and flist pointers.
 */
				current_dirptr = direntry;
				current_flistptr = saveflistptr;
#if defined(DUX) || defined(DISKLESS)
				if ( chdir(wayback) == -1 ) {
#ifdef DEBUG
cp = gethcwd((char *)0, MAXPATHLEN);
fprintf(stderr, "fbackup: working directory: <%s> after chdir(%s) failed\n", cp, wayback);
free(cp);
#endif /* DEBUG */
#else /* defined(DUX) || defined(DISKLESS) */
				if ( chdir("..") == -1 ) {
#endif /* defined(DUX) || defined(DISKLESS) */
				    (void) mydirname();
				    new_free2(&mbuf);
				    return(LOST);
				}
			    } 
			    
			    else {			/* we're lost */
				if (*path != '/') {
				    (void) strcpy(lostpath, startdir);
				    (void) strcat(lostpath, "/");
				} else {
				    *lostpath = '\0';
				}
				lostch = mydirname();
				(void) strcat(lostpath, path);
/*
 * Restore current directory and flist pointers.
 */
				current_dirptr = direntry;
				current_flistptr = saveflistptr;

				if (chdir(lostpath) == -1) {
				    new_free2(&mbuf);
				    return(LOST);
				}
				*lostch = '/';
			    }
			}
			}
		    }
		}
	    }
	}
    }
    *cp = '\0';
    new_free2(&mbuf);

    return(OK);
}





/***************************************************************************
    This function is used to detect one type of weirdness, when a directory's
    .. pointer does not point to its 'parent' directory (what one would
    normally think of as being its parent directory).  It does this by knowig
    the (inode, device) number pair for the current working directory.  The
    parent directory of the child directory of interest is stat-ed to see
    if it is the same (inode, device) pair as the cwd.  Ie, if we are in 
    directory Parent, and we are checking directory Child, we stat Child/..
    to see if we get the same resuts obtained by stating Parent.  If the
    results differ, we have a .. pointer which is 'weird'.
***************************************************************************/
int
dotdotisparent(dir, this_ino, this_dev, statbuf)
char *dir;
ino_t this_ino;
dev_t this_dev;
struct stat *statbuf;
{
    struct stat dotdotstatbuf;
    char dotdotdir[MAXPATHLEN];

    (void) strcpy(dotdotdir, dir);
    (void) strcat(dotdotdir, "/..");
#if defined(DUX) || defined(DISKLESS)
    if ( Hflag ) {
	(void) strcat(dotdotdir, "+");
	if (statcall(dotdotdir, &dotdotstatbuf) == -1 )
	    dotdotdir[strlen(dotdotdir)-1] = '\0';

	/* If this statcall succeeds then we'll do 2 stat's. Oh well.   */
	/* Otherwise, we must do another statcall on the ".."           */
	/* directory, instead of "..+".                                 */
    }
#endif /* DUX || DISKLESS */

    if(statcall(dotdotdir, &dotdotstatbuf) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,513, "fbackup(1513): statcall failed in dotdotnotparent\n")));
	return(TRUE);
    }
    if ((this_ino!=dotdotstatbuf.st_ino) || (this_dev!=dotdotstatbuf.st_dev)) {
	add_weirddir(statbuf, &dotdotstatbuf);
	return(FALSE);
    } else
	return(TRUE);
}






/***************************************************************************
    This is my version of dirname, which should work even with NLS.
***************************************************************************/
char *
mydirname()
{
    char *ch, *nextch, *savech;

    ch = nextch = savech = path;
    while (*ch) {
	ADVANCE(nextch);
	if ((*ch == '/') && (*nextch != '/') && (*nextch != '\0'))
	    savech = ch;
	ch = nextch;
    } 
    if (savech == path) {
	*savech = '.';
	savech += 1;
    }
    *savech = '\0';
    return(savech);
}


#if defined(DUX) || defined(DISKLESS)
/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : findpath()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path ( possibly an unexpanded cdf with ..,.,.+ & ..+ in it)
 *      return a fully expanded path, a bitstring which contains
 *      the permissions of each component, and the number of components.
 *
 *
 *      Returns: expanded path (returned through "path" argument)
 */

static char fullpath[MAXPATHLEN];   /* See the "init_findpath" routine  */
static char *rest_of_path;          /* for more information.            */

findpath(path)
    char *path;
{
    int bitindex;
    int bitstring[MAXCOMPONENTS];
    static char tmp[MAXPATHLEN];
    short pass;
    char *r,*s,*lastslash;
    char *cp, *cp2;
    int end;
    char ts,save;
    struct stat t,dest;


    /* Make sure that "init_findpath" has been called. */

    if ( fullpath[0] != '/' ) {
	msg((catgets(nlmsg_fd,NL_SETN,509, "fbackup(1509): initialization error for findpath.\n")));
	(void) exit(ERROR_EXIT);
    }


    /*
     * Insure absolute path name.
     */

    if ( *path == '/' )
	strcpy(fullpath,path);
    else
	strcpy(rest_of_path,path);


    bitindex = end = 0;
    pass = -1;


#ifdef DEBUG
msg("findpath: path <%s>\n", path);
#endif DEBUG

    /*
     * Remove extra slashes in pathname.
     */

    for (cp=fullpath; *cp; cp++) {
	if ( *cp == '/' && *(cp+1) == '/' ) {
	    for (cp2=cp+2; *cp2 == '/'; cp2++)
		;
	    overlapcpy(cp+1,cp2);
	}
    }

#ifdef DEBUG
msg("findpath: fullpath <%s>\n", fullpath);
#endif DEBUG

Reset:
    bitindex=0;
    lastslash = cp = fullpath;
    if (*cp == '/') {
	lastslash = cp;
	cp++;
    }

    while ( (cp2=strchr(cp,'/')) != (char *)NULL || *cp ) {
	if ( cp2 == (char *)0 && *cp != '\0' ) {
	    cp2=cp;
	    while (*cp2)
		cp2++;
	    end = 1; /* Last component. */
	}


	/* Get a component by putting in a null. */

	save = *cp2;
	*cp2 = '\0';

#ifdef DEBUG
msg("fbackup: findpath: looking at component <%s> (pass=%d)\n", cp, pass);
#endif DEBUG

	if (!strcmp(lastslash,"/..") || !strcmp(lastslash,"/..+")) {
	    if (lastslash == fullpath ) {
		strcpy(fullpath,"/");
		if (!end)
		  strcat(fullpath,cp2+1);
		cp=cp2;
		continue;
	    } else
		*lastslash=0;

	    (void) getcdf(fullpath,tmp,MAXPATHLEN);
	    if(!strcmp(fullpath, tmp)) {
		*lastslash = '/';
		*cp2 = save;
		lastslash = cp2;
		cp = cp2+1;
		continue;
	    }

	    *lastslash='/';
	    if ( statcall(fullpath,&dest) == -1 )
		msg((catgets(nlmsg_fd,NL_SETN,510, "fbackup(1510): could not stat %s\n")), fullpath);
	    r = tmp;
	    if ( *r == '/' )
		r++;

	    while ( ((s=strchr(r,'/')) != (char*)NULL) || *r ) {
		if ( *s == '\0' && *r ) {
		    s=r;
		    while (*s++)
			;
		}
		ts = *s;
		*s = '\0';
		if ( statcall(tmp,&t) == -1 )
		    msg((catgets(nlmsg_fd,NL_SETN,514, "fbackup(1514): could not stat %s\n")), tmp);

		if (t.st_ino == dest.st_ino &&
		    t.st_cnode == dest.st_cnode &&
		    t.st_dev == dest.st_dev ) {
		    if (!end) {
			strcat(tmp,"/");
			strcat(tmp,cp2+1);
		    } else
			cp = cp2;
		   break;
		}

		*s =ts;
		if ( *s == '\0' )
		    break;
		r=s+1;
	    }

	    strncpy(fullpath,tmp,MAXPATHLEN);
	    cp=fullpath;
	    if (*cp == '/') {
		lastslash = cp;
		cp++;
	    }
	    continue;
	}

	/* ordinary file */
	/* if a cdf and there is a plus at the end, take it out
	  else just sit bitstring */
	switch (pass) {
	    case -1:             /* first pass , just increment ptrs */
		*cp2=save;
		lastslash = cp2;
		break;

	    case 0:              /* second pass, stat */
		if ( statcall(fullpath,&t) == -1 )
		    msg((catgets(nlmsg_fd,NL_SETN,515, "fbackup(1515): could not stat %s\n")), fullpath);
		bitstring[bitindex++]=t.st_mode;
		*cp2=save;
		lastslash = cp2;
		break;

	    case 1:                 /* Third pass, change '+'
				       to "+/"  */
		t.st_mode = bitstring[bitindex];
		*cp2=save;
		lastslash = cp2;
		if ( ( (t.st_mode & S_IFMT) == S_IFDIR ) &&
		     (  t.st_mode & S_ISUID )
		   ) {
		    if ( ( cp2 > fullpath ) && (*(cp2-1) == '+') ) {
			strcpy(tmp,cp2);
			strcpy(cp2,"/");
			*(cp2+1) = '\0';
			strcat(fullpath,tmp);
			cp2++;
		    }
		}
		bitindex++;
		lastslash = cp2;
		break;

	    default:
		break;
	}
	if ( *cp2 == '\0' )
	    break;
	cp=cp2+1;

    }

#ifdef DEBUG
msg("findpath: fullpath <%s> (pass %d)\n", fullpath, pass);
cp = getcwd((char *)0, MAXPATHLEN);
msg("working directory: <%s>\n\n", cp);
free(cp);
#endif DEBUG

    if ( ++pass < 2 )
	goto Reset;


    /* Copy new pathname back into "path". */

    if ( *path == '/' )
	strcpy(path,fullpath);
    else
	strcpy(path,rest_of_path);

    for (cp=path; *cp; cp++) {
	if ( *cp == '/' && *(cp+1) == '/' ) {
	    for (cp2=cp+2; *cp2 == '/'; cp2++)
		;
	    overlapcpy(cp+1,cp2);
	}
    }
    if((path[strlen(path)-1] == '/') && (strlen(path) != 1))
        path[strlen(path)-1] = '\0';

    return;
}


/* init_findpath - This routine must be called in the main() procedure! */

init_findpath()
{
    /*
     * Set up buffer and pointer to insure absolute path
     * for findpath() routine.
     *
     *
     * The "fullpath" string contains the following:
     *
     *      <cwd>/<rest_of_path>
     *
     * where <cwd> is the current working directory prefix
     * and <rest_of_path> is the actual path name that we're
     * processing. Together these two sections form the
     * absolute path name of a file.
     *
     * The "rest_of_path" pointer points to the <rest_of_path>
     * portion of "fullpath".
     */

    strcpy(fullpath, startdir);
    rest_of_path = &fullpath[strlen(fullpath)];


    /*
     * Insure termination of "fullpath" with a slash.
     */

    if ( fullpath[strlen(fullpath)-1] != '/' ) {
	strcpy(rest_of_path,"/");
	rest_of_path++;
    }

#ifdef DEBUG
msg("fbackup: call init_findpath:\n");
msg("\trest_of_path -> <%s>\n", rest_of_path);
msg("\tfullpath -> <%s>\n", fullpath);
#endif /* DEBUG */
}


overlapcpy(s1,s2)
    char *s1;
    char *s2;
{

    /*
     * Copy overlapping areas.
     * Don't forget '\0' character at end.
     *
     * Note: this copy will only work if "s1" precedes "s2" in memory.
     */

    for (; *s2; s1++,s2++)
	*s1 = *s2;
    *s1 = '\0';
}


#endif DUX || DISKLESS

int
qsstrcmp(s1, s2)
char *s1, *s2;
{
    register DIRENTRY *name1, *name2;

    name1 = (DIRENTRY *)s1;
    name2 = (DIRENTRY *)s2;

#ifdef NLS
    return(strcoll(name1->name, name2->name));
#else
    return(strcmp(name1->name, name2->name));
#endif
}

/*
 * malloc a large chunk to be given out in smaller pieces.
 * This routine uses the global mbufs which is a frame pointer to the
 * currently active chunk.  If a request cannot fit in the current chunk
 * then another mbuf is allocated in the space left in the current chunk
 * which points to a newly allocated chunk.  The new mbuf is added to the
 * beginning of the linked list (ie. top of the stack) and mbufs is set
 * to it.
 */
#define CHUNK	(8*1024)

char *new_malloc2(size)
     size_t size;
{
  char *ptr;
  m_buf *mbuf;
  size_t minsize;

  size = ((size + (sizeof(int)-1)) & ~(sizeof(int)-1));	/* round up */
  minsize = size + sizeof(m_buf);

  if (mbufs->left < minsize) {
    if (mbufs->mbuf == NULL) {
      mbuf = mbufs;
    } else {
      mbuf = (m_buf *)mbufs->buf;
    }
/*
 * allocate enough memory for the request
 */
    for (mbuf->left = CHUNK; mbuf->left < minsize; mbuf->left += CHUNK);

    while ((mbuf->mbuf = (char *)malloc(mbuf->left)) == NULL) {
      if ((mbuf->left >>= 1) < minsize) {
	return NULL;
      }
    }

    if (mbuf != mbufs) {
      mbuf->next = mbufs;
      mbufs = mbuf;
    }

    mbufs->buf = mbufs->mbuf;
  }

  ptr = mbufs->buf;
  mbufs->left -= size;
  mbufs->buf = &mbufs->buf[size];
  return ptr;
} /* new_malloc2 */

/*
 * Free the chunks up to and including the one pointed to by mbuf
 * and set mbufs to the next chunk.
 */
new_free2(mbuf)
     m_buf *mbuf;
{
  while (mbufs != mbuf) {
    free(mbufs->mbuf);
    mbufs = mbufs->next;
  }

  free(mbufs->mbuf);
  mbufs = mbufs->next;
} /* new_free2 */
