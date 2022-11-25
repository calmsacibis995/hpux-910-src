/*
 * @(#)ntp_filegen.h: $Revision: 1.2.109.1 $ $Date: 94/10/26 16:55:06 $
 * $Locker:  $
 */

/* ntp_filegen.h,v 3.1 1993/07/06 01:06:52 jbj Exp
 *
 * definitions for NTP file generations support
 *
 *
 * Copyright (c) 1992
 * Rainer Pruy Friedrich-Alexander Unuiversitaet Erlangen-Nuernberg
 *
 * This code may be modified and used freely
 * provided the credits remain intact.
 */

#include "ntp_types.h"

/*
 * supported file generation types
 */

#define FILEGEN_NONE	255	/* no generations - use plain file name */
#define FILEGEN_PID	1	/* one filegen per process incarnation */
#define FILEGEN_DAY	2	/* one filegen per day */
#define FILEGEN_WEEK	3	/* one filegen per week */
#define FILEGEN_MONTH	4	/* one filegen per month */
#define FILEGEN_YEAR	5	/* one filegen per year */
#define FILEGEN_AGE     6	/* change filegen each FG_AGE_SECS */

/*
 * supported file generation flags
 */

#define FGEN_FLAG_LINK		0x01 /* make a link to base name */

#define FGEN_FLAG_ENABLED	0x80 /* set this to really create files   */
				     /* without this, open is suppressed */

typedef struct FILEGEN
        {
	  FILE   *fp;		/* file referring to current generation */
	  char   *prefix;	/* filename prefix and basename to be used*/
	  char   *basename;	/* for constructing filename of generation file */
				/* WARNING: must be malloced !!! will be fed to free()*/
	  U_LONG  id;		/* id of current generation */
	  u_char  type;		/* type of file generation */
	  u_char  flag;		/* flags modifying processing of file generation */
	}	FILEGEN;

extern	void	filegen_setup	P((FILEGEN *, U_LONG));
extern	void	filegen_config	P((FILEGEN *, char *, u_int, u_int));
extern	FILEGEN *filegen_get	P((char *));
extern	void	filegen_register P((char *, FILEGEN *));
