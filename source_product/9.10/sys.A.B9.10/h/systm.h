/* @(#) $Revision: 1.40.83.5 $ */    
#ifndef _SYS_SYSTM_INCLUDED /* allows multiple inclusion */
#define _SYS_SYSTM_INCLUDED

/*
 * Random set of variables used by more than one routine.
 */
#ifdef	_KERNEL
extern struct vnode *rootdir;	/* pointer to vnode of root directory */
extern struct vnode *pipevp;	/* pointer to vnode on device with pipes*/
				/* often the same as rootdir */
#endif /* _KERNEL */

#ifdef _KERNEL_BUILD
#include "../h/kern_sem.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/kern_sem.h>
#endif /* _KERNEL_BUILD */

#ifdef _KERNEL

extern	int	lbolt;			/* awoken once a second */

	/* utilized in assessing system memory management status */
extern	int	ticks_since_boot;	/* lots of other uses as well */

#ifdef	MP
extern	sync_t	runin;		/* scheduling flag */
extern	sync_t	runout;		/* scheduling flag */
#else	/* ! MP */
extern	char	runin;		/* scheduling flag */
extern	char	runout;		/* scheduling flag */
#endif	/* ! MP */
extern	int	runrun;		/* scheduling flag */
#ifndef	MP
/*  MP has this in a per-processor storage area */
extern	u_char	curpri;		/* more scheduling */
#endif /* ! MP */

extern	int	maxmem;		/* actual max memory per process */
extern	int	physmem;	/* physical memory on this CPU */
#if defined(__hp9000s800) && defined(HPUXBOOT)
extern	int	btphysmem;	/* physical memory on this CPU used by boot */
#endif

#ifdef	__hp9000s800
extern	dev_t	bootdev;	/* Device containing kernel, from hpuxboot */
extern	int	nswap;		/* size of swap space */
extern	dev_t	rootdev;	/* device of the root */
extern	dev_t	swapdev;	/* swapping device */
#endif	/* __hp9000s800 */
#ifdef __hp9000s300
int	nswap;		/* size of swap space */
dev_t	rootdev;	/* device of the root */
dev_t	swapdev;	/* swapping device */
#endif /* __hp9000s300 */
struct vnode	*swapdev_vp;	/* vnode equivalent to above */
struct vnode	*argdev_vp;	/* vnode equivalent to above */

#if defined(__hp9000s800) && defined(VOLATILE_TUNE) && defined(_KERNEL)
extern volatile char *panicstr;		/* panic string pointer */
#else
extern char *panicstr;		/* panic string pointer */
#endif /* __hp9000s800 && VOLATILE_TUNE && _KERNEL */

#endif /* _KERNEL */

daddr_t	bmap();
struct	inode *ialloc();
struct	inode *iget();
struct	inode *owner();
struct	inode *maknode();
struct	inode *namei();
struct	buf *alloc();
struct	buf *getblk();
struct	buf *geteblk();
struct	buf *bread();
struct	buf *breada();
struct	fs *getfs();
struct	file *getf();
struct	file *falloc();
caddr_t	kmem_alloc();
struct vnode *devtovp();

/*
 * Structure of the system-entry table
 */
extern struct sysent
{
	int	sy_narg;		/* total number of arguments */
	int	(*sy_call)();		/* handler */
	char	*sy_name;		/* for syscall tracing */
};

#define	sysent_assign(a1, a2, a3)\
	{\
		sysent[a1].sy_narg = a2;\
		sysent[a1].sy_call = a3;\
		sysent[a1].sy_name = "a3";\
	}

extern struct sysent	sysent[];
extern int		nsysent;

#ifdef __hp9000s300
extern struct sysent	compat_sysent[];
extern int		ncompat_sysent;
#endif

#ifdef __hp9000s300
extern	char version[];		/* system version */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
extern	int ref_hand;		/* current referance hand used by pageout 
				   daemon*/
extern	char _release_version[];	/* system version */
#endif /* __hp9000s800 */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in binit/bio.c by making
 * a pass over the switch.
 * Used in bounds checking on major
 * device numbers.
 */

extern int	nblkdev;

/*
 * Number of character switch entries.
 * Set by cinit/prim.c
 */

extern int	nchrdev;

#ifdef __hp9000s800
extern	int	nswdev;		/* number of swap devices */
extern	pid_t	mpid;		/* generic for unique process id's */
extern	char	kmapwnt;	/* kernel map want flag */
extern	int	updlock;	/* lock for sync */
extern	daddr_t	rablock;	/* block to be read ahead */
extern	int	rasize;		/* size of block in rablock */
#endif /* __hp9000s800 */
#ifdef __hp9000s300
int	nswdev;		/* number of swap devices */
pid_t	mpid;		/* generic for unique process id's */
char	kmapwnt;	/* kernel map want flag */
int	updlock;	/* lock for sync */
daddr_t	rablock;	/* block to be read ahead */
int	rasize;		/* size of block in rablock */
extern	int intstack[];		/* stack for interrupts */
#endif /* __hp9000s300 */

#if defined(__hp9000s800)
extern	dev_t	dumpdev;	/* device to take dumps on */
extern	long	dumplo;		/* offset into dumpdev */
extern	int	dumpmag;	/* magic number for savecore */
extern	int	dumpsize;	/* also for savecore */
#endif /* defined(__hp9000s800) */

#if defined(__hp9000s300) 
/* extern dev_t	dumpdev;	/* device to take dumps on */
/* extern long	dumplo;		/* offset into dumpdev */
#endif /* defined(__hp9000s300) */

extern	dev_t	argdev;		/* device for argument lists */

#ifdef __hp9000s300
int	physmembase;		/* lowram page frame number CPU */
int	p1pages;		/* max # of pages per process */
int	highpages;		/* max # of pages above user stack */
caddr_t	usrstack;		/* logical address of user stack start */
caddr_t	user_area;		/* logical address of uarea */
caddr_t float_area;		/* logical address of float card start */
int	processor;		/* 68010, 68020 */
#endif /* __hp9000s300 */

#if defined(__hp9000s300)
extern  int icode[];		/* user init code */
#endif

extern	int szicode;		/* size of user init code */
#ifdef __hp9000s800
long    total_lockable_mem;     /* Maximum lockable memory, in clicks */
long    lockable_mem;           /* Current amount left, in clicks */
#else /* ! __hp9000s800 */
long    total_lockable_mem;     /* Maximum lockable memory, in clicks */
long    lockable_mem;           /* Current amount left, in clicks */
long	unlockable_mem;		/* Amount reserved from locking, in bytes */
#endif /* __hp9000s800 */

#if defined(__hp9000s800) 
extern	int global_psw;		/* global variable psw bits */
int icode();		/* beginning of user init code */
int eicode();		/* end of user init code */
int fcode();		/* beginning of user fsck code */
int efcode();		/* end of user fsck code */
extern	int szfcode;		/* size of user init code */
#endif /* defined(__hp9000s800) */

caddr_t	calloc();
unsigned max();
unsigned min();
int	memall();
int	uchar(), schar();
int	vmemall();
caddr_t	wmemall();
swblk_t	vtod();

#ifndef	MP
/*  MP has this in a per-processor storage area */
extern	int	noproc;		/* no one is running just now */
#endif	/* ! MP */
extern	int	wantin;
extern	int	boothowto;	/* reboot flags, from console subsystem */
extern	int	selwait;

#ifdef __hp9000s300
extern	char vmmap[];		/* poor name! */

#ifdef DISPATCHLCK
struct proc *dispatchlocked;    /* dispatch locking*/
#endif
#endif /* __hp9000s300 */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)
#define	queue(q,p)	_queue((caddr_t)q,(caddr_t)p)
#define	dequeue(q)	_dequeue((caddr_t)q)

#ifdef __hp9000s300
/* float variable used globally */
int float_present;
int dragon_present;
extern int bus_master_count;	/* count of all bus masters in the system */
#endif /* __hp9000s300 */

#endif /* _SYS_SYSTM_INCLUDED */
