/* @(#) $Revision: 1.9.84.13 $ */    
#ifndef _MSYS_SPACE_INCLUDED /* allows multiple inclusion */
#define _MSYS_SPACE_INCLUDED

#include "../ufs/fsdir.h"

#include "../h/user.h"
#include "../h/proc.h"
#include "../h/sem_beta.h"

#include "../h/vnode.h"
#include "../ufs/inode.h"

#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#ifdef SIXR
#include "../machine/sna_space.h" /*  for SNAP */
#endif

#include "../h/callout.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/pty.h"
#include "../h/nvs.h"
#include "../wsio/iobuf.h";
#include "../wsio/dilio.h";
#include "../dux/rmswap.h"
#ifdef DSKLESS
#include "../dux/dm.h"
#include "../dux/protocol.h"
#endif
#include "../dux/nsp.h"

#include "../s200io/lnatypes.h"
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#include "../s200io/drvhw.h"
#include "../h/devices.h"
#include "../h/dnlc.h"
#include "../h/file.h"

/*
 * System parameter formulae.
 */

struct	timezone tz = { TIMEZONE, DST };

#ifdef DSKLESS
short rootlink[3] = { 0xffff, 0xffff, 0xffff };
char *bootlink = 0;
int lanselectcode = -1;

int num_cnodes		 = NUM_CNODES;

/*
** Size the using/serving arrays. USING_ARRAY_SIZE and SERVING_ARRAY_SIZE
** are configurable parameters.
*/
int using_array_size = USING_ARRAY_SIZE;
struct using_entry using_array[ USING_ARRAY_SIZE ];

int serving_array_size = (SERVING_ARRAY_SIZE > MAX_SERVING_ARRAY) ? MAX_SERVING_ARRAY : SERVING_ARRAY_SIZE;
struct serving_entry serving_array[ (SERVING_ARRAY_SIZE > MAX_SERVING_ARRAY) ? MAX_SERVING_ARRAY : SERVING_ARRAY_SIZE];

int dskless_fsbufs = (DSKLESS_FSBUFS > MAX_SERVING_ARRAY) ? MAX_SERVING_ARRAY : DSKLESS_FSBUFS;

/*
** Define timeout periods for selftest and crash detection.  SELFTEST_PERIOD,
** SEND_ALIVE_PERIOD and CHECK_ALIVE_PERIOD are configurable parameters.
*/

/* If selftest period is 0 then no selftest, otherwise lowerbound of 90 secs */
int selftest_period = ((SELFTEST_PERIOD == 0) ? SELFTEST_PERIOD : ((SELFTEST_PERIOD < 90) ? 90 : SELFTEST_PERIOD));

int check_alive_period = CHECK_ALIVE_PERIOD;
int retry_alive_period = RETRY_ALIVE_PERIOD;

int ngcsp = NGCSP;
int ncsp = NGCSP + 1;			/* always one for limited CSP */
struct nsp nsp[NGCSP+1];		/* always one for limited CSP */
struct nsp *nspNCSP = &nsp[NGCSP+1];

#else
int dskless_fsbufs = 0;
#endif /* DSKLESS */

/* semaphore to prevent regular LAN init to reinitialize the network. */
/* USEFUL ??? */
int 	DUX_init = 1; 
/* dskless subsystem initialization flag */
int	dskless_initialized = 0;

#ifdef UIPC    /* UIPC is the umbrella subsystem for networking */

/*
 * Networking
 */

#include "../h/mbuf.h"
#define	PRUREQUESTS
#include "../h/protosw.h"
#include "../h/socket.h"

#ifdef INET
#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../net/if_ni.h"
/* ni */
int ni_max = NNI;
struct ni_cb ni_cb[NNI];

/*
 * Internet Domain
 */
#define	TCPSTATES
#include "../netinet/tcp_fsm.h"
struct ifqueue ipintrq;

/*
 * (X)NS Domain
 */
struct ifqueue nsintrq;

#endif /* INET */
#endif /* UIPC */

/*
 * Netisr
 */
int netisr_priority = NETISR_PRIORITY;
int     netmemmax       = NETMEMMAX;


#ifdef NSDIAG
#include        "../sio/nsdiag0.h"
#define NSDIAG_MAX_QUEUE	500
int nsdiag0_high_water = NSDIAG_MAX_QUEUE;
nsdiag_event_msg_type *nsdiag0_msg_queue;	/* msg queue */
#endif /* NSDIAG */

#ifdef LAN01
#include "../sio/lanc.h"
#include "../s200io/drvhw_ift.h"

#if ((NUM_LAN_CARDS > 0) && (MAX_LAN_CARDS > NUM_LAN_CARDS))
int num_lan_cards = NUM_LAN_CARDS;
#else
#if (NUM_LAN_CARDS > MAX_LAN_CARDS)
int num_lan_cards = MAX_LAN_CARDS;   /* exceed MAX_LAN_CARDS */
#else  /* We force it to defatul */
int num_lan_cards = 2;
#endif  /* NUM_LAN_CARDS > MAX_LAN_CARDS */
#endif

lan_ift * lan_dio_ift_ptr[10];
#endif /* LAN01 */


/*
 * Streams subsystem
 */

#ifdef HPSTREAMS

int strmsgsz = STRMSGSZ;
int strctlsz = STRCTLSZ;
int nstrevent = NSTREVENT;
int nstrpush  = NSTRPUSH;

#include "../streams/str_hpux.h"
#include "../streams/str_stream.h"

#endif /* HPSTREAMS */


#define	NETSLOP	20

#ifdef	NOSWAP
#define	NOSWAP	1
#else
#define	NOSWAP	0
#endif

#define	NCLIST  (100+16*MAXUSERS)
int	nclist = NCLIST;

int	nproc = NPROC;
int	ninode = NINODE;

/*
 * maxfiles is the system default soft limit for the maximum number of 
 * open files per process.  maxfiles defaults to 60 if not configured.  
 * maxfiles_lim is the system default hard limit for the maximum number of
 * open files per process.  maxfiles_lim defaults to 1024 if not configured.
 */

int     maxfiles = MAXFILES;
int     maxfiles_lim = MAXFILES_LIM;

/*The NCDNODE should be defined in master for configurability. Before we
can actually do it, this is what we can do now.*/
#define	NCDNODE	150
int	ncdnode = NCDNODE;
int	ncallout = NCALLOUT;
long	unlockable_mem = UNLOCKABLE_MEM;
int	nfile = NFILE + FILE_PAD;
int     file_pad = FILE_PAD;
int     dbc_min_pct = DBC_MIN_PCT;
int     dbc_max_pct = DBC_MAX_PCT;
int	dbc_cushion = DBC_CUSHION;
int	nbuf = NBUF;
int	bufpages = BUFPAGES;
int	nflocks = NFLOCKS;
int	npty = NPTY;
int	ndilbuffers = NDILBUFFERS;
int	ncsize = NINODE;
struct ncache ncache[NINODE];

/*
 *	Hash table of open devices.
 */
dtaddr_t devhash[DEVHSZ];

int	maxuprc = MAXUPRC;
int	maxdsiz = MAXDSIZ/NBPG; /* unit: page size */
int	maxssiz = MAXSSIZ/NBPG; /* unit: page size */
int	maxtsiz = MAXTSIZ/NBPG; /* unit: page size */
int	parity_option = PARITY_OPTION;
int	reboot_option = REBOOT_OPTION;
int	noswap	= NOSWAP;
int	install	= NOSWAP;
int	timeslice = TIMESLICE;		/* unit: 20ms tick */
int	acctsuspend = ACCTSUSPEND;	/* unit: percent of filesystem free */
int	acctresume = ACCTRESUME;	/* unit: percent of filesystem free */
int	dos_mem_byte = DOS_MEM_BYTE;	/* mem. reserved for dos in bytes   */

int	mem_no = 3; 	/* major device number of memory special file */
int 	ieee802_no  = 18;
int	ethernet_no = 19;
uint	dos_mem_start;			/* physical addr. of dos mem. */
int	scroll_lines = SCROLL_LINES;	/* number of lines of ITE buffer */

/*
   The tty stuff that needs to be declared somewhere.
*/
#define	NPCI	16
short	npci = NPCI;
struct tty *tty_line[NPCI];
struct tty *cons_tty;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted.
 */
struct	proc *proc, *procNPROC, *cur_proc;
struct	inode *inode, *inodeNINODE;
struct 	callout *callout;
struct	file *file, *fileNFILE, *file_reserve;
struct locklist locklist[NFLOCKS];	/* The lock table itself */
struct	tty		*pt_line[NPTY];
struct  nvsj            nvsj[NPTY];
struct	buf    dil_bufs[NDILBUFFERS];
struct	iobuf  dil_iobufs[NDILBUFFERS];
struct	dil_info  dil_info[NDILBUFFERS];
int (*fhs_timeout_proc)() = NULL;

/* declarations for stub routines for non-configurable portions of EISA bus support */
extern nop();
int (*eisa_init_routine)() = nop;
int (*eisa_nmi_routine)() = nop;
int (*eisa_eoi_routine)() = nop;

/* declarations for stub routines for non-configurable portions of MTV (VME) bus support */
int (*vme_init_routine)() = nop;

/*
**	The following supports savecore on the s300
*/

long	dumplo;		/* offset into dumpdev */
int	dumpsize;	/* amount of NBPG phys mem to save - dep on swap */
int	dumpmag;	/* magic number for savecore, 0x8fca0101 */
/* dumpdev is now generated into conf.c by config */

struct	cblock *cfree;
struct	buf *buf, *swbuf;
short	*swsize;
int	*swpf;
char	*buffers;

struct	bufqhead bfreelist[BQUEUES];	/* heads of available lists */
struct	buf	bswlist;		/* head of free swap header list */

char	runin;				/* scheduling flag */
char	runout;				/* scheduling flag */
int     runrun;				/* scheduling flag */
u_char	curpri;				/* more scheduling */

int	maxmem;				/* actual max memory per process */
int	physmem;			/* physical memory on this CPU */
int	hand;				/* current index into coremap used by daemon */
int	wantin;
int	selwait;
/*
 * The following is for the shared memory subsystem (if configured)
 */

#if MESG==1
#include	"../h/ipc.h"
#include	"../h/msg.h"

struct ipcmap	msgmap[MSGMAP];
struct msqid_ds	msgque[MSGMNI];
struct msg	msgh[MSGTQL];
struct msginfo	msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG
};
int	messages_present = 1;
#else
int	messages_present = 0;
#endif

#if SEMA==1
#	ifndef IPC_ALLOC
#	include	"../h/ipc.h"
#	endif
#include	"../h/sem.h"
struct semid_ds	sema[SEMMNI];
struct sem	sem[SEMMNS];
struct map	semmap[SEMMAP];
struct	sem_undo	*sem_undo[NPROC];
#define	SEMUSZ	(sizeof(struct sem_undo)+sizeof(struct undo)*SEMUME)
int	semu[((SEMUSZ*SEMMNU)+NBPW-1)/NBPW];
union {
	short		semvals[SEMMSL];
	struct semid_ds	ds;
	struct sembuf	semops[SEMOPM];
}	semtmp;

struct	seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM
};
int	semaphores_present = 1;
#else
int	semaphores_present = 0;
#endif

#if SHMEM == 1
#	ifndef	IPC_ALLOC
#	include	"../h/ipc.h"
#	endif
#include	"../h/shm.h"
struct	shmid_ds	shmem[SHMMNI];	
struct	shminfo shminfo = {
	SHMMAX,
	SHMMIN,
	SHMMNI,
	SHMSEG
};
int	shared_memory_present = 1;
#else
#	ifndef	IPC_ALLOC
#	include	"../h/ipc.h"
#	endif
#include	"../h/shm.h"
struct	shmid_ds shmem[1];	
int	shared_memory_present = 0;
#endif

/* The parser is currently not configurable, but when it is, modify the
 * assignment of (*pn_getcomponent)() = to your choice of parser.
 * right now its pn_getcomponent_n_computer() (8bit).
 */
/* two-byte characters in file names. */
/* extern int pn_getcomponent_chinese_t(); not supported yet */
extern int pn_getcomponent_n_computer();
#ifndef PARSER
#define PARSER pn_getcomponent_n_computer
#endif
int (*pn_getcomponent)() = PARSER;

#ifdef DSKLESS
struct pidchunk
{
	int start;
	int end;
} mypidchunks[NPROC];
#endif /* DSKLESS */

/* The following are configuration flags for networking */
int rel1nsc_1_flag = 1;
int rel1nsc_2_flag = 1;
int rel1nsc_3_flag = 1;

int     swapspc_cnt;    /* pages of available swap space */
int     swapmem_max;    /* total pages of system available swap space */
int     swapmem_cnt;    /* pages of available memory for "swap" */
int     maxfs_pri;      /* highest available device priority */
int     maxdev_pri;     /* highest available swap prioirity*/
int     sys_mem;	/* pages of memory not available for "swap" */

int minswapchunks = MINSWAPCHUNKS;

#ifdef X25
#if (defined(NUM_PDN0) && (NUM_PDN0 >= 0))
#ifndef IPPROTO_ICMP
#include "../netinet/in.h"
#endif /* NOT IPPROTO_ICMP */
#ifndef IFF_UP
#include "../net/if.h"
#endif /* IFF_UP not defined */
#include "../x25/x25gen.h"
#endif /* NUM_PDN0 */
#endif /* X25 */

/*
 * Double Stuff data structures/configuration; a -1 value means that the
 * parameter will be calculated from available memory at boot time.
 */
#define VHNDFRAC	-1
#define MAXPMEM		-1

#include "../h/sysinfo.h"
#include "../h/pfdat.h"
#include "../h/swap.h"

int desperate;
struct minfo minfo;
struct pfdat **phash;
struct pfdat *pfdat;
int phashmask;	/* Page hash mask */
struct pfdat phead;
long phread, phwrite;

int swchunk = SWCHUNK;

int nswapfs = NSWAPFS;
struct fswdevt fswdevt[NSWAPFS];

int nswapdev = NSWAPDEV;

#ifdef FSD_KI
struct swap_stats swap_stats[NSWAPDEV+NSWAPFS+1];
#endif

int swapmem_on = SWAPMEM_ON;
int sysmem_max = SYSMEMMAX;
int maxswapchunks = MAXSWAPCHUNKS;

struct devpri swdev_pri[NSWPRI];
struct fspri swfs_pri[NSWPRI];

struct swaptab swaptab[MAXSWAPCHUNKS];

vm_sema_t swap_lock;
int nextswap;
int swapwant;
int mpid;		/* For generating unique process IDs */

#include "../h/var.h"
struct var v = {
	NBUF,
	NCALLOUT,
	NINODE,
	(char *) 0, 		/*  (&inode[NINODE]),  */
	NFILE,
	(char *) 0, 		/*  (&file[NFILE]),  */
	0, 			/*  MNTHASHSZ,  */
	(char *) 0, 		/*  (&mounthash[MNTHASHSZ]),  */
	NPROC,
	(char *) 0,		/*  (&proc[1]),  */
#ifdef	LATER
	NCLIST,
	NSABUF,
	MAXUPRC,
	0,
	NHBUF,
	NHBUF-1,
	NPBUF,
	0,
	0,
#endif	
	VHNDFRAC,
#ifdef	LATER
	MAXPMEM,
	MAXDBUF
#endif	
};
int ticks_since_boot;

/*
 * Variables used for sar
 */
#include "../h/sar.h"

long sar_swapin;
long sar_swapout;
long sar_bswapin;
long sar_bswapout;
struct syswait syswait;

struct sysinfo sysinfo;
struct syserr syserr;

long readdisk;
long writedisk;
long phread;
long phwrite;
long lwrite;
long sysexec;
long sysnami;
long sysiget;
long runque;
long runocc;
long swpque;
long swpocc;
long dirblk;
long inodeovf;
long fileovf;
long textovf;
long procovf;
long sysread;
long syswrite;
long mux0incnt;
long mux0outcnt;
long mux2incnt;
long mux2outcnt;
long ptyincnt;
long ptyoutcnt;
long hptt0cancnt;
long hptt0outcnt;
long hptt0rawcnt;
long msgcnt;
long semacnt;

int procovf = 0;
int istackptr = 0;	/* True if running on istack */
int freemem_cnt = 0;


/* Set by graphics_make_entry(), used in main() to decide whether or */
/* not to start vdmad.                                               */

int vdma_present = 0;

/*
 * A bunch of stuff was allocated in proc.h.  I've moved it here.
 */
short	freeproc_list;		/* Header of free proc table slots */

struct	prochd qs[NQS];
int	whichqs[NQELS];		/* Bit mask summarizing non-empty qs's */

struct	map *sysmap;		/* Map of vaddr pool for system */


#ifdef NUM_DKITPPI0

#define	DKCHANS		128
#define	NUMREGSET	8
#define MAXDKIT 	4	/* max number of interfaces */
#define MAXDKCHANS	128  /* NUMBER OF CHANS PER DKITPPI0 */
#define DKCBSIZE	256	/* the size of dkcblock */
#define MAXDKCHANSHIFT	  7  /* LOG2 of MAXDKCHANS */

/* 
 * machine/space.h will include the following lines
 * should datakit become a standard product.
 *
 * #if DKIT
 * #include "../dkit/dkspace.h"
 * #endif
 *
 * but now the file is included in dkconf.c
 */

/*
 * The real dkit/dkspace.h
 */

#ifdef NUM_DKITPPI0
#define	MAXDKCB		(NUM_DKITPPI0 * MAXDKCHANS * 2)
#else
#define MAXDKCB		2
#endif

int	maxdkcb = MAXDKCB;
int	maxdkchans = MAXDKCHANS;
int	dkcbsize = DKCBSIZE;
int	delayclosetimer = DELAYCLOSETIMER;

#include "../h/types.h"
#include "../sys/dkit.h"
#include "../sys/dkdev.h"
#include "../sys/dkcmc.h"
#include "../sys/dkkmc.h"
#include "../sys/dktrace.h"
#include "config.h"

char			dkcblock[(MAXDKCB+1)*DKCBSIZE];
caddr_t			dkcblockstack[MAXDKCB];

int	trace_wanted = 0; 	/* a flag to indicate /dev/ppitrace is opened */

int      ppi_created = 0;/* number of PPI cards recognized and 
				   configured by CONFIGURATOR.		*/
struct buf  	*remove_aev();	 

/* hex digits used by printhex() to convert an integer number into hex */
char hex[16] = {'0','1','2','3','4','5','6','7','8','9',
			'a','b','c','d','e','f'};

/* for debugging purpose */
int	numread = 0; 
int	numaev; 

int	dktrap;
int       chanmask = 0;
/* struct    ppi0_msg	msgq[NUMREGSET]; ccc 032889  for 7.0 */
int	dkdebug = 512;		/* not used - here for dk.c extern */

/*  int	dkactive[MAXDKIT] = {0,0,0,0};   */
int	dkactive[MAXDKIT] = {-1,-1,-1,-1};
int	vcsisup[MAXDKIT] = { 0, 0, 0, 0};


int	dkpanic = 0;		/* # of dk_close(0)'s in this run */

short dup_count = 0; /* counter for number of duplicate sends */

#ifdef NUM_DKITPPI0
int			dk_nchan =  NUM_DKITPPI0 * MAXDKCHANS;
int			dk_ccnt =   NUM_DKITPPI0;
char			cmcinpbuf [(NUM_DKITPPI0+1)*DKCBSIZE];
char			*cmcinp    [NUM_DKITPPI0];
short			dkcactive  [NUM_DKITPPI0];
short			dkc1omplete[NUM_DKITPPI0] ;
short			dkc2omplete[NUM_DKITPPI0] ;
short			dkcstall[ NUM_DKITPPI0] ;
int			dk_achan[ NUM_DKITPPI0];
int			dkserverid[ NUM_DKITPPI0 ];
struct dkdev		dkdev   [ NUM_DKITPPI0 * MAXDKCHANS ];
struct tty		dkt_ttys[ NUM_DKITPPI0 * MAXDKCHANS ];
struct dksetupreq	dkreq   [ NUM_DKITPPI0 * MAXDKCHANS ];
struct    dkchan	dkit    [ NUM_DKITPPI0 * MAXDKCHANS ];
struct dktracebuf	dktracebuf[ NUM_DKITPPI0 * MAXDKCHANS ];
#else 
int			dk_nchan = 0;
int			dk_ccnt = 0;
char			cmcinpbuf[2*DKCBSIZE];
char			*cmcinp[1];
short			dkcactive[1];
short			dkc1omplete[1] ;
short			dkc2omplete[1] ;
short			dkcstall[1] ;
int			dk_achan[1];
int			dkserverid[1];
struct dkdev		dkdev[1];
struct dksetupreq	dkreq[1];
struct dkchan		dkit[1];
struct dktracebuf	dktracebuf[1];
#endif /* NUM_DKITPPI0 */

#ifdef DKRAW
#else
#define dkr_open	nodev
#define dkr_close	nodev
#define dkr_read	nodev
#define dkr_write	nodev
#define dkr_ioctl	nodev
#endif /* DKRAW */

#ifdef DKTTY
#	ifdef NUM_DKITPPI0
/* struct tty	dkt_ttys[ NUM_DKITPPI0 * MAXDKCHANS ];  */
#	else
struct tty	dkt_ttys[ 1 ];
#	endif /* NUM_DKITPPI0 */
#else
#define dkt_open	nodev
#define dkt_close	nodev
#define dkt_read	nodev
#define dkt_write	nodev
#define dkt_ioctl	nodev
#define dkt_select	nodev
#define dkt_option1	nodev
#define dkt_control	nodev
#endif /* DKTTY */

#ifdef DKXQT
#else
#define dkx_open	nodev
#define dkx_close	nodev
#define dkx_read	nodev
#define dkx_write	nodev
#define dkx_ioctl	nodev
#define dkx_select	nodev
#endif /* DKXQT */

/* We have modified llconf.o in order to have the kernel recognize
 * the ppi card.  However, DATAKIT code does not reference anything
 * from llconf.o.  The uxgen process, using 'ld ... libdkit.a libhp-ux.a...',
 * grabs llconf.o from libhp-ux.a since that archive is open when
 * it is first referenced.
 * The following unused pointer to a function simply causes the linker to
 * resolve *something* from llconf.o while scanning libdkit.a.  If
 * this is not done, llconf.o will be taken from libhp-ux.a during
 * customer gen's.
 */

extern int	ioconf();
int	(*ldfudge)() = ioconf;

/* ppi space declaration */

#ifdef NUM_DKITPPI0
#include "../dkit/nioppi.h"
#include "../dkit/ppi0.h"
#include "../dkit/ppi0_spc.h"

 struct ppi0_pda      *ppi_pda[NUM_DKITPPI0];
 int                  num_ppi0 = NUM_DKITPPI0;
 int                  dk_cnt   = 7;       /* 128 channel per interface  */
 struct ppi0_step     ppi_steps[NUM_DKITPPI0][NUMSTEPS+1];


/* special debug/printf flag */
int     dkitdebug;
int	cfreemap[((100 + 16 * MAXUSERS) >> 5) + 1];

#endif /* NUM_DKITPPI0 */
#endif

/* File system async flag. If set file system data structures
   are written asychronously.                 */

int fs_async = FS_ASYNC;

/*
 * flag to control creation of "fast" symbolic links.
 */
int create_fastlinks = CREATE_FASTLINKS;

/*
 * flag to turn off new AES conformance behavior for hp-ux system calls.
 */
int hpux_aes_override = AES_OVERRIDE;

/* hash table size scale with number of items hashed */

/* lpow2 returns largest power of 2 less than arg, min value 16, max 8192 */
#define lpow2(arg) \
	(arg) < 32? 16: \
	(arg) < 64? 32: \
	(arg) < 128? 64: \
	(arg) < 256? 128: \
	(arg) < 512? 256: \
	(arg) < 1024? 512: \
	(arg) < 2048? 1024: \
	(arg) < 4096? 2048: \
	(arg) < 8192? 4096: \
	8192

#ifdef SCALE_PERF_TEST
#define hashsize(length, items, default) default
#else
#define hashsize(length, items, default) \
	(lpow2((items)/(length)))
#endif

/* proc table */

#define	PIDHSZ hashsize(4, NPROC, 64)
int	PIDHMASK = PIDHSZ - 1;
short pidhash[PIDHSZ];

#define	PGRPHSZ hashsize(4, NPROC, 64)
int	PGRPHMASK = PGRPHSZ - 1;
short pgrphash[PGRPHSZ];

#define	UIDHSZ hashsize(4, NPROC, 64)
int	UIDHMASK = UIDHSZ - 1;
short uidhash[UIDHSZ];

#define SIDHSZ hashsize(4, NPROC, 64)
int	SIDHMASK = SIDHSZ - 1;
short sidhash[SIDHSZ];


/* sleep table */

#define	SQSIZEDEF hashsize(4, NPROC, 64)
int	SQSIZE	= SQSIZEDEF;
int	SQMASK	= SQSIZEDEF-1;

#ifdef MP
sema_t slpsem[SQSIZEDEF];
#else
struct proc *slpque[SQSIZEDEF];
struct proc *slptl[SQSIZEDEF];	/* For FIFO sleep queues */
#endif /* MP */

/* buffer table */

/* average buf hash chain length desired -- see machdep.c */
int bufhash_chain_length = 4;
struct  bufhd *bufhash; /* buffer hash table */
int	BUFHSZ, BUFMASK;/* size and mask for accessing bufhash */

/* inode table */

#define	INOHSZDEF hashsize(6, NINODE, 64)
int	INOHSZ	= INOHSZDEF;
int	INOMASK	= INOHSZDEF-1;
union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZDEF];


/* spinlocks */

#define SPINSIZEDEF (B_SEMA_HTBL_SIZE + SQSIZEDEF + 50)
int MAX_SPINLOCKS = SPINSIZEDEF;

lock_t	spin_alloc_base[SPINSIZEDEF] = { 0 };
lock_t	*spin_alloc_end = spin_alloc_base + SPINSIZEDEF;

int ddb_boot = DDBBOOT;


#endif /* _MSYS_SPACE_INCLUDED */
