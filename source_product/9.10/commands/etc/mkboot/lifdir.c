/* $Source: /misc/source_product/9.10/commands.rcs/etc/mkboot/lifdir.c,v $
 * $Revision: 70.2 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/06/30 12:26:42 $
 */

#include <stdio.h>
#include <sys/param.h>
#include <fcntl.h>
#include "lifio.h"
#include "global.h"
#include "lifdir.h"

/*
 * Provide a way of accessing and creating LIF
 * directories so that character devices are supported, and so that
 * tape devices can be used with minimal rewinds.  The directory
 * remains in memory until closed.  All IO's are LIFBUFSIZE bytes
 * long (currently 2K).
 *
 * Primitives:
 *	open_lifdir()	- open device/file and set up data structures
 *	new_lifentry()	- get memory for the next lif entry
 *	read_lifentry()	- retrieve the next lif entry
 *	close_lifdir()	- if needed, flush_lifdir() and close device/file
 *
 * Additional functions:
 *	scan_lifdir()	- search what's in memory for a named entry
 *	flush_lifdir()	- make lif header and directory consistent
 *			  and correct and write them out
 *	reset_lifdir()	- change access to the directory from APPEND
 *			  to READONLY or vice versa
 *	lifstrcmp()	- compare LIF file names against each other
 *			  or normal C strings
 *	lifstrcpy()	- Copy a C string into a LIF file name
 *
 * Data structures:
 *	lif_directory	- descriptor for lif directory access
 *	dlist		- buffers containing the directory
 *
 * These routines may be used to modify an existing lif directory,
 * but you can add entries only to the end of the directory and the
 * directory size cannot increase.  If you modify an existing
 * directory entry, you are responsible for setting the start
 * location of that file and for making sure it doesn't overlap
 * other lif files.  The start locations of files corresponding
 * to appended entries are calculated automatically (2K aligned)
 * by flush_lifdir() (which is called by close_lifdir()).  Since
 * flush_lifdir() assumes the directory is sorted when it does this
 * calculation, deciding on a starting location for modified entries
 * is tricky, so just appending new entries is preferred.
 *
 * ASSUME:  N * sizeof(struct dentry) == LIFSECTORSIZE
 *	    M * LIFSECTORSIZE == LIFBUFSIZE
 */

#define DLIST_ENTRY(dlist,i)	(&(dlist)->dentry[(i)])

struct lif_directory *open_lifdir();
struct dentry *new_lifentry();
struct dentry *read_lifentry();
struct dentry *scan_lifdir();
void flush_lifdir();
void reset_lifdir();
void close_lifdir();
struct dlist *get_newdirbuf();
int lifstrcmp();
char *lifstrcpy();
void Read();
void Write();
void Lseek();


/*
 * Prepare to dole out one directory entry at a time.
 *
 * Returns a descriptor for the lif directory.
 *
 * To support character devices, we need to do "large" IO's.
 * Since we 2K align all files anyway, just do 2K IO's.  This is
 * double DEV_BSIZE, so IO's to or from most devices should be OK.
 * This should be fine for tape devices also.
 */
struct lif_directory *
open_lifdir(file, flag)
char	*file;
int	flag;
{
    struct lif_directory	*ldp;
    struct dentry	*dentry;
    int			dirsize;
    int			status;
    int			oflag;
    int			i;

    /* Validate flag */
    switch (flag) {
    case APPEND:
	    oflag = O_RDWR;
	    break;
    case READONLY:
	    oflag = O_RDONLY;
	    break;
    default:
	    fprintf(stderr, "open_lifdir: bad flag 0x%x\n", flag);
	    exit(1);
    }

    /* Allocate the new directory struct */
    if (!(ldp = (struct lif_directory *)malloc(sizeof(*ldp)))) {
	fprintf(stderr, "Error opening LIF directory on ");
	perror(file);
	exit(1);
    }
    bzero(ldp, sizeof(*ldp));
    strncpy(ldp->name, file, sizeof(ldp->name));
    ldp->flag = flag;
    ldp->hdr = (struct lvol *)&ldp->buf[0];	/* Examine as a header */

    /* Open the device (for simplicity, open r/w in all cases) */
    ldp->fd = Open(file, oflag);

    /* Read the header */
    Read(ldp->fd, (char *)ldp->hdr, MAKEBUF(*ldp->hdr), ldp->name);

    if (flag == APPEND) {
	/* Allocate a buffer's worth of directory */
	ldp->tail = get_newdirbuf(ldp);
    }
    else {	/* flag == READONLY */
	if (!valid_lif(ldp->hdr)) {
	    fprintf(stderr, "%s does not contain a valid lif\n", file);
	    exit(1);
	}

	/* Allocate a buffer for the whole directory */
	dirsize = ROUNDUP(ldp->hdr->dsize, LIFALIGNMENT) * LIFSECTORSIZE;
	ldp->tail = (struct dlist *)malloc(dirsize + sizeof(*ldp->tail) -
					   sizeof(ldp->tail->dentry));
	if (!ldp->tail) {
	    fprintf(stderr, "Error opening LIF directory on ");
	    perror(file);
	    exit(1);
	}
	ldp->tail->numentries = dirsize / sizeof(struct dentry);
	ldp->tail->next = (struct dlist *)0;

	/* Read in the whole directory */
	Lseek(ldp->fd, ldp->hdr->dstart * LIFSECTORSIZE, SEEK_SET);
	dentry = DLIST_ENTRY(ldp->tail, 0);
	Read(ldp->fd, dentry, dirsize, ldp->name);

	/* Find the last valid entry */
	while (dentry[ldp->tail->lastent].ftype != LIFTYPE_EOF &&
	       ldp->tail->lastent < ldp->tail->numentries)
	    ldp->tail->lastent++;
	ldp->tail->lastent--;
	ldp->tail->append = -1;

	/* Determine the last entry that will fit in this directory. */
	ldp->tail->dirend = ((ldp->hdr->dsize * LIFSECTORSIZE) /
			     sizeof(struct dentry)) - 1;

	ldp->flag |= NEVERCHANGED | FIXEDSIZE;
    }

    /* Point to first directory entry in our (only) buffer */
    ldp->tail->scanent = -1;
    ldp->head = ldp->scan = ldp->tail;

    return ldp;
}


/*
 * Get the next vacant lif directory entry.
 */
struct dentry *
new_lifentry(ldp)
struct lif_directory	*ldp;
{
    struct dlist	*dirbuf = ldp->tail;
    struct dentry	*dentry;	
    int			lastdirent;

    if (!(ldp->flag & APPEND)) {
	fprintf(stderr, "mkboot internal error in new_lifentry\n");
	exit(1);
    }

    ldp->flag &= ~(FLUSHED | NEVERCHANGED);

    /*
     * For fixed size directories, fail BEFORE we reach the end
     * to leave room for the end-of-file entry.  Assumes only one buffer
     * per fixed size directory.
     */
    if (ldp->flag & FIXEDSIZE)
	if (dirbuf->lastent >= dirbuf->dirend)
	    return (struct dentry *)0;

    /* Make sure we have something to offer */
    if (dirbuf->lastent >= dirbuf->numentries - 1) {
	dirbuf = get_newdirbuf(ldp);
	ldp->tail->next = dirbuf;
	ldp->tail = dirbuf;
    }

    dirbuf->lastent++;
    dentry = DLIST_ENTRY(dirbuf, dirbuf->lastent);

    /* If it wasn't done yet, set the "first appended entry" field */
    if (dirbuf->append < 0)
	dirbuf->append = dirbuf->lastent;

    return dentry;
}


/*
 * Remove the last lif entry.
 */
void
rm_lifentry(ldp)
struct lif_directory	*ldp;
{
    struct dentry	*dentry;
    struct dlist	*dirbuf = ldp->tail;

    /* If we're readonly, fail */
    if (!(ldp->flag & APPEND)) {
	fprintf(stderr, "mkboot internal error in rm_lifentry\n");
	exit(1);
    }

    /* If there are no more entries, we're done */
    if (dirbuf->lastent < 0)
	return;

    dentry = DLIST_ENTRY(dirbuf, dirbuf->lastent);
    bzero(dentry, sizeof(dentry));
    dirbuf->lastent--;

    /* See if we removed the last entry in this buffer */
    if (dirbuf->lastent < 0) {
	/* See if we removed the last buffer */
	if (dirbuf != ldp->head) {
	    /* If not, find the new last buffer and reset lastent */
	    free(dirbuf);
	    for (dirbuf = ldp->head;
		 dirbuf->next != ldp->tail;
		 dirbuf = dirbuf->next)
		;
	    dirbuf->lastent = dirbuf->numentries - 1;
	    ldp->tail = dirbuf;
	}
    }
}


/*
 * Retrieve the next unpurged directory entry.
 */
struct dentry *
read_lifentry(ldp)
struct lif_directory	*ldp;
{
    struct dlist	*dirbuf = ldp->scan;
    struct dentry	*dentry;	
    int			ent;

    if (ldp->flag & APPEND) {
	fprintf(stderr, "mkboot internal error in read_lifentry\n");
	exit(1);
    }

    /* Make sure we have something to offer */
    do {
	dirbuf->scanent++;
	if (dirbuf->scanent > dirbuf->lastent) {
	    /* Assume lif EOF occurs only in the last buffer */
	    if (ldp->scan == ldp->tail)
		return (struct dentry *)0;
	    /* Reset scan pointer for this buffer to the beginning */
	    dirbuf->scanent = 0;
	    dirbuf = ldp->scan = ldp->scan->next;
	}
	dentry = DLIST_ENTRY(ldp->tail, dirbuf->scanent);
    }
    while (dentry->ftype == LIFTYPE_PURGED);

    return dentry;
}


/*
 * Find an entry with the given name.
 */
struct dentry *
scan_lifdir(ldp, name)
struct lif_directory	*ldp;
char			*name;
{
    int		namlen = strlen(name);
    int		ent;
    struct dentry	*dentry;
    struct dlist	*dirbuf;

    if (namlen <= 0)
	return (struct dentry *)0;

    for (dirbuf = ldp->head; dirbuf; dirbuf = dirbuf->next)
	for (ent = 0; ent <= dirbuf->lastent; ent++) {
	    dentry = DLIST_ENTRY(dirbuf, ent);
	    if (dentry->ftype == LIFTYPE_PURGED)
		continue;
	    if (lifstrcmp(dentry->fname, name) == 0)
		return dentry;
	}

    return (struct dentry *)0;
}


/*
 * Flush the lif directory out to disk.  Be careful to make
 * the lif header and directory entries consistent.
 */
void
flush_lifdir(ldp)
struct lif_directory	*ldp;
{
    struct dentry	*dentry;
    struct dlist	*dirbuf;
    int			oldflag;
    int			bufent;
    int			ent;
    int			numbufs;
    int			fstart;
    int			status;

    if (ldp->flag & (FLUSHED | NEVERCHANGED))
	return;
    oldflag = ldp->flag;

    /* Terminate the directory with an EOF */
    ldp->flag = (ldp->flag & ~READONLY) | APPEND;
    dentry = new_lifentry(ldp);
    /*
     * When handling modified directories, new_lifentry returns NULL when
     * all but the last directory entry has been used.  In this case, we
     * just grab that last entry.
     */
    if (dentry == (struct dentry *)0) {
	ldp->tail->lastent = ldp->tail->numentries - 1;
	dentry = DLIST_ENTRY(ldp->tail, ldp->tail->lastent);
    }
    dentry->ftype = LIFTYPE_EOF;

    /* If the directory could grow, calculate the size of the directory */
    if (!(oldflag & FIXEDSIZE)) {
	dirbuf = ldp->head;
	for (numbufs = 1; dirbuf != ldp->tail; numbufs++)
	    dirbuf = dirbuf->next;
	ldp->hdr->dsize = numbufs * SECTORSPERBUF;
    }

    /* Calculate the starting locations of each lif file */
    fstart = ROUNDUP(ldp->hdr->dstart + ldp->hdr->dsize, LIFALIGNMENT);
    for (dirbuf = ldp->head; dirbuf; dirbuf = dirbuf->next) {
	for (bufent = 0; bufent < dirbuf->lastent; bufent++) {
	    dentry = DLIST_ENTRY(dirbuf, bufent);
	    if (dentry->ftype == LIFTYPE_EOF)
		break;
	    /* Only modify entries we have appended */
	    if (dirbuf->append >= 0 && dirbuf->append <= bufent) {
		/* Need to keep ISL's location in the header */
		if (lifstrcmp(dentry->fname, ISL) == 0)
		    ldp->hdr->iplstart = fstart * LIFSECTORSIZE;
		dentry->start = fstart;
	    }
	    fstart = ROUNDUP(dentry->start + dentry->size, LIFALIGNMENT);
	}
    }

    /* Write out the header */
    Lseek(ldp->fd, 0, SEEK_SET);
    Write(ldp->fd, ldp->hdr, LIFBUFSIZE, ldp->name);
    
    /* Write out the directory */
    Lseek(ldp->fd, ldp->hdr->dstart*LIFSECTORSIZE, SEEK_SET);
    for (dirbuf = ldp->head; dirbuf; dirbuf = dirbuf->next)
	Write(ldp->fd, (char *)DLIST_ENTRY(dirbuf,0),
	       dirbuf->numentries * sizeof(struct dentry), ldp->name);

    /* Fix things up so the directory is in its original state */
    rm_lifentry(ldp);			/* Get rid of EOF */
    ldp->flag = oldflag | FLUSHED;
}


/*
 * Go back to the beginning of the directory for reads.
 */
void
reset_lifdir(ldp, newflag)
struct lif_directory	*ldp;
int			newflag;
{
    int oflag;

    if (newflag == READONLY) {
	/* Make sure the directory is readonly */
	ldp->flag = (ldp->flag & ~APPEND) | READONLY;

	/* Reset pointers */
	ldp->scan->scanent = -1;
	ldp->scan = ldp->head;
	oflag = O_RDONLY;
    }
    else {	/* newflag == APPEND */
	/* Just reset the flag */
	ldp->flag = (ldp->flag & ~READONLY) | APPEND;
	oflag = O_RDWR;
    }
    Close(ldp->fd);
    ldp->fd = Open(ldp->name, oflag);
}


/*
 * Close a lif directory.
 */
void
close_lifdir(ldp)
struct lif_directory	*ldp;
{
    struct dlist	*dirbuf;

    if (!(ldp->flag & (FLUSHED | NEVERCHANGED)))
	flush_lifdir(ldp);

    /* Close the device */
    Close(ldp->fd);

    /* Free data structures */
    for (dirbuf = ldp->head; dirbuf; dirbuf = dirbuf->next)
	free(dirbuf);
    free(ldp);
}


/*
 * Get a buffer for a lif directory.
 */
struct dlist *
get_newdirbuf(ldp)
struct lif_directory *ldp;
{
    struct dlist	*new;
    int			bufsize;

    if (!(ldp->flag & APPEND)) {
	fprintf(stderr, "mkboot internal error in get_newdirbuf\n");
	exit(1);
    }

    bufsize = LIFBUFSIZE + sizeof(*new) - sizeof(new->dentry);

    new = (struct dlist *)malloc(bufsize);
    if (!new) {
	perror("buffer allocation error");
	exit(1);
    }
    bzero(new, bufsize);
    new->numentries = LIFBUFSIZE / sizeof(struct dentry);
    new->lastent = -1;
    new->append = -1;

    return new;
}


/*
 * Compare strings such that spaces are considered string terminators.
 * Consider only MAXFILENAME characters.
 */
int
lifstrcmp(s1, s2)
char	*s1;
char	*s2;
{
    char	c1, c2;
    int		i;

    for (i = 0; i < MAXFILENAME; i++) {
	c1 = s1[i];
	c2 = s2[i];
	if ((c1 == ' ' || c1 == '\0') && (c2 == ' ' || c2 == '\0'))
	    return 0;
	if (c1 != c2)
	    return c1 - c2;
    }
    return 0;
}


/*
 * Copy a string into a space-filled lif file name.
 */
char	*
lifstrcpy(s1, s2)
char	*s1;
char	*s2;
{
    int	i;
    char	*os1 = s1;

    for (i = 0; i < MAXFILENAME; i++) {
	if (*s2) 
	    *s1 = *s2++;
	else
	    *s1 = ' ';
	s1++;
    }
    return os1;
}


/*
 * Criteria stolen from lifcp for a "valid" lif header.
 */
int
valid_lif(hdr)
struct lvol	*hdr;
{
    if (hdr->discid == LIFID &&
	hdr->dummy1 == 4096  &&
	hdr->dummy2 == 0     &&
	hdr->dummy3 == 0     &&
	hdr->dstart >= 1     &&
	hdr->dsize  >= 1)
	    return 1;
    else
	    return 0;
}


/*
 * I/O interface routines for handling errors
 */
int
Open(file, flags)
char	*file;
int	flags;
{
    int	fd;

    fd = topen(file, flags);
    if (fd == -1) {
	fprintf(stderr, "Could not open ");
	perror(file);
	exit(1);
    }

    return fd;
}

int
Close(fd)
int	fd;
{
    tclose(fd);
}

void
Read(fd, buf, size, name)
int	fd;
void	*buf;
size_t	size;
char	*name;
{
    int	status;

    status = tread(fd, buf, size);

    if (status < 0) {
	fprintf(stderr, "Error reading ");
	perror(name);
	exit(1);
    }
}

void
Write(fd, buf, size, name)
int	fd;
void	*buf;
size_t	size;
char	*name;
{
    int	status;

    status = twrite(fd, buf, size);

    if (status < 0) {
	fprintf(stderr, "Error writing to ");
	perror(name);
	exit(1);
    }
}

void
Lseek(fd, loc, whence)
int	fd;
off_t	loc;
int	whence;
{
    int	status;

    status = tlseek(fd, loc, whence);

    if (status < 0) {
	perror("Seek error");
	exit(1);
    }
}
