/*
 * @(#)dsk_label.h: $Revision: 1.6.83.4 $ $Date: 93/09/17 18:25:41 $
 * $Locker:  $
 */

#ifdef _KERNEL_BUILD
#    include "../h/libIO.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/libIO.h>
#endif /* _KERNEL_BUILD */

#ifndef _SYS_DSK_LABEL_INCLUDED
#define _SYS_DSK_LABEL_INCLUDED


#ifndef DEV_SECTION_MASK
#define	DEV_SECTION_MASK	0xf
#endif

#ifndef DEV_TRANSPARENT_MASK
#define	DEV_TRANSPARENT_MASK	0x800000
#endif

#if DEV_SECTION_MASK & (DEV_SECTION_MASK + 1)
syntax error checking for power of two
#endif

#define MAX_LOCK  	3		/* number of boot devices */

#define	DSK_LABEL_MAGIC	0x8b7f6a3c	/* a magic(!?!) number */

#define	DSK_LABEL_VERSION	1	/* dsk_label version number  */

/* the actual disk label format */
struct dsk_label {
	long	dkl_lbstart;			/* should match lbend */
	long	dkl_magic;			/* location of magic number */
	long	dkl_version;			/* label version number */
	struct	part_def {			/* partition definitions */
		long	cpu_id;			/* owner of the partition */
		long	last_cpu_id;		/* owner of the partition */
		daddr_t	start;			/* unused */
		daddr_t	length;			/* unused */
	}	dkl_part [DEV_SECTION_MASK + 1];	/* 16 partitions/disk */
	char	dkl_flag [DEV_SECTION_MASK + 1];	/* 16 partitions/disk */
        hdw_path_type dkl_hwpaths[MAX_LOCK];
	short	dkl_boot;			/* How this label should be */
						/* used to boot (see below) */
	short	dkl_reserved;
	long	dkl_lbend;			/* should match lbstart */
};


#define STRT_END(cpu_id, lbmark) ((cpu_id << 16)| ((lbmark + 1) & 0xffff))

/* status bits for dkl_flag */
#define	DK_LOCKED	0x01		/* section lock in disk label */
#define	DK_ROOT		0x02		/* root PDISC */
#define	DK_SWAP		0x04		/* swap PDISC */
#define	DK_DUMP		0x08		/* dump PDISC */
#define	DK_PDISC	0x10		/* to differentiate from disk slice */
#define DK_NOBDRA	0x20		/* OK to boot without BDRA */

/* values for dkl_boot */
#define	DK_NONE		0		/* Don't use for boot */
#define DK_FLAG		1		/* Search dkl_flag for DK_ROOT */
#define DK_MINOR	2		/* Use boot device minor number to */
					/* index into dkl_part */

#ifdef	_KERNEL

extern	int label_configured;

/*****************************************************************************/
/*	These macros define the interface to the disk labelling system.	     */
/*	On a system that supports labelling, LABELLED should be defined.     */
/*	On a system that runs the labelling code, "label_configured"	     */
/*	should be set, and the disk labelling library included in the	     */
/*	kernel.								     */
/*****************************************************************************/

#ifdef	LABELLED
extern struct buf * label_read();
/* is this buffer being used for a label read? */
#define	LABEL_READ_BP(x)	(label_configured ? label_read_bp(x) : 0)

/* can this device be label_configured? */
#define	LABEL_OK(x)		(label_configured ? label_ok(x) : 0)

/* set CANT_LABEL flag due to I/O error  */
#define	LABEL_NOT_OK(x)		(label_configured ? label_not_ok(x) : 0)

/* perform the next action to read in the label (returns a buffer on the
 * i/o queue */
#define LABEL_READ(u,v,w,x,y,z)	(label_configured ? label_read(u,v,w,x,y,z) : 0)

/* is the label in memory? */
#define	LABEL_IN_MEM(x)		(label_configured ? label_in_mem(x) : 1)

/* clear the label (called after every device reset) */
#define	LABEL_CLEAR(x)		(label_configured ?  label_clear(x) : 0)

/* initialize the label structure for this device control structure */
/* NOTE: this routine is not protected by "label_configured", because it
 * needs to be called before subsys_init().  This doesn't matter,
 * because it only gets called during system initialization, anyway.
 */
#define LABEL_LU_INIT(x)	label_lu_init(x)

/* turn off labelling in order to do coredumps */
#define LABEL_OFF		(label_configured = 0)

#else	/* !LABELLED */

/* NULL macros corresponding to those defined above */
#define	LABEL_READ_BP(x)	0
#define	LABEL_OK(x)	0
#define LABEL_NOT_OK(x)	0
#define	LABEL_CLEAR(x)	
#define	LABEL_IN_MEM(x)	1
#define LABEL_READ(u,v,w,x,y,z)	0
#define LABEL_LU_INIT(x)
#define LABEL_OFF

#endif	/* !LABELLED */


/* status flags for dlb_flag */

#define	DLB_VALID	0x1		/* label is valid */
#define	DLB_NOLABEL	0x2		/* no label on disk */
#define	DLB_INCORE	0x4		/* label has been read */
#define	DLB_CANT_LABEL	0x8		/* ignore labels on this disk */

#define	EREQUEUE	900		/* special error return for labelproc */

#ifndef b_cylin
#define	b_cylin		b_resid		/* re-defined here, as elsewhere */
#endif

/* holds string "LABEL     " */
extern char LABEL_NAME[];

#endif	/* _KERNEL */

/* the structure included in the disk control structure to hold the label */
struct disk_label {
	long			 dlb_flag;	/* see DLB_* */
	struct label_proc	*dlb_lbp;	/* -> labelproc struct */
	struct dsk_label	*dlb_label;	/* -> actual disk label */
};

/* all sizes and offsets in QRT_K chunks */
struct label_proc {
	struct label_proc	*lbpr_next;	/* free list */
	struct disk_label	*lbpr_dlp;	/* active disk structure */
	struct buf		*lbpr_bp;	/* i/o buffer */
	struct buf		*(*lbpr_nextop)();	/* next labelproc op */
	int			lbpr_nerror;	/* errors while trying to read
						 * disk label */
	int			lbpr_dstart;	/* start of lif directory */
	int			lbpr_dsize;	/* size of lif directory */
	int			lbpr_start;	/* start of lif file */
	int			lbpr_size;	/* size of lif file */
	int			lbpr_csize;	/* size of cylinder */
	int			lbpr_bytesread;	/* number of bytes read in op */
	int			lbpr_byteswrite;/* number of bytes written */
	struct dentry		lbpr_lfile;	/* lif directory entry */
	struct dsk_label	lbpr_label;	/* local copy of disk label */
};


#endif /* _SYS_DSK_LABEL_INCLUDED */
