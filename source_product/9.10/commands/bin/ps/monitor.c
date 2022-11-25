/************************************************************
 * Performance monitor for 4.2 BSD originated HP-UX kernels *
 ***********************************************************************
 * This is a shared source program!                                    *
 * Changes made should be coordinated with: 			       *
 *  - Jack Applin for the series 300 at neutron@fc in Fort Collins     *
 *  - Ali Shahmirza for the series 800 at alis@cup.hp.com in Cupertino *
 **********************************************************************/

#include <sys/stdsyms.h>
#if  defined(__hp9000s800) && !defined(_WSIO)
#define MP
#endif /* ! _WSIO */

#include "monitor.h"

struct	dux_mbstat dux_mbstat;

extern int errno;
extern int optind;	/* getopt() global */
extern char *optarg;	/* getopt() global */
extern char *strcpy(), *strrchr(), *strdup();

#include "kernelvars.h"
#ifdef REGION_TO_PATH
#include <sys/inode.h>
#endif

/************************************************************* Global */

#define NICEVAL -10
#define ROWS 24
#define COLUMNS 80
#define MAX_ESCAPE 128

extern char help_target;

char monitorfile[] = "/etc/monitor_data";	/* NEW FILE FOR FAST MONITOR */
						/* /etc/mon_data WAS THE OLD */
						/* VERSION THAT DIDN'T HAVE  */
						/* WHATSTRING TO IDENTIFY IT */

int current_x, current_y;

char		*getenv();

char		*myname;
int		time_now;
struct timeval	start_time;
struct utsname	name;
char		cwd[1024];
char		cl[MAX_ESCAPE], ho[MAX_ESCAPE], fmt[MAX_ESCAPE];
char		*stuff_ptr;
int		stuff_len;
int		ho_len, cl_len, ho_or_cm;
int		rows, columns, automargin, eolwrap;

union	{
		char	*ch_scrn;
		int	*int_scrn;
	} screen1, screen2;
#define next_screen	screen2.ch_scrn
#define current_screen  screen1.ch_scrn
#define next_scrni	screen2.int_scrn
#define current_scrni   screen1.int_scrn

int		cursor_row;

char		*buffer;
int		buffer_size;

struct termio	tty_state, tty_state_old;
int		old_flags;
int		old_valid = 0;
char		veof, vintr;

int		reverse = 1;

int		fdout, fdin;

int		osdesignermode = 0;

int kmem_fd;
int mem_fd;

struct sigvec interval_vec;
struct sigvec interactive_vec;
struct sigvec oldint_vec;
struct sigvec oldquit_vec;
struct sigvec ignore_vec;

int		not_done = 1;
int		no_error;
int		iteration = 1;
struct timeval	current_time, previous_time, timeout;
char		cmd, last_cmd = 'x';
int		in_screen = 0;
int		max_screen = 1;
int		readfds, savefds;
int		update_interval = 1;
int		iterationcnt = 1000000000;
char		hardcopy[80] = "";
char		hpux_file[256] = "/hp-ux";
char 		whatstring[256] = "@(#) $Revision: 70.5 $";

struct pst_static pst_static_info;
struct pst_dynamic pst_dynamic_info;
#ifdef __hp9000s800
int runningprocs;
#endif  /* __hp9000s800 */

/***************************************************** Mode G variables */

#define SWAP_MY_SITE 0
#define SWAP_NOT_MY_SITE 1
#define SW_DEV_CONF 2
#define SW_DEV_ENAB 3
#define SW_DEV_FREE 4
#define FSW_AVAIL 5
#define FSW_FREE 6
#define SWAP_MAX 7

long	swap_conf;
int	physmem;
int	maxmem;

int	old_v_swtch;
int	old_v_trap;
int	old_v_syscall;
int	old_v_intr;

struct timeval boottime;

#ifdef hp9000s200
struct	msus msus;
char	msus_string[16];
char	sysname[11];
#endif  /* hp9000s200 */

u_int	my_site_status;
site_t	my_site;
struct swaptab *swaptable, *swapMAXSWAPTAB;
int nswapfs, nswapdev;
int swchunk, maxswapchunks, swapspc_max, swapspc_cnt;
int lockable_mem;
struct swdevt *swp, swpdev;
struct fswdevt *fswp, fswpdev;

/*************************************************** Mode I variables */

int bufpages;

int mount_devices_gotten = 0;

int nmnt=0;
struct mntl {
	char	*name;
	dev_t	dev;
};

struct mntl **mntl; /* NMOUNT can be unlimited, dynamically allocate */

time_t	mnttab_time=0;

int	*swap_dev_size;

#ifdef __hp9000s800
int drives_conf;

struct nlist nl_800[] = {
	{"dk_ndrive"},
	{ NULL }
};
#define X_DK_NDRIVE 0
#define DK_NDRIVE 8
	
#endif  /* __hp9000s800 */

struct
{
#ifdef hp9000s200
	long	dk_time[DK_NDRIVE];
	long	dk_wds[DK_NDRIVE];
	long	dk_seek[DK_NDRIVE];
	long	dk_xfer[DK_NDRIVE];
	float	dk_mspw[DK_NDRIVE];
	dev_t	dk_devt[DK_NDRIVE];
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	int	dk_ndrive;
	int	dk_ndrive1;
	int	dk_ndrive2;
	int	dk_ndrive3;
	long	*dk_time;
	long	*dk_wds;
	long	*dk_seek;
	long	*dk_xfer;
	float	*dk_mspw;
#endif  /* __hp9000s800 */
} s, s1;

#define XDIFF(fld) t = s.fld[i]; s.fld[i] -= s1.fld[i]; s1.fld[i] = t;

int old_sync_reads;
int old_sync_writes;

/* Next four defines are from dmmsgtype.h */

#define DMNETSTRAT_READ	17	/* network logical block read */
#define DMNETSTRAT_WRITE 18	/* network logical block write */
#define DMSYNCSTRAT_READ 19	/* synchronous read */
#define DMSYNCSTRAT_WRITE 20	/* synchronous write */

#ifdef __hp9000s800
#define X_DK_DEVT 0

struct nlist nl_snio[] = {
	{"dk_devt"},
	{ NULL }
};

#endif  /* __hp9000s800 */


/***************************************************** Mode K variables */

#define MAXSITE 256

int	ncsp;
struct nsp *nsp;

#define CLUSTERCONF "/etc/clusterconf"

int	cct_ok = 0;
int	cct_count = 0;

struct monitor_cct_entry {
	cnode_t cnode_id;
	char cnode_name[15];
	cnode_t swap_serving_cnode;
	int kcsp;
	int csps_active;
} cct[MAXCNODE];			

cnode_t	rootid = -1;
time_t	cct_time = 0;

int	cct_compare();

/***************************************************** Mode L variables */

#ifdef hp9000s200
char lan_name[80] = "/dev/ieee";
#endif  /* hp9000s200 */

#ifdef __hp9000s800
char lan_name[80] = "/dev/lan0";
#endif  /* __hp9000s800 */

unsigned char lanic_address[6];
int lan_fd;
int output_last, input_last;

struct receive {
	int reqtype;
	char *description;
} receive_table[] = { RX_FRAME_COUNT,"Input packets w/o error",
		      MULTICAST_ADDRESSES,"Multicasts accepted",
		      UNDEL_RX_FRAMES,"Undeliverable",
		      RX_BAD_CRC_FRAMES,"Bad CRC",
		      ALIGNMENT_ERRORS,"Frame alignment errors",
		      MISSED_FRAMES,"Lost due to lack of resources",
		      BAD_CONTROL_FIELD,"Bad control field",
		      UNKNOWN_PROTOCOL,"Unknown protocol",
		      0,"" };

struct transmit {
	int reqtype;
	char *description;
} transmit_table[] = { TX_FRAME_COUNT,"Output packets w/o error",
		       UNTRANS_FRAMES,"Not transmitted due to error",
		       DEFERRED,"Deferred due to other traffic",
		       COLLISIONS,"Collisions",
		       ONE_COLLISION,"Exactly 1 collision",
		       MORE_COLLISIONS,"Between 2 and 15 collisions",
		       EXCESS_RETRIES,"Greater than 15 collisions",
		       LATE_COLLISIONS,"Late collision",
		       CARRIER_LOST,"Carrier lost when transmitting",
		       NO_HEARTBEAT,"No heartbeat after transmission",
		       0,"" };

/***************************************************** Mode M variables */

int oldoutcount, oldincount;

#define STATS_RESET_PROTOCOL_INFO	0
#define STATS_READ_PROTOCOL_INFO 	1
#define STATS_READ_MBUF_INFO		2
#define STATS_READ_CBUF_INFO		3
#define STATS_READ_INBOUND_OPCODES	4
#define STATS_RESET_INBOUND_OPCODES	5
#define STATS_READ_OUTBOUND_OPCODES	7
#define STATS_RESET_OUTBOUND_OPCODES	8
#define STATS_READ_IN_OUT_OPCODES	9
#define STATS_RESET_IN_OUT_OPCODES	10

/*
** Op-code statistics.
*/
struct proto_opcode_stats {
	unsigned int opcode_stats[100];
};
struct  proto_opcode_stats inbound_opcode_stats;
struct  proto_opcode_stats outbound_opcode_stats;

/* This following array needs to match up with dmmsgtype.h */

/* #define NOPCODES 80   Number of entries in opcode_descrp[] */

char *opcode_descrp[] = {
"INVALID                 0 Invalid OP-Code",
"DMSIGNAL                1 Signal a GCSP",
"DM_CLUSTER              2 Clustering operation",
"DM_ADD_MEMBER           3 Adding new site to cluster",
"DM_READCONF             4 Read clusterconf file",
"DM_CLEANUP              5 Invoke recovery process",
"DMNDR_READ              6 NOT Implemented",
"DMNDR_WRITE             7 NOT Implemented",
"DMNDR_OPEND             8 NOT Implemented",
"DMNDR_CLOSE             9 NOT Implemented",
"DMNDR_IOCTL            10 NOT Implemented",
"DMNDR_SELECT           11 NOT Implemented",
"DMNDR_STRAT            12 Block device strategy call",
"DMNDR_BIGREAD          13 NOT Implemented",
"DMNDR_BIGWRITE         14 NOT Implemented",
"DMNDR_BIGIOFAIL        15 NOT Implemented",
"DM_LOOKUP              16 Lookup a pathname",
"DMNETSTRAT_READ        17 Network logical block read",
"DMNETSTRAT_WRITE       18 Network logical block write",
"DMSYNCSTRAT_READ       19 Synchronous read",
"DMSYNCSTRAT_WRITE      20 Synchronous write",
"DM_CLOSE               21 Close file",
"DM_GETATTR             22 Get file attributes",
"DM_SETATTR             23 Set file attributes",
"DM_IUPDAT              24 NOT Implemented",
"DM_SYNC                25 Switch to synchronous mode",
"DM_REF_UPDATE          26 Update an inode reference count",
"DM_OPENPW              27 Wait for pipe open",
"DM_FIFO_FLUSH          28 Flush FIFO info to server",
"DM_PIPE                29 Open a remote pipe",
"DM_GETMOUNT            30 Request the mount table",
"DM_INITIAL_MOUNT_ENTRY 31 Mount entry during bootup",
"DM_MOUNT_ENTRY         32 Update mount table with mount entry",
"DM_UFS_MOUNT           33 Mount a UFS file system",
"DM_COMMIT_MOUNT        34 Mount committed",
"DM_ABORT_MOUNT         35 Mount aborted",
"DM_UMOUNT_DEV          36 Unmount given device number",
"DM_UMOUNT              37 Participate in global unmount",
"DM_SYNCDISC            38 Synchronize disc",
"DM_FAILURE             39 Declare the site as a failure",
"DM_SERSETTIME          40 Set time to the reference site time",
"DM_SERSYNCREQ          41 Time stamp the time packet",
"DM_RECSYNCREP          42 Process the reply time packet",
"DM_GETPIDS             43 Get new process IDS",
"DM_RELEASEPIDS         44 Release process IDS",
"DM_LSYNC               45 Sync Local Buffers",
"DM_FSYNC               46 Fsync a file",
"DM_CHUNKALLOC          47 Allocate chunk from the common pool",
"DM_CHUNKFREE           48 Return a chunk to the common pool",
"DM_TEXT_CHANGE         49 Change ref count on a text segment",
"DM_XUMOUNT             50 Cluster-wide xumount",
"DM_XRELE               51 Multisite xrele",
"DM_USTAT               52 Ustat",
"DM_RMTCMD              53 NOT Implemented",
"DM_ALIVE               54 Alive Packet for Crash Detection",
"DM_DMMAX               55 Return the value of dmmax",
"DM_SYMLINK             56 Symbolic Link",
"DM_RENAME              57 Rename system call",
"DM_FSTATFS             58 fstatfs",
"DM_LOCKF               59 lockf request",
"DM_PROCLOCKF           60 Check proc for lockf status",
"DM_LOCKWAIT            61 Wait for a lock to clear",
"DM_INOUPDATE           62 Update certain fields in an inode",
"DM_UNLOCKF             63 unlockf request",
"DM_NFS_UMOUNT          64 NFS unmount",
"DM_COMMIT_NFS_UMOUNT   65 NFS unmount commit",
"DM_ABORT_NFS_UMOUNT    66 NFS unmount abort",
"DM_MARK_FAILED         67 Declare a cnode failed",
"DM_LOCK_MOUNT          68 Obtain cluster-wide mount lock",
"DM_UNLOCK_MOUNT        69 Release mount lock",
"DM_SETACL              70 Set Access Control List",
"DM_GETACL              71 Get Access Control List",
"DM_FPATHCONF           72 Get configurable infor on file",
"DM_SETEVENT            73 Broadcast auditing events",
"DM_AUDCTL              74 Set name of audit file",
"DM_AUDOFF              75 Turning auditing off",
"DM_GETAUDSTUFF         76 Get auditing information",
"DM_SWAUDFILE           77 Switch audit file",
"DM_CL_SWAUDFILE        78 Broadcast switch audit file",
"DM_FSCTL               79 CDFS file system control",
"DM_QUOTACTL            80 Quotactl",
"DM_QUOTAONOFF          81 Quota on/off info for mount table"
/*"DM_LOCKED              82 Check if region of file is locked"*/
};
/* Number of entries in opcode_descrp[] */
#define NOPCODES (sizeof(opcode_descrp)/sizeof(opcode_descrp[0])) 

#define NREVISION "1.0"

int flags_set = 0;
unsigned int sigflags = 0;
#define SF_RTIME	1
#define SF_TIME		2
#define SF_USER		4
#define SF_SYSTEM	8
#define SF_DISC		16
#define SF_MEMORY	32
#define SF_SWAP		64
#define SF_LAN		128
#define SF_DROPPED	256
#define SF_MSG		512
#define SF_RETRIES	1024
#define SF_CSP		2048
#define SF_BLOCK	4096
#define SF_SYSCALLS	8192
#define SF_PAGE		16384

struct sf_info {
	int sf_flag;
	char *sf_name;
} sf_info[] = { SF_RTIME,"rtime",
		SF_TIME,"time",
		SF_USER,"user",
		SF_SYSTEM,"system",
		SF_DISC,"disc",
		SF_MEMORY,"memory",
		SF_SWAP,"swap",
		SF_LAN,"lan",
		SF_DROPPED,"dropped",
		SF_MSG,"msg",
		SF_RETRIES,"retries",
		SF_CSP,"csp",
		SF_BLOCK,"block",
		SF_SYSCALLS,"syscalls",
		SF_PAGE,"page",
		0,"" };

#define MAXSYSTEMS 25
#define SYSTEMLEN 16
#define TIMELEN 10

struct systems {
	char name[SYSTEMLEN];
	int fd;
	int pid;
	int olditeration;
	int oldrtime;
	int olduser;
	int oldsystem;
	int olddisc;
	int oldmemory;
	int oldswap;
	int oldlan;
	int lanrate;
	int olddropped;
	int droppedrate;
	int oldmsg;
	int msgrate;
	int oldretries;
	int retriesrate;
	int oldcsp;
	int csprate;
	int oldblock;
	int blockrate;
	int oldsyscalls;
	int syscallsrate;
	int oldpage;
	int pagerate;
} systems[MAXSYSTEMS];

int syscount = 0;

int startup = 0;

/***************************************************** Mode R variables */

#define HOSTNAMELEN 32
#define	NHOSTS	128

int	nhosts;

struct	hs {
	char	hs_hostname[HOSTNAMELEN];
	int	hs_sendtime;
	int	hs_recvtime;
	int	hs_boottime;
	int	hs_loadav[3];
	int	hs_nusers;
	int	hs_idleusers;
} hs[NHOSTS];

struct	whod awhod;
#define	WHDRSIZE	(sizeof (awhod) - sizeof (awhod.wd_we))
#define	RWHODIR		"/usr/spool/rwho"

char	*interval();

char	resbuf[32];

int	hcmp(), ucmp(), lcmp(), tcmp();
int	(*cmp)() = hcmp;
char	cmp_type = 'H';

#define down(h,now) (now - (h)->hs_recvtime > (11 * 60))

/***************************************************** Mode S variables */


int	Spid;
struct proc proc_buf;
struct pst_status pstat_buf;
char command_buf[PST_CLEN+1];
long	proc_ticks;
struct proc *proc;
vas_t vas;

#ifdef hp9000s200
int	addrspace;		/* Max. addr space for the given processor */
#endif  /* hp9000s200 */
#ifdef __hp9000s800
int	koffset;
#endif  /* __hp9000s800 */

/***************************************************** Mode T variables */

struct timestruct { char  	 seen;
		    short	 pid;
		    unsigned int sticks;
		    unsigned int uticks;
} *timestruct;

long *pidlist;

int timecount = 0;

char	*gettty();

char	t_filter = 'A';
char	t_user[20];

#define NDEV  32767   /* MAXDEVS - sys/devices.h, max num of oopen dev on sys */
int	ndev=0;
struct devl {
	char	dname[DIRSIZ];
	dev_t	dev;
} devl[NDEV];

#define MAX_CMD_LEN 15

struct	proc2 {
	int	d_pid;
	int	d_cmdl;
	char	d_cmd[MAX_CMD_LEN];
#ifdef hp9000s200
	char	d_tty[2];
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	char	d_tty[4];
#endif  /* __hp9000s800 */
};

struct	proc	*proc_table;
struct	proc2	*proc_data;

struct	proc2 *mproc2;

/***  commented out for 8.0
union {
	struct	user user;
	char	upages[NBPG];
} user;

#define u	user.user
***/

struct	forkstat forkstat, old_forkstat;
long	*cp_time, old_cp_time[CPUSTATES];
#ifdef MP 
long	old_mcp_time[MAX_PROCS][CPUSTATES];
#endif /* MP */
int	nproc;
double	avenrun[3];

char *username();
char get_user_buff[20];

#define HASHTABLESIZE 853
struct hash_el {
    int  u_id;
    char name[8];
};
struct hash_el hash_table[HASHTABLESIZE];
int uid_count = 0;
#define HASHIT(i) ((i) % HASHTABLESIZE)
#define H_empty -1

#ifdef hp9000s200
typedef	unsigned short pt_entry_t;
#endif  /* hp9000s200 */

#ifdef __hp9000s800
dev_t	cons_mux_dev;
#endif  /* __hp9000s800 */

/*************************************************** Mode V variables */

struct	vmtotal	total;
struct	vmmeter	cnt, old_cnt;

/* int	desscan; */  /* It has been removed from kernel, performance tuning */
/* int	maxpgio; */  /* It has been removed from kernel, performance tuning */
int	maxslp;
int	lotsfree;
int	minfree;
int	desfree;

/**********************************************************************/

usage()

{
    fprintf(stderr,
"Usage: [-c cmd] [-d dur] [-i int] [-l lan] [-n kern] [-r rev] [-s lp] [stats]\n");
    fprintf(stderr,
    "  cmd   - Initial command character (default: ?)\n");
    fprintf(stderr,
    "  dur   - Duration of time in iterations (stdout mode only)\n");
    fprintf(stderr,
    "  int   - Update interval in seconds (default: 1)\n");
    fprintf(stderr,
    "  kern  - Name of kernel file (default: /hp-ux)\n");
    fprintf(stderr,
    "  lan   - Device for LAN statistics (default: /dev/ieee)\n");
    fprintf(stderr,
    "  lp    - Device for lp spooling (default: NONE)\n");
    fprintf(stderr,
    "  rev   - Don't start if N screen protocol revision ID doesn't match\n");
    fprintf(stderr,
    "  stats - Names of kernel statistics to write out:\n");
    fprintf(stderr,
"    +-------------------------------------------------------------------------+\n");
    fprintf(stderr,
"    | rtime    - Millisec since startup  | time     - Time in HH:MM:SS format |\n");
    fprintf(stderr,
"    | user     - User CPU utilization    | system   - System CPU utilization  |\n");
    fprintf(stderr,
"    | disc     - Active disc utilization | memory   - User memory utilization |\n");
    fprintf(stderr,
"    | swap     - Swap disc utilization   | lan      - LAN in/out packet count |\n");
    fprintf(stderr,
"    | dropped  - Discless msgs dropped   | msg      - Discless message count  |\n");
    fprintf(stderr,
"    | retries  - Discless msgs retries   | csp      - Discless CSP requests   |\n");
    fprintf(stderr,
"    | block    - Blk I/O reqs-cache hits | syscalls - System calls            |\n");
    fprintf(stderr,
"    | page     - VM page read/write reqs |                                    |\n");
    fprintf(stderr,
"    +-------------------------------------------------------------------------+\n");
    fprintf(stderr,
"    Default: Display data in full screen interactive mode\n");

} /* usage */



/**********************************************************************/


#define get(loc, buf, nbytes) get_kmem((off_t) (loc), (char *) (buf), (nbytes))
#define getvar(loc, var) get(loc, &(var), sizeof(var))


/* READ VALUE FROM /dev/kmem */

getvalue(loc)

	unsigned long loc;
{
	long word = 0;

	getvar(loc, word);
	return (word);

} /* getvalue */

get_kmem(loc, buf, nbytes)
off_t loc;
char *buf;
int nbytes;
{
	int ret;

/* #ifdef hp9000s200 */
#ifndef KI_KMEM_GET
#define KI_KMEM_GET 19
#endif  /* not KI_KMEM_GET */
/***	ret = ki_call(KI_KMEM_GET, buf, nbytes, loc);
	if (ret == -1) {
		perror("ki_call(KI_KMEM_GET)");
		abnorm_exit(1);
	} ***/
/* #else */
	if (lseek(kmem_fd, loc, 0L) == -1) {
		perror("lseek");
		fprintf(stderr, "get: can't seek to %#x\n", loc);
		abnorm_exit(1);
	}

	if ((ret = read(kmem_fd, buf, nbytes)) != nbytes) {
		perror("read");
		fprintf(stderr, "get: read %d bytes from %#x, got %d\n",
			nbytes, loc, ret);
		abnorm_exit(1);
	}
/* #endif  hp9000s200 */
	return ret;
}


#ifdef hp9000s200
#pragma OPT_LEVEL 1
asm("		global	_ki_call");
asm("		global	__cerror");
asm("_ki_call:			");
asm("		movq	&45,%d0	");
asm("		trap	&0	");
asm("		bcc.b	noerror	");
asm("		jmp	__cerror");
asm("noerror:			");
asm("		rts		");
#pragma OPT_LEVEL 2
#endif /* hp9000s200 */


/**********************************************************************/

/* READ OUT VARIOUS KERNEL VARIABLE VALUES */

getkvars()

{
	/* CHECK X_NINODE BECAUSE CONFIG VARIABLES NOT CHECKED FOR PRESENCE */

	if (nl[X_NINODE].n_value == 0)
	{
		fprintf(stderr,"nlist variable %s not found in %s\n",
			nl[X_NINODE].n_name,hpux_file);
		abnorm_exit(-1);
	}
	nproc = pst_static_info.max_proc; 
	proc = (struct proc *) getvalue(nl[X_PROC].n_value);
/*	desscan = getvalue(nl[X_DESSCAN].n_value);  */
/*	maxpgio = getvalue(nl[X_MAXPGIO].n_value);  */
	maxslp = getvalue(nl[X_MAXSLP].n_value);
	lotsfree = getvalue(nl[X_LOTSFREE].n_value);
	minfree = getvalue(nl[X_MINFREE].n_value);
	desfree = getvalue(nl[X_DESFREE].n_value);
 	swaptable = (struct swaptab *) nl[X_SWAPTAB].n_value;
  	swapMAXSWAPTAB=(struct swaptab *)getvalue(nl[X_SWAPMAXSWAPTAB].n_value);
	swchunk = getvalue(nl[X_SWCHUNK].n_value);
        maxswapchunks = getvalue(nl[X_MAXSWAPCHUNKS].n_value);

#ifdef __hp9000s800
#ifdef MP 
	runningprocs = getvalue(nl[X_RUNNINGPROCS].n_value);
#endif /* MP */
	cons_mux_dev = getvalue(nl[X_CONS_MUX_DEV].n_value);
#endif  /* __hp9000s800 */

} /* getkvars */

/**********************************************************************/

/* SPECIAL READ UNLINKS monitorfile ON READ ERROR */

monread(fd, bp, bs)

	int fd;
	char *bp;
	unsigned bs;
{
	if (read(fd, bp, bs) != bs) {
		fprintf(stderr, "monitor: error on read: %s\n",monitorfile);
		perror("read");
		unlink(monitorfile);
		return(1);
	}
	else
		return(0);

} /* monread */

/**********************************************************************/

/* SPECIAL WRITE UNLINKS monitorfile ON WRITE ERROR */

monwrite(fd, bp, bs)

	int fd;
	char *bp;
	unsigned bs;
{
	if (write(fd, bp, bs) != bs) {
		fprintf(stderr, "monitor: error on write: %s\n",monitorfile);
		perror("write");
		unlink(monitorfile);
	}

} /* monwrite */

/**********************************************************************/

/* CHECK TO SEE IF WE CAN READ DATA FROM A PRE-RECORDED FILE RATHER THAN
   OPENING /hp-ux AND SCANNING THE /dev DIRECTORY */

int readata()

{
	struct stat sbuf1, sbuf2, sbuf3, sbuf4;
	char whatbuff[128];
	int result = 0;
	int fd;

	/* RE-READ /dev & /hp-ux DEPENDING ON VARIOUS FILE LAST CHANGE DATES */
	if ((stat(monitorfile, &sbuf1) < 0) ||
	    (stat(hpux_file, &sbuf2) < 0) ||
	    (stat("/dev", &sbuf3) < 0) ||
	    (stat("/dev/pty", &sbuf4) < 0))
		return(1);  /* Most likely, the monitorfile is missing */
	if ((sbuf1.st_mtime <= sbuf2.st_mtime) ||
	    (sbuf1.st_mtime <= sbuf2.st_ctime) ||
	    (sbuf1.st_mtime <= sbuf3.st_mtime) ||
	    (sbuf1.st_mtime <= sbuf3.st_ctime) ||
	    (sbuf1.st_mtime <= sbuf4.st_mtime) ||
	    (sbuf1.st_mtime <= sbuf4.st_ctime))
		return(1);  /* One of them is newer than the monitorfile */

    	/* SEE IF THE DATA WAS WRITTEN BY THIS VERSION OF THE MONITOR */
	if ((fd = open(monitorfile,O_RDONLY)) < 0)
		return(1);
	result = monread(fd, whatbuff, strlen(whatstring));
	if (strncmp(whatbuff, whatstring, strlen(whatstring)))
		/* VERSIONS DON'T MATCH */
		result = 1;
	else {
		/* READ /dev AND /hp-ux DATA FROM monitorfile */
		result = result || monread(fd, &ndev, sizeof(ndev));
		if (ndev > NDEV) {
			fprintf(stderr, "Greater than %d device files\n",NDEV);
			exit(-1);
		}
		result = result || monread(fd, devl, ndev * sizeof(*devl));
		result = result || monread(fd, nl, sizeof(nl));
	}
	if (close(fd) != 0) {
		fprintf(stderr,"%s: Can't close %s\n",myname,monitorfile);
		result = 1;
	}

	/* RETURNING A NON-ZERO VALUE MEANS RE-BUILD monitorfile */
	return(result);

} /* readata */

/**********************************************************************/

getdev(dev_dir)

	char *dev_dir;
{
	DIR *dirp;
	struct direct *dp;
	struct stat sbuf1;

	/* HAVE A LOOK INSIDE THE SPECIFIED DIRECTORY */
	if ((dirp = opendir(dev_dir)) == NULL) {
		fprintf(stderr, "Can't opendir %s\n",dev_dir);
		perror("opendir");
		exit(-1);
	}
	if (chdir(dev_dir) < 0) {
		fprintf(stderr, "Can't chdir to %s\n",dev_dir);
		perror("chdir");
		exit(-1);
	}

	/* SEARCH DIRECTORY FOR DEVICE FILES FOR CONTROLLING PTYS */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if ((dp->d_ino == 0) || (stat(dp->d_name, &sbuf1) < 0))
			continue;

		/* STORE DEVICE FILE INFORMATION AS WE SEE IT */
		if ((sbuf1.st_mode & S_IFMT) != S_IFCHR)
			continue;
		strcpy(devl[ndev].dname, dp->d_name);
		devl[ndev].dev = sbuf1.st_rdev;
		ndev++;
		if (ndev > NDEV) {
			fprintf(stderr,"Greater than > %d device files\n",NDEV);
			exit(-1);
		}
	}

	/* CLEAN UP BEFORE EXIT */
	closedir(dirp);

} /* getdev */

/**********************************************************************/

/* WRITE OUT A NEW monitorfile */

wrdata()

{
	int fd;

	/* MAKE SURE WE START FRESH */
	unlink(monitorfile);

	/* CREATE NEW FILE */
	umask(022);
	if ((fd = open(monitorfile,O_WRONLY | O_CREAT | O_EXCL, 0664)) > -1) {

		/* WRITE whatstring FOR IDENTIFICATION */
		monwrite(fd, whatstring, strlen(whatstring));

		/* WRITE /dev DATA */
		monwrite(fd, &ndev, sizeof(ndev));
		monwrite(fd, devl, ndev * sizeof(*devl));

		/* WRITE /hp-ux DATA */
		monwrite(fd, nl, sizeof(nl));

		if (close(fd) != 0) {
			perror("close");
			exit(-1);
		}
	}

} /* wrdata */

/**********************************************************************/

abnorm_exit(ex_val)

	int 	ex_val;
{
	/* PUT TTY BACK TO NORMAL BEFORE ABORTING */
	tty_normal();
	exit(ex_val);

} /* abnorm_exit */

/**********************************************************************/

tty_raw()

{
#ifdef CURSES
	nonl();
	cbreak();
	noecho();
#endif  /* CURSES */
	/* GET TTY STATE */
	if (ioctl(fdin,TCGETA, &tty_state) == -1)
	{
		perror("ioctl TCGETA");
		no_error = 0;
		return(-1);
	}

	/* SAVE VEOF AND VINTR CHARACTER DEFINITIONS */
	veof = tty_state.c_cc[VEOF];
	vintr = tty_state.c_cc[VINTR];

#ifndef CURSES
	/* SET THE TERMINAL IN RAW MODE - NON-BLOCKING READS */
	tty_state_old = tty_state;
	tty_state.c_oflag = tty_state.c_lflag = 0;
	tty_state.c_cc[VMIN] = 1;

	if (ioctl(fdin,TCSETA, &tty_state) == -1)
	{
		perror("ioctl TCSETA");
		no_error = 0;
		return(-1);
	}

	old_flags = fcntl(fdin,F_GETFL,0);
	if (old_flags == -1)
	{
		perror("fcntl F_GETFL");
		ioctl(fdin,TCSETA, &tty_state_old);
		no_error = 0;
		return(-1);
	}

	if (fcntl(fdin,F_SETFL,old_flags | O_NDELAY) == -1)
	{
		perror("fcntl F_SETFL");
		ioctl(fdin,TCSETA, &tty_state_old);
		no_error = 0;
		return(-1);
	}

	/* SET FLAG SO tty_normal KNOWS IT CAN USE THE OLD SAVED VALUES */
	old_valid = 1;
#endif  /* not CURSES */

	return(0);

} /* tty_raw */

/**********************************************************************/

tty_normal()

{
#ifdef CURSES
	nonl();
	nocbreak();
	echo();
#else
	/* SET THE TTY BACK TO THE ORIGINAL MODE (SAVED BY tty_raw()) */
	if (old_valid)
	{
		if (fcntl(fdin,F_SETFL,old_flags) == -1)
		{
			no_error = 0;
			perror("fcntl F_SETFL");
		}

		if (ioctl(fdin,TCSETA, &tty_state_old) == -1)
		{
			no_error = 0;
			perror("ioctl TCSETA");
		}
	}
	if (no_error)
		return(0);
	else
		return(-1);
#endif  /* not CURSES */

} /* tty_normal */

/**********************************************************************/

/*
 * This is the signal handler when the monitor is in interactive mode.
 */

void
interactive_signals(sig)
int sig;
{
#ifdef SIGTSTP
	if (sig == SIGSTOP || sig == SIGTSTP) {
		update_crt();
		kill(getpid(), sig);
		return;
	}
#endif  /* SIGTSTP */

	/* CLEAR THE SCREEN */
	clear_screen();

	/* PAUSE TO ALLOW TERMINAL TO STABILIZE */
	sleep(1);

	/* PUT TTY BACK TO NORMAL */
	/* TERMINATE PROGRAM */
	abnorm_exit(0);

} /* interactive_signals */

/**********************************************************************/

/* This is the signal handler when the monitor is selected to output data
   to stdout rather than operate in interactive mode. */

void
interval_signals(sig)

{
	register char *timestring;
	register int out_index = 0;
	register int i, temp, count;
	char outline[256];
	int lan_count = 0;
	int t, ticks, system, user, time_now;
	struct fis lan_info;
	long swap_avail, freepages;
	struct csp_stats csp_stats;
	struct swaptab *sp, swapent;
#ifdef __hp9000s800
	long *dktime_ptr;
#endif  /* __hp9000s800 */

	/* RELATIVE TIME SINCE STARTING */
	if (sigflags & SF_RTIME) {
		gettimeofday(&current_time,0);
		out_index += sprintf(&outline[out_index],"%-10d ",
				     ((current_time.tv_sec -
				       start_time.tv_sec) * 1000) +
				     (current_time.tv_usec / 1000) -
				     (start_time.tv_usec / 1000));
	}

	/* LOCAL TIME AS HH:MM:SS */
	if (sigflags & SF_TIME) {
		time_now = time(0);
		timestring = ctime(&time_now);
		strncpy(&outline[out_index], &timestring[11],8);
		outline[out_index + 8] = 
		  outline[out_index + 9] = 
		  outline[out_index + 10] = ' ';
		out_index += 11;
	}

	/* READ OUT THE CPU UTILIZATION SUMMARY? */
	if (sigflags & (SF_USER | SF_SYSTEM | SF_DISC)) {
	    pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),1,0);
	    cp_time = pst_dynamic_info.psd_cpu_time;
	}

	/* CPU UTILIZATION */
	if (sigflags & (SF_USER | SF_SYSTEM)) {
	    if (iteration == 1)
		/* NOTHING AVAILABLE FIRST TIME AROUND */
		user = system = 0;
	    else {
		/* CAN GIVE PERCENTAGE AFTER 2ND ITERATION */
		ticks = 0;
		for (i = 0; i < CPUSTATES; i++)
			ticks += cp_time[i] - old_cp_time[i];
		if (!ticks)
			ticks = 1;
		user = ((cp_time[CP_USER] -
			 old_cp_time[CP_USER]) * 100) / ticks;
		system = ((cp_time[CP_SYS] -
			   old_cp_time[CP_SYS]) * 100) / ticks;
	    }

	    /* USER CPU UTILIZATION */
	    if (sigflags & SF_USER)
		out_index += sprintf(&outline[out_index],"%3d%%       ",user);

	    /* SYSTEM CPU UTILIZATION */
	    if (sigflags & SF_SYSTEM)
		out_index += sprintf(&outline[out_index],
				     "%3d%%       ",system);
	}
	
	/* DISC UTILIZATION */
	if (sigflags & SF_DISC) {
	    /* INITIALIZE */
	    count = temp = 0;

	    /* COUNT UP CPU TICKS OVER LAST INTERVAL */
	    ticks = 0;
	    for (i = 0; i < CPUSTATES; i++)
		ticks += cp_time[i] - old_cp_time[i];
	    if (!ticks)
		ticks = 1;

	    /* READ OUT PER DISC STATISTICS */
#ifdef hp9000s200
	    get(nl[X_DK_TIME].n_value,s.dk_time, sizeof s.dk_time);

	    /* SUBTRACT PREVIOUS VALUE THEN SAVE NEXT PREVIOUS VALUE */
	    for (i = 0; i < DK_NDRIVE; i++) {
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	    dktime_ptr = s.dk_time;
	    if (sysconf(_SC_IO_TYPE) != IO_TYPE_WSIO) { /* if NOT Snakes */
	        if (s.dk_ndrive) {
		    /* CIO HP-IB DRIVES */
		    get(nl[X_DK_TIME].n_value,s.dk_time,s.dk_ndrive * sizeof(long));
		    dktime_ptr += s.dk_ndrive;
	        }
	    }
	    if (s.dk_ndrive1) {
		    /* NIO HPIB DRIVES */
		    get(nl[X_DK_TIME1].n_value,dktime_ptr,s.dk_ndrive1 * sizeof(long));
		    dktime_ptr += s.dk_ndrive1;
	    }
	    if (s.dk_ndrive2) {
		    /* A-LINK DRIVES */
		    get(nl[X_DK_TIME2].n_value,dktime_ptr,s.dk_ndrive2 * sizeof(long));
		    dktime_ptr += s.dk_ndrive2;
	    }
	    if (s.dk_ndrive3) {
		    /* SCSI  DRIVES */
		    get(nl[X_DK_TIME3].n_value,dktime_ptr,s.dk_ndrive3 * sizeof(long));
		    dktime_ptr += s.dk_ndrive3;
	    }
	    /* SUBTRACT PREVIOUS VALUE THEN SAVE NEXT PREVIOUS VALUE */
	    for (i = 0; i < drives_conf; i++) {
#endif  /* __hp9000s800 */
		    XDIFF(dk_time);
	    }

	    if (iteration != 1)
		/* SUM UTILIZATION FOR ALL DISCS */
#ifdef hp9000s200
	        for (i = 0; i < DK_NDRIVE; i++)
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	        for (i = 0; i < drives_conf; i++)
#endif  /* __hp9000s800 */
		    /* ANY ACTIVITY? */
		    if (s.dk_time[i]) {
			/* ACCUMULATE UTILIZATION PERCENTAGES */
			temp += (s.dk_time[i] * 100) / ticks;
			count++;
		    }

	    /* RESULT */
	    out_index += sprintf(&outline[out_index],"%3d%%       ",
				 (count ? temp / count : 0));
	}

	/* SAVE CPU TIME VALUES FOR THE NEXT ITERATION? */
	if (sigflags & (SF_USER | SF_SYSTEM | SF_DISC))
	    for (i = 0; i < CPUSTATES; i++)
		old_cp_time[i] = cp_time[i];

	/* USER MEMORY UTILIZATION */
	if (sigflags & SF_MEMORY)
		out_index += sprintf(&outline[out_index],"%3d%%       ",
				     (100 * (maxmem -
					     getvalue(nl[X_FREEMEM].n_value))) /
				     maxmem);

	freepages = 0;
	/* FETCH DISCLESS SITE STATUS? */
	if (sigflags & (SF_SWAP | SF_BLOCK))
		my_site_status = getvalue(nl[X_MY_SITE_STATUS].n_value);

	/* SWAP SPACE UTILIZATION */
	if (sigflags & SF_SWAP) {
		/* IS SITE SWAPLESS? */
		if (!(my_site_status & CCT_SLWS) 
				&& swapent.st_site != -1) {  
			freepages += swapsize(SW_DEV_FREE);
		}
		

		/* FETCH SWAP SPACE AVAILABLE STATISTICS */
		swap_avail =  swapsize(SW_DEV_ENAB);

		/* ARE WE WITHOUT SWAP SPACE ON THIS SYSTEM? */
		if (my_site_status & CCT_SLWS)
			/* YES, SO PRINT ZERO INSTEAD */
			out_index += sprintf(&outline[out_index],
					     "  0%%       ");
		else
			out_index += sprintf(&outline[out_index],"%3ld%%       ",
					     (100 * (swap_avail - 
						     freepages)) / swap_avail);
	}

	/* IN/OUT PACKET RATES FOR LAN */
	if (sigflags & SF_LAN) {
		/* FETCH TRANSMIT PACKET COUNT */
		lan_info.reqtype = TX_FRAME_COUNT;
		lan_info.vtype = INTEGERTYPE;
		if (ioctl(lan_fd,NETSTAT, &lan_info) == -1) {
			perror("ioctl NETSTAT TX_FRAME_COUNT");
			exit(-1);
		} else
			lan_count += lan_info.value.i;

		/* FETCH RECEIVE PACKET COUNT */
		lan_info.reqtype = RX_FRAME_COUNT;
		lan_info.vtype = INTEGERTYPE;
		if (ioctl(lan_fd,NETSTAT, &lan_info) == -1) {
			perror("ioctl NETSTAT RX_FRAME_COUNT");
			exit(-1);
		} else
			lan_count += lan_info.value.i;

		/* PRINT SUM */
		out_index += sprintf(&outline[out_index],"%-10d ",lan_count);
	}

	/* READ DISCLESS MESSAGE PROTOCOL STATS? */
	if (sigflags & (SF_DROPPED | SF_MSG | SF_RETRIES)) {
	    /* IS DISKLESS CONFIGURED INTO THE KERNEL? */
	    if (getvalue(nl[X_DSKLESS_INITIALIZED].n_value)) {
		/* FETCH STATISTICS FROM PROTOCOL LEVEL */
		if (dskless_stats(STATS_READ_PROTOCOL_INFO,
				  &proto_stats,NULL,NULL) < 0) {
		    fprintf(stderr,
			"Can't read dskless protocol statistics from kernel");
		    exit(-1);
	        }
	    }
	}

	/* DROPPED DISCLESS PACKETS */
	if (sigflags & SF_DROPPED) {
	    /* IS DISKLESS CONFIGURED INTO THE KERNEL? */
	    if (getvalue(nl[X_DSKLESS_INITIALIZED].n_value))
		out_index += sprintf(&outline[out_index],"%-10d ",
				     STATS_recv_no_mbuf +
				     STATS_recv_no_cluster +
				     STATS_recv_no_buf +
				     STATS_serving_entry +
				     STATS_recv_dgram_no_mbuf +
				     STATS_recv_dgram_no_cluster +
				     STATS_recv_dgram_no_buf +
				     STATS_recv_dgram_no_buf_hdr);
	    else
		out_index += sprintf(&outline[out_index],"%-10d ",0);
	}

	/* DISCLESS PACKETS SENT/RECEIVED */
	if (sigflags & SF_MSG) {
	    /* IS DISKLESS CONFIGURED INTO THE KERNEL? */
	    if (getvalue(nl[X_DSKLESS_INITIALIZED].n_value))
		out_index += sprintf(&outline[out_index],"%-10d ",
				     STATS_xmit_op_P_REQUEST +
				     STATS_xmit_op_P_REPLY +
				     STATS_xmit_op_P_ACK +
				     STATS_xmit_op_P_SLOW_REQUEST +
				     STATS_xmit_op_P_DATAGRAM +
				     STATS_recv_op_P_REQUEST +
				     STATS_recv_op_P_REPLY +
				     STATS_recv_op_P_ACK +
				     STATS_recv_op_P_SLOW_REQUEST +
				     STATS_recv_op_P_DATAGRAM);
	    else
		out_index += sprintf(&outline[out_index],"%-10d ",0);
	}

	/* DISCLESS PACKET RETRIES */
	if (sigflags & SF_RETRIES) {
	    /* IS DISKLESS CONFIGURED INTO THE KERNEL? */
	    if (getvalue(nl[X_DSKLESS_INITIALIZED].n_value))
		out_index += sprintf(&outline[out_index],"%-10d ",
				     STATS_req_retries + STATS_retry_reply);
	    else
		out_index += sprintf(&outline[out_index],"%-10d ",0);
	}

	/* CSP REQUESTS */
	if (sigflags & SF_CSP) {
		getvar(nl[X_CSP_STATS].n_value, csp_stats);
		out_index += sprintf(&outline[out_index],"%-10d ",
				     csp_stats.requests[CSPSTAT_LIMITED] +
				     csp_stats.requests[CSPSTAT_SHRT] +
				     csp_stats.requests[CSPSTAT_MED] +
				     csp_stats.requests[CSPSTAT_LONG]);
	}

	/* READ VMMETER STRUCTURE? */
	if (sigflags & (SF_BLOCK | SF_SYSCALLS | SF_PAGE)) {
		getvar(nl[X_SUM].n_value, cnt);
	}

	/* FILE SYSTEM BLOCK I/O */
	if (sigflags & SF_BLOCK) {
	    /* DISCLESS CLIENT OR LOCAL FILE SYSTEM? */
	    if ((my_site_status & CCT_CLUSTERED) &&
		!(my_site_status & CCT_ROOT)) {
		/* FETCH OUTBOUND DISCLESS MESSAGE OPCODE STATISTICS */
		if (dskless_stats(STATS_READ_OUTBOUND_OPCODES,
				  &outbound_opcode_stats,NULL,NULL) < 0) {
			fprintf(stderr,
		"Can't read outbound discless opcode statistics from kernel");
			abnorm_exit(-1);
		}

		/* DISPLAY BLOCK STATISTICS INCLUDING SYNC ACTIVITY */
		out_index += sprintf(&outline[out_index],"%-10d ",
				     cnt.f_bread - cnt.f_breadcache +
				     cnt.f_breada - cnt.f_breadacache +
				     cnt.f_bwrite + cnt.f_bdwrite +
				     outbound_opcode_stats.
				       opcode_stats[DMSYNCSTRAT_READ] +
				     outbound_opcode_stats.
				       opcode_stats[DMSYNCSTRAT_WRITE]);
	    } else
		/* FILE SYSTEM BLOCK I/O REQUESTS MINUS CACHE HITS */
		out_index += sprintf(&outline[out_index],"%-10d ",
				     cnt.f_bread - cnt.f_breadcache +
				     cnt.f_breada - cnt.f_breadacache +
				     cnt.f_bwrite + cnt.f_bdwrite);
	}

	/* SYSTEM CALLS */
	if (sigflags & SF_SYSCALLS)
		out_index += sprintf(&outline[out_index],"%-10d ",
				     cnt.v_syscall);

	/* VM PAGE READS/WRITES */
	if (sigflags & SF_PAGE)
		out_index += sprintf(&outline[out_index],"%-10d ",
				     cnt.v_pswpin + cnt.v_pgpgin +
				     cnt.v_pswpout + cnt.v_pgpgout);

	/* SEND OUT RESULTS */
	outline[out_index - 1] = '\n';
	outline[out_index] = '\0';
	printf("%s",outline);

	/* ITERATION COUNTS */
	iteration++;
	iterationcnt--;

	/* RE-ENABLE FOR NEXT INTERVAL */
	if ((int) sigvector(SIGALRM,&interval_vec, 0) == -1) {
		perror("sigvector");
		exit(-1);
	}

} /* interval_signals */

/**********************************************************************/

invoke_shell()

{
	register char *shell;
	char name[256];
	register int pid, i;
	typedef void (*sigptr)();
	sigptr old_int, old_quit;

	/* PICK UP USER SPECIFIED SHELL */
	shell = getenv("SHELL");
	if (shell == NULL)
		shell = "/bin/sh";

	/* FORK CHILD PROCESS FOR THE SHELL */
	pid = fork();
	if (pid == -1) {
		fprintf(stderr,"Can't invoke a shell\n");
		perror("fork");
		sleep(2);
		return;
	}

	if (pid == 0) {
		/* CHILD PROCESS - MAKE SURE UID/GID ARE RIGHT */
		setuid(getuid());
		setgid(getgid());

		/* DROP BACK TO ORIGINAL NICE VALUE */
		nice(-(NICEVAL));

		/* COME UP WITH THE NAME OF THE SHELL BEFORE EXEC'ING IT */
		if (strchr(shell,'/')) {
			/* NEED BASENAME OF SHELL PATH */
			i = strlen(shell);
			while (shell[i] != '/')
				i--;
			strcpy(name, &shell[i + 1]);
		}
		else
			strcpy(name,shell);

		/* EXEC THE SHELL */
		execl(shell,name,0);

		/* IF WE GET HERE, THE EXEC FAILED */
		fprintf(stderr,"Can't invoke %s\n",shell);
		perror("execl");
		sleep(2);
		exit(-1);
	}

	/* WAIT FOR CHILD PROCESS TO TERMINATE, WHILE IGNORING SIGINT/SIGQUIT */
	ignore_vec.sv_mask = 0;
	ignore_vec.sv_onstack = 0;
	ignore_vec.sv_handler = SIG_IGN;

	(void) sigvector(SIGINT,&ignore_vec,&oldint_vec);
	(void) sigvector(SIGQUIT,&ignore_vec,&oldquit_vec);
	wait(0);
	(void) sigvector(SIGINT,&oldint_vec,0);
	(void) sigvector(SIGQUIT,&oldquit_vec,0);

} /* invoke_shell */

/**********************************************************************/

dump_to_printer(screen_ptr)

	char *screen_ptr;
{
	FILE 	*popen(), *stream;
	register int count;
	char    command_line[80];

	/* THE SINGLE QUOTES PREVENT A POSSIBLE SECURITY HOLE! */
 	if (strlen(hardcopy))
		/* USER SPECIFIED A DEVICE */
		sprintf(command_line,"/usr/bin/lp -s '-d%s'\n",hardcopy);
	else
		/* TAKE DEFAULT DEVICE */
		sprintf(command_line,"/usr/bin/lp -s\n");
	if ((stream = popen(command_line,"w")) == NULL)
	{
		perror("popen");
		abnorm_exit(-1);
	 }
	else
	{
		for (count = 0; count < rows; count++ )
		  {
		    if (fwrite(screen_ptr, COLUMNS, 1, stream) != 1) {
			perror("fwrite");
			abnorm_exit(-1);
		    }
		    screen_ptr += COLUMNS;
		    if (fwrite("\n", 2,1, stream) != 1) {
			perror("fwrite");
			abnorm_exit(-1);
		    }
		  }

		if (pclose(stream) == -1)
		{
			perror("pclose");
			abnorm_exit(-1);
		}
         }

} /* dump_to_printer */

/**********************************************************************/

clear_screen()

{
   	register int *nscr_pt, *cscr_pt, *next_end;

	/* CLEAR THE DISPLAY AND ZERO OUT THE SCREEN VARIABLES */
	if (write(fdout,cl,cl_len) != cl_len)
	{
		perror("write cl");
		abnorm_exit(-1);
	}

	/* USE 8 CHAR / TAB_GROUP TO DO INTEGER INITIALIZES */
	nscr_pt = next_scrni;
	next_end = &next_scrni[(rows * COLUMNS) / 4];
	cscr_pt = current_scrni;
	while (nscr_pt < next_end) {
		*nscr_pt++ = 0x20202020;
		*cscr_pt++ = 0x20202020;
	}
	cursor_row = 0;

} /* clear_screen

/**********************************************************************/

update_crt()

{
	register int	i, j, buffer_index, screen_index, end_of_changes;
   	register int	*nscr_pt, *cscr_pt;
	int		row_count, save_index;
	int		current_row, current_col;
	int		temp_row, temp_col;

#ifdef CURSES
	move(0,0);
#else /* not CURSES */
	/* MOVE CURSOR TO THE TOP OF THE SCREEN */
	if (ho_or_cm)
	{
		/* START OUT WITH A HOME CURSOR ESCAPE SEQUENCE */
		strncpy(buffer,ho,ho_len);
		buffer_index = ho_len;
	}
	else
	{
		/* USE THE UP CURSOR SEQUENCE TO HOME TO THE TOP */
		buffer[0] = '\r';
		buffer_index = 1;
		row_count = ((cursor_row < (rows - 1)) ? cursor_row :
							 (rows - 1));
		for (i = 1; i <= row_count; i++)
		{
			strncpy(&buffer[buffer_index],ho,ho_len);
			buffer_index += ho_len;
		}
	}

#endif  /* CURSES */

	/* ITERATE UNTIL ALL OF THE SCREENS ARE COMPARED */
	current_row = current_col = end_of_changes = 0;
	do
	{
		/* SEARCH FOR NEXT MIS-MATCH */
		screen_index = (current_row * COLUMNS) + current_col;

		/* USE 8 CHAR / TAB GROUP TO DO INTEGER COMPARES */
		nscr_pt = &next_scrni[screen_index/4];
		cscr_pt = &current_scrni[screen_index/4];
		while ((screen_index < (rows * COLUMNS)) &&
		       (*nscr_pt++ == *cscr_pt++) &&
		       (*nscr_pt++ == *cscr_pt++))
			screen_index += 8;

		/* CONVERT BACK TO COORDINATE VALUES */
		temp_row = screen_index / COLUMNS;
		temp_col = screen_index % COLUMNS;

		/* DID WE REACH THE END? */
		if (temp_row >= rows)
			/* YES, DONE! */
			break;
		else
		{
			/* MOVE THE CURSOR BACKWARDS? */
			if (temp_col < current_col)
			{
				buffer[buffer_index++] = '\r';
				current_col = 0;
			}

			/* MOVE CURSOR FORWARD? */
			for (i = 1; i <= ((temp_col - current_col) / 8); i++)
				buffer[buffer_index++] = '\t';
			current_col = temp_col;

			/* ADJUST THE ROW POSITION */
			for (i = 1; i <= (temp_row - current_row); i++)
				buffer[buffer_index++] = '\n';
			current_row = temp_row;

			/* FIND THE NEXT TAB GROUP WHERE ALL THE CHAR MATCH */
			save_index = screen_index;
			screen_index += 8;

		        /* USE 8 CHAR / TAB GROUP TO DO INTEGER COMPARES */
			nscr_pt = &next_scrni[screen_index/4];
			cscr_pt = &current_scrni[screen_index/4];
			i = (*nscr_pt++ != *cscr_pt++);
			j = (*nscr_pt++ != *cscr_pt++);
			while ((screen_index < (rows * COLUMNS)) && (i || j)) {
				/* NOTE THAT ONLY i MAY BE EVALUATED ABOVE */
				i = (*nscr_pt++ != *cscr_pt++);
				j = (*nscr_pt++ != *cscr_pt++);
				screen_index += 8;
			}
			end_of_changes = screen_index;

			/* DO WE NEED A CR/LF AT THE END OF LINES? */
			if ((columns > COLUMNS) || (automargin == 0) || eolwrap)
			{
				/* NEXT MATCHING TAB GROUP NOT ON THIS LINE? */
				if ((screen_index / COLUMNS) ==
				    (save_index / COLUMNS))
				{
					/* NO, JUST MOVE TEXT IN */
					strncpy(&buffer[buffer_index],
						&next_screen[save_index],
						screen_index - save_index);
					buffer_index += screen_index -
							save_index;
				}
				else
				{
					/* YES, ONLY MOVE REST OF LINE */
					end_of_changes =
					  screen_index = ((save_index /
							   COLUMNS) + 1) *
						         COLUMNS;
					strncpy(&buffer[buffer_index],
						&next_screen[save_index],
						screen_index - save_index);
					buffer_index += screen_index -
							save_index;

					/* ADD CR/LF TO BUFFER */
					buffer[buffer_index++] = '\r';
					buffer[buffer_index++] = '\n';
				}
			}
			else
			{
				/* MOVE THE REVISION TEXT TO THE BUFFER */
				strncpy(&buffer[buffer_index],
					&next_screen[save_index],
					screen_index - save_index);
				buffer_index += screen_index - save_index;
			}

			/* UPDATE THE CURRENT CURSOR POSITION */
			cursor_row = current_row = screen_index / COLUMNS;
			current_col = screen_index % COLUMNS;
		}
	} while (current_row != rows);

	/* DON'T SEND OUT LAST 1 TO 7 CHARACTERS IF UNNECESSARY */
	if ((end_of_changes > 0) && (current_screen[end_of_changes - 1] ==
				     next_screen[end_of_changes - 1]))
	{
		/* HAS THE LAST TAB GROUP SEND STOPPED AT END OF THE LINE? */
		if ((end_of_changes % COLUMNS) == 0)
		{
			/* IF WE BACKUP, THEN CURSOR IS ACTUAL BACK UP A ROW */
			cursor_row -= 1;

			/* WATCH OUT FOR THOSE ADDED CR/LF */
			if ((columns > COLUMNS) || (automargin == 0) || eolwrap)
				buffer_index -= 2;
		}

		/* IGNORE THE 8TH CHARACTER OF THE TAB GROUP */
		buffer_index -= 1;

		/* CHECK THE REST */
		for (i = 2; i <= 7; i++)
			if (current_screen[end_of_changes - i] ==
			    next_screen[end_of_changes - i])
				buffer_index -= 1;
			else
				break;
	}

	/* SEND IT OUT TO THE TTY */
	if (buffer_index > buffer_size) {
		fprintf(stderr,
			"Internal error, overflowed buffer array (%d>%d)\n",
			buffer_index,buffer_size);
		abnorm_exit(-1);
	}
	if (write(fdout,buffer,buffer_index) != buffer_index) {
		perror("write buffer");
		abnorm_exit(-1);
	}

	/* NOW THE NEXT SCREEN BECOMES THE CURRENT SCREEN */
	strncpy(current_screen,next_screen,rows * COLUMNS);

} /* update_crt */

/**********************************************************************/

/* FILL stuff_ptr[] ARRAY WITH CHARACTERS PASSED TO THIS ROUTINE BY tputs() */

stuff_array(c)

	char c;

{
	stuff_ptr[stuff_len++] = c;
	stuff_ptr[stuff_len] = '\0';
	if (stuff_len > MAX_ESCAPE) {
		fprintf(stderr,"Terminfo escape sequence > %d\n",MAX_ESCAPE);
		exit(-1);
	}

} /* stuff_array */

int largest_line_number;

extern int mode_c(),  mode_c_init();
int mode__t(), mode_t_init();
int mode_v();
int mode_g(),  mode_g_init();
int mode_i(),  mode_i_init();
/* int mode_i_sn(),  mode_i_sn_init(); */
int mode_i_sn_init();
int mode_s(),  mode_s_init();
int mode_l(),  mode_l_init();
int mode_k(),  mode_k_init();
int mode_m();
int mode_n();
int mode_o();
int mode_r();
extern int mode_help();
extern int mode_x();

struct modes *mode, modes[] = {
	{ 'C',  mode_c,    mode_c_init,	"CONFIG VAL/DRIVERS"},
	{ 'T',  mode__t,   mode_t_init,	"TASKS RUNNING"},
	{ 'V',  mode_v,    NULL,	"VIRTUAL MEMORY STATUS"},
	{ 'G',  mode_g,    mode_g_init,	"GLOBAL SYS STATUS"},
	{ 'I',  mode_i,    mode_i_init,	"I/O STATUS"},
	{ 'S',  mode_s,    mode_s_init,	"SINGLE PROCESS INFO"},
	{ 'L',  mode_l,    mode_l_init,	"LAN STATUS"},
	{ 'K',  mode_k,    mode_k_init,	"DISKLESS STATUS"},
	{ 'M',  mode_m,    NULL,	"PROTOCOL STATUS"},
	{ 'N',  mode_n,    NULL,	"NETWORKED STATUS"},
	{ 'O',  mode_o,    NULL,	"OPCODE STATITISTICS"},
	{ 'R',  mode_r,    NULL,	"REMOTE UPTIME"},
	{ '?',  mode_help, NULL,	"HELP LISTING"},
	{ '\0', mode_x,    NULL,	"HELP MENU"},
};




mode_g_init()
{
	register int i;

	/* INITIALIZE */
	maxmem = getvalue(nl[X_MAXMEM].n_value);
	swap_conf = swapsize(SW_DEV_CONF);

	/* PHYSICAL MEMORY IS DESCRIBED IN NBPG UNITS */
	physmem = pst_static_info.physical_memory * NBPG;

#ifdef hp9000s200
	/* BOOT ROM USES FIRST 8K OF physmem, SO ROUND UPTO 64K TO GET REAL */
	physmem = ((physmem + 65535) / 65536) * 65536;
#endif  /* hp9000s200 */

	/* RETURN TO NBPG UNITS FOR LATER USE */
	physmem /= NBPG;

#ifdef hp9000s200
	/* READ OUT BOOT DEVICE MSUS AND FORMAT IT FOR DISPLAY */
	getvar(nl[X_MSUS].n_value, msus);
	switch(msus.dir_format) {
	  case 0: /* LIF FORMAT */
		  switch(msus.device_type) {
		    case 14: sprintf(msus_string,"SCSI: 0x%02x%02x%01x%01x",
				     msus.sc,msus.ba,msus.unit,msus.vol);
			     break;
		    case 16:
		    case 17: sprintf(msus_string,"CS80: 0x%02x%02x%01x%01x",
				     msus.sc,msus.ba,msus.unit,msus.vol);
			     break;
		    default: sprintf(msus_string,"MSUS: 0x%08x",*(int *)&msus);
		  }
		  break;

	  case 7: /* ROM/SRM/LAN */
		  if (msus.device_type == 2) {
			  sprintf(msus_string,"LAN: 0x%02x0000",msus.sc);
			  break;
		  }

	  default: /* UNKNOWN */
		  sprintf(msus_string,"MSUS: 0x%08x",*(int *)&msus);
	}

	/* FETCH SYSTEM NAME AND FIX IT UP */
	get(nl[X_SYSNAME].n_value,sysname, sizeof(sysname));
	sysname[10] = '\0';
	for (i = 9; i >= 0; i--)
		if (sysname[i] == ' ')
			/* STRIP TRAILING BLANKS */
			sysname[i] = '\0';
		else
			/* DONE */
			break;
	if (!strcmp(sysname,"SYSHPUX"))
		strcpy(sysname,"hp-ux");
#endif  /* hp9000s200 */

} /* mode_g_init */

/**********************************************************************/

mode_g()
{
	register int i, j, k;
	register struct swdevt *swp;
	int scrolling=0;

	struct fis lan_info1, lan_info2;
	struct swdevt swapdev;
	int freemem; 
	long swap_avail; 
	int freepages; 
#ifdef SWFS
	long fswap_avail;
	int fsfreepages;
#endif  /* SWFS */
	int drive_count = 0;
	int t, found;
	char drive_name[256];
	int cpticks = 0;
#ifdef MP 
	int mcpticks[MAX_PROCS];
#endif /* MP */ 

	float delta = 0.0;
#ifdef __hp9000s800
	long *dktime_ptr, *dkxfer_ptr;
	float *dkmspw_ptr;
#endif  /* __hp9000s800 */
#ifdef SWFS
	struct swapfs_info swapfs_buf;
	struct statfs statfs_buf;
#endif  /* SWFS */

	/* These change, so we have to read them every time */
	swapspc_max = getvalue(nl[X_SWAPSPC_MAX].n_value);
	swapspc_cnt = getvalue(nl[X_SWAPSPC_CNT].n_value);

	/* READ OUT PER DISC STATISTICS */
#ifdef hp9000s200
	get(nl[X_DK_TIME].n_value, s.dk_time, sizeof s.dk_time);
	get(nl[X_DK_XFER].n_value, s.dk_xfer, sizeof s.dk_xfer);
	get(nl[X_DK_MSPW].n_value, s.dk_mspw, sizeof s.dk_mspw);
	get(nl[X_DK_DEVT].n_value, s.dk_devt, sizeof s.dk_devt);
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	dktime_ptr = s.dk_time;
	dkxfer_ptr = s.dk_xfer;
	dkmspw_ptr = s.dk_mspw;
	if (sysconf(_SC_IO_TYPE) != IO_TYPE_WSIO) { /* if NOT Snakes */
	  if (s.dk_ndrive) {
	    /* CIO HP-IB DRIVES */
	    get(nl[X_DK_TIME].n_value,dktime_ptr,s.dk_ndrive * sizeof(long));
	    dktime_ptr += s.dk_ndrive;
	    get(nl[X_DK_XFER].n_value, dkxfer_ptr,s.dk_ndrive * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive;
       
	    get(nl[X_DK_MSPW].n_value, dkmspw_ptr,s.dk_ndrive * sizeof(long));
	    dkmspw_ptr += s.dk_ndrive;
	  }
	}
	if (s.dk_ndrive1) {
	    /* NIO HP-IB DRIVES */
	    get(nl[X_DK_TIME1].n_value,dktime_ptr,s.dk_ndrive1 * sizeof(long));
	    dktime_ptr += s.dk_ndrive1;
	    get(nl[X_DK_XFER1].n_value, dkxfer_ptr,s.dk_ndrive1 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive1;
       
	    get(nl[X_DK_MSPW1].n_value, dkmspw_ptr,s.dk_ndrive1 * sizeof(long));
	    dkmspw_ptr += s.dk_ndrive1;
	}
	if (s.dk_ndrive2) {
	    /* A-LINK DRIVES */
	    get(nl[X_DK_TIME2].n_value,dktime_ptr,s.dk_ndrive2 * sizeof(long));
	    dktime_ptr += s.dk_ndrive2;
	    get(nl[X_DK_XFER2].n_value, dkxfer_ptr,s.dk_ndrive2 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive2;
       
	    get(nl[X_DK_MSPW2].n_value, dkmspw_ptr,s.dk_ndrive2 * sizeof(float));
	    dkmspw_ptr += s.dk_ndrive2;
	}
	if (s.dk_ndrive3) {
	    /* A-LINK DRIVES */
	    get(nl[X_DK_TIME3].n_value,dktime_ptr,s.dk_ndrive3 * sizeof(long));
	    dktime_ptr += s.dk_ndrive3;
	    get(nl[X_DK_XFER3].n_value, dkxfer_ptr,s.dk_ndrive3 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive3;
       
	    get(nl[X_DK_MSPW3].n_value, dkmspw_ptr,s.dk_ndrive3 * sizeof(float));
	    dkmspw_ptr += s.dk_ndrive3;
	}
#endif  /* __hp9000s800 */

	/* DISPLAY HOSTNAME, DISCLESS FUNCTION AND UPTIME OF SYSTEM */
	atf(0, 2, "HOSTNAME: %s",name.nodename);

	/* READ OUT AND DISPLAY STATUS FLAG DESCRIBING THIS SITE */
	my_site_status = getvalue(nl[X_MY_SITE_STATUS].n_value);
	if (my_site_status & CCT_CLUSTERED) {
		if (my_site_status & CCT_ROOT) {
			addf(" (Root server)");
		} else {
			addf(" (Clustered)");
		}
	}


	/* DISPLAY TIME SINCE BOOT AND BOOT DEVICE/SYSTEM */
	atf(39,2, "SYSTEM BOOTED:  %.24s", ctime(&pst_static_info.boot_time));
#ifdef hp9000s200
	atf(39,3, "/%s from %s", sysname, msus_string);
#endif  /* hp9000s200 */


	/* FETCH SWAP SPACE AVAILABLE STATISTICS */
	if (!(my_site_status & CCT_SLWS)) {
		freepages = swapsize(SW_DEV_FREE);
	}
	swap_avail = swapsize(SW_DEV_ENAB);
#ifdef SWFS
	fswap_avail = swapsize(FSW_AVAIL);
	fsfreepages = swapsize(FSW_FREE);
#endif  /* SWFS */

	pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);

	lockable_mem = getvalue(nl[X_LOCKABLE_MEM].n_value);

	/* DISPLAY RESULTS */
	atf(0, 4, "RESOURCE UTILIZATION IN KILOBYTES\n");
	addf("  Physical memory: %17d   Lockable memory: %16d\n",
		physmem * (NBPG >> 10),
		lockable_mem * (NBPG>>10));

	freemem = pst_dynamic_info.psd_free;
	addf("  Free memory: %21d   Maximum user memory: %12d\n",
		freemem * (NBPG >> 10), maxmem * (NBPG >> 10));

	addf("  Buffer cache memory: %13d   User memory utilization: %7d%%\n",
		getvalue(nl[X_BUFPAGES].n_value)*(NBPG>>10),
		(100 * (maxmem - freemem)) / maxmem);

	if (!(my_site_status & CCT_SLWS)) {
           if (swap_conf > swapsize(SWAP_MAX)) {
             addf("  Swap space configured: %11ld   Maximum swap limit:     %9ld\n", swap_conf, swapsize(SWAP_MAX));
           } else {
             addf("  Swap space configured: %11ld   Maximum swap limit:     %9ld\n", swap_conf, swap_conf);
           }
        }

	/* ARE WE WITHOUT SWAP SPACE ON THIS SYSTEM -> USE ROOT SERVER SWAP */
	if (my_site_status & CCT_SLWS) {
		/* YES, DISKLESS */
	  	 addf(
"  Client swap allocated:      %6ld   More space is available on root server\n",
                        swapspc_max * (NBPG >> 10));
                addf(
"  Client swap reserved:       %6ld   Actual swap utilization:      %2ld%%\n",
		swapsize(SWAP_MY_SITE), swap_avail==0 ? 0 : (100 * (swap_avail-freepages))/swap_avail);
	} else { /* NOT DISKLESS */
		addf("  Swap space reserved:        %6ld   Actual swap utilization:      %2ld%%\n",
		swapsize(SWAP_MY_SITE), swap_avail==0 ? 0 : (100 * (swap_avail-freepages))/swap_avail);
	
#ifdef SWFS
		addf("  File system swap available: %6ld   File system swap utilization: %2ld%%\n",
		fswap_avail, fswap_avail==0 ? 0 : (100 * (fswap_avail-fsfreepages))/fswap_avail);
#endif  /* SWFS */
	}

	/* READ OUT THE CPU UTILIZATION SUMMARY */
	cp_time = pst_dynamic_info.psd_cpu_time;
	if (last_cmd == 'G')
	{
		/* AFTER FIRST TIME AROUND, DISPLAY UTILIZATION PERCENTAGES */
		for (i = 0; i < CPUSTATES; i++) {
			cpticks += cp_time[i] - old_cp_time[i];
		}
		if (!cpticks) {
			cpticks = 1;
		}
#ifdef MP 
	    if (runningprocs == 1) {  /* NON-MP Code */
#endif /* MP */ 
		atf(0, 12, "User CPU:%3d%%\n",
				     (100 * (cp_time[CP_USER] -
					     old_cp_time[CP_USER])) / cpticks);
		addf("Sys CPU: %3d%%\n",
		     (100 * ((cp_time[CP_SYS] - old_cp_time[CP_SYS]) 
			    + (cp_time[CP_INTR] - old_cp_time[CP_INTR])
			    + (cp_time[CP_SSYS] - old_cp_time[CP_SSYS])) 
					/ cpticks));
		addf("Idle CPU:%3d%%\n",
				     (100 * ((cp_time[CP_IDLE] -
					      old_cp_time[CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
					     + (cp_time[CP_WAIT] -
					      old_cp_time[CP_WAIT])
#endif  /* CP_WAIT */
					    ) / cpticks));
		addf("Nice CPU:%3d%%\n",
				     (100 * (cp_time[CP_NICE] -
	     				     old_cp_time[CP_NICE])) / cpticks);
#ifdef MP 
	    } /* MP CPU codes are below with the CPU averages */
#endif /* MP */
	}

	/* FETCH LAN PACKET INPUT/OUTPUT RATES */
	delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
		       ((current_time.tv_usec - previous_time.tv_usec) /
			1000000.0));
	if (lan_fd == -1)
		atf(16, 12, "Can't open %s", lan_name);
	else {
		lan_info1.reqtype = RX_FRAME_COUNT;
		lan_info2.reqtype = TX_FRAME_COUNT;
		lan_info1.vtype = lan_info2.vtype = INTEGERTYPE;
		if ((ioctl(lan_fd,NETSTAT, &lan_info1) != -1) &&
		    (ioctl(lan_fd,NETSTAT, &lan_info2) != -1)) {
			/* DISPLAY LAN PACKET INPUT/OUTPUT RATES */
			if (last_cmd == 'G') {
#ifdef MP 
			    if (runningprocs == 1) {
#endif /* MP */ 
				atf(16, 12, "LAN packets in:%5d",
						     (int) ((lan_info1.value.i -
							     input_last) *
							    delta));
				atf(16, 13, "LAN packets out:%4d",
						     (int) ((lan_info2.value.i -
							     output_last) *
							    delta));
#ifdef MP 
			    } else { /* MP system */
				atf(0, 11, "  LAN packets in:    %15d",
						     (int) ((lan_info1.value.i -
							     input_last) *
							    delta));
				atf(39, 11, "LAN packets out:  %15d",
						     (int) ((lan_info2.value.i -
							     output_last) *
							    delta));
			    } /* else */
#endif /* MP */ 
			}

			/* SAVE LAST VALUES FOR RATE CALCULATIONS */
			input_last = lan_info1.value.i;
			output_last = lan_info2.value.i;
		} else
			atf(16, 12, "Can't ioctl %s", lan_name);
	}

	/* DISPLAY CONTEXT SWITCHES, TRAPS, SYSTEM CALL AND INTERRUPT RATES */
	getvar(nl[X_SUM].n_value, cnt);
	if (last_cmd == 'G') {
#ifdef MP 
	    if (runningprocs == 1) {
#endif /* MP */ 
		atf(39, 12, "Context switches: %4d",
				     (int) ((cnt.v_swtch -
					     old_v_swtch) * delta));
		atf(39, 13, "Trap calls:       %4d",
				     (int) ((cnt.v_trap - old_v_trap) * delta));
		atf(39, 14, "System calls:     %4d",
				     (int) ((cnt.v_syscall -
					     old_v_syscall) * delta));
		atf(39, 15, "Device interrupts:%4d",
				     (int) ((cnt.v_intr - old_v_intr) * delta));
#ifdef MP 
	    } else { /* MP system */
		atf(0, 12, "  Context switches:             %4d",
				     (int) ((cnt.v_swtch -
					     old_v_swtch) * delta));
		atf(39, 12, "Trap calls:                  %4d",
				     (int) ((cnt.v_trap - old_v_trap) * delta));
		atf(0, 13, "  System calls:                 %4d",
				     (int) ((cnt.v_syscall -
					     old_v_syscall) * delta));
		atf(39, 13, "Device interrupts:           %4d",
				     (int) ((cnt.v_intr - old_v_intr) * delta));
	    } /* else */
#endif /* MP */ 
	}

	/* SAVE LAST VALUES FOR RATE CALCULATIONS */
	old_v_swtch = cnt.v_swtch;
	old_v_trap = cnt.v_trap;
	old_v_syscall = cnt.v_syscall;
	old_v_intr = cnt.v_intr;

	/* DISPLAY LOAD AVERAGES */
#ifdef MP 
        if (runningprocs == 1) {
#endif /* MP */ 
	    atf(65, 12, "LOAD AVERAGES");
	    pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);
	    atf(65,13, " 1 min: %5.2f", pst_dynamic_info.psd_avg_1_min);
    	    atf(65,14, " 5 min: %5.2f", pst_dynamic_info.psd_avg_5_min);
	    atf(65,15, "15 min: %5.2f", pst_dynamic_info.psd_avg_15_min);
#ifdef MP 
        } else {
	    for (i = 0; i < MAX_PROCS; i++) {
	       mcpticks[i] = 0;
	    }
	    atf(0, 15, "");		/* position for scrolling */
	    start_scrolling_here();
	    scrolling=1;
	    if (last_cmd == 'G') {
	 	atf(13, 15, "MULTI CPU UTILIZATION");
	        atf(59, 15, "LOAD AVERAGES");
	        addf("\n  CPU:  User CPU:  Sys CPU: Idle CPU: Nice CPU:      1 min:  5 min:  15 min:\n");
	        for (i = 0; i < runningprocs; i++) {
		    for (j = 0; j < CPUSTATES; j++) {
		        mcpticks[i] += pst_dynamic_info.psd_mp_cpu_time[i][j] - old_mcp_time[i][j];
		        if (!mcpticks[i])
			    mcpticks[i] = 1;
		    }
		    addf("  %3d %7d%% %8d%% %8d%% %8d%%       %7.2f %7.2f %8.2f\n",
			i, ((100 * 
				(pst_dynamic_info.psd_mp_cpu_time[i][CP_USER] -
				     old_mcp_time[i][CP_USER])) / mcpticks[i]),
			   (100 * 
				((pst_dynamic_info.psd_mp_cpu_time[i][CP_SYS] -
				 old_mcp_time[i][CP_SYS])
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_INTR]
				 - old_mcp_time[i][CP_INTR])
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_SSYS]
				 - old_mcp_time[i][CP_SSYS])) / mcpticks[i]),
			   (100 * 
				((pst_dynamic_info.psd_mp_cpu_time[i][CP_IDLE] -
				     old_mcp_time[i][CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_WAIT]
				 - old_mcp_time[i][CP_WAIT])
#endif  /* CP_WAIT */
				    ) / mcpticks[i]),
			   (100 * 
				(pst_dynamic_info.psd_mp_cpu_time[i][CP_NICE] -
	     			     old_mcp_time[i][CP_NICE]) / mcpticks[i]),
			   pst_dynamic_info.psd_mp_avg_1_min[i],
			   pst_dynamic_info.psd_mp_avg_5_min[i],
			   pst_dynamic_info.psd_mp_avg_15_min[i]);
	        }   
		addf("  AVG: %6d%% %8d%% %8d%% %8d%%       %7.2f %7.2f %8.2f\n",
		    100 * (cp_time[CP_USER] - old_cp_time[CP_USER]) / cpticks,
		    100 * ((cp_time[CP_SYS] - old_cp_time[CP_SYS]) 
		    + (cp_time[CP_INTR] - old_cp_time[CP_INTR])
		    + (cp_time[CP_SSYS] - old_cp_time[CP_SSYS])) / cpticks,
		    100 * ((cp_time[CP_IDLE] - old_cp_time[CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
					     + (cp_time[CP_WAIT] -
					      old_cp_time[CP_WAIT])
#endif  /* CP_WAIT */
					    ) / cpticks,
		    100 * (cp_time[CP_NICE] - old_cp_time[CP_NICE]) / cpticks,
		    pst_dynamic_info.psd_avg_1_min,
		    pst_dynamic_info.psd_avg_5_min,
		    pst_dynamic_info.psd_avg_15_min);
	    } 
	}
#endif /* MP */ 

	/* READ OUT SWAP DEVICES */
#ifdef hp9000s200
	for (i = 0; i < DK_NDRIVE; i++)
		swap_dev_size[i] = 0;
#endif  /* hp9000s200 */

	/* READ OUT /etc/mnttab IF NOT DONE YET */
	if (!mount_devices_gotten) {
		get_mount_devices();
		mount_devices_gotten = 1;
	}

#ifndef MP
#define FIRST_LINE_G 18
#else
#define FIRST_LINE_G ((runningprocs>1) ? 20+runningprocs : 18)
#endif

	atf(0, FIRST_LINE_G-1, "    SWAP DEVICE         KILOBYTES\n");

	if (!scrolling)
		start_scrolling_here();
	if (swap_conf) {
	  	swp = (struct swdevt *)nl[X_SWDEVT].n_value;
	 	for (j=0; j < nswapdev; j++) {
			/* READ OUT ENTRY */
			getvar(swp++, swapdev);

#ifdef hp9000s200
			/* IF A DRIVE IS USED FOR SWAP, STORE SIZE */
			for (i = 0; i < DK_NDRIVE; i++)
			    if (minor(s.dk_devt[i]) == minor(swapdev.sw_dev))
				swap_dev_size[i] = swapdev.sw_nblks;
#endif  /* hp9000s200 */

			/* DISPLAY SWAP DEVICES */
			if (swapdev.sw_dev && swapdev.sw_nblks)
				addf("  %3u  0x%06x       %11d\n",
					     major(swapdev.sw_dev),
					     minor(swapdev.sw_dev),
					     swapdev.sw_nblks);
		}
#ifdef SWFS
		for (i = 0; i < nmnt; i++)
		{
		    if (swapfs(mntl[i]->name, &swapfs_buf) == 0 &&
			statfs(mntl[i]->name, &statfs_buf) == 0)
			    addf("   %-20.20s %9d\n",
				 mntl[i]->name,
				 (swapfs_buf.sw_bavail * statfs_buf.f_bsize) >> 10);
		}
#endif  /* SWFS */
	}

	/* SUBTRACT PREVIOUS VALUE (s1) FROM s THEN SAVE NEXT PREVIOUS VALUE */
#ifdef hp9000s200
	for (i = 0; i < DK_NDRIVE; i++) 
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	for (i = 0; i < drives_conf; i++) 
#endif  /* __hp9000s800 */
	{
		XDIFF(dk_xfer); XDIFF(dk_time);
	}

	/* ONLY AFTER THE FIRST ITERATION */
	if (last_cmd == 'G') {
	    /* DISPLAY PER DISC DRIVE STATISTICS */
	    atf(39, FIRST_LINE_G-1, "XFERS  UTIL DISC");
#ifdef hp9000s200
	    for (i = 0; i < DK_NDRIVE; i++)
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	    for (i = 0; i < drives_conf; i++)
#endif  /* __hp9000s800 */
		/* ARE THE DATA STRUCTURES INITIALIZED FOR THIS DRIVE? */
		/* if (s.dk_mspw[i] != 0.0) */ 
	    {
#ifdef hp9000s200
		    /* FIND DRIVE NAME? - LOOK IN MOUNT TABLE 1ST */
		    found = 0;
		    sprintf(drive_name,"0x%06x",minor(s.dk_devt[i]));

		    /* ONLY LOOK IF NOT A DISCLESS CLIENT */
		    if (!(my_site_status & CCT_CLUSTERED) ||
		        (my_site_status & CCT_ROOT)) {
			if (s.dk_devt[i]) {
				/* LOOK FOR DEVICE NUMBER IN /etc/mnttab DATA */
				k = 0;
				while (k < nmnt) {
				    if (minor(mntl[k]->dev) ==
					minor(s.dk_devt[i])) {
					found = 1;
					strcpy(drive_name,mntl[k]->name);
					break;
				    }
				    k++;
				}

				/* FOUND NAME? */
				if (!found) {
				    /* RE-READ /etc/mnttab AND */
				    get_mount_devices();

				    /* LOOK AGAIN BEFORE GIVING UP */
				    k = 0;
				    while (k < nmnt) {
				    	if (minor(mntl[k]->dev) ==
					    minor(s.dk_devt[i])) {
					       found = 1;
					       strcpy(drive_name,mntl[k]->name);
					       break;
					}
					k++;
				    } /* while */
				}   /* !found */
			} /* if */
		    }
#endif  /* hp9000s200 */

#ifdef __hp9000s800
		    /* SPECTRUM DOESN'T SAVE THE MAJOR/MINOR NUMBER */
		    if (!(my_site_status & CCT_CLUSTERED) ||
		        (my_site_status & CCT_ROOT))
		        found = 1;
		    else
		        found = 0;
		    sprintf(drive_name,"%d",i);
#endif  /* __hp9000s800 */

		    /* DISPLAY ONLY IF DRIVE IS MOUNTED */
		    if (found)
#ifdef hp9000s200
			    /* OR A SWAP DISK */
		    if (found || swap_dev_size[i])
#endif  /* hp9000s200 */
		    {
			 /* DISPLAY THIS DRIVE ON THIS SCREEN? */
			 atf(39, FIRST_LINE_G+drive_count, "%-4.0f   %3d%% ",
				  s.dk_xfer[i] * delta,
				  (s.dk_time[i] * 100) / cpticks);

			  drive_name[29] = '\0';  /* Force to < 29 char */
#ifdef hp9000s200
			  if (swap_dev_size[i]) {
				  drive_name[15] = '\0';  /* Force < 15 char */
				  addf("%s (%dMB swap)\n",
					       drive_name,
					       swap_dev_size[i] >> 10);
			  } else
#endif  /* hp9000s200 */
				  addf("%s\n",drive_name);
			 drive_count++;
		     }  /* if found */
		} /* for loop */
	}

	/* SAVE CURRENT VALUES FOR NEXT ITERATION */
#ifdef MP 
	if (runningprocs > 1) {
	    for (i=0; i < runningprocs; i++) {
		for (j=0; j < CPUSTATES; j++) {
		     old_mcp_time[i][j] = pst_dynamic_info.psd_mp_cpu_time[i][j];
		}
	    }
	} 
#endif /* MP */ 

	for (i = 0; i < CPUSTATES; i++)
	    old_cp_time[i] = cp_time[i];

} /* mode_g */

/**********************************************************************/

swapsize(flag)
int flag;

{

#ifdef SWFS
	struct swapfs_info swapfs_buf;
	struct statfs statfs_buf;
#endif  /* SWFS */
	int i, myid;
	int freeblks, totalblks; 
	struct swaptab *sp, swapent;

	totalblks = freeblks = 0;

	/* SWAP DEV */

 	swp = (struct swdevt *)nl[X_SWDEVT].n_value;
        nswapdev = getvalue(nl[X_NSWAPDEV].n_value);
        fswp = (struct fswdevt *)nl[X_FSWDEVT].n_value;
        nswapfs = getvalue(nl[X_NSWAPFS].n_value);
	

	if (flag == SWAP_MY_SITE) {
	    totalblks += swapspc_max - swapspc_cnt;
	    totalblks = totalblks * (NBPG >> 10);
	    return (totalblks);
	}

	if (flag == SWAP_NOT_MY_SITE) {   /* get space allocated to clients */
	    myid = cnodeid();
	    if (myid < 0) {
		atf(0, 8, "Can't read my cnode ID via cnodeid(2)");
		return;
	    }

	    sp = swaptable;
	    do {
		getvar(sp++, swapent);
		    /* local swap */
		if (swapent.st_site != -1) { /* allocated */
		    if (swapent.st_site != myid) {  /*allocated to client*/
		        totalblks += NPGCHUNK * (NBPG >> 10);
		    }
		}
  	    } while (sp < swapMAXSWAPTAB);

	    return (totalblks);

	}




	if (flag == SW_DEV_CONF) {
	    for (i=0; i<nswapdev; i++) {
		getvar(swp++, swpdev);
		if (swpdev.sw_dev != -1) {
			totalblks += swpdev.sw_nblks;
		}
	    }
	
    	    totalblks = (totalblks * DEV_BSIZE) >> 10; 
	    return (totalblks);
	}

	if (flag == SW_DEV_ENAB) {
	    for (i=0; i<nswapdev; i++) {
		getvar(swp++, swpdev);
		if ((swpdev.sw_dev != -1) && swpdev.sw_enable) {
			totalblks += swpdev.sw_nblks;
		}
	    }
	    totalblks = (totalblks * DEV_BSIZE) >> 10;
	    return (totalblks);
	}

	if (flag == SWAP_MAX) {
            totalblks = maxswapchunks * NPGCHUNK;
            totalblks = totalblks * (NBPG >> 10);
            return (totalblks);
        }

	if (flag == SW_DEV_FREE) {
	    for (i=0; i<nswapdev; i++) {
		getvar(swp++, swpdev);
		if ((swpdev.sw_dev != -1) && swpdev.sw_enable) {
			freeblks += swpdev.sw_nfpgs;
		}
	    }
	    freeblks = freeblks * (NBPG >> 10);
	    return (freeblks);
	}

#ifdef SWFS
	/* FS SWAP */

	if (flag == FSW_AVAIL) {
	    for (i=0; i<nswapfs; i++) {
		getvar(fswp++, fswpdev);
		if (fswpdev.fsw_enable) {
		    if (swapfs(fswpdev.fsw_mntpoint, &swapfs_buf) == 0 &&
                        statfs(fswpdev.fsw_mntpoint, &statfs_buf) == 0) {
			totalblks += swapfs_buf.sw_bavail *
				(statfs_buf.f_bsize >> 10);
		    }
	        }
	    }
	    return (totalblks);
	}

	if (flag == FSW_FREE) {
	    for (i=0; i<nswapfs; i++) {
		getvar(fswp++, fswpdev);
		if (fswpdev.fsw_enable) {
		    if (swapfs(fswpdev.fsw_mntpoint, &swapfs_buf) == 0 &&
                        statfs(fswpdev.fsw_mntpoint, &statfs_buf) == 0) {
			freeblks += (swapfs_buf.sw_bavail-swapfs_buf.sw_binuse)
					* (statfs_buf.f_bsize >> 10);
		    }
	        }
	    }
	    return (freeblks);
	}
#endif  /* swapsize */
} 


/**********************************************************************/

mode_i_init()

{
	register int i;

#ifdef __hp9000s800
	if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { /* if Snakes */
	  	mode_i_sn_init();
		return;
	}
#endif  /* __hp9000s800 */

#ifdef hp9000s200
	swap_dev_size = (int *) sbrk(DK_NDRIVE * sizeof(int));
	for (i = 0; i < DK_NDRIVE; i++)
	{
		s1.dk_time[i] = 0;
		s1.dk_wds[i] = 0;
		s1.dk_seek[i] = 0;
		s1.dk_xfer[i] = 0;
	}
#endif  /* hp9000s200 */

#ifdef __hp9000s800
	/* ALLOCATE SPACE FOR PER DISC STATISTICS (INITIALIZED TO ZERO) */

	if (sysconf(_SC_IO_TYPE) != IO_TYPE_WSIO) { /* if NOT Snakes */
	    if (nlist(hpux_file,nl_800) != 0) {
	 	perror("nlist");
		fprintf(stderr,"Can't do an nlist on %s\n", hpux_file);
		exit(-1);
	    }
	    s.dk_ndrive = getvalue(nl_800[X_DK_NDRIVE].n_value);
	}
	s.dk_ndrive1 = getvalue(nl[X_DK_NDRIVE1].n_value);
	s.dk_ndrive2 = getvalue(nl[X_DK_NDRIVE2].n_value);
	s.dk_ndrive3 = getvalue(nl[X_DK_NDRIVE3].n_value);
	/* CIO HP-IB + NIO HP-IB + A-LINK + SCSI? DISC COUNT */
	drives_conf = s.dk_ndrive + s.dk_ndrive1 + s.dk_ndrive2 + s.dk_ndrive3;
 	s.dk_time = (long *) sbrk(drives_conf * sizeof(long));
 	s1.dk_time = (long *) sbrk(drives_conf * sizeof(long));
 	s.dk_wds = (long *) sbrk(drives_conf * sizeof(long));
 	s1.dk_wds = (long *) sbrk(drives_conf * sizeof(long));
 	s.dk_seek = (long *) sbrk(drives_conf * sizeof(long));
 	s1.dk_seek = (long *) sbrk(drives_conf * sizeof(long));
 	s.dk_xfer = (long *) sbrk(drives_conf * sizeof(long));
 	s1.dk_xfer = (long *) sbrk(drives_conf * sizeof(long));
 	s.dk_mspw = (float *) sbrk(drives_conf * sizeof(float));
 	s1.dk_mspw = (float *) sbrk(drives_conf * sizeof(float));
#endif  /* __hp9000s800 */

} /* mode_i_init */

/**********************************************************************/

mode_i()
{
	register int i, j, k;
	int t, found;
	float delta;
	unsigned number, size;
	double atime, words, xtime, itime;
	char drive_name[256];
#ifdef __hp9000s800
	float *dkmspw_ptr;
	long *dktime_ptr, *dkxfer_ptr, *dkwds_ptr, dkseek_ptr;
#endif  /* __hp9000s800 */
	struct swdevt *swp, swapdev;
	int sync_reads, sync_writes;

#ifdef __hp9000s800
/*** if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { 
	  	mode_i_sn();
		return;
	}
***/
#endif  /* __hp9000s800 */

	/* READ OUT DISCLESS STATUS */
	my_site_status = getvalue(nl[X_MY_SITE_STATUS].n_value);

	/* READ OUT DISCLESS SYNCHRONOUS ACTIVITY */
	if ((my_site_status & CCT_CLUSTERED) && !(my_site_status & CCT_ROOT)) {
		/* FETCH OUTBOUND DISCLESS MESSAGE OPCODE STATISTICS */
		if (dskless_stats(STATS_READ_OUTBOUND_OPCODES,
				  &outbound_opcode_stats,NULL,NULL) < 0) {
			atf(0, 9, "Can't read outbound discless opcode statistics from kernel");
			my_site_status = 0;
		} else {
			sync_reads = outbound_opcode_stats.
					 opcode_stats[DMSYNCSTRAT_READ];
			sync_writes = outbound_opcode_stats.
					 opcode_stats[DMSYNCSTRAT_WRITE];
		}
	}


	/* DISPLAY THE CACHE BUFFER SIZE */
	bufpages = getvalue(nl[X_BUFPAGES].n_value);	/* dynamic on s[37]00 */

	atf(42,2, "Pages in buffer pool: %5d",bufpages);

	/* TIME DELTA SINCE LAST ITERATION */
	delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
		       ((current_time.tv_usec - previous_time.tv_usec)
		       / 1000000.0));

	/* READ OUT /etc/mnttab IF NOT DONE YET */
	if (!mount_devices_gotten) {
		get_mount_devices();
		mount_devices_gotten = 1;
	}

	/* READ OUT THE VMMETER STRUCTURE */
	getvar(nl[X_SUM].n_value, cnt);

	/* READ OUT THE CPU UTILIZATION STRUCTURE (FOR DISC UTILIZATION) */
	pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);
	cp_time = pst_dynamic_info.psd_cpu_time;

	/* READ OUT PER DISC STATISTICS */
#ifdef hp9000s200
	get(nl[X_DK_TIME].n_value, s.dk_time, sizeof s.dk_time);
	get(nl[X_DK_XFER].n_value, s.dk_xfer, sizeof s.dk_xfer);
	get(nl[X_DK_WDS].n_value, s.dk_wds, sizeof s.dk_wds);
	get(nl[X_DK_SEEK].n_value, s.dk_seek, sizeof s.dk_seek);
	get(nl[X_DK_MSPW].n_value, s.dk_mspw, sizeof s.dk_mspw);
	get(nl[X_DK_DEVT].n_value, s.dk_devt, sizeof s.dk_devt);
#endif  /* hp9000s200 */

#ifdef __hp9000s800
	dktime_ptr = s.dk_time;
	dkxfer_ptr = s.dk_xfer;
	dkwds_ptr = s.dk_wds;
	dkseek_ptr = s.dk_seek;
	dkmspw_ptr = s.dk_mspw;
	if (s.dk_ndrive) {
	    /* CIO HP-IB DRIVES */
	    get(nl[X_DK_TIME].n_value,dktime_ptr,s.dk_ndrive * sizeof(long));
	    dktime_ptr += s.dk_ndrive;
	    get(nl[X_DK_XFER].n_value, dkxfer_ptr,s.dk_ndrive * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive;
	    get(nl[X_DK_WDS].n_value, dkwds_ptr,s.dk_ndrive * sizeof(long));
	    dkwds_ptr += s.dk_ndrive;
	    get(nl[X_DK_SEEK].n_value, dkseek_ptr,s.dk_ndrive * sizeof(long));
	    dkseek_ptr += s.dk_ndrive;
       
	    get(nl[X_DK_MSPW].n_value,dkmspw_ptr,s.dk_ndrive * sizeof(long));
	    dkmspw_ptr += s.dk_ndrive;
	}
	if (s.dk_ndrive1) {
	    /* NIO HP-IB DRIVES */
	    get(nl[X_DK_TIME1].n_value,dktime_ptr,s.dk_ndrive1 * sizeof(long));
	    dktime_ptr += s.dk_ndrive1;
	    get(nl[X_DK_XFER1].n_value, dkxfer_ptr,s.dk_ndrive1 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive1;
	    get(nl[X_DK_WDS1].n_value, dkwds_ptr,s.dk_ndrive1 * sizeof(long));
	    dkwds_ptr += s.dk_ndrive1;
	    get(nl[X_DK_SEEK1].n_value, dkseek_ptr,s.dk_ndrive1 * sizeof(long));
	    dkseek_ptr += s.dk_ndrive1;
       
	    get(nl[X_DK_MSPW1].n_value, dkmspw_ptr,s.dk_ndrive1 * sizeof(float));
	    dkmspw_ptr += s.dk_ndrive1;
	}
	if (s.dk_ndrive2) {
	    /* A-LINK DRIVES */
	    get(nl[X_DK_TIME2].n_value,dktime_ptr,s.dk_ndrive2 * sizeof(long));
	    dktime_ptr += s.dk_ndrive2;
	    get(nl[X_DK_XFER2].n_value, dkxfer_ptr,s.dk_ndrive2 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive2;
	    get(nl[X_DK_WDS2].n_value, dkwds_ptr,s.dk_ndrive2 * sizeof(long));
	    dkwds_ptr += s.dk_ndrive2;
	    get(nl[X_DK_SEEK2].n_value, dkseek_ptr,s.dk_ndrive2 * sizeof(long));
	    dkseek_ptr += s.dk_ndrive2;
       
	    get(nl[X_DK_MSPW2].n_value, dkmspw_ptr,s.dk_ndrive2 * sizeof(float));
	    dkmspw_ptr += s.dk_ndrive2;
	}
	if (s.dk_ndrive3) {
	    /* A-LINK DRIVES */
	    get(nl[X_DK_TIME3].n_value,dktime_ptr,s.dk_ndrive3 * sizeof(long));
	    dktime_ptr += s.dk_ndrive3;
	    get(nl[X_DK_XFER3].n_value, dkxfer_ptr,s.dk_ndrive3 * sizeof(long));
	    dkxfer_ptr += s.dk_ndrive3;
	    get(nl[X_DK_WDS3].n_value, dkwds_ptr,s.dk_ndrive3 * sizeof(long));
	    dkwds_ptr += s.dk_ndrive3;
	    get(nl[X_DK_SEEK3].n_value, dkseek_ptr,s.dk_ndrive3 * sizeof(long));
	    dkseek_ptr += s.dk_ndrive3;
       
	    get(nl[X_DK_MSPW3].n_value, dkmspw_ptr,s.dk_ndrive3 * sizeof(float));
	    dkmspw_ptr += s.dk_ndrive3;
	}
#endif  /* __hp9000s800 */

	/* SUBTRACT PREVIOUS VALUE (s1) FROM s THEN SAVE NEXT PREVIOUS VALUE */
#ifdef hp9000s200
	for (i = 0; i < DK_NDRIVE; i++) {
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	for (i = 0; i < drives_conf; i++) {
#endif  /* __hp9000s800 */
		XDIFF(dk_xfer); XDIFF(dk_seek); XDIFF(dk_wds); XDIFF(dk_time);
	}

	/* DISPLAY HEADER */
	atf(0, 4, "TYPE\t\t   REQUESTS    AVE SIZE\t   TOTAL BYTES\t  CACHE HITS");

	/* DATA MUST BE SHOWN AFTER FIRST ITERATION */
	if (last_cmd == 'I') {
	  number = (int) ((cnt.f_bread - old_cnt.f_bread) * delta);
	  atf(0, 2, "Cache efficiency: %3d%%",
		(number > 0)
		? (100 * (int) ((cnt.f_breadcache - old_cnt.f_breadcache) *
				delta)) / number : 0);
	  size = (int) ((cnt.f_breadsize - old_cnt.f_breadsize) * delta);
	  atf(0, 5, "Buffer reads   %12d %11d   %12d  %12d\n",number,
			(int) ((number == 0) ? 0 : (size / number)),
			size,
			(int) ((cnt.f_breadcache -
				old_cnt.f_breadcache) * delta));

	  number = (int) ((cnt.f_breada - old_cnt.f_breada) * delta);
	  size = (int) ((cnt.f_breadasize - old_cnt.f_breadasize) * delta);
	  addf("Read ahead     %12d %11d   %12d  %12d\n", number,
			(int)((number == 0) ? 0 : (size / number)),
			size,
			(int) ((cnt.f_breadacache -
				old_cnt.f_breadacache) * delta));
	  number = (int) ((cnt.f_bwrite - old_cnt.f_bwrite) * delta);
	  size = (int) ((cnt.f_bwritesize - old_cnt.f_bwritesize) * delta);
	  addf("Buffer writes  %12d %11d   %12d\n", number,
			(int) ((number == 0) ? 0 : (size / number)), size);
	  number = (int) ((cnt.f_bdwrite - old_cnt.f_bdwrite) * delta);
	  size = (int) ((cnt.f_bdwritesize - old_cnt.f_bdwritesize) * delta);
	  addf("Delayed writes %12d %11d   %12d\n", number,
			(int) ((number == 0) ? 0 : (size / number)), size);

	  /* ONLY IF A DISCLESS CLIENT */
	  if ((my_site_status & CCT_CLUSTERED) &&
	      !(my_site_status & CCT_ROOT)) {
		addf("Synchronous reads  %8d\n",
		     (int)((sync_reads - old_sync_reads) * delta));
		addf("Synchronous writes %8d\n",
			     (int)((sync_writes - old_sync_writes) * delta));
	  }

	  /* SUM NUMBER OF CPU TICKS OVER LAST INTERVAL */
	  t = 0;
	  for (i = 0; i < CPUSTATES; i++)
		t += cp_time[i] - old_cp_time[i];
	  if (!t)
		t = 1;

#ifdef hp9000s200
	  /* READ OUT LIST OF SWAP DEVICES */
	  for (i = 0; i < DK_NDRIVE; i++)
		swap_dev_size[i] = 0;
	  swp = (struct swdevt *)nl[X_SWDEVT].n_value;
	  for (j = 0; j < nswapdev; j++) {
		getvar(swp++, swapdev);
		for (i = 0; i < DK_NDRIVE; i++)
			/* IF A DRIVE IS USED FOR SWAP, STORE SIZE */
			if (minor(s.dk_devt[i]) == minor(swapdev.sw_dev))
				swap_dev_size[i] = swapdev.sw_nblks;
	  }
#endif  /* hp9000s200 */

	  /* DISPLAY PER DISC DRIVE STATISTICS */
	atf(0, 12, "KBYTE/SEC  XFERS/SEC  MILLISEC/SEEK  UTILIZATION  DRIVE\n");
	start_scrolling_here();
#ifdef hp9000s200
	  for (i = 0; i < DK_NDRIVE; i++)
#endif  /* hp9000s200 */
#ifdef __hp9000s800
	  for (i = 0; i < drives_conf; i++)
#endif  /* __hp9000s800 */
		/* ARE THE DATA STRUCTURES INITIALIZED FOR THIS DRIVE? */
		/* if (s.dk_mspw[i] != 0.0) */ {
#ifdef hp9000s200
		    /* FIND DRIVE NAME? - LOOK IN MOUNT TABLE 1ST */
		    found = 0;
		    sprintf(drive_name,"0x%06x",minor(s.dk_devt[i]));

		    /* ONLY LOOK IF NOT A DISCLESS CLIENT */
		    if (!(my_site_status & CCT_CLUSTERED) ||
		        (my_site_status & CCT_ROOT)) {
			if (s.dk_devt[i]) {
				/* LOOK FOR DEVICE NUMBER IN /etc/mnttab DATA */
				k = 0;
				while (k < nmnt) {
				    if (minor(mntl[k]->dev) ==
					minor(s.dk_devt[i])) {
					found = 1;
					strcpy(drive_name,mntl[k]->name);
					break;
				    }
				    k++;
				}

				/* FOUND NAME? */
				if (!found) {
				    /* RE-READ /etc/mnttab AND */
				    get_mount_devices();

				    /* LOOK AGAIN BEFORE GIVING UP */
				    k = 0;
				    while (k < nmnt) {
				    	if (minor(mntl[k]->dev) ==
					    minor(s.dk_devt[i])) {
					       found = 1;
					       strcpy(drive_name,mntl[k]->name);
					       break;
					}
					k++;
				    }
				}
			}
		    }
#endif  /* hp9000s200 */
#ifdef __hp9000s800
		    /* SPECTRUM DOESN'T SAVE THE MAJOR/MINOR NUMBER */
		    found = 1;
		    sprintf(drive_name,"%d",i);
#endif  /* __hp9000s800 */

		    /* DISPLAY ONLY IF DRIVE IS MOUNTED */
		    if (found
#ifdef hp9000s200
			    /* OR A SWAP DISK */
			    || swap_dev_size[i]
#endif  /* hp9000s200 */
			   ) {
			  atime = s.dk_time[i] / (float) HZ;
			  words = s.dk_wds[i] * 32.0;	/* NO OF WORDS XFERED */
			  xtime = s.dk_mspw[i] * words;	/* TRANSFER TIME */
			  itime = atime - xtime;	/* TIME NOT XFERING */
			  if (xtime < 0)
				itime += xtime, xtime = 0;
			  if (itime < 0)
				xtime += itime, itime = 0;
			  addf("%9.0f  %9.0f      %9.1f   %9d%%  ",
				  words/512*delta, s.dk_xfer[i]*delta,
				  s.dk_seek[i] ? itime*1000./s.dk_seek[i] : 0.0,
				  (s.dk_time[i] * 100) / t);

			  drive_name[29] = '\0';  /* Force to < 29 char */
#ifdef hp9000s200
			  if (swap_dev_size[i]) {
				  drive_name[15] = '\0';  /* Force < 15 char */
				  addf("%s (%dMB swap)\n",
				       drive_name, swap_dev_size[i]  >> 10);
			  } else
#endif  /* hp9000s200 */
				  addf("%s\n",drive_name);
		    }
		}
	}

	/* SAVE CURRENT VALUES FOR NEXT ITERATION */
	old_sync_reads = sync_reads;
	old_sync_writes = sync_writes;
	old_cnt = cnt;

	for (i = 0; i < CPUSTATES; i++)
	   old_cp_time[i] = cp_time[i];

} /* mode_i */

/**********************************************************************/

mode_i_sn_init()

{
#ifdef hp9000s800
	register int i;

	bufpages = getvalue(nl[X_BUFPAGES].n_value);

	s.dk_ndrive1 = getvalue(nl[X_DK_NDRIVE1].n_value);
	s.dk_ndrive2 = getvalue(nl[X_DK_NDRIVE2].n_value);
	s.dk_ndrive3 = getvalue(nl[X_DK_NDRIVE3].n_value);
	/* CIO HP-IB + NIO HP-IB + A-LINK + SCSI? DISC COUNT */
	drives_conf = s.dk_ndrive + s.dk_ndrive1 + s.dk_ndrive2 + s.dk_ndrive3;

	swap_dev_size = (int *) sbrk(drives_conf * sizeof(int));
        for (i = 0; i < drives_conf; i++)
        {
                s1.dk_time[i] = 0;
                s1.dk_wds[i] = 0;
                s1.dk_seek[i] = 0;
                s1.dk_xfer[i] = 0;
        }

#endif  /* hp9000s800 */
} /* mode_i_sn_init */


/**********************************************************************/

#ifdef SNAKESNOTYET
mode_i_sn()
{
#ifdef hp9000s800
	register int i, j, k;
	int t, found;
	float delta;
	unsigned number, size;
	double atime, words, xtime, itime;
	char drive_name[256];
	float *dkmspw_ptr;
	long *dktime_ptr, *dkxfer_ptr, *dkwds_ptr, dkseek_ptr;
	struct swdevt *swp, *swphead, swapdev;
	int sync_reads, sync_writes;

	/* READ OUT DISCLESS STATUS */
	my_site_status = getvalue(nl[X_MY_SITE_STATUS].n_value);

	/* READ OUT DISCLESS SYNCHRONOUS ACTIVITY */
	if ((my_site_status & CCT_CLUSTERED) && !(my_site_status & CCT_ROOT)) {
		/* FETCH OUTBOUND DISCLESS MESSAGE OPCODE STATISTICS */
		if (dskless_stats(STATS_READ_OUTBOUND_OPCODES,
				  &outbound_opcode_stats,NULL,NULL) < 0) {
			atf(0, 9, "Can't read outbound discless opcode statistics from kernel");
			my_site_status = 0;
		} else {
			sync_reads = outbound_opcode_stats.
					 opcode_stats[DMSYNCSTRAT_READ];
			sync_writes = outbound_opcode_stats.
					 opcode_stats[DMSYNCSTRAT_WRITE];
		}
	}


	/* DISPLAY THE CACHE BUFFER SIZE */
	atf(42,2, "Pages in buffer pool: %5d",bufpages);

	/* TIME DELTA SINCE LAST ITERATION */
	delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
		       ((current_time.tv_usec - previous_time.tv_usec)
		       / 1000000.0));

	/* READ OUT /etc/mnttab IF NOT DONE YET */
	if (!mount_devices_gotten) {
		get_mount_devices();
		mount_devices_gotten = 1;
	}

	/* READ OUT THE VMMETER STRUCTURE */
	getvar(nl[X_SUM].n_value, cnt);

	/* READ OUT THE CPU UTILIZATION STRUCTURE (FOR DISC UTILIZATION) */
	pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);
	cp_time = pst_dynamic_info.psd_cpu_time;

	/* READ OUT PER DISC STATISTICS */
	get(nl[X_DK_TIME].n_value, s.dk_time, sizeof s.dk_time);
	get(nl[X_DK_XFER].n_value, s.dk_xfer, sizeof s.dk_xfer);
	get(nl[X_DK_WDS].n_value, s.dk_wds, sizeof s.dk_wds);
	get(nl[X_DK_SEEK].n_value, s.dk_seek, sizeof s.dk_seek);
	get(nl[X_DK_MSPW].n_value, s.dk_mspw, sizeof s.dk_mspw);

	if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { /* if Snakes */
	    if (nlist(hpux_file,nl_snio) != 0) {
	 	perror("nlist");
		fprintf(stderr,"Can't do an nlist on %s\n",
			hpux_file);
		exit(-1);
	    }
 	    get(nl_snio[X_DK_DEVT].n_value, s.dk_devt, sizeof s.dk_devt); 
	}

	/* SUBTRACT PREVIOUS VALUE (s1) FROM s THEN SAVE NEXT PREVIOUS VALUE */
	for (i = 0; i < drives_conf; i++) {
		XDIFF(dk_xfer); XDIFF(dk_seek); XDIFF(dk_wds); XDIFF(dk_time);
	}

	/* DISPLAY HEADER */
	atf(0, 4, "TYPE\t\t   REQUESTS    AVE SIZE\t   TOTAL BYTES\t  CACHE HITS");

	/* DATA MUST BE SHOWN AFTER FIRST ITERATION */
	if (last_cmd == 'I') {
	  number = (int) ((cnt.f_bread - old_cnt.f_bread) * delta);
	  atf(0, 2, "Cache efficiency: %3d%%",
		(number > 0)
		? (100 * (int) ((cnt.f_breadcache - old_cnt.f_breadcache) *
				delta)) / number : 0);
	  size = (int) ((cnt.f_breadsize - old_cnt.f_breadsize) * delta);
	  atf(0, 5, "Buffer reads   %12d %11d   %12d  %12d\n",number,
			(int) ((number == 0) ? 0 : (size / number)),
			size,
			(int) ((cnt.f_breadcache -
				old_cnt.f_breadcache) * delta));

	  number = (int) ((cnt.f_breada - old_cnt.f_breada) * delta);
	  size = (int) ((cnt.f_breadasize - old_cnt.f_breadasize) * delta);
	  addf("Read ahead     %12d %11d   %12d  %12d\n", number,
			(int)((number == 0) ? 0 : (size / number)),
			size,
			(int) ((cnt.f_breadacache -
				old_cnt.f_breadacache) * delta));
	  number = (int) ((cnt.f_bwrite - old_cnt.f_bwrite) * delta);
	  size = (int) ((cnt.f_bwritesize - old_cnt.f_bwritesize) * delta);
	  addf("Buffer writes  %12d %11d   %12d\n", number,
			(int) ((number == 0) ? 0 : (size / number)), size);
	  number = (int) ((cnt.f_bdwrite - old_cnt.f_bdwrite) * delta);
	  size = (int) ((cnt.f_bdwritesize - old_cnt.f_bdwritesize) * delta);
	  addf("Delayed writes %12d %11d   %12d\n", number,
			(int) ((number == 0) ? 0 : (size / number)), size);

	  /* ONLY IF A DISCLESS CLIENT */
	  if ((my_site_status & CCT_CLUSTERED) &&
	      !(my_site_status & CCT_ROOT)) {
		addf("Synchronous reads  %8d\n",
		     (int)((sync_reads - old_sync_reads) * delta));
		addf("Synchronous writes %8d\n",
			     (int)((sync_writes - old_sync_writes) * delta));
	  }

	  /* SUM NUMBER OF CPU TICKS OVER LAST INTERVAL */
	  t = 0;
	  for (i = 0; i < CPUSTATES; i++)
		t += cp_time[i] - old_cp_time[i];
	  if (!t)
		t = 1;

	  /* READ OUT LIST OF SWAP DEVICES */
	  for (i = 0; i < drives_conf; i++)
		swap_dev_size[i] = 0;
	  swp = (struct swdevt *)nl[X_SWDEVT].n_value;
	  for (j = 0; j < nswapdev; j++) {
		getvar(swp++, swapdev);
		for (i = 0; i < drives_conf; i++)
			/* IF A DRIVE IS USED FOR SWAP, STORE SIZE */
		    if (swapdev.sw_dev && swapdev.sw_nblks) {
			swap_dev_size[i] += swapdev.sw_enable > 0 ? swapdev.sw_nblks:0;
		    } else { /* SWAP_POSSIBLE */
			swap_dev_size[i] += swapdev.sw_nblks;
		    }
	  } 

	  /* DISPLAY PER DISC DRIVE STATISTICS */
	atf(0, 12, "KBYTE/SEC  XFERS/SEC  MILLISEC/SEEK  UTILIZATION  DRIVE\n");
	start_scrolling_here();

	for (i = 0; i < drives_conf; i++) 
		/* ARE THE DATA STRUCTURES INITIALIZED FOR THIS DRIVE? */
	{ 
	    if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { /* if Snakes */
		    /* FIND DRIVE NAME? - LOOK IN MOUNT TABLE 1ST */
		    found = 0;
		    sprintf(drive_name,"0x%06x",minor(s.dk_devt[i]));

		    /* ONLY LOOK IF NOT A DISCLESS CLIENT */
		    if (!(my_site_status & CCT_CLUSTERED) ||
		        (my_site_status & CCT_ROOT)) {
			if (s.dk_devt[i]) {
				/* LOOK FOR DEVICE NUMBER IN /etc/mnttab DATA */
				k = 0;
				while (k < nmnt) {
				    if (minor(mntl[k]->dev) ==
					minor(s.dk_devt[i])) {
					found = 1;
					strcpy(drive_name,mntl[k]->name);
					break;
				    }
				    k++;
				}

				/* FOUND NAME? */
				if (!found) {
				    /* RE-READ /etc/mnttab AND */
				    get_mount_devices();

				    /* LOOK AGAIN BEFORE GIVING UP */
				    k = 0;
				    while (k < nmnt) {
				    	if (minor(mntl[k]->dev) ==
					    minor(s.dk_devt[i])) {
					       found = 1;
					       strcpy(drive_name,mntl[k]->name);
					       break;
					}
					k++;
				    }
				}
			}
		    }
		} /* if Snakes */
	    } /* for */
	}


	if (found || swap_dev_size[i]) {
		  atime = s.dk_time[i] / (float) HZ;
		  words = s.dk_wds[i] * 32.0;	/* NO OF WORDS XFERED */
		  xtime = s.dk_mspw[i] * words;	/* TRANSFER TIME */
		  itime = atime - xtime;	/* TIME NOT XFERING */
		  if (xtime < 0)
			itime += xtime, xtime = 0;
		  if (itime < 0)
			xtime += itime, itime = 0;
		  addf("%9.0f  %9.0f      %9.1f   %9d%%  ",
			  words/512*delta, s.dk_xfer[i]*delta,
			  s.dk_seek[i] ? itime*1000./s.dk_seek[i] : 0.0,
			  (s.dk_time[i] * 100) / t);

		  drive_name[29] = '\0';  /* Force to < 29 char */

		  if (swap_dev_size[i]) {
			  drive_name[15] = '\0';  /* Force < 15 char */
			  addf("%s (%dMB swap)\n",
			       drive_name, swap_dev_size[i] >> 10);
		  } else
			  addf("%s\n",drive_name);
	}

	/* SAVE CURRENT VALUES FOR NEXT ITERATION */
	old_sync_reads = sync_reads;
	old_sync_writes = sync_writes;
	old_cnt = cnt;
	for (i = 0; i < CPUSTATES; i++)
		old_cp_time[i] = cp_time[i];

#endif  /* hp9000s800 */
} /* mode_i_sn */
#endif  /* SNAKESNOTYET */


/**********************************************************************/

get_mount_devices()

{
#define MNTL_GROW	10
	static int 	mntl_size = 0;
	struct mntent	*entry;
	FILE		*mnttabf;
	struct stat	sbuf;
	dev_t		device;
	int		i;

	/* HAS /etc/mnttab CHANGED SINCE LAST ACCESS? */
	if (!stat("/etc/mnttab", &sbuf)) {
		if (mnttab_time == sbuf.st_mtime)
			/* NO, SO CURRENT TABLE IS FINE */
			return;
	}

	/*
	 * Free up space previously allocated for path names, we
	 * don't free the mntl structs since we can re-use them
	 * easily.
	 */
	for (i = 0; i < nmnt; i++) {
		free(mntl[i]->name);
		mntl[i]->name = NULL;
	}

	/* OPEN /etc/mnttab AND READ NAMES AND DEVICE MINOR NUMBERS */
	nmnt = 0;
	mnttabf = setmntent("/etc/mnttab","r");

	/* SCAN ALL MOUNTED DEVICES AND SAVE VALUES */
	while (entry = getmntent(mnttabf)) {
		if (strcmp(MNTTYPE_NFS,entry->mnt_type) == 0 ||
		    strcmp(MNTTYPE_IGNORE,entry->mnt_type) == 0)
			continue; /* skip NFS and IGNORE entries */

		/*
		 * Stat the device file, if the stat fails, then
		 * just ignore this entry.
		 */
		{
			char dev_name[MAXPATHLEN];
			struct stat st;

			if (entry->mnt_fsname[0] != '/')
				strcpy(dev_name,"/dev/");
			else
				dev_name[0] = '\0';
			strcat(dev_name, entry->mnt_fsname);

			/*
			 * Get major/minor numbers for matching later
			 * in I screen
			 */
			if (stat(dev_name, &st) < 0)
				continue;
			device = st.st_rdev;
		}

		if (mntl_size == 0) {
			mntl_size = MNTL_GROW;
			mntl = (struct mntl **) malloc(mntl_size * sizeof (struct mntl *));
			if (mntl != (struct mntl **)0)
				for (i = 0; i < mntl_size; i++)
					mntl[i] = (struct mntl *)0;
		}
		else
		    if (nmnt >= mntl_size) {
			    int old_size = mntl_size;

			    mntl_size += MNTL_GROW;
			    mntl = (struct mntl **)realloc(mntl,
				       mntl_size * sizeof (struct mntl *));
			if (mntl != (struct mntl **)0)
				for (i = old_size; i < mntl_size; i++)
					mntl[i] = (struct mntl *)0;
		    }

		if (mntl == (struct mntl **)0) {
			fprintf(stderr, "get_mount_devices: malloc of %d bytes for mount array failed\n",
			    mntl_size * sizeof (struct mntl *));
			return;
		}

		/*
		 * Allocate a new mntl struct if necessary.
		 */
		if (mntl[nmnt] == (struct mntl *)0) {
			mntl[nmnt] = (struct mntl *)
					  malloc(sizeof (struct mntl));
			if (mntl[nmnt] == (struct mntl *)0) {
			    fprintf(stderr, "get_mount_devices: malloc of %d bytes for mount entry failed\n",
			    sizeof (struct mntl));
			    return;
			}
		}

		/*
		 * Get the filesystem name.  Use "swap" for MNTTYPE_SWAP
		 */
		if (strcmp(MNTTYPE_SWAP,entry->mnt_type) == 0)
			i = sizeof MNTTYPE_SWAP + 1;
		else
			i = strlen(entry->mnt_dir) + 1;

		if ((mntl[nmnt]->name = (char *)malloc(i)) == NULL) {
			fprintf(stderr, "get_mount_devices: malloc of %d bytes for mount path failed\n", i);
			return;
		}

		if (strcmp(MNTTYPE_SWAP,entry->mnt_type) == 0)
			strcpy(mntl[nmnt]->name, MNTTYPE_SWAP);
		else
			strcpy(mntl[nmnt]->name, entry->mnt_dir);

		mntl[nmnt]->dev = device;
		nmnt++;
	}
	/* REMEMBER WHEN WE LAST READ IT */
	mnttab_time = sbuf.st_mtime;
	endmntent(mnttabf);

} /* get_mount_devices */

/**********************************************************************/

mode_k_init()

{
    if (getvalue(nl[X_DSKLESS_INITIALIZED].n_value)) {
	/* READ OUT NCSP */
	getvar(nl[X_NCSP].n_value, ncsp);

	/* ALLOCATE SPACE FOR NSP ARRAY */
	if ((int) (nsp = (struct nsp *) calloc(ncsp,
					       sizeof(struct nsp))) == NULL) {
		perror("mode_k_init calloc");
		abnorm_exit(-1);
	}
    }

} /* mode_k_init */



mode_k()
{
	register int i, j;
	int found, cid, kline, first_line_k;
	int cleanup_running, failed_sites, retrysites;
	int allocated[MAXSITE];
	int swfree[MAXSITE];
	char *cp_type, *cp_swap;
	char flag;
	char numberstr[16];
	cnode_t	myid = -1;
	struct stat sbuf;
	struct csp_stats csp_stats;
	struct cct clustab[MAXSITE];
	struct swaptab *sp, swapent;
	int swap_avail;

	/* ARE WE CLUSTERED? */
	if (cnodes(0) <= 0) {
		atf(0, 2, "This system is not clustered");
		return;
	}

	/* DISPLAY CSP STATISTICS */
	getvar(nl[X_CSP_STATS].n_value, csp_stats);
	atf(0, 2, "Limited CSP queue length: %3d (%2d max)",
			     csp_stats.limitedq_curlen,
			     csp_stats.limitedq_maxlen);
	atf(0, 3,"Limited CSP requests:%8d (%6.3f second maximum service time)",
		     csp_stats.requests[CSPSTAT_LIMITED],
		     (float) csp_stats.max_lim_time / (float) HZ);
	if ((nl[X_NUM_NSPS].n_value == 0) || (nl[X_FREE_NSPS].n_value == 0)) {
		fprintf(stderr,"num_nsps or freensps not in nlist\n");
		abnorm_exit(-1);
	}
	atf(0, 4,
"General CSP queue length: %3d (%2d max)  %d active  %d idle (%d minimum idle)",
			     csp_stats.generalq_curlen,
			     csp_stats.generalq_maxlen,
			     getvalue(nl[X_NUM_NSPS].n_value),
			     getvalue(nl[X_FREE_NSPS].n_value),
			     csp_stats.min_gen_free);
	atf(0, 5, "GCSP requests by timeout:%10d short %9d medium %9d long",
			     csp_stats.requests[CSPSTAT_SHRT],
			     csp_stats.requests[CSPSTAT_MED],
			     csp_stats.requests[CSPSTAT_LONG]);
	atf(0, 6, "GCSP timeouts:           %10d short %9d medium %9d long",
			     csp_stats.timeouts[CSPSTAT_SHRT],
			     csp_stats.timeouts[CSPSTAT_MED],
			     csp_stats.timeouts[CSPSTAT_LONG]);

	/* DISPLAY CRASH DETECTION STUFF IF IN OS DESIGNER'S MODE */
	if (osdesignermode) {
		if ((nl[X_CLEANUP_RUNNING].n_value == 0) ||
		    (nl[X_FAILED_SITES].n_value == 0) ||
		    (nl[X_RETRYSITES].n_value == 0)) {
			fprintf(stderr,
		      "cleanup_running/failed_sites/retrysites not in nlist\n");
			abnorm_exit(-1);
		}
		cleanup_running = getvalue(nl[X_CLEANUP_RUNNING].n_value),
		failed_sites = getvalue(nl[X_FAILED_SITES].n_value),
		retrysites = getvalue(nl[X_RETRYSITES].n_value),
		atf(0, 8,
"CRASH DETECTION STATE:  cleanup_running: %1d  failed_sites: %2d  retrysites: %2d",
				     cleanup_running,failed_sites,retrysites);
		first_line_k = 12;
	} else
		first_line_k = 10;

	/* HAS /etc/clusterconf CHANGED SINCE LAST ACCESS? */
	if (!stat(CLUSTERCONF, &sbuf)) {
		if (cct_time != sbuf.st_mtime)
			/* YES, RE-READ TABLE */
			cct_ok = read_cct();
	}

	/* DO WE HAVE A GOOD TABLE? */
	if (!cct_ok) {
		atf(0, 8, "%s is bad", CLUSTERCONF);
		return;
	}

	/* WHICH ID IS MINE? */
	myid = cnodeid();
	if (myid < 0) {
		atf(0, 8, "Can't read my cnode ID via cnodeid(2)");
		return;
	}

	/* READ OUT CLUSTAB */
	get(nl[X_CLUSTAB].n_value,clustab, sizeof(clustab));

	/* IF WE ARE THE ROOT SERVER, FIGURE OUT SWAP SPACE PER CNODE */
	if (myid == rootid) {
		for (i = 0; i < MAXSITE; i++) {
			allocated[i] = 0;
			swfree[i] = 0;
		}
		swap_avail = swapsize (SW_DEV_ENAB);

		sp = swaptable;
		do {
		    getvar(sp++, swapent);
			/* local swap */
		    if (swapent.st_site == -1) { /* nonallocated avail. swap */
                           allocated[rootid] += NPGCHUNK * (NBPG >> 10);
                    } else {  /* allocated to server and client */
                           allocated[swapent.st_site] += NPGCHUNK *(NBPG >> 10);
                    }
		} while (sp < swapMAXSWAPTAB);
	}

	/* READ OUT NSP ARRAY */
	get(nl[X_NSP].n_value,nsp,ncsp * sizeof(struct nsp));

	/* ACCOUNT FOR CSP ACTIVITY PER SITE */
	for (i = 0; i < cct_count; i++) {
		/* ZERO THE ACTIVITY COUNTERS */
		cct[i].csps_active = 0;
	}

	for (i = 0; i < ncsp; i++) {
		/* IS THIS NSP ACTIVE? */
		if (nsp[i].nsp_site) {
			/* YES, LOOK FOR THAT SITE */
			found = 0;
			for (j = 0; j < cct_count; j++) {
				if (cct[j].cnode_id == nsp[i].nsp_site) {
					/* GOT IT! */
					cct[j].csps_active++;
					found = 1;
					break;
				}
			}
			if (!found) {
				/* DON'T ABORT, BUT LET THE USER KNOW */
			   printf("Can't find cnode_id: %d\n", nsp[i].nsp_site);
			}
		}
	}

	/* DISPLAY VALUES IN /etc/clusterconf */
	if (myid == rootid) {
	    atf(0, first_line_k-2,
	"     CNODE             CSPs      SWAP   CSPs ACTIVE    SWAP SPACE\n");
	    addf(
	"      NAME  STATUS     ON CNODE  SITE   SERVING CNODE  ALLOCATED (KB)\n");
	} else {
	    /* SWAP SPACE ALLOCATION INFO ONLY AVAILABLE ON ROOT SERVER */
	    atf(0, first_line_k-2,
		    "     CNODE             CSPs      SWAP   CSPs ACTIVE\n");
	    addf("      NAME  STATUS     ON CNODE  SITE   SERVING CNODE\n");
	}
	start_scrolling_here();
	kline = 0;
	for (i = 0; i < cct_count; i++) {
	    /* DISPLAY THIS LINE? */
		/* DON'T DISPLAY INACTIVE SITES */
		cid = cct[i].cnode_id;
		if (clustab[cid].status == CL_INACTIVE)
			continue;

		/* DETERMINE STATUS */
		if (cid == rootid)
			cp_type = "Root";
		else if (clustab[cid].status == CL_ACTIVE)
			cp_type = "Active";
		else if (clustab[cid].status == CL_ALIVE)
			cp_type = "Alive";
		else if (clustab[cid].status == CL_CLEANUP)
			cp_type = "Cleanup";
		else if (clustab[cid].status == CL_FAILED)
			cp_type = "Failed";
		else if (clustab[cid].status == CL_RETRY)
			cp_type = "Retrying";
		else {
			sprintf(numberstr,"0x%x",clustab[cid].status);
			cp_type = numberstr;
		}

		/* FLAG MY SITE */
		if (cid == myid)
		    flag = '*';
		else
		    flag = ' ';
    
		/* SWAP EITHER LOCAL OR ROOT? */
		if (cct[i].swap_serving_cnode == cid)
		    cp_swap = "Local";
		else
		    cp_swap = "Root";
    
		/* DISPLAY LINE */
		if (myid == rootid)
		    addf("%c %8s  %-9s  %-3d       %-5s  %-3d            %-6d\n",
			         flag,cct[i].cnode_name,cp_type,
			         cct[i].kcsp,cp_swap,cct[i].csps_active,
			         allocated[cct[i].cnode_id]);
		else
		    addf("%c %8s  %-9s  %-3d       %-5s  %-3d\n",
			         flag,cct[i].cnode_name,cp_type,
			         cct[i].kcsp,cp_swap,cct[i].csps_active);
	}
} /* mode_k */

/**********************************************************************/

int read_cct()

{
	register int i;
	register struct cct_entry *entry;
	struct stat sbuf;

	/* READ /etc/clusterconf INTO LOCAL TABLE */
	cct_count = 0;
	for (i = 0; i <= MAXCNODE; i++)
		/* CLEAR OUT ENTRIES */
		cct[i].cnode_name[0] = '\0';
	setccent();
	while (entry = getccent()) {
		/* FILL IN ENTRY */
		cct_count++;
		i = entry->cnode_id;
		if ((i > MAXCNODE) || (i < 0))
			/* BAD CNODE ID */
			return(0);
		strcpy(cct[i].cnode_name,entry->cnode_name);
		cct[i].cnode_id = i;
		cct[i].swap_serving_cnode = entry->swap_serving_cnode;
		if ((entry->swap_serving_cnode > MAXCNODE) ||
		    (entry->swap_serving_cnode < 0))
			/* BAD SWAP SERVING CNODE */
			return(0);
		cct[i].kcsp = entry->kcsp;

		/* REMEMBER ROOT SERVER ID */
		if (entry->cnode_type == 'r')
			rootid = entry->cnode_id;
	}
	endccent();

	/* DID WE FIND THE ROOT SERVER IN /etc/clusterconf? */
	if (rootid < 0)
		return(0);

	/* SORT CNODE TABLE BY CNODE NAME */
	qsort((char *) cct,MAXCNODE, sizeof(cct[0]),cct_compare);

	/* REMEMBER LAST MODIFICATION TIME */
	if (stat(CLUSTERCONF, &sbuf))
		/* CAN'T STAT /etc/clusterconf */
		return(0);
	else
		cct_time = sbuf.st_mtime;

	/* EVERYTHING LOOKS OK */
	return(1);

} /* read_cct */

/**********************************************************************/

cct_compare(entry1,entry2)

	struct monitor_cct_entry *entry1, *entry2;

{
	/* FORCE EMPTY ENTRIES TO END OF TABLE */
	if ((entry1->cnode_name[0] == '\0') &&
	    (entry2->cnode_name[0] != '\0'))
		return(1);
	if ((entry1->cnode_name[0] != '\0') &&
	    (entry2->cnode_name[0] == '\0'))
		return(-1);
	return(reverse * strcmp(entry1->cnode_name,entry2->cnode_name));

} /* cct_compare */

/**********************************************************************/

mode_l_init()

{
	struct	fis lan_info;

	/* OPEN LAN DEVICE FILE */
	if ((lan_fd = open(lan_name,O_RDWR)) != -1)
	{
		/* SET CLOSE ON EXEC FLAG */
		fcntl(lan_fd,F_SETFD,1);

		/* FETCH LANIC ADDRESS */
		lan_info.reqtype = LOCAL_ADDRESS;
		lan_info.vtype = 6;
		if (ioctl(lan_fd,NETSTAT, &lan_info) == -1)
			memset(lanic_address,'\0',6);
		else
			memcpy(lanic_address,lan_info.value.s,6);
	}

} /* mode_l_init */

/**********************************************************************/

mode_l()
{
	register int i;
	float	delta = 0.0;
	int	input, output = 0;
	struct	fis lan_info;

	/* LAN CARD INFORMATION */
	if (lan_fd == -1) {
		atf(0, 2, "Can not open LAN device file: %s", lan_name);
		return;
	}
	atf(0, 2, "LAN CARD:");
	atf(0, 4, "  Device file: %s", lan_name);
	atf(0, 5, "  LANIC address = 0x%02X%02X%02X%02X%02X%02X",
		     lanic_address[0], lanic_address[1], lanic_address[2],
		     lanic_address[3], lanic_address[4], lanic_address[5]);
	lan_info.reqtype = DEVICE_STATUS;
	lan_info.vtype = INTEGERTYPE;
	if (ioctl(lan_fd,NETSTAT, &lan_info) != -1) {
		atf(0, 6, "  Device status = ");
		switch (lan_info.value.i) {
		  case INACTIVE:	atf(18, 6, "INACTIVE");		break;
		  case INITIALIZING:	atf(18, 6, "INITIALIZING");	break;
		  case ACTIVE:		atf(18, 6, "ACTIVE");		break;
		  case FAILED:		atf(18, 6, "FAILED");		break;
		  default:		atf(18, 6, "?");		break;
		}
	}

	/* TRANSMIT STATISTICS */
	i = 0;
	atf(0, 8, "TRANSMIT PACKET STATISTICS:");
	while (strlen(transmit_table[i].description)) {
		lan_info.reqtype = transmit_table[i].reqtype;
		lan_info.vtype = INTEGERTYPE;
		if (ioctl (lan_fd, NETSTAT, &lan_info) != -1) {
			if (lan_info.reqtype == TX_FRAME_COUNT)
				output = lan_info.value.i;
			atf(0, 10+i, "  %s = %d",
				     transmit_table[i].description,
				     lan_info.value.i);
		}
		i++;
	}

	/* RECEIVE STATISTICS */
	i = 0;
	atf(40, 8, "RECEIVE PACKET STATISTICS:");
	while (strlen(receive_table[i].description)) {
		lan_info.reqtype = receive_table[i].reqtype;
		lan_info.vtype = INTEGERTYPE;
		if (ioctl (lan_fd, NETSTAT, &lan_info) != -1) {
			if (lan_info.reqtype == RX_FRAME_COUNT)
				input = lan_info.value.i;
			atf(40, 10+i, "  %s = %d",
				     receive_table[i].description,
				     lan_info.value.i);
		}
		i++;
	}

	/* DISPLAY RATE VALUES */
	if (last_cmd == 'L')
	{
		delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
			       ((current_time.tv_usec -
				 previous_time.tv_usec) / 1000000.0));
		atf(40, 4, "  Input packet rate =%4d per sec",
			(int) ((input - input_last) * delta));
		atf(40, 5, "  Output packet rate =%4d per sec",
			(int) ((output - output_last) * delta));
	}

	/* SAVE LAST VALUES FOR RATE CALCULATIONS */
	input_last = input;
	output_last = output;

} /* mode_l */


mode_m()
{
	register int i, k;
	int incount, outcount;
	float delta;
	struct  dux_mbstat dux_mbstat;

	/* ARE WE CLUSTERED? */
	if (cnodes(0) <= 0) {
		atf(0, 2, "This system is not clustered");
		return;
	}

	/* DISPLAY STATISTICS OR OPCODE INFORMATION? */
	/* FETCH NETBUF STATS */
	if (dskless_stats(STATS_READ_MBUF_INFO, &dux_mbstat,NULL,NULL) < 0) {
		atf(0, 2, "Can't read netbuf statistics from kernel");
		return;
	}

	/* DISPLAY NETBUF STATS */
	atf(0, 2,
	    "TYPE   TOTAL    FREE             ALLOCATIONS   FROM PAGE POOL\n");
/* 	addf("MBUF   %-4d     %3d%% (%d)\t %-8d\n",
	    dux_mbstat.m_mbufs,
	    (dux_mbstat.m_mbfree * 100) / dux_mbstat.m_mbufs,
	    dux_mbstat.m_mbfree,dux_mbstat.m_mbtotal);	   
	addf("CBUF   %-4d     %3d%% (%d)\t %-8d      %-6d\n",
	    dux_mbstat.m_clusters,
	    (dux_mbstat.m_clfree * 100) / dux_mbstat.m_clusters,
	    dux_mbstat.m_clfree,dux_mbstat.m_cltotal,
	    dux_mbstat.pagepool_clust);      **	No mbuf/cbuf in 8.0*/
	addf("FSBUF  %-4d     %3d%% (%d)\t %-8d\n",
	    dux_mbstat.m_netbufs,
	    (dux_mbstat.m_nbfree * 100) / dux_mbstat.m_netbufs,
	    dux_mbstat.m_nbfree,
	    dux_mbstat.m_nbtotal);
	addf("FSPAGE %-4d     %3d%% (%d)\t %-8d\n",
	    dux_mbstat.m_netpages,
	    (dux_mbstat.m_freepages * 100) / dux_mbstat.m_netpages,
	    dux_mbstat.m_freepages,
	    dux_mbstat.m_totalpages);

	/* FETCH PROTOCOL STATS */
	if (dskless_stats(STATS_READ_PROTOCOL_INFO,
	    &proto_stats,NULL,NULL) < 0){
		atf(0, 8, "Can't read dskless protocol statistics from kernel");
		return;
	}

	/* DISPLAY PROTOCOL STATS */
	atf(0, 6,
	    "     REQUESTS   REPLIES   ACK       SLOW-REQ   DATAGRAM   RATE\n");
	outcount = STATS_xmit_op_P_REQUEST + STATS_xmit_op_P_REPLY +
	    STATS_xmit_op_P_ACK + STATS_xmit_op_P_SLOW_REQUEST +
	    STATS_xmit_op_P_DATAGRAM;
	incount = STATS_recv_op_P_REQUEST + STATS_recv_op_P_REPLY +
	    STATS_recv_op_P_ACK + STATS_recv_op_P_SLOW_REQUEST +
	    STATS_recv_op_P_DATAGRAM;

	/* ON SECOND ITERATION DUE TO RATE CALCULATION */
	if (last_cmd == 'M') {
		/* SENT */
		delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
		    ((current_time.tv_usec -
		    previous_time.tv_usec) / 1000000.0));
		addf("Out: %8d   %-8d  %-8d  %-8d   %-8d   %d\n",
		    STATS_xmit_op_P_REQUEST,
		    STATS_xmit_op_P_REPLY,
		    STATS_xmit_op_P_ACK,
		    STATS_xmit_op_P_SLOW_REQUEST,
		    STATS_xmit_op_P_DATAGRAM,
		    (int) ((outcount - oldoutcount) * delta));

		/* RECEIVED */
		addf("In:  %8d   %-8d  %-8d  %-8d   %-8d   %d\n",
		    STATS_recv_op_P_REQUEST,
		    STATS_recv_op_P_REPLY,
		    STATS_recv_op_P_ACK,
		    STATS_recv_op_P_SLOW_REQUEST,
		    STATS_recv_op_P_DATAGRAM,
		    (int) ((incount - oldincount) * delta));
	}
	oldoutcount = outcount;
	oldincount = incount;

	/* DISPLAY DROPPED PACKET STATS */
	atf(0, 10, "DROPPED, LACK OF:   FSPAGE  FSBUF   SERVING ENTRY\n");
	addf("Request             %-6d          %-6d\n",
	    STATS_recv_no_buf,STATS_serving_entry);
	addf("Receive Datagram    %-6d  %-6d\n\n",
	    STATS_recv_dgram_no_buf,
	    STATS_recv_dgram_no_buf_hdr);

	/* DISPLAY PROTOCOL STATS */
	addf("PROTOCOL STATISTICS:\n");
	addf("Request retries:        %7d      Reply retries:          %7d\n",
		STATS_req_retries,STATS_retry_reply);
	addf("Cnode not clustered:    %7d      Unexpected message:     %7d\n",
		STATS_not_clustered,STATS_unexpected);
	addf("No avail using entry:   %7d      Req. out-of-seq:        %7d\n",
		STATS_waiting_using,STATS_recv_req_OOS);
	addf("Invalid request type:   %7d      Lost first packet:      %7d\n",
		STATS_recv_bad_flags,STATS_lost_first);
	addf("Recv not valid member:  %7d      Req not valid member:   %7d\n",
		STATS_recv_not_member,STATS_req_not_member);
	addf("Duplicate request:      %7d      Can not send (Flow-cntl):%6d\n",
		STATS_dup_req,STATS_delta_sec);
	addf("No transmit buf on card:%7d      Card HW transmit failure:%6d\n",
		STATS_xmit_no_buffer_send,STATS_xmit_hw_failure);
} /* mode_m */



mode_o()
{
	register int i;
	struct  dux_mbstat dux_mbstat;

	/* Are we clustered? */
	if (cnodes(0) <= 0) {
		atf(0, 2, "This system is not clustered");
		return;
	}

	/* FETCH OPCODE STATS */
	if (dskless_stats(STATS_READ_INBOUND_OPCODES,
	    &inbound_opcode_stats,NULL,NULL) < 0) {
		atf(0, 2, "Can't read inbound opcode statistics from kernel");
		return;
	}
	if (dskless_stats(STATS_READ_OUTBOUND_OPCODES,
	    &outbound_opcode_stats,NULL,NULL) < 0) {
		atf(0, 2, "Can't read outbound opcode statistics from kernel");
		return;
	}

	/* Display opcode stats */
	atf(0, 2, "MESSAGE\t\t   NUMBER DESCRIPTION\t\t\t       IN\tOUT\n");
	start_scrolling_here();
	for (i = 0; i < NOPCODES; i++) {
		addf("%-62s %-7d  %-7d\n",
			opcode_descrp[i],
			inbound_opcode_stats.opcode_stats[i],
			outbound_opcode_stats.opcode_stats[i]);
	}
}

mode_n()
{
	register int i, j, k, m, r;
	int cnt, temp, p[2];
	int rtime, user, system, disc, memory, swap, lan, dropped, msg, retries;
	int csp, block, syscalls, page;
	char cmdline[256], dataline[256];
	float delta = 0.0;

	/* PROMPT FOR SYSTEM NAMES IF NECESSARY */
	if (syscount == 0) {
		/* GO INTO COOKED MODE TO READ VALUES */
		if (tty_normal()) {
			fprintf(stderr,"tty_normal error\n");
			exit(-1);
		}

		/* READ SYSTEM NAMES */
		fetch_n_names();

		/* BACK TO RAW MODE */
		if (tty_raw()) {
			fprintf(stderr,"tty_raw error\n");
			abnorm_exit(-1);
		}
		clear_screen();

		/* IF NONE SPECIFIED, EXIT SCREEN */
		if (syscount == 0) {
			cmd = 'x';
			return;
		}
	}

	/* INVOKE SELF ON REMOTE SYSTEMS ON FIRST ITERATION */
	if (startup) {
		startup = 0;
		for (i = 0; i < syscount; i++) {
		    /* FORK OFF REMOTE SHELLS TO SPECIFIED SYSTEMS */
		    sprintf(cmdline,  /* CHANGE NREVISION IF CHANGED */
	"exec remsh %s exec %s -i %d -r %s rtime user system disc memory swap ",
			    systems[i].name,myname,update_interval,NREVISION);
		    strcat(cmdline,  /* CHANGE NREVISION IF CHANGED */
			   "lan dropped msg retries csp block syscalls page");
#define STDOUT_CNT 14  /* CHANGE NREVISION IF CHANGED */
#define STDOUT_LEN STDOUT_CNT * 11

		    /* GET A PIPE TO USE */
		    if (pipe(p) < 0)
			/* PIPE OPEN FAILED */
			systems[i].fd = -1;
		    else {
			/* SAVE FILE DESCRIPTOR */
			systems[i].fd = p[0];

			/* FORK CHILD */
			if ((systems[i].pid = vfork()) == 0) {
			    /* ON CHILD - CLEAN UP A BIT */
			    setuid(getuid());
			    setgid(getgid());
			    close(p[0]);

			    /* FORCE PIPE TO BE stdout */
			    if (p[1] != 1) {
				    close(1);
				    fcntl(p[1],F_DUPFD,1);
				    close(p[1]);
			    }

			    /* START UP COMMAND */
			    execl("/bin/sh","sh","-c",cmdline,0);
			    exit(1);  /* Should not get here */
			}

			/* CLOSE WRITE SIDE OF PIPE */
			close(p[1]);

			/* CHILD FORK FAIL? */
			if (systems[i].pid == -1) {
			    /* YES */
			    close(p[0]);
			    systems[i].fd = -1;
			} else
			    /* NO, SET CLOSE ON EXEC */
			    fcntl(systems[i].fd,F_SETFD,1);
		    }
		}

		/* ISSUE FIRST READ FROM EACH REMOTE SYSTEM */
		clear_screen();
		atf(0, 2,
		"Please wait while remote system communications are set up...");
		update_crt();
		for (i = 0; i < syscount; i++) {
		    if (systems[i].fd != -1) {
			/* READ AND DISCARD THE HEADER LINE */
			cnt = 0;
			while (cnt < STDOUT_LEN) {
			    m = read(systems[i].fd, &dataline[cnt],
				     STDOUT_LEN - cnt);
			    if (m <= 0) {
			        /* QUIT IF ERROR SEEN */
			        shutup(i);
				break;
			    } else
			        cnt += m;
			}

			/* SET O_NDELAY ON CONNECTION */
			if ((r = fcntl(systems[i].fd,F_GETFL,0)) == -1)
			    shutup(i);
			else {
			    r |= O_NDELAY;
			    if (fcntl(systems[i].fd,F_SETFL,r) == -1)
				shutup(i);
			}
		    }
		}

		/* SINCE WE CLEARED THE SCREEN ABOVE, RETURN TO RE-CONSTRUCT */
		return;
	}

	/* DISPLAY PER SYSTEM STATISTICS */
	atf(0, 2, "SYSTEM   USER  SYS ACTV USER SWAP LAN ------ DISKLESS ------- BLOCK   SYS PAGES\n");
	addf("NAME      CPU  CPU DISC  MEM UTIL PKT DROPPED MSG RETRIES CSP  I/Os CALLS IN+OUT\n");
#define FIRST_LINE_N 4
	start_scrolling_here();
	for (i = 0; i < syscount; i++) {
			if (systems[i].fd == -1)
			    addf("%-8s Can't reach\n", systems[i].name);
			else if (systems[i].olditeration == -2)
			    addf("%-8s Out of sync\n", systems[i].name);
			else {
			    /* READ NEXT LINE FROM REMOTE SYSTEM */
			    j = 0;
			    while ((cnt = read(systems[i].fd,
					       dataline,
					       STDOUT_LEN)) == STDOUT_LEN)
				/* MAY READ SEVERAL IF OTHER SYSTEM IS AHEAD */
				j++;

			    /* ERROR ON READ? */
			    if (cnt == -1) {
				addf("%-8s Can't read data from\n",
					     systems[i].name);
				continue;
			    }

			    /* NOTHING READ? */
			    if ((j == 0) && (cnt == 0)) {
				if ((iteration - systems[i].olditeration) >= 5)
				    /* NOTHING READ FOR A WHILE */
				    addf("%-8s No new data for %d iterations\n",
					         systems[i].name,
					         iteration -
						 systems[i].olditeration);
				else
				    /* DISPLAY LAST DATA */
				    addf(
	"%-8s %3d%% %3d%% %3d%% %3d%% %3d%% %3d %7d %3d %7d %3d %5d %5d %-5d\n",
						systems[i].name,
						systems[i].olduser,
						systems[i].oldsystem,
						systems[i].olddisc,
						systems[i].oldmemory,
						systems[i].oldswap,
						systems[i].lanrate,
						systems[i].droppedrate,
						systems[i].msgrate,
						systems[i].retriesrate,
						systems[i].csprate,
						systems[i].blockrate,
						systems[i].syscallsrate,
						systems[i].pagerate);
			    } else {
				/* DID WE GET A PARTIAL LINE? */
				if ((cnt > 0) && (cnt < STDOUT_LEN)) {
				    /* READ THE REST WITHOUT O_NDELAY */
				    if ((r = fcntl(systems[i].fd,
						   F_GETFL,0)) == -1) {
					fprintf(stderr,"fcntl F_GETFL error\n");
					abnorm_exit(-1);
				    }
				    r &= ~O_NDELAY;
				    if (fcntl(systems[i].fd,F_SETFL,r) == -1) {
					fprintf(stderr,"fcntl F_SETFL error\n");
					abnorm_exit(-1);
				    }
				    while (cnt < STDOUT_LEN) {
					m = read(systems[i].fd, &dataline[cnt],
						 STDOUT_LEN - cnt);
					if (m <= 0) {
					    /* QUIT IF ERROR SEEN */
					    shutup(i);
					    continue;
					} else
					    cnt += m;
				    }

				    /* SET O_NDELAY AGAIN */
				    if ((r = fcntl(systems[i].fd,
						   F_GETFL,0)) == -1) {
					fprintf(stderr,"fcntl F_GETFL error\n");
					abnorm_exit(-1);
				    }
				    r |= O_NDELAY;
				    if (fcntl(systems[i].fd,F_SETFL,r) == -1) {
					fprintf(stderr,"fcntl F_SETFL error\n");
					abnorm_exit(-1);
				    }
				}

				/* READ DATA VALUES OUT OF THE STRING */
				cnt = sscanf(dataline,
				/* CHANGE NREVISION IF FORMAT IS CHANGED */
"%10d %3d%%       %3d%%       %3d%%       %3d%%       %3d%%       %10d %10d %10d %10d %10d %10d %10d %10d",
					     &rtime, &user, &system, &disc,
					     &memory, &swap, &lan, &dropped, &msg,
					     &retries, &csp, &block, &syscalls,
					     &page);

				/* DID WE GET EVERYTHING? */
				if (cnt != STDOUT_CNT) {
				    /* OUT OF SYNC */
				    systems[i].olditeration = -2;
				    continue;
				}

				if (systems[i].olditeration != -1) {
				    /* ONLY AFTER FIRST ITERATION */
				    delta = 1000.0 / (float) (rtime -
							   systems[i].oldrtime);
				    systems[i].lanrate = (int) ((float) (lan -
							 systems[i].oldlan) *
							 delta);
				    systems[i].droppedrate = (int) ((float)
								    (dropped -
							systems[i].olddropped) *
							 delta);
				    systems[i].msgrate = (int) ((float) (msg -
							 systems[i].oldmsg) *
							 delta);
				    systems[i].retriesrate = (int) ((float)
								    (retries -
							systems[i].oldretries) *
							 delta);
				    systems[i].csprate = (int) ((float) (csp -
							 systems[i].oldcsp) *
							 delta);
				    systems[i].blockrate = (int) ((float)
								    (block -
							 systems[i].oldblock) *
							 delta);
				    systems[i].syscallsrate = (int) ((float)
								    (syscalls -
						       systems[i].oldsyscalls) *
							 delta);
				    systems[i].pagerate = (int) ((float) (page -
							 systems[i].oldpage) *
							 delta);
				    addf(
	"%-8s %3d%% %3d%% %3d%% %3d%% %3d%% %3d %7d %3d %7d %3d %5d %5d %-5d\n",
						systems[i].name,
						user,system,disc,memory,swap,
						systems[i].lanrate,
						systems[i].droppedrate,
						systems[i].msgrate,
						systems[i].retriesrate,
						systems[i].csprate,
						systems[i].blockrate,
						systems[i].syscallsrate,
						systems[i].pagerate);
				}

				/* SAVE VALUES FOR NEXT ITERATION */
				systems[i].olditeration = iteration;
				systems[i].oldrtime = rtime;
				systems[i].olduser = user;
				systems[i].oldsystem = system;
				systems[i].olddisc = disc;
				systems[i].oldmemory = memory;
				systems[i].oldswap = swap;
				systems[i].oldlan = lan;
				systems[i].olddropped = dropped;
				systems[i].oldmsg = msg;
				systems[i].oldretries = retries;
				systems[i].oldcsp = csp;
				systems[i].oldblock = block;
				systems[i].oldsyscalls = syscalls;
				systems[i].oldpage = page;
			    }
			}
	}
} /* mode_n */

/**********************************************************************/

kill_remshs()

{
	register int i;

	/* CLOSE DOWN EXISTING REMOTE SHELLS */
	for (i = 0; i < syscount; i++)
		/* FOR ALL SUCCESSFULLY STARTED CHILDREN, SHUT THEM DOWN */
		shutup(i);

	/* CLEAR VARIABLES */
	syscount = startup = 0;

} /* kill_remshs */

/**********************************************************************/

shutup(i)

	int i;

{
	register int r;

	/* IS CHILD RUNNING? */
	if (systems[i].fd != -1) {
		/* KILL THE CHILD */
		kill(systems[i].pid,SIGKILL);

		/* CLOSE THE PIPE */
		close(systems[i].fd);

		/* REAP CHILD */
		while ((r = wait(0)) != systems[i].pid)
			if (r == -1)
				/* ERROR */
				break;
		
		/* MARK AS UNUSED */
		systems[i].fd = -1;
	}

} /* shutup */

/**********************************************************************/

fetch_n_names()

{
	register int i, savecount, read_cnt;
	char read_str[SYSTEMLEN], read_name[SYSTEMLEN];
	int active_cnodes, have_cnodes;
	struct stat sbuf;
	cnode_t cnodetable[MAXCNODE];
	char cnodenames[MAXCNODE];

	/* FETCH LIST OF ACTIVE CNODES */
	active_cnodes = have_cnodes = 0;
	if (cnodes(cnodetable) != -1) {
		/* WHO'S ALIVE? */
		while (cnodetable[active_cnodes])
			active_cnodes++;

		/* HAS /etc/clusterconf CHANGED SINCE LAST ACCESS? */
		if (!stat(CLUSTERCONF, &sbuf)) {
			if (cct_time != sbuf.st_mtime)
				/* YES, RE-READ TABLE */
				cct_ok = read_cct();

		/* HAVE WE SOME NAMES TO CHOOSE FROM? */
		have_cnodes = cct_ok;
		}
	}

	/* SAVE COUNT OF CURRENTLY DEFINED NAMES */
	savecount = syscount;

	/* SHUTDOWN EXISTING PROCESSES */
	if (syscount)
		kill_remshs(); /* Zeros syscount */

	/* FETCH NAMES */
	while (1) {
		/* START WITH A CLEAR SCREEN */
		clear_screen();
		for (i = 0; i < syscount; i++) {
			/* DISPLAY SELECTED SYSTEMS IN MULTIPLE COLUMNS */
			addf((SYSTEMLEN + 2) * (i / (rows - 2)),
				(i % (rows - 2)) + 2, "%s",systems[i].name);
		}
		if (syscount)
			/* SO CURSOR IS AFTER PROMPT NOT END OF LIST */
			update_crt();

		/* PROMPT FOR NAME */
		if (syscount < savecount) {
			/* PRESENT PREVIOUS LIST ENTRIES AS DEFAULTS */
			strcpy(read_name,systems[syscount].name);
			atf(0,0,
		"Enter remote system for N screen (. to exit) [%s]: ",
				read_name);
		} else if (have_cnodes && (savecount == 0) &&
			   (syscount < active_cnodes)) {
			/* PRESENT ACTIVE CNODES AS DEFAULTS */
			for (i = 0; i < cct_count; i++)
				/* FIND SYSTEM NAME GIVEN CNODE ID */
				if (cnodetable[syscount] == cct[i].cnode_id)
					break;
			strcpy(read_name,cct[i].cnode_name);
			atf(0,0,
	"Enter remote system for N screen (. to exit) [active cnode: %s]: ",
				read_name);
		} else {
			read_name[0] = '.';
			atf(0,0, "Enter remote system for N screen (RETURN to exit):");
		}
		update_crt();

		/* FETCH RESPONSE */
		read_cnt = read(fdin,read_str, sizeof(read_str));

		/* GOT ANYTHING? */
		if (read_cnt > 1) {
			/* YES */
			for (i = 0; i < sizeof(read_str); i++)
				if (read_str[i] == '\n') {
					/* STRIP TERMINATING LF */
					read_str[i] = '\0';
					break;
				}
			strcpy(read_name,read_str);
		}

		/* DONE? */
		if ((syscount >= MAXSYSTEMS) || (read_name[0] == '.'))
			/* LAST ONE */
			return;

		/* STORE NAME AWAY / INITIALIZE */
		strcpy(systems[syscount].name,read_name);
		systems[syscount].lanrate = 0;
		systems[syscount].fd = systems[syscount].olditeration = -1;
		syscount++;

		/* FORCE INVOCATION OF REMOTE MONITOR PROCESSES */
		startup = 1;
	}

} /* fetch_n_names */

/**********************************************************************/

mode_r()
{
	DIR *dirp;
	struct direct *dp;
	int f, i, k, cc;
	char buf[sizeof(struct whod)];
	register struct hs *hsp = hs;
	register struct whod *wd;
	register struct whoent *we;

	/* DISPLAY HEADER */
	atf(0,2,"HOSTNAME  STATUS     TIME   USERS  IDLE      LOAD AVERAGES\n");

	/* MOVE INTO /usr/spool/rwho TO LOOK FOR DATA TO DISPLAY */
	if (chdir(RWHODIR) < 0) {
		atf(0, 4, "Can't find %s directory",RWHODIR);
		atf(0, 6, "You probably aren't running /etc/rwhod");
		return;
	}

	/* SCAN ALL DIRECTORY ENTRIES BEGINNING WITH "whod." */
	if ((dirp = opendir(".")) == NULL) {
		atf(0, 4, "Can't open %s directory",RWHODIR);
		atf(0, 6, "You probably aren't running /etc/rwhod");
		return;
	}
	nhosts = 0;
	while (dp = readdir(dirp)) {
		/* SKIP ENTRY? */
		if ((dp->d_ino == 0) || (strncmp(dp->d_name, "whod.", 5)))
			continue;
		if (nhosts >= NHOSTS) {
			atf(0, 4, "Greater than %d hosts",NHOSTS);
			atf(0, 6, "Maybe you should clean out %s", RWHODIR);
			closedir(dirp);
			return;
		}

		/* FOUND A FILE, OPEN IT UP AND LOOK INSIDE */
		if ((f = open(dp->d_name,O_RDONLY)) > 0) {
			cc = read(f, buf, sizeof(struct whod));
			wd = (struct whod *) buf;

			/* NEED AT LEAST A HEADER */
			if (cc >= WHDRSIZE) {
				/* SAVE INTERESTING FIELDS */
				hsp->hs_recvtime = wd->wd_recvtime;
				hsp->hs_sendtime = wd->wd_sendtime;
				hsp->hs_boottime = wd->wd_boottime;
				strncpy(hsp->hs_hostname,wd->wd_hostname,
					HOSTNAMELEN);
				for (i = 0; i < 2; i++)
					hsp->hs_loadav[i] = wd->wd_loadav[i];

				/* ZERO OUT FIELDS WHEN SYSTEM IS DOWN */
				hsp->hs_nusers = hsp->hs_idleusers = 0;
				if (down(hsp,time_now)) {
					for (i = 0; i < 2; i++)
						hsp->hs_loadav[i] = 0;
				}
				else {	
					/* COUNT UP ACTIVE AND IDLE USERS */
					we = (struct whoent *)(buf+cc);
					while (--we >= wd->wd_we) {
						hsp->hs_nusers++;
						if (we->we_idle >= 3600)
							/* IDLE > 1 HOUR */
							hsp->hs_idleusers++;
					}
				}

				/* INCREMENT */
				nhosts++;
				hsp++;
			}
		}
		/* CLOSE FILE */
		close(f);
	}
	closedir(dirp);

	/* DON'T BOTHER IF NOBODY IS AROUND */
	if (nhosts == 0) {
		atf(0, 4, "You probably aren't running /etc/rwhod");
		return;
	}

	/* SORT RESULTS */
	qsort((char *)hs, nhosts, sizeof (hs[0]), cmp);

	/* DISPLAY RESULTS */
	start_scrolling_here();
	for (i = 0; i < nhosts; i++) {
		hsp = &hs[i];
		addf("%-12.12s", hsp->hs_hostname);
		if (down(hsp,time_now))
			addf("%s\n",
			     interval(time_now - hsp->hs_recvtime, "down"));
		else
			addf("%s %7d %5d %7.2f %6.2f %6.2f\n",
			     interval(hsp->hs_sendtime-hsp->hs_boottime,"  up"),
			     hsp->hs_nusers, hsp->hs_idleusers,
			     hsp->hs_loadav[0] / 100.0,
			     hsp->hs_loadav[1] / 100.0,
			     hsp->hs_loadav[2] / 100.0);
	}


	/* RESTORE WORKING DIRECTORY */
	if (chdir(cwd) < 0) {
		perror("chdir");
		fprintf(stderr,"cannot chdir back to %s\n",cwd);
		abnorm_exit(-1);
	}
} /* mode_r */

/**********************************************************************/

/* FORMAT A STRING DESCRIBING THE UPTIME FOR DISPLAY */

char *interval(time, updown)

	int time;
	char *updown;
{
	int days, hours, minutes;

	if (time < 0 || time > 365*24*60*60) {
		sprintf(resbuf, "   %s ??:??", updown);
		return (resbuf);
	}
	minutes = (time + 59) / 60;		/* round to minutes */
	hours = minutes / 60;
	minutes %= 60;
	days = hours / 24;
	hours %= 24;
	if (days)
		sprintf(resbuf, "%s %2d+%02d:%02d",updown,days,hours,minutes);
	else
		sprintf(resbuf, "%s    %2d:%02d",updown,hours,minutes);
	return (resbuf);

} /* interval */

/**********************************************************************/

/* COMPARE ACCORDING TO HOSTNAME */
 
hcmp(h1, h2)

	struct hs *h1, *h2;
{
	return (reverse * strcmp(h1->hs_hostname,h2->hs_hostname));

} /* hcmp */

/**********************************************************************/

/* COMPARE ACCORDING TO LOAD AVERAGE */

lcmp(h1, h2)

	struct hs *h1, *h2;
{
	return (reverse * (h2->hs_loadav[0] - h1->hs_loadav[0]));

} /* lcmp */

/**********************************************************************/

/* COMPARE ACCORDING TO NUMBER OF USERS */

ucmp(h1, h2)

	struct hs *h1, *h2;
{
	return (reverse * (h2->hs_nusers - h1->hs_nusers));

} /* ucmp */

/**********************************************************************/

/* COMPARE ACCORDING TO UPTIME */

tcmp(h1, h2)
	struct hs *h1, *h2;
{


	return(reverse * ((down(h2,time_now) ? h2->hs_recvtime - time_now
					     : h2->hs_sendtime -
					       h2->hs_boottime)
		          -
		          (down(h1,time_now) ? h1->hs_recvtime - time_now
					     : h1->hs_sendtime -
					       h1->hs_boottime)));
} /* tcmp */

/**********************************************************************/

mode_s_init()
{
	Spid = getpid();
}

#ifndef DTYPE_VNODE
#define	DTYPE_VNODE	1	/* file */
#endif 
#ifndef DTYPE_SOCKET
#define	DTYPE_SOCKET	2	/* communications endpoint */
#endif 
#ifndef DTYPE_REMOTE
#define	DTYPE_REMOTE	3	/* remote file (RFA) */
#endif 
#ifndef DTYPE_NSIPC
#define	DTYPE_NSIPC	4	/* CND communication type */
#endif 
#ifndef DTYPE_UNSP
#define DTYPE_UNSP	5	/* user nsp control */
#endif 
#ifndef DTYPE_LLA
#define DTYPE_LLA	6	/* link level access */
#endif 

#ifdef REGION_TO_PATH
char *inum_to_path();
#endif

dump_vas(vas)
register vas_t *vas;
{
        register preg_t *prp;
        register preg_t *last_prp;
        preg_t pr_buf;
        reg_t r_buf;
        char *reg_type, *preg_type;

        prp = vas->va_next;
        last_prp = vas->va_prev;
        for (;;) {
                get_kmem(prp , &pr_buf, sizeof(preg_t));
                get_kmem(pr_buf.p_reg , &r_buf, sizeof(reg_t));
                switch (r_buf.r_type) {
		case RT_UNUSED:  reg_type="UNSD"; break;
		case RT_PRIVATE: reg_type="PRIV"; break;
		case RT_SHARED:  reg_type="SHRD"; break;
		default:         reg_type="UNKN"; break;
                }
                switch (pr_buf.p_type) {
		case PT_UNUSED:     preg_type="UNUSD";  break;
		case PT_UAREA:      preg_type="UAREA";  break;
		case PT_TEXT:       preg_type="TEXT";   break;
		case PT_DATA:       preg_type="DATA";   break;
		case PT_STACK:      preg_type="STACK";  break;
		case PT_SHMEM:      preg_type="SHMEM";  break;
		case PT_NULLDREF:   preg_type="NLLDRF"; break;
		case PT_LIBTXT:     preg_type="LIBTXT"; break;
		case PT_LIBDAT:     preg_type="LIBDAT"; break;
		case PT_SIGSTACK:   preg_type="SIGSTK"; break;
		case PT_IO:         preg_type="IO";     break;
		case PT_MMAP:       preg_type="MMAP";   break;
		case PT_GRAFLOCKPG: preg_type="GRAPHS"; break;
		default:            preg_type="UNKNWN"; break;
                }

                addf("%6s %8x %7d %4s %3d\n",
                        preg_type, pr_buf.p_vaddr, pr_buf.p_count,
                        reg_type, r_buf.r_nvalid);
#ifdef REGION_TO_PATH
		if (r_buf.r_fstore) {
			struct vnode vnode;
			struct inode inode;
			char *path;

			getvar(r_buf.r_fstore, vnode);
			getvar(vnode.v_data, inode);
			if (path=inum_to_path(inode.i_number))
				addf("%s\n", path);
		}
#endif
		if (prp==last_prp)
			break;

                prp = pr_buf.p_next;
         }
}

#ifdef REGION_TO_PATH
/*
 * Convert an i-number to a pathname.
 * return ptr to string or NULL if not found.
 */
char *dev_dirs[] = {
	/* Put the most common directories first so we'll find things quick */
	"/bin",
	"/etc",
	"/lib",
	"/usr/lib",
	"/usr/lib/X11R4",
	"/usr/lib/X11R5",
	"/usr/bin",
	"/usr/bin/X11",
	"/usr/lib/Motif1.1",
	"/usr/lib/Motif1.2",
	"/usr/audio/bin",
	"/usr/contrib/bin",
	"/usr/contrib/bin/X11",
	"/usr/diag/bin",
	"/usr/etc",
	"/usr/etc/ncs",
	"/usr/games",
	"/usr/hosts",
	"/usr/local/bin",
	"/usr/local/bin/X11",
	"/usr/local/etc",
	"/usr/lib/uucp",
	"/usr/lib/X11/vue/etc",
	"/usr/lib/X11/extensions",
	"/usr/softbench/bin",
	"/usr/softbench/lib",
	"/usr/vue/bin",
	NULL,
};

/*
 * Things to do:
 *	only pay attention to files with an execute bit set.
 *	Augment directory list with argv[0].
 *	Check device as well as i-number.
 *	cache only the hits.
 *	Add future directories like /usr/lib/Motif1.3 or /sbin.
 */
char *
inum_to_path(inum)
{
	register struct direct *d;
	static char fname[1024];
	int count, rc;
	DIR *fdev;
	static struct cache {
		int inum;
		char *fname;
		struct cache *next;
	} *cache_head=NULL, *cache_tail=NULL;
	register struct cache *cp;
	struct stat sbuf;

	/* Scan all dirs, build the list */
	if (cache_head==NULL) {
		for (count=0; dev_dirs[count]; count++) {
			fdev = opendir(dev_dirs[count]);
			if (fdev == NULL)
				continue;
			while (d = readdir(fdev)) {
				if (d->d_name[0]=='.')
					continue;
				strcpy(fname, dev_dirs[count]);
				strcat(fname, "/");
				strcat(fname, d->d_name);
				rc = stat(fname, &sbuf);
				if (rc == -1)
					continue;
				/* Must have an execute bit set somewhere */
				if ((sbuf.st_mode & 0111)==0)
					continue;
				cp = (struct cache *) malloc(sizeof(*cp));
				cp->inum = sbuf.st_ino;	/* true inum, not CDF */
				cp->fname = strdup(fname);
				if (cache_head==NULL)	/* first entry */
					cache_head = cp;
				else
					cache_tail->next = cp;
				cache_tail = cp;
				cp->next = NULL;
			}
			closedir(fdev);
		}
	}

	/* Compared to the rest of this program, linear search is high-tech */

	for (cp=cache_head; cp; cp=cp->next)
		if (cp->inum == inum)
			return cp->fname;

	{
		static char buf[20];
		sprintf(buf, "inum=%d", inum);
		return buf;
	}
	return NULL;			/* failure */
}
#endif

char *dev_to_path();

mode_s()
{
	register int i, fline = 0, fd;
	register int nfiles, ret;
	long addr;
	char *p;
	
	int utime, stime, first_line_s;
	int cpticks = 0;
	int j, ofds;

	struct file f;
	struct vnode v;
	struct ofile_t *ptr_chunk[MAXFUPLIM/SFDCHUNK + 1], ofile_buf, *op;
	int chunkno;

	/* READ OUT THE CPU UTILIZATION VALUES */
	pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);
	cp_time = pst_dynamic_info.psd_cpu_time;

   	if (pstat(PSTAT_PROC,&pstat_buf,sizeof(pstat_buf),0,Spid) <= 0) {
		if (Spid == 2) 
			atf(1, 3, "pagedaemon (PID 2) statistics are not kept");
		else 
			atf(1, 3, "Process %d is not active", Spid);
		return;
	}
	/* get the proc structure for Spid */

	get_kmem(proc + pstat_buf.pst_idx , &proc_buf, sizeof(struct proc));
	get_kmem(pstat_buf.pst_addr, &vas, sizeof(vas_t));

	/* IS THIS THE FIRST ITERATION? */
	if (last_cmd == 'S') {
		atf(0, 2, "  Resource usage for pid %d", pstat_buf.pst_pid);
		atf(42,2, "Started: %.20s\n", ctime(&pstat_buf.pst_start));
		addf("  Command: %.68s\n", pstat_buf.pst_cmd);

		utime = pstat_buf.pst_utime;
		stime = pstat_buf.pst_stime;
		addf("  User time accumulated:   %3d:%02d:%02d",
		utime / 3600, (utime / 60) % 60, utime % 60);
		addf("      System time accumulated  %3d:%02d:%02d\n",
		 stime / 3600, (stime / 60) % 60, stime % 60);

		for (i = 0; i < CPUSTATES; i++)
			cpticks += cp_time[i] - old_cp_time[i];
		if (!cpticks)
			cpticks = 1;

		i = (100 * (pstat_buf.pst_cptickstotal - proc_ticks)) / cpticks;
		addf("  Average CPU usage: %14d%%      Instantaneous CPU usage:      %3d%%\n",
			(int)(pstat_buf.pst_pctcpu*100),
			(i > 100 ? 100 : i));
#ifdef MP 
		if (runningprocs > 1) {
			addf("  CPU Process is running on:  %6d\n", pstat_buf.pst_procnum);  
		} 
#endif /* MP */
		addf("  Current resident set size: %7d\n", pstat_buf.pst_rssize); 
		/* DISPLAY AMOUNT OF MEMORY USED PER TYPE */
		addf("\n----- Address Space Usage -------\n");
		addf("Region    Vaddr    Size Type Vpgs\n");
		start_scrolling_here();
		first_line_s = current_y;
		dump_vas(&vas);

		/* SAVE CURRENT NUMBER OF CPU TICKS FOR THIS PROCESS */
		proc_ticks = pstat_buf.pst_cptickstotal;

		/* dump out open file table */
		atf(37, first_line_s-2, "--------------- Open Files ---------------");
		atf(37, first_line_s-1, "fd  flags type  maj minor    count offset");
		atf(37, first_line_s, "");	/*	 position for scrolling */
		nfiles=0;

		ofds=NFDCHUNKS(proc_buf.p_maxof);
		if (proc_buf.p_ofilep != NULL)
			get_kmem(proc_buf.p_ofilep,
				 (char *)ptr_chunk,
				 sizeof(*ptr_chunk) * ofds);
		for (j = 0; j < ofds; j++) {
			if (ptr_chunk[j] == NULL)
				continue;
			getvar(ptr_chunk[j], ofile_buf); 
			op = &ofile_buf;

			for (fd=0; fd<SFDCHUNK; fd++) {
				int path=0;
				if (op->ofile[fd] == NULL)
					continue;
				getvar((long)op->ofile[fd], f);
				nfiles++;
				atf(37, first_line_s+fline, "%-3d %05x ", 
					j*SFDCHUNK + fd, f.f_flag);
				switch (f.f_type) {
				case DTYPE_VNODE:
					getvar(f.f_data, v);
					switch (v.v_type) {
					case VREG:  addf("file "); break;
					case VEMPTYDIR:
					case VDIR:  addf("dir  "); break;
					case VBLK:  addf("block"); path=1;break;
					case VCHR:  addf("char "); path=1;break;
					case VLNK:  addf("link "); break;
					case VSOCK: addf("sock "); break;
					case VFIFO: addf("pipe "); break;
					case VFNWK: addf("net  "); break;
					default:    addf("%5d",v.v_type); break;
					}
					addf(" %-3d 0x%06x",
						major(v.v_rdev), minor(v.v_rdev));
					break;
				case DTYPE_SOCKET: addf("%-18s","socket");break;
				case DTYPE_REMOTE: addf("%-18s","remote");break;
				case DTYPE_NSIPC:  addf("%-18s","nsipc"); break;
				case DTYPE_UNSP:   addf("%-18s","unsp");  break;
				case DTYPE_LLA:    addf("%-18s","lla");   break;
				default: 	   addf("%-18d", f.f_type);
						   break;
				}
				addf(" %-5d %d\n", f.f_count, f.f_offset);
				fline++;
				if (path && (p=dev_to_path(v.v_rdev)) != NULL)
					atf(37, first_line_s+fline++,"    %.*s",
						columns-41, p);
			}   /* for */
		} /* for */
		atf(42, 6, "Number of open files: %12d\n", nfiles);
	} else {
		proc_ticks = pstat_buf.pst_cptickstotal;
	}
	/* SAVE CURRENT CPU UTILIZATION VALUES FOR NEXT ITERATION */
	for (i = 0; i < CPUSTATES; i++)
		old_cp_time[i] = cp_time[i];

} /* mode_s */ 


/*
 * Search a given directory for a device file.
 * Return the pathname or NULL if not found.
 */

char *
dir_search(device, dir)
dev_t device;
char *dir;
{
	struct stat sb;
	register struct direct *d;
	char fname[128];
	static char result_buf[128];
	DIR *fdev;
	char *result;

	fdev = opendir(dir);
	if (fdev == NULL)
		return NULL;

	result = NULL;
	while ((d = readdir(fdev)) != NULL) {
		if (d->d_name[0]=='.')
			continue;
		strcpy(fname, dir);
		strcat(fname, "/");
		strcat(fname, d->d_name);
		if (stat(fname, &sb) == -1)
			continue;
		if (sb.st_rdev == device) {
			result = fname;
			break;
		}
		if ((sb.st_mode & S_IFMT) == S_IFDIR) {
			result = dir_search(device, fname);
			if (result)
				break;
		}
	}

	closedir(fdev);
	if (result) {
		strcpy(result_buf, result);
		result = result_buf;
	}
		
	return result;
}


/*
 * Convert a dev_t to a pathname.
 * Return the pathname or NULL if not found.
 */
char *
dev_to_path(device)
dev_t device;
{
	char *result;
	static struct dev_cache {
		char	*name;		/* full pathname */
		dev_t	dev;		/* device */
		struct dev_cache *next;
	} *dev_cache = NULL;
	register struct dev_cache *cp;

	/* Is it in the cache? */
	for (cp = dev_cache; cp; cp=cp->next)
		if (device == cp->dev)
			return(cp->name);

	result = dir_search(device, "/dev");

	/* Whatever we found (even failure), remember that in the cache */
	cp = (struct dev_cache *) malloc(sizeof(*cp));
	if (cp==NULL) {
		fprintf(stderr, "can't malloc device cache entry\n");
		exit(1);
	}
	cp->dev = device;
	if (result==NULL)
		cp->name = result;
	else
		cp->name = strdup(result);
	cp->next = dev_cache;
	dev_cache = cp;
	return result;
}
mode_t_init()
{
	/* INITIALIZE THE HASH TABLE */
	init_hash();

	/* DEFAULT USER NAME FOR FILTER */
	strcpy(t_user,username(getuid()));

	/* FETCH KERNEL VARIABLE VALUES */
/*	getvar(nl[X_TEXT].n_value, atext); */

	/* ALLOCATE SPACE FOR PROC TABLE AND PARALLEL DATA STRUCTURE */
	if ((int) (proc_table = (struct proc *) calloc(nproc,
						 sizeof(struct proc))) == NULL) {
		perror("proc_table calloc");
		abnorm_exit(-1);
	}
	if ((int) (proc_data = (struct proc2 *) calloc(nproc,
						 sizeof(struct proc2))) == NULL) {
		perror("proc_data calloc");
		abnorm_exit(-1);
	}

	if ((int) (timestruct = (struct timestruct *) calloc(nproc,
					    sizeof(struct timestruct))) == NULL) {
		perror("timestruct calloc");
		abnorm_exit(-1);
	}

	if ((int) (pidlist = (long *) calloc(nproc,
						 sizeof(long))) == NULL) {
		perror("pidlist calloc");
		abnorm_exit(-1);
	}
} /* mode_t_init */

/**********************************************************************/

mode__t()			/* mode_t conflicts with a header file */
{
	register int i, j;
	register int proc_count, hidden_procs;
	register int total_procs;
	register char *cp;
	register char *ttyp, *usr_name;
	register char *command;
	struct pst_status proc_status;
	struct pst_status psbuf[10];
	int idx, cnt, swapped, found, pid;
	unsigned int sticks, uticks;
	unsigned int ticks;
	float delta = 0.0;
	int cpticks = 0;
#ifdef MP 
	int mcpticks[MAX_PROCS];
#endif /* MP */

	total_procs=idx=0;   /* replace pstat_pidlist(0, pidlist,nproc) */
	while ((cnt = pstat(PSTAT_PROC,psbuf, sizeof(struct pst_status),
				10, idx)) > 0) {
		for (i=0; i < cnt; i++) {
			pidlist[total_procs++]=psbuf[i].pst_pid;
		}
		idx = psbuf[cnt-1].pst_idx+1;
	}

	/* READ OUT THE STATISTICS RECORD FOR FORK */
	getvar(nl[X_FORKSTAT].n_value, forkstat);

	/* READ OUT THE CPU UTILIZATION SUMMARY */
	pstat(PSTAT_DYNAMIC, &pst_dynamic_info, sizeof(pst_dynamic_info),0,0);
	cp_time = pst_dynamic_info.psd_cpu_time;

	/* DISPLAY THE HEADER AND GENERAL DATA */
	if (last_cmd == 'T')
	{
	    for (i = 0; i < CPUSTATES; i++)
		cpticks += cp_time[i] - old_cp_time[i];
	    if (!cpticks)
		cpticks = 1;
	    delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
			       ((current_time.tv_usec -
				 previous_time.tv_usec) / 1000000.0));
#ifdef MP 
	    if (runningprocs == 1) {
#endif /* MP */

		atf(0, 2,
"User   CPU: %3d%%    Nice CPU: %3d%%            Load average over 1 min: %6.2f\n",
		   (100 * (cp_time[CP_USER] - old_cp_time[CP_USER])) / cpticks,
		   (100 * (cp_time[CP_NICE] - old_cp_time[CP_NICE])) / cpticks,
		   pst_dynamic_info.psd_avg_1_min);

		addf(
"System CPU: %3d%%    Forks:  %2d (%3d pages)    Load average over 5 min: %6.2f\n",
		     (100 * (cp_time[CP_SYS] - old_cp_time[CP_SYS])) / cpticks,
		     (int) ((forkstat.cntfork - old_forkstat.cntfork) * delta),
		     (int) ((forkstat.sizfork - old_forkstat.sizfork) * delta),
		     pst_dynamic_info.psd_avg_5_min
		    );
		addf(
"Idle   CPU: %3d%%    Vforks: %2d (%3d pages)    Load average over 15 min:%6.2f\n\n",
			     (100 * ((cp_time[CP_IDLE] - old_cp_time[CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
			     + (cp_time[CP_WAIT] - old_cp_time[CP_WAIT])
#endif  /* CP_WAIT */
					    ) / cpticks),
		    (int) ((forkstat.cntvfork - old_forkstat.cntvfork) * delta),
		    (int) ((forkstat.sizvfork - old_forkstat.sizvfork) * delta),
		    pst_dynamic_info.psd_avg_15_min);
#ifdef MP 
	    } else {  /* MP system */
		atf(2, 2, "Forks:  %2d (%3d pages)      Vforks:  %2d (%3d pages)\n",
		   (int) ((forkstat.cntfork - old_forkstat.cntfork) * delta),
		   (int) ((forkstat.sizfork - old_forkstat.sizfork) * delta),
		   (int) ((forkstat.cntvfork - old_forkstat.cntvfork) * delta),
		   (int) ((forkstat.sizvfork - old_forkstat.sizvfork) * delta));
		
	        for (i = 0; i < MAX_PROCS; i++) {
		    mcpticks[i] = 0;
	        }  
	 	atf(13, 4, "MULTI CPU UTILIZATION");
	        atf(59, 4, "LOAD AVERAGES");
	        addf("\n  CPU:  User CPU:  Sys CPU: Idle CPU: Nice CPU:      1 min:  5 min:  15 min:\n");
	        for (i = 0; i < runningprocs; i++) {
		    for (j = 0; j < CPUSTATES; j++) {
		        mcpticks[i] += pst_dynamic_info.psd_mp_cpu_time[i][j] 
				- old_mcp_time[i][j];
		        if (!mcpticks[i])
			    mcpticks[i] = 1;
		    }

		    addf("  %3d %7d%% %8d%% %8d%% %8d%%       %7.2f %7.2f %8.2f\n",
			i, ((100 * 
				(pst_dynamic_info.psd_mp_cpu_time[i][CP_USER] -
				     old_mcp_time[i][CP_USER])) / mcpticks[i]),
			   (100 * 
				((pst_dynamic_info.psd_mp_cpu_time[i][CP_SYS] 
				  - old_mcp_time[i][CP_SYS])
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_INTR]
				 - old_mcp_time[i][CP_INTR])
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_SSYS] -
				    old_mcp_time[i][CP_SSYS])) / mcpticks[i]),
			   (100 * 
				((pst_dynamic_info.psd_mp_cpu_time[i][CP_IDLE] -
				     old_mcp_time[i][CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
				 + (pst_dynamic_info.psd_mp_cpu_time[i][CP_WAIT]
				 - old_mcp_time[i][CP_WAIT])
#endif  /* CP_WAIT */
				    ) / mcpticks[i]),
			   (100 * 
				(pst_dynamic_info.psd_mp_cpu_time[i][CP_NICE] -
	     			     old_mcp_time[i][CP_NICE]) / mcpticks[i]),
			   pst_dynamic_info.psd_mp_avg_1_min[i],
			   pst_dynamic_info.psd_mp_avg_5_min[i],
			   pst_dynamic_info.psd_mp_avg_15_min[i]);
	        }   
		addf("  AVG: %6d%% %8d%% %8d%% %8d%%       %7.2f %7.2f %8.2f\n",
		    100 * (cp_time[CP_USER] - old_cp_time[CP_USER]) / cpticks,
		    100 * ((cp_time[CP_SYS] - old_cp_time[CP_SYS]) 
		    + (cp_time[CP_INTR] - old_cp_time[CP_INTR])
		    + (cp_time[CP_SSYS] - old_cp_time[CP_SSYS])) / cpticks,
		    100 * ((cp_time[CP_IDLE] - old_cp_time[CP_IDLE])
#ifdef CP_WAIT /* 2.0 Spectrum release */
					     + (cp_time[CP_WAIT] -
					      old_cp_time[CP_WAIT])
#endif  /* CP_WAIT */
					    ) / cpticks,
		    100 * (cp_time[CP_NICE] - old_cp_time[CP_NICE]) / cpticks,
		    pst_dynamic_info.psd_avg_1_min,
		    pst_dynamic_info.psd_avg_5_min,
		    pst_dynamic_info.psd_avg_15_min);
	    }
#endif /* MP */ 
	}
		
	/* SAVE CURRENT VALUES FOR NEXT ITERATION */
#ifdef MP 
	if (runningprocs > 1) {
	    for (i=0; i < runningprocs; i++) {
		for (j=0; j < CPUSTATES; j++) {
		     old_mcp_time[i][j] = pst_dynamic_info.psd_mp_cpu_time[i][j];
		}
	    }
	} 
#endif /* MP */ 
	for (i = 0; i < CPUSTATES; i++)
	    old_cp_time[i] = cp_time[i];

    	old_forkstat = forkstat;
	

	/* PUT UP HEADER FOR TASKS LIST */
#ifdef hp9000s200
	atf(0, 6,
" PID  PPID TTY STATE  FLAG PRI SYS USR TEXT DATA STACK RSS    USER COMMAND\n");
#endif  /* hp9000s200 */

#ifdef __hp9000s800
#ifdef MP 
    if (runningprocs > 1) 
	atf(0, 8 + runningprocs,
"  PID  PPID TTY STATE   FLAG PRI SYS USR TEXT DATA STACK RSS CPU   USER COMMAND\n");
    else
#endif /* MP */ 
	atf(0, 6,
"  PID  PPID TTY STATE   FLAG PRI SYS USR TEXT DATA STACK RSS    USER COMMAND\n");

#endif  /* __hp9000s800 */
	start_scrolling_here();

	/* FOR ALL THE PROCESSES IN THE PROC TABLE */
	hidden_procs = proc_count = 0;
	for (i = 0; i < total_procs; i++)
	{
		pid = pidlist[i];
   		if ((pstat(PSTAT_PROC,&proc_status,sizeof(proc_status),0,pid) 
		   == -1))
			continue;
		mproc2 = &proc_data[i];

		/* APPLY USERNAME FILTER CRITERIA? */
		usr_name = username(proc_status.pst_uid);
		if ((t_filter == 'U') && strcmp(usr_name,t_user)) {
			hidden_procs++;
			continue;
		}

	
		/* FETCH TTY NAME */
#ifdef hp9000s200
		if (mproc2->d_pid != pid)
		{
			ttyp = gettty(proc_status.pst_major,
				proc_status.pst_minor);
			mproc2->d_tty[0] = ttyp[0];
			mproc2->d_tty[1] = ttyp[1] ? ttyp[1] : ' ';
			mproc2->d_pid = pid;
		}
#endif  /* hp9000s200 */
#ifdef __hp9000s800
		if ((mproc2->d_pid != pid) || (mproc2->d_pid == 0))
		{
			ttyp = gettty(proc_status.pst_major, 
				proc_status.pst_minor);
			mproc2->d_tty[0] = ttyp[0];
			mproc2->d_tty[1] = ttyp[1];
			if (!ttyp[2])
			    /* 2 LETTER TTY */
			    mproc2->d_tty[2] = mproc2->d_tty[3] = ' ';
			else if (!ttyp[3]) {
				/* 3 LETTER TTY */
				mproc2->d_tty[2] = ttyp[2];
				mproc2->d_tty[3] = ' ';
			}
			else {
				/* 4 LETTER TTY */
				mproc2->d_tty[2] = ttyp[2];
				mproc2->d_tty[3] = ttyp[3];
			}
			mproc2->d_pid = pid;
		}
#endif  /* __hp9000s800 */

		/* SKIP CPU UTILIZATION SECTION? */
		if ((proc_status.pst_flag & PS_SYS) ||
		    (proc_status.pst_stat == PS_ZOMBIE)) {
			sticks = uticks = 0;
			goto state;
		}

		/* CALCULATE CPU UTILIZATION */
		found = 0;
		/* QUICK CHECK */
		if (timestruct[i].pid == pid) {
			timestruct[i].seen = 1;
			found = 1;
			j = i;
		}
		else for (j = 0; j < timecount; j++)
			/* HAVE WE SEEN IT BEFORE? */
			if (timestruct[j].pid == pid) {
				timestruct[j].seen = 1;
				found = 1;
				break;
			}

		/* HAVE WE SEEN THIS PROCESS BEFORE? */
		if (found) {
		    /* YES, CALCULATE DELTA/SAVE VALUE */
		    ticks = proc_status.pst_stime * HZ;
		    sticks = ticks - timestruct[j].sticks ;
		    timestruct[j].sticks = ticks;
		    ticks = proc_status.pst_utime * HZ;
		    uticks = ticks - timestruct[j].uticks ;
		    timestruct[j].uticks = ticks;
		} else {
		    /* NO, FILL IN AN ENTRY FOR IT */
		    if (!timestruct[i].pid)
		    {
			j = i;
		    }
		    else
		    {
		    for (j = 0; j < timecount; j++)
			/* RE-USABLE ENTRY? */
			if (!timestruct[j].pid)
			    break;
		    if (j == timecount) {
			/* NEED A NEW ENTRY */
			timecount++;
			if (timecount > nproc) {
			    fprintf(stderr,"Too many processes (%d)\n",
				    nproc);
			    abnorm_exit(-1);
			}
		    }

		    }
		    /* STORE VALUES */
		    timestruct[j].pid = pid;
		    timestruct[j].seen = 1;
		    timestruct[j].sticks = proc_status.pst_stime * HZ;
		    timestruct[j].uticks = proc_status.pst_utime * HZ;
		    sticks = uticks = 0;
		}

		/* TRANSLATE TICKS TO PERCENTAGE */
#if (HZ == 50)
		sticks = (unsigned int) ((sticks * 2) * delta);
		uticks = (unsigned int) ((uticks * 2) * delta);
#else
#if (HZ == 100)
		sticks = (unsigned int) (sticks * delta);
		uticks = (unsigned int) (uticks * delta);
#else
		sticks = (unsigned int) (((100 * sticks) / HZ) * delta);
		uticks = (unsigned int) (((100 * uticks) / HZ) * delta);
#endif 
#endif 
		
		/* MAKE SURE THE PERCENTAGES ARE <= 100 SINCE */
		/* THERE MAY BE INCONSISTENTIES IN THE DATA */
		if (sticks > 100)
			sticks = 100;
		if (uticks > 100)
			uticks = 100;

		/* PROCESS STATE? */
state:
		switch (proc_status.pst_stat)
		{
			case PS_SLEEP:	cp = "SL";  break;
	   /*		case SWAIT:	cp = "WT";  break;     */
			case PS_RUN:	cp = "RU";  break;
			case PS_IDLE:	cp = "ID";
					sticks = uticks = 0;
					break;
			case PS_ZOMBIE:	cp = "ZB";
					sticks = uticks = 0;
					break;
			case PS_STOP:	cp = "ST";  break;
			default:	cp = "??";
		}

		/* APPLY CPU USAGE FILTER CRITERIA? */
		if ((t_filter == 'N') && ((sticks + uticks) == 0)) {
			hidden_procs++;
			continue;
		}

		/* WE HAVE A PROCESS, SHALL WE DISPLAY IT? */
			/* IS THIS A SYSTEM OR ZOMBIE PROCESS? */
			if ((proc_status.pst_flag & PS_SYS) ||
			    (proc_status.pst_stat == PS_ZOMBIE)) {
			    /* THEN DON'T DISPLAY EVERYTHING */
#ifdef hp9000s200
			    addf( "%5u %5u    %2.2s %08X %3d                            ",
					 pid,proc_status.pst_ppid,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks);
#endif  /* hp9000s200 */
#ifdef __hp9000s800
#ifdef MP 
			    if (runningprocs > 1) 
			        addf( "%5u %5u      %2.2s %08X %3d                               ",
					 pid,proc_status.pst_ppid,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks);
			    else
#endif /* MP */ 
			        addf( "%5u %5u      %2.2s %08X %3d                            ",
					 pid,proc_status.pst_ppid,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks);

#endif  /* __hp9000s800 */
			} else {
			    /* NORMAL PROCESS */
#ifdef hp9000s200
			    addf( "%5u %5u %-2.2s %2.2s %08X %3d%3d%%%3d%%%5u%5u%5u%5u",
					 pid,proc_status.pst_ppid,
					 mproc2->d_tty,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks,
					 proc_status.pst_tsize,
					 proc_status.pst_dsize,
					 proc_status.pst_ssize,
					 proc_status.pst_rssize);  
#endif  /* hp9000s200 */
#ifdef __hp9000s800
#ifdef MP 
			    if (runningprocs > 1)
			        addf( "%5u %5u %-4.4s %2.2s %08X %3d%3d%%%3d%%%5u%5u%5u%5u%3u",
					 pid,proc_status.pst_ppid,
					 mproc2->d_tty,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks,
					 proc_status.pst_tsize,
					 proc_status.pst_dsize,
					 proc_status.pst_ssize,
					 proc_status.pst_rssize,
					 proc_status.pst_procnum);  
			    else 
#endif /* MP */
			        addf( "%5u %5u %-4.4s %2.2s %08X %3d%3d%%%3d%%%5u%5u%5u%5u",
					 pid,proc_status.pst_ppid,
					 mproc2->d_tty,cp,
					 proc_status.pst_flag,
					 proc_status.pst_pri,
					 sticks,uticks,
					 proc_status.pst_tsize,
					 proc_status.pst_dsize,
					 proc_status.pst_ssize,
					 proc_status.pst_rssize);  
#endif  /* __hp9000s800 */
			}
			addf("%8s ",usr_name);
			if (pid == 0)
				addf("swapper");
			else if (pid == 2)
				addf("pagedaemon");
#ifdef __hp9000s800
			else if (pid == 3)
				addf("statdaemon");
#endif  /* __hp9000s800 */
			else {
				if (proc_status.pst_stat == PS_ZOMBIE) {
				    command = "<defunct>";
				} else {
				    char *temp_cp;

				    command = proc_status.pst_cmd;
				    if (*command == '-')
					command++;

				    for (temp_cp = command;
					 (*temp_cp && *temp_cp != ' ');
					 temp_cp++)
					if (*temp_cp == '/')
					    command = temp_cp + 1;

				    if (*temp_cp == ' ')
					*temp_cp = '\0';
				}
#ifdef hp9000s200
				addf( "%-12.12s", command);
#endif  /* hp9000s200 */
#ifdef __hp9000s800
#ifdef MP 
			        if (runningprocs > 1) 
				    addf( "%-7.7s", command);
				else
#endif /* MP */ 
				    addf( "%-10.10s", command);
#endif  /* __hp9000s800 */
			}
			addf("\n");
		
		proc_count++;
	}

	/* MARK ENTRIES NOT USED AVAILABLE FOR RE-USE */
	for (i = 0; i < timecount; i++)
		if (timestruct[i].seen)
			/* CLEAR BIT FOR NEXT TIME */
			timestruct[i].seen = 0;
		else
			/* MARK FOR RE-USE */
			timestruct[i].pid = 0;
	addf(0, 66, "(%d)", hidden_procs + proc_count);
} /* mode__t */

/**********************************************************************/

#ifdef hp9000s200

/* RETURNS THE USER'S TTY NUMBER OR ? IF NONE  */

char *gettty(ttyp, ttyd)
long ttyp;   /* major */
long ttyd;   /* minor */
{
	register i;
	register char *p;
	long minor_number=0, major_number=0;

	if ((ttyp == -1) && (ttyd == -1))
		return("?");
	for (i = 0; i < ndev; i++) {
		/* LOOK FOR TTY NAME IGNORING /dev/syscon AND /dev/systty */
		minor_number = (devl[i].dev & 0x00ffffff);
		major_number = (0xff000000 & devl[i].dev);
		major_number = major_number >> 24;
		if ((minor_number == ttyd) && (major_number == ttyp) &&
		   (strcmp(devl[i].dname,"syscon")) &&
		   (strcmp(devl[i].dname,"systty"))) {
			p = devl[i].dname;
			if ((p[1] =='t') && (p[2]=='y'))
				p += 3;
			return(p);
		}
	}
	return("?");

} /* gettty */

/**********************************************************************/
#endif  /* hp9000s200 */

/**********************************************************************/

#ifdef __hp9000s800

/* RETURNS THE USER'S TTY NUMBER OR ? IF NONE */

char *gettty(ttyp, ttyd)
long ttyp;   /* major */
long ttyd;   /* minor */
{
        register i;
        register char *p;
	long minor_number=0, major_number=0;

	if ((ttyp == -1) && (ttyd == -1))
		return("?");

        if (ttyd == cons_mux_dev)           /* alias for console */
                        ttyd = 0;           /* real console */

        for (i = 0; i < ndev; i++) {
		minor_number = (devl[i].dev & 0x00ffffff);
		major_number = (0xff000000 & devl[i].dev);
		major_number = major_number >> 24;
		if ((minor_number == ttyd) && (major_number == ttyp)) {
                        p = devl[i].dname;
			if ((p[1] == 't') && (p[2] == 'y'))
				p += 3;
                        return(p);
                }
	}
	return("?");

}
#endif  /* __hp9000s800 */

/**********************************************************************/

/* THESE FOLLOWING ROUTINES HANDLE UID TO USERNAM MAPPING
   THEY USE A HASHING TABLE SCHEME TO REDUCE READING OVERHEAD */

init_hash()

{
    register int i;
    register struct hash_el *h;

    /* INITIALIZE THE HASH TABLE */
    for (h = hash_table, i = 0; i < HASHTABLESIZE; h++, i++)
        h->u_id = H_empty;

} /* init_hash */

/**********************************************************************/

char *username(uid)

    register int uid;

{
    register int index;
    register int found;

    /* SEARCH FOR UID IN HASH TABLE */
    index = HASHIT(uid);
    while ((found = hash_table[index].u_id) != uid)
    {
	if (found == H_empty)
	{
	    /* NOT HERE SO GET IT OUT OF /etc/passwd */
	    index = get_user(uid);
	    break;
	}
	index = ++index % HASHTABLESIZE;
    }

    /* RETURN USER NAME STRING */
    return(hash_table[index].name);

} /* username */

/**********************************************************************/

get_user(uid)

    register int uid;
{
    struct passwd *getpwent();
    register struct passwd *pwd;

    /* LOOK FOR UID IN /etc/passwd */
    setpwent();
    while ((pwd = getpwent()) != NULL)
	if (pwd->pw_uid == uid)
	    return(enter_user(pwd->pw_uid, pwd->pw_name));

    /* CAN'T FIND IT SO STORE THE VALUE INSTEAD */
    sprintf(get_user_buff, "%d", uid);
    return(enter_user(uid, get_user_buff));

} /* get_user */

/**********************************************************************/

enter_user(uid, name)

    register int  uid;
    register char *name;
{
    register int index, i;

    /* HASH TABLE OVERFLOW? */
    if (++uid_count >= HASHTABLESIZE)
    {
        /* AVOID TABLE OVERFLOW BY RE-USING AN EXISTING SLOT */
	uid_count--;
        index = HASHIT(uid);

	/* MAKE ABSOLUTELY SURE WE DON'T RE-USE THE EMPTY SLOT */
        while (hash_table[index].u_id == H_empty)
	    index = ++index % HASHTABLESIZE;
    }
    else
    {
	/* FIND NEXT EMPTY SLOT */
        index = HASHIT(uid);
        while (hash_table[index].u_id != H_empty)
	    index = ++index % HASHTABLESIZE;
    }

    /* ENTER NEW UID AND NAME */
    hash_table[index].u_id = uid;
    i = 0;
    while ((i < 8) && (name[i] != '\0'))
    {
        hash_table[index].name[i] = name[i];
        i++;
    }
    hash_table[index].name[i] = '\0';
    return(index);

} /* enter_user */


/**********************************************************************/

mode_v()
{
	float delta;

	/* READ OUT THE SYSTEMWIDE TOTALS DATA STRUCTURE */
	getvar(nl[X_TOTAL].n_value, total);

	/* DISPLAY IT */
	atf(0, 2, "  PROCESS TOTALS:           VIRTUAL MEMORY PAGES:       PHYSICAL MEMORY PAGES:\n\n");
	addf("  Run queue: %12d   Total in use: %7d       Total in use: %8d\n",
		total.t_rq,total.t_vm,total.t_rm);
	addf("  Disc wait: %12d   Active:       %7d       Active:       %8d\n",
			     total.t_dw,total.t_avm,total.t_arm);
	addf("  Page wait: %12d   Text:         %7d       Text:         %8d\n",
			     total.t_pw,total.t_vmtxt,total.t_rmtxt);
	addf("  Sleeping:  %12d   Active text:  %7d       Active text:  %8d\n",
			     total.t_sl,total.t_avmtxt,total.t_armtxt);
	addf("  Swapped:   %12d                               Free pages:  %9d\n",
			     total.t_sw,total.t_free);

	/* READ OUT THE VMMETER STRUCTURE */
	getvar(nl[X_SUM].n_value, cnt);

	/* DISPLAY IT */
	if (last_cmd == 'V')
	{
		delta = 1.0 / ((current_time.tv_sec - previous_time.tv_sec) +
			       ((current_time.tv_usec -
				 previous_time.tv_usec) / 1000000.0));

/*		atf(0, 10, "  Total faults:      %4d   Pageout daemon scans:  %4d   Max paging/sec\n", */
		atf(0, 10, "  Total faults:      %4d   Pageout daemon scans:  %4d   Max sleep for very\n",
				     (int) ((cnt.v_faults -
					     old_cnt.v_faults) * delta),
				     (int) ((cnt.v_scan -
					     old_cnt.v_scan) * delta));
		addf("  Total reclaims:    %4d   Revolutions of hand:   %4d",
				     (int) ((cnt.v_pgrec -
					     old_cnt.v_pgrec) * delta),
				     (int) ((cnt.v_rev -
					     old_cnt.v_rev) * delta));
		addf("    swappable:     %4d\n",maxslp);
		addf("  Free list reclaims:%4d   Pages freed by daemon: %4d   Free pages before\n",
				     (int) ((cnt.v_pgfrec -
					     old_cnt.v_pgfrec) * delta),
				     (int) ((cnt.v_dfree -
					     old_cnt.v_dfree) * delta));
		addf("  Pages in transit:  %4d   Taken from sequential: %4d",
				     (int) ((cnt.v_intrans -
					     old_cnt.v_intrans) * delta),
				     (int) ((cnt.v_seqfree -
					     old_cnt.v_seqfree) * delta));
		addf("     swapping: %8d", minfree);
	}

	atf(28,14, "Clock freeze level: %7d", lotsfree); 
	atf(28, 15, "Desired free pages: %7d   ", desfree);

	if (last_cmd == 'V')
	{
		atf(0, 17, "  Processes swapped in:  %4d                           Context switches: %4d\n",
				     (int) ((cnt.v_swpin -
					     old_cnt.v_swpin) * delta),
				     (int) ((cnt.v_swtch -
					     old_cnt.v_swtch) * delta));
		addf("  Processes swapped out: %4d                           Trap calls:       %4d\n",
				     (int) ((cnt.v_swpout -
					     old_cnt.v_swpout) * delta),
				     (int) ((cnt.v_trap -
					     old_cnt.v_trap) * delta));
		addf("  Pages swapped in:      %4d (Paged in:  %4d)         System calls:     %4d\n",
				     (int) ((cnt.v_pswpin -
					     old_cnt.v_pswpin) * delta),
				     (int) ((cnt.v_pgpgin -
					     old_cnt.v_pgpgin) * delta),
				     (int) ((cnt.v_syscall -
					     old_cnt.v_syscall) * delta));
		addf("  Pages swapped out:     %4d (Paged out: %4d)         Device interrupts:%4d\n",
				     (int) ((cnt.v_pswpout -
					     old_cnt.v_pswpout) * delta),
				     (int) ((cnt.v_pgpgout -
					     old_cnt.v_pgpgout) * delta),
				     (int) ((cnt.v_intr -
					     old_cnt.v_intr) * delta));
		addf("  Executable demand fill:%4d (Objects:%4d)\n",
				     (int) ((cnt.v_exfod -
					     old_cnt.v_exfod) * delta),
				     (int) ((cnt.v_nexfod -
					     old_cnt.v_nexfod) * delta));
		addf("  Zero demand fill: %9d (Objects:%4d)            Bytes per page:%7d\n",
				     (int) ((cnt.v_zfod -
					     old_cnt.v_zfod) * delta),
				     (int) ((cnt.v_nzfod -
					     old_cnt.v_nzfod) * delta),
				     NBPG);

	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	/*!!! Not displaying: v_pgin,v_pgout, v_xsfrec,v_xifrec,   !!!*/
	/*!!!                 v_rfod,v_nfrfod,v_pdma,              !!!*/
	/*!!!                 v_vrfod, v_nvrfod, deficit           !!!*/
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	}
	old_cnt = cnt;

} /* mode_v */

/**********************************************************************/

main(argc, argv)
	
	int	argc;
	char	*argv[];

{
	register int	nd;
	register int	i, j;
   	register int	*nscr_pt, *next_end;
	struct itimerval itimerval;
	char		buf[1024], *cp, c;
	int		found;
	int		offset_index, offsets[100];
	struct modes *m;
	FILE		*hfd;
	int read_cnt;
	char read_str[80];

	/* FETCH PARAMETERS */
	myname = argv[0];
	while ((c = getopt(argc,argv,"c:d:i:l:n:r:s:")) != EOF)
		switch(c) {
		    case 'c':
			cmd = optarg[0];
			if ((cmd >= 'a') && (cmd <= 'z'))  /* UPPERCASE */
				cmd = cmd - 'a' + 'A';
			break;

		    case 'd':
			iterationcnt = atoi(optarg);
			if (iterationcnt < 1) {
				fprintf(stderr,
				      "%s: Iteration count (%d) must be > 0\n",
					myname,iterationcnt);
				exit(-1);
			}
			break;

		    case 'i':
			update_interval = atoi(optarg);
			if (update_interval < 1) {
				fprintf(stderr,
				       "%s: Update interval (%d) must be > 0\n",
					myname,update_interval);
				exit(-1);
			}
			break;

		    case 'l':
			strcpy(lan_name,optarg);
			break;

		    case 'n':
			strcpy(hpux_file,optarg);
			break;

		    case 'r':
			if (strcmp(NREVISION,optarg)) {
				fprintf(stderr,
"%s: N screen protocol revision IDs ('%s' and '%s') don't match\n",
					myname,NREVISION,optarg);
				exit(-1);
			}
			break;

		    case 's':
			strcpy(hardcopy,optarg);
			break;

		    default:
			usage();
			exit(-1);
		}

	/* If monitor is installed improperly, give them a nice message. */
	if (geteuid()) {
		fprintf(stderr, "%s must be set-user-id root.\n", myname);
		exit(1);
	}

	/* Put is in the default mode */
	for (mode=modes; mode->letter; mode++)
		/* do nothing */;

	/* FIND OUT WHAT WE ARE RUNNING ON */
	if (uname(&name) < 0) {
		perror("uname");
		exit(-1);
	}

#ifdef hp9000s200
	addrspace = 8192;	/* address space in pages 16M/2k pg (UMM) */
	if (strncmp(name.machine, "9000/310", 8) == 0)
		addrspace = 4096;	/* address space in pages 16M/4k pg */
	else if (strncmp(name.machine, "9000/3", 6) == 0)
		addrspace = 1048576;	/* address space in pages 4G/4k pg */
#endif  /* hp9000s200 */

	pstat(PSTAT_STATIC, &pst_static_info, sizeof(pst_static_info),0,0);

	/* GET DATA FROM monitorfile */
	no_error = 1;
	if (getcwd(cwd,1024) == NULL) {  /* USED ALSO IN mode_r */
		perror("getcwd length > 1024?");
		exit(-1);
	}
	if (readata()) {
		/* NO LUCK, SO GET IT THE HARD WAY */
		if (argc == 1)
		    /* DON'T PRINT IF POSSIBLY INVOKED BY N SCREEN, */
		    /* N SCREEN EXPECTS STRICT FORMATTING OF OUTPUT */
		    printf("%s: Creating new %s file\n",myname,monitorfile);
		getdev("/dev");
		getdev("/dev/pty");
		if (chdir(cwd) < 0) {
			perror("chdir");
			fprintf(stderr,"Can't chdir back to %s\n",
				cwd);
			exit(-1);
		}
		if (nlist(hpux_file,nl) != 0) {
			perror("nlist");
			fprintf(stderr,"Can't do an nlist on %s\n",
				hpux_file);
			exit(-1);
		}


		/* MAKE SURE THAT ALL VALUES WERE READ OUT OK */
		for (nd = 0; nd <= X_LASTENTRY; nd++)
			/* LACK OF CONFIG, DISKLESS OR DRIVER VARIABLE IS OK */
			if ((nl[nd].n_value == 0) &&
#ifdef hp9000s200
			    (!((nd >= X_FIRST_DRIVER) &&
			       (nd <= X_LAST_DRIVER))) &&
#endif  /* hp9000s200 */
			    (!((nd >= X_FIRST_CONFIG) &&
			       (nd <= X_LAST_CONFIG))) &&
			    (nd != X_MY_SITE_STATUS) && (nd != X_NSP) &&
			    (nd != X_NUM_NSPS) && (nd != X_FREE_NSPS) &&
			    (nd != X_CLUSTAB) && (nd != X_NCSP) &&
			    (nd != X_CLEANUP_RUNNING) && (nd != X_FAILED_SITES) &&
			    (nd != X_RETRYSITES)
			   ) {
				no_error = 0;
				fprintf(stderr,
					"nlist variable %s not found in %s\n",
					nl[nd].n_name,hpux_file);
			}
		if (!no_error)
			exit(-1);

		/* WRITE IT OUT FOR FUTURE USE */
		wrdata();
	}

	/* LOCK THE MONITOR PAGES IN MEM */
	if (plock(PROCLOCK))
		perror("plock");


#ifdef RTPRIO
	/* SET TO LOWEST REALTIME PRIORITY (DOMINATES SYSTEM) */
	if (rtprio(0,127) == -1)
		perror("rtprio");
#else /* not RTPRIO */
	/* NICE VAL WILL DEGRADE WITH TIME */
	if (nice(NICEVAL) == -1)
		perror("nice");
#endif  /* RTPRIO */

	/* OPEN THE KERNEL VIRTUAL AND PHYSICAL MEMORY DEVICES */

	if ((kmem_fd = open("/dev/kmem", O_RDONLY))< 0) {
		perror ("kmem read failed");
	}

	if ((mem_fd = open("/dev/mem", O_RDONLY))< 0) {
		perror ("mem read failed");
	}

	/* READ OUT GENERAL KERNEL VARIABLES */
	getkvars();

	/* MODE DEPENDENT INITIALIZATION */
	for (m=modes; m->letter; m++) {
		if (m->init != NULL)
			(*m->init)();
        }

	/* ARE WE ASKED TO LOG DATA TO STANDARD OUT? */
	if (optind < argc) {
		/* SET UP STANDARD OUT AS NON-BUFFERED */
		setbuf(stdout,NULL);

		/* YES, SEE WHAT SPECIFICALLY */
		while (optind < argc) {
			/* VALID OPTION NAME? */
			found = i = 0;
			while (sf_info[i].sf_flag) {
			    if (strcmp(argv[optind],sf_info[i].sf_name) == 0) {
				sigflags |= sf_info[i].sf_flag;
				found = 1;
				flags_set++;
				break;
			    }
			    i++;
			}

			/* ERROR IF NOT A VALID NAME */
			if (!found) {
			    if (strcmp(argv[optind],"help"))
			        fprintf(stderr,
					"\n%s: %s invalid statistic name\n",
				        myname,argv[optind]);
			    usage();
			    exit(-1);
			}

			/* NEXT */
			optind++;
		}

		/* PRINT HEADER LINE FOR SELECTED STATISTICS */
		i = j = 0;
		while (sf_info[i].sf_flag) {
		    /* SPECIFIED? */
		    if (sigflags & sf_info[i].sf_flag) {
			j++;
		        if (j == flags_set) {
			    /* LAST ONE */
			    printf("%-10s\n",sf_info[i].sf_name);
			    break;
		        } else
			    printf("%-10s ",sf_info[i].sf_name);
		    }
		    i++;
		}

		/* MAKE SURE LAN CARD IS OPENED */
		if (lan_fd == -1) {
			fprintf(stderr,"Can not open LAN device file: %s",
				lan_name);
			exit(-1);
		}

		/* SET UP SIGNAL HANDLER */
		interval_vec.sv_mask = 0;
		interval_vec.sv_onstack = 0;
		interval_vec.sv_handler = interval_signals;    
		if (sigvector(SIGALRM, &interval_vec, 0) == -1) {
			perror("sigvector");
			exit(-1);
		}

		/* WHEN DID WE START THIS? */
		gettimeofday(&start_time,0);

		/* ARRANGE FOR PERIODIC SIGALRM SIGNALS */
		itimerval.it_interval.tv_sec = update_interval;
		itimerval.it_interval.tv_usec = 0;
		itimerval.it_value.tv_sec = 0;
		itimerval.it_value.tv_usec = 1;
		if (setitimer(ITIMER_REAL, &itimerval,0)) {
			perror("setitimer");
			exit(-1);
		}

		/* SLEEP WHILE THE SIGNAL HANDLER DOES THE REAL WORK */
		while (iterationcnt > 0)
			 /* SLEEP ABORTS EACH TIME THE SIGNAL HANDLER IS HIT */
			 sleep(update_interval);

		/* CLEAN UP AN EXIT */
		goto done;
	}

	/* FETCH TERMCAP ENTRY INFORMATION */
#ifdef CURSES
	initscr();
#endif  /* CURSES */
	i = tgetent(buf,getenv("TERM"));
	if (i < 0) {
		fprintf(stderr,"Can't open the termcap file");
		perror("tgetent");
	}
	if (i == 0) {
		fprintf(stderr,"Termcap entry not found for %s\n",
		        getenv("TERM"));
	}
	if (i <= 0) {
		fprintf(stderr,
	"Will default to %dx%d terminal with standard HP escape sequences\n",
			ROWS,COLUMNS);
		sleep(2);
		rows = ROWS;
		columns = COLUMNS;
		automargin = 1;
		eolwrap = 0;
		ho_or_cm = 1;
		cl_len = 4;
		cl[0] = '\033';
		cl[1] = 'H';
		cl[2] = '\033';
		cl[3] = 'J';
		cl[4] = '\0';
		ho_len = 2;
		ho[0] = '\033';
		ho[1] = 'H';
		ho[2] = '\0';
	} else {
		/* FETCH THE NUMBER OF LINES PER SCREEN */
		/* FROM TERMCAP OR FROM LINES ENVIRONMENT VARIABLE */
		rows = tgetnum("li");
		if (rows == -1) {
			fprintf(stderr,
"Lines per screen (termcap 'li' or LINES) was not found (TERM = '%s')\n",
				getenv("TERM"));
			rows = ROWS;
			fprintf(stderr,"Will assume it to be %d lines\n",ROWS);
			sleep(2);
		}
		if (rows < ROWS) {
			fprintf(stderr,
"%d Lines per screen (termcap 'li' or LINES) less than %d (TERM = '%s')\n",
				rows,ROWS,getenv("TERM"));
			rows = ROWS;
			fprintf(stderr,"Will assume it to be %d\n",ROWS);
			sleep(2);
		}

		/* FETCH THE NUMBER OF CHARACTERS PER LINE */
		/* FROM TERMCAP OR FROM COLUMNS ENVIRONMENT VARIABLE */
		columns = tgetnum("co");
		if (columns == -1) {
			fprintf(stderr,
"Columns per line (termcap 'co' or COLUMNS) was not found (TERM = '%s')\n",
				getenv("TERM"));
			columns = COLUMNS;
			fprintf(stderr,"Will assume it to be %d\n",COLUMNS);
			sleep(2);
		}
		if (columns < COLUMNS) {
			fprintf(stderr,
"%d Columns per line (termcap 'co' or COLUMNS) less than %d (TERM = '%s')\n",
				columns,COLUMNS,getenv("TERM"));
			columns = COLUMNS;
			fprintf(stderr,"Will assume it to be %d columns\n",
				COLUMNS);
			sleep(2);
		}

		/* FETCH THE AUTO MARGIN FLAG AND THE EOL WRAP FLAG */
		automargin = tgetflag("am");
		eolwrap = tgetflag("xn");

		/* FETCH HOME CURSOR SEQUENCE */
		ho_or_cm = 1;
		if ((cp = (char *) tgetstr("ho",0)) == NULL) {
		   /* TRY FOR THE CURSOR MOVEMENT SEQUENCE */
		   if ((cp = (char *) tgetstr("cm",0)) == NULL) {
			/* TRY FOR UP CURSOR SEQUENCE */
		        if ((cp = (char *) tgetstr("up",0)) == NULL){
			   fprintf(stderr,
	    "'ho','cm' or 'up' was not found in termcap entry (TERM = '%s')\n",
				   getenv("TERM"));
			   ho[0] = '\033';
			   ho[1] = 'H';
			   ho[2] = '\0';
			   fprintf(stderr,"Will set it to 'ESC-H'\n");
			   sleep(2);
			} else {
				/* SEQUENCE MOVES CURSOR UP */
				ho_or_cm = 0;
				strcpy(ho,cp);
			}
		   } else {
			strcpy(fmt,cp);
			strcpy(ho,tparm(fmt,0,0));
			if (strlen(ho) <= 0){
			   fprintf(stderr,
				"'cm' was bad in termcap entry: TERM = '%s'\n",
				   getenv("TERM"));
			   ho[0] = '\033';
			   ho[1] = 'H';
			   ho[2] = '\0';
			   fprintf(stderr,"Will set it to 'ESC-H'\n");
			   sleep(2);
		   	}
		   }
		} else
			strcpy(ho,cp);

		/* INSURE THE SEQUENCE FITS */
		j = 0;
		for (i = 0; i < MAX_ESCAPE; i++)
			if (ho[i] == '\0')
				j = 1;
		if (j == 0) {
			fprintf(stderr,
"'ho', 'cm', or 'up' was > %d char in termcap entry (TERM = '%s')\n",
				MAX_ESCAPE - 1,getenv("TERM"));
			ho[0] = '\033';
			ho[1] = 'H';
			ho[2] = '\0';
			fprintf(stderr,"Will set it to 'ESC-H'\n");
			sleep(2);
		}

		/* HANDLE PAD CHARACTERS */
		stuff_ptr = ho;
		stuff_len = 0;
		strcpy(fmt,ho);
		ho[0] = '\0';
		tputs(fmt,1,stuff_array);
		ho_len = stuff_len;

		/* FETCH CLEAR SCREEN SEQUENCE */
		if ((cp = (char *) tgetstr("cl",0)) == NULL)
		{
			fprintf(stderr,
"Clear screen ('cl') was not found in termcap entry (TERM = '%s')\n",
				getenv("TERM"));
			cl[0] = '\033';
			cl[1] = 'H';
			cl[2] = '\033';
			cl[3] = 'J';
			cl[4] = '\0';
			fprintf(stderr,"Will set it to 'ESC-H ESC-J'\n");
			sleep(2);
		} else
			strcpy(cl, cp);

		/* INSURE THE SEQUENCE FITS */
		j = 0;
		for (i = 0; i < MAX_ESCAPE; i++)
			if (cl[i] == '\0')
				j = 1;
		if (j == 0) {
			fprintf(stderr,
			"'cl' was > %d char in termcap entry (TERM = '%s')\n",
				MAX_ESCAPE - 1,getenv("TERM"));
			cl[0] = '\033';
			cl[1] = 'H';
			cl[2] = '\033';
			cl[3] = 'J';
			cl[4] = '\0';
			fprintf(stderr,"Will set it to 'ESC-H ESC-J'\n");
			sleep(2);
		}

		/* HANDLE PAD CHARACTERS */
		stuff_ptr = cl;
		stuff_len = 0;
		strcpy(fmt,cl);
		cl[0] = '\0';
		tputs(fmt,1,stuff_array);
		cl_len = stuff_len;
	}

	/* ALLOCATE SPACE FOR ARRAYS USED BY update_crt() */
	buffer_size = (rows * (COLUMNS + 2)) +
		      (ho_or_cm ? MAX_ESCAPE : rows * MAX_ESCAPE);
	if ((int) (buffer = (char *) calloc(buffer_size, sizeof(char))) == NULL) {
		perror("calloc buffer");
		exit(-1);
	}
	if ((int) (current_screen = (char *) calloc((rows * COLUMNS),
						    sizeof(char))) == NULL) {
		perror("calloc current_screen");
		exit(-1);
	}
	if ((int) (next_screen = (char *) calloc((rows * COLUMNS),
						 sizeof(char))) == NULL) {
		perror("calloc next_screen");
		exit(-1);
	}

	/* CLEAR THE SCREEN */
	fdout = fileno(stdout);
	clear_screen();

	/* PUT TTY IN RAW MODE */
	fdin = fileno(stdin);
	if (tty_raw()) {
		fprintf(stderr,"tty_raw error\n");
		abnorm_exit(-1);
	}

	/* INSTALL THE SIGNAL HANDLER ROUTINE */
	interactive_vec.sv_mask = 0;
	interactive_vec.sv_onstack = 0;
	interactive_vec.sv_handler = interactive_signals;

	if ((sigvector(SIGHUP,&interactive_vec,0) == -1) ||
	    (sigvector(SIGSEGV,&interactive_vec,0) == -1) ||
	    (sigvector(SIGINT,&interactive_vec,0) == -1) ||
	    (sigvector(SIGQUIT,&interactive_vec,0) == -1) ||
	    (sigvector(SIGTERM,&interactive_vec,0) == -1))
	{
		perror("sigvector");
		abnorm_exit(-1);
	}

	/* INITIALIZE THE VALUES FOR SELECT */
	timeout.tv_sec = update_interval;
	timeout.tv_usec = 0;
	savefds = readfds = 1 << fdin;

	/* ENTER THE MONITOR LOOP */
	while (not_done && no_error)
	{
		/* BUMP ITERATION COUNTER */
		iteration += 1;

		/* START OUT WITH A CLEAR SCREEN */
		nscr_pt = next_scrni;
		next_end = &next_scrni[(rows * COLUMNS) / 4];
		while (nscr_pt < next_end)
			*nscr_pt++ = 0x20202020;

		/* PUT TITLE/DATE ON THE SCREEN */
		time_now = time(0);
		atf(0, 0, "%.24s  %8s HP-UX monitor ",
			ctime(&time_now), name.machine);

		/* FETCH CURRENT TIME */
		if (gettimeofday(&current_time,0) == -1)
		{
			perror("gettimeofday");
			no_error = 0;
			break;
		}

		/* DECODE MODE CHARACTER AND EXECUTE IT */
		max_screen = 1;
		in_screen = 1;
		for (mode=modes; mode->letter && cmd!=mode->letter; mode++)
			/* do nothing */;

		addf("- %s", mode->label);
		largest_line_number = 0;
		(*mode->routine)();
		show_page_number();
		last_cmd = cmd;

		/* SAVE PREVIOUS TIME */
		previous_time = current_time;

		/* THROW IT UP ON TO THE CRT */
		update_crt();
		if (no_error == 0)
			break;

		/* LOOK FOR COMMAND CHARACTER FOR UPDATE INTERVAL */
		readfds = savefds;
		if (select(1, &readfds,0,0, &timeout) == -1)
			fprintf(stderr,"Unable to select stdin errno = %d\n",
				errno);
		if (readfds==0)
			continue;
		i = read(fdin, &cmd,1);
		if (i == -1) {
			perror("read");
			no_error = 0;
			break;
		}

		if (i==0)		/* still in cooked mode? */
			cmd='\n';

		/* HANDLE LOWERCASE AS UPPERCASE */
		if ((cmd >= 'a') && (cmd <= 'z'))
			cmd = cmd - 'a' + 'A';

		/* Return in help mode goes to previous mode */
		if (last_cmd=='?' && (cmd=='\n' || cmd=='\r'))
			cmd = help_target;

		/* HANDLE COMMAND KEY */
		switch (cmd) {
		case 12:
			/* CONTROL-L REPAINTS THE SCREEN */
			clear_screen();
			cmd = last_cmd;
			break;
		
		case 15:
			/* CONTROL-O -> ENTER OS DESIGNER MODE */
			osdesignermode = 1;
			cmd = last_cmd;
			break;

		case 21:
			/* CONTROL-U -> EXIT OS DESIGNER MODE */
			osdesignermode = 0;
			cmd = last_cmd;
			break;

#ifdef SIGTSTP
		case 26:
			update_crt();
			clear_screen();
			if (tty_normal()) {
				fprintf(stderr,"tty_normal error\n");
				exit(-1);
			}
			kill(getpid(),SIGTSTP);
			if (tty_raw()) {
				fprintf(stderr,"tty_raw error\n");
				abnorm_exit(-1);
			}
			cmd = last_cmd;
			break;
#endif  /* SIGTSTP */

		case '!':
			clear_screen();
			if (tty_normal()) {
				fprintf(stderr,"tty_normal error\n");
				exit(-1);
			}
			invoke_shell();
			if (tty_raw()) {
				fprintf(stderr,"tty_raw error\n");
				abnorm_exit(-1);
			}
			clear_screen();
			cmd = last_cmd;
			break;
		
		case 'C':
		case 'I':
		case 'G':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'R':
		case 'S':
		case 'T':
		case 'V':
			/* IF WE ARE ENTERING NEW MODE, CLEAR */
			if (cmd != last_cmd)
				clear_screen();
			break;

		case '-':
		case 'B':
			/* SCROLL BACKWARD A SCREEN */
			if (mode->screen_number > 0)
				    mode->screen_number--;
			cmd = last_cmd;
			break;

		case 'D':
			/* DUMP CURRENT SCREEN TO PRINTER */
			dump_to_printer(current_screen);
			cmd = last_cmd;
			break;

		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			/* Go to screen <n> */
			if (cmd-'0' <= max_screen)
			    mode->screen_number = cmd-'0'-1;	/* 0..n-1 */
			cmd = last_cmd;
			break;

		case '+':
		case 'F':
		case ' ':
			/* SCROLL FORWARD A SCREEN */
			/* SCROLL BACKWARD A SCREEN */
			if (mode->screen_number + 2 <= max_screen)
				    mode->screen_number++;
			cmd = last_cmd;
			break;

		case 'H':
			/* HALT UNTIL A "R" IS TYPED */
			atf(73, 0, " HALTED");
			update_crt();
			do
			{
				int flags;

				/* Turn off NDELAY temporarily */
				flags = fcntl(fdin, F_GETFL, 0);
				fcntl(fdin, F_SETFL, flags & ~O_NDELAY);
				i = read(fdin, &cmd,1);
				fcntl(fdin, F_SETFL, flags);
				/* ALLOW D OR Q COMMANDS */
				if ((cmd == 'd') || (cmd == 'D'))
					dump_to_printer(current_screen);
				if ((cmd == 'q') || (cmd == 'Q')) {
					not_done = 0;
					break;
				}
			}
			while ((cmd != 'r') && (cmd != 'R'));
			cmd = last_cmd;
			break;

		case 'P':
			/* SELECT PROCESS OF INTEREST */
			if (tty_normal()) {
				fprintf(stderr,"tty_normal error\n");
				exit(-1);
			}
			do
			{
				clear_screen();
				if (Spid < 1)
				    atf(0,0,
"Enter the process ID of the process to scan (greater than 0!) [%d]: ",Spid);
				else
				    atf(0,0,
"Enter the process ID or name of the process to scan [%d]: ",Spid);
				update_crt();
				read_cnt = read(fdin,read_str,
						sizeof(read_str));
				if (read_cnt > 1) {
					if (isdigit(read_str[0]))
						Spid = atoi(read_str);
					else
						Spid = proc_to_pid(read_str);
				}
			} while (Spid < 1);

			if (tty_raw()) {
				fprintf(stderr,"tty_raw error\n");
				abnorm_exit(-1);
			}
			clear_screen();
			last_cmd = 'P'; /* FORCE INITIALIZE */
			cmd = 'S';
			break;

		case 'Q':
			/* QUIT THE MONITOR */
			not_done = 0;
			break;

		case 'U':
			/* GO INTO COOKED MODE TO READ VALUES */
			if (tty_normal()) {
				fprintf(stderr,"tty_normal error\n");
				exit(-1);
			}

			/* PROMPT FOR UPDATE INTERVAL */
			do
			{
				clear_screen();
				atf(0,0,
		  "Enter the update interval in seconds [%d]: ",
					update_interval);
				update_crt();
				read_cnt = read(fdin,read_str,
						sizeof(read_str));
				if (read_cnt > 1)
				update_interval = atoi(read_str);
			} while (update_interval < 1);
			timeout.tv_sec = update_interval;

			/* PROMPT FOR HARD COPY DEVICE */
			clear_screen();
			atf(0,0,
		"Enter the hardcopy device for lp -d option [%s]: ",
				hardcopy);
			update_crt();
			read_cnt = read(fdin,read_str,
					sizeof(read_str));
			if (read_cnt > 1) {
			    for (i = 0; i < sizeof(read_str); i++)
				/* TRUNCATE STRING? */
				if ((read_str[i] <= ' ') ||
				    (read_str[i] == '\''))
				    read_str[i] = '\0';
			    strcpy(hardcopy, read_str);
			}

			/* PROMPT FOR T SCREEN FILTER */
			if ((!in_screen) || (last_cmd == 'T')) {
			  mode->screen_number = 0;
			  read_str[0] = t_filter;
			  do
			  {
				clear_screen();
				atf(0,0,
"Enter T screen process filter (All, Non-zero CPU usage, or Username [%c]: ",
					t_filter);
				update_crt();
				read_cnt = read(fdin,read_str,
						sizeof(read_str));
				if (read_cnt > 1)
					t_filter = read_str[0];
				if ((t_filter >= 'a') &&
				    (t_filter <= 'z'))
				    t_filter = t_filter - 'a' + 'A';
			  } while ((t_filter != 'A') &&
				   (t_filter != 'N') &&
				   (t_filter != 'U'));
			  if (t_filter == 'U') {
			    /* PROMPT FOR USER NAME */
			    clear_screen();
			    atf(0,0,
	     "Enter the username for T screen process filter [%s]: ",
				    t_user);
			    update_crt();
			    read_cnt = read(fdin,read_str, 
					    sizeof(read_str));
			    for (i = 0; i < sizeof(read_str); i++)
				if (read_str[i] == '\n')
				    /* STRIP TERMINATING LF */
				    read_str[i] = 0;
			    if (read_cnt > 1)
				    strcpy(t_user,read_str);
			  }
			}

			/* PROMPT FOR REVERSE SORT */
			if ((!in_screen) || (last_cmd == 'K') ||
			    (last_cmd == 'R')) {
			  clear_screen();
			  atf(0,0,
				"Reverse sorting order (yes/no) [n]: ");
			  update_crt();
			  read_cnt = read(fdin,read_str,
					  sizeof(read_str));
			  if ((read_cnt > 1) &&
			      ((read_str[0] == 'y') ||
			       (read_str[0] == 'Y')))
				  reverse = -reverse;
			}

			/* PROMPT FOR R SCREEN SORT KEY */
			if ((!in_screen) || (last_cmd == 'R')) {
			  read_str[0] = cmp_type;
			  do
			  {
				clear_screen();
				atf(0,0,
"Enter R screen sort key (Hostname Loadaverage upTime Usercount) [%c]: ",
					cmp_type);
				update_crt();
/* read_cct() ROUTINE USES reverse, SO LET IT RE-SORT WHILE USER THINKS */
				cct_ok = read_cct();
				read_cnt = read(fdin, read_str,
						sizeof(read_str));
				if (read_cnt > 1)
					cmp_type = read_str[0];
				if ((cmp_type >= 'a') &&
				    (cmp_type <= 'z'))
				    cmp_type = cmp_type - 'a' + 'A';
			  } while ((cmp_type != 'H') &&
				   (cmp_type != 'L') &&
				   (cmp_type != 'T') &&
				   (cmp_type != 'U'));

			  /* SET UP NEW SORT KEY PROCEDURE */
			  switch (cmp_type) {
			  case 'H': cmp = hcmp; break;
			  case 'L': cmp = lcmp; break;
			  case 'T': cmp = tcmp; break;
			  case 'U': cmp = ucmp; break;
			  }
			}

			/* PROMPT FOR LAN DEVICE */
			if ((!in_screen) || (last_cmd == 'G') ||
			    (last_cmd == 'L')) {
			  clear_screen();
			  atf(0,0, "Enter the LAN device [%s]: ",
				  lan_name);
			  update_crt();
			  read_cnt = read(fdin,read_str,
					  sizeof(read_str));
			  for (i = 0; i < sizeof(read_str); i++)
			      if (read_str[i] == '\n')
				  /* STRIP TERMINATING LF */
				  read_str[i] = 0;
			  if (read_cnt > 1)
				  strcpy(lan_name, read_str);
			  close(lan_fd);  /* Close old LAN */
			  mode_l_init();  /* Open up new LAN */
			}
			
			/* PROMPT FOR N SCREEN SYSTEMS */
			if ((!in_screen) || (last_cmd == 'N')) {
				/* GET NAMES */
				fetch_n_names();
				if (syscount == 0)
					last_cmd = 'x';
			}
			
			/* ALL DONE */
			if (tty_raw()) {
				fprintf(stderr,"tty_raw error\n");
				abnorm_exit(-1);
			}
			clear_screen();
			cmd = last_cmd;
			break;

		default:
			/* WAS IT VINTR OR VEOF? */
			if ((cmd  == veof) || (cmd == vintr)) {
				not_done = 0;
				break;
			}

			/* EVERYTHING ELSE */
			break;
		}
	}

	/* SHUTDOWN remsh PROCESSES? */
	if (syscount)
		kill_remshs();

	/* CLEAR THE SCREEN */
	if (no_error)
		clear_screen();

	/* RESTORE TTY MODE */
	tty_normal();

	/* CLOSE DEVICE FILES */
done:
	close(lan_fd);
	close(kmem_fd);
	close(mem_fd);

	/* EXIT */
	if (no_error)
		return  0;
	else
		return  -1;

} /* main */



#include <varargs.h>
/*
 *   atf -- do an sprintf in next_screen at the given x,y location.
 *
 *   Calling sequence:
 *	atf(x,y, format, arg1, arg2...);
 *
 *   Example:
 *	atf(60, i++, "a%d=0x%08x", reg, u.u_rsave.val[7+reg-1]);
 */
/*VARARGS3*/
atf(x, y, fmt, va_alist)
char *fmt;
va_dcl
{
	va_list args;
	int len;
	char buf[10240];

	va_start(args);
	len = vsprintf(buf, fmt, args);
	va_end(args);

	current_x = x;
	current_y = y;
	add_string(buf);
	return len;
}


/*VARARGS1*/
addf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list args;
	int len;
	char buf[10240];

	va_start(args);
	len = vsprintf(buf, fmt, args);
	va_end(args);

	add_string(buf);
	return len;
}

int scroll_x=9999, scroll_y=9999;

add_string(buf)
register char *buf;
{
	register char c;
	register int y;

	while (c = *buf++) {
		if (c=='\n') {
			current_x = 0;
			current_y++;
		}
		else if (c=='\t') {
			while (++current_x & 07);
		}
		else {
			if (current_y > largest_line_number)
				largest_line_number = current_y;

			/* Should this character really be printed? */
			if (current_y < scroll_y
			    || current_x < scroll_x)
				next_screen[COLUMNS*current_y+current_x] = c;
			else if ( (current_y - scroll_y)/
				    (rows - scroll_y) ==
			   mode->screen_number) {
				y = current_y - mode->screen_number * 
				    (rows - scroll_y);
				next_screen[COLUMNS*y+current_x] = c;
			}
			current_x++;
		}
	}
}

start_mode()
{
	scroll_x = 0;
	scroll_y = 0;
}


start_scrolling_here()
{
	scroll_x = current_x;
	scroll_y = current_y;
}


/* Display summary information */
show_page_number()
{
	int z;
	char buf[20];

	if (scroll_y == 9999)		/* does this screen do scrolling? */
		return;			/* no, print nothing */

	z = largest_line_number - scroll_y;
	max_screen = (z / (rows - scroll_y)) +
		     ((z % (rows - scroll_y)) != 0);

	if (max_screen == 0)
		max_screen = 1;

	sprintf(buf, "%d of %d", mode->screen_number + 1, max_screen);
	atf(80-1-strlen(buf), 0, "%s", buf);
}

/*
 * Translate a process name (e.g., "vi") to a pid.
 * Return -1 if no matching process can be found.
 */
int
proc_to_pid(name)
char *name;
{
	int idx;
	struct pst_status psbuf;
	char *p;

	/* The string is newline-terminated, make it null terminated */
	for (p=name; *p; p++)
		if (*p=='\n')
			*p = '\0';

	/* This is inefficient, but this is all at human speed anyway. */
	idx=0;
	while (pstat(PSTAT_PROC, &psbuf, sizeof(psbuf), 1, idx) > 0) {
		if (strcmp(psbuf.pst_ucomm, name)==0)
			return psbuf.pst_pid;
		idx = psbuf.pst_idx+1;
	}
	return -1;			/* failure */
}
