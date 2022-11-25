/*
 * @(#)defs.h: $Revision: 1.51.83.3 $ $Date: 93/09/17 16:30:13 $
 * $Locker:  $
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

#include "an_nldefs.h"

/*
 * kp = kernel ptr, lp = local ptr,
 * lbp = local base ptr, kbp = kernel base ptr
 */
#define localizer(kp,lbp,kbp) (((kp) - (kbp)) + (lbp))
#define unlocalizer(lp,lbp,kbp) (((lp) - (lbp)) + (kbp))
#ifdef hp9000s300
#undef btoc
#undef ctob
#endif /* hp9000s300 */
#define btoc(x)  	(btop (x))
#define ctob(x)		(ptob (x))

#define GETBYTES(cast, addr, length) (cast)getbytes(addr, length)
caddr_t getbytes();

/* table sizes for configurable hash tables */
extern	int	BUFHSZ;
extern	int	NBUFHASH;
extern	int	INOHSZ;


/* This is defined in inode.h under ifdef KERNEL !!! */
#define VTOI(VP)	((struct inode *)(VP)->v_data)
#define ITOV(IP)	((struct vnode *)&(IP)->i_vnode)


/*
 * Warning! THIS IS HARD CODED. (look for it in nfs/nfs_subr.c).
 */
#define RTABLESIZE 16

/*
 * Warning! RPB Sizes and layout are known, they should be done
 *	    dynamically.
 */

/* THIS IS HARD CODED WATCH OUT (look for it in h/dnlc.h) */
/* NC_HASH has increased to 256 for release 7.0 */
/* #define NC_HASH_SIZE 256 */

/* THIS IS HARD CODED WATCH OUT (look for it in h/dux_mbuf.h) */
#define ADDRESS_SIZE 6

/* THIS IS HARD CODED WATCH OUT (look for it in h/mount.h) */
#define M_UNLOCKED	0	/*  Mount table being locked  */
#define	M_BOOTING	1	/*  Site is booting  */
#define	M_MODIFY	2	/*  Mount table being modified  */
#define	M_COPY		3	/*  Mount table being copied  */


/* THIS IS HARD CODED WATCH OUT (look for it in proc.h) !!!  */
#define NQS 160

#ifdef MP
#define MAX_PROCS 4
#endif

/* THIS IS HARD CODED WATCH OUT (look for it in config.h/conf.c)!!! */
#ifdef  hp9000s800
#define MAXSWAPCHUNKS   256
#else
#define MAXSWAPCHUNKS   512
#endif
/* THIS IS HARD CODED WATCH OUT (look for it in mach.800/space.h)!!! */
#define NSWAPDEV	 10
#define swchunk 	2048

/* use vprintf with care; it plays havoc with ``else's'' */
#define	vprintf	if (vflg) printf

#define clear(x)	(x)


/* stack tracing numbers */
#define OVRHD 2
#define MAXSTKTRC 50

/* kludge for development compiler */
#ifdef	PA89

#ifndef sign21ext
#ifndef pgndx_sext
#ifndef PGNDXWDTH
#ifndef PGSHIFT
#define PGSHIFT 12		/* bit width of offset */
#endif	/* PGSHIFT */
#define PGNDXWDTH (32-PGSHIFT)	/* bit width of a physical page no */
#endif	/* PGNDXWDTH */
#define pgndx_sext(ndx) \
	(((ndx) & (1<<(PGNDXWDTH-1))) ? ((ndx) | (~((1<<PGNDXWDTH)-1))): (ndx))
#endif	/* pgndx_sext */
#define sign21ext(foo) pgndx_sext(foo)
#endif	/* sign21ext */

#else	/* PA89 */

#define sign21ext(foo) \
	(((foo) & 0x100000) ? ((foo) | 0xffe00000) : (foo))

#endif	/* PA89 */


/*
 * 300 and 800 count kernel stack size in pages and bytes, respectively.
 * Code was changed to match 800.  This backpatches the 300.
 */
#ifdef hp9000s300
#define KSTACKBYTES (ptob(KSTACK_PAGES))
#endif

#ifdef hp9000s800
/* THIS IS HARD CODED WATCH OUT (look for it in locore.s) !!! */
/* It must be larger then STACKSIZE, since its used for both */
/* #define ICS_SIZE 5 for rel2 ic2 and before */
#ifdef	MP
#define ICS_SIZE 8
#else	not MP
#define ICS_SIZE 7
#endif	not MP

#define MAX_RPBS ((CRASH_EVENT_TABLE_SIZE/sizeof (struct crash_event_table_struct)) + 10)
#define RPBSIZE_ALL 86
#define RPBSIZE_STANDARD 76

#else
#define ICS_SIZE 7
#define DMASTACKSIZE 1

#define RPBSIZE_ALL 31
#define RPBSIZE_STANDARD 31
#endif



/* YOU MUST CHANGE THIS IF THE SYSMAP SIZE CHANGES IN THE KERNEL */
#define SYSMAPSIZE 400

/* This was defined in vm_swalloc */
#define DPPSHFT 1

/* old macros */
#define dumptext(a,b,c,d)	fprintf(outf," No longer supported\n");
#define cmtopg(a)	fprintf(outf," No longer supported\n");
#define kmxtob(a)	fprintf(outf," No longer supported\n");
#define pgtocm(a)	fprintf(outf," No longer supported\n");
#define CMHASH(a)	fprintf(outf," No longer supported\n");


#define	SFREE	0


#define pttozpt(n)	(n+3)
/* page log table (type codes) defines */
#define ZLOST           0
#define ZFREE           1
#define ZSYS            2
#define ZUNUSED         3
#define ZUAREA          4
#define ZTEXT           5
#define ZDATA           6
#define ZSTACK          7
#define ZSHMEM          8
#define ZNULLDREF       9
#define ZLIBTXT         10
#define ZLIBDAT         11
#define ZSIGSTACK       12
#define ZIO             13
#define ZMMAP           14
#define ZISRFREE        15
/* MEMSTATS */
#define ZMAX        16


/* region log table (type codes) defines */
#define RLOST           -2
#define RFREE           -1
#define RUNUSED         0
#define RPRIVATE        1
#define RSHARED         2

/* pregion log table (type codes) defines */
#define PLOST           -2
#define PFREE           -1
#define PUNUSED         0
#define PUAREA          1
#define PTEXT           2
#define PDATA           3
#define PSTACK          4
#define PSHMEM          5
#define PNULLDREF       6
#define PLIBTXT         7
#define PLIBDAT         8
#define PSIGSTACK       9
#define PIO             10
#define PMMAP           11

/* vfd chunk size -- also declared in vfd.h */
#define CHUNKENT (32)



/* status codes */
#define Z_INTRAN 1

#ifdef hp9000s800
/* pdir type defines */
#define ZINVALID 0
#define ZPREVALID 1
#define ZVALID 2
#endif

/* buffer system log table defines */
#define BFREE	0
#define BUNUSED 1
#define BLRU	2
#define BAGE	3
#define BEMPTY	4
#define BVIRT	5
#define BHASH 6
#define BFREELIST 7


/* swap buf headr log table defines */
#define BFREE	0
#define BSWAP	1
#define BCLEAN	2


/* inode log table defines */
#define IEMPTY 0
#define ICHAIN 1
#define IFREE 2


/* Error cookies */

#define ERROR_STRING	"ERROR:\n"
#define fprintf_err()	fprintf(outf,ERROR_STRING)

#ifdef hp9000s800
/* HPMC defines */
/* (location of memory where PDC places registers on HPMC) */
#define HPMC_LOC 0x80

/* Check type word codes */
#define CTW_CACHE 0x80000000
#define CTW_TLB 0x40000000
#define CTW_BUS 0x20000000
#define CTW_ASSIST 0x10000000
#define CTW_PROCESS 0x08000000

/* Check CPU state word */
#define CPU_IQV 0x80000000
#define CPU_IQF 0x40000000
#define CPU_IPV 0x20000000
#define CPU_GRV 0x10000000
#define CPU_CRV 0x08000000
#define CPU_SRV 0x04000000
#define CPU_POK 0x00000002
#define CPU_ERC 0x00000001

/* Check Cache check */
#define CACHE_ICHECK 0x80000000
#define CACHE_DCHECK 0x40000000
#define CACHE_TAGCHECK 0x20000000
#define CACHE_DATACHECK 0x10000000

/* Check TLB check */
#define TLB_ICHECK 0x80000000
#define TLB_DCHECK 0x40000000


/* Check BUS check */
#define BUS_ADDRESS_ERR 0x80000000
#define BUS_DATA_SLAVE_ERR 0x40000000
#define BUS_DATA_PARITY_ERR 0x20000000
#define BUS_DATA_PROTOCOL_ERR 0x10000000
#define BUS_READ_TRANSACTION 0x08000000
#define BUS_WRITE_TRANSACTION 0x04000000
#define BUS_MEMORY_SPC_TRANS 0x02000000
#define BUS_IO_SPC_TRANS 0x01000000
#define BUS_PROCESSOR_MASTER 0x00800000
#define BUS_PROCESSOR_SLAVE 0x00400000

/* Assists Check word */
#define ASSIST_COPROCESSOR_CHECK 0x80000000
#define ASSIST_SFU_CHECK 0x40000000
#define ASSIST_ID_VALID 0x20000000


/* Hp9000s800 symbol table stuff */
#define	iscodesym(sdp)	(((sdp)->symbol_type == ST_ABSOLUTE) \
	|| ((sdp)->symbol_type == ST_CODE) \
	|| ((sdp)->symbol_type == ST_PRI_PROG) \
	|| ((sdp)->symbol_type == ST_SEC_PROG) \
	|| ((sdp)->symbol_type == ST_ENTRY) \
	|| ((sdp)->symbol_type == ST_MILLICODE) \
	|| ((sdp)->symbol_type == ST_STUB))

#define	isdatasym(sdp)	(((sdp)->symbol_type == ST_ABSOLUTE) \
	|| ((sdp)->symbol_type == ST_DATA) \
	|| ((sdp)->symbol_type == ST_STORAGE))

#define	issym(sdp)	(iscodesym(sdp) || isdatasym(sdp))

#ifdef	PA89
#ifndef	pdirhash
#define pdirhash(sid, sof)      \
	((sid << 5 ) ^ ((unsigned int) (sof) >> PGSHIFT)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))
#endif	/* pdirhash */
#else	/* PA89 */
#define pdirhash(sid, sof)	\
	((sid << 5 ) ^ ((unsigned int) (sof) >> 11)  ^ \
	(sid << 13 ) ^ ((unsigned int) (sof) >> 19))
#endif	/* PA89 */

#define	KSTACKBYTES	8192		/* size of kernel stack */
#define	UPAGES		btorp(sizeof(user_t) + KSTACKBYTES)

#else hp9000s300

/* Include some constant defines needed from <sys/param.h> and */
/* <machine/param.h> so both UMM & WOPR can be defined at once */

/*
#define UPAGES_M320     3
#define FLOAT_M320      5
#define DRAGON_PAGES	32
#define GAP_PAGES	216
#define	HIGHPAGES_M320	(UPAGES_M320+FLOAT_M320+DRAGON_PAGES+GAP_PAGES)
#define	USRSTACK_M320 	(caddr_t) (-(HIGHPAGES_M320*NBPG_M320))
#define	USER_AREA_M320 	(USRSTACK_M320+((DRAGON_PAGES+GAP_PAGES)*NBPG_M320))
#define	UPAGES_WOPR	UPAGES_M320
#define	FLOAT_WOPR	FLOAT_M320
#define	HIGHPAGES_WOPR	HIGHPAGES_M320
#define	USRSTACK_WOPR	USRSTACK_M320
#define	USER_AREA_WOPR	USER_AREA_M320
#define	NBPG_M320	4096
#define	PGSHIFT_M320	12
#define	NBPG_WOPR	NBPG_M320
#define	PGSHIFT_WOPR	PGSHIFT_M320
#define	CLSIZE		1
*/


#define	NBPG_M320	4096

/*
 * Kernel dependent constants for the Series 200/300
 */

/* take care of differences between UMM, 310 and other Series 300 */

#define KERNEL_300 "9000/3"        /* uname.machine for Series 300 */
#define KERNEL_300_LEN 6           /* length of KERNEL_300 */
#define KERNEL_310 "9000/310"      /* uname.machine for 310 */
#define KERNEL_310_LEN 8           /* length of KERNEL_310_LEN */

/* from sys/param.h */

#define KERNELSPACE 0

/* Segment table entries */
#define SEGTBLENT	1024
#define PGTBLENT        1024

/* These are normally in inode.h */
#define IMOUNT 0x8
#define ISHLOCK 0x80
#define IEXLOCK 0x100

#endif
