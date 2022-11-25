/*
 * @(#)ty.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:24:05 $
 * $Locker:  $
 */

/*
 * Original version based on: 
 * Revision 62.2  88/04/20  08:34:26  08:34:26  sjl (Steve Lilker)
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
 * This file defines types, structures, etc. for handling type information
 * in memory during expression evaluation.
 */

#define ityNil (-1)
#define cTyMax	 2		/* most TYRs needed to describe any one var */


/***********************************************************************
 * TYPE DESCRIPTOR:
 *
 * These float around in memory during expression evaluation.
 */

#ifdef M68000

typedef struct TDS {
	    uchar	st;		/* symbol type (storage class)	*/
	    bits	fConstant : 1;	/* accompanying "adr" is value? */
					/* HPSYMTABs: constant value?   */
	    bits	width	  : 7;	/* for bit fields, else 0	*/
	    bits	tq6	  : 2;	/* type qualifiers (prefixes)	*/
	    bits	tq5	  : 2;
	    bits	tq4	  : 2;
	    bits	tq3	  : 2;
	    bits	tq2	  : 2;
	    bits	tq1	  : 2;
	    bits	bt	  : 4;	/* base type */
	} TDR, *pTDR;

#else /* not M68000 */

typedef struct TDS {
	    uchar	st;		/* symbol type (storage class)	*/
	    bits	fConstant : 1;	/* accompanying "adr" is value? */
					/* HPSYMTABs: constant value?	*/
	    bits	width	  : 7;	/* for bit fields, else 0	*/
	    bits	bt	  : 4;	/* base type			*/
	    bits	tq1	  : 2;	/* type qualifiers (prefixes)	*/
	    bits	tq2	  : 2;
	    bits	tq3	  : 2;
	    bits	tq4	  : 2;
	    bits	tq5	  : 2;
	    bits	tq6	  : 2;
	} TDR, *pTDR;

#endif /* not M68000 */

#define cbTDR (sizeof (TDR))
#define tdNil ((pTDR) 0)


/***********************************************************************
 * TYPE RECORD ("nlist" structure):
 *
 * This is how types are manipulated internally.  For some symbol tables
 * this is exactly how data appears in the object file.
 */

typedef struct {
	    long	sbVar;			/* index into string table */
	    TDR		td;			/* type descriptor	 */
	    ulong	valTy;			/* misc value; see below */
	    bits	fAdrIsVal : 1;		/* accompanying adr is val */
	    bits	dummy : 15;		/* unused -- XTYR larger */
	} TYR, *pTYR;

/*
 * valTy is either:
 *	a field offset (in bytes or bits, as appropriate) for fields;
 *	an address or register number for debugger locals (stSpc or stReg);
 *	the value of a member of enumeration (in which case fConstant is set);
 *	the size of a struct or union in a stLeng entry.
 *
 * It must be zero or a field offset unless the one of the last three applies.
 */

#define cbTYR (sizeof (TYR))
#define tyNil ((pTYR) 0)


/***********************************************************************
 * TYPE RECORD EXTENSION ENTRY:
 *
 * This remembers structure affiliation, array dimensions, etc.  It
 * must be carefully matched to TYR so that "st" lands on the same byte,
 * for those symbol table formats which use it.
 */

typedef struct {
	    long	isymRef;	/* isym of matching symbol */
	    uchar	st;		/* must be stExtend or stLeng	   */
	    char	btX;		/* extended base type for HPSYMTAB */
	    long	cb;		/* bytes in enum, struct, or union */
	    long	isymTq;		/* isym matching top tq (tq1) */
	} XTYR, *pXTYR;

#define cbXTYR (sizeof (XTYR))
#define xtyNil ((pXTYR) 0)


/***********************************************************************
 * ALTERNATE FORM OF TYPE RECORD EXTENSION FOR SAVING CONSTANT DOUBLES:
 *
 * When the TYR is a constant double with no type qualifiers, the
 * value is stored in the XTYR and accessed this way.  No one should
 * look at an XTYR this way except if (fAdrIsVal AND (bt == btDouble)
 * AND (tq1 == tqNil)).
 */

typedef struct {
	    double	doub;
	    short	dummy;		/* unused */
	} DXTYR, *pDXTYR;


#ifdef HPSYMTABII
/***********************************************************************
 * UNION OF DXTYR & TYR TO FORCE DOUBLE WORD ALLIGNMENT
 *
 * Spectrum requires double word allignment of double reals (in a
 * DXTYR).  All over XDB type records are declared as type TYR 
 * (which does not force double allignment), and cast to a DXTYR
 * when needed. The correct solution would be to change all uses
 * of TYRs XTYRs and DXTYRs to use a union of the 3 types. For now,
 * all variables (but not parameters) of type TYR will be changed to
 * pointers to a variable of this type, so no other code need change.
 */

typedef union {
           TYR   type;
           DXTYR dxtype;
        } UTYR;


/***********************************************************************
 * COBOL TABLE SIZE AND BOUNDS RECORD
 */

typedef struct {
          int low;
          int high;
          int bitsize;
       } CTR;
#endif


/***********************************************************************
 * TYPE QUALIFIERS for TDR.tq1 - TDR.tq6:
 */

#define tqNil		0	/* as in "int x"	*/
#define tqPtr		1	/* as in "int *x"	*/
#define tqFunc		2	/* as in "int (*x)()"	*/
#define tqArray		3	/* as in "int x[]"	*/


/***********************************************************************
 * BASIC TYPES seen in TDR.bt:
 */

#define btNil		0	/* unknown or extended (see XTYR.bt) */
#define btFArg		1	/* func argument */
#define btChar		2
#define btShort		3
#define btInt		4
#define btLong		5
#define btFloat		6
#define btDouble	7
#define btStruct	8
#define btUnion		9
#define btEType		10	/* enumeration	  */
#define btEMember	11	/* enum member	  */
#define btUChar		12	/* unsigned char  */
#define btUShort	13	/* unsigned short */
#define btUInt		14	/* unsigned int	  */
#define btULong		15	/* unsigned long  */


/***********************************************************************
 * ALIASES FOR ST (symbol type):
 *
 * Based on symtab.h values when possible, else arbitrary numbers in 0-255
 * (must fit in unsigned char!).  stNil overlaps K_SRCFILE, but that's OK
 * because the latter should never show up in a TYR.
 */

#define stNil		0		/* type info is undefined	*/
#define stPascal	100		/* global pascal symbol		*/
#define stGlobal	K_SVAR		/* global (static) symbol	*/
#define stFName		K_FUNCTION	/* procedure name (f77 kludge)	*/
#define stProc		K_FUNCTION	/* procedure name		*/
#define stStatic	K_SVAR		/* static symbol		*/
#define stLComm		110		/* .lcomm symbol		*/
#define stReg		115		/* register symbol		*/
#define stLine		120		/* source line (we use SLT)	*/
#define stStruct	K_FIELD		/* struct element		*/
#define stSource	K_SRCFILE	/* source file name		*/
#define stLocal		K_DVAR		/* local (dynamic) symbol	*/
#define stInclude	K_SRCFILE	/* include file name		*/
#define stParam		K_FPARAM	/* formal parameter		*/
#define stEntry		K_ENTRY		/* alternate entry		*/
#define stLBrac		K_BEGIN		/* left bracket			*/
#define stRBrac		K_END		/* right bracket		*/
#ifdef HPSYMTABII
#define stBComm		K_COMMON	/* begin common			*/
#endif
#define stEComm		K_END		/* end common			*/
#define stELocal	132		/* end common			*/
#define stExtend	140		/* cdb:	 extension entry	*/
#define stSpc		150		/* cdb:	 special var, in parent */
#define stPure		160		/* cdb:	 this is PURE type info */
#define stValue		170		/* cdb:	 value field is VALUE	*/
					/* used in expr evaluation	*/
#define stLeng		180		/* extension entry		*/

/*
 * Kludge:  To handle some HPSYMTAB(II) base types unknown to the debugger we
 * set bt = btNil and btX = T_<type>.  Unfortunately, K_SET is a DNTT type,
 * not a base type, but it must be handled using the same sort of escape.
 */

#define	TX_COBOL      95	/* extended "base type" for Cobol records */
#define	TX_BOOL_VAX   96	/* extended "base type" for booleans */
#define	TX_BOOL_S300  97	/* extended "base type" for booleans */
#define	TX_BOOL	      98	/* extended "base type" for booleans */
#define	TX_SET	      99	/* extended "base type" for K_SET */

#ifdef HPSYMTABII
/*
 * Classes of Cobol Items
 */

#define Group              0
#define Numeric            1
#define Alphabetic         2
#define Alphanumeric       3
#define NumericEdited      4
#define AlphanumericEdited 5

/*
 * Values for the "CatUsage" field of the DNTT COBSTRUCT
 */

#define uc_i     0 	/* index (integer) 			*/
#define uc_cu    1 	/* comp unsigned   			*/
#define uc_cs    2 	/* comp signed     			*/
#define uc_c3u   3 	/* comp-3 unsigned   			*/
#define uc_c3s   4 	/* comp-3 unsigned   			*/
#define uc_nu    5  	/* numeric unsigned			*/
#define uc_ns    6  	/* numeric signed			*/
#define uc_nl    7  	/* numeric leading 			*/
#define uc_nls   8  	/* numeric leading sign seperate 	*/
#define uc_nts   9  	/* numeric trailing sign seperate 	*/
#define uc_ne   10 	/* numeric edited 			*/
#define uc_ane  11 	/* alphanumeric edited   		*/
#define uc_a    12 	/* alphabetic    			*/
#define uc_aj   13 	/* alphabetic right justified   	*/
#define uc_an   14 	/* alphanumeric   			*/
#define uc_anj  15 	/* alphanumeric right justified  	*/

#endif
