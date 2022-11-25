/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/pstat.c,v $
 * $Revision: 1.13.83.7 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 13:33:54 $
 */


/*
 * Pstat is a system call which allows one process to obtain information
 * about other processes, and about system operation in general.  It is
 * designed to support the operation of programs such as ps, top, uptime,
 * monitor, and so on, but it can also be used by multiple-process
 * programs to determine the state of the other processes.
 */


#include "../h/types.h"
#include "../h/systm.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/dk.h"
#include "../h/proc.h"
#include "../h/param.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/pstat.h"
#include "../h/vmmeter.h"
#include "../h/vmmac.h"
#include "../h/vnode.h"		/* For inode.h */
#include "../ufs/inode.h"	/* For IFBLK, IFCHR */
#include "../h/vmsystm.h"	/* For forkstat */
#include "../h/conf.h"          /* For swdev_t */
#include "../h/swap.h"          /* For fswdev_t */
#ifdef __hp9000s800
#include "../h/fss.h"		/* for FSS group ID */
#endif
#include "../h/spinlock.h"	/* for sched_lock */
#include "../h/debug.h"		/* for vasunlock */

#ifndef lint
static char rcsid[] = "$Header: pstat.c,v 1.13.83.7 93/12/09 13:33:54 marshall Exp $";
#endif /* lint */

char *pst_cmds = 0;	/* Command line cache */
char *pst_ucomms = 0;	/* u_comm cache */

extern caddr_t kvalloc();

/*
 * Copy one proc's command line as another's.  Used to keep it up
 *  to date when fork()s happen.
 */
pstat_fork(old, new)
    struct proc *old, *new;
{
    int oldidx = old-proc, newidx = new-proc;

    bcopy(pst_cmds + (oldidx * PST_CLEN), 
	pst_cmds + (newidx * PST_CLEN), PST_CLEN);
    bcopy(pst_ucomms + (oldidx * PST_UCOMMLEN), 
	pst_ucomms + (newidx * PST_UCOMMLEN), PST_UCOMMLEN);
}

/*
 * Take a command line in compressed format from getxfile(), store
 *  it as a blank-seperated line.
 *
 * On first invocation, allocate our data for the command cache.
 */
void
pstat_cmd(p, cmd, narg, ucomm)
    struct proc *p;		/* Proc getting the command line */
    register char *cmd;		/* Command line */
    int narg;			/* # args on command line */
    register char *ucomm;	/* new u_comm */
{
    register char *cp;
    int idx;
    extern int nproc;
    register int x = 0;
    register char c;

    VASSERT(p == u.u_procp);	/* so we know who the u_comm is for */
    VASSERT(cmd != NULL);
    VASSERT(ucomm != NULL);
    VASSERT(ucomm[0] != '\0');

    /* On first use get the data */
    if (!pst_cmds) {
	pst_cmds = kvalloc((int)btorp(PST_CLEN*nproc));
	bzero(pst_cmds, PST_CLEN*nproc);

    }
    if (!pst_ucomms) {
	pst_ucomms = kvalloc((int)btorp(PST_UCOMMLEN*nproc));
	bzero(pst_ucomms, PST_UCOMMLEN*nproc);
    }

    /* Point to the entry */
    idx = p-proc;
    cp = pst_cmds + PST_CLEN*idx;

    /* Copy it in */
    while ((narg-- > 0) && (x < PST_CLEN-4)) {
	/* Make them blank-seperated */
	if (x) {
	    *cp++ = ' ';
	    ++x;
	}
	/* Copy in argument */
	while (c = *cmd++) {
	    if (x < PST_CLEN-4) {
		*cp++ = c;
		++x;
	    }
	}
    }

    /* Zero out trailing data */
    bzero(cp, PST_CLEN-x);

    /* now do the u_comm. */
    {
	int len = MIN(strlen(ucomm) + 1, PST_UCOMMLEN-1);
	bcopy(ucomm, u.u_comm, len);
	cp = pst_ucomms + PST_UCOMMLEN * idx;
	bcopy(ucomm, cp, len);
	bzero(cp + len, PST_UCOMMLEN - len);
    }
}

/*
 * Translate proc information into pstat format
 */

pstat_fillin(p, ps)
	register struct proc *p;
	register struct pst_status *ps;
{
	int idx;
	int flg, flg2;
#ifdef __hp9000s800
	dev_t   tty_dev;
#endif /* hp9000s800 */
	
	idx = ps->pst_idx = p-proc;
	ps->pst_uid = p->p_uid;
	ps->pst_suid = p->p_suid;
	ps->pst_pid = p->p_pid;
	ps->pst_ppid = p->p_ppid;

	/*
	 * used to return in K.  Now give as pages.  Callers can use the 
	 * page_size field from the static struct.
	 */
	
	(void)pregusage(p->p_vas, &(ps->pst_dsize), &(ps->pst_tsize),
		  &(ps->pst_ssize));

	if ((!p->p_vas) || (!cvaslock(p->p_vas))) {
		ps->pst_rssize = 0;
	} else {
		ps->pst_rssize = p->p_vas->va_prss;
		vasunlock(p->p_vas);
	}
	
	ps->pst_nice = p->p_nice;

	/*
	 * Fill in the controlling tty's device.
	 * Note that p_ttyd is valid only if p_ttyp is non-NULL
	 */
	if (p->p_ttyp != NULL) {
#if defined(__hp9000s800) && !defined(_WSIO)
		tty_dev = p->p_ttyd;
		if (map_mi_to_lu(&tty_dev, IFCHR) == 0) {
			ps->pst_major = major(tty_dev);
			ps->pst_minor = minor(tty_dev);
		} else {
			ps->pst_major = -1;
			ps->pst_minor = -1;
		}
#endif /* hp9000s800 and not _WSIO */
#if defined(__hp9000s300) || defined(_WSIO)
		ps->pst_major = major(p->p_ttyd);
		ps->pst_minor = minor(p->p_ttyd);
#endif /* hp9000s300 or _WSIO */
	} else {
		ps->pst_major = -1;
		ps->pst_minor = -1;
	}

	ps->pst_pgrp = p->p_pgrp;
	ps->pst_pri = p->p_pri;
	
	/* do UAREA? vas more useful, more obscure */
	ps->pst_addr = (long)p->p_vas;
	
	ps->pst_cpu = p->p_cpu;
#ifdef __hp9000s800
	ps->pst_utime = p->p_utime.tv_sec;
	ps->pst_stime = p->p_stime.tv_sec;
#endif
#ifdef __hp9000s300
	ps->pst_utime = (p->p_utime.tv_sec)/hz;
	ps->pst_stime = (p->p_stime.tv_sec)/hz;
#endif
	ps->pst_start = p->p_start; 
#ifdef MP
	ps->pst_procnum = p->p_procnum;
#else  !MP
	ps->pst_procnum = 0;
#endif MP
	ps->pst_wchan = 0;
	switch (p->p_stat) {
	      case SSLEEP:
		ps->pst_stat = PS_SLEEP;
		ps->pst_wchan = (long)p->p_wchan;
		break;
	      case SRUN:
		ps->pst_stat = PS_RUN;
		break;
	      case SZOMB:
		ps->pst_stat = PS_ZOMBIE;
		break;
	      case SSTOP:
		ps->pst_stat = PS_STOP;
		break;
	      case SIDL:
		ps->pst_stat = PS_IDLE;
		break;
	      default:
		ps->pst_stat = PS_OTHER;
	}
	flg = p->p_flag;
	flg2 = 0;
	if (flg & SLOAD) {
		flg2 |= PS_INCORE;
	}
	if (flg & SSYS) {
		flg2 |= PS_SYS;
	}
	if (flg & SLOCK) {
		flg2 |= PS_LOCKED;
	}
	if (flg & STRC) {
		flg2 |= PS_TRACE;
	}
	if (flg & SWTED) {
		flg2 |= PS_TRACE2;
	}
	ps->pst_flag = flg2;
	
	/* Get cached command line */
	VASSERT(strlen(pst_cmds + idx*PST_CLEN) < PST_CLEN);
	strcpy(ps->pst_cmd, pst_cmds + idx*PST_CLEN);

	/* Get cached u_comm */
	strcpy(ps->pst_ucomm, pst_ucomms + idx*PST_UCOMMLEN);
	
	ps->pst_time = p->p_time;
	ps->pst_cpticks = p->p_cpticks;
	ps->pst_cptickstotal = p->p_cptickstotal;
#ifdef __hp9000s800
	ps->pst_fss = p->p_fss->fs_id;
#endif
#ifdef __hp9000s300
	ps->pst_fss = 0;			/* not on 300s (yet(?)) */
#endif
	ps->pst_pctcpu = p->p_pctcpu;	/* a float. ugh. */

	{
		/*
		 * The following fields exist in pstat.h for the
		 * Series 300, 700, but are not (yet?) implemented on these
		 * platforms.  They are explictly cleared here, rather than
		 * return uninitialized data to the user.
		 */
		ps->pst_shmsize		= 0;
		ps->pst_mmsize		= 0;
		ps->pst_usize		= 0;
		ps->pst_iosize		= 0;
		ps->pst_vtsize		= 0;
		ps->pst_vdsize		= 0;
		ps->pst_vssize		= 0;
		ps->pst_vshmsize	= 0;
		ps->pst_vmmsize		= 0;
		ps->pst_vusize		= 0;
		ps->pst_viosize		= 0;
		ps->pst_minorfaults	= 0;
		ps->pst_majorfaults	= 0;
		ps->pst_nswap		= 0;
		ps->pst_nsignals	= 0;
		ps->pst_msgrcv		= 0;
		ps->pst_msgsnd		= 0;
		ps->pst_maxrss		= 0;
	}

}

/* pstat() system call */
pstat()
{
	struct a {
		int function;
		caddr_t e;
		int size;
		int entries;
		int offset;
	} *uap = (struct a *)u.u_ap;
	
	register int fc = uap->function;
	register int size = uap->size;
	
	/* select which function */
	switch (fc) {
	      case PSTAT_PROC:
		pstat_proc((struct pst_status *)uap->e, size, uap->entries,
			   uap->offset);
		break;
	      case PSTAT_STATIC:
		pstat_static((struct pst_static *)uap->e, size);
		break;
	      case PSTAT_DYNAMIC:
		pstat_dynamic((struct pst_dynamic *)uap->e, size);
		break;
	      case PSTAT_SETCMD:
		pstat_setcmd((char **)uap->e);
		break;
	      case PSTAT_VMINFO:
		pstat_vminfo((struct pst_vminfo *)uap->e, size);
		break;
	      case PSTAT_DISKINFO:
		pstat_diskinfo((struct pst_diskinfo *)uap->e, size, uap->entries,
			       uap->offset);
		break;
	      case PSTAT_SWAPINFO:
		pstat_swapinfo((struct pst_swapinfo *)uap->e, uap->size,
				uap->entries, uap->offset);
		break;
	      case PSTAT_PROCESSOR:
	      case PSTAT_LVINFO:
	      default:
		u.u_error = EINVAL;
		break;
	}
}

pstat_proc(dest,size,num,offset)
	struct pst_status *dest;
	int size, num, offset;
{
	register struct proc *p, *end = procNPROC;
	struct pst_status pst;
	int count = 0;
	
	if ((size <= 0) || (size > sizeof(struct pst_status))) {
		u.u_error = EINVAL;
		return;
	}
	if (num) {	/* getting a group of num entries starting at offset */
		/* Catch wraparound */
		if ((p = proc+offset) < proc) {
			u.u_error = EINVAL;
			return;
		}
		
		/* Copy out proc info 'til end of proc[] */
		SPINLOCK(sched_lock);
		while (p < end) {
			if (p->p_stat != 0) {
				pstat_fillin(p, &pst);
				SPINUNLOCK(sched_lock);
				if (copyout(&pst, dest, size) < 0) {
					u.u_error = EFAULT;
					SPINLOCK(sched_lock); /* avoid unlocking
						 unowned lock after break */
					break;
				}
				count++;
				dest = (struct pst_status *)( (int)dest + size);
				SPINLOCK(sched_lock);
				if (--num < 1)
					break;
			} 
			p++;
		}
		SPINUNLOCK(sched_lock);
		u.u_r.r_val1 = count;
		
	} else {	/* getting proc entry for pid = offset */
		if ((p = pfind(offset)) != (proc_t *)NULL) {
			/*SPINLOCK(sched_lock) already grabbed by pfind() */
			pstat_fillin(p, &pst);
			SPINUNLOCK(sched_lock);
			if (copyout(&pst, dest, size) < 0) {
				u.u_error = EFAULT;
				return;
			}
			u.u_r.r_val1 = 1;
			return;
		} else {
			u.u_error = ESRCH;
			return;
		}
	}
}


pstat_static(dest,size)
	struct pst_static *dest;
	int size;
{
	struct pst_static pst;
	register struct pst_static *psp = &pst;
	extern struct timeval boottime;
#ifdef __hp9000s800
	extern dev_t cons_mux_dev;
	dev_t	     con_dev;
#endif
#if defined(__hp9000s300) || defined(_WSIO)
	extern dev_t cons_dev;
#endif
	
	if ((size <= 0) || (size > sizeof(struct pst_static))) {
		u.u_error = EINVAL;
		return;
	}
	/* fill in the fields. */
	psp->max_proc = nproc;
#ifdef __hp9000s800
	con_dev = cons_mux_dev;
#ifdef _WSIO
	psp->console_device.psd_major = major(con_dev);
	psp->console_device.psd_minor = minor(con_dev);
#else
	if (map_mi_to_lu(&con_dev, IFCHR) == 0) {
		psp->console_device.psd_major = major(con_dev);
		psp->console_device.psd_minor = minor(con_dev);
	} else {
		psp->console_device.psd_major = -1;
		psp->console_device.psd_minor = -1;
	}
#endif	/* _WSIO */
#endif /* hp9000s800 */
#ifdef __hp9000s300
	psp->console_device.psd_major = major(cons_dev);
	psp->console_device.psd_minor = minor(cons_dev);
#endif /* hp9000s300 */
	psp->boot_time = boottime.tv_sec;
	psp->physical_memory = physmem;
	psp->page_size = NBPG;
	psp->cpu_states = CPUSTATES;
	psp->pst_status_size = sizeof(struct pst_status);
	psp->pst_static_size = sizeof(struct pst_static);
	psp->pst_dynamic_size = sizeof(struct pst_dynamic);
	psp->pst_vminfo_size = sizeof(struct pst_vminfo);
	psp->command_length = PST_CLEN;
	psp->pst_swapinfo_size = sizeof(struct pst_swapinfo);
	{
		/*
		 * The following fields exist in pstat.h for the
		 * Series 300, 700, but are not (yet?) implemented on these
		 * platforms.  They are explictly cleared here, rather than
		 * return uninitialized data to the user.
		 */
		psp->pst_processor_size = 0; /* sizeof(struct pst_processor) */
		psp->pst_lvinfo_size = 0;    /* sizeof(struct pst_lvinfo) */
	}

	if ((copyout(&pst,dest,size) < 0)) {
		u.u_error = EFAULT;
	}
}

pstat_dynamic(dest,size)
	struct pst_dynamic *dest;
	int size;
{
	struct pst_dynamic pst;
	register struct pst_dynamic *psp = &pst;
	register struct vmtotal *vmt = &total;
	int i;
	
	if ((size <= 0) || (size > sizeof(struct pst_dynamic))) {
		u.u_error = EINVAL;
		return;
	}
	/* fill in the fields. */
	
#ifdef MP
	/* put MP variants here. */
#endif /* MP */
	psp->psd_proc_cnt = 1;	/* Need #ifdef MP info for these two */
	psp->psd_max_proc_cnt = 1;
	psp->psd_rq = vmt->t_rq;
	psp->psd_dw = vmt->t_dw;
	psp->psd_pw = vmt->t_pw;
	psp->psd_sl = vmt->t_sl;
	psp->psd_sw = vmt->t_sw;
	psp->psd_vm = vmt->t_vm;
	psp->psd_avm = vmt->t_avm;
	psp->psd_rm = vmt->t_rm;
	psp->psd_arm = vmt->t_arm;
	psp->psd_vmtxt = vmt->t_vmtxt;
	psp->psd_avmtxt = vmt->t_avmtxt;
	psp->psd_rmtxt = vmt->t_rmtxt;
	psp->psd_armtxt = vmt->t_armtxt;
	psp->psd_free = vmt->t_free;
	
	psp->psd_avg_1_min = avenrun[0];
	psp->psd_avg_5_min = avenrun[1];
	psp->psd_avg_15_min = avenrun[2];
	
	for (i = 0; i < PST_MAX_CPUSTATES; i++) {
		psp->psd_cpu_time[i] = ((i < CPUSTATES) ? cp_time[i] : 0);
	}

	{
		/*
		 * The following fields exist in pstat.h for the
		 * Series 300, 700, but are not (yet?) implemented on these
		 * platforms.  They are explictly cleared here, rather than
		 * return uninitialized data to the user.
		 */
		int j, k;
		for (j = 0; j < PST_MAX_PROCS; j++) {
			psp->psd_mp_avg_1_min[j] = 0;
			psp->psd_mp_avg_5_min[j] = 0;
			psp->psd_mp_avg_15_min[j] = 0;
			for (k = 0; k < PST_MAX_CPUSTATES; k++) {
				psp->psd_mp_cpu_time[j][k] = 0;
			}
		}
		psp->psd_openlv		= 0;
		psp->psd_openvg		= 0;
		psp->psd_allocpbuf	= 0;
		psp->psd_usedpbuf	= 0;
		psp->psd_maxpbuf	= 0;
		psp->psd_activeprocs	= 0; 	
		psp->psd_activeinodes	= 0;   
		psp->psd_activefiles	= 0;  
	}
	
	if ((copyout(&pst,dest,size) < 0)) {
		u.u_error = EFAULT;
	}
}
pstat_setcmd(cmdp)
	register char *cmdp;
{
	register int l;
	register char c, *s1;
	char buffer[PST_CLEN];
	s1 = buffer;
	bzero(s1,PST_CLEN);
	
	for (l = 0; l < PST_CLEN; l++) {
		c = fubyte(cmdp++);
		if (c == -1) {
			goto bad;
		}
		*s1++ = c;
		if (c == 0) {
			break;
		}
	}
	if (l != 0) {		/* got something */
		buffer[PST_CLEN-1] ='\0';	/* guarantee truncation */
		bcopy(buffer,pst_cmds + ((u.u_procp - proc)*PST_CLEN), PST_CLEN);
		u.u_r.r_val1 = 0;
		return;
	}
      bad:
	u.u_error = EFAULT;
	return;
}

pstat_vminfo(dest,size)
	struct pst_vminfo *dest;
	int size;
{
	extern int deficit;
#ifdef __hp9000s300
	extern int tk_nin, tk_nout;	/* externed in dk.h for s800 */
#endif
	struct pst_vminfo pst;
	register struct pst_vminfo *psp = &pst;
	
	if ((size <= 0) || (size > sizeof(struct pst_vminfo))) {
		u.u_error = EINVAL;
		return;
	}
	/* fill in the fields */
	/* from the rate structure */
	psp->psv_rdfree =	rate.v_dfree;
	psp->psv_rintr =	rate.v_intr;
	psp->psv_rpgpgin =	rate.v_pgpgin;	
	psp->psv_rpgpgout =	rate.v_pgpgout;
	psp->psv_rpgrec =	rate.v_pgrec;
	psp->psv_rscan =	rate.v_scan;
	psp->psv_rswtch =	rate.v_swtch;
	psp->psv_rsyscall =	rate.v_syscall;
	psp->psv_rxifrec =	rate.v_xifrec;
	psp->psv_rxsfrec =	rate.v_xsfrec;
#ifdef __hp9000s800
	psp->psv_rpgtlb	=	rate.v_pgtlb;
#endif /* hp9000s800 */
#ifdef __hp9000s300
	psp->psv_rpgtlb = 	0;	/* currently none for S300 */
#endif /* __hp9000s300 */
	
	/* from the cnt structure */
	psp->psv_cfree = 	cnt.v_free;
	
	/* from the sum structure */
	psp->psv_sdfree =	sum.v_dfree;
	psp->psv_sexfod =	sum.v_exfod;
	psp->psv_sfaults =	sum.v_faults;
	psp->psv_sintr =	sum.v_intr;
	psp->psv_sintrans =	sum.v_intrans;
	psp->psv_snexfod =	sum.v_nexfod;
	psp->psv_snzfod =	sum.v_nzfod;
	psp->psv_spgfrec =	sum.v_pgfrec;
	psp->psv_spgin =	sum.v_pgin;
	psp->psv_spgout =	sum.v_pgout;
	psp->psv_spgpgin =	sum.v_pgpgin;
	psp->psv_spgpgout =	sum.v_pgpgout;
	psp->psv_spswpin =	sum.v_pswpin;
	psp->psv_spswpout =	sum.v_pswpout;
	psp->psv_srev =		sum.v_rev;
	psp->psv_sseqfree =	sum.v_seqfree;
	psp->psv_sswpin =	sum.v_swpin;
	psp->psv_sswpout =	sum.v_swpout;
	psp->psv_sswtch =	sum.v_swtch;
	psp->psv_ssyscall =	sum.v_syscall;
	psp->psv_strap =	sum.v_trap;
	psp->psv_sxifrec =	sum.v_xifrec;
	psp->psv_sxsfrec =	sum.v_xsfrec;
	psp->psv_szfod =	sum.v_zfod;
	psp->psv_sscan =	sum.v_scan;
	psp->psv_spgrec =	sum.v_pgrec;
	
	psp->psv_deficit =	deficit;
	psp->psv_tknin =	tk_nin;
	psp->psv_tknout =	tk_nout;
	
	psp->psv_cntfork =      forkstat.cntfork;
	psp->psv_sizfork =      forkstat.sizfork;
	
	{
		/*
		 * The following fields exist in pstat.h for the
		 * Series 300, 700, but are not (yet?) implemented on these
		 * platforms.  They are explictly cleared here, rather than
		 * return uninitialized data to the user.
		 */
		psp->psv_lreads		= 0;
		psp->psv_lwrites	= 0;
		psp->psv_swpocc		= 0;
		psp->psv_swpque		= 0;
	}

	if ((copyout(&pst,dest,size) < 0)) {
		u.u_error = EFAULT;
	}
}

pstat_diskinfo(dest,size,num,offset)
	struct pst_diskinfo *dest;
	int size, num, offset;
{
#if defined(__hp9000s800) && !defined(_WSIO)
	extern int dk_ndrive1;
	extern int dk_ndrive2;
	dev_t tmpdev;	/* for constructing MI-LU devt's */
	int n_no_lu = 0; /* count of non-lu disks found (FL cards..)*/
#endif /* hp9000s800 and !_WSIO */
#if defined(__hp9000s300) || defined(_WSIO)
	int dk_ndrive = DK_NDRIVE;
	extern dev_t rootdev;
#endif /* hp9000s300 or _WSIO */
	struct pst_diskinfo psd;
	register int count = 0;
	register int base = 0;
	register int index;
	if ((size <= 0) || (size > sizeof(struct pst_diskinfo)) ||
	    (offset < 0) || (num <= 0)) {
		u.u_error = EINVAL;
		return;
	}
	
	/*
	 * getting a group of num entries starting at offset
	 * NB that we must go through all of the entries of a 
	 * given driver type moving on to the next type,
	 * and they should be given in order from the
	 * dk_foo*[] array as these correspond to the disk's address
	 * (Internal) users of pstat(pst_diskinfo) may rely on this!
	 */
#if defined(__hp9000s300) || defined(_WSIO)
	while ((index = (count + offset)) < dk_ndrive) {
		psd.psd_idx = offset + count;
		/*
		 * the 300 plays a game where a dev_t of -1 means the rootdev.
		 * since the user can't do this translation, we'll patch it up
		 * for him before returning the field.
		 * The 800 doesn't do this., so we don't neither.
		 */
		if ((psd.psd_dev.psd_major = major(dk_devt[index])) == 0xff) {
			psd.psd_dev.psd_major = major(rootdev);
		}
		if ((psd.psd_dev.psd_minor = minor(dk_devt[index])) == 0xffffff) {
			psd.psd_dev.psd_minor = minor(rootdev);
		}
		psd.psd_dktime = dk_time[index];
		psd.psd_dkseek = dk_seek[index];
		psd.psd_dkxfer = dk_xfer[index];
		psd.psd_dkwds = dk_wds[index];
		psd.psd_dkmspw = dk_mspw[index];
		
		if (copyout(&psd,dest,size) < 0) {
			u.u_error = EFAULT;
			return;
		}
		count++;
		dest = (struct pst_diskinfo *)( (int)dest + size );
		if (--num == 0) {
			u.u_r.r_val1 = count;
			return;
		}
	}
#endif /* hp9000s300 or _WSIO */
#if defined(__hp9000s800) && !defined(_WSIO)
	/*
	 * now do any disc1 entries	 
	 *
	 * we no longer have disc0, so don't try to skip past it
	 */
	/* base += dk_ndrive; */
	for (;(index = (count + offset - base)) < dk_ndrive1; count++) {
		psd.psd_idx = offset + count;
		psd.psd_dev.psd_major = 8; /* disc1 */
		tmpdev = makedev(8, (index << 8));
		if (map_mi_to_lu(&tmpdev, IFBLK) == 0) {
			psd.psd_dev.psd_minor = minor(tmpdev);
			psd.psd_dktime = dk_time1[index];
			psd.psd_dkseek = dk_seek1[index];
			psd.psd_dkxfer = dk_xfer1[index];
			psd.psd_dkwds = dk_wds1[index];
			psd.psd_dkmspw = dk_mspw1[index];
		
			if (copyout(&psd,dest,size) < 0) {
				u.u_error = EFAULT;
				return;
			}
			dest = (struct pst_diskinfo *)( (int)dest + size );
			if (--num == 0) {
				u.u_r.r_val1 = count + 1 - n_no_lu;
				return;
			}
		} else n_no_lu++;
	}
	/* now do any disc2 entries */
	base += dk_ndrive1;
	for (;(index = (count + offset - base)) < dk_ndrive2; count++) {
		psd.psd_idx = offset + count;
		psd.psd_dev.psd_major = 10; /* disc2 */
		tmpdev = makedev(10, (index << 8));
		if (map_mi_to_lu(&tmpdev, IFBLK) == 0) {
			psd.psd_dev.psd_minor = minor(tmpdev);
			psd.psd_dktime = dk_time2[index];
			psd.psd_dkseek = dk_seek2[index];
			psd.psd_dkxfer = dk_xfer2[index];
			psd.psd_dkwds = dk_wds2[index];
			psd.psd_dkmspw = dk_mspw2[index];
			if (copyout(&psd,dest,size) < 0) {
				u.u_error = EFAULT;
				return;
			}
			dest = (struct pst_diskinfo *)( (int)dest + size );
			if (--num == 0) {
				u.u_r.r_val1 = count + 1 - n_no_lu;
				return;
			}
		} else n_no_lu++;
	}
	/* now do any disc3 entries */
	base += dk_ndrive2;
	for (;(index = (count + offset - base)) < dk_ndrive3; count++) {
		psd.psd_idx = offset + count;
		psd.psd_dev.psd_major = 7; /* disc3 */
		tmpdev = makedev(7, (index << 8));
		if (map_mi_to_lu(&tmpdev, IFBLK) == 0) {
			psd.psd_dev.psd_minor = minor(tmpdev);
			psd.psd_dktime = dk_time3[index];
			psd.psd_dkseek = dk_seek3[index];
			psd.psd_dkxfer = dk_xfer3[index];
			psd.psd_dkwds = dk_wds3[index];
			psd.psd_dkmspw = dk_mspw3[index];
			if (copyout(&psd,dest,size) < 0) {
				u.u_error = EFAULT;
				return;
			}
			dest = (struct pst_diskinfo *)( (int)dest + size );
			if (--num == 0) {
				u.u_r.r_val1 = count + 1 - n_no_lu;
				return;
			}
		} else n_no_lu++;
	}
	count -= n_no_lu;
#endif /* hp9000s800 and not _WSIO */
	u.u_r.r_val1 = count;
}


/* BEGIN_IMS pstat_swapinfo *
 ********************************************************************
 ****
 ****                   pstat_swapinfo ()
 ****
 ********************************************************************
 *
 * Input Parameters:    
 *  struct pst_swapinfo *dest;	* ptr to user's buffer(s) to fill   *
 *  int size;				* size of struct, declared by user  *
 *  int num;				* number of instances desired       *
 *  int offset;			* which instance to start with      *
 *
 * Output Parameters:   none
 *
 * Return Value:        (in u.u_r.r_val1)
 *	number of instances copied out to the user's buffer
 *	or -1 on error.
 *
 * Globals Referenced:
 *	u.u_error, u.u_r.r_val1
 *	swdevt, nswapdev
 *	fswdevt, nswapfs
 *
 * Description:
 *	Fills in a local (stack) copy of the pst_swapinfo structure
 *	with per-swap-area data for the specified swap area(s).  Calls
 *	copyout() to place the data in the user's buffer.
 *
 * Algorithm:
 *	Bounds check user's declaration of structure size for sanity.  It
 *		must be:
 *			(1) > 0
 *			(2) <= actual size of the kernel's version
 *	set u.u_error to EINVAL and return if these conditions aren't met.
 *	Set the number of instances reported on to 0.
 *	Loop:
 *		If offset is beyond limits, there are no more entries to
 *			return, so break out of the loop.
 *		Get a local pointer to the current swap-area data area
 *			in either the swdevt[] or fswdevt[] tables.
 *			The two tables are logically placed one after the
 *			other and the user's index, "offset" is used to
 *			select from this "combined table".
 *		Fill in the fields of the local from the swap data area.
 *		Call copyout() to fill in the first size bytes of the user's
 *			copy of the pst_swapinfo structure.
 *		Update the user's destination pointer by size, the actual
 *			number of bytes copied out.
 *		Increment the number of instances reported on.
 *		Decrement the number still to do.
 *		Increment offset.
 *	Until requested number of instances have been reported on.
 *	Set the system call return value to the number of instances reported on
 *
 * Exception Conditions:
 *	If the user's size declaration isn't valid, EINVAL is returned.
 *	If the initial selection, offset, isn't valid, EINVAL is returned.
 *	If the copyout() returns an error, pass it back to the caller.
 *
 * In/Out conditions:
 *	none
 *
 ********************************************************************
 * END_IMS */

pstat_swapinfo(dest, size, num, offset)
	struct pst_swapinfo *dest;
	int size, num, offset;
{
	extern char *strncpy();		/* This is from ../sys/kern_subr.c */
					/* and does NOT act like libc version*/
	extern int nswapdev;
	extern int nswapfs;
	struct pst_swapinfo pss;
	int n_empty = 0;
	int count = 0;
	int base = 0;
	int index;

	if ((num == 0) || (offset < 0))  {
		u.u_error = EINVAL;
		return;
	}
	if ((size <= 0) || (size > sizeof(struct pst_swapinfo))) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Do any Device Swap entries
	 */
	for (;(index = (count + offset /* - base=0 */)) < nswapdev; count++) {
		swdev_t *swp = &swdevt[index];
		if (swp->sw_dev != NODEV && swp->sw_dev != SWDEF) {
			pss.pss_idx = offset + count;
			pss.pss_major = major(swp->sw_dev);
			pss.pss_minor = minor(swp->sw_dev);
			pss.pss_flags = SW_BLOCK;
			if (swp->sw_enable)
				pss.pss_flags |= SW_ENABLED;
			pss.pss_priority = swp->sw_priority;
			pss.pss_nfpgs = swp->sw_nfpgs;
			pss.pss_start = swp->sw_start;
			pss.pss_nblks = swp->sw_nblks;

			if (u.u_error = copyout((unsigned int)&pss,
						(unsigned int)dest, size)) {
				return;
			}
			dest = (struct pst_swapinfo *)( (int)dest + size );
			if (--num == 0) {
				u.u_r.r_val1 = count + 1 - n_empty;
				return;
			}
		} else {
			n_empty++;
		}
	}
	/*
	 * Now do any File System Swap entries
	 */
	base += nswapdev;
	for (;(index = (count + offset - base)) < nswapfs; count++) {
		fswdev_t *fswp = &fswdevt[index];
		if (fswp->fsw_vnode != (struct vnode *)NULL) {
			pss.pss_idx = offset + count;
			pss.pss_flags = SW_FS;
			if (fswp->fsw_enable)
				pss.pss_flags |= SW_ENABLED;
			pss.pss_priority = fswp->fsw_priority;
			pss.pss_nfpgs =	fswp->fsw_nfpgs;
			pss.pss_allocated = fswp->fsw_allocated;
			pss.pss_min = fswp->fsw_min;
			pss.pss_limit = fswp->fsw_limit;
			pss.pss_reserve = fswp->fsw_reserve;
			(void)strncpy(pss.pss_mntpt, fswp->fsw_mntpoint,
				sizeof(pss.pss_mntpt));

			if (u.u_error = copyout((unsigned int)&pss,
						(unsigned int)dest, size)) {
				return;
			}
			dest = (struct pst_swapinfo *)( (int)dest + size );
			if (--num == 0) {
				u.u_r.r_val1 = count + 1 - n_empty;
				return;
			}
		} else {
			n_empty++;
		}
	}
	count -= n_empty;
	u.u_r.r_val1 = count;
}


/* 
 * Display rudimentary PS
 */
osps()
{
	register struct proc *p;
	struct user *uptr;
	struct pte *pte_ptr;
	unsigned int x;
		

	printf(" uid  pid  ppid  pgrp stat pri   wchan  signals  flag    uticks sticks    p_addr   Name      \n");
	
	for (p = proc; p < procNPROC; p++) {
		if (p->p_stat == 0) 
			continue;

		
		printf("%5d %5d %5d %4d  %2d  %3d 0x%8x 0x%3x  0x%5x %5d %5d   0x%8x",
			p->p_uid, p->p_pid, p->p_ppid, 
			(p->p_pgrp >= 0) ? p->p_pgrp : 0, 
			p->p_stat, p->p_pri, p->p_wchan, p->p_sig, p->p_flag, 
			p->p_uticks, p->p_sticks, p->p_addr);

		if (p->p_pid > 10)
			printf("*");
		if (p->p_ppid > 10)
			printf("*");
#ifdef __hp9000s300			
		if ((p->p_flag & SLOAD) == 0)
			printf("   <swapped>\n");
		else {
			if (p->p_addr->pg_v & PG_IV) {
				x = (unsigned int) *((int *) p->p_addr);
				pte_ptr = (struct pte *) (x & 0xfffffffc);
			} else 
				pte_ptr = p->p_addr;
			uptr = (struct user *) (pte_ptr->pg_pfnum << PGSHIFT);
			printf(" %s\n", uptr->u_comm);
		}
#endif		
	}
}
