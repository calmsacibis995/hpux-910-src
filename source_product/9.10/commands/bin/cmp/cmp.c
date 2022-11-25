static char HPUX_ID[] = "@(#) $Revision: 66.4 $";
/*
 * cmp [-ls] file1 file2 (compare two files)
 */

/*
 * I/O is done in BUFFERSZ chunks
 */
#define BUFFERSZ	(64 * 1024)

#include	<stdio.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<sys/param.h>
#include	<errno.h>
#ifdef NLS
#   define NL_SETN 1
#   include <nl_types.h>
    char cmd_name[] = "cmp";
    nl_catd nlmsg_fd = -2; /* not opened yet */
#else
#   define catgets(i,sn,mn,s) (s)
#   define open_catalog()
#endif

int sflg; /* = 0 */
int lflg; /* = 0 */
long line    = 1;	/* current line count */
long chr; /* = 0 */	/* current char count */

long buf1cnt;		/* bytes in buf1 */
long buf2cnt;		/* bytes in buf2 */
unsigned char buf1[BUFFERSZ];
unsigned char buf2[BUFFERSZ];

/*
 * The following buffer is hardly every used.  Always ensure that
 * this buffer is declared last.  This avoids wasting address space
 * that isn't used.
 */
unsigned char buf3[BUFFERSZ];

#ifdef NLS
/*
 * open_catalog() --
 *    open the NLS message catalog file if not already open.
 */
void
open_catalog()
{
    if (nlmsg_fd == -2)
    {
	int err = errno;
	nlmsg_fd = catopen(cmd_name);
	errno = err;
    }
}
#endif /* NLS */

static void
usage()
{
    static char USAGE[] =
	"usage: cmp [-l] [-s] file1 file2\n";  /* catgets 2 */
    open_catalog();
    fputs(catgets(nlmsg_fd,NL_SETN,2, USAGE), stderr);
    exit(2);
}

static void
barg(arg)
char *arg;
{
    if (!sflg)
    {
	open_catalog();
	fprintf(stderr,
	    catgets(nlmsg_fd,NL_SETN,3, "cmp: cannot open %s\n"), arg);
    }
    exit(2);
}

static void
earg(arg)
char *arg;
{
    if (!sflg)
    {
	open_catalog();
	fprintf(stderr,
	    catgets(nlmsg_fd,NL_SETN,4, "cmp: EOF on %s\n"), arg);
    }
    exit(1);
}

/*
 * full_read() --
 *    do a read, waiting for 'n' bytes, handling EINTER and other
 *    causes of a short read.
 */
static int
full_read(fd, buf, n, name)
int fd;
unsigned char *buf;
unsigned long n;
char *name;
{
    unsigned long total = 0;
    int errno_save = errno;
    int nread;

    do
    {
	if ((nread = read(fd, buf + total, n - total)) == -1)
	{
	    if (errno == EINTR)
	    {
		errno = errno_save;
		continue;
	    }
	    else
	    {
		if (!sflg)
		{
		    open_catalog();
		    fputs(catgets(nlmsg_fd,NL_SETN,5, "cmp: read error on "),
			stderr);
		    perror(name);
		}
		exit(2);
	    }
	}
	total += nread;
    } while (total < n && nread != 0);

    return total;
}

/*
 * count_newlines() --
 *    count up the number of '\n' characters in ``buf''.
 */
static int
count_newlines(buf, bufcnt)
register unsigned char *buf;
unsigned long bufcnt;
{
    extern unsigned char *memchr();
    register unsigned char *bufend = buf + bufcnt;
    register int count = 0;

    while (buf < bufend)		/* update line count */
    {
	if ((buf = memchr(buf, '\n', bufend-buf)) == (unsigned char *)0)
	    break;

	count++;
	buf++; /* skip past this '\n' */
    }
    return count;
}

/*
 * quick_cmp() --
 *    function to quickly compare two files.  This function is called
 *    if the sflg or lflg was set OR we can seek on fd1.
 *    We quickly compare the two files without counting newlines.
 *    If we find a difference and both the sflg and lflg were *not*
 *    set, we re-read the first portion of file1 and count the newlines.
 *    We then return MIN(buf1cnt, buf2cnt).
 */
int
quick_cmp(fd1, fd2, file1, file2)
int fd1;
int fd2;
char *file1;
char *file2;
{
    long bytes_same = 0; /* # of bytes same so far */
    long bufcnt;

    for (;;)
    {
	buf1cnt = full_read(fd1, buf1, sizeof buf1, file1);
	buf2cnt = full_read(fd2, buf2, sizeof buf2, file2);

	/*
	 * This condition only happens when both files are empty
	 * or end on an even "sizeof buf1" boundry.
	 */
	if (buf1cnt == 0 && buf2cnt == 0)
	    exit(0);

	if (memcmp(buf1, buf2, bufcnt = MIN(buf1cnt, buf2cnt)) != 0)
	{
	    if (sflg)
		exit(1);
	    break;
	}

	/*
	 * EOF on one file or the other, print EOF message with name
	 * of file that is short.
	 */
	if (buf1cnt != buf2cnt)
	    earg(buf1cnt < buf2cnt ? file1 : file2);

	/*
	 * The only way that we can get a "short" read from full_read
	 * is if we hit EOF.  So, if either buf1cnt or buf2cnt is
	 * less than what we asked for, we must be at EOF.  Since we
	 * know that (buf1cnt == buf2cnt), we also know that we are
	 * at EOF on both files (so we are done).
	 */
	if (buf1cnt < sizeof buf1)
	    exit(0);	/* files are the same */

	bytes_same += bufcnt;
    }

    /*
     * Update current character counter
     */
    chr = bytes_same;

    /*
     * The files are different.  If lflg is set or we are still in
     * the first buffer, we simply return and let main() do the rest
     * of the work.
     */
    if (lflg || bytes_same == 0)
	return bufcnt;

    /*
     * Neither the lflg or sflg was set and the files are different.
     * We must re-scan file1 up to the data that we currently have in
     * buf1, counting the newlines.  We then return and let main()
     * do the rest of the work.
     */
    (void)lseek(fd1, 0, SEEK_SET); /* rewind file1 */
    do
    {
	int nread;
	int rsize = MIN(sizeof buf3, bytes_same);

	/*
	 * Since the only case when full_read() returns something
	 * other than what we requested is when you hit EOF, we fail
	 * if we didn't read the number of bytes we wanted.
	 *
	 * This only happens when file1 changes size on us
	 * while we are accessing it.  So, we just pretend that
	 * we hit the EOF at this point the first time (we
	 * already know that the bytes up to this point match).
	 */
	if ((nread = full_read(fd1, buf3, rsize, file1)) != rsize)
	    earg(file1); /* EOF on file1 */

	line += count_newlines(buf3, nread);
	bytes_same -= nread;
    } while (bytes_same > 0);

    /*
     * restore the file position in file1 to where we were (which is
     * where we are now + the number of bytes in buf1).
     */
    (void)lseek(fd1, bufcnt, SEEK_CUR);

    return bufcnt;
}

main(argc, argv)
int argc;
char *argv[];
{
    extern char *optarg;		/* for getopt */
    extern int	optind;			/* for getopt */
    int c;				/* for getopt */

    int fd1;
    int fd2;
    int can_seek = 1;
    char *file1;
    char *file2;
    int same = 1;		/* files are same so far */
    int bufcnt;			/* bytes in buffers */
    int bufs_different;		/* flag to avoid doing memcmp() twice */

    if (argc < 3)		/* check for a sane number of args */
	usage();

    while ((c = getopt(argc, argv, "ls")) != EOF)
	switch (c)
	{
	case 'l':
	    ++lflg;
	    break;
	case 's':
	    ++sflg;
	    break;
	default:
	    usage();
	}

    if (strcmp(argv[optind], argv[optind + 1]) == 0)	/* same name? */
	exit(0);

    /*
     * Open up the first file.  Determine if we can seek on it.
     */
    file1 = argv[optind++];
    if (file1[0] == '-' && file1[1] == 0)
	fd1 = 0; /* stdin */
    else
	if ((fd1 = open(file1, O_RDONLY)) == -1)
	    barg(file1);
    can_seek = (sflg || lflg || lseek(fd1, 0, SEEK_SET) != -1);

    /*
     * Open up the second file.
     */
    file2 = argv[optind++];
    if (file2[0] == '-' && file2[1] == 0)
	fd2 = 0; /* stdin */
    else
	if ((fd2 = open(file2, O_RDONLY)) == -1)
	    barg(file2);

    if (optind < argc)
	usage();

    /*
     * Quickly check to see if the files are identical (but only if
     * we can seek [actually, we pretend we can seek if either sflg or
     * lflg is set, since we don't ever have to backtrack when -s or
     * -l are used]).
     */
    if (can_seek)
    {
	bufcnt = quick_cmp(fd1, fd2, file1, file2);
	same = 0;  /* quick_cmp only returns if files different */
	bufs_different = 1; /* we know buf1,buf2 are different */
    }
    else
    {
	/*
	 * Since the following for-loop assumes that we have data
	 * in the buffers, we must do an initial fill of the
	 * buffers.
	 */
	buf1cnt = full_read(fd1, buf1, sizeof buf1, file1);
	buf2cnt = full_read(fd2, buf2, sizeof buf2, file2);
	bufs_different = 0; /* don't know if buf1,buf2 are different */
    }

    /*
     * The only time when buf1cnt and buf2cnt are both zero is when
     * both files are empty or exactly an even multiple of our buffer
     * size.
     */
    while (buf1cnt != 0 || buf2cnt != 0)
    {
	if (bufs_different ||
	    memcmp(buf1, buf2, bufcnt = MIN(buf1cnt, buf2cnt)) != 0)
	{
	    register unsigned char *c1 = buf1;
	    register unsigned char *c2 = buf2;

	    /*
	     * Since we don't care about counting newlines if lflg
	     * is set, we use two separate loops to find the mismatched
	     * byte(s).  This is so that -l is slightly faster.
	     */
	    if (lflg)
	    {
		for (; bufcnt--; c1++, c2++)
		{
		    chr++;
		    if (*c1 != *c2)
			printf("%6ld %3o %3o\n", chr, *c1, *c2);
		}
		same = 0; /* files are different */
	    }
	    else
	    {
		/*
		 * This case always does an exit(1).
		 */
		for (; bufcnt--; c1++, c2++)
		{
		    chr++;
		    if (*c1 != *c2)
		    {
			open_catalog();
			printf(catgets(nlmsg_fd,NL_SETN,1, "%s %s differ: char %ld, line %ld\n"),
			    file1, file2, chr, line);
			exit(1);
		    }

		    /*
		     * We increment the line count *after* checking
		     * for a difference.  This is because we consider
		     * the '\n' terminating line X to be on line X
		     * (rather than line X+1).
		     */
		    if (*c1 == '\n')
			line++;
		}
	    }
	}
	else
	{
	    /*
	     * This buffer-full is the same, just update counts.
	     * We don't really care about counting newlines if
	     * lflg is set.
	     *
	     * If this is the last buffer (i.e. we are at EOF now),
	     * we skip counting the new lines.  (We are at EOF if
	     * buf1cnt != buf2cnt or either count is not equal to
	     * our buffer size).
	     */
	    chr += bufcnt;
	    if (!lflg && buf1cnt == buf2cnt && buf1cnt == sizeof buf1)
		line += count_newlines(buf1, bufcnt);
	}

	/*
	 * Check for an EOF condition.  If buf1cnt and buf2cnt dont
	 * match, we had a premature EOF on one of the files; we print
	 * a message and exit.  If the two counts match, but are less
	 * than our buffer size, then we must be done, so we simply
	 * exit.  (we know this to be true since full_read only does
	 * a "short" read when we are at EOF).
	 */
	if (buf1cnt != buf2cnt)
	    earg(buf1cnt < buf2cnt ? file1 : file2);
	if (buf1cnt < sizeof buf1)
	    exit(same ? 0 : 1);

	buf1cnt = full_read(fd1, buf1, sizeof buf1, file1);
	buf2cnt = full_read(fd2, buf2, sizeof buf2, file2);
	bufs_different = 0; /* don't know if buf1,buf2 are different */
    }
    exit(same ? 0 : 1);
}
