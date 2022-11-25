/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/dump.c,v $
 * $Revision: 1.74.83.4 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/05 15:29:30 $
 */

/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"
#include "dux/duxfs.h"
#include "ufs/fs.h"

char *pst_cmds = 0;

#define PLURAL(x)	((x == 0) ? "s" : (((x) > 1) ? "s" : "" ))

char *
time_since_boot(time)
	unsigned int time;
{
	extern unsigned int boottime;
	static char delta_string[64];
	unsigned delta;
	int days;
	int hours;
	int minutes;
	int seconds;
	int rem_hours;
	int rem_minutes;

	delta = time - boottime;
	days = delta / (60*60*24);
	rem_hours = delta - (days * (60*60*24));
	hours = rem_hours / (60*60);
	rem_minutes = rem_hours - (hours * (60*60));
	minutes = rem_minutes / 60;
	seconds = rem_minutes - (minutes * 60);
	if (days > 0) {
		sprintf(delta_string, "%d day%s, %d hour%s, %d minute%s, %d second%s",
			days, PLURAL(days), hours, PLURAL(hours),
			minutes, PLURAL(minutes), seconds, PLURAL(seconds));
	} else if (hours > 0) {
		sprintf(delta_string, "%d hour%s, %d minute%s, %d second%s",
			hours, PLURAL(hours),
			minutes, PLURAL(minutes), seconds, PLURAL(seconds));
	} else if (minutes > 0) {
		sprintf(delta_string, "%d minute%s, %d second%s",
			minutes, PLURAL(minutes), seconds, PLURAL(seconds));
	} else {
		sprintf(delta_string, "%d second%s",
			seconds, PLURAL(seconds));
	}
	return (delta_string);
}

/* Dump contents of buffer */
/* Pass in cp (message) bp (pointer) lb (local block) vb (virtual block) */
dumpbuf(cp, bp, vbp)
	char *cp;
	struct buf *bp ;
{
	caddr_t addr;
	int count;

	fprintf(outf, "Buffer entry : %s   buffer headr address 0x%08x\n",
	    cp, vbp);

	fprintf(outf, "   bflags 0x%08x b_count  0x%08x b_error  0x%08x b_dev   0x%08x\n",
	    bp->b_flags, bp->b_bcount, bp->b_error, bp->b_dev);

	fprintf(outf, "   b_addr 0x%08x b_blkno  0x%08x b_proc   0x%08x b_spc   0x%08x\n",
	    bp->b_un.b_addr, bp->b_blkno, bp->b_proc, bp->b_spaddr);
	fprintf(outf, "   b_forw 0x%08x b_back   0x%08x av_forw  0x%08x av_back  0x%08x\n",
	    bp->b_forw, bp->b_back, bp->av_forw, bp->av_back);

	fprintf(outf, "   b_vp   0x%08x b_bufsiz 0x%08x b_virtsz 0x%08x accs_cnt 0x%08x\n",
	    bp->b_vp, bp->b_bufsize,  bp->b_virtsize, bp->access_cnt);
	fprintf(outf, "   b_flg2 0x%08x b_nxthdr 0x%08x wich_lst 0x%08x\n",
	    bp->b_flags2, bp->b_nexthdr, bp->which_list);

	fprintf(outf, "   ordcnt 0x%08x ord_proc 0x%08x ordinter 0x%08x ordinfra 0x%08x\n",
	    bp->b_ord_refcnt, bp->b_ord_proc, bp->b_ord_inter, bp->b_ord_infra);
	fprintf(outf, "   b_upid 0x%08x b_bptype 0x%08x b_queue  0x%08x b_s2     0x%08x\n",
	    bp->b_upid, bp->b_bptype, bp->b_queue, bp->b_s2);
	/*
	 * Check buffer addresses for validity
	 */
	addr = bp->b_un.b_addr;
	for (count = bp->b_bufsize; count > 0; count -= NBPG) {
	    if (!(ltor(0, addr)))
		fprintf(outf, "   WARNING: buffer address %08x is not mapped!!\n",
		    addr);
	    addr += NBPG;
	}
	fprintf(outf, "   ");

#ifndef B_ORDWRI	/* may not be defined on s300 yet */
# define B_ORDWRI 0
#endif

# define B_ELSE (~(B_READ|B_DONE|B_ERROR|B_BUSY|B_PHYS|B_CACHE|    \
		   B_INVAL|B_CALL|B_WANTED|B_ASYNC|B_DELWRI|B_ORDWRI))

	if (bp->b_flags & B_ELSE)
		fprintf(outf, " B_0x%08x", bp->b_flags & B_ELSE);
	if (bp->b_flags & B_READ)
		fprintf(outf, " B_READ");
	else
		fprintf(outf, " B_WRITE");
	if (bp->b_flags & B_DONE)
		fprintf(outf, " B_DONE");
	if (bp->b_flags & B_ERROR)
		fprintf(outf, " B_ERROR");
	if (bp->b_flags & B_BUSY)
		fprintf(outf, " B_BUSY");
	if (bp->b_flags & B_PHYS)
		fprintf(outf, " B_PHYS");
	if (bp->b_flags & B_CACHE)
		fprintf(outf, " B_CACHE");
	if (bp->b_flags & B_INVAL)
		fprintf(outf, " B_INVAL");
	if (bp->b_flags & B_CALL)
		fprintf(outf, " B_CALL");
	if (bp->b_flags & B_WANTED)
		fprintf(outf, " B_WANTED");
	if (bp->b_flags & B_ASYNC)
		fprintf(outf, " B_ASYNC");
	if (bp->b_flags & B_DELWRI)
		fprintf(outf, " B_DELWRI");
	if (bp->b_flags & B_ORDWRI)
		fprintf(outf, " B_ORDWRI");
	if (bp->b_flags & B_NOCACHE)
		fprintf(outf, " B_NOCACHE");
	if (bp->b_flags & B_NETBUF)
		fprintf(outf, " B_NETBUF");
	if (bp->b_flags & B_SYNC)
		fprintf(outf, " B_SYNC");
	if (bp->b_flags & B_FSYSIO)
		fprintf(outf, " B_FSYSIO");

#ifdef B_DUX_CANT_FREE
	if (bp->b_flags & B_DUX_CANT_FREE)
		fprintf(outf, " B_DUX_CANT_FREE");
#endif
#ifdef B_LOCAL
	if (bp->b_flags & B_LOCAL)
		fprintf(outf, " B_LOCAL");
#endif
#ifdef B_REIMAGE_WRITE
	if (bp->b_flags & B_REIMAGE_WRITE)
		fprintf(outf, " B_REIMAGE_WRITE");
#endif

	if (bp->b_flags)
		fprintf(outf, "\n");

	if (bp->b_flags2)
		fprintf(outf, "   ");
	if (bp->b_flags2 & B2_LINKDATA)
		fprintf(outf, " B2_LINKDATA");
	if (bp->b_flags2 & B2_UNLINKDATA)
		fprintf(outf, " B2_UNLINKDATA");
	if (bp->b_flags2 & B2_LINKMETA)
		fprintf(outf, " B2_LINKMETA");
	if (bp->b_flags2 & B2_UNLINKMETA)
		fprintf(outf, " B2_UNLINKMETA");
	if (bp->b_flags2)
		fprintf(outf, "\n");
}


/* dump contents of proc table entry */
dumpproc(cp, p, lp, vp)
	char *cp;
	register struct proc *p, *lp, *vp;
{
	register int mpflag, flag, flag2, stat, usrpri, pri, i;
	int diff;
	extern char *pst_cmds;

	/* Put often used values or shorts in register variables */
	stat = p->p_stat;
	flag = p->p_flag;
	flag2 = p->p_flag2;
	usrpri = p->p_usrpri;
	pri = p->p_pri;
#ifdef MP
	mpflag = p->p_mpflag;
#endif

	fprintf(outf, "proc[%d]  pid %3d  ppid %3d  pflag 0x%08x  pstat 0x%02x  wchan 0x%08x\n",
		(p - lp), p->p_pid, p->p_ppid, flag, stat, p->p_wchan);

	fprintf(outf, " p_link   0x%08x p_rlink  0x%08x p_usrpri 0x%08x p_pri    0x%08x\n",
		p->p_link, p->p_rlink, usrpri, pri);
	fprintf(outf, " p_rtpri  0x%08x p_cpu    0x%08x p_stat   0x%08x p_nice   0x%08x\n",
		p->p_rtpri, p->p_cpu, p->p_stat, p->p_nice);
	fprintf(outf, " p_cursig 0x%08x p_sig    0x%08x p_sigmk  0x%08x p_sigig  0x%08x\n",
		p->p_cursig, p->p_sig, p->p_sigmask, p->p_sigignore);
	fprintf(outf, " p_sigcat 0x%08x p_flag   0x%08x p_flag2  0x%08x p_uid    0x%08x\n",
		p->p_sigcatch, flag, flag2, p->p_uid);
	fprintf(outf, " p_suid   0x%08x p_pgrp   0x%08x p_pid    0x%08x p_ppid   0x%08x\n",
		p->p_suid, p->p_pgrp, p->p_pid, p->p_ppid);

	fprintf(outf, " p_wchan  0x%08x p_maxrss 0x%08x\n",
		p->p_wchan, p->p_maxrss);

	fprintf(outf, " p_cptick 0x%08x p_tot_tk 0x%08x p_pctcpu %8.8f p_idhash 0x%08x\n",
		p->p_cpticks, p->p_cptickstotal, p->p_pctcpu, p->p_idhash);

	fprintf(outf, " p_pgrphx 0x%08x p_uidhx  0x%08x p_fandx  0x%08x p_pandx  0x%08x\n",
		p->p_pgrphx, p->p_uidhx, p->p_fandx, p->p_pandx);

	fprintf(outf, " p_pptr   0x%08x p_cptr   0x%08x p_osptr  0x%08x p_ysptr  0x%08x\n",
		p->p_pptr, p->p_cptr, p->p_osptr, p->p_ysptr);

	fprintf(outf, " p_dptr   0x%08x p_vas    0x%08x p_upreg  0x%08x p_mpgned 0x%08x\n",
		p->p_dptr, p->p_vas, p->p_upreg, p->p_mpgneed);

	fprintf(outf, " p_mlink  0x%08x p_memrsv 0x%08x p_swprsv 0x%08x p_xstat  0x%08x\n",
		p->p_mlink, p->p_memresv, p->p_swpresv, p->p_xstat);
	fprintf(outf, " p_time   0x%08x p_slptim 0x%08x p_sid    0x%08x p_sidhx  0x%08x\n",
		p->p_time, p->p_slptime, p->p_sid, p->p_sidhx);
	fprintf(outf, " p_idwrit 0x%08x p_fss    0x%08x p_dbpicp 0x%08x p_wakpri 0x%08x\n",
#ifdef AUDIT
		p->p_idwrite, p->p_fss, p->p_dbipcp, p->p_wakeup_pri);
#else
		0x0, p->p_fss, p->p_dbipcp, p->p_wakeup_pri);
#endif
	fprintf(outf, " p_reglck 0x%08x p_fillck 0x%08x p_dlchan 0x%08x p_faddr  0x%08x\n",
		p->p_reglocks, p->p_filelock, p->p_dlchan, p->p_faddr);

	fprintf(outf, " p_ndx    0x%08x p_cttyfp 0x%08x p_cttybp 0x%08x\n",
		p->p_ndx, p->p_cttyfp, p->p_cttybp);
#ifdef PSTAT
	fprintf(outf, " p_utim.s 0x%08x p_utim.u 0x%08x p_stim.s 0x%08x p_stim.u 0x%08x\n",
		p->p_utime.tv_sec, p->p_utime.tv_usec,
		p->p_stime.tv_sec, p->p_stime.tv_usec);
	fprintf(outf, " p_ttyd   0x%08x p_start  0x%08x p_ttyp   0x%08x p_wakcnt 0x%08x\n",
		p->p_ttyd, p->p_start, p->p_ttyp, p->p_wakeup_cnt);
#else
	fprintf(outf, " p_ttyp   0x%08x p_wakcnt 0x%08x\n",
		p->p_ttyp, p->p_wakeup_cnt);
#endif /* PSTAT */
#ifdef MP
	fprintf(outf, " p_prcnum 0x%08x\n",
		p->p_procnum);
#ifdef SYNC_SEMA_RECOVERY
	fprintf(outf, " p_recsem 0x%08x\n", p->p_recover_sema);
#endif
	fprintf(outf, " p_descnt 0x%08x p_desprc 0x%08x p_mpflag 0x%08x p_prcnum 0x%08x\n",
		p->p_descnt, p->p_desproc, p->p_mpflag, p->p_procnum);
	fprintf(outf, " p_wait_l 0x%08x p_rwait_ 0x%08x p_sleep_ 0x%08x p_sema   0x%08x\n",
		p->p_wait_list, p->p_rwait_list, p->p_sleep_sema, p->p_sema);
#ifdef SEMAPHORE_DEBUG
	fprintf(outf, " p_bsema  0x%08x p_sema_r 0x%08x\n",
		p->p_bsema, p->p_sema_reaquire);
#endif
#else  MP
	fprintf(outf, " p_prcnum 0x%08x\n", 0);
#endif MP

	fprintf(outf, " p_maxof  0x%08x p_cdir   0x%08x p_rdir   0x%08x p_ofilep 0x%08x\n",
		p->p_maxof, p->p_cdir, p->p_rdir, p->p_ofilep);
	fprintf(outf, " p_vforkb 0x%08x p_msem_i 0x%08x\n",
		p->p_vforkbuf, p->p_msem_info);

#ifdef _WSIO
	fprintf(outf, " p_dil_ef 0x%08x p_dil_el 0x%08x p_dil_signal   0x%02x\n",
		p->p_dil_event_f, p->p_dil_event_l, p->p_dil_signal);
	fprintf(outf, " p_addr   0x%08x p_segptr 0x%08x p_stapgs 0x%08x\n",
		p->p_addr, p->p_segptr, p->p_stackpages);
#endif /* _WSIO */

#ifdef __hp9000s800
#ifdef _WSIO
	fprintf(outf, " p_pindx      0x%04x graf_ss 0x%08x\n",
		p->p_pindx, p->graf_ss);
#else
	fprintf(outf, "p_pindx       0x%04x\n", p->p_pindx);
#endif
#endif /* __hp9000s800 */

	fprintf(outf, " p_realtimer:\n");
	fprintf(outf, " it_int.s 0x%08x it_int.u 0x%08x it_val.s 0x%08x it_val.u 0x%08x\n",
		p->p_realtimer.it_interval.tv_sec, p->p_realtimer.it_interval.tv_usec,
		p->p_realtimer.it_value.tv_sec, p->p_realtimer.it_value.tv_usec);

#ifdef __hp9000s800
#ifdef PESTAT
	fprintf(outf, " intr_i.s 0x%08x intr_i.u 0x%08x intr_t.s 0x%08x intr_t.u 0x%08x\n",
		p->intr_icstime.tv_sec, p->intr_icstime.tv_usec,
		p->intr_time.tv_sec, p->intr_time.tv_usec);
	fprintf(outf, " intr_trc 0x%08x intr_cid 0x%08x\n",
		p->intr_trace, p->intr_callid);
#endif /* PESTAT */
#endif /* __hp9000s800 */

	fprintf(outf, " proc  0x%08x \n", unlocalizer(p, proc, vproc));
	fprintf(outf, "\n");

#ifdef MP
	/*
	 * Print p_mpflag values
	 */
#define MP_ELSE (~(SLPT|SRUNPROC|SMPLOCK|SMP_SEMA_WAKE|SMP_STOP| \
		   SMP_SEMA_BLOCK|SMP_SEMA_NOSWAP))

	fprintf(outf, " MP Flags: ");
	if (mpflag & MP_ELSE)
		fprintf(outf, "MP_0x%08x ", mpflag & MP_ELSE);
	if (mpflag & SLPT)		fprintf(outf, "SLPT ");
	if (mpflag & SRUNPROC)		fprintf(outf, "SRUNPROC ");
	if (mpflag & SMPLOCK)		fprintf(outf, "SMPLOCK ");
	if (mpflag & SMP_SEMA_WAKE)	fprintf(outf, "SMP_SEMA_WAKE ");
	if (mpflag & SMP_STOP)		fprintf(outf, "SMP_STOP ");
	if (mpflag & SMP_SEMA_BLOCK)	fprintf(outf, "SMP_SEMA_BLOCK ");
	if (mpflag & SMP_SEMA_NOSWAP)	fprintf(outf, "SMP_SEMA_NOSWAP ");
	fprintf(outf, "\n");
#endif

	/*
	 * Print p_flag values
	 */
#ifdef _WSIO
#define S_ELSE (~(SLOAD|SSYS|SLOCK|STRC|SWTED|SKEEP|SOMASK|SWEXIT| \
		  SPHYSIO|SVFORK|SSEQL|SUANOM|SOUSIG|SOWEUPC|SSEL| \
		  SRTPROC|SSIGABL|SPRIV|SPREEMPT|		   \
		  SSTOPFAULTING|SSWAPPED|SFAULTING))
#else
#define S_ELSE (~(SLOAD|SSYS|SLOCK|STRC|SWTED|SKEEP|SOMASK|SWEXIT| \
		  SPHYSIO|SVFORK|SSEQL|SUANOM|SOUSIG|SOWEUPC|SSEL| \
		  SRTPROC|SSIGABL|SPRIV|SPREEMPT))
#endif /* _WSIO */

	fprintf(outf, " Flags: ");
	if (flag & S_ELSE)
		fprintf(outf, "S_0x%08x ", flag & S_ELSE);
	if (flag & SLOAD)	  fprintf(outf, "SLOAD ");
	if (flag & SSYS)	  fprintf(outf, "SSYS ");
	if (flag & SLOCK)	  fprintf(outf, "SLOCK ");
	if (flag & STRC)	  fprintf(outf, "STRC ");
	if (flag & SWTED)	  fprintf(outf, "SWTED ");
	if (flag & SKEEP)	  fprintf(outf, "SKEEP ");
	if (flag & SOMASK)	  fprintf(outf, "SOMASK ");
	if (flag & SWEXIT)	  fprintf(outf, "SWEXIT ");
	if (flag & SPHYSIO)	  fprintf(outf, "SPHYSIO ");
	if (flag & SVFORK)	  fprintf(outf, "SVFORK ");
	if (flag & SSEQL)	  fprintf(outf, "SSEQL ");
	if (flag & SUANOM)	  fprintf(outf, "SUANOM ");
	if (flag & SOUSIG)	  fprintf(outf, "SOUSIG ");
	if (flag & SOWEUPC)	  fprintf(outf, "SOWEUPC ");
	if (flag & SSEL)	  fprintf(outf, "SSEL ");
	if (flag & SRTPROC)	  fprintf(outf, "SRTPROC ");
	if (flag & SSIGABL)	  fprintf(outf, "SSIGABL ");
	if (flag & SPRIV)	  fprintf(outf, "SPRIV ");
	if (flag & SPREEMPT)	  fprintf(outf, "SPREEMPT ");
#ifdef _WSIO
	if (flag & SSTOPFAULTING) fprintf(outf, "SSTOPFAULTING ");
	if (flag & SSWAPPED)	  fprintf(outf, "SSWAPPED ");
	if (flag & SFAULTING)	  fprintf(outf, "SFAULTING ");
#endif /* _WSIO */

	/*
	 * Print p_flag2 values
	 */
#define S2_COMMON (S2CLDSTOP|S2EXEC|SGRAPHICS|SADOPTIVE|SANYPAGE| \
		   SPA_ON|S2POSIX_NO_TRUNC|SLKDONE|SISNFSLM| \
		   S2TRANSIENT)
#ifdef _WSIO
#define S2_WSIO   (S2SENDDILSIG)
#else
#define S2_WSIO   0
#endif

#ifdef __hp9000s800
#define S2_S800   (SSAVED|SCHANGED|SPURGE_SIDS)
#else
#define S2_S800   0
#endif

#ifdef __hp9000s300
#define S2_S300   (S2DATA_WT|S2STACK_WT)
#else
#define S2_S300   0
#endif

#define S2_ELSE (~(S2_COMMON|S2_WSIO|S2_S800|S2_S300))

	if (flag2 & S2_ELSE)
		fprintf(outf, "S2_0x%08x ", flag & S2_ELSE);
	if (flag2 & S2CLDSTOP)		fprintf(outf, "S2CLDSTOP ");
	if (flag2 & S2EXEC)		fprintf(outf, "S2EXEC ");
	if (flag2 & SGRAPHICS)		fprintf(outf, "SGRAPHICS ");
	if (flag2 & SADOPTIVE)		fprintf(outf, "SADOPTIVE ");
	if (flag2 & SANYPAGE)		fprintf(outf, "SANYPAGE ");
	if (flag2 & SPA_ON)		fprintf(outf, "SPA_ON ");
	if (flag2 & S2POSIX_NO_TRUNC)	fprintf(outf, "S2POSIX_NO_TRUNC ");
	if (flag2 & SLKDONE)		fprintf(outf, "SLKDONE ");
	if (flag2 & SISNFSLM)		fprintf(outf, "SISNFSLM ");
	if (flag2 & S2TRANSIENT)	fprintf(outf, "S2TRANSIENT ");
#ifdef _WSIO
	if (flag2 & S2SENDDILSIG)	fprintf(outf, "S2SENDDILSIG ");
#endif
#ifdef __hp9000s800
	if (flag2 & SSAVED)		fprintf(outf, "SSAVED ");
	if (flag2 & SCHANGED)		fprintf(outf, "SCHANGED ");
	if (flag2 & SPURGE_SIDS)	fprintf(outf, "SPURGE_SIDS ");
#endif
#ifdef __hp9000s300
	if (flag2 & S2DATA_WT)		fprintf(outf, "S2DATA_WT ");
	if (flag2 & S2STACK_WT)		fprintf(outf, "S2STACK_WT ");
#endif
	fprintf(outf, "\n");

	fprintf(outf, " Current state ");
	switch (stat) {
	case SSLEEP:
		fprintf(outf, "SSLEEP ");  break;
	case SWAIT:
		fprintf(outf, "SWAIT ");   break;
	case SRUN:
		fprintf(outf, "SRUN ");	   break;
	case SIDL:
		fprintf(outf, "SIDL ");	   break;
	case SZOMB:
		fprintf(outf, "SZOMB ");   break;
	case SSTOP:
		fprintf(outf, "SSTOP ");   break;
	default:
		fprintf(outf, "<unknown: %d> ", stat);
		break;
	}

	if ((p->p_rlink != 0) && (p->p_stat == SRUN)) {
		fprintf (outf, "on run queue ");
	}

	/* for grins lets find wchan */
	if (p->p_wchan) {
		diff = findsym(p->p_wchan);
		if (diff == -1)
			fprintf(outf, "WCHAN sym unknown\n");
		else
			if (diff) {
				fprintf(outf, "WCHAN = %s+0x%04x\n",
					cursym, diff);
			} else {
				fprintf(outf, "WCHAN = %s\n", cursym);
			}
	} else {
		fprintf(outf, "\n");
	}

	if (pst_cmds != NULL) {
		char buffer [PST_CLEN+1];
		int ret;

		ret = pstat_cmd_fill(pst_cmds, (p - lp), buffer);
		if (ret) {
			buffer[PST_CLEN] = 0;
			fprintf(outf, " Command:  %s\n", buffer);
		}
	}
}

set_pstat_cmds (address)
	int address;
{
	int local;

	if (address == 0) {
		pst_cmds = NULL;
		return;
	}
	longlseek (fcore, (long)clear(address), 0);
	if (longread (fcore, &local, sizeof (int)) != sizeof (int)) {
		pst_cmds = NULL;
		return;
	}
	pst_cmds = (char *)local;
}

pstat_cmd_fill (pst_cmds, index, buffer)
	char *pst_cmds;
	int   index;
	char *buffer;
{
	static int ps_addr = 0;
	static int pst_initialized = 0;

	if (pst_cmds == NULL)
		return (0);

	ps_addr = getphyaddr (((unsigned) pst_cmds) + (index * PST_CLEN));

	longlseek (fcore, (long)clear(ps_addr), 0);
	if (longread (fcore, buffer, PST_CLEN) != PST_CLEN) {
		return (0);
	}
	return (1);
}

char *exec_fifo[] = {
	"   i_execsites:  ",
	"   i_fifordsites:"
	};

/* Dump contents of inode */
/* Pass in cp (message) ip (pointer) lip (local block) vip (virtual block) */

dumpinode(cp, ip, lip, vip)
	char *cp;
	struct inode *ip, *lip, *vip;
{
	int icmode, icnlink;
	int icsize;
	struct inode *ino = (struct inode *)0;
	char *inodecommon, *inodeaddress;

	inodeaddress = (char *)(vip + (ip - lip));
#ifndef ACLS
	inodecommon = (inodeaddress + (int)(&ino->i_ic));
#else /* ACLS */
	inodecommon = (inodeaddress + (int)(&ino->i_icun.i_ic));
#endif /* ACLS */
	fprintf(outf, "Inode entry : %s    Inode address 0x%08x    i_ic address 0x%08x\n",
		cp, inodeaddress, inodecommon);

	fprintf(outf, "   i_flags  0x%08x  i_lockl 0x%08x  i_dev   0x%08x  i_fs 0x%08x\n",
		ip->i_flag, ip->i_locklist, ip->i_dev, ip->i_fs);

	icmode = ip->i_mode;
	icnlink = ip->i_nlink;

	/* ic_size in the inode is of type quad, and we need to print the
	 * entire value */
	icsize = (int)ip->i_size;
	fprintf(outf,
		"   ic_nlink 0x%08x  ic_size 0x%08x  ic_gen  0x%08x  ic_mode 0%06o\n",
		icnlink, icsize, ip->i_gen, icmode);
	fprintf(outf,
		"   i_number 0x%08x  i_un[0] 0x%08x  i_un[1] 0x%08x  i_dq 0x%08x\n",
		ip->i_number, ip->i_freef, ip->i_freeb, ip->i_dquot);
	fprintf(outf,
		"   i_vnode  0x%08x  i_devvp 0x%08x  ic_fver 0x%08x  i_pid %9d\n",
	((char *)vinode + ((char *)&ip->i_vnode - (char *)inode)),
	ip->i_devvp, ip->i_fversion, ip->i_pid);
	fprintf(outf,
		"   i_diroff 0x%08x  i_chain[0] 0x%08x  i_chain[1] 0x%08x\n",
		ip->i_diroff, ip->i_chain[0], ip->i_chain[1]);
	fprintf(outf,
		"   i_rdev   0x%08x  i_mount  0x%08x  i_ilocksite 0x%02x\n",
		ip->i_rdev, ip->i_mount, ip->i_ilocksite);
	dumpsitemap("   i_opensites:  ", ip->i_opensites);
	dumpsitemap("   i_writesites: ", ip->i_writesites);
	dumpsitemap(exec_fifo[ip->i_mode&IFIFO ? 1 : 0], ip->i_execsites);
	dumpsitemap("   i_refsites:   ", ip->i_refsites);
	fprintf(outf, "   i_execdcnt: r 0x%04x  v 0x%04x  i_refcnt: r 0x%04x  v 0x%04x\n",
		ip->i_execdcount.d_rcount, ip->i_execdcount.d_vcount,
		ip->i_refcount.d_rcount, ip->i_refcount.d_rcount);

#ifdef QFS
	fprintf(outf, "   i_hiwater 0x%08x  i_linkoff 0x%08x\n",
		ip->i_hiwater, ip->i_linkoff);
	fprintf(outf, "   ");
	qfs_qlink("i_qlink", &ip->i_qlink, &(vip+(ip-lip))->i_qlink);
#endif /* QFS */
	fprintf(outf, "   ic_db[0]   0x%05x 0x%05x 0x%05x 0x%05x 0x%05x 0x%05x\n",
		ip->i_db[0], ip->i_db[1], ip->i_db[2],
		ip->i_db[3], ip->i_db[4], ip->i_db[5]);
	fprintf(outf, "   ic_db[6]   0x%05x 0x%05x 0x%05x 0x%05x 0x%05x 0x%05x\n",
		ip->i_db[6], ip->i_db[7], ip->i_db[8],
		ip->i_db[9], ip->i_db[10], ip->i_db[11]);

#define I_ELSE (~(ILOCKED|IUPD|IACC|IWANT|ITEXT|ICHG|ILWAIT|IREF|	\
		  ISYNCLOCKED|ISYNC|IDUXMNT|ISYNCWANT|IDUXMRT|IBUFVALID \
		  |IPAGEVALID|IFRAGSYNC))

	/* Print out flags */
	if (ip->i_flag|ip->i_mode|icmode)
		fprintf(outf, "   ");
	if (ip->i_flag & I_ELSE)
		fprintf(outf, " I_0x%08x", ip->i_flag & I_ELSE);
	if (ip->i_flag & ILOCKED)
		fprintf(outf, " ILOCKED");
	if (ip->i_flag & IUPD)
		fprintf(outf, " IUPD");
	if (ip->i_flag & IACC)
		fprintf(outf, " IACC");
	if (ip->i_flag & IWANT)
		fprintf(outf, " IWANT");
	if (ip->i_flag & ITEXT)
		fprintf(outf, " ITEXT");
	if (ip->i_flag & ICHG)
		fprintf(outf, " ICHG");
	if (ip->i_flag & ILWAIT)
		fprintf(outf, " ILWAIT");
	if (ip->i_flag & IREF)
		fprintf(outf, " IREF");
	if (ip->i_flag & ISYNCLOCKED)
		fprintf(outf, " ISYNCLOCKED");
	if (ip->i_flag & ISYNC)
		fprintf(outf, " ISYNC");
	if (ip->i_flag & IDUXMNT)
		fprintf(outf, " IDUXMNT");
	if (ip->i_flag & ISYNCWANT)
		fprintf(outf, " ISYNCWANT");
	if (ip->i_flag & IDUXMRT)
		fprintf(outf, " IDUXMRT");
	if (ip->i_flag & IBUFVALID)
		fprintf(outf, " IBUFVALID");
	if (ip->i_flag & IPAGEVALID)
		fprintf(outf, " IPAGEVALID");
	if (ip->i_flag & IFRAGSYNC)
		fprintf(outf, " IFRAGSYNC");
	if (ip->i_flag)
		fprintf(outf, "\n   ");

	/* print out modes */

	icmode = icmode & IFMT;

	if (icmode == IFIFO)
		fprintf(outf, " IFIFO");
	if (icmode == IFNWK)
		fprintf(outf, " IFNWK");
	if (icmode == IFIR)
		fprintf(outf, " IFIR");
	if (icmode == IFIW)
		fprintf(outf, " IFIW");
	if (icmode == IFBLK)
		fprintf(outf, " IFBLK");
	if (icmode == IFCHR)
		fprintf(outf, " IFCHR");
	if (icmode == IFDIR)
		fprintf(outf, " IFDIR");

	if (icmode == IFREG)
		fprintf(outf, " IFREG");
	if (icmode == IFLNK)
		fprintf(outf, " IFLNK");
	if (icmode == IFSOCK)
		fprintf(outf, " IFSOCK");

	if (ip->i_mode & ISUID)
		fprintf(outf, " ISUID");
	if (ip->i_mode & ISGID)
		fprintf(outf, " ISGID");
	if (ip->i_mode & IENFMT)
		fprintf(outf, " IENFMT");
	if (ip->i_mode & ISVTX)
		fprintf(outf, " ISVTX");
	if (ip->i_mode & IREAD)
		fprintf(outf, " IREAD");
	if (ip->i_mode & IWRITE)
		fprintf(outf, " IWRITE");
	if (ip->i_mode & IEXEC)
		fprintf(outf, " IEXEC");
	if (ip->i_mode)
		fprintf(outf, "\n");

	dumpvnode("", &ip->i_vnode, ((char *)vinode +
			((char *)&ip->i_vnode - (char *)inode)));
}

static char *vnodetypes[] = {
    "VNON",
    "VREG",
    "VDIR",
    "VBLK",
    "VCHR",
    "VLNK",
    "VSOCK",
    "VBAD",
    "VFIFO",
    "VFNWK",
    "VEMPTYDIR"
};

static char *vfstypes[] = {
    "VDUMMY",
    "VNFS",
    "VUFS",
    "VDUX",
    "VDUX_PV",
    "VDEV_VN"
};

/* Dump contents of a vnode */
/* Pass in cp (message) lvp (local vnode pointer)  */
dumpvnode(cp, lvp, vvp)
	char *cp;
	struct vnode *lvp, *vvp;
{
	fprintf(outf, "Vnode entry : %s  Vnode address 0x%08x  vfstype %s\n",
		cp, vvp, vfstypes[(int)lvp->v_fstype]);
	fprintf(outf,
	    "    v_flag 0x%08x  v_cnt  0x%08x  v_op   0x%08x  v_vfsp 0x%08x\n",
	    lvp->v_flag, lvp->v_count, lvp->v_op, lvp->v_vfsp);
	fprintf(outf,
	    "    v_type %10s  v_rdev 0x%08x  v_data 0x%08x  \n",
	    vnodetypes[(int)lvp->v_type], lvp->v_rdev, lvp->v_data);
	fprintf(outf,
	    "    v_vas 0x%08x  v_tcount 0x%08x  \n",
	    lvp->v_vas, lvp->v_tcount);
	fprintf(outf,
	    "   clean  0x%08x  dirty  0x%08x  ord_d  0x%08x  ord_m  0x%08x\n",
	    lvp->v_cleanblkhd, lvp->v_dirtyblkhd,
	    lvp->v_ord_lastdatalink, lvp->v_ord_lastmetalink);

	if (lvp->v_shlockc | lvp->v_exlockc )
		fprintf(outf, "    v_shlk 0x%08x  v_exlk 0x%08x\n",
			lvp->v_shlockc, lvp->v_exlockc);
	if (lvp->v_socket)
		fprintf(outf, "    v_sock 0x%08x\n", lvp->v_socket);
	if (lvp->v_vfsmountedhere)
		fprintf(outf, "    v_vfsmountedhere: 0x%08x\n",
			lvp->v_vfsmountedhere);

#define VF_ELSE (~(VROOT|VTEXT|VEXLOCK|VSHLOCK|VLWAIT))

	/* Print out flags */
	if (lvp->v_flag)
		fprintf(outf, "   ");
	if (lvp->v_flag & VF_ELSE)
		fprintf(outf, " V_0x%08x", lvp->v_flag & VF_ELSE);
	if (lvp->v_flag & VROOT)
		fprintf(outf, " VROOT");
	if (lvp->v_flag & VTEXT)
		fprintf(outf, " VTEXT");
	if (lvp->v_flag & VEXLOCK)
		fprintf(outf, " VEXLOCK");
	if (lvp->v_flag & VSHLOCK)
		fprintf(outf, " VSHLOCK");
	if (lvp->v_flag & VLWAIT)
		fprintf(outf, " VLWAIT");
#ifdef nodef
	if (lvp->v_flag)
#endif
		fprintf(outf, "\n   ");

#ifdef notdef
	/* print out types */

	if (lvp->v_type == VNON)
		fprintf(outf, " VNON");
	if (lvp->v_type == VREG)
		fprintf(outf, " VREG");
	if (lvp->v_type == VDIR)
		fprintf(outf, " VDIR");
	if (lvp->v_type == VBLK)
		fprintf(outf, " VBLK");
	if (lvp->v_type == VCHR)
		fprintf(outf, " VCHR");
	if (lvp->v_type == VLNK)
		fprintf(outf, " VLNK");
	if (lvp->v_type == VSOCK)
		fprintf(outf, " VSOCK");
	if (lvp->v_type == VFIFO)
		fprintf(outf, " VFIFO");
	if (lvp->v_type == VBAD)
		fprintf(outf, " VBAD");
	if (lvp->v_type == VFNWK)
		fprintf(outf, " VFNWK");
	if (lvp->v_type == VEMPTYDIR)
		fprintf(outf, " VEMPTYDIR");
	if (lvp->v_type)
		fprintf(outf, "\n");
#endif notdef
}


dumpcred(cp, vcrp)
	char *cp;
	struct ucred *vcrp;
{
	int i;
	struct ucred lcred;

	if (getchunk(KERNELSPACE, vcrp, &lcred, sizeof (struct ucred), "dumpcred"))
		return(-1);
	fprintf(outf,
		"%s address 0x%08x  cr_ref 0x%08x  cr_uid %5hu  cr_gid %5hu\n",
		cp, vcrp, lcred.cr_ref, lcred.cr_uid, lcred.cr_gid);
	fprintf(outf, "    cr_ruid %05hu  cr_rgid %05hu",
		lcred.cr_ruid, lcred.cr_rgid);
	fprintf(outf, "  cr_groups:");
	for (i = 0; lcred.cr_groups[i] != NOGROUP; i++) {
	    if (i >= NGROUPS) {
		fprintf(outf, "...\nWarning: Credential group list not terminated\n");
		break;
	    }
	    fprintf(outf, " %d", lcred.cr_groups[i]);
	}
	fprintf(outf, "\n");
}

char *nfsftypes[] = {
	"NFNON",
	"NFREG",		/* regular file */
	"NFDIR",		/* directory */
	"NFBLK",		/* block special */
	"NFCHR",		/* character special */
	"NFLNK"			/* symbolic link */
};

dumprnode(cp, rp, vrp)
	char *cp;
	struct rnode *rp, *vrp;
{
	char buf[MAXNAMLEN];
	struct nfsfattr *ap;

	fprintf(outf,
		"Rnode entry : %s  Rnode address 0x%08x  r_flags 0x%08x",
		cp, vrp, rp->r_flags);
	if (rp->r_rcred) {
	    fprintf(outf, "\n");
	    dumpcred("  r_rcred:", rp->r_rcred);
	} else
	    fprintf(outf, " r_rcred 0x0\n");
	if (rp->r_wcred) {
	    fprintf(outf, "\n");
	    dumpcred("  r_wcred:", rp->r_wcred);
	} else
	    fprintf(outf, " r_wcred 0x0\n");
	fprintf(outf,
	    "  r_next 0x%08x  r_lastr 0x%08x  r_size 0x%08x  r_error 0x%04x\n",
		rp->r_next, rp->r_lastr, rp->r_size, rp->r_error);
	ap = &(rp->r_nfsattr);
	fprintf(outf,
		"  r_nfsattr: na_type %s  na_mode 0%06o  na_nlink 0x%08x\n",
		nfsftypes[(int)ap->na_type], ap->na_mode, ap->na_nlink);
	fprintf(outf,
		"    na_size 0x%08x  na_blksiz 0x%08x  na_rdev 0x%08x  na_uid %d\n",
		ap->na_size, ap->na_blocksize, ap->na_rdev, ap->na_uid);
	fprintf(outf,
		"    na_blks 0x%08x  na_nodeid 0x%08x  na_fsid 0x%08x  na_gid %d\n",
		ap->na_blocks, ap->na_fsid, ap->na_nodeid, ap->na_gid);
	fprintf(outf,
		"    na_atime 0x%08x 0x%08x       na_mtime 0x%08x 0x%08x\n",
		ap->na_atime.tv_sec, ap->na_atime.tv_usec,
		ap->na_mtime.tv_sec, ap->na_mtime.tv_usec);
	fprintf(outf,
		"    na_ctime 0x%08x 0x%08x  r_nfsattrtime 0x%08x 0x%08x\n",
		ap->na_ctime.tv_sec, ap->na_ctime.tv_usec,
		rp->r_nfsattrtime.tv_sec, rp->r_nfsattrtime.tv_sec);
	if (rp->r_unlcred) {
	    dumpcred("  r_unlcred:", rp->r_unlcred);
	    if (getchunk(KERNELSPACE, rp->r_unlname, buf, MAXNAMLEN, "dumprnode"))
		    goto skip;
	    fprintf(outf, "r_unlname:  %s   r_unldvp:  0x%08x\n",
		    buf, rp->r_unldvp);
	}
	/*
	 * The following assumes NFS_FHSIZE == 32 bytes
	 */
skip:	{
	fhandle_t foo;
	int *lp = (int *)&foo;
	foo = rp->r_fh;
	fprintf(outf,
	"  r_fh: 0x%08x 0x%08x 0x%08x 0x%08x\n", lp[0], lp[1], lp[2], lp[3]);
	fprintf(outf,
	"        0x%08x 0x%08x 0x%08x 0x%08x\n", lp[4], lp[5], lp[6], lp[7]);
	}

#define RF_ELSE (~(RLOCKED|RWANT|RATTRVALID|REOF|RDIRTY|ROPEN))

	if (rp->r_flags) {
	    fprintf(outf, "  ");
	    if (rp->r_flags & RF_ELSE)
		fprintf(outf, " R_0x%08x", rp->r_flags & RF_ELSE);
	    if (rp->r_flags & RLOCKED)
		fprintf(outf, " RLOCKED");
	    if (rp->r_flags & RWANT)
		fprintf(outf, " RWANT");
	    if (rp->r_flags & RATTRVALID)
		fprintf(outf, " RATTRVALID");
	    if (rp->r_flags & REOF)
		fprintf(outf, " REOF");
	    if (rp->r_flags & RDIRTY)
		fprintf(outf, " RDIRTY");
	    if (rp->r_flags & ROPEN)
		fprintf(outf, " ROPEN");
	    fprintf(outf, "\n");
	}
	dumpvnode("", &(rp)->r_vnode,
		  (char *)vrp + (int)((char *)&(rp)->r_vnode - (char *)(rp)));
}

char *vfsmtypes[] = {
	"MOUNT_UFS",
	"MOUNT_NFS"
};

/* Dump contents of a vfs */
/* Pass in cp (message) vfsp (vfs pointer) and vvfs (virtual vfs pointer)  */
dumpvfs(cp, vfsp, vvfs)
	char *cp;
	struct vfs *vfsp, *vvfs;
{
	struct vfs vfs1;

	fprintf(outf, "Vfs entry : %s    vfs address 0x%08x \n", cp, vvfs);

#ifdef GETMOUNT
	fprintf(outf, "   vfs_name %s\n", vfsp->vfs_name);
#endif /* GETMOUNT */

	fprintf(outf,
	    "   vfs_next  0x%08x  vfs_op  0x%08x  vfs_bsize  0x%08x\n",
	    vfsp->vfs_next, vfsp->vfs_op, vfsp->vfs_bsize);
	fprintf(outf,
	    "   vfs_flag  0x%08x  vfs_cov 0x%08x  vfs_exroot 0x%08x\n",
	    vfsp->vfs_flag, vfsp->vfs_vnodecovered, vfsp->vfs_exroot);
	fprintf(outf,
	    "   vfs_exflg 0x%08x  vfs_dat 0x%08x  vfs_mtype  %s\n",
	    vfsp->vfs_exflags, vfsp->vfs_data, vfsmtypes[vfsp->vfs_mtype]);
	fprintf(outf,
	    "   vfs_icount  %8d  vfs_site    0x%04x\n",
	    vfsp->vfs_icount, vfsp->vfs_site);
	fprintf(outf,
	    "   vfs_fsid  0x%08x.%08x\n",
	    vfsp->vfs_fsid[0], vfsp->vfs_fsid[1]);

#ifdef GETMOUNT
	fprintf(outf, "   vfs_mnttime %8d\n", vfsp->vfs_mnttime);
#endif /* GETMOUNT */

#ifdef QFS
	fprintf(outf, "   vfs_logp  0x%08x\n", vfsp->vfs_logp);
#endif /* QFS */

#ifdef __hp9000s300
#define VFS_MI_DEV	0
#endif

#define VFS_ELSE (~(VFS_RDONLY|VFS_MLOCK|VFS_MWAIT|VFS_NOSUID|\
		    VFS_EXPORTED|VFS_MI_DEV))

	/* Print out flags */
	if (vfsp->vfs_flag)
		fprintf(outf, "   ");
	if (vfsp->vfs_flag & VFS_ELSE)
		fprintf(outf, " VFS_0x%08x", vfsp->vfs_flag & VFS_ELSE);
	if (vfsp->vfs_flag & VFS_RDONLY)
		fprintf(outf, " VFS_RDONLY");
	if (vfsp->vfs_flag & VFS_MLOCK)
		fprintf(outf, " VFS_MLOCK");
	if (vfsp->vfs_flag & VFS_MWAIT)
		fprintf(outf, " VFS_MWAIT");
	if (vfsp->vfs_flag & VFS_NOSUID)
		fprintf(outf, " VFS_NOSUID");
	if (vfsp->vfs_flag & VFS_EXPORTED)
		fprintf(outf, " VFS_EXPORTED");
	if (vfsp->vfs_flag & VFS_MI_DEV)
		fprintf(outf, " VFS_MI_DEV");
	if (vfsp->vfs_flag)
		fprintf(outf, "\n");

#ifdef QFS
	if (vfsp->vfs_logp && Jflg)
		qfs_loghdr(vfsp->vfs_logp);
#endif /* QFS */
}


dumpncache(cp, ncp, vncp)
	char *cp;
	struct ncache *ncp, *vncp;
{
	ncp->name[ncp->namlen] = (char)0;
	fprintf(outf, "ncache entry: %s  address 0x%08x  namlen 0x%x  name: %s\n",
		cp, vncp, ncp->namlen, ncp->name);
	fprintf(outf, "  hash_next 0x%08x  hash_prev  0x%08x  vp 0x%08x\n",
		ncp->hash_next, ncp->hash_prev, ncp->vp);
	fprintf(outf, "  lru_next  0x%08x  lru_prev   0x%08x  dp 0x%08x",
		ncp->lru_next, ncp->lru_prev, ncp->dp);
	if (ncp->cred) {
		fprintf(outf, "\n");
		dumpcred("  cred:", ncp->cred);
	} else
		fprintf(outf, "  cred 0x0\n");
}

dumpmt(cp, mp, vmp)
	char *cp;
	register struct mount *mp, *vmp;
{
	int brmtdev;

	fprintf(outf, "%s   Mount address 0x%08x\n", cp, vmp);

	brmtdev = get(lookup("brmtdev"));
	if (major(mp->m_dev) == brmtdev)
	{
	    /* mp->m_dfs->dfs_fsmt */
	    u_long addr;
	    struct duxfs m_dfs;

	    addr = mp->m_dfs;
	    if (getchunk(KERNELSPACE, addr, &m_dfs,
			 sizeof m_dfs, "dumpmt") == 0)
	    {
		fprintf(outf, "   mounted on %s\n", m_dfs.dfs_fsmnt);
	    }
	}
	else
	{
	    /* mp->m_bufp->b_un.b_fs->fs_fsmt */

	    u_long addr;
	    struct buf buf;

	    addr = mp->m_bufp;
	    if (getchunk(KERNELSPACE, addr, &buf,
			 sizeof buf, "dumpmt") == 0)
	    {
		struct fs b_fs;

		addr = buf.b_un.b_fs;
		if (getchunk(KERNELSPACE, addr, &b_fs,
			     sizeof b_fs, "dumpmt") == 0)
		{
		    fprintf(outf, "   mounted on %s\n",
			b_fs.fs_fsmnt);
		}
	    }
	}

	fprintf(outf, "   m_hforw   0x%08x  m_hback   0x%08x\n",
		mp->m_hforw, mp->m_hback);
	fprintf(outf, "   m_rhforw  0x%08x  m_rhback  0x%08x\n",
		mp->m_rhforw, mp->m_rhback);
	fprintf(outf, "   m_dev  0x%08x  m_bufp    0x%08x  m_vfsp 0x%08x\n",
		mp->m_dev, mp->m_bufp, mp->m_vfsp);
	fprintf(outf, "   m_flag 0x%08x  m_dfs     0x%08x\n",
		mp->m_flag, mp->m_dfs);
	fprintf(outf, "   m_rdev 0x%08x  m_maxbufs 0x%08x  m_site 0x%02x\n",
		mp->m_rdev, mp->m_maxbufs, mp->m_site);
	dumpsitemap("   m_dwrites: ", &mp->m_dwrites);


#define	M_ELSE (~(MAVAIL|MINUSE|MINTER|MUNMNT|M_IS_SYNC|	\
		  M_WAS_SYNC|M_NOTIFYING|M_RMTMDIR|M_FLOATING))

	if (mp->m_flag)
		fprintf(outf, "   ");
	if (mp->m_flag & M_ELSE)
		fprintf(outf, " M_0x%08x", mp->m_flag & M_ELSE);
	if (mp->m_flag & MINUSE)
		fprintf(outf, " MINUSE");
	if (mp->m_flag & MINTER)
		fprintf(outf, " MINTER");
	if (mp->m_flag & MUNMNT)
		fprintf(outf, " MUNMNT");
	if (mp->m_flag & M_IS_SYNC)
		fprintf(outf, " M_IS_SYNC");
	if (mp->m_flag & M_WAS_SYNC)
		fprintf(outf, " M_WAS_SYNC");
	if (mp->m_flag & M_NOTIFYING)
		fprintf(outf, " M_NOTIFYING");
	if (mp->m_flag & M_RMTMDIR)
		fprintf(outf, " M_RMTMDIR");
	if (mp->m_flag & M_FLOATING)
		fprintf(outf, " M_FLOATING");
	if (mp->m_flag)
		fprintf(outf, "\n");
}

dumpft(cp, fp, lfp, vfp)
	char *cp;
	register struct file *fp, *lfp, *vfp;
{
	fprintf(outf, "File entry : %s   File address 0x%08x   file[%d]\n",
		cp, (vfp + (fp-lfp)), (fp-lfp));
	fprintf(outf, "   f_flag 0x%08x  f_count  0x%08x  f_type ",
		fp->f_flag, fp->f_count);
	switch (fp->f_type) {
	case DTYPE_VNODE:
		fprintf(outf, "VNODE\n");
		break;
	case DTYPE_SOCKET:
		fprintf(outf, "SOCKET\n");
		break;
#ifdef DTYPE_REMOTE
	case DTYPE_REMOTE:
		fprintf(outf, "RFA\n");
		break;
#endif
	case DTYPE_UNSP:
		fprintf(outf, "UNSP\n");
		break;
	default:
		fprintf(outf, "0x%08x\n", fp->f_type);
	}
	fprintf(outf, "   f_data 0x%08x  f_offset 0x%08x  f_cred 0x%08x\n",
		fp->f_data, fp->f_offset, fp->f_cred);
}

dump_device_table(cp, hash, vaddr, dtp)
	char *cp;
	u_long hash;
	u_long vaddr;
	dtaddr_t dtp;
{
	fprintf(outf, "%s addr   0x%08x  dt_flags 0x%08x ",
		cp, vaddr, dtp->dt_flags);

#define D_ELSE	(~(D_INUSE|D_ALLCLOSES|D_LOCKED))

	if (dtp->dt_flags & D_ELSE)
		fprintf(outf, " D_0x%08x", dtp->dt_flags & D_ELSE);
	if (dtp->dt_flags & D_INUSE)
		fprintf(outf, " D_INUSE");
	if (dtp->dt_flags & D_ALLCLOSES)
		fprintf(outf, " D_ALLCLOSES");
	if (dtp->dt_flags & D_LOCKED)
		fprintf(outf, " D_LOCKED");
	fprintf(outf, "\n dt_dev 0x%08x  dt_mode  0x%08x  dt_cnt %5d\n",
		dtp->dt_dev, dtp->dt_mode, dtp->dt_cnt);
	fprintf(outf, " dt_id  %10d  dt_next  0x%08x  hash   %5d\n\n",
		dtp->dt_id, dtp->next, hash);
}


dumpmntinfo(cp, mip)
	char *cp;
	register struct mntinfo *mip;
{
	struct mntinfo mntbuf;

	if (getchunk(KERNELSPACE, mip, &mntbuf, sizeof(struct mntinfo), "dumpmntinfo"))
		return(-1);
	fprintf(outf,
	    "NFS Mount entry : %s  Mount address 0x%08x  mi_hostname %s\n",
	     cp, mip, mntbuf.mi_hostname);
#ifdef GETMOUNT
	fprintf(outf, "   mounted on %s\n", mntbuf.mi_fsmnt);
#endif
	fprintf(outf,
	    "   mi_addr  0x%08x  mi_rootvp  0x%08x  mi_refct 0x%08x\n",
	    mntbuf.mi_addr, mntbuf.mi_rootvp, mntbuf.mi_refct);
	fprintf(outf,
	    "   mi_tsize 0x%08x  mi_stsize  0x%08x  mi_bsize 0x%08x\n",
	    mntbuf.mi_tsize, mntbuf.mi_stsize, mntbuf.mi_bsize);
	fprintf(outf,
	    "   mi_mntno 0x%08x  mi_retrans 0x%08x  mi_timeo 0x%08x\n",
	    mntbuf.mi_mntno, mntbuf.mi_timeo, mntbuf.mi_retrans);
	fprintf(outf, "   mi_next  0x%08x  mi_vfs     0x%08x\n",
		mntbuf.mi_next, mntbuf.mi_vfs);
	fprintf(outf,
	    "   mi_hard %1d  mi_printed %1d  mi_int %1d  mi_down %1d\n",
	    mntbuf.mi_hard, mntbuf.mi_printed, mntbuf.mi_int, mntbuf.mi_down);
}


#ifdef __hp9000s800
/* Dump the contents of a pde entry */
dumppde(cp,pde,index)
char *cp;
register struct hpde *pde;
register int index;
{
	unsigned int pde_space_e, pde_page_e;
	unsigned int pde_space_o, pde_page_o;
	/* These values should be available via macros in pde.h! */
	static char *usage_bits[] = {
		"FREE_ENTRY",		/* 0 */
		"ODD_HALF_USED",	/* 1 */
		"EVEN_HALF_USED",	/* 2 */
		"BOTH_HALVES_USED",	/* 3 */
	};
	pde_space_e = pde->pde_space_e;
	pde_page_e = ((pde->pde_page_e << 5) | (((int)pde & 0x1E0) >> 4));  /*YECHH-JFB*/
	pde_space_o = pde->pde_space_o;
	pde_page_o = (((pde->pde_page_o << 5) | (((int)pde & 0x1E0) >> 4))+1);  /*YECHH-JFB*/

	fprintf(outf,"   Current %s index: 0x%08x  usage: 0x%x (%s)\n",
		cp,index,pde->pde_os, usage_bits[pde->pde_os & 0x3]);

	/* print even half */
	fprintf(outf, "     pde_space 0x%08x  pde_page   0x%08x  (shifted<0x%08x)\n",
		pde_space_e, pde_page_e, (pde_page_e<<PGSHIFT));
	fprintf(outf, "     pde_ar_e 0x%02x   pde_protid_e 0x%04x  (shifted>0x%04x)   pde_phys_e 0x%04x\n",
		pde->pde_ar_e, pde->pde_protid_e, (pde->pde_protid_e>>1),
		pde->pde_phys_e);
	fprintf(outf, "     pde_next  0x%08x\n", pde->pde_next);
	fprintf(outf, "     ");
	if (pde->pde_valid_e)		fprintf(outf, "PDE_VALID ");
	if (pde->pde_ref_e)		fprintf(outf, "PDE_REF ");
	if (pde->pde_rtrap_e)		fprintf(outf, "PDE_RTRAP ");
	if (pde->pde_dirty_e)		fprintf(outf, "PDE_DIRTY ");
	if (pde->pde_dbrk_e)		fprintf(outf, "PDE_DBRK ");
	if (pde->pde_shadow_e)		fprintf(outf, "PDE_BUDDY ");
	if (pde->pde_executed_e)	fprintf(outf, "PDE_EXECUTED ");
	if (pde->pde_accessed_e)	fprintf(outf, "PDE_ACCESSED(in cache) ");
	if (pde->pde_modified_e)	fprintf(outf, "PDE_MODIFIED(in cache)");
	fprintf(outf, "\n");

	/* print odd half */
	fprintf(outf, "      pde_space 0x%08x  pde_page   0x%08x  (shifted<0x%08x)\n",
		pde_space_o, pde_page_o, pde_page_o<<PGSHIFT);
	fprintf(outf, "      pde_ar_o 0x%02x   pde_protid_o 0x%04x  (shifted>0x%04x)   pde_phys_o 0x%04x\n",
		pde->pde_ar_o, pde->pde_protid_o, (pde->pde_protid_o>>1),
		pde->pde_phys_o);
	fprintf(outf, "      ");
	if (pde->pde_valid_o)		fprintf(outf, "PDE_VALID ");
	if (pde->pde_ref_o)		fprintf(outf, "PDE_REF ");
	if (pde->pde_rtrap_o)		fprintf(outf, "PDE_RTRAP ");
	if (pde->pde_dirty_o)		fprintf(outf, "PDE_DIRTY ");
	if (pde->pde_dbrk_o)		fprintf(outf, "PDE_DBRK ");
	if (pde->pde_shadow_o)		fprintf(outf, "PDE_BUDDY ");
	if (pde->pde_executed_e)	fprintf(outf, "PDE_EXECUTED ");
	if (pde->pde_accessed_o)	fprintf(outf, "PDE_ACCESSED(in cache) ");
	if (pde->pde_modified_o)	fprintf(outf, "PDE_MODIFIED(in cache)");
	fprintf(outf, "\n");
}
#endif /* __hp9000s800 */


/* Dump the contents of a kmemstats entry */
dumpkmemstat(cp, kmemst, index)
	char *cp;
	register struct kmemstats *kmemst;
	int index;
{
/* MEMSTATS */
	fprintf(outf, "   Kmemstat [%2d] ", index);

	if (index < M_LAST)
		fprintf(outf, "  %s\n", kmemnames[index]);
	else
		fprintf(outf, "   unknown M_TYPE %d\n", index);

	fprintf(outf, "   ks_flags  0x%04x  ks_sleep 0x%04x  ks_reslimit 0x%04x  ks_resinuse 0x%04x\n",
	    kmemst->ks_flags, kmemst->ks_sleep, kmemst->ks_reslimit, kmemst->ks_resinuse);

#ifdef OSDEBUG
	if (kmemst->ks_maxused == 0) {
		return;
	}
	fprintf(outf, "   ks_inuse  0x%04x  ks_calls 0x%04x  ks_memuse   0x%04x\n",
	    kmemst->ks_inuse, kmemst->ks_calls, kmemst->ks_memuse);
	fprintf(outf, "   ks_maxused  0x%04x\n", kmemst->ks_maxused);
#endif

	fprintf(outf, "\n");
}

/* Dump the contents of a rpb */
dumpsavestate(ss)
	register int *ss;
{
	register int i;

	fprintf(outf, "\n");
	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*  Save state                                       *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	for (i = 0; i < 60; i++) {
		if ((i % 3) == 0)
			fprintf(outf, "\n");

		fprintf(outf, " %s =0x%08x  ",
			savestatedescriptor[i], *(ss +i));
	}

	fprintf(outf, "\n\n");
}

#ifdef __hp9000s800

dump_crash_processor_table()
{
	extern int crash_processor_table [];
	struct crash_proc_table_struct *cpt_ptr;
	int i;
	int max_entries;

	fprintf(outf, "\n\n*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*  Crash Processor Table                            *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	cpt_ptr = (struct crash_proc_table_struct *) crash_processor_table;
	max_entries = (CRASH_TABLE_SIZE / sizeof (struct crash_proc_table_struct));
	for (i = 0; i < max_entries; i++) {
		if (cpt_ptr->cpt_hpa == NULL) {
			break;
		}
		fprintf(outf, "\n CPT Table Index %d:\n", i);
#ifdef MP
		 /* figure out which slot the processor if plugged in.
		    Currently this will work only for MP systems in a
		    850/855 box */
#if defined(__hp9000s800) && defined(_WSIO)
		if (! is_snake()) {
			int i;
			i = (cpt_ptr->cpt_hpa & 0x7000) >> 12;
			switch(i) {
			case 0x0000:
				fprintf(outf,
					"   Processor Slot Number = 0\n");
				break;
			case 0x0001:
				fprintf(outf,
					"   Processor Slot Number = 1\n");
				break;
			case 0x0010:
			case 0x0011:
				fprintf(outf,
					"   Processor Slot Number = 2\n");
				break;
			default:
				 fprintf(outf,
					"   Processor Slot Number = 3\n");
				break;
			}
		}
#else /* defined(__hp9000s800) && defined(_WSIO) */
		{ int i;
		  i = (cpt_ptr->cpt_hpa & 0x7000) >> 12;
		  switch(i) {
		  case 0x0000:
			  fprintf(outf, "   Processor Slot Number = 0\n");
			  break;
		  case 0x0001:
			  fprintf(outf, "   Processor Slot Number = 1\n");
			  break;
		  case 0x0010:
		  case 0x0011:
			  fprintf(outf, "   Processor Slot Number = 2\n");
			  break;
		  default: fprintf(outf, "   Processor Slot Number = 3\n");
			   break;
		  }
		}
#endif /* defined(__hp9000s800) && defined(_WSIO) */
#endif /* MP */
		fprintf(outf, "   Processor HPA = 0x%08X   PDC Vector = 0x%08X   IVA = 0x%08X\n",
			cpt_ptr->cpt_hpa, cpt_ptr->cpt_pdc_vector, cpt_ptr->cpt_iva);
		fprintf(outf, "   HPMC RPB Address = 0x%08X    TOC RPB Address = 0x%08X\n",
			cpt_ptr->cpt_hpmc_savestate, cpt_ptr->cpt_toc_savestate);
		fprintf(outf, "   HPMC Count       = 0x%08X    TOC count       = 0x%08X\n",
			cpt_ptr->cpt_hpmc_count, cpt_ptr->cpt_toc_count);
		fprintf(outf, "   mpinfo table index = 0x%08d  State Saved = 0x%08X (%s)\n",
			hpa_to_mpproc_index (cpt_ptr->cpt_hpa),
			cpt_ptr->cpt_state_saved, cpt_ptr->cpt_state_saved ? "YES" : "NO");
		cpt_ptr++;
	}
	fprintf(outf, "\n");
}


hpa_to_mpproc_index(hpa)
	unsigned int hpa;
{
#ifdef MP
	int i;

	for (i = 0; i < MAX_PROCS; i++) {
		if (mp[i].prochpa == hpa)
			return (i);
	}
	return (-1);
#else
	return (0);
#endif
}

hpa_to_processor_number(hpa)
	unsigned int hpa;
{
	int SMB_processor_convention;
	int processor_number;

#if defined(__hp9000s800) && defined(_WSIO)
	/* for now just assume snake machines are uniprocessor */
	if (is_snake()) {
		return 0;
	}
#endif /* defined(__hp9000s800) && defined(_WSIO) */
	processor_number = ((hpa & 0x0003F000) >> 12);
	/* SMB uses an unusual mapping to number its four processors.
	 * It would be correct to use processor number as set above,
	 * but to help the user map HPA to processor slot number,
	 * we use the SMB convention.  If we ever support processors with
	 * another numbering convention, this will have to be changed
	 * to check the cpu type in the dump and number appropriately.
	 */
	SMB_processor_convention = 1;
	if (SMB_processor_convention) {
		switch (processor_number) {
			case 0:
			case 8:
				processor_number = 0;
				break;

			case 9:
				processor_number = 1;
				break;

			case 0xA:
			case 0xB:
				processor_number = 2;
				break;

			case 0xC:
			case 0xD:
			case 0xE:
			case 0xF:
				processor_number = 3;
				break;

			default:
				processor_number = -1;
				break;
		}
	}
	return (processor_number);
}

dump_event_table()
{
	extern int crash_event_table [];
	struct crash_event_table_struct *cet_ptr;
	char *type;
	int i;
	int max_entries;
	int weird;
	int processor_number;

	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*  Crash Event Table                                *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "\n");

	cet_ptr = (struct crash_event_table_struct *) crash_event_table;
	max_entries = (CRASH_EVENT_TABLE_SIZE / sizeof (struct crash_event_table_struct));
	for (i = 0; i < max_entries; i++) {
		if (cet_ptr->cet_hpa == NULL) {
			break;
		}
		weird = 0;
		switch (cet_ptr->cet_event) {
			case CT_PANIC:
				type = "Panic";
				break;
			case CT_HPMC:
				type = "HPMC ";
				break;
			case CT_TOC:
				type = "TOC  ";
				break;
			default:
				type = "WEIRD";
				weird = 1;
				break;
		}
		/*
		 * This test will have to change if we ever support processors
		 * on remote busses.  That's a remote possibility...
		 */
		if ((cet_ptr->cet_hpa & 0xFFFC0000) != 0xFFF80000) {
			weird = 1;
		}
		processor_number = hpa_to_processor_number (cet_ptr->cet_hpa);
		if (processor_number < 0) {
			weird = 1;
		}
		if (!weird) {
			fprintf(outf, " %s on Processor %d (0x%08X),  RPB vector = 0x%08X\n",
				type, processor_number, cet_ptr->cet_hpa,
				cet_ptr->cet_savestate);
			fprintf(outf, "   Start timestamp 0x%08X   Event entry timestamp 0x%08X\n",
				cet_ptr->cet_start_itmr, cet_ptr->cet_cur_itmr);
		} else {
			fprintf(outf, " Weird event table entry:  HPA = 0x%08X,  Event = 0x%08X (%s)\n",
				cet_ptr->cet_hpa, cet_ptr->cet_event, type);
			fprintf(outf, "   RPB vector 0x%08X,  timestamp1 = 0x%08X,  timestamp2 = 0x%08X\n",
				cet_ptr->cet_savestate, cet_ptr->cet_start_itmr,
				cet_ptr->cet_cur_itmr);
			fprintf(outf, "   reserve1 0x%08X,  reserve2 = 0x%08X,  reserve3 = 0x%08X\n",
				cet_ptr->cet_reserved1, cet_ptr->cet_reserved2,
				cet_ptr->cet_reserved3);
		}
		cet_ptr++;
	}
	fprintf(outf, "\n");
}

dump_system_rpbs()
{
	extern int rpb_list [];
	int rpbindex;

	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*  System RPB Structures:                           *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	fprintf(outf, "\n");
	for (rpbindex = 0; rpbindex < MAX_RPBS; rpbindex++) {
		if (rpb_list [rpbindex] == 0)
			break;
		getrpb (rpb_list [rpbindex]);
		dumprpb ();
	}
}

int Last_rpb_sr4 = 0;
int Last_rpb_sr5 = 0;
int Last_rpb_sr6 = 0;
int Last_rpb_sr7 = 0;


/* Dump the contents of a rpb */
dumprpb()
{
	register int i;
	int j;
	int diff;
	struct rpb *rp;
	int type;
	int processor_number;
	unsigned int time;
	unsigned int sps;
	unsigned int sp;
	unsigned int pcs;
	unsigned int pc;
	char *type_str;
	char *conv_string;
	char *time_since_boot();

	rp = (struct rpb *) rpbbuf;
	type = rp->rp_crash_type;
	switch (type) {
		case CT_PANIC:
			type_str = "Panic";
			break;
		case CT_HPMC:
			type_str = "HPMC";
			break;
		case CT_TOC:
			type_str = "TOC";
			break;
		default:
			type_str = "Strange Crash ";
			break;
	}
	processor_number = hpa_to_processor_number (rp->rp_proc_hpa);
	fprintf(outf, "%s registers from processor %d (HPA 0x%08X):\n",
		type_str, processor_number, rp->rp_proc_hpa);
	time = rp->rp_time;
	if (time > 0x386db400) {
		/* Happy New Century (+/- a year)! */
		conv_string = "on %a %h %d 20%y, at %T %z";
	} else {
		conv_string = "on %a %h %d 19%y, at %T %z";
	}
	fprintf(outf, "  This %s occurred %s.\n",
		type_str, nl_cxtime (&time, conv_string), rp->rp_time2);
	fprintf(outf, "  The microsecond count was 0x%08X and interval timer was 0x%08X.\n",
		rp->rp_time2, rp->rp_itimer);
	fprintf(outf, "  This crash occured %s after boot.\n",
		time_since_boot(rp->rp_time));
	fprintf(outf, "  At the time, noproc was 0x%08X (there was %s active process).\n",
		rp->rp_noproc, rp->rp_noproc ? "no" : "an");
	fprintf(outf, "  The global PSW word was 0x%08X.\n", rp->rp_global_psw);
	if (type == CT_HPMC || type == CT_TOC) {
		fprintf(outf, "  The PDC PIM return value was %d (%s).\n",
			rp->rp_pim_return,
			rp->rp_pim_return == 0 ? "successful" : "unsuccessful");
	}


	for (i = 0; i < RPBSIZE_STANDARD; i++) {
		if ((i % 3) == 0)
			fprintf(outf, "\n");
		fprintf(outf, " %s =0x%08x  ", rpbdescriptor[i], rpbbuf[i]);
	}

	if (type == CT_HPMC) {
		fprintf(outf, "\n\n");
		if (rpbbuf[76] != 0)
			fprintf(outf, "HPMC occured !!!");
		fprintf(outf, " Check type word :");
		if (rpbbuf[76] & CTW_CACHE)
			fprintf(outf, "Cache System Check ");
		if (rpbbuf[76] & CTW_TLB)
			fprintf(outf, "TLB Check ");
		if (rpbbuf[76] & CTW_BUS)
			fprintf(outf, "Bus Transaction Check ");
		if (rpbbuf[76] & CTW_ASSIST)
			fprintf(outf, "Assist Check ");
		if (rpbbuf[76] & CTW_PROCESS)
			fprintf(outf, "Processor internal Check ");
		fprintf(outf, "\n");

		fprintf(outf, " CPU state word  :");
		if (rpbbuf[77] & CPU_IQV)
			fprintf(outf, "IIAqueue valid, ");
		if (rpbbuf[77] & CPU_IQF)
			fprintf(outf, "IIAqueue fault, ");
		if (rpbbuf[77] & CPU_IPV)
			fprintf(outf, "IPRs valid, ");
		if (rpbbuf[77] & CPU_GRV)
			fprintf(outf, "Grs valid, ");
		if (rpbbuf[77] & CPU_CRV)
			fprintf(outf, "CRs valid, ");
		if (rpbbuf[77] & CPU_SRV)
			fprintf(outf, "SRs valid, ");
		if (rpbbuf[77] & CPU_POK)
			fprintf(outf, "Past OK, ");
		if (rpbbuf[77] & CPU_ERC)
			fprintf(outf, "Error cleared ");
		fprintf(outf, "\n");


		fprintf(outf, " Cache check word: ");
		if (rpbbuf[79] & CACHE_ICHECK)
			fprintf(outf, "I-cache check, ");
		if (rpbbuf[79] & CACHE_DCHECK)
			fprintf(outf, "D-cache check, ");
		if (rpbbuf[79] & CACHE_TAGCHECK)
			fprintf(outf, "tag field check ");
		if (rpbbuf[79] & CACHE_DATACHECK)
			fprintf(outf, "data field check ");
		fprintf(outf, "\n");

		fprintf(outf, " TLB check word  : ");
		if (rpbbuf[80] & TLB_ICHECK)
			fprintf(outf, "I-TLB check ");
		if (rpbbuf[80] & TLB_DCHECK)
			fprintf(outf, "D-TLB check ");
		fprintf(outf, "\n");

		fprintf(outf, " Bus check word  : ");
		if (rpbbuf[81] & BUS_ADDRESS_ERR)
			fprintf(outf, "Address error,  ");
		if (rpbbuf[81] & BUS_DATA_SLAVE_ERR)
			fprintf(outf, "Data slave error, ");
		if (rpbbuf[81] & BUS_DATA_PARITY_ERR)
			fprintf(outf, "Data parity error, ");
		if (rpbbuf[81] & BUS_DATA_PROTOCOL_ERR)
			fprintf(outf, "Data protocol error, ");
		if (rpbbuf[81] & BUS_READ_TRANSACTION)
			fprintf(outf, "Read transaction, ");
		if (rpbbuf[81] & BUS_WRITE_TRANSACTION)
			fprintf(outf, "Write transaction, ");
		if (rpbbuf[81] & BUS_MEMORY_SPC_TRANS)
			fprintf(outf, "Memory space transaction, ");
		if (rpbbuf[81] & BUS_IO_SPC_TRANS)
			fprintf(outf, "I/O space transaction, ");
		if (rpbbuf[81] & BUS_PROCESSOR_MASTER)
			fprintf(outf, "Processor master, ");
		if (rpbbuf[81] & BUS_PROCESSOR_SLAVE)
			fprintf(outf, "Processor slave, ");
		fprintf(outf, "\n");

		fprintf(outf, " Assist chk word : ");
		if (rpbbuf[82] & ASSIST_COPROCESSOR_CHECK)
			fprintf(outf, "assist coprocessor check, ");
		if (rpbbuf[82] & ASSIST_SFU_CHECK)
			fprintf(outf, "SFU check, ");
		if (rpbbuf[82] & ASSIST_ID_VALID)
			fprintf(outf, "Assist ID valid, ");
	}
	fprintf(outf, "\n");

	Last_rpb_sr4 = rp->rp_sr4;
	Last_rpb_sr5 = rp->rp_sr5;
	Last_rpb_sr6 = rp->rp_sr6;
	Last_rpb_sr7 = rp->rp_sr7;

	sp = rp->rp_sp;
	sps = (((sp & 0xc0000000) >> 30) & 0x3);
	switch (sps) {
		case 0:
			sps = rp->rp_sr4;
			break;

		case 1:
			sps = rp->rp_sr5;
			break;

		case 2:
			sps = rp->rp_sr6;
			break;

		case 3:
			sps = rp->rp_sr7;
			break;
	}

	if (type == CT_PANIC) {
		pcs = 0;	/* Ok, so I'm taking a short-cut. */
		pc = rp->rp_rp;
	} else {
		pcs = rp->rp_pcsq_head;
		pc = rp->rp_pcoq_head;
	}

	new_stktrc (pcs, pc, sps, sp);

	fprintf(outf, "\n Kernel Stack trace:\n");
/* ?????
	if (!Uflg) {
 */
		fprintf(outf, "         ");
		/* This is the more compact form */
		for (i = 2, j= 0; i <= trc[1]+1; i++) {
			if (j++ == 3){
				j = 1;
				fprintf(outf, "\n         ");
			}

			/* look up closet symbol */
			diff = findsym(trc[i]);
			if (diff == 0xffffffff)
				fprintf(outf, "    0x%04x", trc[i]);
			else

				fprintf(outf, "    %s+0x%04x", cursym, diff);
		}
/* ????
	}
 */

	fprintf(outf, "\n\n");
}

new_stktrc(pcs, pc, sps, sp)
	int pcs;
	int pc;
	int sps;
	int sp;
{
	new_stktrc2(pc, sp, 0);
}


#ifdef __hp9000s800
dumphpmc()
{
	register int i;

	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*  HPMC Registers                                   *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	/* See if we had an HPMC, if not then don't dump registers */
	if (rpbbuf[RPBSIZE_STANDARD] == 0) {
		fprintf(outf, "\nNo HPMC occured !!\n\n");
		return;
	}

	for (i = 2; i < RPBSIZE_ALL; i++) {
		if (((i - 2)  % 3) == 0)
			fprintf(outf, "\n");
		fprintf(outf, " %s =0x%08x  ", rpbdescriptor[i], rpbbuf[i]);
	}

	fprintf(outf, "\n\n");
	if (rpbbuf[76] != 0)
		fprintf(outf, "HPMC occured !!!");
	fprintf(outf, " Check type word :");
	if (rpbbuf[76] & CTW_CACHE)
		fprintf(outf, "Cache System Check ");
	if (rpbbuf[76] & CTW_TLB)
		fprintf(outf, "TLB Check ");
	if (rpbbuf[76] & CTW_BUS)
		fprintf(outf, "Bus Transaction Check ");
	if (rpbbuf[76] & CTW_ASSIST)
		fprintf(outf, "Assist Check ");
	if (rpbbuf[76] & CTW_PROCESS)
		fprintf(outf, "Processor internal Check ");
	fprintf(outf, "\n");

	fprintf(outf, " CPU state word  :");
	if (rpbbuf[77] & CPU_IQV)
		fprintf(outf, "IIAqueue valid, ");
	if (rpbbuf[77] & CPU_IQF)
		fprintf(outf, "IIAqueue fault, ");
	if (rpbbuf[77] & CPU_IPV)
		fprintf(outf, "IPRs valid, ");
	if (rpbbuf[77] & CPU_GRV)
		fprintf(outf, "Grs valid, ");
	if (rpbbuf[77] & CPU_CRV)
		fprintf(outf, "CRs valid, ");
	if (rpbbuf[77] & CPU_SRV)
		fprintf(outf, "SRs valid, ");
	if (rpbbuf[77] & CPU_POK)
		fprintf(outf, "Past OK, ");
	if (rpbbuf[77] & CPU_ERC)
		fprintf(outf, "Error cleared ");
	fprintf(outf, "\n");

	fprintf(outf, " Cache check word: ");
	if (rpbbuf[79] & CACHE_ICHECK)
		fprintf(outf, "I-cache check, ");
	if (rpbbuf[79] & CACHE_DCHECK)
		fprintf(outf, "D-cache check, ");
	if (rpbbuf[79] & CACHE_TAGCHECK)
		fprintf(outf, "tag field check ");
	if (rpbbuf[79] & CACHE_DATACHECK)
		fprintf(outf, "data field check ");
	fprintf(outf, "\n");

	fprintf(outf, " TLB check word  : ");
	if (rpbbuf[80] & TLB_ICHECK)
		fprintf(outf, "I-TLB check ");
	if (rpbbuf[80] & TLB_DCHECK)
		fprintf(outf, "D-TLB check ");
	fprintf(outf, "\n");

	fprintf(outf, " Bus check word  : ");
	if (rpbbuf[81] & BUS_ADDRESS_ERR)
		fprintf(outf, "Address error,  ");
	if (rpbbuf[81] & BUS_DATA_SLAVE_ERR)
		fprintf(outf, "Data slave error, ");
	if (rpbbuf[81] & BUS_DATA_PARITY_ERR)
		fprintf(outf, "Data parity error, ");
	if (rpbbuf[81] & BUS_DATA_PROTOCOL_ERR)
		fprintf(outf, "Data protocol error, ");
	if (rpbbuf[81] & BUS_READ_TRANSACTION)
		fprintf(outf, "Read transaction, ");
	if (rpbbuf[81] & BUS_WRITE_TRANSACTION)
		fprintf(outf, "Write transaction, ");
	if (rpbbuf[81] & BUS_MEMORY_SPC_TRANS)
		fprintf(outf, "Memory space transaction, ");
	if (rpbbuf[81] & BUS_IO_SPC_TRANS)
		fprintf(outf, "I/O space transaction, ");
	if (rpbbuf[81] & BUS_PROCESSOR_MASTER)
		fprintf(outf, "Processor master, ");
	if (rpbbuf[81] & BUS_PROCESSOR_SLAVE)
		fprintf(outf, "Processor slave, ");
	fprintf(outf, "\n");

	fprintf(outf, " Assist chk word : ");
	if (rpbbuf[82] & ASSIST_COPROCESSOR_CHECK)
		fprintf(outf, "assist coprocessor check, ");
	if (rpbbuf[82] & ASSIST_SFU_CHECK)
		fprintf(outf, "SFU check, ");
	if (rpbbuf[82] & ASSIST_ID_VALID)
		fprintf(outf, "Assist ID valid, ");
	fprintf(outf, "\n");

	fprintf(outf, "\n");


}
#endif /* __hp9000s800 */

#ifdef MP

dumpmp()
{
	int i;

	dumpmplocks();

	for (i = 0; i < MAX_PROCS; i++) {
		if (mp[i].prochpa != 0) {
			dumpmpiva((mpiva + i), (mp[i].iva - sizeof(struct mp_iva)));
		}
	}

	for (i = 0; i < MAX_PROCS; i++) {
		if (mp[i].prochpa != 0) {
			dumpmpinfo((mp + i), mp, vmp);
		}
	}
}


dumpmpinfo(mp, lmp, vmp)
	register struct mpinfo *mp, *lmp, *vmp;
{
	fprintf(outf, " Mpinfo struct address 0x%08x   mpinfo[%d]\n",
		 vmp + (mp - lmp), (mp-lmp));
	fprintf(outf, "   prochpa  0x%08x\n", mp->prochpa);
	fprintf(outf, "   cpustate 0x%08x  blktime 0x%08x  iva    0x%08x\n",
		mp->cpustate, mp->blktime, mp->iva);
	fprintf(outf, "\n");
}



dumpmpiva(mpiva, vmpiva)
	register struct mp_iva *mpiva, *vmpiva;
{
	fprintf(outf, "\n Mp iva struct address 0x%08x  \n", vmpiva);
	fprintf(outf, "   crashstk  0x%08x  tmpsave 0x%08x  procindex 0x%08x\n",
		mpiva->crashstack, mpiva-> tmpsavestate, mpiva->procindex);
	fprintf(outf, "   curpri  0x%08x  noproc 0x%08x\n",
		mpiva->iva_curpri, mpiva->iva_noproc);
	fprintf(outf, "   timeinval 0x%08x  glbpsw  0x%08x  topics    0x%08x\n",
		mpiva->timeinval, mpiva->globalpsw, mpiva->topics);
	fprintf(outf, "   ispare    0x%08x  rpb     0x%08x  mpinfo    0x%08x\n",
		mpiva->ispare, mpiva->rpb_offset, mpiva->mpinfo);
	fprintf(outf, "   ibase     0x%08x  procp   0x%08x  ics       0x%08x\n",
		mpiva->ibase, mpiva->procp, mpiva->ics);
	fprintf(outf, "\n");
}


dumpmplocks()
{
#ifdef notdef
	fprintf(outf, "\n   Multiprocessor structures\n");
	fprintf(outf, "   spl_word 0x%04x  spl_lock 0x%04x\n",
		spl_word, spl_lock);
	fprintf(outf, "\n");
#endif
}

#endif /* MP */

#else /* not __hp9000s800 */

dump_crash_processor_table()
{
}

hpa_to_mpproc_index(hpa)
	unsigned int hpa;
{
	return (0);
}

hpa_to_processor_number(hpa)
	unsigned int hpa;
{
}

dump_system_rpbs()
{
}

int Last_rpb_sr4 = 0;
int Last_rpb_sr5 = 0;
int Last_rpb_sr6 = 0;
int Last_rpb_sr7 = 0;

/* Dump the contents of a rpb */
dumprpb()
{
}

new_stktrc (pcs, pc, sps, sp)
	int pcs;
	int pc;
	int sps;
	int sp;
{
}

#ifdef MP

dumpmp()
{
}

dumpmpinfo(mp, lmp, vmp)
register struct mpinfo *mp, *lmp, *vmp;
{
}

dumpmpiva(mpiva, vmpiva)
register struct mp_iva *mpiva, *vmpiva;
{
}

dumpmplocks()
{
}

#endif /* MP */

#endif /* __hp9000s800 */

#ifndef NETWORK
/* catch networking undefines if NETWORK off */
struct mbuf	*mbuf_memory;
pty_display()	{}
netopt()	{}
net_display()	{}
vmtod()		{}
nethelp()	{}
listdisplay()	{}
mux_display()	{}
#endif

#ifdef	MP
#ifdef	SEMAPHORE_DEBUG
#include <sys/sem_utl.h>

dumpsemaphores()
{
	lock_t *lockp, tlock;
	sema_t *semp, tsem;
	int diff, i;
	struct mp_sem_log *semaphore_log, *slp;

	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*   MP Spinlocks                                    *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	lockp = (lock_t *) get(nl[X_GLOBAL_LOCK_LIST].n_value);
	while (lockp != NULL) {
		getchunk(KERNELSPACE, (int)lockp, (char *)&tlock,
			(sizeof (lock_t)), "dumpsema: locks");
		dump_lock(&tlock);
		lockp = tlock.sl_g_link;
	}
	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*   MP Semaphores                                   *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");

	semp = (sema_t *) get(nl[X_GLOBAL_SEMA_LIST].n_value);
	while (semp != NULL) {
		diff = findsym((int)semp);
		if (diff == 0)
			fprintf(outf, "Semaphore %s:\n", cursym);
		else if (diff < 0xffff)
			fprintf(outf, "Semaphore at %s+0x%04x:\n",
				cursym, diff);
		else
			fprintf(outf, "Semaphore at 0x%08x:\n", semp);
		getchunk(KERNELSPACE, (int)semp, (char *)&tsem,
			sizeof (sema_t), "dumpsema: semas");
		dump_sema(&tsem);
		semp = tsem.sa_link;
		fprintf(outf, "\n");
	}

	if (!lflg)
		return 0;

	fprintf(outf, "*****************************************************\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*   MP Semaphore Log                                *\n");
	fprintf(outf, "*                                                   *\n");
	fprintf(outf, "*****************************************************\n");


	semaphore_log = (struct mp_sem_log *)
		calloc (LOG_ENTRIES, (sizeof (struct mp_sem_log)));
	getchunk(KERNELSPACE, get(nl[X_SEMAPHORE_LOG].n_value),
		(char *)semaphore_log, (LOG_ENTRIES*(sizeof (struct mp_sem_log))),
		 "dumpsema: sema log");
	for (i=0; i<LOG_ENTRIES; i++) {
		slp = semaphore_log+i;
		fprintf(outf, "semaphore_log[%d]: caller_address = 0x%08x\n",
			i, slp->caller_address);
		fprintf(outf, "caller_pid = %08d last_pid = %08d\n",
			slp->caller_pid, slp->last_pid);
		fprintf(outf, "sema_function = %08d sema_value = %08d semaphore = 0x%08x\n",
			slp->sema_function, slp->sema_value, slp->semaphore);
		fprintf(outf, "max_waiters = %08d timestamp = 0x%08x\n",
			slp->max_waiters, slp->timestamp);
	}
	free ((char *) semaphore_log);
}

dump_sema(semp)
	sema_t *semp;
{
	unsigned int diff;

	fprintf(outf, "\n");
	fprintf(outf, " lock = 0x%08x	count = 0x%08x\n",
		semp->sa_lock, semp->sa_count);
	fprintf(outf, " last_pid = %d max_waiters = %d p_count = %d\n",
		semp->sa_last_pid, semp->sa_max_waiters, semp->sa_p_count);
	fprintf(outf, " failed_cp_count = %d sleepy_p_count = %d order = %d\n",
		semp->sa_failed_cp_count, semp->sa_sleepy_p_count, semp->sa_order);
	fprintf(outf, " contention = %d deadlock_safe %d\n",
		semp->sa_contention, semp->sa_deadlock_safe);
	fprintf(outf, " owner = 0x%08x priority = %d next sema = 0x%08x\n",
		semp->sa_owner, semp->sa_priority, semp->sa_next);
	diff = findsym(semp->sa_pcaller);
	if (diff == 0xffffffff)
	      fprintf(outf, " sa_pcaller = 0x%08x\n");
	else
	      fprintf(outf, " sa_pcaller = %s+0x%x\n", cursym, diff);
	diff = findsym(semp->sa_vcaller);
	if (diff == 0xffffffff)
	      fprintf(outf, " sa_vcaller = 0x%08x\n");
	else
	      fprintf(outf, " sa_vcaller = %s+0x%x\n", cursym, diff);
#ifdef MP_LIFTTW
	fprintf(outf, " s_ks_unprotect = 0x%08x s_ks_reacquire = 0x%08x\n",
			semp->s_ks_unprotect, semp->s_ks_reacquire);
#endif

	fprintf(outf, " prev = 0x%08x link = 0x%08x wait list = 0x%08x\n",
			semp->sa_prev, semp->sa_link, semp->sa_wait_list);
}


dump_lock(lp)
	lock_t *lp;
{
	char name[64];
	unsigned int diff;
	int boring=0;
	int bytes_on_page = (NBPG-((int)lp->sl_name) & (NBPG-1));


	/*	Don't try to get 64 bytes if it would cross a page
	**	boundary. I'm sure there's a better way.
	*/
	getchunk(KERNELSPACE, (int)lp->sl_name, name,
		((bytes_on_page < 64) ? bytes_on_page : 64), "dump_lock");
	name[63] = '\0';	/* just in case */
	if (strcmp(name, "bh_sema_t spinlock") == 0)
		boring = 1;
	if (strcmp(name, "sync_t spinlock") == 0)
		boring = 1;
	if (strcmp(name, "p_lock_spare") == 0)
		boring = 1;
	if (strcmp(name, "sema_t spinlock") == 0)
		boring = 1;
	if (boring) return;
	fprintf(outf, "\n");
	fprintf(outf, "Lock Name: %s\n", name);
	fprintf(outf, " count: 0x%08x\n", lp->sl_count);
	fprintf(outf, " owner: %d\n", lp->sl_owner);
#if defined(MP) || defined(SPINLOCK_DEBUG)
	fprintf(outf, " order: %d\n", lp->sl_order);
#endif
#ifdef  SPINLOCK_DEBUG
	fprintf(outf, " g_link: 0x%08x\n", lp->sl_g_link);
	fprintf(outf, " link: 0x%08x\n", lp->sl_link);
	diff = findsym(lp->sl_lock_caller);
	if (diff == 0xffffffff)
	      fprintf(outf, " lock_caller = 0x%08x\n");
	else
	      fprintf(outf, " lock_caller = %s+0x%x\n", cursym, diff);
	diff = findsym(lp->sl_unlock_caller);
	if (diff == 0xffffffff)
	      fprintf(outf, " unlock_caller = 0x%08x\n");
	else
	      fprintf(outf, " unlock_caller = %s+0x%x\n", cursym, diff);
#endif  /* SPINLOCK_DEBUG */


}
#else 	/* ! SEMAPHORE_DEBUG */
dumpsemaphores()
{
}
#endif	/* ! SEMAPHORE_DEBUG */

#else	/* ! MP */
dumpsemaphores()
{
}
#endif	/* MP */

#if defined(__hp9000s800) && defined(_WSIO)
/*
 * BEGIN_DESC
 *
 * is_snake()
 *
 * Input Parameters:
 *	None
 *
 * Output Parameters:
 *	None
 *
 * Return Value:
 *	1 (TRUE)  if cpu type is a snake
 *	0 (FALSE) if cpu type is not a snake
 *	0 (FALSE) if some error was encountered
 *
 * Globals Referenced:
 *	None
 *
 * Description:
 *	This routine is used to determine if we are dealing with
 *	a snake kernel.  This is a temporary solution: it looks
 *	at utsname now, but eventually it will look at cputype.
 *	Currently, cputype is not set up for snakes.
 *
 *	Probably want to add the cpu type symbol to an_nldefs.h
 *	and an_nlist.h when a more permanent solution is written.
 *	This way the cpu type symbol can be found when the rest
 *	of the symbols are scanned.  Currently, the symbol is
 *	looked up each time the routine is invoked.
 *
 * Algorithm:
 *	1. Lookup address of cpu type symbol in the symbol table.
 *	2. Allocate a local buffer to read in cpu type structure.
 *	3. Seek into the core file to the cpu type symbol.
 *	4. Read cpu type structure into the local buffer.
 *	5. See if it's a snake.
 *	6. Free local buffer.
 *	7. Return 1 if snake, else return 0.
 *
 * In/Out conditions:
 *	None
 *
 * END_DESC
 */

#include <h/utsname.h>			/* for utsname structure def */

#define	LONGREAD(fd, buf, count) \
	(longread((int)(fd), (char *)(buf), (unsigned int)(count)) \
		!= (count))

int
is_snake()
{
	unsigned int offset;		/* location of cpu type symbol */
	struct utsname my_utsname;	/* local buffer */

	/* find location of cpu type symbol */
	offset = lookup("utsname");
	if (!offset) {
		fprintf(outf, "is_snake: could not find \"utsname\"\n");
		return 0;
	}

	/* seek and read cpu type information */
	longlseek(fcore, (long)clear(offset), 0);
	if (LONGREAD(fcore, &my_utsname, sizeof my_utsname)) {
		perror("is_snake: utsname longread");
		if (!tolerate_error) {
			exit(1);
		}
		return 0;
	}

	/*
	 * extract cpu type & see if it's a snake
	 */
	return strncmp(my_utsname.machine, "9000/7", 6) ? 0 : 1;
}
#endif	/* defined(__hp9000s800) && defined(_WSIO) */
