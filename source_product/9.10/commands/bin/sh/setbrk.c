/* @(#) $Revision: 32.1 $ */    
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"

tchar 	*sbrk();

tchar*
setbrk(incr)
{

	register tchar *a = sbrk(incr);

	brkend = (tchar *)(Rcheat(a) + incr);
	return(a);
}
