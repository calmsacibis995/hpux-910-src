/* @(#) $Revision: 51.1 $ */   
/* LINTLIBRARY */
/*
    8/27/87:

	1)  Rewritten to handle 16 bit characters.
*/

#include	<stdio.h>
#include   	<ctype.h>

#if defined NLS || defined NLS16
#include	<nl_ctype.h>
#else
#define ADVANCE(p)	(++p)
#define CHARAT(p)	(*p)
#define CHARADV(p)	(*p++)
#define PCHAR(c,p)	(*p = c)
#define PCHARADV(c,p)	(*p++ = c)
#endif

#define FALSE	0
#define TRUE	1
#define UCHAR	unsigned char
#define UINT	unsigned int

/*
    _nl_scan() will read the caller's string until %digits$ or
    the end of the string is detected.

    SYNTAX:

	int _nl_scan(fmt)
	UCHAR *fmt;	-> format string to be scanned

	return(0)	: no % detected or first % wasn't %digits$
	return(1)	: first % was %digits$
*/

int _nl_scan(fmt)
register UCHAR *fmt;
{
    register UINT achar, found_digit;

    while (achar = CHARADV(fmt)) {
	if (achar != '%')
	    continue;
	achar = CHARADV(fmt);
	if (achar == '%')
	    continue;
	found_digit = FALSE;
	while (isascii(achar) && isdigit(achar)) {
	    found_digit = TRUE;
	    achar = CHARADV(fmt);
	}
	if (found_digit && achar == '$')
	    return(1);
    }
    return(0);
}
