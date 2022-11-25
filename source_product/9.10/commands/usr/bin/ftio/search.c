/* HPUX_ID: @(#) $Revision: 72.1 $  */
/*
 * search.c
 *
 * this file contains the search algorithm(s).
 */
 
#include "ftio.h"
#include "define.h"
#ifdef HP_NFS
#include <mntent.h>
#endif /* HP_NFS */


/* 
 * search starts here 
 */

search(startpath, func_p)
char 	*startpath;
int	(*func_p)();
{
	register char *cp, *sp = 0;

	sp = "\0";

	strcpy(Pathname, startpath);

	/* 
	 * if we didn't get an absolute path, get into the user's
	 * current working directory.
	 */
	if(*Pathname != '/')	
		if ( chdir(Home) == -1)
		{
		       fprintf(stderr,
			"%s: chdir bad starting directory\n", Myname);
		       myexit(2);
		}
	/*
	 * if the path has got an `/' in it, move down
	 * set Fname as the last part of specified path name
	 */
	if(cp = strrchr(Pathname, '/')) 
	{
		/* 
		 * move into the lower starting directory
		 * then work relative to it.
		 */
		sp = cp + 1; 	/* sp points to last part of path */
		*cp = '\0';
		/* change to directory specified */
		if(chdir(*Pathname? Pathname: "/") == -1) 
		{
		       fprintf(stderr,
			"%s: bad starting directory\n", Myname);
		       return;
		}
		*cp = '/';	/* reset */
	}

	/* 
	 * set Filename
	 */
	Filename = *sp? sp: Pathname;

	/* 
	 * find files 
	 */
	(void) descend(Pathname, Filename, func_p); 
	
	return(0);
}

/*
 * descend.c	
 */
/* Pathname, Filename from main */
descend(current_path, current_fname, func_p)	
char *current_path, *current_fname;
int	(*func_p)();
{
	int  	cdret = 0;		/* return value of chdir */
	DIR 	*dirp = NULL;		/* pointer to dir opened by opendir*/
	register struct direct	*dp;	/* pointer to directory entry */
	char 	*endofpathname;		/* these 3 used in finding next path*/
	register char *pathnameptr1, *pathnameptr2;
	register i;			/* general purpose */
	int     symlink_seen;           /* =1 if original file is symbolic link */
	int     hidden = 0;             /* =1 if CDF */
	char	*basename();
#if defined(DUX) || defined(DISKLESS)
	char     wayback[MAXPATHLEN];   /* Holds path back to parent dir */
	struct stat before, after;	/* Used to compute wayback */
#endif DUX || DISKLESS



	/* 
	 * check the file is ok 
	 */

#if defined(DUX) || defined(DISKLESS)
	/*
	 * Check for hidden directories if -H was specified.
	 * (AND . or .. is not the starting directory.)
	 */

	if ( Hidden ) {

	    strcat(current_fname,"+");

	    if ( (lstat(current_fname,&Stats) != -1)    &&
		 ((Stats.st_mode & S_IFMT) == S_IFDIR)  &&
		 (Stats.st_mode & S_ISUID) ) {
		++hidden;
	    } else {
		current_fname[strlen(current_fname)-1] = '\0';
	    }
	}
#endif DUX || DISKLESS


#ifdef SYMLINKS
	if(lstat(current_fname, &Stats) == -1)
#else
	if(stat(current_fname, &Stats) == -1)
#endif SYMLINKS
	{
#ifdef DEBUG
fprintf(stderr, "(debug) current_fname: <%s>\n", current_fname);
#endif DEBUG
		(void)ftio_mesg(FM_NSTAT, current_path);
		return(0);
	}


	if ( symlink_check(&current_path, &current_fname, &Stats) )
	    symlink_seen = 1;
	else
	    symlink_seen = 0;

#ifdef DEBUG
if ( Symbolic_link ) fprintf(stderr, "ftio: symbolic link <%s>\n", current_path);
#endif DEBUG

#if defined(DUX) || defined(DISKLESS)
#ifdef DEBUG
fprintf(stderr, "pre findpath: <%s>\n", current_path);
#endif DEBUG
	(void) findpath(current_path);
#ifdef DEBUG
fprintf(stderr, "post findpath: <%s>\n", current_path);
#endif DEBUG
	current_fname = basename(current_path);
#endif DUX || DISKLESS

	/* 
	 * don't do it for current directory '.'
	 */
	if ( *(current_path + 1) != '\0' || *current_path != '.' )
	{
		/*
		 * ok put that file out to tape!
		 *
		 * HERE IS THE BIT THAT CALLS FILEGRABBER!!!
		 * i can never find it thats all!
		 */
		(*func_p)(current_fname, Pathname, &Stats, NORMAL);
	}

	/*
	 * if the entry is not a directory
	 * ie if we have descended to a file
	 *
	 * Note: that we don't want to descend directories pointed
	 *       to by symbolic links. So if we've seen a symbolic
	 *       link, return here instead of descending.
	 */
#ifdef SYMLINKS
	if ( symlink_seen ||  (Stats.st_mode & S_IFMT) != S_IFDIR )
#else
	if ( (Stats.st_mode & S_IFMT) != S_IFDIR )
#endif SYMLINKS
	{
		return(0);
	}

	/*
	 * we have a directory in our little hot hands
	 */

#ifdef HP_NFS
	/* Should we descend this directory? */

	if ( Fstype != (char *)0 && checkfs(&Stats) != OK )
	    return 0;
#endif /* HP_NFS */


	/* 
	 * set a pointer to the end of the pathname,
	 * if the pathname has a '/' on the end, leave it out
	 * 
	 * get to end of pathname
	 */
	for (pathnameptr1 = current_path; *pathnameptr1; ++pathnameptr1);

	if ( *(pathnameptr1-1) == '/') /* note *pathnameptr1 == NULL */
		--pathnameptr1;

	endofpathname = pathnameptr1;

	/* 
	 * check the size of this directory 
	 * and complain if it is really big
	 */
	if (Stats.st_size > 32000)
		fprintf(stderr,"huge directory %s - call administrator\n", 
			current_path);

	/*
	 * If  hidden  directories  are  not  explicitly  being  searched,
	 * remember where we are before doing the chdir().
	 *
	 * This is necessary because our starting directory may have  been
	 * a   CDF  even  though  we're  not  searching  CDFs,  and  after
	 * traversing all the sub-directories, a  simple  ".."  would  not
	 * bring us back to our original starting directory.
	 */

#if    defined(DUX) || defined(DISKLESS)
	if ( ! Hidden )
		lstat(".", &before);
#endif defined(DUX) || defined(DISKLESS)


	/*
	 * attempt to change directory to the directory we have 
	 * found.
	 */
	if ((cdret = chdir(current_fname)) == -1) 
	{
		(void)ftio_mesg(FM_NCHDIR, current_path);
	} 
	else 
	{
#if    defined(DUX) || defined(DISKLESS)
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
		if ( Hidden ) {
		  if ( lstat(wayback, &after) == -1 ) {
		    wayback[strlen(wayback)-1] = '\0';
		  }
		} else { /* ( ! Hidden ) */
		  while (1) {
		    if ( lstat(wayback, &after) == -1 ) {
		      wayback[strlen(wayback)-1] = '\0';
		      break;
		    }
		    else if ((before.st_ino == after.st_ino) &&
			     (before.st_dev == after.st_dev))
		      break;
		    else strcat(wayback, "/..+");
		  }
		}
#endif defined(DUX) || defined(DISKLESS)
		/* 
		 * open the current directory 
		 */
		if ( (dirp = opendir(".")) == NULL ) 
		{
			/*
			 * 	Couldn't open the directory
			 * 	finish with this directory.
			 */
			(void)ftio_mesg(FM_NOPEN, current_path);
			goto exitdir; 
		}
		/* 
		 * while not at the end of the directory 
		 * read the next entry
		 */
		while ( (dp = readdir(dirp)) != NULL ) 
		{
			/* 
			 * if the entry has inode no zero (file attribute file)
			 * or it is `.' (the current directory)
			 * or it is `..' (the above directory)
			 * then continue at completion of the loop
			 * ie skip all this stuff
			 */
			if (dp->d_ino==0 || 
			   (dp->d_name[0]=='.' && dp->d_name[1]=='\0')||
			   (dp->d_name[0]=='.' && dp->d_name[1]=='.' && 
			    dp->d_name[2]=='\0')) 
				continue;

			/*
			 * ok we have a usable entry
			 * add it's name onto current pathname and call
			 * ourself
			 */
			pathnameptr1 = endofpathname;

			*pathnameptr1++ = '/';	/* add a `/' */

			/*
			 * get the name of the dir we want to jump into
			 */
			pathnameptr2 = dp->d_name; 

			/*
			 * add the new directory name to the current
			 * path.
			 * just in case!, don't loop any more than the
			 * expected max length of an entry.
			 */
			for (i = 0; i < MAXNAMLEN; ++i)
				if(*pathnameptr2)
					*pathnameptr1++ = *pathnameptr2++;
				else
					break;
		
			*pathnameptr1 = '\0';   /* add null delimiter */	 
			/* 
			 * if file name is null length
			 * get out of this directory
			 */
			if(pathnameptr1 == (endofpathname + 1)) 
			{
				break;
			}

			/* 
			 * call ourself again
			 */
			Filename = endofpathname+1;
			(void) descend(current_path, Filename, func_p);
		}
	}
exitdir:
	/*
	 * finished with that directory!
	 *
	 * if the directory is open, close it 
	 */
	if(dirp)
		closedir(dirp);

	/*
	 * fix pathname, so it is the same as entry into this routine
	 */
	pathnameptr1 = endofpathname;
	*pathnameptr1 = '\0'; 

	/*
	 * if we could not change into this directory
	 * or if we cannot change into the directory above
	 *
	 * note that the second test will leave the process in
	 * the above directory, if it fails.
	 */
#if    defined(DUX) || defined(DISKLESS)
	if(cdret == -1 || chdir(wayback) == -1)
#else
	if(cdret == -1 || chdir("..") == -1)
#endif defined(DUX) || defined(DISKLESS)
	{
#ifdef DEBUG
fprintf(stderr, "ftio: chdir(%s) failed, recovering ..\n", wayback);
#endif /* DEBUG */
		/* 
		 * if the path is an absolute one.. 
		 * get into the root directory
		 */
		if ((endofpathname=strrchr(Pathname,'/')) == Pathname)
			chdir("/");
		else 
		/*
		 * relative path, we are in trouble, so change to home
		 * directory.
		 */
		{
			/* 
			 * set Pathname to starting path  
			 */
			if (endofpathname != NULL)
				*endofpathname = '\0';

			/* 
			 * get into home directory 
			 */
			chdir(Home);

			/* 
			 * if the original path is bad, winge and get out 
			 */
			if(chdir(Pathname) == -1) 
			{
				(void)ftio_mesg(FM_NCHDIR, Pathname);
				myexit(1);
			}
			/*
			 * put Pathname back the way it was
			 */
			if(endofpathname != NULL)
				*endofpathname = '/';
		}
	}
	return(0);
}



#ifdef HP_NFS

#define DEVS    100
static int num_of_devs = DEVS;

/*
 * checkfs - Given a stat(2) structure of a file, "checkfs" returns OK
 *           if the file resides on a file system we want to descend,
 *           not OK (i.e. "!OK") otherwise.
 *
 */

checkfs(filestatp)
    struct stat *filestatp;
{
    static dev_t *ok_devs = (dev_t *)0; /* Set up table once, then reuse. */
    int curr;


    /* Get table of st_dev's that are acceptable if we don't have it. */

    if ( ok_devs == (dev_t *)0 )
	init_ok_devs(&ok_devs);


    /* Is the st_dev of our file acceptable? */

    for (curr=0; ok_devs[curr] != (dev_t)0 && curr < num_of_devs; curr++)
	if ( filestatp->st_dev == ok_devs[curr] )
	    return OK;


    /* Unable to find st_dev in "ok_devs" table. Don't descend this one. */

    return !OK;
}


/*
 * init_ok_devs - Initialize the table with st_dev's of acceptable
 * mounted file systems. This is used by the "checkfs" routine.
 *
 */

init_ok_devs(ok_devsp)
    dev_t **ok_devsp;
{
    FILE *mntp;
    struct mntent *mnt;
    dev_t *devs;
    struct stat dirstat;
    int curr;

#ifdef DEBUG
fprintf(stderr, "ftio: entered init_ok_devs\n");
#endif /* DEBUG */

    /* Open the mount tab file. */

    mntp = setmntent(MNT_MNTTAB, "r");
    if ( mntp == (FILE *)0 ) {
	(void)ftio_mesg(FM_NOPEN, MNT_MNTTAB);
	myexit(1);
    }


    /* Allocate memory for the table of acceptable st_dev's. */

    devs = (dev_t *)malloc(num_of_devs*sizeof(dev_t));
    if ( devs == (dev_t *)0 ) {
	(void)ftio_mesg(FM_NMALL);
	(void) myexit(1);
    }


    /* Now, store all st_dev's of mount points that are
     * acceptable to descend.
     */

    for (curr=0; (mnt=getmntent(mntp)) != (struct mntent *)0;) {

	/* Skip MNTTYPE_IGNORE and MNTTYPE_SWAP. */

	if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
	    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
	  continue;


	/* Is this mount point of the type we want to descend?
	 * If so, enter its st_dev into our array of "OK" st_dev's.
	 *
	 * What if ... the stat(2) fails on the mount point?
	 * For now, report the stat(2) failure but ignore it
	 * otherwise.
	 */

	if ( strcmp(mnt->mnt_type, Fstype) == 0 ) {
	    if ( stat(mnt->mnt_dir, &dirstat) == -1 )
		(void)ftio_mesg(FM_NSTAT, mnt->mnt_dir);
	    else {
		if ( (curr+1) >= num_of_devs ) {
		    /* Allocate a bigger table. */

		    num_of_devs += DEVS;
		    devs = (dev_t *)realloc(devs,num_of_devs*sizeof(dev_t));
		    if ( devs == (dev_t *)0 ) {
			(void)ftio_mesg(FM_NMALL);
			(void) myexit(1);
		    }
		}
		devs[curr++] = dirstat.st_dev;
	    }
	}
    }

    endmntent(mntp);
    devs[curr] = (dev_t)0;
    *ok_devsp = devs;


#ifdef DEBUG
for (curr=0; devs[curr] != (dev_t)0 && curr < num_of_devs; curr++)
    fprintf(stderr, "ftio: devs[%d]: 0x%x\n", curr, devs[curr]);
#endif /* DEBUG */


    /* Done. Got all those good st_dev's stored away :-). */

    return;
}
#endif /* HP_NFS */

