/* @(#) $Revision: 66.1 $ */     
/*LINTLIBRARY*/
/*
 *	tmpfile - return a pointer to an update file that can be
 *		used for scratch. The file will automatically
 *		go away if the program using it terminates.
 */

#ifdef _NAMESPACE_CLEAN
#define fopen _fopen
#define unlink _unlink
#define tmpnam _tmpnam
#define tmpfile _tmpfile
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

extern FILE *fopen();
extern int unlink();
extern char *tmpnam();

#ifdef _NAMESPACE_CLEAN
#undef tmpfile
#pragma _HP_SECONDARY_DEF _tmpfile tmpfile
#define tmpfile _tmpfile
#endif /* _NAMESPACE_CLEAN */

FILE *
tmpfile()
{
	char	tfname[L_tmpnam];
	register FILE	*p;

	(void) tmpnam(tfname);
	if((p = fopen(tfname, "wb+")) == NULL)
		return NULL;
	else
		(void) unlink(tfname);
	return(p);
}
