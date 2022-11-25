/* @(#) $Revision: 66.3 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines tokenize command lines.
 */

#include <ctype.h>
#include "cdb.h"
#include "Kdb.h"

#define chTab		'\t'		/* aliases for certain chars */
#define chSpace		' '
#define chUnder		'_'
#define chDollar	 '$'
#define chBackSlash	'\\'

/***********************************************************************
 * CURRENT AND PEEKAHEAD TOKENS:
 */

export	int	vcbTok;			/* size of current token   */
export	int	vcbPeek;		/* size of peekahead token */
export	TKE	vtk;			/* type of current token   */
export	TKE	vtkPeek;		/* type of peekahead token */
export	char	vsbTok  [cbTokMax];	/* current token	   */
export	char	vsbPeek [cbTokMax];	/* peekahead token	   */
export  ushort  fWhiteSpace;


/***********************************************************************
 * PREDEFINED KEYWORDS:
 */

typedef struct {			/* the struct	*/
	    char	*sbKey;		/* the keyword	*/
	    short	tk;		/* token type	*/
	} KWR, *pKWR;

#define cbKWR	(sizeof (KWR))
#define kwNil	((pKWR) 0)


#ifdef KASSERT
KWR vrgKwCdb[] = {			/* debugger keywords */
	    "$in",	tkInside
	};
#endif

#define ikwCdbMax (sizeof (vrgKwCdb) / cbKWR)


KWR vrgKwC[] = {			/* C keywords */
	    "sizeof",	tkSizeof
	};

#define ikwCMax (sizeof (vrgKwC) / cbKWR)


/***********************************************************************
 * T K	 F   K W
 *
 * Search a keyword list for a keyword and return the token type if
 * found, else don't do anything.
 */

export void TkFKw (ptk, sbKey, rgKw, ikwMax)
    TKE		*ptk;		/* result token, if any */
    char	*sbKey;		/* possible keyword	*/
    pKWR	rgKw;		/* list to search	*/
    int		ikwMax;		/* number in list	*/
{
    register int	ikw;	/* current keyword	*/

    for (ikw = 0; ikw < ikwMax; ikw++)
    {
	if (FSbCmp (sbKey, rgKw[ikw].sbKey))
	{
	    *ptk = rgKw[ikw].tk;
	    return;
	}
    }
} /* TkFKw */


/***********************************************************************
 * C H	 F   E S C A P E
 *
 * Given a string, return the next char, taking account of backslash
 * notation for special chars.  Note that '\\' and '\'' don't need
 * handling as special cases.  Advances the string ptr as needed, but
 * not past a true chNull!
 */

export char ChFEscape (psbIn)
    char	**psbIn;	/* string to parse */
{
    register char	ch  = **psbIn;	/* current char	    */
    register int	val = 0;	/* for octal format */
    register int	i   = 0;	/* which digit	    */

    if (ch == chNull)				/* true null (not "\0") */
	return (ch);				/* don't advance psbIn  */

    (*psbIn)++;					/* at following char */

    if (ch != chBackSlash)			/* not special case */
	return (ch);				/* just return char */

    if ((ch = **psbIn) != chNull)		/* char after '\'    */
	(*psbIn)++;				/* at following char */

    switch (ch)
    {
	default:	return (ch);		/* just ignore the backslash */

	case 'b':	return ('\b');
	case 'f':	return ('\f');
	case 'n':	return ('\n');
	case 'r':	return ('\r');
	case 't':	return ('\t');

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':		/* arbitrary 1-3 octal digit form */
		while (true)	/* until break */
		{
		    val = (val * 8) + (ch - '0');
		    ch  = **psbIn;		/* next char */

		    if ((++i > 2)		/* ate three already */
		    OR  (ch < '0') OR (ch > '7'))
			break;

		    (*psbIn)++;			/* at following char */
		}
		return (val);

    } /* switch */

} /* ChFEscape */


/***********************************************************************
 * T K	 N U M   F   S T R
 *
 * Given a string representing a hex, decimal, octal, or floating point
 * number, advance the string pointer past the number, and return the
 * token type (tkHex, tkDecimal, tkOctal, or tkFloat).  Assumes no
 * leading white space before the number, but leaves the string pointer
 * immediately following the token, even if that's white space.  Some
 * callers depend on this!
 *
 * Errors out if the number is invalid in format.
 *
 * Floating constants must be:  <digits>.<digits>[e|E|d|D[+|-]<digits>]
 * with no blanks in it.  This restrictive format is necessary to avoid
 * confusion with some command syntaxes.
 *
 * If there is a blank before "e|E|d|D" or it is not followed by a validi
 * exponent, no exponent is taken, but no error is given, as it might be
 * a command or something else.  If an exponent is taken, it's converted
 * to "e" for atof() compatibility (all floating constants are doubles).
 */

local TKE TkNumFStr (psbCmd)
    char	**psbCmd;			/* line to tokenize */
{
    register char	*sbCmd	  = *psbCmd;	/* quick copy	    */
    FLAGT	fLeadZero = (*sbCmd == '0');	/* leading zero?    */
    FLAGT	fFloat = 0;			/* floating num?    */

/*
 * Physical HEX NUMBER; pass, check, and return:
 */

    if (fLeadZero AND ((sbCmd[1] == 'p') OR (sbCmd[1] == 'P')))
    {
	unsigned int addr;
	extern unsigned int loadpoint;
	char buf[512];
	char *s = &buf[0];
	char *sbsave;

	sbsave = sbCmd;
	sbCmd[1] = 'x';
	addr = (unsigned int)strtoul(sbCmd, 0, 16);
	addr -= loadpoint;

	sbCmd += 2;					/* skip "0x|0X" */

	while (isxdigit (*sbCmd))			/* skip digits */
	    sbCmd++;

	if (sbCmd == (*psbCmd + 2))			/* no digits */
	    UError ("Misformed hex number");

	sprintf(s,"0x%x %s", addr, sbCmd);
	sbCmd = sbsave;
	strcpy(sbCmd, s);

	sbCmd += 2;					/* skip "0x|0X" */

	while (isxdigit (*sbCmd))			/* skip digits */
	    sbCmd++;

	if (sbCmd == (*psbCmd + 2))			/* no digits */
	    UError ("Misformed hex number");

	*psbCmd = sbCmd;				/* update pointer */
	return (tkHex);
    }

/*
 * HEX NUMBER; pass, check, and return:
 */

    if (fLeadZero AND ((sbCmd[1] == 'x') OR (sbCmd[1] == 'X')))
    {
	sbCmd += 2;					/* skip "0x|0X" */

	while (isxdigit (*sbCmd))			/* skip digits */
	    sbCmd++;

	if (sbCmd == (*psbCmd + 2))			/* no digits */
	    UError ("Misformed hex number");

	*psbCmd = sbCmd;				/* update pointer */
	return (tkHex);
    }

/*
 * NON-HEX; PASS INTEGER PART:
 *
 * Note that '8' and '9' are skipped even for octal numbers (checked later).
 */

    while (isdigit (*sbCmd))
	sbCmd++;

#ifdef KFLPT
/*
 * FLOATING NUMBER; PASS FRACTIONAL PART AND EXPONENT:
 */

    if (fFloat = ((sbCmd[0] == '.') AND (isdigit (sbCmd[1]))))
    {
	do sbCmd++;
	while (isdigit (*sbCmd));

	if ((*sbCmd != chNull)			/* not end of line  */
	AND strchr ("eEdD", *sbCmd))		/* no leading blank */
	{
	    int cSkip = 1 + ((sbCmd[1] == '+') OR (sbCmd[1] == '-'));

	    if (isdigit (sbCmd [cSkip]))	/* followed by digit?	*/
	    {					/* we have an exponent	*/
		*sbCmd = 'e';			/* change for atof()	*/
		sbCmd += cSkip;			/* skip to exponent	*/

		while (isdigit (*sbCmd))	/* skip exponent */
		    sbCmd++;
	    }
	}
    }
#endif

/*
 * OCTAL NUMBER; CHECK DIGITS:
 */

    if (fLeadZero AND (! fFloat))
    {
	char	*sb;

	for (sb = *psbCmd; sb < sbCmd; sb++)
	    if ((*sb == '8') OR (*sb == '9'))
		UError ("Misformed octal number");
    }

/*
 * RETURN VALUES:
 */

    *psbCmd = sbCmd;
    return (fFloat ? tkFloat : (fLeadZero ? tkOctal : tkDecimal));

} /* TkNumFStr */


/***********************************************************************
 * T K	 F   S T R
 *
 * Given a string, copy the first token (if any) to a safe place,
 * advance the string, and return the token type.  Skips leading
 * white space but leaves the string pointer immediately following
 * the token, even if that's white space.  Some callers depend on this!
 */

export TKE TkFStr (psbCmd, sbTok, pcb)
    char	**psbCmd;		/* line to tokenize */
    char	*sbTok;			/* where save token */
    int		*pcb;			/* bytes in token   */
{
    char	*sbCmd = *psbCmd;	/* quick copy	  */
    char	*sbStart;		/* start of token */
    TKE	tk = tkNil;			/* result type	  */
    register char	ch;			/* current char	  */
    register char	chNext;			/* next	   char	  */
    int		cb;			/* size of string */

/*
 * EAT LEADING WHITE SPACE:
 */

    fWhiteSpace = 0;
    if (sbCmd != sbNil)
	while ((*sbCmd == chTab) OR (*sbCmd == chSpace))
	{   sbCmd++;
	    fWhiteSpace = 1;
	}

/*
 * NO TOKEN:
 */

    if ((sbCmd == sbNil) OR (*sbCmd == chNull))	/* no line or end of line */
    {
	*psbCmd	 = sbNil;			/* no more line	*/
	*pcb	 = 0;				/* no size	*/
	sbTok[0] = chNull;			/* no token	*/
	return (tkNil);
    }

/*
 * FIGURE OUT TYPE OF TOKEN:
 *
 * All cases but a few set tk and leave sbCmd advanced past the token
 * as an exit condition.
 */

    sbStart = sbCmd;			/* remember start */
    ch	    = *sbCmd;

/*
 * STRING TOKEN (proc, var, or command):
 */

    if (isalpha (ch) OR (ch == chUnder) OR (ch == chDollar))
    {
	tk = tkStr;

	do ch = *(++sbCmd);
	while (isalnum (ch) OR (ch == chUnder));
    }

/*
 * NUMBER TOKEN:
 */

    else if (isdigit (ch))
	tk = TkNumFStr (& sbCmd);

/*
 * VARIOUS CHARS:
 */

    else if (ispunct (ch))
    {
	chNext = *(++sbCmd);

	switch (ch)
	{
	    case '[':	tk = tkLSB;		break;
	    case ']':	tk = tkRSB;		break;
	    case '(':	tk = tkLP;		break;
	    case ')':	tk = tkRP;		break;
	    case '{':	tk = tkLCB;		break;
	    case '}':	tk = tkRCB;		break;
	    case '?':	tk = tkQuest;		break;
	    case '@':
			tk = tkStar;
			vimap = 1;
			break;
	    case '$':	tk = tkDollar;		break;
	    case '#':	tk = tkHash;		break;
	    case ';':	tk = tkSemi;		break;
	    case ',':	tk = tkComma;		break;
	    case '_':	tk = tkUnderScore;	break;
	    case '\\':	tk = tkBackSlash;	break;
	    case '~':	tk = tkTilda;		break;

	    case '\'':	tk = tkCharConstant;
			sbTok[0] = ChFEscape (& sbCmd);

			if (*sbCmd != '\'')
			    UError ("Character constant is missing ending '");

			sbCmd++;			/* skip ' */
			*pcb = 1;
			goto special;

	    case '"':	tk = tkStrConstant;
			cb = 0;

			while (((ch = *sbCmd) != chNull) /* not end of line   */
			AND    (ch != '"'))		 /* not closing quote */
			{
			    ch = ChFEscape (& sbCmd);	/* go through sbCmd */
			    /* note that '\"' is handled right, as simple " */

			    if (cb < cbTokMax - 1)	/* don't exceed size */
				sbTok [cb++] = ch;	/* (stop at max - 1) */
			}
			if (ch != '"')
			    UError ("String constant is missing ending \"");

			sbCmd++;			/* skip " */
			*pcb = cb;			/* always < cbTokMax */
			goto special;

	    case '.':	tk = tkDot;
			break;

	    case ':':	tk = tkColon;
			if (chNext == '=')		/* := */
			{
			    tk = tkAssign;
			    sbCmd++;
			}
			break;

	    case '+':	tk = tkPlus;
			if (chNext == '+')		/* ++ */
			{
			    tk = tkPlusPlus;
			    sbCmd++;
			}
			if (chNext == '=')		/* += */
			{
			    tk = tkAssPlus;
			    sbCmd++;
			}
			break;

	    case '-':	tk = tkMinus;
			if (chNext == '-')		/* -- */
			{
			    tk = tkMinusMinus;
			    sbCmd++;
			}
			if (chNext == '>')		/* -> */
			{
			    tk = tkPtr;
			    sbCmd++;
			}
			else if (chNext == '=')		/* -= */
			{
			    tk = tkAssMinus;
			    sbCmd++;
			}
			break;

	    case '*':	tk = tkStar;
			if (chNext == '=')		/* *= */
			{
			    tk = tkAssMult;
			    sbCmd++;
			}
			break;

	    case '/':	tk = tkSlash;
			if (chNext == '/')		/* // */
			{
			    tk = tkDiv;
			    sbCmd++;

			    if (*sbCmd == '=')		/* //= */
			    {
				tk = tkAssDiv;
				sbCmd++;
			    }
			}
			break;

	    case '%':	tk = tkModulo;
			if (chNext == '=')		/* %= */
			{
			    tk = tkAssMod;
			    sbCmd++;
			}
			break;

	    case '&':	tk = tkBitAnd;
			if (chNext == '&')		/* && */
			{
			    tk = tkLAND;
			    sbCmd++;
			}
			else if (chNext == '=')		/* &= */
			{
			    tk = tkAssBAND;
			    sbCmd++;
			}
			break;

	    case '|':	tk = tkBitOr;
			if (chNext == '|')		/* || */
			{
			    tk = tkLOR;
			    sbCmd++;
			}
			else if (chNext == '=')		/* |= */
			{
			    tk = tkAssBOR;
			    sbCmd++;
			}
			break;

	    case '^':	tk = tkXOR;
			if (chNext == '=')		/* ^= */
			{
			    tk = tkAssXOR;
			    sbCmd++;
			}
			break;

	    case '!':	tk = tkBang;
			if (chNext == '=')		/* != */
			{
			    tk = tkNotEqual;
			    sbCmd++;
			}
			break;

	    case '=':
			    tk = tkAssign;
			    if (chNext == '=')		/* == */
			    {
				tk = tkEqual;
				sbCmd++;
			    }
			break;

	    case '<':	tk = tkLT;
			if (chNext == '=')		/* <= */
			{
			    tk = tkLE;
			    sbCmd++;
			}
			else if (chNext == '<')		/* << */
			{
			    tk = tkLShift;
			    sbCmd++;

			    if (*sbCmd == '=')		/* <<= */
			    {
				tk = tkAssLeft;
				sbCmd++;
			    }
			}
			break;

	    case '>':	tk = tkGT;
			if (chNext == '=')		/* >= */
			{
			    tk = tkGE;
			    sbCmd++;
			}
			else if (chNext == '>')		/* >> */
			{
			    tk = tkRShift;
			    sbCmd++;

			    if (*sbCmd == '=')		/* >>= */
			    {
				tk = tkAssRight;
				sbCmd++;
			    }
			}
			break;

	} /* switch */
    }
    else
    {						/* it's something else */
	tk = tkOther;
	sbCmd++;
    }

/*
 * SET SIZE AND SAVE TOKEN:
 */

    *pcb = Min (sbCmd - sbStart, cbTokMax - 1);	/* don't exceed size */
    strncpy (sbTok, sbStart, *pcb);

special:	/* char and string constants set pcb and sbTok themselves */

    sbTok [*pcb] = chNull;			/* add terminator	*/
    *psbCmd	 = sbCmd;			/* advance command ptr	*/

/*
 * CHECK FOR DEBUGGER OR LANGUAGE KEYWORD (latter take precedence):
 */

    if (tk == tkStr)
    {
#ifdef KASSERT
	TkFKw (& tk, sbTok, vrgKwCdb, ikwCdbMax);
#endif
	TkFKw (&tk, sbTok, vrgKwC,   ikwCMax  );
    }
    return (tk);

} /* TkFStr */


/***********************************************************************
 * T K	 P E E K
 *
 * Peek at the next token in the global command line.
 * Results go into the special Peek variables and vsbCmd is not altered.
 */

export TKE TkPeek()
{
    char	*sbCmd = vsbCmd;	/* temporary copy */

    vtkPeek = TkFStr (&sbCmd, vsbPeek, &vcbPeek);
    return (vtkPeek);

} /* TkPeek */


/***********************************************************************
 * T K	 N E X T
 *
 * Eat the next token in the current command line.
 */

export TKE TkNext()
{
    return (vtk = TkFStr (&vsbCmd, vsbTok, &vcbTok));

} /* TkNext */
