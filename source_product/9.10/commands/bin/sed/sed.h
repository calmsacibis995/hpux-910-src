/* @(#) $Revision: 70.2 $ */       
/*
 * sed -- stream  editor
 *
 * Copyright 1975 Bell Telephone Laboratories, Incorporated
 *
 */

/*
 * define some macros for rexexp.h
 */

#include <limits.h>     /* It's needed for LINE_MAX size */

#define INIT	extern char *cp, *badp;	/* cp points to RE string */\
		register char *sp = cp;
#define GETC()		(*sp++)
#define PEEKC()		(*sp)
#define UNGETC(c)	(--sp)
#define RETURN(c)	cp = sp; return(c);
#define ERROR(c)	{ cp = sp; return(badp); }

#define CEND	16
#define CLNUM	14

#define NLINES  256
#define DEPTH   20
#define PTRSIZE 101
#define RESIZE  14600	/* FSDlj07363 					 */
			/* increased in 8.0 from 5000;  compiled regular */
			/* expressions from regcomp now take up ~32 more */
			/* bytes per regular expression.  There can be 3 */
			/* regular expressions per command, with a limit */
			/* of 100 commands, so (3 * 32 * 100) = 9600,    */
			/* 9600 + 5000 = 14600.  Changing to dynamic  	 */
			/* memory allocation would be the *ideal* way to */
			/* solve this problem.				 */

#define ABUFSIZE        20
#define LBSIZE  8192    /* Changed from 4000 --> 8192 which is the min. */
			/* required for POSIX.2.  Changing to dynamic   */
			/* allocation would be ideal.                   */
/*  
 *  POSIX.2: sed shall process lines of LINE_MAX length
 */
#ifndef LINE_MAX
#define LINE_MAX 2048
#endif
#define GOCCUR  LINE_MAX+1	/* g flag exists, eg. s/../.../g */
#define ESIZE   256
#define LABSIZE 50

extern union reptr     *abuf[];
extern union reptr **aptr;
extern char    genbuf[];
extern char    *lbend;
extern char    *lcomend;
extern long    lnum;
extern char    linebuf[];
extern char    holdsp[];
extern char    *spend;
extern char    *hspend;
extern int     nflag;
extern long    tlno[];

#define ACOM    01
#define BCOM    020
#define CCOM    02
#define CDCOM   025
#define CNCOM   022
#define COCOM   017
#define CPCOM   023
#define DCOM    03
#define ECOM    015
#define EQCOM   013
#define FCOM    016
#define GCOM    027
#define CGCOM   030
#define HCOM    031
#define CHCOM   032
#define ICOM    04
#define LCOM    05
#define NCOM    012
#define PCOM    010
#define QCOM    011
#define RCOM    06
#define SCOM    07
#define TCOM    021
#define WCOM    014
#define CWCOM   024
#define YCOM    026
#define XCOM    033

/**********
* example command:  sed -e '/ABC/,3s/ABC/XYZ/' file
*                             ^   ^^  ^   ^
*			      a   bc  d   e
* where a = ad1
*	b = ad2
*	c = command
*	d = re1
*	e = rhs
*
**********/

union   reptr {
        struct reptr1 {
                char    *ad1;		/* address 1 */
                char    *ad2;		/* address 2 */
                char    *re1;		/* regular expression1 */
                char    *rhs;		/* replacement text */
                FILE    *fcode;
                char    command;	/* sed command to execute */
                int     gfl; 		/* global flag; true if cmd should */
					/* be performed on entire line     */
                char    pfl;		/* print flag */
                char    inar;		/* in_a_range - true if we are in  */
					/* the range between ad1 and ad2   */
                char    negfl;		/* negate range; i.e., if range is */
					/* 3-5, work on lines 1-2, 6-$     */
        } r1;
        struct reptr2 {
                char    *ad1;
                char    *ad2;
                union reptr     *lb1;
                char    *rhs;
                FILE    *fcode;
                char    command;
                int     gfl;
                char    pfl;
                char    inar;
                char    negfl;
        } r2;
};
extern union reptr ptrspace[];



struct label {
        char    asc[9];
        union reptr     *chain;
        union reptr     *address;
};



extern int     eargc;

extern union reptr     *pending;
extern char    *badp;
char    *compile();
char    *ycomp();
char    *address();
char    *text();
char    *compsub();
struct label    *search();
char    *gline();
char    *place();
