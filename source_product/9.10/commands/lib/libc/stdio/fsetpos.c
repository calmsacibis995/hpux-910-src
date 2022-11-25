/* @(#) $Revision: 64.3 $ */   

/*LINTLIBRARY*/

/*
** int fsetpos (iop, pos)
** FILE *iop;
** fpos_t *pos;
** 
** Fsetpos sets the file position indicator for the stream
** pointed to by iop according to the value of the object
** pointed to by pos, which shall be a value set by an earlier
** call to fgetpos on the same stream.
** 
** If successful, the function returns zero;  otherwise non-zero.
**
** The code assumes fpos_t is a long.  If this changes to a structure,
** the code can no longer be written in terms of fseek(3c).
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fseek _fseek
#define fsetpos _fsetpos
#endif

#include <stdio.h>

#ifdef _NAMESPACE_CLEAN
#undef fsetpos
#pragma _HP_SECONDARY_DEF _fsetpos fsetpos
#define fsetpos _fsetpos
#endif /* _NAMESPACE_CLEAN */

int
fsetpos( iop, pos)
FILE *iop;
const fpos_t *pos;
{
	return fseek( iop, (long) *pos, SEEK_SET);
}
