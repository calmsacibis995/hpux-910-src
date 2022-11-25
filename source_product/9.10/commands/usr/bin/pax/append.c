/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/append.c,v $
 *
 * $Revision: 66.1 $
 *
 * append.c - append to a tape archive. 
 *
 * DESCRIPTION
 *
 *	Routines to allow appending of archives
 *
 * AUTHORS
 *
 *     	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
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
 * $Log:	append.c,v $
 * Revision 66.1  90/05/11  08:04:33  08:04:33  michas
 * inital checkin
 * 
 * Revision 2.0.0.6  89/12/16  10:33:47  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.5  89/10/30  19:51:53  mark
 * patch1: Added some additional support for MS-DOS
 * 
 * Revision 2.0.0.4  89/10/30  07:42:36  mark
 * Added <sys/mtio.h> for MSDOS
 * 
 * Revision 2.0.0.3  89/10/13  02:34:15  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: append.c,v 2.0.0.6 89/12/16 10:33:47 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"
#ifdef MTIOCTOP
#include <sys/mtio.h>
#endif


#ifdef ___STDC__
#define P(x)    x
#else
#define P(x)	()
#endif
    
static int	    back_space P((int));

#undef P    

/* append_archive - main loop for appending to a tar archive
 *
 * DESCRIPTION
 *
 *	Append_archive reads an archive until the end of the archive is
 *	reached once the archive is reached, the buffers are reset and the
 *	create_archive function is called to handle the actual writing of
 *	the appended archive data.  This is quite similar to the
 *	read_archive function, however, it does not do all the processing.
 */

void
append_archive()
{
    Stat                sb;
    char                name[PATH_MAX + 1];
    int             	blk;
    OFFSET          	btotal = 0;
    OFFSET		skp;
    
    DBUG_ENTER("append_archive");
    name[0] = '\0';
    while (get_header(name, &sb) == 0) {
	skp = (ar_format == TAR)
	    ? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE))
		: buf_skip((OFFSET) sb.sb_size);
	if (buf_skip(skp) < 0) {
	    warn(name, "File data is corrupt");
	}
	/* the tar file header is written out in on BLOCKSIZE block */
	btotal += skp + BLOCKSIZE;
    }
    /* we have now gotten to the end of the archive... */
    
    /* backspace to the end of the last archive */
    if (ar_format == TAR) {
        blk = (OFFSET) (bufend - bufstart - (btotal % blocksize)) / BLOCKSIZE;
    	blk = back_space(blk);
    	if (blk < 0) {
    	    warn("can't backspace", "append archive may be no good");
	}
    }

    /* reset the buffer now that we have read the entire archive */
    bufend = bufidx = bufstart;
    create_archive();
    DBUG_VOID_RETURN;
}


/*
 * backspace - backspace a certain number of records on a file
 *
 * RETURNS
 *
 *	The number of bytes backed up, or -1 if an error occured.
 *
 * CAVEATS
 * 
 *	A tar append without a warning message does not mean sucess, to be
 *	sure, do a tar tv to see if your device can support backspacing.
 */

static int 
back_space(n)
    int                 n;
{
    OFFSET              pos;

#ifdef MTIOCTOP
    struct mtop         mt;

    mt.mt_op = MTBSR;
    mt.mt_count = n;
    if (ioctl(archivefd, MTIOCTOP, &mt) < 0) {
	if (errno == EIO) {	/* try again */
	    if (ioctl(archivefd, MTIOCTOP, &mt) < 0) {
		warn(strerror(errno),
		     "probably can't backspace on device");
		return (-1);
	    }
	}
    } else {
	return (n);
    }
#endif /* MTIOCTOP */
    pos = LSEEK(archivefd, (OFFSET) - n * BLOCKSIZE, 1);
    if (pos == (OFFSET) -1) {
	warn(strerror(errno), "probably can't backspace on device");
	return (-1);
    }
    return (n);
}
