/* @(#) $Revision: 66.4 $ */

#include <cpio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include "libcpio.h"

/*
 * Ascii cpio header
 */
#define CPIOHDRFMT \
	  "%06ho%06ho%06ho%06ho%06ho%06ho%06ho%06ho%011lo%06ho%011lo%s"
#define CPIOHDRSIZ 76
#define CPIO_MAGIC	070707

#define UNREP_NO	0xffff

#ifdef __hp9000s300
#   define LOCALMAGIC 0xdddd
#else
#   ifdef __hp9000s800
#      define LOCALMAGIC 0xcccc
#   endif /* __hp9000s800 */
#endif /* __hp9000s300 */

#define WRITE_LONG(sv, lv)				\
{							\
    union { long l; short s[2]; char c[4]; } U;		\
							\
    U.l = 1L;						\
    if (U.s[0])	/* Little Endian */			\
    {							\
	U.l = (lv);					\
	(sv)[0] = U.s[1];				\
	(sv)[1] = U.s[0];				\
    }							\
    else	/* Big Endian */			\
    {							\
	U.l = (lv);					\
	(sv)[0] = U.s[0];				\
	(sv)[1] = U.s[1];				\
    }							\
}

static int cpio_errno;

static long
buffer_write(desc, src, nbytes)
CPIO *desc;
char *src;
long nbytes;
{
    if (nbytes > 0 && desc->bufp->buflen <= 0)
    {
	cpio_errno = CPIO_BADBUFFER;
	return -1;
    }

    while (nbytes > 0)
    {
	unsigned char *dbuf = desc->bufp->buffer;
	int len = MIN(desc->bufp->buflen, nbytes);

	memcpy(desc->bufp->buffer, src, len);
	nbytes -= len;
	src += len;

	while (len > 0)
	{
	    int nwrite;

	    errno = 0;
	    nwrite = (*desc->write)(desc->write_parm, dbuf, len, desc);

	    if (nwrite == -1)
		if (errno == EINTR)
		    continue;
		else
		    return -1;

	    len -= nwrite;
	    dbuf += nwrite;
	    desc->size += nwrite;
	}
    }
    return 0;
}

static int
ascii_header(desc, name, st)
CPIO *desc;
char *name;
struct stat *st;
{
    int namelen = strlen(name) + 1;
    char buf[CPIOHDRSIZ + MAXPATHLEN + 1];

    sprintf(buf, CPIOHDRFMT,
	CPIO_MAGIC,
	(ushort)st->st_dev,
	(ushort)st->st_ino,
	(ushort)st->st_mode,
	(ushort)st->st_uid,
	(ushort)st->st_gid,
	(short)st->st_nlink,
	(short)st->st_rdev,
	(long)st->st_mtime,
	(short)namelen,
	(long)st->st_size,
	name);

    return buffer_write(desc, buf, CPIOHDRSIZ + namelen);
}

static int
binary_header(desc, name, st)
CPIO *desc;
char *name;
struct stat *st;
{
    int namelen = strlen(name) + 1;
    int hdrsiz;
    struct header {
	short   h_magic;            /* cpio magic number */
	short	h_dev;              /* device id */
	ushort  h_ino;              /* inode number */
	ushort	h_mode;             /* file mode */
	ushort	h_uid;              /* user ID of file owner */
	ushort	h_gid;              /* group ID of file group */
	short   h_nlink;            /* number of links */
	short	h_rdev;             /* device ID; only for spec files */
	short	h_mtime[2];         /* last modification time */
	short	h_namesize;         /* length of path name */
	short	h_filesize[2];      /* file size */
	char	h_name[MAXPATHLEN]; /* file name */
    } Hdr;

    Hdr.h_magic	= CPIO_MAGIC;
    Hdr.h_dev	= (unsigned short)st->st_dev;
    Hdr.h_ino	= (unsigned short)st->st_ino;
    Hdr.h_mode	= (unsigned short)st->st_mode;
    Hdr.h_uid	= (unsigned short)st->st_uid;
    Hdr.h_gid	= (unsigned short)st->st_gid;
    Hdr.h_nlink	= (short)st->st_nlink;
    Hdr.h_rdev	= (short)st->st_rdev;
    WRITE_LONG(Hdr.h_mtime, (long)st->st_mtime);
    Hdr.h_namesize = (short)namelen;
    WRITE_LONG(Hdr.h_filesize, (long)st->st_size);
    strcpy(Hdr.h_name, name);

    /*
     * The header must always be an even multiple of 2 bytes
     */
    hdrsiz = (sizeof Hdr - MAXPATHLEN) + namelen;
    if (hdrsiz & 0x01)
    {
	*((char *)&Hdr + hdrsiz) = '\0';
	hdrsiz++;
    }
    return buffer_write(desc, (char *)&Hdr, hdrsiz);
}

/*
 * The following two routines, "nextdevino()" and "makedevino()", map
 * the 32 bit each (dev,ino) pairs to 16 bit each (dev,ino) pairs.
 * without imposing any limits, other than there can be no more than
 * (2^32 - (65536 * 4) distinct files in an archive (which is a very
 * large number).
 *
 * A hash table is used to retain the mappings that were used for those
 * files whose link count is > 1.  We never need to know what files
 * with a link count <= 1 were mapped to, so these are not put in the
 * table.
 */

/*
 * nextdevino() -- calculate and return the next available 16 bit
 *                 each (dev,ino) pair.  The value of 0 is not used
 *                 for dev.  The values 0,1, and 65535 are not used
 *                 for ino.
 */
static void
nextdevino(desc, dev, ino)
CPIO *desc;
unsigned short *dev;
unsigned short *ino;
{
    if (++desc->lastino == UNREP_NO)
    {
	desc->lastino = 2;
	desc->lastdev++;
    }
    *dev = desc->lastdev;
    *ino = desc->lastino;
}

#define HASHSIZE	103

/*
 * makedevino() -- map a 32 bit each (dev,ino) pair to a 16 bit each
 *                 (dev,ino) pair, keeping track of mappings for files
 *                 who have a link count > 1.
 *
 * Returns:
 *  -1   out of memory
 *   0   first time we've seen this file
 *   1   we've seen this one before
 */
static int
makedevino(desc, links, l_dev, l_ino, s_dev, s_ino)
CPIO *desc;
int links;
unsigned long l_dev;
unsigned long l_ino;
unsigned short *s_dev;
unsigned short *s_ino;
{
    typedef struct devinomap SYMBOL;
    SYMBOL *sym;
    int key;

    /*
     * Simple case -- link count is 1, just map dev and ino to the
     *                next available number
     */
    if (links <= 1)
    {
	nextdevino(desc, s_dev, s_ino);
	return 0; /* first time for this one */
    }

    /*
     * We have a file with link count > 1, allocate our hash table if
     * we haven't already
     */
    if (desc->map == NULL)
    {
	int i;

	desc->map = (SYMBOL **)malloc(sizeof(SYMBOL *) * HASHSIZE);
	if (desc->map == (SYMBOL **)0)
	{
	    cpio_errno = CPIO_NOMEM;
	    return -1;
	}
	for (i = 0; i < HASHSIZE; i++)
	    desc->map[i] = NULL;
    }

    /*
     * Search the hash table for this (dev,ino) pair
     */
    key = (l_dev ^ l_ino) % HASHSIZE;
    for (sym = desc->map[key]; sym != NULL; sym = sym->next)
	if (sym->l_ino == l_ino && sym->l_dev == l_dev)
	{
	    *s_dev = sym->s_dev;
	    *s_ino = sym->s_ino;
	    return 1; /* we've seen this one before */
	}

    /*
     * Didn't find this one in the table, add it
     */
    nextdevino(desc, s_dev, s_ino);
    if ((sym = (SYMBOL *)malloc(sizeof(SYMBOL))) == NULL)
    {
	cpio_errno = CPIO_NOMEM;
	return -1;
    }

    sym->l_dev = l_dev;
    sym->l_ino = l_ino;
    sym->s_dev = *s_dev;
    sym->s_ino = *s_ino;
    sym->next = desc->map[key];
    desc->map[key] = sym;
    return 0; /* first time for this one */
}

/*
 * Pad the archive with 'nbytes' '\0's.
 */
static int
nullpad(desc, nbytes)
CPIO *desc;
long nbytes;
{
    while (nbytes > 0)
    {
	long cnt = MIN(nbytes, desc->bufp->buflen);
	unsigned char *buf = desc->bufp->buffer;

	memset(buf, '\0', cnt);
	nbytes -= cnt;

	while (cnt > 0)
	{
	    int nwrite;

	    errno = 0;
	    nwrite = (*desc->write)(desc->write_parm, buf, cnt, desc);
	    if (nwrite == -1)
		if (errno == EINTR)
		    continue;
		else
		    return CPIO_WRITE;

	    cnt -= nwrite;
	    buf += nwrite;
	    desc->size += nwrite;
	}
    }
    return CPIO_OK;
}

void
cpio_initialize(desc, flags, blksize)
CPIO *desc;
unsigned long flags;
unsigned long blksize;
{
    extern int open(), read(), write(), close();
#ifdef SYMLINKS
    extern int readlink();
#endif

    desc->open = open;
    desc->read = read;
    desc->write = write;
    desc->close = close;
#ifdef SYMLINKS
    desc->readlink = readlink;
#endif
    desc->size = 0;
    desc->blksize = blksize;
    desc->flags = flags;
    desc->lastino = 1;	/* 0 and 1 are special */
    desc->lastdev = 1;	/* 0 and 1 are special */
    desc->map = (struct devinomap **)0;
}

void
cpio_openfn(desc, fn)
CPIO *desc;
int (*fn)();
{
    desc->open = fn;
}

void
cpio_readfn(desc, fn)
CPIO *desc;
int (*fn)();
{
    desc->read = fn;
}

void
cpio_writefn(desc, fn)
CPIO *desc;
int (*fn)();
{
    desc->write = fn;
}

void
cpio_closefn(desc, fn)
CPIO *desc;
int (*fn)();
{
    desc->close = fn;
}

#ifdef SYMLINKS
void
cpio_readlinkfn(desc, fn)
CPIO *desc;
int (*fn)();
{
    desc->readlink = fn;
}
#endif /* SYMLINKS */

int
cpio_write(desc, name, real_name, pst)
CPIO *desc;
char *name;
char *real_name;
struct stat *pst;
{
    ushort type;	/* type of current file */
    ushort sdev, sino;
    int duplicate;
    struct stat st;

    st = *pst;
    type = st.st_mode & S_IFMT;	/* type of current file */

    /*
     * First, setup the st.st_size field.
     */
    switch (type)
    {
    case S_IFCHR:
    case S_IFBLK:
    case S_IFIFO:
	st.st_size = st.st_rdev;
#if (defined(DUX) || defined(DISKLESS)) && defined(CNODE_DEV)
	st.st_rdev = st.st_rcnode;
#else
	st.st_rdev = LOCALMAGIC;
#endif /* (DUX || DISKLESS) && CNODE_DEV */
	break;
    case S_IFDIR:
	st.st_size = 0;
	st.st_rdev = 0;
	break;
    case S_IFREG:
#ifdef SYMLINKS
    case S_IFLNK:
#endif
	st.st_rdev = 0;
	break;
    default:
	return CPIO_BADTYPE;
    }

    /*
     * Map the (dev,ino) pair
     */
    duplicate = makedevino(desc, st.st_nlink,
			   st.st_dev, st.st_ino, &sdev, &sino);
    if (duplicate == -1)
	return cpio_errno;

    st.st_dev = sdev;
    st.st_ino = sino;

    /*
     * If we've already written this file to the archive and the
     * EFFICIENT_LINKS mode is enabled, we only write the header for
     * this file.
     */
    if (duplicate == 1 && (desc->flags & EFFICIENT_LINKS))
	st.st_size = 0;

#ifdef SYMLINKS
    if (type == S_IFLNK)
    {
	int nbytes;
	char linkpath[MAXPATHLEN];

	nbytes = desc->readlink(real_name, linkpath, sizeof linkpath,
				desc);
	if (nbytes != st.st_size)
	    return CPIO_READLINK;

	/*
	 * Write the header
	 */
	if (desc->flags & CPIO_ASCII)
	    nbytes = ascii_header(desc, name, &st);
	else
	    nbytes = binary_header(desc, name, &st);
	if (nbytes != 0)
	    return nbytes;

	return buffer_write(desc, linkpath, st.st_size);
    }
#endif

    if (st.st_size == 0 || type != S_IFREG)
    {
	/*
	 * just write the header
	 */
	if (desc->flags & CPIO_ASCII)
	    return ascii_header(desc, name, &st);
	else
	    return binary_header(desc, name, &st);
    }

    {
	long remaining = st.st_size;
	int fd = (*desc->open)(real_name, O_RDONLY, 0666, desc);

	if (fd < 0)
	    return CPIO_OPEN;

	/*
	 * Now write the header
	 */
	{
	    int ret;

	    if (desc->flags & CPIO_ASCII)
		ret = ascii_header(desc, name, &st);
	    else
		ret = binary_header(desc, name, &st);
	    if (ret != CPIO_OK)
		return ret;
	}

	/*
	 * only write as many bytes as when we "stat"d the file
	 */
	while (remaining > 0)
	{
	    int nbytes = MIN(remaining, desc->bufp->buflen);

	    /*
	     * Read some data
	     */
	    errno = 0;
	    nbytes =(*desc->read)(fd, desc->bufp->buffer, nbytes, desc);
	    if (nbytes == -1)
		if (errno == EINTR)
		    continue;
		else
		    return CPIO_READ;

	    if (nbytes == 0)
		break;  /* EOF before we expected it */
	    remaining -= nbytes;

	    /*
	     * Write the data
	     */
	    {
		int nleft = nbytes;
		unsigned char *buf = desc->bufp->buffer;

		while (nleft > 0)
		{
		    int nwrite;

		    errno = 0;
		    nwrite = (*desc->write)(desc->write_parm, buf,
					    nleft, desc);
		    if (nwrite == -1)
			if (errno == EINTR)
			    continue;
			else
			    return CPIO_WRITE;

		    nleft -= nwrite;
		    buf += nwrite;
		    desc->size += nwrite;
		}
	    }
	}

	if ((*desc->close)(fd, desc) != 0)
	    return CPIO_CLOSE;

	{
	    int pad;

	    /*
	     * If we are writing a "binary" archive, we now
	     * pad out the file to an even multiple of 2 bytes.
	     *
	     * We must also pad the file out if we didn't read
	     * as many bytes from the file as we expected.
	     */
	    if ((desc->flags & CPIO_ASCII) == 0 && st.st_size & 0x01)
		pad = 1;
	    else
		pad = 0;

	    pad = nullpad(desc, pad + remaining);
	    if (pad != CPIO_OK)
		return pad;

	    if (remaining == 0)
		return CPIO_OK;
	    return CPIO_FILESHRANK;
	}
    }
}

int
cpio_finish(desc)
CPIO *desc;
{
    static char trailer[] = "TRAILER!!!";
    int ret;
    struct stat st;
    unsigned short sdev, sino;

    memset(&st, '\0', sizeof st);
    (void)makedevino(desc, 1, 0, 0, &sdev, &sino);
    st.st_dev = sdev;
    st.st_ino = sino;

    if (desc->flags & CPIO_ASCII)
	ret = ascii_header(desc, trailer, &st);
    else
	ret = binary_header(desc, trailer, &st);
    if (ret != CPIO_OK)
	return ret;

    if (desc->blksize == 0)
	return CPIO_OK;

    ret = desc->size % desc->blksize;
    if (ret != 0)
	return nullpad(desc, desc->blksize - ret);
}
