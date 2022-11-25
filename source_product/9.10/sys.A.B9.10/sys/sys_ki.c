/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sys_ki.c,v $
 * $Revision: 1.13.83.4 $        $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 13:33:27 $
 */
/* HPUX_ID: @(#)sys_ki.c        54.6            88/12/21 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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

/* Kernel Instrumentation (KI) Interface code */

#ifdef  FSD_KI

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/syscall.h"
#include "../h/signal.h"
#include "../h/sysmacros.h"
#include "../h/time.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../ufs/inode.h"
#include "../h/tty.h"
#include "../h/dk.h"
#include "../dux/dm.h"
#include "../h/vmmeter.h"
#include "../h/map.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/rnode.h"
#include "../rpc/svc.h"
#include "../h/mp.h"
#include "../h/kernel.h"

#include "../h/spinlock.h"
#include "../h/ki_calls.h"
#include "../h/ki.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"

#include "../machine/reg.h"

#include "../h/sem_alpha.h"
#include "../h/sem_beta.h"
#include "../h/sem_sync.h"
#include "../h/pstat.h"

#ifdef uniprocessor

/* If compiled UP, spinlock's never necessary */

#define	KI_SPINLOCK(lock)
#define	KI_SPINUNLOCK(lock)

/* Just disable interrupts for protection in UP */

#define KI_CRIT_REPLACEMENT(lock,var) DISABLE_INT(var)
#define KI_UNCRIT_REPLACEMENT(lock,var) ENABLE_INT(var)

#else /* ! uniprocessor */

/* For locking the ki_cf structure and KI trace buffers */
extern lock_t *ki_proc_lock[KI_MAX_PROCS];

/* Must do spinlocks in multi-processor configuration */

#define	KI_SPINLOCK(lock)	spinlock(lock)
#define	KI_SPINUNLOCK(lock)	spinunlock(lock)

/* Protection requires spinlocks when executing MP */
/* but, not necessary if just running UP */

#define KI_CRIT_REPLACEMENT(lock,var) \
if (uniprocessor) { DISABLE_INT(var);} \
else { KI_SPINLOCK(lock); }

#define KI_UNCRIT_REPLACEMENT(lock,var)	\
if (uniprocessor) { ENABLE_INT(var); } \
else { KI_SPINUNLOCK(lock); }

#endif /* ! uniprocessor */

#ifdef  __hp9000s300

#define _MFCTL(CR_IT, VAR)      (VAR = mfctl_CR_IT())
extern	u_int	read_adjusted_itmr();
extern	u_int	mfctl_CR_IT();
#define	runningprocs 1

#else	/* ! __hp9000s300 */

extern int runningprocs;
extern	u_int read_adjusted_itmr();

#endif  /* __hp9000s300 */


/************************************************************************/
/* Class 0 table                                                        */
/************************************************************************/

struct ki_config ki_cf = {
        KI_VERSION
};


        /* pointers to routines to handle system call traces            */
/* Allocate the procedure pointers for system call traces               */
/* P_rocedure V_ariable returning a S_truct _kd_syscall P_ointer        */
typedef struct  kd_syscall *(*PV_SkdP)();       /* pointer are fun in C */

/* verious routines to handle different system calls                    */
struct  kd_syscall *ki_generic();       /* all calls                    */
struct  kd_syscall *ki_exec();          /* exec                         */
struct  kd_syscall *ki_fork();          /* fork and vfork               */
struct  kd_syscall *ki_open();          /* open                         */
struct  kd_syscall *ki_fd();            /* call passing file descripter */
struct  kd_syscall *ki_pipe();          /* call with 2 file descripters */
struct  kd_syscall *ki_socketpair();    /* call with 2 file descripters */
struct  kd_syscall *ki_1string();       /* call passing one string      */
struct  kd_syscall *ki_2string();       /* call passing two strings     */
struct  kd_syscall *ki_mmap();		/* mmap                         */

/*
 * This contains the syscall trace current and initialize values, and
 * has to be reviewed each time when ../h/syscall.h modified.
 */
PV_SkdP ki_traceinit[KI_MAXSYSCALLS] = {
    ki_generic,	    /* 0 = indir */
    ki_generic,	    /* 1 = exit -- this one never occurs */
    ki_fork,	    /* 2 = fork */
    ki_fd,	    /* 3 = read */
    ki_fd,	    /* 4 = write */
    ki_open,	    /* 5 = open */
    ki_generic,	    /* 6 = close */
    ki_generic,	    /* 7 = wait */
    ki_open,	    /* 8 = creat */
    ki_2string,	    /* 9 = link */
    ki_1string,	    /* 10 = unlink */
    ki_exec,	    /* 11 = execv */
    ki_1string,	    /* 12 = chdir */
    ki_generic,	    /* 13 = time */
    ki_1string,	    /* 14 = mknod */
    ki_1string,	    /* 15 = chmod */
    ki_1string,	    /* 16 = chown; now 3 args */
    ki_generic,	    /* 17 = old break */
#ifdef  __hp9000s300
    ki_1string,	    /* 18 = stat */
#else   /* __hp9000s300 */
    ki_generic,	    /* 18 = nosys */
#endif  /* __hp9000s300 */
    ki_fd,	    /* 19 = lseek */
    ki_generic,	    /* 20 = getpid */
    ki_2string,	    /* 21 = mount */
    ki_1string,	    /* 22 = umount */
    ki_generic,	    /* 23 = old setuid */
    ki_generic,	    /* 24 = getuid */
    ki_generic,	    /* 25 = old stime */
    ki_generic,	    /* 26 = ptrace */
    ki_generic,	    /* 27 = old alarm */
#ifdef  __hp9000s300
    ki_fd,	    /* 28 = old fstat */
#else   /* __hp9000s300 */
    ki_generic,	    /* 28 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 29 = opause */
    ki_1string,	    /* 30 = old utime */
    ki_fd,	    /* 31 = old stty */
    ki_fd,	    /* 32 = old gtty */
    ki_1string,	    /* 33 = access */
    ki_generic,	    /* 34 = old nice */
    ki_generic,	    /* 35 = old ftime */
    ki_generic,	    /* 36 = sync */
    ki_generic,	    /* 37 = kill */
    ki_1string,	    /* 38 = stat */
    ki_generic,	    /* 39 = old setpgrp */
    ki_1string,	    /* 40 = lstat */
    ki_fd,	    /* 41 = dup */
    ki_pipe,	    /* 42 = pipe */
    ki_generic,	    /* 43 = old times */
    ki_generic,	    /* 44 = profil */
    ki_generic,	    /* 45 = ki_syscall */
    ki_generic,	    /* 46 = old setgid */
    ki_generic,	    /* 47 = getgid */
#ifdef  __hp9000s300
    ki_generic,	    /* 48 = old sig */
#else   /* __hp9000s300 */
    ki_generic,	    /* 48 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 49 = nosys */
    ki_generic,	    /* 50 = nosys */
    ki_1string,	    /* 51 = turn acct off/on */
    ki_generic,	    /* 52 = nosys */
    ki_generic,	    /* 53 = nosys */
    ki_fd,	    /* 54 = ioctl */
    ki_generic,	    /* 55 = reboot */
    ki_2string,	    /* 56 = symlink */
    ki_generic,	    /* 57 = utssys */
    ki_1string,	    /* 58 = readlink */
    ki_exec,	    /* 59 = execve */
    ki_generic,	    /* 60 = umask */
    ki_1string,	    /* 61 = chroot */
    ki_fd,	    /* 62 = fcntl */
    ki_generic,	    /* 63 = ulimit */
#ifdef  __hp9000s300
    ki_generic,	    /* 64 = getpagesize */
    ki_generic,	    /* 65 = mremap */
    ki_fork,	    /* 66 = vfork */
    ki_fd,	    /* 67 = old vread */
    ki_fd,	    /* 68 = old vwrite */
    ki_generic,	    /* 69 = sbrk */
    ki_generic,	    /* 70 = sstk */
    ki_mmap, 	    /* 71 = mmap */   
    ki_generic,	    /* 72 = nosys */
    ki_generic,	    /* 73 = munmap */
    ki_generic,	    /* 74 = mprotect */
    ki_generic,	    /* 75 = madvise */
    ki_generic,	    /* 76 = vhangup */
    ki_generic,	    /* 77 = nosys */
    ki_generic,	    /* 78 = mincore */
#else   /* __hp9000s300 */
    ki_generic,	    /* 64 = nosys */
    ki_generic,	    /* 65 = nosys */
    ki_fork,	    /* 66 = vfork */
    ki_generic,	    /* 67 = nosys */
    ki_generic,	    /* 68 = nosys */
    ki_generic,	    /* 69 = nosys */
    ki_generic,	    /* 70 = nosys */
    ki_mmap,	    /* 71 = mmap */
    ki_generic,	    /* 72 = nosys */
    ki_generic,	    /* 73 = nosys */
    ki_generic,	    /* 74 = nosys */
    ki_generic,	    /* 75 = nosys */
    ki_generic,	    /* 76 = nosys */
    ki_generic,	    /* 77 = nosys */
    ki_generic,	    /* 78 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 79 = getgroups */
    ki_generic,	    /* 80 = setgroups */
    ki_generic,	    /* 81 = HP-UX getpgrp2 */
    ki_generic,	    /* 82 = HP-UX setpgrp2 */
    ki_generic,	    /* 83 = setitimer */
    ki_generic,	    /* 84 = HP-UX's new wait3 */
    ki_1string,	    /* 85 = swapon */
    ki_generic,	    /* 86 = getitimer */
    ki_generic,	    /* 87 = nosys */
    ki_generic,	    /* 88 = nosys */
#ifdef  __hp9000s300
    ki_generic,	    /* 89 = getdtablesize */
    ki_fd,	    /* 90 = dup2 */
    ki_generic,	    /* 91 = getdopt */
    ki_fd,	    /* 92 = fstat */
    ki_generic,	    /* 93 = select */
    ki_generic,	    /* 94 = setdopt */
    ki_fd,	    /* 95 = fsync */
    ki_generic,	    /* 96 = setpriority */
    ki_fd,	    /* 97 = socket */
    ki_fd,	    /* 98 = connect */
    ki_fd,	    /* 99 = accept */
    ki_generic,	    /* 100 = getpriority */
    ki_fd,	    /* 101 = send */
    ki_fd,	    /* 102 = recv */
    ki_generic,	    /* 103 = socketaddr */
#else   /* __hp9000s300 */
    ki_generic,	    /* 89 = nosys */
    ki_fd,	    /* 90 = dup2 */
    ki_generic,	    /* 91 = nosys */
    ki_fd,	    /* 92 = fstat */
    ki_generic,	    /* 93 = select */
    ki_generic,	    /* 94 = nosys */
    ki_fd,	    /* 95 = fsync */
    ki_generic,	    /* 96 = nosys */
    ki_fd,	    /* 97 = socket */
    ki_fd,	    /* 98 = connect */
    ki_fd,	    /* 99 = accept */
    ki_generic,	    /* 100 = nosys */
    ki_fd,	    /* 101 = send */
    ki_fd,	    /* 102 = recv */
    ki_generic,	    /* 103 = nosys */
#endif  /* __hp9000s300 */
    ki_fd,	    /* 104 = bind */
    ki_fd,	    /* 105 = setsockopt */
    ki_fd,	    /* 106 = listen */
    ki_generic,	    /* 107 = nosys */
    ki_generic,	    /* 108 = sigvec */
    ki_generic,	    /* 109 = sigblock */
    ki_generic,	    /* 110 = sigsetmask */
    ki_generic,	    /* 111 = sigpause */
    ki_generic,	    /* 112 = sigstack */
    ki_generic,	    /* 113 = recvmsg */
    ki_generic,	    /* 114 = sendmsg */
    ki_generic,	    /* 115 = nosys */
    ki_generic,	    /* 116 = gettimeofday */
#ifdef  __hp9000s300
    ki_generic,	    /* 117 = getrusage */
    ki_generic,	    /* 118 = getsockopt */
    ki_generic,	    /* 119 = hpib_io */
#else   /* __hp9000s300 */
    ki_generic,	    /* 117 = nosys */
    ki_generic,	    /* 118 = getsockopt */
    ki_generic,	    /* 119 = nosys */
#endif  /* __hp9000s300 */
    ki_fd,	    /* 120 = readv */
    ki_fd,	    /* 121 = writev */
    ki_generic,	    /* 122 = settimeofday */
    ki_fd,	    /* 123 = fchown */
    ki_fd,	    /* 124 = fchmod */
    ki_fd,	    /* 125 = recvfrom */
    ki_generic,	    /* 126 = setreuid */
    ki_generic,	    /* 127 = setregid */
    ki_2string,	    /* 128 = rename */
    ki_1string,	    /* 129 = truncate */
    ki_fd,	    /* 130 = ftruncate */
#ifdef  __hp9000s300
    ki_fd,	    /* 131 = flock */
#else   /* __hp9000s300 */
    ki_generic,	    /* 131 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 132 = sysconf */
    ki_fd,	    /* 133 = sendto */
    ki_generic,	    /* 134 = shutdown */
    ki_socketpair,  /* 135 = socketpair */
    ki_1string,	    /* 136 = mkdir */
    ki_1string,	    /* 137 = rmdir */
#ifdef  __hp9000s300
    ki_1string,	    /* 138 = utimes */
#else   /* __hp9000s300 */
    ki_generic,	    /* 138 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 139 = sigcleanup */
    ki_generic,	    /* 140 = nosys */
    ki_generic,	    /* 141 = getpeername */
    ki_generic,	    /* 142 = gethostid */
    ki_generic,	    /* 143 = sethostid */
#ifdef  __hp9000s300
    ki_generic,	    /* 144 = getrlimit */
    ki_generic,	    /* 145 = setrlimit */
    ki_generic,	    /* 146 = killpg */
    ki_generic,	    /* 147 = nosys */
    ki_generic,	    /* 148 = quotactl */
    ki_generic,	    /* 149 = nosys */
#else   /* __hp9000s300 */
    ki_generic,	    /* 144 = nosys */
    ki_generic,	    /* 145 = nosys */
    ki_generic,	    /* 146 = nosys */
    ki_generic,	    /* 147 = nosys */
    ki_generic,	    /* 148 = quotactl */
    ki_generic,	    /* 149 = nosys */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 150 = getsockname */
    ki_generic,	    /* 151 = privgrp */
    ki_generic,	    /* 152 = rtprio */
    ki_generic,	    /* 153 = plock */
    ki_generic,	    /* 154 = netioctl */
    ki_fd,	    /* 155 = lockf */
    ki_generic,	    /* 156 = semget */
    ki_generic,	    /* 157 = semctl */
    ki_generic,	    /* 158 = semop */
    ki_generic,	    /* 159 = msgget */
    ki_generic,	    /* 160 = msgctl */
    ki_generic,	    /* 161 = msgsnd */
    ki_generic,	    /* 162 = msgrcv */
    ki_generic,	    /* 163 = shmget */
    ki_generic,	    /* 164 = shmctl */
    ki_generic,	    /* 165 = shmat */
    ki_generic,	    /* 166 = shmdt */
    ki_generic,	    /* 167 = nosys */
    ki_generic,	    /* 168 = nsp_init */
    ki_generic,	    /* 169 = cluster */
    ki_1string,	    /* 170 = mkrnod */
#ifdef  __hp9000s300
    ki_generic,	    /* 171 = test */
#else   /* __hp9000s300 */
    ki_generic,	    /* 171 = nosys */
#endif  /* __hp9000s300 */
    ki_fd,	    /* 172 = unsp_open */
    ki_generic,	    /* 173 = nosys (used to be dstat) */
    ki_generic,	    /* 174 = getcontext */
    ki_generic,	    /* 175 = setcontext */
    ki_fd,	    /* 176 = bigio */
    ki_generic,	    /* 177 = pipenode */
    ki_generic,	    /* 178 = lsync */
    ki_generic,	    /* 179 = nosys (was getmachineid) */
    ki_generic,	    /* 180 = cnodeid */
    ki_generic,	    /* 181 = cnodes */
    ki_generic,	    /* 182 = nosys */
    ki_generic,	    /* 183 = rmtprocess */
    ki_generic,	    /* 184 = dskless_stats */
#ifdef  __hp9000s300
    ki_generic,	    /* 185 = nosys */
    ki_1string,	    /* 186 = setacl */
    ki_fd,	    /* 187 = fsetacl */
    ki_1string,	    /* 188 = getacl */
    ki_fd,	    /* 189 = fgetacl */
    ki_1string,	    /* 190 = getaccess */
    ki_generic,	    /* 191 = getaudid */
    ki_generic,	    /* 192 = setaudid */
    ki_generic,	    /* 193 = getaudproc */
    ki_generic,	    /* 194 = setaudproc */
    ki_generic,	    /* 195 = getevent */
    ki_generic,	    /* 196 = setevent */
    ki_generic,	    /* 197 = audwrite */
    ki_generic,	    /* 198 = audswitch */
    ki_generic,	    /* 199 = audctl */
    ki_generic,	    /* 200 = waitpid */
    ki_generic,	    /* 201 = nosys */
    ki_generic,	    /* 202 = nosys */
    ki_generic,	    /* 203 = nosys */
    ki_generic,	    /* 204 = nosys */
    ki_generic,	    /* 205 = nosys */
    ki_generic,	    /* 206 = nosys */
    ki_generic,	    /* 207 = nosys */
    ki_generic,	    /* 208 = nosys */
    ki_generic,	    /* 209 = ipconnect */
    ki_generic,	    /* 210 = nosys */
    ki_generic,	    /* 211 = nosys */
    ki_generic,	    /* 212 = nosys */
    ki_generic,	    /* 213 = nosys */
    ki_generic,	    /* 214 = nosys */
    ki_generic,	    /* 215 = nosys */
    ki_generic,	    /* 216 = nosys */
    ki_generic,	    /* 217 = nosys */
    ki_generic,	    /* 218 = nosys */
    ki_generic,	    /* 219 = nosys */
    ki_generic,	    /* 220 = nosys */
    ki_generic,	    /* 221 = nosys */
    ki_generic,	    /* 222 = nosys */
    ki_generic,	    /* 223 = nosys */
    ki_generic,	    /* 224 = nosys */
    ki_generic,	    /* 225 = nosys */
    ki_generic,	    /* 226 = nosys */
    ki_generic,	    /* 227 = nosys */
    ki_generic,	    /* 228 = nosys */
    ki_generic,	    /* 229 = async_daemon */
    ki_generic,	    /* 230 = config_status */
    ki_generic,	    /* 231 = getdirentries */
    ki_generic,	    /* 232 = getdomainname */
    ki_generic,	    /* 233 = nfs_getfh */
    ki_generic,	    /* 234 = vfsmount */
    ki_fd,	    /* 235 = nfs_svc */
    ki_generic,	    /* 236 = setdomeanname */
    ki_1string,	    /* 237 = statfs */
    ki_fd,	    /* 238 = fstatfs */
    ki_generic,	    /* 239 = */
    ki_generic,	    /* 240 = */
    ki_generic,	    /* 241 = */
    ki_generic,	    /* 242 = */
    ki_generic,	    /* 243 = */
    ki_generic,	    /* 244 = */
    ki_generic,	    /* 245 = */
    ki_generic,	    /* 246 = */
    ki_generic,	    /* 247 = */
    ki_generic,	    /* 248 = */
    ki_generic,	    /* 249 = */
    ki_generic,	    /* 250 = */
    ki_generic,	    /* 251 = */
    ki_generic,	    /* 252 = */
    ki_generic,	    /* 253 = */
    ki_generic,	    /* 254 = */
    ki_generic,	    /* 255 = */
    ki_generic,	    /* 256 = */
    ki_generic,	    /* 257 = */
    ki_generic,	    /* 258 = */
    ki_1string,	    /* 259 = swapfs */
    ki_generic,	    /* 260 = */
#else   /* __hp9000s300 */
    ki_generic,	    /* 185 = sigprocmask */
    ki_generic,	    /* 186 = sigpending */
    ki_generic,	    /* 187 = sigsuspend */
    ki_generic,	    /* 188 = sigaction */
    ki_generic,	    /* 189 = nosys */
    ki_fd,	    /* 190 = nfs_svc */
    ki_generic,	    /* 191 = nfs_getfh */
    ki_generic,	    /* 192 = getdomainname */
    ki_generic,	    /* 193 = setdomainname */
    ki_generic,	    /* 194 = async_daemon */
    ki_generic,	    /* 195 = getdirentries */
    ki_1string,	    /* 196 = statfs */
    ki_fd,	    /* 197 = fstatfs */
    ki_generic,	    /* 198 = vfsmount */
    ki_generic,	    /* 199 = nosys */
    ki_generic,	    /* 200 = nosys */
    ki_generic,	    /* 201 = netunam */
    ki_generic,	    /* 202 = netioctl */
    ki_generic,	    /* 203 = ipccreate */
    ki_generic,	    /* 204 = ipcname */
    ki_generic,	    /* 205 = ipcnamerase */
    ki_generic,	    /* 206 = ipclookup */
    ki_generic,	    /* 207 = ipcsendto */
    ki_generic,	    /* 208 = ipcrecvfrom */
    ki_generic,	    /* 209 = ipcconnect */
    ki_generic,	    /* 210 = ipcrecvcn */
    ki_generic,	    /* 211 = ipcsend */
    ki_generic,	    /* 212 = ipcrecv */
    ki_generic,	    /* 213 = ipcgive */
    ki_generic,	    /* 214 = ipcget */
    ki_generic,	    /* 215 = ipccontrol */
    ki_generic,	    /* 216 = ipcsendreq */
    ki_generic,	    /* 217 = ipcrecvreq */
    ki_generic,	    /* 218 = ipcsendreply */
    ki_generic,	    /* 219 = ipcrecvreply */
    ki_generic,	    /* 220 = ipcshutdown */
    ki_generic,	    /* 221 = ipcdest */
    ki_generic,	    /* 222 = ipcmuxconnect */
    ki_generic,	    /* 223 = ipcmuxrecv */
    ki_generic,	    /* 224 = sigsetreturn */
    ki_generic,	    /* 225 = sigsetstatemask */
    ki_generic,	    /* 226 = bfactl */
    ki_generic,	    /* 227 = cs */
    ki_generic,	    /* 228 = cds */
    ki_generic,	    /* 229 = set_no_trunc */
    ki_generic,	    /* 230 = pathconf */
    ki_generic,	    /* 231 = fpathconf */
    ki_generic,	    /* 232 = nosys */
    ki_generic,	    /* 233 = nosys */
    ki_generic,	    /* 234 = nfs_fcntl */
    ki_1string,	    /* 235 = getacl */
    ki_fd,	    /* 236 = fgetacl */
    ki_1string,	    /* 237 = setacl */
    ki_fd,	    /* 238 = fsetacl */
    ki_generic,	    /* 239 = pstat */
    ki_generic,	    /* 240 = getaudid */
    ki_generic,	    /* 241 = setaudid */
    ki_generic,	    /* 242 = getaudproc */
    ki_generic,	    /* 243 = setaudproc */
    ki_generic,	    /* 244 = getevent */
    ki_generic,	    /* 245 = setevent */
    ki_generic,	    /* 246 = audwrite */
    ki_generic,	    /* 247 = audswitch */
    ki_generic,	    /* 248 = audctl */
    ki_1string,	    /* 249 = getaccess */
    ki_generic,	    /* 250 = nosys */
    ki_generic,	    /* 251 = nosys */
    ki_generic,	    /* 252 = nosys */
    ki_generic,	    /* 253 = nosys */
    ki_generic,	    /* 254 = nosys */
    ki_generic,	    /* 255 = nosys */
    ki_generic,	    /* 256 = ulrecvcn */
    ki_generic,	    /* 257 = ulsend */
    ki_generic,	    /* 258 = ulshutdown */
    ki_1string,	    /* 259 = swapfs */
    ki_generic,	    /* 260 = fss */
#endif  /* __hp9000s300 */
    ki_generic,	    /* 261 = pstat_static_sys */
    ki_generic,	    /* 262 = pstat_dynamic_sys */
    ki_generic,	    /* 263 = pstat_count */
    ki_generic,	    /* 264 = pstat_pidlist */
    ki_generic,	    /* 265 = pstat_set_command */
    ki_generic,	    /* 266 = pstat_status */
    ki_generic,	    /* 267 = tsync */
    ki_generic,	    /* 268 = getnumfds */
#ifdef  __hp9000s800
    ki_generic,	    /* 269 = poll */
    ki_generic,	    /* 270 = getmsg */
    ki_generic,	    /* 271 = putmsg */
#else   /* __hp9000s800 */
    ki_generic,	    /* 269 = nosys */
    ki_generic,	    /* 270 = nosys */
    ki_generic,	    /* 271 = nosys */
#endif  /* __hp9000s800 */
    ki_generic,	    /* 272 = nosys */
    ki_generic,	    /* 273 = nosys */
    ki_generic,	    /* 274 = nosys */
    ki_generic,	    /* 275 = nosys */
    ki_fd,	    /* 276 = bind */
    ki_fd,	    /* 277 = connect */
    ki_fd,	    /* 278 = getpeername */
    ki_fd,	    /* 279 = getsockname */
    ki_fd,	    /* 280 = getsockopt */
    ki_fd,	    /* 281 = listen */
    ki_generic,	    /* 282 = nosys */
    ki_generic,	    /* 283 = nosys */
    ki_generic,	    /* 284 = nosys */
    ki_generic,	    /* 285 = nosys */
    ki_generic,	    /* 286 = nosys */
    ki_generic,	    /* 287 = nosys */
    ki_fd,	    /* 288 = setsockopt */
    ki_fd,	    /* 289 = shutdown */
    ki_generic,	    /* 290 = nosys */
    ki_socketpair,  /* 291 = socketpair */
    ki_generic,	    /* 292 = nosys */
    ki_generic,	    /* 293 = nosys */
    ki_generic,	    /* 294 = nosys */
    ki_generic,	    /* 295 = nosys */
    ki_generic,	    /* 296 = nosys */
    ki_generic,	    /* 297 = nosys */
    ki_generic,	    /* 298 = nosys */
    ki_fd,	    /* 299 = ipcname */
    ki_generic,	    /* 300 = nosys */
    ki_generic,	    /* 301 = nosys */
    ki_generic,	    /* 302 = nosys */
    ki_generic,	    /* 303 = ipcconnect */
    ki_fd,	    /* 304 = ipcrecvcn */
    ki_fd,	    /* 305 = ipcsend */
    ki_fd,	    /* 306 = ipcrecv */
    ki_generic,	    /* 307 = nosys */
    ki_generic,	    /* 308 = nosys */
    ki_fd,	    /* 309 = ipccontrol */
    ki_fd,	    /* 310 = ipcshutdown */
    ki_generic,	    /* 311 = nosys */
    ki_generic,	    /* 312 = nosys */
    ki_generic,	    /* 313 = nosys */
    ki_generic,	    /* 314 = nosys */
    ki_generic,	    /* 315 = nosys */
    ki_generic,	    /* 316 = nosys */
    ki_generic,	    /* 317 = nosys */
    ki_generic,	    /* 318 = nosys */
    ki_generic,	    /* 319 = nosys */
    ki_generic,	    /* 320 = nosys */
    ki_generic,	    /* 321 = nosys */
    ki_generic,	    /* 322 = nosys */
    ki_generic,	    /* 323 = nosys */
    ki_generic,	    /* 324 = nosys */
    ki_generic,	    /* 325 = nosys */
    ki_generic,	    /* 326 = nosys */
    ki_generic,	    /* 327 = nosys */
    ki_generic,	    /* 328 = nosys */
    ki_generic,	    /* 329 = nosys */
    ki_generic,	    /* 330 = nosys */
    ki_generic,	    /* 331 = nosys */
    ki_generic,	    /* 332 = nosys */
    ki_generic,	    /* 333 = nosys */
    ki_generic,	    /* 334 = nosys */
    ki_generic,	    /* 335 = nosys */
    ki_generic,	    /* 336 = nosys */
    ki_generic,	    /* 337 = nosys */
    ki_generic,	    /* 338 = nosys */
    ki_generic,	    /* 339 = nosys */
    ki_generic,	    /* 340 = nosys */
    ki_generic,	    /* 341 = nosys */
    ki_generic,	    /* 342 = nosys */
    ki_generic,	    /* 343 = nosys */
    ki_generic,	    /* 344 = nosys */
    ki_generic,	    /* 345 = nosys */
    ki_generic,	    /* 346 = nosys */
    ki_generic,	    /* 347 = nosys */
    ki_generic,	    /* 348 = nosys */
    ki_generic,	    /* 349 = nosys */
    ki_generic,	    /* 350 = nosys */
    ki_generic,	    /* 351 = nosys */
    ki_generic,	    /* 352 = nosys */
    ki_generic,	    /* 353 = nosys */
    ki_generic,	    /* 354 = nosys */
    ki_generic,	    /* 355 = nosys */
    ki_generic,	    /* 356 = nosys */
    ki_generic,	    /* 357 = nosys */
    ki_generic,	    /* 358 = nosys */
    ki_generic,	    /* 359 = nosys */
    ki_generic,	    /* 360 = nosys */
    ki_generic,	    /* 361 = nosys */
    ki_generic,	    /* 362 = nosys */
    ki_generic,	    /* 363 = nosys */
    ki_generic,	    /* 364 = nosys */
    ki_generic,	    /* 365 = nosys */
    ki_generic,	    /* 366 = nosys */
    ki_generic,	    /* 367 = nosys */
    ki_generic,	    /* 368 = nosys */
    ki_generic,	    /* 369 = nosys */
    ki_generic,	    /* 370 = nosys */
    ki_generic,	    /* 371 = nosys */
    ki_generic,	    /* 372 = nosys */
    ki_generic,	    /* 373 = nosys */
    ki_generic,	    /* 374 = nosys */
    ki_generic,	    /* 375 = nosys */
    ki_generic,	    /* 376 = nosys */
    ki_generic,	    /* 377 = nosys */
    ki_generic,	    /* 378 = nosys */
    ki_generic,	    /* 379 = nosys */
    ki_generic,	    /* 380 = nosys */
    ki_generic,	    /* 381 = nosys */
    ki_generic,	    /* 382 = nosys */
    ki_generic,	    /* 383 = nosys */
    ki_generic,	    /* 384 = nosys */
    ki_generic,	    /* 385 = nosys */
    ki_generic,	    /* 386 = nosys */
    ki_generic,	    /* 387 = nosys */
    ki_generic,	    /* 388 = nosys */
    ki_generic,	    /* 389 = nosys */
    ki_generic,	    /* 390 = nosys */
    ki_generic,	    /* 391 = nosys */
    ki_generic,	    /* 392 = nosys */
    ki_generic,	    /* 393 = nosys */
    ki_generic,	    /* 394 = nosys */
    ki_generic,	    /* 395 = nosys */
    ki_generic,	    /* 396 = nosys */
    ki_generic,	    /* 397 = nosys */
    ki_generic,	    /* 398 = nosys */
    ki_generic,	    /* 399 = nosys */
    ki_generic,	    /* 400 = nosys */
    ki_generic,	    /* 401 = nosys */
    ki_generic,	    /* 402 = nosys */
    ki_generic,	    /* 403 = nosys */
    ki_generic,	    /* 404 = nosys */
    ki_generic,	    /* 405 = nosys */
    ki_generic,	    /* 406 = nosys */
    ki_generic,	    /* 407 = nosys */
    ki_generic,	    /* 408 = nosys */
    ki_generic,	    /* 409 = nosys */
    ki_generic,	    /* 410 = nosys */
    ki_generic,	    /* 411 = nosys */
    ki_generic,	    /* 412 = nosys */
    ki_generic,	    /* 413 = nosys */
    ki_generic,	    /* 414 = nosys */
    ki_generic,	    /* 415 = nosys */
    ki_generic,	    /* 416 = nosys */
    ki_generic,	    /* 417 = nosys */
    ki_generic,	    /* 418 = nosys */
    ki_generic,	    /* 419 = nosys */
    ki_generic,	    /* 420 = nosys */
    ki_generic,	    /* 421 = nosys */
    ki_generic,	    /* 422 = nosys */
    ki_generic,	    /* 423 = nosys */
    ki_generic,	    /* 424 = nosys */
    ki_generic,	    /* 425 = nosys */
    ki_generic,	    /* 426 = nosys */
    ki_generic,	    /* 427 = nosys */
    ki_generic,	    /* 428 = nosys */
    ki_generic,	    /* 429 = nosys */
    ki_generic,	    /* 430 = nosys */
    ki_generic,	    /* 431 = nosys */
    ki_generic,	    /* 432 = nosys */
    ki_generic,	    /* 433 = nosys */
    ki_generic,	    /* 434 = nosys */
    ki_generic,	    /* 435 = nosys */
    ki_generic,	    /* 436 = nosys */
    ki_generic,	    /* 437 = nosys */
    ki_generic,	    /* 438 = nosys */
    ki_generic,	    /* 439 = nosys */
    ki_generic,	    /* 440 = nosys */
    ki_generic,	    /* 441 = nosys */
    ki_generic,	    /* 442 = nosys */
    ki_generic,	    /* 443 = nosys */
    ki_generic,	    /* 444 = nosys */
    ki_generic,	    /* 445 = nosys */
    ki_generic,	    /* 446 = nosys */
    ki_generic,	    /* 447 = nosys */
    ki_generic,	    /* 448 = nosys */
    ki_generic,	    /* 449 = nosys */
    ki_generic,	    /* 450 = nosys */
    ki_generic,	    /* 451 = nosys */
    ki_generic,	    /* 452 = nosys */
    ki_generic,	    /* 453 = nosys */
    ki_generic,	    /* 454 = nosys */
    ki_generic,	    /* 455 = nosys */
    ki_generic,	    /* 456 = nosys */
    ki_generic,	    /* 457 = nosys */
    ki_generic,	    /* 458 = nosys */
    ki_generic,	    /* 459 = nosys */
    ki_generic,	    /* 460 = nosys */
    ki_generic,	    /* 461 = nosys */
    ki_generic,	    /* 462 = nosys */
    ki_generic,	    /* 463 = nosys */
    ki_generic,	    /* 464 = nosys */
    ki_generic,	    /* 465 = nosys */
    ki_generic,	    /* 466 = nosys */
    ki_generic,	    /* 467 = nosys */
    ki_generic,	    /* 468 = nosys */
    ki_generic,	    /* 469 = nosys */
    ki_generic,	    /* 470 = nosys */
    ki_generic,	    /* 471 = nosys */
    ki_generic,	    /* 472 = nosys */
    ki_generic,	    /* 473 = nosys */
    ki_generic,	    /* 474 = nosys */
    ki_generic,	    /* 475 = nosys */
    ki_generic,	    /* 476 = nosys */
    ki_generic,	    /* 477 = nosys */
    ki_generic,	    /* 478 = nosys */
    ki_generic,	    /* 479 = nosys */
    ki_generic,	    /* 480 = nosys */
    ki_generic,	    /* 481 = nosys */
    ki_generic,	    /* 482 = nosys */
    ki_generic,	    /* 483 = nosys */
    ki_generic,	    /* 484 = nosys */
    ki_generic,	    /* 485 = nosys */
    ki_generic,	    /* 486 = nosys */
    ki_generic,	    /* 487 = nosys */
    ki_generic,	    /* 488 = nosys */
    ki_generic,	    /* 489 = nosys */
    ki_generic,	    /* 490 = nosys */
    ki_generic,	    /* 491 = nosys */
    ki_generic,	    /* 492 = nosys */
    ki_generic,	    /* 493 = nosys */
    ki_generic,	    /* 494 = nosys */
    ki_generic,	    /* 495 = nosys */
    ki_generic,	    /* 496 = nosys */
    ki_generic,	    /* 497 = nosys */
    ki_generic,	    /* 498 = nosys */
    ki_generic,	    /* 499 = nosys */
};
/* set when trace daemons are to pick up new trace buffer */
int     ki_wakeup_flag = 0;

/* major and minor numbers for kmem driver */
#define MM_MAJOR 3
#define MM_KMEM  1

/* light-weight system call disable flag */
extern int lw_syscall_off;

/* define macro for allocating a trace buffer with protection */
#define KI_data_mp(A, B) \
        { \
                A = (struct A *)ki_data(sizeof(struct A)); \
                A->kd_recid     = B; \
                ki_getprectime6(&((A)->kd_struct.cur_time)); \
        }

/* define macro for copying syscall args to trace buffer */
#define KI_copyargs(A, B) \
        { \
                register u_int   NARG; \
                if (NARG = (u_int)(A) / sizeof(u_int)) { \
                        register int    *INTp1 = (int *)(B); \
                        register int    *INTp2 = &u.u_arg[0]; \
                        do { *INTp1++ = *INTp2++; } while(--NARG); \
                } \
        }

/* this is the routine that allocates trace structures from the trace buffer */
/* NOTE: This routine MUST be called at non-interruptable processor priority */
caddr_t
ki_data(rec_sz)
register u_int  rec_sz;
{
        register struct kd_struct *kd_structp;
#ifndef	MP
#define	 my_index	0
#else
	register my_index =	getprocindex();
#endif	/* MP */
        register u_int		buf_num, next_tb, buf_num_old; 
        register 		reg_X;

        /* round up to mod 4 for long word aligned boundrys */
        rec_sz = (rec_sz + (sizeof(u_int)-1)) & ~(sizeof(u_int)-1);

loop:
	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* get the trace buffer number */
        buf_num = ki_curbuf(my_index) & (NUM_TBUFS-1);
        buf_num_old = buf_num;

        /* try and get trace memory if possible */
	if ( ((ki_next(my_index)[buf_num] + rec_sz) > ki_trace_sz) || (rec_sz == 0) ) 
	{ 
#ifdef	MP
		register		pindex;

		/* couldn't get trace memory or forcing a switch */
		if (!uniprocessor) 
		{ 
			/* Release my lock in case other processor is spinning */
			/* We don't want to wait on each other for ever!! */
			KI_SPINUNLOCK(ki_proc_lock[my_index]);
		
			/* get all processor locks to switch trace buffers */
			for (pindex = 0; pindex < KI_MAX_PROCS; pindex++)
			{
        			/* Skip if cpu is not configured */
				if (mpproc_info[pindex].prochpa == 0) continue;
       	 
				KI_SPINLOCK(ki_proc_lock[pindex]);
			}
		}
		/*
		 * we have all locks
		 * check again if we have enough space in case
		 *     somebody else switched trace buffers
		 */
        	buf_num = (ki_curbuf(my_index) & (NUM_TBUFS-1));

		if ( ((ki_next(my_index)[buf_num] + rec_sz) > ki_trace_sz) ||
			((rec_sz == 0) && (buf_num_old == buf_num)) ) 
		{	/*
			 * we still couldn't get trace memory, so switch
			 *    trace buffers for everyone since we also
			 *    now hold all the locks
			 * if could be forcing a switch if rec_sz == 0
			 */
			for (pindex = 0; pindex < KI_MAX_PROCS; pindex++)
			{
        			/* Skip if cpu is not configured */
				if (mpproc_info[pindex].prochpa == 0) continue;

				/* increment cur buf */
				ki_curbuf(pindex)++;

				/* now set next to zero */
        			buf_num = (ki_curbuf(pindex) & (NUM_TBUFS-1));
				/* set next to zero */
				ki_next(pindex)[buf_num] = 0;
			}
			/* wakeup any sleeping daemon that was waiting */
			ki_wakeup_flag++;

			/* reset the timeout count - we hold all locks */
			ki_timeoutct = ki_timeout;

			if (!uniprocessor) 
			{
				for (pindex = 0; pindex < KI_MAX_PROCS; pindex++)
				{
        				/* Skip if cpu is not configured or MY CPU */
					if ( (pindex == my_index) ||
						(mpproc_info[pindex].prochpa == 0) ) continue;
	
					KI_SPINUNLOCK(ki_proc_lock[pindex]);
       				}
			}
		} else 
		{	/* release all spinlocks except my_index */
			if (uniprocessor) 
			{
				panic("Uniprocessor Cpu interrupted in ki_data();\n");
			} else
			{	/* release all spinlocks except my_index */
				for (pindex = 0; pindex < KI_MAX_PROCS; pindex++)
				{
        				/* Skip if cpu is not configured or MY CPU */
					if ( (pindex == my_index) ||
						(mpproc_info[pindex].prochpa == 0) ) 
						continue;
       		
					KI_SPINUNLOCK(ki_proc_lock[pindex]);
				}
			}
			goto loop;
		}
#else	/* ! MP */
		/* increment cur buf */
		ki_curbuf(0)++;

		/* now set next to zero */
       		buf_num = (ki_curbuf(0) & (NUM_TBUFS-1));

		/* set next to zero */
		ki_next(0)[buf_num] = 0;

		/* wakeup any sleeping daemon that was waiting */
		ki_wakeup_flag++;

		/* reset the timeout count - we hold all locks */
		ki_timeoutct = ki_timeout;
#endif	/* ! MP */
	} 
	T_SPINLOCK(ki_proc_lock[getprocindex()]);
        
        /* get usage of buffer after allocation and bump */
        next_tb = (ki_next(my_index)[buf_num] += rec_sz);

        /* calculate pointer into trace buffer for stuffing structure */
        kd_structp = (struct kd_struct *)
			(ki_tbuf(my_index)[buf_num] + next_tb - rec_sz);

        /* stuff the length into the record */
        kd_structp->rec_sz = rec_sz;

        /* pass back, only if not valid */
	if (ON_ICS)
	{
        	kd_structp->P_pid       = NO_PID;
        	kd_structp->P_site      = 0;
	} else
	{
        	kd_structp->P_pid       = u.u_procp->p_pid;
        	kd_structp->P_site      = u.u_site;
	}
        kd_structp->P_cpu      		= my_index;	/* cpu id */

        /* stuff the sequence counter for */
        kd_structp->seqcnt = ++ki_sequence_count(my_index);

        return((caddr_t)kd_structp);
#ifndef	MP
#undef	 my_index
#endif	/* ! MP */
}

ki_syscalltrace()       /* entry to trace syscalls from locore.s */
{
        register struct kd_syscall      *kd_syscall;
        register                         PV_SkdP ki_callt;
/*      register struct kd_syscall      *(*ki_callt)();         XXX */
        register struct user            *up;
        register u_int                   call_numb;
        register                         nargs;
        register                         error;
	register int 			my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_SYSCALLS]++;
	}
        /* check if tracing enabled */
        if (ki_trace_sz == 0)	return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

	/* save origional errno */
        error = u.u_error;

         /* get pointer to u_area */
        up = &u;                        

        /* get the system call number */
        call_numb = up->u_syscall; 

        /* check if code is within the table range and cap it */
        if (call_numb >= KI_MAXSYSCALLS) call_numb = KI_MAXSYSCALLS-1;

        /* check if this call is enabled (count calls only if not) */
        if ((ki_callt = (PV_SkdP)ki_syscallenable[call_numb]) == NULL) 
		goto ki_syscalltrace_exit;

        /* get the number of arguments passed from caller */
        nargs = sysent[call_numb].sy_narg * sizeof(int);

        /* call the collection routine with number of args from proc */
        if ((kd_syscall = (*ki_callt)(nargs)) == NULL) 
		goto ki_syscalltrace_exit;

        /* on return the trace memory is allocated to the correct size */
        kd_syscall->kd_recid            = KI_SYSCALLS;

        kd_syscall->sttime      = up->u_syscall_time; /* syscall start time */
        kd_syscall->ks_syscal   = call_numb;          /* call number */
        kd_syscall->ks_error    = up->u_error = error;/* rtn&restore errno */
        kd_syscall->ks_rval1    = up->u_r.r_val1;     /* call func value */
        kd_syscall->ks_rval2    = up->u_r.r_val2;     /* call func value */
        kd_syscall->ks_nargs    = nargs;              /* number of args X 4 */
	{
		register struct ki_runtimes *kps;
		kps = &ki_timesstruct(my_index)[KT_SYS_CLOCK];
		/* 
		 * Kludge: kp_accumin set by ki_accum_push_TOS_sys() and
		 * ki_resume(). Take diff to get SYS time for this syscall.
		*/
        	kd_syscall->ks_sys_time.tv_sec = kps->kp_accumtm.tv_sec - 
			/* The kludge is this should be in the u_area */
			kps->kp_accumin.tv_sec; 

        	if ((u_int)(call_numb = kps->kp_accumtm.tv_nunit - 
			kps->kp_accumin.tv_nunit) >= ki_nunit_per_sec)
		{
			/* tv_nunit underflowed, borrow one from tv_sec */
        		kd_syscall->ks_sys_time.tv_sec--;
			call_numb += ki_nunit_per_sec;
		}
        	kd_syscall->ks_sys_time.tv_nunit = call_numb;
	}
        /* ki_timeval for current time stamp */
        ki_getprectime6(&kd_syscall->kd_struct.cur_time);

ki_syscalltrace_exit:
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

/* Just allocate the memory and set the structure type code */
struct  kd_syscall *
ki_generic(nargs)       /* good for all system calls */
{
        struct kd_syscall *kd_syscall;
        
	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* allocate trace buffer space */
        kd_syscall = (struct kd_syscall *)ki_data(sizeof(struct kd_syscall) + nargs);
        kd_syscall->ks_stype    = KD_SYSCALL_GENERIC;

        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_syscall + 1);

        return(kd_syscall);
}

/* For system calls that have a file descriptor as 1st parameter */
struct  kd_syscall *
ki_fd(nargs)                    
register u_int nargs;
{
        register struct kd_fd *kd_fd;
	register struct file	*fp;

	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* allocate trace buffer space */
        kd_fd = (struct kd_fd *)ki_data(sizeof(struct kd_fd) + nargs);
        kd_fd->KS_stype                 = KD_SYSCALL_FD;

	/* return a vnode pointer for fd identity */
	if ((u.u_error == 0) && ((fp = getf(u.u_arg[0])) != NULL))
	{
		kd_fd->f_data	= fp->f_data;
		kd_fd->f_type	= fp->f_type;
	} else 
	{
		kd_fd->f_data	= NULL;
		kd_fd->f_type	= -1;
	}
        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_fd + 1);

        return((struct kd_syscall *)kd_fd);
}

/* For pipe call that returns 2 file descriptors */
struct  kd_syscall *
ki_pipe(nargs)                  
register u_int nargs;
{
        register struct kd_fd2 *kd_fd2;
	register struct file	*fp;
        
	T_SPINLOCK(ki_proc_lock[getprocindex()]);
        
        /* allocate trace buffer space */
        kd_fd2 = (struct kd_fd2 *)ki_data(sizeof(struct kd_fd2) + nargs);
        kd_fd2->KS_stype                = KD_SYSCALL_FD2;

        /* return two file pointer for fd identify */
	if ((u.u_error == 0) && ((fp = getf(u.u_rval1)) != NULL))
	{
		kd_fd2->f_data1	= fp->f_data;
		kd_fd2->f_type1	= fp->f_type;
        } else 
	{
                kd_fd2->f_data1 = NULL;
                kd_fd2->f_type1 = -1;
        }
	if ((u.u_error == 0) && ((fp = getf(u.u_rval2)) != NULL))
	{
		kd_fd2->f_data2	= fp->f_data;
		kd_fd2->f_type2	= fp->f_type;
	} else
	{
                kd_fd2->f_data2 = NULL;
                kd_fd2->f_type1 = -1;
	}
        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_fd2 + 1);

        return((struct kd_syscall *)kd_fd2);
}

/* For socketpair call that returns 2 file descriptors */
struct  kd_syscall *
ki_socketpair(nargs)                    
register u_int nargs;
{
        register struct kd_fd2  *kd_fd2;
	register struct file	*fp;
        u_int 			 fd[2];
        
	T_SPINLOCK(ki_proc_lock[getprocindex()]);
        
        /* allocate trace buffer space */
        kd_fd2 = (struct kd_fd2 *)ki_data(sizeof(struct kd_fd2) + nargs);
        kd_fd2->KS_stype                = KD_SYSCALL_FD2;

        /* return two f_data pointers for identify */
        if (u.u_error) 
	{
                kd_fd2->f_data1 = NULL;
                kd_fd2->f_type1 = -1;
                kd_fd2->f_data2 = NULL;
                kd_fd2->f_type1 = -1;
        } else 
	{
		/* copyin pre-tested ?? */
                copyin((caddr_t)u.u_arg[3], fd, 2*sizeof(int));
		if (fp = getf(fd[0]))
		{
			kd_fd2->f_data1	= fp->f_data;
			kd_fd2->f_type1	= fp->f_type;	/* DTYPE_SOCKET	*/
		} else
		{
                	kd_fd2->f_data1 = NULL;
                	kd_fd2->f_type1 = -1;
		}
		if (fp = getf(fd[1]))
		{
			kd_fd2->f_data2	= fp->f_data;
			kd_fd2->f_type2	= fp->f_type;	/* DTYPE_SOCKET	*/
		} else
		{
                	kd_fd2->f_data2 = NULL;
                	kd_fd2->f_type2 = -1;
		}
	}
        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_fd2 + 1);

        return((struct kd_syscall *)kd_fd2);
}

/* for system calls that have a string for the 1st parameter */
struct  kd_syscall *
ki_1string(nargs)
register nargs;
{
        register struct kd_1string *kd_1string;
        int                      realen;
        char                     filename[KI_MAXPATH]; /* max file name length */

	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* copy in the file name with minimum fuss */
        if (copyinstr(u.u_arg[0], filename, KI_MAXPATH, &realen) == EFAULT) {
                /* oops - process was bad, return null string */
                filename[0] = '\0';
                realen = 1;
        }
        /* assure null termination */
        filename[KI_MAXPATH-1] = '\0';

        /* allocate trace buffer space */
        kd_1string = (struct kd_1string *)
                ki_data(sizeof(struct kd_1string) + realen + nargs);
        kd_1string->KS_stype    = KD_SYSCALL_1STRING;

        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_1string + 1);

        /* copy the name into the trace buffer */
        bcopy(filename, (char *)(kd_1string + 1) + nargs, realen);

        return((struct kd_syscall *)kd_1string);
}

/* for system calls that have strings for the 1st & 2nd parameter */
struct  kd_syscall *
ki_2string(nargs)
register nargs;
{
        register struct kd_2string *kd_2string;
        int                      realen1;
        int                      realen2;
        char                     filename1[KI_MAXPATH]; /* max file name length */
        char                     filename2[KI_MAXPATH]; /* max file name length */

	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* copy in the file name with minimum fuss */
        if (copyinstr(u.u_arg[0], filename1, KI_MAXPATH, &realen1) == EFAULT) {
                /* oops - process was bad, return null string */
                filename1[0] = '\0';
                realen1 = 1;
        }
        /* copy in the file name with minimum fuss */
        if (copyinstr(u.u_arg[1], filename2, KI_MAXPATH, &realen2) == EFAULT) {
                /* oops - process was bad, return null string */
                filename2[0] = '\0';
                realen2 = 1;
        }
        /* assure null termination */
        filename1[KI_MAXPATH-1] = '\0';
        filename2[KI_MAXPATH-1] = '\0';

        /* allocate trace buffer space */
        kd_2string = (struct kd_2string *) 
                ki_data(sizeof(struct kd_2string) + realen1 + realen2 + nargs); 
        kd_2string->KS_stype    = KD_SYSCALL_2STRING;

        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_2string + 1);

        /* copy filename1 into the trace buffer */
        bcopy(filename1, (char *)(kd_2string + 1) + nargs, realen1);

        /* copy filename2 into the trace buffer */
        bcopy(filename2, (char *)(kd_2string + 1) + nargs + realen1, realen2);

        return((struct kd_syscall *)kd_2string);
}

struct  kd_syscall *
ki_fork(nargs)                  /* process birth (just afterwards) */
register nargs;
{
        register struct kd_fork *kd_fork;
        register struct proc *p = u.u_procp;

	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* allocate trace buffer space */
        kd_fork = (struct kd_fork *)ki_data(sizeof(struct kd_fork) + nargs);
        kd_fork->KS_stype       = KD_SYSCALL_FORK;

        /* process name and a.out parameters same as parent */
        kd_fork->p_ttyd		= u.u_procp->p_ttyd;    /* controlling tty dev */
        kd_fork->p_uid		= p->p_uid;     /* user id, used to direct tty signals */
        kd_fork->p_suid		= p->p_suid;    /* set (effective) uid */
        kd_fork->p_pgrp		= p->p_pgrp;    /* name of process group leader */
        kd_fork->p_ppid		= p->p_ppid;    /* process id of parent */
        kd_fork->kd_u_uid	= u.u_uid;	/* real            user user  id     */
        kd_fork->kd_u_ruid	= u.u_ruid;	/*                 user user  id     */
        kd_fork->kd_u_gid	= u.u_gid;	/*                 user group id     */
        kd_fork->kd_u_rgid	= u.u_rgid;	/* real            user group id     */
        kd_fork->u_sgid		= u.u_sgid;	/* set (effective) user group id     */

        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_fork + 1);

        return((struct kd_syscall *)kd_fork);
}

/* successfull exec of a process (just afterwards) */
struct  kd_syscall *
ki_exec(nargs)
register nargs;
{
        register struct kd_exec *kd_exec;
        int                      realen;
        char                     filename[PST_CLEN]; /* max file name length */
	extern	char 		*pst_cmds;
        
	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        if (u.u_error) {              /* errno */
                /* copy in the 1st arg name with minimum fuss */
                if (copyinstr(u.u_arg[0], filename, PST_CLEN, &realen) == EFAULT) {
                        /* oops - process was bad, return null string */
                        filename[0] = '\0';
                        realen = 1;
                }
        } else {
		strcpy(filename, pst_cmds + ((u.u_procp-proc)*PST_CLEN));
		realen = strlen(filename) + 1;
        }
        /* allocate trace buffer space */
        kd_exec = (struct kd_exec *)ki_data(sizeof(struct kd_exec) + realen + nargs); 
        kd_exec->KS_stype       = KD_SYSCALL_EXEC;

        /* get the vnode pointer from a funny place */
        kd_exec->vp 	= (struct vnode *)u.u_dev_t;

        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_exec + 1);

        /* copy filename into the trace buffer */
        bcopy(filename, (char *)(kd_exec + 1) + nargs, realen);

        return((struct kd_syscall *)kd_exec);
}

/* system call open a pathname as the 1st parameter */
struct  kd_syscall *
ki_open(nargs)
register nargs;
{
        register struct kd_open *kd_open;
        int                      realen;
        char                     filename[KI_MAXPATH]; /* max file name length */

        register struct file *fp;


	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* copy in the file name with minimum fuss */
        if (copyinstr(u.u_arg[0], filename, KI_MAXPATH, &realen) == EFAULT) 
	{
                /* oops - process was bad, return null string */
                filename[0] = '\0';
                realen = 1;
        }
        /* assure null termination */
        filename[KI_MAXPATH-1] = '\0';

        /* allocate trace buffer space */
        kd_open = (struct kd_open *)ki_data(sizeof(struct kd_open) + realen + nargs);
        kd_open->KS_stype       = KD_SYSCALL_OPEN;

	/* return a vnode pointer for fd identity */
	if ((u.u_error == 0) && ((fp = getf(u.u_rval1)) != NULL))
	{
		kd_open->f_data	= fp->f_data;
		kd_open->f_type	= fp->f_type;
	} else 
	{
		kd_open->f_data	= NULL;
		kd_open->f_type	= -1;
	}
        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_open + 1);

        /* copy the name into the trace buffer */
        bcopy(filename, (char *)(kd_open + 1) + nargs, realen);

        return((struct kd_syscall *)kd_open);
}

struct  kd_syscall *
ki_mmap(nargs)
register nargs;
{
        register struct kd_mmap *kd_mmap;
	register preg_t		*prp;

	T_SPINLOCK(ki_proc_lock[getprocindex()]);

        /* allocate trace buffer space */
        kd_mmap = (struct kd_mmap *)ki_data(sizeof(struct kd_mmap) + nargs);
        kd_mmap->KS_stype       = KD_SYSCALL_MMAP;

	/* return vnode, region and pregion pointers */
        if (u.u_error || ((prp = (preg_t *)u.u_rval2) == NULL))
	{
		kd_mmap->prp		= NULL;
       		kd_mmap->rp		= NULL;
       		kd_mmap->r_fstore	= NULL;
       		kd_mmap->r_bstore	= NULL;
	} else
	{
        	kd_mmap->prp		= prp;
	        kd_mmap->rp		= prp->p_reg;
	        kd_mmap->region		= *prp->p_reg; /* XXX needs shortening */
       		kd_mmap->r_fstore	= prp->p_reg->r_fstore;
       		kd_mmap->r_bstore	= prp->p_reg->r_bstore;
	}
        /* copy the passed args into trace buffer */
        KI_copyargs(nargs, kd_mmap + 1);

        return((struct kd_syscall *)kd_mmap);
}

/* this is when a request is put on the disk queue */
ki_enqueue(bp)                  /* add request hpib, scci, gpio (disk) queue */
struct  buf     *bp;
{
        register struct kd_enqueue *kd_enqueue;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_ENQUEUE]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_enqueue, KI_ENQUEUE);

        /* return the request parameters and enqueue time */
        kd_enqueue->bp          = bp;           /* buffer ident */
        kd_enqueue->b_dev       = bp->b_dev;    /* device id */
        kd_enqueue->b_flags     = bp->b_flags;  /* see buf.h */
        kd_enqueue->b_bcount    = bp->b_bcount; /* request size */
        kd_enqueue->b_blkno     = bp->b_blkno;  /* disk address */
        kd_enqueue->b_bptype    = bp->b_bptype; /* see buf.h */
        kd_enqueue->b_queuelen  = bp->b_queuelen;/* queue length on disc */
        kd_enqueue->b_upid      = bp->b_upid;   /* pid to last use it */
        kd_enqueue->b_apid      = bp->b_apid;   /* pid to allocate it */
        kd_enqueue->b_timeval_at = bp->b_timeval_eq;/* allocated time */
        kd_enqueue->b_timeval_lu = bp->b_timeval_qs;/* last used time */
        kd_enqueue->b_vp 	= (caddr_t)bp->b_rp;/* region of request */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

/* start of actual I/O on hpib, scci, gpio (disk) */
ki_queuestart(bp)
struct  buf     *bp;
{
        register struct kd_queuestart *kd_queuestart;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_QUEUESTART]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_queuestart, KI_QUEUESTART);

        kd_queuestart->bp       = bp;           /* buffer ident */
        kd_queuestart->b_vp 	= (caddr_t)bp->b_rp;/* region of request */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

/* done with request on hpib, scci, gpio (disk) */
ki_queuedone(bp)
struct  buf     *bp;
{
        register struct kd_queuedone *kd_queuedone;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_QUEUEDONE]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_queuedone, KI_QUEUEDONE);

        kd_queuedone->bp                = bp;           /* buffer ident */
        kd_queuedone->b_dev     = bp->b_dev;    /* device id */
        kd_queuedone->b_flags   = bp->b_flags;  /* see buf.h */
        kd_queuedone->b_bcount  = bp->b_bcount; /* request size */
        kd_queuedone->b_blkno   = bp->b_blkno;  /* disk address */
        kd_queuedone->b_bptype  = bp->b_bptype; /* see buf.h */
        kd_queuedone->b_queuelen= bp->b_queuelen;/* queue length on disc */
        kd_queuedone->b_resid   = bp->b_resid;  /* zero in most cases */
        kd_queuedone->b_upid    = bp->b_upid;   /* pid to last use it */
        kd_queuedone->b_apid    = bp->b_apid;   /* pid to allocate it */
	kd_queuedone->b_site	= bp->b_site;	/* site(cnode) for bp */
        kd_queuedone->b_timeval_eq = bp->b_timeval_eq;/* enqueue */
        kd_queuedone->b_timeval_qs = bp->b_timeval_qs;/* queuestart */
        kd_queuedone->b_vp 	= (caddr_t)bp->b_rp;/* region of request */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

/* this is called for each cpu */
#ifndef	 __hp9000s300
ki_hardclock(ssp)
register struct save_state *ssp;
#else
ki_hardclock(pc, cpstate)
caddr_t	pc;
int	cpstate;
#endif	/* __hp9000s300 */
{
	register int i;
#ifndef	 __hp9000s300
	register int cpstate;
#endif	/* __hp9000s300 */
        register struct kd_hardclock *kd_hardclock;
	register int my_index = getprocindex();
        struct ki_timeval	ki_cur_time;
        register 		reg_X;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

	/* if monarch */
	if (my_index == MP_MONARCH_INDEX)
	{
		/* if monarch (does not really matter) and timeouts are enabled */
		if (ki_timeout) 
		{
			/* first grab the lock before decrementing ki_timeoutct
		 	* otherwise some other CPU may switch buffers
		 	*/
			if (--ki_timeoutct == 0) {
#ifdef KI_DEBUG
				ki_test();
#endif /* KI_DEBUG */
		        	/* force a switch and reset ki_timeoutct in ki_data */
				ki_data(0);
			}
		} 
		/* only monarch can reset wakeup flag */
        	if (ki_wakeup_flag) 
		{
                	/* acquire my lock since this will prevent anybody else 
                 	 * from changing the ki_wakeup_flag in ki_data
                 	 */
                	ki_wakeup_flag = 0;
                	wakeup(&ki_cf);
        	}
	}
#ifdef	 __hp9000s300
	ki_freq_ratio(my_index) = 1.0;
#else	/* __hp9000s300 */
	/* Update the ratio, just in case it changed */
	ki_freq_ratio(my_index) = (getproc_info())->tod_info.freq_ratio;
#endif	/* __hp9000s300 */
	{
	register u_int		itmr;
	/* Calculate correction from monarch's CR_IT of this cpu (for ki_getprectime()) */
	_MFCTL(CR_IT, itmr);	
#ifdef	 __hp9000s300
	ki_offset_correction(my_index) = (u_short)(read_adjusted_itmr() - itmr);
#else	/* __hp9000s300 */
	ki_offset_correction(my_index) = read_adjusted_itmr() - itmr;
#endif	/* __hp9000s300 */
	}

	/* keep the timers current (to prevent rollover of CR_IT values) */
	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_getprectime6(&ki_cur_time);
	}
        /* current cpstate definitions - notice "nice" */
        /*      CP_USER  user mode of USER process */
        /*      CP_NICE  user mode of USER process at nice priority */
        /*      CP_SYS   kernel mode of USER process */
        /*      CP_SSYS  kernel mode of KERNEL process */
        /*      CP_IDLE  IDLE mode */
        /*      CP_INTR  INTERRUPT mode */

#ifndef	 __hp9000s300
	/* Calculate the cpstate */
        if (ssp->ss_flags&SS_PSPKERNEL)
	{
                cpstate = CP_INTR;

	} else if (getnoproc())
	{
                cpstate = CP_IDLE;
                if (ki_kernelenable[KI_HARDCLOCK_IDLE]) 
		{
                	/* count number of calls */
			if (ki_kernelenable[KI_GETPRECTIME]) 
			{
                		ki_kernelcounts(my_index)[KI_HARDCLOCK_IDLE]++;
			}
                } else { goto ki_hardclock_exit; }
	} else 
	{
		if (USERMODE(ssp->ss_pcoq_head)) 
		{
			if (u.u_procp->p_nice > NZERO)
				{ cpstate = CP_NICE; }
			else
				{ cpstate = CP_USER; }
                } else 
		{
			if (u.u_procp->p_flag&SSYS)
				{ cpstate = CP_SSYS; }
			else
				{ cpstate = CP_SYS; }
		}
	}
#endif	/* ! __hp9000s300 */

        if (ki_kernelenable[KI_HARDCLOCK]) 
	{ 	/* count number of non-IDLE hardclock calls */
                if (cpstate != CP_IDLE) 
		{ 
			if (ki_kernelenable[KI_GETPRECTIME]) 
			{
				ki_kernelcounts(my_index)[KI_HARDCLOCK]++; 
			}
		}
	} else { goto ki_hardclock_exit; }
	/* To reach here KI_HARDCLOCK must be ON (and maybe KI_HARDCLOCK_IDLE) */

        /* check if tracing enabled */
        if (ki_trace_sz == 0) { goto ki_hardclock_exit; }

        /* allocate trace buffer space */
        kd_hardclock = (struct kd_hardclock *)
		ki_data(sizeof(struct kd_hardclock));
        kd_hardclock->kd_recid          = KI_HARDCLOCK;
        ki_getprectime6(&kd_hardclock->kd_struct.cur_time);

	if (cpstate != CP_IDLE && cpstate != CP_INTR)
	{
       		kd_hardclock->kd_pid       = u.u_procp->p_pid;
       		kd_hardclock->kd_site      = u.u_site;
	}
        kd_hardclock->cpstate   = cpstate;
#ifdef	 __hp9000s300
        kd_hardclock->pc        = pc;
#else
        kd_hardclock->pc        = ssp->ss_pcoq_head;
#endif	/* ! __hp9000s300 */

ki_hardclock_exit:
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_brelse(bp)                   /* buffer being put on LOCK/AGE/LRU list */
register struct buf     *bp;
{
        register struct kd_brelse *kd_brelse;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_BRELSE]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_brelse, KI_BRELSE);

        kd_brelse->bp           = bp;           /* buffer ident */
        kd_brelse->b_flags      = bp->b_flags;  /* see buf.h */
        kd_brelse->b_bptype	= bp->b_bptype; /* see buf.h */
        kd_brelse->b_vp 	= (caddr_t)bp->b_rp;/* region for now of request */

        /* return time removed from lru list (hash chain) */
        /* OR last placed on the disk (queuestart) */
        kd_brelse->sttime = bp->b_timeval_qs; 

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_getnewbuf(bp)                /* buffer being allocated from buffer cache */
register struct buf     *bp;
{
        register struct kd_getnewbuf *kd_getnewbuf;
	register int x;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_GETNEWBUF]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_getnewbuf, KI_GETNEWBUF);

        /* return buffer identity, type and size */
        kd_getnewbuf->bp        = bp;
        kd_getnewbuf->b_bptype  = bp->b_bptype; /* see buf.h */

        /* approximate buffer size (may change after allocation) */
        kd_getnewbuf->b_bufsize = bp->b_bufsize;        /* buffer size */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_setrq(setrq_caller, p)
caddr_t          setrq_caller;  /* this is a kludge, did not want to mod locore */
struct  proc    *p;
{
        register struct kd_setrq *kd_setrq;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_SETRQ]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_setrq, KI_SETRQ);

        kd_setrq->p_pid         = p->p_pid;
        kd_setrq->p_pri         = p->p_pri;
        kd_setrq->setrq_caller  = setrq_caller;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

#if defined(__hp9000s800) && !defined(_WSIO)
ki_miss_alpha()
{
        register int    my_index = getprocindex();
        struct kd_miss_alpha *kd_m_a;
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_MISS_ALPHA]++;
	}
        /* allocate trace buffer space (spl level is non-interruptable) */
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        kd_m_a = (struct kd_miss_alpha *)ki_data(sizeof(struct kd_miss_alpha));
        ki_getprectime6(&kd_m_a->kd_struct.cur_time);
        kd_m_a->kd_recid  = KI_MISS_ALPHA;
        kd_m_a->nswitches = u.u_swtchd_for_sema;
        u.u_swtchd_for_sema = 0;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}
#endif /* defined(__hp9000s800) && !defined(_WSIO) */


/*
 * For MP sleep_caller will have the lower two bits set to
 *		1 for alpha semaphores
 *		2 for beta semaphores
 *		3 for sync semaphores
 *
 * This works since the caller address will always be word aligned
 */
ki_swtch(sleep_caller)
caddr_t sleep_caller;   /* sleep return address */
{
        register struct kd_swtch        *kd_swtch;
        register struct proc            *p = u.u_procp;
	register int			my_index = getprocindex();
        register 			reg_X;

#if defined(__hp9000s800) && !defined(_WSIO)
	if (p->p_needs_sema) {
		u.u_swtchd_for_sema++;
		if (u.u_swtchd_for_sema > 1) 
			return;
	}
#endif /* defined(__hp9000s800) && !defined(_WSIO) */

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_SWTCH]++;
	}
        /* allocate trace buffer space (spl level is non-interruptable) */
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        KI_data_mp(kd_swtch, KI_SWTCH);

	kd_swtch->ki_syscallbeg = u.u_syscall_time; /* syscall start time */
	kd_swtch->ks_syscal	= u.u_syscall;      /* last syscall number*/

        /* initialize trace buffer with current time parameters */
	bcopy(ki_timesstruct(my_index), 
		kd_swtch->ks_swt, 
		sizeof(struct ki_runtimes) * KT_NUMB_CLOCKS);
	{
		register struct ki_runtimes *kps;
		register u_int		     nunit;

		kps = &ki_timesstruct(my_index)[KT_SYS_CLOCK];
		/* 
		 * Kludge: kp_accumin set by ki_accum_push_TOS_sys() and
		 * ki_resume(). Take diff to get SYS time from syscall.
		*/
        	kd_swtch->ks_sys_time.tv_sec = kps->kp_accumtm.tv_sec - 
			/* The kludge is this should be in the u_area */
			kps->kp_accumin.tv_sec; 

        	if ((u_int)(nunit = kps->kp_accumtm.tv_nunit - 
			kps->kp_accumin.tv_nunit) >= ki_nunit_per_sec)
		{
			/* tv_nunit underflowed, borrow one from tv_sec */
        		kd_swtch->ks_sys_time.tv_sec--;
			nunit += ki_nunit_per_sec;
		}
        	kd_swtch->ks_sys_time.tv_nunit = nunit;
	}
        /* current stat codes                                              */
        /*      SSLEEP  awaiting an event                                  */
        /*      SWAIT   (abandoned state)                                  */
        /*      SRUN    running                                            */
        /*      SIDL    intermediate state in process creation             */
        /*      SZOMB   intermediate state in process termination          */
        /*      SSTOP   process being traced                               */

        /* proc table stuff */

        kd_swtch->ki_p_wchan    = p->p_wchan;   /* sleep channel           */
        kd_swtch->ki_p_stat     = p->p_stat;    /* see above               */
        kd_swtch->ki_p_pri      = p->p_pri;     /* process priority        */
        kd_swtch->ki_p_cpu      = p->p_cpu;     /* cpu usage for scheduling*/
        kd_swtch->ki_p_usrpri   = p->p_usrpri;  /* based on p_cpu & p_nice */
        kd_swtch->ki_p_rtpri    = p->p_rtpri;   /* real time priority      */
        kd_swtch->ki_p_nice     = p->p_nice;    /* nice value priority     */

        { /* rusage table stuff XXX */
#if defined(__hp9000s800) && !defined(_WSIO)
        register struct rusage          *ru = u.u_procp->p_rusagep;
#else /* !Series 800 */
        register struct rusage          *ru = &u.u_ru;
#endif /* Series 800 */

        kd_swtch->ki_ru_minflt  = ru->ru_minflt;/* page reclaims           */
        kd_swtch->ki_ru_majflt  = ru->ru_majflt;/* page faults             */
        kd_swtch->ki_ru_nswap   = ru->ru_nswap; /* number swaps            */
        kd_swtch->ki_ru_nsignals= ru->ru_nsignals;/* number signals sent   */
        kd_swtch->ki_ru_maxrss  = ru->ru_maxrss;/* high water  RSS of all  */
        kd_swtch->ki_ru_msgsnd  = ru->ru_msgsnd;/* messages sent */
        kd_swtch->ki_ru_msgrcv  = ru->ru_msgrcv;/* messages received */
        }
        if (p->p_stat == SSLEEP) {
             kd_swtch->ki_sleep_caller = sleep_caller;
        } else {
                kd_swtch->ki_sleep_caller = NULL;
        }
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_resume_csw()
{
        register struct kd_resume_csw *kd_resume_csw;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_RESUME_CSW]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_resume_csw, KI_RESUME_CSW);

        /* initialize trace buffer with current time parameters */
	bcopy(ki_timesstruct(my_index), 
		kd_resume_csw->ks_swt, 
		sizeof(struct ki_runtimes) * KT_NUMB_CLOCKS);

        kd_resume_csw->ki_p_pri = u.u_procp->p_pri;     /* process priority */
	{
		register struct ki_runtimes *kps;
		kps = &ki_timesstruct(my_index)[KT_SYS_CLOCK];
		/* 
		 * Kludge: kp_accumin set by ki_accum_push_TOS_sys() and
		 * ki_resume().
		*/
		/* The kludge is this should be in the u_area */
		kps->kp_accumin = kps->kp_accumtm;
	}
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_servestrat(bp, length, starttime)/* server side Diskless read/write */
struct  buf     *bp;
u_int           length;
struct  ki_timeval      *starttime;
{
        register struct kd_servestrat *kd_servestrat;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_SERVESTRAT]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_servestrat, KI_SERVESTRAT);

        kd_servestrat->sttime           = *starttime;
        kd_servestrat->dev              = bp->b_dev;
        kd_servestrat->rw               = (bp->b_flags&B_READ)?0:1; /* XXX */
        kd_servestrat->length           = length;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_servsyncio(dev, rw, length, starttime)/* server side Diskless(syncroness) read/write */
dev_t           dev;
enum    uio_rw  rw;
u_int           length;
struct  ki_timeval      *starttime;
{
        register struct kd_servsyncio *kd_servsyncio;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_SERVSYNCIO]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_servsyncio, KI_SERVSYNCIO);

        kd_servsyncio->sttime           = *starttime;
        kd_servsyncio->rw               = rw;
        kd_servsyncio->dev              = dev;
        kd_servsyncio->length           = length;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

struct  ki_timeval      ki_zero_timeval = {0, 0};

ki_dm1_recv(hp, starttime)
register struct dm_header *hp;
struct  ki_timeval      *starttime;
{
        register struct kd_dm1_recv *kd_dm1_recv;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_DM1_RECV]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_dm1_recv, KI_DM1_RECV);

        kd_dm1_recv->sttime             = *starttime;
        if (hp->dm_bufp != NULL) {
                kd_dm1_recv->send_time  = hp->dm_bufp->b_timeval_qs;
                kd_dm1_recv->b_upid     = hp->dm_bufp->b_upid;  /* pid to last use it */
                kd_dm1_recv->b_apid     = hp->dm_bufp->b_apid;  /* pid to allocate it */
        } else {
                kd_dm1_recv->send_time  = ki_zero_timeval;
                kd_dm1_recv->b_upid     = 0;
                kd_dm1_recv->b_apid     = 0;
        }
        kd_dm1_recv->kdr_op             = hp->dm_op;
        kd_dm1_recv->kdr_site           = hp->dm_srcsite;
        kd_dm1_recv->kdr_length         = hp->dm_ph.p_dmmsg_length +
						sizeof(struct proto_header);
        kd_dm1_recv->kdr_dmmsg_length   = hp->dm_ph.p_dmmsg_length;
        kd_dm1_recv->kdr_data_length    = hp->dm_ph.p_data_length;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_dm1_send(hp, starttime)
register struct dm_header *hp;
struct  ki_timeval      *starttime;
{
        register struct kd_dm1_send *kd_dm1_send;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_DM1_SEND]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_dm1_send, KI_DM1_SEND);

        kd_dm1_send->sttime             = *starttime;
        kd_dm1_send->kds_op             = hp->dm_op;
        kd_dm1_send->kds_site           = hp->dm_dest;
        kd_dm1_send->kds_length         = hp->dm_ph.p_dmmsg_length +
						sizeof(struct proto_header);
        kd_dm1_send->kds_dmmsg_length   = hp->dm_ph.p_dmmsg_length;
        kd_dm1_send->kds_data_length    = hp->dm_ph.p_data_length;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_rfscall(mi, which, wrargs, rdres, starttime)
register struct mntinfo *mi;
int			which;
caddr_t			wrargs;
caddr_t			rdres;
struct	ki_timeval	*starttime;
{
	register struct kd_rfscall	*kd_rfscall;
	register int 	my_index =	getprocindex();
	register			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME])
	{
		ki_kernelcounts(my_index)[KI_RFSCALL]++;
	}
	if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

	/* allocate trace buffer space */
	KI_data_mp(kd_rfscall, KI_RFSCALL);

	kd_rfscall->sttime	= *starttime;
	kd_rfscall->kr_which	= which;
	kd_rfscall->kr_mntno	= mi->mi_mntno | 0xff000000;
	kd_rfscall->kr_addr	= mi->mi_addr.sin_addr.s_addr;

	switch(which)
	{
	case RFS_READ:
		kd_rfscall->kr_length = ((struct nfsrdresult *)rdres)->rr_count;
		break;

	case RFS_WRITE:
		kd_rfscall->kr_length = ((struct nfswriteargs *)wrargs)->wa_count;
		break;
	default:
		kd_rfscall->kr_length = 0;
		break;
	}
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_rfs_dispatch(args, req, res, starttime)
struct svc_req  *req;
caddr_t         *args;
caddr_t         *res;
struct ki_timeval       *starttime;
{
        register struct kd_rfs_dispatch *kd_rfs_dispatch;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_RFS_DISPATCH]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_rfs_dispatch, KI_RFS_DISPATCH);

        /* check if a READ or WRITE request */
        switch(kd_rfs_dispatch->kr_which = req->rq_proc) {
        case RFS_READ:
                kd_rfs_dispatch->kr_length = 
                        ((struct nfsrdresult *)res)->rr_count;
                break;

        case RFS_WRITE:
                kd_rfs_dispatch->kr_length = 
                        ((struct nfswriteargs *)args)->wa_count;
                break;

        default:
                kd_rfs_dispatch->kr_length = 0;
                break;
        }
        kd_rfs_dispatch->sttime = *starttime;
        kd_rfs_dispatch->kr_addr        = req->rq_xprt->xp_raddr.sin_addr.s_addr;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_do_bio(bp)
struct  buf     *bp;
{
        register struct kd_do_bio *kd_do_bio;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_DO_BIO]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_do_bio, KI_DO_BIO);

        kd_do_bio->bp           = bp;           /* buffer ident */
        kd_do_bio->mntno        = vtomi(bp->b_vp)->mi_mntno | 0xff000000; /* pseuto dev_t */
        kd_do_bio->b_flags      = bp->b_flags;  /* see buf.h */
        kd_do_bio->b_bcount     = bp->b_bcount; /* request size */
        kd_do_bio->b_bptype     = bp->b_bptype; /* see buf.h */
        kd_do_bio->b_queuelen   = bp->b_queuelen;/* queue length on disc */
        kd_do_bio->b_resid      = bp->b_resid;  /* zero in most cases */
        kd_do_bio->b_upid       = bp->b_upid;   /* pid to last use it */
        kd_do_bio->b_apid       = bp->b_apid;   /* pid to allocate it */
        kd_do_bio->b_timeval_eq = bp->b_timeval_eq;/* enqueue */
        kd_do_bio->b_timeval_qs = bp->b_timeval_qs;/* queuestart */
        kd_do_bio->ip_addr      = vtomi(bp->b_vp)->mi_addr.sin_addr.s_addr;
        kd_do_bio->b_vp 	= (caddr_t)bp->b_rp;/* vnode of request */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_closef(fp)
struct  file    *fp;
{
        register struct kd_closef *kd_closef;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_CLOSEF]++;
	}
        if (ki_trace_sz == 0)   return; 

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_closef, KI_CLOSEF);

        kd_closef->f_data       = fp->f_data;
        kd_closef->f_type       = fp->f_type;
        kd_closef->f_count      = fp->f_count;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_locallookuppn(pathp, vp, error, starttime)
char    *pathp;
struct  vnode   *vp;
struct ki_timeval       *starttime;
{
        register struct kd_locallookuppn *kd_locallookuppn;
        int				 pathlen;
        register 		 	 reg_X;
	register int			 my_index= getprocindex();
	register ino_t		 	 i_number;
	register dev_t		 	 dev;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_LOCALLOOKUPPN]++;
	}
        if (ki_trace_sz == 0)   return;

        pathlen = strlen(pathp);
	/* set max path length */
        if (pathlen > KI_MAXPATH) { pathlen = KI_MAXPATH; }

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        kd_locallookuppn = (struct kd_locallookuppn *)
                ki_data(sizeof(struct kd_locallookuppn) + pathlen + 1);
	kd_locallookuppn->kd_recid = KI_LOCALLOOKUPPN;
        ki_getprectime6(&((kd_locallookuppn)->kd_struct.cur_time));

        bcopy(pathp, kd_locallookuppn+1, pathlen);

	/* make sure null terminated */
        *((char *)(kd_locallookuppn+1) + pathlen) = '\0';

        kd_locallookuppn->sttime	= *starttime;
        kd_locallookuppn->vp		= vp;
        kd_locallookuppn->error		= error;
	if (error || (vp == NULL))
	{
        	kd_locallookuppn->v_fstype 	= 0;
        	kd_locallookuppn->v_type 	= 0;
        	i_number		 	= 0;
        	dev	 			= 0;
	} else 
	{
		/* default dev */
		dev				= vp->v_rdev;
        	kd_locallookuppn->v_fstype 	= vp->v_fstype;
        	kd_locallookuppn->v_type 	= vp->v_type;

		/* The rest is just to find the i_number and dev */

		switch(vp->v_fstype) 
		{
		case	VNFS:
			/* get the NFS inode number */
			i_number = (&vtor(vp)->r_nfsattr)->na_nodeid;
	
			switch(vp->v_type) 
			{
#ifdef NOT_NECESSARY
			case	VREG:
			case	VDIR:
			case	VSOCK:
			case	VFIFO:
			case	VBLK:
			case	VCHR:
			/* don't believe these are legal */
			case	VNON:
			case	VLNK:
			case	VBAD:
			case	VFNWK:
			case	VEMPTYDIR:
				break;
#endif /* NOT_NECESSARY */
			default:
				dev = 0xff000000 | vtomi(vp)->mi_mntno;
				break;
			}
			break;

		case	VUFS:
		case	VDUX:
			/* get the UFS/DUX inode number */
			i_number = VTOI(vp)->i_number;
	
			switch(vp->v_type) 
			{
			case	VBLK:
			case	VCHR:
         			/* v_rdev field not filled in if the inode */
                                /* for the device file is type VDUX and */
                                /* if the device has not been opened. */
				/* dev = (VTOI(vp))->i_device;????? ***/

				dev	= VTOI(vp)->i_rdev;
				break;
	
			case	VREG:
			case	VDIR:
				dev	= VTOI(vp)->i_dev;
				break;
#ifdef NOT_NECESSARY
			case    VNON:
			case    VLNK: 
			case    VBAD: 
			case    VSOCK: 
			case    VFIFO: 
			case    VFNWK: 
			case    VEMPTYDIR:
#endif /* NOT_NECESSARY */
			default:
				break;
			}
			break;

		case    VCDFS: 
		case    VDUX_CDFS: 
			/* get the CDFS/DUX_CDFS cdnode number */
			i_number = VTOCD(vp)->cd_num;
	
			switch(vp->v_type) 
			{
			case    VREG: 
			case    VDIR: 
				dev	= VTOCD(vp)->cd_dev;
				break;
#ifdef NOT_NECESSARY
			case	VBLK:
			case	VCHR:
			case	VFIFO:
			/* don't believe these are legal */
			case	VNON:
			case	VLNK:
			case	VSOCK:
			case	VBAD:
			case	VFNWK:
			case	VEMPTYDIR:
				break;
#endif /* NOT_NECESSARY */
			default:
				break;
			}
			break;

		case	VDUX_CDFS_PV:
		case	VDUMMY: 
		case	VDUX_PV: 
		case	VDEV_VN: 
		case	VNFS_SPEC:
		case	VNFS_BDEV: 
		case	VNFS_FIFO: 
		default:
			i_number = -1; /**** ?????? ***/
			break;
		}
	}
        kd_locallookuppn->dev 		= dev;
        kd_locallookuppn->i_number 	= i_number;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_vfault(wrt, space, vaddr, prp)
int	wrt;
int	space;
caddr_t	vaddr;
preg_t	*prp;
{
        register struct kd_vfault *kd_vfault;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_VFAULT]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_vfault, KI_VFAULT);

        kd_vfault->cl_numb	= KI_VFAULTN;
        kd_vfault->wrt		= wrt;
        kd_vfault->space	= space;
        kd_vfault->vaddr	= vaddr;
        kd_vfault->r_nvalid	= prp->p_reg->r_nvalid;
        kd_vfault->prp		= prp;
        kd_vfault->pregion	= *prp;

#ifdef  __hp9000s800
	if (u.u_sstatep)
	{
        	kd_vfault->pcoq_head	= u.u_sstatep->ss_pcoq_head;
	} else
#endif  /* hp9000s800 */
	{
        	kd_vfault->pcoq_head	= NULL;
	}
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_pfault(wrt, space, vaddr, prp)
int	wrt;
int	space;
caddr_t	vaddr;
preg_t	*prp;
{
        register struct kd_vfault *kd_vfault;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_VFAULT]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_vfault, KI_VFAULT);

        kd_vfault->cl_numb	= KI_PFAULTN;
        kd_vfault->wrt		= wrt;
        kd_vfault->space	= space;
        kd_vfault->vaddr	= vaddr;
        kd_vfault->r_nvalid	= prp->p_reg->r_nvalid;
        kd_vfault->prp		= prp;
        kd_vfault->pregion	= *prp;

#ifdef  __hp9000s800
	if (u.u_sstatep)
	{
        	kd_vfault->pcoq_head	= u.u_sstatep->ss_pcoq_head;
	} else
#endif  /* hp9000s800 */
	{
        	kd_vfault->pcoq_head	= NULL;
	}
	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_region(rp, numb)
reg_t	*rp;
int	numb;
{
	register struct kd_region *kd_region;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_REGION]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_region, KI_REGION);

	kd_region->cl_numb	= numb;
	kd_region->rp		= rp;
	kd_region->region	= *rp;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_pregion(prp, numb)
preg_t	*prp;
int	numb;
{
	register struct kd_pregion *kd_pregion;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_PREGION]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_pregion, KI_PREGION);

	kd_pregion->cl_numb 	= numb;
	kd_pregion->prp		= prp;
	kd_pregion->pregion	= *prp;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_asyncpageio(bp)
struct  buf     *bp;
{
	register struct kd_asyncpageio *kd_asyncpageio;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_ASYNCPAGEIO]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_asyncpageio, KI_ASYNCPAGEIO);

        kd_asyncpageio->bp		= bp;	/* buffer ident */
        kd_asyncpageio->b_vp 		= (caddr_t)bp->b_rp;/* region of request */
	kd_asyncpageio->b_flags		= bp->b_flags;
	kd_asyncpageio->b_bcount	= bp->b_bcount;
	kd_asyncpageio->b_blkno		= bp->b_blkno;
	kd_asyncpageio->b_bptype	= bp->b_bptype; /* see buf.h */

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}

ki_memfree(rp, pfn)
reg_t	*rp;
int	pfn;
{
	register struct kd_memfree *kd_memfree;
	register int my_index = getprocindex();
        register 			reg_X;

	if (ki_kernelenable[KI_GETPRECTIME]) 
	{
        	ki_kernelcounts(my_index)[KI_MEMFREE]++;
	}
        if (ki_trace_sz == 0)   return;

	KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

        /* allocate trace buffer space */
        KI_data_mp(kd_memfree, KI_MEMFREE);

	kd_memfree->rp		= rp;
	kd_memfree->r_nvalid	= rp->r_nvalid;
	kd_memfree->pfn		= pfn;

	KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
}


/************************************************************************/
/*              The Measurement Interface (MI) system call              */
/************************************************************************/

ki_syscall()
{
        register error = 0;     /* error value on return to caller          */
        register 		reg_X;

        register struct a {     /* caller will have up to 4 params on stack */
                u_int parm0;/* NOTE: these must be unsigned for limit tests */
                u_int parm1;
                u_int parm2;
                u_int parm3;
        } *uap = (struct a *)u.u_ap;
	register int my_index = getprocindex();

        /* make sure process has permission (suser set u.u_error) */
        if ((uap->parm0 != KI_USER_TRACE) && (!suser())) return;

        switch(uap->parm0) {

/*****************************************************************************/
/* Allocate the kernel trace buffer memory                                   */
/* if ((tr_buf_sz = ki_call(KI_ALLOC_TRACEMEM, 32768)) < 0)                  */
/* where:     32768 = total kernel allocated memory for trace buffer         */
/*            tr_bf_sz = returned trace buffer size for KI_TRACE_GET (1/4th) */
/*****************************************************************************/
        case    KI_ALLOC_TRACEMEM:
                {
                register char *tbuf;
                register kk, jj;
		register int cpu;

                /* check if already initialized */
                if (ki_trace_sz) {
                        u.u_r.r_val1 = ki_trace_sz;
                        break;
                }

                /* trace buffer must be reasonable in size 16k to 1M ?? */
                if ((uap->parm1 < MIN_TBSZ) || (uap->parm1 > MAX_TBSZ)) {
                        error = EINVAL; /* requested size out of range */
                        break;
                }

                /* get word-aligned trace buffer max for caller */
                jj = (uap->parm1 / NUM_TBUFS) & (~3);
#ifdef	MP
                /* allocate the trace buffer for each cpu */
		for (cpu = 0; cpu < KI_MAX_PROCS; cpu++)
		{
		    if (mpproc_info[cpu].prochpa == 0) continue;

                    /* allocate the trace buffer */
                    tbuf = (char *)kmem_alloc(jj * NUM_TBUFS);
		    if (tbuf == NULL) {
#ifdef KI_DEBUG
			printf("sys_ki: tbuf is NULL\n");
#endif /* KI_DEBUG */
			while (cpu) {
			    cpu--;
		    	    if (mpproc_info[cpu].prochpa == 0) continue;

			    kmem_free(ki_tbuf(cpu)[0], jj * NUM_TBUFS);
			    ki_tbuf(cpu)[0] = NULL;
                	}
		        error = ENOMEM;
		        break; /* out of for */
		    }
		    ki_tbuf(cpu)[0] = tbuf;
#ifdef KI_DEBUG
		    printf("sys_ki: ki_tbuf[%d] = 0x%x\n", cpu, tbuf);
#endif /* KI_DEBUG */
		}
		if (error)
		    break;

		/* fill in the start of each quarter buffer for each cpu */
		for (cpu = 0; cpu < KI_MAX_PROCS; cpu++)
		{
        	    /* Skip if cpu is not configured */
		    if (mpproc_info[cpu].prochpa == 0) continue;

		    tbuf = ki_tbuf(cpu)[0];
                    /* fill in the trace buffer address */
                    for (kk = 0; kk < NUM_TBUFS; kk++) {
#ifdef KI_DEBUG
		    	printf("sys_ki: ki_tbuf[cpu = %d, bufno = %d] = 0x%x\n",
				cpu, kk, tbuf);
#endif /* KI_DEBUG */
                        ki_tbuf(cpu)[kk] = tbuf;
                        tbuf += jj;
		    }
                }
#else	/* ! MP */
                /* allocate the trace buffer */
		if ((tbuf = (char *)kmem_alloc(jj * NUM_TBUFS)) == NULL)
		{
		        error = ENOMEM;
			break;
		}
                for (kk = 0; kk < NUM_TBUFS; kk++) 
		{
                        ki_tbuf(0)[kk] = tbuf;
                        tbuf += jj;
		}
#endif	/* ! MP */

                /* return the real trace buffer max size to caller */
                u.u_r.r_val1 = ki_trace_sz = jj;

                break;
                }
/*****************************************************************************/
/* Deallocate the kernel trace buffer memory so another of different size    */
/* if (ki_call(KI_FREE_TRACEMEM)) < 0)                                       */
/* if errno == EINVAL  >> trace memory was not allocated                     */
/*****************************************************************************/
        case    KI_FREE_TRACEMEM:
                {
                register kk, jj;
		register int cpu;

                /* check if already initialized */
                if ((jj = ki_trace_sz) == 0) {
                        error = EINVAL; /* not initialized */
                        break;
                }
                /* TURN OFF trace memory allocator */
                ki_trace_sz = 0;

		/* free memory and zero out pointers for each cpu */
		for (cpu = 0; cpu < KI_MAX_PROCS; cpu++)
		{
#ifdef	MP
			/* Skip if cpu is not configured */
			if (mpproc_info[cpu].prochpa == 0) continue;
#endif	/* MP XXX */
                	/* free trace buffer */
			kmem_free(ki_tbuf(cpu)[0], jj * NUM_TBUFS);

                	/* clear out the trace buffer address and sizes */
                	for (kk = 0; kk < NUM_TBUFS; kk++) 
			{
                        	ki_tbuf(cpu)[kk] = NULL;
                        	ki_next(cpu)[kk] = 0;
                	}
			/* bump buffer number for each cpu */
                	ki_curbuf(cpu)++;
		}
                /* wakeup any process waiting - process will loose 
		 * that buffer and get an error return
		 */
                ki_wakeup_flag++;

                break;
                }
/*****************************************************************************/
/* Clear all the counters in the config table                                */
/* if (ki_call(KI_CONFIG_CLEAR) < 0)                                         */
/*****************************************************************************/
        case    KI_CONFIG_CLEAR:
                {
                ki_config_clear(); 
                break;
                }
/*****************************************************************************/
/* Allocate kernel memory for counter memory                                 */
/* if (ki_call(KI_ALLOCATE_CT, 1000) < 0)                                    */
/* where:     1000 = total kernel allocated memory for the counter table     */
/*****************************************************************************/
        case    KI_ALLOCATE_CT:
                {
                /* check if already initialized */
                if (ki_cbuf) {
                        error = EMFILE; /* well sort of close XXX */
                        break;
                }
                /* counter buffer must be reasonable in size 4 to 65kb ?? */
                if ((uap->parm1 < MIN_CTSZ) || (uap->parm1 > MAX_CTSZ)) {
                        error = EINVAL; /* requested size out of range */
                        break;
                }
                /* allocate counter buffer - allowed one time only per boot */
                if ((ki_cbuf = (char *)kmem_alloc(uap->parm1)) == NULL) {
                        error = ENOMEM; /* no kernel memory for requested size */
                        break;
                }
                /* clear out the memory for the counter buffer */
                bzero(ki_cbuf, uap->parm1);

                /* return the size of the buffer */
                ki_count_sz = uap->parm1;
                break;
                }
/*****************************************************************************/
/* Read/write kernel memory allocated for counter memory                     */
/* if (ki_call(KI_READ_CT, count_bf, count_sz, count_off) < 0)               */
/* if (ki_call(KI_WRITE_CT, count_bf, count_sz, count_off) < 0)              */
/* where:     count_bf = buffer to read into                                 */
/*            count_sz = number of bytes to read                             */
/*            count_off= starting offset to read                             */
/*****************************************************************************/
        case    KI_READ_CT:
        case    KI_WRITE_CT:
                {
                register count_sz = ki_count_sz;

                /* check request is in the counter buffer */
                if ((uap->parm2			> count_sz) ||
                    (uap->parm2			== 0)       ||
                    (uap->parm3			> count_sz) ||
                    ((uap->parm2 + uap->parm3)	>= count_sz))
                        { error = EINVAL; break; } 

                /* now move the data in/out to the counter memory */
                switch(uap->parm0) {
                case    KI_READ_CT:
                        error = copyout(ki_cbuf + uap->parm3, 
                                (caddr_t)uap->parm1, uap->parm2);
                        break;

                case    KI_WRITE_CT:
                        error = copyin((caddr_t)uap->parm1, 
                                ki_cbuf + uap->parm3, uap->parm2);
                        break;
                }
                u.u_r.r_val1 = uap->parm2;
                break;
                }
/*****************************************************************************/
/*           To enable process SYSTEM CALL traces/counters                   */
/*                                                                           */
/*   if (ki_call(KI_SET_SYSCALLTRACE, <system call numb>) < 0)               */
/*   if (ki_call(KI_SET_SYSCALLTRACE, SYS_FORK          ) < 0)               */
/*   if (ki_call(KI_SET_SYSCALLTRACE, SYS_READ          ) < 0)               */
/*   if (ki_call(KI_SET_SYSCALLTRACE, SYS_WRITE         ) < 0)               */
/*   if (ki_call(KI_SET_ALL_SYSCALTRACES           ) < 0)                    */
/*                                                                           */
/*  Where: SYS_FORK, SYS_READ and SYS_WRITE are parameters 2, 3 & 4          */
/*         See syscall.h, signal.h, init_sent.c                  XXX         */
/*                                                                           */
/*          To disable process SYSTEM CALL traces and counters               */
/*                                                                           */
/*   if (ki_call(KI_CLR_SYSCALLTRACE, <system call numb>) < 0)               */
/*   if (ki_call(KI_CLR_SYSCALLTRACE, SYS_FORK          ) < 0)               */
/*   if (ki_call(KI_CLR_SYSCALLTRACE, SYS_READ          ) < 0)               */
/*   if (ki_call(KI_CLR_SYSCALLTRACE, SYS_WRITE         ) < 0)               */
/*   if (ki_call(KI_CLR_ALL_SYSCALTRACES           ) < 0)                    */
/*                                                                           */
/*****************************************************************************/
        case    KI_SET_ALL_SYSCALTRACES: 
                {
                register kk;

                for (kk = 0; kk < KI_MAXSYSCALLS; kk++) {
                        /* Turn on all traces */
                        ki_syscallenable[kk] = (caddr_t)ki_traceinit[kk];
                }
#ifndef  __hp9000s300
                /* disable light-weight system calls */
                ki_disable_light_weight_system_calls();
#endif  /* __hp9000s300 */

                /* turn on system call tracing */
                ki_kernelenable[KI_SYSCALLS]  = 1;

                break;
                }
        case    KI_SET_SYSCALLTRACE:
                {
                if (uap->parm1 >= KI_MAXSYSCALLS) {
                        error = EINVAL; /* request size out of range */
                        break;
                }
                /* enable the requested tracing routine */
                ki_syscallenable[uap->parm1] = (caddr_t)ki_traceinit[uap->parm1];
#ifndef  __hp9000s300
                /* disable light-weight system calls */
                ki_disable_light_weight_system_calls();
#endif  /* __hp9000s300 */

                /* turn on system call tracing */
                ki_kernelenable[KI_SYSCALLS]  = 1;

                break;
                }
        case    KI_CLR_ALL_SYSCALTRACES: 
                {
                register kk;

                for (kk = 0; kk < KI_MAXSYSCALLS; kk++) {
                        /* Turn off all system call traces */
                        ki_syscallenable[kk] = (caddr_t)NULL;
                }

                break;
                }
        case    KI_CLR_SYSCALLTRACE:
                {
                if (uap->parm1 >= KI_MAXSYSCALLS) {
                        error = EINVAL; /* request size out of range */
                        break;
                }

                /* disable the tracing routine */
                ki_syscallenable[uap->parm1] = (caddr_t)NULL;

                break;
                }
/*****************************************************************************/
/*                     To enable KERNEL traces                               */
/*                                                                           */
/*   if (ki_call(KI_SET_KERNELTRACE, <kernel stub numb>) < 0)                */
/*   if (ki_call(KI_SET_KERNELTRACE, KI_GETPRECTIME,   ) < 0)                */
/*   if (ki_call(KI_SET_KERNELTRACE, KI_SWTCH,         ) < 0)                */
/*   if (ki_call(KI_SET_KERNELTRACE, KI_SYSCALLS       ) < 0)                */
/*   if (ki_call(KI_SET_ALL_KERNELTRACES               ) < 0)                */
/*                                                                           */
/*  Where: KI_GETPRECTIME, KI_SWTCH are kernel routines                      */
/*         KI_ALL_SYSKETRACE >> all kernel traces stubs                      */
/*         See ki_calls.h                                                    */
/*                                                                           */
/*                     To disable KERNEL traces                              */
/*                                                                           */
/*   if (ki_call(KI_CLR_KERNELTRACE, <kernel stub numb>) < 0)                */
/*   if (ki_call(KI_CLR_KERNELTRACE, KI_GETPRECTIME,   ) < 0)                */
/*   if (ki_call(KI_CLR_KERNELTRACE, KI_SWTCH,         ) < 0)                */
/*   if (ki_call(KI_CLR_KERNELTRACE, KI_SYSCALLS       ) < 0)                */
/*   if (ki_call(KI_CLR_ALL_KERNELTRACES               ) < 0)                */
/*                                                                           */
/* Note: OFF to ON transition of KI_GETPRECTIME will clear all time counters */
/*                                                                           */
/*****************************************************************************/
        case    KI_SET_ALL_KERNELTRACES:
                {
                register        kk;
#ifndef  __hp9000s300
                /* disable light-weight system calls */
                ki_disable_light_weight_system_calls();
#endif  /* __hp9000s300 */

                /* set all trace enable chars to true */
                for (kk = 1; kk < KI_MAXKERNCALLS; kk++) {
                        ki_kernelenable[kk] = 1;
                }
                break;
                }
        case    KI_SET_KERNELTRACE:
                {
                /* check if in range */
                if (uap->parm1 >= KI_MAXKERNCALLS) { 
                        error = EINVAL; /* request size out of range */
                        break;
                }
                switch(uap->parm1) 
		{
		case	KI_GETPRECTIME:
			/* reset timers if they were off  */
			if (ki_kernelenable[KI_GETPRECTIME] == 0) 
			{
				ki_reset_time();
			}
			break;

		case	KI_SYSCALLS:
#ifndef  __hp9000s300
			/* disable light-weight system calls */
			ki_disable_light_weight_system_calls();
#endif  /* __hp9000s300 */
			break;
		}
		ki_kernelenable[uap->parm1] = 1;
		break;
		}
	case	KI_CLR_ALL_KERNELTRACES:
		{
		register	kk;

		/* clear all trace enable chars */
                for (kk = 1; kk < KI_MAXKERNCALLS; kk++) {
			ki_kernelenable[kk] = 0;
		}
#ifndef  __hp9000s300
		/* enable light-weight system calls */
		ki_enable_light_weight_system_calls();
#endif  /* __hp9000s300 */

		break;
		}
	case	KI_CLR_KERNELTRACE:
		{
		/* check if in range */
		if (uap->parm1 >= KI_MAXKERNCALLS) {
			error = EINVAL; /* request size out of range */
			break;
		}
		/* clear the trace enable flag */
		ki_kernelenable[uap->parm1] = 0;
#ifndef  __hp9000s300
		/* enable light-weight system calls */
		if (uap->parm1 == KI_SYSCALLS)
			ki_enable_light_weight_system_calls();
#endif  /* __hp9000s300 */
		break;
		}
/*****************************************************************************/
/* Read the global configuration counter table from the kernel               */
/*         if (ki_call(KI_CONFIG_READ, &ki_cf, KI_CF) < 0)                   */
/* where:     &ki_cf = buffer for structure                                  */
/*            KI_CF  = sizeof(struct ki_config)                              */
/*****************************************************************************/
        case    KI_CONFIG_READ:
                {
                if (uap->parm2 != KI_CF) {
                        error = EINVAL;
                        break;
                }
#ifdef  __hp9000s300
                ki_nunit_per_sec = 250000;
#endif  /* __hp9000s300 */
		/* initialize this since its necessary for the daemon */
		ki_runningprocs = runningprocs;
                error = copyout(&ki_cf, (caddr_t)uap->parm1, KI_CF);

                u.u_r.r_val1 = KI_CF;
                break;
                }
/*****************************************************************************/
/* Set the max sleep timeout counter value for the MI_trace daemon           */
/* if (ki_call(KI_TIMEOUT_SET, 2*HZ) < 0)                                    */
/* where:     2 = max number of seconds trace daemon will wait for buffer    */
/*            Minimum of 200 milliseconds is allowed to avoid too much ovhd. */
/*            0 = (default) infinite (2.72 years) timeout value              */
/*****************************************************************************/
        case    KI_TIMEOUT_SET:
                {
                if ((uap->parm1) && (uap->parm1 < KI_MINTIMEOUT)) {
                        error = EINVAL;
                        break;
                }

                /* reset the timeout and count */
                ki_timeoutct =  ki_timeout = uap->parm1;

                break;
                }
/*****************************************************************************/
/* Trace daemon request to get the next requested trace buffer               */
/* if ((buf_sz = ki_call(KI_TRACE_GET, trace_buf, tr_bf_sz, &req_num)) < 0)  */
/* where:     trace_buf = buffer for trace data                              */
/*            tr_bf_sz  = size returned from KI_ALLOC_TRACEMEM               */
/*            buf_sz    = actual number of bytes returned (may be zero)      */
/*            req_num    = passed as requested buffer number                 */
/*            req_num    = returned as next buffer number to request         */
/*            if req_num =  0 then request the current trace buffer          */
/*            if req_num = -1 then request the oldest buffer still available */
/* if errno == ESPIPE then deamon was too late for the requested buffer      */
/* if errno == ENOMEM then deamon requested with a buffer size too small     */
/* if errno == EFAULT then deamon had illegal address in the call            */
/* if errno == ENXIO  then KI does not have an allocated trace buffer        */
/*****************************************************************************/
        case    KI_TRACE_GET:
                {
                register caddr_t        req_buf;
                register                req_sz;
                register                numtrys;
        	register 		reg_X;
                int                     bufr_numb;
                int                     x, i;
		struct ki_iovec		ki_iov[KI_MAX_PROCS];

                /* check if initialized */
                if (ki_trace_sz == 0) {
		    error = ENXIO;
		    break;
		}

                /* check callers buffer big enough size */
                if (uap->parm2 < ki_trace_sz) {
		    error = ENOMEM;
		    break;
		}

		if (error = copyin((caddr_t)uap->parm1, (caddr_t)ki_iov,
		            (unsigned)(KI_MAX_PROCS * sizeof (struct ki_iovec)))) break;

                /* check callers buffer big enough size */
                if (uap->parm2 < ki_trace_sz) { error = ENOMEM; break; }

                /* get requested buffer numb to be copied out */
                if (error = copyin((caddr_t)uap->parm3, &bufr_numb, sizeof(int))) break; 

                /* check if special GET requests */
                switch(bufr_numb) {
                case    -1:
                        /* give him a more trys if he fails */
                        numtrys = NUM_TBUFS;

                        /* return the oldest trace buffer available */
                        if ((bufr_numb = ki_curbuf(0) - (NUM_TBUFS-1)) < 0)
                                bufr_numb = 0;
                        break;

                case     0:
                        numtrys = NUM_TBUFS;

                        /* if get current buffer set to current buffer number */
                        bufr_numb = ki_curbuf(0);

                        break;

                default:
                        /* dont give him more trys if he fails */
                        numtrys = 1;
                        break;
                }

                for(;;) 
		{
                        /* - if request is current buffer, wait until full
			 * [its ok to only check zero since even if 
			 *  zero were not full, somebody else will
			 *  wake us up when their buffer is full]
			 */
                        while (bufr_numb == ki_curbuf(0)) 
			    sleep(&ki_cf, PZERO+1);
        
#ifdef KI_DEBUG
			printf("sys_ki: bufr_num = %d, cur_buf = %d\n",
				   bufr_numb, ki_curbuf(0));
#endif /* KI_DEBUG */
        
			for (i = 0; i < KI_MAX_PROCS; i++)
			{
			    ki_iov[i].iov_len = 0;
#ifdef	MP
        		    /* Skip if cpu is not configured */
			    if (mpproc_info[i].prochpa == 0) continue;
#endif	/* MP XXX */
                            /* get length and address of requested buffer */
                            req_sz  = ki_next(i)[bufr_numb & (NUM_TBUFS-1)];
                            req_buf = ki_tbuf(i)[bufr_numb & (NUM_TBUFS-1)];

#ifdef KI_DEBUG
			    ki_size(req_sz, req_buf, i);
#endif /* KI_DEBUG */
                        /* move trace buffer into userland */
                        if (req_sz) {
#ifdef KI_DEBUG
		printf("Trying to Copyout buf %d to addr 0x%x size %d\n",
					bufr_numb, ki_iov[i].iov_base, req_sz);
#endif /* KI_DEBUG */
                                if (error = copyout(req_buf, 
                                        (caddr_t)ki_iov[i].iov_base, req_sz))
					break;

#ifdef KI_DEBUG
				printf("Copyout Done buf %d to addr 0x%x size %d\n",
					bufr_numb, ki_iov[i].iov_base, req_sz);
#endif /* KI_DEBUG */

			  	ki_iov[i].iov_len = req_sz;
                            } /* if */
			} /* for */

                        /* check if the copied out buffer is still around */
                        if ((u_int)(ki_curbuf(0) - bufr_numb) < NUM_TBUFS) {
                                /* yes, return next number to user */
                                bufr_numb++;
                                break; /* out of infinite for */
                        }
                        /* no, check if we have another try */
                        if (--numtrys) {
                                /* missed the buffer, try next one if allowed */
                                bufr_numb++;
                                continue; /* for */
                        } else {
                                /* we missed it, return current oldest buffer */
                                bufr_numb = ki_curbuf(0) - (NUM_TBUFS-1);
                                error = ESPIPE; /* well sort of close XXX */
                                break; /* out of infinite for */
                        }
                } /* for */

                /* return bufr_numb buffer number for next time */
                /* no error check because checked above with copyin */
                copyout(&bufr_numb, (caddr_t)uap->parm3, sizeof(int));
		copyout((caddr_t)ki_iov, (caddr_t)uap->parm1,
		            (unsigned)(KI_MAX_PROCS * sizeof (struct ki_iovec)));

                /* return number of cpus or iov's valid */
                u.u_r.r_val1 = runningprocs;
#ifdef KI_DEBUG
		printf("sys_ki: Done KI_TRACE_GET for buf = %d\n", bufr_numb);
#endif /* KI_DEBUG */
                break;
                }
/*****************************************************************************/
/* Allow a user process to force a trace buffer change (wakeup trace daemon) */
/*         if (ki_call(KI_USER_TRACE) < 0)           */
/*****************************************************************************/
        case    KI_TRACE_SWITCH:
                {
                /* check if tracing enabled */
                if (ki_trace_sz != 0) 
		{
			KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
			ki_data(0);
			KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);
		}
		/* hardclock on monarch will wakeup sleepers */

                break;
                }
/*****************************************************************************/
/* Allow a user process to put a trace structure into the trace buffer       */
/*         if (ki_call(KI_USER_TRACE, trace_buf, trace_len) < 0)             */
/* where:     trace_buf = pointer to structure to put into trace buffer      */
/*            trace_len = length of structure                                */
/* if errno == ENXIO  then KI trace is not enabled                           */
/* if errno == ENOMEM then KI does not have an allocated trace buffer        */
/*****************************************************************************/
        case    KI_USER_TRACE:
                {
                char            ki_my_data[MAX_TRACE_REC];
		struct	kd_userproc	*kd_userproc;

                register unsigned stru_sz;

                /* get the structure size */
                stru_sz = uap->parm2;

                /* check if user structure is too big */
                if (stru_sz > MAX_TRACE_REC) {
                        error = ENOMEM; 
                        break;
                }
                if (ki_kernelenable[KI_USERPROC] == 0) {
                	error = EINVAL;
                	break;
                }
		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
       			ki_kernelcounts(my_index)[KI_USERPROC]++;
		}
                u.u_r.r_val1 = 0;

                /* check if initialized */
                if (ki_trace_sz == 0) 
		{
			error = ENXIO;
			break;
		}
                /* get the user supplied trace structure 
		 * - copy it in first since copyin may sleep
		 */
                if (error = copyin((caddr_t)uap->parm1, ki_my_data, stru_sz))
			break;

		KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

                kd_userproc = (struct kd_userproc *)
                        ki_data(stru_sz + sizeof(struct kd_userproc));
                kd_userproc->kd_recid      = KI_USERPROC;
                ki_getprectime6(&kd_userproc->kd_struct.cur_time);
        	/* initialize trace buffer with current time parameters */
		bcopy(ki_timesstruct(my_index), 
			kd_userproc->ks_swt, 
			sizeof(struct ki_runtimes) * KT_NUMB_CLOCKS);
		bcopy(ki_my_data, kd_userproc+1, stru_sz);

                /* return number of buffer it was placed in */
                u.u_r.r_val1 = ki_curbuf(0);

		KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

                break;
		}

/*****************************************************************************/
/* Allow a user process to put a trace structure into the trace buffer       */
/*         if (ki_call(KI_PROC_TRACE, trace_buf, trace_len, trace_id) < 0)   */
/* where:     trace_buf = pointer to structure to put into trace buffer      */
/*            trace_len = length of structure                                */
/*            trace_id  = ID to be put into trace buffer for trace daemon    */
/* if errno == ENXIO  then KI does not have an allocated trace buffer        */
/*****************************************************************************/
        case    KI_PROC_TRACE:
                {
                char            ki_my_data[MAX_TRACE_REC];
                struct kd_trace {
                        struct kd_struct kd_struct;
                        char             data[MAX_TRACE_REC];
                } *kd_trace; 

                register unsigned stru_sz, record_id;

                /* get the structure size */
                stru_sz = uap->parm2;

                /* check if user structure is too big */
                if (stru_sz > MAX_TRACE_REC) {
                        error = ENOMEM; 
                        break;
                }
                /* check if KI_SUIDPROC call is enabled */
                if (ki_kernelenable[KI_SUIDPROC] == 0) {
			error = EINVAL;
			break;
		}
		if (ki_kernelenable[KI_GETPRECTIME]) 
		{
			ki_kernelcounts(my_index)[KI_SUIDPROC]++;
		}
		/* simulate any type of record desired */
		record_id = uap->parm3;

                u.u_r.r_val1 = 0;

                /* check if initialized */
                if (ki_trace_sz == 0) 
		{
			error = ENXIO;
			break;
		}
                /* get the user supplied trace structure 
		 * - copy it in first since copyin may sleep
		 */
                if (error = copyin((caddr_t)uap->parm1, ki_my_data, stru_sz))
			break;

		KI_CRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

                kd_trace = (struct kd_trace *)
                        ki_data(stru_sz + sizeof(struct kd_struct));
                kd_trace->kd_recid      = record_id;
                ki_getprectime6(&kd_trace->kd_struct.cur_time);
		bcopy(ki_my_data, kd_trace->data, stru_sz);

                /* return number of buffer it was placed in */
                u.u_r.r_val1 = ki_curbuf(0);

		KI_UNCRIT_REPLACEMENT(ki_proc_lock[my_index], reg_X);

                break;
                }
/*****************************************************************************/
/* Allow a user process to get a kernel structure quickly                    */
/*         if (ki_call(KI_KMEM_GET, kern_struct, length, kern_addr) < 0)     */
/* where:     kern_struct = pointer to return structure                      */
/*            length      = length of structure                              */
/*            kern_addr   = kernel address of the structure                  */
/*****************************************************************************/
        case    KI_KMEM_GET:
#ifdef  __hp9000s300
                {
                /* should check for I/O space access */
                if (!kernacc((caddr_t)uap->parm3, uap->parm2, B_READ)) {
                        error = EFAULT;
                        break;
                }
                /* now copy out the structure */
                error = copyout((caddr_t)uap->parm3, 
                        (caddr_t)uap->parm1, uap->parm2);

                /* return number of bytes copyied */
                u.u_r.r_val1 = uap->parm2;
                break;
                }
#else   /* __hp9000s300 */
                {
                /*
                 * When reading kernel memory on the Series 800,
                 * cache flushing and kernel preemption must be
                 * taken into account.  Rather than replicate the
                 * complexities of the kmem driver here, a call
                 * is made to it.  This should still perform
                 * better than /dev/kmem because the lseek(2) is
                 * combined with the read(2).
                 */

                dev_t           dev;
                struct iovec    iov;
                struct uio      uio;

                /* check length a la read(2) */
                if ((int)uap->parm2 < 0) {
                        error = EINVAL;
                        break;
                }

                /* build dev_t for kmem driver */
                dev = makedev(MM_MAJOR, MM_KMEM);

                /* build iovec to user's buffer */
                iov.iov_base = (caddr_t)uap->parm1;
                iov.iov_len = uap->parm2;

                /* build uio structure for the i/o */
                uio.uio_iov = &iov;
                uio.uio_iovcnt = 1;
                uio.uio_offset = uap->parm3;
                uio.uio_seg = UIOSEG_USER;
                uio.uio_resid = uap->parm2;
                uio.uio_fpflags = FREAD;

                /* now call kmem driver to actually do the i/o */
                if (!(error = mm_read(dev, &uio))) {

                        /* return number of bytes copied */
                        u.u_r.r_val1 = uap->parm2 - uio.uio_resid;
                }
                break;
                }
#endif  /* __hp9000s300 */

        default:
                error = EINVAL;
                break;
        }
        if (error) u.u_error = error;
}
#ifndef  __hp9000s300
ki_disable_light_weight_system_calls()
{
        register int s;

        /* disable light-weight system calls */

        s = spl7();
        lw_syscall_off |= LW_SYSCALL_OFF_KI;
        splx(s);
}

ki_enable_light_weight_system_calls()
{
        register int s;

        /* enable light-weight system calls */

        s = spl7();
        lw_syscall_off &= (~LW_SYSCALL_OFF_KI);
        splx(s);
}
#endif  /* __hp9000s300 */

#ifdef KI_DEBUG
ki_test()
{

}

ki_size(req_sz, req_buf, proc)
int req_sz;
caddr_t req_buf;
int proc;
{
    printf("sys_ki: req_sz = %d req_buf = 0x%x proc = %d\n",
            req_sz, req_buf, proc);
}
#endif /* KI_DEBUG */
#endif	/* FSD_KI */
