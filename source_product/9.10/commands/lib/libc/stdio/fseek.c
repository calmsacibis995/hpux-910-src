/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/
/*
 * Seek for standard library.  Coordinates with buffering.
 */

#ifdef _NAMESPACE_CLEAN
#define lseek _lseek
#define fseek _fseek
#ifdef __lint
#  define fileno _fileno
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <sys/types.h>		/* for off_t */
#include "stdiom.h"

extern off_t lseek();
extern int __fflush();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fseek
#pragma _HP_SECONDARY_DEF _fseek fseek
#define fseek _fseek
#endif

int
fseek(iop, offset, ptrname)
register FILE *iop;
long	offset;
int	ptrname;
{
	register int c, f_ret;
	off_t	p;
	register int fno = fileno(iop);

	if (! iop) return -1;		/* leave if NULL file pointer */

	if(iop->_flag & _IOREAD) {
		iop->_flag &= ~_IOEOF;
		if(ptrname < SEEK_END && iop->_base && !(iop->_flag&_IONBF)) {
			c = iop->_cnt;
			p = (off_t)offset;
			if(ptrname == SEEK_SET)
				p += (off_t)c-lseek(fno, (off_t)0, SEEK_CUR);
			else
				offset -= (long)c;
			if (!(iop->_flag&_IORW) && c > 0 && p <= c &&
					(p >= iop->_base - iop->_ptr) &&
					!(iop->_flag&_IOBUFDIRTY)) {
				iop->_ptr += (int)p;
				iop->_cnt -= (int)p;
				return(0);
			}
/* ANSI C requires that fseek undo any pushed-back characters */
		}
		if ((p = lseek(fno, (off_t)offset, ptrname)) != -1) {
			if(iop->_flag & _IORW) {
				iop->_ptr = iop->_base;
				iop->_flag &= ~_IOREAD;
			}
			iop->_cnt = 0;
			iop->_flag &= ~_IOBUFDIRTY;
		}
	}
	else if(iop->_flag & (_IOWRT | _IORW)) {
		iop->_flag &= ~_IOEOF;
		f_ret = __fflush(iop);
		iop->_cnt = 0;
		if(iop->_flag & _IORW) {
			iop->_flag &= ~_IOWRT;
			iop->_ptr = iop->_base;
		}
		/* We put this check here (before the lseek()) since ANSI C
		   says that fseek() returns EOF ONLY when it cannot
		   perform the seek.  There are cases when the lseek will
		   succeed even though the the fflush() fails (such as the
		   case with EFBIG and want to seek to beginning of file
		 */

		if (f_ret == EOF)
			return(EOF);
		p = lseek(fno, (off_t)offset, ptrname);
	}
	else {
		return(-1);
	}
	return((p == (off_t)-1)? -1: 0);
}
