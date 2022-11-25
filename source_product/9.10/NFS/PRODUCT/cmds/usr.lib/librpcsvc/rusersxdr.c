/*	@(#)rusersxdr.c	$Revision: 1.21.109.1 $	$Date: 91/11/19 14:25:17 $
rusersxdr.c	2.1 86/04/14 NFSSRC
static  char sccsid[] = "rusersxdr.c 1.1 86/02/05 Copyr 1984 Sun Micro";
*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <utmp.h>
#include <rpcsvc/rusers.h>
/*	HPNFS
**	Added tracing stuff to track down problems with utmp translation
**	see comments regarding LIBTRACE in <arpa/trace.h>
**	unfortunately the trace macros use stdio.h ...
*/
#include <stdio.h>
#include <arpa/trace.h>


rusers(host, up)
	char *host;
	struct utmpidlearr *up;
{
	TRACE("rusers SOP");
	return (callrpc(host, RUSERSPROG, RUSERSVERS_IDLE, RUSERSPROC_NAMES,
	    xdr_void, 0, xdr_utmpidlearr, up));
}

rnusers(host)
	char *host;
{
	int nusers;
	
	TRACE("rnusers SOP");
	if (callrpc(host, RUSERSPROG, RUSERSVERS_ORIG, RUSERSPROC_NUM,
	    xdr_void, 0, xdr_u_long, &nusers) != 0)
		return (-1);
	else
		return (nusers);
}

xdr_utmp(xdrsp, up)
	XDR *xdrsp;
	struct utmp *up;
{
	int len;
	char *p, empty[16];

	TRACE("xdr_utmp SOP");
	p = up->ut_line;
	/*	HPNFS
	**	if we have a pty/ entry then skip the "pty/" part
	*/
	if (!strncmp(p, "pty/", 4))
	    p += 4;
#ifdef	hpux
	/*
	**	Have to use 8 here, since our ut_line field is longer
	**	than Sun's equivalent field (sigh) ...
	*/
	len = 8;
#else	hpux
	len = sizeof(up->ut_line);
#endif	hpux
	TRACE3("xdr_utmp len = %d, line = %s, calling xdr_bytes", len, p);
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);

	len = sizeof(up->ut_name);
	p = up->ut_name;
	TRACE3("xdr_utmp len = %d, name = %s, calling xdr_bytes", len, p);
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);

	p = up->ut_host;
	len =  16;
	TRACE3("xdr_utmp len = %d, host = %s, calling xdr_bytes", len, p);
	if (xdr_bytes(xdrsp, &p, &len, len) == FALSE)
		return (0);
	/*
	**	the ut_time field can be sent as is ...
	*/
	TRACE2("xdr_utmp time = %ld, calling xdr_long", up->ut_time);
	if (xdr_long(xdrsp, &up->ut_time) == FALSE)
		return (0);
	TRACE("xdr_utmp all is OK, returning 1");
	return (1);
}

xdr_utmpidle(xdrsp, ui)
	XDR *xdrsp;
	struct utmpidle *ui;
{
	TRACE("utmpidle SOP, calling xdr_utmp");
	if (xdr_utmp(xdrsp, &ui->ui_utmp) == FALSE)
		return (0);
	TRACE("utmpidle calling xdr_u_int");
	if (xdr_u_int(xdrsp, &ui->ui_idle) == FALSE)
		return (0);
	TRACE("utmpidle all is OK, returning 1");
	return (1);
}

xdr_utmpptr(xdrsp, up)
	XDR *xdrsp;
	struct utmp **up;
{
	TRACE("xdr_utmpptr SOP");
	if (xdr_reference(xdrsp, up, sizeof (struct utmp), xdr_utmp) == FALSE)
		return (0);
	TRACE("xdr_utmpptr all is OK, returning 1");
	return (1);
}

xdr_utmpidleptr(xdrsp, up)
	XDR *xdrsp;
	struct utmpidle **up;
{
	TRACE("xdr_utmpidleptr SOP");
	if (xdr_reference(xdrsp, up, sizeof (struct utmpidle), xdr_utmpidle)
	    == FALSE)
		return (0);
	TRACE("xdr_utmpidleptr all is OK, returning 1");
	return (1);
}

xdr_utmparr(xdrsp, up)
	XDR *xdrsp;
	struct utmparr *up;
{
	TRACE("xdr_utmparr SOP");
	return (xdr_array(xdrsp, &up->uta_arr, &(up->uta_cnt),
	    MAXUSERS, sizeof(struct utmp *), xdr_utmpptr));
}

xdr_utmpidlearr(xdrsp, up)
	XDR *xdrsp;
	struct utmpidlearr *up;
{
	TRACE("xdr_utmpidlearr SOP");
	return (xdr_array(xdrsp, &up->uia_arr, &(up->uia_cnt),
	    MAXUSERS, sizeof(struct utmpidle *), xdr_utmpidleptr));
}
