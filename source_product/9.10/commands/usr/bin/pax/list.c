/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/list.c,v $
 *
 * $Revision: 66.2 $
 *
 * list.c - List all files on an archive
 *
 * DESCRIPTION
 *
 *	These function are needed to support archive table of contents and
 *	verbose mode during extraction and creation of achives.
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
 * $Log:	list.c,v $
 * Revision 66.2  90/09/13  06:07:31  06:07:31  kawo
 * Changes in function read_header. It wasn't able to recreate a splitted
 * pathname when reading a "new tar format" archive.
 * 
 * Revision 66.1  90/05/11  08:52:42  08:52:42  michas (#Michael Sieber)
 * inital checkin
 * 
 * Revision 2.0.0.6  89/12/16  10:35:28  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.5  89/10/30  19:52:57  mark
 * patch1: Added some additional support for MS-DOS
 * 
 * Revision 2.0.0.4  89/10/30  07:01:45  mark
 * Added declaration of symnam
 * 
 * Revision 2.0.0.3  89/10/13  02:35:13  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: list.c,v 2.0.0.6 89/12/16 10:35:28 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Defines */

/*
 * isodigit returns non zero iff argument is an octal digit, zero otherwise
 */
#define	ISODIGIT(c)	(((c) >= '0') && ((c) <= '7'))


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

static void 		cpio_entry P((char *, Stat *));
static void 		tar_entry P((char *, Stat *));
static void 		pax_entry P((char *, Stat *));
static void 		pr_mode P((ushort));
static long 		from_oct P((int digs, char *where));

#undef P


/* Internal Identifiers */

static char        *monnames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/* read_header - read a header record
 *
 * DESCRIPTION
 *
 * 	Read a record that's supposed to be a header record. Return its 
 *	address in "head", and if it is good, the file's size in 
 *	asb->sb_size.  Decode things from a file header record into a "Stat". 
 *	Also set "head_standard" to !=0 or ==0 depending whether header record 
 *	is "Unix Standard" tar format or regular old tar format. 
 *
 * PARAMETERS
 *
 *	char   *name		- pointer which will contain name of file
 *	Stat   *asb		- pointer which will contain stat info
 *
 * RETURNS
 *
 * 	Return 1 for success, 0 if the checksum is bad, EOF on eof, 2 for a 
 * 	record full of zeros (EOF marker). 
 */

int
read_header(name, asb)
    char               *name;
    Stat               *asb;
{
    int                 i;
    long                sum;
    long                recsum;
    char               *p;
    char                hdrbuf[BLOCKSIZE];

    DBUG_ENTER("read_header");
    memset((char *) asb, 0, sizeof(Stat));
    /* read the header from the buffer */
    if (buf_read(hdrbuf, BLOCKSIZE) != 0) {
	DBUG_RETURN(EOF);
    }

    recsum = from_oct(8, &hdrbuf[148]);
    sum = 0;
    p = hdrbuf;
    for (i = 0; i < 500; i++) {

	/*
	 * We can't use unsigned char here because of old compilers, e.g. V7. 
	 */
	sum += 0xFF & *p++;
    }

    /* Adjust checksum to count the "chksum" field as blanks. */
    for (i = 0; i < 8; i++) {
	sum -= 0xFF & hdrbuf[148 + i];
    }
    sum += ' ' * 8;

    if (sum == 8 * ' ') {

	/*
	 * This is a zeroed record...whole record is 0's except for the 8
	 * blanks we faked for the checksum field. 
	 */
	DBUG_RETURN(2);
    }
    if (sum == recsum) {
	/*
	 * Good record.  Decode file size and return. 
	 */
	if (hdrbuf[156] != LNKTYPE) {
	    asb->sb_size = from_oct(1 + 12, &hdrbuf[124]);
	}
	asb->sb_mtime = from_oct(1 + 12, &hdrbuf[136]);
	asb->sb_mode = from_oct(8, &hdrbuf[100]);

	if (strcmp(&hdrbuf[257], TMAGIC) == 0) {
    	    strcpy(name, &hdrbuf[345]);
	    if (strlen(name) > 0)
    	    	strcat(name, "/");
    	    strcat(name, hdrbuf);
	    /* Unix Standard tar archive */
	    head_standard = 1;
	    asb->sb_uid = finduid(&hdrbuf[265]);
	    asb->sb_gid = findgid(&hdrbuf[297]);
	    switch (hdrbuf[156]) {
	    case BLKTYPE:
	    case CHRTYPE:
		asb->sb_rdev = makedev(from_oct(8, &hdrbuf[329]),
				       from_oct(8, &hdrbuf[337]));
		break;
	    default:
		/* do nothing... */
		break;
	    }
	} else {
    	    strcpy(name, hdrbuf);
	    /* Old fashioned tar archive */
	    head_standard = 0;
	    asb->sb_uid = from_oct(8, &hdrbuf[108]);
	    asb->sb_gid = from_oct(8, &hdrbuf[116]);
	}

	switch (hdrbuf[156]) {

	case REGTYPE:
	case AREGTYPE:
	    /*
	     * Berkeley tar stores directories as regular files with a
	     * trailing slash 
	     */
	    if (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = '\0';
		asb->sb_mode |= S_IFDIR;
	    } else {
		asb->sb_mode |= S_IFREG;
	    }
	    break;

	case LNKTYPE:
	    asb->sb_nlink = 2;
	    linkto(&hdrbuf[157], asb);
	    linkto(name, asb);
	    asb->sb_mode |= S_IFREG;
	    break;

#ifdef S_IFBLK
	case BLKTYPE:
	    asb->sb_mode |= S_IFBLK;
	    break;
#endif /* S_IFBLK */	    

#ifdef S_IFCHR	    
	case CHRTYPE:
	    asb->sb_mode |= S_IFCHR;
	    break;
#endif /* S_IFCHR */	    

	case DIRTYPE:
	    asb->sb_mode |= S_IFDIR;
	    break;

#ifdef S_IFLNK
	case SYMTYPE:
	    asb->sb_mode |= S_IFLNK;
	    strcpy(asb->sb_link, &hdrbuf[157]);
	    break;
#endif /* S_IFLNK */

#ifdef S_IFIFO
	case FIFOTYPE:
	    asb->sb_mode |= S_IFIFO;
	    break;
#endif /* S_IFIFO */

#ifdef S_IFCTG
	case CONTTYPE:
	    asb->sb_mode |= S_IFCTG;
	    break;
#endif
	}
	DBUG_RETURN(1);
    }
    DBUG_RETURN(0);
}


/* print_entry - print a single table-of-contents entry
 *
 * DESCRIPTION
 * 
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the LS command.  For some
 *	archive formats, various fields may not make any sense, such as
 *	the link count on tar archives.  No error checking is done for bad
 *	or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */

void
print_entry(name, asb)
    char               *name;
    Stat               *asb;
{
    DBUG_ENTER("print_entry");
    switch (ar_interface) {

    case TAR:
	tar_entry(name, asb);
	break;

    case CPIO:
	cpio_entry(name, asb);
	break;

    case PAX:
	pax_entry(name, asb);
	break;
    }
    DBUG_VOID_RETURN;
}


/* cpio_entry - print a verbose cpio-style entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the traditional cpio 
 *	command.  No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */

static void
cpio_entry(name, asb)
    char               *name;
    Stat               *asb;
{
    struct tm          *atm;
    Link               *from;
    struct passwd      *pwp;

    DBUG_ENTER("cpio_entry");
    if (f_list && f_verbose) {
	fprintf(msgfile, "%-7o", asb->sb_mode);
	atm = localtime(&asb->sb_mtime);
	if (pwp = getpwuid((UIDTYPE) USH(asb->sb_uid))) {
	    fprintf(msgfile, "%-6s", pwp->pw_name);
	} else {
	    fprintf(msgfile, "%-6u", USH(asb->sb_uid));
	}
	fprintf(msgfile, "%7ld  %3s %2d %02d:%02d:%02d %4d  ",
		asb->sb_size, monnames[atm->tm_mon],
		atm->tm_mday, atm->tm_hour, atm->tm_min,
		atm->tm_sec, atm->tm_year + 1900);
    }
    fprintf(msgfile, "%s", name);
    if ((asb->sb_nlink > 1) && (from = linkfrom(name, asb)) != (Link *)NULL) {
	fprintf(msgfile, " linked to %s", from->l_name);
    }
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	fprintf(msgfile, " symbolic link to %s", asb->sb_link);
    }
#endif				/* S_IFLNK */
    putc('\n', msgfile);
    DBUG_VOID_RETURN;
}


/* tar_entry - print a tar verbose mode entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of tar file information.  The format
 *	of the line is the same as that produced by the traditional tar 
 *	command.  No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */

static void
tar_entry(name, asb)
    char               *name;
    Stat               *asb;
{
    struct tm          *atm;
    int			i;
    Link               *link;
    int                 mode;
#ifdef S_IFLNK
    char                symnam[PATH_MAX];
#endif

    DBUG_ENTER("tar_entry");
    if ((mode = asb->sb_mode & S_IFMT) == S_IFDIR) {
	DBUG_VOID_RETURN;	/* don't print directories */
    }
    if (f_extract) {
	switch (mode) {

#ifdef S_IFLNK
	case S_IFLNK:		/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX - 1);
	    if (i < 0) {	/* Could not find symbolic link */
		warn("can't read symbolic link", strerror(errno));
	    } else {		/* Found symbolic link filename */
		symnam[i] = '\0';
		fprintf(msgfile, "x %s symbolic link to %s\n", name, symnam);
	    }
	    break;
#endif

	case S_IFREG:		/* It is a link or a file */
	    if ((asb->sb_nlink > 1) && 
		(link = linkfrom(name, asb)) != (Link *)NULL) {
		fprintf(msgfile, "%s linked to %s\n", name, link->l_name);
	    } else {
		fprintf(msgfile, "x %s, %ld bytes, %ld tape blocks\n",
			name, asb->sb_size, 
			ROUNDUP(asb->sb_size, BLOCKSIZE) / BLOCKSIZE);
	    }
	}
    } else if (f_append || f_create) {
	switch (mode) {

#ifdef S_IFLNK
	case S_IFLNK:		/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX - 1);
	    if (i < 0) {	/* Could not find symbolic link */
		warn("can't read symbolic link", strerror(errno));
	    } else {		/* Found symbolic link filename */
		symnam[i] = '\0';
		fprintf(msgfile, "a %s symbolic link to %s\n", name, symnam);
	    }
	    break;
#endif

	case S_IFREG:		/* It is a link or a file */
	    fprintf(msgfile, "a %s ", name);
	    if ((asb->sb_nlink > 1) && (link = linkfrom(name, asb))) {
		fprintf(msgfile, "link to %s\n", link->l_name);
	    } else {
		fprintf(msgfile, "%ld Blocks\n",
			ROUNDUP(asb->sb_size, BLOCKSIZE) / BLOCKSIZE);
	    }
	    break;
	}

    } else if (f_list) {
	if (f_verbose) {
	    atm = localtime(&asb->sb_mtime);
	    pr_mode(asb->sb_mode);
	    fprintf(msgfile, " %d/%d %6ld %3s %2d %02d:%02d %4d %s",
		    asb->sb_uid, asb->sb_gid, asb->sb_size,
		    monnames[atm->tm_mon], atm->tm_mday, atm->tm_hour,
		    atm->tm_min, atm->tm_year + 1900, name);
	} else {
	    fprintf(msgfile, "%s", name);
	}
	switch (mode) {

#ifdef S_IFLNK
	case S_IFLNK:		/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX - 1);
	    if (i < 0) {	/* Could not find symbolic link */
		warn("can't read symbolic link", strerror(errno));
	    } else {		/* Found symbolic link filename */
		symnam[i] = '\0';
		fprintf(msgfile, " symbolic link to %s", symnam);
	    }
	    break;
#endif

	case S_IFREG:		/* It is a link or a file */
	    if ((asb->sb_nlink > 1) && 
		(link = linkfrom(name, asb)) != (Link *) NULL) {
		fprintf(msgfile, " linked to %s", link->l_name);
	    }
	    break;		/* Do not print out directories */
	}

	fputc('\n', msgfile);
    } else {
	fprintf(msgfile, "? %s %ld blocks\n", name,
		ROUNDUP(asb->sb_size, BLOCKSIZE) / BLOCKSIZE);
    }
    DBUG_VOID_RETURN;
}


/* pax_entry - print a verbose cpio-style entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the LS command.  
 *	No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */

static void
pax_entry(name, asb)
    char               *name;
    Stat               *asb;
{
    struct tm          *atm;
    Link               *from;
    struct passwd      *pwp;
    struct group       *grp;

    DBUG_ENTER("pax_entry");
    if (f_list && f_verbose) {
	pr_mode(asb->sb_mode);
	fprintf(msgfile, " %3d", asb->sb_nlink);
	atm = localtime(&asb->sb_mtime);
	if (pwp = getpwuid((UIDTYPE) USH(asb->sb_uid))) {
	    fprintf(msgfile, " %-8s", pwp->pw_name);
	} else {
	    fprintf(msgfile, " %-8u", USH(asb->sb_uid));
	}
	if (grp = getgrgid((GIDTYPE) USH(asb->sb_gid))) {
	    fprintf(msgfile, " %-8s", grp->gr_name);
	} else {
	    fprintf(msgfile, " %-8u", USH(asb->sb_gid));
	}
	switch (asb->sb_mode & S_IFMT) {

#ifdef S_IFBLK	    
	case S_IFBLK:
#endif /* S_IFBLK */
	    
#ifdef S_IFCHR
	case S_IFCHR:
	    fprintf(msgfile, "\t%3d, %3d",
		    major(asb->sb_rdev), minor(asb->sb_rdev));
	    break;
#endif /* S_IFCHR */

	case S_IFREG:
	    fprintf(msgfile, "\t%8ld", asb->sb_size);
	    break;

	default:
	    fprintf(msgfile, "\t        ");
	}
	fprintf(msgfile, " %3s %2d %02d:%02d ",
		monnames[atm->tm_mon], atm->tm_mday,
		atm->tm_hour, atm->tm_min);
    }
    fprintf(msgfile, "%s", name);
    if ((asb->sb_nlink > 1) && (from = linkfrom(name, asb))) {
	fprintf(msgfile, " == %s", from->l_name);
    }
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	fprintf(msgfile, " -> %s", asb->sb_link);
    }
#endif				/* S_IFLNK */
    putc('\n', msgfile);
    DBUG_VOID_RETURN;
}


/* pr_mode - fancy file mode display
 *
 * DESCRIPTION
 *
 *	Pr_mode displays a numeric file mode in the standard unix
 *	representation, ala ls (-rwxrwxrwx).  No error checking is done
 *	for bad mode combinations.  FIFOS, sybmbolic links, sticky bits,
 *	block- and character-special devices are supported if supported
 *	by the hosting implementation.
 *
 * PARAMETERS
 *
 *	ushort	mode	- The integer representation of the mode to print.
 */

static void
pr_mode(mode)
    int			mode;
{
    DBUG_ENTER("pr_mode");
    /* Tar does not print the leading identifier... */
    if (ar_interface != TAR) {
	switch (mode & S_IFMT) {

	case S_IFDIR:
	    putc('d', msgfile);
	    break;

#ifdef	S_IFLNK
	case S_IFLNK:
	    putc('l', msgfile);
	    break;
#endif /* S_IFLNK */

	    
#ifdef	S_IFBLK
	case S_IFBLK:
	    putc('b', msgfile);
	    break;
#endif /* S_IFBLK */

#ifdef	S_IFCHR
	case S_IFCHR:
	    putc('c', msgfile);
	    break;
#endif /* S_IFCHR */

#ifdef	S_IFIFO
	case S_IFIFO:
	    putc('p', msgfile);
	    break;
#endif /* S_IFIFO */

	case S_IFREG:
	default:
	    putc('-', msgfile);
	    break;
	}
    }
    putc(mode & 0400 ? 'r' : '-', msgfile);
    putc(mode & 0200 ? 'w' : '-', msgfile);
    putc(mode & 0100
	 ? mode & 04000 ? 's' : 'x'
	 : mode & 04000 ? 'S' : '-', msgfile);
    putc(mode & 0040 ? 'r' : '-', msgfile);
    putc(mode & 0020 ? 'w' : '-', msgfile);
    putc(mode & 0010
	 ? mode & 02000 ? 's' : 'x'
	 : mode & 02000 ? 'S' : '-', msgfile);
    putc(mode & 0004 ? 'r' : '-', msgfile);
    putc(mode & 0002 ? 'w' : '-', msgfile);
    putc(mode & 0001
	 ? mode & 01000 ? 't' : 'x'
	 : mode & 01000 ? 'T' : '-', msgfile);
    DBUG_VOID_RETURN;
}


/* from_oct - quick and dirty octal conversion
 *
 * DESCRIPTION
 *
 *	From_oct will convert an ASCII representation of an octal number
 *	to the numeric representation.  The number of characters to convert
 *	is given by the parameter "digs".  If there are less numbers than
 *	specified by "digs", then the routine returns -1.
 *
 * PARAMETERS
 *
 *	int digs	- Number to of digits to convert 
 *	char *where	- Character representation of octal number
 *
 * RETURNS
 *
 *	The value of the octal number represented by the first digs
 *	characters of the string where.  Result is -1 if the field 
 *	is invalid (all blank, or nonoctal). 
 *
 * ERRORS
 *
 *	If the field is all blank, then the value returned is -1.
 *
 */

static long
from_oct(digs, where)
    int                 digs;	/* number of characters to convert */
    char               *where;	/* character representation of octal number */
{
    long                value;

    DBUG_ENTER("from_oct");
    while (isspace(*where)) {	/* Skip spaces */
	where++;
	if (--digs <= 0) {
	    DBUG_RETURN(-1);	/* All blank field */
	}
    }
    value = 0;
    while (digs > 0 && ISODIGIT(*where)) {	/* Scan til nonoctal */
	value = (value << 3) | (*where++ - '0');
	--digs;
    }

    if (digs > 0 && *where && !isspace(*where)) {
	DBUG_RETURN(-1);	/* Ended on non-space/nul */
    }
    DBUG_RETURN(value);
}
