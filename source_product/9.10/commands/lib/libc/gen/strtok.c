/* @(#) $Revision: 64.2 $ */      
/*LINTLIBRARY*/
/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */
#ifdef _NAMESPACE_CLEAN
#define strspn _strspn
#define strpbrk _strpbrk
#define strtok  _strtok
#endif

#define	NULL	(char*)0

extern int strspn();
extern char *strpbrk();
#ifdef _NAMESPACE_CLEAN
#undef strtok
#pragma _HP_SECONDARY_DEF _strtok strtok
#define strtok _strtok
#endif
char *
strtok(string, sepset)
char	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/*first or subsequent call*/
	p = (string == NULL)? savept: string;

	if(p == 0)		/* return if no tokens remaining */
		return(NULL);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept = ++r;
	}
	return(q);
}
