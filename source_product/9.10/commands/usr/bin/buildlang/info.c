/* @(#) $Revision: 64.2 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	<nl_ctype.h>
#include	"global.h"

#define	isoctal(c)	('0' <= c && c <= '7')
int	gotstr;

/* info_init: initialize langinfo table.
** Get here when you see a langinfo-item keyword.
*/
void
info_init(token)
int	token;				/* keyword token */
{
	extern void info_str();
	extern void info_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = info_str;		/* set string function for info.c */
	finish = info_finish;		/* set up info finish */

	gotstr = FALSE;			/* no langinfo string yet */
}

/* info_str: process one langinfo string.
** Check length of the langinfo string, and remove quote characters.
*/
void
info_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr();
	unsigned char buf[LEN_INFO_MSGS];
	unsigned char *cp;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process langinfo string */
	if (strlen(buf) > LEN_INFO_MSGS-1)	/* check length */
		error(INFO_LEN);
	if ((cp = (unsigned char *)malloc((unsigned) strlen((char *)buf)+1))
		== NULL)
		error(NOMEM);
	else {					/* store langinfo string */
		info_tab[num_value] = cp;
		(void) strcpy((char *)cp, (char *)buf);
	}

	gotstr++;
}

/* info_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars or item number.
*/
void
info_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}

/* getstr: process one quoted string.
** Take string from yytext, remove double quote '"' characters,
** and check the '\' characters.  Put the result in buf.
** (NLS16 compiling flag is not turned on at this time because
** lex cannot handle byte redefinition.
** Therefore, yytext will not contain double quote '"' as a SECof2 char).
*/
void
getstr(buf)
unsigned char *buf;
{
	unsigned char *p1, *p2, c;
	int	n;
#ifdef NLS16
	int	secondbyte = FALSE;
#endif NLS16
 
	p1 = (unsigned char *)yytext;
	p2 = buf;
	if ((c = *p1) == '"') 
		p1++;
	while (c = *p1++) {
#ifdef NLS16
		if (secondbyte) {
			*p2++ = c;
			secondbyte = FALSE;
		} else if (ctype2[c+c_offset]&_K1) {
			*p2++ = c;
			secondbyte = TRUE;
		} else
#endif NLS16
		if ((unsigned char)c == '"')
			break;
		else if (c == '\\') {		/* '\' */
			c = *p1++;
			if (c == 'n') {	 	/* '\n' */
				*p2++ = '\n';
			}
			else if (c == 't') { 	/* '\t' */
				*p2++ = '\t';
			}
			else if (c == 'b') { 	/* '\b' */
				*p2++ = '\b';
			}
			else if (c == 'r') { 	/* '\r' */
				*p2++ = '\r';
			}
			else if (c == 'f') { 	/* '\f' */
				*p2++ = '\f';
			}
			else if (c == '\n') {
				/* '\carriage_return' */
				/* do nothing */
			}
			else if (isoctal(c)) { 	/* '\nnn' */
				(void) sscanf((char *)&p1[-1], "%3o", &n);
				*p2++ = (char)n;
				if (isoctal(*p1)) {
					p1++;
					if (isoctal(*p1))
						p1++;
				}
			}
#ifdef NLS16
			else if (ctype2[c+c_offset]&_K1) {
				*p2++ = c;
				secondbyte = TRUE;
			}
#endif NLS16
			else {
				*p2++ = c;
			}
		}
		else {
			*p2++ = c;
		}
	}
	*p2 = NULL;
}
