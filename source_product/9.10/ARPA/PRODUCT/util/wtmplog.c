/*
 * @(#) $Revision: 1.1.109.1 $
 *
 * wtmplog(), btmplog() --
 *     write entries to the wtmp or btmp files using the accepted
 *     method of only logging if the file exists.
 */

#ifdef _NAMESPACE_CLEAN
#   define close	_close
#   define fstat	_fstat
#   define ftruncate	_ftruncate
#   define open		_open
#   define write	_write
#   define btmplog	_btmplog
#   define wtmplog	_wtmplog
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utmp.h>

/*
 * xtmplog() --
 *    log to a given utmp-like file only if the file already exists.
 *
 *    We first save the size of the file so that if the write
 *    fails and only writes a partial record, truncate the file
 *    back to its original position.
 *
 * NOTE: the truncation may cause the loss of records written  by other
 *       processes if they successfully write entries between our
 *       fstat() and our write().  This is okay, since this condition
 *       should be rare and its only a log file anyway.  To make it
 *       truly robust would be *way* too inefficient.
 */
static void
xtmplog(path, u)
char *path;
struct utmp *u;
{
    int fd;

    if (u != (struct utmp *)0 &&
	(fd = open(path, O_WRONLY|O_APPEND)) != -1)
    {
	struct stat st;
	int stat_ret = fstat(fd, &st);
	int n = write(fd, u, sizeof (struct utmp));

	/*
	 * Only truncate the file if we wrote a partial record
	 * and the fstat() was successful.
	 * Truncate to an even number of utmp records, just to be
	 * safe (in case someone else trashed the file somehow).
	 */
	if (n != sizeof (struct utmp) && n > 0 && stat_ret != -1)
	{
	    long nrecs = st.st_size / sizeof (struct utmp);

	    ftruncate(fd, nrecs * sizeof (struct utmp));
	}

	close(fd);
    }
}

#ifdef _NAMESPACE_CLEAN
#   undef wtmplog
#   pragma _HP_SECONDARY_DEF _wtmplog wtmplog
#   define wtmplog _wtmplog
#endif

void
wtmplog(u)
struct utmp *u;
{
    xtmplog(WTMP_FILE, u);
}

#ifdef _NAMESPACE_CLEAN
#   undef btmplog
#   pragma _HP_SECONDARY_DEF _btmplog btmplog
#   define btmplog _btmplog
#endif

void
btmplog(u)
struct utmp *u;
{
    xtmplog(BTMP_FILE, u);
}
