/* Internal RCS $Header: hpux_rel.c,v 70.14 93/10/26 10:10:29 ssa Exp $ */

#include	"ld.defs.h"

#if	VERSION == PATCH801

static char *HPUX_ID = "@(#) PATCH_8.0:	/bin/ld	4.3	91/07/29";

#else

/* @(#) $Revision: 70.14 $ */   
#ifndef   NO_HPUX_ID
static char *HPUX_ID = "@(#) $Revision: 70.14 $";
#endif
/*
   The declaration above is the RCS/SCCS ID string for an HP-UX command.

*/
#ifdef	BBA
static char *BBA_ID = "@(#) BBA version";
#endif

#endif
