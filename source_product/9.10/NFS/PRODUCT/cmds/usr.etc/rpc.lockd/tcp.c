/* @(#)rpc.lockd:	$Revision: 1.10.109.1 $	$Date: 91/11/19 14:18:32 $
*/
/* (#)tcp.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)tcp.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)tcp.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * make tcp calls
	 */
/* ************************************************************** */
/* NOTE: prot_alloc.c, prot_libr.c, prot_lock.c, prot_main.c,     */
/* prot_msg.c, prot_pklm.c, prot_pnlm.c, prot_priv.c, prot_proc.c,*/
/* sm_monitor.c, svc_udp.c, tcp.c, udp.c, AND pmap.c share        */
/* a single message catalog (lockd.cat).  The last three files    */
/* pmap.c, tcp.c, udp.c have messages BOTH in lockd.cat AND       */
/* statd.cat.  For that reason we have allocated message ranges   */
/* for each one of the files.  If you need more than 10 messages  */
/* in this file check the  message numbers used by the other files*/
/* listed above in the NLS catalogs.                              */
/* ************************************************************** */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>

extern int debug;
int HASH_SIZE = 29;		/* Need to link in cache code in udp.c */

/*
 * NOTE: The call_tcp code was rewritten to use caching as part of allowing
 * us to asynchronously look for portmapper requests.  Since the algorithm
 * was basically the same as for call_udp(), the two were merged to allow
 * for better maintainability, but this interface was kept to make it easy
 * to switch to a separate routine in the future if necessary.
 */

int
call_tcp(host, prognum, versnum, procnum, inproc, in, outproc, out, tot )
        char *host;
        xdrproc_t inproc, outproc;
        char *in, *out;
	int tot;
{
	enum clnt_stat clnt_stat;
	int valid_in = 1;

	clnt_stat = call_remote( IPPROTO_TCP, host, prognum, versnum,
			procnum, inproc, in, outproc, out, valid_in, tot);
	if ( debug )
		logmsg((catgets(nlmsg_fd, NL_SETN, 10040, "call_tcp[%1$s, %2$d, %3$d, %4$d] returns %5$d")),
			host, prognum, versnum, procnum, clnt_stat);
	return ((int) clnt_stat);
}
