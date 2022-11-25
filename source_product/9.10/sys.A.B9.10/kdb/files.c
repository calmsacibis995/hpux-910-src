/* @(#) $Revision: 66.2 $ */   
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines manage sourcefile access and related mappings, listings,
 * searches, etc., including mapping to/from procedure descriptors.
 */

#include "cdb.h"
#include "macdefs.h"

/***********************************************************************
 * I M D   F   I P D
 *
 * Given a proc descriptor, look up which module it's in.
 */

export int ImdFIpd (ipd)
    register long ipd;		/* ipd to map */
{
    register long imd;
    register long imdLow;
    register long imdHi;
    register long adr;

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


/***********************************************************************
 * I P D   F   A D R
 *
 * Given an address, look up which procedure it falls in.
 * Checks the current procedure first (if any), then the whole list.
 */

export int IpdFAdr (adr)
    register ADRT	adr;	/* address to map */
{
    register int	ipd;	/* resulting ipd */

    if ((vipd != ipdNil)
    AND (vrgPd[vipd].adrStart <= adr)		/* in lower bound */
    AND (vrgPd[vipd].adrEnd >= adr))		/* in higher bound */
    {
	return (vipd);
    }

    for (ipd = 0; ipd < vipdMac; ipd++)
    {
	if ((vrgPd[ipd].adrStart <= adr) AND
	    (vrgPd[ipd].adrEnd	 >= adr))	/* simple bounds check */
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
 */

export int IpdFName (sbProc)
    char	*sbProc;	/* name to look up */
{
    register int	ipd;	/* resulting ipd */

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
 * O P E N   I P D
 *
 * Open the sourcefile associated with a procedure and set global values.
 */

export void OpenIpd (ipd)
    register int	ipd;	/* procedure to open		    */
{
    if (ipd == ipdNil)				/* set to nothing */
    {
	vipd = ipdNil;
	return;					/* nothing else needed */
    }

    if (ipd == vipd)		/* already open */
	return;

    IfdFOpen (IfdFAdr (vrgPd[ipd].adrStart));	/* open parent file */

    viln = IlnFIsym (vrgPd[ipd].isym);		/* find ln from this isym */

    if (viln == ilnNil)
    {
	printf ("WARNING:  \"%s\" does not appear to have line symbols\n",
			vrgFd[vifd].sbFile);
    }
    vipd  = ipd;
    vslop = 0;

} /* OpenIpd */


/***********************************************************************
 * I F D   F   O P E N
 *
 * Given a file descriptor, open the sourcefile, set global values
 * (including default ln and slop), and return the ifd for the file
 * (or ifdNil):
 */

export int IfdFOpen (ifd)
    register int	ifd;	/* ifd	of file to open */
{
    if ((ifd < 0) OR (ifd >= vifdMac))		/* open nothing */
    {
	vifd = ifdNil;
	return (ifdNil);
    }

/*
 * If a KNOWN FILE IS ALREADY OPEN, no action is required:
 */

    if (ifd == vifd)
	return (ifd);

/*
 * SET PTR AND DEFAULT FILENAME, THEN OPEN THE NEW FILE and set global values:
 */

    vifd   = ifd;
    vipd   = vrgFd[ifd].ipd;
    viln   = 1;				/* caller changes if bad default */
    vslop  = 0;
    return (ifd);

} /* IfdFOpen */


/***********************************************************************
 * I F D   F   A D R
 *
 * Given an address, look up which sourcefile it falls in.
 * Checks the current file first (if any), then the whole list.
 */

export int IfdFAdr (adr)
    register ADRT	adr;	/* address to map */
{
    register int	ifd;	/* resulting ifd */

    if ((vifd != ifdNil)
    AND (vrgFd[vifd].adrStart <= adr)		/* in lower bound */
    AND (vrgFd[vifd].adrEnd >= adr))		/* in upper bound */
    {
	return (vifd);
    }

    for (ifd = 0; ifd < vifdMac; ifd++)
    {
	if ((vrgFd[ifd].adrStart <= adr) AND
	    (vrgFd[ifd].adrEnd	 >= adr))	/* simple bounds check */
	{
	    return (ifd);
	}
    }
    return (ifdNil);			/* failed */

} /* IfdFAdr */


#ifdef JUNK
/***********************************************************************
 * I F D   F   N A M E
 *
 * Given a filename, look up which sourcefile descriptor it belongs to.
 * Checks the current file first (if any), then the whole list.
 * For symbol tables with short names, it makes another pass if
 * necessary, looking for cases where the symbol table name is a
 * max-length substring of the given name.
 */

export int IfdFName (sbFile)
    char	*sbFile;	/* filename to find */
{
    register int	ifd;	/* resulting ifd */

    if ((vifd != ifdNil) AND FSbCmp (vrgFd[vifd].sbFile, sbFile))
	return (vifd);

    for (ifd = 0; ifd < vifdMac; ifd++)			/* try for exact */
	if (FSbCmp (vrgFd[ifd].sbFile, sbFile))
	    return (ifd);


    return (ifdNil);			/* failed */

} /* IfdFName */
#endif /* JUNK */


/***********************************************************************
 * I F D   F   I P D
 *
 * Given a proc descriptor, look up which file it's in.
 */

export int IfdFIpd (ipd)
    register int	ipd;		/* ipd to map */
{
    register pFDR	fd  = vrgFd;	/* fd to check	 */
    register int	ifd = 0;	/* resulting ifd */

    if (ipd == ipdNil)
	return (ifdNil);

    for ( ; ifd < vifdMac; fd++, ifd++)
	if (fd->ipd > ipd)			/* passed it	*/
	    return (--ifd);			/* prev owns it */

    return (--ifd);				/* last owns it */

} /* IfdFIpd */


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

export void PrintPos (adr, fmt)
    register ADRT	adr;		/* code loc to set or adrNil */
    register int	fmt;		/* flag word: parts to print */
{
    int		ifd;			/* what file we're in	*/
    int		iln;			/* what line we're in	*/
    int		slop;			/* from start of line	*/

    int		ifdSave;		/* save while printing	*/
    int		ipdSave;
    int		ilnSave;
    int		slopSave;

/*
 * SAVE CURRENT LOCATION:
 */

    if (fmt & fmtSave)
    {
	ifdSave	 = vifd;
	ipdSave	 = vipd;
	ilnSave	 = viln;
	slopSave = vslop;
    }

/*
 * SET NEW LOCATION:
 */

    if (adr != adrNil)					/* not already done */
    {
	IfdLnFAdr (adr, isym0, &ifd, &iln, &slop);

	if (ifd != ifdNil)				/* file known */
	{
	    IfdFOpen (ifd);				/* open file */
	    OpenIpd  (IpdFAdr (adr), false);		/* sets vipd */
	    viln  = iln;
	    vslop = slop;
	}
	else						/* file unknown */
	{
	    vifd  = ifdNil;
	    vipd  = ipdNil;
	    viln  = ilnNil;
	    vslop = 0;
	}
    }

/*
 * QUIT (JUST WANTED SET):
 */

    if (fmt == fmtNil)
	return;

/*
 * PRINT FILE NAME:
 */

    if (fmt & fmtFile)
    {
	if (vifd != ifdNil)				/* file known */
	    printf ("%s: ", vrgFd[vifd].sbFile);
	else
	    printf ("(file unknown): ");
    }

/*
 * PRINT PROCEDURE NAME:
 */

    if (fmt & fmtProc)
    {
	if (vipd != ipdNil)				/* proc known */
	{
	    printf ("%s: ", vrgPd[vipd].sbProc);
	}
	else
	{


	    char	*sbLabel;
	    long	offset;

	    LabelFAdr (adr, &sbLabel, &offset);	  /* returns values directly */
	    printf ("%s", sbLabel);

	    if (offset)
		printf (" +%#lx", offset);


	    printf (": ");

	} /* if */
    } /* if */

/*
 * PRINT LINE NUMBER AND CONTENTS:
 */

    if (viln == ilnNil)					/* line unknown */
	printf ("(line unknown)\n");
    else						/* line known */
    {
	printf ("%d", viln);				/* minimum width */

	if (vslop != 0)					/* not exact hit */
	    printf (" +%#lx", lengthen (vslop));

	printf("\n");
    }

/*
 * RESET GLOBAL VALUES if no changes desired:
 */

    if (fmt & fmtSave)
    {
	vifd  = ifdNil;
	IfdFOpen (ifdSave);
	vipd  = ipdSave;
	viln  = ilnSave;
	vslop = slopSave;
    }

} /* PrintPos */
