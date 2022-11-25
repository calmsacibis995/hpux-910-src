/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/init.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:22:53 $
 */

/*
 * Original version based on: 
 * Revision 63.7  88/05/27  15:52:59  15:52:59  sjl (Steve Lilker)
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
 * InitAll() is the top of the tree for debugger setup.
 */

#ifdef S200
#include <setjmp.h>
#endif
#include <signal.h>
#include "cdb.h"

char *strrchr();

pFDR	vrgFd;		/* file descriptors	*/
int	vifdMac;	/* total files		*/
int	vifd;		/* current file		*/
int	vifdNil;	/* means "no such file" */
int	vifdTemp;	/* view "other" file	*/

pPDR	vrgPd;		/* procedure descriptors */
int	vipdMac;	/* total procedures	 */
int	vipd;		/* current procedure	 */
int	vipd_MAIN_;	/* main procedure	 */
int	vipd_END_;	/* procedure in end.o	 */

#ifdef HPSYMTABII
pMDR	vrgMd;		/* module descriptors    */
int	vimdMac;	/* total modules   	 */
int	vimd;		/* current module   	 */
#endif	/* HPSYMTABII */

ADRT	vadrStart;	/* AFTER init, BEFORE user's code	    */

static	MODER	amode;		/* instance of struct	*/
pMODER	vmode;		/* a ptr to it		*/

/*
 * Defines for string pointers
 */
char	*vsbYesNo;	/* characters for "yes" */
char	*vsbcoredumped;
char	*vsbnoignore;
char	*vsbunnamed;
char	*vsbignore;
char	*vsbnostop;
char	*vsbnocore;
char	*vsbnosourcefile;
char	*vsbunknown;
char	*vsbfmt1;
char	*vsbfmt2;
char	*vsbfmt3;
char	*vsbfmt4;
char	*vsbfmt5;
char	*vsbMore;
char    *vsbHitRETURN;  /* "Hit RETURN for more..."     */
char	*vsbPanic;	/* assumed to be init'd to sbNil */
char	*vsbActive;
char	*vsbACTIVE;
char	*vsbSuspended;
char	*vsbSUSPENDED;
char	*vsbBpState;
char	*vsbAssState;

#ifdef HPE
PLABEL   vplFixer;	/* XDB's ctrl-Y handler */
#endif


#if (S200BSD || INSTR)
#include <sys/utsname.h>
#endif

#ifdef S200BSD
FLAGT	vfUMM;		/* Running on an UMM (modified 200) ? */
FLAGT	vfW310;		/* Running on a WOPR (BOBCAT 310) ? */
FLAGT   vffloat_soft;   /* have a hardware float card ?     */
FLAGT   vfmc68881;	/* have a mc68881 co-processor ?    */
FLAGT   vffpa;		/* have a Dragon card ?		    */
ADRT    adrFpa;		/* location of Dragon card 	    */
#endif

#ifdef INSTR			/* only if collecting execution stats	*/
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <pwd.h>
char	*vsbStatsFile;	/* name of stats file 	*/
long	*vfnStats;	/* stats file descriptor	*/
long	vtimeStart;	/* start time from time(2)	*/
ulong	viCmdCount = 0;	/* number of commands executed	*/
char	*sccsidAdd = "@(#)INSTRUMENTED WITH STATS MAILING";
				/* additional what string for id	*/
#endif


/***********************************************************************
 * I N I T   A L L
 *
 * Set up a lot of miscellaneous data values, starting with the default
 * display record.
 */


void InitAll()
{



    amode.cnt	= 1;		/* one object	*/
    amode.len	= CBINT;	/* size is int	*/
    amode.df	= dfDecimal;	/* treat as dec */
    amode.imap = 0;
    vmode	= &amode;
    vimap	= 0;		/* kludgey alternate map selector */

/*
 * RANDOM DEFAULTS:
 */

    vfcaseMod = true;		/* case INsensitive searches	*/

#ifdef S200BSD
    vUarea = UareaFLst();	/* read address of Uarea from /hp-ux */
    InitConfig();               /* determine OS and hardware config */
    viFpa_flag = vffpa;
    viFpa_reg = 2;
#endif
#if (S200 && (! S200BSD))
    vfRegValid = false;         /* see GetRegPtr() */
    vUarea = 0x80000;
#endif


	printf("\n Please wait, scanning debug records (if any)\n");




/*
 * CALL OTHER INIT ROUTINES:
 */

    InitSymfile (vsbSymfile);
    InitCorefile (vsbCorefile); /* core file image, if any */


#ifdef S200
        if (!strcmp(vsbCorefile, "core") AND FYesNo(nl_msg(473,
                "Do you want to save a backup copy of the core file? ")))
        {
           char sbBuffer[24];
           sprintf(sbBuffer,"cp core core%d",vpidMyself);
           if (system(sbBuffer) < 0)
              printf(nl_msg(468, "Cannot create core file backup copy\n"));
           else
              printf(nl_msg(469, "Core file saved as \"core%d\"\n"),vpidMyself);
        }
#endif

#ifdef XDB
    if (!vfNoDebugInfo) {
       InitSs();
    }
    InitFdPd();			/* source file and proc quick ref	*/
#ifndef SPECTRUM
    if (!vfNoDebugInfo) {
       InitSd();
    }
#endif
#else
    if (visymMax) {
       InitSs();
       InitFdPd();		/* source file and proc quick ref	*/
#ifndef SPECTRUM
       InitSd();
#endif
    }
    else {
       InitNoSyms();
    }
#endif

#ifdef SPECTRUM
    vipd_MAIN_ = IpdFName("_MAIN_");
    vipd_END_ = IpdFName("_end_");
#endif

    InitSpc();			/* special (debugger resident) var	*/

#ifdef S200
    InitLSTPd();		/* LST proc quick ref			*/
#endif



/*
 * Set the adr of the START OF THE MAIN PROGRAM:
 */

	vifd = vifdNil;


/*
 * Set the adrs for BREAKPOINTING AND PROC CALLS:
 */

    SetCurLang();

} /* InitAll */


#ifdef S200BSD
/***********************************************************************
 * I N I T   C O N F I G
 *
 * Determine OS and hardware configuration we're running on.
 */

jmp_buf config_env;

void InitConfig()
{
    struct utsname utsname;	/* for checking machine type		*/
    int ConfigSigCatch();

#ifdef INSTR
    vfStatProc[139]++;
#endif

    vfUMM = false;
    vfW310 = false;
    asm (" mov.b flag_68881+1,_vfmc68881 ");  /* Check for 68881   */
    asm (" mov.b flag_fpa+1,_vffpa ");	      /* Check for 98248A  */
    asm (" mov.l &fpa_loc,_adrFpa ");	      /* Address of 98248A */
    signal(SIGILL,ConfigSigCatch);
    if (setjmp(config_env)) {
        utsname.machine[0] = ' ';
        uname(&(utsname));	/* read machine OS type, e.g., 9000/310 */
        if (strncmp(utsname.machine, "9000/31", 7) == 0) vfW310 = true;
        else vfUMM = true;
    } else {
        asm (" short 0x51FC "); /* trapf - 68020 only instruction */
        signal(SIGILL, SIG_DFL);
    };

    /* float_soft will indicate presence of hardware float card */
    asm (" move.b float_soft+1,_vffloat_soft ");

    return;

}  /* InitConfig */

/***********************************************************************
 * C O N F I G   S I G   C A T C H
 *
 * Catch signals while determining hardware configuration.
 */

int ConfigSigCatch()
{

#ifdef INSTR
    vfStatProc[140]++;
#endif

    signal(SIGILL, SIG_DFL);
    longjmp(config_env, 1);
}
#endif  /* S200BSD */




/***********************************************************************
 * M I N
 *
 * Return the min of two values.
 */

int Min (x, y)
    int		x, y;
{

#ifdef INSTR
    vfStatProc[143]++;
#endif

    return ((x < y) ? x : y);
} /* Min */


/***********************************************************************
 * M A X
 *
 * Return the max of two values.
 */

int Max (x, y)
    int		x, y;
{

#ifdef INSTR
    vfStatProc[144]++;
#endif

    return ((x > y) ? x : y);
} /* Max */

