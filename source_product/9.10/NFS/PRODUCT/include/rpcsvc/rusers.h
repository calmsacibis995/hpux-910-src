/*	@(#)rusers.h	$Revision: 1.19.109.2 $	$Date: 92/03/04 18:56:23 $  */


/*      (c) Copyright 1987 Hewlett-Packard Company  */
/*      (c) Copyright 1984 Sun Microsystems, Inc.   */

#ifndef _RPCSVC_RUSERS_INCLUDED
#define _RPCSVC_RUSERS_INCLUDED

#define RUSERSPROC_NUM 1
#define RUSERSPROC_NAMES 2
#define RUSERSPROC_ALLNAMES 3
#define RUSERSPROG 100002
#define RUSERSVERS_ORIG 1
#define RUSERSVERS_IDLE 2
#define RUSERSVERS 2

#define MAXUSERS 100

#ifndef	UTMP_FILE
#include <utmp.h>
#endif	/* UTMP_FILE */

struct utmparr {
	struct utmp **uta_arr;
	int uta_cnt;
};

struct utmpidle {
	struct utmp ui_utmp;
	unsigned ui_idle;
};

struct utmpidlearr {
	struct utmpidle **uia_arr;
	int uia_cnt;
};

int xdr_utmparr();
int xdr_utmpidlearr();

#endif /* _RPCSVC_RUSERS_INCLUDED */
