/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_audit.c,v $
 * $Revision: 1.10.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:09 $
 */
/* HPUX_ID: @(#)dux_audit.c	55.1		88/12/23 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#ifdef AUDIT
#include "../h/types.h"
#include "../h/audit.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../dux/dux_lookup.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/cct.h"
#include "../h/systm.h"
#include "../h/buf.h"

extern struct aud_type aud_syscall[];
extern struct aud_event_tbl aud_event_table;
extern site_t root_site, my_site;
extern struct dux_context dux_context;
extern struct cct clustab[];

/* space in the DUX message to pass syscall array must be a multiple of 4
 * and larger than MAX_SYSCALL
 */
#define DUX_SYSCALL_SPACE (( MAX_SYSCALL ) + ( 4 - ( MAX_SYSCALL )%4 ))

struct setevent_request {
	unsigned char 	dux_aud_syscall[DUX_SYSCALL_SPACE];
	union {
		struct aud_event_tbl dux_aud_event_table;
		int dummy[2]; /* to ensure proper alignment */
	} dux_aud_event_table;
};

/* D U X _ S E T E V E N T
 *
 * dux_setevent is called to package up the setevent request to ship it
 * to the root server. All setevent requests are handled at the server
 * to give the cluster a single point of control.
 */

dux_setevent(paud_syscall,paud_evtab,dest_cnode)
struct aud_type 	*paud_syscall;	/* ptr to the aud_syscall table */
struct aud_event_tbl	*paud_evtab;	/* ptr to the event table */
{
	dm_message request;
	register struct setevent_request *srqp;
	register int i;

	request = dm_alloc(sizeof(struct setevent_request),WAIT);
	srqp = DM_CONVERT(request, struct setevent_request);

	/*We can't use a bcopy for syscall because the 800 and the 300
	 *build the table out of different sized structures.
	 */
	for (i=0; i<MAX_SYSCALL; i++)
		srqp->dux_aud_syscall[i] = paud_syscall[i].logit;

	bcopy(paud_evtab,&srqp->dux_aud_event_table,
			sizeof(struct aud_event_tbl));

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST, DM_SETEVENT,
		dest_cnode, DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);

	u.u_error = DM_RETURN_CODE(request);
	dm_release(request,1);
}

/* D U X _ S E T E V E N T _ S E R V E
 *
 * Handle the setevent request at the root server.
 */

dux_setevent_serve(request)
dm_message request;
{
	void aud_trans_syscall();
	struct aud_type temp_aud_syscall[MAX_SYSCALL];
	register int i;

	register struct setevent_request *srqp =
		DM_CONVERT(request, struct setevent_request);

	/*
	 * If the source and destination are not the same architecture
	 * translate the syscall table to the host format before
	 * filling the kernel event table.
	 *
	 * Note that the Series 700 and Series 800 system call tables
	 * are the same, so no translation is required in this case.
	 */
	if (clustab[my_site].cpu_arch != clustab[root_site].cpu_arch &&
	    !((clustab[my_site].cpu_arch == CCT_CPU_ARCH_700 &&
	       clustab[root_site].cpu_arch == CCT_CPU_ARCH_800) ||
	      (clustab[my_site].cpu_arch == CCT_CPU_ARCH_800 &&
	       clustab[root_site].cpu_arch == CCT_CPU_ARCH_700)))
		aud_trans_syscall(srqp->dux_aud_syscall);

	/* Convert the syscalls from a char array to an array of aud_type */
	bzero(temp_aud_syscall,sizeof(temp_aud_syscall));

	for (i=0; i<MAX_SYSCALL; i++)
		temp_aud_syscall[i].logit = srqp->dux_aud_syscall[i];
	fill_event_table(temp_aud_syscall,&srqp->dux_aud_event_table);

	dm_quick_reply (u.u_error);
}

struct audctl_request
{
	int mode;
	int cmd;
};

dux_audctl(fnamep,mode,cmd,dest_cnode)
char *fnamep;
{
	dm_message request;
	register struct audctl_request *arqp;
	register struct buf *bp;
	register int pathlen;

	request = dm_alloc(sizeof(struct audctl_request),WAIT);
	arqp = DM_CONVERT(request, struct audctl_request);

	bp = geteblk(MAXPATHLEN);
	pathlen = strlen(fnamep)+1;
	bcopy(fnamep,bp->b_un.b_addr,pathlen);
	arqp->mode = mode;
	arqp->cmd = cmd;

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|DM_REQUEST_BUF,
		DM_AUDCTL, dest_cnode, DM_EMPTY, NULL, bp, pathlen, 0,
		NULL, NULL, NULL);

	u.u_error = DM_RETURN_CODE(request);
	dm_release(request,1);
}

dux_audctl_serve(request)
dm_message request;
{
	register struct audctl_request *arqp =
		DM_CONVERT(request, struct audctl_request);
	register struct buf *bp;
	register int site_save;

	u.u_cntxp = (char **)&dux_context;	/* set context to my context */
	site_save = u.u_site;			/* save the proc's site */
	u.u_site = my_site;			/* set site to my site */

	bp = DM_BUF(request);
	set_audit_file(bp->b_un.b_addr,arqp->mode,arqp->cmd);

	u.u_site = site_save;			/* restore proc's site */
	dm_quick_reply (u.u_error);
}

struct audoff_request
{
	int cmd;
};

dux_audoff(cmd,dest_cnode)
{
	dm_message request;
	register struct audoff_request *arqp;

	request = dm_alloc(sizeof(struct audoff_request),WAIT);
	arqp = DM_CONVERT(request, struct audoff_request);
	arqp->cmd = cmd;

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST, DM_AUDOFF,
		dest_cnode, DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);

	u.u_error = DM_RETURN_CODE(request);
	dm_release(request,1);
}

dux_audoff_serve(request)
dm_message request;
{
	register struct audoff_request *arqp =
		DM_CONVERT(request, struct audoff_request);
	register int site_save;

	site_save = u.u_site;			/* save the proc's site */
	u.u_site = my_site;			/* set site to my site */

	set_audit_off(arqp->cmd);

	u.u_site = site_save;			/* restore proc's site */
	dm_quick_reply (u.u_error);
}

struct getaudstuff_reply
{
	unsigned char 	dux_aud_syscall[DUX_SYSCALL_SPACE];
	union {
		struct aud_event_tbl dux_aud_event_table;
		int dummy[2]; /* to ensure proper alignment */
	} dux_aud_event_table;
	int audit_ever_on;
	int audit_state_flag;
	int c_mode;
}

dux_getaudstuff()
{
	dm_message request;
	register struct getaudstuff_reply *gasrep;
	register struct buf *bp;
	register i;

	void aud_trans_syscall();

	request = dm_alloc(0,WAIT);
	bp = geteblk (2*MAXPATHLEN);

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|DM_REPLY_BUF,
		DM_GETAUDSTUFF, root_site, sizeof(struct getaudstuff_reply),
		NULL, bp, 2*MAXPATHLEN, 0, NULL, NULL, NULL);

	if (DM_RETURN_CODE(request))
		panic("could not get audit stuff");
	gasrep = DM_CONVERT(request,struct getaudstuff_reply);

	/* if the source and destination are not the same archetecture
	 * translate the syscall table to the host format
	 * before filling the kernel
	 */
	 if (clustab[my_site].cpu_arch != clustab[root_site].cpu_arch)
	 	aud_trans_syscall(gasrep->dux_aud_syscall);

	/* fill event table and syscall table */
	bcopy(&gasrep->dux_aud_event_table,&aud_event_table,
			sizeof(aud_event_table));

	/*We can't use a bcopy for syscall because the 800 and the 300
	 *build the table out of different sized structures.
	 */
	for (i=0; i<MAX_SYSCALL; i++)
		aud_syscall[i].logit = gasrep->dux_aud_syscall[i];

	/* Turn auditing on if it is on for the root server.  */
	audit_ever_on = gasrep->audit_ever_on;
	if (gasrep->audit_state_flag) {
		set_audit_file(bp->b_un.b_addr, gasrep->c_mode, AUD_ON);
		if (u.u_error) {
			dm_release(request,1);
			panic("SECURITY FAILURE: unable to open current audit file.\nCannot start auditing on this system.\nSpecify an accessible audit file and reboot this client.");
		}
		if ((char)*(bp->b_un.b_addr + currlen + 1)) {
			set_audit_file(bp->b_un.b_addr + currlen + 1,
				gasrep->c_mode, AUD_SETNEXT);
			if (u.u_error) {
				dm_release(request,1);
				panic("SECURITY FAILURE: unable to open next audit file.\nCannot start auditing on this system.\nSpecify an accessible audit file and reboot this client.");
			}
		}
	}

	dm_release(request,1);
}

/*ARGSUSED*/
dux_getaudstuff_serve(request)
dm_message request;
{
	register struct getaudstuff_reply *gasrep;
	register struct buf *bp;
	dm_message reply;
	register i;

	bp = geteblk (2*MAXPATHLEN);
	reply = dm_alloc(sizeof(struct getaudstuff_reply),WAIT);
	gasrep = DM_CONVERT(reply,struct getaudstuff_reply);

	/*We can't use a bcopy for syscall because the 800 and the 300
	 *build the table out of different sized structures.
	 */
	for (i=0; i<MAX_SYSCALL; i++)
		gasrep->dux_aud_syscall[i] = aud_syscall[i].logit;

	bcopy(&aud_event_table,&gasrep->dux_aud_event_table,
			sizeof(aud_event_table));

	gasrep->audit_state_flag = audit_state_flag;
	gasrep->audit_ever_on = audit_ever_on;
	gasrep->c_mode = audit_mode;
	bcopy(curr_file, bp->b_un.b_addr, currlen + 1);
	bcopy(next_file, bp->b_un.b_addr + currlen + 1, nextlen + 1);
	dm_reply (reply, DM_REPLY_BUF, 0, bp, currlen + nextlen + 2, 0);
}

struct swaudfile_request {
	int cursize;
}

/* D U X _ S W A U D F I L E
 *
 * Switch audit files for the cluster. As part of this operation we package
 * up the names of the current and next audit files. On the root server
 * we will then check these names against the root server's version of the
 * names. If they are different then either another system has also
 * switched or an audctl() has been performed.
 *
 * Note that dux_swaudfile_serve is executed only on the root_site, and
 * cluster_swaudfile_serve  is executed only on the clients.
 */

dux_swaudfile()
{
	dm_message request;
	register struct swaudfile_request *swrqp;
	register struct buf *bp;

	request = dm_alloc(sizeof(struct swaudfile_request),WAIT);
	swrqp = DM_CONVERT(request, struct swaudfile_request);
	swrqp->cursize = currlen + 1;

	bp = geteblk (2*MAXPATHLEN);
	bcopy(curr_file, bp->b_un.b_addr, currlen + 1);
	bcopy(next_file, bp->b_un.b_addr + currlen + 1, nextlen + 1);

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|DM_REQUEST_BUF,
		DM_SWAUDFILE, root_site, 0,
		NULL, bp, 2*MAXPATHLEN, 0, NULL, NULL, NULL);

	dm_release(request,1);
}

dux_swaudfile_serve(request)
dm_message request;
{
	register struct swaudfile_request *swrqp =
		DM_CONVERT(request, struct swaudfile_request);
	register struct buf *bp;
	register int site_save;

	bp = DM_BUF(request);
	site_save = u.u_site;			/* save the proc's site */
	u.u_site = my_site;			/* set site to my site */

	aud_swtch(bp->b_un.b_addr,bp->b_un.b_addr + swrqp->cursize);

	u.u_site = site_save;			/* restore proc's site */

	dm_quick_reply (0);
}

cluster_swaudfile()
{
	dm_message request;

	request = dm_alloc(0,WAIT);

	request = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST, DM_CL_SWAUDFILE,
		DM_CLUSTERCAST, DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
	dm_release(request,1);
}

/*ARGSUSED*/
cluster_swaudfile_serve(request)
dm_message request;
{
	register int site_save;

	site_save = u.u_site;			/* save the proc's site */
	u.u_site = my_site;			/* set site to my site */

	aud_swtch(0,0);

	u.u_site = site_save;			/* restore proc's site */

	dm_quick_reply (0);
}
#endif AUDIT
