/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/h/RCS/globals.h,v $
 * $Revision: 1.7.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 16:53:17 $
 */
/* HPUX_ID: @(#)globals.h	52.2		88/04/28 */
#ifndef _SYS_GLOBALS_INCLUDED /* allows multiple inclusion */
#define _SYS_GLOBALS_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/acct.h"
#include "../h/callout.h"
#include "../h/map.h"
#include "../h/dk.h"
#include "../h/mount.h"
#include "../h/vm.h"
#include "../h/dbg.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/callout.h>
#include <sys/map.h>
#include <sys/dk.h>
#include <sys/mount.h>
#include <sys/vm.h>
#include <sys/dbg.h>
#endif /* _KERNEL_BUILD */

/*
 * Real declarations of kernel global variables.
 * This file should only be included in conf.c
 */
struct	acct	acctbuf;
struct	inode	*acctp;
struct	buf *swbuf;	/* swap I/O headers */
int	nswbuf;
struct	bufhd bufhash[BUFHSZ];	/* heads of hash lists */
#ifdef _WSIO
struct	bufqhead bfreelist[BQUEUES];	/* heads of available lists */
#else
struct	buf bfreelist[BQUEUES];	/* heads of available lists */
#endif
struct fswdevt *fswdevt;
int failure_sent;
struct	buf bswlist;		/* head of free swap header list */
struct	callout *callfree, *callout, calltodo;
int	ncallout;
struct	cblock *cfree;
int	nclist;
struct	cblock *cfreelist;
int	cfreecount;
int	firstfree, maxfree;
long	cp_time[CPUSTATES];
int	dk_busy;
long	dk_time[DK_NDRIVE];
long	dk_seek[DK_NDRIVE];
long	dk_xfer[DK_NDRIVE];
long	dk_wds[DK_NDRIVE];
float	dk_mspw[DK_NDRIVE];
long	tk_nin;
long	tk_nout;
struct	domain *domains;
struct	file *file, *fileNFILE, *file_reserve;
int	nfile;
struct	inode *inode;		/* the inode table itself */
struct	inode *inodeNINODE;	/* the end of the inode table */
int	ninode;			/* number of slots in the table */
struct	inode *rootdir;		/* pointer to inode of root directory */
long	hostid;
char	hostname[32];
int	hostnamelen;
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;		/* XXX */
int	hz;
int	phz;			/* alternate clock's frequency */
int	tick;
int	lbolt;			/* awoken once a second */
double	avenrun[3];
struct mounthead mounthash[MNTHASHSZ];
short	pidhash[PIDHSZ];
struct	proc *proc, *procNPROC;	/* the proc table */
int	nproc;
struct	pst_command_struct       *pstat_command_buffer = 0;
struct	pst_exec_filename_struct *pstat_exec_filenames = 0;
struct	prochd qs[NQS];
int	whichqs;	/* bit mask summarizing non-empty qs's */
int	nswdev;		/* number of swap devices */
pid_t	mpid;		/* generic for unique process id's */
char	runin;		/* scheduling flag */
char	runout;		/* scheduling flag */
int	runrun;		/* scheduling flag */
char	curpri;		/* more scheduling */
int	maxmem;		/* actual max memory per process */
int	physmem;	/* physical memory on this CPU */
int	nswap;		/* size of swap space */
int	updlock;	/* lock for sync */
long	dumplo;		/* offset into dumpdev */
daddr_t	rablock;	/* block to be read ahead */
int	rasize;		/* size of block in rablock */
int 	global_psw;	/* global variable psw bits */
int	noproc;		/* no one is running just now */
char	*panicstr;
int	wantin;
int	boothowto;	/* reboot flags, from console subsystem */
int	selwait;

#ifdef TRACE
char	traceflags[TR_NFLAGS];
struct	proc *traceproc;
int	tracebuf[TRCSIZ];
u_int	tracex;
int	tracewhich;
#endif
#ifdef DBG
int	dbgflags[DBG_NFLAGS];
#endif
struct	vmmeter cnt, rate, sum;
struct	vmtotal total;
int	freemem;	/* remaining blocks of free memory */
int	avefree;	/* moving average of remaining free blocks */
int	avefree30;	/* 30 sec (avefree is 5 sec) moving average */
int	deficit;	/* estimate of needs of new swapped in procs */
int	nscan;		/* number of scans in last second */
int	multprog;	/* current multiprogramming degree */
int	desscan;	/* desired pages scanned per second */
int	maxslp;		/* max sleep time before very swappable */
int	lotsfree;	/* max free before clock freezes */
int	minfree;	/* minimum free pages before swapping begins */
int	desfree;	/* no of pages to try to keep free via daemon */
int	saferss;	/* no pages not to steal; decays with slptime */
struct	forkstat forkstat;
struct	swptstat swptstat;
#endif /* _SYS_GLOBALS_INCLUDED */
