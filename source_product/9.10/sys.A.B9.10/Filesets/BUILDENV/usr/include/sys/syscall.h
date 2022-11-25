/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/syscall.h,v $
 * $Revision: 1.46.83.7 $       $Author: drew $
 * $State: Exp $        $Locker:  $
 * $Date: 94/11/10 11:41:26 $
 */
/*
 * @(#)syscall.h: $Revision: 1.46.83.7 $ $Date: 94/11/10 11:41:26 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_SYSCALL_INCLUDED
#define _SYS_SYSCALL_INCLUDED

#ifdef _KERNEL
/*
 * A pair of tutorials appear in sys/init_sent.c, one on adding system
 * calls, one on deleting them.  PLEASE READ THEM BEFORE MAKING CHANGES
 * TO THIS FILE OR sys/init_sent.c.
 */
#endif /* _KERNEL */

#define	SYS_NOTSYSCALL		0
#ifdef	_KERNEL
/* The following values are used internally by the kernel.       */
/* They should not overlap with any of the SYS_xxx values above. */
#define	SYS_SIGCLEANUP	139   	/* needed for commands */
#endif	/* _KERNEL */


#ifndef	_XPG2

#define	SYS_EXIT	1
#define	SYS_exit	1
#define	SYS_FORK	2
#define	SYS_fork	2
#define	SYS_READ	3
#define	SYS_read	3
#define	SYS_WRITE	4
#define	SYS_write	4
#define	SYS_OPEN	5
#define	SYS_open	5
#define	SYS_CLOSE	6
#define	SYS_close	6
#define	SYS_WAIT	7
#define	SYS_wait	7
#define	SYS_CREAT	8
#define	SYS_creat	8
#define	SYS_LINK	9
#define	SYS_link	9
#define	SYS_UNLINK	10
#define	SYS_unlink	10
#define	SYS_EXECV	11
#define	SYS_execv	11
#define	SYS_CHDIR	12
#define	SYS_chdir	12
#define	SYS_TIME	13
#define	SYS_time	13
#define	SYS_MKNOD	14
#define	SYS_mknod	14
#define	SYS_CHMOD	15
#define	SYS_chmod	15
#define	SYS_CHOWN	16
#define	SYS_chown	16
#define SYS_BRK		17
#define SYS_brk		17
				/* 18 is old: stat */
#define	SYS_LSEEK	19
#define	SYS_lseek	19
#define	SYS_GETPID	20
#define	SYS_getpid	20
#define	SYS_MOUNT	21
#define	SYS_mount	21
#define	SYS_UMOUNT	22
#define	SYS_umount	22
#define	SYS_SETUID	23
#define	SYS_setuid	23
#define	SYS_GETUID	24
#define	SYS_getuid	24
#define	SYS_STIME	25
#define	SYS_stime	25
#define	SYS_PTRACE	26
#define	SYS_ptrace	26
#define	SYS_ALARM	27
#define	SYS_alarm	27
#ifdef __hp9000s300
#define	SYS_OFSTAT	28
#define	SYS_ofstat	28
#endif
#ifdef __hp9000s800
				/* 28 is old: fstat */
#endif
#define	SYS_PAUSE	29
#define	SYS_pause	29
#define	SYS_UTIME	30
#define	SYS_utime	30
#define SYS_STTY	31	/* 31 is old: stty */
#define SYS_stty	31
#define SYS_GTTY	32	/* 32 is old: gtty */
#define SYS_gtty	32
#define	SYS_ACCESS	33
#define	SYS_access	33
#define	SYS_NICE	34
#define	SYS_nice	34
#define	SYS_FTIME	35
#define	SYS_ftime	35
#define	SYS_SYNC	36
#define	SYS_sync	36
#define	SYS_KILL	37
#define	SYS_kill	37
#define	SYS_STAT	38
#define	SYS_stat	38
#define	SYS_SETPGRP	39
#define	SYS_setpgrp	39
#define	SYS_LSTAT	40
#define	SYS_lstat	40
#define	SYS_DUP		41
#define	SYS_dup		41
#define	SYS_PIPE	42
#define	SYS_pipe	42
#define	SYS_TIMES	43
#define	SYS_times	43
#define	SYS_PROFIL	44
#define	SYS_profil	44
#define	SYS_KI_CALL	45
#define	SYS_ki_call	45
#define	SYS_SETGID	46
#define	SYS_setgid	46
#define	SYS_GETGID	47
#define	SYS_getgid	47
				/* 48 is old: sigsys */
				/* 49 is unused (reserved for USG) */
				/* 50 reserved for USG */
#define	SYS_ACCT	51
#define	SYS_acct	51
				/* 52 is old: phys */
				/* 53 is old: syslock */
#define	SYS_IOCTL	54
#define	SYS_ioctl	54
#define	SYS_REBOOT	55
#define	SYS_reboot	55
#define	SYS_SYMLINK	56
#define	SYS_symlink	56
#define	SYS_UTSSYS	57
#define	SYS_utssys	57
#define	SYS_READLINK	58
#define	SYS_readlink	58
#define	SYS_EXECVE	59
#define	SYS_execve	59
#define	SYS_UMASK	60
#define	SYS_umask	60
#define	SYS_CHROOT	61
#define	SYS_chroot	61
#define	SYS_FCNTL	62
#define	SYS_fcntl	62
#define	SYS_ULIMIT	63
#define	SYS_ulimit	63
				/* 64 = 4.2 getpagesize */
				/* 65 = 4.2 mremap */
#define SYS_VFORK	66
#define SYS_vfork	66
				/* 67 = old vread */
				/* 68 = old vwrite */
				/* 69 = sbrk USE ENTRY #17 INSTEAD */
				/* 70 = sstk */
#define SYS_MMAP	71
#define SYS_mmap	71
				/* 72 = old vadvise */
#define SYS_MUNMAP	73
#define SYS_munmap	73
#define SYS_MPROTECT	74
#define SYS_mprotect	74
#define SYS_MADVISE	75
#define SYS_madvise	75
#define SYS_VHANGUP	76
#define SYS_vhangup	76
#define	SYS_SWAPOFF	77	/* 77 is new: swapoff*/
#define	SYS_swapoff	77
				/* 78 = mincore */
#define	SYS_GETGROUPS	79
#define	SYS_getgroups	79
#define	SYS_SETGROUPS	80
#define	SYS_setgroups	80
#define	SYS_GETPGRP2	81
#define	SYS_getpgrp2	81
#define	SYS_SETPGRP2	82	/* setpgid and setpgrp2 */
#define	SYS_setpgrp2	82	/* share the same kernel */
#define	SYS_SETPGID	82	/* entry point */
#define	SYS_setpgid	82
#define	SYS_SETITIMER	83
#define	SYS_setitimer	83
#define	SYS_wait3	84	/* 84 is new HPUX wait3 */
#define	SYS_WAIT3	84	/* 84 is new HPUX wait3 */
#define	SYS_SWAPON	85
#define	SYS_swapon	85
#define	SYS_GETITIMER	86
#define	SYS_getitimer	86
				/* 87 = 4.2 gethostname */
				/* 88 = 4.2 sethostname */
				/* 89 = getdtablesize */
#define	SYS_DUP2	90
#define	SYS_dup2	90
				/* 91 = getdopt */
#define	SYS_FSTAT	92
#define	SYS_fstat	92
#define	SYS_SELECT	93
#define	SYS_select	93
				/* 94 = setdopt */
#define	SYS_FSYNC	95
#define	SYS_fsync	95
#define SYS_SETPRIORITY	96
#define SYS_setpriority	96
				/* 97 was SYS_socket	*/
				/* 98 was SYS_connect	*/
				/* 99 was SYS_accept	*/
#define SYS_GETPRIORITY	100
#define SYS_getpriority	100
				/* 101 was SYS_send	*/
				/* 102 was SYS_recv	*/
				/* 103 was socketaddr 	*/
				/* 104 was SYS_bind 	*/
				/* 105 was SYS_setsockopt */
				/* 106 was SYS_listen	*/
				/* 107 was vtimes 	*/
#define	SYS_SIGVECTOR	108
#define	SYS_sigvector	108
#define	SYS_SIGBLOCK	109
#define	SYS_sigblock	109
#define	SYS_SIGSETMASK	110
#define	SYS_sigsetmask	110
#define	SYS_SIGPAUSE	111
#define	SYS_sigpause	111
#define	SYS_SIGSTACK	112
#define	SYS_sigstack	112
				/* 113 was SYS_recvmsg	*/
				/* 114 was SYS_sendmsg	*/
				/* 115 is old vtrace */
#define	SYS_GETTIMEOFDAY 116
#define	SYS_gettimeofday 116
#define	SYS_GETRUSAGE	 117
#define	SYS_getrusage	 117
				/* 118 was SYS_getsockopt */
				/* 119 is old resuba */
#define	SYS_READV	120
#define	SYS_readv	120
#define	SYS_WRITEV	121
#define	SYS_writev	121
#define	SYS_SETTIMEOFDAY 122
#define	SYS_settimeofday 122
#define	SYS_FCHOWN	123
#define	SYS_fchown	123
#define	SYS_FCHMOD	124
#define	SYS_fchmod	124
				/* 125 was SYS_recvfrom	*/
#define	SYS_SETRESUID	126
#define	SYS_setresuid	126
#define	SYS_SETRESGID	127
#define	SYS_setresgid	127
#define SYS_RENAME	128
#define SYS_rename	128
#define	SYS_TRUNCATE	129
#define	SYS_truncate	129
#define	SYS_FTRUNCATE	130
#define	SYS_ftruncate	130
				/* 131 was flock */
#define SYS_SYSCONF     132
#define SYS_sysconf     132
				/* 133 was SYS_sendto	*/
				/* 134 was SYS_shutdown	*/
				/* 135 was SYS_socketpair */
#define	SYS_MKDIR	136
#define	SYS_mkdir	136
#define	SYS_RMDIR	137
#define	SYS_rmdir	137
				/* 138 was utimes */
				/* 139 is sigcleanup */
#define SYS_SETCORE     140
#define SYS_setcore	140
				/* 141 was SYS_getpeername */
#define	SYS_GETHOSTID	142
#define	SYS_gethostid	142
#define	SYS_SETHOSTID	143
#define	SYS_sethostid	143
#define SYS_getrlimit   144
#define SYS_GETRLIMIT   144
#define SYS_SETRLIMIT   145
#define SYS_setrlimit   145
				/* 146 was killpg */
#ifdef  __hp9000s300		/* S800 doesn't need a syscall for this */
#define SYS_CACHECTL    147
#define SYS_cachectl    147
#endif
#ifdef	__hp9000s800
				/* 147 is unused (cachectl for Series 300) */
#endif
#define	SYS_QUOTACTL	148
#define	SYS_quotactl	148
#ifdef	__hp9000s300
				/* 149 unused */
#endif
#ifdef	__hp9000s800
#define SYS_get_sysinfo  149
#define SYS_GET_SYSINFO  149
#endif
				/* 149 unused */
				/* 150 was SYS_getsockname */
#define	SYS_PRIVGRP	151
#define	SYS_privgrp	151
#define SYS_RTPRIO	152
#define SYS_rtprio	152
#define SYS_PLOCK	153
#define SYS_plock	153
#ifdef	__hp9000s300
#define SYS_NETIOCTL	154	/* historically different from s800 */
#define SYS_netioctl	154
#endif
#ifdef	__hp9000s800
				/* 154 is unused */
#endif
#define SYS_LOCKF	155
#define SYS_lockf	155
#define	SYS_SEMGET	156
#define	SYS_semget	156
				/* 157 was semctl, now osemctl */
#define	SYS_SEMOP	158
#define	SYS_semop	158
#define	SYS_MSGGET	159
#define	SYS_msgget	159
				/* 160 was msgctl, now omsgctl */
#define	SYS_MSGSND	161
#define	SYS_msgsnd	161
#define	SYS_MSGRCV	162
#define	SYS_msgrcv	162
#define	SYS_SHMGET	163
#define	SYS_shmget	163
				/* 164 was shmctl, now oshmctl */
#define	SYS_SHMAT	165
#define	SYS_shmat	165
#define	SYS_SHMDT	166
#define	SYS_shmdt	166
				/* 167 was m68020_advise */
/*
 * The numbers 168 through 189 are for Discless/DUX Use.
 */

#define SYS_csp		168
#define SYS_CSP		168
#define SYS_nsp_init	168
#define SYS_NSP_INIT	168
#define SYS_cluster	169
#define SYS_CLUSTER	169
#define SYS_mkrnod	170
#define SYS_MKRNOD	170
#define SYS_test	171
#define SYS_TEST	171
#define SYS_unsp_open	172
#define SYS_UNSP_OPEN	172
				/* 173 is unused */
#define SYS_getcontext	174
#define SYS_GETCONTEXT	174
#define SYS_setcontext	175
#define SYS_SETCONTEXT	175
#define SYS_bigio	176
#define SYS_BIGIO	176
#define SYS_pipenode	177
#define SYS_PIPENODE	177
#define SYS_lsync	178
#define SYS_LSYNC	178
#define	SYS_getmachineid 179
#define	SYS_GETMACHINEID 179
#define SYS_CNODEID	180
#define SYS_cnodeid	180
#define SYS_MYSITE	180
#define SYS_mysite	180
#define SYS_CNODES	181
#define SYS_cnodes	181
#define SYS_SITELS	181
#define SYS_sitels	181
#define SYS_swapclients 182
#define SYS_SWAPCLIENTS 182
#define SYS_rmtprocess	183
#define SYS_RMTPROCESS	183
#define SYS_dskless_stats 184
#define SYS_DSKLESS_STATS 184

/*
 * Series 300 and Series 800 syscall numbers diverge
 * significantly for 185 - 199.
 */

#ifdef __hp9000s300
				/* 185 used for Northrop special */
#define SYS_SETACL	186
#define SYS_setacl	186
#define SYS_FSETACL	187
#define SYS_fsetacl	187
#define SYS_GETACL	188
#define SYS_getacl	188
#define SYS_FGETACL	189
#define SYS_fgetacl	189
#define SYS_GETACCESS	190
#define SYS_getaccess	190
#define SYS_GETAUDID	191
#define SYS_getaudid	191
#define SYS_SETAUDID	192
#define SYS_setaudid	192
#define SYS_GETAUDPROC	193
#define SYS_getaudproc	193
#define SYS_SETAUDPROC	194
#define SYS_setaudproc	194
#define SYS_GETEVENT	195
#define SYS_getevent	195
#define SYS_SETEVENT	196
#define SYS_setevent	196
#define SYS_AUDWRITE	197
#define SYS_audwrite	197
#define SYS_AUDSWITCH	198
#define SYS_audswitch	198
#define SYS_AUDCTL	199
#define SYS_audctl	199
#endif
#ifdef	__hp9000s800
#define	SYS_SIGPROCMASK	185
#define	SYS_sigprocmask	185
#define	SYS_SIGPENDING	186
#define	SYS_sigpending	186
#define	SYS_SIGSUSPEND	187
#define	SYS_sigsuspend	187
#define	SYS_SIGACTION	188
#define	SYS_sigaction	188
				/* 189 unused */
#define SYS_NFSSVC     190
#define SYS_nfssvc     190
#define SYS_GETFH       191
#define SYS_getfh       191
#define SYS_GETDOMAINNAME 192
#define SYS_getdomainname 192
#define SYS_SETDOMAINNAME 193
#define SYS_setdomainname 193
#define SYS_ASYNC_DAEMON  194
#define SYS_async_daemon  194
#define SYS_GETDIRENTRIES 195
#define SYS_getdirentries 195
#define SYS_STATFS      196
#define SYS_statfs	196
#define SYS_FSTATFS     197
#define	SYS_fstatfs	197
#define SYS_VFSMOUNT    198
#define SYS_vfsmount    198
				/* 199 unused */
#endif

#define SYS_WAITPID     200
#define SYS_waitpid     200
				/* 201 was SYS_netunam, (RFA) gone in 8.0 */
				/* 202 was SYS_netioctl, for s800 */
				/* 203 was SYS_ipccreate */
				/* 204 was SYS_ipcname */
				/* 205 was SYS_ipcnamerase */
				/* 206 was SYS_ipclookup */
				/* 207 was SYS_ipcsendto */
				/* 208 was SYS_ipcrecvfrom */
				/* 209 was SYS_ipcconnect */
				/* 210 was SYS_ipcrecvcn */
				/* 211 was SYS_ipcsend */
				/* 212 was SYS_ipcrecv */
				/* 213 was SYS_ipcgive */
				/* 214 was SYS_ipcget */
				/* 215 was SYS_ipccontrol */
				/* 216 was SYS_ipcsendreq */
				/* 217 was SYS_ipcrecvreq */
				/* 218 was SYS_ipcsendreply */
				/* 219 was SYS_ipcrecvreply */
				/* 220 was SYS_ipcshutdown */
				/* 221 was SYS_ipcdest */
				/* 222 was SYS_ipcmuxconnect */
				/* 223 was SYS_ipcmuxrecv */

/*
 * Series 300 and Series 800 syscall numbers diverge
 * significantly for 224 - 250.
 */

#ifdef __hp9000s300
#define SYS_set_no_trunc 224
#define SYS_SET_NO_TRUNC 224
#define SYS_PATHCONF	225
#define SYS_pathconf	225
#define SYS_FPATHCONF	226
#define SYS_fpathconf	226
				/* 227 unused */
				/* 228 unused */
#define SYS_ASYNC_DAEMON 229
#define SYS_async_daemon 229
#define SYS_NFS_FCNTL	230
#define SYS_nfs_fcntl	230
#define SYS_GETDIRENTRIES 231
#define SYS_getdirentries 231
#define SYS_GETDOMAINNAME 232
#define SYS_getdomainname 232
#define SYS_GETFH	233
#define SYS_getfh	233
#define	SYS_VMOUNT	234
#define SYS_VFSMOUNT	234
#define SYS_vfsmount	234
#define SYS_NFSSVC	235
#define SYS_nfssvc	235
#define SYS_SETDOMAINNAME 236
#define SYS_setdomainname 236
#define	SYS_STATFS	237
#define	SYS_statfs	237
#define	SYS_FSTATFS	238
#define	SYS_fstatfs	238
#define	SYS_SIGACTION	239
#define	SYS_sigaction	239
#define	SYS_SIGPROCMASK	240
#define	SYS_sigprocmask	240
#define	SYS_SIGPENDING	241
#define	SYS_sigpending	241
#define	SYS_SIGSUSPEND	242
#define	SYS_sigsuspend	242
#define SYS_fsctl	243
#define SYS_FSCTL	243
				/* 244 unused */
#define SYS_pstat	245
#define SYS_PSTAT	245
				/* 246 unused */
				/* 247 unused */
				/* 248 unused */
				/* 249 unused */
				/* 250 unused */
#endif
#ifdef	__hp9000s800
#define	SYS_SIGSETRETURN 224
#define	SYS_sigsetreturn 224
#define	SYS_SIGSETSTATEMASK 225
#define	SYS_sigsetstatemask 225
#define	SYS_BFACTL	226
#define	SYS_bfactl	226
#define SYS_CS		227
#define SYS_cs		227
#define SYS_CDS		228
#define SYS_cds		228
				/* 229 is old: mtctl */
#define SYS_pathconf	230
#define SYS_PATHCONF	230
#define SYS_fpathconf	231
#define SYS_FPATHCONF	231
				/* 232 is old: rsm */
				/* 233 is old: mtsm */
#define	SYS_NFS_FCNTL	234
#define	SYS_nfs_fcntl	234
#define SYS_GETACL	235
#define SYS_getacl	235
#define SYS_FGETACL	236
#define SYS_fgetacl	236
#define SYS_SETACL	237
#define SYS_setacl	237
#define SYS_FSETACL	238
#define SYS_fsetacl	238
#define	SYS_PSTAT	239
#define	SYS_pstat	239
#define SYS_GETAUDID	240
#define SYS_getaudid	240
#define SYS_SETAUDID	241
#define SYS_setaudid	241
#define SYS_GETAUDPROC	242
#define SYS_getaudproc	242
#define SYS_SETAUDPROC	243
#define SYS_setaudproc	243
#define SYS_GETEVENT	244
#define SYS_getevent	244
#define SYS_SETEVENT	245
#define SYS_setevent	245
#define SYS_AUDWRITE	246
#define SYS_audwrite	246
#define SYS_AUDSWITCH	247
#define SYS_audswitch	247
#define SYS_AUDCTL	248
#define SYS_audctl	248
#define SYS_GETACCESS	249
#define SYS_getaccess	249
#define SYS_FSCTL	250
#define SYS_fsctl	250
#endif

/*
 * Series 300 and Series 800 syscall numbers 251 and up 
 * are converged for the most part.
 */

#define SYS_ULCONNECT	251
#define SYS_ulconnect	251
#define SYS_ULCONTROL	252
#define SYS_ulcontrol	252
#define SYS_ULCREATE	253
#define SYS_ulcreate	253
#define SYS_ULDEST	254
#define SYS_uldest	254
#define SYS_ULRECV	255
#define SYS_ulrecv	255
#define SYS_ULRECVCN	256
#define SYS_ulrecvcn	256
#define SYS_ULSEND	257
#define SYS_ulsend	257
#define SYS_ULSHUTDOWN	258
#define SYS_ulshutdown	258
#define SYS_SWAPFS	259
#define SYS_swapfs	259
#ifdef	__hp9000s800		/* FSS implemented only on Series 800 */
#define SYS_FSS		260
#define SYS_fss		260
#endif
#ifdef	__hp9000s300
				/* 260 is FSS for Series 800 */
#endif
				/* 261 was pstat_static_sys */
				/* 262 was pstat_dynamic_sys */
				/* 263 was pstat_count */
				/* 264 was pstat_pidlist */
				/* 265 was pstat_set_command */
				/* 266 was pstat_status */
#define SYS_TSYNC	267
#define SYS_tsync	267
#define SYS_GETNUMFDS	268
#define SYS_getnumfds	268
#define SYS_POLL	269
#define SYS_poll	269
#define SYS_GETMSG	270
#define SYS_getmsg	270
#define SYS_PUTMSG	271
#define SYS_putmsg	271
#define SYS_FCHDIR	 272
#define SYS_fchdir	 272

#define	SYS_GETMOUNT_CNT 273
#define	SYS_getmount_cnt 273
#define	SYS_GETMOUNT_ENTRY 274
#define	SYS_getmount_entry 274

/* New networking system calls */

#define	SYS_ACCEPT	275
#define	SYS_accept	275
#define	SYS_BIND	276
#define	SYS_bind	276
#define	SYS_CONNECT	277
#define	SYS_connect	277
#define	SYS_GETPEERNAME	278
#define	SYS_getpeername	278
#define	SYS_GETSOCKNAME	279
#define	SYS_getsockname	279
#define	SYS_GETSOCKOPT	280
#define	SYS_getsockopt	280
#define	SYS_LISTEN	281
#define	SYS_listen	281
#define	SYS_RECV	282
#define	SYS_recv	282
#define	SYS_RECVFROM	283
#define	SYS_recvfrom	283
#define	SYS_RECVMSG	284
#define	SYS_recvmsg	284
#define	SYS_SEND	285
#define	SYS_send	285
#define	SYS_SENDMSG	286
#define	SYS_sendmsg	286
#define	SYS_SENDTO	287
#define	SYS_sendto	287
#define	SYS_SETSOCKOPT	288
#define	SYS_setsockopt	288
#define	SYS_SHUTDOWN	289
#define	SYS_shutdown	289
#define	SYS_SOCKET	290
#define	SYS_socket	290
#define	SYS_SOCKETPAIR	291
#define	SYS_socketpair	291

/*
 * 292 - 297: space for db_xxx system calls used internally
 */

/* Netipc syscalls */
#define	SYS_IPCCREATE	298
#define	SYS_ipccreate	298
#define	SYS_IPCNAME	299
#define	SYS_ipcname	299
#define	SYS_IPCNAMERASE	300
#define	SYS_ipcnamerase	300
#define	SYS_IPCLOOKUP	301
#define	SYS_ipclookup	301
#define SYS_IPCSELECT	302
#define SYS_ipcselect	302
#define	SYS_IPCCONNECT	303
#define	SYS_ipcconnect	303
#define	SYS_IPCRECVCN	304
#define	SYS_ipcrecvcn	304
#define	SYS_IPCSEND	305
#define	SYS_ipcsend	305
#define	SYS_IPCRECV	306
#define	SYS_ipcrecv	306
#define SYS_IPCGETNODENAME 307
#define SYS_ipcgetnodename 307
#define SYS_IPCSETNODENAME 308
#define SYS_ipcsetnodename 308
#define	SYS_IPCCONTROL	309
#define	SYS_ipccontrol	309
#define	SYS_IPCSHUTDOWN	310
#define	SYS_ipcshutdown	310
#define	SYS_IPCDEST	311
#define	SYS_ipcdest	311

/* new sysV ipc *ctl calls */
#define	SYS_SEMCTL	312
#define	SYS_semctl	312
#define	SYS_MSGCTL	313
#define	SYS_msgctl	313
#define	SYS_SHMCTL	314
#define	SYS_shmctl	314

/* mpctl */
#define	SYS_MPCTL	315
#define	SYS_mpctl	315

/* NFS 4.1 call */
#define SYS_EXPORTFS	316
#define SYS_exportfs	316

/* new streams */
#define SYS_GETPMSG     317
#define SYS_getpmsg     317
#define SYS_PUTPMSG     318
#define SYS_putpmsg     318
#define SYS_STRIOCTL    319
#define SYS_strioctl    319

/* more memory mapped file related system calls */
#define SYS_MSYNC	320
#define SYS_msync	320
#define SYS_MSLEEP	321	/* used internally by msem_lock() */
#define SYS_msleep	321	/* used internally by msem_lock() */
#define SYS_MWAKEUP	322	/* used internally by msem_unlock() */
#define SYS_mwakeup	322	/* used internally by msem_unlock() */
#define SYS_MSEM_INIT   323
#define SYS_msem_init   323
#define SYS_MSEM_REMOVE 324
#define SYS_msem_remove 324

#define SYS_ADJTIME	325
#define SYS_adjtime	325

#define SYS_LCHMOD	326
#define SYS_lchmod	326

#endif	/* not _XPG2 */

#ifdef __hp9000s300

#ifndef _SYS_TYPES_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_TYPES_INCLUDED */

/* make sure kernel & command are in sync                              */
/* bump whenever fields in struct sct (or their semantics) are changed */
#define	SCT_REV		4

#define SCT_MAX_CMD	14	/* same as MAXNAMLEN */

/* Note - many of these declarations assume 32 bit ints. */

struct sct {
	int	sct_flags;
	pid_t	sct_pid;
	pid_t	sct_pgrp;
	dev_t	sct_tty;
	int	sct_syscall_mask[16];	/* 512 bits */
	int	sct_errno_mask[16];	/* 512 bits */
	char	sct_command[SCT_MAX_CMD + 1];
};

/* bits in sct_flags */
#define	SCT_ENTRY	0x001	/* trace entry to syscall() - as in 4.2 */
#define	SCT_NORMAL_EXIT	0x002	/* trace normal exit from syscall() */
#define	SCT_ERROR_EXIT	0x004	/* trace error exit from syscall() */
#define	SCT_PID_FILTER	0x010	/* filter trace by pid */
#define	SCT_PGRP_FILTER	0x020	/* filter trace by process group */
#define	SCT_TTY_FILTER	0x040	/* filter trace by tty group */
#define	SCT_CMD_FILTER	0x080	/* filter trace by command name */
#define	SCT_PID_COMP	0x100	/* complement filter by pid */
#define	SCT_PGRP_COMP	0x200	/* complement filter by process group */
#define	SCT_TTY_COMP	0x400	/* complement filter by tty group */
#define	SCT_CMD_COMP	0x800	/* complement filter by command name */

#define SCT_FLAGS	syscalltrace.sct_flags
#define	SCT_BIT(array, index)	(syscalltrace.array[index>>5] & \
					((unsigned)0x80000000 >> (index&0x1f)))
#define	SCT_BIT_SET(array, index)     (syscalltrace.array[index>>5] |= \
					((unsigned)0x80000000 >> (index&0x1f)))
#define	SCT_BIT_CLEAR(array, index)   (syscalltrace.array[index>>5] &= \
					~((unsigned)0x80000000 >> (index&0x1f)))
#define	SCT_ON(which, p, code)					          \
	       ((SCT_FLAGS & which) &&                                    \
		((!(SCT_FLAGS & SCT_PID_FILTER)) ||                       \
			((SCT_FLAGS & SCT_PID_COMP) ?                     \
			((p)->p_pid != syscalltrace.sct_pid) :            \
			((p)->p_pid == syscalltrace.sct_pid))) &&         \
		((!(SCT_FLAGS & SCT_PGRP_FILTER)) ||                      \
			((SCT_FLAGS & SCT_PGRP_COMP) ?                    \
			((p)->p_pgrp != syscalltrace.sct_pgrp) :          \
			((p)->p_pgrp == syscalltrace.sct_pgrp))) &&       \
		((!(SCT_FLAGS & SCT_TTY_FILTER)) ||                       \
			((SCT_FLAGS & SCT_TTY_COMP) ?                     \
			(u.u_procp->p_ttyd != syscalltrace.sct_tty) :     \
			(u.u_procp->p_ttyd == syscalltrace.sct_tty))) &&  \
		((!(SCT_FLAGS & SCT_CMD_FILTER)) ||                       \
			((SCT_FLAGS & SCT_CMD_COMP) ?                     \
			strcmp (u.u_comm, syscalltrace.sct_command) :     \
			!strcmp (u.u_comm, syscalltrace.sct_command))) && \
		SCT_BIT (sct_syscall_mask, code))

#endif /* __hp9000s300 */

#ifdef __hp9000s800

/*
 * Light-Weight System Call Disable Flags
 */
#define	LW_SYSCALL_OFF_MS	0x1		/* Measurement System    */
#define	LW_SYSCALL_OFF_KI	0x2		/* Kernel Interface      */
#define	LW_SYSCALL_OFF_AUDIT	0x4		/* Audit Trail	         */
#define	LW_SYSCALL_OFF_MI	LW_SYSCALL_OFF_KI /* Measurement Interface */

/*
 * Specific to HP Precision Architecture Procedure Calling Convention
 */

#define	NUMARGINREGS	4	/* Number of arguments in registers */

/* Register Usage */
/*
 * DO NOT put the comments in the same line as the definitions.
 * It will screw the heck out of the assembler while making the stubs.
 */
    /* System call number (New Call) */
#define	SYS_CN	r22
#define	sys_CN	SYS_CN
    /* Return status (New Call) */
#define	RS	r22
#define	sys_RS	RS
/* Return status save_state element */
#define	ss_rs	ss_gr22


/*
 * System Call Gateway Address information
 */
    /* Space ID of gateway pages */
#define	GATEWAYSID	0x0
    /* Offsets of gate instructions */
#define	SYSCALLGATE	0xc0000004

/* System Call Frame */
#define	SCF_INCR	SS_SIZE+FM_SIZE+16
#define	SCF_DECR	-SS_SIZE-FM_SIZE-16

#endif /* __hp9000s800 */

#ifdef _UNSUPPORTED

	/* 
	 * NOTE: The following header file contains information specific
	 * to the internals of the HP-UX implementation. The contents of 
	 * this header file are subject to change without notice. Such
	 * changes may affect source code, object code, or binary
	 * compatibility between releases of HP-UX. Code which uses 
	 * the symbols contained within this header file is inherently
	 * non-portable (even between HP-UX implementations).
	*/
#ifdef _KERNEL_BUILD
#	include "../h/_syscall.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_syscall.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_SYSCALL_INCLUDED */
