/* HPUX_ID: @(#) $Revision: 66.1 $  */
/*
 * toolong - determines if a filename exceeds the max length of the filesystem.
 *
 *	toolong takes a single pathname (name) as its argument.
 *	If the file portion (name - preceding directories) is shorter than
 *		DIRSIZ, toolong returns 0.
 *	Otherwise, the fullname is stat'ed (created if necessary), and the
 *		filename minus the last character is stat'ed (created if
 *		necessary).  If these two names correspond to two distinct
 *		physical files, the long name will fit,	and toolong returns 0.
 *		If the names point to the same file, the system has truncated
 *		the long name (name will not fit), and toolong returns 1.
 *	Before returning, toolong removes any files it created.
 *
 *
 *	I wish there were an easier way to do this, but currently there
 *	does not seem to be, since one cannot be sure if the file is on a
 *	local lfn system, a local 14 system, a remote lfn system, or a
 *	remote 14 system.  s800 ifdefs are NOT sufficient.
 *
 *	The following commands currently use this code:
 *		compact/uncompact
 *		compress
 *		rcs
 *		sccs
 */
#include <sys/param.h>		/* for MAXPATHLEN value	*/
#include <errno.h>		/* for error values	*/
#include <ndir.h>		/* for DIRSIZ value	*/
#include <fcntl.h>		/* for O_CREAT value	*/
#include <sys/stat.h>		/* for stat structures	*/

#define	SMALLENOUGH	0
#define TOOBIG		1
#define TRUE		1
#define FALSE		0

#define	ERROR	{error = TRUE ; goto done;}

extern	int	errno;

toolong (name)
char	*name;

{
	char	*slashp,	/* pointer to last / in filepath */
		*endp;		/* pointer to end of filename */
	char	lastchar;	/* holds last character in filename */
	int	fullfd = 0;
	int     shortfd = 0;
	struct	stat	fullstat, shortstat;
	int	fullcreat = FALSE,	/* TRUE if fullname file created */
		shortcreat = FALSE,	/* TRUE if shortname file created */
		sizechk	= TOOBIG,	/* presume fullname too long */
		error	= FALSE;

	/* find length of filename after any or all directories in path */

	for (slashp = endp = name ; *endp ; endp++)
		if (*endp == '/')
			slashp = endp + 1;

	if (endp - name > MAXPATHLEN)	/* check for path too long */
		return (TOOBIG);

	if (endp - slashp <= DIRSIZ)	/* if filename < DIRSIZ, no problem */
		return (SMALLENOUGH);

	lastchar = *--endp;		/* save last char, mark its place */

	/* get stat block of fullname file, creating the file if necessary */

	if (stat (name, &fullstat)) 
	  {
	    if (errno == ENOENT) 
	      {		/* file not found, create */
		if ((fullfd = open (name, O_CREAT, 0666)) < 0)
		  {
		    if ( errno == EACCES )
		      {
			if ((fullfd = open( name, O_RDONLY, 0 )) < 0 )
			  {
			    /*
			    ** We can not test a protected directory, so
			    ** we have to assume it is too long.
			    */
			    errno = EACCES;
			    ERROR;
			  }
		      }
		    else
		      {
			ERROR;
		      }
		  }
		fullcreat = TRUE;
		if (fstat (fullfd, &fullstat))
		  {
		    ERROR;
		  }
	      }
	    else 
	      if (errno == ENAMETOOLONG)
		return (TOOBIG);
	      else			/* stat error toolong can't handle */
		{
		  ERROR;
		}
	  }

	*endp = '\0';			/* remove last character */

	/* get stat block of shortname file, creating the file if necessary */

	if (stat (name, &shortstat)) 
	  {
	    if (errno == ENOENT) 
	      {		/* file not found, create */
		if ((shortfd = open (name, O_CREAT, 0666)) < 0)
		  if (errno == EACCES) 
		    {
		      if  ( (shortfd = open(name, O_RDONLY)) < 0)
			{
			  /*
			  ** If the name doesn't exist, then it
			  ** wasn't truncated to match either,
			  ** so we're OK.
			  */
			  sizechk = SMALLENOUGH;
			  goto done;
			}
		    }
		  else
		    {
		      ERROR;
		    }
		shortcreat = TRUE;
		if (fstat (shortfd, &shortstat))
		  {
		    ERROR;
		  }
	      }
	    else			/* stat error toolong can't handle */
	      {
		ERROR;
	      }
	  }

	/* iff the two names are not the same file, fullname will fit */
	/* since the two files are on the same device,	*/
	/*	a simple inode check is sufficient	*/

	if (fullstat.st_ino != shortstat.st_ino)
		sizechk = SMALLENOUGH;



done:	if (shortcreat && unlink (name))	/* rm shortname if created */
		error = TRUE;

	*endp = lastchar;			/* replace last character */

	if (fullcreat && unlink (name))		/* rm fullname if created */
		error = TRUE;

        if ( fullfd > 0 )                      /* close file descriptors */
	    close(fullfd);

        if ( shortfd > 0 )                      /* close file descriptors */
	    close(shortfd);

	if (error)
		perror ("toolong");
	return (sizechk);
}
