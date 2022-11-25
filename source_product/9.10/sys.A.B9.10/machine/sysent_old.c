/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/sysent_old.c,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:08:24 $
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
 * Sysent table for object compatibility with 2.x s200 systems
 * and Integral PC (aka Pisces) systems.
 */

#include "../h/param.h"
#include "../h/systm.h"

int	oalarm();
int	chdir();
int	chmod();
int	chown();
int	chroot();
int	close();
int	creat();
int	dup();
int	execv();
int	execve();
int	fcntl();
int	fork();
int	ofstat();
int	getgid();
int	getpid();
int	getuid();
int	otime();
int	ogtty();
int	ioctl();
int	kill();
int	link();
int	mknod();
int	onice();
int	nosys();
int	open();
int	opause();
int	pipe();
int	profil();
int	ptrace();
int	oread();
int	rexit();
int	saccess();
int	obreak();
int	lseek();
int	osetgid();
int	osetpgrp();
int	osetuid();
int	smount();
int	ossig();
int	ostat();
int	ostime();
int	ostty();
int	umount();
int	sync();
int	sysacct();
int	otimes();
int	ulimit();
int	umask();
int	unlink();
int	outime();
int	utssys();
int	wait();
int	write();

#define	scall(n, name)		n, name, "name"

struct sysent compat_sysent[] =
{
	scall(0, nosys),		/*  0 = indir */
	scall(1, rexit),		/*  1 = exit */
	scall(0, fork),			/*  2 = fork */
	scall(3, oread),		/*  3 = read */
	scall(3, write),		/*  4 = write */
	scall(3, open),			/*  5 = open */
	scall(1, close),		/*  6 = close */
	scall(1, wait),			/*  7 = wait */
	scall(2, creat),		/*  8 = creat */
	scall(2, link),			/*  9 = link */
	scall(1, unlink),		/* 10 = unlink */
	scall(2, execv),		/* 11 = exec */
	scall(1, chdir),		/* 12 = chdir */
	scall(1, otime),		/* 13 = time */
	scall(3, mknod),		/* 14 = mknod */
	scall(2, chmod),		/* 15 = chmod */
	scall(3, chown),		/* 16 = chown; now 3 args */
	scall(1, obreak),		/* 17 = break */
	scall(2, ostat),		/* 18 = stat */
	scall(3, lseek),		/* 19 = seek */
	scall(0, getpid),		/* 20 = getpid */
	scall(3, smount),		/* 21 = mount */
	scall(1, umount),		/* 22 = umount */
	scall(1, osetuid),		/* 23 = setuid */
	scall(0, getuid),		/* 24 = getuid - ignore old parameter */
	scall(1, ostime),		/* 25 = stime */
	scall(4, ptrace),		/* 26 = ptrace */
	scall(1, oalarm),		/* 27 = alarm */
	scall(2, ofstat),		/* 28 = fstat */
	scall(0, opause),		/* 29 = pause */
	scall(2, outime),		/* 30 = utime */
	scall(2, ostty),		/* 31 = stty */
	scall(2, ogtty),		/* 32 = gtty */
	scall(2, saccess),		/* 33 = access */
	scall(1, onice),		/* 34 = nice */
	scall(0, nosys),		/* 35 = sleep; inoperative */
	scall(0, sync),			/* 36 = sync */
	scall(2, kill),			/* 37 = kill */
	scall(0, nosys),		/* 38 = CSW obsolete */
	scall(1, osetpgrp),		/* 39 = setpgrp */
	scall(0, nosys),		/* 40 = tell - obsolete */
	scall(1, dup),			/* 41 = dup */
	scall(0, pipe),			/* 42 = pipe */
	scall(1, otimes),		/* 43 = times */
	scall(4, profil),		/* 44 = prof */
	scall(0, nosys),		/* 45 = unused */
	scall(1, osetgid),		/* 46 = setgid */
	scall(0, getgid),		/* 47 = getgid - ignore old parameter */
	scall(2, ossig),		/* 48 = sig */
	scall(0, nosys),		/* 49 = reserved for USG */
	scall(0, nosys),		/* 50 = reserved for local use */
	scall(1, sysacct),		/* 51 = turn acct off/on */
	scall(0, nosys),		/* 52 */
	scall(0, nosys),		/* 53 */
	scall(3, ioctl),		/* 54 = ioctl */
	scall(0, nosys),		/* 55 = x */
	scall(0, nosys),		/* 56 = x */
	scall(3, utssys),		/* 57 = utssys */
	scall(0, nosys),		/* 58 = reserved for USG */
	scall(3, execve),		/* 59 = exece */
	scall(1, umask),		/* 60 = umask */
	scall(1, chroot),		/* 61 = chroot */
	scall(3, fcntl),		/* 62 = fcntl */
	scall(2, ulimit),		/* 63 = ulimit */
#ifdef	notdef	/* the rest of the IPC sysent - not supported */
	/*
	 * added for FSD s200 compatibility
	 */
	scall(4, shmem),		/* 64 = shmem */
	scall(1, reboot),		/* 65 = reboot */
	/*
	 * added for HP-UX/RT compatibility 
	 */
	scall(2, getitimer),		/* 66 = getitimer */
	scall(2, setitimer),		/* 67 = setitimer */
	scall(2, gettimeofday),	 	/* 68 = gettimeofday */
	scall(2, settimeofday), 	/* 69 = settimeofday */
	scall(1, proclock),		/* 70 = plock */
	scall(2, rtprio), 		/* 71 = rtprio */
	/*
	 * added for PISCES
	 */
	scall(0, vfork),		/* 72 = vfork */
	scall(2, bstat),		/* 73  32-bit stat entry */
	scall(2, bfstat),		/* 74  32-bit fstat entry */
	/*
	 * more HP-UX/RT (and etcetera)  entry points
	 */
	scall(1, fsync),		/* 75 fsync- sync one file */
	scall(3, shmctl),		/* 76 shared memory control */
	scall(3, shmget),		/* 77 shared memory stats get */
	scall(3, shmat),		/* 78 attach to shared mem seg */
	scall(1, sigblock),		/* 79 block signals */
	scall(1, sigpause),		/* 80 atomic signal release */
	scall(1, sigsetmask),		/* 81 set signal enable mask */
	scall(1, sigstack),		/* 82 set/examine signal stack */
	scall(3, sigvec),		/* 83 set signal handler vector */
	scall(3, fa_ioctl),		/* 84 fast alpha */
	scall(1, shmdt),		/* 85 detach shared mem seg */
#endif	notdef
};

int	ncompat_sysent = sizeof(compat_sysent)/sizeof(struct sysent);
