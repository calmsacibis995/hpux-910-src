/* @(#) $Revision: 64.4 $ */     
/*LINTLIBRARY*/
/****************************************************************
 *	Routine expects a string of length at least 6, with
 *	six trailing 'X's.  These will be overlaid with a
 *	letter and the last (5) digigts of the proccess ID.
 *	If every letter (a thru z) thus inserted leads to
 *	an existing file name, your string is shortened to
 *	length zero upon return (first character set to '\0').
 ***************************************************************/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define mktemp _mktemp
#define getpid _getpid
#define strlen _strlen
#define access _access
#endif

extern int strlen(), access(), getpid();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mktemp
#pragma _HP_SECONDARY_DEF _mktemp mktemp
#define mktemp _mktemp
#endif

char *
mktemp(as)
char *as;
{
	register char *s=as;
	register unsigned pid;
	register unsigned i = 0;

	pid = getpid();
	s += strlen(as);	/* point at the terminal null */
	while(*--s == 'X') {
		*s = (pid%10) + '0';
		pid /= 10;
		i++;
	}
	if((*++s) && i>5) {		/* maybe there were less that 6 'X's */
		*s = 'a';
		while(access(as, 0) == 0) {
			if(++*s > 'z') {
				*as = '\0';
				break;
			}
		}
	} else
		if(access(as, 0) == 0)
			*as = '\0';
	return(as);
}
