# include "stdio.h"
#ifdef __cplusplus
   extern "C" {
     extern int yyreject();
     extern int yywrap();
     extern int yylook();
     extern void main();
     extern int yyback(int *, int);
     extern int yyinput();
     extern void yyoutput(int);
     extern void yyunput(int);
     extern int yylex();
   }
#endif	/* __cplusplus */
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern unsigned char yytext[];
int yymorfg;
extern unsigned char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = {stdin}, *yyout = {stdout};
extern int yylineno;
struct yysvf { 
	int yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
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

# define CONFIG 2
# define PP 4
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
{
			trace(TR_LEX, 0, "lex: %s COMMENT",
				parse_where());
		}
break;
case 2:
{
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
break;
case 3:
	{ ; }
break;
case 4:
{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s STRING: %s",
				parse_where(),
				yylval.ptr);
			return(STRING);
		}
break;
case 5:
{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s STRING: %s",
				parse_where(),
				yylval.ptr);
			return(STRING);
		}
break;
case 6:
{
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
break;
case 7:
{
			YY_NEWLINE;
			trace(TR_LEX, 0, "lex: %s EOS",
				parse_where());
			YBEGIN(CONFIG);
			return(EOS);
		}
break;
case 8:
{
			YY_NEWLINE;
		}
break;
case 9:
{
			trace(TR_LEX, 0, "lex: %s EOS",
			parse_where());
			return(EOS);
		}
break;
case 10:
{
			YBEGIN(PP);
			trace(TR_LEX, 0, "lex: %s '%s'",
				parse_where(),
				yytext);
			return(*yytext);
		}
break;
case 11:
{
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
break;
case 12:
{
			yylval.ptr = yytext;
			trace(TR_LEX, 0, "lex: %s HNAME: %s",
				parse_where(),
				yylval.ptr);
			return(HNAME);
		}
break;
case 13:
{
			trace(TR_LEX, 0, "lex: %s '%s'",
				parse_where(),
				yytext);
			return(*yytext);
		}
break;
case 14:
{
			yylval.num = atoi(yytext);
			trace(TR_LEX, 0, "lex: %s NUMBER: %d",
				parse_where(),
				yylval.num);
			return(NUMBER);
		}
break;
case 15:
	{
			trace(TR_ALL, LOG_ERR, "lex: %s unrecognized character: `%s'",
				parse_where(),
				yytext);
			/* Return invalid character so Yacc will generate and error message */
			return(*yytext);
		}
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */


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

int yyvstop[] = {
0,

15,
0,

3,
15,
0,

15,
0,

1,
15,
0,

15,
0,

15,
0,

8,
0,

13,
15,
0,

14,
15,
0,

9,
15,
0,

15,
0,

10,
15,
0,

7,
0,

15,
0,

3,
0,

5,
0,

1,
0,

2,
0,

4,
0,

14,
0,

12,
0,

11,
12,
0,

6,
0,

12,
0,

12,
0,
0};
# define YYTYPE unsigned char
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,7,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,8,	1,0,	
0,0,	24,0,	2,0,	8,21,	
0,0,	5,19,	0,0,	0,0,	
3,13,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	6,19,	
0,0,	0,0,	0,0,	1,9,	
1,10,	0,0,	8,21,	2,10,	
0,0,	1,7,	5,10,	11,25,	
0,0,	3,10,	4,13,	0,0,	
1,11,	1,7,	3,14,	2,11,	
31,33,	35,33,	5,11,	0,0,	
6,10,	3,11,	3,15,	0,0,	
0,0,	1,12,	0,0,	1,7,	
2,12,	0,0,	1,7,	5,12,	
6,11,	3,16,	3,12,	4,10,	
5,20,	4,18,	0,0,	3,17,	
4,14,	0,0,	0,0,	9,22,	
0,0,	6,12,	0,0,	4,11,	
4,15,	0,0,	6,20,	9,22,	
9,22,	10,24,	0,0,	12,26,	
0,0,	0,0,	0,0,	4,16,	
4,12,	10,24,	10,0,	12,26,	
12,26,	4,17,	15,28,	15,28,	
15,28,	15,28,	15,28,	15,28,	
15,28,	15,28,	15,28,	15,28,	
9,23,	0,0,	0,0,	0,0,	
0,0,	0,0,	9,22,	0,0,	
0,0,	0,0,	10,24,	0,0,	
12,26,	0,0,	9,22,	0,0,	
10,24,	0,0,	12,26,	0,0,	
0,0,	0,0,	0,0,	0,0,	
10,24,	0,0,	12,26,	0,0,	
9,22,	0,0,	0,0,	9,22,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	10,24,	0,0,	
12,27,	10,24,	17,29,	12,26,	
0,0,	17,30,	17,30,	17,30,	
17,30,	17,30,	17,30,	17,30,	
17,30,	17,30,	17,30,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
17,31,	17,31,	17,31,	17,31,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	20,32,	20,32,	
20,32,	20,32,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
29,30,	29,30,	29,30,	29,30,	
30,33,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	30,30,	30,30,	30,30,	
30,30,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	33,34,	
33,34,	33,34,	33,34,	34,34,	
0,0,	0,0,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	34,35,	34,35,	34,35,	
34,35,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
-1,	0,		0,	
-4,	yysvec+1,	0,	
-10,	yysvec+1,	0,	
-36,	yysvec+1,	0,	
-7,	yysvec+1,	0,	
-21,	yysvec+1,	0,	
0,	0,		yyvstop+1,
6,	0,		yyvstop+3,
-78,	0,		yyvstop+6,
-88,	0,		yyvstop+8,
1,	0,		yyvstop+11,
-90,	0,		yyvstop+13,
0,	0,		yyvstop+15,
0,	0,		yyvstop+17,
54,	0,		yyvstop+20,
0,	0,		yyvstop+23,
109,	0,		yyvstop+26,
0,	0,		yyvstop+28,
0,	0,		yyvstop+31,
167,	0,		yyvstop+33,
0,	yysvec+8,	yyvstop+35,
0,	yysvec+9,	0,	
0,	0,		yyvstop+37,
-3,	yysvec+10,	yyvstop+39,
0,	0,		yyvstop+41,
0,	yysvec+12,	0,	
0,	0,		yyvstop+43,
0,	yysvec+15,	yyvstop+45,
225,	yysvec+17,	0,	
302,	yysvec+17,	yyvstop+47,
6,	yysvec+17,	yyvstop+49,
0,	yysvec+20,	yyvstop+52,
360,	0,		yyvstop+54,
438,	0,		0,	
7,	yysvec+34,	yyvstop+56,
0,	0,	0};
struct yywork *yytop = yycrank+560;
struct yysvf *yybgin = yysvec+1;
unsigned char yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,'"' ,01  ,01  ,01  ,01  ,01  ,
'(' ,'(' ,01  ,'(' ,01  ,'(' ,'(' ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,'(' ,01  ,01  ,01  ,'>' ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'(' ,01  ,'(' ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
unsigned char yyextra[] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/* @(#) $Revision: 66.2 $      */
int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
/* char yytext[YYLMAX];
 * ***** nls8 ***** */
unsigned char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
/* char yysbuf[YYLMAX];
 * char *yysptr = yysbuf;
 * ***** nls8 ***** */
unsigned char yysbuf[YYLMAX];
unsigned char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych, yyfirst;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
/*	char *yylastch;
 * ***** nls8 ***** */
	unsigned char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = &yycrank[yystate->yystoff];
			if(yyt == yycrank && !yyfirst){  /* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == 0)break;
				}
			*yylastch++ = yych = input();
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt = &yycrank[yystate->yystoff]) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}

# ifdef __cplusplus
yyback(int *p, int m)
# else
yyback(p, m)
	int *p;
# endif
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	
	}

#ifdef __cplusplus
void yyoutput(int c)
#else
yyoutput(c)
  int c;
# endif
{
	output(c);
}

#ifdef __cplusplus
void yyunput(int c)
#else
yyunput(c)
   int c;
#endif
{
	unput(c);
}
