/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/subr_xxx.c,v $
 * $Revision: 1.7.84.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/06/08 16:56:27 $
 */

/* HPUX_ID: @(#)subr_xxx.c	55.1		88/12/23 */

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


#include "../h/param.h"
#ifdef POSIX
#include "../h/unistd.h"
#endif /* POSIX */
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/uio.h"
#include "../h/dbg.h"

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{
	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{
	return (0);
}

/*
 * notty routine; placed in ioctl cdevsw entries
 * for drivers that do not support ioctl.
 */
notty()
{
	return (ENOTTY);
}

imin(a, b)
{
	return (a < b ? a : b);
}

imax(a, b)
{
	return (a > b ? a : b);
}

unsigned
min(a, b)
	u_int a, b;
{
	return (a < b ? a : b);
}

unsigned
max(a, b)
	u_int a, b;
{
	return (a > b ? a : b);
}

caddr_t	cacur = (caddr_t)0;
/*
 * This is a kernel-mode storage allocator.
 * It is very primitive, currently, in that
 * there is no way to give space back.
 * It serves, for the time being, the needs of
 * auto-configuration code and the like which
 * need to allocate some stuff at boot time.
 */
caddr_t
calloc(size)
	int size;
{
	register caddr_t res;

	if (cacur+size > (caddr_t)roundup((int)cacur, NBPG)) {
		cacur = (caddr_t)zmemall(memall, size);
	}
	res = cacur;
	if (cacur != 0) {	/* if no zmemall() error */
		cacur += size;
		cacur = (caddr_t)roundup((int)cacur, sizeof (double));
	}
	return (res);
}


ffs(mask)
	register long mask;
{
	register int i;

	if (mask == 0) return(0);
	for (i = 1; ; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
}

bcmp(s1, s2, len)
	register char *s1, *s2;
	register unsigned len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}



strlen(s1)
	register char *s1;
{
	register int len;

	for (len = 0; *s1++ != '\0'; len++)
		/* void */;
	return (len);
}





#ifdef	DBG
chkrqs()
{
	struct proc *p;
	int s;
	
	s = spl7();
	for (p = proc; p < procNPROC; p++) {
		if ( (p->p_stat == SRUN) && (p->p_rlink == (struct proc *) 0)
			&& (p->p_flag & SLOAD)
			&& (p != u.u_procp) )
			panic("chkrqs");
	}
	splx(s);
}
#endif	DBG

/*
 * Emulate the vax instructions for queue insertion and deletion.
 * Although they are generally used for process insertion and deletion,
 * they may be used for other types of queues.  That's why the arguments
 * are caddr_t's.  However, we cast them as proc pointers as a model. 
 * NOTE: if used for other than proc insertion and deletion, the offsets
 * of the links must be the same as proc's.
 */

_insque(elementp, quep)
	caddr_t  elementp, quep;
{
#define element ( (struct proc *) elementp)
#define que ( (struct proc *) quep)
	element->p_link = que->p_link;
	element->p_rlink = que;

	que->p_link->p_rlink = element;
	que->p_link = element;
}

_remque(elementp)
	caddr_t elementp;
{
	element->p_link->p_rlink = element->p_rlink;
	element->p_rlink->p_link = element->p_link;

	element->p_rlink = element->p_link = (struct proc *) 0;
}
#undef element
#undef que

scanc(size, cp, table, mask)
	register unsigned size;
	register caddr_t cp, table;
	register int mask;
{
	register int i = 0;

	while ((table[*(u_char *)(cp + i)]&mask) == 0 && i < size)
		i++;
	return (size - i);
}


/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
strcmp(s1, s2)
	register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return (0);
	return (*s1 - *--s2);
}

strncmp(s1, s2, n)
        register char *s1, *s2;
        register int n;
{

        while (--n >= 0 && *s1 == *s2++)
                if (*s1++ == '\0')
                        return (0);
        return (n<0 ? 0 : *s1 - *--s2);
}

/*
 * Copy string s2 to s1.  s1 must be large enough.
 * return ptr to null in s1
 */
char *
strcpy(s1, s2)
	register char *s1, *s2;
{

	while (*s1++ = *s2++)
		;
	return (s1 - 1);
}

/*
 * Copy s2 to s1, truncating to copy n bytes
 * return ptr to null in s1 or s1 + n
 */
char *
strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register i;

	for (i = 0; i < n; i++) {
		if ((*s1++ = *s2++) == '\0') {
			return (s1 - 1);
		}
	}
	return (s1);
}

sysconf() {
	extern int processor;

	register struct a {
	        int     name;
	} *uap = (struct a *)u.u_ap;
	extern int maxuprc;
	extern int msem_proc_init();

	switch (uap->name) {

	case _SC_ARG_MAX:
		u.u_r.r_val1 = NCARGS-2;	/* max len of args+env 2 exec */
		break;

	case _SC_CHILD_MAX:
		u.u_r.r_val1 = maxuprc+1;	/* max procs per user */
		break;

	case _SC_CLK_TCK:
		u.u_r.r_val1 = HZ;              /* ticks per second */
		break;

	case _SC_NGROUPS_MAX:
		u.u_r.r_val1 = NGROUPS;		/* max supplementary groups */
		break;

	case _SC_OPEN_MAX:
		u.u_r.r_val1 = u.u_maxof;	/* max open files per process */
		break;

	case _SC_JOB_CONTROL:
#ifdef _POSIX_JOB_CONTROL
		u.u_r.r_val1 = _POSIX_JOB_CONTROL;    /* yes, we have it */
#else
		u.u_r.r_val1 = -1;              /* not yet */
#endif
		break;

	case _SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		u.u_r.r_val1 = _POSIX_SAVED_IDS;	/* yes, we have them */
#else
		u.u_r.r_val1 = -1;			/* not yet */
#endif
		break;

	case _SC_VERSION:
#ifdef _POSIX_VERSION
		u.u_r.r_val1 = _POSIX_VERSION;
#else
		u.u_r.r_val1 = -1;              /* We don't really conform */
#endif                                          /* fully to any version yet*/
		break;

	case _SC_XOPEN_VERSION:
		u.u_r.r_val1 = _XOPEN_VERSION;
		break;

	case _SC_PASS_MAX:
		u.u_r.r_val1 = 8;         /* ==> PASS_MAX in <limits.h> */
		break;
	case _SC_PAGE_SIZE:
		u.u_r.r_val1 = NBPG;         /* software page size */
		break;
        case _SC_AES_OS_VERSION:
                u.u_r.r_val1 = _AES_OS_VERSION;
                break;
        case _SC_ATEXIT_MAX:
                u.u_r.r_val1 = 32;  /* ==> ATEXIT_MAX in <limits.h> */
                break;
        case _SC_CPU_VERSION:
		switch(processor) {
			case M68020: u.u_r.r_val1 = CPU_HP_MC68020; break;
			case M68030: u.u_r.r_val1 = CPU_HP_MC68030; break;
			case M68040: u.u_r.r_val1 = CPU_HP_MC68040; break;
			default: 
				/* unlikely case, blow it off as an error */
				u.u_error = EINVAL;
				u.u_r.r_val1 = -1; 
				break;
		}
		break;
	case _SC_IO_TYPE:
		/* style of I/O, always WSIO on Series 300 */
		u.u_r.r_val1 = IO_TYPE_WSIO;
		break;
	case _SC_MSEM_LOCKID:
		u.u_r.r_val1 = msem_proc_init(); /* msemaphore lock id */
		break;
	default:
		u.u_error = EINVAL;
	}
}

/*
 * This procedure is defined only for NFS.
 * It does not have the same semantics as
 * the standard library function.
 * Copy s2 to s1, truncating to copy n bytes
 * return ptr to null in s1 or s1 + n
 */
char *
nfs_strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register i;

	for (i = 0; i < n; i++) {
		if ((*s1++ = *s2++) == '\0') {
			return (s1 - 1);
		}
	}
	return (s1);
}

/*
 * MP control system call.  At present, we have no plans to officially (or
 * permanently) support this.  As a result, there is no man page or system
 * call stub.  This interface may change or disappear with little or no
 * notice at any time.
 *
 * XXX This is being added to IF3 for binary compatability and its contents
 * has been gutted.  This should be restored when MP is really supported
 * on the S700.  See the S800-MU source for the real version of this routine.
 */

int
mpctl() {
	register struct a {
	        int cmd;
	        unsigned long arg1;
	        unsigned long arg2;
	} *uap = (struct a *)u.u_ap;
	spu_t spu;

	switch (uap->cmd) {

	case MPC_GETNUMSPUS:
		/* 
		 * The number of CURRENTLY running processors.  Always
		 * one in a uniprocessor system
		 */
		u.u_rval1 = 1; /* The number of CURRENTLY running processors. */
		break;

	case MPC_GETFIRSTSPU:
	case MPC_GETCURRENTSPU:
		/*
		 * The index of the first and current SPUs are
		 * equal in a uniprocessor system.
		 */
		u.u_rval1 = 0;
		break;

	case MPC_GETNEXTSPU:
		/*
		 * XXX We could get tricky and say that if the arg was -1 then
		 * the next processor is 0, however, for a single SPU machine
		 * we just say there is no next processor, fall through.
		 */
	case MPC_SETPROCESS:
		/*
		 * XXX We are not supporting CPU groups, fall through.
		 */
	default:
		u.u_rval1 = -1;
		u.u_error = EINVAL;
		break;
	}
}




/*
 * The following routines are for dumping the stacks, or accessing the user
 * structures, of processes other than the current process.  All are callable
 * from the kernel debugger.  Only in-core processes are accessable.
 * The routines are:
 *
 *	dump_all_stacks		Dumps the stacks of all in-core processes
 *				except the current one (or zombies).
 *				No parameters.
 *
 *	dump_csp_stacks		Dumps the stacks of all CSPs.  No parameters.
 * 				(in test.c)
 *
 *	dump_stack_pid		Dumps the stack of one process.  Parameter
 *				is pid.
 *
 *	dump_stack		Dumps the stack of one process.  Parameter
 *				is pointer to proc structure.
 *
 */

extern int (*test_printf)(); /* defined in trap.c */
#define	printf (*test_printf)



is_mappable_proc (p, verbose)
register struct proc *p;
int verbose;
{
	if (p < proc || p >= procNPROC ||
	    ((int)p - (int)proc) % sizeof(struct proc) != 0) {
		printf ("0x%x not a valid proc pointer\n", p);
		return(0);
	}
	if (p->p_stat == 0) {
		printf ("that proc structure is unused\n");
		return(0);
	}
	if ((p->p_flag & SLOAD) == 0) {
		if (verbose)
			printf ("that process is swapped out\n");
		else
			printf ("<swapped>\n");
		return(0);
	}
	if (p->p_stat == SZOMB) {
		if (verbose)
			printf ("that process is a zombie\n");
		else
			printf ("<defunct>\n");
		return(0);
	}
	if (p->p_stat == SIDL) {
		if (verbose)
			printf ("that process is just being forked (SIDL)\n");
		else
			printf ("<forking>\n");
		return(0);
	}
	if (p == u.u_procp) {
		if (verbose)
			printf ("cannot map in the current process\n");
		else
			printf ("<current>\n");
		return(0);
	}
	return (1);
}

dump_all_stacks()
{
	register struct proc *p;

	for (p = proc; p < procNPROC; p++) {
		if (p->p_stat != 0) {
			printf ("0x%x: pid %d, ppid %d, pgrp %d, state %d, ",
				p, p->p_pid, p->p_ppid, p->p_pgrp, p->p_stat);
			if (is_mappable_proc (p, 0))
				dump_stack(p);
		}
	}
}



dump_stack_pid(pid)
int pid;
{
	register struct proc *p;

	for (p = proc; p < procNPROC; p++) {
		if (p->p_pid == pid && p->p_stat != 0) {
			dump_stack(p);
			return;
		}
	}
	printf ("pid %d not found\n", pid);
}

extern unsigned int CMAP2_ptes;
extern unsigned int kgenmap_ptes;


/* translate pointer to other process's kernel stack (uses the tt window) */
caddr_t
kern_stack_trans(p, ptr)
	struct proc *p;
	unsigned ptr;
{
	register int *ks_ptr;

	ptr -= KSTACKBASE; /* make it page relative */

	if (ptr >= KSTACK_PAGES * NBPG) {
		printf("ptr = 0x%x; should be < 0x%x\n", ptr, KSTACK_PAGES*NBPG);
		return(0);
	}
	
	ks_ptr = ((int *)p->p_addr) + (ptr >> PGSHIFT) - KSTACK_PAGES;
	if (indirect_ptes)
	    ks_ptr = (int *)(*ks_ptr & ~0x3);
	return (caddr_t)((*ks_ptr & ~PGOFSET) + (ptr & PGOFSET));
}

/*
 * Most of the routine dump_stack() was lifted from the code in boot()
 * to dump stacks when we panic.
 */

#define	CALL2	0x4E90		/* jsr (a0)    */
#define	CALL6	0x4EB9		/* jsr address */
#define	BSR	0x6100		/* bsr address */
#define BSRL    0x61FF		/* bsr address w/32 bit displacement */

#define	ADDQL	0x500F		/* addq.l #xxx,sp */
#define	ADDL	0xDEFC		/* add.l  #xxx,sp */
#define	LEA 	0x4fef		/* lea    xxx(sp),sp */

dump_stack(p)
	register struct proc *p;
{
	int rtn, *pp; 
	short inst;
	int calladr, entadr;
	int args_size;
	int *stack_ptr;
	int *bt_link;
	struct user *uptr;

	if (!is_mappable_proc (p, 1))
		return;

	purge_tlb();
	purge_dcache();

	if (indirect_ptes)
	    uptr = (struct user *)(*(int *)(*(int *)p->p_addr & ~0x3) & ~PGOFSET);
	else
	    uptr = (struct user *)(*(int *)p->p_addr & ~PGOFSET);

	switch(p->p_pid)
	{
		case PID_SWAPPER:	
			printf("swapper\n");
			break;
		case PID_PAGEOUT: 
			printf("vhand\n");
			break;
		case PID_STAT: 
			printf("statdaemon\n");
			break;
		case PID_LCSP:
			printf("lcsp\n");
			break;
		case PID_NETISR:
			printf("netisr\n");
			break;
		case PID_SOCKREGD:
			printf("sockregd\n");
			break;
#ifdef GENESIS
		case PID_VDMAD:
			printf("vdmad\n");
			break;
#endif
		default:
#ifdef notdef
			if (uptr->u_nsp == nsp)
				printf ("limited CSP\n");
			else
#endif				
			if (uptr->u_nsp != 0)
				printf ("general CSP\n");
			else if (uptr->u_comm[0] == '\0')
				printf ("u_comm is null\n");
			else
				printf ("%s\n", uptr->u_comm);
	}

	bt_link = (int *)uptr->u_rsave.val[11];
	stack_ptr = (int *)uptr->u_rsave.val[12];

	while ((pp = bt_link) >= stack_ptr
	    && pp < (int *) KSTACKADDR ) {

		if (((int) pp & 01) || !testr(kern_stack_trans(p, pp), 8))
			break;


		bt_link = *(int **) kern_stack_trans(p, pp);
		rtn = *(int *)kern_stack_trans(p, ++pp);


		calladr = NULL;
		entadr = NULL;

		if (!testr(rtn-6, 12) || (rtn & 01))
			break;

		if (*(short *)(rtn-6) == CALL6) {
			entadr = *(int *)(rtn-4);
			calladr = rtn - 6;
		} else if (*((short *) (rtn-6)) == BSRL) {
			entadr = *((int *) (rtn-4)) + (rtn - 4);
			calladr = rtn - 6;
		} else if ((*(short *)(rtn-4) & ~0xFFL) == BSR) {
			calladr = rtn - 4;
			entadr = (short) *(short *)(rtn-2) + rtn-2;
		} else if ((*(short *)(rtn-2) & ~0xFFL) == BSR) {
			calladr = rtn - 2;
			entadr = (char) *(short *)(rtn-2) + rtn;
		} else if (*(short *)(rtn-2) == CALL2) {
			calladr = rtn - 2;
		}

		inst = *(short *)rtn;

		if ((inst & 0xF13F) == ADDQL) {
			args_size = (inst>>9) & 07;
			if (args_size == 0) args_size = 8;
		}
		else if ((inst & 0xFEFF) == ADDL) {
			if (inst & 0x100)
				args_size = *(int *)(rtn + 2);
			else
				args_size = *(short *)(rtn + 2);
		}
		else if ((inst & 0xFFFF) == LEA)
			args_size = *(short *)(rtn + 2);
		else
			args_size = 0;	   /* no adjustment, no args! */

		printf("%08x: ", calladr);
		debug_print_addr(entadr); printf(" (");

		for (args_size-=4; args_size>=0; args_size-=4) {
			pp++;
			printf(*(int *)kern_stack_trans(p, pp)>=0 &&
			       *(int *)kern_stack_trans(p, pp)<=9 ? "%d" : "0x%x",
			       *(int *)kern_stack_trans(p, pp));
			if (args_size)
				printf(", ");
		}

		printf(")\n");
	}
}

/* A small symbol table of common entry points for panic backtraces. */

extern int trap(), buserror(), pmmubuserror(), syscall(), sleep(), _sleep(),
	wait1(), ilock(), isynclock(), 
	bwrite(), iupdat(), update(), ufs_rdwr(), biowait(),
	rwip(), rwuio(), ttread(), ttwrite(), canon();

extern int main(), syncd_fork(), 
	schedcpu(), vhand(), sched(), activate(), open(), close(),
	read(), write(), fork(), exit(), execve();

struct debug_symtab {
	char *symbol_name;
	int (*symbol_addr)();
} debug_symtab[] = {
	{"   buserror",		buserror},
	{"pmmubuserror",	pmmubuserror},
	{"    syscall",		syscall},
	{"      sleep",		sleep},
	{"     _sleep",		_sleep},
	{"       trap",		trap},
	{"      wait1",		wait1},
	{"      ilock",		ilock},
	{"     getblk",		(int (*)())getblk}, /* declared in a header */
	{"      bread",		(int (*)())bread},  /* declared in a header */
	{"     bwrite",		bwrite},
	{"     biowait",	biowait},
	{"     iupdat",		iupdat},
	{"     update",		update},
	{"   ufs_rdwr",		ufs_rdwr},
	{"       rwip",		rwip},
	{"      rwuio",		rwuio},
	{"     ttread",		ttread},
	{"    ttwrite",		ttwrite},
	{"      canon",		canon},
	{"       main",		main},
	{" syncd_fork",		syncd_fork},
	{"   schedcpu",		schedcpu},
	{"      vhand",		vhand},
	{"      sched",		sched},
	{"   activate",		activate},
	{"       open",		open},
	{"      close",		close},
	{"       read",		read},
	{"      write",		write},
	{"       fork",		fork},
	{"       exit",		exit},
	{"     execve",		execve},
	{"", 0}
};

debug_print_addr(a)
register char  *a;
{
	register struct debug_symtab *p;

	/* Try to translate the address to a name */
	for (p=debug_symtab; p->symbol_addr; p++) {
		if ((char *) p->symbol_addr == a) {
			printf("%s", p->symbol_name);
			return;
		}
	}

	if (a == NULL)
		printf("        ???");
	else
		printf("   %08x", a);
}




os_global_summary()
{
	extern int sum_pfdat();
	extern char sysname;


	printf("physmem = %d, freemem = %d, nbuf = %d, bufpages = %d, lockable_mem = %d\n",
		physmem, freemem, nbuf, bufpages, lockable_mem);
	printf("booted from %s, rate.v_runq = %d\n", &sysname, rate.v_runq);	
	sum_pfdat();
}




#undef	printf

extern int printf(), uprintf(), msg_printf(), (*kdb_printf)();

/* return values from calls to switch printf destination */
#define	PRINTF		0
#define	UPRINTF		1
#define	MSG_PRINTF	2
#define	KDB_PRINTF	3



switch_test_printf(new)
int new;
{
	int old;
	extern int monitor_on;

	if (test_printf == printf)
		old = PRINTF;
	if (test_printf == uprintf)
		old = UPRINTF;
	if (test_printf == msg_printf)
		old = MSG_PRINTF;
	if (test_printf == kdb_printf)
		old = KDB_PRINTF;

	switch (new) {
	case PRINTF:
		test_printf = printf;
		break;

	case UPRINTF:
		test_printf = uprintf;
		break;

	case MSG_PRINTF:
		test_printf = msg_printf;
		break;

	case KDB_PRINTF:
		if (monitor_on)
			test_printf = kdb_printf;
		else {
			u.u_error = ENXIO;
			return (-1);
		}
		break;
	}
	return (old);
}

testprint_console()
{
	return (switch_test_printf(PRINTF));
}

testprint_user()
{
	return (switch_test_printf(UPRINTF));
}

testprint_dmesg()
{
	return (switch_test_printf(MSG_PRINTF));
}

testprint_debug()
{
	return (switch_test_printf(KDB_PRINTF));
}

