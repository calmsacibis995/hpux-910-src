/* @(#) $Revision: 64.1 $ */   

/*
**************************************************************************
** Include Files
**************************************************************************
*/

#include <nl_types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "justify.h"
#include "extern.h"

/*
**************************************************************************
** External references
**************************************************************************
*/

extern void put_message();		/* put message on stderr */
extern char *strord();			/* change order (Key, Screen) */
extern char *malloc();			/* get memory */
extern void free();			/* free memory */

extern UCHAR arabfnts[];		/* actual printer fonts */
extern UCHAR arabenhance[];		/* enhanced printer fonts */
extern UCHAR *laam_alif();		/* single laam alif char */
extern int context_analysis();		/* context analysis routine */

/*
**************************************************************************
** Forward references
**************************************************************************
*/

extern void leading_alt();		/* get leading alternative spaces */
extern void replace_tabs();		/* replace a char with a tab */
extern void expand_tabs();		/* tabs to spaces */

/*
**************************************************************************
** Do The Justification
**************************************************************************
*/

void
justify()
{
	register UCHAR *p1;		/* input buffer pointer */
	register UCHAR *p2;		/* output buffer pointer */
	register UCHAR *wrap_boundary;	/* char following wrap boundary */
	register UCHAR *opp_lang;	/* opp lang string at wrap boundary */
	register int opp_char;		/* number of opposite lang chars */
	register int mar_spaces;	/* num spaces to right just margin */
	register int leading;		/* num spaces to right just line */
	register int i;			/* loop counter */
	register UCHAR space;		/* kind of space for leading blanks */

	if (HaveWrap) {
		HaveWrap = FALSE;
	} else {	/* no wrap from previous line */
		if (LBlanks) leading_alt();
		if (Tab.tab != '\t') replace_tabs();
		if (Order == NL_KEY && Lang != OTHER) {
			/* convert key order to screen order */
			(void) strord(OutBuf,InBuf,Mode);
			SWITCH(OutBuf,InBuf,p1);
		}
		expand_tabs();
	}

	/* get number of leading blanks */
	if ((leading = Margin - Len) < 0) {
		/* have a wrap or truncation */
		/* an opp lang char following the wrap boundary ? */
		if ((OPP_LANG(*(wrap_boundary = InBuf+Margin))) && Lang != OTHER) {
			/* get number of opp lang char's before wrap boundary */
			opp_char = 0;
			for (p1=wrap_boundary-1 ; p1>=InBuf && OPP_LANG(*p1) ; p1--) {
				opp_char++;
			}
			/* split an opposite language string ? */
			if (opp_char) {
				/* move complete opp lang string to output buffer */
				p1++;
				p2 = OutBuf;
				for ( ; *p1 && OPP_LANG(*p1) ; p1++, p2++) {
					*p2 = *p1;
				}
				*p2 = EOL;

				/* put opp lang string in keyboard order */
				if (!(opp_lang = (UCHAR *)malloc((unsigned)strlen(OutBuf)+1))) {
					put_message(FATAL,NO_MEMORY);
				}
				(void) strord(opp_lang,OutBuf,Mode);

				/* rewrite part before the wrap boundary */
				p1 = opp_lang;
				p2 = wrap_boundary - 1;
				for (i=0 ; i < opp_char ; i++, p1++, p2--) {
					*p2 = *p1;
				}

				/* rewrite part after the wrap boundary */
				p1 = opp_lang + strlen(opp_lang) - 1;
				p2 = wrap_boundary ;
				for ( ; *p2 && OPP_LANG(*p2) ; p1-- , p2++) {
					*p2 = *p1;
				}

				/* free up memory of opp lang wrap */
				if (opp_lang) free(opp_lang);
			}
		}
		if (End == WRAP) {
			/* save extra stuff in wrap buffer */
			(void) strcpy(WrapBuf,wrap_boundary);
			HaveWrap = TRUE;
		}
		InBuf[Margin] = EOL;	/* truncate input string */
		leading = 0;		/* no leading blanks */
		Len = Margin;		/* max length */
	}

	if (Just == LEFT) {		/* left justification */
		/* copy input to output */
		(void) strcpy(OutBuf,InBuf);
	} else {			/* right justification */
		/* get type of leading blank */
		space = SPACE;
		if (Lang == ARABIC && NL_CHAR(*(InBuf+Len-1))) {
			space = AltSpace;
		}

		/* right justify the margin */
		mar_spaces = Width - Margin;
		for ( i=0, p2=OutBuf ; i < mar_spaces ; i++, p2++ ) {
			*p2 = space;
		}

		/* copy leading blanks from the margin to the line */
		for ( i=0 ; i < leading ; i++, p2++ ) {
			*p2 = space;
		}

		if (Lang == OTHER) {
			/* copy input into output just beyond leading blanks */
			for ( i=0, p1=InBuf ; i < Len ; i++, p1++, p2++ ) {
				*p2 = *p1;
			}
		} else {
			/* flip input into output just beyond leading blanks */
			for ( i=0, p1=InBuf+Len-1 ; i < Len ; i++, p1--, p2++ ) {
				*p2 = *p1;
			}
		}
		*p2 = EOL;		/* be sure string is null terminated */
	}
}

/*
**************************************************************************
** Shift between primary & secondary languages using shift-in & shift-out.
**************************************************************************
*/

void
shift()
{
	register UCHAR *p1;		/* pointer to input buffer */
	register UCHAR *p2;		/* pointer to output buffer */
	register int len;		/* length of string */
	register int IN_NL;		/* state of string */
	register UCHAR ch;		/* pointer to output buffer */

	SWITCH(OutBuf,InBuf,p1);	/* get new input buffer */

	IN_NL = ~NL_CHAR(*p1);
	for ( p1=InBuf, p2=OutBuf, len=0 ; *p1 ; ) {
		ch = ( NL_CHAR(*p1) == IN_NL ) ? *p1++ 
			: ( IN_NL = NL_CHAR(*p1) ) ? SI : SO;
		STORE(p2++,ch,len++);
	}
	*p2 = EOL;			/* terminate output string */
}

/*
**************************************************************************
** Get Printer Fonts Thru Context Analysis
**************************************************************************
*/

void
shapes()
{
	register UCHAR *p1;		/* pointer to input buffer */
	register UCHAR *p2;		/* pointer to output buffer */
	register UCHAR *p3;		/* pointer to printer fonts */
	register int i;			/* loop counter */
	register int pos;		/* position within input buffer */

	SWITCH(OutBuf,InBuf,p2);	/* get new input buffer */

	pos = strlen(InBuf) + 1;	/* get position past the null */

	for (i=0, p1=InBuf+pos ; i < FILL ; i++, p1++, pos++ ) {
		*p1 = EOL;	/* be sure end of input buffer's clean */
	}

	/* get the appropriate printer font table */
	p3 = Enhanced ? arabenhance : arabfnts;

	for ( p1=InBuf, p2=OutBuf ; *p1 ; p1++, p2++ ) {
		/* get arabic font of current character */
		*p2 = (UCHAR) context_analysis(p1,p3,TRUE);
	}
	*p2 = EOL;

	/* if necessary get single laam-alif font */
	if (Enhanced) {
		SWITCH(OutBuf,InBuf,p1);
		(void) laam_alif(OutBuf,InBuf);
	}
}

/*
**************************************************************************
** Replace leading spaces with alternative space
**************************************************************************
*/

void
leading_alt()
{
	register UCHAR *p1;
	
	for (p1=InBuf; _ISSPACE(*p1) ; p1++) *p1 = AltSpace;
}

/*
**************************************************************************
** Replace a specified character with a tab
**************************************************************************
*/

void
replace_tabs()
{
	register UCHAR *p1;

	for (p1=InBuf ; *p1 ; p1++) if (*p1 == Tab.tab) *p1 = '\t';
}

/*
**************************************************************************
** Expand tabs to spaces
**************************************************************************
*/

void
expand_tabs()
{
	register UCHAR *p1;		/* input buffer pointer */
	register UCHAR *p2;		/* output buffer pointer */
	register int pos;		/* column position base 0 */
	register int tab_spaces;	/* number of spaces to next tab */
	register int i;			/* loop counter */

	for (p1=InBuf, p2=OutBuf, pos=0 ; *p1 ; p1++, p2++, pos++) {
		if (*p1 == '\t') {
			/* expand tab char to next tab stop */
			tab_spaces = Tab.stop - (pos % Tab.stop);
			for ( i=0 ; i<tab_spaces ; i++,p2++,pos++) {
				STORE(p2,SPACE,pos);
			}
			p2--; pos--;
		} else {
			/* no tab, so simply copy to output buffer */
			STORE(p2,*p1,pos);
		}
	}
	*p2 = EOL;			/* be sure string is null terminated */
	SWITCH(OutBuf,InBuf,p1);	/* new input buffer */
	Len = strlen(InBuf);		/* new length reflecting expansion */
	if (LBlanks) leading_alt();	/* leading blanks with alt space */
}
