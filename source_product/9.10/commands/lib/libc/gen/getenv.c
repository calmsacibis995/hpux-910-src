/* @(#) $Revision: 64.6 $ */      
/*LINTLIBRARY*/
/*
 *	getenv(name)
 *	returns ptr to value associated with name, if any, else NULL
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
# define environ _environ
#define getenv _getenv
#endif

#include <nl_ctype.h>

#define NULL	0
extern char **environ;
static char *nvmatch();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getenv
#pragma _HP_SECONDARY_DEF _getenv getenv
#define getenv _getenv
#endif

char *
getenv(name)
register char *name;
{
	register char *v, **p=environ;

	if(name == NULL || *name == '\0')
		return(NULL);
	if(p == NULL)
		return(NULL);
	while(*p != NULL)
		if((v = nvmatch(name, *p++)) != NULL)
			return(v);
	return(NULL);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 *	used for environment searching: see getenv
 */

static char *
nvmatch(s1, s2)
register char *s1, *s2;
{
	register unsigned int s1_char, s2_char;

	for (;;) {
		s1_char = CHARADV(s1);
	    	s2_char = CHARADV(s2);
		if (!s1_char || s1_char == '=') {
			if(s2_char == '=')
				return(s2);
			return((char *) NULL);
		}
		if (s1_char != s2_char)
			return((char *) NULL);
	}
}
