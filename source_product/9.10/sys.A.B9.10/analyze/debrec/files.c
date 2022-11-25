/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/files.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:22:39 $
 */

/*
 * Original version based on:
 * Revision 63.6  88/05/27  09:48:02  09:48:02  markm
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
 * These routines manage sourcefile access and related mappings, listings,
 * searches, etc., including mapping to/from procedure descriptors.
 */

#include "cdb.h"


FILE	*vfpSrc;	/* source file of current interest	*/
/* int	vfnDecl;	   declaration file of current interest */

int	vcbMalloc;	/* number bytes malloc'd at present	*/
char	*vsbSbrkFirst;	/* initial return from sbrk (0)		*/

static  short vidirMax;		/* current maximum		*/
static  short vidirDlta;	/* expansion increment		*/
static  char **vrgSbDir;	/* a list of directory names	*/
static  short vidirMac;		/* the number in use		*/



#ifdef HPSYMTABII
/***********************************************************************
 * I M D   F   I P D
 *
 * Given a proc descriptor, look up which module it's in.
 */

int ImdFIpd (ipd)
    register long ipd;		/* ipd to map */
{
    register long imd;
    register long imdLow;
    register long imdHi;
    register long adr;

#ifdef INSTR
    vfStatProc[106]++;
#endif

    if (ipd == ipdNil)
	return (imdNil);

    adr = vrgPd[ipd].adrStart;

    imdLow = 0;
    imdHi = vimdMac - 1;
    while (imdLow <= imdHi)
    {
	imd = imdLow + ((imdHi - imdLow) >> 1);	/* split interval */
	if (adr < vrgMd[imd].adrStart)		/* take lower half */
	{
	    imdHi = imd - 1;
	}
	else if (adr > vrgMd[imd].adrEnd)	/* take upper half */
	{
	    imdLow = imd + 1;
	}
	else					/* found it */
	{
	    return (imd);
	}
    }

    return (imdNil);

} /* ImdFIpd */
#endif /* HPSYMTABII */


/***********************************************************************
 * I P D   F   A D R
 *
 * Given an address, look up which procedure it falls in.
 * Checks the current procedure first (if any), then the whole list.
 */

int IpdFAdr (adr)
    register ADRT adr;		/* address to map */
{
    register long ipd;		/* resulting ipd */
    register long ipdLow;	/* low bound of binary search */
    register long ipdHi;	/* upper bound of binary search */

#ifdef INSTR
    vfStatProc[108]++;
#endif

    if ((vipd != ipdNil)
    AND (vrgPd[vipd].adrStart <= adr)		/* in lower bound */
    AND (vrgPd[vipd].adrEnd > adr))		/* in higher bound */
    {
	return (vipd);
    }

    ipdLow = 0;
    ipdHi = vipdMac - 1;
    while (ipdLow <= ipdHi)
    {
	ipd = ipdLow + ((ipdHi - ipdLow) >> 1);	/* split interval */
	if (adr < vrgPd[ipd].adrStart)		/* take lower half */
	{
	    ipdHi = ipd - 1;
	}
	else if (adr > vrgPd[ipd].adrEnd)	/* take upper half */
	{
	    if (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd)  /* FORTRAN ENTRY */
	    {
		register long ipdnew = ipd - 1;
		while (vrgPd[ipdnew].adrStart == vrgPd[ipdnew].adrEnd)
		    ipdnew--;
		if (adr > vrgPd[ipdnew].adrEnd)	/* nope, not in function */
		{
		    do
			ipd++;
		    while (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd);
		    ipdLow = ipd;
		}
		else				/* yep, in this function */
		{
		    return (ipdnew);
		}
	    }
	    else				/* not FORTRAN ENTRY */
	    {
	        ipdLow = ipd + 1;
	    }
	}
	else					/* found it */
	{
	    return (ipd);
	}
    }
    return (ipdNil);				/* not found */

} /* IpdFAdr */


/***********************************************************************
 * I P D   F   N A M E
 *
 * Given a proc name, look up which procedure descriptor it belongs to.
 * Checks the current procedure first (if any), then the whole list.
 * For symbol tables with short names, it makes another pass if
 * necessary, looking for cases where the symbol table name is a
 * max-length substring of the given name.
 */

int IpdFName (sbProc)
    register char *sbProc;	/* name to look up */
{
    register long ipd;		/* resulting ipd */

#ifdef INSTR
    vfStatProc[109]++;
#endif

    if ((vipd != ipdNil)
    AND (FSbCmp (vrgPd[vipd].sbProc,  sbProc)
      OR FSbCmp (vrgPd[vipd].sbAlias, sbProc)))		/* also check alias */
    {
	return (vipd);
    }

    for (ipd = 0; ipd < vipdMac; ipd++)			/* try for exact */

	if (FSbCmp (vrgPd[ipd].sbProc,  sbProc)
	OR  FSbCmp (vrgPd[ipd].sbAlias, sbProc))	/* also check alias */
	{
	    return (ipd);
	}

    return (ipdNil);				/* failed */

} /* IpdFName */



/***********************************************************************
 * I P D   F   E N T R Y   I P D
 *
 * Find the ipd of the function containing the entry ipd
 */

int IpdFEntryIpd (ipdEntry)
    int		ipdEntry;		/* entry ipd	*/
{
    register long ipd;			/* index for searching pd table */

#ifdef INSTR
    vfStatProc[111]++;
#endif

    ipd = ipdEntry;
    while (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd)
	ipd--;

    return (ipd);

} /* IpdFEntryIpd */


/***********************************************************************
 * P R I N T   P O S
 *
 * Print the current position and/or set current location.  If adr ==
 * adrNil, assumes the caller has already set globals for the current
 * location, else it sets them up here using the given adr (temporarily
 * or permanently, depending on whether fmtSave is given).  Makes no
 * assumptions about known file <==> known proc, e.g. either might be
 * unknown independent of the other.
 *
 * fmtNil causes all other formats to be ignored, so it makes no sense
 * to call with both fmtNil and adr == adrNil.
 *
 * fmtFile causes the file name to be printed, if known.
 * fmtProc causes the proc name to be printed, if known.
 * fmtNil NOT given causes the line number, slop (if any), and line
 * contents to be printed, if known, plus a trailing newline.
 */

void PrintPos (adr, fmt)
    ADRT	adr;			/* code loc to set or adrNil */
    int		fmt;			/* flag word: parts to print */
{
    int		ifd;			/* what file we're in	*/
    int		iln;			/* what line we're in	*/
    int		slop;			/* from start of line	*/

    int		ifdSave;		/* save while printing	*/
    int		ipdSave;
    int		ilnSave;
    int		slopSave;
    long	adrCurSave;

#ifdef INSTR
    vfStatProc[120]++;
#endif

/*
 * SAVE CURRENT LOCATION:
 */

#ifndef NOTDEF
	printf("PrintPos called : check into \n");
#else

#endif NOTDEF

} /* PrintPos */
