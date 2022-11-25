#ifndef lint
static char * version = "@(#) $Revision: 72.2 $";
#endif
/*
 * REPORT SYSTEM SWAP SPACE INFORMATION
 *
 * (To the best of the author's knowledge, this program was developed without
 * use of or reference to AT&T, BSD, or other non-HP proprietary software.)
 *
 * USAGE:  See the manual entry, or run with -? for a usage summary, or see
 * below.  Returns 0 if all is well, 1 if any errors or warnings are reported.
 *
 * COMPILATION:  This version of this program can only compile and run on
 * release 8.X.  (See earlier in the DUI RCS file for a 7.0 version.)
 * Compilation is vanilla *except* on S800, add -lIO.  Unfortunately this
 * program is *not* portable between S700 and S800; it must be separately
 * compiled for each.
 *
 * POSSIBLE ENHANCEMENT:  It would be easy to allow one or more non-option
 * arguments (device or file system names), and only print "dev" or "fs" lines
 * whose NAME field matches the arguments, then warn about any arguments not
 * matched.  This is similar to bdf(1).  It was decided this was not worth
 * implementing.
 *
 * CONVENTIONS:
 *
 * - Pointer variable names end in one "p" per level of indirection.
 *
 * - Exception:  (char *) and (char []) types, that is, strings, do not
 *   necessarily end in "p".  A generic char pointer is named "cp".  Variables
 *   of type (char **) end in a single p.
 *
 * - Exception:  the well known name "char ** argv".
 *
 * - Line lengths less than 80 columns, except long strings are not broken with
 *   backslash because that's even harder to manage.  Message parameters to
 *   Error() begin on the same line for easy finding.
 *
 * - Error messages might exceed one line when emitted, but no effort is made
 *   to wrap them nicely.
 */

#include <sys/types.h>		/* for various types			*/
#include <sys/param.h>		/* for MAXPATHLEN, DEV_BSIZE, NBPG	*/
#include <sys/sysmacros.h>	/* for major(), minor() */
#include <sys/vmmac.h>		/* for dtop() */
#ifdef __hp9000s800
#include <sys/utsname.h>	/* for uname() and related		*/
#endif
#include <sys/stat.h>		/* for stat() and related		*/
#include <sys/conf.h>		/* for struct swdevt			*/
#include <sys/vfs.h>		/* for statfs() and related		*/
#include <sys/swap.h>		/* for various structures		*/
#include <sys/fs.h>		/* for struct fs                        */

#if __hp9000s800 && (! __hp9000s700)
#include <sys/libIO.h>		/* to support MapMinor()		*/
#endif

#include <stdio.h>
#include <string.h>		/* for str*()				*/
#include <nlist.h>		/* for struct nlist			*/
#include <fcntl.h>		/* for O_RDONLY				*/
#include <unistd.h>		/* for lseek()				*/
#include <cluster.h>		/* for getcccid() and related		*/

extern	void	exit();		/* not in any header file		*/
extern	int	errno;		/* not in any header file		*/
extern	cnode_t	cnodeid();	/* not in any header file		*/


/************************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	PROC				/* null; easy to find procs */
#define	REG	register
#define	FALSE	0
#define	TRUE	1
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)

char * usage [] = {
    "usage: %s [-mtadfhqw]",
    "",
    "-m print AVAIL, USED, FREE, and RESERVE in Mb (rounded to nearest 1024^2,",
    "   units compatible with /etc/disktab)",
    "-t add a totals line with a TYPE of \"tot\"",
    "-a show all device swap areas, including those configured but unused",
    "-d print information about device swap area allocations only",
    "-f print information about file system swap areas allocations only",
    "-h print information about unallocated swap space held for existing processes",
    "-q quiet, just print a total block count for all available swap space",
    "-w with -d, warn about wasted space (extra fraction of a swap chunk) in device",
    "   swap areas; wasted space is listed as USED",
    "",
    "By default, lists information on all device and file system swap areas on the",
    "system, and swap space on hold for existing processes but not yet allocated",
    "on any device or file system, in units of 1024 bytes.  Swap areas are found in",
    "kernel memory.",
    "",
    "Warning:  File system swap space available can shrink if the file system grows.",
    "Also, file system swap space is taken in swap chunks.  This is reflected in",
    "df(1) and bdf(1) output as lost free space, but only swap space allocated to",
    "processes is reflected by this program.",
    CPNULL,
};


char *	myname;				/* how program was invoked	*/

int	mflag = FALSE;			/* -m (Mb) option		*/
int	tflag = FALSE;			/* -t (totals line) option	*/
int	aflag = FALSE;			/* -a (include all) option	*/
int	dflag = FALSE;			/* -d (device swap only) option	*/
int	fflag = FALSE;			/* -f (fs swap only) option	*/
int	hflag = FALSE;			/* -h (held space only) option	*/
int	qflag = FALSE;			/* -q (quiet) option		*/
int	wflag = FALSE;			/* -w (wasted space) option	*/

long	total_avail   = 0;		/* for totals line		*/
long	total_used    = 0;		/* (units depend on mflag)	*/
long	total_free    = 0;
long	total_reserve = 0;

long	pages_alloc   = 0;		/* total allocated for dev + fs	*/

#define	OKEXIT	0
#define	ERREXIT	1

#define	NOERRNO	0			/* for Error()			*/
#define	ERROR	0
#define	WARNING	1

int	exitcode = OKEXIT;		/* to return from the program	*/


/************************************************************************
 * NAME LIST STRUCTURE:
 *
 * This is used to read kernel values.
 */

struct nlist nl[] = {		/* initialize n_name field only */
#ifdef __hp9000s300
	{ "_swdevt"	 },	/* location of tables in kernel	*/
	{ "_fswdevt"	 },
	{ "_nswapdev"	 },
	{ "_nswapfs"	 },
	{ "_swchunk"	 },	/* values from kernel		*/
	{ "_swapspc_cnt" },
	{ "_swapspc_max" },
#endif
#ifdef __hp9000s800
	{ "swdevt"	},	/* location of tables in kernel	*/
	{ "fswdevt"	},
	{ "nswapdev"	},
	{ "nswapfs"	},
	{ "swchunk"	},	/* values from kernel		*/
	{ "swapspc_cnt" },
	{ "swapspc_max" },
#endif
	{ CPNULL },
};

#define	NL_SWDEVT	0	/* index of swdevt entry	*/
#define	NL_FSWDEVT	1	/* index of fswdevt entry	*/
#define	NL_NSWAPDEV	2	/* index of nswapdev entry	*/
#define	NL_NSWAPFS	3	/* index of nswapfs entry	*/
#define	NL_SWCHUNK	4	/* index of swchunk entry	*/
#define	NL_SWAPSPC_CNT	5	/* index of swapspc_cnt entry	*/
#define	NL_SWAPSPC_MAX	6	/* index of swapspc_max entry	*/


/************************************************************************
 * OUTPUT FORMAT:
 *
 * There are a number of permutations of title strings depending on the -d, -f,
 * -h, and -m options.
 */

#define	TITLE1_KB	"          Kb      Kb      Kb   PCT  START/      Kb"
#define	TITLE1_KB_DEV	"          Kb      Kb      Kb   PCT              Kb"
#define	TITLE1_KB_FS	"          Kb      Kb      Kb   PCT      Kb      Kb"
#define	TITLE1_KB_HELD	"          Kb      Kb      Kb"

#define	TITLE1_MB	"          Mb      Mb      Mb   PCT  START/      Mb"
#define	TITLE1_MB_DEV	"          Mb      Mb      Mb   PCT              Mb"
#define	TITLE1_MB_FS	"          Mb      Mb      Mb   PCT      Mb      Mb"
#define	TITLE1_MB_HELD	"          Mb      Mb      Mb"

#define	TITLE2		"TYPE   AVAIL    USED    FREE  USED   LIMIT RESERVE  PRI  NAME"
#define	TITLE2_DEV	"TYPE   AVAIL    USED    FREE  USED   START RESERVE  PRI  NAME"
#define	TITLE2_HELD	"TYPE   AVAIL    USED    FREE"

/*
 * The form_* strings are a bit of a mess because of S300/800 differences.
 * Their meanings are:
 *
 * *_dev	device swap area entries
 * *_un		unused device swap area entries
 * *_fs		file system swap area entries
 * *_held	held space entry
 */

#if __hp9000s300 || __hp9000s700
char *	form_dev    = "dev  %7d %7d %7d %4.0f%% %7d       - %4d  %s\n";
char *	form_dev_un = "dev        0       0       0  100%% %7d       - %4d  %s unused\n";
#endif

#if __hp9000s800 && (! __hp9000s700)	/* sw_start is unavailable */
char *	form_dev    = "dev  %7d %7d %7d %4.0f%%       -       - %4d  %s\n";
char *	form_dev_un = "dev        0       0       0  100%%       -       - %4d  %s unused\n";
#endif

char *	form_fs	      = "fs   %7d %7d %7d %4.0f%% %7d %7d %4d  %s\n";
char *	form_fs_nolim = "fs   %7d %7d %7d %4.0f%%    none %7d %4d  %s\n";
char *	form_held     = "hold       0 %7d %7d\n";
char *	form_total    = "tot  %7d %7d %7d %4.0f%%       - %7d    -\n";
char *	form_total_h  = "tot  %7d %7d %7d\n";
char *	form_quiet    = "%d\n";


/************************************************************************
 * FUNCTION TYPES:
 */

void	main();
void	RemoteSwap();		/* check and report on remote swap	*/
void	DeviceSwap();		/* summarize device swap		*/
char *	DeviceName();		/* map device ID to file name		*/
#if __hp9000s800 && (! __hp9000s700)
void	MapMinor();		/* map device ID minor MI to LU value	*/
#endif
void	FileSysSwap();		/* summarize file system swap		*/
void	HeldSwap();		/* summarize swap space on hold		*/
int	GetNamelist();		/* get nl[] info from kernel memory	*/
char *	WhatKernel();		/* return name of active kernel		*/
long	GetSwchunk();		/* get swchunk value from kernel memory	*/
int	GetKernelValue();	/* read value from kernel memory	*/
long	Blocks();		/* convert bytes to 1Kb or 1Mb blocks	*/
float	Percent();		/* figure percentage value		*/
void	Usage();		/* give a usage message			*/
void	Error();		/* handle an error			*/


/************************************************************************
 * M A I N
 *
 * Check invocation, check for remote swap on a cnode, determine Series and
 * system version, check device and file system swap space, print total.
 */

PROC void main (argc, argv)
	int	argc;
	char **	argv;
{
extern	int	optind;			/* from getopt()	*/
REG	int	option;			/* option "letter"	*/
#ifdef __hp9000s800
	struct utsname unamebuf;	/* to get system info	*/
#endif

/*
 * PARSE OPTIONS:
 */

	myname = *argv;

	while ((option = getopt (argc, argv, "mtadfhqw")) != EOF)
	{
	    switch (option)
	    {
	    case 'm':	mflag = TRUE;	break;
	    case 't':	tflag = TRUE;	break;
	    case 'a':	aflag = TRUE;	break;
	    case 'd':	dflag = TRUE;	break;
	    case 'f':	fflag = TRUE;	break;
	    case 'h':	hflag = TRUE;	break;
	    case 'q':	qflag = TRUE;	break;
	    case 'w':	wflag = TRUE;	break;
	    default:	Usage();
	    }
	}

	if (argc > optind)		/* other arguments given */
	    Usage();

	if (! (dflag || fflag || hflag))	/* none specified */
	    dflag = fflag = hflag = TRUE;	/* set all three  */

/*
 * CHECK IF RUNNING ON CLUSTER NODE WITH REMOTE SWAP:
 */

	RemoteSwap();			/* exits if true */

#ifdef __hp9000s800			/* S700 or S800 */
/*
 * DETERMINE IF RUNNING ON WRONG SYSTEM TYPE:
 *
 * There are header file macro differences, you see...
 */

	if ((uname (& unamebuf) == 0)			/* got info */
#ifdef __hp9000s700		/* compiled on 700, running on 800 */
	 && strncmp (unamebuf.machine, "9000/7", 6))
#else				/* compiled on 800, running on 700 */
	 && strncmp (unamebuf.machine, "9000/8", 6))
#endif
	{
	    Error (NOERRNO, ERROR, "Cannot run on Series type (S700, S800) different than where compiled");
	    exit (ERREXIT);
	}
#endif

/*
 * PRINT OUTPUT HEADER:
 *
 * Use default title line 1 if both -d and -f options, the _DEV title if -d
 * without -f, the _FS title if -f without -d, and the _HELD title only if
 * neither -d or -f.
 *
 * For title line 2 mflag doesn't matter and the _FS title is the same as the
 * default title, so the test is simpler.
 *
 * This is complicated but that's how user interfaces can be.
 */

	if (! qflag)
	{
	    (void) puts (mflag ?
		(dflag ? (fflag ? TITLE1_MB    : TITLE1_MB_DEV) :
			 (fflag ? TITLE1_MB_FS : TITLE1_MB_HELD)) :
		(dflag ? (fflag ? TITLE1_KB    : TITLE1_KB_DEV) :
			 (fflag ? TITLE1_KB_FS : TITLE1_KB_HELD)));

	    (void) puts (fflag ? TITLE2 : dflag ? TITLE2_DEV : TITLE2_HELD);
	}

/*
 * SUMMARIZE DEVICE, FILE SYSTEM, AND HELD SWAP SPACE:
 *
 * These routines increment global pages_alloc and total_* values, so the first
 * two must be called even if not dflag or not fflag.
 */

	if (dflag || hflag)	DeviceSwap();
	if (fflag || hflag)	FileSysSwap();
	if (hflag)		HeldSwap();

	if (qflag)
	{
	    (void) printf (form_quiet, total_avail);
	}
	else if (tflag)
	{
	    if (dflag || fflag)		/* need to print full line */
	    {
		(void) printf (form_total, total_avail, total_used, total_free,
					   Percent (total_used, total_avail),
					   total_reserve);
	    }
	    else
	    {
		(void) printf (form_total_h, total_avail, total_used,
					     total_free);
	    }
	}

	exit (exitcode);
	/* NOTREACHED */

} /* main */


/************************************************************************
 * R E M O T E   S W A P
 *
 * Check if the system is a cluster node with remote swap.  If so, print one
 * line saying the name of the remote server host and exit with OKEXIT;
 * otherwise return.  If unable to read a cnode's entry from clusterconf, issue
 * a warning and return; hope it's local swap.  If unable to read a remote swap
 * server cnode's entry from clusterconf, issue an error and exit non-zero.
 *
 * Apparently the only way to know about local/remote swap is by reading the
 * clusterconf file.  Hope it's accurate...
 */

PROC void RemoteSwap()
{
	cnode_t	local_id;		/* local  host's cnode ID */
	cnode_t	remote_id;		/* remote host's cnode ID */
	struct cct_entry * cctent;	/* one clusterconf entry  */

/*
 * FIND CNODE ENTRY IN CLUSTERCONF:
 */

	if (cnodes ((cnode_t *) NULL) == 0)	/* standalone system */
	    return;

	local_id = cnodeid();

	if ((cctent = getcccid (local_id)) == (struct cct_entry *) NULL)
	{
	    Error (errno, WARNING, "Cannot find local cnode ID %d in cluster configuration file; assuming local swap",
		   local_id);

	    endccent();
	    return;
	}

/*
 * CHECK IF LOCAL SWAP:
 */

	if (local_id == (remote_id = cctent -> swap_serving_cnode))
	{
	    endccent();
	    return;
	}

/*
 * PRINT INFO ABOUT REMOTE SWAP:
 */

	if ((cctent = getcccid (remote_id)) == (struct cct_entry *) NULL)
	{
	    Error (errno, ERROR, "Cannot find swap server cnode ID %d in cluster configuration file",
		   remote_id);

	    exit (ERREXIT);		/* no need to endccent() */
	}

	(void) printf ("remote swap server is cluster node \"%s\"\n",
		       cctent -> cnode_name);

	exit (OKEXIT);			/* no need to endccent() */

} /* RemoteSwap */


/************************************************************************
 * D E V I C E   S W A P
 *
 * Locate the device swap table and nswapdev and swchunk values in system
 * memory using global nl[] and the name list in the booted kernel's virtual
 * file, read each entry in the table, and if not qflag, print one line of
 * information about it.  Increment global pages_alloc in any case, and globals
 * total_* if dflag, in units of 1Kb or 1Mb according to mflag.
 *
 * In case of error, write a message to stderr and do not count the entry.
 */

PROC void DeviceSwap()
{
	struct swdevt swapentry;	/* one entry from system memory	*/
	long	my_nswapdev;		/* number of table entries	*/
	long	swchunk_bytes;		/* swap chunk size (bytes)	*/
	off_t	address;		/* of swdevt entry in /dev/kmem	*/

REG	long	avail;			/* available blocks (Kb or Mb)	*/
REG	long	used;			/* used blocks (Kb or Mb)	*/
REG	long	free;			/* free blocks (Kb or Mb)	*/
REG	long	avail_bytes;		/* available space (bytes)	*/
REG	long	wasted_bytes;		/* wasted space (bytes)		*/

/*
 * LOCATE DEVICE SWAP TABLE, nswapdev, AND swchunk IN SYSTEM MEMORY:
 */

	if (! GetNamelist ("report on device swap space"))
	    return;			/* error message already printed */

	address = nl [NL_SWDEVT] . n_value;

/*
 * GET MAXIMUM NUMBER OF TABLE ENTRIES:
 */

	if (! GetKernelValue ((off_t) (nl [NL_NSWAPDEV] . n_value),
			      sizeof (my_nswapdev), (void *) & my_nswapdev,
			      "number of device swap table entries (nswapdev)"))
	{
	    Error (NOERRNO, ERROR, "Cannot report on device swap areas");
	    return;
	}

/*
 * GET SWAP CHUNK SIZE IN BYTES:
 */

	if ((swchunk_bytes = GetSwchunk()) == 0) /* one err msg already given */
	{
	    Error (NOERRNO, WARNING, "Cannot check for wasted blocks on each device");
	}

/*
 * READ DEVICE SWAP TABLE ENTRIES:
 */

	while (my_nswapdev-- > 0)
	{
	    if (! GetKernelValue (address, sizeof (struct swdevt),
				  (void *) & swapentry,
				  "device swap table entry"))
	    {
		break;			/* error message already printed */
	    }

	    address += sizeof (struct swdevt);

/*
 * SKIP EMPTY OR UNUSED ENTRIES:
 */

	    if (swapentry.sw_dev == -1)		/* empty entry */
		continue;

	    if (! swapentry.sw_enable)		/* unused entry */
	    {
		if (! aflag)			/* don't print it */
		    continue;

		swapentry.sw_nblks = swapentry.sw_nfpgs = 0; /* ensure zeroes */
	    }

/*
 * ACCUMULATE PAGES ALLOCATED; WARN ABOUT WASTED SPACE:
 *
 * Units in kernel memory:
 *
 *	sw_start:  device blocks (never converted)
 *	sw_nblks:  device blocks (convert using DEV_BSIZE)
 *	sw_nfpgs:  swap pages (convert using NBPG)
 *
 * Always deduct wasted pages (fractional swap chunks) from pages_alloc, and
 * optionally warn about them.
 */

	    avail_bytes	 = swapentry.sw_nblks * DEV_BSIZE;
	    wasted_bytes = swchunk_bytes ? (avail_bytes % swchunk_bytes) : 0;

	    pages_alloc += ((avail_bytes - wasted_bytes) / NBPG)
			   - swapentry.sw_nfpgs;

	    if (! dflag)			/* no need to go further */
		continue;

	    if (wflag && wasted_bytes)
	    {
		Error (NOERRNO, WARNING, "Device swap area \"%s\":  Bytes wasted in fractional swap chunk:  %d",
		       DeviceName (swapentry.sw_dev), wasted_bytes);
	    }

/*
 * CONVERT BLOCKS; COMPUTE AND ACCUMULATE VALUES FOR ONE ENTRY:
 *
 * Per the manual entry, if avail is not an integer number of pages, any left
 * over (wasted) blocks appear to be used, not free.
 */

	    total_avail += (avail = Blocks ((long) swapentry.sw_nblks,
					    DEV_BSIZE));

	    total_free  += (free  = Blocks ((long) (swapentry.sw_nfpgs * NBPG
					    / DEV_BSIZE), DEV_BSIZE));

	    total_used  += (used  = avail - free);

	    if (qflag)				/* no need to print anything */
		continue;

/*
 * PRINT ONE ENTRY:
 */

	    {
		if (swapentry.sw_enable)	/* enabled entry */
		{
		    (void) printf (form_dev,
				   avail, used, free, Percent (used, avail),
#if __hp9000s300 || __hp9000s700
				   swapentry.sw_start,
#endif
				   swapentry.sw_priority,
				   DeviceName (swapentry.sw_dev));
		}
		else
		{
		    (void) printf (form_dev_un,
#if __hp9000s300 || __hp9000s700
				   swapentry.sw_start,
#endif
				   swapentry.sw_priority,
				   DeviceName (swapentry.sw_dev));
		}
	    }
	} /* while */

} /* DeviceSwap */


/************************************************************************
 * D E V I C E   N A M E
 *
 * Given a device specifier (dev_t) for a block special device used for
 * swapping, map its minor value if required (S800 9.0 still requires it
 * because the dev field from pstat() is *not* mapped), and then call devnm(3)
 * to map it to a device name.  In case of any error or failure to find the
 * device, return a suitable string showing dev_id numerically.
 */

PROC char * DeviceName (dev_id)
	dev_t	dev_id;			/* device ID to find	*/
{
static	char	devicename [MAXPATHLEN + 1];	/* to return	*/

#if __hp9000s800 && (! __hp9000s700)
/*
 * MAP MINOR FIELD ON S800 8.0:
 */
	MapMinor (& dev_id);
#endif

/*
 * MAP DEVICE ID TO NAME:
 */

	switch (devnm (S_IFBLK, dev_id, devicename, MAXPATHLEN + 1,
		       /* cache = */ 1))
	{
	case 0: case -3:		/* name OK or truncated */
	    break;

	default:			/* return consolation string */
	    (void) sprintf (devicename, "%d,0x%06x", major (dev_id),
						     minor (dev_id));
	    break;
	}

	return (devicename);

} /* DeviceName */


#if __hp9000s800 && (! __hp9000s700)

/************************************************************************
 * M A P   M I N O R
 *
 * Given a pointer to a device ID, map the MI (manager index) in the minor
 * number to an LU (logical unit).
 *
 * Too bad this is necessary on S800s starting with 8.0...  I don't really
 * understand the code, I stole it from SAM and cleaned it up.
 */

#define	LU_MASK		0x0000ff00		/* logical unit in minor */
#define	LU_SHIFT	8
#define	MI(dev)		((dev & LU_MASK) >> LU_SHIFT)	/* manager index */

PROC void MapMinor (devidp)
	dev_t *		devidp;		/* device ID to modify	*/
{
	io_node_type	io_node;	/* to look up I/O node	*/
	int		status;		/* from system calls	*/
	int		mgr_index;	/* value to map		*/
	int		type;		/* type of I/O search	*/
	int		try = 0;	/* number of tries	*/

/*
 * INITIALIZE:
 */

	if ((status = io_init (O_RDONLY)) != SUCCESS)
	{
	    Error (errno, ERROR, "Cannot do io_init() to map device ID manager index to logical unit");
	    return;
	}

	io_node.b_major	= major	(*devidp);
	mgr_index	= MI	(*devidp);

/*
 * SEARCH I/O TREE FOR THIS MI'S ENTRY:
 */

	do {
	    type = SEARCH_FIRST;

	    do {
		if ((status = io_search_tree (type, KEY_B_MAJOR, QUAL_NONE,
					      & io_node)) < 0)
		{
		    if (status == STRUCTURES_CHANGED)
			break;

		    io_end();
		    return;
		}

/*
 * FOUND ENTRY FOR THIS MI:
 */

		if (io_node.mgr_index == mgr_index)
		{
		    if (io_node.lu == NONE)
		    {
			Error (NOERRNO, ERROR, "Cannot map device ID manager index; no logical unit found");
			/* the device ID will be printed numerically */
		    }
		    else
		    {
			*devidp = (*devidp & ~LU_MASK)
				| (io_node.lu << LU_SHIFT);
		    }

		    io_end();
		    return;
		}

		type = SEARCH_NEXT;
	    }
	    while (status > 0);
	}
	while ((status == STRUCTURES_CHANGED) && (++try < MAX_TRIES));

	if (status != STRUCTURES_CHANGED)
	{
	    Error (NOERRNO, ERROR, "Cannot map device ID manager index; entry not found in I/O structure");
	}

	io_end();

} /* MapMinor */

#endif /* S800 */


/************************************************************************
 * F I L E   S Y S   S W A P
 *
 * Locate the file system swap table, nswapfs, and swchunk values in system
 * memory using the name list in the booted kernel's virtual file and global
 * nl[], read each entry in the table, and if not qflag, print one line of
 * information about it.  Use statfs(2) to determine the file system's block
 * size.  Increment global pages_alloc in any case, and globals total_* if
 * fflag, in units of 1Kb or 1Mb according to mflag.
 *
 * In case of error, write a message to stderr and do not count the entry.
 *
 * Why not use hidden system call swapfs(2)?  An earlier version of this
 * program did that.  Unfortunately beginning with 8.0 a value from fswdevt
 * (fsw_nfpgs) needed to compute actual used space is not available via
 * swapfs().  Furthermore calling swapfs() requires finding mounted volume
 * names elsewhere, that is, /etc/mnttab, which is not 100% reliable.  Finally,
 * the mount point name recorded in /etc/mnttab (the true mount point) is not
 * necessarily the same as the fsw_mntpoint value in fswdevt, because the
 * latter can be any subdirectory on the volume (whatever name was given to
 * swapon(1M)).
 */

PROC void FileSysSwap()
{
	struct fswdevt swapentry;	/* one entry from system memory	*/
	struct statfs  statfsbuf;	/* FS info about volume		*/
	long	my_nswapfs;		/* number of table entries	*/
	long	swchunk_bytes;		/* swap chunk size (bytes)	*/
	off_t	address;		/* of fswdevt ent in /dev/kmem	*/

REG	long	avail;			/* available blocks (Kb or Mb)	*/
REG	long	used;			/* used blocks (Kb or Mb)	*/
REG	long	free;			/* free blocks (Kb or Mb)	*/
REG	long	limit;			/* limit blocks (Kb or Mb)	*/
REG	long	reserve;		/* reserve blocks (Kb or Mb)	*/
        long    blocksize;              /* blocksize that fsctl reports */

/*
 * LOCATE FILE SYSTEM SWAP TABLE, nswapfs, AND swchunk IN SYSTEM MEMORY:
 */

	if (! GetNamelist ("report on file system swap space"))
	    return;			/* error message already printed */

	address = nl [NL_FSWDEVT] . n_value;

/*
 * GET MAXIMUM NUMBER OF TABLE ENTRIES AND SWAP CHUNK SIZE IN BYTES:
 */

	if ((! GetKernelValue ((off_t) (nl [NL_NSWAPFS] . n_value),
			       sizeof (my_nswapfs), (void *) & my_nswapfs,
			       "number of file system swap table entries (nswapfs)"))
	 || ((swchunk_bytes = GetSwchunk()) == 0))
	{
	    Error (NOERRNO, ERROR, "Cannot report on file system swap areas");
	    return;
	}

/*
 * READ FILE SYSTEM SWAP TABLE ENTRIES:
 */

	while (my_nswapfs-- > 0)
	{
	    if (! GetKernelValue (address, sizeof (struct fswdevt),
				  (void *) & swapentry,
				  "file system swap table entry"))
	    {
		break;			/* error message already printed */
	    }

	    address += sizeof (struct fswdevt);

/*
 * SKIP EMPTY OR UNUSED ENTRIES:
 */

	    if (((swapentry . fsw_mntpoint [0]) == CHNULL)
	     || (! (swapentry . fsw_enable)))
	    {
		continue;
	    }

/*
 * GET FILE SYSTEM'S FS INFO (block size and free blocks):
 */

	    if (statfs (swapentry.fsw_mntpoint, & statfsbuf) < 0)
	    {
		Error (errno, ERROR, "Cannot get file system info for swap on \"%s\"",
		       swapentry.fsw_mntpoint);

		continue;
	    }

	    {
#ifndef UFS_GET_SB
#define UFS_GET_SB 0x21
#endif
		int mntpointfd;
		blocksize = 0;
	        if ((mntpointfd = open (swapentry.fsw_mntpoint, O_RDONLY)) >= 0)
	        {
		    struct fs filesystem;
	            if (fsctl (mntpointfd, UFS_GET_SB, &filesystem,
			sizeof(struct fs)) >= 0)
			    blocksize = filesystem.fs_bsize;
		    close(mntpointfd);
	        }
            }

/*
 * ACCUMULATE PAGES ALLOCATED:
 *
 * Units in kernel memory:
 *
 *	fsw_nfpgs:	swap pages (convert using NBPG)
 *	fsw_allocated:	swap chunks (convert using swchunk_bytes)
 *	fsw_min:	swap chunks (convert using swchunk_bytes)
 *	fsw_limit:	swap chunks (convert using swchunk_bytes)
 *	fsw_reserve:	file system blocks (convert using statfsbuf.f_bsize)
 *
 * Deduct any partially freed blocks from fsw_allocated.
 */

	    used = (swapentry.fsw_allocated * swchunk_bytes) -
		   (swapentry.fsw_nfpgs * NBPG);

	    pages_alloc += used / NBPG;

	    if (! fflag)			/* no need to go further */
		continue;

/*
 * COMPUTE VALUES FOR ONE ENTRY:
 *
 * First figure the free swap space available = file system available to
 * ordinary users - swap reserve, but not less than zero.  Then figure the
 * actual space available = used + free.  Reduce avail to the swapon(1M) limit
 * value if there is one and it's lower, but not less than blocks in use (just
 * in case that somehow exceeds the limit).
 *
 * All this figuring takes place in bytes, not yet converted to Kb or Mb.
 */

	    reserve = swapentry.fsw_reserve *
		(blocksize ? blocksize : statfsbuf.f_bsize);

	    if ((free = (statfsbuf.f_bavail * statfsbuf.f_bsize) - reserve) < 0)
		free = 0;

	    avail = used + free;
	    limit = swapentry.fsw_limit * swchunk_bytes;

	    if ((avail > limit)			/* exceeds limit	*/
	     && (limit > 0)			/* limit is valid	*/
	     && (used  > (avail = limit)))	/* should never happen	*/
	    {
		avail = used;			/* all avail is used	*/
	     /* free  = 0;			/* happens below	*/
	    }

/*
 * CONVERT BLOCKS; ACCUMULATE TOTALS FOR ONE ENTRY:
 */

	    total_avail	  += (avail   = Blocks (avail, 1));
	    total_used	  += (used    = Blocks (used,  1));
	    total_free	  += (free    = avail - used);
	    total_reserve += (reserve = Blocks (reserve, 1));

	    if (qflag)				/* no need to print anything */
		continue;

	    limit = Blocks (limit, 1);

/*
 * PRINT ONE ENTRY:
 */

	   if (limit)
	   {
	       (void) printf (form_fs, avail, used, free, Percent (used, avail),
			      limit, reserve,
			      swapentry.fsw_priority,
			      swapentry.fsw_mntpoint);
	   }
	   else
	   {
	       (void) printf (form_fs_nolim, avail, used, free,
			      Percent (used, avail), reserve,
			      swapentry.fsw_priority,
			      swapentry.fsw_mntpoint);
	   }

	} /* while */

} /* FileSysSwap */


/************************************************************************
 * H E L D   S W A P
 *
 * Given global pages_alloc (allocated for devices and file systems), look up
 * total swap space available (swapspc_max) and free (swapspc_cnt) from system
 * memory (both in units of pages) using global nl[], and use all the values to
 * compute the amount of swap space held for existing processes.  If not qflag,
 * print one line of information about it.  Always add the held space to
 * total_used, in units of 1Kb or 1Mb according to mflag.
 *
 * In case of error, write a message to stderr and do not print info about held
 * swap space.
 */

PROC void HeldSwap()
{
	long	my_swapspc_cnt;		/* from kernel; free pages  */
	long	my_swapspc_max;		/* from kernel; avail pages */
	long	held;			/* held blocks (Kb or Mb)   */

/*
 * GET VALUES FROM SYSTEM MEMORY:
 */

	if (! GetNamelist ("report on swap space on hold"))
	    return;			/* error message already printed */

	if (! (GetKernelValue ((off_t) (nl [NL_SWAPSPC_CNT] . n_value),
			       sizeof (my_swapspc_cnt),
			       (void *) & my_swapspc_cnt,
			       "swap pages free (swapspc_cnt)")
	    && GetKernelValue ((off_t) (nl [NL_SWAPSPC_MAX] . n_value),
			       sizeof (my_swapspc_max),
			       (void *) & my_swapspc_max,
			       "swap pages available (swapspc_max)")))
	{
	    Error (NOERRNO, WARNING, "Cannot report on swap space on hold");
	    return;
	}

/*
 * REPORT ON HELD SWAP SPACE:
 */

	held = Blocks ((long) (my_swapspc_max - my_swapspc_cnt - pages_alloc),
		       NBPG);

	total_used += held;
	total_free -= held;

	if (! qflag)
	    (void) printf (form_held, held, -held);

} /* HeldSwap */


/***********************************************************************
 * G E T   N A M E L I S T
 *
 * Given global struct nlist nl[] and a string describing the purpose of the
 * call, get namelist information from the currently booted kernel.  Do this
 * only until it succeeds once, for all values that might be needed anywhere in
 * this program (for efficiency).  If successful, return TRUE; otherwise emit
 * an error message and return FALSE.
 */

PROC int GetNamelist (purpose)
	char *	purpose;	/* part of error message	*/
{
static	int	nodata = TRUE;	/* flag:  no data gotten yet	*/
	char *	whatkernel;	/* name of booted kernel file	*/

	if (nodata && (nlist (whatkernel = WhatKernel(), nl) < 0))
	{
	    Error (errno, ERROR, "Cannot read name list information from booted kernel file \"%s\" to %s",
		   whatkernel, purpose);

	    return (FALSE);
	}

	nodata = FALSE;
	return (TRUE);

} /* GetNamelist */


/***********************************************************************
 * W H A T   K E R N E L
 *
 * Return a constant string that is the filename of the kernel (system) file
 * from which the currently running system was booted.
 *
 * For S600/700/800, assume DEFAULTKERN (see below).  Apparently there is no
 * reliable way to do this on S600/700/800.
 *
 * For S300/400, read a known (physical) address in /dev/mem to get the LIF
 * name of the boot file and map that to the HP-UX file name.  If there's any
 * problem reading memory, emit a warning message to stderr and assume
 * DEFAULTKERN.
 *
 * WARNING:  This code is not portable beyond S300/S400, and is dependent on an
 * address in /dev/mem.
 *
 * Return value:  as shown in the table below.
 */

#ifdef V4FS
#define DEFAULTKERN "/stand/vmunix"
#else
#define DEFAULTKERN "/hp-ux"
#endif /* V4FS */

#ifdef __hp9000s300
struct mapping {		/* map LIF names to kernel names */
	char *	lif_name;	/* LIF file name to map		 */
	size_t	lif_len;	/* strlen (lif_name)		 */
	char *	kern_name;	/* result of mapping		 */
} mapping[] = {
	{ "SYSHPUX",  7, DEFAULTKERN },
	{ "SYSBCKUP", 8, "/SYSBCKUP" },
	{ "SYSDEBUG", 8, DEFAULTKERN },
	{ CPNULL,     0, CPNULL	     },
};
#endif

PROC char * WhatKernel()
{
#ifdef __hp9000s800
	    return (DEFAULTKERN);
#endif

#ifdef __hp9000s300
#define	MEMFILE		"/dev/mem"		/* where to read LIF name */
#define	BOOTNAME_LOC	((off_t) 0xfffffdc2)	/* location in MEMFILE	  */
#define	KERNNAMELEN	8			/* field is 8 chars long  */

	int  fd;				/* file descriptor	  */
	char lif_name [KERNNAMELEN + 1];	/* read from MEMFILE	  */
	struct mapping * mapent;		/* place in list	  */

/*
 * GET LIF NAME FROM KERNEL MEMORY:
 *
 * Note:  address can be so large it appears negative, so check the lseek()
 * return value carefully.
 */

	if (((fd = open (MEMFILE, O_RDONLY)) < 0)
	 || (lseek (fd, BOOTNAME_LOC, SEEK_SET) != BOOTNAME_LOC)
	 || (read  (fd, (void *) lif_name, KERNNAMELEN) < 0)
	 || (close (fd) < 0))
	{
	    Error (errno, WARNING, "Cannot read LIF file name of booted system from system memory; assuming kernel file is \"%s\"",
		   DEFAULTKERN);

	    return (DEFAULTKERN);
	}

	lif_name [KERNNAMELEN] = CHNULL;	/* ensure trailing null */

/*
 * MAP LIF NAME TO UNIX FILE NAME:
 *
 * Ensure a matched string *and* the next char is either a blank (sometimes
 * the names are padded) or CHNULL.
 */

	for (mapent = mapping; mapent -> lif_len; ++mapent)
	{
	    if ((strncmp (lif_name, mapent -> lif_name, mapent -> lif_len) == 0)
	     && ((lif_name [mapent -> lif_len] == ' ')
	      || (lif_name [mapent -> lif_len] == CHNULL)))
	    {
		return (mapent -> kern_name);
	    }
	}

	Error (NOERRNO, WARNING, "Cannot map LIF file name of booted system from system memory (\"%s\") to kernel file name; assuming \"%s\"",
	       lif_name, DEFAULTKERN);

	return (DEFAULTKERN);

#endif /*__hp9000s300 */

} /* WhatKernel */


/***********************************************************************
 * G E T   S W C H U N K
 *
 * Given global nl[] containing the address of swchunk in NL_SWCHUNK, read the
 * value from kernel memory (if not already done), convert it to bytes, and
 * return it.  In case of error, emit an error message and return 0.
 *
 * Pass the value from the kernel through dtop() to convert to pages, times
 * NBPG to convert to bytes.
 */

PROC long GetSwchunk()
{
static	long	my_swchunk = 0;		/* swap chunk size */

	if (my_swchunk			/* already gotten */
	 || GetKernelValue ((off_t) (nl [NL_SWCHUNK] . n_value),
			    sizeof (my_swchunk), (void *) & my_swchunk,
			    "swap chunk size (swchunk)"))
	{
	    return (dtop (my_swchunk) * NBPG);
	}

	my_swchunk = 0;		/* just to be safe */
	return (0);

} /* GetSwchunk */


/***********************************************************************
 * G E T   K E R N E L   V A L U E
 *
 * Given an address and size in kernel memory, a pointer to an object of that
 * size, and a string describing the value to be read, open /dev/kmem (first
 * time only, after that keep it open), read the value from the address, and
 * return TRUE.  If the address is 0 or the open, seek, or read fails, emit an
 * error message, leave the object unchanged, and return FALSE.
 */

PROC int GetKernelValue (address, size, valuep, valuename)
	off_t	address;	/* of value in kmem		*/
	unsigned int size;	/* of value in bytes		*/
	void *	valuep;		/* where to write data		*/
	char *	valuename;	/* for error message		*/
{
static	int	kmemfd = -1;	/* open kernel file descriptor	*/

/*
 * CHECK THAT THE VALUE (SYMBOL) IS KNOWN:
 */

	if (address == 0)
	{
	    Error (NOERRNO, ERROR, "Cannot read %s from kernel memory:  Symbol not found",
		   valuename);

	    return (FALSE);
	}

/*
 * OPEN KERNEL MEMORY:
 *
 * Only do this the first time (until it succeeds).
 */

	if ((kmemfd < 0) && ((kmemfd = open ("/dev/kmem", O_RDONLY)) < 0))
	{
	    Error (errno, ERROR, "Cannot open /dev/kmem to read %s from address 0x%lx",
		   valuename, address);

	    return (FALSE);
	}

/*
 * SEEK AND READ VALUE:
 */

	if ((lseek (kmemfd, address, SEEK_SET) != address)
	 || (read  (kmemfd, (void *) valuep, size) != size))
	{
	    Error (errno, ERROR, "Cannot seek and read from /dev/kmem to read %s from address 0x%lx",
		   valuename, address);

	    return (FALSE);
	}

	return (TRUE);

} /* GetKernelValue */


/************************************************************************
 * B L O C K S
 *
 * Given a number of blocks, a block size in bytes, and global mflag, convert
 * the blocks value to units of 1Kb or 1Mb, rounding off as necessary, and
 * return the result.  If blocksize is 1024 and mflag is false, this routine
 * is a no-op.
 */

PROC long Blocks (blocks, blocksize)
	long	blocks;		/* to convert	*/
	long	blocksize;	/* bytes each	*/
{
#define	Mb2  524288		/* 2^20 / 2	*/
#define	Mb  1048576		/* 2^20		*/

	return (mflag ?		      (((blocks * blocksize) + Mb2) / Mb) :
		(blocksize == 1024) ? blocks :
				      (((blocks * blocksize) + 512) / 1024));

} /* Blocks */


/************************************************************************
 * P E R C E N T
 *
 * Given a numerator and denominator, figure and return a percentage the first
 * represents of the second, as a floating point number.  The caller must round
 * it off as desired.  If denom is 0, avoid dividing by zero, instead return
 * 100.0.
 */

PROC float Percent (num, denom)
	long	num;	/* numerator	*/
	long	denom;	/* denominator	*/
{
	return ((denom == 0) ? 100.0 : ((num * 100.0) / denom));

} /* Percent */


/************************************************************************
 * U S A G E
 *
 * Print usage messages (char *usage[]) to stderr and exit with ERREXIT.
 * Follow each message line by a newline.
 */

PROC void Usage()
{
REG	int	which = 0;		/* current line */

	while (usage [which] != CPNULL)
	{
	    (void) fprintf (stderr, usage [which++], myname);
	    (void) putc ('\n', stderr);
	}

	(void) exit (ERREXIT);

} /* Usage */


/************************************************************************
 * E R R O R
 *
 * Given an errno value, an ERROR/WARNING flag, a message (printf) string, zero
 * or more argument strings, and global exitcode, print an error/warning
 * message to stderr and if it's an error, set exitcode to ERREXIT.  Message is
 * preceded by "<myname>:  " using global char *myname, and followed by a
 * period and newline.  If myerrno (system error number) is not NOERRNO, a
 * relevant message is appended before the period.
 */

/* VARARGS2 */
PROC void Error (myerrno, errtype, message, arg1, arg2, arg3, arg4)
	int	myerrno;	/* system errno if relevant */
	int	errtype;	/* ERROR or WARNING	    */
	char *	message;
	long	arg1, arg2, arg3, arg4;
{
	(void) fprintf (stderr, "%s:  %s:  ", myname,
			(errtype == ERROR) ? "ERROR" : "WARNING");
	(void) fprintf (stderr, message, arg1, arg2, arg3, arg4);

	if (myerrno != NOERRNO)
	{
	    (void) fprintf (stderr, ":  %s (errno = %d)", strerror (myerrno),
			    myerrno);
	}

	(void) putc ('.',  stderr);
	(void) putc ('\n', stderr);

	if (errtype == ERROR)
	    exitcode = ERREXIT;

} /* Error */
