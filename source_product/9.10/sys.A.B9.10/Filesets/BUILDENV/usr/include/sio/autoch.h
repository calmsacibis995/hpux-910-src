/* @(#) $Revision: 1.7.83.3 $ */

#ifndef _SIO_AUTOCH_INCLUDED /* allows multiple inclusion */
#define _SIO_AUTOCH_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifndef _KERNEL_BUILD
#include <sys/ioctl.h>
#endif /* ! _KERNEL_BUILD */

/*
 * This structure holds the request queue constants.
 */
struct queue_const {
	int wait_time;
	int hog_time;
};

/*
 * This structure holds queue statistics.
 */
struct queue_stats {
	long size;	/* number of requests in swap request queue */
	long mix;	/* number of different surfaces in queue */
};

/*
 * ioctl commands
 */
#define ACIOC_MOVE_MEDIUM		_IOW('S', 1, struct move_medium_parms)
#define ACIOC_INITIALIZE_ELEMENT_STATUS _IO('S', 2)
#define ACIOC_WRITE_Q_CONST             _IOW('S', 3, struct queue_const)
#define ACIOC_READ_Q_CONST              _IOR('S', 4, struct queue_const)
#define ACIOC_READ_ELEMENT_STATUS	_IO('S', 5)
#define ACIOC_READ_Q_STATS		_IOR('S', 6, struct queue_stats)
#define ACIOC_FORMAT			_IOW('S', 7, int)

/* field definitions for surface minor numbers */
#ifndef _WSIO
#define ac_addr(x) (int)((unsigned)(x)>>16&0xff)
#define ac_section(x) (int)((unsigned)(x)>>12&0xf)
#define drive_section(x) (int)((unsigned)(x)&0xf)
#endif /* s800 */

#ifdef __hp9000s700
#define ac_section(x) 0
#define ac_addr(x) (int)((unsigned)(x)>>9&0x7)
#define ac_card(x) ((int)((unsigned)(x)>>12&0xfff))
#define ac_minor(sc, ba, lu, vl) ((long)((sc)<<12|(ba)<<8|(lu)<<4|(vl)))
#define drive_section(x) (int)((unsigned)(x)&0xf)
#define MAKE_SURFACE_MINOR(card, addr, sur) ((long)((card)<<12|(addr)<<9|(sur)))
#endif /* s700 */

#ifdef __hp9000s300
#define ac_section(x) 0
#define ac_addr(x) (int)((unsigned)(x)>>12&0xf)
#define ac_minor(sc, ba, un, vl) makeminor((sc), (ba), (un), (vl))
#define ac_card(x) m_selcode((x))
#endif /* s300 */

#define ac_surface(x) (int)((unsigned)(x)&0x1ff)

#ifdef _WSIO
#define AC_MAJ_CHAR 55  /* autochanger driver major character number */
#define AC_MAJ_BLK 10   /* autochanger driver major block number */
#else /* s800 */
#define AC_MAJ_CHAR 19  /* autochanger driver major character number */
#define AC_MAJ_BLK 12   /* autochanger driver major block number */
#endif  /* _WSIO */

#ifdef _KERNEL
#ifdef KERNEL_DEBUG_ONLY
struct {
        unsigned release_element:1;             /* 0x80000000 */
        unsigned reserve_element:1;             /* 0x40000000 */
        unsigned release_drives:1;              /* 0x20000000 */
        unsigned reserve_drives:1;              /* 0x10000000 */
        unsigned autoch_scsi_open:1;            /* 0x08000000 */
        unsigned ac_open_sleep:1;               /* 0x04000000 */
        unsigned get_storage_offset:1;          /* 0x02000000 */
        unsigned get_transport_address:1;       /* 0x01000000 */
        unsigned get_drive_info:1;              /* 0x00800000 */
        unsigned surface_in_ac:1;               /* 0x00400000 */
        unsigned ioctl_rezero_unit:1;           /* 0x00200000 */
        unsigned state_flush:1;                 /* 0x00100000 */
        unsigned state_move_out:1;              /* 0x00080000 */
        unsigned state_move_in:1;               /* 0x00040000 */
        unsigned state_spinup:1;                /* 0x00020000 */
        unsigned move_home:1;                   /* 0x00010000 */
        unsigned move_to_drive:1;               /* 0x00008000 */
        unsigned wait_spinup_sleep:1;           /* 0x00004000 */
        unsigned wait_spinup:1;                 /* 0x00002000 */
        unsigned call_sync_op:1;                /* 0x00001000 */
        unsigned state_init_el:1;               /* 0x00000800 */
        unsigned no_daemons:1;                  /* 0x00000400 */
        unsigned always_deadlock:1;             /* 0x00000200 */
        unsigned kill_xportd:1;                 /* 0x00000100 */
} force_error;
#endif KERNEL_DEBUG_ONLY

/* defines for element status */
#define INVERT_MSK        0x40
#define SOURCE_VAL_MSK    0x80
#define FULL_MSK          0x01
#define EL_STATUS_HDR_LEN 0x08
#define PC_HDR_LEN        0x08
#define EL_STATUS_SZ      0x1000
#define NOT_THIS_BUS_MSK  0x80
#define ID_VAL_MSK        0x20

/* page codes for element status */
#define TRANSPORT_PC     0x01
#define STORAGE_PC       0x02
#define INPUT_OUTPUT_PC  0x03
#define DATA_TRANSFER_PC 0x04
#define ALL_PC           0x00

/* for SCSI reserves */
#define RESERVE_ELEMENT_LIST_LENGTH 6

#define PMOUNT_BUF_SIZE 0x2000

#ifdef _WSIO
/* For 700 and 300 */

#define SCSI_MAJ_CHAR 47     /* SCSI driver major character number */
#define SCSI_MAJ_BLK 7      /* SCSI driver major block number     */
#define AUTOX0_MAJ_CHAR 33      /* autox driver major character number */
#define MAX_SECTION 0      /* 300 does not support sections */
#define ENTIRE_SURFACE 0      /* section number for entire disk */

#else /* ! _WSIO */
/* For 800 */

#define SCSI_MAJ_CHAR 13     /* SCSI driver major character number */
#define SCSI_MAJ_BLK 7     /* SCSI driver major block number     */
#define AUTOX0_MAJ_CHAR 33      /* autox driver major character number */
#define MAX_SECTION 15      /* highest number of the supported sections 0-15 */
#define ENTIRE_SURFACE 2      /* section number for entire surface */

#endif /* _WSIO */

#define EMPTY -1

#define SPINUP_WAIT 8000       /* time in milliseconds to wait for spin up.
                                 * used after initialize_element_status
                                 */

#define WAIT_INTERVAL 500       /* time in milliseconds */
#define INITIAL_WAIT_INTERVAL 2000      /* time in milliseconds */
#define MAX_WAIT_INTERVALS 40   /* maximum number of WAIT_INTERVALs 
                                before error */

#define DEFAULT_WAIT_TIME 1     /* time in seconds */
#define DEFAULT_HOG_TIME 20     /* time in seconds */
#define DEFAULT_CLOSE_TIME 10   /* time in seconds */

#define MAX_SUR 512            /* maximum number of surfaces per ac */
#define NUM_DRIVE 6            /* number of drives per ac */
#define NUM_AC  4               /* maximum number autochangers on host */

#define MAKE_WORD(hb,lb) (((unsigned)(hb & 0xff) << 8) + ((unsigned)lb & 0xff))

/* these three are tied together */
#define SURTOEL(surface,offset) ((surface < 0) ? surface : ((surface - 1) / 2) + offset)
#define ELTOSUR(element,offset,flipped) (((element - offset) * 2) + 1 + flipped)
#define FLIP(x) (!(x & 0x01))

/* mask to zero out the address bits in a SCSI dev_t */
#ifdef __hp9000s700
#define ZERO_ADDR_MSK makedev(0xff,ac_minor(0xfff,0,0xf,0xf))
#else
/* Same for both 300 and 800 */
#define ZERO_ADDR_MSK makedev(0xff,makeminor(0xff,0,0xf,0xf))
#endif

#define IS_STORAGE(element,ac_device_ptr) (element >= ac_device_ptr->storage_element_offset)

#define try

#define INITIALIZING            1
#define INITIALIZING_WANTED     2

#define WANT_OPEN               0x01
#define IN_OPEN                 0x02
#define WANT_CLOSE              0x04
#define IN_CLOSE                0x08

#define WANT_MOUNT              0x01
#define IN_MOUNT                0x02
#define WANT_PMOUNT             0x04
#define IN_PMOUNT               0x08
#define WANT_PUNMOUNT           0x10
#define IN_PUNMOUNT             0x20

#define REZERO_BUSY 1
#define REZERO_WANTED 2

#define INIT			0x01
#define PUNMOUNT		0x02

/* values for AC_DEBUG */
#define TOP_CASE  0x0001 /* print the switch variable for top case 
                                                             in process_op */
#define BOT_CASE  0x0002 /* print the switch variable for bottom case
                                                             in process_op */
#define WAIT_CASE 0x0004 /* print the switch variable for case in      
                                                              wait_time_done */
#define OPERATION 0x0008 /* print the operation                              */
#define SPIN_TIME 0x0010 /* print out the spinup time */
#define AC_STATES 0x0020 /* print out the state machine states */
#define PANICS    0x0040 /* panic on all errors */
#define NO_PANICS 0x0080 /* don't panic on all errors */
#define XPORT_OP  0x0100 /* print xport daemon operation */
#define SPINUP_OP 0x0200 /* print spinup daemon operation */
#define QUEUES    0x0400 /* print contents of the queues */
#define PMOUNT    0x0800 /* print pmount stuff */
#define DELAYED_CLOSE 0x1000 /* print delayed close stuff */
#define RESET_DS  0x2000 /* print reset_data_structures stuff */
#define DEADLOCK  0x4000 /* print deadlock recovery stuff */
#define DEADPANIC 0x8000 /* panic when deadlock detected */
#define MP_IOCTL  0x10000 /* new ioctl's for woe and wwv */
#define XPORTD_KILL  0x20000 /* xport daemon kill request */
#define OPEN_CLOSE  0x40000 /* open close lock info */

#define escape(x) \
        escapecode = x;\
        goto recover;

#ifdef __hp9000s800

#ifndef _WSIO
        /* temp struct definition until I integrate with Pauls files */
#define SZ_VAR_CNT              0xFF    /* Max Size of Variable Size Packet */
#define makeminor(sc, ba, un, vl) ((long)((sc)<<16|(ba)<<8|(un)<<4|vl))
#define m_busaddr(x) (int)((unsigned)(x)>>8&0xff)
extern struct timeval time;
#define GET_TIME(x) (x = time)
#endif   /* _WSIO */
#endif

#ifdef __hp9000s300  
#define GET_TIME(x) (get_precise_time(&x))
#endif

#ifdef __hp9000s700
#define GET_TIME(x) (uniqtime(&x))
#endif

/*
 * This structure holds all possible parameters called by the driver entry 
 * points. It exists so that the routine process_op can be passed an 
 * operation and the parameters.
 */
struct operation_parms {
        dev_t dev;
        struct uio *uio;
        struct buf *bp;
        struct ioctlarg *v;
        int flag;
        int code;
        struct ac_device *ac_device_ptr;
        struct ac_drive *drive_ptr;
};

enum scheduling_result {AC_DELAY, AC_NO_DELAY, AC_FLUSH};

/*
 * This structure contains the surface requests.  Surface requests are kept on
 * a doubly linked list.
 */
struct surface_request {
        struct surface_request *forw ,*back;  /* doublely linked list         */
        int surface;                       /* surface associated with request */
        int r_flags;                       /* see flag definitions below      */
        short pid;                         /* for swap_stats                  */
        struct buf *bp;
        struct ac_device *ac_device_ptr;
        struct ac_drive *drive_ptr;
        int (*op)();
        struct operation_parms *op_parmp;
        int spinup_polls;                 /* number of times that drive has
                                           * been polled
                                           */
        enum scheduling_result result;
        int case_switch;
};

/*
 * This structure contains the information about a specific section.
 * The physical mount and unmount routin pointers
 * passed in by the file system are stored here.
 */

struct section_struct {
        caddr_t mount_ptr;
        int (*pmount)();
        int (*punmount)();
        int open_count;
	int section_flags;	/* Write Without Erase/Write With Verify */
	int exclusive_lock;     /* Only valid for the entire surface */
        };

/*
 * These flags are kept in r_flags
 */
#define R_NOOP       0x01         /* Request for the surface but no fuction to perform */
#define R_BUSY       0x02         /* surface request is being serviced     */
#define R_ASYNC      0x04         /* op for surface is asynchronous        */

#define S_WOE        0x01         /* Turns on the write without erase during writes */
#define S_WWV        0x02         /* Turns on the write with verify during writes */

#ifndef _WSIO
/* These flags are used in section_flags in the section struct
 * to keep track of char device and block device opens.  This
 * is to make sure a final close does not occure on one device
 * while the other is still open.
 */
#define AC_BLK_OPEN  0x04
#define AC_CHAR_OPEN 0x08 

/* These flags are used in phys_drv_state to keep track of states
 * that can be turned on for a drive, but don't make sense for sections.
 */
#endif /* ! _WSIO */

#define S_IMMED      0x01

enum drive_states {DAEMON_CALL,FLUSH,MOVE_OUT,MOVE_IN,
                   SPINUP,DO_OP,WAIT_TIME,NOT_BUSY,ALLOCATED};
/*
 * This structure contains information about the drive.
 *
 */
struct ac_drive {
   struct ac_device *ac_device_ptr;/* pointer back to ac device         */
   int scsi_address;            /* SCSI id for this drive            */
#if defined(__hp9000s800) && !defined(__hp9000s700)
   int logical_unit;            /* Logical unit number of the drive */
#endif 
   int surface;                 /* surface that is inserted could be EMPTY  */
   int surface_inproc;          /* surface in process,    
                                 * the surface that is on its way in.  */       
   long hog_time_start;         /* start of hog time */
   int ref_count;               /* number of processes using drive   */
   int element_address;         /* autochanger element address of drive */
   int inuse;                   /* boolean for drive in use          */
   int opened;                  /* boolean for drive opened          */
   enum drive_states drive_state;
   struct move_medium_parms *move_parms_ptr;
   struct xport_request *close_request; /* delayed close request */
   struct xport_request *interrupt_request; /* request to be used within an */
					    /* interrupt, cant alloc in interrupt */
   int interleave;              /* for 800 ioctl for format */
   struct surface_request *interrupt_sreqp; /* surface request to be used */
                                            /* within an interrupt */
};

/*
 *
 * This structure contains the xport_request.
 *
 */

struct xport_request {
   struct xport_request *forw, *back;   /* linked list pointers */
   struct move_medium_parms move_request;  /* function to perform  */
   struct ac_drive *drive_ptr;          /* drive to spin up */
   struct surface_request *sreqp;       /* request that started it all */
   struct buf *bp;                      /* buf for async operations */
   int x_flags;                         /* flags, see defn below */
   int error;                           /* error on operation */
   int surface;                         /* surface to spin up */
   short pid;  /* id of process that started the swap, if valid */
   }; 

/* used for x_flags in xport_request */

#define X_DONE          0x01    /* the xport request finished */
#define X_ASYNC         0x02    /* the request that started the 
                                   swap was an async request */
#define X_ERROR         0x04    /* the move had an error */
#define X_NO_WAITING    0x08    /* the surface request sleeping on 
                                   the daemon was killed */
#define X_EXIT          0x10    /* terminate the daemons */
#define X_CLOSE_SURFACE 0x20    /* the request came from the close 
                                   of the surface */
#define X_RESET_AC      0x40    /* Reset the autochanger device */
#define X_FLUSH_CLOSE   0x80    /* Move all close delayed surfaces home */
#define X_RECOVER       0x100   /* Initiate daemon deadlock recovery */
#define X_CLOSE_MOVE    0x200   /* Initiate daemon deadlock recovery */
#define X_KILL_XPORTD   0x400   /* Kill xportd if spinupd doesn't start */


/*
 * This structure holds autochanger information.
 */
struct ac_device {
   struct ac_device *forw, *back;             /* doublely linked list    */
   struct surface_request *sreqq_head;        /* pointer to doubley linked
                                               * list of surface requests
                                               */       
   struct xport_request *xport_queue_head;   /* head of xport request queue */
   struct xport_request *spinup_queue_head;  /* head of spinup request queue */
   dev_t device;                    /* major+minor of mechanical changer */     
   int opened;                                /* true if ac is opened    */
#if defined(__hp9000s800) && !defined(__hp9000s700)
   int logical_unit;               /* Logical unit number of the autochanger */
#endif
   struct ac_drive ac_drive_table[NUM_DRIVE]; /* drive information 
                                               * associated with this ac */
   int number_drives;                         /* number of valid drives  */
   int mailslot_address;                      /* SCSI element address
                                               for mailslot */
   int transport_address;                 /* SCSI element address for picker */ 
   struct section_struct *elements[MAX_SUR][MAX_SECTION+1];
                        /* pointer to Element and section information 
                           in the pairs [element,section].
                           Note:  elements[0][0] is for surface 0 section 0.  */
   int storage_element_offset;                /* the first storage element 
                                               */       
   int ocl;                                   /* for open/close 
                                               * synchronization 
                                               */
   int pmount_lock;                           /* for pmount autoch_mount
                                               * synchronization 
                                               */
   int read_element_status_flag;              /* if set next read to
                                               * autochanger will give
                                               * read_element_status data.
                                               */
   int ac_wait_time[MAX_SUR];                 /* time (in seconds) to wait
                                               * for another operation on
                                               * the same media.
                                               */
   int ac_hog_time[MAX_SUR];                  /* time (in seconds) to keep
                                               * processing on the same
                                               * media.
                                               */
   int ac_close_time;                      /* time (in seconds) to wait for
                                            * another move after a close before
                                            * putting the cartridge away.
                                            */
   int transport_busy;                        /* picker in use */
   int rezero_lock;                        /* to prohibit execution of
                                            * other commands while
                                            * rezero is in progress.
                                            */
   struct buf *xport_bp;		   /* scratch buffers to pass to fs_pmount */
   struct buf *spinup_bp;                  /* functions. 
                                            */
   struct ac_deadlock_struct deadlock;     /* for xport daemon deadlock recovery */
   struct ac_drive *deadlocked_drive_ptr;  /* the drive that might be deadlocked */
   struct xport_request *recover_reqp;     /* for deadlock recovery because cant do
                                              kmemallocs from an interrupt */
   int jammed;                             /* error during move */
   int phys_drv_state[MAX_SUR];            /* used for states that can be
                                            * turned on for a drive, like immed.
                                            */

};


extern caddr_t kmem_alloc();

/* msg_printf stuff because the 800 msg_printf is so brain damaged */

#define AC_MSG_MAGIC    0x062259
#define AC_MSG_BSIZE    (4096 - 2 * sizeof (long))
struct  ac_msgbuf {
   long    msg_magic;
   long    msg_bufx;
   char    msg_bufc[AC_MSG_BSIZE];
};

struct  ac_msgbuf ac_msgbuf;

/*
 * Structure that holds dev's temporarily.  Doublely linked list.
 */
struct dev_store {
    struct dev_store *forw, *back;	/* doublely linked list 	*/
    dev_t   dev;			/* device to be stored  	*/
    short   B_CALL_set;			/* if B_CALL orig set 		*/
    int	    (*b_iodone)();		/* original b_iodone		*/
    struct buf *bp;                     /* bp associated with dev	*/
};

#endif /* _KERNEL */
#endif /* ! _SIO_AUTOCH_INCLUDED */


