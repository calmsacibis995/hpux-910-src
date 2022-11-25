/* @(#) $Revision: 64.3 $ */   

/*LINTLIBRARY*/

/*
** int
** fgetpos( iop, pos)
** FILE *iop;
** fpos_t *pos;
** 
** Function stores the current value of the file position
** indicator for the stream pointed to by iop in the object
** pointed to by pos.  The value stored contains information
** usable by fsetpos for repositioning the stream to its
** position at the time of the call to fgetpos.
**
** If successful, the function returns zero; otherwise  non-zero.
**
** The code assumes fpos_t is a long.  If this changes to a structure,
** the code can no longer be written in terms of ftell(3c).
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fgetpos _fgetpos
#define ftell _ftell
#endif

#include <stdio.h>

extern long ftell();

#define GOOD	0
#define BAD	-1

#ifdef _NAMESPACE_CLEAN
#undef fgetpos
#pragma _HP_SECONDARY_DEF _fgetpos fgetpos
#define fgetpos _fgetpos
#endif /* _NAMESPACE_CLEAN */

int
fgetpos( iop, pos)
FILE *iop;
fpos_t *pos;
{
	long p;

	if ((p = ftell( iop)) == BAD) {
		return BAD;
	}

	*pos = (fpos_t) p;

	return GOOD;
}
