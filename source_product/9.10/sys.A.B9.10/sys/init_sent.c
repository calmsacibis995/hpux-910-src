/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/init_sent.c,v $
 * $Revision: 1.73.83.9 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/10 11:40:46 $
 */

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

/*
 * System call switch table.
 */

/*
 * A pair of tutorials follow, one on adding system calls, one on
 * deleting them.  PLEASE READ THEM BEFORE MAKING CHANGES TO THIS FILE
 * OR h/syscall.h.
 */

/*
 * How To Add A System Call - A Short Tutorial
 *
 * Every system call must have a unique number on each architecture
 * (300, 800, etc...).  The number needn't be the same on every
 * architecture (and for some old calls, they aren't the same), but no
 * new system calls should be added that don't use the same number on
 * all machines.
 * 
 * Also, even if the call is only intended for one of the architectures,
 * choose a number that is free on all and define the number for all
 * machines.  This way, if a decision is made some years down the road
 * to port the call to the other machines, the system call number
 * doesn't have to be different on the other machines.
 *
 * Adding a new system call requires changing two kernel files:
 *	h/syscall.h and
 *	sys/init_sent.c.
 *
 * If you look at syscall.h, you will notice that every system call
 * has two entries, one in all uppercase and one partly in lower case.
 * For example,
 *	#define	SYS_EXIT	1
 *	#define	SYS_exit	1
 * You must continue this tradition.
 *
 * Please keep the system call defines in numeric order.
 *
 * The corresponding entries for this file would look like this:
 *	int rexit();
 *
 *	scall(1, rexit),
 *
 * The first is a declaration to satisfy the C compiler.  Please put it
 * under the appropriate sub-heading ("1.1 processes and protection" or
 * whatever).  These declarations are grouped by general function or,
 * where there is no general connection, alphabetically.
 *
 * The second is the actual entry in the sysent table.  This is the
 * table the kernel actually branches through to make system calls.
 * There is an entry for every system call, consisting of a struct that
 * describes the call.  This includes the number of arguments to expect
 * the user to have passed in, the address of the kernel routine to
 * call, and a string used for syscalltracing (a debugging tool).
 *
 * In the above example, rexit is the kernel routine that implements the
 * exit system call and it expects 1 argument.  NOTE:  Please add a
 * trailing comment after each entry indicating what the system call number
 * and name are expected to be.  For example, the comment after the above
 * entry would be "1 = exit".
 *
 * The scall macro automatically builds the syscalltracing string, so
 * you should use this macro (or one that calls it) to make sure the
 * data structures stay current and consistent.
 *
 * Some additional macros are provided for use instead of scall for
 * special situations.  These macros are provided to keep the sysent
 * table's definition readable.  These macros are all written in terms
 * of the scall macro.  Feel free to add new macros of this type, but
 * please stick to the conventions used by the others.
 *
 * Some examples:
 *
 * bsd()        used for entries that should be nosys if BSD_ONLY is not
 *		defined
 * nohpuxboot() used for entries that should be nosys if HPUXBOOT is
 *		defined
 * fsd_ki()	used for entries that should be nosys if FSD_KI is not
 *		defined
 * s300()       used for entries that should be nosys if __hp9000s300 is
 *		not defined
 */

/*
 * How To Delete A System Call - A Short Tutorial
 *
 * If you have to delete a system call for some reason, leave a comment
 * in h/syscall.h that warns people never to use that system call number
 * for anything else.  Change the sysent table entry for the call to
 * scall(0, nosys) in this file and place a comment next to it giving
 * the same warning that you used in syscall.h.
 *
 * If a system call number ever gets reused and an old a.out is run on
 * such a system, the program will run a new and exciting system call
 * which it was not expecting.  The preferred behavior is to return from
 * the system call with errno set to ENOSYS, which is what nosys() does.
 */

#ifdef __hp9000s300
#include "../sio/dvio.h"
#endif

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/syscall.h"

#ifdef PDG_NOTDEF
#include "../h/user.h"
#include "../h/errno.h"
#endif PDG_NOTDEF

int	nosys();

#ifdef _WSIO
/* hpib_io hook for dil */
int (*hpib_io_proc)() = nosys;

hpib_io_stub(eid, iovec, ioveccnt)
int eid;
struct iodetail *iovec;
int ioveccnt;
{
	(*hpib_io_proc)(eid, iovec, ioveccnt);
}
#endif /* _WSIO */

/* 1.1 processes and protection */

int	getgid();
int	getgroups();
#ifndef HPUXBOOT
int	getpgrp2();
#endif HPUXBOOT
int	getpid();
int	getuid();
int	privgrp();
int	profil();
int	pstat();
int	setgroups();
#ifndef HPUXBOOT
int	setpgrp2();
#endif HPUXBOOT
int	setresgid();		/* used to be setregid */
int	setresuid();		/* used to be setreuid */
int	wait3();

#ifndef HPUXBOOT
int	execv();
int	execve();
int	fork();
int	rexit();
int	vfork();
int	wait();
#endif /* !HPUXBOOT */

#ifdef FSS
int	fss_default();		/* fair share scheduler default */
#endif

#ifdef GETMOUNT
int	getmount_cnt();		/* get kernel mount table info */
int	getmount_entry();
#endif

/* 1.2 memory management */

int	obreak();		/* awaiting new sbrk */

#ifdef  QUOTA
int	quotactl();
#endif

#if defined(__hp9000s300) && defined(BSD_ONLY)
int	getpagesize();
int	mincore();
int	mremap();
int	sbrk();
int	sstk();
#endif	/* __hp9000s300 && BSD_ONLY */

/* Memory map system calls */

int	smmap();	/*  71 */
int	munmap();	/*  73 */
int	mprotect();	/*  74 */
int	madvise();	/*  75 */
int	msync();	/* 320 */
int	msleep();	/* 321 */
int	mwakeup();	/* 322 */
int     msem_init();    /* 323 */
int     msem_remove();  /* 324 */

/* 1.3 signals */

int	kill();
int	sigaction();
int	sigblock();
int	sigpause();
int	sigpending();
int	sigprocmask();
int	sigsetmask();
int	sigsetreturn();
int	sigstack();
int	sigsuspend();
int	sigvec();
int	waitpid();

#ifdef __hp9000s300
int	sigcleanup();
#endif

#if defined(__hp9000s300) && defined(BSD_ONLY)
int	killpg();
#endif

#ifdef __hp9000s800
int	sigsetstatemask();
#endif

/* 1.4 timing and statistics */

int	getitimer();
int	setitimer();
int	gettimeofday();
int	settimeofday();
int	adjtime();

/* 1.5 descriptors */

int	close();
int	dup();
int	dup2();
int	fcntl();
int	getnumfds();
int	lockf();
int	select();

#if defined(__hp9000s300) && defined(BSD_ONLY)
int	flock();
int	getdopt();
int	getdtablesize();
int	setdopt();
#endif

/* 1.6 resource controls */

int	getpriority();
int	setpriority();
int	getrlimit();
int	setrlimit();
int	getrusage();

#ifdef lint
/*
 * int & long are the same, but keep ulimit() as long for the real thing.
 * (ulimit() is of type long on the man page.)
 */
int	ulimit();
#else
long	ulimit();
#endif /* lint */

/* 1.7 system operation support */

int	reboot();
int	smount();
int	swapoff();
int	swapon();
int	sync();
int	sysacct();
int	umount();

/* 2.1 generic operations */

int	ioctl();
int	read();
int	readv();
int	write();
int	writev();

/* 2.2 file system */

int	chdir();
int	chmod();
int	lchmod();
int	chown();
int	chroot();
int	creat();
int	fchdir();
int	fchmod();
int	fchown();
int	fstat();
int	fsync();
int	ftruncate();
int	link();
int	lseek();
int	lstat();
int	mkdir();
int	mknod();
int	open();
int	pipe();
int	readlink();
int	rename();
int	rmdir();
int	saccess();
int	stat();
int	symlink();
int	truncate();
int	umask();
int	unlink();

int	swapfs();

#if defined(__hp9000s300) && defined(BSD_ONLY)
int	utimes();
#endif

/* streams */

int	getmsg();
int	poll();
int	putmsg();
int     getpmsg();
int     putpmsg();
int     strioctl();

/* 2.4 processes */

int	ptrace();
int	setcore();

/* 2.5 terminals */

/* 2.6 realtime additions */
int	lock();			/* process locking */
int	rtprio();		/* realtime priorities */

/* System V calls */

int	utssys();

#if defined(MESG) || defined(_WSIO)
int	msgctl();
int	msgget();
int	msgrcv();
int	msgsnd();
#endif

#if defined(SEMA) || defined(_WSIO)
int	semctl();
int	semget();
int	semop();
#endif

#if defined(SHMEM) || defined(_WSIO)
int	shmat();
int	shmctl();
int	shmdt();
int	shmget();
#endif

#ifdef NSYNC
int	tsync();		/* trickle sync used by syncer */
#endif

/* emulations for backward incompatibility */

#define	compat(name)		o/**/name

#define	scall(n, name)		n, name, "name"

#ifdef BSD_ONLY
#define	bsd(n, name)		scall(n, name)
#else
#define	bsd(n, name)		scall(0, nosys)
#endif

#ifndef HPUXBOOT
#define	nohpuxboot(n, name)	scall(n, name)
#else
#define	nohpuxboot(n, name)	scall(0, nosys)
#endif

#ifdef FSD_KI
#define	fsd_ki(n, name)		scall(n, name)
#else
#define	fsd_ki(n, name)		scall(0, nosys)
#endif

#if defined(__hp9000s300) && defined(TRACE)
#define	trace(n, name)		scall(n, name)
#else
#define	trace(n, name)		scall(0, nosys)
#endif

#ifdef __hp9000s300
#define	s300(n, name)		scall(n, name)
#else
#define	s300(n, name)		scall(0, nosys)
#endif

#ifdef ACLS
#define	acls(n, name)		scall(n, name)
#else
#define	acls(n, name)		scall(0, nosys)
#endif

#ifdef AUDIT
#define	audit(n, name)		scall(n, name)
#else
#define	audit(n, name)		scall(0, nosys)
#endif

#if defined(SEMA) || defined(_WSIO)
#define	sema(n, name)		scall(n, name)
#else
#define	sema(n, name)		scall(0, nosys)
#endif

#if defined(SHMEM) || defined(_WSIO)
#define	shmem(n, name)		scall(n, name)
#else
#define	shmem(n, name)		scall(0, nosys)
#endif

#if defined(MESG) || defined(_WSIO)
#define	mesg(n, name)		scall(n, name)
#else
#define	mesg(n, name)		scall(0, nosys)
#endif


#define	dux(n, name)		scall(n, name)

int	oalarm();		/* now use setitimer */
int	oftime();		/* now use gettimeofday */
int	ogtty();		/* now use ioctl */
#ifdef XTERM_MEM_TEST
int	getfreemem ();		/* in xterm_syms.c. Uses the nice() entrypoint*/
#else
int	onice();		/* now use setpriority,getpriority */
#endif /* XTERM_MEM_TEST */
int	opause();		/* now use sigpause */
int	osetgid();		/* now use setregid */
int	osetpgrp();		/* ??? */
int	osetuid();		/* now use setreuid */
int	ostime();		/* now use settimeofday */
int	ostty();		/* now use ioctl */
int	otime();		/* now use gettimeofday */
int	otimes();		/* now use getrusage */
int	outime();		/* now use utimes */

#ifdef __hp9000s300
int	ofstat();		/* now use fstat */
int	ossig();		/* now use sigvec, etc */
int	ostat();		/* now use stat */
#endif

#if defined(SEMA) || defined(_WSIO)
int     osemctl();		/* now use semctl */
#endif

#if defined(MESG) || defined(_WSIO)
int     omsgctl();		/* now use msgctl */
#endif

#if defined(SHMEM) || defined(_WSIO)
int     oshmctl();		/* now use shmctl */
#endif

#if defined(POSIX_SET_NOTRUNC) || defined(S2POSIX_NO_TRUNC)
int	set_no_trunc();
#endif

int	fpathconf();
int	pathconf();
int	sysconf();

int	fsctl();

#if defined(__hp9000s800) && defined(BFA)
int	bfactl();
#endif

/* Priveleged Instruction System Calls */

/* proc_sendrecv */

int	proc_close();
int	proc_open();
int	proc_recv();
int	proc_send();
int	proc_sendrecv();
int	proc_syscall();

#ifdef __hp9000s800
/*
 * compare and swap system calls
 */
int	cds();
int	cs();
#endif /* hp9000s800 */


#ifdef __hp9000s800
/* this system call is used by sar(1m) */
int	get_sysinfo();	       /* used to obtain sysinfo structure */
#endif
/*
 * These are forward declarations for NFS by project NERFS.
 * Taken out of ifdefs for configuration -- gmf.
 */
/*int	async_daemon();*/
/*int	getdirentries(); see notes below*/
/*int	nfs_getfh();*/
/*int	nfs_svc();*/
/*int	configstatus();  not used -- gmf */
/*int	setdomainname(); */

int	fstatfs();
int	getdirentries();
int	getdomainname();
int	statfs();
int	vfsmount();

#if defined(notneeded) && defined(__hp9000s800)
int	exportfs();		/* export file systems */
#endif /* notneeded && hp9000s800 */

#ifdef FULLDUX
int	bigio();
int	pipenode();
int	remount();
#endif /* FULLDUX */

int	dux_notconfigured();
int	returnzero();

#ifdef AUDIT
int	audctl();
int	audswitch();
int	audwrite();
int	getaudid();
int	setaudid();
int	getaudproc();
int	setaudproc();
int	getevent();
int	setevent();
#endif

#ifdef ACLS
int	fgetacl();
int	fsetacl();
int	getaccess();
int	getacl();
int	setacl();
#endif

int	cluster();
int	getcontext();
int	setcontext();
int	lsync();
int	mysite();

int	mkrnod();

#ifdef TRACE
int	vtrace();
#endif

int	vhangup();		/*clean up that tty */

#ifdef	FSD_KI
int	ki_syscall();		/* control kernel instrumentation */
#endif

int mpctl();

/*
 * System call branch and tracing table.
 */

struct sysent sysent[] = {
	scall(0, nosys),		/*   0 = indir */
	nohpuxboot(1, rexit),		/*   1 = exit */
	nohpuxboot(0, fork),		/*   2 = fork */
	scall(3, read),			/*   3 = read */
	scall(3, write),		/*   4 = write */
	scall(3, open),			/*   5 = open */
	scall(1, close),		/*   6 = close */
	nohpuxboot(1, wait),		/*   7 = wait (only one) */
	scall(2, creat),		/*   8 = creat */
	scall(2, link),			/*   9 = link */
	scall(1, unlink),		/*  10 = unlink */
	nohpuxboot(2, execv),		/*  11 = execv */
	scall(1, chdir),		/*  12 = chdir */
	scall(1, compat(time)),		/*  13 = old time */
	scall(3, mknod),		/*  14 = mknod */
	scall(2, chmod),		/*  15 = chmod */
	scall(3, chown),		/*  16 = chown; now 3 args */
	scall(1, compat(break)),	/*  17 = old break */
	s300(2, compat(stat)),		/*  18 = old stat */
	scall(3, lseek),		/*  19 = lseek */
	scall(0, getpid),		/*  20 = getpid */
	scall(3, smount),		/*  21 = mount */
	scall(1, umount),		/*  22 = umount */
	scall(1, compat(setuid)),	/*  23 = old setuid */
	scall(0, getuid),		/*  24 = getuid */
	scall(1, compat(stime)),	/*  25 = old stime */
	scall(5, ptrace),		/*  26 = ptrace */
	scall(1, compat(alarm)),	/*  27 = old alarm */
	s300(2, compat(fstat)),		/*  28 = old fstat */
	scall(0, compat(pause)),	/*  29 = opause */
	scall(2, compat(utime)),	/*  30 = old utime */
	scall(2, compat(stty)),		/*  31 = was stty */
	scall(2, compat(gtty)),		/*  32 = was gtty */
	scall(2, saccess),		/*  33 = access */
#ifdef XTERM_MEM_TEST
	scall(1, getfreemem),		/*  34 = getfreemem. Relpaces nice. */
#else
	scall(1, compat(nice)),		/*  34 = old nice */
#endif /* XTERM_MEM_TEST */
	scall(1, compat(ftime)),	/*  35 = old ftime */
	scall(0, sync),			/*  36 = sync */
	scall(2, kill),			/*  37 = kill */
	scall(2, stat),			/*  38 = stat */
	scall(1, compat(setpgrp)),	/*  39 = old setpgrp */
	scall(2, lstat),		/*  40 = lstat */
	scall(1, dup),			/*  41 = dup */
	scall(0, pipe),			/*  42 = pipe */
	scall(1, compat(times)),	/*  43 = old times */
	scall(4, profil),		/*  44 = profil */
	fsd_ki(4, ki_syscall),		/*  45 = ki_syscall */
	scall(1, compat(setgid)),	/*  46 = old setgid */
	scall(0, getgid),		/*  47 = getgid */
	s300(2, compat(ssig)),		/*  48 = old sig */
	scall(0, nosys),		/*  49 = reserved for USG */
	scall(0, nosys),		/*  50 = reserved for USG */
	scall(1, sysacct),		/*  51 = turn acct off/on */
	scall(0, nosys),		/*  52 = old set phys addr */
	scall(0, nosys),		/*  53 = old lock in core */
	scall(3, ioctl),		/*  54 = ioctl */
	scall(4, reboot),		/*  55 = reboot */
	scall(2, symlink),		/*  56 = symlink (57 in 4.2) */
	scall(3, utssys),		/*  57 = utssys (symlink in 4.2) */
	scall(3, readlink),		/*  58 = readlink */
	nohpuxboot(3, execve),		/*  59 = execve */
	scall(1, umask),		/*  60 = umask */
	scall(1, chroot),		/*  61 = chroot */
	scall(3, fcntl),		/*  62 = fcntl */
	scall(2, ulimit),		/*  63 = ulimit */
	bsd(1, getpagesize),		/*  64 = 4.2 getpagesize */
	bsd(5, mremap),			/*  65 = 4.2 mremap */
	nohpuxboot(0, vfork),		/*  66 = vfork */
	s300(0, read),			/*  67 = old vread */
	s300(0, write),			/*  68 = old vwrite */
	bsd(1, sbrk),			/*  69 = sbrk USE ENTRY #17 INSTEAD */
	bsd(1, sstk),			/*  70 = sstk */
	scall(6, smmap),		/*  71 = mmap */
	scall(0, nosys),		/*  72 = old vadvise */
	scall(2, munmap),		/*  73 = munmap */
	scall(3, mprotect),		/*  74 = mprotect */
	scall(3, madvise),		/*  75 = madvise */
	scall(1, vhangup),		/*  76 = vhangup */
#ifdef INSTALL
	scall(2, swapoff),		/*  77 = swapoff */
#else
	scall(0, nosys),		/*  77 = placeholder */
#endif
	bsd(3, mincore),		/*  78 = mincore */
	scall(2, getgroups),		/*  79 = getgroups */
	scall(2, setgroups),		/*  80 = setgroups */
	nohpuxboot(1, getpgrp2),	/*  81 = HP-UX getpgrp2 */
	nohpuxboot(2, setpgrp2),	/*  82 = HP-UX setpgrp2 */
	scall(3, setitimer),		/*  83 = setitimer */
	scall(3, wait3),		/*  84 = wait3 */
	scall(5, swapon),		/*  85 = swapon */
	scall(2, getitimer),		/*  86 = getitimer */
	scall(0, nosys),		/*  87 = 4.2 gethostname */
	scall(0, nosys),		/*  88 = 4.2 sethostname */
	bsd(0, getdtablesize),		/*  89 = getdtablesize */
	scall(2, dup2),			/*  90 = dup2 */
	bsd(2, getdopt),		/*  91 = getdopt */
	scall(2, fstat),		/*  92 = fstat */
	scall(5, select),		/*  93 = select */
	bsd(2, setdopt),		/*  94 = setdopt */
	scall(1, fsync),		/*  95 = fsync */
	scall(3, setpriority),		/*  96 = setpriority */
	scall(0, nosys),		/*  97 = compatibility socket */
	scall(0, nosys),		/*  98 = compatibility connect */
	scall(0, nosys),		/*  99 = compatibility accept */
	scall(2, getpriority),		/* 100 = getpriority */
	scall(0, nosys),		/* 101 = compatibility send */
	scall(0, nosys),		/* 102 = compatibility receive */
	scall(0, nosys),		/* 103 = old socketaddr */
	scall(0, nosys),		/* 104 = compatibility bind */
	scall(0, nosys),		/* 105 = compatibility setsockopt */
	scall(0, nosys),		/* 106 = compatibility listen */
	scall(0, nosys),		/* 107 = nosys */
	scall(3, sigvec),		/* 108 = sigvec */
	scall(1, sigblock),		/* 109 = sigblock */
	scall(1, sigsetmask),		/* 110 = sigsetmask */
	scall(1, sigpause),		/* 111 = sigpause */
	scall(2, sigstack),		/* 112 = sigstack */
	scall(0, nosys),		/* 113 = compatibility recvmsg */
	scall(0, nosys),		/* 114 = compatibility sendmsg */
	trace(2, vtrace),		/* 115 = vtrace */
	scall(2, gettimeofday),		/* 116 = gettimeofday */
	scall(2, getrusage),		/* 117 = getrusage */
	scall(0, nosys),		/* 118 = compatibility getsockopt */
	s300(3, hpib_io_stub),		/* 119 = hpib_io */
	scall(3, readv),		/* 120 = readv */
	scall(3, writev),		/* 121 = writev */
	scall(2, settimeofday),		/* 122 = settimeofday */
	scall(3, fchown),		/* 123 = fchown */
	scall(2, fchmod),		/* 124 = fchmod */
	scall(0, nosys),		/* 125 = compatibility recvfrom */
	scall(3, setresuid),		/* 126 = setresuid. was setreuid */
	scall(3, setresgid),		/* 127 = setresgid. was setregid */
	scall(2, rename),		/* 128 = 4.2 rename */
	scall(2, truncate),		/* 129 = truncate */
	scall(2, ftruncate),		/* 130 = ftruncate */
	bsd(2, flock),			/* 131 = flock */
	scall(1, sysconf),		/* 132 = sysconf */
	scall(0, nosys),		/* 133 = compatibility sendto */
	scall(0, nosys),		/* 134 = compatibility shutdown */
	scall(0, nosys),		/* 135 = compatibility socketpair */
	scall(2, mkdir),		/* 136 = mkdir */
	scall(1, rmdir),		/* 137 = rmdir */
	bsd(2, utimes),			/* 138 = utimes */
	s300(0, sigcleanup),		/* 139 = sigcleanup */
					/* (used internally on s800) */
	scall(2, setcore),		/* 140 = setcore */
	scall(0, nosys),		/* 141 = compatibility getpeername */
	scall(0, nosys),		/* 142 = 4.2 gethostid */
	scall(0, nosys),		/* 143 = 4.2 sethostid */
	scall(2, getrlimit),		/* 144 = 4.2 getrlimit */
	scall(2, setrlimit),		/* 145 = 4.2 setrlimit */
	bsd(2, killpg),			/* 146 = 4.2 killpg */
	scall(0, nosys),                /* 147 = nosys */
#ifdef QUOTA
	scall(4, quotactl),		/* 148 = quotactl */
#else
	scall(0, nosys),		/* 148 = nosys */
#endif
#ifdef __hp9000s800
	scall(0, get_sysinfo),		/* 149 = get_sysinfo */
#endif /* hp9000s800 */
#ifdef __hp9000s300
	scall(0, nosys),		/* 149 = nosys */
#endif /* __hp9000s300 */
	scall(0, nosys),		/* 150 = compatibility getsockname */
	scall(3, privgrp),		/* 151 = privgrp */
	scall(2, rtprio),		/* 152 = rtprio */
	scall(1, lock),			/* 153 = process locking */
	scall(0, nosys),		/* 154 = compat netioctl */
	scall(4, lockf),		/* 155 = lockf file locking */
	sema(3, semget),		/* 156 = semget */
	sema(4, compat(semctl)),	/* 157 = old semctl */
	sema(3, semop),			/* 158 = semop */
	mesg(2, msgget),		/* 159 = msgget */
	mesg(3, compat(msgctl)),	/* 160 = old msgctl */
	mesg(4, msgsnd),		/* 161 = msgsnd */
	mesg(5, msgrcv),		/* 162 = msgrcv */
	shmem(3, shmget),		/* 163 = shmget */
	shmem(3, compat(shmctl)),	/* 164 = old shmctl */
	shmem(3, shmat),		/* 165 = shmat */
	shmem(1, shmdt),		/* 166 = shmdt */
	scall(0, nosys),		/* 167 = was m68020_advise */
/*
 * The following are DUX specific. They are filled in at boot time
 * by the dskless_link routine if DUX is configured in.
 */
	dux(2, dux_notconfigured),	/* 168 = nsp_init */
	dux(3, cluster),		/* 169 = cluster */
	scall(4, mkrnod),		/* 170 = mkrnod	- CNODE_DEV */
	scall(0, nosys),		/* 171 = test */
	dux(0, dux_notconfigured),	/* 172 = unsp_open */
	scall(0, nosys),		/* 173 = used to be dstat */
	scall(3, getcontext),		/* 174 = getcontext */
	scall(1, setcontext),		/* 175 = setcontext	- unsupported */
	scall(0, nosys),		/* 176 = bigio		- FULLDUX */
	scall(0, nosys),		/* 177 = pipenode	- FULLDUX */
	scall(0, lsync),		/* 178 = lsync */
	scall(0, nosys),		/* 179 = ?? used to be getmachineid */
	scall(0, mysite),		/* 180 = mysite */
	dux(0, returnzero),		/* 181 = sitels */
	dux(0, returnzero),             /* 182 = swap_clients */
	scall(0, nosys),		/* 183 = rmtprocess - FULLDUX */
/* Reserved for future use */
	scall(0, nosys),		/* 184 = dskless_stats */

#ifdef __hp9000s300
	scall(0, nosys),		/* 185 = used for Northrop special */
	acls(3, setacl),		/* 186 = setacl */
	acls(3, fsetacl),		/* 187 = fsetacl */
	acls(3, getacl),		/* 188 = getacl */
	acls(3, fgetacl),		/* 189 = fgetacl */
	acls(6, getaccess),		/* 190 = getaccess */
	audit(0, getaudid),		/* 191 = getaudid */
	audit(1, setaudid),		/* 192 = setaudid */
	audit(0, getaudproc),		/* 193 = getaudproc */
	audit(1, setaudproc),		/* 194 = setaudproc */
	audit(2, getevent),		/* 195 = getevent */
	audit(2, setevent),		/* 196 = setevent */
	audit(1, audwrite),		/* 197 = audwrite */
	audit(1, audswitch),		/* 198 = audswitch */
	audit(4, audctl),		/* 199 = audctl */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
	scall(3, sigprocmask),		/* 185 = sigprocmask */
	scall(1, sigpending),		/* 186 = sigpending */
	scall(1, sigsuspend),		/* 187 = sigsuspend */
	scall(3, sigaction),		/* 188 = sigaction */
	scall(0, nosys),		/* 189 = nosys */
	scall(0, nosys),		/* 190 = nfs_svc if NFS configured */
	scall(0, nosys),		/* 191 = nfs_getfh if NFS configured */
	scall(2, getdomainname),	/* 192 = getdomainname */
	scall(0, nosys),		/* 193 = setdomainname if NFS
						 configured */
	scall(0, nosys),		/* 194 = async_daemon if NFS
						 configured */
	scall(4, getdirentries),	/* 195 = getdirentries */
	scall(2, statfs),		/* 196 = statfs*/
	scall(2, fstatfs),		/* 197 = fstatfs */
	scall(4, vfsmount),		/* 198 = vfsmount */
	scall(0, nosys),		/* 199 = nosys */
#endif /* hp9000s800 */

	scall(3, waitpid),		/* 200 = waitpid */
	scall(0, nosys),		/* 201 = was netunam */
	scall(0, nosys),		/* 202 = was netioctl for s800 */
	scall(0, nosys),		/* 203 = was ipccreate */
	scall(0, nosys),		/* 204 = was ipcname */
	scall(0, nosys),		/* 205 = was ipcnamerase */
	scall(0, nosys),		/* 206 = was ipclookup */
	scall(0, nosys),		/* 207 = was ipcsendto */
	scall(0, nosys),		/* 208 = was ipcrecvfrom */
	scall(0, nosys),		/* 209 = was ipcconnect */
	scall(0, nosys),		/* 210 = was ipcrecvcn */
	scall(0, nosys),		/* 211 = was ipcsend */
	scall(0, nosys),		/* 212 = was ipcrecv */
	scall(0, nosys),		/* 213 = was ipcgive */
	scall(0, nosys),		/* 214 = was ipcget */
	scall(0, nosys),		/* 215 = was ipccontrol */
	scall(0, nosys),		/* 216 = was ipcsendreq */
	scall(0, nosys),		/* 217 = was ipcrecvreq */
	scall(0, nosys),		/* 218 = was ipcsendreply */
	scall(0, nosys),		/* 219 = was ipcrecvreply */
	scall(0, nosys),		/* 220 = was ipcshutdown */
	scall(0, nosys),		/* 221 = was ipcdest */
	scall(0, nosys),		/* 222 = was ipcmuxconnect */
	scall(0, nosys),		/* 223 = was ipcmuxrecv */

#ifdef __hp9000s300
#ifdef	POSIX_SET_NOTRUNC
	scall(1, set_no_trunc),		/* 224 = set_no_trunc */
#else
	scall(0, nosys),		/* 224 = nosys */
#endif
	scall(2, pathconf),		/* 225 = pathconf */
	scall(2, fpathconf),		/* 226 = fpathconf */
	scall(0, nosys),		/* 227 = nosys */
	scall(0, nosys),		/* 228 = nosys */

/*
 * async_daemon, nfs_getfh, and nfs_svc have been replaced by
 * nosys, which will return EINVAL if NFS is not configured
 * into the kernel.  The user space routines that call these functions
 * can check the errno and print an appropriate message, rather than
 * dump core -- gmf.
 *
 * taken out of ifdef HPNFS for configurability -- gmf
 */
	scall(0, nosys),		/* 229 = async_daemon */
	scall(3, nosys),		/* 230 = nfs_fcntl */
	scall(4, getdirentries),	/* 231 = getdirentries */
	scall(2, getdomainname),	/* 232 = getdomainname */
	scall(2, nosys),		/* 233 = nfs_getfh */
	scall(4, vfsmount),		/* 234 = vfsmount */
	scall(1, nosys),		/* 235 = nfs_svc */
	scall(2, nosys),		/* 236 = setdomainname */
	scall(2, statfs),		/* 237 = statfs */
	scall(2, fstatfs),		/* 238 = fstatfs */
	scall(3, sigaction),		/* 239 = sigaction */
	scall(3, sigprocmask),		/* 240 = sigprocmask */
	scall(1, sigpending),		/* 241 = sigpending */
	scall(1, sigsuspend),		/* 242 = sigsuspend */
	scall(4, fsctl),		/* 243 = fsctl */
	scall(0, nosys),		/* 244 = nosys */
	scall(5, pstat),		/* 245 = pstat */
	scall(0, nosys),		/* 246 = nosys */
	scall(0, nosys),		/* 247 = nosys */
	scall(0, nosys),		/* 248 = nosys */
	scall(0, nosys),		/* 249 = nosys */
	scall(0, nosys),		/* 250 = nosys */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
	scall(3, sigsetreturn),		/* 224 = sigsetreturn */
	scall(2, sigsetstatemask),	/* 225 = sigsetstatemask */
#ifdef	BFA
	scall(2, bfactl),		/* 226 = bfactl */
#else
	scall(0, nosys),		/* 226 = nosys */
#endif
	scall(3, cs),			/* 227 = cs */
	scall(5, cds),			/* 228 = cds */
#ifdef S2POSIX_NO_TRUNC
	scall(1, set_no_trunc),		/* 229 = set_no_trunc */
#else
	scall(0, nosys),		/* 229 = nosys */
#endif
	scall(2, pathconf),		/* 230 = pathconf */
	scall(2, fpathconf),		/* 231 = fpathconf */
	scall(0, nosys),		/* 232 = nosys */
	scall(0, nosys),		/* 233 = nosys */
	scall(0, nosys),		/* 234 = nfs_fcntl - see nfs_init() */
	acls(3, getacl),		/* 235 = getacl */
	acls(3, fgetacl),		/* 236 = fgetacl */
	acls(3, setacl),		/* 237 = setacl */
	acls(3, fsetacl),		/* 238 = fsetacl */
	scall(5, pstat),		/* 239 = pstat */
	audit(0, getaudid),		/* 240 = getaudid */
	audit(1, setaudid),		/* 241 = setaudid */
	audit(0, getaudproc),		/* 242 = getaudproc */
	audit(1, setaudproc),		/* 243 = setaudproc */
	audit(2, getevent),		/* 244 = getevent */
	audit(2, setevent),		/* 245 = setevent */
	audit(1, audwrite),		/* 246 = audwrite */
	audit(1, audswitch),		/* 247 = audswitch */
	audit(4, audctl),		/* 248 = audctl */
	acls(6, getaccess),		/* 249 = getaccess */
	scall(4, fsctl),		/* 250 = fsctl */
#endif /* hp9000s800 */

	scall(0, nosys),		/* 251 = ? ulconnect */
	scall(0, nosys),		/* 252 = ? ulcontrol */
	scall(0, nosys),		/* 253 = ? ulcreate */
	scall(0, nosys),		/* 254 = ? uldest */
	scall(0, nosys),		/* 255 = ? ulrecv */
	scall(0, nosys),		/* 256 = ? ulrecvcn */
	scall(0, nosys),		/* 257 = ? ulsend */
	scall(0, nosys),		/* 258 = ? ulshutdown */
	scall(2, swapfs),		/* 259 = swapfs */
#ifdef FSS
	scall(3, fss_default),		/* 260 = fss */
#else
	scall(0, nosys),		/* 260 = nosys */
#endif
	/*
	 * These were the pstat system call numbers for 7.0; no longer in use.
	 */
	scall(0, nosys),		/* 261 = was 7.0 pstat */
	scall(0, nosys),		/* 262 = was 7.0 pstat */
	scall(0, nosys),		/* 263 = was 7.0 pstat */
	scall(0, nosys),		/* 264 = was 7.0 pstat */
	scall(0, nosys),		/* 265 = was 7.0 pstat */
	scall(0, nosys),		/* 266 = was 7.0 pstat */
#ifdef NSYNC
	scall(1, tsync),		/* 267 = tsync */
#else
	scall(0, nosys),		/* 267 = nosys */
#endif
	scall(0, getnumfds),		/* 268 = getnumfds */
/* these were under ifdef HPNSE */
	scall(3, poll),			/* 269 = poll */
	scall(0, nosys),		/* 270 = getmsg */
	scall(0, nosys),		/* 271 = putmsg */
/* endif */
	scall(1, fchdir),		/* 272 = fchdir */
#ifdef GETMOUNT
	scall(1, getmount_cnt),		/* 273 = getmount_cnt */
	scall(4, getmount_entry),	/* 274 = getmount_entry */
#else
	scall(0, nosys),		/* 273 = nosys */
	scall(0, nosys),		/* 274 = nosys */
#endif
	scall(0, nosys),		/* 275 = accept */
	scall(0, nosys),		/* 276 = bind */
	scall(0, nosys),		/* 277 = connect */
	scall(0, nosys),		/* 278 = getpeername */
	scall(0, nosys),		/* 279 = getsockname */
	scall(0, nosys),		/* 280 = getsockopt */
	scall(0, nosys),		/* 281 = listen */
	scall(0, nosys),		/* 282 = recv */
	scall(0, nosys),		/* 283 = recvfrom */
	scall(0, nosys),		/* 284 = recvmsg */
	scall(0, nosys),		/* 285 = send */
	scall(0, nosys),		/* 286 = sendmsg */
	scall(0, nosys),		/* 287 = sendto */
	scall(0, nosys),		/* 288 = setsockopt */
	scall(0, nosys),		/* 289 = shutdown */
	scall(0, nosys),		/* 290 = socket */
	scall(0, nosys),		/* 291 = socketpair */
	scall(2, proc_open),		/* 292 = proc_open DB_SENDRECV */
	scall(2, proc_close),		/* 293 = proc_close */
	scall(3, proc_send),		/* 294 = proc_send */
	scall(2, proc_recv),		/* 295 = proc_recv */
	scall(3, proc_sendrecv),	/* 296 = proc_sendrecv */
	scall(4, proc_syscall),		/* 297 = proc_syscall */
	scall(0, nosys),		/* 298 = ipccreate */
	scall(0, nosys),		/* 299 = ipcname */
	scall(0, nosys),		/* 300 = ipcnamerase */
	scall(0, nosys),		/* 301 = ipclookup */
	scall(0, nosys),		/* 302 = ipcselect */
	scall(0, nosys),		/* 303 = ipcconnect */
	scall(0, nosys),		/* 304 = ipcrecvcn */
	scall(0, nosys),		/* 305 = ipcsend */
	scall(0, nosys),		/* 306 = ipcrecv */
	scall(0, nosys),		/* 307 = ipcgetnodename */
	scall(0, nosys),		/* 308 = ipcsetnodename */
	scall(0, nosys),		/* 309 = ipccontrol */
	scall(0, nosys),		/* 310 = ipcshutdown */
	scall(0, nosys),		/* 311 = ipcdest */
        sema(4, semctl),                /* 312 = semctl */
        mesg(3, msgctl),                /* 313 = msgctl */
        shmem(3, shmctl),               /* 314 = shmctl */
	scall(3, mpctl),		/* 315 = mpctl */
	scall(0, nosys),		/* 316 = exportfs */
        scall(0, nosys),                /* 317 = getpmsg */
        scall(0, nosys),                /* 318 = putpmsg */
        scall(0, nosys),                /* 319 = strioctl */
	scall(3, msync),		/* 320 = msync */
	scall(2, msleep),               /* 321 = msleep */
	scall(1, mwakeup),		/* 322 = mwakeup */
	scall(2, msem_init),            /* 323 = msem_init */
	scall(1, msem_remove),          /* 324 = msem_remove */
	scall(2, adjtime),              /* 325 = adjtime */
	scall(2, lchmod)                /* 326 = lchmod */
};

int	nsysent = sizeof(sysent)/sizeof(sysent[0]);

/*
 * Syscall tracing.
 */

#if defined(__hp9000s300) && defined(_KERNEL)

/* make sure kernel & command are in sync                              */
/* bump whenever fields in struct sct (or their semantics) are changed */
int	syscalltrace_rev = SCT_REV;

/* Note - many of these declarations assume 32 bit ints. */
/*
 * ESC XXX NB that the two mask fields assume that there are only
 * 256 (max) system calls.  We are now at # 267. 6/13/89
 *			    We are now at # 291. 12/5/89
 */
struct sct syscalltrace = {
	0,				/* flags        */
	0,				/* pid          */
	0,				/* pgrp         */
	0,				/* tty          */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* syscall_mask */
	-1, -1, -1, -1, -1, -1, -1, -1,	/* errno_mask   */
	'\0'				/* command      */
};

#endif /* __hp9000s300 && _KERNEL */
