%{
/*
 *  $Header: lexer.l,v 1.1.109.6 92/03/04 08:21:42 ash Exp $/
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


/* On some systems a system include file redefines ECHO */
#ifdef	ECHO
#undef	ECHO
#endif	/* ECHO */

#include "include.h"
#include "parse.h"
#include "parser.h"

#ifdef	vax11c
extern YYSTYPE yylval;
#endif	/* vax11c */

#ifdef	ECHO
#undef	ECHO
#endif	/* ECHO */

#define	printf(fmt, arg)	trace(TR_LEX, 0, fmt, arg)
#undef	yywrap
#define	yywrap	parse_eof
extern int parse_eof();

#define	YBEGIN(state)	trace(TR_LEX, 0, "yylex: %s State %d", parse_where(), state); BEGIN state

static fi_info parse_fi[FI_MAX+1];			/* Table of input files */
static int	fi_file = 0;			/* Index into file table */

#ifdef	FLEX_SCANNER

int	yylineno;

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size)	result = fgets(buf, max_size, yyin) ? strlen(buf) : YY_NULL;

#define	YY_NEWLINE	yylineno++

#ifdef	YY_USER_ACTION
#define	parse_restart(fp)	yyrestart(fp)
#endif	/* YY_USER_ACTION */

#else	/* FLEX_SCANNER */

#define	YY_NEWLINE

#endif	/* FLEX_SCANNER */

#ifndef	parse_restart
#define	parse_restart(fp)	yyin = fp
#endif	/* parse_restart */

%}

%s CONFIG PP

nl				\n
ws				[ \t]+
digit				[0-9]
letter				[A-Za-z]
dot				\.
hyphen				\-
special				[{}()\.\-+:]
percent				\%
semi				\;

comment				\#.*
ccomment			"/*"
bstring				\<[^>]*\>
qstring				\"[^"]*\"
number				{digit}+
keyword				{letter}{letter}+
name				{letter}({letter}|{digit}|{hyphen})*({letter}|{digit})
hname				{name}({dot}{name})*{dot}?

%%
{comment}	{
			trace(TR_LEX, 0, "lex: %s COMMENT",
				parse_where());
		}
{ccomment}	{
			char ttchar;
			char search = '*';

			while (ttchar = input()) {
				if (ttchar == search) {
					if (search == '/') {
						break;
					} else {
						search = '/';
					}
				}
			}
			trace(TR_LEX, 0, "lex: %s COMMENT",
				parse_where());
		}
{ws}		{ ; }
{bstring}	{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s STRING: %s",
				parse_where(),
				yylval.ptr);
			return(STRING);
		}
{qstring}	{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s STRING: %s",
				parse_where(),
				yylval.ptr);
			return(STRING);
		}

<PP>{keyword}	{
			int token;

			token = parse_keyword(yytext);
			if (token == UNKNOWN) {
				trace(TR_LEX, 0, "lex: %s unknown keyword: %s",
					parse_where(),
					yytext);
				yylval.ptr = yytext;
			} else {
				trace(TR_LEX, 0, "lex: %s KEYWORD: %s",
					parse_where(),
					yytext);
			}
			return(token);
		}
<PP>{nl}	{
			YY_NEWLINE;
			trace(TR_LEX, 0, "lex: %s EOS",
				parse_where());
			YBEGIN(CONFIG);
			return(EOS);
		}

<CONFIG>{nl}	{
			YY_NEWLINE;
		}
<CONFIG>{semi}	{
			trace(TR_LEX, 0, "lex: %s EOS",
			parse_where());
			return(EOS);
		}
<CONFIG>^{percent}	{
			YBEGIN(PP);
			trace(TR_LEX, 0, "lex: %s '%s'",
				parse_where(),
				yytext);
			return(*yytext);
		}
<CONFIG>{keyword}	{
			int token;

			token = parse_keyword(yytext);
			if (token != UNKNOWN) {
				trace(TR_LEX, 0, "lex: %s KEYWORD: %s",
					parse_where(),
					yytext);
			} else {
				yylval.ptr = yytext;
				token = HNAME;
				trace(TR_LEX, 0, "lex: %s HNAME: %s",
					parse_where(),
					yylval.ptr);
			}
			return(token);
		}
<CONFIG>{hname}	{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s HNAME: %s",
				parse_where(),
				yylval.ptr);
			return(HNAME);
		}
<CONFIG>{special}	{
			trace(TR_LEX, 0, "lex: %s '%s'",
				parse_where(),
				yytext);
			return(*yytext);
		}
<CONFIG>{number}	{
			yylval.num = atoi(yytext);
			trace(TR_LEX, 0, "lex: %s NUMBER: %d",
				parse_where(),
				yylval.num);
			return(NUMBER);
		}
.		{
			trace(TR_ALL, LOG_ERR, "lex: %s unrecognized character: `%s'",
				parse_where(),
				yytext);
			/* Return invalid character so Yacc will generate and error message */
			return(*yytext);
		}
%%


/*
 *	Process an include directive, read another input file
 */
int
parse_include(name)
char *name;
{
	fi_info *fip;
	FILE *file;

	/* Verify file name is unique (this is easy to fool) */
	for (fip = parse_fi; fip <= &parse_fi[fi_file]; fip++) {
		if (fip->fi_name && !strcmp(name, fip->fi_name)) {
			(void) sprintf(parse_error, "recursive includes: %s",
				name);
			return(1);
		}
	}

	/* Try to open new file */
	if (fi_file >= FI_MAX) {
		(void) sprintf(parse_error, "too many levels of includes");
		fi_file--;
		return(1);
	}

	file = fopen(name, "r");
	if (!file) {
		(void) sprintf(parse_error, "error opening %s: %m",
			name);
		return(1);
	}

	/* Update line number of current file and make sure we have correct file pointer */
	fip = &parse_fi[fi_file++];
	fip->fi_lineno = yylineno;
	fip->fi_FILE = yyin;

	fip++;
	fip->fi_name = name;
	fip->fi_FILE = file;
	yylineno = 1;
	yyin = fip->fi_FILE;
	parse_filename = fip->fi_name;
	trace(TR_LEX, 0, "parse_include: %s now reading %s (%d)",
	      parse_where(),
	      fip->fi_name,
	      fileno(fip->fi_FILE));
	return(0);
}


/*
 *	Open the first file (called from parse_parse).  If no file specified,
 *	assume stdin (for testing ).
 */
int
parse_open(name)
char *name;
{
	fi_info *fip;

	fip = &parse_fi[fi_file = 0];

	if (name) {
	    fip->fi_FILE = fopen(name, "r");
	    if (!fip->fi_FILE) {
		trace(TR_ALL, LOG_ERR, "parse_open: error opening %s: %m",
		      name);
		return(1);
	    }
	} else {
#if	YYDEBUG != 0
	    name = "stdin";
#else	/* YYDEBUG */
	    trace(TR_ALL, LOG_ERR, "parse_open: no file specified");
	    return(1);
#endif	/* YYDEBUG */
	}

        yylineno = 1;
	parse_restart(fip->fi_FILE);
	parse_filename = fip->fi_name = name;
	trace(TR_LEX, 0, "parse_open: reading %s (%d)",
	      fip->fi_name,
	      fileno(fip->fi_FILE));
	
	YBEGIN(CONFIG);
	return(0);
}


/* parse_eof - process end of file on current input file */
int
parse_eof()
{
	fi_info *fip;

	fip = &parse_fi[fi_file];
	trace(TR_LEX, 0, "parse_eof: %s EOF on %s (%d)",
	      parse_where(),
	      fip->fi_name,
	      fileno(fip->fi_FILE));
	if (fclose(fip->fi_FILE)) {
	    trace(TR_ALL, LOG_ERR, "parse_eof: error closing %s: %m",
		  fip->fi_name);
	}
	fip->fi_FILE = NULL;
	(void) free(fip->fi_name);
	fip->fi_name = NULL;

	if (--fi_file >= 0) {
		fip = &parse_fi[fi_file];
		parse_filename = fip->fi_name;
		yylineno = fip->fi_lineno;
		yyin = fip->fi_FILE;
		trace(TR_LEX, 0, "parse_eof: %s resuming %s",
			parse_where(),
			fip->fi_name);
		return(0);
	}
	yyin = (FILE *) 0;
	trace(TR_LEX, 0, "parse_eof: %s EOF",
		parse_where());
	return(1);
}

