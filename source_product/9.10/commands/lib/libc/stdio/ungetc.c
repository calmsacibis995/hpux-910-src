/* @(#) $Revision: 66.1 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define ungetc _ungetc
#define _findbuf __findbuf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include "stdiom.h"

#ifdef _NAMESPACE_CLEAN
#undef ungetc
#pragma _HP_SECONDARY_DEF _ungetc ungetc
#define ungetc _ungetc
#endif /* _NAMESPACE_CLEAN */

extern _findbuf();

int
ungetc(c, iop)
int	c;
register FILE *iop;
{
	if(c == EOF)
		return(EOF);
        /* it should be possible to do an ungetc on an empty file stream */
        /* so get buffer if we don't have one */

	if ((iop->_ptr == NULL) && (iop->_base == NULL) )  
		_findbuf(iop);

	if((iop->_flag & _IOREAD) == 0 || iop->_ptr <= iop->_base)
		if(iop->_ptr == iop->_base && iop->_cnt == 0)
			++iop->_ptr;
		else
			return(EOF);
	*--iop->_ptr = c;
	++iop->_cnt;

/* According to ANSI C, a successful call to the ungetc function clears
   the end-of-file indicator for the stream.
*/
	iop->_flag &= ~_IOEOF;
	if (iop->_flag & _IORW)
                        iop->_flag |= _IOREAD;
	iop->_flag |= _IOBUFDIRTY;
	return(c);
}
