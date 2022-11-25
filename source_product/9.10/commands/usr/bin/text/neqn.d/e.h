/* @(#) $Revision: 26.2 $ */       
#include <stdio.h>
#ifdef NLS16
#include <langinfo.h>
#include <nl_ctype.h>
/* Legal second byte range of HP15. */
#define SECONDof2(c) !(iscntrl(c)||isspace(c))
#define HP15_1st  0400	/* First byte flag */
#define HP15_2nd 01000	/* Second byte flag */
#endif

#define	FATAL	1
#define	ROM	'1'
#define	ITAL	'1'
#define	BLD	'1'

#define	VERT(n)	(20 * (n))
#define	EFFPS(p)	((p) >= 6 ? (p) : 6)

extern int	dbg;
extern int	ct;
extern int	lp[];
extern int	used[];	/* available registers */
extern int	ps;	/* dflt init pt size */
extern int	deltaps;	/* default change in ps */
extern int	gsize;	/* global size */
extern int	gfont;	/* global font */
extern int	ft;	/* dflt font */
extern FILE	*curfile;	/* current input file */
extern int	ifile;	/* input file number */
extern int	linect;	/* line number in current file */
extern int	eqline;	/* line where eqn started */
extern int	svargc;
extern char	**svargv;
extern int	eht[];
extern int	ebase[];
extern int	lfont[];
extern int	rfont[];
extern int	yyval;
extern int	*yypv;
extern int	yylval;
extern int	eqnreg, eqnht;
extern int	lefteq, righteq;
extern int	lastchar;	/* last character read by lex */
extern int	markline;	/* 1 if this EQ/EN contains mark or lineup */

typedef struct s_tbl {
	char	*name;
	char	*defn;
	struct s_tbl *next;
} tbl;
