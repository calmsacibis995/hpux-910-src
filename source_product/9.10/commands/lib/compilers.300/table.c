/* file table.c */
/*    SCCS    REV(64.1);       DATE(92/04/03        14:22:32) */
/* KLEENIX_ID @(#)table.c	64.1 91/08/28 */

# include "mfile2"

#ifdef UNDOTSTFIX
#define QFORCC FORCC
#define QRESCC RESCC
#else
#define QFORCC 0
#define QRESCC 0
#endif

# define TSCALAR TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT|TLONG|TULONG
# define EA  SNAME|SOREG|SCON|STARREG|SAREG|SBREG
#		define EA_FLT EA|STAREG|STBREG
# define EAX  SNAME|SOREG|SCON|SAREG|SBREG	/* no post increment/decrement*/
# define EAA SNAME|SOREG|SCON|STARREG|SAREG
# define EAAX SNAME|SOREG|SCON|SAREG	/* no post increment/decrement */
# define EB SBREG
# define DATAALTERABLE SAREG|STAREG|STARREG|SOREG|SNAME
# define IMMEDIATE SNAME|SCON

/************************************************************************
*									*
*	Definitions gleaned from the code:				*
*									*
*	SAREG -		Operand is in an A (data) register		*
*	STAREG -	Operand is in a temporary A register		*
*	SBREG -		Operand is in a B (address) register		*
*	STBREG -	Operand is in a temporary B register		*
*	SFREG -		Operand is in a F register		        *
*	STFREG -	Operand is in a temporary F register		*
*	SDREG -		Operand is in a Dragon F register		*
*	STDREG -	Operand is in a temporary Dragon F register	*
*	SCC -		Operand is in the condition codes		*
*	SCON -		Operand is a constant				*
*	SCCON -		Operand is a constant -128 <= c <= 127		*
*	S8CON -		Operand is a constant 1 <= c <= 8		*
*	SFLD -		Operand is a subfield of a word.		*
*	STARNM -	Operand is accessed indirectly thru a register	*
*				variable				*
*			e.g. the subtree:				*
*				U* ...					*
*					OREG ...			*
*	STARREG -	Operand is accessed indirectly thru a register  *
*				pointer (post increment or decrement)	*
*	SZERO -		Operand is the special constant '0'		*
*	SONE -		Operand is the special constant '1'		*
*	SMONE -		Operand is the special constant '-1'		*
*	SOREG -		Operand is addressable by register + offset	*
*	SICON -		Operand is a constant -32768 <= c <= 32767	*
*	STASG -		Structure assignment				*
*	SNAME -		Operand is addressable with '&' (immediate)	*
*									*
*	OPLTYPE -	Leaf type node (ICON, NAME, etc.)		*
*************************************************************************

NOTES:
	1.  All templates specifying FORCC should also specify RESCC.
	Otherwise the message "Illegal reclaim" can occur.

	2.  Any template specifying Zc (also implied by ZK) should also
	request NAREG|NBREG in addition to the result register so that
	the movm.l can be added as a 'wait' for the float card.  The
	result field RESCx should be adjusted accordingly as well.

# ifdef LCD
	3.  Zb and Za specifiers should always have a numeric suffix
	which is the index into the resc[] array for the result F reg.
# endif	

	4. All templates specifying FORARG should specify RNULL. Failure
	to do so will result in a "cannot reclaim" error.

	5. STARREG cannot be handled well by upput() when the type is 
	   TDOUBLE because the increment value is 8.

*/




/* optab defined in mfile2 */

struct optab  table[] = {

ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|STAREG,	TSCALAR,
	SCCON,	TSCALAR,
		0,	RLEFT  |RRIGHT|RESCC  ,
#ifndef cookies
		"	movq	AR,AL\n",
#else
		"	movq	AR,AL\t#template 1\n",
#endif





ASSIGN,	INAREG|FOREFF|FORCC,
	EAA,	TSCALAR,
	SZERO,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	clrZL	AL\n",
#else
		"	clrZL	AL\t#template 2\n",
#endif



ASSIGN,	INAREG|FOREFF,
	EAA,	TFLOAT,
	SZERO,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	clrZL	AL\n",
#else
		"	clrZL	AL\t#template 2b\n",
#endif




ASSIGN,	INAREG|FOREFF,
	EAAX,	TDOUBLE,
	SZERO,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	clr.l	AL\n	clr.l	UL\n",
#else
		"	clr.l	AL\t#template 3a\n	clr.l	UL\t#template 3b\n",
#endif




#ifndef FORTY
/* this template is only a code "improver". It may not be justified when the
   MC68020 is used. Removing this template just causes a slightly slower and
   less efficient code sequence to be generated.
*/
ASSIGN,	FOREFF|FORCC,
	SOREG|SNAME,	TSCALAR,
	SCCON,		TSCALAR,
		NAREG,	RLEFT|RRIGHT|RESCC|RESC1,
#ifndef cookies
		"	movq	AR,A1\n	movZL	A1,AL\n",
#else
		"	movq	AR,A1\t#template 3c\n	movZL	A1,AL\t#template 3d\n",
#endif
#endif /* FORTY */




ASSIGN, INAREG|INTAREG|FOREFF|FORCC,
	SAREG|STAREG,	TCHAR|TSHORT|TINT|TLONG,
	EA,	TCHAR|TSHORT|TINT|TLONG|TUNSIGNED,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	movZL	AR,AL\nZEL",
#else
		"\tmovZL\tAR,AL\t#template 4a\nZEL",
#endif




ASSIGN, INAREG|INTAREG|FOREFF|FORCC,
	EA,	TSCALAR,
	SAREG|STAREG,	TCHAR|TSHORT|TINT|TLONG|TUNSIGNED,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"ZER	movZL	AR,AL\n",
#else
		"ZER\tmovZL\tAR,AL\t#template 4b\n",
#endif




/* this template should always follow 4ab and 4cd so that tshort->tshort and
   tchar->tchar assignments get extended properly.
*/

ASSIGN,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TSCALAR,
	EA,	TUCHAR|TUSHORT|TCHAR|TSHORT,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"ZF\tmovZL\tAR,AL\nZeL",
#else
		"ZF\tmovZL\tAR,AL\t#template 4e\nZeL",
#endif






/* the following template has been broken down into constituent parts above
   to correct a bug in code generation. What doesn't get caught in 4a-f gets
   handled here.
*/
ASSIGN,	INAREG|INTAREG|FOREFF|FORCC,
	EAA,	TSCALAR,
	EA,	TSCALAR,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	movZl	AR,AL\n",
#else
		"	movZl	AR,AL\t#template 4\n",
#endif




ASSIGN,	INAREG|INTAREG|FOREFF,
	EAA,	TFLOAT,
	EA,	TFLOAT,
		0,	RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"	movZl	AR,AL\n",
#else
		"	movZl	AR,AL\t#template 4g\n",
#endif




ASSIGN,	INFREG|INTFREG|FORCC|FOREFF,
	SFREG|STFREG,				TANY,
	EA_FLT|SFREG|STFREG|SDREG|STDREG,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate 5r\n",
# endif



ASSIGN,	INAREG|INTAREG|INBREG|INTBREG|FORCC|FOREFF,
	EA_FLT,					TANY,
	SFREG|STFREG,				TANY,
		0,	RLEFT|RRIGHT|RESCC,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate 5sa\n",
# endif



ASSIGN,	INAREG|INTAREG|INBREG|INTBREG|FOREFF,
	EA_FLT,					TANY,
	SDREG|STDREG,				TANY,
		0,	RLEFT|RRIGHT,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate 5sb\n",
# endif



ASSIGN,	INAREG|INTAREG|INBREG|INTBREG|FORCC|FOREFF,
	EA_FLT,					TFLOAT|TDOUBLE,
	EA_FLT,					TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT|RESCC,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate 5t\n",
# endif




ASSIGN,	INDREG|INTDREG|FOREFF,
	SDREG|STDREG,				TANY,
	EA_FLT|SFREG|STFREG|SDREG|STDREG,	TANY,
		0,	RLEFT|RRIGHT,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate d5r\n",
# endif


ASSIGN,	INAREG|INTAREG|INBREG|INTBREG|FOREFF,
	EA_FLT,					TANY,
	SFREG|STFREG|SDREG|STDREG,		TANY,
		0,	RLEFT|RRIGHT,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate d5s\n",
# endif



ASSIGN,	INAREG|INTAREG|INBREG|INTBREG|FOREFF,
	EA_FLT,					TFLOAT|TDOUBLE,
	EA_FLT|SFREG|STFREG|SDREG|STDREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT,
# ifndef cookies
		"ZD",
# else
		"ZD#\ttemplate d5t\n",
# endif




ASSIGN, INTAREG|FORCC|FOREFF,
        SNAME|SOREG,                       TLONGDOUBLE,
        SNAME|SOREG,                       TLONGDOUBLE,
                0,      RLEFT|RRIGHT|RESCC,
#ifndef cookies
		"\tmov.l\tWR,WL\n\tmov.l\tVR,VL\n\tmov.l\tUR,UL\n\tmov.l\tAR,AL\n",
#else
		"\tmov.l\tWR,WL\t#template q5a\n\tmov.l\tVR,VL\t#template q5a\n\tmov.l\tUR,UL\t#template q5a\n\tmov.l\tAR,AL\t#template q5a\n",
#endif









ASSIGN,	INBREG|FOREFF,
	EB,	TSCALAR,
	SZERO,	TSCALAR,
		0,	RLEFT|RRIGHT,
#ifndef cookies
		"\tsuba.l\tAL,AL\n",
#else
		"\tsuba.l\tAL,AL\t#\ttemplate 7\n",
#endif






ASSIGN,	INBREG|FOREFF,
	EB,	TSCALAR,
	EA,	TSCALAR,
		0,	RLEFT|RRIGHT,
#ifndef cookies
		"ZF\tmovZR\tAR,AL\n",
#else
		"ZF\tmovZR\tAR,AL\t#\ttemplate 7a\n",
#endif






# ifndef FORT

/* This template is used regardless of LCD flag because masks are in general
   faster than bfclr or bfins instructions. If timing relationship should
   change in the future, some consideration should be given to altering or
   removing this template.
*/
ASSIGN, FOREFF,
	SFLD,	TANY,
	SICON,	TANY,
		0,	RRIGHT,
#	ifndef cookies
		"	ZYa\n",
#	else	/* cookies */
		"	ZYa\t#template 8\n",
#	endif	/* cookies */





ASSIGN, FOREFF|INTAREG,
	SFLD,			TANY,
	SAREG|STAREG,	TANY,
		0,	RRIGHT,
#		ifndef cookies
		"	Z0\n",
#		else	/* cookies */
		"	Z0\t#template 9\n",
#		endif	/* cookies */
# endif	/* FORT */




/* the following 2 templates differs from template 4 only in the FREG stuff and
   in the macros.
*/





ASSIGN,		INTBREG|INBREG,
	SBREG|STBREG,			TPOINT,
	SADRREG,				TPOINT,
		0,	RLEFT,
#ifndef cookies
		"	lea	AR,AL\n",
#else
		"	lea	AR,AL\t#template 10e\n",
#endif



/* put this here so UNARY MUL nodes match OPLTYPE when appropriate */
UNARY MUL,	INTAREG|INAREG|FORCC,
	SBREG,	TSCALAR,
	SANY,	TANY,
		NAREG|NASR,	RESC1|RESCC,
#ifndef cookies
		"	movZB	(AL),A1\n",
#else
		"	movZB	(AL),A1\t#template 11\n",
#endif






/* OPLTYPE nodes are used for ltypes that are the correct type but aren't
   where they need to be for subsequent operations (i.e. wrong shape).
*/

OPLTYPE,	FOREFF,
	SNAME|SOREG|SCON|SAREG|SBREG,	TANY,
	EA,	TANY,
		0,	RRIGHT,
#ifndef cookies
		"",   /* this entry throws away computations which don't do anything */
#else
		"#\ttemplate 12\n",
#endif






OPLTYPE,	FORCC,
	SANY,	TANY,
	DATAALTERABLE,	TSCALAR,
		0,	RESCC,
#	ifndef cookies
		"	tstZB	AR\n",
#	else
		"	tstZB	AR\t#template 13\n",
#	endif





OPLTYPE,	FORCC,
	SANY,				TANY,
	SDREG|STDREG,			TFLOAT|TDOUBLE,
		0,	RESCC,
#	ifndef cookies
		"	fptestZR	AR\n",
#	else
		"	fptestZR	AR\t#template d14\n",
#	endif




OPLTYPE,	FORCC,
	SANY,		TANY,
	STARREG|SOREG|SNAME|SCON,	TFLOAT|TDOUBLE,
		NDREG,	RESCC,
#	ifndef cookies
		"\tfpmovZR\tAR,A1\n\tfptestZR\tA1\n",
#	else
		"\tfpmovZR\tAR,A1\t#template d14a\n\tfptestZR\tA1\n",
#	endif



OPLTYPE,	FORCC,
	SANY,				TANY,
	SAREG|STAREG|SBREG|STBREG,	TFLOAT,
		NDREG,	RESCC,
#	ifndef cookies
		"\tfpmovZR\tAR,A1\n\tfptestZR\tA1\n",
#	else
		"\tfpmovZR\tAR,A1\t#template d14bx\n\tfptestZR\tA1\t#template d14by\n",
#	endif





OPLTYPE,	FORCC,
	SANY,		TANY,
	DATAALTERABLE,	TFLOAT,
		0,	RESCC,
#	ifndef cookies
		"	ftestZB	AR\n",
#	else
		"	ftestZB	AR\t#template 14\n",
#	endif





OPLTYPE,	FORCC,
	SANY,				TANY,
	STARREG|SOREG|SNAME|SFREG|STFREG,	TFLOAT|TDOUBLE,
		0,	RESCC,
#	ifndef cookies
		"	ftestZR	AR\n",
#	else
		"	ftestZR	AR\t#template 15\n",
#	endif







OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SCCON,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
#ifndef cookies
		"	movq	AR,A1\n",
#else
		"	movq	AR,A1\t#template 16\n",
#endif






OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	EAA,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
#ifndef cookies
		"	movZB	AR,A1\n",
#else
		"	movZB	AR,A1\t#template 17a\n",
#endif






OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SBREG,	TSHORT|TUSHORT|TINT|TUNSIGNED|TLONG|TULONG|TPOINT,
		NAREG|NASR,	RESC1|RESCC,
#ifndef cookies
		"	movZB	AR,A1\n",
#else
		"	movZB	AR,A1\t#template 17b\n",
#endif






/* this template picks up anything template 17b can't */
/* mov.b not allowed from an address reg. mov.w is an overkill that works,
   however.
*/
OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SBREG,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
#ifndef cookies
		"	mov.w	AR,A1\n",
#else
		"	mov.w	AR,A1\t#template 17c\n",
#endif






OPLTYPE,	INTDREG,
	SANY,					TANY,
	SAREG|STAREG|SBREG|STBREG,	TDOUBLE,
		NDREG,	RESC1,
#	ifndef cookies
		"\tfpmov.d\tAR:UR,A1\n",
#	else
		"\tfpmov.d\tAR:UR,A1\t# template d18c\n",
#	endif



OPLTYPE,	INTFREG|FORCC,
	SANY,					TANY,
	SAREG|STAREG|SBREG|STBREG,	TDOUBLE,
		2*NTEMP|NFREG,	RESC1|RESC2|RESCC,
#	ifndef cookies
		"\tmov.l\tAR,A2\n\tmov.l\tUR,U2\n\tfmov.d\tA2,A1\n",
#	else
		"\tmov.l\tAR,A2\t# template 18c\n\tmov.l\tUR,U2\n\tfmov.d\tA2,A1\
		# template 18d\n",
#	endif






OPLTYPE,	INTDREG,
	SANY,	TANY,
	EAA|STAREG|STBREG,	TSCALAR|TFLOAT|TDOUBLE,
		NDREG|NDSR,	RESC1,
#	ifndef cookies
		"\tfpmovZB\tAR,A1\n",
#	else
		"\tfpmovZB\tAR,A1\t#template d18e\n",
#	endif

OPLTYPE,	INTDREG|FORCC,
	SANY,	TANY,
	SAREG|STAREG|SBREG|STBREG,	TDOUBLE,
		NDREG,		RESC1|RESCC,
#	ifndef cookies
		"\tfpmov.d\tAR:UR,A1\n\tfptest.d\tA1\n",
#	else
		"\tfpmov.d\tAR:UR,A1\t#template d18gx\n\tfptest.d\tA1\t#template d18gy\n",
#	endif


OPLTYPE,	INTFREG|FORCC,
	SANY,	TANY,
	EAA,	TSCALAR|TFLOAT|TDOUBLE,
		NFREG|NFSR,	RESC1|RESCC,
#	ifndef cookies
		"\tfmovZB\tAR,A1\n",
#	else
		"\tfmovZB\tAR,A1\t#template 18e\n",
#	endif

OPLTYPE,	INTFREG|INTDREG|FORCC,
	SANY,	TANY,
	SBREG|STBREG,	TSCALAR,
		NAREG,		RESC1|RESCC,
#	ifndef cookies
		"\tmovZB\tAR,A1\n",
#	else
		"\tmovZB\tAR,A1\t#template 18g\n",
#	endif



OPLTYPE,	INTFREG|FORCC,
	SANY,	TANY,
	SFREG,	TFLOAT|TDOUBLE,
		NFREG,	RESC1|RESCC,
#ifndef cookies
		"\tfmov.x\tAR,A1\n",
#else
		"\tfmov.x\tAR,A1\t#template 18f\n",
#endif



OPLTYPE,	INTDREG,
	SANY,	TANY,
	SDREG,	TFLOAT|TDOUBLE,
		NDREG,	RESC1,
#ifndef cookies
		"\tfpmovZB\tAR,A1\n",
#else
		"\tfpmovZB\tAR,A1\t#template d18f\n",
#endif


OPLTYPE,	FORCC,
	SANY,	TANY,
	SDREG|STDREG,	TFLOAT|TDOUBLE,
		0,	RESCC,
#ifndef cookies
		"\tfptestZB\tAR\n",
#else
		"\tfptestZB\tAR\t#template d18j\n",
#endif


OPLTYPE,	INTFREG|INFREG|FOREFF,
	SANY,	TANY,
	SDREG|STDREG,	TFLOAT|TDOUBLE,
		NFREG,	RESC1,
#	ifndef cookies
		"\tfpmovZB\tAR,-(%sp)\n\tfmovZB\t(%sp)+,A1\n",
#	else
		"\tfpmovZB\tAR,-(%sp)\t#template d18h\n\tfmovZB\t(%sp)+,A1\t#template d18i\n",
#	endif


OPLTYPE,	INTDREG|INDREG|FOREFF,
	SANY,	TANY,
	SFREG|STFREG,	TFLOAT|TDOUBLE,
		NDREG,	RESC1,
#	ifndef cookies
		"\tfmovZB\tAR,-(%sp)\n\tfpmovZB\t(%sp)+,A1\n",
#	else
		"\tfmovZB\tAR,-(%sp)\t#template 18h\n\tfpmovZB\t(%sp)+,A1\t#template 18i\n",
#	endif




OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	SNAME|SCON|SAREG|SBREG,	TDOUBLE,
		NAREG|NASR,	RESC1,
#ifndef cookies
		"\tmov.l\tUR,U1\n\tmov.l\tAR,A1\n",
#else
		"\tmov.l\tUR,U1\t#template 18a\n\tmov.l\tAR,A1\t#template 18a\n",
#endif


/* The following is broken off the above template so that the order of the
   two moves can be changed.  The order is chosen in order to eliminate the
   problem of using the destination register of the first move in the second
   of move needed to accomplish the double move.  This is only a problem
   where the source is a complex OREG using a data reg (AREG) as part of the
   offset.
*/



OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	SOREG,	TDOUBLE,
		NAREG|NASR,	RESC1,
#ifndef cookies
		"Zd\n",
#else
		"Zd\t#template 18b\n",
#endif




OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	EA,	TFLOAT,
		NAREG|NASR,	RESC1,
#ifndef cookies
		"	mov.l	AR,A1\n",
#else
		"	mov.l	AR,A1\t#template 19\n",
#endif




OPLTYPE,	INTAREG|INAREG,
	SANY,		TANY,
	SFREG|STFREG,	TFLOAT,
		NAREG|NASR,	RESC1,
#ifndef cookies
		"	fmov.s	AR,A1\n",
#else
		"	fmov.s	AR,A1\t#template 19b\n",
#endif




#ifdef FORTY

OPLTYPE,	INTBREG|INBREG,
	SANY,	TANY,
	SCON,	TSCALAR,
		NBREG|NBSR,	RESC1,
#ifndef cookies
		"	mov.l	&CR,A1\n",
#else
		"	mov.l	&CR,A1\t#template 20a\n",
#endif

#else /* FORTY */

OPLTYPE,	INTBREG|INBREG,
	SANY,	TANY,
	SCON,	TSCALAR,
		NBREG|NBSR,	RESC1,
#ifndef cookies
		"	lea	CR,A1\n",
#else
		"	lea	CR,A1\t#template 20a\n",
#endif

#endif /* FORTY */





OPLTYPE,	INTBREG|INBREG,
	SANY,	TANY,
	EA,	TSCALAR,
		NBREG|NBSR,	RESC1,
#ifndef cookies
		"	movZB	AR,A1\n",
#else
		"	movZB	AR,A1\t#template 20b\n",
#endif






OPLTYPE,	INTEMP|FORCC,
	SANY,	TANY,
	EAA|EB,	TSCALAR,
		NTEMP,	RESC1|RESCC,
#	ifndef cookies
		"	movZb	AR,A1\n",
#	else
		"	movZb	AR,A1\t#template 21a\n",
#	endif


OPLTYPE,	INTEMP,
	SANY,	TANY,
	EAA|EB,	TFLOAT,
		NTEMP,	RESC1,
#	ifndef cookies
		"	movZb	AR,A1\n",
#	else
		"	movZb	AR,A1\t#template 21b\n",
#	endif






OPLTYPE,	INTEMP,
	SANY,	TANY,
	EAX,	TDOUBLE,
		2*NTEMP,	RESC1,
#ifndef cookies
		"	mov.l	UR,U1\n	mov.l	AR,A1\n",
#else
		"\tmov.l\tUR,U1\t#template 22a\n	mov.l	AR,A1\t#template 22b\n",
#endif



OPLTYPE,	INTEMP,
	SANY,			TANY,
	SNAME|SOREG,		TLONGDOUBLE,
	4*NTEMP,		RESC1,
#ifndef cookies
		"\tmov.l\tWR,W1\n\tmov.l\tVR,V1\n\tmov.l\tUR,U1\n\tmov.l\tAR,A1\n",
#else
		"\tmov.l\tWR,W1\t#template 22q\n\tmov.l\tVR,V1\t#template 22q\n\tmov.l\tUR,U1\t#template 22q\n\tmov.l\tAR,A1\t#template 22q\n",
#endif



#ifndef FORTY

OPLTYPE,	FORARG,
	SANY,	TANY,
	SBREG,	TINT|TUNSIGNED|TPOINT,
		0,	RNULL,
#ifndef cookies
		"	pea	(AR)\nZP",
#else
		"	pea	(AR)\t#template 23a\nZP#\ttemplate 23b\n",
#endif






OPLTYPE,	FORARG,
	SANY,	TANY,
	SCON,	TSCALAR,
		0,	RNULL,
#ifndef cookies
		"ZV\n",
#else
		"ZV\t#template 24a\n",
#endif

#endif /* FORTY */




OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TINT|TUNSIGNED|TPOINT|TFLOAT,
		0,	RNULL,
#ifndef cookies
		"	mov.l	AR,Z-\n",
#else
		"	mov.l	AR,Z-\t#template 24b\n",
#endif






OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TSHORT,
		NAREG|NASR,	RNULL,
#ifndef cookies
		"\tmov.w	AR,A1\n\text.l\tA1\n	mov.l	A1,Z-\n",
#else
		"\tmov.w\tAR,A1\t#template 25a\n\text.l\tA1\n\tmov.l\tA1,Z-\t#template 25b\n",
#endif






OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TUSHORT,
		NAREG,		RNULL,
#ifndef cookies
		"\tmovq\t&0,A1\n\tmov.w\tAR,A1\n\tmov.l\tA1,Z-\n",
#else
		"\tmovq\t&0,A1\t#template 26a\n\tmov.w\tAR,A1\n\tmov.l\tA1,Z-\t#template 26b\n",
#endif






OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TCHAR,
		NAREG|NASR,	RNULL,
#	ifndef cookies
		"\tmov.b\tAR,A1\n\textb.l\tA1\n\tmov.l\tA1,Z-\n",
#	else	/* cookies */
		"\tmov.b\tAR,A1\t#template 27a\n\textb.l\tA1\n\tmov.l\
		\tA1,Z-\t#template 27b\n",
#	endif	/* cookies */







OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TUCHAR,
		NAREG,		RNULL,
#ifndef cookies
		"\tmovq\t&0,A1\n\tmov.b\tAR,A1\n\tmov.l\tA1,Z-\n",
#else
		"\tmovq\t&0,A1\t#template 28a\n\tmov.b\tAR,A1\n\tmov.l\tA1,Z-\t#template 28b\n",
#endif








OPLTYPE,	FORARG,
	SANY,		TANY,
	EAX,		TDOUBLE,
		0,	RNULL,
#	ifndef cookies
		"ZG\n",
#	else
		"ZG\t#template 30\n",
#	endif





OPLTYPE,	FORARG,
	SANY,		TANY,
	SDREG|STDREG,	TFLOAT|TDOUBLE|TINT|TLONG|TUNSIGNED|TULONG|TPOINT,
		0,	RNULL,
#	ifndef cookies
		"Z9\tfpmovZB\tAR,Z-\n",
#	else
		"Z9\tfpmovZB\tAR,Z-\t#template d30a\n",
#	endif




OPLTYPE,	FORARG,
	SANY,		TANY,
	SFREG|STFREG,	TFLOAT|TDOUBLE|TINT|TLONG|TUNSIGNED|TULONG|TPOINT,
		0,	RNULL,
#	ifndef cookies
		"Z9\tfmovZB\tAR,Z-\n",
#	else
		"Z9\tfmovZB\tAR,Z-\t#template 30a\n",
#	endif




OPLTYPE,        FORARG,
        SANY,                   TANY,
        SNAME|SOREG,            TLONGDOUBLE,
        0,                      RNULL,
#ifndef cookies
		"\tmov.l\tWR,Z-\n\tmov.l\tVR,Z-\n\tmov.l\tUR,Z-\n\tmov.l\tAR,Z-\n",
#else
		"\tmov.l\tWR,Z-\t#template 30q\n\tmov.l\tVR,Z-\t#template 30q\n\tmov.l\tUR,Z-\t#template 30q\n\tmov.l\tAR,Z-\t#template 30q\n",
#endif



OPLTYPE,	INTEMP,
	SANY,	TANY,
	SDREG|STDREG,	TFLOAT|TDOUBLE,
		2*NTEMP,	RESC1,
#	ifndef cookies
		"\tfpmovZB\tAR,A1\n",
#	else
		"\tfpmovZB\tAR,A1\t#\t cookie d30b\n",
#	endif


OPLTYPE,	INTEMP,
	SANY,	TANY,
	SFREG|STFREG,	TFLOAT|TDOUBLE,
		2*NTEMP,	RESC1,
#	ifndef cookies
		"\tfmovZB\tAR,A1\n",
#	else
		"\tfmovZB\tAR,A1\t#\t cookie 30b\n",
#	endif



/* This was faster on the 68030, is slower on the 68040 */

#if 0

OPLOG,	FORCC,
	SAREG|STAREG|SBREG|STBREG,	TSCALAR,
	SCCON,	TSCALAR,
		NAREG,	RESCC,
#ifndef cookies
		"	movq	AR,A1\n\tcmpZL	AL,A1\nK",
#else
		"\tmovq\tAR,A1\t#template 31a\n\tcmpZL	AL,A1\nK#\ttemplate 31b\n",
#endif
#endif






OPLOG,	FORCC,
	SAREG|STAREG|SBREG|STBREG,	TSCALAR,
	EA,	TSCALAR,
		0,	RESCC,
#ifndef cookies
		"	cmpZL	AL,AR\nK",
#else
		"	cmpZL	AL,AR\t#template 31c\nK#\ttemplate 31d\n",
#endif






OPLOG,	FORCC,
	SNAME|SOREG|STARREG|SAREG|SBREG,	TSCALAR,
	SCON,	TSCALAR,
		0,	RESCC,
#ifndef cookies
		"	cmpZL	AL,AR\nK",
#else
		"	cmpZL	AL,AR\t#template 32a\nK#\t\t\ttemplate 32b\n",
#endif





/* the following template is due to a rewrite in cbranch() */
/* this template is not strictly necessary since a rewrite will force the
   operands into registers anyway but it does ensure that the optimum code
   sequence is generated. Otherwise the rhs could be forced into a register
   unnecessarily.
   THIS IS NO LONGER TRUE FOR THE 68040, THAT IS WHY IT IS IFDEF'ED OUT
*/
#if 0
OPLOG,	FORCC,
	SCCON,	TSCALAR,
	EA,	TSCALAR,	/* SCON-SCCON cmps would be folded earlier */
		NAREG,	RESCC,
#ifndef cookies
		"\tmovq\tAL,A1\n\tcmpZL	A1,AR\nK",
#else
		"\tmovq\tAL,A1\t#template 32c\n\tcmpZL	A1,AR\nK#\t\t\ttemplate 32d\n",
#endif
#endif




OPLOG,	FORCC,
	SDREG|STDREG,	TFLOAT|TDOUBLE,
	SDREG|STDREG,	TFLOAT|TDOUBLE,
		0,	RESCC,
#	ifndef cookies
		"	fpcmpZR	AL,AR\nG",
#	else
		"	fpcmpZR	AL,AR\t#template d32e\nG#\t\t\ttemplate d32f\n",
#	endif


OPLOG,	FORCC,
	SFREG|STFREG,		TSCALAR|TFLOAT|TDOUBLE,
	SFREG|STFREG|EA,	TSCALAR|TFLOAT|TDOUBLE,
		0,	RESCC,
#	ifndef cookies
		"	fcmpZR	AL,AR\nG",
#	else
		"	fcmpZR	AL,AR\t#template 32e\nG#\t\t\ttemplate 32f\n",
#	endif








CCODES,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SANY,	TANY,
		NAREG,	RESC1|RESCC,
#ifndef cookies
		"	movq	&1,A1\nZN",
#else
		"	movq	&1,A1\t#template 33a\nZN#\ttemplate 33b\n",
#endif






UNARY MINUS,	INTAREG|INAREG,
	STAREG,	TSCALAR, /* must be STAREG. Otherwise reg vars and mem would
				be altered incrorrectly. e.g. x = -y would be
				neg.l _y
				mov.l _y,_x
			*/
	SANY,	TANY,
		0,	RLEFT,
#	ifndef cookies
		"	negZB	AL\n",
#	else
		"	negZB	AL\t#template 34\n",
#	endif



UNARY MINUS,	INTAREG,
	EA,	TSCALAR,
	SANY,	TANY,
		NAREG,	RESC1,
#	ifndef cookies
		"	movZB	AL,A1\n	negZB	A1\n",
#	else
		"	movZB	AL,A1\n	negZB	A1\t#template 34s\n",
#	endif




UNARY MINUS,	INTDREG|FORCC,
	STDREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfpnegZL\tAL\n",
#	else
		"\tfpnegZL\tAL\t#template d34b\n",
#	endif



UNARY MINUS,	INTFREG|FORCC,
	STFREG,	TSCALAR|TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfnegZL\tAL\n",
#	else
		"\tfnegZL\tAL\t#template 34b\n",
#	endif


UNARY MINUS,	INTDREG|FORCC,
	SDREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		NDREG,	RESC1|RESCC,
#	ifndef cookies
		"\tfpmovZL\tAL,A1\n\tfpnegZL\tA1\n",
#	else
		"\tfpmovZL\tAL,A1\t#template d34g\n\tfpnegZL\tA1\t#template d34h\n",
#	endif



UNARY MINUS,	INTFREG|FORCC,
	SFREG,	TSCALAR|TFLOAT|TDOUBLE,
	SANY,	TANY,
		NFREG,	RESC1|RESCC,
#	ifndef cookies
		"\tfnegZL\tAL,A1\n",
#	else
		"\tfnegZL\tAL,A1\t#template 34g\n",
#	endif




UNARY MINUS,	INTAREG|FORCC,
	STDREG,	TFLOAT,
	SANY,	TANY,
		NAREG,	RESC1|RLEFT|RESCC,
#	ifndef cookies
		"\tfpnegZL\tAL\n\tfpmov.s\tAL,A1\n",
#	else
		"\tfpnegZL\tAL\t#template d34c\n\tfpmov.s\tAL,A1\t#template d34d\n",
#	endif




UNARY MINUS,	INTAREG|FORCC,
	STFREG,	TFLOAT,
	SANY,	TANY,
		NAREG,	RESC1|RLEFT|RESCC,
#	ifndef cookies
		"\tfnegZL\tAL\n\tfmov.s\tAL,A1\n",
#	else
		"\tfnegZL\tAL\t#template 34c\n\tfmov.s\tAL,A1\t#template 34d\n",
#	endif





UNARY MINUS,	INTAREG|FORCC,
	STDREG,	TDOUBLE,
	SANY,	TANY,
		2*NAREG,	RESC1|RLEFT|RESCC,
#	ifndef cookies
		"\tfpneg.d\tAL\n\tfpmov.d\tAL,A1:U1\n",
#	else
		"\tfpneg.d\tAL\t#template d34e\n\tfpmov.d\tAL,A1:U1\t#template d34f\n",
#	endif




UNARY MINUS,	INTAREG|FORCC,
	STFREG,	TDOUBLE,
	SANY,	TANY,
		2*NTEMP|NAREG,	RESC1|RLEFT|RESCC,
#	ifndef cookies
		"\tfneg.x\tAL\n\tfmov.d\tAL,A2\n\tmov.l\tA2,A1\n\tmov.l\tU2,U1\n",
#	else
		"\tfneg.x\tAL\t#template 34e\n\tfmov.d\tAL,A2\n\tmov.l\tA2,A1\n\
	mov.l\tU2,U1\t#template 34f\n",
#	endif






UNARY MINUS,	INTEMP|QFORCC,
	EA,		TSCALAR,
	SANY,		TANY,
		  NTEMP,		RESC1|QRESCC,
#ifndef cookies
		"Z2",
#else
		"Z2#\ttemplate 34c\n",
#endif





UNARY MINUS,	INTEMP,
	EA,		TFLOAT,
	SANY,		TANY,
		NTEMP,			RESC1,
#ifndef cookies
		"Z2",
#else
		"Z2#\ttemplate 34d\n",
#endif







UNARY MINUS,	INTEMP,
	EAX,		TDOUBLE,
	SANY,		TANY,
		2*NTEMP,	RESC1,
#ifndef cookies
		"Z2",
#else
		"Z2#\ttemplate 34e\n",
#endif







COMPL,	INTAREG|INAREG,
	STAREG,	TSCALAR, /* same reasoning as for template 34 */
	SANY,	TANY,
		0,	RLEFT,
#ifndef cookies
		"	notZB	AL\n",
#else
		"	notZB	AL\t#template 35\n",
#endif






INCR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		NAREG,	RESC1,
#ifndef cookies
		"F\tmovZB\tAL,A1\n\taddqZB\tAR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 36a\n\taddqZB\tAR,AL\t#template 36b\n",
#endif






DECR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		NAREG,	RESC1,
#ifndef cookies
		"F\tmovZB\tAL,A1\n\tsubqZB\tAR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 37a\n\tsubqZB\tAR,AL\t#template 37b\n",
#endif






INCR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		NAREG,	RESC1,
#ifndef cookies
		"F\tmovZB\tAL,A1\n\taddZB\tAR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 38a\n\taddZB	AR,AL\t#template 38b\n",
#endif






DECR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		NAREG,	RESC1,
#ifndef cookies
		"F\tmovZB\tAL,A1\n\tsubZB\tAR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 39a\n\tsubZB	AR,AL\t#template 39b\n",
#endif






INCR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		NBREG,	RESC1,
#ifndef cookies
		"F	movZB	AL,A1\n	addq.w	AR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 40a\n	addq.w	AR,AL\t#template 40b\n",
#endif






DECR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		NBREG,	RESC1,
#ifndef cookies
		"F	movZB	AL,A1\n	subq.w	AR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 41a\n	subq.w	AR,AL\t#template 41b\n",
#endif






/* The following two templates use sub/add.l . The suffix could be .w 
except that short constants such as 0x8000 get sign extended to long, thereby
reversing the sense of the operation.
*/

INCR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	SCON,	TSCALAR,
		NBREG,	RESC1,
#ifndef cookies
		"F	movZB	AL,A1\n	add.l	AR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 42a\n	add.l	AR,AL\t#template 42b\n",
#endif






DECR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	SCON,	TSCALAR,
		NBREG,	RESC1,
#ifndef cookies
		"F	movZB	AL,A1\n	sub.l	AR,AL\n",
#else
		"F\tmovZB\tAL,A1\t#template 43a\n	sub.l	AR,AL\t#template 43b\n",
#endif




#ifndef FORTY

PLUS,		INBREG|INTBREG,
	SBREG,	TPOINT,
	SICON,	TANY,
		NBREG|NBSL,	RESC1,
#ifndef cookies
		"	lea	ZO(AL),A1\n",
#else
		"	lea	ZO(AL),A1\t#template 44\n",
#endif

#endif /* FORTY */




PLUS,		FORARG,
	SBREG,	TPOINT,
	SICON,	TANY,
		0,	RNULL,
#ifndef cookies
		"	pea	ZO(AL)\nZP",
#else
		"	pea	ZO(AL)\t#template 45a\nZP#\ttemplate 45b\n",
#endif




#ifndef FORTY

MINUS,		INBREG|INTBREG,
	SBREG,	TPOINT,
	SICON,	TANY,
		NBREG|NBSL,	RESC1,
#ifndef cookies
		"	lea	ZM(AL),A1\n",
#else
		"	lea	ZM(AL),A1\t#template 46\n",
#endif

#endif /* FORTY */




MINUS,		FORARG,
	SBREG,	TPOINT,
	SICON,	TANY,
		0,	RNULL,
#ifndef cookies
		"	pea	ZM(AL)\nZP",
#else
		"	pea	ZM(AL)\t#template 47a\nZP#\ttemplate 47b\n",
#endif




/* New opltype for LEA operation, must be after PLUS/MINUS to work */

OPLTYPE,	INTBREG|INBREG|INAREG|INTAREG,
    SANY,		TANY,
    SADRREG,	TPOINT,
        NBREG|NBSR, RESC1,
#ifndef cookies
        "	lea	AR,A1\n",
#else
        "	lea	AR,A1\t#template 47y\n",
#endif


OPLTYPE,	FORARG,
	SANY,		TANY,
	SADRREG,	TPOINT,
	NBREG|NBSR, RNULL,
#ifndef cookies
	"	pea	AR\n",
#else
	"	pea	AR\t#template 47z\n",
#endif




ASG PLUS,	INAREG|INTAREG|QFORCC,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"	addqZB	AR,AL\n",
#else
		"	addqZB	AR,AL\t#template 48\n",
#endif






ASG PLUS,	INBREG|INTBREG,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	addq.w	AR,AL\n",
#else
		"	addq.w	AR,AL\t#template 49\n",
#endif






ASG PLUS,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TSCALAR,
	EA,	TSCALAR,
		0,	RLEFT|QRESCC,
#	ifndef cookies
		"	addZB	AR,AL\n",
#	else
		"	addZB	AR,AL\t#template 50\n",
#	endif










ASG PLUS,	INDREG|INTDREG|FORCC,
	SDREG|STDREG,			TFLOAT|TDOUBLE,
	SDREG|STDREG,			TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"	fpaddZR	AR,AL\n",
#	else
		"	fpaddZR	AR,AL\t#template d50a\n",
#	endif



ASG PLUS,	INFREG|INTFREG|FORCC,
	SFREG|STFREG,			TSCALAR|TFLOAT|TDOUBLE,
	EAA|SFREG|STFREG,			TSCALAR|TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"	Z5addZR	AR,AL\n",
#	else
		"	Z5addZR	AR,AL\t#template 50a\n",
#	endif









ASG PLUS,	INBREG|INTBREG,
	SBREG|STBREG,	TSCALAR,
	EA,	TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	addZB	AR,AL\n",
#else
		"	addZB	AR,AL\t#template 51\n",
#endif






ASG MINUS,	INAREG|INTAREG|QFORCC,
	EAA|STAREG,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"	subqZB	AR,AL\n",
#else
		"	subqZB	AR,AL\t#template 53\n",
#endif





ASG MINUS,	INBREG|INTBREG,
	EB|STBREG,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	subq.w	AR,AL\n",
#else
		"	subq.w	AR,AL\t#template 54\n",
#endif






ASG MINUS,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TSCALAR,
	EA,			TSCALAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"	subZB	AR,AL\n",
#else
		"	subZB	AR,AL\t#template 55\n",
#endif









ASG MINUS,	INDREG|INTDREG|FORCC,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"	fpsubZR	AR,AL\n",
#	else
		"	fpsubZR	AR,AL\t#template d55a\n",
#	endif


ASG MINUS,	INFREG|INTFREG|FORCC,
	SFREG|STFREG,		TSCALAR|TFLOAT|TDOUBLE,
	EAA|SFREG|STFREG,	TSCALAR|TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"	fsubZR	AR,AL\n",
#	else
		"	fsubZR	AR,AL\t#template 55a\n",
#	endif









ASG MINUS,	INBREG|INTBREG,
	SBREG|STBREG,	TSCALAR,
	EA,			TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	subZB	AR,AL\n",
#else
		"	subZB	AR,AL\t#template 56\n",
#endif






ASG ER, 	INAREG|INTAREG|FORCC,
	EAA,	TSCALAR,
	SCON|SAREG|STAREG,	TSCALAR,
		0,	RLEFT|RESCC,
#ifndef cookies
		"	eorZB	AR,AL\n",
#else
		"	eorZB	AR,AL\t#template 58\n",
#endif






ASG OPSIMP, 	INAREG|QFORCC,
	SAREG|STAREG,	TSCALAR,
	EAA,	TSCALAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"	OIZB	AR,AL\n",
#else
		"	OIZB	AR,AL\t#template 60\n",
#endif






ASG OPSIMP, 	INAREG|QFORCC,
	EAA,	TSCALAR,
	SCON|SAREG|STAREG,	TSCALAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"	OIZB	AR,AL\n",
#else
		"	OIZB	AR,AL\t#template 61\n",
#endif










#ifdef FORT

ASG MUL,	INAREG|INTAREG|FORCC,
	SAREG|STAREG,	TSHORT,
	STARNM,	TSHORT,
		NBREG,	RLEFT|RESCC,
#	ifndef cookies
		"\tmov.l	AR,A1\n\tmuls.w\t(A1),AL\n",
#	else
		"\tmov.l	AR,A1\t#template 62a\n\tmuls.w\t(A1),AL\t#template 62b\n",
#	endif

#endif /* FORT */







ASG MUL,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TINT|TUNSIGNED|TPOINT|TLONG|TULONG,
	SCON,		TINT|TUNSIGNED|TPOINT|TLONG|TULONG,
	NAREG,          RLEFT|QRESCC,
#	ifndef cookies
		"Zm",
#	else
		"Zm\t#template 64b\n",
#	endif

ASG MUL,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TINT|TUNSIGNED|TPOINT|TLONG|TULONG,
	SCON,		TINT|TUNSIGNED|TPOINT|TLONG|TULONG,
	NBREG,          RLEFT|QRESCC,
#	ifndef cookies
		"Zm",
#	else
		"Zm\t#template 64c\n",
#	endif

ASG MUL,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TINT|TUNSIGNED|TPOINT|TLONG|TULONG|TSHORT|TUSHORT,
	EAA,			TINT|TUNSIGNED|TPOINT|TLONG|TULONG|TSHORT|TUSHORT,
		0,	RLEFT|QRESCC,
#	ifndef cookies
		"\tmulQZR	AR,AL\n",
#	else
		"\tmulQZR	AR,AL\t#template 64\n",
#	endif






ASG MUL,	INDREG|INTDREG|FORCC,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfpmulZR	AR,AL\n",
#	else
		"\tfpmulZR	AR,AL\t#template d64a\n",
#	endif


ASG MUL,	INFREG|INTFREG|FORCC,
	SFREG|STFREG,		TFLOAT|TDOUBLE|TSCALAR,
	EAA|SFREG|STFREG,		TFLOAT|TDOUBLE|TSCALAR,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfZcmulQZR	AR,AL\n",
#	else
		"\tfZcmulQZR	AR,AL\t#template 64a\n",
#	endif









ASG MUL,	INAREG|FORCC,
	SAREG|STAREG,	TUCHAR|TCHAR,
	SAREG|STAREG,	TUCHAR|TCHAR,
		0,	RLEFT|RESCC,
#ifndef cookies
		"Z3L\nZ3R\n\tmulQ.w\tAR,AL\n",
#else
		"Z3L\t#template 67a\nZ3R\n\tmulQ.w\tAR,AL#template 67b\n",
#endif






ASG MUL,	INAREG|FORCC,
	SAREG|STAREG,	TCHAR,
	EAA,			TCHAR,
		NAREG,	RLEFT|RESCC,
#ifndef cookies
		"\text.w\tAL\n\tmov.b\tAR,A1\n\text.w\tA1\n\tmuls.w\tA1,AL\n",
#else
		"\text.w\tAL\t#template 66a\n\tmov.b\tAR,A1\n\text.w\tA1\n\
\tmuls.w	A1,AL\t#template 66b\n",
#endif






ASG MUL,	INAREG|FORCC,
	SAREG|STAREG,	TUCHAR|TCHAR,
	SAREG|STAREG,	TUCHAR|TCHAR,
		0,	RLEFT|RESCC,
#ifndef cookies
		"\tand.w\t&0xff,AL\n\tand.w\t&0xff,AR\n\tmulu.w\tAR,AL\n",
#else
		"\tand.w\t&0xff,AL\t#template 67a\n\
\tand.w\t&0xff,AR\n\tmulu.w\t#template 67b\n",
#endif






ASG MUL,	INAREG|FORCC,
	SAREG|STAREG,	TUCHAR|TCHAR,
	EAA,			TUCHAR|TCHAR,
		NAREG,	RLEFT|RESCC,
#ifndef cookies
		"\tand.w\t&0xff,AL\n\tmovq\t&0,A1\n\tmov.b\tAR,A1\n\
\tmulu.w\tA1,AL\n",
#else
		"\tand.w\t&0xff,AL\t#template 67c\n\tmovq\t&0,A1\n\
\tmov.b\tAR,A1\n\tmulu.w	A1,AL\t#template 67d\n",
#endif











/* like ASG DIV except we know it's a division by 1 */
ASG DIVP2,	INTAREG|INAREG|QFORCC,
	SAREG|STAREG,	TINT|TLONG|TSHORT|TCHAR|TPOINT,
	SONE,		TANY,
		NAREG|NFSL,	RLEFT|QRESCC,
#   ifndef cookies
        "ZH1",
#   else
        "ZH1\t#template 67la\n",
#   endif


/* like ASG DIV except we know it's a power of 2 divide ( <= 8 ). */
ASG DIVP2,	INTAREG|INAREG|QFORCC,
	SAREG|STAREG,	TINT|TLONG|TSHORT|TCHAR|TPOINT,
	S8CON,		TANY,
		NAREG|NFSL,	RLEFT|QRESCC,
#   ifndef cookies
        "ZH8",
#   else
        "ZH8\t#template 67lb\n",
#   endif
			


/* like ASG DIV except we know it's a power of 2 divide. */
ASG DIVP2,	INTAREG|INAREG|QFORCC,
	SAREG|STAREG,	TINT|TLONG|TSHORT|TCHAR|TPOINT,
	SCON,		TANY,
		NAREG|NFSL,	RLEFT|QRESCC,
#	ifndef cookies
		"ZH0",
#	else
		"ZH0\t#template 67l\n",
#	endif






ASG DIV,	INAREG|QFORCC,
	SAREG|STAREG,	TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
	EAA,		TSHORT|TUSHORT|TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
		0,	RLEFT|QRESCC,
#	ifndef cookies
		"\tdivQZR	AR,AL\n",
#	else
		"\tdivQZR	AR,AL\t#template 68\n",
#	endif




#ifdef FORT

ASG DIV,	INAREG|INTAREG|QFORCC,
	SAREG|STAREG,	TSHORT,
	STARNM,		TSHORT,
		NBREG,	RLEFT|QRESCC,
#	ifndef cookies
		"ZeL\tmov.l\tAR,A1\n\tdivs.w	(A1),AL\n",
#	else
		"ZeL\tmov.l\tAR,A1\t#template 67j\n\tdivs.w	(A1),AL\n\
\t#template 67k\n",
#	endif

#endif /* FORT */





ASG DIV,	INAREG|QFORCC,
	SAREG|STAREG,	TCHAR|TSHORT,
	EAA,			TSHORT,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"ZeL	divs.w	AR,AL\n",
#else
		"ZeL#\ttemplate 68a\n	divs.w	AR,AL\t#template 68b\n",
#endif





/* template 68 should precede template 69 to preclude short/short */
ASG DIV,	INAREG|QFORCC,
	SAREG|STAREG,	TUSHORT|TSHORT,
	EAA,			TUSHORT|TSHORT,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"\tand.l\t&0xffff,AL\n	divu.w	AR,AL\n",
#else
		"\tand.l\t&0xffff,AL\t#template 69a\n\
\tdivu.w	AR,AL\t#template 69b\n",
#endif






ASG DIV,	INAREG|FOREFF|QFORCC,
	SAREG|STAREG,	TCHAR|TSHORT,
	SAREG|STAREG,	TCHAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"ZeL\text.w\tAR\n\tdivs.w\tAR,AL\n",
#else
		"ZeL#\ttemplate 70a\n\text.w\tAR\n\tdivs.w\tAR,AL\t#template 70b\n",
#endif








ASG DIV,	INAREG|FOREFF|QFORCC,
	SAREG|STAREG,	TUCHAR|TCHAR,
	SAREG|STAREG,	TUCHAR|TCHAR,
		0,	RLEFT|QRESCC,
#ifndef cookies
		"\tand.l\t&0xff,AL\n\tand.w\t&0xff,AR\n\tdivu.w\tAR,AL\n",
#else
		"\tand.l\t&0xff,AL\t#template 70c\n\tand.w\t&0xff,AR\n\tdivu.w\tAR,AL\t#template 70d\n",
#endif








ASG DIV,	INAREG|QFORCC,
	SAREG|STAREG,	TCHAR|TSHORT,
	EAA,			TCHAR,
		NAREG,	RLEFT|QRESCC,
#ifndef cookies
		"ZeL\tmov.b\tAR,A1\n\text.w\tA1\n\tdivs.w\tA1,AL\n",
#else
		"ZeL#\ttemplate 71a\n\tmov.b\tAR,A1\n\text.w\tA1\n\tdivs.w\tA1,AL\t#template 71b\n",
#endif








ASG DIV,	INAREG|FOREFF|QFORCC,
	SAREG|STAREG,	TUCHAR|TCHAR,
	EAA,			TUCHAR|TCHAR,
		NAREG,	RLEFT|QRESCC,
#ifndef cookies
		"\tand.l\t&0xff,AL\n\tmovq\t&0,A1\n\tmov.b\tAR,A1\n\
\tdivs.w\tA1,AL\n",
#else
		"\tand.l\t&0xff,AL\t#template 72a\n\tmovq\t&0,A1\n\tmov.b\tAR,A1\n\
\tdivs.w	A1,AL\t#template 72b\n",
#endif









ASG DIV,	INDREG|INTDREG|FORCC,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
	SDREG|STDREG,		TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfpdivZR	AR,AL\n",
#	else
		"\tfpdivZR	AR,AL\t#template d73\n",
#	endif


ASG DIV,	INFREG|INTFREG|FORCC,
	SFREG|STFREG,	TSCALAR|TFLOAT|TDOUBLE,
	EAA|SFREG|STFREG,	TSCALAR|TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
#	ifndef cookies
		"\tfZcdivQZR	AR,AL\n",
#	else
		"\tfZcdivQZR	AR,AL\t#template 73\n",
#	endif





ASG MOD,	INAREG|INTAREG|FOREFF,
	STAREG,		TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
	STAREG,		TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
		0,	RRIGHT,
# 	ifndef cookies
		"\tdivQl.l\tAR,AR:AL\n",
#	else
		"\tdivQl.l\tAR,AR:AL\t# template 73a\n",
#	endif


ASG MOD,	INAREG|INTAREG|FOREFF,
	STAREG,	TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
	EAA,	TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
		NAREG,	RESC1,
# 	ifndef cookies
		"\tdivQl.l\tAR,A1:AL\n",
#	else
		"\tdivQl.l\tAR,A1:AL\n\t# template 73b\n",
#	endif


ASG MOD,	INAREG|INTAREG|FOREFF,
	SAREG,	TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
	EAA,	TINT|TPOINT|TLONG|TUNSIGNED|TULONG,
		NAREG,	RLEFT,
# 	ifndef cookies
		"\tdivQl.l\tAR,A1:AL\n\texg\tAL,A1\n",
#	else
		"\tdivQl.l\tAR,A1:AL\n\texg\tAL,A1\n\t# template 73c\n",
#	endif



ASG MOD,	INAREG|FOREFF,
	SAREG|STAREG,	TCHAR|TSHORT,
	EAA,		TSHORT,
		0,	RLEFT,
#ifndef cookies
		"ZeL\tdivs.w	AR,AL\n	swap	AL\n",
#else
		"ZeL#\ttemplate 73c\n\tdivs.w\tAR,AL\n\tswap\tAL\t#template 73c\n",
#endif


ASG MOD,	INAREG|INTAREG,
	SAREG|STAREG,	TSHORT,
	STARNM,		TSHORT,
		NBREG,	RLEFT,
#ifndef cookies
		"ZeL\tmov.l\tAR,A1\n\tdivs.w	(A1),AL\n	swap	AL\n",
#else
		"ZeL#\ttemplate 73d\n\tmov.l\tAR,A1\n\tdivs.w\t(A1),AL\n\tswap\tAL\t#template 73d\n",
#endif



ASG MOD,	INAREG,
	SAREG|STAREG,	TUSHORT,
	EAA,			TUSHORT|TSHORT,
		0,	RLEFT,
#ifndef cookies
		"\tand.l\t&0xffff,AL\n\tdivu.w\tAR,AL\n\tswap\tAL\n",
#else
		"\tand.l\t&0xffff,AL\t#template 74a\n\tdivu.w\tAR,AL\n\tswap\tAL\t#template 74b\n",
#endif






ASG MOD,	INAREG,
	SAREG|STAREG,	TSHORT,
	EAA,			TUSHORT,
		0,	RLEFT,
#ifndef cookies
		"	and.l\t&0xffff,AL\n\tdivu.w\tAR,AL\n\tswap\tAL\n",
#else
		"\tand.l\t&0xffff,AL\t#template 75a\n\tdivu.w\tAR,AL\n\tswap\tAL\
\t#template 75b\n",
#endif





ASG MOD,	INAREG|FOREFF,
	SAREG|STAREG,	TCHAR|TSHORT,
	SAREG|STAREG,	TCHAR,
		0,	RLEFT,
#ifndef cookies
		"ZeL\text.w\tAR\n\tdivs.w\tAR,AL\n\tswap\tAL\n",
#else
		"ZeL#\ttemplate 76a\n\text.w\tAR\n\tdivs.w\tAR,AL\n\tswap\tAL\t#template 76b\n",
#endif






ASG MOD,	INAREG|FOREFF,
	SAREG|STAREG,	TCHAR|TSHORT,
	EAA,			TCHAR,
		NAREG,	RLEFT,
#ifndef cookies
		"ZeL\tmov.b	AR,A1\n\text.w	A1\n\tdivs.w\tA1,AL\n\tswap\tAL\n",
#else
		"ZeL#\ttemplate 76c\n\tmov.b\tAR,A1\n\
\text.w	A1\n\tdivs.w\tA1,AL\n\tswap\tAL\t#template 76d\n",
#endif






/* template 76 should precede template 77 to preclude char mod char */
ASG MOD,	INAREG,
	SAREG|STAREG,	TUCHAR|TCHAR,
	SAREG|STAREG,	TUCHAR|TCHAR,
		0,	RLEFT,
#ifndef cookies
		"\tand.l\t&0xff,AL\n\tand.w\t&0xff,AR\n\tdivs.w\tAR,AL\n\tswap\tAL\n",
#else
		"\tand.l\t&0xff,AL\t#template 77a\n\tand.w\t&0xff,AR\n\
\tdivs.w\tAR,AL\n\tswap\tAL\t#template 77b\n",
#endif





ASG MOD,	INAREG,
	SAREG|STAREG,	TUCHAR|TCHAR,
	EAA,			TUCHAR|TCHAR,
		NAREG,	RLEFT,
#ifndef cookies
		"\tand.l\t&0xff,AL\n\tmovq\t&0,A1\n\tmov.b	AR,A1\n\
\tdivs.w	A1,AL\n	swap	AL\n",
#else
		"\tand.l\t&0xff,AL\t#template 77c\n\tmovq\t&0,A1\n\tmov.b\tAR,A1\n\
\tdivs.w\tA1,AL\n\tswap\tAL\t#template 77d\n",
#endif





ASG OPSHFT, 	INAREG,
	SOREG|STARREG,	TSHORT|TUSHORT,
		/*it doesn't seem that STARREG can be seen here somehow*/
	SONE,				TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	POI.w	AL\n",
#else
		"	POI.w	AL\t#template 80b\n",
#endif







ASG OPSHFT, 	INAREG,
	SAREG,		TSCALAR,
	S8CON|SAREG,	TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	POIZB	AR,AL\n",
#else
		"	POIZB	AR,AL\t#template 81b\n",
#endif





ASG OPSHFT,	INBREG|INTBREG,
	SBREG|STBREG,		TSCALAR,
	S8CON|SAREG|STAREG,	TSCALAR,
		NAREG,	RLEFT,
#ifndef cookies
		"	movZB	AL,A1\n	POIZB	AR,A1\n	movZB	A1,AL\n",
#else
		"\tmovZB\tAL,A1\t#cookie82c\n\tPOIZB\tAR,A1\n\tmovZB\tA1,AL\t#cookie82d\n",
#endif





ASG OPSHFT, 	INAREG,
	SOREG|STARREG,	TSHORT|TUSHORT,
		/*it doesn't seem that STARREG can be seen here somehow*/
	SONE,				TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	JOI.w	AL\n",
#else
		"	JOI.w	AL\t#template 80ax\n",
#endif




ASG OPSHFT, 	INAREG|FORCC,
	SOREG|STARREG,	TSHORT|TUSHORT,
		/*it doesn't seem that STARREG can be seen here somehow*/
	SONE,				TSCALAR,
		0,	RLEFT|RESCC,
#ifndef cookies
		"	JOI.w	AL\n\ttst.w\tAL\n",
#else
		"	JOI.w	AL\n\ttst.w\tAL\t#template 80ay\n",
#endif



ASG OPSHFT, 	INAREG,
	SAREG,		TSCALAR,
	S8CON|SAREG,	TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"	JOIZB	AR,AL\n",
#else
		"	JOIZB	AR,AL\t#template 81ax\n",
#endif



ASG OPSHFT, 	INAREG|FORCC,
	SAREG,		TSCALAR,
	S8CON|SAREG,	TSCALAR,
		0,	RLEFT|RESCC,
#ifndef cookies
		"	JOIZB	AR,AL\n\ttstZB\tAL\n",
#else
		"	JOIZB	AR,AL\n\ttstZB\tAL\t#template 81ay\n",
#endif



ASG OPSHFT,	INBREG|INTBREG|FORCC,
	SBREG|STBREG,		TSCALAR,
	S8CON|SAREG|STAREG,	TSCALAR,
		NAREG,	RLEFT|RESCC,
#ifndef cookies
		"	movZB	AL,A1\n	JOIZB	AR,A1\n	movZB	A1,AL\n",
#else
		"\tmovZB\tAL,A1\t#cookie82a\n\tJOIZB\tAR,A1\n\tmovZB\tA1,AL\t#cookie82b\n",
#endif



UNARY CALL,	INTAREG,
	SBREG|SNAME|SOREG|SCON,	TANY,
	SANY,				TANY,
		NAREG|NASL,	RESC1, /* should be register 0 */
#ifndef cookies
		"ZC\n",
#else
		"ZC\t#template 84\n",
#endif






UNARY STCALL,	INTAREG,
	SBREG|SNAME|SOREG|SCON,	TANY,
	SANY,				TANY,
		NAREG|NASL,	RESC1, /* should be register 0 */
#ifndef cookies
		"ZC\n",
#else
		"ZC\t#template 84a\n",
#endif






/* Note : SCONV templates, being a UTYPE, have only 1 child (in the left node).
	  The right node line below is what we are attempting to make the 
	  conversion TO.
*/

/* Note : Avoid SCONV templates that attempt to work INAREG because they can
	  munge register variables. Templates that goto INAREG are ok if they
	  don't generate any code that modifies the source register.

	  The only cases, I think, where this restriction is unnecessary is
	  when the conversion goes to a type at least as long as the root.
	  e.g. CHAR->INT.
*/

SCONV,	INTAREG,
        STAREG, 	TINT|TUNSIGNED|TPOINT,
	SANY,		TINT|TUNSIGNED|TPOINT|TSHORT|TCHAR,
		0,	RLEFT,
#ifndef cookies
		"",
#else
		"#\ttemplate 84b\n",

#endif






SCONV,	INTAREG,
	STAREG,	TSCALAR,
	SANY,		TUCHAR|TUSHORT,
		0,	RLEFT,
#ifndef cookies
		"Z1\n",
#else
		"Z1\t#\ttemplate 85\n",
#endif






SCONV,	INAREG|INTAREG|FORCC,
	SAREG|STAREG,	TCHAR|TSHORT,
	SANY,		TINT|TUNSIGNED|TPOINT|TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
#ifndef cookies
		"ZEl",
#else
		"ZEl#\ttemplate 87\n",
#endif






SCONV,	INTAREG,
	STAREG,	TUCHAR|TUSHORT,
	SANY,		TSCALAR,
		0,	RLEFT,
#ifndef cookies
		"Z1\n",
#else
		"Z1\t#\ttemplate 90\n",
#endif





SCONV,	INTAREG,
	SNAME|SOREG|STAREG,	TINT|TUNSIGNED|TPOINT|TSHORT|TUSHORT,
	SANY,			TSHORT|TUSHORT|TCHAR|TUCHAR,
		0,	RLEFT,
#ifndef cookies
		"ZT",
#else
		"ZT#\ttemplate 91a\n",
#endif






SCONV,	INAREG|INTAREG,
	EA,	TCHAR|TUCHAR,
	SANY,	TCHAR|TUCHAR,
		0,	RLEFT,
#ifndef cookies
		"",
#else
		"#\ttemplate 91b\n",
#endif






SCONV,	INAREG,
	EAA,	TINT|TUNSIGNED|TLONG|TULONG|TPOINT,
	SANY,	TINT|TUNSIGNED|TLONG|TULONG|TPOINT,
		0,	RLEFT,
#ifndef cookies
		"",
#else
		"#\ttemplate 91c\n",
#endif




/* the following template should follow 91c above to prevent excess code
   for short->short, long->long, etc.
*/







SCONV,	INAREG|INTAREG|FORCC,
	EA,	TSHORT|TCHAR,
	SANY,	TUNSIGNED|TPOINT|TUSHORT,	/* should TPOINT be here? */
		NAREG,	RESC1|RESCC,
#ifndef cookies
		"\tmovZL	AL,A1\nZE1",
#else
		"\tmovZL\tAL,A1\t#template 91d\nZE1#\ttemplate 91e\n",
#endif






SCONV,	INAREG|INTAREG,
	EA,	TUSHORT|TUCHAR,
	SANY,	TINT|TLONG|TSHORT|TUNSIGNED|TPOINT|TUSHORT,
		NAREG,	RESC1|RESCC,
#ifndef cookies
		"\tmovq	&0,A1\n	movZL	AL,A1\n",
#else
		"\tmovq\t&0,A1\t#template 91f\n\tmovZL\tAL,A1\t#template 91g\n",
#endif





SCONV,	INTDREG,	/* double to float (from a register) */
	SAREG|STAREG|SBREG|STBREG,	TDOUBLE,
	SANY,				TFLOAT,
		NDREG,	RESC1,
#	ifndef cookies
		"\tfpmov.d\tAL:UL,A1\n\tfpcvs.d\tA1\n",
#	else
		"\tfpmov.d\tAL:UL,A1\t#template d92g\n\tfpcvs.d\tA1#template d92h\n",
#	endif






SCONV,	INTDREG,	/* double to float (within registers) */
	STDREG,	TDOUBLE,
	SANY,	TFLOAT,
		0,	RLEFT,
#	ifndef cookies
		"\tfpcvs.d\tAL\n",
#	else
		"\tfpcvs.d\tAL\t#\ttemplate d92i\n",
#	endif




SCONV,	INTDREG,	/* float to double (within registers) */
	STDREG,	TFLOAT,
	SANY,	TDOUBLE,
		0,	RLEFT,
#	ifndef cookies
		"\tfpcvd.s\tAL\n",
#	else
		"\tfpcvd.s\tAL\t#\ttemplate d92ii\n",
#	endif






SCONV,	INTDREG,	/* integer to float */
	EA|STAREG|STBREG,	TINT|TPOINT|TLONG,
	SANY,	TFLOAT,
		NDREG,	RESC1,
#	ifndef cookies
		"\tfpmovZL\tAL,A1\n\tfpcvs.l\tA1\n",
#	else
		"\tfpmovZL\tAL,A1\t# template d92j\n\tfpcvs.l\tA1\n",
#	endif




SCONV,	INTDREG,	/* integer to double */
	EA|STAREG|STBREG,	TINT|TPOINT|TLONG,
	SANY,	TDOUBLE,
		NDREG,	RESC1,
#	ifndef cookies
		"\tfpmovZL\tAL,A1\n\tfpcvd.l\tA1\n",
#	else
		"\tfpmovZL\tAL,A1\t# template d92jj\n\tfpcvd.l\tA1\n",
#	endif






SCONV,	INAREG|INTAREG,		/* float to integer (to register) */
	STDREG,	TFLOAT,
	SANY,		TINT|TPOINT|TLONG,
		NAREG,	RESC1,
#	ifndef cookies
		"Zr\n\tfpmovZR	AL,A1\n",
#	else
		"Zr\n\tfpmovZR\tAL,A1\t#template d93a\n",
#	endif





SCONV,	INAREG|INTAREG,		/* double to integer (to register) */
	STDREG,	TDOUBLE,
	SANY,		TINT|TPOINT|TLONG,
		NAREG,	RESC1,
#	ifndef cookies
		"Zr\n\tfpmovZR	AL,A1\n",
#	else
		"Zr\n\tfpmovZR\tAL,A1\t#template d93b\n",
#	endif





SCONV,	INTDREG,		/* float/double to integer (in registers) */
	STDREG,	TFLOAT|TDOUBLE,
	SANY,		TINT|TPOINT|TLONG,
		0,	RLEFT,
#	ifndef cookies
		"Zr\n",
#	else
		"Zr\t#template d93c\n",
#	endif





SCONV,	INAREG|INTAREG,		/* double to float (%fpa -> %d) */
	STDREG,		TDOUBLE,
	SANY,		TFLOAT,
		NAREG,	RESC1,
#	ifndef cookies
		"\tfpcvs.d\tAL\n\tfpmovZR	AL,A1\n",
#	else
		"\tfpcvs.d\tAL\t#template d93d\n\tfpmovZR	AL,A1\n",
#	endif





SCONV,	INAREG|INTAREG,		/* int to double (result in %d) */
	EAA,	TINT|TPOINT|TLONG,
	SANY,	TDOUBLE,
		NAREG|NDREG,	RESC1,
#	ifndef cookies
		"\tfpmov.l\tAL,A2\n\tfpcvd.l\tA2,A2\n\tfpmov.d\tA2,A1:U1\n",
#	else
		"\tfpmov.l\tAL,A2\t# template d93d\n\tfpcvd.l\tA2,A2\n\tfpmov.d\tA2,A1:U1\t# template d93e\n",
#	endif





SCONV,	INTFREG,	/* double to float (from a register) */
	SAREG|STAREG|SBREG|STBREG,	TDOUBLE,
	SANY,				TFLOAT,
		NFREG|2*NTEMP,	RESC1,
#	ifndef cookies
		"\tmov.l\tAL,A2\n\tmov.l\tUL,U2\n\tfmov.d\tA2,A1\n",
#	else
		"\tmov.l\tAL,A2\t#template 92g\n\tmov.l\tUL,U2\n\tfmov.d\tA2,A1\t#template 92h\n",
#	endif










SCONV,	INFREG|INTFREG,	/* anything to float or double (within registers) */
	SFREG|STFREG,	TANY,
	SANY,	TFLOAT|TDOUBLE,
		0,	RLEFT,
#	ifndef cookies
		"",
#	else
		"#\ttemplate 92i\n",
#	endif






SCONV,	INTFREG,	/* anything to float or double */
	EAA,	TCHAR|TSHORT|TINT|TPOINT|TLONG|TFLOAT|TDOUBLE,
	SANY,	TFLOAT|TDOUBLE,
		NFREG,	RESC1,
#	ifndef cookies
		"\tfmovZL\tAL,A1\n",
#	else
		"\tfmovZL\tAL,A1#\ttemplate 92j\n",
#	endif






SCONV,	INAREG|INTAREG,
	STFREG,	TFLOAT|TDOUBLE,
	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TPOINT|TLONG,
		NAREG,	RESC1,
#	ifndef cookies
		"Zq\n",
#	else
		"Zq\t#template 93a\n",
#	endif



#if 0

/* This template would generate fintrz instructions, since this instruction
   is not on 040's, this template has been commented out and replaced with
   the one above.  - sje
*/

SCONV,	INTFREG,
	STFREG,	TFLOAT|TDOUBLE,
	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TPOINT|TLONG,
		0,	RLEFT,
#	ifndef cookies
		"Zr\n",
#	else
		"Zr\t#template 93b\n",
#	endif

#endif



SCONV,	INAREG|INTAREG,
	STFREG,		TDOUBLE,
	SANY,		TFLOAT,
		NAREG,	RESC1,
#	ifndef cookies
		"\tfmovZR	AL,A1\n",
#	else
		"\tfmovZR\tAL,A1\t#template 93c\n",
#	endif





SCONV,	INAREG|INTAREG,
	EAA,	TCHAR|TSHORT|TINT|TPOINT|TLONG,
	SANY,	TDOUBLE,
		NAREG|NFREG|2*NTEMP,	RESC1,
#	ifndef cookies
		"\tfmovZL\tAL,A2\n\tfmov.d\tA2,A3\n\tmov.l\tA3,A1\n\tmov.l\tU3,U1\n",
#	else
		"\tfmovZL\tAL,A2\t# template 93d\n\tfmov.d\tA2,A3\n\tmov.l\tA3,A1\n\tmov.l\tU3,U1\t# template 93e\n",
#	endif












/* #ifndef FORT */

STASG,	FOREFF,
	SOREG|SNAME,	TANY,
	STBREG,	TANY,
		NAREG|NBREG|NBSL,	RNOP,	/* NBSL is intentional */
#ifndef cookies
		"Zs",
#else
		"Zs\t#template 94a\n",
#endif





#ifdef FORTY

STASG,	INBREG|INTBREG,
	SOREG|SNAME,	TANY,
	SBREG|STBREG,	TANY,
		NAREG|NBREG|NBSL,	RRIGHT,	/* NBSL is intentional */
#ifndef cookies
		"\tmov.l\tAR,-(%sp)\nZs\tmov.l\t(%sp)+,AR\n",
#else
		"\tmov.l\tAR,-(%sp)\nZs\tmov.l\t(%sp)+,AR\t#template 94b\n",
#endif

#else /* FORTY */

STASG,	INBREG|INTBREG,
	SOREG|SNAME,	TANY,
	SBREG|STBREG,	TANY,
		NAREG|NBREG|NBSL,	RRIGHT,	/* NBSL is intentional */
#ifndef cookies
		"\tpea\t(AR)\nZs\tmov.l\t(%sp)+,AR\n",
#else
		"\tpea\t(AR)\nZs\tmov.l\t(%sp)+,AR\t#template 94b\n",
#endif

#endif /* FORTY */




/* The following template is useful only for register unions (unionflag!=0) */
STASG,	FOREFF|INAREG|INTAREG|INBREG|INTBREG,
	SAREG|STAREG|SBREG|STBREG,	TANY,
	SAREG|STAREG|SBREG|STBREG,	TANY,
		0,	RRIGHT|RLEFT,
#ifndef cookies
		"\tmov.l\tAR,AL\n",
#else
		"\tmov.l\tAR,AL\t#template 94c\n",
#endif





#ifndef FORT

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT,
		0,	RNOP,
#ifndef cookies
		"Zh	CL\n",
#else
		"Zh	CL\t#template 95\n",
#endif








INIT,	FOREFF,
	SNAME,	TANY,
	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT,
		0,	RNOP,
#ifndef cookies
		"Zh	AL\n",
#else
		"Zh	AL\t#template 96\n",
#endif

#endif /* ifndef FORT */







FMONADIC,	INTDREG|FOREFF|FORCC,
	SANY,	TANY,
	STDREG,	TANY,
		0,	RRIGHT|RESCC,
#		ifndef cookies
		"\tZaZR\tAR\n",
#		else	/* cookies */
		"\tZaZR\tAR\t#template d97\n",
#		endif	/* cookies */



FMONADIC,	INTFREG|FOREFF|FORCC,
	SANY,	TANY,
	STFREG,	TANY,
		0,	RRIGHT|RESCC,
#		ifndef cookies
		"\tZ8\n",
#		else	/* cookies */
		"\tZ8\t#template 97\n",
#		endif	/* cookies */



FMONADIC,	INTDREG|FOREFF|FORCC,
	SANY,	TANY,
	SDREG,	TANY,
		NDREG,	RESC1|RESCC,
#		ifndef cookies
 		"\tZaZR\tAR,A1\n",
#		else	/* cookies */
		"\tZaZR\tAR,A1\t#template d97b\n",
#		endif	/* cookies */



FMONADIC,	INTFREG|FOREFF|FORCC,
	SANY,	TANY,
	SFREG,	TANY,
		NFREG,	RESC1|RESCC,
#		ifndef cookies
		"\tZ8,A1\n",
#		else	/* cookies */
		"\tZ8,A1\t#template 97b\n",
#		endif	/* cookies */



FMONADIC,	INTDREG|FOREFF,
	SANY,	TANY,
	STFREG,	TANY,
		2*NTEMP|NDREG,	RESC1|RESC2,
#		ifndef cookies
		"\tZ8\n\tfmovZB\tAR,A2\n\tfpmovZB\tA2,A1\n",
#		else	/* cookies */
		"\tZ8\t#template d97cx\n\tfmovZB\tAR,A2\t#template d97cy\n\tfpmovZB\tA2,A1\t#template d97cz\n",
#		endif	/* cookies */



#ifdef FORT
GOTO,	FOREFF,		/* I don't know how this template can be used */
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
#		ifndef cookies
		"\tbra.l\tCL\n",
#		else
		"\tbra.l\tCL\t#template 98\n",
#		endif
#	endif	/* FORT */




#ifdef FORT
GOTO,	FOREFF,	  /* this template used for assigned gotos to addr reg opds */
	SBREG|STBREG,		TANY,
	SANY,			TANY,
		0,	RLEFT,
#ifndef cookies
		"\tjmp\t(AL)\n",
#else
		"\tjmp\t(AL)\t#template 99\n",
#endif


GOTO,	FOREFF,	  /* this template used for assigned gotos to addr reg opds */
	SAREG|STAREG,		TANY,
	SANY,			TANY,
		NBREG,	RESC1,
#ifndef cookies
		"\tmov.l\tAL,A1\n\tjmp\t(A1)\n",
#else
		"\tmov.l\tAL,A1\t#template 99c\n\tjmp\t(A1)\t#template 99d\n",
#endif


GOTO,	FOREFF,		/* this template used for assigned gotos to local vars */
	SNAME,	TANY,
	SANY,	TANY,
		NBREG,	RESC1,
#ifndef cookies
		"\tmov.l\tAL,A1\n\tjmp\t(A1)\n",
#else
		"\tmov.l\tAL,A1\t#template 99a\n\tjmp\t(A1)\t#template 99b\n",
#endif


GOTO,	FOREFF,		/* used for assigned gotos to dynamic vars */
	SOREG,	TANY,
	SANY,	TANY,
		NBREG,	RESC1,
#ifndef cookies
		"\tmov.l\tAL,A1\n\tjmp\t(A1)\n",
#else
		"\tmov.l\tAL,A1\t#template 100a\n\tjmp\t(A1)\t#template 100b\n",
#endif

GOTO,	FOREFF,		/* used for assigned gotos to static vars */
	STARNM,	TANY,
	SANY,	TANY,
		NBREG,	RESC1,
#ifndef cookies
		"\tmov.l\tAL,A1\n\tmov.l\t(A1),A1\n\tjmp\t(A1)\n",
#else
		"\tmov.l\tAL,A1\t#template 101a\n\tmov.l\t(A1),A1\n\tjmp\t(A1)\t#template 101b\n",
#endif

# else	/* FORT */




FLD,	FORCC,
	STARNM,		TANY,
	SANY,		TANY,
		NBREG,	RESCC,
#	ifndef cookies
		"\tmov.l	AL,A1\nZU\n",
#	else	/* cookies */
		"\tmov.l	AL,A1\nZU\t#template 102\n",
#	endif	/* cookies */






FLD,	FORCC,
	SAREG|STAREG|SOREG|SNAME,	TANY,
	SANY,				TANY,
		0,		RESCC,
#	ifndef cookies
	"Z7\n",
#	else	/* cookies */
	"Z7\t#template 104\n",
#	endif	/* cookies */







FLD,	INTAREG|FORCC,
	SAREG|STAREG|SOREG|SNAME,	TANY,
	SANY,				TANY,
		NAREG,	RESC1|RESCC,
#	ifndef cookies
	"Z6\n",
#	else	/* cookies */
	"Z6\t#template 105\n",
#	endif	/* cookies */


#endif /* FORT */


#ifdef FORT
EQV,	INTAREG|FORCC,
	EAA|STAREG,	TCHAR|TUCHAR,
	STAREG,		TCHAR|TUCHAR,
		NAREG,	RRIGHT|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne.b\tA1\n\ttst.b\tAR\n\tseq.b\tAR\n\teor.b\tA1,AR\
\n\tneg.b\tAR\n",
#	else
	"\ttst.b\tAL\t#template 109\n\tsne.b\tA1\n\ttst.b\tAR\n\tseq.b\tAR\
\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	endif


EQV,	INTAREG|FORCC,
	STAREG,	TCHAR|TUCHAR,
	EAA,	TCHAR|TUCHAR,
		NAREG,	RLEFT|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne.b\tAL\n\ttst.b\tAR\n\tseq.b\tA1\n\teor.b\tA1,AL\
\n\tneg.b\tAL\n",
#	else
	"\ttst.b\tAL\t#template 110\n\tsne.b\tAL\n\ttst.b\tAR\n\tseq.b\tA1\
\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	endif


EQV,	INAREG|INTAREG|FORCC,
	EAA,	TCHAR|TUCHAR,
	EAA,	TCHAR|TUCHAR,
		2*NAREG,	RESC1|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne.b\tA1\n\ttst.b\tAR\n\tseq.b\tA2\n\teor.b\tA2,A1\
\n\tneg.b\tA1\n",
#	else
	"\ttst.b\tAL\t#template 111\n\tsne.b\tA1\n\ttst.b\tAR\n\tseq.b\tA2\
\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	endif


EQV,	INTAREG|FORCC,
	EAA|STAREG,	TSCALAR,
	STAREG,		TSCALAR,
		NAREG,	RRIGHT|RESCC,
#	ifndef cookies
	"\ttstZR\tAR\n\tseq.b\tA1\n\tmovq\t&0,AR\n\ttstZl\tAL\n\tsne.b\tAR\
\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	else
	"\ttstZR\tAR\t#template 106\n\tseq.b\tA1\n\tmovq\t&0,AR\n\ttstZl\tAL\
\n\tsne.b\tAR\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	endif


EQV,	INTAREG|FORCC,
	STAREG,	TSCALAR,
	EAA,	TSCALAR,
		NAREG,	RLEFT|RESCC,
#	ifndef cookies
	"\ttstZl\tAL\n\tseq.b\tA1\n\tmovq\t&0,AL\n\ttstZR\tAR\n\tsne.b\tAL\
\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	else
	"\ttstZl\tAL\t#template 107\n\tseq.b\tA1\n\tmovq\t&0,AL\n\ttstZR\tAR\
\n\tsne.b\tAL\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	endif


EQV,	INAREG|INTAREG|FORCC,
	EAA,	TSCALAR,
	EAA,	TSCALAR,
		2*NAREG,	RESC1|RESCC,
#	ifndef cookies
	"\tmovq\t&0,A1\n\ttstZl\tAL\n\tsne.b\tA1\n\ttstZR\tAR\n\tseq.b\tA2\
\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	else
	"\tmovq\t&0,A1\t#template 108\n\ttstZl\tAL\n\tsne.b\tA1\n\ttstZR\tAR\
\n\tseq.b\tA2\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	endif


NEQV,	INTAREG|FORCC,
	STAREG,		TCHAR|TUCHAR,
	EAA|STAREG,	TCHAR|TUCHAR,
		NAREG,	RLEFT|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne\tAL\n\ttst.b\tAR\n\tsne.b\tA1\n\teor.b\tA1,AL\n\
\tneg.b\tAL\n",
#	else
	"\ttst.b\tAL\t#template 115\n\tsne.b\tAL\n\ttst.b\tAR\n\tsne.b\tA1\
\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	endif


NEQV,	INTAREG|FORCC,
	EAA,	TCHAR|TUCHAR,
	STAREG,	TCHAR|TUCHAR,
		NAREG,	RRIGHT|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne\tA1\n\ttst.b\tAR\n\tsne.b\tAR\n\teor.b\tA1,AR\n\
\tneg.b\tAR\n",
#	else
	"\ttst.b\tAL\t#template 116\n\tsne.b\tA1\n\ttst.b\tAR\n\tsne.b\tAR\
\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	endif


NEQV,	INAREG|INTAREG|FORCC,
	EAA,	TCHAR|TUCHAR,
	EAA,	TCHAR|TUCHAR,
		2*NAREG,	RESC1|RESCC,
#	ifndef cookies
	"\ttst.b\tAL\n\tsne\tA1\n\ttst.b\tAR\n\tsne.b\tA2\n\teor.b\tA2,A1\n\
\tneg.b\tA1\n",
#	else
	"\ttst.b\tAL\t#template 117\n\tsne.b\tA1\n\ttst.b\tAR\n\tsne.b\tA2\
\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	endif


NEQV,	INTAREG|FORCC,
	EAA|STAREG,	TSCALAR,
	STAREG,		TSCALAR,
		NAREG,	RRIGHT|RESCC,
#	ifndef cookies
	"\ttstZR\tAR\n\tsne.b\tA1\n\tmovq\t&0,AR\n\ttstZl\tAL\n\tsne.b\tAR\
\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	else
	"\ttstZR\tAR\t#template 112\n\tsne.b\tA1\n\tmovq\t&0,AR\n\ttstZl\tAL\
\n\tsne.b\tAR\n\teor.b\tA1,AR\n\tneg.b\tAR\n",
#	endif


NEQV,	INTAREG|FORCC,
	STAREG,	TSCALAR,
	EAA,	TSCALAR,
		NAREG,	RLEFT|RESCC,
#	ifndef cookies
	"\ttstZl\tAL\n\tsne.b\tA1\n\tmovq\t&0,AL\n\ttstZR\tAR\n\tsne.b\tAL\
\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	else
	"\ttstZl\tAL\t#template 113\n\tsne.b\tA1\n\tmovq\t&0,AL\n\ttstZR\tAR\
\n\tsne.b\tAL\n\teor.b\tA1,AL\n\tneg.b\tAL\n",
#	endif


NEQV,	INAREG|INTAREG|FORCC,
	EAA,	TSCALAR,
	EAA,	TSCALAR,
		2*NAREG,	RESC1|RESCC,
#	ifndef cookies
	"\tmovq\t&0,A1\n\ttstZl\tAL\n\tsne.b\tA1\n\ttstZR\tAR\n\tsne.b\tA2\
\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	else
	"\tmovq\t&0,A1\t#template 114\n\ttstZl\tAL\n\tsne.b\tA1\n\ttstZR\tAR\
\n\tsne.b\tA2\n\teor.b\tA2,A1\n\tneg.b\tA1\n",
#	endif
#endif  /* FORT */




/* cookies for logical op type nodes */




/* note : all rewrite nodes must be defined at the end of table in a group.
	Otherwise setrew will not work properly.
*/

	/* Default actions for hard trees ... */

#ifndef cookies
#	 define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""
#else
#	 define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,"\t#default cookie\n"
#endif

UNARY MUL, DF( UNARY MUL ),

INCR, DF(INCR),

DECR, DF(INCR),

ASSIGN, DF(ASSIGN),

STASG, DF(STASG),

OPLEAF, DF(NAME),

OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	BITYPE,
#ifndef cookies
		"",
#else
		"#\toplog default cookie\n",
#endif






OPLOG,	DF(NOT),

COMOP, DF(COMOP),

INIT, DF(INIT),

OPUNARY, DF(UNARY MINUS),


ASG OPANY, DF(ASG PLUS),

OPANY, DF(BITYPE),

FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" };
