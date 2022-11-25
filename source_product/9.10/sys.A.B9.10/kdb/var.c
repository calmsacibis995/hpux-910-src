/* @(#) $Revision: 70.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines find addresses and sizes of various kinds of variables,
 * scan, display, and otherwise manage stack frames, list registers and
 * special variables, etc.
 */

#include "macdefs.h"
#include "cdb.h"

#ifndef CDBKDB
extern long	notime;
#endif /* CDBKDB */

/***********************************************************************
 * PREDEFINED SPECIAL VARIABLES:
 *
 * These include accesses to debugger internal values, user registers,
 * and user procedure return values.  They are initialized here to
 * appropriate types and locations.  In general they're marked NOT
 * constant, e.g. the value is the address, and users can write them.
 * For registers, the address is the register index used with GetReg().
 */

#define SbCast(x)  ((int) x)	/* (char*) pretends to be int */


TYR	vrgTySpc[] = {

    /* name,		st,    const, unused, bt, desc (tq),  value */

#ifndef CDBKDB
    {SbCast ("$notime"),   stSpc, false, 0, 0,0,0,0,0,0, btInt, (ulong)&notime},
#endif /* CDBKDB */
    {SbCast ("$scroll"),   stSpc, true, 0, 0,0,0,0,0,0, btInt, (ulong)&scroll},
    {SbCast ("$memsize"),  stSpc, true, 0, 0,0,0,0,0,0, btLong,(ulong)&memsize},
    {SbCast ("$noprotect"),stSpc, false, 0, 0,0,0,0,0,0, btInt, (ulong)&noprotect},

    {SbCast ("$result"),stReg, false, 0, 0,0,0,0,0,0, btLong, ud0},

    {SbCast ("$pc"),	stReg, false, 0, 0,0,0,0,0,0, btLong, upc},
    {SbCast ("$fp"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ufp},
    {SbCast ("$sp"),	stReg, false, 0, 0,0,0,0,0,0, btLong, usp},
    {SbCast ("$ps"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ups},
    {SbCast ("$usp"),	stReg, false, 0, 0,0,0,0,0,0, btLong, uusp},

    {SbCast ("$a0"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua0},
    {SbCast ("$a1"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua1},
    {SbCast ("$a2"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua2},
    {SbCast ("$a3"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua3},
    {SbCast ("$a4"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua4},
    {SbCast ("$a5"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua5},
    {SbCast ("$a6"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua6},
    {SbCast ("$a7"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ua7},

    {SbCast ("$d0"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud0},
    {SbCast ("$d1"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud1},
    {SbCast ("$d2"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud2},
    {SbCast ("$d3"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud3},
    {SbCast ("$d4"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud4},
    {SbCast ("$d5"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud5},
    {SbCast ("$d6"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud6},
    {SbCast ("$d7"),	stReg, false, 0, 0,0,0,0,0,0, btLong, ud7},
};


int	vispcMac = (sizeof (vrgTySpc) / cbTYR);		/* total number */


/***********************************************************************
 * USER-DEFINED SPECIAL VARIABLES ("local vars"):
 *
 * They are set up as all tyLong, and their values are saved in the
 * value fields directly.
 */

int		vilvMac;		/* current number */
export int	vilvMax;		/* limit	  */
pTYR		vrgTyLv;		/* array of them  */


/***********************************************************************
 * I N I T   S P C
 *
 * Set up the debugger special variables.  It allocates memory and sets
 * default type information for user-defined variables (lv's), and sets
 * bitHighs in names of predefined variables (for the string space vs.
 * pointer hack).
 */

export void InitSpc()
{
    TYR		aty;			/* instance of a type */
    register int		i;			/* which special var  */

    aty = *vtyLong;			/* set to non-const long */
    aty.td.st = stSpc;			/* all are of type stSpc */

    for (i = 0; i < vispcMac; i++)
	vrgTySpc[i].sbVar |= bitHigh;	/* bitHigh is zero for some systems */

    vrgTyLv = (pTYR) Malloc ((vilvMax + 1) * cbTYR, "InitSpc");	/* get memory */

    for (i = 0; i < vilvMax; i++)	/* set them all to defaults */
	CopyTy (vrgTyLv + i, &aty);

} /* InitSpc */


/***********************************************************************
 * A D R   F   S P E C I A L
 *
 * Return the address and type of a special variable from its name,
 * checking predefined variables first.	 An EXACT MATCH is required (no
 * substrings allowed).  If the variable is not found, add it to the
 * user-defined list if room permits, else UError() out.
 *
 * This is the only AdrF routine which returns void and returns the adr
 * through a parameter.
 */

export void AdrFSpecial (sbSpc, rgTy, padr)
    char	*sbSpc;		/* name req'd, if any	*/
    TYR		rgTy[];		/* type info to return	*/
    pADRT	padr;		/* address to return	*/
{
    register int i;		/* index */

    for (i = 0; i < vispcMac; i++)		/* predefined specials */
    {
	if (FSbCmp (SbInCore (vrgTySpc[i].sbVar), sbSpc))	/* found it */
	{
	    CopyTy (rgTy, vrgTySpc + i);
	    *padr = vrgTySpc[i].valTy;
	    return;
	}
    }

    for (i = 0; i < vilvMac; i++)		/* user defined specials */
    {
	if (FSbCmp (SbInCore (vrgTyLv[i].sbVar), sbSpc))
	{
	    CopyTy (rgTy, vrgTyLv + i);
	    *padr = (ADRT) & (vrgTyLv[i].valTy);
	    return;
	}
    }

/*
 * DEFINE NEW VARIABLE:
 */

    if (vilvMac >= vilvMax)
	UError ("No room to define local \"%s\"", sbSpc);

    i = vilvMac++;


    vrgTyLv[i].sbVar = (int) Malloc (strlen (sbSpc) + 2, "AdrFSpecial");
    strcpy (vrgTyLv[i].sbVar, sbSpc);
    vrgTyLv[i].sbVar |= bitHigh;			/* see sym.h */

    CopyTy (rgTy, vrgTyLv + i);
    *padr = (ADRT) & (vrgTyLv[i].valTy);

} /* AdrFSpecial */


/***********************************************************************
 * A D R   F   R E G
 *
 * Given a desired fp and the register, walk back through the stack,
 * looking for frames that may have saved this register.  If we find one,
 * figure out the memory address of the saved register and continue.
 * Only when we reach the desired frame, after (perhaps) seeing this
 * register get saved N times, do we actually know for sure where it is!
 */

export ADRT AdrFReg (reg, fpIn, ty)
    ADRT	reg;	/* register to find	*/
    ADRT	fpIn;	/* stop at frame ptr	*/
    pTYR	ty;	/* type to return	*/
{

#define maskReg 0x1
    register int	maskSave;		/* save mask for frame	*/
    int		maskTest = maskReg << reg;	/* target frame mask	*/
    ADRT	adrRet	 = reg;			/* def: still in reg	*/
    ADRT	pc = vpc;			/* prog cntr (for call) */
    ADRT	fp;				/* current frame ptr	*/
    register int	i;			/* index		*/
    ADRT	adrStart;			/* start address of proc */
    long	saveInst;			/* reg save instruction	*/

    for (fp = GetReg(ufp); fp != fpIn; NextFrame (&fp, &pc))
    {
	if (fp == 0)
	    return adrNil;

	adrStart = AdrFLSTAdr (pc);		/* map pc -> start adr	*/
	saveInst = GetWord (adrStart + 8, spaceText);
	if ((saveInst & 0xffc00000) == 0x48c00000)  /* movem.l instr	*/
	{
	    maskSave = saveInst & 0xffff;	/* get save mask from instr */
	    if (maskSave & maskTest)		/* frame saved copy of reg */
	    {
	        ty->td.st = stValue;		/* no longer a register	   */
	        adrRet    = fp - 4;		/* base of saved registers */

	        for (i = 15; i > reg; i--)	/* look for other regs */
	        {
		    if (maskSave & (1 << i))
		        adrRet -= CBLONG;	/* saved reg takes up space */
		}
	    }
	}

    }
    return (adrRet);


} /* AdrFReg */


/***********************************************************************
 * A D R   F   F I E L D
 *
 * Given the start of a struct and its address, look up the given field
 * and return both its type and actual address (using offset info).
 * Field lookup is strictly by name; no other information about the
 * field is saved.
 */

export ADRT AdrFField (adrLong, tyStruct, tyField)
    long	adrLong;	/* start of structure	*/
    pTYR	tyStruct;	/* parent structure	*/
    register pTYR	tyField;	/* field to get adr of	*/
{
    int		iss;		/* index in string spc	 */
    pXTYR	xty = (pXTYR) (tyStruct + 1); /* ext rec */
    pXTYR	xtyField = (pXTYR) (tyField + 1); /* ext rec */


/*
 * INITIALIZE FIELD INFO:
 */

    iss = tyField[0].sbVar;		/* save the field name */
    tyField[0] = *vtyZeros;		/* wipe out field info */
    tyField[1] = *vtyZeros;
    tyField[0].sbVar = iss;		/* put the name back */
    tyField[0].td.st = tyStruct[0].td.st;
    xtyField->isymRef = isymNil;


/*
 * NOT A STRUCTURE or NO INFO:
 *
 * Don't currently do a search through other structures, so you can't
 * reference a field name not part of the appropriate struct/union.
 * Check bt before checking xty in case the XTYR is in use as a DXTYR.
 */

    if (((tyStruct->td.bt != btStruct)
     AND (tyStruct->td.bt != btUnion ))
    OR  (xty->st != stExtend))
    {
	UError ("Invalid field access:  \"%s\"", SbInCore (tyField->sbVar));
	/* may not have a valid name in tyStruct */
    }
    else

/*
 * GET FIELD INFO:
 *
 * Offset comes back in tyField->valTy.
 */

    {

	TyFFieldHp (tyField, xty->isymRef);	/* isymRef is parent struct */

/*
 * NO SUCH FIELD:
 */

	if ((tyField->td.bt == btNil) && (xtyField->st != stExtend))
	{
	    UError ("No such field name \"%s\" for that %s",
		    SbInCore (tyField->sbVar),
		    (tyStruct->td.bt == btStruct) ? "struct" : "union");
	}

    } /* else */

    tyField->valTy += tyStruct->valTy;
    adrLong += ((long) tyField->valTy) / SZCHAR;	/* avoid uns int div */
    tyField->valTy %= SZCHAR;

    return (adrLong);


/* no support for other versions */

} /* AdrFField */


/***********************************************************************
 * A D R   F   F I E L D   S Y M
 *
 * Given the address of the start of a struct and the symbol index for
 * one of its fields (must be a K_FIELD), look up the given field and
 * return both its type and actual address (using offset info).
 */

ADRT AdrFFieldSym(adrLong, tyStruct, tyField, isymField)
    long adrLong;       /* start of structure   */
    pTYR tyStruct;      /* parent structure     */
    pTYR tyField;       /* field to get adr of  */
    long isymField;     /* field's sym index    */
{
    int iss;            /* index in string spc  */
    int offset;         /* field offset         */
    int width;          /* field bit length     */
    pXTYR xtyField = (pXTYR) (tyField + 1);

    iss = tyField[0].sbVar;             /* save the field name */
    tyField[0] = *vtyZeros;             /* wipe out field info */
    tyField[1] = *vtyZeros;
    tyField[0].sbVar = iss;             /* put the name back */
    tyField[0].td.st = tyStruct[0].td.st;
    xtyField->isymRef = isymNil;

    SetSym(isymField);
    offset = vsymCur->dfield.bitoffset;
    width = vsymCur->dfield.bitlength;

    tyField->td.width = 0;

    TyFScanHp(tyField, vsymCur->dfield.type);

    tyField->valTy = offset;
    if (tyField->td.width == 0) {
	if ((offset % SZCHAR) || (width % SZCHAR))
	    tyField->td.width = width;
    }
    tyField->valTy += tyStruct->valTy;
    adrLong += ((long) tyField->valTy) / SZCHAR;
    tyField->valTy %= SZCHAR;
    return adrLong;

} /* AdrFFieldSym */


/***********************************************************************
 * A D R   F   L O C A L
 *
 * Look up a local variable on the stack (at a given depth or any depth).
 * The given ipd must be valid.	 If the procedure is not on the stack,
 * returns adrNil, but still tries to return type info if possible, e.g.
 * *ty == *vtyZeros or else it is valid.
 */

export ADRT AdrFLocal (ipd, cnt, sbVar, ty)
    int		ipd;			/* whose locals		*/
    int		cnt;			/* depth on stack or -1 */
    char	*sbVar;			/* name pattern if any	*/
    TYR		*ty;			/* type info to return	*/
{
    ADRT	fp;			/* frame and arg ptrs	*/
    ADRT	adr;			/* the resulting adr	*/

    FpFIpd (&fp, ipd, cnt);	/* find the proc, or fp == 0  */
    SetNext  (vrgPd[ipd].isym + 1);	/* prepare to search his syms */

    if (! FNextVar (K_FPARAM, K_SVAR, K_DVAR, K_CONST, sbVar))
    {
	*ty = *vtyZeros;
	return (adrNil);			/* no such local var */
    }


    TyFLocal (ty, visym);	/* must set ty first */

    if (fp == 0)
	return (adrNil);

    adr = AdrFIsym (visym, fp, ty);	/* makes all adjustments to net adr */


    return (adr);

} /* AdrFLocal */


/***********************************************************************
 * A D R   F   G L O B A L
 *
 * Look up the address and type of a global variable by name.
 */

export ADRT AdrFGlobal (sbVar, ty)
    char	*sbVar;			/* var to find	    */
    TYR		*ty;			/* return type info */
{

    SetNext (visymGlobal);

    if (FNextVar (K_SVAR, K_CONST, K_NIL, K_NIL, sbVar))
    {
	TyFGlobal (ty);
	return (AdrFIsym (visym, 0, tyNil));
    } else {
	return (adrNil);
    }


} /* AdrFGlobal */


/***********************************************************************
 * F P	 F   I P D
 *
 * Look up the given procedure on the stack at the specified depth or at
 * any depth if not specified, and return the fp and ap for it.  Returns
 * fp == 0 if it runs out of stack, but complains if a depth was given,
 * the stack is deeper than that, and the proc isn't found by the depth.
 */

export void FpFIpd (pfp, ipd, cnt)
    pADRT	pfp;		/* frame ptr to return	*/
    int		ipd;		/* ipd to find on stack */
    int		cnt;		/* stack depth or -1	*/
{
    FLAGT	fAnyDepth;	/* any depth OK? */
    register int	depth = 0;	/* current depth */
    ADRT	pc    = vpc;	/* current pc	 */

    if (fAnyDepth = (cnt == -1))
	cnt = 10000;			/* set to a LARGE value */

    *pfp = GetReg(ufp);

    for ( ; *pfp; depth++)
    {
	if ( (fAnyDepth OR (depth == cnt))	/* at any or right depth */
	AND  (IpdFAdr (pc) == ipd) )		/* hit the right proc	 */
	{
	    return;
	}

	if (depth >= cnt)
	    UError ("Procedure \"%s\" not found at stack depth %d",
		vrgPd[ipd].sbProc, cnt);

	NextFrame (pfp, &pc);
    }
} /* FpFIpd */


/***********************************************************************
 * N E X T   F R A M E
 *
 * Step to the next stack (procedure) frame, if any, and return the
 * register values.  Returns fp == 0 if already at the last frame.
 */

#ifndef CDBKDB
extern int kdb_processor;
#else /* CDBKDB */
int kdb_processor = 1;
#endif /* CDBKDB */
export void NextFrame (pfp, ppc)
    ADRT	*pfp;		/* fp to return */
    ADRT	*ppc;		/* pc to return */
{
    *ppc = GetWord (*pfp + CBINT, spaceData);
    *pfp = GetWord (*pfp, spaceData);

    if ((kdb_processor == 1) || (kdb_processor == 2)) /* 68020 or 68030 */
    {	if (*pfp & 0x10000000) *pfp = 0;
    }
    else
    {	if ((*pfp & 0x00f00000) == 0x00f00000) *pfp = 0;
    }

} /* NextFrame */


/***********************************************************************
 * L I S T   R E G S
 *
 * List register special variables (all of which are predefined only),
 * along with current values (if available).
 * There are always registers (no reason to check for
 * none as a special case).
 */

export void ListRegs ()
{
	printf("$pc %#10.8x  $ps %#10.8x\n",GetReg(upc),GetReg(ups));
	printf("$a0 %#10.8x  $a1 %#10.8x  $a2 %#10.8x  $a3 %#10.8x\n",
		GetReg(ua0),GetReg(ua1),GetReg(ua2),GetReg(ua3));
	printf("$a4 %#10.8x  $a5 %#10.8x  $a6 %#10.8x  $a7 %#10.8x\n",
		GetReg(ua4),GetReg(ua5),GetReg(ua6),GetReg(ua7));
	printf("$d0 %#10.8x  $d1 %#10.8x  $d2 %#10.8x  $d3 %#10.8x\n",
		GetReg(ud0),GetReg(ud1),GetReg(ud2),GetReg(ud3));
	printf("$d4 %#10.8x  $d5 %#10.8x  $d6 %#10.8x  $d7 %#10.8x\n",
		GetReg(ud4),GetReg(ud5),GetReg(ud6),GetReg(ud7));
}


/***********************************************************************
 * L I S T   S P E C I A L
 *
 * List non-register special variables (predefined ones first), along
 * with current values.	 Optionally select by name only.
 *
 * There are always specials; no need to check for none as a special case.
 *
 * This routine overlaps a lot with ListRegister(); they could be merged.
 */

export void ListSpecial (sbSpecial)
    char	*sbSpecial;	/* name to match */
{
    register int	i;		/* current index */
    register long	adr;		/* adr of var	 */

    /* have to handle $result as an exception because it's a register */
    if (sbSpecial == sbNil || FHdrCmp(sbSpecial, "$result"))
	printf ("%-9s = 0x%x\n", "$result", GetReg(ud0));

    for (i = 0; i < vispcMac; i++)		/* predefined specials */
    {
	if ((vrgTySpc[i].td.st == stReg)
	OR  ((sbSpecial != sbNil)
	 AND (! FHdrCmp (sbSpecial, SbInCore (vrgTySpc[i].sbVar)))))
	{
	    continue;				/* skip this one */
	}
	adr = vrgTySpc[i].valTy;
	DispVal (adr, (vrgTySpc + i), modeNil, true, true, true);
	printf ("\n");
    }

    for (i = 0; i < vilvMac ; i++)		/* user-defined specials */
    {						/* (no registers here)	 */
	if ((sbSpecial != sbNil)
	AND (! FHdrCmp (sbSpecial, SbInCore (vrgTyLv[i].sbVar))))
	{
	    continue;				/* skip this one */
	}
	adr = (long) & (vrgTyLv[i].valTy);
	DispVal (adr, (vrgTyLv + i), modeNil, true, true, true);
	printf ("\n");
    }
} /* ListSpecial */



#ifdef JUNK
/***********************************************************************
 * O P E N   S T A C K
 *
 * Set global values so the current stack frame is the one at the
 * given depth.
 */

export void OpenStack (cnt)
    int		cnt;		/* stack depth to set */
{
    ADRT	pc = vpc;	/* current values */
    ADRT	fp = GetReg(ufp);

    while ((cnt--) AND fp)		/* look for the frame */
	NextFrame (&fp, &pc);

    if (fp == 0)
	UError ("Stack isn't that deep");


    PrintPos (pc, fmtFile + fmtProc);

} /* OpenStack */
#endif /* JUNK */
