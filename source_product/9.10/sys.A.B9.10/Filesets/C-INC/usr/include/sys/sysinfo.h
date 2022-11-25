/*
 * @@(#)sysinfo.h: $Revision: 1.8.83.4 $ $Date: 93/09/17 18:35:47 $
 * $Locker:  $
 */

/*
 *	System Information.
 */

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */
struct sysinfo {
	time_t	cpu[5];
#define	CPU_IDLE	0
#define	CPU_USER	1
#define	CPU_KERNEL	2
#define	CPU_WAIT	3
#define CPU_SXBRK	4
	time_t	wait[3];
#define	W_IO	0
#define	W_SWAP	1
#define	W_PIO	2
	long	bread;		/*  transfer of data between system buffers and disk */
	long	bwrite;		/*  or other block devices                           */
	long	lread;		/*  access of system buffers                         */
	long	lwrite;
	long	phread;		/*  transfer via physical device mechanism	     */
	long	phwrite;
	long	swapin;		/*  number of swap transfer		             */
	long 	swapout;
	long	bswapin;	/*  number of 512-bytes transferred (for bswapin:include */
	long	bswapout;	/* initial loading of some programs		         */
	long	pswitch;       	/* process switches 					 */
	long	syscall;	/* system calls of all types				 */        
	long	sysread;	/* specific system calls				 */
	long	syswrite;
	long	sysfork;
	long	sysexec;
	long	runque;	 	/* run queue of processes in memory and runable		*/
	long	runocc;		/* time occurring					*/
	long	swpque;		/* swap queue of processes swapped out but ready to run.  */
	long	swpocc;
	long	iget;  		/* use of file access system routines			*/
	long	namei;
	long	dirblk;
	long	readch;		/* characters transferred by read/write system calls	*/
	long	writech;
	long	rcvint;		/* receive interrupt					*/
	long	xmtint;         /* transfer interrupt                                   */
	long	mdmint;         /* modem interrupt					*/   
	long	rawch;		/* input character					*/
	long	canch;		/* input character processed by cannon			*/
	long	outch;		/* output character					*/
	long	msg; 		/* message primitve					*/
	long	sema; 		/* semaphore primitive				        */  
	long	pnpfault;
	long	wrtfault;
};

extern struct sysinfo sysinfo;

struct minfo {
	long 	freemem; 	/* freemem in page */
	long	freeswap;	/* free swap space */
	long    vfault;  	/* translation fault */
	long    demand;		/*  demand zero and demand fill pages */
	long    swap;		/*  pages on swap */
	long    cache;		/*  pages in cache */
	long    file;		/*  pages on file */
	long    pfault;		/* protection fault */
	long    cw;		/*  copy on write */
	long    steal;		/*  steal the page */
	long    freedpgs;	/* pages are freed */
	long    unmodsw;	/* getpages finds unmodified pages on swap */
	long	unmodfl;	/* getpages finds unmodified pages in file */ 
};

extern struct minfo minfo;
extern struct syswait syswait;


struct syserr {
	long	inodeovf;
	long	fileovf;
	long	textovf;
	long	procovf;
};

extern struct syserr syserr;
