/* HPUX_ID: @(#) $Revision: 66.2 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
#define R_FLAG	01
#define I_FLAG	02
#define U_FLAG	04
#define L_FLAG	010
#define Z_FLAG	020
#define W_FLAG	040
#define S_FLAG	0100
#define T_FLAG	0200
#define M_FLAG	0400
#define X_FLAG	01000
#define N_FLAG	02000
#define F_FLAG	04000
#define G_FLAG	010000
#define P_FLAG	020000
#define V_FLAG	040000
#define E_FLAG	0100000
#define	A_FLAG	0200000
#define B_FLAG	0400000
#define	C_FLAG	01000000
#define	D_FLAG	02000000
#define	H_FLAG	04000000


/*
 *  These flags are used by shlib/strmatch.c:onematch()
 *  and sh/msg.c:ctype_funcs to denote which character
 *  class was found in bracket expressions.
 */

#define	CTYPE_ALNUM	00001
#define	CTYPE_ALPHA	00002
#define	CTYPE_ASCII	00004
#define	CTYPE_CNTRL	00010
#define	CTYPE_DIGIT	00020
#define	CTYPE_GRAPH	00040
#define	CTYPE_LOWER	00100
#define	CTYPE_PRINT	00200
#define	CTYPE_PUNCT	00400
#define	CTYPE_SPACE	01000
#define	CTYPE_UPPER	02000
#define	CTYPE_XDIGIT	04000

