/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/xdbmain.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:24:27 $
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


#if (S200 || FOCUS)
#include <sys/types.h>
#define SYS_USHORT 1
#if (S200BSD || SYSVHDRS)
#define SYS_UINT 1
#endif
#endif

#include "cdb.h"


#ifdef NLS
#ifdef DEBREC
#include <limits.h>
#endif
#include <msgbuf.h>
char     vsbMsgBuf[NL_TEXTMAX];  /* buffer for output messages   */
#else
char     vsbMsgBuf[BUFSIZ];  /* buffer for output messages   */
#endif

#include <signal.h>
#include <setjmp.h>

#ifdef XDB
#include <fcntl.h>
#include <termio.h>
#endif

LCE vlc;
LCE vlcSet;

void xdbmain ()
{

    vlc = lcC;
    vlcSet = lcC;

    vsbSbrkFirst = sbrk (0);			/* before changing it	    */


#ifndef DEBREC
    vsbSymfile	= "hp-ux.0";
    vsbCorefile = "hp-core.0";
#endif

    /* InitStrings();  For NLS  */


/*
 * FINISH INITIALIZATION and set up general error catcher:
 *
 * Any return (longjmp()) after this setjmp() is not fatal.  However, if
 * an error occurs in the completion of initialization (the if {} block)
 * the rest of it is never done.
 */

    InitAll();				/* calls all initialization routines */


	/* May need */
#ifndef DEBREC
	OpenStack(0);
#endif



/*
 * SET/RESET GLOBAL VALUES:
 *
 */


    /* vsbCmd	= sbNil; */
    /* vivarMac	= 0; */
    /* viopMac	= 0; */
    vcNest	= 0;
    vsig	= 0;			/* so we don't use a bad one */
    /* vcLines	= 0; */
    /* vfRunAssert	= true; */

    /* InitSignals(); */


} /* main */

