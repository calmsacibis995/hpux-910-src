/*
 *	Fast Tape I/O
 *
 *	David Williams,
 *	Hewlett-Packard Australian Software Operation.
 *
 *	(C) 1986 Hewlett Packard Australia.
 */

#ifndef	_MAIN_
#define	EXTERN	extern
#define	INIT(a)
#else
#define	EXTERN
#define	INIT(a)	a
#endif

#include <stdio.h>	/* you always need this! */
#include <time.h>	/* for printperformance stuff */
#include <sys/types.h>	/* for lots of stuff */
#include <signal.h>	/* for signal handling */
#include <setjmp.h>	/* ditto */
#include <sys/stat.h>	/* for getting stats on files */
#include <ndir.h>	/* for opendir, readdir, etc */
#include <string.h>	/* str* used lots */
#include <ctype.h>	/* ? */
#include <unistd.h>	
#include "f_struct.h"	/* defines file and teape header structures */
#include "ftio_mesg.h"	/* message defines */	
#include <sys/sysmacros.h> /* replaces old mknod.h amongst other things */	
#include <sys/errno.h>  /* system error codes. */

#define MAGIC 070707

#ifdef hp9000s500
#define LOCALMAGIC 0xaaaa	/* as of 6/29/82 */
#endif
#ifdef hp9000s200
#define LOCALMAGIC 0xdddd	/* as of 4/17/85 (Manx file system)*/
#endif
#ifdef hp9000s800
#define LOCALMAGIC 0xcccc	/* as of 4/1/85 */
#endif

#define TRUE 	1	/* Generic definition for TRUE */
#define FALSE 	0	/* Generic definition for FALSE */

#define CHARS	76
#define ENTRIES	25	/* how many file systems will cpio visit */
#define OUT_OF_SPACE 0	/* all normal 16bit quantities are > 0 */
#define HDRSIZE (sizeof (struct cpio_f_hdr) - MAXPATHLEN)
#define DEFBLOCKSIZE 16384
#define MAXBLOCKSIZE 61440
#define DEFNOBUFFERS 16
#define MAXNOBUFFERS 128
#define CPIOSIZE 512  
#define POINTERSPACE (2 * sizeof(int))
#define NULL 0
#define SHMACCESS 0600	/* access rights to the shared memory segment */
#define INPUT	0
#define OUTPUT	1
#define MUTEX	2
#define	DOWN	-1
#define	UP     	1 
#define	NOTUSE	0
#define OK      1
#define PATHLENGTH	MAXPATHLEN	/* max chars in a pathcurrent_path */
#define MAXNOPATHS	60	/* max no of paths in command line */
#define MAXCOMPONENTS   32      /* max no of components in path name */
#define MAXHDRBUF (2*CHARS)     /* size of header buffer scratch pad */
#define EXTRA_MEMORY    50000   /* used to increase break value */

#define UNREP_NO 65535
#define HASHSIZE 501




EXTERN	int	Nobuffers  INIT(=DEFNOBUFFERS);	/* no of buffers */
EXTERN	int	Blocksize  INIT(=DEFBLOCKSIZE); /* input/output blocksize */
EXTERN	char	Errmsg[512];	/* error message buffer */
EXTERN	char	*host;		/* remote host name */
EXTERN	char 	*Output_dev_name;	/* name of output device (tape) */
EXTERN	char 	*Change_name INIT(=NULL); /* script run at chg reel*/
EXTERN	char 	*Filename ;	/* name of current file */
EXTERN	int	Pid 	INIT(=1);	/* process id */
EXTERN	int	Dev_type;	/* device type */
EXTERN	int	Tape_no INIT(=1);	/* tape no */

/* options: */
EXTERN	int	Listfiles INIT(=0);	/* list file names */
EXTERN	int	Listall INIT(=0);	/*list all file names evenifnot loaded*/
EXTERN	int	Prealloc INIT(=0);	/* to prealloc or not to prealloc */
EXTERN	int	Diagnostic INIT(=0);	/* print diagnostics yes/no */
EXTERN	int	Dontload INIT(=0);	/* dont load files, only list */
EXTERN	int	Dospecial INIT(=0);	/* do special files */
EXTERN	int	Htype INIT(=0);		/* header type */
EXTERN	int	Makedir INIT(=0);	/* make directories if true */
EXTERN	int	Mymode;			/* = o if reading, 1 if writing */
#ifdef ACLS
EXTERN  int	Aclflag;		/* 0 for warn 1 no warn */
#endif
EXTERN	int	Doacctime INIT(=0);	/* reset access time */
EXTERN	int	Domodtime INIT(=0);	/* reset modification time */
EXTERN	int	User_id;		/* =0 if effective id is superuser */
EXTERN	int	Neweronly INIT(=1);	/* only load files that are newer */
EXTERN	int	Makerelative INIT(=0); /* makes all absolute paths relative */	
EXTERN	int	Matchpatterns INIT(=0); /* match patterns */
EXTERN	int	Performance INIT(=0);  /* print extra performance data */   
EXTERN	int	Except_patterns INIT(=0); /* copy in files except patterns*/   
EXTERN	int	Incremental INIT(=0); /* do incremental backup instead */
EXTERN	int	Checkpoint INIT(=0);	/* Checkpointing */
EXTERN	int	Resync INIT(=0);	/* Resyncing */
EXTERN	int	File_number INIT(=0);
EXTERN	FILE	*Restart_fp;
EXTERN	char	Restart_name[16];
EXTERN	int	Tape_headers INIT(=1);
EXTERN	char	*Komment INIT(=(char *)0); /* header comment field */
EXTERN	int	Read_stdin INIT(=0);	/* read file list, else recursive */	
EXTERN	char	*Filelist INIT(=(char *)0); /*make file list on tape and file*/
EXTERN	char	*Tty INIT(="/dev/tty");	/* specify interaction file */
EXTERN  char    *Fstype INIT(=(char *)0); /* specify file system type */
EXTERN  int     Realfile INIT(=0);      /* Use file symbolic link points to. */
EXTERN  int     Symbolic_link INIT(=0); /* =1 if file is symbolic link */
EXTERN  int     Hidden INIT(=0);        /* Search CDFs. */

/*
 * buffer count related globals 
 */
EXTERN	char	*Pathname;	/* buffer for pathname */
EXTERN	char	*Home;		/* buffer to hold working directory */
EXTERN	char	*Myname;		/* pointer to argv[0] */
EXTERN	char	*User_name;		/* pointer to username */
EXTERN	time_t	Inc_date;	/* backup all files newer than this */

struct stat Stats;		/* stat structure that gets used a lot */

extern int errno;               /* System errno. */

/* 
 * Global state flag for valid/invalid death of child.
 *
 *    If cld_flag = TRUE  Child's death is NOT to be ignored
 *    otherwise if cld_flag = FALSE then Child's death is to be 
 *    ignored.  This is only true if the dieing child is not the
 *    ftio co-process that is forked by ftio.
 *
 */
EXTERN	char	cld_flag INIT(=1); /* Default to TRUE */

/*
 *	The new all singing, all dancing, ftio "packet".
 */
struct	ftio_packet
{
	char	*block;		/* pointer to where the block starts */
	long	block_size;	/* we still need this for backward compat' */
	int	status;		/* inter process return status */
	long	fname_offset;	/* offset to filename in file list */
	off_t	file_size;	/* size of the file currently being backed up */
	int	file_section;	/* section of the archive we are in */
	off_t	offset;		/* how much of the section we have backed up */
	struct	stat	stats;	/* file stats, only if in the header */	
} *Packets;


