/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/create.c,v $
 *
 * $Revision: 66.4 $
 *
 * create.c - Create a tape archive. 
 *
 * DESCRIPTION
 *
 *	These functions are used to create/write and archive from an set of
 *	named files.
 *
 * AUTHOR
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
 * $Log:	create.c,v $
 * Revision 66.4  90/11/27  04:49:17  04:49:17  kawo
 * changes done for cnode handling
 * 
 * Revision 66.3  90/07/13  02:51:26  02:51:26  kawo (#Wolfgang Kapellen)
 * Changhanges for CDF-files in writetar().
 * Now tar is able to restore CDFs archived by pax -H .
 * 
 * Revision 66.2  90/07/10  08:22:27  08:22:27  kawo (#Wolfgang Kapellen)
 * Changes for cnode specific device files in writecpio()
 * 
 * Revision 66.1  90/05/11  08:10:18  08:10:18  michas (#Michael Sieber)
 * inital checkin
 * 
 * Revision 2.0.0.5  89/12/16  10:34:58  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.4  89/10/13  02:34:36  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: create.c,v 2.0.0.5 89/12/16 10:34:58 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

static void	    writetar P((char *, Stat *));
static void	    writecpio P((char *, Stat *));
static char	    tartype P((int));

#undef P


/* create_archive - create a tar archive.
 *
 * DESCRIPTION
 *
 *	Create_archive is used as an entry point to both create and append
 *	archives.  Create archive goes through the files specified by the
 *	user and writes each one to the archive if it can.  Create_archive
 *	knows how to write both cpio and tar headers and the padding which
 *	is needed for each type of archive.
 *
 * RETURNS
 *
 *	Always returns 0
 */

int
create_archive()
{
    char                name[PATH_MAX + 1];
    Stat                sb;
    int                 fd;

    DBUG_ENTER("create_archive");
    while (name_next(name, &sb) != -1) {
	if ((fd = openin(name, &sb)) < 0) {
	    /* FIXME: pax wants to exit here??? */
	    continue;
	}
	/* check to see if name needs to have substitutions done on it */
	if (rplhead != (Replstr *) NULL) {
	    rpl_name(name);
	    if (strlen(name) == 0) {
		continue;
	    }
	}
	/* check for user dispositions of file */
	if (get_disposition("add", name) || get_newname(name, sizeof(name))) {
	    /* skip file... */
	    if (fd) {
		close(fd);
	    }
	    continue;
	}
	if (!f_link && sb.sb_nlink > 1) {
	    if (islink(name, &sb)) {
		sb.sb_size = 0;
	    }
	    linkto(name, &sb);
	}
	if (ar_format == TAR) {
	    writetar(name, &sb);
	} else {
	    writecpio(name, &sb);
	}
	if (fd) {
	    outdata(fd, name, sb.sb_size);
	}
	if (f_verbose) {
	    print_entry(name, &sb);
	}
    }

    write_eot();
    close_archive();
    DBUG_RETURN(0);
}


/* writetar - write a header block for a tar file
 *
 * DESCRIPTION
 *
 * 	Make a header block for the file name whose stat info is in st.  
 *	Return header pointer for success, NULL if the name is too long.
 *
 * 	The tar header block is structured as follows:
 *
 *		FIELD NAME	OFFSET		SIZE
 *      	-------------|---------------|------
 *		name		  0		100
 *		mode		100		  8
 *		uid		108		  8
 *		gid		116		  8
 *		size		124		 12
 *		mtime		136		 12
 *		chksum		148		  8
 *		typeflag	156		  1
 *		linkname	157		100
 *		magic		257		  6
 *		version		263		  2
 *		uname		265		 32
 *		gname		297		 32
 *		devmajor	329		  8
 *		devminor	337		  8
 *		prefix		345		155
 *
 * PARAMETERS
 *
 *	char	*name	- name of file to create a header block for
 *	Stat	*asb	- pointer to the stat structure for the named file
 *
 */

static void
writetar(name, asb)
    char               *name;	/* name of file to write */
    Stat               *asb;	/* stat block for file */
{
    char               *p;
    char               *prefix = (char *) NULL;
    int                 i;
    int                 sum;
    char                hdr[BLOCKSIZE];
    Link               *from;

    DBUG_ENTER("writetar");
    memset(hdr, 0, BLOCKSIZE);
    if (strlen(name) >= 255) {
	warn(name, "name too long");
	DBUG_VOID_RETURN;
    }

    /* 
     * FIXME: this has problems with split names or names that are long
     * Add a trailing slash if the file is a directory 
     * DONE on 07/12/90 by kawo 
     */
    if (  (S_ISDIR(asb->sb_mode)) &&
         !(S_ISCDF(asb->sb_mode))   ) {
	int	temporary;
	temporary = strlen(name);
	*(name + temporary    ) = '/';
	*(name + temporary + 1) = '\0';
    }

    /*
     * FIXME: If the pathname is longer than TNAMLEN, but less than 255, then
     * we can split it up into the prefix and the filename. 
     * DONE on 07/12/90 by kawo 
     */
    if (strlen(name) > 100) {
	prefix = name;
        /* add the correct offset to the pointer */
        if (strlen(name) > 155) name += 155;
        else                    name += strlen(name);

	while (name > prefix && *name != '/') {
	    name--;
	}

	/* no slash found....hmmm.... */
	if (name == prefix) {
	    warn(prefix, "Name too long");
	    DBUG_VOID_RETURN;
	}
	*name++ = '\0';

	/* may be the name is too long now */
        if (strlen(name) >= 100) {
	    warn(name, "Name too long after split");
	    DBUG_VOID_RETURN;
	}
    }
#ifdef S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	strcpy(&hdr[157], asb->sb_link);
	asb->sb_size = 0;
    }
#endif /* S_IFLNK */
    
    strcpy(hdr, name);

    /* sprintf(&hdr[100], "%07o\0", asb->sb_mode & ~S_IFMT); */
    sprintf(&hdr[100], "%07o\0", asb->sb_mode);
    sprintf(&hdr[108], "%07o\0", asb->sb_uid);
    sprintf(&hdr[116], "%07o\0", asb->sb_gid);
    sprintf(&hdr[124], "%011lo\0", (OFFSET) asb->sb_size);
    sprintf(&hdr[136], "%011lo\0", (long) asb->sb_mtime);
    strncpy(&hdr[148], "        ", 8);
    hdr[156] = tartype(asb->sb_mode);

    /*
     * Handle linked files here.  They can be a bit tricky though... 
     */
    if (asb->sb_nlink > 1 && (from = linkfrom(name, asb)) != (Link *) NULL) {
	if (strlen(from->l_name) > 99) {
	    /*
	     * the linkname is too long to store in the link field, therefore
	     * the link name is not store, and the initial file size is
	     * restored to the header. 
	     */
	    sprintf(&hdr[124], "%011lo\0", (OFFSET) from->l_size);
	} else {

	    /*
	     * The linkname will fit, put the linkname is the linkname field
	     * and set the header type to LINKTYPE. 
	     */
	    strcpy(&hdr[157], from->l_name);
	    hdr[156] = LNKTYPE;
	}
    }
    strcpy(&hdr[257], TMAGIC);
    strncpy(&hdr[263], TVERSION, 2);
    strcpy(&hdr[265], finduname((UIDTYPE) asb->sb_uid));
    strcpy(&hdr[297], findgname((GIDTYPE) asb->sb_gid));
    sprintf(&hdr[329], "%07o\0", major(asb->sb_rdev));
    sprintf(&hdr[337], "%07o\0", minor(asb->sb_rdev));
    if (prefix != (char *) NULL) {
	strncpy(&hdr[345], prefix, 155);
    }
    /* Calculate the checksum */

    sum = 0;
    p = hdr;
    for (i = 0; i < 500; i++) {
	sum += 0xFF & *p++;
    }

    /* Fill in the checksum field. */

    sprintf(&hdr[148], "%07o\0", sum);

    outwrite(hdr, BLOCKSIZE);
    DBUG_VOID_RETURN;
}


/* tartype - return tar file type from file mode
 *
 * DESCRIPTION
 *
 *	tartype returns the character which represents the type of file
 *	indicated by "mode". 
 *
 * PARAMETERS
 *
 *	int	mode	- file mode from a stat block
 *
 * RETURNS
 *
 *	The character which represents the particular file type in the 
 *	ustar standard headers.
 */

static char
tartype(mode)
    int                 mode;
{
    DBUG_ENTER("tartype");
    switch (mode & S_IFMT) {

#ifdef S_IFCTG
    case S_IFCTG:
	DBUG_RETURN(CONTTYPE);
#endif /* S_IFCTG */

    case S_IFDIR:
	DBUG_RETURN(DIRTYPE);

#ifdef S_IFLNK
    case S_IFLNK:
	DBUG_RETURN(SYMTYPE);
#endif /* S_IFLNK */

#ifdef S_IFIFO
    case S_IFIFO:
	DBUG_RETURN(FIFOTYPE);
#endif /* S_IFIFO */

#ifdef S_IFCHR
    case S_IFCHR:
	DBUG_RETURN(CHRTYPE);
#endif  /* S_IFCHR */

#ifdef S_IFBLK
    case S_IFBLK:
	DBUG_RETURN(BLKTYPE);
#endif /* S_IFBLK */

    default:
	DBUG_RETURN(REGTYPE);
    }
}


/* writecpio - write a cpio archive header
 *
 * DESCRIPTION
 *
 *	Writes a new CPIO style archive header for the file specified.
 *
 * PARAMETERS
 *
 *	char	*name	- name of file to create a header block for
 *	Stat	*asb	- pointer to the stat structure for the named file
 */

static void
writecpio(name, asb)
    char               *name;	/* name of file to create header block for */
    Stat               *asb;	/* pointer to the stat structure for 'name' */
{
    uint                namelen;
    char                header[M_STRLEN + H_STRLEN + 1];

    DBUG_ENTER("writecpio");
#ifdef _CNODE_T
    if ( (S_ISCHR(asb->sb_mode)) ||
	 (S_ISBLK(asb->sb_mode)) ||
	 (S_ISFIFO(asb->sb_mode))  )
	if ( (asb->sb_rcnode >= 0) &&
	     (asb->sb_rcnode <= 255) ) {
	    asb->sb_size = (off_t)asb->sb_rdev;
	    asb->sb_rdev = (dev_t)asb->sb_rcnode;
	}
	else
	    asb->sb_rdev = LOCALMAGIC;
    else
	asb->sb_rdev = 0;
#endif	/* _CNODE_T */
    namelen = (uint) strlen(name) + 1;
    strcpy(header, M_ASCII);
    sprintf(header + M_STRLEN, "%06o%06o%06o%06o%06o",
	    USH(asb->sb_dev), USH(asb->sb_ino), USH(asb->sb_mode),
	    USH(asb->sb_uid), USH(asb->sb_gid));
    sprintf(header + M_STRLEN + 30, "%06o%06o%011lo%06o%011lo",
	    USH(asb->sb_nlink), USH(asb->sb_rdev),
	    f_mtime ? asb->sb_mtime : time((time_t *) 0),
	    namelen, asb->sb_size);
    outwrite(header, M_STRLEN + H_STRLEN);
    outwrite(name, namelen);
    
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	outwrite(asb->sb_link, (uint) asb->sb_size);
    }
#endif /* S_IFLNK */
    
    DBUG_VOID_RETURN;
}
