/* @(#) $Revision: 70.1 $ */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include "walkfs.h"

#ifndef NLS
#   define catgets(cd,sn,mn,ds) (ds)
#else
#   define NL_SETN 1		/* set number */
#   include <msgbuf.h>
    extern nl_catd catd;
#endif /* NLS */

#include "libcpio.h"

/*
 * cpio/ncpio constants
 */
#define CPIOBSZ	4096
#define Bufsize	5120

struct cpio_files
{
    CPIO desc;
    char *name;
    unsigned long flags;
    unsigned char *buf;
    long buflen;
    int blksize;
    int blockio;
    struct cpio_files *next;
};

typedef struct cpio_files CFILES;
static CFILES *cfiles = (CFILES *)0;

static void
cpio_perror(cpio_error)
int cpio_error;
{
    fputs(catgets(catd,NL_SETN,62, "find: cpio I/O failed -- "),
	stderr);

    switch (cpio_error)
    {
    case CPIO_OK:
       fputs(catgets(catd,NL_SETN,64, "No error\n"), stderr);
       break;
    case CPIO_NOMEM:
       fputs(catgets(catd,NL_SETN,65, "out of memory\n"), stderr);
       break;
    case CPIO_BADTYPE:
       fputs(catgets(catd,NL_SETN,66, "unknown file type\n"), stderr);
       break;
    case CPIO_BADBUFFER:
       fputs(catgets(catd,NL_SETN,67, "internal buffering error\n"), stderr);
       break;
    case CPIO_FILESHRANK:
       fputs(catgets(catd,NL_SETN,68, "file changed size\n"), stderr);
       break;
    case CPIO_OPEN:
       perror("open");
       break;
    case CPIO_READ:
       perror("read");
       break;
    case CPIO_WRITE:
       perror("write");
       break;
    case CPIO_CLOSE:
       perror("close");
       break;
#ifdef SYMLINKS
    case CPIO_READLINK:
       perror("readlink");
       break;
#endif
    default:
       fprintf(stderr,
       catgets(catd,NL_SETN,69,"unknown error (CPIO_%d)\n"), cpio_error);
       break;
    }
}

static void
chgreel(cfile)
CFILES *cfile;
{
    extern char *sys_errlist[];
    int fd;			/* value of open call for device/file */
    FILE *devtty, *devttyout;	/* for terminal device output */
    char str[MAXPATHLEN];
    struct stat st;
    int real_error = errno;

    if (cfile->blockio == 0)
    {
	fprintf(stderr,
	    catgets(catd,NL_SETN,17, "find: can't write %s: %s\n"),
	    cfile->name, sys_errlist[real_error]);
	exit(2);
    }

    /* open dev/tty, or die with message to stderr */
    devtty = fopen("/dev/tty", "r");
    devttyout = fopen("/dev/tty", "w");

    if (devtty == NULL || devttyout == NULL)
    {
	fputs(catgets(catd,NL_SETN,16, "Can't open /dev/tty to prompt for more media.\n"),
	    stderr);
	exit(2);
    }

    /*
     * print end-of-reel message to /dev/tty
     */
    fprintf(devttyout,
	catgets(catd,NL_SETN,17, "find: can't write %s: %s\n"),
	cfile->name, sys_errlist[real_error]);
    fflush(devttyout);

    /*
     * close the original file (ie, first reel)
     */
    close(cfile->desc.write_parm);

    do
    {
	fputs(catgets(catd,NL_SETN,19, "If you want to go on, type device/file name when ready\n"),
	    devttyout);
	fflush(devttyout);

	/*
	 * read new name from dev/tty and delete the trailing '\n'
	 */
	fgets(str, MAXPATHLEN, devtty);
	str[strlen(str) - 1] = '\0';

	/*
	 * if we got a null name, quit with message to stderr
	 */
	if (*str == '\0')
	{
	    fputs(catgets(catd,NL_SETN,20, "User entered a null name for next device file.\n"), stderr);
	    exit(2);
	}

	/* try new name, notify /dev/tty if a problem */
	if ((fd = creat(str, 0666)) < 0)
	    fprintf(stderr,
		catgets(catd,NL_SETN,21, "find: can't open %s: %s\n"),
		str, sys_errlist[errno]);
    } while (fd < 0);

    /*
     * close tty, log reel change, return raw file descriptor after
     * good open
     */
    fclose(devtty);
    fclose(devttyout);
    fprintf(stderr,
	catgets(catd,NL_SETN,22, "User opened file %s to continue.\n"),
	str);
    cfile->desc.write_parm = fd;
    cfile->name = (char *)malloc(strlen(str)+1);
    strcpy(cfile->name, str);
}

static int
cpio_writer(fd, buf, size, cfile)
int fd;
unsigned char *buf;
int size;
CFILES *cfile;
{
    int tot_bytes = (buf + size) - cfile->buf;
    int rem_bytes = tot_bytes % cfile->blksize;
    int wrt_bytes = tot_bytes - rem_bytes;
    unsigned char *src = cfile->buf;

    /*
     * A write of less than blksize bytes.  Simply advance the buffer
     * pointer, adjust the count, and return indicating the the
     * requested number of bytes were "written".
     */
    if (wrt_bytes == 0)
    {
	cfile->desc.bufp->buffer += size;
	cfile->desc.bufp->buflen -= size;
	return size;
    }

    /*
     * We've got at least 1 blocks worth of data to write.  Write it
     * out.
     */
    while (wrt_bytes > 0)
    {
	int cnt = write(fd, src, wrt_bytes);

	if (cnt <= 0)
	    chgreel(cfile);
	else
	{
	   wrt_bytes -= cnt;
	   src += cnt;
	}
    }

    /*
     * Shift any remaining bytes to the beginning of the buffer
     */
    if (rem_bytes > 0)
	memcpy(cfile->buf, src, rem_bytes);

    /*
     * And point the buffer to the end of the remainder (if any).
     */
    cfile->desc.bufp->buffer = cfile->buf + rem_bytes;
    cfile->desc.bufp->buflen = cfile->buflen - rem_bytes;
    return size;
}

CPIO *
cpio_open(name, flags)
char *name;
unsigned long flags;
{
    CFILES *p;
    int fd;
    struct stat st;

    for (p = cfiles; p != (CFILES *)0; p = p->next)
	if (strcmp(name, p->name) == 0)
	    break;

    if (p)
    {
	if (flags != p->flags)
	{
	    fputs(catgets(catd,NL_SETN,61, "find: can't use both -ncpio and -cpio on the same file\n"), stderr);
	    exit(1);
	}
	return &p->desc;
    }

    fd = creat(name, 0666);
    if (fd == -1)
    {
	fprintf(stderr,
	    catgets(catd,NL_SETN,8, "find: cannot create %s\n"), name);
	exit(1);
    }

    p = (CFILES *)malloc(sizeof (CFILES));
    p->name = name;
    p->flags = flags;
    p->next = cfiles;
    cfiles = p;

    if (fstat(fd, &st) != -1 &&
	(S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)))
    {
	p->blockio = 1;
	p->blksize = 5120;
	p->buflen = 5120 * 12;
    }
    else
    {
	p->blockio = 0;
	p->blksize = 512;
	p->buflen = 64 * 1024;
    }

    p->buf = (unsigned char *)malloc(p->buflen);

    p->desc.bufp =
	      (struct cpio_buffer *)malloc(sizeof (struct cpio_buffer));
    p->desc.bufp->buffer = p->buf;
    p->desc.bufp->buflen = p->buflen;
    p->desc.write_parm = fd;
    cpio_initialize(&p->desc, flags, p->blksize);

    cpio_writefn(&p->desc, cpio_writer);

    return &p->desc;
}

void
cpio_close()
{
    CFILES *p;

    for (p = cfiles; p != (CFILES *)0; p = p->next)
    {
	int ret;

	if ((ret = cpio_finish(&p->desc)) != CPIO_OK)
	{
	    cpio_perror(ret);
	}
	(void)close(p->desc.write_parm);
    }
}

int
cpio(p)
register struct { int f; CPIO *desc; } *p;
{
#if defined(DUX) || defined(DISKLESS)
    extern char cpio_path[];
#endif /* DUX || DISKLES) */
    extern struct walkfs_info *Info;
    extern struct stat Statb;
    extern char *Pathname;
    int ret;

#ifdef S_IFSOCK
    /*
     * Silently ignore unix-domain sockets
     */
    if (S_ISSOCK(Statb.st_mode))
	return 1;
#endif /* S_IFSOCK */

#if defined(DUX) || defined(DISKLESS)
    ret = cpio_write(p->desc, cpio_path, Info->shortpath, &Statb);
#else
    ret = cpio_write(p->desc, Pathname, Info->shortpath, &Statb);
#endif /* DUX || DISKLES) */

    if (ret != CPIO_OK && ret != CPIO_FILESHRANK)
	cpio_perror(ret);
    return 1;
}
