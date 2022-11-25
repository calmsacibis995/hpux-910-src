/* @(#) $Revision: 66.1 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define setvbuf _setvbuf
#define isatty _isatty
#       ifdef   _ANSIC_CLEAN
#define free _free
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

extern void free();
extern char * malloc();

extern int isatty();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setvbuf
#pragma _HP_SECONDARY_DEF _setvbuf setvbuf
#define setvbuf _setvbuf
#endif

int
setvbuf(iop, buf, type, size)
register FILE *iop;
register int type;
register char	*buf;
register size_t size;
{

	if(iop->_base != NULL && iop->_flag & _IOMYBUF)
		free((char*)iop->_base);
	iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
	switch (type)  {
	    /*note that the flags are the same as the possible values for type*/
	    case _IONBF:
		/* file is unbuffered */
		iop->_flag |= _IONBF;
		_bufend(iop) = iop->_base = NULL;
		break;
	    case _IOLBF:
	    case _IOFBF:
		iop->_flag |= type;
		if (buf != NULL) {
		   if (size <= 0) return (-1);
		   iop->_base = (unsigned char *) buf;
		   _bufend(iop) = iop->_base + size;
		   break;
		}
		else  {
		   /* buf pointer null, allocate a buffer here */
		   if (size < 0) return(-1);
		   size = (size == 0) ? BUFSIZ : size;
		   if ( (iop->_base = (unsigned char *) malloc(size+8)) != NULL)
		   {
			_bufend(iop) = iop->_base + size;
			iop->_flag |= _IOMYBUF;
			break;
		   }
		   else {
			iop->_base = NULL;
			return (-1);
		   }
		}
	    default:
		return -1;
	}
	iop->_ptr = iop->_base;
	iop->_cnt = 0;
	return 0;
}
