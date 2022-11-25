/* @(#) $Revision: 10.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
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


typedef struct TDS {
	    uchar	st;		/* symbol type (storage class)	*/
	    bits	fConstant : 1;	/* accompanying "adr" is value? */
	    bits	width	  : 7;	/* for bit fields, else 0	*/
	    bits	tq6	  : 2;	/* type qualifiers (prefixes)	*/
	    bits	tq5	  : 2;
	    bits	tq4	  : 2;
	    bits	tq3	  : 2;
	    bits	tq2	  : 2;
	    bits	tq1	  : 2;
	    bits	bt	  : 4;	/* base type */
	} TDR, *pTDR;


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
	    short	cb;		/* bytes in enum, struct, or union */

	    long	isymTq;		/* isym matching top tq (tq1) */

	} XTYR, *pXTYR;

#define cbXTYR (sizeof (XTYR))
#define xtyNil ((pXTYR) 0)


/***********************************************************************
 * ALTERNATE FORM OF TYPE RECORD EXTENSION FOR SAVING CONSTANT DOUBLES:
 *
 * When the TYR is a constant double with no type qualifiers, the
 * value is stored in the XTYR and accessed this way.  No one should
 * look at an XTYR except this way if (fConstant AND (bt == btDouble)
 * AND (tq1 == tqNil)).
 */

typedef struct {
	    double	doub;
	} DXTYR, *pDXTYR;




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
#define stReg		115		/* register symbol		*/
#define stStruct	K_FIELD		/* struct element		*/
#define stSource	K_SRCFILE	/* source file name		*/
#define stExtend	140		/* cdb:	 extension entry	*/
#define stSpc		150		/* cdb:	 special var, in parent */
#define stValue		170		/* cdb:	 value field is VALUE	*/
					/* used in expr evaluation	*/
#define stLeng		180		/* extension entry		*/

/*
 * Kludge:  To handle some HPSYMTAB base types unknown to the debugger we
 * set bt = btNil and btX = T_<type>.  Unfortunately, K_SET is a DNTT type,
 * not a base type, but it must be handled using the same sort of escape.
 */

#define	TX_SET	99			/* extended "base type" for K_SET */

