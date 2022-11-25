/* @(#) $Revision: 66.1 $ */

/*
 * Routines to read and write the /etc/utmp file.
 */

#ifdef _NAMESPACE_CLEAN
#   define getutent	_getutent
#   define open		_open
#   define read		_read
#   define lseek	_lseek
#   define getutid	_getutid
#   define getutline	_getutline
#   define strncmp	_strncmp
#   define fcntl	_fcntl
#   define setutent	_setutent
#   define write	_write
#   define endutent	_endutent
#   define close	_close
#   define utmpname	_utmpname
#   define strlen	_strlen
#   define strcpy	_strcpy
#   define memset	_memset
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

/*
 * Ignore the pututline() function declaration in <utmp.h>.  We want it to
 * be implemented so that it returns a value (as it has done for quite some
 * time) to maintain compatibility, but pututline() is documented by HP and
 * AT&T in the SVID as returning nothing.  We now document _pututline() as
 * behaving the same as pututline(), but it returns a pointer to struct
 * utmp.
 */
#define pututline __realpututline
#include <utmp.h>
#undef pututline

#include <errno.h>
#include <fcntl.h>

extern long lseek();
extern time_t time();
extern int errno;

#ifndef SEEK_SET
#   define SEEK_SET 0   /* supposed to be defined in stdio.h */
#   define SEEK_CUR 1
#   define SEEK_END 2
#endif

#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#endif

#define	MAXFILE	79	/* Maximum pathname length for "utmp" file */

#ifdef DEBUG
#   undef   UTMP_FILE
#   define  UTMP_FILE "utmp"
#endif

/*
 * fd       -- file descriptor for utmp file
 * fd_flags -- the flags bits used to open the utmp file
 * utmpfile -- path name of currrent "utmp" like file
 * loc_utmp -- current position in the utmp file
 * ubuf     -- copy of last entry read in
 */
static int fd = -1;
static int fd_flags;
static char utmpfile[MAXFILE+1] = UTMP_FILE;
static long loc_utmp;     /* BSS, initialized to 0 */
static struct utmp ubuf;  /* BSS, initialized to 0 */

/*
 * setutent() --
 *    reset the utmp file back to the beginning.
 *    Also zeros the stored copy of the last entry read, since we are
 *    resetting to the beginning of the file.
 */

#ifdef _NAMESPACE_CLEAN
#   undef setutent
#   pragma _HP_SECONDARY_DEF _setutent setutent
#   define setutent _setutent
#endif

void
setutent()
{
    if (fd != -1)
	lseek(fd, 0L, SEEK_SET);
    loc_utmp = 0;
    memset(&ubuf, '\0', sizeof ubuf);
}

/*
 * endutent() --
 *    close the utmp file.
 */

#ifdef _NAMESPACE_CLEAN
#   undef endutent
#   pragma _HP_SECONDARY_DEF _endutent endutent
#   define endutent _endutent
#endif

void
endutent()
{
    if (fd != -1)
	close(fd);
    fd = -1;
    loc_utmp = 0;
    fd_flags = 0;
    memset(&ubuf, '\0', sizeof ubuf);
}

/*
 * utmpname() --
 *    allow the user to read a file other than the normal
 *    "utmp" file.
 *    Returns TRUE if successful, FALSE if the filename is too long.
 */

#ifdef _NAMESPACE_CLEAN
#   undef utmpname
#   pragma _HP_SECONDARY_DEF _utmpname utmpname
#   define utmpname _utmpname
#endif

int
utmpname(newfile)
char *newfile;
{
    extern char utmpfile[];

    /*
     * Determine if the new filename will fit.  If not, return FALSE.
     * Otherwise copy in the new file name and then reset everything
     * to the initial state.
     */
    if (strlen(newfile) > MAXFILE)
	return FALSE;

    strcpy(utmpfile, newfile);
    endutent();
    return TRUE;
}

/*
 * getutent() --
 *    get the next entry in the utmp file.  Returns a pointer to a
 *    static area containing the entry, or NULL if no more entries.
 */

#ifdef _NAMESPACE_CLEAN
#   undef getutent
#   pragma _HP_SECONDARY_DEF _getutent getutent
#   define getutent _getutent
#endif

struct utmp *
getutent()
{
    int nread; /* number of bytes read */

    /*
     * If the "utmp" file is not open, attempt to open it for
     * reading.  If there is no file, attempt to create one.  If
     * both attempts fail, return NULL.  If the file exists, but
     * isn't readable and writeable, do not attempt to create.
     */
    if (fd < 0)
    {
	loc_utmp = 0;
	if ((fd = open(utmpfile, (fd_flags= O_RDWR|O_CREAT), 0644)) < 0)
	{
	    /*
	     * If the open failed for permissions, try opening it
	     * only for reading.  All pututline() later will fail on
	     * the writes.
	     */
	    if (errno == EACCES &&
		(fd = open(utmpfile, (fd_flags = O_RDONLY))) < 0)
	    {
		fd_flags = 0;
		return (struct utmp *)NULL;
	    }
	}
    }

    /*
     * Make sure that we are at an integral offset into the utmp file.
     */
    if ((loc_utmp % sizeof (struct utmp)) != 0)
    {
	loc_utmp -= (loc_utmp % sizeof ubuf);
	lseek(fd, loc_utmp, SEEK_SET);
    }

    /*
     * Try to read in the next entry from the utmp file.
     */
    if ((nread = read(fd, &ubuf, sizeof ubuf)) != sizeof ubuf)
    {
	/*
	 * If the read failed, return NULL.  Set the current position
	 * in the utmp file to where we really are.
	 */
	if (nread > 0)
	    loc_utmp += nread;

	/*
	 * Make sure that our "buffered" item is emptied, since we
	 * only got a partial read.
	 */
	memset(&ubuf, '\0', sizeof ubuf);

	return (struct utmp *)NULL;
    }

    /*
     * update our current understanding of the position in the
     * utmp file and return the new entry.
     */
    loc_utmp += sizeof ubuf;
    return &ubuf;
}

/*
 * id_match() --
 *    return TRUE if two utmp entries "match" using the same criteria
 *    that getutid uses.
 */
static int
id_match(u1, u2)
register struct utmp *u1;
register struct utmp *u2;
{
    /*
     * EMPTY entries never match.
     */
    if (u1->ut_type == EMPTY || u2->ut_type == EMPTY)
	return FALSE;

    switch (u1->ut_type)
    {
    case RUN_LVL:
    case BOOT_TIME:
    case OLD_TIME:
    case NEW_TIME:
	/*
	 * For RUN_LVL, BOOT_TIME, OLD_TIME, and NEW_TIME
	 * entries, only the types have to match.
	 */
	return u1->ut_type == u2->ut_type;
    case INIT_PROCESS:
    case LOGIN_PROCESS:
    case USER_PROCESS:
    case DEAD_PROCESS:
	/*
	 * For INIT_PROCESS, LOGIN_PROCESS, USER_PROCESS, and
	 * DEAD_PROCESS the ids have to match and the types has
	 * to be of the same class.
	 */
	{
	    register int type = u2->ut_type;

	    return (type == INIT_PROCESS || type == LOGIN_PROCESS ||
		    type == USER_PROCESS || type == DEAD_PROCESS) &&
		   u1->ut_id[0] == u2->ut_id[0] &&
		   u1->ut_id[1] == u2->ut_id[1] &&
		   u1->ut_id[2] == u2->ut_id[2] &&
		   u1->ut_id[3] == u2->ut_id[3];
	}
    }
    return FALSE;
}

/*
 * getutid() --
 *    find the specified entry in the utmp file.
 *    If it can't find it, it returns NULL.
 */

#ifdef _NAMESPACE_CLEAN
#   undef getutid
#   pragma _HP_SECONDARY_DEF _getutid getutid
#   define getutid _getutid
#endif

struct utmp *
getutid(entry)
register struct utmp *entry;
{
    register struct utmp *cur = &ubuf;

    /*
     * Start looking for entry.
     * Look in our current buffer before reading in new entries.
     */
    do
    {
	if (id_match(cur, entry))
	    return cur;
    } while ((cur = getutent()) != (struct utmp *)NULL);

    return (struct utmp *)NULL;  /* not found */
}

/*
 * getutline() --
 *    search the "utmp" file for a LOGIN_PROCESS or USER_PROCESS with
 *    the same "line" as the specified "entry".
 */

#ifdef _NAMESPACE_CLEAN
#   undef getutline
#   pragma _HP_SECONDARY_DEF _getutline getutline
#   define getutline _getutline
#endif

struct utmp *
getutline(entry)
register struct utmp *entry;
{
    register struct utmp *cur = &ubuf;

    do
    {
	/*
	 * If the current entry is the one we are interested in,
	 * return a pointer to it.
	 */
	if (cur->ut_type != EMPTY &&
	    (cur->ut_type == LOGIN_PROCESS ||
	     cur->ut_type == USER_PROCESS) &&
	    strncmp(entry->ut_line, cur->ut_line,
		    sizeof cur->ut_line) == 0)
	    return cur;
    } while ((cur = getutent()) != (struct utmp *)NULL);

    return (struct utmp *)NULL;  /* not found */
}

/*
 * pututline() --
 *    writes the structure sent into the utmp file.  If there is
 *    already an entry with the same id, then it is overwritten,
 *    otherwise a new entry is made at the end of the utmp file.
 */

#ifdef _NAMESPACE_CLEAN
#undef pututline
#pragma _HP_SECONDARY_DEF _pututline pututline
#define pututline _pututline
#endif

struct utmp *
pututline(entry)
struct utmp *entry;
{
    long start_pos;          /* Where we started looking */
    int nwrite;              /* return value of write()  */
    register int found;      /* did we find a matching entry */
    struct utmp *answer;     /* what we are going to return */
    struct utmp tmpbuf;      /* copy of user's entry */
#ifdef ERRDEBUG
    int look1_cnt = 0;
    int look2_cnt = 0;

    gdebug("pututline() looking for %-4.4s\n", entry->ut_id);
#endif

    /*
     * Copy the user supplied entry into our temporary buffer to
     * avoid the possibility that the user is actually passing us
     * the address of "ubuf".
     */
    tmpbuf = *entry;

    /*
     * If the utmp file isn't open, open it (and buffer up the first
     * element).  If the file still isn't open, return NULL.
     */
    if (fd < 0)
    {
#ifdef ERRDEBUG
	gdebug("\tfile %s is not open yet\n", utmpfile);
#endif
	getutent();
	if (fd < 0)
	{
#ifdef ERRDEBUG
	    gdebug("\tstill couldn't open utmp file %s, errno is %d\n",
		utmpfile, errno);
#endif
	    return (struct utmp *)NULL;
	}
    }

    /*
     * Make sure file is writable
     */
    if ((fd_flags & O_RDWR) != O_RDWR)
    {
#ifdef ERRDEBUG
	gdebug("\tutmp file %s is not writable\n", utmpfile);
#endif
	return (struct utmp *)NULL;
    }

    /*
     * Search for the entry from the current position to the end of
     * the utmp file.
     */
    start_pos = loc_utmp == 0 ? 0 : loc_utmp - sizeof (struct utmp);
    found = FALSE;
    do
    {
	found = id_match(&ubuf, &tmpbuf);
#ifdef ERRDEBUG
	look1_cnt++;
#endif
    } while (!found && getutent() != (struct utmp *)NULL);

#ifdef ERRDEBUG
    gdebug("\t1st search read %d entries\n", look1_cnt);
#endif

    /*
     * The entry wasn't found.  Reset the file to the beginning and
     * search from there to where we started before.
     */
    if (!found && start_pos > 0)
    {
#ifdef ERRDEBUG
	gdebug("\t2nd search for %-4.4s, start_pos was %ld",
	    tmpbuf.ut_id, start_pos);
#endif
	setutent();
	while (!found && loc_utmp < start_pos &&
	       getutent() != (struct utmp *)NULL)
	{
	    found = id_match(&ubuf, &tmpbuf);
#ifdef ERRDEBUG
	    look2_cnt++;
#endif
	}
    }

#ifdef ERRDEBUG
    gdebug("\t2nd search read %d entries, found is %s\n",
	look2_cnt, found ? "TRUE" : "FALSE");
#endif

    /*
     * If the entry is found, just seek back to where it goes and
     * write it.
     *
     * If we didn't find the entry, seek to the end of the file and
     * write the entry there.
     *
     * We don't actually use seek to append to the file.  Instead, we
     * change the file modes to O_APPEND.  This is so that the write
     * to the end is an atomic operation.  If we used lseek(), there is
     * the possibility of two processes writing on top of each other.
     */
    if (found)
    {
	if ((loc_utmp -= sizeof (struct utmp)) < 0)
	    loc_utmp = 0;
	else
	{
	    /*
	     * Make sure that we are at an integral offset into the
	     * utmp file.
	     */
	    if ((loc_utmp % sizeof (struct utmp)) != 0)
		loc_utmp -= (loc_utmp % sizeof tmpbuf);
	}
	lseek(fd, loc_utmp, SEEK_SET);
    }
    else
	fcntl(fd, F_SETFL, fd_flags|O_APPEND);

    /*
     * Write out the user supplied structure.
     */
    if ((nwrite = write(fd, &tmpbuf, sizeof tmpbuf)) != sizeof tmpbuf)
    {
#ifdef ERRDEBUG
	gdebug("pututline() failed: wrote %d bytes, errno is %d\n",
	    nwrite, errno);
#endif
	if (nwrite > 0)
	    loc_utmp += nwrite;
	answer = (struct utmp *)NULL;
    }
    else
    {
	/*
	 * Update where we are in the utmp file and copy the user
	 * structure into ubuf so that it will be up to date in the
	 * future.
	 */
	loc_utmp += sizeof tmpbuf;
	ubuf = tmpbuf;
	answer = &ubuf;
    }

    /*
     * If we had set the file mode to O_APPEND, set it back and figure
     * out where we are in the utmp file.
     */
    if (!found)
    {
	fcntl(fd, F_SETFL, fd_flags);
	loc_utmp = lseek(fd, 0, SEEK_CUR);
    }
    return answer;
}

#ifdef ERRDEBUG
gdebug(format, arg1, arg2, arg3, arg4, arg5, arg6)
char *format;
int arg1, arg2, arg3, arg4, arg5, arg6;
{
    register FILE *fp;
    register int errnum;

    if ((fp = fopen("/etc/dbg.getut", "a+")) == NULL)
	return;
    fprintf(fp, format, arg1, arg2, arg3, arg4, arg5, arg6);
    fclose(fp);
}
#endif
