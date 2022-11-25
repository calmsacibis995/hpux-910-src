/*
 * @(#)macdefs.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:23:10 $
 * $Locker:  $
 */

/*
 * Original version based on: 
 * Revision 63.1  88/05/16  10:06:55  10:06:55  markm
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * This file defines machine-dependent types, sizes, etc.
 * SZ and AL values are BITS while CB values are BYTES.
 */

#ifdef FOCUS
#include <user.h>
#endif

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

#ifdef S200
#define	ARGINIT		64
#define	AUTOINIT	 0

#define	SZCHAR		 8
#define	SZINT		32
#define	SZFLOAT		32
#define	SZDOUBLE	64
#define	SZLONG		32
#define	SZSHORT		16
#define	SZPOINT		32

#define	ALCHAR		 8
#define	ALINT		16
#define	ALFLOAT		16
#define	ALDOUBLE	16
#define	ALLONG		16
#define	ALSHORT		16
#define	ALPOINT		16
#define	ALSTRUCT	16
#define	ALSTACK		16

#define EVENBYTES	/* accesses must be made on even byte boundaries */
#define LTORBYTES	/* bytes are numbered from left to right	 */
#define LTORBITS	/* bits are numbered from left to right		 */ 

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

#endif /* S200 */

#ifdef SPECTRUM
#define SZCHAR	  8
#define SZSHORT	 16
#define SZINT	 32
#define SZLONG	 32
#define SZPOINT	 32
#define SZFLOAT	 32
#define SZDOUBLE 64

#define ALCHAR	  8
#define ALSHORT	 16
#define ALINT	 32
#define ALLONG	 32
#define ALPOINT	 32
#define ALFLOAT	 32
#define ALDOUBLE 64
#define ALSTRUCT  8
#define ALSTACK  32

#define LTORBYTES	/* bytes are numbered from left to right	     */
#define LTORBITS	/* bits are numbered from left to right		     */

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

#endif /* SPECTRUM */

#ifdef FOCUS
#define SZCHAR	  8
#define SZSHORT	 16
#define SZINT	 32
#define SZLONG	 32
#define SZPOINT	 32
#define SZFLOAT	 32
#define SZDOUBLE 64

/*
 * These are from our C compiler; hope they are right.
 * They don't seem to be used anywhere.
 */
#define ALCHAR	  8
#define ALSHORT	 16
#define ALINT	 32
#define ALLONG	 32
#define ALPOINT	 32
#define ALFLOAT	 32
#define ALDOUBLE 32
#define ALSTRUCT 16
#define ALSTACK  32

/* #define EVENBYTES	/* accesses NEED NOT be made on even byte boundaries */
#define LTORBYTES	/* bytes are numbered from left to right	     */
#define LTORBITS	/* bits are numbered from left to right		     */
/* #define cbPage 512	/* number of bytes on a disk page (UNUSED)	     */

typedef union {
	    double	doub;
	    float	fl;
	    long	lng;
	    long	longs[2];		/* only FOCUS needs this */
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

#endif /* FOCUS */

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

#ifdef RTOLBITS
#define Extract(x, pos, width)	((x >> pos) & Mask (width))
#define SetBits(x, width, pos, val)			\
			{ x &= ~(Mask (width) << pos);	\
			  x |= val << pos;		\
			}
#else
#define Extract(x, pos, width)	((x >> (SZINT - pos - width)) & Mask (width))
#define SetBits(x, width, pos, val)					 \
			{ x &= ~(Mask (width) << (SZINT - pos - width)); \
			  x |= val << (SZINT - pos - width);		 \
			}
#endif

/*
 * adrReason is the address in a core file header where we can find the reason
 * for the child's death.  I know of no way to determine this number other than
 * by Divine insight, or by looking at the sources for the crash program(?).
 *
 * See InitAll() for more discussion.
 */

#ifdef S200
#define adrReg0	  0			/* not supported or needed */
#define adrReason 0			/* not supported or needed */
#endif

#ifdef FOCUS
#define adrReg0	  ((int *) off_u_pc)	/* start of registers in user.h */
#define adrReason 0			/* not supported or needed	*/
#endif

#ifdef SPECTRUM
#define adrReg0	  ((int *) 0)		/* start of registers in save_state.h */
#define adrflags  ((int *) 0)		/* trap or call flags in save_state.h */
#define adrReason 0 			/* we don't use it */
#endif

/***********************************************************************
 * INTERNAL NAMES FOR REGISTERS:
 *
 * The REAL offsets are in vrgOffset.  These are simply indices into it.
 *
 * --> For HPE, these are word offsets.  The offsets are into the
 *     HPE Modcal Type INTERRUPT_MARKER_TYPE.
 */

#ifdef M68000

#define		ud0	REG_D0
#define		ud1	REG_D1
#define		ud2	REG_D2
#define		ud3	REG_D3
#define		ud4	REG_D4
#define		ud5	REG_D5
#define		ud6	REG_D6
#define		ud7	REG_D7
#define		ua0	REG_A0
#define		ua1	REG_A1
#define		ua2	REG_A2
#define		ua3	REG_A3
#define		ua4	REG_A4
#define		ua5	REG_A5
#define		ua6	REG_A6
#define		ua7	REG_A7
#define		upc	40
#define		ups	41
#define		ufp	ua6
#define		usp	ua7

#ifdef S200
#define		ufp0	REG_FP0
#define		ufp1	REG_FP1
#define		ufp2	REG_FP2
#define		ufp3	REG_FP3
#define		ufp4	REG_FP4
#define		ufp5	REG_FP5
#define		ufp6	REG_FP6
#define		ufp7	REG_FP7
#define		ufpsr	42
#define		ufpcr	43
#define		ufpiar	44
#define		ufpa0	REG_FPA0
#define		ufpa1	REG_FPA1
#define		ufpa2	REG_FPA2
#define		ufpa3	REG_FPA3
#define		ufpa4	REG_FPA4
#define		ufpa5	REG_FPA5
#define		ufpa6	REG_FPA6
#define		ufpa7	REG_FPA7
#define		ufpa8	REG_FPA8
#define		ufpa9	REG_FPA9
#define		ufpa10	REG_FPA10
#define		ufpa11	REG_FPA11
#define		ufpa12	REG_FPA12
#define		ufpa13	REG_FPA13
#define		ufpa14	REG_FPA14
#define		ufpa15	REG_FPA15
#define		ufpacr	45
#define		ufpasr	46
#define		uf0	47
#define		uf1	48
#define		uf2	49
#define		uf3	50
#define		uf4	51
#define		uf5	52
#define		uf6	53
#define		uf7	54
#define		ufs	55
#define		ufe	56
#define		ugenregMax 16
#define		uregMax 18

#else /* not pure S200 */
#define		uregMax 20
#endif /* not S200 */

#else /* not M68000 */

#ifdef FOCUS				/* there are only a few registers */

#define		upc	 0		/* actually "P", as an EDSP	*/
#define		usp	 1		/* actually "S"			*/
#define		ufp	 2		/* actually "Q"			*/
#define		uregMax	 3

#else /* not FOCUS */

#define		u0	 0
#define		u1	 1
#define		u2	 2
#define		u3	 3
#define		u4	 4
#define		u5	 5
#define		u6	 6
#define		u7	 7
#define		u8	 8
#define		u9	 9
#define		u10	10
#define		u11	11
#define		u12	12
#define		u13	13

#ifdef SPECTRUM
#define		u14	14
#define		u15	15
#define		u16	16
#define		u17	17
#define		u18	18
#define		u19	19
#define		u20	20
#define		u21	21
#define		u22	22
#define		u23	23
#define		u24	24
#define		u25	25
#define		u26	26
#define		u27	27
#define		u28	28
#define		u29	29
#define		u30	30
#define		u31	31

#define		uarg3	u23
#define		uarg2	u24
#define		uarg1	u25
#define		uarg0	u26
#define		udp	u27
#define		usp	u30

#define		ud0	 0

#define		urp	u2
#define		umrp	u31
#define		uret0	u28
#define		uret1	u29
#define		usl	u29
 
#define		ufp0	32
#define		ufp1	33
#define		ufp2	34
#define		ufp3	35
#define		ufp4	36
#define		ufp5	37
#define		ufp6	38
#define		ufp7	39
#define		ufp8	40
#define		ufp9	41
#define		ufp10	42
#define		ufp11	43
#define		ufp12	44
#define		ufp13	45
#define		ufp14	46
#define		ufp15	47
#define		ufp16	48
#define		ufp17	49
#define		ufp18	50
#define		ufp19	51
#define		ufp20	52
#define		ufp21	53
#define		ufp22	54
#define		ufp23	55
#define		ufp24	56
#define		ufp25	57
#define		ufp26	58
#define		ufp27	59
#define		ufp28	60
#define		ufp29	61
#define		ufp30	62
#define		ufp31	63

#define		upc	64
#define		upc2	65
#define		upsw	66
#define		uspace	67
#define		uspace2	68
#define		usar	69
#define		utr0	70
#define		utr1	71
#define		utr2	72
#define		utr3	73
#define		utr4	74
#define		utr5	75
#define		utr6	76
#define		utr7	77
#define		usr0	78
#define		usr1	79
#define		usr2	80
#define		usr3	81
#define		usr4	82
#define		usr5	83
#define		usr6	84
#define		usr7	85
#define		upid1	86
#define		upid2	87
#define		upid3	88
#define		upid4	89
#define		uccr	90
#define		ueiem	91
#define		urctr	92
#define		uitmr	93
#define		ueirr	94
#define		uisr	95
#define		uior	96
#define		uiva	97
#define		uiir	98

#define		ugenregMax 32
#define		ufpregMax  64
#define		uregMax    99

#define	calle_save(X)  (X <= u18)
#define	caller_save(X) (X > u18)

#endif /* SPECTRUM */

#endif /* not FOCUS  */
#endif /* not M68000 */
