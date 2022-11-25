/* SCCS  quad.c   REV(64.1);       DATE(92/04/03        14:22:23) */
/* HPUX_ID @(#)quad.c	64.1 91/08/28 */

#if  defined(xcomp300_800) && defined(ANSI)

/* This file contains quad routines which are used in the s300 compiler
 * but are not in the s800 libraries;  so they need to be provided
 * specially for cross-compile situations (cxref, lint built from s300
 * sources on the s800).
 *
 * The contents of this file consists of parts of the following files from
 * the s300 quad library routines:
 *	quadfp.h
 *	U_Qflogneg.c
 *	U_Qfneg.c
 *	U_Qfcnvxf.c
 *
 * The asm's for generating exceptions have been commented out.  The
 * local math lib guru has assured me I can get away with this in the
 * constant folding case.
 */

/*----------------------------- quadfp.h ----------------------------------*/
/* this union can be used for quad floating-point or fixed-point numbers */
typedef union {
    unsigned u[4];
    double d[2];
} QUAD;

/* this union can be used for double floating-point or fixed-point numbers */
typedef union {
    unsigned u[2];
    double d;
} DBL;

/* this union can be used for single floating-point or fixed-point numbers */
typedef union {
    unsigned u;
    float f;
} SGL;

extern QUAD U_Qfadd();
extern QUAD U_Qfsub();
extern QUAD U_Qfmpy();
extern QUAD U_Qfdiv();
extern QUAD U_Qfrem();
extern QUAD U_Qfsqrt();
extern QUAD U_Qfrnd();
extern QUAD U_Qfabs();
extern int U_Qfcmp();

extern QUAD U_Qfcnvff_sgl_to_quad();
extern QUAD U_Qfcnvff_dbl_to_quad();
extern float U_Qfcnvff_quad_to_sgl();
extern double U_Qfcnvff_quad_to_dbl();

extern QUAD U_Qfcnvfx_sgl_to_quad();
extern QUAD U_Qfcnvfx_dbl_to_quad();
extern SGL U_Qfcnvfx_quad_to_sgl();
extern DBL U_Qfcnvfx_quad_to_dbl();
extern QUAD U_Qfcnvfx_quad_to_quad();

extern QUAD U_Qfcnvfxt_sgl_to_quad();
extern QUAD U_Qfcnvfxt_dbl_to_quad();
extern SGL U_Qfcnvfxt_quad_to_sgl();
extern DBL U_Qfcnvfxt_quad_to_dbl();
extern QUAD U_Qfcnvfxt_quad_to_quad();

extern QUAD U_Qfcnvxf_sgl_to_quad();
extern QUAD U_Qfcnvxf_dbl_to_quad();
extern SGL U_Qfcnvxf_quad_to_sgl();
extern DBL U_Qfcnvxf_quad_to_dbl();
extern QUAD U_Qfcnvxf_quad_to_quad();

/* convert formats */
#define QUAD_TO_SGL  0x3
#define QUAD_TO_DBL  0x7
#define SGL_TO_QUAD  0xc
#define DBL_TO_QUAD  0xd
#define QUAD_TO_QUAD 0xf
/*----------------------------- U_Qflogneg.c ----------------------------*/

/*#include "quadfp.h"*/

#define LDEQ 4
#define LDLT 8
#define LDGT 16

_U_Qflogneg(opnd2)
QUAD opnd2;
{
	QUAD opnd1;
	unsigned status, cond, excp;

	/* status is hardcoded in macros used by emulation routine */
	status = 0;
	opnd1.d[0] = 0;
	opnd1.d[1] = 0;

        /* do a a test != 0 */

	cond = LDEQ;

	/* call emulation routine */
	if (excp = _U_qfcmp(&opnd1, &opnd2, cond, &status)) {

		/* Force a Floating point exception */
		/*asm("	trap &8");*/
	}
	return(status);
}

/*----------------------- U_Qfneg.c ----------------------------------*/
/*#include "quadfp.h"*/

QUAD _U_Qfneg(opnd2)
QUAD opnd2;
{
	QUAD opnd1;
	QUAD result;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routine */
	status = 0;
	opnd1.d[0] = 0;
	opnd1.d[1] = 0;

	/* call emulation routine */
	if (excp = _U_qfsub(&opnd1, &opnd2, &result, &status)) {

		/* Force a Floating point exception */
		/*asm("	trap &8");*/
	}
	return(result);
}

/*---------------------- U_Qfcnvxf.c (partial) ---------------------------*/

/*#include "quadfp.h"*/

QUAD _U_Qfcnvxf_usgl_to_quad(opnd)
unsigned opnd;
{
	QUAD result;
        DBL tmp;
	unsigned status, excp;

	/* status is hardcoded in macros used by emulation routines */
	status = 0;

	/* Convert unsigned integer to 64-bit signed integer */
	tmp.u[0] = 0;
	tmp.u[1] = opnd;

	/* call emulation routine */
	if (excp = _U_dqfcnvxf(&tmp, 0, &result, &status)) {

		/* Force a Floating point exception */
		/*asm("	trap &8");*/
	}
	return(result);
}

# endif
