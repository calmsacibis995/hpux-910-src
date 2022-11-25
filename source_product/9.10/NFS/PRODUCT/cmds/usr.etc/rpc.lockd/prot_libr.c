/* @(#)rpc.lockd:	$Revision: 1.17.109.2 $	$Date: 95/03/21 13:20:20 $
*/
/* (#)prot_libr.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)prot_libr.c	1.4 87/06/18 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)prot_libr.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_5380="@(#) PATCH_9.X: prot_libr.o $Revision: 1.17.109.2 $ 95/03/21 PHNE_5380";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */

	/* prot_libr.c
	 * consists of routines used for initialization, mapping and debugging
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
#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include "prot_lock.h"
#include "prot_time.h"

#include "prot_sec.h"


char hostname[255];			/* for generating oh */
int pid;				/* id for monitor usage */
int host_len;				/* for generating oh */
int lock_len;
int res_len;
int msg_len;
int local_state;
int grace_period;
remote_result res_nolock;
remote_result res_working;
remote_result res_grace;

int cookie;				/* monitonically increasing # */

extern int used_le;
extern int used_fe;
extern int used_me;
extern int rel_fe;
extern int rel_me;
extern struct fs_rlck *grant_q;
extern reclock *wait_q;
extern struct fs_rlck *monitor_q;
extern msg_entry *msg_q, *reply_q;
extern int debug;
extern int HASH_SIZE;
extern struct fs_rlck *table_fp[];

extern char *progname;

char *xmalloc();
reclock *get_le();
void release_le();
void release_fe();
void release_me();

init()
{
	gethostname(hostname, 255);	/* used to generate owner handle */
	host_len = strlen(hostname)+1;
	msg_len = sizeof(msg_entry);
	lock_len = sizeof(reclock);
	res_len = sizeof(remote_result);
	pid = getpid();			/* used to generate return id for status monitor */
	res_nolock.lockstat = nolocks;
	res_working.lockstat = blocking;
	res_grace.lockstat = grace;
	grace_period = LM_GRACE;
	initialize_link_sock();
	cancel_mon();
}

/*
 * map input (from kenel) to lock manager internal structure
 * returns -1 if cannot allocate memory;
 * returns 0 otherwise
 */
int
map_kernel_klm(a)
reclock *a;
{
	/* common code shared between map_kernel_klm and map_klm_nlm */
	/* generate op */
	if(a->exclusive)
		a->lck.op = LOCK_EX;
	else
		a->lck.op = LOCK_SH;
	if(!a->block)
		a->lck.op = a->lck.op | LOCK_NB;
	/* generate upper bound */
	if(a->lck.l_len == 0)
		a->lck.ub = MAXLEN;
	else
		a->lck.ub = a->lck.l_offset + a->lck.l_len; 
	if(a->lck.l_len > MAXLEN) {
		logmsg(catgets(nlmsg_fd,NL_SETN,11, "%1$s:  len(%2$d) greater than max len(%3$d)\n"), progname, a->lck.l_len, MAXLEN);
		a->lck.l_len = MAXLEN;
	}

	/* generate svid holder */
	a->lck.svid = a->lck.pid;

	/* owner handle == (hostname, pid);
	 * cannot generate owner handle use obj_alloc
	 * because additioanl pid attached at the end */
	a->lck.oh_len = host_len + sizeof(int);
	if((a->lck.oh_bytes = xmalloc(a->lck.oh_len) ) == NULL)
		return(-1);
	strcpy(a->lck.oh_bytes, hostname);
	memcpy(&a->lck.oh_bytes[host_len], (char *) &a->lck.pid, sizeof(int));
	/* generate cookie */
	/* cookie is generated from monitonically increasing # */
	cookie++;
	if(obj_alloc(&a->cookie, (char *) &cookie, sizeof(int))== -1)
		return(-1);


	/* generate clnt_name */
	if((a->lck.clnt= xmalloc(host_len)) == NULL)
		return(-1);
	strcpy(a->lck.clnt, hostname);
	a->lck.caller_name = a->lck.clnt; 	/* ptr to same area */
	return(0);
}

/*
 * nlm map input from klm to lock manager internal structure
 * return -1, if cannot allocate memory!
 * returns 0, otherwise
 */ 
int
map_klm_nlm(a, choice)
reclock *a;
int choice;
{
	/* common code shared between map_kernel_klm and map_klm_nlm */
	if(choice == NLM_GRANTED || choice == NLM_GRANTED_MSG) 
		a->block = 1;	/* only blocked req will cause call back */
	/* generate op */
	if(a->exclusive)
		a->lck.op = LOCK_EX;
	else
		a->lck.op = LOCK_SH;
	if(!a->block)
		a->lck.op = a->lck.op | LOCK_NB;
	/* generate upper bound */
	if(a->lck.l_len == 0)
		a->lck.ub = MAXLEN;
	else
		a->lck.ub = a->lck.l_offset + a->lck.l_len; 

	if(choice == NLM_GRANTED || choice == NLM_GRANTED_MSG) {
 		/* nlm call back */
		if((a->lck.clnt= xmalloc(host_len)) == NULL)
			return(-1);
		strcpy(a->lck.clnt, hostname);
		a->lck.svr = a->lck.caller_name;
	}
	else {
 		/* normal klm to nlm calls */
		if((a->lck.svr = xmalloc(host_len)) == NULL) {
			return(-1);
		}
		strcpy(a->lck.svr, hostname);
		a->lck.clnt = a->lck.caller_name;
	}
	return(0);
}

pr_oh(a)
netobj *a;
{
	int i;
	int j;
	unsigned p = 0;

/*
	if(a->n_len - sizeof(int) > 4 )
		j = 4;
	else
*/
		j = a->n_len - sizeof(int);

	/* only print out part of oh */
	for(i = 0; i< j; i++) {
		logstr("%c", a->n_bytes[i]);
	}
	for(i = a->n_len - sizeof(int); i< a->n_len ; i++) {
		p = (p << 8) | (((unsigned)a->n_bytes[i]) & 0xff);
	}
	logstr("%u", p);
}

pr_fh(a)
netobj *a;
{
	int i;

	for(i = 0; i< a->n_len; i++) {
		logstr("%02x", (a->n_bytes[i] & 0xff));
	}
}


pr_lock(a)
reclock *a;
{
	logstr((catgets(nlmsg_fd,NL_SETN,12, "(%x), oh= ")), a);
	pr_oh(&a->lck.oh);
	logstr((catgets(nlmsg_fd,NL_SETN,13, ", svr= %s, fh = ")), a->lck.svr);
	pr_fh(&a->lck.fh);
	logstr((catgets(nlmsg_fd,NL_SETN,14, ", op=%1$d, ranges= [%2$d, %3$d)\n")),
 		a->lck.op,
		a->lck.l_offset, (a->lck.ub - 1));
}
 
pr_all()
{
	struct fs_rlck *fp;
	reclock *nl;
	msg_entry *msgp;
	int i;

	if(debug < 2) 
		return;
	logmsg((catgets(nlmsg_fd,NL_SETN,33, "LOCKD QUEUES:")));
	/* print grant_q */
	logstr((catgets(nlmsg_fd,NL_SETN,15, "***** granted reclocks *****\n")));
	for(i = 0; i< HASH_SIZE; i++) {
		if((fp = table_fp[i]) != NULL) {
			while(fp != NULL) {
				nl = fp->rlckp;
				while(nl != NULL) {
					pr_lock(nl);
					nl = nl->nxt;
				}
				fp = fp->nxt;
			}
		}
	}
	/* print msg queue */
	if(msg_q != NULL) {
		logstr((catgets(nlmsg_fd,NL_SETN,16, "***** msg queue *****\n")));
		msgp= msg_q;
		while(msgp != NULL) {
		    logstr((catgets(nlmsg_fd,NL_SETN,17, " (%x, ")), msgp->req);
		    if(msgp->reply != NULL)
			    logstr((catgets(nlmsg_fd,NL_SETN,18, " lockstat =%d),")), msgp->reply->lockstat);
		    else 
			    logstr((catgets(nlmsg_fd,NL_SETN,19, " NULL),")));
		    msgp = msgp->nxt;
		}
                logstr((catgets(nlmsg_fd,NL_SETN,20, "\n")));
	}
	else 
		logstr((catgets(nlmsg_fd,NL_SETN,21, "*****no entry in msg queue *****\n")));


	/* print wait_q */
	if(wait_q != NULL) {
		logstr((catgets(nlmsg_fd,NL_SETN,22, "***** blocked reclocks *****\n")));
		nl = wait_q;
		while( nl != NULL) {
                        pr_lock(nl);
                        nl = nl->wait_nxt;
		}
	}
	else 
		logstr((catgets(nlmsg_fd,NL_SETN,23, "***** no blocked reclocks ****\n")));


	/* print reply_q */
	if(reply_q != NULL) {
	    logstr((catgets(nlmsg_fd,NL_SETN,34, "***** reply queue *****\n")));
	    msgp = reply_q;
	    while( msgp != NULL) {
		logstr((catgets(nlmsg_fd,NL_SETN,35, " (%x, ")), msgp->req);
		logstr((catgets(nlmsg_fd,NL_SETN,36, " lockstat =%d),")), msgp->reply->lockstat);
		pr_lock(msgp->req);
		msgp = msgp->nxt;
	    }
	}

	/* print monitor_q */
	fp = monitor_q;
	while(fp != NULL) {
		logstr((catgets(nlmsg_fd,NL_SETN,24, "***** monitor queue on (%1$s, %2$d) *****\n")),
			fp->svr, fp->fs.procedure);
		nl = fp->rlckp;
		while(nl != NULL) {
			pr_lock(nl);
			nl = nl->mnt_nxt;
		}
		fp = fp->nxt;
	}
	logstr((catgets(nlmsg_fd,NL_SETN,25, "used_le=%1$d, used_fe=%2$d, used_me=%3$d\n")), used_le, used_fe, used_me);

}

#ifdef NOTDEF       /* perhaps was used for debugging? sxn */
perr_lock(stat)
nlm_stats stat;     /* fixes bug 1004713;sxn */
{
	switch(stat) {
		case granted:
			logmsg((catgets(nlmsg_fd,NL_SETN,26, "lock granted\n")));
			break;
		case denied:
			logmsg((catgets(nlmsg_fd,NL_SETN,27, "lock denied\n")));
			break;
		case nolocks:
			logmsg((catgets(nlmsg_fd,NL_SETN,28, "lock denied_nolocks\n")));
			break;
		case blocking:
			logmsg((catgets(nlmsg_fd,NL_SETN,29, "lock blocked\n")));
			break;
		case grace:
			logmsg((catgets(nlmsg_fd,NL_SETN,30, "lock denied_grace_period\n")));
			break;
	}
}
#endif NOTDEF

up(x)
int x;
{
	return((x % 2 == 1) || (x %2 == -1));
}

kill_process(a)
reclock *a;
{
	logmsg(catgets(nlmsg_fd,NL_SETN,31, "%1$s: kill process (%2$d)\n"), progname, a->lock.pid);
	ENABLEPRIV(SEC_KILL);
	kill(a->lock.pid, SIGLOST);
	DISABLEPRIV(SEC_KILL);
}

lock_exit()
{
	/* should notify status monitor */
	logmsg(catgets(nlmsg_fd,NL_SETN,32, "%s: cont_lock: remote and local lock tbl inconsistent\n"),
	    progname);
	semclose();
	exit(1);
}
