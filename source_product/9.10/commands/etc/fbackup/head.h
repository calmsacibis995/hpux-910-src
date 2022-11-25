/* @(#) $Revision: 72.1 $ */

/*
 *	head.h
 *
 *  This is the common header file for all the fbackup (but not the frecover)
 *  files.
 *
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mount.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#ifdef AUDIT
#include <sys/audit.h>
#endif
#ifdef ACLS
#include <unistd.h>
#include <sys/acl.h>
#endif

#include "vdi.h"

#ifdef NLS
#include <nl_types.h>
#define NLSCHAR int
#else NLS
#define catgets(i, sn,mn,s) (s)
#define CHARAT(p)	(*(p))
#define ADVANCE(p)	((p)++)
#define PCHAR(c,p)	(*(p)=(c))
#define PCHARADV(c,p)	(*(p)++=(c))
#define NLSCHAR char
#endif NLS


/*  If the NLS message catalogues are used, the error messages are allocated
    with the following numbering scheme.

    set 1:		  set 2:		set 3:		    set 4:
    ------		  ------		------		    ------
    main.c:   1-99	  reader.c: 1-99	writer.c:  1-99	    util.c: 1-99
    main2.c:  101-199				writer2.c: 101-199
    main3.c:  201-299				tape.c:    201-299
    reset.c:  301-399                           rmt.c      301-399
    parse.c:  401-499
    search.c: 501-599
    inex.c:   601-699
    flist.c:  701-799
    pwgr.c:   801-899
*/


/* for structures (etc) which are common to both fbackup and frecover */
#include <fbackup.h>

#ifdef NLS
#define mystrcmp(a,b) strcoll(a,b)
# define mystrncmp(a,b,c) nl_strncmp(a,b,c)
#else NLS
#define mystrcmp(a,b) strcmp(a,b)
#endif NLS


#define WRTRSIG (1 << (SIGUSR1-1))	/* masks used to block these sigs */
#define RDRSIG  (1 << (SIGUSR2-1))

#define min(a,b) ((a)<(b) ? (a) : (b))
#define rndup(a) (((a)+BLOCKSIZE-1)&(~(BLOCKSIZE-1)))

#define FALSE 0
#define TRUE 1

/****************************** default values ******************************/

#define MAXVOLUSES_DFLT 100	/* default max no times vols may be used */
#define CKPTFREQ_DFLT   256	/* put checkpoint records every n data recs */
#define FSMFREQ_DFLT    200	/* put DAT fast search marks every n files */

#define NRDRS_DFLT	2	/* default number of readers */
#define BLKSPERREC_DFLT 16	/* default number of blocks per record */
#define NRECS_DFLT	16	/* default number of records of shared mem */

#define LEVEL_DFLT      0	/* default 'fbackup level' (see -u option) */
#define MAXRETRIES_DFLT 5	/* max times to retry an active file */
#define RETRYLIM_DFLT   2048	/* max BLOCKS to use for an active file */

/****************************** maximum values ******************************/

#define MAXOUTFILES 256	/* maximum number of output files */
#define MAXRDRS 8	/* maximum number of reader processes */
#define MAXRECS 64	/* maximum number of shared memory data records
			 * Note: this is really an artificial division of
			 * the shared memory segment. */

/****************************** file names **********************************/
#ifdef DEBUG_LOCAL_PROC
#define READER	    "./fbackuprdr"	/* pathname of reader processes */
#define WRITER	    "./fbackupwrtr"	/* pathname of writer process */
#else
#ifdef V4FS
#define READER	    "/usr/sbin/fbackuprdr"	/* pathname of reader processes */
#define WRITER	    "/usr/sbin/fbackupwrtr"	/* pathname of writer process */
#else /* V4FS */
#define READER	    "/etc/fbackuprdr"	/* pathname of reader processes */
#define WRITER	    "/etc/fbackupwrtr"	/* pathname of writer process */
#endif /* V4FS */
#endif 
#define STDOUTFILE  "-"			/* filename used to specify stdout */
#ifdef V4FS
#define DATESFILE   "/var/adm/fbackupfiles/dates"	/* used for saving  */
						    /* past dump histories */
#define DATESTMP    "/var/adm/fbackupfiles/tmpdates"	/* used while making */
#else /* V4FS */
#define DATESFILE   "/usr/adm/fbackupfiles/dates"	/* used for saving  */
						    /* past dump histories */
#define DATESTMP    "/usr/adm/fbackupfiles/tmpdates"	/* used while making */
#endif /* V4FS */

/****************************** exit codes **********************************/

#define NORMAL_EXIT  0	/* fbackup completely finished the session */
#define RESTART_EXIT 1	/* after being interrupeted, state saved for restart */
#define ERROR_EXIT   2	/* fbackup encounted some error condition*/

/************************* output file types ********************************/

#define REGFILE 101
#define MAGTAPE 102

#define CLOSED -1

#define ATTREW	  1
#define UCBNOREW  2
#define ILLEGALMT 3
#define UCBREW	        4
#define ATTNOREW	5

/***************************** index format *********************************/

#define IDXFMT  "#%3d %s\n"		      /* format of index file lines */

/**************************** numeric constants *****************************/

#define FNAMEOFFSET 5			      /* index where file name starts */
#define MAXIDXREC   (MAXPATHLEN+FNAMEOFFSET)  /* max index record length */

#define FIRSTVOL 1		/* the starting number of the first volume */
#define PROTECT 0600			/* protection mode for created files */
#define IDX2TABSIZE (MAXPATHLEN*16)	/* must >= MAXPATHLEN*2 */

#define MODIFIED (time_t) -1

/********************** reader and writer status values *********************/

#define READY    -1
#define START    -2	/* reader only */
#define BUSY     -3
#define DONE     -4
#define RESET    -5	/* writer only */
#define RESUME   -6	/* writer only */
#define EXIT     -7	/* writer only */

/************************ index control command codes ***********************/

#define NOACTION  -8
#define UPDATEIDX -9
#define SENDIDX   -10

/******************* shared memory record status values *********************/

#define FREE      -11	/* initial state */
#define PARTIAL   -12
#define ALLOCATED -13
#define COMPLETE  -14

/* Values that indicate whether or not the first block of the trailer, which
 * contains the status (good or bad) is in shared memory.  If it is, and it
 * it is discovered that the file is active, the file's status is changed to
 * bad. */

#define TRLNOTINMEM -15
#define TRLINMEM    -16

/************************ shared memory block types *************************/

#define HDRFIRST 'H'	/* first header block */
#define HDRCONT  'h'	/* non-first header block */
#define DATA     'd'	/* data block */
#define TRAILER  'T'	/* trailer block */

/*************************** reader data structure ***************************
 * Each reader process has one of these structures associated with it. */

typedef struct {		/* reader data structure */
    int status;			/* status of this reader */
    char file[MAXPATHLEN];	/* name of the file to read */
    unsigned offset;		/* # of byes to skip before starting to read */
    int nbytes;			/* number of bytes to read into shmem */
    int index;			/* index in shmem array where data goes */
    int pend_fidx;		/* pending file index */
    time_t mtime;		/* modification time */
    time_t ctime;		/* change-inode time */
} RDRTYPE;

/**************** shared memory record data structure ************************
 * Each shared memory record has one of these structures associated with it. */

typedef struct {		/* shared memory record data structure */
    int status;
    int count;
} RECTYPE;

/********* data structure when "reseting" after a tape write error ********/

typedef struct {
    int filenum;
    int blknum;
    off_t datasize;
    char blktype;
} RESETINFOTYPE;

/**************** "pad" shared memory data structure *************************
 * This structure is used for all shared memory inter-process communications. */

typedef struct {
    int ppid;		/* parent process id number */
    long begtime;	/* time the session began, used for unique id too */
    int nrecs;		/* number of shared memory "records" */
    int recsize;	/* size of these records */
    int wrtrstatus;	/* status of the writer process */
    int avail;		/* number of available bytes of the "ring" */
    int semaid;		/* semaphore id number */
    int nfiles;		/* number of files being (attempted) backed up */
    int maxretries;	/* max number of retries on an active file */
    int retrylim;	/* max number of bytes used retrying active files */
    int ckptfreq;	/* frequency of ckpt recs on magtape output files */
    int fsmfreq;	/* frequency of fast search marks on DAT output files */
    int updateafter;	/* number of file after which to increase the vol
			 * number in the file list (and hence the index) */
    int idxcmd;		/* index modification command for the main process
			 * to perform, sent by the writer process */
    int idxsize;	/* size of the index, it is invariant for each index */
    int maxvoluses;	/* max no of times a volume can be used (overridable) */
    int yflag;		/* yes to all queries flag */
    int vflag;		/* verbose output flag */

	    /* These variables are in shared memory because they are needed
	     * for the -R option to be able to be started properly. */
    int vol;		/* number of the current volume */
    int startfno;	/* file number to start with, 1 unless it's a restart */
    int pid;		/* process id number of the FIRST parent process
			 * this may NOT be the ppid value above.  This value
			 * is used for backup id, ppid is used for inter-
			 * process communication. */
    char envlang[32];	/* language used for this session */

	    /* These values are filenames to be executed when: */
    char chgvol_file[MAXPATHLEN];	/* a volume change is required */
    char err_file[MAXPATHLEN];		/* a write error occurs */

    RESETINFOTYPE reset;	/* see above */
    RDRTYPE rdr[MAXRDRS];	/* see above */
    RECTYPE rec[MAXRECS];	/* see above */
} PADTYPE;

/************************ pending file data structure ************************
 * These structures are maintained for each file being read into shared memory.
 * For each file, the number of times a reader process is called to read in
 * part of it (a "chunk" of it) is maintained, and the chunk count is
 * incremented.  Also, whenever one of these reader processes reports back
 * that it has finished that chunk, the chunksfull counter is incremented.
 * The main process knows when it has finished allocating readers to read
 * the file into shared memory, so the chunk count is as high as it will
 * ever be for that file.  When the chunksfull value matches the chunks
 * value, the file is completely in shared memory. */

typedef struct {
/*
 * We need this because getfile() returns data in static buffers.  So we
 * need to save the filename in our structure.
 */
    char file[MAXPATHLEN];
    int status;		/* its status, modified or unmodified */
    char *statusidx;	/* sh mem index to modify the status, if needed */
    int chunks;		/* see above */
    int chunksfull;	/* see above */
    int modflag;	/* indicates the file has changed during the time
			 * it was being backed up */
    time_t mtime;	/* modification time */
    time_t ctime;	/* change-inode time */
    time_t atime;	/* access time */
} PENDFTYPE;

/****************** internal checkpoint data structure **********************
   this is all the data needed to get the writer back in sync after a tape
   write error.  The following variables apply to the state of the writer
   at the beginning of the last good record which was written.
****************************************************************************/

typedef struct {
    int readingfname;	/* in the middle of reading a filename flag */
    off_t tmpdatasize;	/* size of the previously written file */
    int ht_ctr;		/* header/trailer block count */
    int d_ctr;		/* data block count */
} INTCKPTTYPE;

/********************** exlude file data structure ***************************
 * This structure is used to keep track of the files (and directories) which
 * are to be exluded from the tree/graph expansion. */

typedef struct {
    ino_t inode;		/* it's inode number */
    dev_t device;	/* and device, since inode numbers aren't unique */
} ETAB;

/*
 * directory hash table
 */
typedef struct dtabentry {
  struct dtabentry *next;	/* pointer to next directory in bucket */
  short len;			/* length of pathname */
  char path;			/* start of directory pathname */
} dtabentry;

/******************** file list (flist) data structure ***********************
 * This structure is used to form a linked list of files to be backed up. */

typedef struct flistnode {
    dtabentry *dirptr;		/* pointer to directory pathname */
    short vol;			/* the (estimated) volume number it resides on
				 * this is only correct on the last index */
    char weirdflag;
    char fwdlinkflag;
    ino_t ino;
    dev_t dev;
    struct flistnode *linkptr;	/* pointer to what this is hard linked to,
				 * NULL, if it's not a link to anything */
    struct flistnode *dotdotptr;/* pointer to this file's .. link, if it is
				 * "unusual", in the normal case, it's NULL */
    struct flistnode *next;	/* next file in the flist */
    unsigned char len;		/* length of filename */
    char pathptr;		/* start of filename, moved here to save space */
} FLISTNODE;

char *strcpy(), *strncpy(), *myctime();
void exit(), perror(); 
long sigblock(), sigsetmask(), sigpause(), *mymalloc();


/* the following is for remote tape device support on S300 */

/* use mt_dsreg1 for the following */
#define GMT_300_EOF(x)              ((x) & 0x00000080)
#define GMT_300_BOT(x)              ((x) & 0x00000040)
#define GMT_300_EOT(x)              ((x) & 0x00000020)
#define GMT_300_WR_PROT(x)          ((x) & 0x00000004)
#define GMT_300_ONLINE(x)           ((x) & 0x00000001)

/* use mt_dsreg2 for the following */
#define GMT_300_D_6250(x)           ((x) & 0x00000080)
#define GMT_300_DR_OPEN(x)          ((x) & 0x00000004)
#define GMT_300_IM_REP_EN(x)        ((x) & 0x00000001)

/* the following are for checking remote S800 devices on a 300 */
/* for examining mt_gstat */

#define GMT_800_EOF(x)              ((x) & 0x80000000)
#define GMT_800_BOT(x)              ((x) & 0x40000000)
#define GMT_800_EOT(x)              ((x) & 0x20000000)
#define GMT_800_WR_PROT(x)          ((x) & 0x04000000)
#define GMT_800_ONLINE(x)           ((x) & 0x01000000)
#define GMT_800_D_6250(x)           ((x) & 0x00800000)
#define GMT_800_D_1600(x)           ((x) & 0x00400000)
#define GMT_800_D_800(x)            ((x) & 0x00200000)
#define GMT_800_DR_OPEN(x)          ((x) & 0x00040000)
#define GMT_800_IM_REP_EN(x)        ((x) & 0x00010000)

struct fn_list {
  char name[MAXPATHLEN+1];
  struct fn_list *next;
};
