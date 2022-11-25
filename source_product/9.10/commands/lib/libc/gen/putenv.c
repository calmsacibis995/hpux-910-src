/* @(#) $Revision: 70.2 $ */      
/*	LINTLIBRARY	*/
/* 
	putenv   -- change environment variables
	clearenv -- clear the process environment
*/

#ifdef _NAMESPACE_CLEAN
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#define realloc _realloc
#       endif  /* _ANSIC_CLEAN */
#define environ _environ
#define memcpy _memcpy
#define putenv _putenv
#define clearenv _clearenv
#endif

#include <nl_ctype.h>
#include <errno.h>

#define NULL 0
extern char **environ;		/* pointer to enviroment */
static reall = 0;		/* flag to reallocate space, if putenv is called
				   more than once */

#ifdef _NAMESPACE_CLEAN
#undef putenv
#pragma _HP_SECONDARY_DEF _putenv putenv
#define putenv _putenv
#endif /* _NAMESPACE_CLEAN */

/*	putenv - change environment variables
	input - char *change = a pointer to a string of the form
			       "name=value"
	output - 0, if successful
		 1, otherwise
*/
int
putenv(change)
char *change;
{
	char **newenv;		    /* points to new environment */
	register int which;	    /* index of variable to replace */
	char *realloc(), *malloc(); /* memory alloc routines */

	if ((which = find(change)) < 0)  {
		/* if a new variable */
		/* which is negative of table size, so invert and
		   count new element */
		which = (-which) + 1;
		if (reall)  {
			/* we have expanded environ before */
			newenv = (char **)realloc(environ,
				  which*sizeof(char *));
			if (newenv == NULL)  return -1;
			/* now that we have space, change environ */
			environ = newenv;
		} else {
			/* environ points to the original space */
			reall++;
			newenv = (char **)malloc(which*sizeof(char *));
			if (newenv == NULL)  return -1;
			(void)memcpy((char *)newenv, (char *)environ,
 				(int)(which*sizeof(char *)));
			environ = newenv;
		}
		environ[which-2] = change;
		environ[which-1] = NULL;
	}  else  {
		/* we are replacing an old variable */
		environ[which] = change;
	}
	return 0;
}

#ifdef AES

#ifdef _NAMESPACE_CLEAN
#undef clearenv
#pragma _HP_SECONDARY_DEF _clearenv clearenv
#define clearenv _clearenv
#endif /* _NAMESPACE_CLEAN */
int clearenv()
{
	char **newenv;

	if (reall) { 		/* we can free up previous malloc'd space */
		if ((newenv=(char **)realloc(environ, sizeof(char *))) == NULL)
		{
			errno=ENOMEM;
			return -1;;
		}
		environ=newenv;		/* environ now has only one slot */
	}
	environ[0]=NULL;
	return 0;
}

#endif /* AES */

/*	find - find where s2 is in environ
 *
 *	input - str = string of form name=value
 *
 *	output - index of name in environ that matches "name"
 *		 -size of table, if none exists
*/
static
find(str)
register char *str;
{
	register int ct = 0;	/* index into environ */

	while(environ[ct] != NULL)   {
		if (match(environ[ct], str)  != 0)
			return ct;
		ct++;
	}
	return -(++ct);
}
/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of 1,
 *	else return 0
 */

static
match(s1, s2)
register char *s1, *s2;
{
	register int s1_char, s2_char;

	for (;;) {
		s1_char = CHARADV(s1);
		s2_char = CHARADV(s2);
		if ((s1_char != s2_char) || !s1_char)
			return(0);
		if (s1_char == '=')
			return(1);
	}
}
