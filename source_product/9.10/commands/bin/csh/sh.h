/* @(#) $Revision: 70.5 $ */      

#include "sh.local.h"
#ifdef VMUNIX
#include <sys/vtimes.h>
#endif

/*
 * C shell
 *
 * Bill Joy, UC Berkeley
 * October, 1978; May 1980
 *
 * Jim Kulp, IIASA, Laxenburg Austria
 * April, 1980
 *
 * Tom Teixeira, MASSCOMP
 * May 1982
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>

#define	isdir(d)	((d.st_mode & S_IFMT) == S_IFDIR)

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/times.h>

#ifndef NONLS
typedef unsigned short 	CHAR;	/* Create new type of characters */		
#else
typedef char		CHAR;	/* if no 8-bit, remain char */
#endif

#ifndef NONLS
	extern char *strerror();	/* translated system errors */
#else
	extern char *sys_errlist[];	/* un-translated system errors */
#endif


typedef	char	bool;

#ifdef NONLS
#define	eq(a, b)	(strcmp(a, b) == 0) /* if NLS, we want to use a different routine that compares strings of char with strings of CHAR */
#endif

#ifdef SIGTSTP
#define setpgrp setpgrp2
#define getpgrp getpgrp2
#define killpg(a,b) (kill(-(a), (b)))
#include <bsdtty.h>
#endif SIGTSTP

/*
 * Global flags
 */
/*  Interestingly enough, lots of these were not initialized:
       
       no initialization       initialized
       -----------------       -----------
	 chkstop                 didcch
	 doneinp                 didfds
	 exiterr                 intty
	 child                   justpr (via enterhist)
	 haderr                  loginsh
	 inact                   setintr
	 neednote                timflg (never used)
	 noexec (if 0)
	 pjobs (now defined in sh.c)

*/
bool	chkstop;		/* Warned of stopped jobs... allow exit */
bool	didcch;			/* Have closed unused fd's for child */
bool	didfds;			/* Have setup i/o fd's for child */
bool	doneinp;		/* EOF indicator after reset from readc */
bool	exiterr;		/* Exit if error or non-zero exit status */
bool	child;			/* Child shell ... errors cause exit */
bool	haderr;			/* Reset was because of an error */
bool	intty;			/* Input is a tty */
bool	intact;			/* We are interactive... therefore prompt */
bool	justpr;			/* Just print because of :p hist mod */
bool	loginsh;		/* We are a loginsh -> .login/.logout */
bool	neednote;		/* Need to pnotify() */
bool	noexec;			/* Don't execute, just syntax check */
bool	pjobs;			/* want to print jobs if interrupted */
bool	setintr;		/* Set interrupts on/off -> Wait intr... */
bool    tenexflag;              /* Should we use tenex() routine? */
bool	timflg;			/* Time the next waited for command */

/*  Added to tell if csh was invoked with -c option */
bool    cflag;

/*  Added to tell if a child would have gone down a vfork path if vfork was
    available.
*/
bool	childVfork;

/*
 * Global i/o info
 */
CHAR	*arginp;		/* Argument input for sh -c and internal `xx` */
int	onelflg;		/* 2 -> need line for -t, 1 -> exit on read */
CHAR	*file;			/* Name of shell file for $0 */

char	*err;			/* Error message from scanner/parser */
int	errno;			/* Error from C library routines */
CHAR	*shtemp;		/* Temp name for << shell files in /tmp */
time_t	time0;			/* Time at which the shell started */

/*
 * Miscellany
 */
CHAR	*doldol;		/* Character pid for $$ */
int	uid;			/* Invokers uid */
time_t	chktim;			/* Time mail last checked */
#ifdef SIGTSTP
int	shpgrp;			/* Pgrp of shell */
#endif
int	tpgrp;			/* Terminal process group */
/* If tpgrp is -1, leave tty alone! */
int	opgrp;			/* Initial pgrp and tty pgrp */
struct	tms shtimes;		/* shell and child times for process timing */

/*
 * These are declared here because they want to be
 * initialized in sh.init.c (to allow them to be made readonly)
 */

struct	biltins {
	char	*bname;
	int	(*bfunct)();
	short	minargs, maxargs;
} bfunc[];

#define	INF	1000

struct srch {
	char	*s_name;
	short	s_value;
} srchn[];

/*
 * To be able to redirect i/o for builtins easily, the shell moves the i/o
 * descriptors it uses away from 0,1,2.
 * Ideally these should be in units which are closed across exec's
 * (this saves work) but for version 6, this is not usually possible.
 * The desired initial values for these descriptors are defined in
 * sh.local.h.
 */
short	SHIN;			/* Current shell input (script) */
short	SHOUT;			/* Shell output */
short	SHDIAG;			/* Diagnostic output... shell errs go here */
short	OLDSTD;			/* Old standard input (def for cmds) */

/*
 * Error control
 *
 * Errors in scanning and parsing set up an error message to be printed
 * at the end and complete.  Other errors always cause a reset.
 * Because of source commands and .cshrc we need nested error catches.
 */

jmp_buf	reslab;

#ifndef NONLS
/* copy now works on CHAR, so we have b_copy to copy bytes */
#define	getexit(a)	b_copy((char *)(a), (char *)reslab, sizeof reslab)
#define	resexit(a)	b_copy((char *)reslab, ((char *)(a)), sizeof reslab)
#else
#define	getexit(a)	copy((char *)(a), (char *)reslab, sizeof reslab)
#define	resexit(a)	copy((char *)reslab, ((char *)(a)), sizeof reslab)
#endif

#define	setexit()	setjmp(reslab)
#define	reset()		longjmp(reslab)
	/* Should use structure assignment here */

CHAR	*gointr;		/* Label for an onintr transfer */
void	(*parintr)();		/* Parents interrupt catch */
void	(*parterm)();		/* Parents terminate catch */

/*
 * Lexical definitions.
 *
 * All lexical space is allocated dynamically.
 * The eighth bit of characters is used to prevent recognition,
 * and eventually stripped.
 */
#ifdef NONLS
#define QUOTE	0200		/* 8th bit used for internal 'ing */
#define TRIM	0177		/* Mask to strip quote bit */
#else
#define	QUOTE 	0100000		/* Change quote bit to 16th bit */
#define	TRIM	 077777		/* Mask to strip quote bit */
#endif
#define KMASK	077400		/* Mask to determine if a character is kanji */

/*
 * Each level of input has a buffered input structure.
 * There are one or more blocks of buffered input for each level,
 * exactly one if the input is seekable and tell is available.
 * In other cases, the shell buffers enough blocks to keep all loops
 * in the buffer.
 */
struct	Bin {
	off_t	Bfseekp;		/* Seek pointer */
	off_t	Bfbobp;			/* Seekp of beginning of buffers */
	off_t	Bfeobp;			/* Seekp of end of buffers */
	short	Bfblocks;		/* Number of buffer blocks */
	char	**Bfbuf;		/* The array of buffer blocks */
} B;

#define	fseekp	B.Bfseekp
#define	fbobp	B.Bfbobp
#define	feobp	B.Bfeobp
#define	fblocks	B.Bfblocks
#define	fbuf	B.Bfbuf

off_t	btell();

/*
 * The shell finds commands in loops by reseeking the input
 * For whiles, in particular, it reseeks to the beginning of the
 * line the while was on; hence the while placement restrictions.
 */
off_t	lineloc;

#ifdef	TELL
off_t	tell();
bool	cantell;			/* Is current source tellable ? */
#endif

/*
 * Input lines are parsed into doubly linked circular
 * lists of words of the following form.
 */
struct	wordent {
	CHAR	*word;
	struct	wordent *prev;
	struct	wordent *next;
};

/*
 * During word building, both in the initial lexical phase and
 * when expanding $ variable substitutions, expansion by `!' and `$'
 * must be inhibited when reading ahead in routines which are themselves
 * processing `!' and `$' expansion or after characters such as `\' or in
 * quotations.  The following flags are passed to the getC routines
 * telling them which of these substitutions are appropriate for the
 * next character to be returned.
 */
#define	DODOL	1
#define	DOEXCL	2
#define	DOALL	DODOL|DOEXCL

/*
 * Labuf implements a general buffer for lookahead during lexical operations.
 * Text which is to be placed in the input stream can be stuck here.
 * We stick parsed ahead $ constructs during initial input,
 * process id's from `$$', and modified variable values (from qualifiers
 * during expansion in sh.dol.c) here.
 */

               /*  WRDSIZ increased from 1024 --> NCARGS; */

#define WRDSIZ NCARGS
CHAR	labuf[WRDSIZ];

CHAR	*lap;

/*
 * Parser structure
 *
 * Each command is parsed to a tree of command structures and
 * flags are set bottom up during this process, to be propagated down
 * as needed during the semantics/exeuction pass (sh.sem.c).
 */
struct	command {
	short	t_dtyp;				/* Type of node */
	short	t_dflg;				/* Flags, e.g. FAND|... */
	union {
		CHAR	*T_dlef;		/* Input redirect word */
		struct	command *T_dcar;	/* Left part of list/pipe */
	} L;
	union {
		CHAR	*T_drit;		/* Output redirect word */
		struct	command *T_dcdr;	/* Right part of list/pipe */
	} R;
#define	t_dlef	L.T_dlef
#define	t_dcar	L.T_dcar
#define	t_drit	R.T_drit
#define	t_dcdr	R.T_dcdr
	CHAR	**t_dcom;			/* Command/argument vector */
	struct	command *t_dspr;		/* Pointer to ()'d subtree */
	short	t_nice;
};

#define	TCOM	1		/* t_dcom <t_dlef >t_drit	*/
#define	TPAR	2		/* ( t_dspr ) <t_dlef >t_drit	*/
#define	TFIL	3		/* t_dlef | t_drit		*/
#define	TLST	4		/* t_dlef ; t_drit		*/
#define	TOR	5		/* t_dlef || t_drit		*/
#define	TAND	6		/* t_dlef && t_drit		*/

#define	FSAVE	(FNICE|FTIME|FNOHUP)	/* save these when re-doing */

#define	FAND	(1<<0)		/* executes in background	*/
#define	FCAT	(1<<1)		/* output is redirected >>	*/
#define	FPIN	(1<<2)		/* input is a pipe		*/
#define	FPOU	(1<<3)		/* output is a pipe		*/
#define	FPAR	(1<<4)		/* don't fork, last ()ized cmd	*/
#define	FINT	(1<<5)		/* should be immune from intr's */
/* spare */
#define	FDIAG	(1<<7)		/* redirect unit 2 with unit 1	*/
#define	FANY	(1<<8)		/* output was !			*/
#define	FHERE	(1<<9)		/* input redirection is <<	*/
#define	FREDO	(1<<10)		/* reexec aft if, repeat,...	*/
#define	FNICE	(1<<11)		/* t_nice is meaningful */
#define	FNOHUP	(1<<12)		/* nohup this command */
#define	FTIME	(1<<13)		/* time this command */

/*
 * The keywords for the parser
 */
#define	ZBREAK		0
#define	ZBRKSW		1
#define	ZCASE		2
#define	ZDEFAULT 	3
#define	ZELSE		4
#define	ZEND		5
#define	ZENDIF		6
#define	ZENDSW		7
#define	ZEXIT		8
#define	ZFOREACH	9
#define	ZGOTO		10
#define	ZIF		11
#define	ZLABEL		12
#define	ZLET		13
#define	ZSET		14
#define	ZSWITCH		15
#define	ZTEST		16
#define	ZTHEN		17
#define	ZWHILE		18

/*
 * Structure defining the existing while/foreach loops at this
 * source level.  Loops are implemented by seeking back in the
 * input.  For foreach (fe), the word list is attached here.
 */
struct	whyle {
	off_t	w_start;		/* Point to restart loop */
	off_t	w_end;			/* End of loop (0 if unknown) */
	CHAR	**w_fe, **w_fe0;	/* Current/initial wordlist for fe */
	CHAR	*w_fename;		/* Name for fe */
	struct	whyle *w_next;		/* Next (more outer) loop */
} *whyles;

/*
 * Variable structure
 *
 * Lists of aliases and variables are sorted alphabetically by name
 */
struct	varent {
	CHAR	**vec;		/* Array of words which is the value */
	CHAR	*name;		/* Name of variable/alias */
	struct	varent *link;
} shvhed, aliases;

/*
 * The following are for interfacing redo substitution in
 * aliases to the lexical routines.
 */
struct	wordent *alhistp;		/* Argument list (first) */
struct	wordent *alhistt;		/* Node after last in arg list */
CHAR	**alvec;			/* The (remnants of) alias vector */

/*
 * Filename/command name expansion variables
 */
short	gflag;				/* After tglob -> is globbing needed? */

/*
 * A reasonable limit on number of arguments would seem to be
 * the maximum number of characters in an arg list / 3.
 * (was NCARGS/6; changed(doubled) for DSDe412639.
 * The parameter 'INF' in sh.init.c is also set to the same value
 * i.e. NCARGS/3.  If you change GAVSIZ, remember to change INF too.
 */
#define	GAVSIZ	NCARGS / 3

/*
 * Variables for filename expansion
 */
CHAR	**gargv;			/* Pointer to the (stack) arglist */
short	gargc;				/* Number args in gargv */
short	gnleft;

/*
 * Variables for command expansion.
 */
CHAR	**pargv;			/* Pointer to the argv list space */
CHAR	*pargs;				/* Pointer to start current word */
short	pargc;				/* Count of arguments in pargv */
short	pnleft;				/* Number of chars left in pargs */
CHAR	*pargcp;			/* Current index into pargs */

/*
 * History list
 *
 * Each history list entry contains an embedded wordlist
 * from the scanner, a number for the event, and a reference count
 * to aid in discarding old entries.
 *
 * Essentially "invisible" entries are put on the history list
 * when history substitution includes modifiers, and thrown away
 * at the next discarding since their event numbers are very negative.
 */
struct	Hist {
	struct	wordent Hlex;
	int	Hnum;
	int	Href;
	struct	Hist *Hnext;
} Histlist;

struct	wordent	paraml;			/* Current lexical word list */
int	eventno;			/* Next events number */
int	lastev;				/* Last event reference (default) */

extern CHAR    HIST;                    /* history invocation character */
extern CHAR    HISTSUB;                 /* auto-substitute character */

CHAR	*Dfix1();
struct	varent *adrof(), *adrof1();
CHAR	**blkcat();
CHAR	**blkcpy();
CHAR	**blkend();
CHAR	**blkspl();
CHAR	**blkspl_spare();
CHAR	*calloc();
CHAR	*cname();
CHAR	**copyblk();
CHAR	**dobackp();
CHAR	*domod();
struct	wordent *dosub();
CHAR	*exp3();
CHAR	*exp3a();
CHAR	*exp4();
CHAR	*exp5();
CHAR	*exp6();
struct	Hist *enthist();
struct	Hist *findev();
struct	wordent *freenod();
char	*getenv();
CHAR	*getinx();
struct	varent *getvx();
struct	passwd *getpwnam();
struct	wordent *gethent();
struct	wordent *getsub();
CHAR	*getwd();
CHAR	*globone();
struct	biltins *isbfunc();
CHAR	**glob();
CHAR	*operate();
void	pintr();
void	pchild();
void	psigusr2();
CHAR	*putn();
CHAR	**saveblk();
CHAR	*savestr();
char	*strcat();
char	*strcpy();
CHAR	*strend();
CHAR	*strings();
char	*strspl();
CHAR	*strip();
CHAR	*subword();
struct	command *syntax();
struct	command *syn0();
struct	command *syn1();
struct	command *syn1a();
struct	command *syn1b();
struct	command *syn2();
struct	command *syn3();
int	tglob();
int	trim();
CHAR	*value(), *value1();
CHAR	*xhome();
CHAR	*xname();
CHAR	*xset();

#ifndef NONLS
/* Routines declared for 8/16 bit user */
CHAR 	*to_short();		/* convert char string to CHAR string */
char 	*to_char();		/* convert CHAR string to char string */
CHAR 	**blk_to_short();	/* convert block from char to CHAR */
char 	**blk_to_char();	/* convert block from CHAR to char */
CHAR 	*Strspl();		/* CHAR version of strspl in sh.misc.c */
CHAR	*Strcpy();		/* analgous to string(3) */
CHAR	*Strcat();
char	*savebyte();		/* like savestr except on bytes */
#else				
/* if no 8 bit processing, define routines to be the original */
#define	to_short
#define	to_char
#define	blk_to_short
#define	blk_to_char
#define savebyte
#define Atoi	atoi
#define Any	any
#define Strspl	strspl
#define Strlen	strlen
#define Strcpy	strcpy
#define Strncpy	strncpy
#define Strcat	strcat
#define Strcmp	strcmp
#define Strchr	strchr
#define Strrchr strrchr
#define Eq	eq
#endif

#define	NOSTR	((CHAR *) 0)


/*
 * setname is a macro to save space (see sh.err.c)
 */
char	*bname;
#define	setname(a)	bname = (a);

/*
  Two of the VFORK variables are used in doexec ().  They are Vcmd
  and Varg.  These are set before being used in doexec, however they
  are freed only in VFORK blocks of code in doexec.  The comment in
  doexec says that this is done for NLS.  It also says that "We vforked
  to get here, so let the paren process release storage malloced here,
  since the child won't need this data after a failed exec."  Seems like
  there are multiple paths to this code, not all through the VFORK area.
*/
char	*Vcmd;
char	**Varg;

#ifdef VFORK
CHAR	*Vsav;
CHAR	**Vav;
CHAR	*Vdp;
#endif

#ifdef VMUNIX
struct	vtimes zvms;
#endif

CHAR	**evalvec;
CHAR	*evalp;

struct	mesg {
	char	*iname;		/* name from /usr/include */
	char	*pname;		/* print name */
} mesg[];

#ifndef NONLS
/* NLS:  The following are some CHAR string constants */
CHAR 	nullstr[];
CHAR	CH_home[];
CHAR	CH_autologout[];
CHAR	CH_listpathnum[];
CHAR	CH_path[];
CHAR	CH_status[];
CHAR	CH_term[];
CHAR	CH_user[];
CHAR    CH_HOME[];
CHAR    CH_PATH[];
CHAR    CH_TERM[];
CHAR    CH_USER[];
CHAR    CH_argv[];
CHAR    CH_cdpath[];
CHAR    CH_child[];
CHAR    CH_cwd[];
CHAR    CH_echo[];
CHAR    CH_histchars[];
CHAR    CH_history[];
CHAR    CH_ignoreeof[];
CHAR    CH_mail[];
CHAR    CH_noclobber[];
CHAR    CH_noglob[];
CHAR    CH_nonomatch[];
CHAR    CH_prompt[];
CHAR    CH_savehist[];
CHAR    CH_shell[];
CHAR    CH_verbose[];
CHAR	CH_time[];
CHAR	CH_notify[];
CHAR	CH_zero[];
CHAR	CH_one[];
CHAR	CH_dot[];
CHAR	CH_slash[];
#ifdef SIGWINCH
CHAR	CH_columns[];
CHAR    CH_lines[];
#endif  /* SIGWINCH */
#if defined(DISKLESS) || defined(DUX)
CHAR	CH_hidden[];
#endif defined(DISKLESS) || defined(DUX)
#else 
#define nullstr		""
#define	CH_home		"home"
#define	CH_autologout	"autologout"
#define	CH_listpathnum	"listpathnum"
#define	CH_path		"path"
#define	CH_status	"status"
#define	CH_term		"term"
#define	CH_user		"user"
#define CH_HOME		"HOME"
#define CH_PATH		"PATH"
#define CH_TERM		"TERM"
#define CH_USER		"USER"
#define CH_argv		"argv"
#define CH_cdpath	"cdpath"
#define CH_child	"child"
#define CH_cwd		"cwd"
#define CH_echo		"echo"
#define CH_histchars	"histchars"
#define CH_history	"history"
#define CH_ignoreeof	"ignoreeof"
#define CH_mail		"mail"
#define CH_noclobber	"noclobber"
#define CH_noglob	"noglob"
#define CH_nonomatch	"nonomatch"
#define CH_prompt	"prompt"
#define CH_savehist	"savehist"
#define CH_shell	"shell"
#define CH_verbose	"verbose"
#define	CH_time		"time"
#define	CH_notify	"notify"
#define	CH_zero		"0"
#define	CH_one		"1"
#define	CH_dot		"."
#define	CH_slash	"/"
#ifdef SIGWINCH
#  define CH_columns	"COLUMNS"
#  define CH_lines	"LINES"
#endif  /* SIGWINCH */
#if defined(DISKLESS) || defined(DUX)
#define	CH_hidden	"hidden"
#endif defined(DISKLESS) || defined(DUX)
#endif

