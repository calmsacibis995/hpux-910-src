/* @(#) $Revision: 10.2 $ */     
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This file defines machine-dependent types, sizes, etc.
 * SZ and AL values are BITS while CB values are BYTES.
 */

/*
 * SIZE AND ALIGNMENT REQUIREMENTS of the various types in BITS.
 * This information differs from machine to machine.
 * If you have the source for the PCC compiler, this can be found in the
 * "macdefs.h" file.
 *
 * ANY use of the union STUFFU is inherently MACHINE DEPENDENT.
 * It is used for stuffing bytes into longs, etc.
 * Most machines can be made to work if you move the fields and recompile.
 */

#define	SZCHAR		 8
#define	SZINT		32
#define	SZFLOAT		32
#define	SZDOUBLE	64
#define	SZLONG		32
#define	SZSHORT		16
#define	SZPOINT		32

typedef union {
	    double	doub;
	    float	fl;
	    long	lng;
	    long	longs[2];
	    struct {
		short	shortHi;
		short	shortLo;
	    } shorts;
	    struct {
		char	chHiHi;
		char	chHiLo;
		char	chLoHi;
		char	chLoLo;
	    } chars;
	} STUFFU;

/***********************************************************************
 * GENERIC STUFF based on the above:
 */

#define CBCHAR	 1			/* hope correct for all machines */
#define CBSHORT	 (SZSHORT  / SZCHAR)
#define CBINT	 (SZINT	   / SZCHAR)
#define CBLONG	 (SZLONG   / SZCHAR)
#define CBPOINT	 (SZPOINT  / SZCHAR)
#define CBFLOAT	 (SZFLOAT  / SZCHAR)
#define CBDOUBLE (SZDOUBLE / SZCHAR)

#define shortMax (((unsigned) ~0) >> (SZINT - SZSHORT + 1))
						/* largest  signed short   */
#define shortMin (-shortMax - 1)		/* smallest signed short   */
#define intMax	 (((unsigned) ~0) >> 1)		/* largest  signed integer */
#define intMin	 (-intMax - 1)			/* smallest signed integer */
#define longMax	 (((unsigned) ~0L) >> 1)	/* largest  signed long	   */
#define longMin	 (-longMax - 1)			/* smallest signed long	   */

#define Mask(width) (~ (~0L << width))		/* generate a lot of 111's */

#define Extract(x, pos, width)	((x >> (SZINT - pos - width)) & Mask (width))
#define SetBits(x, width, pos, val)					 \
			{ x &= ~(Mask (width) << (SZINT - pos - width)); \
			  x |= val << (SZINT - pos - width);		 \
			}

/***********************************************************************
 * INTERNAL NAMES FOR REGISTERS:
 *
 * The REAL offsets are in vrgOffset.  These are simply indices into it.
 */

#define		ud0	 0
#define		ud1	 1
#define		ud2	 2
#define		ud3	 3
#define		ud4	 4
#define		ud5	 5
#define		ud6	 6
#define		ud7	 7
#define		ua0	 8
#define		ua1	 9
#define		ua2	10
#define		ua3	11
#define		ua4	12
#define		ua5	13
#define		ua6	14
#define		ua7	15
#define		upc	16
#define		ups	17
#define		ufp	18
#define		usp	19
#define		uusp	20
#define		uregMax 21
