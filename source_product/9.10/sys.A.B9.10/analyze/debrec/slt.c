/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/slt.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:23:16 $
 */

/*
 * Original version based on: 
 * Revision 56.3  88/02/24  14:33:54  14:33:54  markm
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
 * These routines support accesses of the SLT (source line table) in the
 * object file.	This file should only be included for HPSYMTAB(II) compilations.
 */

#include "cdb.h"

/*
 * These are used for caching SLT entries:
 */

#define iSltCacheMax	1000			/* an experimental value */
#define cbSltCache	((long)(sizeof (vsltCache)))

long	vcbSltFirst;		/* byte offset to first SLT ent */
SLTR	vsltCache [iSltCacheMax];	/* the cache itself	*/
long	visltLo;			/* current cache low	*/
long	visltLim;			/* current cache high	*/
long	visltMax;			/* max in cache		*/

long	vislt;				/* last SLT used	*/
pSLTR	vsltCur;			/* the current entry	*/


/***********************************************************************
 * S E T   S L T   C A C H E
 *
 * Read a section of SLT so the in-memory cache begins with the given
 * SLT index, and set global values.
 */

void SetSltCache (islt)
    register long islt;			/* index to set */
{
    int		cRead;			/* bytes read	*/
    long	offset;			/* read address */

#ifdef INSTR
    vfStatProc[327]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((islt >= visltLo) AND (islt < visltLim))	/* already in memory */
	return;

    offset = vcbSltFirst + (islt * cbSLTR);

    if (lseek (vfnSym, offset, 0) < 0L)
    {
	sprintf (vsbMsgBuf, (nl_msg(535, "Internal Error (IE535) (%d)")), offset);
	Panic (vsbMsgBuf);
    }

    if ((cRead = read (vfnSym, vsltCache, cbSltCache)) < 0)
    {
	sprintf (vsbMsgBuf, (nl_msg(536, "Internal Error (IE535) (%d)")), cbSltCache);
	Panic (vsbMsgBuf);
    }

    visltLo  = islt;
    visltLim = Min (visltMax, islt + (cRead / cbSLTR));
    return;

} /* SetSltCache */


/***********************************************************************
 * S E T   S L T
 *
 * Set global SLT values to the given SLT entry.
 */

void SetSlt (islt)
    register long islt;		/* index to set */
{

#ifdef INSTR
    vfStatProc[328]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((islt < 0) OR (islt >= visltMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(537, "Internal Error (IE537) (%d)")), islt);
	Panic (vsbMsgBuf);
    }

    vislt   = islt;
    SetSltCache (islt);
    vsltCur = & (vsltCache [islt-visltLo]);

} /* SetSlt */


/***********************************************************************
 * S E T   N E X T   S L T
 *
 * Set globals to the given SLT entry preparatory to doing some searching.
 * Actually sets us to the previous entry, which is then skipped.
 */

void SetNextSlt (islt)
    register long islt;		/* index to set */
{

#ifdef INSTR
    vfStatProc[329]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((islt < 0) OR (islt >= visltMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(538, "Internal Error (IE538) (%d)")), islt);
	Panic (vsbMsgBuf);
    }

    if ((vislt = islt - 1) >= islt0)
	SetSlt (vislt);

} /* SetNextSlt */


/***********************************************************************
 * F   N E X T	 S L T
 *
 * Look forward in the SLT, starting at the NEXT entry, for one of the
 * given types, but stop at typStop.
 */

#ifdef HPSYMTABII
FLAGT FNextSlt (typHit1, typHit2, typHit3, typHit4, typHit5,
                       typStop,follow_assists)
    long	typHit1, typHit2, typHit3, typHit4, typHit5;  /* search types */
    long	typStop;			/* stop type	*/
    FLAGT       follow_assists;
#else
FLAGT FNextSlt (typHit1, typHit2, typHit3, typHit4, typHit5, typStop)
    long	typHit1, typHit2, typHit3, typHit4, typHit5;  /* search types */
    long	typStop;			/* stop type	*/
#endif
{
    register long islt = vislt + 1;		/* current loc	 */
    register long i;				/* current index */
    register long typ;				/* current type	 */

#ifdef INSTR
    vfStatProc[330]++;
#endif

    while (islt < visltMax)
    {
	SetSltCache (islt);			/* set only as needed */

	for (i = (islt - visltLo); (islt < visltLim); i++, islt++)
	{
	    vsltCur = & (vsltCache[i]);
#ifdef HPSYMTABII
            if ((vsltCur->sspec.sltdesc == SLT_ASSIST) && follow_assists)
            {				         /* encountered SLT_ASST */
                SetSlt (vsltCur->sasst.address); /* follow ptr to normal */
                islt = vislt;
                i = islt - visltLo;
            }
#endif  /* HPSYMTABII */
	    typ = vsltCur->sspec.sltdesc;

	    if ((typ == typHit1) OR (typ == typHit2) OR (typ == typHit3)
	     OR (typ == typHit4) OR (typ == typHit5))
	    {
		vislt = islt;
		return (true);
	    }
	    else if (typ == typStop)
	    {
		return (false);
	    }
	}
    }
    return (false);			/* off the end of the SLT */

} /* FNextSlt */


/***********************************************************************
 * A D R   F   I S L T
 *
 * Find the next normal SLT entry, beginning with islt, and return its
 * code address.  Don't go past the end of the current function.
 */

ADRT AdrFIslt (islt)
    long	islt;		/* index to get adr of */
{

#ifdef INSTR
    vfStatProc[331]++;
#endif

    SetNextSlt (islt);

#if HPSYMTABII
    if (FNextSlt (SLT_NORMAL, SLT_EXIT, SLT_NIL, SLT_NIL, SLT_NIL,
                  SLT_FUNCTION, true))
#else
    if (FNextSlt (SLT_NORMAL, SLT_NIL, SLT_NIL, SLT_NIL, SLT_NIL, SLT_FUNCTION))
#endif
	return (vsltCur->snorm.address);
    else
	return (adrNil);			/* can't find an address */

} /* AdrFIslt */


/***********************************************************************
 * I S L T   F	 I S Y M
 *
 * Given the index of a scope symbol, return the index of its SLT entry.
 */

long IsltFIsym (isym)
    long	isym;		/* symbol to do */
{

#ifdef INSTR
    vfStatProc[332]++;
#endif

    SetSym (isym);

    switch (vsymCur->dblock.kind)
    {

	default: 		sprintf (vsbMsgBuf, 
				    (nl_msg(539, "Internal Error (IE539) (%d, %d)")),
				    vsymCur->dblock.kind, isym);
				Panic (vsbMsgBuf);

	case K_SRCFILE:		return (vsymCur->dsfile.address );
	case K_MODULE:		return (vsymCur->dmodule.address);
	case K_FUNCTION:	return (vsymCur->dfunc.address	);
	case K_ENTRY:		return (vsymCur->dentry.address );
	case K_BEGIN:		return (vsymCur->dbegin.address );
	case K_END:		return (vsymCur->dend.address	);
	case K_LABEL:		return (vsymCur->dlabel.address );
#ifdef HPSYMTABII
	case K_WITH:		return (vsymCur->dwith.address  );
#endif
    }
} /* IsltFIsym */


/***********************************************************************
 * A D R   F   I S Y M
 *
 * Given the index of a scope symbol, return whatever address it has.
 * In the case of data addresses, take full account of indirection,
 * register types, and offsets.
 *
 * If there is no child process and a storage-type object has an indirect
 * address, returns a special adrUnknown, different than adrNil.  This
 * tells the caller that the global was found, but the address is of course
 * invalid.  However, anyone trying to use it should run into "no child
 * process" before "bad access".
 */

#ifdef SPECTRUM
ADRT AdrFIsym (isym, fp, ap, pc, rgTy)
#else
ADRT AdrFIsym (isym, fp, rgTy)
#endif
    long	isym;			/* symbol to get data from	*/
    ADRT	fp;			/* frame ptr for fp-rel symbols */
#ifdef SPECTRUM
    ADRT	ap;			/* Arg ptr for ap-rel symbols   */
    ADRT	pc;			/* needed for spectrum NPC      */
#endif
    pTYR	rgTy;			/* to possibly modify here	*/
{
    ADRT	adr;			/* working value	*/
    FLAGT	fIndir	= false;	/* adr is indirect	*/
    long	offset	= 0;		/* after indirection	*/
    FLAGT	fReg	= false;	/* actually register	*/
    FLAGT	fUseFp	= false;	/* adr is fp-relative	*/
#ifdef SPECTRUM
    FLAGT	fCopy	= false;	/* parm copied to local */
    pSUR        psu;
#endif
    union {
	double	doub;
	float	fl[2];
	long	lng[2];
	char	ch[8];
	}	stuff;
    register long inx;				/* index		*/
    pDXTYR	dxty = (pDXTYR) (rgTy + 1);	/* store double const here */
    register char *pVTData;			/* ptr to data in VT	*/
#ifdef SPECTRUM
    FLAGT	fUseAp	= false;	/* adr is ap-relative	*/
    FLAGT	fUseReg	= false;	/* adr is reg-relative	*/
#endif

#ifdef INSTR
    vfStatProc[333]++;
#endif

    SetSym (isym);

    switch (vsymCur->dblock.kind)
    {

	default: 		sprintf (vsbMsgBuf, 
				    (nl_msg(540, "Internal Error (IE540) (%d, %d)")),
				    vsymCur->dblock.kind, isym);
				Panic (vsbMsgBuf);

/*
 * LOCATION IS THE ADDRESS OF THE NEXT NORMAL LINE after the matching SLT entry:
 * Note that we skip the matching SLT entry itself, to avoid confusion.
 */

	case K_SRCFILE:
	case K_MODULE:
	case K_FUNCTION:
	case K_ENTRY:
	case K_BEGIN:
	case K_END:
#ifdef HPSYMTABII
	case K_WITH:
#endif
	case K_LABEL:	return (AdrFIslt (IsltFIsym (isym) + 1));

/*
 * LOCATION IS IN SYMBOL ITSELF:
 */

	case K_FPARAM:	adr	= vsymCur->dfparam.location;
#ifdef HPSYMTAB
			fIndir	= vsymCur->dfparam.varparam;
#else	/* HPSYMTABII */
			fIndir	= vsymCur->dfparam.indirect;
                        fCopy   = vsymCur->dfparam.copyparam;
#endif	/* HPSYMTABII */
			fReg	= vsymCur->dfparam.regparam;
#ifdef SPECTRUM
/* In the new calling convention, addr of a param is relative to the previous
   SP. For stack unwind this requires that the pc is passed in!
*/
                        if (!fCopy) {
                           NextFrame(&fp,&ap,&pc); 
                        } 
                        fUseAp	= ! fReg;		/* one or the other */
#else
			fUseFp	= ! fReg;		/* one or the other */
#endif
			break;

	case K_SVAR:	adr	= vsymCur->dsvar.location;
#ifdef HPE
			adr    += (vdp - vipreDp);	/* HPE correction   */
#endif
			fIndir	= vsymCur->dsvar.indirect;
			offset	= vsymCur->dsvar.offset;
			break;

	case K_DVAR:	adr	= vsymCur->ddvar.location;
			fIndir	= vsymCur->ddvar.indirect;
			fReg	= vsymCur->ddvar.regvar;
			fUseFp	= ! fReg;		/* one or the other */
			break;

	case K_MEMENUM:	adr	= vsymCur->dmember.value;
			fIndir	= false;
			offset	= 0;
			break;

	case K_CONST:	adr	= vsymCur->dconst.location;
			fIndir	= vsymCur->dconst.indirect;
			offset	= vsymCur->dconst.offset;

			if (vsymCur->dconst.locdesc == LOC_VT)
			{
			  if (vsymCur->dconst.type.dntti.immediate)
			  {

			    switch (vsymCur->dconst.type.dntti.type)
			    {
			    default:
				    sprintf (vsbMsgBuf,
					(nl_msg(541, "Internal Error (IE541) (%d)")), visym);
				    Panic (vsbMsgBuf);

                            case T_INT:
				    pVTData = VTInCore (
					    vsymCur->dconst.location,
				    	    vsymCur->dconst.type.dntti.bitlength
						/ SZCHAR);
				    if (vsymCur->dconst.type.dntti.bitlength
						== 32)
				    {
					for (inx = 0; inx < 4; inx++)
					    stuff.ch[inx] = *(pVTData + inx);
					adr = stuff.lng[0];
				    }
				    else /* assume 16 bits */
				    {
					for (inx = 0; inx < 2; inx++)
					    stuff.ch[inx+2] = *(pVTData+inx);
					adr = stuff.lng[0];
				    }
				    break;

			    case T_REAL:
				    pVTData = VTInCore (
					    vsymCur->dconst.location,
				    	    vsymCur->dconst.type.dntti.bitlength
						/ SZCHAR);
				    if (vsymCur->dconst.type.dntti.bitlength
						== 32)
				    {
					for (inx = 0; inx < 4; inx++)
					    stuff.ch[inx] = *(pVTData + inx);
					adr = stuff.lng[0];
				    }
				    else
				    {
					for (inx = 0; inx < 8; inx++)
					    stuff.ch[inx] = *(pVTData + inx);
					dxty->doub = stuff.doub;
				    }
				    break;

#ifdef HPSYMTABII
#ifdef NOTDEF
                            case T_MOD_STRING_SPEC:
				        pVTData = SbInCore(vsymCur->dconst.location);
					adr = AdrFStoreModcalString (pVTData,
							 strlen (pVTData) + 1);
#endif NOTDEF
				    break;

                            case T_FTN_STRING_SPEC:
#else
			    case T_STRING500:
				    if (vsymCur->dconst.type.dntti.bitlength
					== 0)  	/* FORTRAN PARAMETER string */
#endif
				    {
#ifdef NOTDEF
					rgTy->td.bt = btChar;
					AdjTd (rgTy, tqArray, isym);
				        pVTData = SbInCore (vsymCur->dconst.location);
					adr = AdrFStore (pVTData,
							 strlen (pVTData) + 1);
#else
					printf("AdrFStore STRING500\n");
#endif NOTDEF
				    }
				    break;

#ifdef S200
			    case T_STRING200:  /* Pascal STRING */
			    case T_LONGSTRING200:
				    {
					long cb = vsymCur->dconst.type.dntti.bitlength / SZCHAR;
				        pVTData = VTInCore (
						  vsymCur->dconst.location, cb);
				        adr = AdrFStore (pVTData, cb);
				    }
				    break;
#endif
			    }
			  }
			  else  /* nonimmediate type -- the only types we  */
				/* handle are Pascal packed arrays of char */
				/* and s200 Pascal sets */
			  {
			    SetSym (vsymCur->dconst.type.dnttp.index);
			    while (vsymCur->dblock.kind == K_TYPEDEF)
			        SetSym (vsymCur->dtype.type.dnttp.index);
			    if ((vsymCur->dblock.kind == K_ARRAY)
			    AND (vsymCur->darray.elemtype.dntti.immediate)
			    AND (vsymCur->darray.elemtype.dntti.type == T_CHAR))
			    {
#ifdef HPSYMTAB
				long cb = vsymCur->darray.bitlength / SZCHAR;
#else  /* HPSYMTABII */
				long cb = vsymCur->darray.arraylength / SZCHAR;
#endif /* HPSYMTABII */
				pVTData = VTInCore (adr, cb);
#ifdef NOTDEF
				adr = AdrFStore (pVTData, cb);
#else
				printf("AdrFStore called in slt.c!!!!\n");
#endif
			    }
#ifdef S200
			    /*  S200 Pascal Sets */
			    else if (vsymCur->dblock.kind == K_SET)
			    {
			      /* get length byte, then ensure entire data */
			      /* item in buffer, then copy to child buffer */
				long cb;	/* size of set in bytes */
				pVTData = VTInCore (adr, 2);
				stuff.lng[0] = 0;	/* zero it out */
				stuff.ch[2] = *pVTData;
				stuff.ch[3] = *(pVTData + 1);
				cb = stuff.lng[0] + 2;	/* data + count */
				pVTData = VTInCore (adr, cb);
				adr = AdrFStore (pVTData, cb); /* copy to child */
								/* process */
			    }
#endif
			    else
			    {
			        sprintf (vsbMsgBuf,
				    (nl_msg(541, "Internal Error (IE541) (%d)")), visym);
				Panic (vsbMsgBuf);
			    }
			  }
			}
			break;
    } /* switch */

/*
 * HAVE A DATA ADDRESS that may need more work, per the values set above:
 */

#ifdef SPECTRUM
    if (fUseAp)
    {
	if (ap == 0)				/* simple, inaccurate test */
        {
	    sprintf (vsbMsgBuf, (nl_msg(542, "Internal Error (IE542) (%d)")), isym);
	    Panic (vsbMsgBuf);
        }
	adr += ap;				/* it was ap-relative */
    }
#endif
    if (fUseFp)
    {
	if (fp == 0)				/* simple, inaccurate test */
	{
	    sprintf (vsbMsgBuf, (nl_msg(543, "Internal Error (IE543) (%d)")), isym);
	    Panic (vsbMsgBuf);
	}
	adr += fp;				/* it was fp-relative */
    }
    if (fReg)
    {
	adr = AdrFReg (adr, fp, rgTy);		/* finds register values */
    }
    if (fIndir)
    {
	ADRT	adrT = adr;

	if ((vpid == pidNil)			/* no child process */
	AND (vfnCore == fnNil))			/* and no core file  */
	    return (adrUnknown);		/* prevent bombout  */

	GetBlock (adrT, spaceData, &adr, 4);	/* replace adr with contents */

#ifdef FOCUS
	adr = AdrFAdr (adr, false, ptrData);	/* break as ptrData if needed */
#endif

    }
    return (adr + offset);

} /* AdrFIsym */
