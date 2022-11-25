/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*+*+*+*************************************************************************
*
*                           src/lex.c
*
*   This file contains the lexical analyzer for the cfg file parser. This
*   file is generated by lex and should not be modified.
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "compiler.h"
#include "yyparse.h"
#include "err.h"
#include "compat.h"
#include "def.h"	

/*-----------------------------------------------------------------------------
 *			  manifest constants/macros
 *-----------------------------------------------------------------------------
 */

#define MAXTOK		40
#define BAD_NUM 	~0L		/* for error returns from strtolong */

#define istoggle(c)	(c == '0' || c == '1' || c == 'N' || c == 'n' || c == 'X' || c == 'x')
#define ismaskbit(c)	(c == '0' || c == '1' || c == 'R' || c == 'r' || c == 'X' || c == 'x')

/*-----------------------------------------------------------------------------
 *		   structure/union/enumeration declarations
 *-----------------------------------------------------------------------------
 */

struct kwentry		/* keyword entry */
{
    char *name;
    int  val;
};

/*-----------------------------------------------------------------------------
 *			     function prototypes
 *-----------------------------------------------------------------------------
 */

static unsigned long strtolong();
struct kwentry *kwlookup();

/*-----------------------------------------------------------------------------
 *	      global variables
 *-----------------------------------------------------------------------------
 */

int ch_invalid;
extern int parse_err_flag;
extern void mn_trapfree();
extern void *mn_trapcalloc();
extern  void parse_errmsg();		

static char	*savetext();


/****+++***********************************************************************
*
* Function:     savetext
*
* Parameters:   str		the string to modify
*
* Used:		internal only
*
* Returns:      the modified string
*
* Description:
*
*    This function removes leading and trailing white space from the string.
*
****+++***********************************************************************/

static char *savetext(str)
    char	*str;
{
    char    	*ptr;
    char    	*savestr;
    int 	cnt;
    char   	 skipws = 1;


    /* save a pointer to the saved string */
    ptr = str;

    /* set up the pointer for saving the string */
    savestr = str;

    for (cnt = 0; *str != '\0'; ++str) {
	if (*str == '\n') {

	    /* truncate trailing whitespace */
	    skipws = 1;
	    while (cnt > 0) {
		--savestr;
		if (isspace(*savestr) && (unsigned)*savestr < 128)
		    --cnt;
		else {
		    ++savestr;
		    if (!((*(str - 2) == '\\') && (*(str - 1) == 'n')))
			*savestr++ = ' ';
		    ++cnt;
		    break;
		}
	    }

	    continue;

	}

	else if (skipws && isspace(*str) && (unsigned)*str < 128)
	    continue;

	skipws = 0;

	*savestr++ = *str;
	++cnt;
    }

    *savestr++ = '\0';

    return(ptr);
}

/*-----------------------------------------------------------------------------
 * yylex - Lexical analyzer called by YACC generated parser.  Returns 0 if
 * end of file, -1 if an error, or a token value otherwise.
 * Input stream returns -1 for end of file and -2 for file error.
 *-----------------------------------------------------------------------------
 */

int yylex()
	{
	static int ch;
	static char *delims = "\t\n\v\f\r \"^-=(){}|;,";
	struct kwentry *kwptr;
	int i, temp_ch, numtype, strlength, base, done;

	if (parse_err_flag)				/* abort if error has occurred ** A **/
		return(-1);
	if (ch_invalid) 				/* get next character if necessary */
		if ((ch = (*get_char)()) < 0)
			return(++ch);

	while (1)							/* skip whitespace and comments */
		{
		if (ch == '\n')
			++lineno;
		else if (ch == ';')
			{
			while ((ch = (*get_char)()) != '\n')
				if (ch < 0)
					return(++ch);
			++lineno;
			}
		else if (!isspace(ch))
			break;
		if ((ch = (*get_char)()) < 0)
			return(++ch);
		}

	switch (ch)							/* look for terminals */
		{
			case '^':
			case '-':
			case '=':
			case ',':
			case '(':
			case ')':
			case '{':
			case '}':
			case '|':
				ch_invalid = 1;
				return(ch);
#ifndef LINT
				break;
#endif
			case '"':		/* quoted string */
				i = 0;
				done = 0;
				while (!done)
					{
					ch = (*get_char)();
					switch (ch)
						{
						case -1:
							parse_errmsg(PARSE41_ERRCODE,2);	/* no closing quote */
						case -2:
							return(-1);
						case '"':
							if (done = ((strbuf[i-1] != '\\') || (i == 0)))	
								break;
						case '\n':
							++lineno;
						default:
							if (i < 999)
								strbuf[i++] = ch;
						}
					}
				strbuf[i] = '\0';

					strbuf = savetext(strbuf);
				yylval.str = (char *) mn_trapcalloc(1, strlen(strbuf) + 1);
				(void)strcpy(yylval.str, strbuf);
				ch_invalid = 1;
				return(STRING);
		}

	if (toggles && istoggle(ch))				/* check for toggles */
		{
		for (i = 0; istoggle(ch) && i < MAXTOK - 1; ++i)
			{
			strbuf[i] = ch;
			if ((ch = (*get_char)()) < 0)
				return(++ch);
			}
		strbuf[i] = '\0';
		(void)strupr(strbuf);
		ch_invalid = (toupper(ch) == 'B');	/* skip binary suffix if present */
		yylval.str = (char *) mn_trapcalloc(1, strlen(strbuf) + 1);
		(void)strcpy(yylval.str, strbuf);
		return(TOGGLES);
		}

	if (mask_bits && ismaskbit(ch)) 	/* check for mask bits */
		{
		for (i = 0; ismaskbit(ch) && i < MAXTOK - 1; ++i)
			{
			strbuf[i] = ch;
			if ((ch = (*get_char)()) < 0)
				return(++ch);
			}
		strbuf[i] = '\0';
		(void)strupr(strbuf);
		ch_invalid = (toupper(ch) == 'B');	/* skip binary suffix if present */
		yylval.str = (char *) mn_trapcalloc(1, strlen(strbuf) + 1);
		(void)strcpy(yylval.str, strbuf);
		return(MASK_BITS);
		}

	if (isdigit(ch))			/* check for numbers */
		{
		for (i = 0; isxdigit(ch) && i < MAXTOK - 1; ++i)
			{
			strbuf[i] = ch;
			if ((ch = (*get_char)()) < 0)
				return(++ch);
			}
		if (toupper(ch) == 'Z' && strbuf[0] == '0' && i == 1)
			{
			ch_invalid = 0;
			ch = '0';
			return(EISAPORT);
			}
		strbuf[i] = '\0';
		strlength = i;
		base = 10;
		if (toupper(ch) == 'H')
			{
			base = 16;
			ch_invalid = 1;
			}
		else
			{
			ch_invalid = 0;
			temp_ch = toupper(strbuf[strlength - 1]);
			if (temp_ch == 'B' || temp_ch == 'D')
				strbuf[strlength - 1] = '\0';
			if (temp_ch == 'B')
				base = 2;
			}
		if (base == 10)
			for (i = 0; isdigit(strbuf[i]); ++i)
				;
		else if (base == 2)
			for (i = 0; strbuf[i] == '0' || strbuf[i] == '1'; ++i)
				;
		if (strbuf[i] != '\0' || (yylval.val = strtolong(strbuf, base)) == BAD_NUM)
			{
			err_add_string_parm(2, strbuf);
			parse_errmsg(PARSE25_ERRCODE, 2);
			return(-1);
			}
		switch (base)
			{
			case 2:
				numtype = BINNUM;
				break;
			case 10:
				numtype = DECNUM;
				break;
			case 16:
				numtype = HEXNUM;
				break;
			}
		return(numtype);
		}

		/* check for keyword tokens */

		i = 0;
		while (strchr(delims,ch) != NULL)
			if ((ch = (*get_char)()) < 0)
				return(++ch);
		while (strchr(delims,ch) == NULL)
			{
			strbuf[i++] = ch;
			if ((ch = (*get_char)()) < 0)
				return(++ch);
			}
		strbuf[i] = '\0';
		(void)strupr(strbuf);
		ch_invalid = 0;
		if ((kwptr = kwlookup(strbuf)) != NULL)
			return(kwptr->val);
		err_add_string_parm(2, strbuf);
		parse_errmsg(PARSE55_ERRCODE, 2);
		return(-1);
	}

/*-----------------------------------------------------------------------------
 * kwlookup - Looks up a string in the token table to see if it is a keyword.
 * If it is, this function will return a pointer to the corresponding entry
 * in the keyword table.  If the string is not found, it returns NULL.
 *
 * Note that there are two requirements for this function to work correctly:
 *
 * 1) Because this function uses a binary search of the keyword table for
 *    speed, the keyword table MUST be kept in sorted order.
 * 2) Because the keyword strings in the keyword table are all uppercase,
 *    the string pointed to by 'str' must also be uppercase.
 *-----------------------------------------------------------------------------
 */

static struct kwentry keywordtab[] =
{
	"ADDRESS",				ADDRESS,
	"AMPERAGE",				AMPERAGE,
	"BEGINOVL",				BEGINOVL,
	"BINNUM",				BINNUM,
	"BOARD",					BOARD,
	"BUSMASTER",			BUSMASTER,
	"BYTE",					BYTE,
	"CACHE",					CACHE,
	"CATEGORY",				CATEGORY,
	"CHOICE",				CHOICE,
	"COMBINE",				COMBINE,
	"COMMENTS",				COMMENTS,
	"CONNECTION",			CONNECTION,
	"COUNT",					COUNT,
	"DECNUM",				DECNUM,
	"DECODE",				DECODE,
	"DEFAULT",				DEFAULT,
	"DIP",					DIP,
	"DISABLE",				DISABLE,
	"DMA",					DMA,
	"DWORD",					DWORD,
	"EDGE",					EDGE,
	"EISA",					EISA,
	"EISACMOS",				EISACMOS,
	"EMB",					EMB,
	"ENDGROUP",				ENDGROUP,
	"ENDOVL",				ENDOVL,
	"EXP",					EXP,
	"FACTORY",				FACTORY,
	"FIRST",					FIRST,
	"FREE",					FREE,
	"FREEFORM",				FREEFORM,
	"FUNCTION",				FUNCTION,
	"GROUP",					GROUP,
	"HELP",					HELP,
	"HEXNUM",				HEXNUM,
	"ID",						ID,
	"INCLUDE",				INCLUDE,
	"INIT",					INIT,
	"INITVAL",				INITVAL,
	"INLINE",				INLINE,
	"INVALID",				INVALID,
	"IOCHECK",				IOCHECK,
	"IOPORT",				IOPORT,
	"IRQ",					IRQ,
	"ISA16",					ISA16,
	"ISA32",					ISA32,
	"ISA8",					ISA8,
	"ISA8OR16",				ISA8OR16,
	"JTYPE",					JTYPE,
	"JUMPER",				JUMPER,
	"K",						KILOBYTES,
	"LABEL",					LABEL,
	"LENGTH",				LENGTH,
	"LEVEL",					LEVEL,
	"LINK",					LINK,
	"LOC",					LOC,
	"M",						MEGABYTES,
	"MASK_BITS",			MASK_BITS,
	"MEMORY",				MEMORY,
	"MEMTYPE",				MEMTYPE,
	"MFR",					MFR,
	"NAME",					NAME,
	"NO",						NO,
	"NONVOLATILE",			NONVOLATILE,
	"OTH",					OTH,
	"OTHER",					OTHER,
	"PAIRED",				PAIRED,
	"PORT",					PORT,
	"PORTADR",				PORTADR,
	"PORTVAR",				PORTVAR,
	"READID",				READID,
	"REVERSE",				REVERSE,
	"ROTARY",				ROTARY,
	"SHARE",					SHARE,
	"SHOW",					SHOW,
	"SIZE",					SIZE,
	"SIZING",				SIZING,
	"SKIRT",					SKIRT,
	"SLIDE",					SLIDE,
	"SLOT",					SLOT,
	"SOFTWARE",				SOFTWARE,
	"STEP",					STEP,
	"STRING",				STRING,
	"STYPE",					STYPE,
	"SUBCHOICE",			SUBCHOICE,
	"SUBFUNCTION",			SUBFUNCTION,
	"SUBTYPE",				SUBTYPE,
	"SUPPORTED",			SUPPORTED,
	"SWITCH",				SWITCH,
	"SYS",					SYS,
	"SYSTEM",				SYSTEM,
	"TIMING",				TIMING,
	"TOGGLES",				TOGGLES,
	"TOTALMEM",				TOTALMEM,
	"TRIGGER",				TRIGGER,
	"TRIPOLE",				TRIPOLE,
	"TYPE",					TYPE,
	"TYPEA",					TYPEA,
	"TYPEB",					TYPEB,
	"TYPEC",					TYPEC,
	"UNSUPPORTED",			UNSUPPORTED,
	"VALID",					VALID,
	"VERTICAL",				VERTICAL,
	"VIR",					VIR,
	"WORD",					WORD,
	"WRITABLE",				WRITABLE,
	"YES",					YES,
};

#define KEYWORDCNT	(sizeof keywordtab / sizeof(struct kwentry))

struct kwentry *kwlookup(str)
char *str;
{
    register int i;
    register int middle;
    int low = 0;
    int high = KEYWORDCNT - 1;

    while (low <= high)
    {
	middle = (low + high) / 2;

	i = strcmp(str, keywordtab[middle].name);

	if (i < 0)
	    high = middle - 1;
	else if (i > 0)
	    low = middle + 1;
	else
	    return(&keywordtab[middle]);
    }

    return(NULL);
}

/*-----------------------------------------------------------------------------
 * initlex - This function does any initialization necessary for the lexical
 * analyzer to process a new file.
 *-----------------------------------------------------------------------------
 */

void initlex()
{
    toggles = mask_bits = 0;
	 ch_invalid = 1;
}

/*-----------------------------------------------------------------------------
 * strtolong - Converts a string to a an unsigned long integer.  If an error
 * occurs, a value with all bits set to 1 will be returned.
 *-----------------------------------------------------------------------------
 */

static unsigned long strtolong(str, base)
char *str;
int base;
{
    unsigned long result;
    char *ptr;

    /* do the conversion */

    result = strtoul(str, &ptr, base);

    /* check for conversion errors and make sure that the
     * resulting value will not overflow the destination variable
     */

    if (errno == ERANGE)
	return(BAD_NUM);	/* some type of error */

    /* make sure that the entire string was a valid number */

    if (*ptr != '\0')
	return(BAD_NUM);	/* invalid string */

    return(result);
}
