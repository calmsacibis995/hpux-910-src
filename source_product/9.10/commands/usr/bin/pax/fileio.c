/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/fileio.c,v $
 *
 * $Revision: 66.3 $
 *
 * fileio.c - file I/O functions for all archive interfaces
 *
 * DESCRIPTION
 *
 *	These function all do I/O of some form or another.  They are
 *	grouped here mainly for convienence.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	fileio.c,v $
 * Revision 66.3  90/11/27  05:55:11  05:55:11  kawo
 * changes done for cnode handling
 * 
 * Revision 66.2  90/07/10  08:29:16  08:29:16  kawo (#Wolfgang Kapellen)
 * Changes for cnode specific device files in openout().
 * Changes for CDF-file handling in openout().
 * 
 * Revision 66.1  90/05/11  08:24:57  08:24:57  michas (#Michael Sieber)
 * inital checkin
 * 
 * Revision 2.0.0.5  89/12/16  10:35:18  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.4  89/10/30  07:51:34  mark
 * Added call to dio_to_binary to support real character devices under MSDOS.
 * 
 * Revision 2.0.0.3  89/10/13  02:34:59  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: fileio.c,v 2.0.0.5 89/12/16 10:35:18 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* open_archive -  open an archive file.  
 *
 * DESCRIPTION
 *
 *	Open_archive will open an archive file for reading or writing,
 *	setting the proper file mode, depending on the "mode" passed to
 *	it.  All buffer pointers are reset according to the mode
 *	specified.
 *
 * PARAMETERS
 *
 * 	int	mode 	- specifies whether we are reading or writing.   
 *
 * RETURNS
 *
 *	Returns a zero if successfull, or -1 if an error occured during 
 *	the open.
 */

int
open_archive(mode)
    int                 mode;
{
    DBUG_ENTER("open_archive");
    if (ar_file[0] == '-' && ar_file[1] == '\0') {
	if (mode == AR_READ) {
	    archivefd = STDIN;
	    bufend = bufidx = bufstart;
	} else {
	    archivefd = STDOUT;
	}
    } else if (mode == AR_READ) {
	archivefd = OPEN2(ar_file, O_RDONLY | O_BINARY);
	bufend = bufidx = bufstart;	/* set up for initial read */
    } else if (mode == AR_WRITE) {
	archivefd = OPEN3(ar_file, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY,
			 0666 & ~mask); 
    } else if (mode == AR_APPEND) {
	archivefd = OPEN3(ar_file, O_RDWR | O_BINARY, 0666 & ~mask);
	bufend = bufidx = bufstart;	/* set up for initial read */
    }
    
#ifndef MSDOS    
    if (archivefd < 0) {
	warnarch(strerror(errno), (OFFSET) 0);
	DBUG_RETURN(-1);
    }
#else /* MSDOS */
    if (dio_open_check(ar_file) < 0) {
	if (archivefd >= 0) {
	    warnarch(strerror(errno), (OFFSET) 0);
	    DBUG_RETURN(-1);
	    }
    } else {
        if (archivefd < 0) {
	    warnarch(strerror(errno), (OFFSET) 0);
	    DBUG_RETURN(-1);
    	}
    }
    if (dio_to_binary(archivefd) < 0) {
        DBUG_RETURN(-1);
    }
#endif /* MSDOS */
    
    ++arvolume;
    DBUG_RETURN(0);
}


/* close_archive - close the archive file
 *
 * DESCRIPTION
 *
 *	Closes the current archive and resets the archive end of file
 *	marker.
 */

void
close_archive()
{
    DBUG_ENTER("close_archive");
    if (archivefd != STDIN && archivefd != STDOUT) {
	/* FIXME: do error checking here */
	CLOSE(archivefd);
    }
    areof = 0;
    DBUG_VOID_RETURN;
}


/* openout - open an output file
 *
 * DESCRIPTION
 *
 *	Openo opens the named file for output.  The file mode and type are
 *	set based on the values stored in the stat structure for the file.
 *	If the file is a special file, then no data will be written, the
 *	file/directory/Fifo, etc., will just be created.  Appropriate
 *	permission may be required to create special files.
 *
 * PARAMETERS
 *
 *	char 	*name		- The name of the file to create
 *	Stat	*asb		- Stat structure for the file
 *	Link	*linkp;		- pointer to link chain for this file
 *	int	 ispass		- true if we are operating in "pass" mode
 *
 * RETURNS
 *
 * 	Returns the output file descriptor, 0 if no data is required or -1 
 *	if unsuccessful. Note that UNIX open() will never return 0 because 
 *	the standard input is in use. 
 */

int
openout(name, asb, linkp, ispass)
    char               *name;
    Stat               *asb;
    Link               *linkp;
    int                 ispass;
{
    int                 exists;
    int                 fd;
    ushort              perm;
    ushort              operm = 0;
    Stat                osb;

#ifdef	S_IFLNK
    int                 ssize;
    char                sname[PATH_MAX + 1];
#endif /* S_IFLNK */

    DBUG_ENTER("openout");
    if (exists = (LSTAT(name, &osb) == 0)) {
	if (ispass && osb.sb_ino == asb->sb_ino && osb.sb_dev == asb->sb_dev) {
	    warn(name, "Same file");
	    DBUG_RETURN(-1);
	} else if ((osb.sb_mode & S_IFMT) == (asb->sb_mode & S_IFMT)) {
	    operm = osb.sb_mode & S_IPERM;
	} else if (REMOVE(name, &osb) < 0) {
	    warn(name, strerror(errno));
	    DBUG_RETURN(-1);
	} else {
	    exists = 0;
	}
    }
    if (linkp) {
	if (exists) {
	    if (asb->sb_ino == osb.sb_ino && asb->sb_dev == osb.sb_dev) {
		DBUG_RETURN(0);
	    } else if (unlink(name) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    } else {
		exists = 0;
	    }
	}
	if (link(linkp->l_name, name) != 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) != 0 ||
			link(linkp->l_name, name) != 0) {
			warn(name, strerror(errno));
			DBUG_RETURN(-1);
		    }
		} else {
		    warn(name,
			 "Directories are not being created (-d option)");
		}
		DBUG_RETURN(0);
	    } else if (errno != EXDEV) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	} else {
	    DBUG_RETURN(0);
	}
    }
    perm = asb->sb_mode & S_IPERM;
    switch (asb->sb_mode & S_IFMT) {

#ifdef S_IFNWK	
    case S_IFNWK:
	fprintf(stderr, "<%s> is a network special file which is not restored.\n", name);
	break;
#endif /* S_IFNWK */

#ifdef S_IFBLK	
    case S_IFBLK:
#endif /* S_IFBLK */
#ifdef S_IFCHR	
    case S_IFCHR:
	fd = 0;
	if (exists) {
	    if (asb->sb_rdev == osb.sb_rdev) {
		if (perm != operm && chmod(name, (int) perm) < 0) {
		    warn(name, strerror(errno));
		    DBUG_RETURN(-1);
		} else {
		    break;
		}
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    } else {
		exists = 0;
	    }
	}

	{
#ifdef _CNODE_T
	int	rcnode_flag;
	if (asb->sb_rcnode >= 0 && asb->sb_rcnode <= 255) rcnode_flag = 1;
	else						  rcnode_flag = 0;

	if (mkrnod(name, asb->sb_mode, asb->sb_rdev,
		   (rcnode_flag ? asb->sb_rcnode : 0)     ) < 0) {
#else
	if (mknod(name, asb->sb_mode, asb->sb_rdev) < 0) {
#endif
	    if (errno == ENOENT) {
		if (f_dir_create) {
#ifdef _CNODE_T
		    if (dirneed(name) < 0 ||
			 mkrnod(name, asb->sb_mode, asb->sb_rdev,
		   		(rcnode_flag ? asb->sb_rcnode : 0)) < 0) {
#else
		    if (dirneed(name) < 0 || 
			mknod(name, asb->sb_mode, asb->sb_rdev) < 0) {
#endif
			warn(name, strerror(errno));
			DBUG_RETURN(-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	}
	DBUG_RETURN(0);
	break;
	}
#endif /* S_IFCHR */
	
    case S_IFDIR:
	if (exists) {
	    if (perm != operm && chmod(name, (int) perm) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	} else if (f_dir_create) {
	    if ( dirmake(name, asb, (int)0) < 0 || dirneed(name) < 0 ) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	} else {
	    warn(name, "Directories are not being created (-d option)");
	}
	DBUG_RETURN(0);

#ifdef	S_IFIFO
    case S_IFIFO:
	fd = 0;
	if (exists) {
	    if (perm != operm && chmod(name, (int) perm) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	    else {
		break;
	    }
	} 

	{
#ifdef _CNODE_T
	int	rcnode_flag;
	if (asb->sb_rcnode >= 0 && asb->sb_rcnode <= 255) rcnode_flag = 1;
	else						  rcnode_flag = 0;

	if (mkrnod(name, asb->sb_mode, asb->sb_rdev,
		   (rcnode_flag ? asb->sb_rcnode : 0)     ) < 0) {
#else
	if (mknod(name, asb->sb_mode, asb->sb_rdev) < 0) {
#endif
	    if (errno == ENOENT) {
		if (f_dir_create) {
#ifdef _CNODE_T
		    if (dirneed(name) < 0 ||
			 mkrnod(name, asb->sb_mode, asb->sb_rdev,
		   		(rcnode_flag ? asb->sb_rcnode : 0)) < 0) {
#else
		    if (dirneed(name) < 0 ||
			 mknod(name, asb->sb_mode, 0) < 0) {
#endif
			warn(name, strerror(errno));
			DBUG_RETURN(-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	}
	DBUG_RETURN(0);
	break;
	}
#endif /* S_IFIFO */

#ifdef	S_IFLNK
    case S_IFLNK:
	if (exists) {
	    if ((ssize = readlink(name, sname, sizeof(sname))) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    } else if (strncmp(sname, asb->sb_link, ssize) == 0) {
		DBUG_RETURN(0);
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    } else {
		exists = 0;
	    }
	}
	if (symlink(asb->sb_link, name) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || symlink(asb->sb_link, name) < 0) {
			warn(name, strerror(errno));
			DBUG_RETURN(-1);
		    }
		} else {
		    warn(name, "Directories are not being created (-d option)");
		}
	    } else {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	}
	DBUG_RETURN(0);		/* Can't chown()/chmod() a symbolic link */
#endif /* S_IFLNK */

    case S_IFREG:
	if (exists) {
	    if (!f_unconditional && osb.sb_mtime > asb->sb_mtime) {
		warn(name, "Newer file exists");
		DBUG_RETURN(-1);
	    } else if (unlink(name) < 0) {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    } else {
		exists = 0;
	    }
	}
	if ((fd = creat(name, (int) perm)) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 ||
			(fd = creat(name, (int) perm)) < 0) {
			warn(name, strerror(errno));
			DBUG_RETURN(-1);
		    }
		} else {
		    /*
		     * the file requires a directory which does not exist and
		     * which the user does not want created, so skip the
		     * file... 
		     */
		    warn(name, "Directories are not being created (-d option)");
		    DBUG_RETURN(0);
		}
	    } else {
		warn(name, strerror(errno));
		DBUG_RETURN(-1);
	    }
	}
	break;

    default:
	warn(name, "Unknown filetype");
	DBUG_RETURN(-1);
    }
    if (f_owner && (!exists || asb->sb_uid != osb.sb_uid ||
		    asb->sb_gid != osb.sb_gid)) {
	chown(name, (UIDTYPE) asb->sb_uid, (GIDTYPE) asb->sb_gid);
    }
    DBUG_RETURN(fd);
}


/* openin - open the next input file
 *
 * DESCRIPTION
 *
 *	Openi will attempt to open the next file for input.  If the file is
 *	a special file, such as a directory, FIFO, link, character- or
 *	block-special file, then the file size field of the stat structure
 *	is zeroed to make sure that no data is written out for the file.
 *	If the file is a special file, then a file descriptor of 0 is
 *	returned to the caller, which is handled specially.  If the file
 *	is a regular file, then the file is opened and a file descriptor
 *	to the open file is returned to the caller.
 *
 * PARAMETERS
 *
 *	char   *name	- pointer to the name of the file to open
 *	Stat   *asb	- pointer to the stat block for the file to open
 *
 * RETURNS
 *
 * 	Returns a file descriptor, 0 if no data exists, or -1 at EOF. This 
 *	kludge works because standard input is in use, preventing open() from 
 *	returning zero. 
 */

int
openin(name, asb)
    char               *name;	/* name of file to open */
    Stat               *asb;	/* ptr to stat structure for file */
{
    int                 fd;

    DBUG_ENTER("openin");
    switch (asb->sb_mode & S_IFMT) {

    case S_IFDIR:
	asb->sb_nlink = 1;
	asb->sb_size = 0;
	DBUG_RETURN(0);

#ifdef S_IFLNK
    case S_IFLNK:
	if ((asb->sb_size = readlink(name,
			     asb->sb_link, sizeof(asb->sb_link) - 1)) < 0) {
	    warn(name, strerror(errno));
	    DBUG_RETURN(0);
	}
	asb->sb_link[asb->sb_size] = '\0';
	DBUG_RETURN(0);
#endif /* S_IFLNK */

    case S_IFREG:
	if (asb->sb_size == 0) {
	    DBUG_RETURN(0);
	}
	if ((fd = open(name, O_RDONLY | O_BINARY)) < 0) {
	    warn(name, strerror(errno));
	}
	DBUG_RETURN(fd);

    default:
	asb->sb_size = 0;
	DBUG_RETURN(0);
    }
}
