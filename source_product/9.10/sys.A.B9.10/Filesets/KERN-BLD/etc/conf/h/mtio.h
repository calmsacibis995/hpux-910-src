/*
 * @(#)mtio.h: $Revision: 1.21.83.5 $ $Date: 93/12/08 18:23:57 $
 * $Locker:  $
 */

#ifndef _SYS_MTIO_INCLUDED
#define _SYS_MTIO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF	0	/* write an end-of-file record */
#define MTFSF	1	/* forward space file */
#define MTBSF	2	/* backward space file */
#define MTFSR	3	/* forward space record */
#define MTBSR	4	/* backward space record */
#define MTREW	5	/* rewind */
#define MTOFFL	6	/* rewind and put the drive offline */
#define MTNOP	7	/* no operation, sets status only */
#define MTEOD	8	/* (DDS and QIC devices only) seek to EOD point*/
#define MTWSS	9	/* (DDS device only) write mt_count save setmarks*/
#define MTFSS	10	/* (DDS device only) forward mt_count save setmarks*/
#define MTBSS	11	/* (DDS device only) backward mt_count save setmarks*/

/* structure for MTIOCGET - mag tape get status command */

struct	mtget	{
	long	mt_type;	/* type of magtape device */
	long	mt_resid;	/* residual count */
	/* device dependent status */
	long	mt_dsreg1;	/* status register (msb) */
	long	mt_dsreg2;	/* status register (lsb) */
	/* device independent status */
	long	mt_gstat;	/* generic status */
	long	mt_erreg;	/* error register */
#ifdef __hp9000s800
	daddr_t	mt_fileno;	/* always set to -1 */
	daddr_t	mt_blkno;	/* always set to -1 */
#endif /* __hp9000s800 */
};

/*
 * Constants for mt_type
 */
#define	MT_ISTS		0x01
#define	MT_ISHT		0x02
#define	MT_ISTM		0x03
#define MT_IS7970E	0x04
#define MT_ISSTREAM	0x05
#define MT_ISDDS1       0x06 /* HPCS/HPIB DDS device without partitions*/
#define MT_ISDDS2       0x07 /* HPCS/HPIB DDS device with partitions*/
#define MT_ISSCSI1      0x08 /* ANSI SCSI-1 device */
#define MT_ISQIC        0x09 /* QIC device */

/* mag tape io control commands */
#define	MTIOCTOP	_IOW('m', 1, struct mtop)		/* do a mag tape op */
#define	MTIOCGET	_IOR('m', 2, struct mtget)	/* get tape status */

/* generic mag tape (device independent) status macros */
/* for examining mt_gstat */
#define GMT_EOF(x)              ((x) & 0x80000000)
#define GMT_BOT(x)              ((x) & 0x40000000)
#define GMT_EOT(x)              ((x) & 0x20000000)
#define GMT_SM(x)               ((x) & 0x10000000)  /* DDS setmark */
#define GMT_EOD(x)              ((x) & 0x08000000)  /* DDS  or QIC EOD */
#define GMT_WR_PROT(x)          ((x) & 0x04000000)
#define GMT_ONLINE(x)           ((x) & 0x01000000)
#define GMT_D_6250(x)           ((x) & 0x00800000)
#define GMT_D_1600(x)           ((x) & 0x00400000)
#define GMT_D_800(x)            ((x) & 0x00200000)
#define GMT_COMPRESS(x)         ((x) & 0x00100000)
#define GMT_DR_OPEN(x)          ((x) & 0x00040000)  /* door open */
#define GMT_IM_REP_EN(x)        ((x) & 0x00010000)

#define GMT_QIC_FORMAT(x)       ((x) & 0x0000000f)  /* QIC only */
/* possible values for QIC format returned by GMT_QIC_FORMAT */
#define UNKNOWN_FORMAT          0x0
#define QIC_24                  0x4
#define QIC_120                 0x5
#define QIC_150                 0x6
#define QIC_525                 0x7

#define GMT_QIC_MEDIUM(x)       (((x) & 0x000000f0) >> 4) /* QIC only */
/* possible values for QIC medium  returned by GMT_QIC_MEDIUM */
#define UNKNOWN_MEDIUM          0x0
#define DC300                   0x2 
#define DC300XLP                0x2
#define DC600A                  0x4
#define DC615                   0x4
#define DC6037                  0x6
#define DC6150                  0x6
#define DC6250                  0x6
#define DC6320                  0x8
#define DC6525                  0x8

#ifndef _KERNEL
#ifdef __hp9000s800
 	/* CONCERN - two defns for DEFTAPE, don't know whether
			   new 200 defn replaces old defn for __hp9000s800 or not.*/
#define	DEFTAPE	"/dev/rmt/0m"
#endif /* __hp9000s800 */
#ifdef __hp9000s300
#define	DEFTAPE	"/dev/rmt/0mn"
#endif /* __hp9000s300 */
#endif /* not _KERNEL */

#ifdef _WSIO

/* Bit Definitions for tape device minor number */

#define MT_DENSITY_MASK	0xC0	/* for extracting density bits */
#define MT_UNIT_MASK	0x30	/* for extracting unit number bits */
#define MT_COMPAT_MODE	0x08	/* don't accept block that crosses eot */
#define MT_SYNC_MODE	0x04	/* don't enable immediate report */
#define MT_UCB_STYLE	0x02	/* don't reposition on read-only closes */
#define MT_NO_AUTO_REW	0x01	/* don't rewind on close */

#define MT_800_BPI	0x00	/* 800 bpi; NRZI format */
#define MT_1600_BPI	0x40	/* 1600 bpi; PE format */
#define MT_6250_BPI	0x80	/* 6250 bpi; GCR format */
#define MT_GCR_COMPRESS	0xC0    /* GCR format COMPRESSED */

#define MT_PARTITION	0x10	/* Select partition 1 
				 * (SCSI DDS devices only) 
				 */

#define MT_FIXED_MODE	0x80	/* Enable fixed mode  (SCSI devices only) 
				 * For QIC Style tape drives.
				 */

#endif	/* _WSIO */

#endif /*  _INCLUDE_HPUX_SOURCE */

#endif /* not _SYS_MTIO_INCLUDED */
