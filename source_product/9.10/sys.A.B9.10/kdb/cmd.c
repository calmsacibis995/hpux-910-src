/* @(#) $Revision: 70.2 $ */     
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines do command line processing.  The monster FDoCommand()
 * reads tokens and branches off to appropriate commands.
 */

#include <ctype.h>
#include "macdefs.h"
#include "cdb.h"

export	char	*vsbCmd = sbNil;	/* command line pointer	*/
export	char	vsbCmdBuf[512];		/* command line itself	*/

export	long	vdot;			/* current data address */
export	TYR	vtyDot[cTyMax];		/* dot's type info	*/

export	int	viln;			/* source line number	*/
export	int	vslop;			/* byte offset from source line start */

local	CMDE	vcmdDef;		/* last cmd, for repeat */

#ifdef KASSERT
local	char	*sbErrAllow = "\"%c\" not allowed while running assertions";
#endif /* KASSERT */


/***********************************************************************
 * E A T   C M D   L I S T
 *
 * Return the next block (commands in "{}") or the rest of the line.
 * If the commands are in "{}", returns a pointer to the start of the
 * next token after the "{", and removes the "}", if any.
 * Leaves vsbCmd set past the "}" or to sbNil if we used the whole line.
 */

export char * EatCmdList()
{
    register char	*sbCmd;			/* command line to do	*/
    register char	*sbTemp;		/* one-block portion	*/

    if (TkPeek() == tkNil)			/* no more tokens	*/
	return (sbNil);

    if (vtkPeek != tkLCB)			/* not coming to block	*/
    {
	sbCmd  = vsbCmd;			/* we'll use whole line */
	vsbCmd = sbNil;				/* so eat it up		*/
    }
    else
    {
	TkNext();				/* eat "{"		*/
	sbCmd  = vsbCmd;			/* start past it	*/
	sbTemp = SbFEob (vsbCmd) - 1;		/* find end of block	*/
	if (*sbTemp == '}')			/* yup, "}" is there	*/
	    *sbTemp = chNull;			/* mark off the block	*/
	vsbCmd = sbTemp + 1;			/* save rest of line	*/
    }
    while ((*sbCmd == ' ') OR (*sbCmd == '\t')) /* skip white space	*/
	sbCmd++;

    return (sbCmd);

} /* EatCmdList */


/***********************************************************************
 * E A T   F I L E N A M E
 *
 * Return what's probably a filename and march past it.
 * Doesn't alter vsbTok.
 */

local void EatFilename (sbName, psbIn)
    char	*sbName;		/* where to save the filename */
    char	**psbIn;		/* command line in, to change */
{
    register char	*sbIn = *psbIn;		/* where in line now */
    register int	cch;			/* chars in filename */

    while ((*sbIn == ' ') OR (*sbIn == '\t'))	/* skip white space */
	sbIn++;

    cch = strcspn (sbIn, "\t ;}");	/* size up to terminator */
    strncpy (sbName, sbIn, cch);	/* save it	*/
    sbName[cch] = chNull;		/* terminate it */
    *psbIn = sbIn + cch;		/* pass it	*/

} /* EatFilename */


/***********************************************************************
 * S B	 F   E O B
 *
 * Return a pointer to after "}" or to the end of the line.
 * Ignores "}" inside '' or "" quote marks.
 */

export char * SbFEob (sb)
    char	*sb;			/* string to examine */
{
    register char  ch;			/* current char in sb		*/
    register int   cNest = 1;		/* nesting depth; start nested	*/
    register FLAGT  fSQuote = false;	/* inside single quotes?	*/
    register FLAGT  fDQuote = false;	/* inside double quotes?	*/

    if (*sb == '{')				/* start nested 1 not 2 */
	sb++;

    while (ch = ChFEscape (& sb))		/* advances but not past null */
    {
	if ((ch == '\'') AND (! fDQuote))	/* start/end single quote */
	{
	    fSQuote = ! fSQuote;
	    continue;
	}
	if ((ch == '"') AND (! fSQuote))	/* start/end double quote */
	{
	    fDQuote = ! fDQuote;
	    continue;
	}

	if (fSQuote OR fDQuote)			/* ignore "}" in quotes */
	    continue;

	if (ch == '{')					/* go deeper */
	    cNest++;
	else if ((ch == '}') AND (--cNest == 0))	/* found the end */
	    break;
    }
    return (sb);

} /* SbFEob */


/***********************************************************************
 * F   D O   C O M M A N D
 *
 * This monster tokenizes and processes commands from a command line.
 * It returns when out of commands (returns the same fTop given), or
 * after an "x" command (returns true), or after a "c" command if it
 * appears that a command line procedure call exited (returns false,
 * so DoProc() gets control back).  It's a big loop through all tokens
 * on the command line.
 *
 * Each command (all or part of a line) sets vcmdDef (default command
 * for repeat) as a side-effect, or leaves it unchanged.
 */

export FLAGT FDoCommand (sbCmdIn)
    char	*sbCmdIn;			/* command line input	*/
{
    char	*sbCmds;			/* all valid commands	*/
    char	*sbNoMod;			/* non-modifiable cmds	*/
    register char	chCmd;			/* command character	*/
    char	chNext;				/* second char of cmd	*/

    FLAGT	fCommand;			/* command or expr?	*/
    FLAGT	fDidit;				/* did part of command? */
    FLAGT	fHaveValue;			/* cmd starts with val? */
    FLAGT	fAteExpr;			/* ate right-side expr?	*/
    int		valExpr;			/* value of expr (int!)	*/
    long	adrLong;			/* expr adr (if valid)	*/
    ADRT	adr;				/* code address		*/

    TYR		rgTy[cTyMax];			/* type info for expr	*/
    char	sbName[80];			/* file or proc name	*/
    register char	*sbTemp;		/* portion of cmd line	*/
    pTYR	ty;				/* for right-side expr	*/
#ifdef KASSERT
    ASE		as;				/* for modify assertion */
#endif /* KASSERT */
    int		depth;				/* for set breakpoint	*/
    FLAGT	fDoIt;				/* for "if" command	*/
    int		ipd, iln;			/* for set viewing loc	*/
    int		cnt;				/* temp for random use	*/
    typedef int (*PFI)();

#define	NJPARS	10
    int		jpars[NJPARS];			/* parameters for J command */

/*
 * These are in the same order as the cases below:
 */

#ifdef KASSERT
#ifdef CMD_EDIT
    sbCmds     = "\n~^/?AaBbCcDdehIiJ{;}lQRrSstUux";
#else
    sbCmds     = "\n~^/?AaBbCcDdeIiJ{;}lQRrSstUux";
#endif
    sbNoMod = "^ABDeIiJ{lQ";

#else /* not KASSERT */
#ifdef CMD_EDIT
    sbCmds     = "\n~^/?BbCcDdehIiJ{;}lQRrSstUux";
#else
    sbCmds     = "\n~^/?BbCcDdeIiJ{;}lQRrSstUux";
#endif
    sbNoMod = "^BDeIiJ{lQ";

#endif

/*
 * SET UP THE COMMAND LINE:
 *
 * Report if any old commands are tossed (for whatever reason).
 */

    if ((vsbCmd != sbNil) AND (*vsbCmd != chNull))	/* was more to do */
	    printf (vsbCmdsIgnored, vsbCmd);		/* notify user	  */

    vsbCmd = strcpy (vsbCmdBuf, sbCmdIn);	/* prepare given line	*/
    TkNext();					/* prepare first token	*/

/*
 * DO ONE COMMAND in big loop:
 *
 * Handle special case commands first.
 */

    do					/* have to get in once even if tkNil */
    {
/*
 * PARSE AN EXPRESSION OR A COMMAND?
 *
 * Start each loop by setting some values.
 * Then assume the token is a command if it's a single letter in the command
 * set, or a multi-letter command, or if it might be part of an expression.
 */

	fCommand   = false;			/* don't have command	 */
	fHaveValue = false;			/* no leading expr value */
	fAteExpr   = false;			/* no right-side expr	 */
	chCmd	   = vsbTok[0];			/* first letter to test	 */

	if (vtk == tkStrConstant)		/* STRING CONSTANT */
	{
	    printf ("%s", vsbTok);		/* just show it */
	    vcmdDef = cmdNil;
	    goto Nextcmd;			/* next cmd -- end of big do */
	}

	if (vtk == tkNil)
	{
	    fCommand = true;			 /* nil line is a cmd too     */
	}
	else if ((strchr (sbCmds, chCmd) != 0)
	     AND (vtk != tkCharConstant))	/* first char looks like cmd */
	{
	    if (vcbTok == 1)				/* just one char */
	    {
		fCommand = true;
	    }
	    else if ((vtk == tkStr) AND (vcbTok == 2))	/* longer command? */ 
	    {
		chNext = vsbTok[1];

#ifdef KASSERT
		fCommand = ((chCmd == 'a') AND strchr ("ads",	     chNext))
			OR ((chCmd == 'b') AND strchr ("AaUu", chNext))
			OR ((chCmd == 'D') AND strchr ("ab",	     chNext))
			OR ((chCmd == 'i') AND (chNext == 'f'))
			OR ((chCmd == 'l') AND strchr ("abfglprstx", chNext));
#else /* not KASSERT */
		fCommand = ((chCmd == 'b') AND strchr ("AaUu", chNext))
			OR ((chCmd == 'i') AND (chNext == 'f'))
			OR ((chCmd == 'l') AND strchr ("bfglprstx", chNext));
#endif
	    }		/* 'z' command must have a following space */

	    TkPeek();			/* final chance to change our minds */

	    if ( (vtk == tkStr)				/* old is a string  */
	    AND	 (*vsbCmd != ' ')			/* not separated    */
	    AND	 (vsbPeek[0] != chNull)			/* not end of line  */
	    AND	 (! strchr (";}", vsbPeek[0])) )	/* not ';' or '}'   */
	    {
		fCommand = false;			/* not a command    */
	    }
	} /* if */

/*
 * ASSUME BEGINNING OF AN EXPRESSION:
 *
 * Set our current proc and evaluate the expression.  If this succeeds and
 * they're not just asking for an address, get, save, and note the expr value.
 * Avoids using GetExpr() so it can avoid ValFAdrTy() if not needed.
 */

	if (! fCommand)
	{
	    vipd = IpdFAdr (AdrFIfdLn (vifd, viln));	/* set current proc */

	    if ((TkFExpr (& adrLong, rgTy)) == tkNil) /* eval failed */
	    {
		UError ("Unknown name or command \"%s\"", vsbTok);
	    }
	    else
	    {
		if ((vsbTok[0] != '/') AND (vsbTok[0] != '?'))
		{
		    /* avoid this if might only want type or address */
		    STUFFU stuff;

		    /* use base size; cast float to long */
		    ValFAdrTy (& stuff, adrLong, rgTy, 0, true, false);
		    valExpr = stuff.lng;
		}

		fHaveValue = true;			/* we DO have a value */
	    }
	}

/*
 * HANDLE ALL COMMANDS in a huge switch:
 *
 * Note well:  Current token and related values are set to the command itself.
 */

	chCmd = vsbTok[0];

	if (fHaveValue AND strchr (sbNoMod, chCmd) AND (chCmd != chNull))
	{
	    UError ("Modifier is not allowed before \"%c\" command", chCmd);
	}

	switch (chCmd)
	{
	    default:					/* INVALID COMMAND */
		    vcmdDef = cmdNil;
		    UError ("Unknown command \"%c\" (%#o)\n", chCmd, chCmd);

/*
 * DEFAULT COMMAND:
 *
 * Repeat vcmdDef (and leave it unchanged), or handle a naked expression.
 * chNull only happens if command line was all blanks or tabs (but not empty).
 * <newline> is not expected but we handle it anyway.
 *
 * This is where we SHOULD handle "<expr>;" and "<expr>}", but we don't for
 * reasons given in the manual entry.
 */

	    case chNull:
	    case '\n':
	    case '~':
		    if (! fHaveValue)			/* NO EXPR GIVEN */
		    {
		        strout("\033A\033K");		/* Remove old prompt */
			switch (vcmdDef)		/* default: no action */
			{
			    case cmdUpArrow:		/* BACKWARDS IN DATA */
			    case cmdDisplay:		/* FORWARDS  IN DATA */
				cnt   = IncFTyMode (vtyDot, vmode);
				vdot += (vcmdDef == cmdUpArrow) ? -cnt : cnt;

				DispVal (vdot, vtyDot, vmode,
					 false, true, true);

				if ((vmode->df != dfProc) && (vmode->df != dfInst))
				    printf ("\n");
				break;

			    case cmdLineSingle:		/* SINGLE STEP */
				printf ("s:  ");
				IbpFSingle (false, false);
				kdbprntins(vpc);
			        printf ("\n");
				break;

			    case cmdProcSingle:		/* BIG SINGLE STEP */
				printf ("S:  ");
				IbpFSingle (true, false);
				kdbprntins(vpc);
			        printf ("\n");
				break;

			    case cmdMachInst:
				printf("u: ");
				IbpFMachInst(false,false);
				kdbprntins(vpc);
			        printf ("\n");
				break;

			    case cmdProcInst:
				printf("U: ");
				IbpFMachInst(true,false);
				kdbprntins(vpc);
			        printf ("\n");
				break;

			} /* switch */
		    }
		    else 		/* HAVE EXPR OR NUMBER RESULT  */
		    {
			vdot = adrLong;			/* save adr  */
			CopyTy  (vtyDot, rgTy);		/* save type */
			DispVal (vdot, vtyDot, modeNil, false, true, true);
			printf  ("\n");
			vcmdDef = cmdDisplay;
		    }
		    break;

/*
 * NORMAL COMMANDS:
 */


	    case '^':					/* BACKWARD IN DATA */
		    if (TkPeek() == tkSlash)		/* given as "^/"    */
			TkNext();			/* set to slash	    */

		    vdot -= IncFTyMode (vtyDot, vmode);	/* do BEFORE get mode */
		    GetMode (vmode);			/* may set temp len   */
		    DispVal (vdot, vtyDot, vmode, false, true, true);

		    if (vmode->df != dfProc)
			printf ("\n");

		    vcmdDef = cmdUpArrow;
		    break;

	    case '/': /* "/hi": search forward  or "x/y": disp *x in format y */
	    case '?': /* "?hi": search backward or "x?y": disp  x in format y */

		    if (! fHaveValue)			/* SEARCH FOR STRING */
		    {
		    }
		    else				/* PRINT WITH FORMAT */
		    {	/* note: valExpr is not set! */
			vdot = adrLong;			/* save address	     */
			CopyTy  (vtyDot, rgTy);		/* save type	     */
			GetMode (vmode);		/* get format info   */

			if (chCmd == '/')		/* CONTENTS OF ADR */
			{
			    DispVal (vdot, vtyDot, vmode, false, true, true);
			    vcmdDef = cmdDisplay;
			}
			else				/* ADDRESS ITSELF */
			{
			    DispVal (vdot, vtyDot, vmode, false, false, false);
			    vcmdDef = cmdNil;
			}
			if ((vmode->df != dfProc) && (vmode->df != dfInst))
			    printf ("\n");
		    }
		    break;


#ifdef KASSERT
	    case 'A':					/* TOGGLE ASSERTIONS */
		    vas = (vas == asSuspended) ? asActive : asSuspended;
		    printf ("Assertions are %s\n",
			(vas == asSuspended) ? "SUSPENDED" : "ACTIVE");
		    vcmdDef = cmdNil;
		    break;

	    case 'a':					/* MODIFY ASSERTIONS */
		    if (fHaveValue)			/* do existing one   */
		    {
			if (vcbTok > 2)			/* too many letters */
			    chCmd = '?';
			else if (vcbTok > 1)		/* single token */
			    chCmd = vsbTok[1];
			else				/* separate tokens */
			{
			    TkNext();			/* set to option */
			    chCmd = (vcbTok == 1) ? vsbTok[0] : '?';
			}

			switch (chCmd)
			{
			    case 'a':	as = asActive;	  break;
			    case 'd':	as = asNil;	  break;
			    case 's':	as = asSuspended; break;

			    default:	UError (
		       "\"a\" must be followed by \"a\", \"d\", or \"s\" only");
			}
			ModAssert (valExpr, as);
		    }
		    else				/* add a new one? */
		    {
			if (vcbTok > 1)			/* single token */
			    sbTemp = "a";		/* cause error  */
			else if ((sbTemp = EatCmdList()) == sbNil)
			    UError ("Empty assertion not added");

			if ((strlen (sbTemp) == 1)	/* is modify command? */
			AND ((*sbTemp == 'a')
			  OR (*sbTemp == 'd')
			  OR (*sbTemp == 's')))
			{
			    UError ("Must specify which assertion to modify");
			}

			AddAssert (sbTemp);
		    }
		    break;				/* vcmdDef unchanged */
#endif

	    case 'B':					/* LIST ALL BPS */
		    ListBp (ibpNil);
		    break;				/* vcmdDef unchanged */

	    case 'b':					/* SET [AGAIN] A BP */
		    chNext = vsbTok[1];			/* bp type,  if any */
		    sbTemp = EatCmdList();		/* commands, if any */

		    if ((chNext == 'a') OR (chNext == 'A'))  /* BREAK ON ADR */
		    {
			if (fHaveValue)
			    adr = valExpr;
			else
			    UError ("Address is required before \"b%c\"",
				    chNext);

			IbpFAdr (adr, ((chNext == 'a') ? 1 : -1), sbTemp, true);
		    }
		    else if (chNext == chNull)		/* SIMPLE BREAKPOINT */
		    {
			if (vipd == ipdNil)
			    UError("Cannot set breakpoint--no debug information for this procedure\n");
			iln = fHaveValue ? valExpr : viln;
			IbpFAdr (AdrFIfdLn (vifd, iln), 1, sbTemp, true);
		    }
		    else				/* SPECIAL BP TYPE */
		    {
			vipd  = IpdFAdr (AdrFIfdLn (vifd, viln));
			depth = fHaveValue ? valExpr : 1;
			IbpFSet (chNext, depth, sbTemp);
		    }
		    vcmdDef = cmdNil;
		    break;

	    case 'C':					/* CONT WITH SIGNAL  */
	    case 'c':					/* CONT, NO  SIGNAL  */
#ifdef KASSERT
		    if (vfRunAssert == false)		/* now in assertions */
			UError (sbErrAllow, chCmd);	/* not allowed now   */
#endif

		    if (fHaveValue AND !fWhiteSpace)
			UError("Need a blank before a continue");
		    if (fHaveValue
		    AND (vibp != ibpNil))
		    {
			vrgBp[vibp].count = valExpr;	/* set its count */
		    }

		    iln	     = GetExpr (&ty, tkNil);	/* get line number */
		    fAteExpr = true;			/* for end of loop */

		    if (ty != tyNil)			/* valid expr  */
		    {					/* set temp bp */
			vipd = IpdFAdr (AdrFIfdLn (vifd, viln));
			IbpFAdr (AdrFIfdLn (vifd, iln), -1, sbNil, true);
		    }
		    vsbCmd = sbNil;			/* toss rest of line */

		    IbpFRun (ptResume);			/* resume child */
		    vcmdDef = cmdNil;
		    break;

	    case 'D':					/* DELETE ALL ASS/BPS */
#ifdef KASSERT
		    if (vcbTok > 2)			/* too many letters   */
			chCmd = '?';
		    else if (vcbTok > 1)		/* single token */
			chCmd = vsbTok[1];
		    else				/* separate tokens? */
		    {
			TkPeek();

			if (FAtSep (vtkPeek))		/* end of command */
			    chCmd = 'b';		/* default is bps */
			else
			{
			    TkNext();			/* set to option */
			    chCmd = (vcbTok == 1) ? vsbTok[0] : '?';
			}
		    }

		    if ((chCmd != 'a') AND (chCmd != 'b'))
		    {
			UError ("\"D\", \"D a\", or \"D b\" is required");
		    }

		    ClearAll (chCmd == 'b');		/* clear bps? */
#else
		    ClearAll (1);			/* clear bps */
#endif
		    vcmdDef = cmdNil;
		    break;

	    case 'd':					/* DELETE ONE BP */
		    adr = adrNil;

		    if (! fHaveValue)			/* use curr bp if any */
			adr = AdrFIfdLn (vifd, viln);
		    else if ((valExpr > 0) AND (valExpr < vibpMac))
			adr = vrgBp[valExpr].adr;	/* use given bp */

		    if ((adr == adrNil) OR (! FClearBp (adr, true)))
		    {
			printf ("No such breakpoint\n");
			ListBp (ibpNil);		/* show all */
		    }
		    vcmdDef = cmdNil;
		    break;

	    case 'e':					/* VIEW FILE OR PROC */
		    TkPeek();

		    if (FAtSep (vtkPeek))		/* use current adr */
		    {
			if (vifd != ifdNil)
			    PrintPos(AdrFIfdLn (vifd, viln), fmtFile + fmtProc);
			else {
			    kdbprntins(vpc);
			    printf("\n");
			}
		    }
		    else				/* file or proc name */
		    {
			fDidit = false;
			EatFilename (sbName, &vsbCmd);

			if ((strchr (sbName, '.') == 0)   /* no extension */
			AND ((ipd = IpdFName (sbName)) != ipdNil))   /* proc */
			{
			    OpenIpd  (ipd, true);
			    PrintPos (adrNil, fmtFile + fmtProc);
			    fDidit = true;
			}
			if (! fDidit)
			    UError ("No such procedure or file name \"%s\"",
				sbName);
		    } /* if */

		    vcmdDef = cmdNil;
		    break;

	    case 'I':					/* INQUIRE OF STATUS */
		    ShowState();
		    vcmdDef = cmdNil;
		    break;

#ifdef CMD_EDIT
	    case 'h':
		    showstack();
		    break;

#endif
	    case 'i':					/* "IF" COMMAND	 */
		    fDoIt = GetExpr (&ty, tkNil);	/* get condition */
						  /* do NOT set fAteExpr */
		    if (ty == tyNil)			/* none given	 */
			fDoIt = false;			/* treat as 0	 */

		    if (vtk != tkLCB)			/* check current */
			UError ("Missing \"{\"");

		    if (! fDoIt)			/* condition false  */
			vsbCmd = SbFEob (vsbCmd);	/* skip first block */

		    if (TkPeek() == tkLCB)		/* might be none */
			TkNext();			/* eat "{"	 */

		    vtk = tkSemi;			/* acts like ";" */

		    /*
		     * We're now ready to resume command execution, having
		     * possibly skipped one command block (a "then" case).
		     * This is a simpleminded way to do "if" statements.
		     */

		    vcmdDef = cmdNil;
		    break;

	    case 'J':
		    adr = GetExpr (&ty, tkNil);		/* get address */
		    if (ty == tyNil)
			UError ("\"J\" needs an address");
		    for (cnt=0;cnt<NJPARS;cnt++)
		    {
			    jpars[cnt] = GetExpr (&ty, tkNil);
			    if (ty == tyNil)
				break;
		    }
		    vcmdDef = cmdNil;
		    cnt = (*(PFI)(adr))(jpars[0],jpars[1],jpars[2],jpars[3],
				jpars[4],jpars[5]);
		    print_symbol(adr," returns");
		    printf(" %#x\n",cnt);
		    break;

	    case '{':					/* BEGINNING OF BLOCK */
		    vsbCmd = SbFEob (vsbCmd);		/* skip to RCB + 1    */
		    break;				/* throw away block   */
							/* vcmdDef unchanged  */

	    case ';':					/* COMMAND SEPARATOR  */
	    case '}':					/* END OF BLOCK	      */
		    break;				/* ignore it	      */
							/* vcmdDef unchanged  */

	    case 'l':					/* LIST SOMETHING */
		    ListSomething();
		    break;				/* vcmdDef unchanged */

#ifndef CDBKDB
	    case 'R':					/* RUN, NO ARGUMENTS  */
	    case 'r':					/* RUN WITH ARGUMENTS */
		    if (vpid != 0)			/* already running    */
			UError ("already running a kernel");

		    cnt = fHaveValue ? valExpr : 1;	/* continue or not    */
		    Boot (((chCmd == 'R') ? sbNil : vsbCmd),cnt);
		    vsbCmd = sbNil;
		    vcmdDef = cmdNil;
		    break;
#endif /* CDBKDB */

	    case 'S':					/* BIG	 SINGLE STEP */
	    case 's':					/* SMALL SINGLE STEP */
#ifdef KASSERT
		    if (vfRunAssert == false)		/* now in assertions */
			UError (sbErrAllow, chCmd);	/* not allowed now   */
#endif

		    cnt = fHaveValue ? Max (valExpr, 0) : 1;   /* step count */

		    while ((cnt--)
		    AND    (IbpFSingle (chCmd == 'S', false) == ibpNil))
			;				/* till count or bp */

		    vcmdDef = (chCmd == 's') ? cmdLineSingle : cmdProcSingle;
		    kdbprntins(vpc);
		    printf("\n");
		    break;

#ifndef CDBKDB
	    case 't':					/* SMALL STACK TRACE */
		    cnt = fHaveValue ? valExpr : 20;	/* number to show    */
		    if (vpid == pidNil)
			UError (vsbErrNoChild);

		    backtr(GetReg(ufp),cnt);
		    vcmdDef = cmdNil;
		    break;
#endif /* CDBKDB */

	    case 'U':
	    case 'u':					/* SMALL SINGLE STEP */
#ifdef KASSERT
		    if (vfRunAssert == false)		/* now in assertions */
			UError (sbErrAllow, chCmd);	/* not allowed now   */
#endif

		    cnt = fHaveValue ? Max (valExpr, 0) : 1;   /* step count */

		    while ((cnt--)
		    AND    (IbpFMachInst (chCmd == 'U', false) == ibpNil))
			;				/* till count or bp */

		    vcmdDef = (chCmd == 'u') ? cmdMachInst : cmdProcInst;
		    kdbprntins(vpc);
		    printf("\n");
		    break;

#ifdef KASSERT
	    case 'x':					/* EXIT CURRENT LEVEL */
		    if (vfRunAssert)			/* not asserting now  */
			UError (
			      "\"x\" is only allowed while running assertions");

		    if ((! fHaveValue) OR (valExpr == 0))
			vsbCmd = sbNil;			/* save nothing */

		    return (true);			/* now get out */
#endif

	} /* switch */

/*
 * SKIP PAST SEPARATOR AND PREPARE NEXT TOKEN:
 *
 * vsbCmd arrives here set before the NEXT token to look at (which might be
 * tkNil).  vtk is set to the CURRENT token (the last one used), so it's not
 * of interest unless it's a naked separator.
 *
 * Note:  It's safe to call TkNext() even after encountering a tkNil.
 */

Nextcmd:

	if ((! fAteExpr)		/* not at next token yet   */
	AND (! FAtSep (vtk)))		/* current not a separator */
	{
	    TkNext();			/* so eat it (ignore it) */
	}

	while (! FAtSep (vtk))		/* report and skip excess tokens */
	{
	    printf ("Extra token ignored:  \"%s\"\n", vsbTok);
	    TkNext();
	}
	TkNext();			/* now eat separator (prepare vtk) */

    } while (vtk != tkNil);

} /* FDoCommand */
