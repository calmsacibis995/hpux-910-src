/* @(#) $Revision: 66.1 $ */      
/* Note this file needs changed when C id is assigned */


/* @(#) $Revision: 66.1 $ */      
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define idtolang _idtolang
#define open _open
#define read _read
#define close _close
#define langtoid _langtoid
#define strcmp _strcmp
#define currlangid _currlangid
#define getenv _getenv
#define _nl_failid __nl_failid
#endif

#include	<fcntl.h>
#include	<stdio.h>
#include	<limits.h>
#include	<setlocale.h>
#include	<langinfo.h>

#define N_COMPUTER 0							/* native computer - default to ASCII */
/* this must be changed when C id is assigned */
#define C	   99 

#define SAME 0

int _nl_errno;								/* read by nl_init & strcmp8 */

extern int _nl_failid;

static unsigned char id_buf[NL_LANGMAX+1];				/* external buf returned to caller */

/*
**	Convert numeric langid to language name string
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef idtolang
#pragma _HP_SECONDARY_DEF _idtolang idtolang
#define idtolang _idtolang
#endif

char *idtolang(langid)
{
	register int fd;
	register unsigned char *p;
	char cbuf[1];
	register char *c = cbuf;
	int lang;
	register int i;

	_nl_errno = 0;

	if (langid == N_COMPUTER)
		return "n-computer";

	if (langid == C)
		return "C";

	if ((fd = open(NL_WHICHLANGS, O_RDONLY)) < 0) {			/* nope, have to match against the config file */
		_nl_errno = ENOCFFILE ;
		_nl_failid=langid;
		return "\0";
	}
	/*
	**  read entries from the file until we get a match or there are no more
	**  (don't use scanf here--has lots of generality which means too much
	**  overhead code size for application programs that don't otherwise
	**  need scanf)
	*/
	while (read(fd, c, 1) == 1) {
		lang = *c - '0';					/* parse langid # until a space is found */
		while ((read(fd, c, 1) == 1) && *c != ' ')
			lang = 10 * lang + *c - '0';
		if (lang == langid) {
			p = id_buf;
			i = 0;
			while ((read(fd, c, 1) == 1) && *c != '\n')	/* rest of line is the language name */
			    if ( i++ < NL_LANGMAX )
				*p++ = *c;
			*p = '\0';
			(void) close(fd);
			return (char *)id_buf;
		} else
			while ((read(fd, c, 1) == 1) && *c != '\n');
	}

	(void) close(fd);

	_nl_errno = ENOCONV;						/* langname not in config file */
	_nl_failid = langid;
	return "\0";
}



/*
**	Convert language name string to numeric langid
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef langtoid
#pragma _HP_SECONDARY_DEF _langtoid langtoid
#define langtoid _langtoid
#endif

int langtoid(langname)
char *langname;
{
	unsigned char buf[NL_LANGMAX+1];
	register int fd;
	register unsigned char *p;
	char cbuf[1];
	register char *c = cbuf;
	register int lang;
	register int i;

	_nl_errno = 0;

	if (strcmp(langname, "n-computer") == SAME)		/* or cause its n-computer */
		return N_COMPUTER;

	else if (strcmp(langname,"C") == SAME)
		return C;
	if ((fd = open(NL_WHICHLANGS, O_RDONLY)) < 0) {			/* nope, have to match against the config file */
		_nl_errno = ENOCFFILE ;
		return N_COMPUTER;
	}
	/*
	**  read entries from the file until we get a match or there are no more
	**  (don't use scanf here--has lots of generality which means too much
	**  overhead code size for application programs that don't otherwise
	**  need scanf)
	*/
	while (read(fd, c, 1) == 1) {
		lang = *c - '0';					/* parse langid # until a space is found */
		while ((read(fd, c, 1) == 1) && *c != ' ')
			lang = 10 * lang + *c - '0';
		p = buf;
		i = 0;
		while ((read(fd, c, 1) == 1) && *c != '\n')		/* rest of line is the language name */
		    if ( i++ < NL_LANGMAX )
			*p++ = *c;
		*p = '\0';
		if (strcmp(langname, (char *)buf) == SAME) {		/* done if this is name we wanted */
			(void) close(fd);
			return lang;
		}
	}

	(void) close(fd);

	_nl_errno = ENOCONV;						/* langname not in config file */
	return N_COMPUTER;
}



/*	
**	Get LANG value from environment and convert to integer language id
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef currlangid
#pragma _HP_SECONDARY_DEF _currlangid currlangid
#define currlangid _currlangid
#endif

int currlangid()
{
	char *getenv();
	char *langname;

	_nl_errno = 0;

	if ((langname = getenv("LANG")) == NULL) 
		return N_COMPUTER;					/* default */

	return langtoid(langname);
}
