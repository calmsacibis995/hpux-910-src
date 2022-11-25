/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/graphics.h,v $
 * $Revision: 1.6.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 16:41:15 $
 */
/* @(#) $Revision: 1.6.84.4 $ */    

#ifndef _SYS_GRAPHICS_INCLUDED
#define _SYS_GRAPHICS_INCLUDED

#ifdef __hp9000s300
#   ifdef _KERNEL_BUILD
#	include "../h/ioctl.h"
#	include "../h/types.h"
#	include "../h/region.h"
#	include "../h/vas.h"
#	include "../graf.300/sti.h"
#   else /* ! _KERNEL_BUILD */
#	include <sys/ioctl.h>
#   endif /* _KERNEL_BUILD */


/************************************************************************/
/* crt_frame_buffer_t                                                   */
/*                                                                      */
/* This structure is returned by the driver when called with the        */
/* GCDESCRIBE command.  It provides information sufficient for the      */
/* generation of graphics on the frame buffer.                          */
/*                                                                      */
/************************************************************************/

#define CRT_MAX_REGIONS 6
#define CRT_NAME_LENGTH 32


typedef struct 
{
    int     crt_id;             /* display identifier                   */
    int     crt_map_size;       /* size in bytes of the total memory 	*/
    				/* the system maps in for the frame	*/
				/* buffer.				*/
    int     crt_x;              /* width in pixels (displayed part)     */
    int     crt_y;              /* length in pixels (displayed part)    */
    int     crt_total_x;        /* width in pixels: total area          */
    int     crt_total_y;        /* length in pixels: total area         */
    int     crt_x_pitch;        /* length of one row in bytes: total    */
				/* area					*/
    int     crt_bits_per_pixel; /* maximum number of bits that can 	*/
				/* be associated with one screen pixel 	*/
				/* with a one word read or write	*/
    int     crt_bits_used;      /* number of those bits that actually  	*/
				/* can contain valid data.		*/
    int     crt_planes;         /* number of frame buffer planes.  A	*/
                          	/* system wih 8 planes can display 	*/
				/* 256 (2^8) colors.			*/
    int     crt_plane_size;     /* size in bytes of the total frame	*/
    				/* buffer area of a minimun configur-	*/
				/* ation of this graphics device	*/
    char    crt_name[CRT_NAME_LENGTH];  /* null terminated product name */
    unsigned int   crt_attributes;     	/* flags denoting attributes    */
    char    *crt_frame_base;    /* address of the first word in the	*/
                                /* frame buffer memory.            	*/
    char    *crt_control_base;  /* address of the first word of the 	*/
                                /* control registers of the device      */
    char    *crt_region[CRT_MAX_REGIONS];
                                        /* other regions associated     */
                                        /* with the frame buffer that   */
                                        /* might be mapped in      	*/
} crt_frame_buffer_t;


/************************************************************************/
/* Definitions of attributes that a frame buffer might possess.  These  */
/* are different bits in a 'crt_frame_buffer.crt_attributes' field.     */
/************************************************************************/

#define CRT_Y_EQUALS_2X         0x01    /* pixel height is 2x width     */
#define CRT_BLOCK_MOVER_PRESENT 0x02    /* hardware Block Mover exists  */
#define CRT_ADVANCED_HARDWARE   0x04    /* a hardware Transform Engine  */
                                        /* or other sophisticated       */
                                        /* features exist               */
#define CRT_CAN_INTERRUPT       0x08    /* the device is capable of     */
                                        /* generating interrupts        */
#define CRT_GRAPHICS_ON_OFF     0x10    /* has GCON, GCOFF capability   */
#define CRT_ALPHA_ON_OFF        0x20    /* has GCAON, GCAOFF capability */
#define CRT_VARIABLE_Y_SIZE     0x40    /* number of lines in the frame */
                                        /* buffer memory is variable;   */
                                        /* this depends on control      */
                                        /* register settings.           */
#define CRT_ODD_BYTES_ONLY      0x80    /* use only odd frame buffer    */
                                        /* bytes                        */
#define CRT_NEEDS_FLUSHING      0x100   /* the system hardware cache    */
                                        /* needs to be flushed in order */
                                        /* to make the memory image     */
                                        /* current with the display     */
#define CRT_DMA_HARDWARE	0x200   /* the "display card" is capable*/
					/* of supporting hardware DMA   */
#define CRT_VDMA_HARDWARE	0x400	/* the interface card is capable*/
                                        /* of supporting hardware VDMA  */


/* graphics ioctl command defines */
#define	GCID	     		_IOR('G',0,int)
#define	GCON			_IO('G',1)
#define	GCOFF			_IO('G',2)
#define	GCAON			_IO('G',3)
#define	GCAOFF			_IO('G',4)
#define	GCMAP			_IOWR('G',5,int)
#define	GCUNMAP			_IOWR('G',6,int)
#define	GCLOCK			_IO('G',7)
#define	GCUNLOCK		_IO('G',8)
#define	GCLOCK_MINIMUM		_IO('G',9)
#define	GCUNLOCK_MINIMUM 	_IO('G',10)
#define	GCSTATIC_CMAP	 	_IO('G',11)
#define	GCVARIABLE_CMAP  	_IO('G',12)
#define	GCSLOT		 	_IOWR('G',13,struct gcslot_info)
#define GCDMA_BUFFER_ALLOC      _IOWR('G',14,struct graphics_work_buffer)
#define GCDMA_BUFFER_FREE       _IOW('G',15,struct graphics_work_buffer)
#define GCDMA_TRY_LOCK          _IOR('G',16,int)
#define GCDESCRIBE		_IOR('G',21,crt_frame_buffer_t)
#define GCDIAG_COMMAND		_IOWR('G',99,struct crt_diag_args)


/************************************************************************/
/* gcslot_info								*/
/*                                                                      */
/* This structure is returned by the driver when called with the        */
/* GCSLOT command.  							*/
/*                                                                      */
/************************************************************************/

struct gcslot_info {
	int	myslot_number;	 	/* fastlock slot of caller	*/
	unsigned char *myslot_address;	/* addr. of shared lock page	*/
};

/************************************************************************/
/* graphics_work_buffer                                                 */
/*                                                                      */
/* This structure is used to provide data to the GCDMA_BUFFER_SET       */
/* command.                                                             */
/*                                                                      */
/************************************************************************/

struct graphics_work_buffer {
    unsigned char *work_buffer_0;    /* Pointer to work buffer 0 (2k align)*/
    unsigned char *work_buffer_1;    /* Pointer to work buffer 1 (2k align)*/
    int spare1;                      /* Allow for future needs             */
    int spare2;
    int spare3;
    int spare4;
};


/************************************************************************/
/* crt_diag_args	                                                */
/*                                                                      */
/* This structure is used to provide data to the GCDIAG_COMMAND	        */
/* command, and is used for diagnostics.                                */
/*                                                                      */
/************************************************************************/

struct crt_diag_args {
    int 	 request;	 	/* function to perform		*/
    union {
	unsigned int diag_attributes;	/* flags denoting attributes	*/
    } u_diag;
};


/*
 * Legal diagnostic request values for the request field of crt_diag_args
 */
#define GRAPH_DIAG_RESET	4	/* perform hard reset		*/
#define GRAPH_DIAG_ATTRIBUTES	6	/* privileged attributes	*/

/*
 * Bit definitions of attributes as found in crt_diag_args.diag_attributes
 */
#define GRAPH_DIAG_ITE		0x1	/* ITE on this display		*/


/*
 * Following are the Frame Buffer Identification numbers.  Each frame
 * buffer has a unique id returned by the GCDESCRIBE ioctl() command
 * in the 'crt_id' field.  The same id is also returned by the GCID
 * call.
 *
 * The Series 800 identifies every hardware graphics subsystem uniquely, while
 * the Series 300 often groups families of graphics subsystems together
 * under one major category.  This is, for instance, why S9000_ID_98550
 * is only defined below for the Series 800, because the Series 300 has
 * historically grouped it under the ID for a family of displays.
 */
#define S9000_ID_98204A		1
#define S9000_ID_9826A		2
#define S9000_ID_9836A		3
#define S9000_ID_9836C		4
#define S9000_ID_98627A		5
#define S9000_ID_98204B		6
#define S9000_ID_9837		7
#define S9000_ID_98700		8
#define S9000_ID_S300		9
#define S9000_ID_98720		10
#ifdef __hp9000s800
#   define S9000_ID_98550	11
#endif
#define S9000_ID_A1096A		12
#define S9000_ID_FRI		13
#define S9000_ID_98730		14
#define S9000_ID_98705		15
#define S9000_ID_98736		16
#define EVRX_ID_COLOR_1280	0x27134c9f
#define EVRX_ID_GREYS_1280	0x27134cc5
#define EVRX_ID_COLOR_1024	0x27134c8e
#define EVRX_ID_COLOR_0640	0x27134cb4
#define EVRX_ID_GREYS_0640	0x2739d2f2
#define GRX_ID_GREYS_1280	0x26d1488c	/* A1942A */
#define CRX_ID_COLOR_1280	0x26d1482a	/* A1659A */

/* graphics minor number macros */
#define	GRAPH_SC(x)	(((x) >> 16) & 0xff)
#define GRAPH_TYPE(x)   (((x) >>  8) & 0x0f)
#define GRAPH_DIAG(x)   (((x) >> 15) & 0x01)

/* maximum number of processes with graphics device open */
#define	MAX_GRAPH_PROCS	255

#ifdef _KERNEL

/* Shadow page table structures and constants. It is assumed that if any  */
/* future interfaces make use of shadow page tables that they will share  */
/* the same basic structure; therefore, the shadow page table constants   */
/* and structure definitions are here instead of in an interface specific */
/* header file. If this assumption turns out to be false, these def-      */
/* initions should be moved to a interface specific header file. On the   */
/* other hand, if some other interface (e.g. a network interface) uses    */
/* shadow page tables then this information should be moved to a header   */
/* file of its own.                                                       */

#define NSVASSEG          32
#define NSPT_PER_SVASSEG  32
#define NSPTE_PER_PAGE    1024
#define NSSTE_PER_PAGE    1024

struct ssegtable {
    unsigned long   sste[NSSTE_PER_PAGE];
};

struct spagetable {
    unsigned long   spte[NSPTE_PER_PAGE];
};

struct spt_block {
    struct spagetable *shpagetables[NSPT_PER_SVASSEG];
};

/* Macros for indexing into spt_block pointer table, spt_block, segment */
/* table and page table. These macros need to change if NSVASSEG,       */
/* NSPT_PER_SVASSEG, NSPTE_PER_PAGE, or NSSTE_PER_PAGE change.          */

#define SPT_BLPTR_INDEX(x) (((x) >> 27) & 0x1f)
#define SPT_BLOCK_INDEX(x) (((x) >> 22) & 0x1f)
#define SSEG_TBL_INDEX(x)  (((x) >> 22) & 0x3ff)
#define SPAGE_TBL_INDEX(x) (((x) >> 12) & 0x3ff)

/* Gdev_info structure. Contains interface specific information. Currently */
/* only a Genesis specific section is defined. This structure is split out */
/* of the graphics_info structure so that interfaces that don't need this  */
/* structure will not have to waste the memory associated with it.         */

struct gdev_info {
    union {
	struct {
	    unsigned long work_buffer_0_ptr;  
	                             /* physical address of work buffer 0  */
	    caddr_t       work_buffer_0_uvptr;
	                             /* user space virtual pointer of wb 0 */
	    unsigned long work_buffer_1_ptr;  
	                             /* physical address of work buffer 1  */
	    caddr_t       work_buffer_1_uvptr;
	                             /* user space virtual pointer of wb 0 */
	    unsigned long root_ptr;  /* physical address of segment table  */
	    struct   ssegtable *segment_table; 
	                             /* kernel virtual address             */
	    struct   spt_block *spt_block[NSVASSEG];
	} genesis_info;
    } g_union;
};

/* Graphics_info structure. Contains process specific information related */
/* to a graphics interface. There is one of these for each process that   */
/* has opened a graphics interface.                                       */

struct graphics_info {
    pid_t pid;                        /* pid of process owning this slot    */
    vas_t *gi_vas;                    /* Virtual Address space for this proc*/
    space_t gi_space;                 /* Space number for this proc         */
    struct proc *gi_proc;             /* proc pointer for this proc.        */
    int vdmad_fault_flag;             /* Is fault being processed by vdmad? */
    unsigned char *slot_page_base;    /* user virtual pointer to lock page  */
    long  gcslot_count;		      /* number of GCSLOTS		    */
    char *crt_control_base;           /* first word of control register mem */
    char *crt_frame_base;             /* first word in frame buffer memory  */
    long  gcmap_count;		      /* number of GCMAPS		    */
    struct  gdev_info *gdev;          /* pointer to device specific info    */
};

/* Graphics_descriptor structure. Contains information related to a specific*/
/* graphics interface. There is one of these for each graphics interface.   */

struct graphics_descriptor {
	struct graphics_descriptor *next;
	unsigned int  flags;
	int  count;
	unsigned char type;
	unsigned char id;
	unsigned char isc;
	unsigned char spare1;			/* for long word alignment   */
	char *primary;
	int	psize;
	char *secondary;
	int	ssize;
	char *gcntl;
	char	goff;
	char	gon;
	char	spare2;				/* for long word alignment   */
	char	spare3;

/* stuff added for fast graphics lock */
	int		 g_map_size;		/* size of GCMAP frame       */
	reg_t *g_rp_slot;			/* region ptr for GCSLOT     */
	reg_t *g_rp_map;			/* region ptr for GCMAP      */
	struct proc 	*g_locking_proc;	/* locking process proc pntr */
	int              g_locking_caught_sigs; /* sigs caught at calltime   */
	int              g_lock_slot;           /* locking proc slot numb    */
	int              g_last_dma_lock_slot;  /* last slot to have dma lock*/
	unsigned char 	*g_lock_page_addr;	/* kernel addrs of slot page */
	unsigned long    save_io_status;        /* saved interrupt status    */
	int 		 g_numb_slots;		/* current max number slots  */
	struct graphics_info *g_info_array[MAX_GRAPH_PROCS]; /* per proc info*/

	crt_frame_buffer_t desc;		/* Descriptor for GCDESCRIBE */

	int (*init_graph)();			/* STI routine from rom */
	int (*state_mgmt)();			/* STI routine from rom */
	int (*font_unpmv)();			/* STI routine from rom */
	int (*block_move)();			/* STI routine from rom */
	int (*inq_conf)();			/* STI routine from rom */
	glob_cfg glob_config;
};

/* Possible types for graphics_descriptor->type & minor number */
#define LOW_TYPE  0
#define HIGH_TYPE 1
#define DIO_TYPE  2
#define SGC_TYPE  3

/* flag field defintions */
#define	GRAPHICS 0x01		/* 1 = graphics on; 0 = graphics off */
#define	ALPHA	 0x02		/* alpha plane present */
#define	MULTI	 0x04		/* uses multiple mapping for on/off */
#define	FRAME	 0x08		/* has separate control and frame */
#define	DEFAULT  0x10		/* default internal device */
#define	DEFAULT2 0x20		/* secondary internal device */
#define	G_GCLOCK 0x40		/* device locked with GCLOCK, not MINIMUN */
#endif /* _KERNEL */

#define GRAF_PRIMARY_OFFSET		0x1
#   define HP_BITMAP_DISPLAY_PRIMARY_ID		0x39
#define GRAF_SECONDARY_OFFSET		0x15
#   define HPGATOR_SECONDARY_ID			0x1
#   define HPBOBCT_SECONDARY_ID			0x2
#   define HP98720_SECONDARY_ID			0x4
#   define HP98549_SECONDARY_ID			0x5
#   define HP98550_SECONDARY_ID			0x6
#   define HP98548_SECONDARY_ID			0x7
#   define HP98730_SECONDARY_ID			0x8
#   define HP98541_SECONDARY_ID			0x9
#   define HPWAMPM_SECONDARY_ID			0xb
#   define HP98705_SECONDARY_ID			0xc
#   define HPBLKFT_SECONDARY_ID			0xd
#   define HPHYPER_SECONDARY_ID			0xe

#endif /* __hp9000s300 */

#endif /* _SYS_GRAPHICS_INCLUDED */
