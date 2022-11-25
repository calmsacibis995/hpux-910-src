/* @(#) $Revision: 64.10 $ */      
/* LINTLIBRARY */
/*
 *	nl_ascxtime(), nl_cxtime(), nl_asctime(), nl_ctime()
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _nl_failid __nl_failid
#define idtolang _idtolang
#define langinit _langinit
#define localtime _localtime
#define strftime _strftime
#define nl_asctime _nl_asctime
#define nl_ascxtime _nl_ascxtime
#define nl_ctime _nl_ctime
#define nl_cxtime _nl_cxtime
#endif

#include	<time.h>
#include	<limits.h>
#include	<locale.h>
#include	<setlocale.h>

extern char		*idtolang();

extern int		_nl_failid;				/* part of langinit() functionality	*/
static unsigned char	datebuf[NL_MAXDATE];			/* oversize for safety			*/
static char		*add_newline();				/* defined and used below		*/


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_ctime
#pragma _HP_SECONDARY_DEF _nl_ctime nl_ctime
#define nl_ctime _nl_ctime
#endif

char *nl_ctime(timer, format, langid)
const time_t	*timer;
const char	*format;
int		langid;
{
	if (langid != __nl_langid[LC_TIME] && langid != _nl_failid)
		(void) langinit(idtolang(langid));

	return add_newline((unsigned char *) nl_ascxtime(localtime(timer), format));
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_asctime
#pragma _HP_SECONDARY_DEF _nl_asctime nl_asctime
#define nl_asctime _nl_asctime
#endif

char *nl_asctime(timeptr, format, langid)
const struct tm	*timeptr;
const char	*format;
int		langid;
{
	if (langid != __nl_langid[LC_TIME] && langid != _nl_failid)
		(void) langinit(idtolang(langid));

	return add_newline((unsigned char *) nl_ascxtime(timeptr, format));
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_ascxtime
#pragma _HP_SECONDARY_DEF _nl_ascxtime nl_ascxtime
#define nl_ascxtime _nl_ascxtime
#endif

char * nl_ascxtime(timeptr, format)
const struct tm	*timeptr;
const char	*format;
{
	(void) strftime(datebuf, NL_MAXDATE, ((format && *format) ? format : "%c"), timeptr);
	return (char *)datebuf;
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_cxtime
#pragma _HP_SECONDARY_DEF _nl_cxtime nl_cxtime
#define nl_cxtime _nl_cxtime
#endif

char * nl_cxtime(timer, format)
const time_t	*timer;
const char	*format;
{
	return nl_ascxtime(localtime(timer), format);
}


/********************* supporting function ************************/

static char *add_newline(str)
register unsigned char	*str;
{
	register int i = 0;

	while (str[i])
		++i;

	if (i >= NL_MAXDATE-2)
		i = NL_MAXDATE-2;

	str[i]   = '\n';
	str[++i] = '\0';

	return ((char *) str);
}
