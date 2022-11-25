/* $Header: io.h,v 1.6.83.4 93/09/17 18:27:15 kcs Exp $ */      

#ifndef _SYS_IO_INCLUDED /* allows multiple inclusion */
#define _SYS_IO_INCLUDED

#ifdef _WSIO	/* Only a workstation header */
#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_STDSYMS_INCLUDED  */

#ifdef _INCLUDE_HPUX_SOURCE

#ifndef _SYS_TYPES_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_TYPES_INCLUDED  */

#ifdef _KERNEL_BUILD
#  include "../wsio/timeout.h"
#  include "../h/hpibio.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/timeout.h>
#  include <sys/hpibio.h>
#endif /* _KERNEL_BUILD */


struct gfsw {
   int	(*init)();	/* Drv initialization routine		      */
   int	(*diag)();	/* Drv diagnostic routine (future)	      */
};

/* General drv info all drivers can have.  Fields must be bus-independent */
struct gdd {
   struct buf *b_actf;
   struct buf *b_actl;
   unsigned char comm_id;	/* Serves as a communication ID between device
				   and interface drivers.  Used for console
				   and rootdev initialization. */
   caddr_t diag_drv;            /* Pointer to the data structure used by
				   the diagnostic device driver */
};

#ifdef OSDEBUG
#define IOASSERT(EX) if (!(EX)) assfail("EX", __FILE__, __LINE__)
#else
#define IOASSERT(EX)
#endif


#if (defined(_WSIO) && defined(__hp9000s800))
#define get_id(isc,id) \
	id = isc->g_drv_data->comm_id;

#define set_id(isc,id) \
	isc->g_drv_data->comm_id = (unsigned char )id;
#endif /* s700 */

/* bus_type definitions		*/
#ifdef __hp9000s300
#define EISA_BUS	1
#define VME_BUS		4
#else
#define EISA_BUS	0x76		/* IODC sversion.model */
#define VME_BUS		0x78		/* IODC sversion.model */
#endif

#define DIO_BUS 	3
#define SGC_BUS		5
#define UNKNOWN_BUS	0
#define PA_CORE		0x70		/* IODC sversion.model */

#define m_function(dev) (int)((unsigned) (dev)>>12&0xf)
#define get_isc(dev, isc) \
	isc = isc_table[m_selcode(dev)]; \
	if (isc != NULL) {	\
		if (isc->ftn_no != -1) { \
			while (m_function(dev) != isc->ftn_no) { \
				isc = isc->next_ftn; \
				if (isc == NULL) \
					break; \
			} \
		} \
	}


/*
 * maintainable I/O documented malloc/free interface flags
 */
#define IOM_WAITOK    1
#define IOM_NOWAIT    2

/*
 * flags for io_testr() and io_testw()
 */
#define BYTE_WIDE     1
#define SHORT_WIDE    2
#define LONG_WIDE     4

#if (defined(__hp9000s800) && defined(_WSIO))
typedef struct buflet_info {
	space_t	space;
	caddr_t buflet_beg;
	caddr_t buffer_beg;
	u_int   cnt_beg;
	caddr_t buflet_end;
	caddr_t buffer_end;
	u_int   cnt_end;
} buflet_info_type;
#endif /* s700 */

struct driver_to_call {
	int	(*drv_routine)();
	int	drv_arg;
	struct	isc_table_type *isc;
	struct	driver_to_call *next;
};

#define IOMAP_REF_COUNT 1
#define IOMAP_PERMANENT 2

struct iomap_data {
	u_int entry_value;
};

struct io_parms {
	int	flags;
	int	key;
	int 	num_entries;
	int	(*drv_routine)();
	int	drv_arg;
	caddr_t	host_addr;
	u_int	spaddr;
	u_int	size;
};

struct addr_chain_type {
	caddr_t phys_addr;
	unsigned int count;
};

struct dma_parms {
	int	channel;
	int	dma_options;
	int	flags;
	int	key;
	int 	num_entries;
#if (defined(__hp9000s800) && defined(_WSIO))
	buflet_info_type *buflet_key;
#endif /* s700 */
	struct addr_chain_type *chain_ptr;
	int	chain_count;
	int	chain_index;
	int	(*drv_routine)();
	int	drv_arg;
	int	transfer_size;
	caddr_t addr;
	space_t	spaddr;
	int	count;
};


struct rsrc_node {
	int entry_number;        /* the starting entry number */
	int size;                /* the size of the block */
	struct rsrc_node *prev;  /* prevoius node pointer */
	struct rsrc_node *next;  /* next node pointer */
};


struct bus_info_type {
        int                   bus_type;         /* indicates the bus type */
	struct isc_table_type *isc;
        struct driver_to_call *iomap_wait_list; /* drivers waiting on entries */
        struct rsrc_node      *iomap_rsrcmap;   /* size and availability info */
        int                   *host_iomap_base; /* host virtual addr of base */
        int                   *bus_iomap_base;  /* bus physical addr of base */
        caddr_t               spec_bus_info;    /* specific bus info for bus */
};



#ifdef _WSIO
enum TFR_type {NO_TFR, DMA_TFR, INTR_TFR, FHS_TFR, BURST_TFR, BO_END_TFR, BUS_TRANSACTION};
#endif

/*
**  interface driver space
*/
   struct isc_table_type {
	struct buf *b_actf;
	struct buf *b_actl;
	struct buf *ppoll_f;	/* those waiting for ppoll */
	struct buf *ppoll_l;
	struct buf *event_f;	/* those waiting for events */
	struct buf *event_l;
	struct buf *status_f;	/* those waiting for status */
	struct buf *status_l;
	unsigned int state;
	char card_type;
	unsigned char my_isc;
	char my_address;
	char active;
	char int_lvl;
	unsigned char spoll_byte;
	enum TFR_type transfer;
	struct drv_table_type *iosw;
	struct tty_drivercp *tty_routine;	/* for tty's selectcodes */
	int *card_ptr;
	unsigned short intcopy;
	unsigned short intmsksav;
	unsigned short intmskcpy;
	struct dma_channel *dma_chan;
	short dma_reserved;		/* dma not to be locked by another sc */
	short dma_active;		/* sc hopes to use dma */
	unsigned char int_flags;
	unsigned char int_enabled;
	struct buf *owner;
 	char ppoll_flag;		/* info for ti9914 ppoll requests */
 	unsigned char ppoll_mask;
 	unsigned char ppoll_sense;
	unsigned char tfr_control;	/* DIL transfer type and term reason */
 	unsigned short ppoll_resp;	/* ppoll response */
	unsigned short pattern; 	/* pattern for transfer termination */
 	int resid;
 	caddr_t buffer;			/* transfer information */
 	int count;
	struct sw_intloc intloc;	/* for software triggers */
	struct sw_intloc intloc1;	/* for software triggers */
	struct sw_intloc intloc2;	/* for software triggers */
	int lock_count;			/* those waiting to lock the bus */
	int intr_wait;			/* interrupt waiting conditions */
	int locks_pending;
	int mapped;			/* keep a copy of the mapped in address */
	int transaction_state;
	int (*transaction_proc)();
	pid_t locking_pid;

	char 		bus_type;
	struct bus_info_type *bus_info;
	int 		if_id;
	caddr_t		if_reg_ptr;
	caddr_t 	if_info;
	int 		ftn_no;
	struct isc_table_type *next_ftn;
	struct gfsw 	*gfsw;
	caddr_t 	ifsw;
	struct gdd	*g_drv_data;
	caddr_t 	if_drv_data;
	struct dma_parms *dma_parms;
   };


#ifdef __hp9000s300
#  define MINISC  	 	0
#  define EXTERNALISC		32
#  define MAXEXTERNALISC	63
#  define MIN_DIOII_ISC		132
#  define MAXISC		255
#  define ISC_TABLE_SIZE	(MAXISC + 1)

/*
**  isc_table includes select codes 0-255!
*/
struct isc_table_type *isc_table[ISC_TABLE_SIZE];

#else /* is __hp9000s700 */

/* defines for vsc numbers in minor numbers */
#define SGC_VSC1        0x0
#define SGC_VSC2        0x1
#define CORE_VSC	0x2
#define EISA_VSC	0x4

/* defines for isc->if_id */
#define SCSI_SV_ID	0x71			/* CORE SCSI */
#define LAN_SV_ID	0x72
#define HIL_SV_ID	0x73
#define CENT_SV_ID	0x74
#define SERIAL_SV_ID	0x75
#define SGC_SV_ID	0x77
#define SCSI_FW_SV_ID	0x7c			/* CORE Fast Wide SCSI */

#define SCSI_F_EISA_SV_ID	0x22f00c80	/* EISA Fast SCSI */
#define SCSI_EISA_SV_ID		0x22f00c90	/* EISA SCSI */

/* there is no snooze on s800 for s700 but busywait does the same thing */
#define snooze busywait


#define PA_ISC_TABLE_SIZE	256	/* 2^^8: number of significant bits  */
					/* in the minor # to get to the slot */
/* Extern to pull in the definition from vsc_config.c.  Used by get_isc */
extern struct isc_table_type *isc_table[];

struct cons_msus_type {
	int sversion;		/* ID of the interface card. */
	int mod_path[9];	/* Module path to the console */
	int bus_type;		/* Console bus type (EISA, SGC, CORE). */
};

struct msus_type {
	int sversion;		/* ID of the interface card booted from. */
	int access_type;	/* Access type (Sequential,random,etc). */
	int mod_path[9];	/* Module path to the boot device. Depends on
				   bus type. */
	int bus_type;		/* Type of bus booted from.(SGC, CORE, EISA) */
	char prod_id[16];	/* Product/Vendor ID of the device.  See 
				   io_build_boot_msus() for more details. */
};

#define DMA_REGISTERS	0xf0820000		/* Core DMA registers */
#define DMA_RESET	0xf0827000		/*  for Parallel intfc*/
#define CORE_EEPROM	0xf0810000		/* for LAN to use     */

#endif /* __hp9000s800 && _WSIO */
#endif /* _INCLUDE_HPUX_SOURCE */
#endif /* _WSIO */

#endif /* _SYS_IO_INCLUDED */
