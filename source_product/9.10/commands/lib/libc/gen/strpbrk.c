/* @(#) $Revision: 64.3 $ */     
/*LINTLIBRARY*/
/*
 * Return ptr to first occurance of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */

#define	NULL	(char *) 0

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _strpbrk strpbrk
#define strpbrk _strpbrk
#endif

char *
strpbrk(string, brkset)
register char *string, *brkset;
{
	register char *p;

	do {
		for(p=brkset; *p != '\0' && *p != *string; ++p)
			;
		if(*p != '\0')
			return(string);
	}
	while(*string++);
	return(NULL);
}
