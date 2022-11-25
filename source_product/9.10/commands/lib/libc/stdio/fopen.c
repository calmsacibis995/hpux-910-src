/* @(#) $Revision: 70.3 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define open    _open
#define close   _close
#define fclose  _fclose
#define lseek   _lseek
#define fopen   _fopen
#define freopen _freopen
#define setbuf  _setbuf
#ifdef _THREAD_SAFE
# define fclose_unlocked  _fclose_unlocked
#endif
#  ifdef _ANSIC_CLEAN
#    define malloc _malloc
#  endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "stdiom.h"
#ifdef _THREAD_SAFE
#include "stdio_lock.h"

extern REC_MUTEX _iop_rmutex;
extern REC_MUTEX _stream_count_rmutex;
#endif

extern int open(), fclose();
extern long lseek();
extern FILE *_findiop();
extern int _ulimit_called;

static FILE *
do_open(file, mode, iop)
char *file;
char *mode;
register FILE *iop;
{
    extern int stream_count;
    register int plus, oflag, fd;
    register char *_bp;

    if (file == NULL || file[0] == '\0')
    {
	errno = ENOENT;
	return (FILE *)NULL;
    }

    if (mode == NULL)
    {
	errno = EINVAL;
	return (FILE *)NULL;
    }

    if (iop == NULL)
    {
	errno = EMFILE;
	return (FILE *)NULL;
    }

    /* 
     * mode[2] must be checked for '+' because of rb+, wb+, and ab+
     */
    plus = ((mode[1] == '+') || ((mode[2] == '+') && (mode[1] == 'b')));
    switch (mode[0])
    {
    case 'w': 
	oflag = (plus ? O_RDWR : O_WRONLY) | O_TRUNC | O_CREAT;
	break;
    case 'a': 
	oflag = (plus ? O_RDWR : O_WRONLY) | O_APPEND | O_CREAT;
	break;
    case 'r': 
	oflag = plus ? O_RDWR : O_RDONLY;
	break;
    default: 
	errno = EINVAL;
	return (FILE *)NULL;
    }

    if ((fd = open(file, oflag, 0666)) < 0)
	return (FILE *)NULL;

    if (mode[0] == 'a')
    {
	if (!plus)
	{
	    /*
	     * if update only mode, move file pointer to the end of the
	     * file
	     */
#ifdef hpux
	    /* 
	     * currently, ignore lseek on pipe error
	     * because Bell has code that fopen's fifo's
	     * in "a+" mode. kah July 84.
	     */
	    if (lseek(fd, 0L, SEEK_END) == -1 && errno != ESPIPE)
#else
	    if (lseek(fd, 0L, SEEK_END) == -1)
#endif
	    {
		/*
		 * The fopen failed, but we have already opened
		 * the file.  We must close it so that we don't
		 * have a dangling file.
		 * We preserve the errno value, since close() may
		 * change it, and we want what lseek() set.
		 */
		int save_errno = errno;

		(void)close(fd);
		errno = save_errno;
		return NULL;
	    }
	}
    }

    /*
     * Initialize the iop structure.
     * We also adjust the count of currently open streams, since
     * we now have a new valid stream.
     */
    iop->_cnt = 0;
    setfileno(iop, fd);
    iop->_flag &= _IOEXT;	/* preserve only the _IOEXT bit */
    iop->_flag |= plus ? _IORW : (mode[0] == 'r') ? _IOREAD : _IOWRT;
#ifdef _THREAD_SAFE
    _rec_mutex_lock(&_stream_count_rmutex);
#endif
    stream_count++;
#ifdef _THREAD_SAFE
    _rec_mutex_unlock(&_stream_count_rmutex);
#endif

    _bufend(iop) = iop->_base = iop->_ptr = NULL;

/*
 * use prs on 37.3 for reasons.
 * This was done to get SVVS to pass when BUFSIZ is not 1k.  I think
 * that it can be removed, but we need to try it on SVVS to be sure.
 *  -- rsh, 12/03/89
 *
 * #ifdef hp9000s800 guard removed so we get the same behavior on all
 * platforms.  SVVS (SVID2) is no longer a problem, but X/Open XPG4 tests
 * are!  Also fixes SR 5003-078907.
 *  -- jms, 09/22/92
 *
 */
    if (_ulimit_called == 0)
    {
	_bp = malloc((unsigned)BUFSIZ);
	setbuf(iop, _bp);
	iop->_flag |= _IOMYBUF;	/* Mark buffer as one we'll free at fclose() */
    }
    return iop;
}

#ifdef _NAMESPACE_CLEAN
#undef fopen
#pragma _HP_SECONDARY_DEF _fopen fopen
#define fopen _fopen
#endif /* _NAMESPACE_CLEAN */

FILE *
fopen(file, mode)
const char	*file, *mode;
{
#ifdef _THREAD_SAFE
    FILE *ret_val;

    _rec_mutex_lock(&_iop_rmutex);
    ret_val = do_open(file, mode, _findiop());
    _rec_mutex_unlock(&_iop_rmutex);
    return ret_val;
#else
    return do_open(file, mode, _findiop());
#endif
}

#ifdef _NAMESPACE_CLEAN
#undef freopen
#pragma _HP_SECONDARY_DEF _freopen freopen
#define freopen _freopen
#endif /* _NAMESPACE_CLEAN */

FILE *
freopen(file, mode, iop)
const char	*file, *mode;
register FILE *iop;
{
    FILE *retfd;
#ifdef _THREAD_SAFE
    register filelock_t filelock;

    filelock = _flockfile(iop);
    (void)fclose_unlocked(iop);	/* doesn't matter if this fails */
#else
    (void)fclose(iop);		/* doesn't matter if this fails */
#endif

    if ((retfd = do_open(file, mode, iop)) != NULL)
	iop->_flag &= ~(_IOERR | _IOEOF); /* clear error and EOF */

#ifdef _THREAD_SAFE
    _funlockfile(filelock);
#endif
    return retfd;
}
