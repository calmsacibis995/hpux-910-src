/* @(#) $Revision: 66.2 $ */     
/* Copyright (c) 1981 Regents of the University of California */
/*
 * Regular expression definitions.
 * The regular expressions in ex are similar to those in ed,
 * with the addition of the word boundaries from Toronto ed
 * and allowing character classes to have [a-b] as in the shell.
 * The numbers for the nodes below are spaced further apart then
 * necessary because I at one time partially put in + and | (one or
 * more and alternation.)
 */
struct	regexp {

	unsigned char	Expbuf[ESIZE + 2];
	short		Nbra;
	bool		Circf;
};

/*
 * There are three regular expressions here, the previous (in re),
 * the previous substitute (in subre) and the previous scanning (in scanre).
 * It would be possible to get rid of "re" by making it a stack parameter
 * to the appropriate routines.
 */
var struct	regexp re;		/* Last re */
var struct	regexp scanre;		/* Last scanning re */
var struct	regexp subre;		/* Last substitute re */

/*
 * Defining circf and expbuf like this saves us from having to change
 * old code in the ex_re.c stuff.
 */
#define	expbuf	re.Expbuf
#define	nbra	re.Nbra
#define	circf	re.Circf

/*
 * Since the phototypesetter v7-epsilon
 * C compiler doesn't have structure assignment...
 */
#define	savere(a)	copy(&a, &re, sizeof (struct regexp))
#define	resre(a)	copy(&re, &a, sizeof (struct regexp))

/* Constants:	*/

#define	REG_NPAREN	9			/* number of substrings reported in sp[]/ep[] arrays	*/
						/* Note: the upper limit for this number is 255		*/
#define RE_BUF_SIZE	1024			/* max size of incoming RE				*/


/*
 * Definitions for substitute
 */

typedef struct {
	CHAR	*lparens[REG_NPAREN];
	CHAR	*rparens[REG_NPAREN];
} lr_parens;

var lr_parens	best_parens;

#define	braslist	best_parens.lparens	/* Starts of \(\)'ed text in lhs */
#define	braelist	best_parens.rparens	/* Ends... */

#ifndef NONLS8	/* 8bit integrity */
var short	rhsbuf[RHSSIZE];	/* Rhs of last substitute */
#else NONLS8
var char	rhsbuf[RHSSIZE];	/* Rhs of last substitute */
#endif NONLS8


#ifdef	NLS16

#define	_ISANK(c)	((unsigned) (c) <= 0xFF)

/* *p++ */
#undef _CHARADV(p)
#define	_CHARADV(p)	(IS_FIRST(*(p)) ? ((p)+=2, (((p)[-2] & TRIM) << 8) | ((p)[-1] & TRIM)) : (*(p)++ & TRIM))
#endif

/* Error Codes:	*/

#define	REG_NOMATCH	20		/* regexec() failed to match					*/
#define	REG_ECOLLATE	21		/* invalid collation element referenced				*/
#define	REG_EESCAPE	22		/* trailing \ in pattern					*/
#define	REG_ENEWLINE	36		/* \n found before end of pattern and REG_NEWLINE flag not set	*/
#define	REG_ENSUB	43		/* more than nine \( \) pairs or nesting level too deep		*/
#define	REG_ESUBREG	25		/* number in \digit invalid or in error				*/
#define	REG_EBRACK	49		/* [ ] imbalance						*/
#define	REG_EPAREN	42		/* \( \) imbalance or ( ) imbalance				*/
#define	REG_EBRACE	45		/* \{ \} imbalance						*/
#define	REG_ERANGE	23		/* invalid endpoint in range statement				*/
#define	REG_ESPACE	50		/* out of memory for compiled pattern				*/

#define	REG_EABRACE	16		/* number too large in \{ \} construct				*/
#define REG_EBBRACE	11		/* invalid number in \{ \} construct				*/
#define REG_ECBRACE	44		/* more than 2 numbers in \{ \} construct			*/
#define REG_EDBRACE	46		/* first number exceeds second in \{ \} construct		*/
#define	REG_ECTYPE	24		/* invalid character class type named				*/
