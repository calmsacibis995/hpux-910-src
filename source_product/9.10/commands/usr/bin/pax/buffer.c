/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/buffer.c,v $
 *
 * $Revision: 70.1 $
 *
 * buffer.c - Buffer management functions
 *
 * DESCRIPTION
 *
 *	These functions handle buffer manipulations for the archiving
 *	formats.  Functions are provided to get memory for buffers, 
 *	flush buffers, read and write buffers and de-allocate buffers.  
 *	Several housekeeping functions are provided as well to get some 
 *	information about how full buffers are, etc.
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
 * $Log:	buffer.c,v $
 * Revision 70.1  91/09/10  16:39:11  16:39:11  ssa
 * Author: lkc@hpfclkc.fc.hp.com
 * changed lseek from < 0 to == -1 for SDS
 * 
 * Revision 66.2  90/09/13  05:57:58  05:57:58  kawo
 * made casting on function return value because of compiler warning
 * 
 * Revision 66.1  90/05/11  08:05:45  08:05:45  michas (#Michael Sieber)
 * 
 * inital checkin
 * 
 * Revision 2.0.0.5  89/12/16  10:34:50  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.4  89/10/30  07:46:05  mark
 * Fixed endless loop condition in outflush() when write returns 0 for
 * end of volume.
 * 
 * Revision 2.0.0.3  89/10/13  02:34:27  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: buffer.c,v 2.0.0.5 89/12/16 10:34:50 mark Exp Locker: mark $";
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

static int	    ar_write P((int, char *, uint));
static void 	    buf_pad P((OFFSET));
static int 	    indata P((int, OFFSET, char *));
static void	    outflush P((void));
static void 	    buf_use P((uint));
static int 	    buf_in_avail P((char **, uint *));
static uint 	    buf_out_avail P((char **));

#undef P
    

/* inentry - install a single archive entry
 *
 * DESCRIPTION
 *
 *	Inentry reads an archive entry from the archive file and writes it
 *	out the the named file.  If we are in PASS mode during archive
 *	processing, the pass() function is called, otherwise we will
 *	extract from the archive file.
 *
 *	Inentry actaully calls indata to process the actual data to the
 *	file.
 *
 * PARAMETERS
 *
 *	char	*name	- name of the file to extract from the archive
 *	Stat	*asb	- stat block of the file to be extracted from the
 *			  archive.
 *
 * RETURNS
 *
 * 	Returns zero if successful, -1 otherwise. 
 */

int
inentry(name, asb)
    char               *name;
    Stat               *asb;
{
    Link               *linkp;
    int                 ifd;
    int                 ofd;
    time_t              tstamp[2];

    DBUG_ENTER("inentry");
    if ((ofd = openout(name, asb, linkp = linkfrom(name, asb), 0)) > 0) {
	if (asb->sb_size || linkp == (Link *) NULL || linkp->l_size == 0) {
	    /* FIXME: do error checking here */
	    close(indata(ofd, asb->sb_size, name));
	} else if ((ifd = open(linkp->l_path->p_name, O_RDONLY)) < 0) {
	    warn(linkp->l_path->p_name, strerror(errno));
	} else {
	    passdata(linkp->l_path->p_name, ifd, name, ofd);
	    /* FIXME: do error checking here */
	    close(ifd);
	    close(ofd);
	}
    } else {
	DBUG_RETURN(buf_skip((OFFSET) asb->sb_size) >= 0);
    }
    tstamp[0] = (!f_pass && f_access_time) ? asb->sb_atime :
	time((time_t *) 0);
    tstamp[1] = f_mtime ? asb->sb_mtime : time((time_t *) 0);
    utime(name, tstamp);
    DBUG_RETURN(0);
}


/* outdata - write archive data
 *
 * DESCRIPTION
 *
 *	Outdata transfers data from the named file to the archive buffer.
 *	It knows about the file padding which is required by tar, but no
 *	by cpio.  Outdata continues after file read errors, padding with 
 *	null characters if neccessary.   Closes the input file descriptor 
 *	when finished.
 *
 * PARAMETERS
 *
 *	int	fd	- file descriptor of file to read data from
 *	char   *name	- name of file
 *	OFFSET	size	- size of the file
 *
 */

void
outdata(fd, name, size)
    int                 fd;	/* open file descriptor for file */
    char               *name;	/* name of file to read */
    OFFSET              size;	/* size of file in bytes */
{
    uint                chunk;
    int                 got;	/* bytes returned from read */
    int                 oops;	/* error flag */
    uint                avail;	/* number of bytes in output buffer */
    OFFSET              pad;	/* number of padding bytes used */
    char               *buf;	/* pointer to output data buffer */

    DBUG_ENTER("outdata");
    oops = got = 0;
    if (pad = (size % BLOCKSIZE)) {
	pad = (BLOCKSIZE - pad);
    }
    while (size) {
	avail = buf_out_avail(&buf);
	size -= (chunk = size < avail ? (uint) size : avail);
	if (oops == 0 && (got = read(fd, buf, (unsigned int) chunk)) < 0) {
	    oops = -1;
	    warn(name, strerror(errno));
	    got = 0;
	}
	if (got < chunk) {
	    if (oops == 0) {
		oops = -1;
	    }
	    warn(name, "Early EOF");
	    while (got < chunk) {
		buf[got++] = '\0';
	    }
	}
	buf_use(chunk);
    }
    /* FIXME: do error checking here */
    close(fd);
    if (ar_format == TAR) {
	buf_pad((OFFSET) pad);
    }
    DBUG_VOID_RETURN;
}


/* write_eot -  write the end of archive record(s)
 *
 * DESCRIPTION
 *
 *	Write out an End-Of-Tape record.  We actually zero at least one 
 *	record, through the end of the block.  Old tar writes garbage after 
 *	two zeroed records -- and PDtar used to.
 */

void
write_eot()
{
    OFFSET              pad;	/* number of padding bytes used */
    char                header[M_STRLEN + H_STRLEN + 1];

    DBUG_ENTER("write_eot");
    if (ar_format == TAR) {
	/* write out two zero blocks for trailer */
	pad = 2 * BLOCKSIZE;
    } else {
	if (pad = (total + M_STRLEN + H_STRLEN + TRAILZ) % BLOCKSIZE) {
	    pad = BLOCKSIZE - pad;
	}
	strcpy(header, M_ASCII);
	sprintf(header + M_STRLEN, H_PRINT, 0, 0,
		0, 0, 0, 1, 0, (time_t) 0, TRAILZ, pad);
	outwrite(header, M_STRLEN + H_STRLEN);
	outwrite(TRAILER, TRAILZ);
    }
    buf_pad((OFFSET) pad);
    outflush();
    DBUG_VOID_RETURN;
}


/* outwrite -  write archive data
 *
 * DESCRIPTION
 *
 *	Writes out data in the archive buffer to the archive file.  The
 *	buffer index and the total byte count are incremented by the number
 *	of data bytes written.
 *
 * PARAMETERS
 *	
 *	char   *idx	- pointer to data to write
 *	uint	len	- length of the data to write
 */

void
outwrite(idx, len)
    char               *idx;	/* pointer to data to write */
    uint                len;	/* length of data to write */
{
    uint                have;
    uint                want;
    char               *endx;	/* pointer to end of buffer */

    DBUG_ENTER("outwrite");
    endx = idx + len;
    while (want = endx - idx) {
	if (bufend - bufidx < 0) {
	    fatal("Buffer overlow in out_write\n");
	}
	if ((have = bufend - bufidx) == 0) {
	    outflush();
	}
	if (have > want) {
	    have = want;
	}
	memcpy(bufidx, idx, (int) have);
	bufidx += have;
	idx += have;
	total += have;
    }
    DBUG_VOID_RETURN;
}


/* passdata - copy data to one file
 *
 * DESCRIPTION
 *
 *	Copies a file from one place to another.  Doesn't believe in input 
 *	file descriptor zero (see description of kludge in openin() comments). 
 *	Closes the provided output file descriptor. 
 *
 * PARAMETERS
 *
 *	char	*from	- input file name (old file)
 *	int	ifd	- input file descriptor
 *	char	*to	- output file name (new file)
 *	int	ofd	- output file descriptor
 */

void
passdata(from, ifd, to, ofd)
    char               *from;	/* input file name */
    int                 ifd;	/* input file descriptor */
    char               *to;	/* output file name */
    int                 ofd;	/* output file descriptor */
{
    int                 got;
    int                 sparse;
    char                block[BUFSIZ];

    DBUG_ENTER("passdata");
    if (ifd) {
	lseek(ifd, (OFFSET) 0, 0);
	sparse = 0;
	while ((got = read(ifd, block, sizeof(block))) > 0
	       && (sparse = ar_write(ofd, block, (uint) got)) >= 0) {
	    total += got;
	}
	if (got) {
	    warn(got < 0 ? from : to, strerror(errno));
	} else if (sparse > 0
		   && (lseek(ofd, (OFFSET) (-sparse), 1) == -1
		       || write(ofd, block, (uint) sparse) != sparse)) {
	    warn(to, strerror(errno));
	}
    }
    /* FIXME: do error checking here */
    close(ofd);
    DBUG_VOID_RETURN;
}


/* buf_allocate - get space for the I/O buffer 
 *
 * DESCRIPTION
 *
 *	buf_allocate allocates an I/O buffer using malloc.  The resulting
 *	buffer is used for all data buffering throughout the program.
 *	Buf_allocate must be called prior to any use of the buffer or any
 *	of the buffering calls.
 *
 * PARAMETERS
 *
 *	int	size	- size of the I/O buffer to request
 *
 * ERRORS
 *
 *	If an invalid size is given for a buffer or if a buffer of the 
 *	required size cannot be allocated, then the function prints out an 
 *	error message and returns a non-zero exit status to the calling 
 *	process, terminating the program.
 *
 */

void
buf_allocate(size)
    OFFSET              size;	/* size of block to allocate */
{
    DBUG_ENTER("buf_allocate");
    if (size <= 0) {
	fatal("invalid value for blocksize");
    }
    if ((bufstart = malloc((unsigned) size)) == (char *) NULL) {
	fatal("Cannot allocate I/O buffer");
    }
    bufend = bufidx = bufstart;
    bufend += size;
    DBUG_VOID_RETURN;
}


/* buf_skip - skip input archive data
 *
 * DESCRIPTION
 *
 *	Buf_skip skips past archive data.  It is used when the length of
 *	the archive data is known, and we do not wish to process the data.
 *
 * PARAMETERS
 *
 *	OFFSET	len	- number of bytes to skip
 *
 * RETURNS
 *
 * 	Returns zero under normal circumstances, -1 if unreadable data is 
 * 	encountered. 
 */

int
buf_skip(len)
    OFFSET              len;	/* number of bytes to skip */
{
    uint                chunk;
    int                 corrupt = 0;

    DBUG_ENTER("buf_skip");
    while (len) {
	if (bufend - bufidx < 0) {
	    fatal("Buffer overlow in buf_skip\n");
	}
	while ((chunk = bufend - bufidx) == 0) {
	    corrupt |= ar_read();
	}
	if (chunk > len) {
	    chunk = len;
	}
	bufidx += chunk;
	len -= chunk;
	total += chunk;
    }
    DBUG_RETURN(corrupt);
}


/* buf_read - read a given number of characters from the input archive
 *
 * DESCRIPTION
 *
 *	Reads len number of characters from the input archive and
 *	stores them in the buffer pointed at by dst.
 *
 * PARAMETERS
 *
 *	char   *dst	- pointer to buffer to store data into
 *	uint	len	- length of data to read
 *
 * RETURNS
 *
 * 	Returns zero with valid data, -1 if unreadable portions were 
 *	replaced by null characters. 
 */

int
buf_read(dst, len)
    char               *dst;
    uint                len;
{
    int                 have;
    int                 want;
    int                 corrupt = 0;
    char               *endx = dst + len;

    DBUG_ENTER("buf_read");
    while (want = endx - dst) {
	if (bufend - bufidx < 0) {
	    fatal("Buffer overlow in buf_read\n");
	}
	while ((have = bufend - bufidx) == 0) {
	    have = 0;
	    corrupt |= ar_read();
	}
	if (have > want) {
	    have = want;
	}
	memcpy(dst, bufidx, have);
	bufidx += have;
	dst += have;
	total += have;
    }
    DBUG_RETURN(corrupt);
}


/* indata - install data from an archive
 *
 * DESCRIPTION
 *
 *	Indata writes size bytes of data from the archive buffer to the output 
 *	file specified by fd.  The filename which is being written, pointed
 *	to by name is provided only for diagnostics.
 *
 * PARAMETERS
 *
 *	int	fd	- output file descriptor
 *	OFFSET	size	- number of bytes to write to output file
 *	char	*name	- name of file which corresponds to fd
 *
 * RETURNS
 *
 * 	Returns given file descriptor. 
 */

static int
indata(fd, size, name)
    int                 fd;	/* output file descriptor */
    OFFSET              size;	/* number of bytes to write to output file */
    char               *name;	/* name of file which corresponds to fd */
{
    uint                chunk;
    char               *oops;
    int                 sparse;
    int                 corrupt;
    char               *buf;
    uint                avail;

    DBUG_ENTER("indata");
    corrupt = sparse = 0;
    oops = (char *) NULL;
    while (size) {
	corrupt |= buf_in_avail(&buf, &avail);
	size -= (chunk = size < avail ? (uint) size : avail);
	if (oops == (char *) NULL && (sparse = ar_write(fd, buf, chunk)) < 0) {
	    oops = (char *)strerror(errno);
	}
	buf_use(chunk);
    }
    if (corrupt) {
	warn(name, "Corrupt archive data");
    }
    if (oops) {
	warn(name, oops);
    } else if (sparse > 0 && (lseek(fd, (OFFSET) - 1, 1) == -1
			      || write(fd, "", 1) != 1)) {
	warn(name, strerror(errno));
    }
    DBUG_RETURN(fd);
}


/* outflush - flush the output buffer
 *
 * DESCRIPTION
 *
 *	The output buffer is written, if there is anything in it, to the
 *	archive file.
 */

static void
outflush()
{
    char               *buf;
    int                 got;
    uint                len;

    DBUG_ENTER("outflush");
    for (buf = bufstart; len = bufidx - buf;) {
	if ((got = WRITE(archivefd, buf, MIN(len, blocksize))) > 0) {
	    buf += got;
	} else if (got <= 0) {
	    next(AR_WRITE);
	}
    }
    bufend = (bufidx = bufstart) + blocksize;
    DBUG_VOID_RETURN;
}


/* ar_read - fill the archive buffer
 *
 * DESCRIPTION
 *
 * 	Remembers mid-buffer read failures and reports them the next time 
 *	through.  Replaces unreadable data with null characters.   Resets
 *	the buffer pointers as appropriate.
 *
 * RETURNS
 *
 *	Returns zero with valid data, -1 otherwise. 
 */

int
ar_read()
{
    int                 got;
    static int          failed;

    DBUG_ENTER("ar_read");
    bufend = bufidx = bufstart;
    if (!failed) {
	if (areof) {
	    if (total == 0) {
		fatal("No input");
	    } else {
		next(AR_READ);
	    }
	}
	while (!failed && !areof && bufstart + blocksize - bufend >= blocksize) {
	    if ((got = READ(archivefd, bufend, (unsigned int) blocksize)) > 0) {
		bufend += got;
	    } else if (got < 0) {
		failed = -1;
		warnarch(strerror(errno), (OFFSET) 0 - (bufend - bufidx));
	    } else {
		++areof;
	    }
	}
    }
    if (failed && bufend == bufstart) {
	failed = 0;
	for (got = 0; got < blocksize; ++got) {
	    *bufend++ = '\0';
	}
	DBUG_RETURN(-1);
    }
    DBUG_RETURN(0);
}


/* ar_write - write a filesystem block
 *
 * DESCRIPTION
 *
 * 	Writes len bytes of data data from the specified buffer to the 
 *	specified file.   Seeks past sparse blocks. 
 *
 * PARAMETERS
 *
 *	int     fd	- file to write to
 *	char   *buf	- buffer to get data from
 *	uint	len	- number of bytes to transfer
 *
 * RETURNS
 *
 *	Returns 0 if the block was written, the given length for a sparse 
 *	block or -1 if unsuccessful. 
 */

static int 
ar_write(fd, buf, len)
    int                 fd;	/* file to write to */
    char               *buf;	/* buffer to get data from */
    uint                len;	/* number of bytes to transfer */
{
    char               *bidx;
    char               *bend;

    DBUG_ENTER("ar_write");
    bend = (bidx = buf) + len;
    while (bidx < bend) {
	if (*bidx++) {
	    DBUG_RETURN(write(fd, buf, len) == len ? 0 : -1);
	}
    }
    DBUG_RETURN(lseek(fd, (OFFSET) len, 1) == -1 ? -1 : len);
}


/* buf_pad - pad the archive buffer
 *
 * DESCRIPTION
 *
 *	Buf_pad writes len zero bytes to the archive buffer in order to 
 *	pad it.
 *
 * PARAMETERS
 *
 *	OFFSET	pad	- number of zero bytes to pad
 *
 */

static void
buf_pad(pad)
    OFFSET              pad;	/* number of zero bytes to pad */
{
    int                 idx;
    int                 have;

    DBUG_ENTER("buf_pad");
    while (pad) {
	if ((have = bufend - bufidx) > pad) {
	    have = pad;
	}
	for (idx = 0; idx < have; ++idx) {
	    *bufidx++ = '\0';
	}
	total += have;
	pad -= have;
	if (bufend - bufidx == 0) {
	    outflush();
	}
    }
    DBUG_VOID_RETURN;
}


/* buf_use - allocate buffer space
 *
 * DESCRIPTION
 *
 *	Buf_use marks space in the buffer as being used; advancing both the
 *	buffer index (bufidx) and the total byte count (total).
 *
 * PARAMETERS
 *
 *	uint	len	- Amount of space to allocate in the buffer
 */

static void
buf_use(len)
    uint                len;	/* amount of space to allocate in buffer */
{
    DBUG_ENTER("buf_use");
    bufidx += len;
    total += len;
    DBUG_VOID_RETURN;
}


/* buf_in_avail - index available input data within the buffer
 *
 * DESCRIPTION
 *
 *	Buf_in_avail fills the archive buffer, and points the bufp
 *	parameter at the start of the data.  The lenp parameter is
 *	modified to contain the number of bytes which were read.
 *
 * PARAMETERS
 *
 *	char   **bufp	- pointer to the buffer to read data into
 *	uint	*lenp	- pointer to the number of bytes which were read
 *			  (returned to the caller)
 *
 * RETURNS
 *
 * 	Stores a pointer to the data and its length in given locations. 
 *	Returns zero with valid data, -1 if unreadable portions were 
 *	replaced with nulls. 
 *
 * ERRORS
 *
 *	If an error occurs in ar_read, the error code is returned to the
 *	calling function.
 *
 */

static int
buf_in_avail(bufp, lenp)
    char              **bufp;
    uint               *lenp;
{
    uint                have;
    int                 corrupt = 0;

    DBUG_ENTER("buf_in_avail");
    while ((have = bufend - bufidx) == 0) {
	corrupt |= ar_read();
    }
    *bufp = bufidx;
    *lenp = have;
    DBUG_RETURN(corrupt);
}


/* buf_out_avail  - index buffer space for archive output
 *
 * DESCRIPTION
 *
 * 	Stores a buffer pointer at a given location. Returns the number 
 *	of bytes available. 
 *
 * PARAMETERS
 *
 *	char	**bufp	- pointer to the buffer which is to be stored
 *
 * RETURNS
 *
 * 	The number of bytes which are available in the buffer.
 *
 */

static              uint
buf_out_avail(bufp)
    char              **bufp;
{
    int                 have;

    DBUG_ENTER("buf_out_avail");
    if (bufend - bufidx < 0) {
	fatal("Buffer overlow in buf_out_avail\n");
    }
    if ((have = bufend - bufidx) == 0) {
	outflush();
    }
    *bufp = bufidx;
    DBUG_RETURN(have);
}
