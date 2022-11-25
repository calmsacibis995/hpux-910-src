/* @(#) $Revision: 70.1 $ */

/*
	The structures pstat, qstat and outq are to be aligned for
	sharing on both the 300 and 800.  Before you make any changes,
	make sure the alignment and size of the structure match on both
	machines.  This is to support DUX on teh 300 and 800.
*/

#include	<sys/param.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<signal.h>
#include	<pwd.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<utmp.h>
#ifdef REMOTE
#include	<sys/utsname.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<errno.h>
#include	<cluster.h>
#endif REMOTE
#ifdef SecureWare
#ifndef REMOTE
#include        <errno.h>       /* so lock.c will compile */
#endif
#include        <sys/security.h>
#include        <sys/audit.h>
#include        <prot.h>
#include        <protcmd.h>
#endif

#ifndef SPOOL
#define	SPOOL	"/usr/spool/lp"
#endif
#ifndef ADMIN
#define	ADMIN	"lp"
#endif
#ifndef ADMDIR
#define	ADMDIR	"/usr/lib"
#endif
#ifndef USRDIR
#define	USRDIR	"/usr/bin"
#endif

time_t time();
char *date(), *getname();
extern uid_t getuid();
extern gid_t getgid();
extern uid_t geteuid();
extern gid_t getegid();

#define	TRUE	1
#define	FALSE	0

#define MINPRI	0
#define MAXPRI	7

#define	CORMSG	"out of memory"
#define	ADMINMSG	"this command for use only by LP Administrators"
#ifdef SecureWare
#define ISADMIN ((ISSECURE) ? (lp_privilege(ADMIN)) :\
			(((int)getuid()) == 0 || strcmp(getname(), ADMIN) == 0))
#else
#define	ISADMIN	(((int)getuid()) == 0 || strcmp(getname(), ADMIN) == 0)
#endif

#define	TITLEMAX	80	/* maximum length of title */
#define	OPTMAX	512
#define	NAMEMAX	(MAXNAMLEN + 1)	/* max length of simple filename + 1 */
#define	DESTMAX	DIRSIZ	/* max length of destination name */
#define	FILEMAX	MAXPATHLEN	/* max pathname for file  */
#define	LOGMAX	15	/* maximum length of logname */
#define	TMPLEN	256	/* maximum length of temporary strings */

#define	MEMBER	"member"
#define	CLASS	"class"
#define	DEFAULT	"default"
#define	REQUEST	"request"
#define	INTERFACE	"interface"
#ifdef REMOTE
#define	CINTERFACE	"cinterface"
#define	SINTERFACE	"sinterface"
#endif REMOTE
#define LPANA	"lpana.log"
#define	ERRLOG	"log"
#define	OLDLOG	"oldlog"
#define	FIFO	"FIFO"
#define	DISABLE	"disable"
#define	REJECT	"reject"
#define	MODEL	"model"
#define CNODEMODEL "clustermodel"
#ifdef REMOTE
#define	CMODEL	"cmodel"
#define	SMODEL	"smodel"
#endif REMOTE
#define	LPDEST	"LPDEST"
#define PRINTER "PRINTER"
#define	SCHEDLOCK	"SCHEDLOCK"

#define	QSTATUS	"qstatus"

#define	SEQLOCK	"SEQLOCK"
#define	SEQFILE	"seqfile"
#define	SEQLEN	4	/* max length of sequence number */
#define	SEQMAX	10000	/* maximum sequence number + 1 */
#define	SEQMAXREMOTE	1000	/* maximum sequence number + 1 for BSD */
#define	IDSIZE	DESTMAX+SEQLEN+1	/* maximum length of request id */
/* Note: SP_MAXHOSTNAMELEN should be a multiple of 4 bytes to allow
   things to line up on specific bounderies */

#ifdef REMOTE
#if UTSLEN > 256
#define SP_MAXHOSTNAMELEN UTSLEN + 3
#else
#define SP_MAXHOSTNAMELEN 260
#endif
#define	RNAMEMAX	sizeof(REQUEST)+DESTMAX+SP_MAXHOSTNAMELEN+2
#else
#define	RNAMEMAX	sizeof(REQUEST)+DESTMAX+DESTMAX+2
#endif REMOTE

#define	OUTPUTQ	"outputq"
#define	TOUTPUTQ	"Toutputq"

#define	PSTATUS	"pstatus"

#define	ALTIME	5

/* single-character commands for request file: */

#ifdef REMOTE

#define R_JOBNAME        'J' /*	Job Name.  String to be used for the job
				name on the burst page. */

#define R_CLASSIFICATION 'C' /*	Classification.  String to be  used  for
				the  classification  line  on  the burst
				page.  The -C option in rlp(UTIL) may be
				used  to  replace  the  default  of  the
				hostname.  In a cluster, this will default
				to the hostname of the client originating
				the request. */

#define R_LITERAL        'L' /*	Literal.      The     line      contains
				identification  info  from  the password
				file and causes the banner  page  to  be
				printed. */

#define R_TITLE          'T' /*	Title.  String to be used as  the  title
				for pr(UTIL). */

#define R_HOSTNAME       'H' /*	Host Name.  Name of  the  machine  where
				lp(UTIL)  was  invoked.  The hostname is
				obtained by a call to gethostname. */

#define R_PERSON         'P' /*	Person.  Login name of  the  person  who
				invoked  The name is obtained by calling
				getuid followed by a call  to  getpwuid.
				This  is used to verify ownership by for
				a cancel operation. */

#define R_MAIL           'M' /*	Send mail to the specified user when the
				current  print  job completes.  The user
				name is the user name  that  appears  on
				the "P" line. */

#define R_PRIORITY	'A' /* Priority.  Priority of request */
#define R_FORMATTEDFILE  'f' /*	Formatted File.  Name of a file to print
				which is already formatted. */

#define R_FILETYPEl      'l' /*	Like ``f'' but passes control characters
				and does not make page breaks. */

#define R_FILETYPEp      'p' /*	Name of a file to print  using  pr(UTIL)
				as a filter. */

#define R_FILETYPEt      't' /*	Troff   File.    The    file    contains
				troff(UTIL)  output (cat phototypesetter
				commands). */

#define R_FILETYPEn      'n' /*	Ditroff  File.  The file contains device
				independent troff(UTIL) commands). */

#define R_FILETYPEd      'd' /*	DVI File.  The file contains Tex  output
				(DVI format from Standford). */

#define R_FILETYPEg      'g' /*	Graph  File.   The  file  contains  data
				produced by plot. */

#define R_FILETYPEc      'c' /*	Cifplot File.  The  file  contains  data
				produced by cifplot. */

#define R_FILETYPEv      'v' /*	The file contains a raster image. */

#define R_FILETYPEr      'r' /*	The file contains text data with FORTRAN
				carriage control characters. */

#define R_FILETYPEk      'k' /*	Reserved for use by Kerberized LPR 
				clients and servers */

#define R_FILETYPEo      'o' /*	The file contains Postscript data. */

#define R_FILETYPEz      'z' /*	Reserved for future use with the
				Palladium print system	*/

#define R_FONT1          '1' /*	Troff Font R.  Name of the font file  to
				use instead of the default. */

#define R_FONT2          '2' /*	Troff Font I.  Name of the font file  to
				use instead of the default. */

#define R_FONT3          '3' /*	Troff Font B.  Name of the font file  to
				use instead of the default. */

#define R_FONT4          '4' /*	Troff Font S.  Name of the font file  to
				use instead of the default. */

#define R_WIDTH          'W' /*	Width.   Changes  the  page  width   (in
				characters)  used  by  pr(UTIL)  and the
				text filters. */

#define R_INDENT         'I' /*	Indent.  The  number  of  characters  to
				indent the output by (in ascii). */

#define R_UNLINKFILE     'U' /*	Unlink.  Name of  file  to  remove  upon
				completion of printing. */

#define R_FILENAME       'N' /*	File name.  The name of the  file  which
				is  being  printed,  or  a blank for the
				standard input (when lp is invoked in  a
				pipeline). */

#define R_FILE           'F' /*	File.  Name of a file as it exists on  a
				HP-UX  system.  Note:  the "F" option is
				a HP-UX option.  When this file is  sent
				to  another  system, this option will be
				removed for the transfer process.

				This name is used to  store  a  file  on
				systems  that  have a 14 character limit
				to file names. */

#define R_OPTIONS        'O' /*	Options.   Printer-dependent  or  class-
				dependent options.

				Note:  To get the user specified options
				transferred  through  a  BSD system, the
				options are included on a  'N'  command.
				To  make  sure  the command does not get
				printed by a status request, the  length
				of  the  argument  is  made  longer than
				there is  room  to  print  in  a  status
				request. */

#define R_COPIES         'K' /*	Copies.  The number of  copies  of  each
				file  to  be  printed.   Note:   the "c"
				option is a  HP-UX  option.   When  this
				file  is  sent  to  another system, this
				option will be removed for the  transfer
				process.   During  the transfer process,
				the number of copies to  be  printed  is
				indicated  by  specifying  the file name
				multiple times i.e. to  print  it  three
				times you specify the file command three
				times with the same file  name  on  each
				command. */

#define R_HEADERTITLE    'B' /*	Header title.  User supplied title. */

#define R_WRITE          'R' /*	Write.  Write to the specified user when
				the  current print job completes.  Note:
				the "w" option is  only  usable  on  the
				system  the  spool  request was made on.
				The "w" option will be  changed  to  the
				"M" option so the user gets notification
				that the  printing has completed. */

#else

#define	R_FILE           'F' /*	file name */
#define	R_MAIL           'M' /*	mail has been requested */
#define	R_WRITE          'W' /*	user wants message via write */
#define	R_TITLE          'T' /*	user-supplied title */
#define	R_COPIES         'C' /*	number of copies */
#define	R_OPTIONS        'O' /*	printer- and class-dependent options */

#endif REMOTE

#define	OSIZE	7
#define	PIDSIZE	5

/*
	The structure outq is to be aligned for sharing on both
	the 300 and 800.  Before you make any changes, make sure
	the alignment and size of the structure match on both
	machines.  This is to support DUX on teh 300 and 800.

	The size of this structure must be multiple of 4 bytes.
*/
#ifdef REMOTE
struct outq {		/* output queue request */
	long o_size;			/* size of request -- */
					/* # of bytes of data */
	time_t o_date;			/* date of entry into */
					/* output queue	      */
	time_t o_pdate;			/* date of printing   */
					/* start              */
	int o_seqno;			/* sequence number of */
					/* the request        */
	short o_flags;			/* See below for flag */
					/* values             */
	short o_priority;		/* priority of this   */
					/* request            */
	short o_rflags;			/* See below for rflag*/
					/* values */
	char o_host[SP_MAXHOSTNAMELEN];	/* Name of the system */
					/* where the request  */
					/* originated.  The   */
					/* clusterserver name */
					/* on a cluster.      */
	char o_logname[LOGMAX+1];	/* logname of the     */
					/* requester          */
	char o_dest[DESTMAX+2];		/* output destination */
					/* (class or member)  */
	char o_dev[DESTMAX+4];		/* if printing, the   */
					/* name of the        */
					/* printer. if not    */
					/* printing, "-".     */
};
#else REMOTE
struct outq {	/* output queue request */
	char o_dest[DESTMAX+1];	/* output destination (class or member) */
	char o_logname[LOGMAX+1];	/* logname of requestor */
	int o_seqno;		/* sequence number of request */
	long o_size;		/* size of request -- # of bytes of data */
	char o_dev[DESTMAX+1];	/* if printing, the name of the printer.
				   if not printing, "-".  */
	time_t o_date;		/* date of entry into output queue */
	time_t o_pdate;		/* date of printing start */
	short o_flags;		/* See below for flag values */
	short o_priority;	/* priority of this request */
}
#endif REMOTE

/* Value interpretation for o_flags: */

#define	O_DEL	1		/* Request deleted */
#define	O_PRINT	2		/* Request now printing */
#define O_PROCESSED 4		/* Control file processed */

/* Q_RSIZE should be at least 161 bytes.  This allows 80 two
   byte characters to fit in the buffer and one byte for the
   0 at the end.  Q_RSIZE is being rounded up to a 4 byte boundry
   to keey things lined up for files shared between different
   machines */

#define	Q_RSIZE	164

/* Value interpretation for o_rflags: */

#define	O_OB3	0001		/* Use three digit sequence numbers */
#define	O_REM	0002		/* Request was received by rlpaemon */

/*
	The structure qstat is to be aligned for sharing on both
	the 300 and 800.  Before you make any changes, make sure
	the alignment and size of the structure match on both
	machines.  This is to support DUX on teh 300 and 800.
*/
#ifdef REMOTE
struct qstat {			/* queue status entry */
	time_t q_date;		/* date status last modified */
	short q_accept;		/* TRUE iff lp accepting requests for
				   dest, otherwise FALSE. */
	short q_ob3;		/* use BSD 3 digit sequence numbers */
				/* if TRUE.  sequence file in */
				/* request directory */
	char q_reason[Q_RSIZE];	/* if accepting then "accepting",
				   otherwise the reason requests for
				   dest are being rejected by lp. */
	char q_dest[DESTMAX+1];	/* destination */
};
#else REMOTE
struct qstat {			/* queue status entry */
	char q_dest[DESTMAX+1];	/* destination */
	short q_accept;		/* TRUE iff lp accepting requests for
				   dest, otherwise FALSE. */
	time_t q_date;		/* date status last modified */
	char q_reason[Q_RSIZE];	/* if accepting then "accepting",
				   otherwise the reason requests for
				   dest are being rejected by lp. */
};
#endif REMOTE

/* P_RSIZE should be at least 161 bytes.  This allows 80 two
   byte characters to fit in the buffer and one byte for the
   0 at the end.  P_RSIZE is being rounded up to a 4 byte boundry
   to keey things lined up for files shared between different
   machines */

#define	P_RSIZE	164

/*
	The structure pstat is to be aligned for sharing on both
	the 300 and 800.  Before you make any changes, make sure
	the alignment and size of the structure match on both
	machines.  This is to support DUX on teh 300 and 800.
*/
#ifdef REMOTE
struct pstat {		/* printer status entry */
	long p_version;		/* current version of the spool	*/
				/* system.  See comments below	*/
	int p_pid;		/* if busy, process id that is printing,
				   otherwise 0 */
	int p_seqno;		/* if busy, sequence # of printing request */
	time_t p_date;		/* date last enabled/disabled */
	short p_flags;		/* See below for flag values */
	short p_remob3;		/* see below for description of remob3 */
	short p_rflags;		/* see below for description of rflags */
	short p_fence;		/* fence priority ( handled by lpfence ) */
	short p_default;	/* default priority */
	char p_reason[P_RSIZE];	/* if enabled, then "enabled",
				   otherwise the reason the printer has
				   been disabled.	*/
	char p_remotedest[SP_MAXHOSTNAMELEN];
	char p_host[SP_MAXHOSTNAMELEN];	/* Name of the system */
	char p_dest[DESTMAX+1];	/* destination name of printer */
	char p_rdest[DESTMAX+1];   /* if busy, the destination requested by
				   user at time of request, otherwise "-" */
	char p_remoteprinter[DESTMAX+1];
};

/* Comments about the version variable

   version contains a indicator to help determine what version
   the various structures are.  This is used when converting
   from one version to another at update time.  The version is
   contained in the define SPOOLING_VERSION and is put there
   by the lpadmin command.

Version		Description

   1		First version of remote spooling (never implemented)
			Included for completness.

   2		Second version of remote spoolint (first implemented)
			The size of variables holding the host name
			were increased to handle longer names
			The size of the reason field was increased
			to 161 to allow 80 character messages when
			a character is 2 bytes.
*/

#define SPOOLING_VERSION 2

/* Value interpretation for p_rflags: */

#define	P_OB3 0001	/* use BSD 3 digit sequence numbers.  The */
			/* sequence file is in the request directory */
#define	P_OCI 0002	/* remote cancel pathname */
#define	P_OCM 0004	/* remote cancel model */
#define	P_OMA 0010	/* remote machine name */
#define	P_OPR 0020	/* remote printer name */
#define	P_ORC 0040	/* restrict local cancel command to owner */
#define	P_OSI 0100	/* remote status pathname */
#define	P_OSM 0200	/* remote status model */
#else REMOTE
struct pstat {		/* printer status entry */
	char p_dest[DESTMAX+1];	/* destination name of printer */
	int p_pid;		/* if busy, process id that is printing,
				   otherwise 0 */
	char p_rdest[DESTMAX+1];   /* if busy, the destination requested by
				   user at time of request, otherwise "-" */
	int p_seqno;		/* if busy, sequence # of printing request */
	time_t p_date;		/* date last enabled/disabled */
	char p_reason[P_RSIZE];	/* if enabled, then "enabled",
				   otherwise the reason the printer has
				   been disabled.	*/
	short p_flags;		/* See below for flag values */
	short p_default;	/* default priority */
	short p_fence;		/* fence priority ( handled by lpfence )*/
};
#endif REMOTE

/* Value interpretation for p_flags: */

#define	P_ENAB	0001		/* printer enabled */
#define	P_AUTO	0002		/* disable printer automatically */
#define	P_BUSY	0004		/* printer now printing a request */
#define P_CNODE 0010		/* printer is attached to cnode */

/* Value interpretation for p_remob3: */
/* use the defines from structure outq entry o_rflags: */

/*#define	O_OB3	0001 */		/* Use three digit sequence numbers */
/*#define	O_REM	0002 */		/* Request was received by rlpaemon */

/* messages for the scheduler that can be written to the FIFO */

#define	F_ENABLE	'e'	/* arg1 = printer */
#define	F_MORE		'm'	/* arg1 = printer */
#define	F_DISABLE	'd'	/* arg1 = printer */
#define	F_ZAP		'z'	/* arg1 = printer */
#define	F_REQUEST	'r'	/* arg1 = destination, arg2 = sequence #,
				   arg3 = logname */
#define F_DEST		'a'	/* arg1 = old destination, arg2 = old seq-#
				   arg3 = new destination, arg4 = new seq-# */
				/* arg5 = hostname
				   arg6 = new priority */
#define F_PRIORITY	'p'	/* arg1 = destination, arg2 = sequence #
				   arg3 = new priority */
				/* arg4 = hostname */
#define	F_CANCEL	'c'	/* arg1 = destination, arg2 = sequence # */
#define	F_DEV		'v'	/* arg1 = printer, arg2 = new device */
#define	F_NOOP		'n'	/* no args */
#define	F_NEWLOG	'l'	/* no args */
#define	F_QUIT		'q'	/* no args */
#define	F_STATUS	's'	/* no args */

/* Arguments to the access(2) system call */

#define	ACC_R	4		/* read access */
#define	ACC_W	2		/* write access */
#define	ACC_X	1		/* execute access */
#define	ACC_DIR	8		/* must be a directory */

#ifdef REMOTE
#define MAXUSERS	50
#define MAXREQUESTS	50
#define MAXJCL		32
#define	MAXPRTITLE	80	/* maximum length of pr title */
#define MASTERLOCK	"/usr/spool/lp/lpd.lock"
#define DEFLOGF		"/usr/spool/lp/lpd.log"
#define SENDINGSTATUS	".sendingstatus"
#define	REMOTESENDING	".remotesending"

char	hostname[SP_MAXHOSTNAMELEN];	/* host name of the sending system */
char	jobprinter[32];	/* request id (printer-sequence) */

/*
void  	gethostname();
*/
#endif REMOTE

#ifdef TRUX
/* "eaccess" clashes with the libsec routine of the same name
 */
#define GET_ACCESS  lp_eaccess
#else
#define GET_ACCESS  eaccess
#endif
