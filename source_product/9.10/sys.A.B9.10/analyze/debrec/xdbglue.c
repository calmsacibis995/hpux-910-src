/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/xdbglue.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:24:21 $
 */

/* Include from expr.c */
#include <ctype.h>
#include <signal.h>
#include "cdb.h"

extern	long	atol();
extern	double	atof();

STUFFU	astuffZeros;		/* defaults to all zeroes */

#define	FUnsigned(bt)	\
    ((bt == btUChar) OR (bt == btUShort) OR (bt == btUInt) \
     OR (bt == btULong) OR (bt == btEType))


#define CMDBUFSIZE 100
int vcBadMax = 0;
int vcbCmdBuf = 0;
int viln = 0;
int vimap = 0;
ADRT vpc = 0;
ADRT vpc2 = 0;
ADRT vsp = 0;
ADRT vdp = 0;
ADRT vpsw = 0;
int vpid = 0;
char * vsbCmd = 0;
char vsbCmdbuf[CMDBUFSIZE];
uint vsig = 0;

/* remove when merge */
#ifndef DEBREC
main()
{
xdbglue();
}
#endif

xdbglue()
{
xdbmain();
}

/* From err.c */
void Panic (msg)
    char	*msg;			/* message (printf() string) */
{

    if (msg)
	printf (msg);
    printf ("\n");			/* always finish a line */

#ifdef NOTDEF
    longjmp (venv, 1);			/* bail out */
#endif

    return;

} /* Panic */



/* From err.c */
void UError (msg)
    char	*msg;			/* message (printf() string) */
{


    if (msg)
	printf (msg);
    else
	printf (" UERROR called\n");	/* generic ouch */

    printf  ("UERROR\n");			/* always finish a line */
#ifdef NOTDEF
    longjmp (venv, 1);				/* then bail out	*/
#endif

} /* UError */



/* From expr.c */
/***********************************************************************
 * V A L   F   A D R   T Y
 *
 * Given an address, type, and optional size, return the data pointed to
 * by that address, or the address itself if the type is a constant or
 * "special" (or the double value for double constants), sign-extended
 * or cast to long if necessary.
 * 
 * Values shorter than double are "masked" in the sense that only the
 * needed bytes are gotten (except for constants, which must already have
 * the right value).  Follows the rules of C for lengthening char and
 * short to int, but does not promote float to double (caller must do that).
 *
 * Odd sizes (3,5,6,7) come back beginning at the start of val->doub.
 * These are only possible if the caller specifies cb != 0, since all
 * the base types have "typical" sizes.  Note that cb < 0 means bit field.
 *
 * This routine is not designed for more than a double.  Unfortunately,
 * it gets called as a side-effect even when processing, say, a structure
 * field, so we can't even give a warning of truncation here.  However,
 * note that PsFDoOp() has independent provision for handling larger
 * values, so it's normally not a problem.
 */

void ValFAdrTy (val, adrLong, ty, cbIn, fNeedLong)
    register STUFFU *val;			/* where to return data	*/
    long	adrLong;			/* adr of the value	*/
    pTYR	ty;				/* type of the value	*/
    int		cbIn;				/* size; 0 => base size	*/
    FLAGT	fNeedLong;			/* need cast to long?	*/
{
    register int cb;				/* bytes to get		*/
    ADRT	 adrSrc = adrLong;		/* for copying value	*/
    ADRT	 adrDest;
    register int bt     = ty->td.bt;		/* base type		*/
    int		 tq1    = TqFTy (ty, 1);	/* top tq		*/
    FLAGT	 fAdrVal = ty->fAdrIsVal;
    pXTYR        xty = (pXTYR) (ty + 1);

#ifdef INSTR
    vfStatProc[92]++;
#endif

    *val = astuffZeros;				/* clear return data area */

    cb = CbFTy (ty, false);			/* default = base size */

    if (cbIn AND (cb >= 0))			/* size given, not bit field */
	cb = cbIn;

    if (((bt == btUChar) || (bt == btUShort) ||
         (bt == btUInt)  || (bt == btULong)) &&
        (xty->btX == TX_BOOL) && (cb >= 0)) {
       cb = CBCHAR;
    }

    if (cb > CBDOUBLE)				/* bt too long	*/
	cb = CBDOUBLE;				/* truncate	*/

    if (fAdrVal AND (tq1 == tqNil) AND (bt == btDouble))
    {
	pDXTYR dxtyr = (pDXTYR) (ty + 1);	/* special use of XTYR */
	val->doub    = dxtyr->doub;		/* get double constant */
	goto Convert;
    }

    if (fAdrVal OR (tq1 == tqArray))		/* simple const, or array */
    {
	val->lng = adrLong;			/* adr is the value */
	goto Convert;
    }

/*
 * GET A REGISTER:
 *
 * Pc is the only register that is modified internal to the debugger for
 * a long time before writing it back to the child.  For this special case
 * we use vpc rather than read the value from the child.
 */

    if (ty->td.st == stReg)
    {
	REGT	regVal = (adrSrc == upc) ? vpc : GetReg (adrSrc);

	if (cb == CBCHAR)
	{
	    val->chars.chLoLo	= regVal;
	}
	else if (cb == CBSHORT)
	{
	    val->shorts.shortLo = regVal;
	}
	else
	{
#if (CBINT == CBSHORT)
/*
 * They want a long reg value but vpc is short; have no choice but to read it:
 */
	    val->shorts.shortHi = GetReg (adrSrc);
	    val->shorts.shortLo = GetReg (adrSrc + 1);
#else
	    val->lng = regVal;
#endif
	}

	goto Convert;

    } /* if */

#ifdef FOCUS
/*
 * ADJUST WHERE TO GET the value (in child's memory), according to its size:
 *
 * This is needed because FORTRAN parameters are always indirect, and the
 * pointer on the stack points to the LSB of the data.
 */

    if ((adrSrc & 3) == 3)		/* at LSB of a word */
    {
	if (cb == CBLONG)
	    adrSrc &= (~ 3);		/* round adr to start of a word	*/
	else if (cb == CBSHORT)
	    adrSrc &= (~ 1);		/* round adr to a halfword */
    }
#endif

/*
 * FIGURE WHERE TO SAVE the value (in debugger memory), according to its size:
 */

    adrDest = (cb == CBCHAR)  ?	(ADRT) & (val->chars.chLoLo)   :
	      (cb == CBSHORT) ?	(ADRT) & (val->shorts.shortLo) :
	      (cb == CBLONG)  ?	(ADRT) & (val->lng)	       :
				(ADRT) & (val->doub);	/* odd sizes too */

/*
 * GET A DEBUGGER SPECIAL:
 */

    if (ty->td.st == stSpc)
    {
	MoveBytes (adrDest, adrSrc, cb);	/* copy in debugger memory */
	goto Convert;
    }

    if (adrSrc == adrNil)		/* nil address; use zero */
	goto Convert;

/*
 * GET A DATA VALUE:
 */

    if (cb > 0)				/* normal case; just get the data */
    {
	GetBlock (adrSrc, spaceData, adrDest, cb);

#ifdef FOCUS		/* break pointer if needed (never for bit field) */
	val->lng = AdrFAdrTy (val->lng, ty, false);
#endif
    }					/* fall to sign extension */
    else
    {					/* bit field specifier */

	long	valTemp;
					/* avoid unsigned divide */
	cb = ((long) (ty->valTy + ty->td.width + (SZCHAR-1))) / SZCHAR;
	GetBlock (adrSrc, spaceData, & valTemp, cb);
	val->lng = Extract (valTemp, ty->valTy, ty->td.width);
	if (((ty->td.bt == btInt)
	  OR (ty->td.bt == btShort)
	  OR (ty->td.bt == btLong)
	  OR (ty->td.bt == btChar))
	AND (val->lng & (1L << (ty->td.width - 1))))  /* signed field */
	{
	    val->lng |= (~0L) << ty->td.width;		/* extend sign */
	}
	    
	return;		/* always unsigned, don't extend, no need to cast */
    }

/*
 * DO CONVERSIONS:  sign extension, cast to long:
 *
 * This is a mess because sign extension is needed if a small, signed basetype
 * was used, OR if the caller gave a small size and the basetype was signed.
 */

Convert:

    if ((tq1 == tqNil) OR cbIn)		/* simple basetype or size given */
    {
	FLAGT	fSigned = ! (FUnsigned (bt));	/* basetype signed? */

	if (((! cbIn) AND (bt   == btChar))	/* really is signed char   */
	OR  (fSigned  AND (cbIn == CBCHAR)))	/* signed and size of char */
	{
	    val->lng = val->chars.chLoLo;	/* sign extend */
	}
	else if (((! cbIn) AND (bt   == btShort))
	     OR  (fSigned  AND (cbIn == CBSHORT)))
	{
	    val->lng = val->shorts.shortLo;	/* sign extend */
	}

#if (CBINT == CBSHORT)
	else if (((! cbIn) AND (bt   == btInt))
	     OR  (fSigned  AND (cbIn == CBINT)))
	{
	    val->lng = val->shorts.shortLo;	/* sign extend */
	}
#endif

	else if (fNeedLong)
	{
	    if (bt == btFloat)			/* shorten */
		val->lng = val->fl;

	    else if (bt == btDouble)		/* shorten */
		val->lng = val->doub;
	}
    }

} /* ValFAdrTy */


/* From err.c */
/***********************************************************************
 * M A L L O C
 *
 * This front-end to malloc(3) optionally does error checking (if the
 * calling proc tells its name) and keeps the totals variable updated.
 * Rather than adding and subtracting cb here and in Free(), we give
 * a more precise estimate using sbrk(2).
 *
 */

char * Malloc (cb, fAbort)
    uint	cb;		/* bytes to malloc */
    FLAGT	*fAbort;	/* abort on error? */
{
    char	*sb;		/* ptr to memory gotten	   */

    if (((sb = malloc (cb)) == sbNil) AND fAbort) /* malloc failed */
    {
	sprintf (vsbMsgBuf, (nl_msg(529, "Internal Error (IE529) (%d)")), cb);
	Panic (vsbMsgBuf);
    }

    vcbMalloc = sbrk (0) - vsbSbrkFirst;
    return (sb);

} /* Malloc */


/***********************************************************************
 * F R E E
 *
 * This front-end to free(3) avoids using a nil pointer, and keeps the
 * totals variable updated.
 */

void Free (sb)
    char	*sb;		/* memory to free */
{


    if (sb == sbNil)
    {
	sprintf (vsbMsgBuf, (nl_msg(530, "Internal Error (IE530) (%d)")), sb);
	Panic (vsbMsgBuf);
    }

    free (sb);
    vcbMalloc = sbrk (0) - vsbSbrkFirst;

} /* Free */


/***********************************************************************
 * R E A L L O C
 *
 * This front-end to realloc(3) optionally does error checking (if the
 * calling proc tells its name) and keeps the totals variable updated.
 */

char * Realloc (ptr, cb)
    long	ptr;		/* input ptr to reallocate */
    uint	cb;		/* bytes to malloc	   */
{
    char	*sb;		/* ptr to memory gotten	   */


    if ((sb = realloc (ptr, cb)) == sbNil)	/* realloc failed */
    {
	sprintf (vsbMsgBuf, (nl_msg(531, "Internal Error (IE531) (%d)")), cb);
	Panic (vsbMsgBuf);
    }

    vcbMalloc = sbrk (0) - vsbSbrkFirst;
    return (sb);

} /* Realloc */


/* From access.c */
/***********************************************************************
 * M O V E   B Y T E S
 *
 * Do a possibly-overlapping move, to the right or left, one byte at a
 * time, of an object in the debugger's memory space.
 */

void MoveBytes (adrDest, adrSrc, cb)
    ADRT	adrDest;	/* where to move from	*/
    ADRT	adrSrc;		/* where to move to	*/
    register int cb;		/* bytes to move	*/
{
    register char *dest = (char *) adrDest;
    register char *src  = (char *) adrSrc;


    if (dest < src)			/* move left to right */
    {
	while (cb--)
	    *dest++ = *src++;
    }
    else {				/* move right to left */
	dest += cb;
	src  += cb;
	while (cb--)
	    *(--dest) = *(--src);
    }
} /* MoveBytes */

/* access.c */
extern int getpc();
extern int getusp();

unsigned int globalp;

/* Glue, limited functionality */
GetReg(reg)
	int reg;
{
	unsigned int val;

	if (reg == upc){
		val = getpc(globalp);
		return(val);
	}

	if (reg == upc2){
		val = getpc2(globalp);
		return(val);
	}

	if (reg == umrp){
		val = getumrp(globalp);
		return(val);
	}

	if (reg == usp){
		val = getusp(globalp);
		return(val);
	}

	if (reg == urp){
		val = getrp(globalp);
		return(val);
	}

	if (reg == udp){
		val = getdp(globalp);
		return(val);
	}


	if (reg == upsw){
		val = getpsw(globalp);
		return(val);
	}

	fprintf(stderr, "GetReg called with reg 0x%x returning 0\n",reg);
	return(0);
}

#ifndef DEBREC
/* For NoW */
getumrp()
{
	printf("Getumrp called returning 0\n");
	return(0);
}
getrp()
{
	printf("Getrp called returning 0\n");
	return(0);
}

getpc()
{
	printf("Getpc called returning 0\n");
	return(0);
}

getusp()
{
	printf("Getsp called returning 0\n");
	return(0);
}
#endif


int xdbdebug = 0;
int xdbtraceabort = 0;

void GetState(p)
unsigned int p;
{
	if (xdbdebug)
		fprintf (stderr,"local p = 0x%x \n",p);
	globalp = (unsigned int)p;
	vsp = getusp(p);
    	vdp = getdp(p);
    	vpc = getpc(p);
    	vpc2 = getpc2(p);
    	vpsw = getpsw(p);

} /* GetState */


/***********************************************************************
 * G E T   W O R D
 *
 * Read and return a word from a child process or corefile using Access().
 *
 */

int GetWord (adr, space)
    ADRT	adr;		/* address to read */
    int		space;		/* part of process */
{
/* We ignore space here spaceText, spaceData */
	int spc;
	int buffer;
	int err;
	spc = ldsid((adr & ~3), globalp);
	err = getchunk(spc, (adr & ~3), &buffer, 4, "GetWord");

	xdbtraceabort = err; /* Record error */

	if (xdbdebug)
   		fprintf(stderr, "\nGetWord spc 0x%x adr 0x%x  returning 0x%x\n",spc, adr & ~3, buffer );
	return(buffer);

} /* GetWord */


/***********************************************************************
 * G E T   B Y T E
 *
 * Read and return one byte from a child process or corefile using Access().
 *
 */

int GetByte (adr, space)
    ADRT	adr;		/* address to read */
    int		space;		/* part of process */
{
    STUFFU	stuff;		/* for unpacking it */

#ifdef EVENBYTES
    ADRT	adrOnWord = (adr & ~1);		/* all but low bit */

    stuff.shorts.shortLo = Access (accRead, adrOnWord, space, 0);
    return ((adrOnWord == adr) ? stuff.chars.chLoHi : stuff.chars.chLoLo);
#endif


#ifdef SPECTRUM
   stuff.lng = GetWord((adr & ~3), space);
   switch (adr & 3) {

      case 0: return( stuff.chars.chHiHi );

      case 1: return( stuff.chars.chHiLo );

      case 2: return( stuff.chars.chLoHi );

      case 3: return( stuff.chars.chLoLo );

   }
#endif

} /* GetByte */


/*
 * GetBlock() and PutBlock() could be sped up by knowing about where word
 * boundaries are and such, but it doesn't seem worth it at the moment.
 */


/***********************************************************************
 * G E T   B L O C K
 *
 * Read a block of data from a child process [or corefile].
 *
 */

void GetBlock (adr, space, adrValue, cb)
    ADRT	adr;		/* where to read data	*/
    int		space;		/* part of child proc	*/
    ADRT	adrValue;	/* where to put data	*/
    int		cb;		/* bytes in data	*/
{
#ifndef S200
    char        *pCh;
#endif


    pCh = (char *) adrValue;	/* point to receiving area */
    if (cb) while (cb--)
	*pCh++ = GetByte (adr++, space);



} /* GetBlock */

extern int xdbenable;
extern int allow_sigint;
extern FILE *outf;

/* display_print() */
display_print(name, param2 ,redir,path)
char *name;
int param2, redir;
char *path;

{

	if (xdbenable == 0)
		return(0);

	fprintf(outf,"\n");
#ifdef DEBUG
	fprintf(stderr,"display_print( 0x%x, 0x%x, 0x%x, 0x%x)\n",name, param2,redir,path);
#endif
	if (redir){
		if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL){
			fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
			goto out;
		}
	}
	allow_sigint = 1;


	xdbprintstruct(param2,name);


reset:
	allow_sigint = 0;
	/* close file if we redirected output */
	if (redir)
		fclose(outf);

out: 	outf = stdout;
	return(0);
}





/* Printstruct */
xdbprintstruct(adr,name)
unsigned int adr;
char * name;
{
int val;
char *sbName; 
TYR		rgTy[cTyMax];		/* type of variable	*/


/*Start at front of combined GNTT & LNTT table */
	SetNext(0);

	val = FNextSym(K_TAGDEF,K_SVAR,K_DVAR,K_NIL,name,FSbCmp,K_TYPEDEF,K_NIL,K_NIL,K_NIL);

	if (val == false){
		fprintf(stderr," Could not find matching symbol for %s\n",name);
		return(0);

	}else{

		if (xdbdebug)
			fprintf(stderr, " visym = %d  name:%s\n",visym, name);

	}

	sbName = NameFCurSym();

	TyFGlobal(rgTy, sbName);


	DispVal(adr, rgTy, modeNil, true, true, true);


}



/* Convert all printfs to this form */

void xdbprintf (msg, arg1, arg2, arg3, arg4, arg5, arg6)
    char	*msg;			/* message (printf() string) */
    int		arg1, arg2, arg3, arg4, arg5, arg6;
{

#undef printf					/* unset locally */

    fprintf (outf, msg, arg1, arg2, arg3, arg4, arg5, arg6);

}

