/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)sendmail.h	5.16 (Berkeley) 6/1/90
 */
# ifdef PATCH_STRING
/* static char *patch_5384="@(#) PATCH_9.X: sendmail.h $Revision: 1.20.109.12 $ 94/03/24 PHNE_5384"; */
# endif	/* PATCH_STRING */

/*
**  SENDMAIL.H -- Global definitions for sendmail.
*/

# ifdef _DEFINE
# 	define EXTERN
# 	ifndef lint
# 		ifndef hpux
static char SmailSccsId[] =	"@(#)sendmail.h	5.16		6/1/90";
# 		else	/* ! not hpux */
static char SmailRcsid[] = "$Header: sendmail.h,v 1.20.109.12 95/03/22 17:42:19 mike Exp $";
# 		endif	/* not hpux */
# 	endif /* lint */
# else /* ! _DEFINE */
# 	define EXTERN extern
# endif /* _DEFINE */

# include <stdio.h>
# include <ctype.h>
# include <setjmp.h>
# include <string.h>
# include "conf.h"
# include "useful.h"

# ifdef LOG
# 	include <syslog.h>
# endif /* LOG */

# ifdef DAEMON
# 	include <sys/socket.h>
# 	include <netinet/in.h>
# endif /* DAEMON */

# ifdef NDBM
# 	include <ndbm.h>
# endif	/* NDBM */

# ifdef hp9000s300
# 	undef m_flags		/* defined in <sys/sysmacros.h> s300 only */
# endif	/* hp9000s300 */

# define PSBUFSIZE	(MAXNAME + MAXATOM)	/* size of prescan buffer */


/*
**  Data structure for bit maps.
**
**	Each bit in this map can be referenced by an ascii character.
**	This is 128 possible bits, or 12 8-bit bytes.
*/

# define BITMAPBYTES	16	/* number of bytes in a bit map */
# define BYTEBITS	8	/* number of bits in a byte */

/* internal macros */
# define _BITWORD(bit)	(bit / (BYTEBITS * sizeof (int)))
# define _BITBIT(bit)	(1 << (bit % (BYTEBITS * sizeof (int))))

typedef int	BITMAP[BITMAPBYTES / sizeof (int)];

/* test bit number N */
# define bitnset(bit, map)	((map)[_BITWORD(bit)] & _BITBIT(bit))

/* set bit number N */
# define setbitn(bit, map)	(map)[_BITWORD(bit)] |= _BITBIT(bit)

/* clear bit number N */
# define clrbitn(bit, map)	(map)[_BITWORD(bit)] &= ~_BITBIT(bit)

/* clear an entire bit map */
# define clrbitmap(map)		bzero((char *) map, BITMAPBYTES)
/*
**  Address structure.
**	Addresses are stored internally in this structure.
*/

struct address
{
	char		*q_paddr;	/* the printname for the address */
	char		*q_user;	/* user name */
	char		*q_ruser;	/* real user name, or NULL if q_user */
	char		*q_host;	/* host name */
	struct mailer	*q_mailer;	/* mailer to use */
	u_short		q_flags;	/* status flags, see below */
	uid_t		q_uid;		/* user-id of receiver (if known) */
	gid_t		q_gid;		/* group-id of receiver (if known) */
	char		*q_home;	/* home dir (local mailer only) */
	char		*q_fullname;	/* full name if known */
	struct address	*q_next;	/* chain */
	struct address	*q_alias;	/* address this results from */
	struct address	*q_tchain;	/* temporary use chain */
	time_t		q_timeout;	/* timeout for this address */
};

typedef struct address ADDRESS;

# define QDONTSEND	000001	/* don't send to this address */
# define QBADADDR	000002	/* this address is verified bad */
# define QGOODUID	000004	/* the q_uid q_gid fields are good */
# define QPRIMARY	000010	/* set from argv */
# define QQUEUEUP	000020	/* queue for later transmission */
# define QSENT		000040	/* has been successfully delivered */
/***********sun's**********/
# define QDOMAIN        000100  /* was result of a domain-wide alias */
# define QWASLOCAL      000200  /* we've seen our host name, stop NISing */

# define QVERIFIED	000400	/* verified, but not expanded */

# define NULLADDR	((ADDRESS *) NULL)
/*
**  Mailer definition structure.
**	Every mailer known to the system is declared in this
**	structure.  It defines the pathname of the mailer, some
**	flags associated with it, and the argument vector to
**	pass to it.  The flags are defined in conf.c
**
**	The argument vector is expanded before actual use.  All
**	words except the first are passed through the macro
**	processor.
*/

struct mailer
{
	char	*m_name;	/* symbolic name of this mailer */
	char	*m_mailer;	/* pathname of the mailer to use */
	BITMAP	m_flags;	/* status flags, see below */
	short	m_mno;		/* mailer number internally */
	char	**m_argv;	/* template argument vector */
	short	m_s_rwset;	/* rewriting set for sender addresses */
	short	m_r_rwset;	/* rewriting set for recipient addresses */
	char	*m_eol;		/* end of line string */
	u_long	m_maxsize;	/* size limit on message to this mailer */
};

typedef struct mailer	MAILER;

/* bits for m_flags */
# define M_CANONICAL	'C'	/* make addresses canonical "u@dom" */
# define M_EXPENSIVE	'e'	/* it costs to use this mailer.... */
# define M_ESCFROM	'E'	/* escape From lines to >From */
# define M_FOPT		'f'	/* mailer takes picky -f flag */
# define M_NO_NULL_FROM	'g'	/* sender of errors should be $g */
# define M_HST_UPPER	'h'	/* preserve host case distinction */
# define M_INTERNAL	'I'	/* SMTP to another sendmail site */
# define M_LOCAL	'l'	/* delivery is to this host */
# define M_LIMITS	'L'	/* must enforce SMTP line limits */
# define M_MUSER	'm'	/* can handle multiple users at once */
# define M_NHDR		'n'	/* don't insert From line */
# define M_FROMPATH	'p'	/* use reverse-path in MAIL FROM: */
# define M_ROPT		'r'	/* mailer takes picky -r flag */
# define M_SECURE_PORT	'R'	/* try to send on a reserved TCP port */
# define M_STRIPQ	's'	/* strip quote chars from user/host */
# define M_RESTR	'S'	/* must be daemon to execute */
# define M_USR_UPPER	'u'	/* preserve user case distinction */
# define M_UGLYUUCP	'U'	/* this wants an ugly UUCP from line */
# define M_XDOT		'X'	/* use hidden-dot algorithm */
# define M_NO_BANGS_OK  '!'     /* OK if ! missing in UUCP from line */

EXTERN MAILER	*Mailer[MAXMAILERS+1];

EXTERN MAILER	*LocalMailer;		/* ptr to local mailer */
EXTERN MAILER	*ProgMailer;		/* ptr to program mailer */
/*
**  Header structure.
**	This structure is used internally to store header items.
*/

struct header
{
	char		*h_field;	/* the name of the field */
	char		*h_value;	/* the value of that field */
	struct header	*h_link;	/* the next header */
	u_short		h_flags;	/* status bits, see below */
	BITMAP		h_mflags;	/* m_flags bits needed */
};

typedef struct header	HDR;

/*
**  Header information structure.
**	Defined in conf.c, this struct declares the header fields
**	that have some magic meaning.
*/

struct hdrinfo
{
	char	*hi_field;	/* the name of the field */
	u_short	hi_flags;	/* status bits, see below */
};

extern struct hdrinfo	HdrInfo[];

/* bits for h_flags and hi_flags */
# define H_EOH		00001	/* this field terminates header */
# define H_RCPT		00002	/* contains recipient addresses */
# define H_DEFAULT	00004	/* if another value is found, drop this */
# define H_RESENT	00010	/* this address is a "Resent-..." address */
# define H_CHECK	00020	/* check h_mflags against m_flags */
# define H_ACHECK	00040	/* ditto, but always (not just default) */
# define H_FORCE	00100	/* force this field, even if default */
# define H_TRACE	00200	/* this field contains trace information */
# define H_FROM		00400	/* this is a from-type field */
# define H_VALID	01000	/* this field has a validated value */
/*
**  Envelope structure.
**	This structure defines the message itself.  There is usually
**	only one of these -- for the message that we originally read
**	and which is our primary interest -- but other envelopes can
**	be generated during processing.  For example, error messages
**	will have their own envelope.
*/

# define ENVELOPE	struct envelope

ENVELOPE
{
	HDR		*e_header;	/* head of header list */
	long		e_msgpriority;	/* adjusted priority of this message */
	time_t		e_ctime;	/* time message appeared in the queue */
	char		*e_to;		/* the target person */
	char		*e_receiptto;	/* return receipt address */
	ADDRESS		e_from;		/* the person it is from */
	char		*e_sender;	/* e_from.q_paddr w comments stripped */
	char		**e_fromdomain;	/* the domain part of the sender */
	ADDRESS		*e_sendqueue;	/* list of message recipients */
	ADDRESS		*e_errorqueue;	/* the queue for error responses */
	u_long		e_msgsize;	/* size of the message in bytes */
	int		e_nrcpts;	/* number of recipients */
	short		e_class;	/* msg class (priority, junk, etc.) */
	short		e_flags;	/* flags, see below */
	short		e_hopcount;	/* number of times processed */
	short		e_nsent;	/* number of sends since checkpoint */
	short		e_sendmode;	/* message send mode */
	short		e_errormode;	/* error return mode */
	int		(*e_puthdr)();	/* function to put header of message */
	int		(*e_putbody)();	/* function to put body of message */
	struct envelope	*e_parent;	/* the message this one encloses */
	struct envelope *e_sibling;	/* the next envelope of interest */
	char		*e_bodytype;	/* type of message body */
	char		*e_df;		/* location of temp file */
	FILE		*e_dfp;		/* temporary file */
	char		*e_id;		/* code for this entry in queue */
	FILE		*e_xfp;		/* transcript file */
	FILE		*e_lockfp;	/* the lock file for this message */
	char		*e_message;	/* error message */
	char		*e_macro[128];	/* macro definitions */
	BITMAP		e_sticky;	/* sticky macros */
};

/* values for e_flags */
# define EF_OLDSTYLE	0x0000001	/* use spaces (not commas) in hdrs */
# define EF_INQUEUE	0x0000002	/* this message is fully queued */
# define EF_TIMEOUT	0x0000004	/* this message is too old */
# define EF_CLRQUEUE	0x0000008	/* disk copy is no longer needed */
# define EF_SENDRECEIPT	0x0000010	/* send a return receipt */
# define EF_FATALERRS	0x0000020	/* fatal errors occured */
# define EF_KEEPQUEUE	0x0000040	/* keep queue files always */
# define EF_RESPONSE	0x0000080	/* this is an error or return receipt */
# define EF_RESENT	0x0000100	/* this message is being forwarded */
# define EF_VRFYONLY	0x0000200	/* verify only (don't expand aliases) */
# define EF_LOGSENDER	0x0008000	/* need to log the sender */

EXTERN ENVELOPE	*CurEnv;	/* envelope currently being processed */
/*
**  Message priority classes.
**
**	The message class is read directly from the Priority: header
**	field in the message.
**
**	CurEnv->e_msgpriority is the number of bytes in the message plus
**	the creation time (so that jobs ``tend'' to be ordered correctly),
**	adjusted by the message class, the number of recipients, and the
**	amount of time the message has been sitting around.  This number
**	is used to order the queue.  Higher values mean LOWER priority.
**
**	Each priority class point is worth WkClassFact priority points;
**	each recipient is worth WkRecipFact priority points.  Each time
**	we reprocess a message the priority is adjusted by WkTimeFact.
**	WkTimeFact should normally decrease the priority so that jobs
**	that have historically failed will be run later; thanks go to
**	Jay Lepreau at Utah for pointing out the error in my thinking.
**
**	The "class" is this number, unadjusted by the age or size of
**	this message.  Classes with negative representations will have
**	error messages thrown away if they are not local.
*/

struct priority
{
	char	*pri_name;	/* external name of priority */
	int	pri_val;	/* internal value for same */
};

EXTERN struct priority	Priorities[MAXPRIORITIES];
EXTERN int		NumPriorities;	/* pointer into Priorities */
/*
**  Rewrite rules.
*/

struct rewrite
{
	char	**r_lhs;	/* pattern match */
	char	**r_rhs;	/* substitution value */
	struct rewrite	*r_next;/* next in chain */
};

EXTERN struct rewrite	*RewriteRules[MAXRWSETS];

/*
**  Special characters in rewriting rules.
**	These are used internally only.
**	The COND* rules are actually used in macros rather than in
**		rewriting rules, but are given here because they
**		cannot conflict.
*/

/* left hand side items */
# define MATCHZANY	'\020'	/* match zero or more tokens */
# define MATCHANY	'\021'	/* match one or more tokens */
# define MATCHONE	'\022'	/* match exactly one token */
# define MATCHCLASS	'\023'	/* match one token in a class */
# define MATCHNCLASS	'\024'	/* match anything not in class */
# define MATCHREPL	'\025'	/* replacement on RHS for above */

/* right hand side items */
# define CANONNET	'\026'	/* canonical net, next token */
# define CANONHOST	'\027'	/* canonical host, next token */
# define CANONUSER	'\030'	/* canonical user, next N tokens */
# define CALLSUBR	'\031'	/* call another rewriting set */

/* HP only right hand side item */
# define CALLNAME	'\037'	/* call nameserver */

/* conditionals in macros */
# define CONDIF		'\032'	/* conditional if-then */
# define CONDELSE	'\033'	/* conditional else */
# define CONDFI		'\034'	/* conditional fi */

/* bracket characters for host name lookup */
# define HOSTBEGIN	'\035'	/* hostname lookup begin */
# define HOSTEND	'\036'	/* hostname lookup end */

/* \001 is also reserved as the macro expansion character */
/*
**  Information about hosts that we have looked up recently.
**
**	This stuff is 4.2/3bsd specific.
*/

# ifdef DAEMON

# 	define HOSTINFO	struct hostinfo

HOSTINFO
{
	char		*ho_name;	/* name of this host */
	struct in_addr	ho_inaddr;	/* internet address */
	short		ho_flags;	/* flag bits, see below */
	short		ho_errno;	/* error number on last connection */
	short		ho_exitstat;	/* exit status from last connection */
};


/* flag bits */
# 	define HOF_VALID	00001		/* this entry is valid */

/* ********* sun's **********
 * Cache entry for NIS lookup operations
*/
struct bitmap2
{
       BITMAP  positive, negative;
};

# endif /* DAEMON */
/*
**  Symbol table definitions
*/

struct symtab
{
	char		*s_name;	/* name to be entered */
	char		s_type;		/* general type (see below) */
	struct symtab	*s_next;	/* pointer to next in chain */
	union
	{
		BITMAP		sv_class;	/* bit-map of word classes */
		ADDRESS		*sv_addr;	/* pointer to address header */
		MAILER		*sv_mailer;	/* pointer to mailer */
		char		*sv_alias;	/* alias */
# ifdef HOSTINFO
		HOSTINFO	sv_host;	/* host information */
# endif /* HOSTINFO */
		char		*sv_hostsig;	/* host signature */
	}	s_value;
};

typedef struct symtab	STAB;

/* symbol types */
# define ST_UNDEF	0	/* undefined type */
# define ST_CLASS	1	/* class map */
# define ST_ADDRESS	2	/* an address in parsed format */
# define ST_MAILER	3	/* a mailer header */
# define ST_ALIAS	4	/* an alias */
# define ST_HOST	5	/* host information */
# define ST_HOSTSIG	7	/* host signature */

# define s_class	s_value.sv_class
# define s_address	s_value.sv_addr
# define s_mailer	s_value.sv_mailer
# define s_alias	s_value.sv_alias
# define s_hostinfo	s_value.sv_host
# define s_hostsig	s_value.sv_hostsig

extern STAB	*stab();

/* opcodes to stab */
# define ST_FIND	0	/* find entry */
# define ST_ENTER	1	/* enter if not there */
/*
**  STRUCT EVENT -- event queue.
**
**	Maintained in sorted order.
**
**	We store the pid of the process that set this event to insure
**	that when we fork we will not take events intended for the parent.
*/

struct event
{
	time_t		ev_time;	/* time of the function call */
	int		(*ev_func)();	/* function to call */
	int		ev_arg;		/* argument to ev_func */
	int		ev_pid;		/* pid that set this event */
	struct event	*ev_link;	/* link to next item */
};

typedef struct event	EVENT;

EXTERN EVENT	*EventQueue;		/* head of event queue */
/*
**  Operation, send, and error modes
**
**	The operation mode describes the basic operation of sendmail.
**	This can be set from the command line, and is "send mail" by
**	default.
**
**	The send mode tells how to send mail.  It can be set in the
**	configuration file.  It's setting determines how quickly the
**	mail will be delivered versus the load on your system.  If the
**	-v (verbose) flag is given, it will be forced to SM_DELIVER
**	mode.
**
**	The error mode tells how to return errors.
*/

EXTERN char	OpMode;		/* operation mode, see below */

# define MD_DELIVER	'm'		/* be a mail sender */
# define MD_ARPAFTP	'a'		/* old-style arpanet protocols */
# define MD_SMTP		's'		/* run SMTP on standard input */
# define MD_DAEMON	'd'		/* run as a daemon */
# define MD_KILL		'k'		/* kill the daemon */
# define MD_VERIFY	'v'		/* verify: don't collect or deliver */
# define MD_TEST		't'		/* test mode: resolve addrs only */
# define MD_INITALIAS	'i'		/* initialize alias database */
# define MD_PRINT	'p'		/* print the queue */
# define MD_FREEZE	'z'		/* freeze the configuration file */


/* values for e_sendmode -- send modes */
# define SM_DELIVER	'i'		/* interactive delivery */
# define SM_QUICKD	'j'		/* deliver w/o queueing */
# define SM_FORK		'b'		/* deliver in background */
# define SM_QUEUE	'q'		/* queue, don't deliver */
# define SM_VERIFY	'v'		/* verify only (used internally) */

/* used only as a parameter to sendall */
# define SM_DEFAULT	'\0'		/* unspecified, use SendMode */


/* values for e_errormode -- error handling modes */
# define EM_PRINT	'p'		/* print errors */
# define EM_MAIL		'm'		/* mail back errors */
# define EM_WRITE	'w'		/* write back errors */
# define EM_BERKNET	'e'		/* special berknet processing */
# define EM_QUIET	'q'		/* don't print messages (stat only) */
/*
**  Additional definitions
*/


/*
**  Privacy flags
**	These are bit values for the PrivacyFlags word.
*/

# define PRIV_PUBLIC		0	/* what have I got to hide? */
# define PRIV_NEEDMAILHELO	00001	/* insist on HELO for MAIL, at least */
# define PRIV_NEEDEXPNHELO	00002	/* insist on HELO for EXPN */
# define PRIV_NEEDVRFYHELO	00004	/* insist on HELO for VRFY */
# define PRIV_NOEXPN		00010	/* disallow EXPN command entirely */
# define PRIV_NOVRFY		00020	/* disallow VRFY command entirely */
# define PRIV_AUTHWARNINGS	00040	/* flag possible authorization probs */
# define PRIV_RESTRICTMAILQ	01000	/* restrict mailq command */
# define PRIV_RESTRICTQRUN	02000	/* restrict queue run */
# define PRIV_GOAWAY		00777	/* don't give no info, anyway, anyhow */

/* struct defining such things */
struct prival
{
	char	*pv_name;	/* name of privacy flag */
	int	pv_flag;	/* numeric level */
};

/* offset used to issure that the error messages for name server error
 * codes are unique.
 */
# define	MAX_ERRNO	300

EXTERN char	Bit8Mode;	/* what to do with 8-bit messages, see below */

# define B8_ENCODE	'e'		/* encode into 7-bit (using mimefy) */
# define B8_REJECT	'r'		/* return the message to the sender */
# define B8_SEND_ANYWAY	's'		/* send message and take our chance */
/*
**  Global variables.
*/

EXTERN bool	FromFlag;	/* if set, "From" person is explicit */
EXTERN bool	NoAlias;	/* if set, don't do any aliasing */
EXTERN bool	ForceMail;	/* if set, mail even if already got a copy */
EXTERN bool	MeToo;		/* send to the sender also */
EXTERN bool	IgnrDot;	/* don't let dot end messages */
EXTERN bool	SaveFrom;	/* save leading "From" lines */
EXTERN bool	Verbose;	/* set if blow-by-blow desired */
EXTERN bool	GrabTo;		/* if set, get recipients from msg */
EXTERN bool	NoReturn;	/* don't return letter to sender */
EXTERN bool	SuprErrs;	/* set if we are suppressing errors */
EXTERN bool	QueueRun;	/* currently running message from the queue */
EXTERN bool	HoldErrs;	/* only output errors to transcript */
EXTERN bool	NoConnect;	/* don't connect to non-local mailers */
EXTERN bool	SuperSafe;	/* be extra careful, even if expensive */
EXTERN bool	ForkQueueRuns;	/* fork for each job when running the queue */
EXTERN bool	AutoRebuild;	/* auto-rebuild the alias database as needed */
EXTERN bool	CheckAliases;	/* parse addresses during newaliases */
EXTERN bool	UseNameServer;	/* use internet domain name server */
EXTERN bool	AltFreezeFile;	/* using alternate frozen configuration file */
EXTERN int	SafeAlias;	/* minutes to wait until @:@ in alias file */
EXTERN time_t	TimeOut;	/* time until timeout */
EXTERN FILE	*InChannel;	/* input connection */
EXTERN FILE	*OutChannel;	/* output connection */
EXTERN uid_t	RealUid;	/* when Daemon, real uid of caller */
EXTERN gid_t	RealGid;	/* when Daemon, real gid of caller */
EXTERN uid_t	DefUid;		/* default uid to run as */
EXTERN gid_t	DefGid;		/* default gid to run as */
EXTERN char	*DefUser;	/* default user to run as (from DefUid) */
EXTERN int	OldUmask;	/* umask when sendmail starts up */
EXTERN int	Errors;		/* set if errors (local to single pass) */
EXTERN int	ExitStat;	/* exit status code */
EXTERN int	AliasLevel;	/* depth of aliasing */
EXTERN int	MotherPid;	/* proc id of parent process */
EXTERN int	LineNumber;	/* line number in current input */
EXTERN bool	Saw8Bits;	/* saw non-ASCII character in sfgets */
EXTERN time_t	ReadTimeout;	/* timeout on reads */
EXTERN int	LogLevel;	/* level of logging to perform */
EXTERN int	FileMode;	/* mode on files */
EXTERN int	QueueLA;	/* load average starting forced queueing */
EXTERN int	RefuseLA;	/* load average refusing connections are */
EXTERN int	CurrentLA;	/* current load average */
EXTERN int	QueueFactor;	/* slope of queue function */
EXTERN time_t	QueueIntvl;	/* intervals between running the queue */
EXTERN char	*AliasFile;	/* location of alias file */
EXTERN char	*ReverseAliasFile;/* location of reverse-alias file */
EXTERN DBM	*AliasDB;	/* alias data-base file descriptor */
EXTERN DBM	*ReverseAliasDB;/* reverse-alias data-base file descriptor */
EXTERN char	*HelpFile;	/* location of SMTP help file */
EXTERN char	*StatFile;	/* location of statistics summary */
EXTERN char	*QueueDir;	/* location of queue directory */
EXTERN char	*FileName;	/* name to print on error messages */
EXTERN char	*SmtpPhase;	/* current phase in SMTP processing */
EXTERN char	*MyHostName;	/* name of this host for SMTP messages */
EXTERN char	*RealHostName;	/* name of host we are talking to */
EXTERN struct	sockaddr_in RealHostAddr;/* address of host we are talking to */
EXTERN char	*CurHostName;	/* current host we are dealing with */
EXTERN jmp_buf	TopFrame;	/* branch-to-top-of-loop-on-error frame */
EXTERN bool	QuickAbort;	/*  .... but only if we want a quick abort */
EXTERN int	PrivacyFlags;	/* privacy flags */
extern char	*ConfFile;	/* location of configuration file [conf.c] */
extern char	*FreezeFile;	/* location of frozen memory image [conf.c] */
extern ADDRESS	NullAddress;	/* a null (template) address [main.c] */
EXTERN char	SpaceSub;	/* substitution for <lwsp> */
EXTERN int	WkClassFact;	/* multiplier for message class -> priority */
EXTERN int	WkRecipFact;	/* multiplier for # of recipients -> priority */
EXTERN int	WkTimeFact;	/* priority offset each time this job is run */
EXTERN u_long	MaxMessageSize;	/* advertised max size we will accept */
EXTERN int	CheckpointInterval;	/* queue file checkpoint interval */
EXTERN char	*PostMasterCopy;	/* address to get errs cc's */
EXTERN char	*TrustedUsers[MAXTRUST+1];	/* list of trusted users */
EXTERN char	*UserEnviron[MAXUSERENVIRON+1];	/* saved user environment */
EXTERN char	*RemoteServer;	/* remote mail server name */

EXTERN char	*CurMxHost;	/* MX host we connect to, for logging */
EXTERN char	*ServerAddress;	/* address we connect to (for logging) */
/*
**  Trace information
*/

/* trace vector and macros for debugging flags */
EXTERN u_char	tTdvect[100];
# define tTd(flag, level)	(tTdvect[flag] >= level)
# define tTdlevel(flag)		(tTdvect[flag])
/*
**  Miscellaneous information.
*/

# include	<sysexits.h>


/*
**  Some in-line functions
*/

/* set exit status */
# define setstat(s)	{ \
				if (ExitStat == EX_OK || ExitStat == EX_TEMPFAIL) \
					ExitStat = s; \
			}

/* make a copy of a string */
# define newstr(s)	strcpy(xalloc(strlen(s) + 1), s)

# define STRUCTCOPY(s, d)	d = s

/* other functions */
# define	MAX(a,b) (((a)>(b))?(a):(b))

/*
**  Declarations of useful functions
*/

extern DBM 	*initaliases();
extern ENVELOPE *newenvelope();
extern char	*hvalue();
extern char	*username();
extern char	**prescan();
extern char	**copyplist();
extern ADDRESS	*parseaddr();
extern char	*xalloc();
extern bool	sameaddr();
extern FILE	*dfopen();
extern EVENT	*setevent();
extern char	*sfgets();
extern char	*queuename();
extern time_t	curtime();
extern char	*errstring();
extern ADDRESS	*recipient();
extern ADDRESS	*getctladdr();
extern char	*remotename();
extern char	*hostsignature();
extern char	*shortenstring();
extern void	syserr();
extern void	usrerr();
extern char    *denlstring ();
extern void     sendtoargv();
extern void 	sendtolist();
extern void	alias();
extern void	forward();
extern void	clrevent();
extern unsigned int	sleep();
extern void	collect();
extern void	setdefaults();
extern void	getrequests();
extern void	setproctitle();
extern void	clrdaemon();
extern void	closeconnection();
extern void	maphostname();
extern void	giveresponse();
extern void	logdelivery();
extern void	putfromline();
extern void	sendall();
extern void	dropenvelope();
extern void	clearenvelope();
extern void	initsys();
extern void	openxscript();
extern void	setsender();
extern void	message();
extern void	nmessage();
extern void	addheader();
extern void	eatheader();
extern void	logsender();
extern void	putheaderline();
extern void	commaize();
extern void	expand();
extern void	define();
extern void	finis();
extern void	disconnect();
extern void	loweraddr();
extern void	rewrite();
extern void	cataddr();
extern void	printqueue();
extern void	unlockqueue();
extern void	setctladdr();
extern void	readcf();
extern void	printrules();
extern void	setoption();
extern void	setclass();
extern void	savemail();
extern void	smtp();
extern void	tTsetup();
extern void	tTflag();
extern void	smtpquit();
extern void	smtpmessage();
extern void	stripquotes();
extern void	printav();
extern void	xputs();
extern void	makelower();
extern void	buildfname();
extern void	fixcrlf();
extern void	xunlink();
extern int	uucpname();
extern int	putheader();
extern int	putbody();
extern char    *macvalue();
# ifdef DEBUG
extern void	printaddr();
# endif	/* DEBUG */
